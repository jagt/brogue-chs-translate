/*
 *  sdl-platform.c
 *
 *  Created by Matt Kimball.
 *  Copyright 2012. All rights reserved.
 *  
 *  This file is part of Brogue.
 *
 *  Brogue is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Brogue is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Brogue.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <iconv.h>

#include "platform.h"
#include "IncludeGlobals.h"

#include "sdl-display.h"
#include "sdl-keymap.h"
#include "sdl-svgset.h"

#define MONO_FONT_FILENAME "Andale_Mono.ttf"
#define SANS_FONT_FILENAME "Zfull-GB.ttf"
#define SVG_PATH "svg"

#define SCREEN_WIDTH 100
#define SCREEN_HEIGHT 34
#define TARGET_ASPECT_RATIO (2.0 / 3.0)
#define MIN_FONT_SIZE 4
#define MAX_FONT_SIZE 128

extern playerCharacter rogue;
extern short brogueFontSize;

/*  We store characters drawn to the console so that we can redraw them
    when scaling the font or switching to full-screen.  */
struct SDL_CONSOLE_CHAR
{
    uchar c;
    SDL_Color fg, bg;
    int flags;
};
typedef struct SDL_CONSOLE_CHAR SDL_CONSOLE_CHAR;

/*  We store font metrics for all potential font sizes so that we can
    intelligently select font sizes for the user.  */
struct SDL_CONSOLE_FONT_METRICS
{
    unsigned point_size;
    int width;
    int height;
    int descent;
};
typedef struct SDL_CONSOLE_FONT_METRICS SDL_CONSOLE_FONT_METRICS;

/*  We store important information about the state of the console in 
    an instance of SDL_CONSOLE.  */
struct SDL_CONSOLE
{
    char *bundle_path;
    char *app_path;

    int width;
    int height;

    int margin_x, margin_y;

    int fullscreen;
    SDL_Surface *screen;
    SDL_Event queued_event;
    int is_queued_event_present;
    rogueEvent queued_rogue_event;
    int is_queued_rogue_event_present;

    BROGUE_DISPLAY *display;

    TTF_Font *mono_font;
    TTF_Font *sans_font;
    SDL_CONSOLE_FONT_METRICS metrics;
    SDL_SVGSET *svgset;

    int mouse_x, mouse_y;
    SDL_KEYMAP *keymap;

    int screen_dirty;
    SDL_CONSOLE_FONT_METRICS *all_metrics;

    unsigned last_frame_timestamp;
    int enable_frame_time_display;
    BROGUE_WINDOW *frame_time_window;
    BROGUE_DRAW_CONTEXT *frame_time_context;
    unsigned frame_count;
    unsigned accumulated_frame_time;
};
typedef struct SDL_CONSOLE SDL_CONSOLE;

static SDL_CONSOLE console;

/*  Return a path relative to the executable location.  While we are
    in game we change to the save game directory, so we can't use
    relative paths.  */
char *SdlConsole_getAppPath(const char *file)
{
    char *path = malloc(strlen(file) + strlen(console.app_path) + 16); // add more padding just for safety
    if (path == NULL)
    {
	return NULL;
    }

    sprintf(path, "%s/%s", console.app_path, file);
    return path;
}

/*  If we are launched from Finder under OS X, remember the bundle name
    so we can open resources.  */
void SdlConsole_setBundlePath(char *bundle_path)
{
    char *pkg = g_path_get_basename(bundle_path);
    char *resources = "/Contents/Resources/";

    console.bundle_path = malloc(strlen(pkg) + strlen(resources) + 1);
    strcpy(console.bundle_path, pkg);
    strcat(console.bundle_path, resources);

    free(pkg);
}

char *SdlConsole_getResourcePath(const char *file)
{
    char *ret, *res;
    char *bundle = console.bundle_path;

    if (bundle == NULL)
    {
	bundle = "";
    }

    res = malloc(strlen(file) + strlen(bundle) + 1);
    if (res == NULL)
    {
	return NULL;
    }

    strcpy(res, bundle);
    strcat(res, file);

    ret = SdlConsole_getAppPath(res);
    free(res);

    return ret;
}

SDL_CONSOLE_FONT_METRICS SdlConsole_getFontSizeForWindow(
    int target_width, int target_height)
{
    SDL_CONSOLE_FONT_METRICS metrics;
    int size;

    size = MIN_FONT_SIZE;
    while (size < MAX_FONT_SIZE)
    {
	SDL_CONSOLE_FONT_METRICS *next_metrics = 
	    &console.all_metrics[size + 1 - MIN_FONT_SIZE];
	
	if (next_metrics->width * SCREEN_WIDTH > target_width)
	{
	    break;
	}
	if (next_metrics->height * SCREEN_HEIGHT > target_height)
	{
	    break;
	}
	
	size++;
    }

    metrics = console.all_metrics[size - MIN_FONT_SIZE];
    while ((metrics.width + 1) * SCREEN_WIDTH <= target_width)
    {
	metrics.width++;
    }
    while ((metrics.height + 1) * SCREEN_HEIGHT <= target_height)
    {
	metrics.height++;
    }
    
    return metrics;
}

/*  Choose the initial font size by trying to fill most of the screen.  */
SDL_CONSOLE_FONT_METRICS SdlConsole_getFontSizeForScreen(void)
{
    const SDL_VideoInfo *video_info;
    int target_width, target_height;

    video_info = SDL_GetVideoInfo();
    target_width = video_info->current_w * 15 / 16;
    target_height = video_info->current_h * 15 / 16;

    return SdlConsole_getFontSizeForWindow(target_width, target_height);
}

/*  Extract metric information for all font sizes we might use to display.  */
void SdlConsole_generateFontMetrics(void)
{
    int size;

    for (size = MIN_FONT_SIZE; size <= MAX_FONT_SIZE; size++)
    {
	char *font_path;
	TTF_Font *font;
	SDL_CONSOLE_FONT_METRICS *metrics = 
	    &console.all_metrics[size - MIN_FONT_SIZE];
	double aspect_ratio;

	font_path = SdlConsole_getResourcePath(MONO_FONT_FILENAME);
	font = TTF_OpenFont(font_path, size);
	free(font_path);
	if (font == NULL)
	{
	    printf("%s\n", TTF_GetError());
	    exit(1);
	}

	metrics->point_size = size;
	metrics->height = TTF_FontAscent(font) - TTF_FontDescent(font) + 1;
	metrics->descent = TTF_FontDescent(font);
	if (TTF_SizeText(font, "M", &metrics->width, NULL))
	{
	    printf("%s\n", TTF_GetError());
	    exit(1);
	}

	TTF_CloseFont(font);

	/*  We'll target the aspect ratio of the SVG tile set, extending
	    the dimensions of the font as necessary to match.  */
	aspect_ratio = (double)metrics->width / (double)metrics->height;
	if (aspect_ratio < TARGET_ASPECT_RATIO)
	{
	    metrics->width = (int)(TARGET_ASPECT_RATIO * metrics->height);
	}
	else
	{
	    metrics->height = (int)(metrics->width / TARGET_ASPECT_RATIO);
	}
    }

    size = brogueFontSize;
    if (size < MIN_FONT_SIZE || size >= MAX_FONT_SIZE)
    {
	size = -1;
    }

    if (size == -1)
    {
	console.metrics = SdlConsole_getFontSizeForScreen();
    }
    else
    {
	console.metrics = console.all_metrics[size - MIN_FONT_SIZE];
    }
}

/*  Use the highest resolution fullscreen video mode available  */
void SdlConsole_setFullscreenMode(void) {
    SDL_Rect **resolutions = SDL_ListModes(NULL, SDL_FULLSCREEN);

    if (resolutions != NULL && resolutions != (SDL_Rect **)-1)
    {
	int resolution_ix = 0;

	while (console.screen == NULL && resolutions[resolution_ix])
	{
	    SDL_Rect *res = resolutions[resolution_ix++];
	    int w = res->w;
	    int h = res->h;

	    console.screen = SDL_SetVideoMode(
		w, h, 32, 
		SDL_SWSURFACE | SDL_ANYFORMAT | SDL_FULLSCREEN);

	    if (console.screen != NULL)
	    {
		console.width = w;
		console.height = h;

		console.metrics = SdlConsole_getFontSizeForWindow(w, h);

		console.margin_x = 
		    (w - console.metrics.width * SCREEN_WIDTH) / 2;
		console.margin_y = 
		    (h - console.metrics.height * SCREEN_HEIGHT) / 2;
	    }
	}
    }
}

/*  We need to reload the font and adjust the video mode when changing
    font size.  The fullscreen enable/disable path goes through here, too.  */
void SdlConsole_scaleFont(boolean set_dimensions)
{
    char *font_path;
    char *svg_path;

    BrogueDisplay_setScreen(console.display, NULL);
    BrogueDisplay_setFonts(console.display, NULL, NULL, 0, 0);
    BrogueDisplay_setSvgset(console.display, NULL);

    if (console.mono_font != NULL)
    {
	TTF_CloseFont(console.mono_font);
    }
    if (console.sans_font != NULL)
    {
	TTF_CloseFont(console.sans_font);
    }
    if (console.svgset != NULL)
    {
	SdlSvgset_free(console.svgset);
    }

    console.screen = NULL;
    if (console.fullscreen && set_dimensions)
    {
	SdlConsole_setFullscreenMode();
    }
    
    font_path = SdlConsole_getResourcePath(MONO_FONT_FILENAME);
    console.mono_font = TTF_OpenFont(font_path, console.metrics.point_size);
    free(font_path);
    if (console.mono_font == NULL)
    {
	printf("%s\n", TTF_GetError());
	exit(1);
    }

    font_path = SdlConsole_getResourcePath(SANS_FONT_FILENAME);
    console.sans_font = TTF_OpenFont(font_path, console.metrics.point_size);
    free(font_path);
    if (console.sans_font == NULL)
    {
	printf("%s\n", TTF_GetError());
	exit(1);
    }

    svg_path = SdlConsole_getResourcePath(SVG_PATH);
    console.svgset = SdlSvgset_alloc(svg_path, 
				     console.metrics.width,
				     console.metrics.height);
    free(svg_path);
    if (console.svgset == NULL)
    {
	printf("Failure to load SVG set\n");
	exit(1);
    }

    /*  If fullscreen fails or isn't set, we'll fall back to windowed.  */
    if (console.screen == NULL)
    {
	if (set_dimensions)
	{
	    console.width = console.metrics.width * SCREEN_WIDTH;
	    console.height = console.metrics.height * SCREEN_HEIGHT;

	    console.margin_x = 0;
	    console.margin_y = 0;
	}

	console.screen = SDL_SetVideoMode(console.width, console.height, 32,
					  SDL_SWSURFACE | SDL_ANYFORMAT |
					  SDL_RESIZABLE);
    }
    
    if (console.screen == NULL)
    {
	printf("Failure to set video mode\n");
	exit(1);
    }

    BrogueDisplay_setScreen(console.display, console.screen);
    BrogueDisplay_setFonts(console.display, 
			   console.mono_font, console.sans_font,
			   console.metrics.width, console.metrics.height);
    BrogueDisplay_setSvgset(console.display, console.svgset);
}

/*  Flip the screen buffer if anything has been drawn since the last flip.  */
void SdlConsole_refresh(void)
{
    BrogueDisplay_prepareFrame(console.display);
    SDL_Flip(console.screen);
}

/*  
    Display the frame generation time average in the upper right 
    corner of the display, so that we can measure performance while
    developing.  
*/
void SdlConsole_displayFrameTime(char *str)
{
    if (console.frame_time_window != NULL)
    {
	BrogueDrawContext_drawAsciiString(
	    console.frame_time_context, 1, 0, str);
    }
}

/*
    If we have generated the previous frame faster than 30 FPS, then 
    sleep for the remainder of the frame to avoid hogging the CPU and
    draining batteries unnecessarily.
*/
void SdlConsole_waitForNextFrame(int wait_time)
{
    int now, delay, frame_time;

    now = SDL_GetTicks();

    if (console.last_frame_timestamp == 0)
    {
	console.last_frame_timestamp = now;
    }

    frame_time = now - console.last_frame_timestamp;
    if (console.enable_frame_time_display)
    {
	double average_frame_time;
	char frame_time_str[32];

	console.accumulated_frame_time += frame_time;
	if (console.frame_count % 30 == 0)
	{
	    average_frame_time = 
		(double)console.accumulated_frame_time / 30.0;

	    snprintf(frame_time_str, sizeof(frame_time_str), 
		     "%6.2f ms", average_frame_time);
	    SdlConsole_displayFrameTime(frame_time_str);
	    
	    console.accumulated_frame_time = 0;
	}
    }

    delay = wait_time - frame_time;
    if (delay > 0)
    {
	SDL_Delay(wait_time); 
    }

    console.last_frame_timestamp = SDL_GetTicks();
    console.frame_count++;
}

/*  Return true if mouse motion is immediately pending in the event queue.  */
boolean SdlConsole_isMouseMotionPending(void)
{
    if (!console.is_queued_event_present)
    {
	if (!SDL_PollEvent(&console.queued_event))
	{
	    return 0;
	}

	console.is_queued_event_present = 1;
    }

    return console.queued_event.type == SDL_MOUSEMOTION;
}

/*  Resize the displayed font for a new window size  */
void SdlConsole_resize(int w, int h)
{
    console.metrics = SdlConsole_getFontSizeForWindow(w, h);

    console.width = w;
    console.height = h;

    SdlConsole_scaleFont(false);

    console.margin_x = (w - console.metrics.width * SCREEN_WIDTH) / 2;
    console.margin_y = (h - console.metrics.height * SCREEN_HEIGHT) / 2;
}

/*  Increase the font size.  We check the width because bumping up the
    height but keeping the same width feels a bit too incremental.  */
void SdlConsole_increaseFont(void)
{
    int size = console.metrics.point_size;
    while (size < MAX_FONT_SIZE)
    {
	if (console.all_metrics[size - MIN_FONT_SIZE].width
	    != console.metrics.width)
	{
	    break;
	}
	
	size++;
    }
    console.metrics = console.all_metrics[size - MIN_FONT_SIZE];

    console.fullscreen = 0;
    SdlConsole_scaleFont(true);
}

/*  Decrease the font size.  */
void SdlConsole_decreaseFont(void)
{
    int size = console.metrics.point_size;
    while (size > MIN_FONT_SIZE)
    {
	if (console.all_metrics[size - MIN_FONT_SIZE].width
	    != console.metrics.width)
	{
	    break;
	}
	
	size--;
    }
    console.metrics = console.all_metrics[size - MIN_FONT_SIZE];

    console.fullscreen = 0;
    SdlConsole_scaleFont(true);
}

/*  Capture a screenshot, assigning a filename based of the screenshots
    which already exist in the running directory.  */
void SdlConsole_captureScreenshot(void)
{
    int screenshot_num = 0;
    char screenshot_file[32];
	
    while (screenshot_num <= 999)
    {
	FILE *f;

	snprintf(screenshot_file, 32, "screenshot%03d.bmp", screenshot_num);
	f = fopen(screenshot_file, "rb");
	if (f == NULL)
	{
	    break;
	}
	fclose(f);

	screenshot_num++;
    }

    SDL_SaveBMP(console.screen, screenshot_file);
}

/*  Handle console specific key responses.  */
int SdlConsole_processKey(SDL_KeyboardEvent *keyboardEvent, boolean textInput)
{
    SDLKey key;
    uchar unicode;
    int mod;

    SdlKeymap_translate(console.keymap, keyboardEvent);

    key = keyboardEvent->keysym.sym;
    mod = keyboardEvent->keysym.mod;
    unicode = keyboardEvent->keysym.unicode;

    /*  Some control characters we aren't interested in -- we'd rather
	have the alphabetic key.  */
    if (unicode < 27)
    {
	keyboardEvent->keysym.unicode = key;
    }

    if (key == SDLK_PAGEUP)
    {
	SdlConsole_increaseFont();
	SdlConsole_refresh();
	return 1;
    }
    if (key == SDLK_PAGEDOWN)
    {
	SdlConsole_decreaseFont();
	SdlConsole_refresh();
	return 1;
    }

    if (key == SDLK_PRINT || key == SDLK_F11)
    {
	SdlConsole_captureScreenshot();
	return 1;
    }
    if (key == SDLK_F12)
    {
	console.fullscreen = !console.fullscreen;
	SdlConsole_scaleFont(true);
	SdlConsole_refresh();
	return 1;
    }
    if (key == SDLK_f && (mod & KMOD_CTRL))
    {
	console.enable_frame_time_display = !console.enable_frame_time_display;
	if (console.enable_frame_time_display)
	{
	    BROGUE_WINDOW *root = BrogueDisplay_getRootWindow(console.display);

	    console.frame_time_window = BrogueWindow_open(
		root, COLS - 10, 0, 10, 1);
	    BrogueWindow_setColor(console.frame_time_window, windowColor);
	    console.frame_time_context = BrogueDrawContext_open(
		console.frame_time_window);
	    BrogueDrawContext_enableProportionalFont(
		console.frame_time_context, 1);
	}
	else
	{
	    BrogueWindow_close(console.frame_time_window);
	    console.frame_time_window = NULL;
	    console.frame_time_context = NULL;
	}

	return 1;
    }

    /*  
	We don't want characters such as shift to be interpreted in
        text input fields.  
    */
    if (key >= SDLK_NUMLOCK && key <= SDLK_COMPOSE)
    {
	return 1;
    }

    if (key == SDLK_BACKSPACE)
    {
	keyboardEvent->keysym.unicode = DELETE_KEY;
    }
    if (key == SDLK_UP || key == SDLK_KP8)
    {
	keyboardEvent->keysym.unicode = UP_KEY;
    }
    if (key == SDLK_LEFT || key == SDLK_KP4)
    {
	keyboardEvent->keysym.unicode = LEFT_KEY;
    }
    if (key == SDLK_RIGHT || key == SDLK_KP6)
    {
	keyboardEvent->keysym.unicode = RIGHT_KEY;
    }
    if (key == SDLK_DOWN || key == SDLK_KP2)
    {
	keyboardEvent->keysym.unicode = DOWN_KEY;
    }
    if (key == SDLK_KP7)
    {
	keyboardEvent->keysym.unicode = UPLEFT_KEY;
    }
    if (key == SDLK_KP9)
    {
	keyboardEvent->keysym.unicode = UPRIGHT_KEY;
    }
    if (key == SDLK_KP1)
    {
	keyboardEvent->keysym.unicode = DOWNLEFT_KEY;
    }
    if (key == SDLK_KP3)
    {
	keyboardEvent->keysym.unicode = DOWNRIGHT_KEY;
    }

    return 0;
}

/*  Return the queued SDL event, if we have one.  Otherwise, return
    the next event from SDL.  */
void SdlConsole_nextSdlEvent(SDL_Event *event, int wait_time)
{
    if (console.is_queued_event_present)
    {
	*event = console.queued_event;
	console.is_queued_event_present = 0;
    }
    else
    {
	int success = 0;

	while (!success)
	{
	    if (wait_time == -1)
	    {
		SdlConsole_refresh();
		success = SDL_WaitEvent(event);

		if (!success)
		{
		    printf("%s\n", SDL_GetError());
		}
	    }
	    else
	    {
		success = SDL_PollEvent(event);
		if (!success)
		{
		    shuffleTerrainColors(3, true);
		    SdlConsole_refresh();

		    SdlConsole_waitForNextFrame(wait_time);
		    
		    success = SDL_PollEvent(event);
		}
	    }
	}
    }
}

/*  Check for shift or ctrl held down.  */
boolean SdlConsole_modifierHeld(int modifier)
{
    int modstate = SDL_GetModState();

    if (modifier == 0)
    {
	return modstate & KMOD_SHIFT;
    }
    
    if (modifier == 1)
    {
	return modstate & KMOD_CTRL;
    }

    return 0;
}

/*  Get the number of milliseconds so far.  Useful for relative intervals, 
    but not as an absolute value.  */
int SdlConsole_getTicks(void)
{
    return SDL_GetTicks();
}

/*  Translate an SDL event to a Brogue event.  Returns true if the event
    is relevant, or false if it should be ignored.  */
int SdlConsole_translateEvent(
    rogueEvent *returnEvent, SDL_Event *event, boolean textInput)
{
    memset(returnEvent, 0, sizeof(rogueEvent));

    returnEvent->shiftKey = SdlConsole_modifierHeld(0);
    returnEvent->controlKey = SdlConsole_modifierHeld(1);

    if (event->type == SDL_KEYDOWN)
    {
	SDL_KeyboardEvent *keyboardEvent = (SDL_KeyboardEvent *)event;

	if (SdlConsole_processKey(keyboardEvent, textInput))
	{
	    return false;
	}

	returnEvent->eventType = KEYSTROKE;
	returnEvent->param1 = keyboardEvent->keysym.unicode;

	return true;
    }

    if (event->type == SDL_MOUSEMOTION)
    {
	if ((SDL_GetAppState() & SDL_APPINPUTFOCUS) == 0)
	{
	    return false;
	}

	if (SdlConsole_isMouseMotionPending())
	{
	    /*  If more mouse motion is already pending, drop this
		mouse motion event in favor of the next one.  */
	    return false;
	}

	SDL_MouseMotionEvent *mouseEvent = (SDL_MouseMotionEvent *)event;
	int mx = (mouseEvent->x - console.margin_x) 
	    / console.metrics.width;
	int my = (mouseEvent->y - console.margin_y) 
	    / console.metrics.height;

	returnEvent->eventType = MOUSE_ENTERED_CELL;
	returnEvent->param1 = mx;
	returnEvent->param2 = my;

	return true;
    }

    if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP)
    {
	SDL_MouseButtonEvent *buttonEvent = (SDL_MouseButtonEvent *)event;
	
	int mx = (buttonEvent->x - console.margin_x) 
	    / console.metrics.width;
	int my = (buttonEvent->y - console.margin_y) 
	    / console.metrics.height;

	if (buttonEvent->button == SDL_BUTTON_LEFT)
	{
	    returnEvent->eventType = 
		((event->type == SDL_MOUSEBUTTONDOWN) ? 
		 MOUSE_DOWN : MOUSE_UP);
	}
	else if (buttonEvent->button == SDL_BUTTON_RIGHT)
	{
	    returnEvent->eventType = 
		((event->type == SDL_MOUSEBUTTONDOWN) ? 
		 RIGHT_MOUSE_DOWN : RIGHT_MOUSE_UP);
	}
	else
	{
	    return false;
	}

	returnEvent->param1 = mx;
	returnEvent->param2 = my;

	return true;
    }

    if (event->type == SDL_VIDEORESIZE)
    {
	SDL_ResizeEvent *resize = (SDL_ResizeEvent *)event;
	
	SdlConsole_resize(resize->w, resize->h);

	return false;
    }

    if (event->type == SDL_QUIT)
    {
	rogue.gameHasEnded = 1;
	rogue.nextGame = NG_QUIT;
	    
	returnEvent->eventType = KEYSTROKE;
	returnEvent->param1 = ESCAPE_KEY;
	    
	return true;
    }

    return false;
}

/*  If we've got a queued SDL event, check to see if it is interesting
    by attempting to translate it to a Brogue event.  */
boolean SdlConsole_isQueuedEventInteresting()
{
    if (!console.is_queued_event_present)
    {
	return false;
    }

    console.is_queued_event_present = 0;
    if (SdlConsole_translateEvent(
	    &console.queued_rogue_event, &console.queued_event, true))
    {
	console.is_queued_rogue_event_present = 1;

	return true;
    }

    return false;
}

/*  Pause for a known amount of time.  Return true if an event happened.  */
boolean SdlConsole_pauseForMilliseconds(short milliseconds)
{
    int start_time, now;
    int pause_time;

    if (console.is_queued_rogue_event_present)
    {
	return true;
    }

    start_time = SDL_GetTicks();

    do
    {
	do
	{
	    if (SdlConsole_isQueuedEventInteresting())
	    {
		return true;
	    }

	    if (SDL_PollEvent(&console.queued_event))
	    {
		console.is_queued_event_present = 1;
	    }

	} while (console.is_queued_event_present);

	SdlConsole_refresh();

	now = SDL_GetTicks();
	pause_time = milliseconds - (now - start_time);

	if (pause_time > FRAME_TIME)
	{
	    pause_time = FRAME_TIME;
	}

	if (pause_time > 0)
	{
	    SdlConsole_waitForNextFrame(pause_time);
	}

    } while (pause_time > 0);

    return false;
}

/*  Wait for the next input Brogue needs to respond to, and return it.  */
void SdlConsole_nextKeyOrMouseEvent(rogueEvent *returnEvent, 
				    boolean textInput, 
				    boolean colorsDance)
{
    SDL_Event event;

    if (console.is_queued_rogue_event_present)
    {
	*returnEvent = console.queued_rogue_event;
	console.is_queued_rogue_event_present = false;

	return;
    }

    do
    {
	memset(&event, 0, sizeof(SDL_Event));

	SdlConsole_nextSdlEvent(&event,
				colorsDance ? FRAME_TIME : -1);

    } while(!SdlConsole_translateEvent(returnEvent, &event, textInput));
}

/*  Set the window icon.  */
void SdlConsole_setIcon(void)
{
    SDL_Surface *icon;

    SDL_WM_SetCaption("Brogue " BROGUE_VERSION_STRING, "Brogue");

    icon = SDL_LoadBMP("icon.bmp");
    if (icon == NULL)
    {
	return;
    }

    SDL_WM_SetIcon(icon, NULL);
}

/*  Remap a key, allocating a keymap as necessary, as the keymap translation
    starts before the platform game loop.  */
void SdlConsole_remap(const char *input_name, const char *output_name)
{
    int err;

    if (console.keymap == NULL)
    {
	console.keymap = SdlKeymap_alloc();
    }

    err = SdlKeymap_addTranslation(console.keymap, input_name, output_name);
    if (err == EINVAL)
    {
	printf("Error mapping '%s' -> '%s'\n", input_name, output_name);
    }
}

/*  Allocate the structures to be used by the console.  */
void SdlConsole_allocate(void)
{
    
    char app_dir[1024];
#ifdef _WIN32
    char utf8_dir[1024];    
    size_t outbytesleft, inbytesleft, conv_ret;
    const char *tin = app_dir;
    char *tout = utf8_dir;
#endif

    if (getcwd(app_dir, 1024) == NULL)
    {
	printf("Failed to get working directory\n");
	exit(1);
    }

#ifdef _WIN32
    // since we're targetting chs user, convert from gbk anyway
    // if all are ascii then nothing happens really
    inbytesleft = strlen(app_dir) + 1;
    outbytesleft = 1024;
    iconv_t cd = iconv_open("UTF-8", "GBK");
    conv_ret = iconv(cd, &tin, &inbytesleft, &tout, &outbytesleft);
    iconv_close(cd);
    memcpy(app_dir, utf8_dir, sizeof(char)*1024);
#endif

    console.app_path = malloc(strlen(app_dir) + 1);
    strcpy(console.app_path, app_dir);

    console.all_metrics = malloc(sizeof(SDL_CONSOLE_FONT_METRICS) *
				 (MAX_FONT_SIZE - MIN_FONT_SIZE + 1));
    if (console.keymap == NULL)
    {
	console.keymap = SdlKeymap_alloc();
    }

    if (console.all_metrics == NULL || console.keymap == NULL)
    {
	printf("Failed to allocate screen contents\n");
	exit(1);
    }

    memset(console.all_metrics, 0, 
	   sizeof(SDL_CONSOLE_FONT_METRICS) 
	   * (MAX_FONT_SIZE - MIN_FONT_SIZE + 1));
}

/*  Free the memory used by the console.  */
void SdlConsole_free(void)
{
    BrogueDisplay_close(console.display);
    console.display = NULL;
    
    free(console.all_metrics);
    console.all_metrics = NULL;

    SdlKeymap_free(console.keymap);
    console.keymap = NULL;

    if (console.svgset != NULL)
    {
	SdlSvgset_free(console.svgset);
	console.svgset = NULL;
    }

    if (console.mono_font != NULL)
    {
	TTF_CloseFont(console.mono_font);
	console.mono_font = NULL;
    }

    if (console.sans_font != NULL)
    {
	TTF_CloseFont(console.sans_font);
	console.sans_font = NULL;
    }
}

/*  The main entry point to the SDL-specific code.  */
void SdlConsole_gameLoop(void)
{
    int err;

    SdlConsole_allocate();

    err = SDL_Init(SDL_INIT_VIDEO);
    if (err)
    {
	printf("Failed to initialize SDL\n");
	exit(1);
    }
    atexit(SDL_Quit);

    err = TTF_Init();
    if (err)
    {
	printf("Failed to initialize SDL_TTF\n");
	exit(1);
    }

    brogue_display = BrogueDisplay_open(COLS, ROWS);
    console.display = brogue_display;

    SdlConsole_setIcon();
    SdlConsole_generateFontMetrics();
    SdlConsole_scaleFont(true);

    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(175, 30);

    rogueMain();

    SdlConsole_free();
}

/*  The function table used by Brogue to call into the SDL implementation.  */
struct brogueConsole sdlConsole = {
    SdlConsole_gameLoop,
    SdlConsole_pauseForMilliseconds,
    SdlConsole_nextKeyOrMouseEvent,
    NULL,
    SdlConsole_remap,
    SdlConsole_modifierHeld,
    SdlConsole_getTicks,
};
