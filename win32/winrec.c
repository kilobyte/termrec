#define WINVER 0x501
#define _WIN32_WINDOWS 0x501
#define _WIN32_WINNT 0x501
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "sys/utils.h"
#include "ttyrec.h"
#include "name_out.h"

//#define EVDEBUG

recorder rec;
#ifdef EVDEBUG
FILE *evlog;
#endif

HANDLE con,proch;
DWORD proc;
UINT_PTR timer;

int vtrec_cursor;
int vtrec_cx,vtrec_cy; // the vt100 cursor
int vtrec_wx,vtrec_wy; // the win32 cursor
int vtrec_x1,vtrec_y1,vtrec_x2,vtrec_y2;
int vtrec_rows,vtrec_cols;
int utf8;
short vtrec_attr;
typedef CHAR_INFO SCREEN[0x10000]; // the SDK says we can't even think of bigger consoles
SCREEN cscr;
char vtrec_buf[0x10000], *vb;



void vtrec_commit()
{
    struct timeval tv;

    if (vb==vtrec_buf)
        return;
    gettimeofday(&tv, 0);
    ttyrec_w_write(rec, &tv, vtrec_buf, vb-vtrec_buf);
    vb=vtrec_buf;
}


void vtrec_printf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vb+=vsprintf(vb,fmt,ap);
    va_end(ap);
}
// FIXME: make it damn sure we don't have any buffer overflows


static inline void vtrec_utf8(unsigned short uv)
{	// Larry Wall style.  The similarities in formatting are, uhm,
        // purely accidental.
    if (uv<0x80)
    {
        *vb++=uv;
        return;
    }
    if (uv < 0x800) {
	*vb++ = ( uv >>  6)         | 0xc0;
	*vb++ = ( uv        & 0x3f) | 0x80;
	return;
    }
#ifdef GATES_FIXED_HIS_CONSOLE
    if (uv < 0x10000) {
#endif
	*vb++ = ( uv >> 12)         | 0xe0;
	*vb++ = ((uv >>  6) & 0x3f) | 0x80;
	*vb++ = ( uv        & 0x3f) | 0x80;
#ifdef GATES_FIXED_HIS_CONSOLE
	return;
    }
    if (uv < 0x200000) {
	*vb++ = ( uv >> 18)         | 0xf0;
	*vb++ = ((uv >> 12) & 0x3f) | 0x80;
	*vb++ = ((uv >>  6) & 0x3f) | 0x80;
	*vb++ = ( uv        & 0x3f) | 0x80;
	return;
    }
    if (uv < 0x4000000) {
	*vb++ = ( uv >> 24)         | 0xf8;
	*vb++ = ((uv >> 18) & 0x3f) | 0x80;
	*vb++ = ((uv >> 12) & 0x3f) | 0x80;
	*vb++ = ((uv >>  6) & 0x3f) | 0x80;
	*vb++ = ( uv        & 0x3f) | 0x80;
	return;
    }
    if (uv < 0x80000000) {
	*vb++ = ( uv >> 30)         | 0xfc;
	*vb++ = ((uv >> 24) & 0x3f) | 0x80;
	*vb++ = ((uv >> 18) & 0x3f) | 0x80;
	*vb++ = ((uv >> 12) & 0x3f) | 0x80;
	*vb++ = ((uv >>  6) & 0x3f) | 0x80;
	*vb++ = ( uv        & 0x3f) | 0x80;
	return;
    }
#endif
}


void vtrec_init()
{
    vtrec_cursor=1;
    vtrec_cx=vtrec_wx=0;
    vtrec_cy=vtrec_wy=0;
    vtrec_cols=-1;
    vtrec_rows=-1; // try to force a resize
    vb=vtrec_buf;
    vtrec_printf("\e[c\e%%%c\e[f\e[?7l", utf8?'G':'@');
}

static char VT_COLORS[]="04261537";


void vtrec_realize_attr()
{
    vtrec_printf("\e[0;3%c;4%c%s%sm",
        VT_COLORS[vtrec_attr&7],
        VT_COLORS[(vtrec_attr>>4)&7],
        vtrec_attr&8?";1":"",
        vtrec_attr&0x80?";5":"");
}


#define is_printable(x) (((x)>=32 && (x)<127) || ((signed char)(x)<0))

static inline void vtrec_outchar(CHAR_INFO c)
{
    if (utf8)
        vtrec_utf8(c.Char.UnicodeChar);
    else
        if (is_printable(c.Char.AsciiChar))
            *vb++=c.Char.AsciiChar;
        else
            *vb++='?';
    vtrec_cx++;
}

void vtrec_char(int x, int y, CHAR_INFO c)
{
#ifdef EVDEBUG
    if (x<0 || x>=vtrec_cols || y<0 || y>=vtrec_rows)
        fprintf(evlog, "Out of bounds: %d,%d\n", x, y);
#endif
    if (x!=vtrec_cx || y!=vtrec_cy)
    {
        if (y==vtrec_cy && vtrec_cx<x && vtrec_cx+10>=x)
        {	// On small skips, it's better to rewrite instead of jumping,
                // especially considering that our algorithm produces a lot of
                // such skips if the new text has a single matching letter in
                // the same place as the old one.
            int z;
            CHAR_INFO *sp;
            
            sp=cscr+vtrec_cy*vtrec_cols+vtrec_cx;
            for(z=vtrec_cx;z<x;z++)
                if (sp++->Attributes!=vtrec_attr)
                    goto jump;
            sp=cscr+vtrec_cy*vtrec_cols+vtrec_cx;
            for(z=vtrec_cx;z<x;z++)
                vtrec_outchar(*sp++);
        }
        else
        {
        jump:
            vtrec_printf("\e[%d;%df", (vtrec_cy=y)+1, (vtrec_cx=x)+1);
        }
    }
    if (c.Attributes!=vtrec_attr)
    {
        vtrec_attr=c.Attributes;
        vtrec_realize_attr();
    }
    vtrec_outchar(c);
}


void vtrec_realize_cursor()
{
    if (!vtrec_cursor)	// cursor hidden -- its position is irrelevant
        return;
    if (vtrec_cx==vtrec_wx && vtrec_wy==vtrec_cy)
        return;
    vtrec_printf("\e[%d;%dH", vtrec_wy+1, vtrec_wx+1);
    vtrec_cx=vtrec_wx;
    vtrec_cy=vtrec_wy;
}


void vtrec_dump(int full)
{
    CONSOLE_SCREEN_BUFFER_INFO cbi;
    SCREEN scr;
    CHAR_INFO *sp,*cp;
    SMALL_RECT reg;
    COORD sz,org;
    int x,y;

    GetConsoleScreenBufferInfo(con, &cbi);
    vtrec_x1=cbi.srWindow.Left;
    vtrec_y1=cbi.srWindow.Top;
    vtrec_x2=cbi.srWindow.Right;
    vtrec_y2=cbi.srWindow.Bottom;
    if (vtrec_cols!=vtrec_x2-vtrec_x1+1
      ||vtrec_rows!=vtrec_y2-vtrec_y1+1)
    {
        vtrec_cols=vtrec_x2-vtrec_x1+1;
        vtrec_rows=vtrec_y2-vtrec_y1+1;
        full=1;
        vtrec_printf("\e[8;%d;%dt\e[1;%dr", vtrec_rows, vtrec_cols, vtrec_rows);
    }
    if (vtrec_rows*vtrec_cols>0x10000)
        return;
    vtrec_wx=cbi.dwCursorPosition.X-vtrec_x1;
    vtrec_wy=cbi.dwCursorPosition.Y-vtrec_y1;
#ifdef EVDEBUG
    fprintf(evlog, "===dump%s===\n", full?" (full)":"");
#endif
    reg=cbi.srWindow;
    sz.X=vtrec_cols;
    sz.Y=vtrec_rows;
    org.X=0;
    org.Y=0;
    if (utf8)
    {
        if (ReadConsoleOutputW(con, scr, sz, org, &reg))
            goto dump_ok;
        if (ReadConsoleOutputA(con, scr, sz, org, &reg))
        {
#ifdef EVDEBUG
            fprintf(evlog, "Disabling Unicode\n");
#endif
            vtrec_printf("\e%%@");
            utf8=0;
            goto dump_ok;
        }
    }
    else
        if (ReadConsoleOutputA(con, scr, sz, org, &reg))
            goto dump_ok;
#ifdef EVDEBUG
    fprintf(evlog, "DUMP FAILED!!\n");
#endif
    return;
dump_ok:
    // FIXME: the region could have changed between the calls, resulting in
    // garbage in the output.
    
    if (full)
    {
        sp=scr;
        for(y=0;y<vtrec_rows;y++)
        {
            for(x=0;x<vtrec_cols;x++)
                vtrec_char(x,y,*sp++);
        }
        memcpy(cscr, scr, vtrec_rows*vtrec_cols*sizeof(CHAR_INFO));
    }
    else
    {
        sp=scr;
        cp=cscr;
        for(y=0;y<vtrec_rows;y++)
        {
            for(x=0;x<vtrec_cols;x++)
            {
                if (sp->Char.UnicodeChar!=cp->Char.UnicodeChar
                 || sp->Attributes!=cp->Attributes)
                    vtrec_char(x,y,*sp);
                *cp++=*sp++;
            }
        }
    }
    vtrec_realize_cursor();
    vtrec_commit();
}


// For any non-base-ASCII chars, the 16 bit value given is neither the local
// win codepage value nor Unicode.  Ghrmblah.
void vtrec_dump_char(int wx, int wy, DWORD ch)
{
    CHAR_INFO c;
    SMALL_RECT reg;
    COORD sz,org;
    
    sz.X=1;
    sz.Y=1;
    org.X=0;
    org.Y=0;
    reg.Left=reg.Right=wx;
    reg.Top=reg.Bottom=wy;
    if ((LOWORD(ch)>=32 && LOWORD(ch)<127) ||
        !(utf8?ReadConsoleOutputW(con, &c, sz, org, &reg):
               ReadConsoleOutputA(con, &c, sz, org, &reg)))
    {	    // last resort
        c.Char.UnicodeChar=LOWORD(ch);
        c.Attributes=HIWORD(ch);
    }
    
    wx-=vtrec_x1;
    wy-=vtrec_y1;
    vtrec_char(wx, wy, c);
    cscr[wy*vtrec_cols+wx]=c;
    vtrec_commit();
}


void vtrec_scroll(int d)
{
    CONSOLE_SCREEN_BUFFER_INFO cbi;
    CHAR_INFO *cp;
    int x,h=abs(d);
    
    if (h>=vtrec_rows)
    {
        vtrec_dump(1);
        return;
    }
    GetConsoleScreenBufferInfo(con, &cbi);
    vtrec_attr=cbi.wAttributes;
    vtrec_realize_attr();
    x=vtrec_cols*h;
    if (d<0)
    {
        memmove(cscr, cscr+h*vtrec_cols, (vtrec_cols*(vtrec_rows-h))*sizeof(CHAR_INFO));
        cp=cscr+(vtrec_cols*(vtrec_rows-h));
        vtrec_printf("\e[%df", vtrec_rows);
        while(h--)
            vtrec_printf("\eD");
    }
    else
    {
        memmove(cscr+h*vtrec_cols, cscr, (vtrec_cols*(vtrec_rows-h))*sizeof(CHAR_INFO));
        cp=cscr;
        vtrec_printf("\e[f");
        while(h--)
            vtrec_printf("\eM");
    }
    while(x--)
    {
        cp->Char.UnicodeChar=' ';
        cp->Attributes=vtrec_attr;
        cp++;
    }
    vtrec_x1=cbi.srWindow.Left;
    vtrec_y1=cbi.srWindow.Top;
    vtrec_x2=cbi.srWindow.Right;
    vtrec_y2=cbi.srWindow.Bottom;
    vtrec_wx=cbi.dwCursorPosition.X-vtrec_x1;
    vtrec_wy=cbi.dwCursorPosition.Y-vtrec_y1;
    vtrec_realize_cursor();
    vtrec_commit();
}


int vtrec_dirty=0;
int vtrec_reent=0;

#ifdef EVDEBUG
static char *evnames[]={
0,
"EVENT_CONSOLE_CARET",
"EVENT_CONSOLE_UPDATE_REGION",
"EVENT_CONSOLE_UPDATE_SIMPLE",
"EVENT_CONSOLE_UPDATE_SCROLL",
"EVENT_CONSOLE_LAYOUT",
"EVENT_CONSOLE_START_APPLICATION",
"EVENT_CONSOLE_END_APPLICATION"};
#endif


VOID CALLBACK WinEventProc(
  HWINEVENTHOOK hWinEventHook,
  DWORD event,
  HWND hwnd,
  LONG idObject,
  LONG idChild,
  DWORD dwEventThread,
  DWORD dwmsEventTime
)
{
#ifdef EVDEBUG
    if (event>0x4000 && event<=0x4007)
    fprintf(evlog, "%s: %d(%d,%d), %d(%d,%d)\n",
        evnames[event-0x4000],
        (int)idObject,
        (short int)LOWORD(idObject),
        (short int)HIWORD(idObject),
        (int)idChild,
        (short int)LOWORD(idChild),
        (short int)HIWORD(idChild));
#endif
    if (timer)	// If events work, let's disable polling.
    {
        if (timer!=(UINT_PTR)(-1))	// magic cookie for Win95/98/ME
            KillTimer(0, timer);
        timer=0;
    }
    if (vtrec_reent)
    {
        vtrec_dirty=1;
        return;
    }
    vtrec_reent=1;
    if (vtrec_dirty)
    {
        vtrec_dirty=0;
        vtrec_dump(0);
        vtrec_reent=0;
        return;
    }
    switch(event)
    {
    case EVENT_CONSOLE_CARET:
        if (vtrec_cursor!=!!(idObject&CONSOLE_CARET_VISIBLE))
        {
            vtrec_cursor=!vtrec_cursor;
            vtrec_printf("\e[?4%c", vtrec_cursor?'h':'l');
        }
        if (LOWORD(idChild)+vtrec_x1!=vtrec_wx
         || HIWORD(idChild)+vtrec_y1!=vtrec_wy)
        {
            vtrec_wx=LOWORD(idChild)-vtrec_x1;
            vtrec_wy=HIWORD(idChild)-vtrec_y1;
            vtrec_realize_cursor();
        }
        vtrec_commit();
        break;
    case EVENT_CONSOLE_LAYOUT:
        vtrec_dump(0);
        break;
    case EVENT_CONSOLE_UPDATE_REGION:
        vtrec_dump(0);
        break;
    case EVENT_CONSOLE_UPDATE_SIMPLE:
        vtrec_dump_char(LOWORD(idObject), HIWORD(idObject), idChild);
        break;
    case EVENT_CONSOLE_UPDATE_SCROLL:
        if (idObject)
            vtrec_dump(1);
        else
            if (idChild)
                vtrec_scroll(idChild);
        break;
#if 0
    case EVENT_CONSOLE_END_APPLICATION:
        // doesn't work on Win98 -- replaced by MsgWait
        if (idObject==proc)
            PostQuitMessage(0);
#endif
    }
    vtrec_reent=0;
}


void finish_up()
{
    vtrec_printf("\e[7?h\e[?4h");
    vtrec_commit();
    ttyrec_w_close(rec);
#ifdef EVDEBUG
    fprintf(evlog, "*** THE END ***\n");
#endif
}


VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    if (vtrec_reent)
        return;
    vtrec_reent=1;
    vtrec_dump(0);
    vtrec_reent=0;
}


BOOL WINAPI CtrlHandler(DWORD dwCtrlType)
{
    switch(dwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        return 1;
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        finish_up();
        exit(0);
        return 1;
    case CTRL_LOGOFF_EVENT:
    default:
        return 0;
    }
}


int check_console()
{
    CONSOLE_SCREEN_BUFFER_INFO cbi;
    
    if (!(con=GetStdHandle(STD_OUTPUT_HANDLE)) || con==INVALID_HANDLE_VALUE)
        return 0;
    if (!GetConsoleScreenBufferInfo(con, &cbi))
        return 0;
    printf("termrec version " PACKAGE_VERSION "\n"
           "Recording through %s to %s.\n"
           "Console size: %dx%d.\n",
        format, record_name,
        cbi.srWindow.Right-cbi.srWindow.Left+1,
        cbi.srWindow.Bottom-cbi.srWindow.Top+1);
    return 1;
}


int create_console()
{
    HANDLE fd;
    SECURITY_ATTRIBUTES sec;

    FreeConsole();
    AllocConsole();
    SetConsoleTitle("termrec");
    
    sec.nLength=sizeof(SECURITY_ATTRIBUTES);
    sec.lpSecurityDescriptor=0;
    sec.bInheritHandle=1;
    
    fd=CreateFile("CONIN$", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|
        FILE_SHARE_WRITE, &sec, OPEN_EXISTING, 0, 0);
    if (fd==INVALID_HANDLE_VALUE)
        return 0;
    SetStdHandle(STD_INPUT_HANDLE, fd);
    
    fd=CreateFile("CONOUT$", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|
        FILE_SHARE_WRITE, &sec, OPEN_EXISTING, 0, 0);
    if (fd==INVALID_HANDLE_VALUE)
        return 0;
    SetStdHandle(STD_OUTPUT_HANDLE, fd);
    SetStdHandle(STD_ERROR_HANDLE, fd);
    
    return 1;
}


void set_event_hook()
{
    typedef HWINEVENTHOOK (WINAPI* SWEH)(UINT,UINT,HMODULE,WINEVENTPROC,DWORD,DWORD,UINT);
    HMODULE dll;
    SWEH sweh;
    
    if ((dll=LoadLibrary("user32")))
        if ((sweh=(SWEH)GetProcAddress(dll,"SetWinEventHook")))
            sweh(EVENT_CONSOLE_CARET,
                         EVENT_CONSOLE_END_APPLICATION,
                         0,
                         (WINEVENTPROC)WinEventProc,
                         0,
                         0,
                         WINEVENT_OUTOFCONTEXT);
    /* If the above fails -- we just fall back to the polling scheme.
     * Actually, we start polling by default, and just disable it when
     * we get the first event.
     */
}


void spawn_process()
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    memset(&si, 0, sizeof(si));
    si.cb=sizeof(STARTUPINFO);
    memset(&pi, 0, sizeof(pi));

    if (!command)
        command=getenv("COMSPEC");
    if (!command)
    {
        fprintf(stderr, "No command given and no COMSPEC set, aborting.\n");
        finish_up();
        exit(1);
    }
    printf("Trying to spawn [%s]\n", command);
    if (!CreateProcess(0, command,
          0, 0, 0,
          CREATE_DEFAULT_ERROR_MODE,
          0, 0, &si, &pi))
    {
        fprintf(stderr, "Can't spawn child process.\n");
        finish_up();
        exit(1);
    }
    proc=pi.dwProcessId;
    proch=pi.hProcess;
}


int main(int argc, char **argv)
{
    struct timeval tv;
    int record_f;

#ifdef EVDEBUG
    evlog=fopen("evlog", "w");
#endif
    get_parms(argc, argv, 0);
    record_f=fopen_out(&record_name, 1);
    if (record_f==-1)
    {
        fprintf(stderr, "Can't open %s\n", record_name);
        return 1;
    }
    utf8=1;

    if (!check_console())
        if (!create_console() || !check_console())
        {
            show_error("Not on a console.  My attempts");
            return 1;
        }
    vtrec_init();
    proc=0;
    proch=0;
    timer=(UINT_PTR)(-1);
    vtrec_reent=0;
    vtrec_dirty=0;
    gettimeofday(&tv, 0);
    rec=ttyrec_w_open(record_f, format, record_name, &tv);
    vtrec_dump(1);
    SetConsoleCtrlHandler(CtrlHandler,1);
    set_event_hook();
    spawn_process();

    vtrec_dump(0);
    if (timer==(UINT_PTR)(-1))
    {
        if (!(timer=SetTimer(0,0,20 /* poll 50 times per second */,TimerProc)))
        {
            fprintf(stderr, "Failed to set up pollination.\n");
            finish_up();
            return 1;
        }
    }
    message_loop(&proch, 1);
    finish_up();
    return 0;           
}
