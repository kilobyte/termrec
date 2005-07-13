#define _WIN32_IE 0x400
#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "config.h"
#include "utils.h"
#include "vt100.h"
#include "draw.h"
#include "stream.h"
#include "formats.h"
#include "timeline.h"
#include "synch.h"

#undef THREADED

HINSTANCE inst;
HWND wnd, termwnd, wndTB, ssProg, wndProg, ssSpeed, wndSpeed;
int tsx,tsy;
HANDLE pth;
int cancel;
int speed;
fpos_t lastp;
int codec;
int play_state;	// 0: not loaded, 1: paused, 2: playing, 3: waiting for input
struct timeval t0,tmax;
int progmax,progdiv,progval;

vt100 vt;
FILE *play_f;
HANDLE tev_sem, pth_sem;
CRITICAL_SECTION vt_mutex;

extern struct tty_event *tev_tail;	// FIXME: don't use tev_tail
extern int tev_done;
HANDLE timer;
int button_state;


#define MAXFILENAME 256
char filename[MAXFILENAME];

LRESULT APIENTRY MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY TermWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void do_replay();

void win32_init()
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
        show_error("RegisterClass"), exit(0);

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
        show_error("RegisterClass"), exit(0);

    draw_init();
    if (!(tev_sem=CreateSemaphore(0, 1, 1, 0)))
        show_error("CreateSemaphore"), exit(0);
    InitializeCriticalSection(&vt_mutex);
    if (!(timer=CreateWaitableTimer(0, 0, 0)))
        show_error("CreateWaitableTimer");
    cancel=0;
}


void create_window(int nCmdShow)
{
    wnd = CreateWindow(
        "MainWindow", "TermPlay",
        WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        80*chx, 25*chy, NULL, NULL, inst,
        NULL);
    
    if (nCmdShow==SW_SHOWDEFAULT)
        nCmdShow=SW_MAXIMIZE;
    ShowWindow(wnd, nCmdShow);
    UpdateWindow(wnd);
}

#define TERMBORDER 2

HWND create_term(HWND wnd)
{
    RECT rect;

    rect.left=0;
    rect.top=0;
    rect.right=chx*80+TERMBORDER;
    rect.bottom=chy*25+TERMBORDER;
    AdjustWindowRect(&rect, WS_CHILD|WS_BORDER, 0);
    termwnd = CreateWindow(
        "Term", 0,
        WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
        0, 0,
        rect.right-rect.left, rect.bottom-rect.top, wnd, NULL, inst,
        NULL);
    return termwnd;
}


int speeds[]={100,500,1000,2000,5000,10000,25000,100000};
//{250,500,1000,2000,4000,8000,16000};

int bcd(int a, int b)
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


void vulgar_fraction(char *buf, int x)
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


void set_button_state(int id, int onoff)
{
    TBBUTTONINFO bi;

    bi.cbSize=sizeof(TBBUTTONINFO);
    bi.dwMask=TBIF_COMMAND|TBIF_STATE;
    bi.idCommand=id;
    bi.fsState=TBSTATE_ENABLED|(onoff?TBSTATE_CHECKED:0);
    SendMessage(wndTB, TB_SETBUTTONINFO, (WPARAM)id, (LPARAM)&bi);
}

int get_button_state(int id)
{
    TBBUTTONINFO bi;

    bi.cbSize=sizeof(TBBUTTONINFO);
    bi.dwMask=TBIF_COMMAND|TBIF_STATE;
    bi.idCommand=id;
    SendMessage(wndTB, TB_GETBUTTONINFO, (WPARAM)id, (LPARAM)&bi);
    return !!(bi.fsState&TBSTATE_CHECKED);
}


int create_toolbar(HWND wnd)
{
    int height;
    TBBUTTON tbb[6];
    TBADDBITMAP tab;
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

    tab.hInst=inst;

    tab.nID=100;
    tbb[0].iBitmap = SendMessage(wndTB, TB_ADDBITMAP, 1, (LPARAM)&tab); 
    tbb[0].idCommand = 100; 
    tbb[0].fsState = TBSTATE_ENABLED; 
    tbb[0].fsStyle = TBSTYLE_BUTTON;
    tbb[0].dwData = 0; 
    tbb[0].iString = SendMessage(wndTB, TB_ADDSTRING, 0, (LPARAM)(LPSTR)"Open");
    
    tbb[1].iBitmap = 220; 
    tbb[1].fsState = 0; 
    tbb[1].fsStyle = TBSTYLE_SEP;
    tbb[1].dwData = 0;

    tab.nID=101;
    tbb[2].iBitmap = SendMessage(wndTB, TB_ADDBITMAP, 1, (LPARAM)&tab); 
    tbb[2].idCommand = 101; 
    tbb[2].fsState = 0;
    tbb[2].fsStyle = TBSTYLE_BUTTON;
    tbb[2].dwData = 0; 
    tbb[2].iString = SendMessage(wndTB, TB_ADDSTRING, 0, (LPARAM)(LPSTR)"Restart");
 
    tab.nID=102;
    tbb[3].iBitmap = SendMessage(wndTB, TB_ADDBITMAP, 1, (LPARAM)&tab); 
    tbb[3].idCommand = 102;
    tbb[3].fsState = 0; 
    tbb[3].fsStyle = TBSTYLE_CHECK;
    tbb[3].dwData = 0; 
    tbb[3].iString = SendMessage(wndTB, TB_ADDSTRING, 0, (LPARAM)(LPSTR)"Pause");

    tab.nID=103;
    tbb[4].iBitmap = SendMessage(wndTB, TB_ADDBITMAP, 1, (LPARAM)&tab); 
    tbb[4].idCommand = 103;
    tbb[4].fsState = 0; 
    tbb[4].fsStyle = TBSTYLE_CHECK; 
    tbb[4].dwData = 0; 
    tbb[4].iString = SendMessage(wndTB, TB_ADDSTRING, 0, (LPARAM)(LPSTR)"Play");
    
    tbb[5].iBitmap = 250; 
    tbb[5].fsState = 0; 
    tbb[5].fsStyle = TBSTYLE_SEP;
    tbb[5].dwData = 0;

    SendMessage(wndTB, TB_ADDBUTTONS, (WPARAM) ARRAYSIZE(tbb),
         (LPARAM) (LPTBBUTTON) &tbb);
    
    SendMessage(wndTB, TB_AUTOSIZE, 0, 0); 
    button_state=0;

    
    // The progress bar.
    
    SendMessage(wndTB, TB_GETITEMRECT, 1, (LPARAM)&rc);
    
    wndProg = CreateWindowEx( 
        0,                             // no extended styles 
        TRACKBAR_CLASS,                // class name 
        "Progress",            // title (caption) 
        WS_CHILD | WS_VISIBLE | WS_DISABLED |
        TBS_NOTICKS,  // style 
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
    
    
    SendMessage(wndTB, TB_GETITEMRECT, 5, (LPARAM)&rc);    
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


void set_toolbar_state(int onoff)
{
    SendMessage(wndTB, TB_ENABLEBUTTON, 101, onoff);
    SendMessage(wndTB, TB_ENABLEBUTTON, 102, onoff);
    SendMessage(wndTB, TB_ENABLEBUTTON, 103, onoff);
    EnableWindow(wndSpeed, onoff);
    EnableWindow(ssSpeed, onoff);
}


void set_buttons(int force)
{
    if (play_state==button_state && !force)
        return;
    set_button_state(102, play_state==1);
    set_button_state(103, play_state>1);
}


void set_prog_max()
{
    SendMessage(wndProg, TBM_SETRANGEMAX, 0, (LPARAM)progmax);
    EnableWindow(wndProg, 1);
}

void set_prog()
{
    int t=tr.tv_sec*(1000000/progdiv)+tr.tv_usec/progdiv;
    
    if (t!=progval)
    {
        progval=t;
        SendMessage(wndProg, TBM_SETPOS, 1, (LPARAM)t);
    }
}


void draw_size()
{
    RECT r;

    if (vt.sx!=tsx || vt.sy!=tsy)
    {
        r.left=0;
        r.top=0;
        r.right=(tsx=vt.sx)*chx+TERMBORDER;
        r.bottom=(tsy=vt.sy)*chy+TERMBORDER;
        AdjustWindowRect(&r, GetWindowLong(termwnd, GWL_STYLE), 0);
        SetWindowPos(termwnd, 0, 0, 0, r.right, r.bottom, SWP_NOACTIVATE|
            SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
    }
}


void timeline_lock()
{
    if (cancel)
    {
        tev_done=1;
        ExitThread(0);
    }
    WaitForSingleObject(tev_sem, INFINITE);
}

void timeline_unlock()
{
    ReleaseSemaphore(tev_sem, 1, 0);
}


void playfile(int arg)
{
    (*play[arg].play)(play_f);
    synch_print("\e[0mEnd of recording.", 20);
    
    {	// AXE ME
        struct timeval d;
        d.tv_sec=0;
        d.tv_usec=0;
        synch_wait(&d);
    }
}

void replay_start(int arg)
{
    timeline_clear();
    tr.tv_sec=tr.tv_usec=0;
    replay_seek();
    tev_done=0;
    cancel=0;
    tmax.tv_sec=tmax.tv_usec=0;
    progmax=0;
    progdiv=1000000;
    progval=-1;
    // TODO: re-enable threading
#ifdef THREADED
    pth=CreateThread(0, 0, (LPTHREAD_START_ROUTINE)playfile, (LPDWORD)arg, 0, 0);
#else
//    printf("Buffering: started.\n");
    playfile(arg);
//    printf("Buffering: done.\n");
    tev_done=1;
    tmax=tev_tail->t;
    if (tmax.tv_sec<100)
        progdiv=10000;
    else
        progdiv=1000000;
    progmax=tmax.tv_sec*(1000000/progdiv)+tmax.tv_usec/progdiv;
    set_prog_max();
    set_prog();
#endif
}

void replay_abort()
{
    cancel=1;
#ifdef THREADED
    WaitForSingleObject(pth, INFINITE);
#endif
}


int start_file(char *name)
{
    char buf[MAXFILENAME+20];
    
    if (play_f)
    {
        replay_abort();
        fclose(play_f);
        play_f=0;
        CancelWaitableTimer(timer);
        replay_pause();
        set_buttons(0);
    }
    play_f=stream_open(fopen(name, "rb"), name, "rb", decompressors, 0);
    if (!play_f)
        return 0;
    codec=codec_from_ext_play(name);
    if (codec==-1)
        codec=0;
    replay_start(codec);
    sprintf(buf, "Termplay: %s (%s)", filename, play[codec].name);
    SetWindowText(wnd, buf);
    set_toolbar_state(1);
    play_state=2;
    do_replay();
    return 1;
}


void open_file()
{
    char fn[MAXFILENAME];
    OPENFILENAME dlg;
    
    memset(&dlg, 0, sizeof(dlg));
    dlg.lStructSize=sizeof(dlg);
    dlg.hwndOwner=wnd;
    dlg.lpstrFilter="all known formats\000*.ttyrec;*.ttyrec.gz;*.ttyrec.bz2;*.nh;*.nh.gz;*.nh.bz2;*.txt;*.txt.gz;*.txt.bz2\000"
                    "ttyrec videos (*.ttyrec, *.ttyrec.gz, *.ttyrec.bz2)\000*.ttyrec;*.ttyrec.gz;*.ttyrec.bz2\000"
                    "nh-recorder videos (*.nh, *.nh.gz, *.nh.bz2)\000*.nh;*.nh.gz;*.nh.bz2\000"
                    "ANSI logs (*.txt, *.txt.gz, *.txt.bz2)\000*.txt;*.txt.gz;*.txt.bz2\000"
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


void replay_speed(int x)
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


void print_banner()
{
    int i;

    vt100_printf(&vt, "\e[?25l\e[36mTermplay v\e[1m"PACKAGE_VERSION"\e[0m\n");
    vt100_printf(&vt, "\e[33mTerminal size: \e[1m%d\e[21mx\e[1m%d\e[0m\n", vt.sx, vt.sy);
    vt100_printf(&vt, "\e[34;1m\e%%G\xd0\xa1\xd0\xb4\xd0\xb5\xd0\xbb\xd0\xb0\xd0\xbd\xd0\xbe by KiloByte (kilobyte@angband.pl)\e[0m\n");
    vt100_printf(&vt, "\nFeatures compiled in:\n");
    vt100_printf(&vt, "* UTF-8 (no CJK)\n");
    vt100_printf(&vt, "Compression plugins:\n");
    for (i=0;decompressors[i].name;i++)
        vt100_printf(&vt, "* %s\n", decompressors[i].name);
    vt100_printf(&vt, "Replay plugins:\n");
    for (i=0;play[i].name;i++)
        vt100_printf(&vt, "* %s\n", play[i].name);
}


void paint(HWND hwnd)
{
    HDC dc;
    PAINTSTRUCT paints;
    
    if (!(dc=BeginPaint(hwnd,&paints)))
        return;
    EnterCriticalSection(&vt_mutex);
    draw_vt(dc, paints.rcPaint.right, paints.rcPaint.bottom, &vt);
    draw_border(dc, &vt);
    LeaveCriticalSection(&vt_mutex);
    EndPaint(hwnd,&paints);
}



void do_replay()
{
    HDC dc;
    struct timeval delay;
    LARGE_INTEGER del;
    
again:
    if (play_state!=2)
    {
        set_buttons(0);
        CancelWaitableTimer(timer);
        return;
    }
    timeline_lock();
    EnterCriticalSection(&vt_mutex);
    replay_play(&delay);
    draw_size();
    dc=GetDC(termwnd);
    draw_vt(dc, chx*vt.sx, chy*vt.sy, &vt);
    ReleaseDC(termwnd, dc);
    LeaveCriticalSection(&vt_mutex);
    timeline_unlock();
    set_prog();
    if (play_state!=2)
    {
        set_buttons(1);	// finished
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


void speed_scrolled()
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
    GetWindowText(ssSpeed, buf, 32);
}


void adjust_speed(int dir)
{
    int pos=SendMessage(wndSpeed, TBM_GETPOS, 0, 0);
    SendMessage(wndSpeed, TBM_SETPOS, 1, pos+dir);
    speed_scrolled();
}


void redraw_term()
{
    HDC dc;
    
    EnterCriticalSection(&vt_mutex);
    draw_size();
    dc=GetDC(termwnd);
    draw_vt(dc, chx*vt.sx, chy*vt.sy, &vt);
    ReleaseDC(termwnd, dc);
    LeaveCriticalSection(&vt_mutex);
}


void prog_scrolled()
{
    uint64_t v;
    int oldstate=play_state;
    int pos=SendMessage(wndProg, TBM_GETPOS, 0, 0);
    
    timeline_lock();
    if (pos<0)
        pos=0;
    replay_pause();
    
    v=pos;
    v*=progdiv;
    tr.tv_sec=v/1000000;
    tr.tv_usec=v%1000000;
    replay_seek();
    timeline_unlock();
    play_state=oldstate;
    redraw_term();
    do_replay();
}


void adjust_pos(int d)
{
    int oldstate=play_state;
    
    if (play_state==-1)
        return;
    timeline_lock();
    replay_pause();
    tr.tv_sec+=d;
    if (tr.tv_sec<0)
        tr.tv_sec=tr.tv_usec=0;
    if (tcmp(tr, tmax)==1)
        tr=tmax;
    replay_seek();
    timeline_unlock();
    play_state=oldstate;
    redraw_term();
    set_prog();
    do_replay();
}


void get_def_size(int nx, int ny)
{
    RECT r;
    
    GetWindowRect(wndTB, &r);

    defsx=nx/chx;
    defsy=(ny+r.top-r.bottom)/chy;
}

void constrain_size(RECT *r)
{
    RECT wr,cr,tbr;
    int fx,fy;

    GetWindowRect(wnd,&wr);
    GetClientRect(wnd,&cr);
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


LRESULT APIENTRY MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_CREATE:
	    create_toolbar(hwnd);
            create_term(hwnd);

            return 0;
        
        case WM_SIZE:
            SendMessage(wndTB, TB_AUTOSIZE, 0, 0);
            get_def_size(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
            case 100:
                open_file();
                break;
            case 101:
            but_rewind:
                if (play_state==-1)
                    break;
                EnterCriticalSection(&vt_mutex);
                timeline_lock();
                replay_pause();
                tr.tv_sec=0;
                tr.tv_usec=0;
                set_prog();
                replay_seek();
                timeline_unlock();
                LeaveCriticalSection(&vt_mutex);
                play_state=1;
                replay_resume();
                do_replay();
                break;
            case 102:
                if (play_state==1)
                    goto but_unpause;
            but_pause:
                CancelWaitableTimer(timer);
                replay_pause();
                set_buttons(1);
                break;
            case 103:
                if (play_state>1)
                    goto but_pause;
            but_unpause:
                replay_resume();
                do_replay();
                set_buttons(1);
                break;
            }
            return 0;
        
        case WM_HSCROLL:
            if ((HANDLE)lParam==wndSpeed)
                speed_scrolled();
            else if ((HANDLE)lParam==wndProg)
                prog_scrolled();
            return 0;
        
        case WM_KEYDOWN:
            switch(wParam)
            {
            case VK_ADD:
            case 'F':
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
                DestroyWindow(wnd);
                break;
            case 'O':
                open_file();
                break;
            case VK_SPACE:
                switch(play_state)
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
    switch (uMsg) {
        case WM_PAINT:
            paint(hwnd);
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
    play_f=0;
    play_state=-1;
    tsx=tsy=0;
    
    vt100_init(&vt);
    
    strncpy(filename, lpCmdLine, MAXFILENAME-1);
    filename[MAXFILENAME-1]=0;

    defsx=80;
    defsy=25;
    
    create_window(nCmdShow);
    
    vt100_resize(&vt, defsx, defsy);
    print_banner();
    draw_size();
    UpdateWindow(wnd);

    timeline_init();

    if (*filename)
        if (!start_file(filename))
        {
            vt100_printf(&vt, "\n\e[41;1mFile not found: %s\e[0m\n", filename);
            *filename=0;
            redraw_term();
        }

    while(message_loop(&timer, 1)==0)
        do_replay();
    return 0;
    UNREFERENCED_PARAMETER(hPrevInstance);
}
