#include <windows.h>
#include <stdio.h>
#include "vt100.h"
#include "draw.h"

HINSTANCE inst;
HWND wnd;

vt100 vt;
FILE *f;
CRITICAL_SECTION vt_mutex;

LRESULT APIENTRY MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LPTHREAD_START_ROUTINE playfile();

void win32_init()
{
    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC) MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = inst;
    wc.hIcon = LoadIcon(inst, MAKEINTRESOURCE(0));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground=0;
//    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = "DeathToUselessButNeededParams";

    if (!RegisterClass(&wc))
        exit(0);

}


void create_window(int nCmdShow)
{
    wnd = CreateWindow(
        "DeathToUselessButNeededParams", "Szkolniki",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, inst,
        NULL);

    ShowWindow(wnd, nCmdShow);
    UpdateWindow(wnd);
}


int APIENTRY WinMain(HINSTANCE instance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;

    inst = instance;
    
    win32_init();
    
    InitializeCriticalSection(&vt_mutex);
    vt100_init(&vt);
    vt100_resize(&vt, 80, 25);
    vt100_printf(&vt, "Ala ma \e[36mk\e[34ma\e[31mc\e[32ma\e[0m\n");

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
    
    dc=GetDC(wnd);
    EnterCriticalSection(&vt_mutex);
    draw_vt(dc, chx*vt.sx, chy*vt.sy, &vt);
    LeaveCriticalSection(&vt_mutex);
    ReleaseDC(wnd, dc);
}


LRESULT APIENTRY MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DWORD foo;

    switch (uMsg) {
        case WM_CREATE:
            f=fopen("test.txt", "rb");
            if (!f)
                return -1;
            draw_init();
            CreateThread(0, 0, (LPTHREAD_START_ROUTINE)playfile, 0, 0, &foo);
            return 0;
    

        case WM_PAINT:
            paint(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        
        case WM_TIMER:
            do_timer();
            return 0;
        
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
