#define WINVER 0x501
#define AC_SRC_ALPHA 1
#define ULW_ALPHA 2
#define ULW_COLORKEY 1
#define ULW_OPAQUE 0

#include <windows.h>
#include <stdio.h>
#include "vt100.h"
#include "trend.h"
#include "rend.h"
#include "readpng.h"

HINSTANCE inst;
HWND wnd;
LOGFONT lf;
int layered,mode_new,mode_cur;

vt100 vt;

HBITMAP bmp;
pixel *pic,*bgpic,*ovpic;
int psx,psy;
int bgsx,bgsy;
int ovsx,ovsy;

unsigned char multable[256][256]; // AXE ME

typedef BOOL (WINAPI* SLWA)(HWND,COLORREF,BYTE,DWORD);
typedef BOOL (WINAPI* ULW)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD);

SLWA pSetLayeredWindowAttributes;
ULW pUpdateLayeredWindow;

LRESULT APIENTRY MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void redraw();

void win32_init()
{
    WNDCLASS wc;
    HINSTANCE dll;
    int x,y;

    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC) MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = inst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground=0;
//    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = "DieDieDie!";

    if (!RegisterClass(&wc))
        exit(0);

    pSetLayeredWindowAttributes=0;
    pUpdateLayeredWindow=0;
    if ((dll=LoadLibrary("user32")))
    {
        pSetLayeredWindowAttributes=(SLWA)GetProcAddress(dll,"SetLayeredWindowAttributes");
        pUpdateLayeredWindow=(ULW)GetProcAddress(dll,"UpdateLayeredWindow");
    }
    for(x=0;x<256;x++)
        for(y=0;y<256;y++)
            multable[x][y]=x*y/255;
}

void create_window(int nCmdShow)
{
    switch(layered=mode_cur=mode_new)
    {
    case 1:
        if (!pUpdateLayeredWindow)
            layered=0;
        break;
    case 2:
        if (!pSetLayeredWindowAttributes)
            layered=0;
    }
    mode_new=-1;

    wnd = CreateWindowEx(
        layered?WS_EX_LAYERED:0,"DieDieDie!", "Szkolniki",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, NULL,
#ifdef SYSMENU
        0,
#else
        CreateMenu(),
#endif
        inst,
        NULL);

    if (wnd == NULL)
    {
        fprintf(stderr, "Couldn't create window.\n");
        exit(0);
    }
    if (layered==2)
    {
        (*pSetLayeredWindowAttributes)(wnd, 0, 0xc0, ULW_ALPHA);
        layered=0;
    }

    psx=psy=0;
    
    ShowWindow(wnd, nCmdShow);
    if (layered)
        redraw();
    else
        UpdateWindow(wnd);
}

#define GETMENU(wnd) GetMenu(wnd)

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;

    mode_new=0;

    inst = instance;
    
    win32_init();
    
    if ((bgpic=read_png("background.png",&bgsx,&bgsy)))
        printf("Background: loaded.\n");
    else
        printf("Background: couldn't load.\n");
    if ((ovpic=read_png("overlay.png",&ovsx,&ovsy)))
        printf("Overlay: loaded.\n");
    else
        printf("Overlay: couldn't load.\n");

    memset(&lf,0,sizeof(LOGFONT));
    lf.lfHeight=20;
    lf.lfWidth=10;
    lf.lfPitchAndFamily=FIXED_PITCH;
    lf.lfQuality=ANTIALIASED_QUALITY;
    lf.lfOutPrecision=OUT_TT_ONLY_PRECIS;

    vt100_init(&vt);

    create_window(nCmdShow);

    // Start the main message loop.

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
        UNREFERENCED_PARAMETER(hPrevInstance);
}

#define MID_CHANGEFONT 1
#define MID_CTEST 2
#define MID_SPAM 3
#define MID_TEST 4
#define MID_PNGFONT 5
#define MID_REND 8 /* +bits */

void create_font(HDC dc)
{
    HBITMAP bmp;
    BITMAPINFO bmi;
    HDC memdc;
    HBITMAP oldbmp;
    RECT rect;
    HFONT font,oldfont;
    SIZE sf;
    unsigned char ch[2];
    int scrdc=0;
    pixel *pic;
    
    if (!dc)
        dc=GetDC(0), scrdc=1;
    memdc=CreateCompatibleDC(dc);
    lf.lfQuality=ANTIALIASED_QUALITY/*5*/;
    oldfont=SelectObject(memdc,font=CreateFontIndirect(&lf));
    GetTextExtentPoint32(memdc,"W",1,&sf);
    init_trend(sf.cx, sf.cy);
    memset(&bmi.bmiHeader,0,sizeof(BITMAPINFOHEADER));
    bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth=chx;
    bmi.bmiHeader.biHeight=-chy;
    bmi.bmiHeader.biPlanes=1;
    bmi.bmiHeader.biBitCount=32;
    bmi.bmiHeader.biCompression=BI_RGB;
    printf("Creating bmp.\n");
    bmp=CreateDIBSection(0,&bmi,DIB_RGB_COLORS,(void *)&pic,0,0);
    if (!bmp)
    {
        fprintf(stderr,"CreateDIBSection failed.\n");
        exit(0);
    }
    oldbmp=SelectObject(memdc,bmp);
    rect.left=0;
    rect.top=0;
    rect.right=chx;
    rect.bottom=chy;
    ch[1]=0;
    
    SetTextColor(memdc, 0xffffff);
    SetBkColor(memdc, 0);
    
    for(ch[0]=0;++ch[0];)
    {
        fflush(stdout);
        TextOut(memdc, 0, 0, ch, 1);
        GdiFlush();
        learn_char(pic, ch[0], chx, chy, 0, 0);
    }
    SelectObject(memdc,oldbmp);
    printf("Deleting bmp.\n");
    DeleteObject(bmp);
    SelectObject(memdc,oldfont);
    DeleteObject(font);
    DeleteDC(memdc);
    if (scrdc)
        ReleaseDC(0, dc);
}

void resized(int nsx, int nsy)
{
    if (!trend_data)
        create_font(0);
    nsx/=chx;
    nsy/=chy;
    if (vt100_resize(&vt, nsx, nsy)&&!layered)
        InvalidateRect(wnd,0,0);
}

void change_font()
{
    CHOOSEFONT cf;
    RECT cl;
    
    memset(&cf,0,sizeof(CHOOSEFONT));
    cf.lStructSize=sizeof(CHOOSEFONT);
    cf.hwndOwner=wnd;
    cf.lpLogFont=&lf;
    cf.Flags=CF_FIXEDPITCHONLY|CF_FORCEFONTEXIST|CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT;
    
    if (!ChooseFont(&cf))
        return;
    free(trend_data);
    trend_data=0;
    GetClientRect(wnd, &cl);
    resized(cl.right, cl.bottom);
    redraw();
}

void change_font_png()
{
    pixel *png;
    int sx,sy;
    int i;
    RECT cl;
    
    png=read_png("3x5.png", &sx, &sy);
    if (!png)
    {
        MessageBox(wnd, "Cannot load PNG font.\n", "PNG font", MB_ICONERROR);
        return;
    }

    init_trend(sx/256, sy);
    for(i=0;i<256;i++)
        learn_char(png, i, sx, sy, i*chx, 0);
    free(png);
    
    GetClientRect(wnd, &cl);
    resized(cl.right, cl.bottom);
    redraw();
}

void fix_alpha(pixel *pic, int psx, int psy, RECT *rect)
{
    int x,y;
    
    for(y=rect->top;y<rect->bottom;y++)
        for(x=rect->left;x<rect->right;x++)
            pic[y*psx+x].a=0xff;
}

void clear_region(pixel *pic, int psx, int psy, RECT *rect)
{
    int x,y,rx,rd;
    
    y=rect->bottom-rect->top;
    rx=rect->right-rect->left;
    rd=psx-rx;
    pic+=rect->top*psx+rect->left;
    while(y--)
    {
        x=rx;
        while(x--)
        {
#ifdef RV
            *(int*)pic++=0xffffffff;
#else
            *(int*)pic++=0xff000000;
#endif
        }
        pic+=rd;
    }
}

void premult_alpha(pixel *pic, int psx, int psy, RECT *rect)
{
    int x,y,rx,rd;
    
    y=rect->bottom-rect->top;
    rx=rect->right-rect->left;
    rd=psx-rx;
    pic+=rect->top*psx+rect->left;
    while(y--)
    {
        x=rx;
        while(x--)
        {
            pic->a=255-pic->a;
#if 0
            pic->r=((unsigned int)pic->r)*pic->a/255;
            pic->g=((unsigned int)pic->g)*pic->a/255;
            pic->b=((unsigned int)pic->b)*pic->a/255;
#else
            pic->r=multable[pic->a][pic->r];
            pic->g=multable[pic->a][pic->g];
            pic->b=multable[pic->a][pic->b];
#endif
            pic++;
        }
        pic+=rd;
    }
}

void draw_frame()
{
    int i;
    RECT all;
    pixel fg,*p;
    
    i=psx*psy;
    p=pic;
    
    fg.r=0xff;
    fg.g=0xc0;
    fg.b=0x80;
    while(i--)
    {
        fg.a=rand()%128;
        *p++=fg;
    }
    
    all.left=0;
    all.top=0;
    all.right=psx;
    all.bottom=psy;
    premult_alpha(pic, psx, psy, &all);
}

void create_buffer(int sx, int sy)
{
    HDC dc;
    BITMAPINFO bmi;
    
    dc=GetDC(0);
    
    if (bmp)
        DeleteObject(bmp);
    
    memset(&bmi.bmiHeader,0,sizeof(BITMAPINFOHEADER));
    bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth=sx;
    bmi.bmiHeader.biHeight=-sy;
    bmi.bmiHeader.biPlanes=1;
    bmi.bmiHeader.biBitCount=32;
    bmi.bmiHeader.biCompression=BI_RGB;
    printf("Creating paint buffer.\n");
    bmp=CreateDIBSection(0,&bmi,DIB_RGB_COLORS,(void *)&pic,0,0);
    if (!bmp)
    {
        fprintf(stderr,"CreateDIBSection failed!\n");
        abort();
    }
    
    psx=sx;
    psy=sy;
    
    ReleaseDC(0, dc);
    if (layered)
        draw_frame();
}

void draw(RECT *crect, HDC dc)
{
    HDC memdc;
    HBITMAP oldbmp;
    POINT pt,pos;
    SIZE sz;
    BLENDFUNCTION bf;
    RECT rect,wr;
    int ox,oy;
    LARGE_INTEGER t1,t1a,t1b,t1c,t2,t3,t0;
    
    if (!trend_data)
        create_font(dc);
    
    rect=*crect;
    if (layered)
    {
        GetWindowRect(wnd, &wr);
        pt.x=0;
        pt.y=0;
        ClientToScreen(wnd, &pt);
        rect.left+=pt.x-wr.left;
        rect.top+=pt.y-wr.top;
    }
    else
        wr=rect;
    
    QueryPerformanceCounter(&t1);
    
    rect.right=rect.left+vt.sx*chx;
    rect.bottom=rect.top+vt.sy*chy;


    if (bgpic)
        tile_pic(pic, psx, psy, rect.left, rect.top, crect->right-crect->left, crect->right-crect->left, bgpic, bgsx, bgsy);
    else
        clear_region(pic, psx, psy, &rect);

    QueryPerformanceCounter(&t1a);
    
    render_vt(pic, psx, psy, rect.left, rect.top, &vt);
    
    QueryPerformanceCounter(&t1b);
    
    if (ovpic)
    {
        ox=(rect.right-rect.left-ovsx)/2;
        oy=(rect.bottom-rect.top-ovsy)/2;
        overlay_pic(pic, psx, psy, rect.left+ox, rect.top+oy, ovpic, ovsx, ovsy, 0x80);
    }

    QueryPerformanceCounter(&t1c);
    if (layered)
        premult_alpha(pic, psx, psy, &rect);
    
    QueryPerformanceCounter(&t2);
    
    memdc=CreateCompatibleDC(dc);
    oldbmp=SelectObject(memdc,bmp);

    if (layered)
    {
        pt.x=0;
        pt.y=0;
        pos.x=wr.left;
        pos.y=wr.top;
        sz.cx=psx;
        sz.cy=psy;
        bf.BlendOp=AC_SRC_OVER;
        bf.BlendFlags=0;
        bf.AlphaFormat=AC_SRC_ALPHA;
        bf.SourceConstantAlpha=0xff;		// FIX ME
//        cr.top=0;
//        cr.left=0;
//        cr.right=wr.right-wr.left;
//        cr.bottom=GetSystemMetrics(SM_CYCAPTION);
//        DrawCaption(wnd,memdc,&cr,DC_ACTIVE|DC_ICON|DC_TEXT);
//        fix_alpha(pic, wr.right, wr.bottom, &cr);
        if (!(*pUpdateLayeredWindow)(wnd,dc,&pos,&sz,memdc,&pt,0,&bf,ULW_ALPHA))
        {
            fprintf(stderr, "UpdateLayeredWindow failed.\n");
            exit(0);
        }
    }
    else
        BitBlt(dc,0,0,psx,psy,memdc,0,0,SRCCOPY);
    SelectObject(memdc,oldbmp);
    DeleteDC(memdc);
    
    QueryPerformanceCounter(&t3);
    QueryPerformanceFrequency(&t0);
    printf("Render: %6lu (clear %6lu, text %6lu, overlay %6lu, mult %6lu) Flush: %6lu (unit=1/%6lus)\n",
          (unsigned long)(t2.LowPart-t1.LowPart),
          (unsigned long)(t1a.LowPart-t1.LowPart),
          (unsigned long)(t1b.LowPart-t1a.LowPart),
          (unsigned long)(t1c.LowPart-t1b.LowPart),
          (unsigned long)(t2.LowPart-t1c.LowPart),
          (unsigned long)(t3.LowPart-t2.LowPart),
          (unsigned long)t0.LowPart);
}

void paint(HWND hwnd)
{
    HDC dc;
    PAINTSTRUCT paints;
    RECT fullrect;
    
    if (!GetClientRect(hwnd,&fullrect))
        return;
    if (!(dc=BeginPaint(hwnd,&paints)))
        return;
    draw(&fullrect, dc);    
    EndPaint(hwnd,&paints);
}

void redraw()
{
    HDC dc;
    RECT rect;

    if (!GetClientRect(wnd,&rect))
        return;
    dc=GetDC(layered?0:wnd);
    draw(&rect, dc);
    ReleaseDC(wnd, dc);
}

char *baton="/-\\|";
int bt=0;

int cols[]={9,0,4,2,6,1,5,3,7};
char *xtra[]={"",";2",";3",";4",";5",";7"};
char *xnames[]={"normal","dim","italic","underline","blink","inverse"};
void vt100_test(vt100 *vt)
{
    int f,b,ty,tx;
    
    for(ty=0;ty<3;ty++)
    {
        for(tx=0;tx<2;tx++)
            vt100_printf(vt,"%-20s\t",xnames[ty*2+tx]);
        vt100_printf(vt,"\n");
        for(b=0;b<9;b++)
        {
            for(tx=0;tx<2;tx++)
            {
                if (tx)
                    vt100_printf(vt,"\t");
                for(f=0;f<18;f++)
                    vt100_printf(vt,"\033[%u;3%u;4%u%sm*%s",
                        f/9,cols[f%9],cols[b],xtra[ty*2+tx], (f%9)?"":"\e[0m ");
                vt100_printf(vt,"\033[0m");
            }
            vt100_printf(vt,"\n");
        }
    }
}

void out_file(char *filename)
{
    FILE *f=fopen(filename, "r");
    char buf[1024];
    int n;
    
    if (!f)
    {
        vt100_printf(&vt, "Can't open file: %s\n", filename);
        return;
    }
    while((n=fread(buf,1,1024,f))>0)
        vt100_write(&vt, buf, n);
    fclose(f);
}

void set_spam()
{
    HMENU menu;
    MENUITEMINFO mii;

    menu=GETMENU(wnd);
    mii.cbSize=sizeof(MENUITEMINFO);
    mii.fMask=MIIM_STATE;
    GetMenuItemInfo(menu,MID_SPAM,0,&mii);
    if (mii.fState&MFS_CHECKED)
        KillTimer(wnd,0);
    else
        SetTimer(wnd,0,200,0);
    mii.fState^=MFS_CHECKED;
    SetMenuItemInfo(menu,MID_SPAM,0,&mii);
}

LRESULT APIENTRY MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_CREATE:
        {
            HMENU smenu,rmenu;
            
            smenu=GETMENU(hwnd);
            InsertMenu(smenu,-1,MF_BYPOSITION|MF_SEPARATOR,0,0);
            InsertMenu(smenu,-1,MF_BYPOSITION|MF_STRING,MID_CHANGEFONT,"Change &font");
            InsertMenu(smenu,-1,MF_BYPOSITION|MF_STRING,MID_PNGFONT,"&PNG &font");
            InsertMenu(smenu,-1,MF_BYPOSITION|MF_STRING,MID_CTEST,"Color &test");
            InsertMenu(smenu,-1,MF_BYPOSITION|MF_STRING,MID_SPAM,"&Spam");
            InsertMenu(smenu,-1,MF_BYPOSITION|MF_STRING,MID_TEST,"&Load file");
            InsertMenu(smenu,-1,MF_BYPOSITION|MF_SEPARATOR,0,0);
            rmenu=CreatePopupMenu();
            InsertMenu(smenu,-1,MF_BYPOSITION|MF_POPUP|MF_STRING,(int)rmenu,"&Window mode");
            InsertMenu(rmenu,-1,MF_BYPOSITION|MF_STRING|((mode_cur==0)?MF_CHECKED:0),MID_REND+0,"GDI DIB\t(no alpha)");
            InsertMenu(rmenu,-1,MF_BYPOSITION|MF_STRING|((mode_cur==2)?MF_CHECKED:0)|(pSetLayeredWindowAttributes?0:MF_GRAYED),MID_REND+2,"GDI layered\t(constant translucency)");
            InsertMenu(rmenu,-1,MF_BYPOSITION|MF_STRING|((mode_cur==1)?MF_CHECKED:0)|(pUpdateLayeredWindow?0:MF_GRAYED),MID_REND+1,"Owned\t(full alpha)");
            return 0;
        }
        
        case WM_SIZING:
        {
            RECT wr,cr;
            int nsx,nsy,fx,fy;
            
            if (!trend_data)
                create_font(0);
            GetWindowRect(wnd,&wr);
            GetClientRect(wnd,&cr);
            fx=(wr.right-wr.left)-(cr.right-cr.left);
            fy=(wr.bottom-wr.top)-(cr.bottom-cr.top);
            nsx=((LPRECT)lParam)->right-((LPRECT)lParam)->left-fx;
            nsy=((LPRECT)lParam)->bottom-((LPRECT)lParam)->top-fy;
            nsx/=chx;
            nsy/=chy;
            if (nsx<20)
                nsx=20;
            if (nsy<5)
                nsy=5;
            ((LPRECT)lParam)->right=((LPRECT)lParam)->left+nsx*chx+fx;
            ((LPRECT)lParam)->bottom=((LPRECT)lParam)->top+nsy*chy+fy;
            return 1;
        }
        
        case WM_SIZE:
        {
            int nx=LOWORD(lParam);
            int ny=HIWORD(lParam);
            RECT sr;
            
            if (!nx || !ny)
                return 0;
            if (nx==psx && ny==psy)
                return 0;
        
            printf("WM_SIZE.\n");
            resized(nx, ny);
            
            if (layered)
            {
                GetWindowRect(wnd, &sr);
                create_buffer(sr.right-sr.left, sr.bottom-sr.top);
                redraw();
            }
            else
                create_buffer(nx, ny);
            return 0;
        }

        case WM_PAINT:
            if (layered)
            {
                printf("WM_PAINT on a layered window.\n");
                ValidateRect(hwnd, 0);
                redraw();
                return 0;
            }
            paint(hwnd);
            return 0;

        case WM_DESTROY:
            if (mode_new==-1)
                PostQuitMessage(0);
            else
            {
                WINDOWPLACEMENT wndpl;
                
                wndpl.length=sizeof(WINDOWPLACEMENT);
                GetWindowPlacement(hwnd, &wndpl);
                create_window(wndpl.showCmd);
            }
            return 0;

#ifdef SYSMENU        
        case WM_SYSCOMMAND:
#else
        case WM_COMMAND:
#endif
            switch(wParam)
            {
            case MID_CHANGEFONT:
                change_font();
                return 0;
            case MID_PNGFONT:
                change_font_png();
                return 0;
            case MID_CTEST:
                vt100_test(&vt);
                redraw();
                return 0;
            case MID_SPAM:
                set_spam();
                return 0;
            case MID_TEST:
                out_file("test.txt");
                redraw();
                return 0;
            case MID_REND+0:
            case MID_REND+1:
            case MID_REND+2:
                mode_new=wParam-MID_REND;
                DestroyWindow(wnd);
                return 0;
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        
        case WM_TIMER:
            vt100_printf(&vt,"%c\n",baton[bt=(bt+1)%4]);
            redraw();
            return 0;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

