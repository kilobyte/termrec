#include <windows.h>
#include <stdio.h>
#define _WIN32_IE 0x400
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
HANDLE pth;
int cancel;
int speed;
fpos_t lastp;
int codec;
int play_state;	// 0: not loaded, 1: paused, 2: playing, 3: waiting for input
struct timeval t0,tmax;
struct tty_event *tev_cur;
int progmax,progdiv,progval;

vt100 vt;
FILE *play_f;
HANDLE tev_sem, pth_sem;
CRITICAL_SECTION vt_mutex;

extern struct tty_event tev_head, *tev_tail;	// TODO: don't use tev_tail
extern int tev_done;


#define MAXFILENAME 256
char filename[MAXFILENAME];

LRESULT APIENTRY MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY TermWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
    wc.hbrBackground = GetStockObject(GRAY_BRUSH);
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
    cancel=0;
}


void create_window(int nCmdShow)
{
    wnd = CreateWindow(
        "MainWindow", "TermPlay",
        WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, inst,
        NULL);
    
    ShowWindow(wnd, nCmdShow);
    UpdateWindow(wnd);
}


HWND create_term(HWND wnd)
{
    RECT rect;

    rect.left=0;
    rect.top=0;
    rect.right=chx*vt.sx;
    rect.bottom=chy*vt.sy;
    AdjustWindowRect(&rect, WS_CHILD|WS_BORDER, 0);
    termwnd = CreateWindow(
        "Term", 0,
        WS_CHILD|WS_BORDER|WS_CLIPSIBLINGS|WS_VISIBLE,
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
    lf.lfHeight=-13;
    strcpy(lf.lfFaceName, "Microsoft Sans Serif");
    font=CreateFontIndirect(&lf);

    // The toolbar.

    wndTB = CreateWindowEx(0, TOOLBARCLASSNAME, (LPSTR) NULL, 
         WS_CHILD|WS_VISIBLE|CCS_BOTTOM, 0, 0, 0, 0, wnd, 
         NULL, inst, NULL); 

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
    
    tbb[5].iBitmap = 280; 
    tbb[5].fsState = 0; 
    tbb[5].fsStyle = TBSTYLE_SEP;
    tbb[5].dwData = 0;

    SendMessage(wndTB, TB_ADDBUTTONS, (WPARAM) ARRAYSIZE(tbb),
         (LPARAM) (LPTBBUTTON) &tbb); 

    SendMessage(wndTB, TB_AUTOSIZE, 0, 0); 

    
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
        80, rc.bottom-rc.top-15,
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
        rc.left+95, rc.top+3, 
        rc.right-rc.left-100, rc.bottom-rc.top-3,
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
    GetWindowRect(termwnd, &rc);
    height+=rc.bottom-rc.top;
    rc.right-=rc.left;
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


void set_prog_max()
{
    SendMessage(wndProg, TBM_SETRANGEMAX, 0, (LPARAM)progmax);
    EnableWindow(wndProg, 1);
}

void set_prog()
{
    int t=tev_cur->t.tv_sec*(1000000/progdiv)+tev_cur->t.tv_usec/progdiv;
    
    if (t!=progval)
    {
        progval=t;
        SendMessage(wndProg, TBM_SETPOS, 1, (LPARAM)t);
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
        d.tv_usec=1;
        synch_wait(&d);
    }
}

void replay_start(int arg)
{
    timeline_clear();
    tev_cur=&tev_head;
    tev_done=0;
    cancel=0;
    gettimeofday(&t0, 0);
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
        set_button_state(103, 0);
    }
    play_f=stream_open(name, "rb");
    if (!play_f)
        return 0;
    replay_start(codec);
    set_button_state(102, 0);
    set_button_state(103, 1);
    sprintf(buf, "Termplay: %s (%s)", filename, play[codec].name);
    SetWindowText(wnd, buf);
    set_toolbar_state(1);
    play_state=2;
    return 1;
}


void open_file()
{
    char fn[MAXFILENAME];
    OPENFILENAME dlg;
    
    memset(&dlg, 0, sizeof(dlg));
    dlg.lStructSize=sizeof(dlg);
    dlg.hwndOwner=wnd;
    dlg.lpstrFilter="ttyrec videos (*.ttyrec, *.ttyrec.gz, *.ttyrec.bz2)\000*.ttyrec;*.ttyrec.gz;*.ttyrec.bz2\000"
                    "nh-recorder videos (*.nh, *.nh.gz, *.nh.bz2)\000*.nh;*.nh.gz;*.nh.bz2\000"
                    "ANSI logs (*.txt)\000*.txt\000"
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
}


int APIENTRY WinMain(HINSTANCE instance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;

    inst = instance;
    
    win32_init();
    
    speed=1000;
    
    vt100_init(&vt);
    vt100_resize(&vt, 80, 25);
    vt100_printf(&vt, "\e[36mTermplay v\e[1m"PACKAGE_VERSION"\e[0m\n");
    vt100_printf(&vt, "\e[33mTerminal size: \e[1m%d\e[21mx\e[1m%d\e[0m\n", vt.sx, vt.sy);
    timeline_init();

    codec=1;
    play_f=0;
    strncpy(filename, lpCmdLine, MAXFILENAME-1);
    filename[MAXFILENAME-1]=0;

    create_window(nCmdShow);
    SetTimer(wnd, 0, 200, 0);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
        UNREFERENCED_PARAMETER(hPrevInstance);
}


void paint(HWND hwnd)
{
    HDC dc;
    PAINTSTRUCT paints;
    
    if (!(dc=BeginPaint(hwnd,&paints)))
        return;
    EnterCriticalSection(&vt_mutex);
    draw_vt(dc, paints.rcPaint.right, paints.rcPaint.bottom, &vt);
    LeaveCriticalSection(&vt_mutex);
    EndPaint(hwnd,&paints);
}



void do_timer()
{
    HDC dc;
    struct timeval now;
    int dirty;
    
    if (play_state!=2)
        return;
    EnterCriticalSection(&vt_mutex);
    gettimeofday(&now, 0);
    timeline_lock();
    tsub(&now, &t0);
    dirty=0;
    while(tev_tail!=tev_cur && tcmp(tev_cur->t, now)==-1)
    {
        dirty=1;
        vt100_write(&vt, tev_cur->data, tev_cur->len);
        tev_cur=tev_cur->next;
    }
    timeline_unlock();
    if (!dirty)
    {
        LeaveCriticalSection(&vt_mutex);
        return;
    }
    dc=GetDC(termwnd);
    draw_vt(dc, chx*vt.sx, chy*vt.sy, &vt);
    LeaveCriticalSection(&vt_mutex);
    ReleaseDC(termwnd, dc);
    set_prog();
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
    speed=speeds[pos];
    if (get_button_state(103))
        replay_speed(speed);
    vulgar_fraction(buf+sprintf(buf, "Speed: x"), speed);
    SetWindowText(ssSpeed, buf);
    GetWindowText(ssSpeed, buf, 32);
}


LRESULT APIENTRY MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_CREATE:
            create_term(hwnd);
	    create_toolbar(hwnd);

            if (*filename)
                if (!start_file(filename))
                {
                    fprintf(stderr, "Can't open %s\n", filename);
                    return -1;
                }

            return 0;
        
        case WM_SIZE:
            SendMessage(wndTB, TB_AUTOSIZE, 0, 0); 
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        
        case WM_TIMER:
            do_timer();
            return 0;
        
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
            case 100:
                open_file();
                break;
            case 101:
                start_file(filename);
                break;
            case 102:
                if (!get_button_state(102))
                {
                    set_button_state(103, 1);
                    goto but_unpause;
                }
                else
                    set_button_state(103, 0);
            but_pause:
                replay_speed(0);
                break;
            case 103:
                if (!get_button_state(103))
                {
                    set_button_state(102, 1);
                    goto but_pause;
                }
                else
                    set_button_state(102, 0);
            but_unpause:
                replay_speed(speed);
                break;
            }
            return 0;
        
        case WM_HSCROLL:
            speed_scrolled();
            return 0;
        
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
