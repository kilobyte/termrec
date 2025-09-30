#define _WIN32_IE 0x400
#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "config.h"
#include <stdint.h>
#include "sys/utils.h"
#include "tty.h"
#include "draw.h"
#include "ttyrec.h"
#include "gettext.h"

#undef THREADED

#define BUT_OPEN        100
#define BUT_REWIND      101
#define BUT_PAUSE       102
#define BUT_PLAY        103
#define BUT_SELSTART    104
#define BUT_SELEND      105
#define BUT_EXPORT      106
#define BUT_FONT        200

HINSTANCE inst;
HWND wndMain, termwnd, wndTB, ssProg, wndProg, ssSpeed, wndSpeed;
int tsx,tsy;
HANDLE pth;
LOGFONT df;

ttyrec ttr;
ttyrec_frame tev_cur;
struct timeval t0, // adjusted wall time at t=0
               tr; // current point of replay
int tev_curlp;  // amount of data already played from the current block
int speed;
fpos_t lastp;
int play_state; // 0: not loaded, 1: paused, 2: playing, 3: waiting for input
struct timeval t0,tdate,tmax,selstart,selend;
int progmax,progdiv,progval;
int defsx, defsy;

tty vt;
int play_f;
const char *play_format, *play_filename;
HANDLE pth_sem;
CRITICAL_SECTION vt_mutex;

int tev_done;
HANDLE timer;
int button_state;

#define MAXFILENAME 256
char filename[MAXFILENAME];

LRESULT APIENTRY MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY TermWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void do_replay(void);
static void load_font(void);

static void win32_init(void)
{
    WNDCLASS wc;

    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC) MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = inst;
    wc.hIcon = LoadIcon(inst, MAKEINTRESOURCE(0));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    if (!(wc.hbrBackground = CreatePatternBrush(LoadBitmap(inst, "wood1"))))
        wc.hbrBackground=CreateSolidBrush(clWoodenDown);
    wc.lpszMenuName = 0;
    wc.lpszClassName = "MainWindow";

    if (!RegisterClass(&wc))
        die("RegisterClass");

    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC) TermWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = inst;
    wc.hIcon = 0;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground=0;
    wc.lpszMenuName = 0;
    wc.lpszClassName = "Term";

    if (!RegisterClass(&wc))
        die("RegisterClass");

    load_font();
    draw_init(&df);
    InitializeCriticalSection(&vt_mutex);
    if (!(timer=CreateWaitableTimer(0, 0, 0)))
        die("CreateWaitableTimer");
}


static void create_window(int nCmdShow)
{
    wndMain = CreateWindow(
        "MainWindow", "TermPlay",
        WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        80*chx, 25*chy, NULL, NULL, inst,
        NULL);

    if (nCmdShow==SW_SHOWDEFAULT)
        nCmdShow=SW_MAXIMIZE;
    ShowWindow(wndMain, nCmdShow);
    UpdateWindow(wndMain);
}

#define TERMBORDER 2

static HWND create_term(HWND wnd)
{
    RECT rect;

    rect.left=0;
    rect.top=0;
    rect.right=chx*80+TERMBORDER;
    rect.bottom=chy*25+TERMBORDER;
    AdjustWindowRect(&rect, WS_CHILD|WS_BORDER|WS_TABSTOP, 0);
    termwnd = CreateWindow(
        "Term", 0,
        WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
        0, 0,
        rect.right-rect.left, rect.bottom-rect.top, wnd, NULL, inst,
        NULL);
    return termwnd;
}


static void create_sysmenu(HWND wnd)
{
    HMENU sysmenu=GetSystemMenu(wnd, 0);
    AppendMenu(sysmenu, MF_SEPARATOR, 0, 0);
    AppendMenu(sysmenu, MF_STRING, BUT_FONT, "Choose &font");
}


static int speeds[]={100,500,1000,2000,5000,10000,25000,100000};
//{250,500,1000,2000,4000,8000,16000};

static int bcd(int a, int b)
{
    int c;

    while (a>1 && b>1)
    {
        if (a>b)
            c=a%b, a=b, b=c;
        else
            c=b%a, b=a, b=c;
    }
    if (a>b)
        return a;
    else
        return b;
}


static void vulgar_fraction(char *buf, int x)
{
    int d;
    d=bcd(x, 1000);
    x/=d;
    d=1000/d;
    if (d>1)
        sprintf(buf, "%d/%d", x, d);
    else
        sprintf(buf, "%d", x);
}


static void set_button_state(int id, int onoff)
{
    TBBUTTONINFO bi;

    bi.cbSize=sizeof(TBBUTTONINFO);
    bi.dwMask=TBIF_COMMAND|TBIF_STATE;
    bi.idCommand=id;
    bi.fsState=TBSTATE_ENABLED|(onoff?TBSTATE_CHECKED:0);
    SendMessage(wndTB, TB_SETBUTTONINFO, (WPARAM)id, (LPARAM)&bi);
}


static void create_button(TBBUTTON *tb, int id, const char *caption)
{
    TBADDBITMAP tab;

    tab.hInst=inst;
    tab.nID=id;
    tb->iBitmap = SendMessage(wndTB, TB_ADDBITMAP, 1, (LPARAM)&tab);
    tb->idCommand = id;
    tb->fsState = 0;
    tb->fsStyle = TBSTYLE_BUTTON;
    tb->dwData = 0;
    tb->iString = SendMessage(wndTB, TB_ADDSTRING, 0, (LPARAM)(LPSTR)caption);
}

static int create_toolbar(HWND wnd)
{
    int height;
    TBBUTTON tbb[9];
    RECT rc;
    LOGFONT lf;
    HFONT font;

    memset(&lf, 0, sizeof(lf));
    lf.lfQuality=ANTIALIASED_QUALITY;
    lf.lfHeight=-9;
    strcpy(lf.lfFaceName, "Microsoft Sans Serif");
    font=CreateFontIndirect(&lf);

    // The toolbar.

    wndTB = CreateWindowEx(0, TOOLBARCLASSNAME, (LPSTR) NULL,
         WS_CHILD|WS_VISIBLE|CCS_BOTTOM|WS_CLIPSIBLINGS|TBSTYLE_CUSTOMERASE,
         0, 0, 0, 0, wnd, NULL, inst, NULL);

    SendMessage(wndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);


    create_button(&tbb[0], BUT_OPEN, "Open");
    tbb[0].fsState = TBSTATE_ENABLED;

    tbb[1].iBitmap = 175;
    tbb[1].fsState = 0;
    tbb[1].fsStyle = TBSTYLE_SEP;
    tbb[1].dwData = 0;

    create_button(&tbb[2], BUT_REWIND, "Restart");
    create_button(&tbb[3], BUT_PAUSE, "Pause");
    tbb[3].fsStyle = TBSTYLE_CHECK;
    create_button(&tbb[4], BUT_PLAY, "Play");
    tbb[4].fsStyle = TBSTYLE_CHECK;

    tbb[5].iBitmap = 150;
    tbb[5].fsState = 0;
    tbb[5].fsStyle = TBSTYLE_SEP;
    tbb[5].dwData = 0;

    create_button(&tbb[6], BUT_SELSTART, "SelStart");
    create_button(&tbb[7], BUT_SELEND, "SelEnd");
    create_button(&tbb[8], BUT_EXPORT, "Export");

    SendMessage(wndTB, TB_ADDBUTTONS, (WPARAM) ARRAYSIZE(tbb),
         (LPARAM) (LPTBBUTTON) &tbb);

    SendMessage(wndTB, TB_AUTOSIZE, 0, 0);
    button_state=0;


    // The progress bar.

    SendMessage(wndTB, TB_GETITEMRECT, 5, (LPARAM)&rc);

    wndProg = CreateWindowEx(
        0,                             // no extended styles
        TRACKBAR_CLASS,                // class name
        "Progress",            // title (caption)
        WS_CHILD | WS_VISIBLE | WS_DISABLED |
        TBS_NOTICKS | TBS_ENABLESELRANGE | TBS_FIXEDLENGTH,  // style
        rc.left+5, rc.top+5,
        rc.right-rc.left-10, rc.bottom-rc.top-10,
        wnd,                       // parent window
        NULL,             // control identifier
        inst,                       // instance
        NULL                           // no WM_CREATE parameter
        );

//    SendMessage(wndProg, TBM_SETPAGESIZE,
//        0, (LPARAM) 4);                  // new page size

    SetParent(wndProg, wndTB);


    SendMessage(wndTB, TB_GETITEMRECT, 1, (LPARAM)&rc);
    ssSpeed = CreateWindowEx(0,
        "STATIC",
        "Speed: x1",
        WS_CHILD|WS_VISIBLE|SS_LEFTNOWORDWRAP|WS_DISABLED,
        rc.left+5, rc.top+10,
        55, rc.bottom-rc.top-15,
        wndTB,
        0,
        inst,
        NULL);
    SendMessage(ssSpeed, WM_SETFONT, (WPARAM)font, 0);

    // The speed bar.

    wndSpeed = CreateWindowEx(
        0,                             // no extended styles
        TRACKBAR_CLASS,                // class name
        "Speed",            // title (caption)
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS |WS_DISABLED,
        rc.left+70, rc.top+3,
        rc.right-rc.left-75, rc.bottom-rc.top-3,
        wnd,                       // parent window
        (HMENU)104,             // control identifier
        inst,                       // instance
        NULL                           // no WM_CREATE parameter
        );

    speed=1000;
    SendMessage(wndSpeed, TBM_SETRANGE, 0, MAKELONG(0,ARRAYSIZE(speeds)-1));
    SendMessage(wndSpeed, TBM_SETPOS, 1, 2);

    SetParent(wndSpeed, wndTB);

    SetFocus(wndProg);

    GetWindowRect(wndTB, &rc);
    height=rc.bottom-rc.top;
    height+=25*chy;
    rc.right-=rc.left;
    if (rc.right<80*chx)
        rc.right=80*chx;
    rc.bottom=height;
    rc.left=0;
    rc.top=0;
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, 0);
    SetWindowPos(wnd, 0, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
        SWP_NOMOVE|SWP_NOREPOSITION);

    return 1;
}


static void set_toolbar_state(int onoff)
{
    SendMessage(wndTB, TB_ENABLEBUTTON, BUT_REWIND, onoff);
    SendMessage(wndTB, TB_ENABLEBUTTON, BUT_PAUSE, onoff);
    SendMessage(wndTB, TB_ENABLEBUTTON, BUT_PLAY, onoff);
    EnableWindow(wndSpeed, onoff);
    EnableWindow(ssSpeed, onoff);
    SendMessage(wndTB, TB_ENABLEBUTTON, BUT_SELSTART, onoff);
    SendMessage(wndTB, TB_ENABLEBUTTON, BUT_SELEND, onoff);
    SendMessage(wndTB, TB_ENABLEBUTTON, BUT_EXPORT, onoff);
}


static void set_buttons(int force)
{
    if (play_state==button_state && !force)
        return;
    set_button_state(BUT_PAUSE, play_state==1);
    set_button_state(BUT_PLAY, play_state>1);
}


static void set_prog_max(void)
{
    EnableWindow(wndProg, 0);
    SendMessage(wndProg, TBM_CLEARSEL, 0, 0);
    SendMessage(wndProg, TBM_SETRANGEMAX, 0, (LPARAM)progmax);
    EnableWindow(wndProg, 1);
}


static void set_prog(void)
{
    int t=tr.tv_sec*(1000000/progdiv)+tr.tv_usec/progdiv;

    if (t!=progval)
    {
        progval=t;
        SendMessage(wndProg, TBM_SETPOS, 1, (LPARAM)t);
    }
}


static void get_pos(void)
{
    if (play_state==2)
    {
        gettimeofday(&tr, 0);
        tsub(tr, t0);
        tmul1000(tr, speed);
    }
}


static void set_prog_sel(void)
{
    int t1=selstart.tv_sec*(1000000/progdiv)+selstart.tv_usec/progdiv;
    int t2=selend.tv_sec*(1000000/progdiv)+selend.tv_usec/progdiv;

    SendMessage(wndProg, TBM_SETSEL, 1, (LPARAM)MAKELONG(t1,t2));
    SendMessage(wndTB, TB_ENABLEBUTTON, BUT_EXPORT, tcmp(selstart, selend)<0);
}


static void draw_size(void)
{
    if (vt->sx==tsx && vt->sy==tsy)
        return;

    RECT r;
    r.left=0;
    r.top=0;
    r.right=(tsx=vt->sx)*chx+TERMBORDER;
    r.bottom=(tsy=vt->sy)*chy+TERMBORDER;
    AdjustWindowRect(&r, GetWindowLong(termwnd, GWL_STYLE), 0);
    SetWindowPos(termwnd, 0, 0, 0, r.right, r.bottom, SWP_NOACTIVATE|
        SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
}


static void playfile(tty tev_vt)
{
    char buf[1024];

    ttr=ttyrec_load(play_f, play_format, play_filename, tev_vt);
    if (!ttr)
        return;
    ttyrec_add_frame(ttr, 0, buf, snprintf(buf, sizeof(buf), "\e[0m%s", _("End of recording.")));
}


static void replay_pause(void)
{
    switch (play_state)
    {
    case 0:
    default:
    case 1:
        break;
    case 2:
        gettimeofday(&tr, 0);
        tsub(tr, t0);
        tmul1000(tr, speed);
    case 3:
        play_state=1;
    }
}


static void replay_resume(void)
{
    struct timeval t;

    switch (play_state)
    {
    case 0:
    default:
    case 1:
        gettimeofday(&t0, 0);
        t=tr;
        tdiv1000(t, speed);
        tsub(t0, t);
        play_state=2;
        break;
    case 2:
    case 3:
        break;
    }
}


static int replay_play(struct timeval *delay)
{ // structures touched: tev, vt
    struct timeval tr1;
    ttyrec_frame fn;

    switch (play_state)
    {
    case 0:
    default:
    case 1:
        return 0;
    case 2:
        gettimeofday(&tr, 0);
        tsub(tr, t0);
        tmul1000(tr, speed);
        tr1=tr;
        tadd(tr1, tdate);
        if (tev_cur && tev_cur->len>tev_curlp)
        {
            tty_write(vt, tev_cur->data+tev_curlp, tev_cur->len-tev_curlp);
            tev_curlp=tev_cur->len;
        }
        while ((fn=ttyrec_next_frame(ttr, tev_cur)) && tcmp(fn->t, tr1)==-1)
        {
            tev_cur=fn;
            if (tev_cur->data)
                tty_write(vt, tev_cur->data, tev_cur->len);
            tev_curlp=tev_cur->len;
        }
        if ((fn=ttyrec_next_frame(ttr, tev_cur)))
        {
            *delay=fn->t;
            tsub(*delay, tdate);
            tsub(*delay, tr);
            tdiv1000(*delay, speed);
            return 1;
        }
        play_state=tev_done?0:3;
    case 3:
        return 0;
    }
}


// find the frame containing time "tr", update "t0"
static void replay_seek(void)
{
    struct timeval t;

    t=tr;
    tadd(t, tdate);
    tev_cur=ttyrec_seek(ttr, &t, &vt);
    tev_curlp=0;

    gettimeofday(&t0, 0);
    tdiv1000(tr, speed);
    tsub(t0, tr);
}


static void replay_start(void)
{
    tty tev_vt;
    ttyrec_frame tev_tail;
    struct timeval doomsday;

    ttyrec_free(ttr);
    tev_vt=tty_init(defsx, defsy, 1);
    tev_vt->cp437=1;
    tty_printf(tev_vt, "\e[36m");
    tty_printf(tev_vt, _("Termplay v%s\n\n"),
        "\e[36;1m"PACKAGE_VERSION"\e[0;36m");
    tty_printf(tev_vt, "\e[0m");

    tr.tv_sec=tr.tv_usec=0;
    tev_done=0;
    tmax.tv_sec=tmax.tv_usec=0;
    progmax=0;
    progdiv=1000000;
    progval=-1;
    // TODO: re-enable threading
#ifdef THREADED
    pth=CreateThread(0, 0, (LPTHREAD_START_ROUTINE)playfile, (LPDWORD)0, 0, 0);
#else
//    printf("Buffering: started.\n");
    playfile(tev_vt);
//    printf("Buffering: done.\n");
    tev_done=1;
    tev_cur=ttyrec_seek(ttr, 0, 0);
    tdate=tev_cur->t;
    replay_seek();
    doomsday.tv_sec=1ULL<<(sizeof(doomsday.tv_sec)*8-1)-1;
    doomsday.tv_usec=0;
    tev_tail=ttyrec_seek(ttr, &doomsday, 0);
    tmax=tev_tail->t;
    tsub(tmax, tdate);
    if (tmax.tv_sec<100)
        progdiv=10000;
    else
        progdiv=1000000;
    selstart.tv_sec=0;
    selstart.tv_usec=0;
    selend=tmax;
    progmax=tmax.tv_sec*(1000000/progdiv)+tmax.tv_usec/progdiv;
    set_prog_max();
    set_prog();
#endif
}

static void replay_abort(void)
{
#ifdef THREADED
    WaitForSingleObject(pth, INFINITE);
#endif
}


static int start_file(char *name)
{
    char buf[MAXFILENAME+20];
    int fd;

    if (play_f!=-1)
    {
        replay_abort();
        close(play_f);
        play_f=-1;
        CancelWaitableTimer(timer);
        replay_pause();
        set_buttons(0);
    }
    fd=open_stream(-1, name, SM_READ, 0);
    if (fd==-1)
        return 0;
    play_f=fd;
    play_format=ttyrec_r_find_format(0, name, "baudrate");
    play_filename=name;
    replay_start();
    snprintf(buf, sizeof(buf), "Termplay: %s (%s)", filename, play_format);
    SetWindowText(wndMain, buf);
    set_toolbar_state(1);
    play_state=2;
    do_replay();
    return 1;
}


static void open_file(void)
{
    char fn[MAXFILENAME];
    OPENFILENAME dlg;

    memset(&dlg, 0, sizeof(dlg));
    dlg.lStructSize=sizeof(dlg);
    dlg.hwndOwner=wndMain;
    dlg.lpstrFilter="all known formats\000"
                        "*.ttyrec;*.ttyrec.gz;*.ttyrec.bz2;*.ttyrec.xz;*.ttyrec.zst;"
                        "*.nh;*.nh.gz;*.nh.bz2;*.nh.xz;*.nh.zst;"
                        "*.dm2;"
                        "*.cast;*.cast.gz;*.cast.bz2;*.cast.xz;*.cast.zst;"
                        "*.txt;*.txt.gz;*.txt.bz2;*.txt.xz;*.txt.zst\000"
                    "ttyrec videos (*.ttyrec, *.ttyrec.[gz|bz2|xz|zst])\000"
                        "*.ttyrec;*.ttyrec.gz;*.ttyrec.bz2;*.ttyrec.xz;*.ttyrec.zst\000"
                    "nh-recorder videos (*.nh, *.nh.[gz|bz2|xz|zst])\000"
                        "*.nh;*.nh.gz;*.nh.bz2;*.nh.xz;*.nh.zst\000"
                    "DosRecorder videos (*.dm2)\000"
                        "*.dm2\000"
                    "asciicast videos (*.cast, *.cast.[gz|bz2|xz|zst])\000"
                        "*.cast;*.cast.gz;*.cast.bz2;*.cast.xz;*.cast.zst\000"
                    "ANSI logs (*.txt, *.txt.[gz|bz2|xz|zst])\000"
                        "*.txt;*.txt.gz;*.txt.bz2;*.txt.xz;*.txt.zst\000"
                    "all files\000*\000"
                    "\000\000";
    dlg.nFilterIndex=1;
    dlg.lpstrFile=fn;
    dlg.nMaxFile=MAXFILENAME;
    dlg.Flags=OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_LONGNAMES;
    *fn=0;

    if (!GetOpenFileName(&dlg))
        return;

    strncpy(filename, fn, MAXFILENAME);
    start_file(filename);
}


static void replay_speed(int x)
{
    if (play_state==-1)
    {
        speed=x;
        return;
    }
    replay_pause();
    speed=x;
    replay_resume();
    do_replay();
}


static void print_banner(void)
{
    int i;
    const char *pn;

    tty_printf(vt, "\e[?25l\e[36mTermplay v\e[1m"PACKAGE_VERSION"\e[0m\n");
    tty_printf(vt, "\e[33mTerminal size: \e[1m%d\e[21mx\e[1m%d\e[0m\n", vt->sx, vt->sy);
    tty_printf(vt, "\e[34;1m\e%%G\xd0\xa1\xd0\xb4\xd0\xb5\xd0\xbb\xd0\xb0\xd0\xbd\xd0\xbe by KiloByte (kilobyte@angband.pl)\e[0m\n");
    tty_printf(vt, "Compression plugins:\n");
#if (defined HAVE_LIBZ) || (SHIPPED_LIBZ)
    tty_printf(vt, "* gzip\n");
#endif
#if (defined HAVE_LIBBZ2) || (defined SHIPPED_LIBBZ2)
    tty_printf(vt, "* bzip2\n");
#endif
#if (defined HAVE_LIBLZMA) || (defined SHIPPED_LIBLZMA)
    tty_printf(vt, "* xz\n");
#endif
#if (defined HAVE_LIBZSTD) || (defined SHIPPED_LIBZSTD)
    tty_printf(vt, "* zstd\n");
#endif
    tty_printf(vt, "Replay plugins:\n");
    for (i=0;(pn=ttyrec_r_get_format_name(i));i++)
        if (strcmp(pn, "live"))
            tty_printf(vt, "* %s\n", pn);
}


static void paint(HWND hwnd)
{
    HDC dc;
    PAINTSTRUCT paints;

    if (!(dc=BeginPaint(hwnd,&paints)))
        return;
    EnterCriticalSection(&vt_mutex);
    draw_vt(dc, paints.rcPaint.right, paints.rcPaint.bottom, vt);
    draw_border(dc, vt);
    LeaveCriticalSection(&vt_mutex);
    EndPaint(hwnd,&paints);
}



static void do_replay(void)
{
    HDC dc;
    struct timeval delay = {};
    LARGE_INTEGER del;

again:
    if (play_state!=2)
    {
        set_buttons(0);
        CancelWaitableTimer(timer);
        return;
    }
    EnterCriticalSection(&vt_mutex);
    replay_play(&delay);
    draw_size();
    dc=GetDC(termwnd);
    draw_vt(dc, chx*vt->sx, chy*vt->sy, vt);
    ReleaseDC(termwnd, dc);
    LeaveCriticalSection(&vt_mutex);
    set_prog();
    if (play_state!=2)
    {
        set_buttons(1); // finished
        return;
    }
    del.QuadPart=delay.tv_sec;
    del.QuadPart*=1000000;
    del.QuadPart+=delay.tv_usec;
    if (del.QuadPart<=0)
        goto again;
    del.QuadPart*=-10;
    SetWaitableTimer(timer, &del, 0, 0, 0, 0);
    set_buttons(0);
}


static void speed_scrolled(void)
{
    char buf[32];
    int pos=SendMessage(wndSpeed, TBM_GETPOS, 0, 0);

    if (pos<0)
        pos=0;
    if (pos>=ARRAYSIZE(speeds))
        pos=ARRAYSIZE(speeds)-1;
    if (speed==speeds[pos])
        return;
    replay_speed(speeds[pos]);
    vulgar_fraction(buf+sprintf(buf, "Speed: x"), speed);
    SetWindowText(ssSpeed, buf);
    GetWindowText(ssSpeed, buf, sizeof(buf));
}


static void adjust_speed(int dir)
{
    int pos=SendMessage(wndSpeed, TBM_GETPOS, 0, 0);
    SendMessage(wndSpeed, TBM_SETPOS, 1, pos+dir);
    speed_scrolled();
}


static void redraw_term(void)
{
    HDC dc;

    EnterCriticalSection(&vt_mutex);
    draw_size();
    dc=GetDC(termwnd);
    draw_vt(dc, chx*vt->sx, chy*vt->sy, vt);
    ReleaseDC(termwnd, dc);
    LeaveCriticalSection(&vt_mutex);
}


static void prog_scrolled(void)
{
    uint64_t v;
    int oldstate=play_state;
    int pos=SendMessage(wndProg, TBM_GETPOS, 0, 0);

    if (pos<0)
        pos=0;
    replay_pause();

    v=pos;
    v*=progdiv;
    tr.tv_sec=v/1000000;
    tr.tv_usec=v%1000000;
    replay_seek();
    play_state=oldstate;
    redraw_term();
    do_replay();
}


static void adjust_pos(int d)
{
    int oldstate=play_state;

    if (play_state==-1)
        return;
    replay_pause();
    tr.tv_sec+=d;
    if (tr.tv_sec<0)
        tr.tv_sec=tr.tv_usec=0;
    if (tcmp(tr, tmax)==1)
        tr=tmax;
    replay_seek();
    play_state=oldstate;
    redraw_term();
    set_prog();
    do_replay();
}


static void get_def_size(int nx, int ny)
{
    RECT r;

    GetWindowRect(wndTB, &r);

    defsx=nx/chx;
    defsy=(ny+r.top-r.bottom)/chy;
}

static void constrain_size(RECT *r)
{
    RECT wr,cr,tbr;
    int fx,fy;

    GetWindowRect(wndMain,&wr);
    GetClientRect(wndMain,&cr);
    GetWindowRect(wndTB, &tbr);
    fx=(wr.right-wr.left)-(cr.right-cr.left);
    fy=(wr.bottom-wr.top)-(cr.bottom-cr.top);

    fx+=80*chx;
    fy+=25*chy+tbr.bottom-tbr.top;
    if (r->right-r->left<fx)
        r->right=r->left+fx;
    if (r->bottom-r->top<fy)
        r->bottom=r->top+fy;
}


static void export_file(void)
{
    char fn[MAXFILENAME],errmsg[MAXFILENAME+20+128];
    OPENFILENAME dlg;
    int record_f;
    const char *format;
    struct timeval sel1, sel2;

    memset(&dlg, 0, sizeof(dlg));
    dlg.lStructSize=sizeof(dlg);
    dlg.hwndOwner=wndMain;
    dlg.lpstrFilter=
                    "ttyrec videos (*.ttyrec, *.ttyrec.[gz|bz2|xz|zst])\000*.ttyrec;*.ttyrec.gz;*.ttyrec.bz2;*.ttyrec.xz;*.ttyrec.zst\000"
                    "nh-recorder videos (*.nh, *.nh.[gz|bz2|xz|zst])\000*.nh;*.nh.gz;*.nh.bz2;*.nh.xz;*.nh.zst\000"
                    "asciinema videos (*.cast, *.cast.[gz|bz2|xz|zst])\000*.cast;*.cast.gz;*.cast.bz2;*.cast.xz;*.cast.zst\000"
                    "ANSI logs (*.txt, *.txt.[gz|bz2|xz|zst])\000*.txt;*.txt.gz;*.txt.bz2;*.txt.xz;*.txt.zst\000"
                    "all files\000*\000"
                    "\000\000";
    dlg.nFilterIndex=1;
    dlg.lpstrFile=fn;
    dlg.nMaxFile=MAXFILENAME;
    dlg.Flags=OFN_HIDEREADONLY|OFN_LONGNAMES;
    dlg.lpstrDefExt="ttyrec.bz2";
    *fn=0;

    if (!GetSaveFileName(&dlg))
        return;

    format=ttyrec_w_find_format(0, fn, "ansi");
    if ((record_f=open(fn, O_WRONLY|O_CREAT|O_TRUNC, 0666))==-1)
    {
        sprintf(errmsg, "Can't write to %s: %s", fn, strerror(errno));
        MessageBox(wndMain, errmsg, "Write error", MB_ICONERROR);
        return;
    }
    record_f=open_stream(record_f, fn, SM_WRITE, 0);
    sel1=selstart;
    tadd(sel1, tdate);
    sel2=selend;
    tadd(sel2, tdate);
    ttyrec_save(ttr, record_f, format, filename, &sel1, &sel2);
}


void load_font(void)
{
    HKEY reg;
    int i;
    DWORD len;

    memset(&df, 0, sizeof(LOGFONT));
    df.lfHeight=16;
    strcpy(df.lfFaceName, "Courier New");

    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\1KB\\termrec", 0, KEY_READ, &reg))
        return;
    len=LF_FACESIZE;
    RegQueryValueEx(reg, "FontName", 0, 0, (BYTE*)df.lfFaceName, &len);
    len=sizeof(i);
    if (!RegQueryValueEx(reg, "FontHeight", 0, 0, (BYTE*)&i, &len))
        df.lfHeight=i;
    len=sizeof(i);
    if (!RegQueryValueEx(reg, "FontWeight", 0, 0, (BYTE*)&i, &len))
        df.lfWeight=i;
    len=sizeof(i);
    if (!RegQueryValueEx(reg, "FontItalic", 0, 0, (BYTE*)&i, &len))
        df.lfItalic=i;
    RegCloseKey(reg);
}


static void save_font(void)
{
    HKEY reg;
    int i;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\1KB\\termrec", 0, 0, 0, KEY_WRITE, 0, &reg, 0))
        return;
    RegSetValueEx(reg, "FontName", 0, REG_SZ, (BYTE*)df.lfFaceName, strlen(df.lfFaceName)+1);
    i=df.lfHeight;
    RegSetValueEx(reg, "FontHeight", 0, REG_DWORD, (BYTE*)&i, sizeof(i));
    i=df.lfWeight;
    RegSetValueEx(reg, "FontWeight", 0, REG_DWORD, (BYTE*)&i, sizeof(i));
    i=df.lfItalic;
    RegSetValueEx(reg, "FontItalic", 0, REG_DWORD, (BYTE*)&i, sizeof(i));
    RegCloseKey(reg);
}


static void choose_font(void)
{
    CHOOSEFONT cf;

    memset(&cf, 0, sizeof(CHOOSEFONT));
    cf.lStructSize=sizeof(CHOOSEFONT);
    cf.hwndOwner=wndMain;
    cf.Flags=CF_FIXEDPITCHONLY|CF_NOSCRIPTSEL|CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT;
    cf.lpLogFont=&df;
    if (!ChooseFont(&cf))
        return;
    save_font();
    draw_free();
    draw_init(&df);
    InvalidateRect(termwnd, 0, 1);
}


LRESULT APIENTRY MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            create_toolbar(hwnd);
            create_term(hwnd);
            create_sysmenu(hwnd);

            return 0;

        case WM_SIZE:
            SendMessage(wndTB, TB_AUTOSIZE, 0, 0);
            get_def_size(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case BUT_OPEN:
                open_file();
                break;
            case BUT_REWIND:
            but_rewind:
                if (play_state==-1)
                    break;
                EnterCriticalSection(&vt_mutex);
                replay_pause();
                tr.tv_sec=0;
                tr.tv_usec=0;
                set_prog();
                replay_seek();
                LeaveCriticalSection(&vt_mutex);
                play_state=1;
                replay_resume();
                do_replay();
                break;
            case BUT_PAUSE:
                if (play_state==1)
                    goto but_unpause;
            but_pause:
                CancelWaitableTimer(timer);
                replay_pause();
                set_buttons(1);
                break;
            case BUT_PLAY:
                if (play_state>1)
                    goto but_pause;
            but_unpause:
                replay_resume();
                do_replay();
                set_buttons(1);
                break;
            case BUT_SELSTART:
                if (play_state==-1)
                    break;
                get_pos();
                selstart=tr;
                set_prog_sel();
                break;
            case BUT_SELEND:
                if (play_state==-1)
                    break;
                selend=tr;
                set_prog_sel();
                break;
            case BUT_EXPORT:
                if (play_state==-1)
                    break;
                if (tcmp(selstart, selend)>=0)
                    break;
                export_file();
                break;
            }
            return 0;

        case WM_SYSCOMMAND:
            switch (LOWORD(wParam))
            {
            default:
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            case BUT_FONT:
                choose_font();
            }
            return 0;

        case WM_HSCROLL:
            if ((HANDLE)lParam==wndSpeed)
                speed_scrolled();
            else if ((HANDLE)lParam==wndProg)
                prog_scrolled();
            return 0;

        case WM_KEYDOWN:
            switch (wParam)
            {
            case 'F':
                if (GetKeyState(VK_CONTROL)&0x8000)
                {
                    choose_font();
                    break;
                }
            case VK_ADD:
                adjust_speed(+1);
                break;
            case VK_SUBTRACT:
            case 'S':
                adjust_speed(-1);
                break;
            case '1':
                SendMessage(wndSpeed, TBM_SETPOS, 1, 2);
                speed_scrolled();
                break;
            case 'Q':
                DestroyWindow(wndMain);
                break;
            case 'O':
                open_file();
                break;
            case VK_SPACE:
                switch (play_state)
                {
                case -1:
                    open_file();
                    break;
                case 0:
                    play_state=2;
                    goto but_rewind;
                case 1:
                    goto but_unpause;
                case 2:
                case 3:
                    goto but_pause;
                }
                break;
            case 'R':
                goto but_rewind;
                break;
            case VK_RIGHT:
                adjust_pos(+10);
                break;
            case VK_LEFT:
                adjust_pos(-10);
                break;
            case VK_UP:
                adjust_pos(+60);
                break;
            case VK_DOWN:
                adjust_pos(-60);
                break;
            case VK_PRIOR:
                adjust_pos(+600);
                break;
            case VK_NEXT:
                adjust_pos(-600);
                break;
            }
            return 0;

        case WM_SIZING:
            constrain_size((LPRECT)lParam);
            return 1;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


LRESULT APIENTRY TermWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_PAINT:
            paint(hwnd);
            return 0;

        case WM_LBUTTONDOWN:
            SetFocus(wndMain);
            return 0;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


int APIENTRY WinMain(HINSTANCE instance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    inst = instance;

    win32_init();

    speed=1000;
    play_f=-1;
    play_state=-1;
    tsx=tsy=0;

    defsx=80;
    defsy=25;

    vt=tty_init(defsx, defsy, 1);
    vt->cp437=1;

    if (*lpCmdLine=='"')        // FIXME: proper parsing
    {
        strncpy(filename, lpCmdLine+1, MAXFILENAME-1);
        filename[strlen(filename)-1]=0;
    }
    else
        strncpy(filename, lpCmdLine, MAXFILENAME-1);
    filename[MAXFILENAME-1]=0;

    create_window(nCmdShow);

    print_banner();
    draw_size();
    UpdateWindow(wndMain);

    ttr=0;

    if (*filename)
        if (!start_file(filename))
        {
            tty_printf(vt, "\n\e[41;1mFile not found: %s\e[0m\n", filename);
            *filename=0;
            redraw_term();
        }

    while (message_loop(&timer, 1)==0)
        do_replay();
    return 0;
    UNREFERENCED_PARAMETER(hPrevInstance);
}
