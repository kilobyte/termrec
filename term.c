#include <windows.h>
#include <stdio.h>
#define _WIN32_IE 0x400
#include <commctrl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "vt100.h"
#include "draw.h"
#include "arch.h"
#include "stream_in.h"

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

HINSTANCE inst;
HWND wnd, termwnd, wndTB, ssProg, wndProg, ssSpeed, wndSpeed;
HANDLE pth;
HANDLE changes;
int cancel;
extern int speed;
int Speed;
fpos_t lastp;

vt100 vt;
FILE *f;
CRITICAL_SECTION vt_mutex;

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
        exit(0);

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
        exit(0);

    draw_init();
    changes=CreateMutex(0,1,0);
    cancel=0;
}


void create_window(int nCmdShow)
{
    wnd = CreateWindow(
        "MainWindow", "Szkolniki",
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


int get_size(FILE *f)
{
    struct stat s;
    if (fstat(fileno(f), &s)==-1)
        return -1;
    return s.st_size;
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


int create_toolbar(HWND wnd)
{
    int height;
    TBBUTTON tbb[5];
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

    tbb[0].iBitmap = 220; 
    tbb[0].fsState = TBSTATE_ENABLED; 
    tbb[0].fsStyle = TBSTYLE_SEP;
    tbb[0].dwData = 0;

    tab.hInst=inst;
    tab.nID=101;
    tbb[1].iBitmap = SendMessage(wndTB, TB_ADDBITMAP, 1, (LPARAM)&tab); 
    tbb[1].idCommand = 101; 
    tbb[1].fsState = TBSTATE_ENABLED; 
    tbb[1].fsStyle = TBSTYLE_BUTTON;
    tbb[1].dwData = 0; 
    tbb[1].iString = SendMessage(wndTB, TB_ADDSTRING, 0, (LPARAM)(LPSTR)"Restart");
 
    tab.nID=102;
    tbb[2].iBitmap = SendMessage(wndTB, TB_ADDBITMAP, 1, (LPARAM)&tab); 
    tbb[2].idCommand = 102;
    tbb[2].fsState = TBSTATE_ENABLED; 
    tbb[2].fsStyle = TBSTYLE_CHECK;
    tbb[2].dwData = 0; 
    tbb[2].iString = SendMessage(wndTB, TB_ADDSTRING, 0, (LPARAM)(LPSTR)"Pause");

    tab.nID=103;
    tbb[3].iBitmap = SendMessage(wndTB, TB_ADDBITMAP, 1, (LPARAM)&tab); 
    tbb[3].idCommand = 103;
    tbb[3].fsState = TBSTATE_ENABLED; 
    tbb[3].fsStyle = TBSTYLE_CHECK; 
    tbb[3].dwData = 0; 
    tbb[3].iString = SendMessage(wndTB, TB_ADDSTRING, 0, (LPARAM)(LPSTR)"Play");
    
    tbb[4].iBitmap = 280; 
    tbb[4].fsState = TBSTATE_ENABLED; 
    tbb[4].fsStyle = TBSTYLE_SEP;
    tbb[4].dwData = 0;

    SendMessage(wndTB, TB_ADDBUTTONS, (WPARAM) ARRAYSIZE(tbb),
         (LPARAM) (LPTBBUTTON) &tbb); 

    SendMessage(wndTB, TB_AUTOSIZE, 0, 0); 

    
    // The progress bar.
    
    SendMessage(wndTB, TB_GETITEMRECT, 0, (LPARAM)&rc);
    
    wndProg = CreateWindowEx( 
        0,                             // no extended styles 
        TRACKBAR_CLASS,                // class name 
        "Progress",            // title (caption) 
        WS_CHILD | WS_VISIBLE | 
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
    
    
    SendMessage(wndTB, TB_GETITEMRECT, 4, (LPARAM)&rc);    
    ssSpeed = CreateWindowEx(0,
        "STATIC",
        "Speed: x1",
        WS_CHILD|WS_VISIBLE|SS_LEFTNOWORDWRAP,
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
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
        rc.left+95, rc.top+3, 
        rc.right-rc.left-100, rc.bottom-rc.top-3,
        wnd,                       // parent window 
        (HMENU)104,             // control identifier 
        inst,                       // instance 
        NULL                           // no WM_CREATE parameter 
        ); 

    Speed=1000;
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


int APIENTRY WinMain(HINSTANCE instance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;

    inst = instance;
    
    win32_init();
    
    speed=1000;
    
    InitializeCriticalSection(&vt_mutex);
    vt100_init(&vt);
    vt100_resize(&vt, 80, 25);
    vt100_printf(&vt, "\e[36mTermplay v\e[1m0\e[21m.\e[1m001\e[0m\n");
    vt100_printf(&vt, "\e[33mTerminal size: \e[1m%d\e[21mx\e[1m%d\e[0m\n", vt.sx, vt.sy);

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
    fpos_t cp;
    
    dc=GetDC(termwnd);
    EnterCriticalSection(&vt_mutex);
    draw_vt(dc, chx*vt.sx, chy*vt.sy, &vt);
    LeaveCriticalSection(&vt_mutex);
    ReleaseDC(termwnd, dc);
    fgetpos(f, &cp);
    if (lastp!=cp)
    {
        lastp=cp;
        SendMessage(wndProg, TBM_SETPOS, 1, (LPARAM)cp/1024);
    }
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


void speed_scrolled()
{
    char buf[32];
    int pos=SendMessage(wndSpeed, TBM_GETPOS, 0, 0);
    
    if (pos<0)
        pos=0;
    if (pos>=ARRAYSIZE(speeds))
        pos=ARRAYSIZE(speeds)-1;
    if (Speed==speeds[pos])
        return;
    Speed=speeds[pos];
    if (get_button_state(103))
        synch_speed(Speed);
    vulgar_fraction(buf+sprintf(buf, "Speed: x"), Speed);
    SetWindowText(ssSpeed, buf);
    GetWindowText(ssSpeed, buf, 32);
}


LRESULT APIENTRY MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_CREATE:
            create_term(hwnd);
	    create_toolbar(hwnd);
            f=stream_open("test.bz2");
            
            if (!f)
            {
                fprintf(stderr, "Can't open test.bz2\n");
                return -1;
            }
                
            SendMessage(wndProg, TBM_SETRANGEMAX,
                (WPARAM) TRUE,
                (LPARAM) get_size(f)/1024);
            
            synch_start(0, 0);
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
            case 102:
                if (!get_button_state(102))
                {
                    set_button_state(103, 1);
                    goto but_unpause;
                }
                else
                    set_button_state(103, 0);
            but_pause:
                synch_speed(0);
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
                synch_speed(Speed);
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
