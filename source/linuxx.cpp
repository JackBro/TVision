/* $Id: linuxx.cpp,v 1.1 2004/08/18 06:51:50 jeremy Exp $ */
/*
 * system.cc
 *
 * Copyright (c) 1998 Sergio Sigala, Brescia, Italy.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Modified by Sergey Clushin <serg@lamport.ru>, <Clushin@deol.ru> */
/* Modified by Dmitrij Korovkin <tkf@glasnet.ru> */
/* Modified by Ilfak Guilfanov, <ig@datarescue.com> */
/* Modified by Jeremy Cooper, <jeremy@baymoo.org> */

#ifdef __LINUX__
#define Uses_TButton
#define Uses_TColorSelector
#define Uses_TDeskTop
#define Uses_TDirListBox
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TEventQueue
#define Uses_TFrame
#define Uses_THistory
#define Uses_TIndicator
#define Uses_TKeys
#define Uses_TListViewer
#define Uses_TMenuBox
#define Uses_TOutlineViewer
#define Uses_TScreen
#define Uses_TScrollBar
#define Uses_TStatusLine
#define Uses_TProgram
#include <tv.h>

#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

static Display *x_disp;
static Window x_win;
static GC *characterGC;
static int x_disp_fd;

static bool inited = false;

static int lastMods;
static int current_color;
static Time last_click_time;
static int last_click_button;
static int keyboardModifierState;
static Region expose_region;

#define	APP_CLASS	"TVision"
#define	APP_NAME	"TVision"

#define TICKS_TO_MS(x)	((x) * 1000/18)
#define MS_TO_TICKS(x)  ((x) * 18/1000)

/*
 * This is the delay in ms before the first evMouseAuto is generated when the
 * user holds down a mouse button.
 */
#define DELAY_AUTOCLICK_FIRST   400

/*
 * This is the delay in ms between next evMouseAuto events.  Must be greater
 * than DELAY_SIGALRM (see below).
 */
#define DELAY_AUTOCLICK_NEXT    100

/*
 * This is the delay in ms between consecutive SIGALRM signals.  This
 * signal is used to generate evMouseAuto and cmSysWakeup events.
 */
#define DELAY_SIGALRM           100

/*
 * This broadcast event is used to update the StatusLine.
 */
#define DELAY_WAKEUP            200

/*
 * Default text display font.
 */
#define DEFAULT_FONT		"vga"

/*
 * X Window border width (in pixels).
 */
#define	BORDER_WIDTH	1

/*
 * Default background and foreground colors.
 */
#define DEFAULT_BGCOLOR_0	"black"
#define DEFAULT_BGCOLOR_1	"blue4"
#define DEFAULT_BGCOLOR_2	"green4"
#define DEFAULT_BGCOLOR_3	"cyan4"
#define DEFAULT_BGCOLOR_4	"red4"
#define DEFAULT_BGCOLOR_5	"magenta4"
#define DEFAULT_BGCOLOR_6	"brown"
#define DEFAULT_BGCOLOR_7	"grey80"
/* Foreground colors */
#define DEFAULT_FGCOLOR_0	"black"
#define DEFAULT_FGCOLOR_1	"blue4"
#define DEFAULT_FGCOLOR_2	"green4"
#define DEFAULT_FGCOLOR_3	"cyan4"
#define DEFAULT_FGCOLOR_4	"red4"
#define DEFAULT_FGCOLOR_5	"magenta4"
#define DEFAULT_FGCOLOR_6	"brown"
#define DEFAULT_FGCOLOR_7	"grey80"
#define DEFAULT_FGCOLOR_8	"grey30"
#define DEFAULT_FGCOLOR_9	"blue"
#define DEFAULT_FGCOLOR_10	"green"
#define DEFAULT_FGCOLOR_11	"cyan"
#define DEFAULT_FGCOLOR_12	"red"
#define DEFAULT_FGCOLOR_13	"magenta"
#define DEFAULT_FGCOLOR_14	"yellow"
#define DEFAULT_FGCOLOR_15	"white"

#define qnumber(x) (sizeof(x)/sizeof(x[0]))

/*
 * Default window geometry.
 */
#define	DEFAULT_GEOMETRY_WIDTH	80
#define	DEFAULT_GEOMETRY_HEIGHT	25

/* Background text colors */
static char *bgColor[] = {
  DEFAULT_BGCOLOR_0,
  DEFAULT_BGCOLOR_1,
  DEFAULT_BGCOLOR_2,
  DEFAULT_BGCOLOR_3,
  DEFAULT_BGCOLOR_4,
  DEFAULT_BGCOLOR_5,
  DEFAULT_BGCOLOR_6,
  DEFAULT_BGCOLOR_7
};

/* Foreground text colors */
static char *fgColor[] = {
  DEFAULT_FGCOLOR_0,
  DEFAULT_FGCOLOR_1,
  DEFAULT_FGCOLOR_2,
  DEFAULT_FGCOLOR_3,
  DEFAULT_FGCOLOR_4,
  DEFAULT_FGCOLOR_5,
  DEFAULT_FGCOLOR_6,
  DEFAULT_FGCOLOR_7,
  DEFAULT_FGCOLOR_8,
  DEFAULT_FGCOLOR_9,
  DEFAULT_FGCOLOR_10,
  DEFAULT_FGCOLOR_11,
  DEFAULT_FGCOLOR_12,
  DEFAULT_FGCOLOR_13,
  DEFAULT_FGCOLOR_14,
  DEFAULT_FGCOLOR_15
};

static char *fontName = DEFAULT_FONT,
            *geometry = NULL;

static int fontHeight, fontAscent, fontDescent, fontWidth;

struct XResource {
  const char *name;
  const char *cls;
  char **value;
};
typedef struct XResource XResource;

static XResource TvisionResources[] = {
  { "tvision.text.bgcolor0",  "Tvision.Text.Bgcolor0",  &bgColor[0] },
  { "tvision.text.bgcolor1",  "Tvision.Text.Bgcolor1",  &bgColor[1] },
  { "tvision.text.bgcolor2",  "Tvision.Text.Bgcolor2",  &bgColor[2] },
  { "tvision.text.bgcolor3",  "Tvision.Text.Bgcolor3",  &bgColor[3] },
  { "tvision.text.bgcolor4",  "Tvision.Text.Bgcolor4",  &bgColor[4] },
  { "tvision.text.bgcolor5",  "Tvision.Text.Bgcolor5",  &bgColor[5] },
  { "tvision.text.bgcolor6",  "Tvision.Text.Bgcolor6",  &bgColor[6] },
  { "tvision.text.bgcolor7",  "Tvision.Text.Bgcolor7",  &bgColor[7] },
  { "tvision.text.fgcolor0",  "Tvision.Text.Fgcolor0",  &fgColor[0] },
  { "tvision.text.fgcolor1",  "Tvision.Text.Fgcolor1",  &fgColor[1] },
  { "tvision.text.fgcolor2",  "Tvision.Text.Fgcolor2",  &fgColor[2] },
  { "tvision.text.fgcolor3",  "Tvision.Text.Fgcolor3",  &fgColor[3] },
  { "tvision.text.fgcolor4",  "Tvision.Text.Fgcolor4",  &fgColor[4] },
  { "tvision.text.fgcolor5",  "Tvision.Text.Fgcolor5",  &fgColor[5] },
  { "tvision.text.fgcolor6",  "Tvision.Text.Fgcolor6",  &fgColor[6] },
  { "tvision.text.fgcolor7",  "Tvision.Text.Fgcolor7",  &fgColor[7] },
  { "tvision.text.fgcolor8",  "Tvision.Text.Fgcolor8",  &fgColor[8] },
  { "tvision.text.fgcolor9",  "Tvision.Text.Fgcolor9",  &fgColor[9] },
  { "tvision.text.fgcolor10", "Tvision.Text.Fgcolor10", &fgColor[10] },
  { "tvision.text.fgcolor11", "Tvision.Text.Fgcolor11", &fgColor[11] },
  { "tvision.text.fgcolor12", "Tvision.Text.Fgcolor12", &fgColor[12] },
  { "tvision.text.fgcolor13", "Tvision.Text.Fgcolor13", &fgColor[13] },
  { "tvision.text.fgcolor14", "Tvision.Text.Fgcolor14", &fgColor[14] },
  { "tvision.text.fgcolor15", "Tvision.Text.Fgcolor15", &fgColor[15] },
  { "tvision.text.font",      "Tvision.Text.Font",      &fontName },
  { "tvision.geometry",       "Tvision.Geometry",       &geometry },
};
static const int TvisionResources_count = qnumber(TvisionResources);

static FILE *logfp = NULL;
void LOG(const char *format, ...)
{
  if ( logfp != NULL )
  {
    va_list va;
    va_start(va, format);
    vfprintf(logfp, format, va);
    va_end(va);
    fprintf(logfp, "\n");
  }
}

static void error(const char *format, ...)
{
  va_list va;
  va_start(va, format);
  vfprintf(stderr, format, va);
  va_end(va);
  abort();
}

static void warning(const char *format, ...)
{
  va_list va;
  va_start(va, format);
  vfprintf(stderr, format, va);
  va_end(va);
}

ushort TEventQueue::doubleDelay = 8;
Boolean TEventQueue::mouseReverse = (enum Boolean) False;

ushort TScreen::screenMode;
int TScreen::screenWidth;
int TScreen::screenHeight;
ushort *TScreen::screenBuffer;
fd_set TScreen::fdSetRead;
fd_set TScreen::fdActualRead;

static TEvent *evIn;            /* message queue system */
static TEvent *evOut;
static TEvent evQueue[eventQSize];
static char env[PATH_MAX];      /* value of the TVOPT environment variable */
static int curX, curY;          /* current cursor coordinates */
static bool cursor_displayed;   /* is cursor visible? */
static int currentTime;         /* current timer value */
static int doRepaint;           /* should redraw the screen ? */
static int evLength;            /* number of events in the queue */
static int msOldButtons;        /* mouse button status */
static TEvent msev;             /* last mouse event */

/*
 * A simple class which implements a timer.
 */

class Timer
{
        int limit;
public:
        Timer() { limit = -1; }
        int isExpired() { return limit != -1 && currentTime >= limit; }
        int isRunning() { return limit != -1; }
        void start(int timeout) { limit = currentTime + timeout; }
        void stop() { limit = -1; }
};

static Timer msAutoTimer;       /* time when generate next cmMouseAuto */
static Timer wakeupTimer;       /* time when generate next cmWakeup */

/*
 * GENERAL FUNCTIONS
 */

inline int range(int test, int min, int max)
{
        return test < min ? min : test > max ? max : test;
}

/*
 * KEYBOARD FUNCTIONS
 */

/*
 * Gets information about modifier keys (Alt, Ctrl and Shift).  This can
 * be done only if the program runs on the system console.
 */

uchar getShiftState()
{
	return keyboardModifierState;
}

ulong getTicks(void)
{
//  struct timeval tv;
//  gettimeofday(&tv, NULL);
//  return (tv.tv_sec*100 + tv.tv_usec / 10000) * 18 / 100;
  return currentTime * 18 / 1000;
}

static void
handleXExpose(XEvent *event)
{
  XRectangle r;

  r.x = event->xexpose.x;
  r.y = event->xexpose.y;
  r.width = event->xexpose.width;
  r.height = event->xexpose.height;

  XUnionRectWithRegion(&r, expose_region, expose_region);

  if (event->xexpose.count == 0) {
    int row, res;

    for (row = 0; row < TScreen::screenHeight; row++) {
      /*
       * First, calculate if anything in this entire row is exposed.
       */
      r.x = 0;
      r.y = row * fontHeight;
      r.width = TScreen::screenWidth * fontWidth;
      r.height = fontHeight;

      res = XRectInRegion(expose_region, 0, row * fontHeight,
        fontWidth * TScreen::screenWidth, fontHeight);

      if (res != RectangleOut) {
        /*
         * Something in this row is exposed, just draw the whole row.
         */
        TScreen::writeRow(row * TScreen::screenWidth,
                          &TScreen::screenBuffer[row * TScreen::screenWidth],
                          TScreen::screenWidth);
      }
    }

    XDestroyRegion(expose_region);
    expose_region = XCreateRegion();
  }
}

static int
translateXModifierState(int x_state)
{
  int state = 0;

  if (x_state & ShiftMask)
    state |= kbLeftShift;
  if (x_state & ControlMask)
    state |= kbCtrlShift;
  if (x_state & LockMask)
    state |= kbCapsState;

  return state;
}

/*
 * Reads a key from the keyboard.
 */
void TScreen::get_key_mouse_event(TEvent &event)
{
  XEvent xEvent;
  int num_events;

  event.what = evNothing;

  /*
   * suspend until there is a signal or some data in file
   * descriptors
   *
   * if event_delay is zero, then do not suspend
   */
  if ( XEventsQueued(x_disp, QueuedAlready) == 0 && TProgram::event_delay != 0)
  {
    fdActualRead = fdSetRead;
    select(FD_SETSIZE, &fdActualRead, NULL, NULL, NULL);
  }

  num_events = XEventsQueued(x_disp, QueuedAfterFlush);

  for (;num_events > 0; num_events--) {
    XNextEvent(x_disp, &xEvent);
    switch (xEvent.type) {
    case Expose:
      handleXExpose(&xEvent);
      break;
    case ButtonPress:
      event.what = evMouseDown;
      event.mouse.buttons = mbLeftButton;
      event.mouse.where.x = xEvent.xbutton.x / fontWidth;
      event.mouse.where.y = xEvent.xbutton.y / fontHeight;
      if (event.mouse.where == msev.mouse.where && last_click_time != 0 &&
          (xEvent.xbutton.time - last_click_time) <
          TICKS_TO_MS(TEventQueue::doubleDelay)) {
        event.mouse.eventFlags = meDoubleClick;
        last_click_time = 0;
      } else {
        event.mouse.eventFlags = 0;
      }
      keyboardModifierState = translateXModifierState(xEvent.xkey.state);
      event.mouse.controlKeyState = keyboardModifierState;
      msev = event;
      // Start the autoclick timer.
      msAutoTimer.start(DELAY_AUTOCLICK_FIRST);
      return;
    case ButtonRelease:
      event.what = evMouseUp;
      event.mouse.buttons = 0;
      event.mouse.where.x = xEvent.xbutton.x / fontWidth;
      event.mouse.where.y = xEvent.xbutton.y / fontHeight;
      event.mouse.eventFlags = 0;
      last_click_time = xEvent.xbutton.time;
      keyboardModifierState = translateXModifierState(xEvent.xkey.state);
      event.mouse.controlKeyState = keyboardModifierState;
      msev = event;
      msAutoTimer.stop();
      return;
    case MotionNotify:
      int x, y;

      // X reports mouse movements at every pixel position.  TVision only
      // cares if the mouse changes character positions.  Calculate the
      // character position for the event and see if it has changed from the
      // last event.
      x = xEvent.xbutton.x / fontWidth;
      y = xEvent.xbutton.y / fontHeight;
      keyboardModifierState = translateXModifierState(xEvent.xkey.state);
      if (msev.mouse.where.x != x || msev.mouse.where.y != y) {
        event.what = evMouseMove;
        event.mouse.buttons = msev.mouse.buttons;
        event.mouse.where.x = x;
        event.mouse.where.y = y;
        event.mouse.eventFlags = 0;
        event.mouse.controlKeyState = keyboardModifierState;
        msev = event;
        return;
      }
      break;
    case KeyPress:
      keyboardModifierState = translateXModifierState(xEvent.xkey.state);
#if 0
      xEvent.xkey.state
      event.keyDown.keyCode = code;
      event.keyDown.controlKeyState = modifiers;
#endif
      printf("Key press\n"); 
      break;
    case KeyRelease:
      keyboardModifierState = translateXModifierState(xEvent.xkey.state);
      break;
    case MappingNotify:
      XRefreshKeyboardMapping(&xEvent.xmapping);
      break;
    case ConfigureNotify:
      screenWidth = range(xEvent.xconfigure.width / fontWidth, 4, maxViewWidth);
      screenHeight = range(xEvent.xconfigure.height / fontHeight, 4, 100);
      delete[] screenBuffer;
      screenBuffer = new ushort[screenWidth * screenHeight];
      LOG("screen resized to %dx%d", screenWidth, screenHeight);
      drawCursor(0); /* hide the cursor */
      event.message.command = cmSysResize;
      event.what = evCommand;
      return;
    }
 }

  /* see if there is data available */
}

static void selectPalette()
{
  TScreen::screenMode = TScreen::smCO80;
}

/*
 * Show a warning message.
 */

static int confirmExit()
{
        /* we need the buffer address */

        class MyBuffer: public TDrawBuffer
        {
        public:
                ushort *getBufAddr() { return data; }
        };
        MyBuffer b;
        static char msg[] = "Warning: are you sure you want to quit ?";

        b.moveChar(0, ' ', 0x4f, TScreen::screenWidth);
        b.moveStr(max((TScreen::screenWidth - (int) (sizeof(msg) - 1)) / 2,
                0), msg, 0x4f);
        TScreen::writeRow(0, b.getBufAddr(), TScreen::screenWidth);

	for (;;) {
		char xlat[20];
		int char_count;
		KeySym key;
		XEvent theEvent;

		XNextEvent(x_disp, &theEvent);

		switch (theEvent.type) {
		case KeyPress:
			char_count = XLookupString(&theEvent.xkey, xlat,
				sizeof(xlat), &key, NULL);
			if (char_count > 0) 
				 return toupper(xlat[0]) == 'Y';
			break;
		case Expose:
			handleXExpose(&theEvent);
			break;
		case MappingNotify:
			XRefreshKeyboardMapping(&theEvent.xmapping);
			break;
		default:
			break;
		}
	}

	return 0;
}

static void freeResources()
{
        TScreen::drawMouse(0);
        delete[] TScreen::screenBuffer;
        LOG("terminated");
}

/*
 * General signal handler.
 */

static void sigHandler(int signo)
{
  struct sigaction dfl_handler;

  sigemptyset(&dfl_handler.sa_mask);
  dfl_handler.sa_flags = SA_RESTART;
  switch (signo)
  {
    case SIGALRM:
      /*
       * called every DELAY_SIGALRM ms
       */
      currentTime += DELAY_SIGALRM;
      break;
    case SIGCONT:
      /*
       * called when the user restart the process after a ctrl-z
       */
      LOG("continuing process");
      TScreen::resume();

      /* re-enable SIGTSTP */

      dfl_handler.sa_handler = sigHandler;
      sigaction(SIGTSTP, &dfl_handler, NULL);
      break;
    case SIGINT:
    case SIGQUIT:
      /*
       * These signals are now trapped.
       * Date: Wed, 12 Feb 1997 10:45:55 +0100 (MET)
       *
       * Ignore SIGINT and SIGQUIT to avoid recursive calls.
       */
      dfl_handler.sa_handler = SIG_IGN;
      sigaction(SIGINT, &dfl_handler, NULL);
      sigaction(SIGQUIT, &dfl_handler, NULL);

      /* ask the user what to do */

      if (confirmExit())
      {
              freeResources();        /* do a nice exit */
              exit(EXIT_FAILURE);
      }
      doRepaint++;

      /* re-enable SIGINT and SIGQUIT */

      dfl_handler.sa_handler = sigHandler;
      sigaction(SIGINT, &dfl_handler, NULL);
      sigaction(SIGQUIT, &dfl_handler, NULL);
      break;
    case SIGTSTP:
      /*
       * called when the user presses ctrl-z
       */
      TScreen::suspend();
      LOG("process stopped");

      /* use the default handler for SIGTSTP */

      dfl_handler.sa_handler = SIG_DFL;
      sigaction(SIGTSTP, &dfl_handler, NULL);
      raise(SIGTSTP);         /* stop the process */
      break;
  }
}

/*
 * INTERNAL
 *
 * Load X resource overrides from the specified file.
 */
static void
loadXResources(const char *filename)
{
  XrmDatabase xdb;
  XrmValue value;
  char *type;
  int res, i;

  XrmInitialize();

  xdb = XrmGetFileDatabase(filename);
  if (xdb == NULL) {
    LOG("Couldn't open Xdefaults file '%s'.", filename);
    return;
  }

  for (i = 0; i < TvisionResources_count; i++) {
    res = XrmGetResource(xdb, TvisionResources[i].name,
                         TvisionResources[i].cls, &type, &value);
    if (res != 0)
      *TvisionResources[i].value = value.addr;
  }
}

static void
setupGCs(Display *disp, Window win, Font font)
{
	int i, j;
	XGCValues gcv;
	XColor dummy, foreground[16], background[8];
	Colormap colormap;

	colormap = DefaultColormap(disp,  DefaultScreen(disp));

	/*
	 * Allocate the eight background colors.
	 */	
	for (i = 0; i < 8; i++) {
		if (XAllocNamedColor(x_disp, colormap, bgColor[i],
			&background[i], &dummy) == 0)
			error("Couldn't allocate color %s", bgColor[i]);
	}

	/*
	 * Allocate the 16 foreground colors.
	 */
	for (i = 0; i < 16; i++) {
		if (XAllocNamedColor(x_disp, colormap, fgColor[i],
			&foreground[i], &dummy) == 0)
			error("Couldn't allocate color %s", fgColor[i]);
	}

	/*
	 * Create a unique graphics context for each combination of
	 * background and foreground colors.
	 */
	characterGC = new GC[256];

	gcv.font = font;

	for (i = 0; i < 16; i++) {
		for (j = 0; j < 16; j++) {
			gcv.foreground = foreground[j].pixel;
			gcv.background = background[i%8].pixel;
			characterGC[i * 16 + j] = XCreateGC(
				disp,
				win,
				GCForeground | GCBackground | GCFont,
				&gcv
			);
		}
	}
}

/*
 * CLASS FUNCTIONS
 */

/*
 * TScreen constructor.
 */

TScreen::TScreen()
{
        char *p = getenv("TVLOG");
        if (p != NULL && *p != '\0')
        {
                logfp = fopen(p, "w");
                LOG("using environment variable TVLOG=%s", p);
        }
        env[0] = '\0';
        if ((p = getenv("TVOPT")) != NULL)
        {
                LOG("using environment variable TVOPT=%s", p);
                for (char *d = env; *p != '\0'; p++) *d++ = tolower(*p);
        }

	p = getenv("DISPLAY");
	if (p == NULL)
		error("No DISPLAY defined.\n");

        /*
	 * Connect to X server.
	 */
	x_disp = XOpenDisplay(p);
	if (x_disp == NULL)
		error("Couldn't open display '%s'.\n", p);

	/*
	 * Read in X resource overrides.
	 */
	p = getenv("HOME");
	if (p != NULL) {
		char rdbFilename[PATH_MAX];
		snprintf(rdbFilename, sizeof(rdbFilename), "%s/.Xdefaults", p);
		loadXResources(rdbFilename);
	}
		
	/*
	 * Load the screen font.
	 */
	XFontStruct *theFont = XLoadQueryFont(x_disp, fontName);
	if (theFont == NULL)
		error("Can't load font '%s'.", fontName);

	/*
	 * Log a warning if the font isn't fixed-width.
	 */
	if (theFont->min_bounds.width != theFont->max_bounds.width)
		warning("Font '%s' is not fixed-width.", fontName);

	fontAscent = theFont->ascent;
	fontDescent = theFont->descent;
	fontHeight = fontAscent + fontDescent;
	fontWidth = theFont->max_bounds.width;
	Font fid = theFont->fid;

	/*
	 * We're done with the font info.
	 */
	XFreeFontInfo(NULL, theFont, 0);

        /*
	 * Negotiate window size with user settings and our defaults.
	 */
	XSizeHints *sh;

	if ((sh = XAllocSizeHints()) == NULL)
		error("alloc");

	sh->min_width = 4 * fontWidth;
	sh->max_width = maxViewWidth * fontWidth;
	sh->min_height = 4 * fontHeight;
	sh->max_height = 80 * fontHeight;
	sh->width_inc = fontWidth;
	sh->height_inc = fontHeight;
	sh->flags = (PSize | PMinSize | PResizeInc);

	char default_geometry[80];

	snprintf(default_geometry, sizeof(default_geometry),
		"%dx%d+%d+%d", DEFAULT_GEOMETRY_WIDTH,
		DEFAULT_GEOMETRY_HEIGHT, 0, 0);

	int geometryFlags = XGeometry(x_disp, DefaultScreen(x_disp),
		geometry, default_geometry, BORDER_WIDTH, fontWidth,
		fontHeight, 0, 0, &sh->x, &sh->y, &sh->width, &sh->height);

	if (geometryFlags & (XValue | YValue))
		sh->flags |= USPosition;
	if (geometryFlags & (WidthValue | HeightValue))
		sh->flags |= USSize;

	sh->width *= fontWidth;
	sh->height *= fontHeight;

	/*
	 * Create our window.
	 */
	x_win = XCreateSimpleWindow(x_disp, DefaultRootWindow(x_disp),
		sh->x, sh->y, sh->width, sh->height, BORDER_WIDTH,
		BlackPixel(x_disp, DefaultScreen(x_disp)),
		WhitePixel(x_disp, DefaultScreen(x_disp)));

	/*
	 * Setup class hint for the window manager.
	 */
	XClassHint *clh;

	if ((clh = XAllocClassHint()) == NULL)
		error("alloc");

	clh->res_name = APP_NAME;
	clh->res_class = APP_CLASS;

	/*
	 * Setup hints to the window manager.
	 */
	XWMHints *wmh;

	if ((wmh = XAllocWMHints()) == NULL)
		error("alloc");

	wmh->flags = (InputHint | StateHint);
	wmh->input = True;
	wmh->initial_state = NormalState;

	XTextProperty WName, IName;
	char *app_name = APP_NAME;

	if (XStringListToTextProperty(&app_name, 1, &WName) == 0)
		error("Error creating text property");
	if (XStringListToTextProperty(&app_name, 1, &IName) == 0)
		error("Error creating text property");

	/*
	 * Send all of these hints to the X server.
	 */
	XSetWMProperties(x_disp, x_win, &WName, &IName, NULL, 0,
		sh, wmh, clh);

	/*
	 * Set the window's bit gravity so that resize events keep the
	 * upper left corner contents.
	 */
	XSetWindowAttributes wa;

	wa.bit_gravity = NorthWestGravity;
	wa.backing_store = WhenMapped;
	wa.event_mask = ExposureMask|KeyPressMask|ButtonPressMask|
	                ButtonReleaseMask|ButtonMotionMask|
	                KeymapStateMask|ExposureMask|
		        StructureNotifyMask;

	XChangeWindowAttributes(x_disp, x_win,
		CWBitGravity|CWBackingStore|CWEventMask, &wa);

        screenWidth = sh->width / fontWidth;
        screenHeight = sh->height / fontHeight;

	XFree(wmh);
	XFree(clh);
	XFree(sh);

	/*
	 * Setup the 128 graphics contexts we require to draw
	 * color text.
	 */
	setupGCs(x_disp, x_win, fid);

	/*
	 * Setup the events we're interested in receiving.
	 */
#if 0
	XSelectInput(x_disp, x_win, ExposureMask|KeyPressMask|ButtonPressMask|
	                            ButtonReleaseMask|ButtonMotionMask|
	                            KeymapStateMask|ExposureMask|
				    StructureNotifyMask);
#endif

	/*
	 * Cause the window to be displayed.
	 */
	XMapWindow(x_disp, x_win);

        LOG("screen size is %dx%d", screenWidth, screenHeight);
        screenBuffer = new ushort[screenWidth * screenHeight];

        /* internal stuff */
	expose_region = XCreateRegion();

	/* setup file descriptors */

	x_disp_fd = ConnectionNumber(x_disp);

	FD_ZERO(&TScreen::fdSetRead);
	FD_SET(x_disp_fd, &TScreen::fdSetRead);

        curX = curY = 0;
        cursor_displayed = true;
        currentTime = doRepaint = evLength = 0;
        evIn = evOut = &evQueue[0];
        msAutoTimer.stop();
        msOldButtons = 0;
        wakeupTimer.start(DELAY_WAKEUP);

        /* catch useful signals */

        struct sigaction dfl_handler;

        dfl_handler.sa_handler = sigHandler;
        sigemptyset(&dfl_handler.sa_mask);
        dfl_handler.sa_flags = SA_RESTART;

        sigaction(SIGALRM, &dfl_handler, NULL);
        sigaction(SIGCONT, &dfl_handler, NULL);
        sigaction(SIGINT, &dfl_handler, NULL);
        sigaction(SIGQUIT, &dfl_handler, NULL);
        sigaction(SIGTSTP, &dfl_handler, NULL);
        sigaction(SIGWINCH, &dfl_handler, NULL);

        /* generates a SIGALRM signal every DELAY_SIGALRM ms */

        struct itimerval timer;
        timer.it_interval.tv_usec = timer.it_value.tv_usec =
                DELAY_SIGALRM * 1000;
        timer.it_interval.tv_sec = timer.it_value.tv_sec = 0;
        setitimer(ITIMER_REAL, &timer, NULL);
}

/*
 * TScreen destructor.
 */

TScreen::~TScreen()
{
  freeResources();
}

void TScreen::resume()
{
  doRepaint++;
}

void TScreen::suspend()
{
}

static bool find_event_in_queue(bool keyboard, TEvent *ev)
{

  TEvent *e = evOut;
  int i;
  for ( i=0; i < evLength; i++, e++ )
  {
    if ( e >= &evQueue[eventQSize] ) e = evQueue;
    if ( keyboard )
    {
      if ( e->what == evKeyDown )
        break;
    }
    else
    {
      if ( (e->what & evMouse) != 0 )
        break;
    }
  }
  if ( i == evLength )
    return false;
  *ev = *e;
  evLength--;
  while ( i < evLength )
  {
    TEvent *next = e + 1;
    if ( next >= &evQueue[eventQSize] ) next = evQueue;
    *e = *next;
    e = next;
  }
  return true;
}

void TEvent::_getKeyEvent(void)
{
  if ( !find_event_in_queue(true, this) )
  {
    TScreen::get_key_mouse_event(*this);
    if ( what != evKeyDown )
    {
      TScreen::putEvent(*this);
      what = evNothing;
    }
  }
}

void TEventQueue::getMouseEvent(TEvent &event)
{
//  if ( !find_event_in_queue(true, &event) )
  {
    TScreen::get_key_mouse_event(event);
    if ( (event.what & evMouse) == 0 )
    {
      TScreen::putEvent(event);
      event.what = evNothing;
    }
  }
}

/*
 * Gets an event from the queue.
 */

void TScreen::getEvent(TEvent &event)
{
  event.what = evNothing;
  if (doRepaint > 0)
  {
    doRepaint = 0;
    event.message.command = cmSysRepaint;
    event.what = evCommand;
  }
  else if (evLength > 0)  /* handles pending events */
  {
    evLength--;
    event = *evOut;
    if (++evOut >= &evQueue[eventQSize]) evOut = &evQueue[0];
  }
  else if (msAutoTimer.isExpired())
  {
    msAutoTimer.start(DELAY_AUTOCLICK_NEXT);
    event = msev;
    event.what = evMouseAuto;
  }
  else if (wakeupTimer.isExpired())
  {
    wakeupTimer.start(DELAY_WAKEUP);
    event.message.command = cmSysWakeup;
    event.what = evCommand;
  }
  else
  {
    get_key_mouse_event(event);
  }
//  if ( event.what != evNothing )
//    printf("event: %d (%d,%d)\n", event.what, event.mouse.where.x, event.mouse.where.y);
}

/*
 * Generates a beep.
 */

void TScreen::makeBeep()
{
  XBell(x_disp, 0);
}

/*
 * Puts an event in the queue.  If the queue is full the event will be
 * discarded.
 */

void TScreen::putEvent(TEvent &event)
{
        if (evLength < eventQSize)
        {
                evLength++;
                *evIn = event;
                if (++evIn >= &evQueue[eventQSize]) evIn = &evQueue[0];
        }
}

/*
 * Hides or shows the cursor.
 */

void TScreen::drawCursor(int show)
{
}

/*
 * Hides or shows the mouse pointer.
 */

void TScreen::drawMouse(int show)
{
  // we do not draw the mouse cursor ourselves
}

/*
 * Moves the cursor to another place.
 */

void TScreen::moveCursor(int x, int y)
{
  curX = x;
  curY = y;
}

/*
 * Draws a line of text on the screen.
 */

void TScreen::writeRow(int dst, ushort *src, int len)
{
  int x, y, code, color, current_color;
  char chunkBuf[1024]; // Holds strings of characters that can be drawn
                       // with the same attribute.
  char *chunk_p;

  x = dst % TScreen::screenWidth;
  y = dst / TScreen::screenWidth;

  while (len > 0) {
    current_color = (*src & 0xff00) >> 8;
    chunk_p = chunkBuf;

    //
    // Gather up the longest run of characters from the current position
    // that have the same attribute. 
    //
    do {
      color = (*src & 0xff00) >> 8; /* color code */
      code = *src & 0xff;           /* character code */
    
      if (color == current_color ||
          // If printing a space character, only the background color need
          // match.
          (code == 0x20 && (color & 0x70) == (current_color & 0x70))) {
        *(chunk_p++) = code;
        src++;
      } else if (code == 0xdb &&
                 (color & 0x0F) == ((current_color & 0x70) >> 4)) {
        // If printing a solid block character and its foreground matches the
        // current background, print a space.
        *(chunk_p++) = ' ';
        src++;
      } else {
        // Color changed.  Stop collecting characters and proceed to drawing
        // what we have.
        break;
      }
    } while (--len > 0);

    // We've gathered the largest block of characters with the same
    // attributes that we can.  Now draw them.
    XDrawImageString(x_disp, x_win, characterGC[current_color],
                     x * fontWidth, y * fontHeight + fontAscent,
                     chunkBuf, chunk_p - chunkBuf);
#ifdef DEBUG_X
    XFlush(x_disp);
#endif

    x += chunk_p - chunkBuf;
  }
}
// x_disp, x_win, colorGC, fontWidth, fontHeight, fontAscent


/*
 * Expands a path into its directory and file components.
 */

void expandPath(const char *path, char *dir, char *file)
{
        char *tag = strrchr(path, '/');

        /* the path is in the form /dir1/dir2/file ? */

        if (tag != NULL)
        {
                strcpy(file, tag + 1);
                strncpy(dir, path, tag - path + 1);
                dir[tag - path + 1] = '\0';
        }
        else
        {
                /* there is only the file name */

                strcpy(file, path);
                dir[0] = '\0';
        }
}

void fexpand(char *path)
{
        char dir[PATH_MAX];
        char file[PATH_MAX];
        char oldPath[PATH_MAX];

        expandPath(path, dir, file);
        getcwd(oldPath, sizeof(oldPath));
        chdir(dir);
        getcwd(dir, sizeof(dir));
        chdir(oldPath);
        if (strcmp(dir, "/") == 0) sprintf(path, "/%s", file);
        else sprintf(path, "%s/%s", dir, file);
}

void TEventQueue::mouseInt() { /* no mouse... */ }

void THWMouse::resume()
{
  LOG("RESUME MOUSE\n");
}

void THWMouse::suspend()
{
  LOG("SUSPEND MOUSE\n");
}

void TV_CDECL THWMouse::show()
{
  LOG("SHOW MOUSE\n");
}

void TV_CDECL THWMouse::hide()
{
  LOG("HIDE MOUSE\n");
}

#endif // __LINUX__