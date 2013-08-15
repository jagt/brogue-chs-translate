/*
 *  sdl-display.h
 *
 *  Created by Matt Kimball.
 *  Copyright 2013. All rights reserved.
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

#ifndef SDL_DISPLAY_H
#define SDL_DISPLAY_H

#ifdef __APPLE__
#include <cairo.h>
#else
#include <cairo/cairo.h>
#endif
#include <librsvg/rsvg.h>

#include <SDL/SDL.h>

#ifdef __APPLE__
#include <SDL_ttf/SDL_ttf.h>
#else
#include <SDL/SDL_ttf.h>
#endif

#include "Display.h"
#include "sdl-svgset.h"

#define FRAME_TIME (1000 / 30)

typedef struct BROGUE_EFFECT_CLASS BROGUE_EFFECT_CLASS;
typedef struct BROGUE_DRAW_CONTEXT_STATE BROGUE_DRAW_CONTEXT_STATE;
typedef struct BROGUE_DEFERRED_DRAW BROGUE_DEFERRED_DRAW;

/*  The global display object for the app  */
struct BROGUE_DISPLAY
{
    int width, height;

    BROGUE_WINDOW *root_window;
    GHashTable *effect_classes;

    SDL_Surface *screen;
    SDL_Surface *render_glyph;
    TTF_Font *mono_font;
    TTF_Font *sans_font;
    int font_width, font_height, font_descent;
    SDL_SVGSET *svgset;
};

/*  A window of the display  */
struct BROGUE_WINDOW
{
    BROGUE_DISPLAY *display;
    BROGUE_WINDOW *parent;

    int x, y;
    int width, height;
    int visible;
    BROGUE_DRAW_COLOR color;
    BROGUE_DEFERRED_DRAW **draws;

    GPtrArray *children;
    GPtrArray *contexts;
};

/*  The drawing stated embedded in a draw context  */
struct BROGUE_DRAW_CONTEXT_STATE
{
    int tile_enable;
    int proportional_enable;
    int wrap_enable;
    int justify_mode;

    int wrap_left, wrap_right;

    int is_foreground_blended;
    BROGUE_DRAW_COLOR foreground;
    BROGUE_DRAW_COLOR blended_foreground[4];

    int is_background_blended;
    BROGUE_DRAW_COLOR background;
    BROGUE_DRAW_COLOR blended_background[4];

    BROGUE_DRAW_COLOR foreground_multiplier;
    BROGUE_DRAW_COLOR foreground_addition;
    BROGUE_DRAW_COLOR foreground_average_target;
    float foreground_average_weight;

    BROGUE_DRAW_COLOR background_multiplier;
    BROGUE_DRAW_COLOR background_addition;
    BROGUE_DRAW_COLOR background_average_target;
    float background_average_weight;

    int tab_stop_count;
    int *tab_stops;

    BROGUE_EFFECT *effect;
};

/*  Deferred draw instances have an associated drawing and free function  */
typedef int (*BROGUE_DEFERRED_DRAW_FUNC)(
    BROGUE_DISPLAY *display, void *param, SDL_Surface *surface);
typedef void (*BROGUE_DEFERRED_FREE_FUNC)(void *param);

/*  Effect classes have a few associated virtual methods  */
typedef void *(*BROGUE_EFFECT_NEW_FUNC)(void);
typedef void (*BROGUE_EFFECT_DESTROY_FUNC)(void *effect_state);
typedef int (*BROGUE_EFFECT_SET_PARAM_FUNC)(void *effect_state, void *param);
typedef BROGUE_DEFERRED_DRAW *(*BROGUE_EFFECT_DRAW_STRING_FUNC)(
    BROGUE_WINDOW *window, const BROGUE_DRAW_CONTEXT_STATE *context_state, 
    void *effect_state, int x, int y, const wchar_t *str);

/*  A custom way of drawing for special UI elements and effects  */
struct BROGUE_EFFECT_CLASS
{
    BROGUE_EFFECT_NEW_FUNC new_func;
    BROGUE_EFFECT_DESTROY_FUNC destroy_func;
    BROGUE_EFFECT_SET_PARAM_FUNC set_param_func;
    BROGUE_EFFECT_DRAW_STRING_FUNC draw_string_func;
};

/*  A package of data stored so that we can draw things on the screen
    when the frame buffer is refreshed.  */
struct BROGUE_DEFERRED_DRAW
{
    BROGUE_WINDOW *window;
    int ref_count;

    BROGUE_DEFERRED_DRAW_FUNC draw_func;
    BROGUE_DEFERRED_FREE_FUNC free_func;
    void *draw_param;
    
    int x, y;
    int width, height;
    int drawn;
};

/*  Utility functions exposed by the console  */
char *SdlConsole_getAppPath(const char *file);
char *SdlConsole_getResourcePath(const char *file);
void Sdl_undoPremultipliedAlpha(SDL_Surface *surface);

/*  Exposed internals of the display  */
int BrogueDisplay_colorToSDL(SDL_Surface *surface, BROGUE_DRAW_COLOR *color);
SDL_Color BrogueDisplay_colorToSDLColor(BROGUE_DRAW_COLOR *color);
void BrogueDisplay_setScreen(BROGUE_DISPLAY *display, SDL_Surface *screen);
void BrogueDisplay_prepareFrame(BROGUE_DISPLAY *display);
TTF_Font *BrogueDisplay_getFont(BROGUE_DISPLAY *display, int proportional);
void BrogueDisplay_getFontSize(BROGUE_DISPLAY *display, 
			       int *width, int *height);
void BrogueDisplay_setFonts(
    BROGUE_DISPLAY *display, TTF_Font *mono_font, TTF_Font *sans_font,
    int font_width, int font_height);
void BrogueDisplay_setSvgset(BROGUE_DISPLAY *display, SDL_SVGSET *svgset);
SDL_SVGSET *BrogueDisplay_getSvgset(BROGUE_DISPLAY *display);
int BrogueDisplay_registerEffectClass(
    BROGUE_DISPLAY *display,
    const wchar_t *name, 
    BROGUE_EFFECT_NEW_FUNC new_func,
    BROGUE_EFFECT_DESTROY_FUNC destroy_func,
    BROGUE_EFFECT_SET_PARAM_FUNC set_param_func,
    BROGUE_EFFECT_DRAW_STRING_FUNC draw_string_func);

void BrogueWindow_replaceCell(
    BROGUE_WINDOW *window, int x, int y, BROGUE_DEFERRED_DRAW *draw);
int BrogueWindow_draw(BROGUE_WINDOW *window, SDL_Surface *surface);

void BrogueEffect_ref(BROGUE_EFFECT *effect);
void BrogueEffect_unref(BROGUE_EFFECT *effect);

BROGUE_DEFERRED_DRAW *BrogueDeferredDraw_create(
    BROGUE_WINDOW *window,
    BROGUE_DEFERRED_DRAW_FUNC draw_func, 
    BROGUE_DEFERRED_FREE_FUNC free_func,
    void *draw_param, 
    int x, int y, int width, int height);
void BrogueDeferredDraw_ref(BROGUE_DEFERRED_DRAW *draw);
void BrogueDeferredDraw_unref(BROGUE_DEFERRED_DRAW *draw);
int BrogueDeferredDraw_draw(
    BROGUE_DISPLAY *display, BROGUE_DEFERRED_DRAW *draw, SDL_Surface *surface);

void BrogueEffects_registerAll(BROGUE_DISPLAY *display);

void BrogueGraphic_ref(BROGUE_GRAPHIC *graphic);
void BrogueGraphic_unref(BROGUE_GRAPHIC *graphic);

#endif
