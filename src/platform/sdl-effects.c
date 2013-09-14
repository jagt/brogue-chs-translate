/*
 *  sdl-effects.c
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

#include <errno.h>
#include <wchar.h>

#include "Rogue.h"
#include "DisplayEffect.h"
#include "sdl-display.h"

typedef struct PROGRESS_BAR PROGRESS_BAR;
typedef struct PROGRESS_BAR_DRAW PROGRESS_BAR_DRAW;
typedef struct BUTTON BUTTON;
typedef struct BUTTON_DRAW BUTTON_DRAW;

/*  A progress bar effect instance  */
struct PROGRESS_BAR
{
    PROGRESS_BAR_EFFECT_PARAM param;
};

/*  A deferred draw for a progress bar  */
struct PROGRESS_BAR_DRAW
{
    PROGRESS_BAR_EFFECT_PARAM param;
    BROGUE_DRAW_COLOR foreground;
    BROGUE_DRAW_COLOR background;
    wchar_t *str;
};

/*  A button effect instance  */
struct BUTTON
{
    BUTTON_EFFECT_PARAM param;

    wchar_t *symbols;
    int *symbol_flags;
};

/*  A deferred draw for a button  */
struct BUTTON_DRAW
{
    BUTTON_EFFECT_PARAM param;
    wchar_t *str;

    wchar_t *symbols;
    int *symbol_flags;

    int tab_stop_count;
    int *tab_stops;
};

/*  Convert a wide char string to uint16's for rendering with SDL_ttf  */
static Uint16 *wcsToUint16(wchar_t *str, int len)
{
    int i;
    Uint16 *unitext;

    unitext = malloc((wcslen(str) + 1) * sizeof(Uint16));
    if (unitext == NULL)
    {
	return NULL;
    }

    for (i = 0; i < len; i++)
    {
	unitext[i] = str[i];
    }
    unitext[i] = 0;

    return unitext;
}

/*  A text drawing method used by both progress bars and buttons  */
static int drawEffectText(
    BROGUE_DISPLAY *display, SDL_Surface *surface, 
    BROGUE_DRAW_COLOR color, int centered, wchar_t *str,
    int symbol_count, wchar_t *symbols, int *symbol_flags,
    int tab_stop_count, int *tab_stops)
{
    Uint16 *unitext;
    SDL_Surface *text_surface, *glyph;
    TTF_Font *font = BrogueDisplay_getFont(display, 1);
    TTF_Font *monofont = BrogueDisplay_getFont(display, 0);
    SDL_Rect rect;
    SDL_Color sdl_fg = BrogueDisplay_colorToSDLColor(&color);
    SDL_SVGSET *svgset = BrogueDisplay_getSvgset(display);
    int i, begin_span, total_len, str_len, x, symbol_index, tab_index;
    int w, h, font_width, font_height;
    int base_x;

    BrogueDisplay_getFontSize(display, &font_width, &font_height);

    begin_span = 0;
    total_len = 0;
    str_len = wcslen(str);
    tab_index = 0;
    for (i = 0; str[i]; i++)
    {
	if (str[i] == COLOR_ESCAPE || str[i] == '*' || str[i] == '\t')
	{
	    if (i > begin_span)
	    {
		unitext = wcsToUint16(&str[begin_span], i - begin_span);
		if (unitext == NULL)
		{
		    return ENOMEM;
		}
		TTF_SizeUNICODE(font, unitext, &w, &h);
		free(unitext);

		total_len += w;
	    }

	    if (str[i] == COLOR_ESCAPE)
	    {
		if (i + 3 < str_len)
		{
		    i += 3;
		}
	    }
	    else if (str[i] == '\t')
	    {
		if (tab_index < tab_stop_count) {
		    total_len = tab_stops[tab_index++] * font_width;
		}
	    }
	    else if (str[i] == '*')
	    {
		total_len += font_width;
	    }
	    begin_span = i + 1;
	}
    }

    if (i > begin_span)
    {
	unitext = wcsToUint16(&str[begin_span], i - begin_span);
	if (unitext == NULL)
	{
	    return ENOMEM;
	}
	TTF_SizeUNICODE(font, unitext, &w, &h);
	free(unitext);
	
	total_len += w;
    }

    begin_span = 0;
    base_x = 0;
    if (centered)
    {
	base_x = (surface->w - total_len) / 2;
    }

    symbol_index = 0;
    tab_index = 0;
    x = base_x;
    for (i = 0; str[i]; i++)
    {
	if (str[i] == COLOR_ESCAPE || str[i] == '*' || str[i] == '\t')
	{
	    if (i > begin_span)
	    {
		unitext = wcsToUint16(&str[begin_span], i - begin_span);
		if (unitext == NULL)
		{
		    return ENOMEM;
		}
		TTF_SizeUNICODE(font, unitext, &w, &h);
		text_surface = TTF_RenderUNICODE_Blended(font, unitext, sdl_fg);
		free(unitext);

		if (text_surface == NULL)
		{
		    return ENOMEM;
		}

		rect.x = x;
		rect.y = (surface->h - text_surface->h) / 2;
		rect.w = text_surface->w;
		rect.h = text_surface->h;
   
		SDL_BlitSurface(text_surface, NULL, surface, &rect);
		SDL_FreeSurface(text_surface);
		
		x += w;
	    }

	    if (str[i] == COLOR_ESCAPE)
	    {
		if (i + 3 < str_len)
		{
		    color.red = (str[i + 1] - COLOR_VALUE_INTERCEPT) / 100.0;
		    color.green = (str[i + 2] - COLOR_VALUE_INTERCEPT) / 100.0;
		    color.blue = (str[i + 3] - COLOR_VALUE_INTERCEPT) / 100.0;
		    sdl_fg = BrogueDisplay_colorToSDLColor(&color);

		    i += 3;
		}
	    }
	    else if (str[i] == '\t')
	    {
		if (tab_index < tab_stop_count) {
		    x = base_x + tab_stops[tab_index++] * font_width;
		}
	    }
	    else if (str[i] == '*' && symbol_index < symbol_count)
	    {
		glyph = NULL;

		if (symbol_flags[symbol_index] & PLOT_CHAR_TILE)
		{
		    glyph = SdlSvgset_render(svgset, 
					     symbols[symbol_index], 0, sdl_fg);
		}

		if (glyph == NULL)
		{
		    glyph = TTF_RenderGlyph_Blended(monofont, 
						    symbols[symbol_index], 
						    sdl_fg);
		}
		if (glyph == NULL)
		{
		    return ENOMEM;
		}
		
		rect.x = x;
		rect.y = (surface->h - glyph->h) / 2;
		rect.w = surface->w + (font_width - glyph->w) / 2;
		rect.h = surface->h;
   
		SDL_BlitSurface(glyph, NULL, surface, &rect);
		SDL_FreeSurface(glyph);
		
		symbol_index++;
		x += font_width;
	    }
	    begin_span = i + 1;
	}
    }
								
    if (i > begin_span)
    {
	unitext = wcsToUint16(&str[begin_span], i - begin_span);
	if (unitext == NULL)
	{
	    return ENOMEM;
	}
	text_surface = TTF_RenderUNICODE_Blended(font, unitext, sdl_fg);
	free(unitext);

	if (text_surface == NULL)
	{
	    return ENOMEM;
	}

	rect.x = x;
	rect.y = (surface->h - text_surface->h) / 2;
	rect.w = text_surface->w;
	rect.h = text_surface->h;
    
	SDL_BlitSurface(text_surface, NULL, surface, &rect);
	SDL_FreeSurface(text_surface);
    }

    return 0;
}

/*  Draw a progress bar just-in-time  */
static int progressBarDeferredDraw(
    BROGUE_DISPLAY *display, void *param, SDL_Surface *surface)
{
    PROGRESS_BAR_DRAW *draw = (PROGRESS_BAR_DRAW *)param;
    cairo_surface_t *cairo_surface;
    cairo_t *cairo;
    int err;
    int w = surface->w;
    int h = surface->h;

    cairo_surface = cairo_image_surface_create_for_data(
	surface->pixels, CAIRO_FORMAT_RGB24,
	surface->w, surface->h, surface->pitch);
    if (cairo_surface == NULL)
    {
	return ENOMEM;
    }

    cairo = cairo_create(cairo_surface);
    if (cairo == NULL)
    {
	cairo_surface_destroy(cairo_surface);
	return ENOMEM;
    }

    /*  Trace out a rounded bar shape  */
    cairo_move_to(cairo, 1.0, h / 2.0);
    cairo_curve_to(cairo, 1.0, 1.0, 8.0, 1.0, 16.0, 1.0);
    cairo_line_to(cairo, w - 16.0, 1.0);
    cairo_curve_to(cairo, w - 8.0, 1.0, w - 1.0, 1.0, w - 1.0, h / 2.0); 
    cairo_curve_to(cairo, w - 1.0, h - 1.0, w - 8.0, h - 1.0, w - 16.0, h - 1.0);
    cairo_line_to(cairo, 16.0, h - 1.0);
    cairo_curve_to(cairo, 8.0, h - 1.0, 1.0, h - 1.0, 1.0, h / 2.0);

    cairo_clip(cairo);

    /*  Fill the amount full with the background color  */
    cairo_set_source_rgb(cairo, 
			 draw->background.red,
			 draw->background.green,
			 draw->background.blue);
    cairo_rectangle(cairo, 0, 0, w * draw->param.amount, h);
    cairo_fill(cairo);

    /*  Fill the remainder with a darker color  */
    cairo_set_source_rgb(cairo,
			 draw->background.red * 0.33,
			 draw->background.green * 0.33,
			 draw->background.blue * 0.33);
    cairo_rectangle(cairo, w * draw->param.amount, 0,
		    w * (1.0 - draw->param.amount), h);
    cairo_fill(cairo);
    
    cairo_destroy(cairo);
    cairo_surface_destroy(cairo_surface);

    err = drawEffectText(display, surface, draw->foreground, 1, draw->str, 
			 0, NULL, NULL, 0, NULL);

    return err;
}

/*  Free the resources associated with a deferred progress bar draw  */
static void progressBarDeferredFree(void *param)
{
    PROGRESS_BAR_DRAW *draw = (PROGRESS_BAR_DRAW *)param;

    free(draw->str);
    free(draw);
}

/*  Constructor for a progress bar effect instance  */
static void *progressBarNew(void)
{
    PROGRESS_BAR *progress_bar;

    progress_bar = malloc(sizeof(PROGRESS_BAR));
    if (progress_bar == NULL)
    {
	return NULL;
    }
    memset(progress_bar, 0, sizeof(PROGRESS_BAR));

    return progress_bar;
}

/*  Destructor for a progress bar  */
static void progressBarDestroy(void *effect_state)
{
    PROGRESS_BAR *progress_bar = (PROGRESS_BAR *)effect_state;

    free(progress_bar);
}

/*  When we are passed progress bar drawing parameters, just store them  */
static int progressBarSetParam(void *effect_state, void *param)
{
    PROGRESS_BAR *progress_bar = (PROGRESS_BAR *)effect_state;

    memcpy(&progress_bar->param, param, sizeof(PROGRESS_BAR_EFFECT_PARAM));

    return 0;
}

/*  Generate a deferred draw for a progress bar  */
static BROGUE_DEFERRED_DRAW *progressBarDraw(
    BROGUE_WINDOW *window, const BROGUE_DRAW_CONTEXT_STATE *context_state, 
    void *effect_state, int x, int y, const wchar_t *str)
{
    PROGRESS_BAR *progress_bar = (PROGRESS_BAR *)effect_state;
    BROGUE_DEFERRED_DRAW *draw;
    PROGRESS_BAR_DRAW *progress_draw;

    progress_draw = (PROGRESS_BAR_DRAW *)malloc(sizeof(PROGRESS_BAR_DRAW));
    if (progress_draw == NULL)
    {
	return NULL;
    }
    memset(progress_draw, 0, sizeof(PROGRESS_BAR_DRAW));

    progress_draw->param = progress_bar->param;
    progress_draw->foreground = context_state->foreground;
    progress_draw->background = context_state->background;
    progress_draw->str = malloc(sizeof(wchar_t) * (wcslen(str) + 1));
    if (progress_draw->str == NULL)
    {
	free(progress_draw);
	return NULL;
    }
    wcscpy(progress_draw->str, str);

    draw = BrogueDeferredDraw_create(
	window, progressBarDeferredDraw, progressBarDeferredFree,
	progress_draw, x, y, progress_bar->param.size, 1);
    if (draw == NULL)
    {
	free(progress_draw);
	return NULL;
    }

    return draw;
}

/*  The just-in-time drawing routing for a button  */
static int buttonDeferredDraw(BROGUE_DISPLAY *display, 
			      void *param, SDL_Surface *surface)
{
    cairo_surface_t *cairo_surface;
    cairo_t *cairo;
    cairo_pattern_t *pattern;
    BUTTON_DRAW *button_draw = param;
    BROGUE_DRAW_COLOR bg_dark = { 0.0, 0.0, 0.4 };
    BROGUE_DRAW_COLOR bg_light = { 0.1, 0.2, 0.5 };
    BROGUE_DRAW_COLOR text_color = { 1.0, 1.0, 1.0 };
    int w, h;

    w = surface->w;
    h = surface->h;

    bg_dark = button_draw->param.color;
    bg_light = button_draw->param.highlight_color; 

    if (button_draw->param.is_shaded)
    {
	cairo_surface = cairo_image_surface_create_for_data(
	    surface->pixels, CAIRO_FORMAT_RGB24,
	    w, h, surface->pitch);
	if (cairo_surface == NULL)
	{
	    return ENOMEM;
	}

	cairo = cairo_create(cairo_surface);
	if (cairo == NULL)
	{
	    cairo_surface_destroy(cairo_surface);
	    return ENOMEM;
	}

	pattern = cairo_pattern_create_linear(0, 0, 0, h);
	cairo_pattern_add_color_stop_rgb(pattern, 0.0, 
					 bg_dark.red,
					 bg_dark.green,
					 bg_dark.blue);
	cairo_pattern_add_color_stop_rgb(pattern, 1.0,
					 bg_light.red,
					 bg_light.green,
					 bg_light.blue);
	
	cairo_move_to(cairo, 0, h / 2.0);
	cairo_curve_to(cairo, 0, h / 4.0, h / 4.0, 0, h / 2.0, 0);
	cairo_line_to(cairo, w - h / 2.0, 0);
	cairo_curve_to(cairo, w - h / 4.0, 0, w, h / 4.0, w, h / 2.0);
	cairo_curve_to(cairo, w, h - h / 4.0, w - h / 4.0, h, w - h / 2.0, h);
	cairo_line_to(cairo, h / 2.0, h);
	cairo_curve_to(cairo, h / 4.0, h, 0, h - h / 4.0, 0, h / 2.0);
	cairo_close_path(cairo);

	cairo_set_source(cairo, pattern);
	cairo_fill(cairo);

	cairo_pattern_destroy(pattern);

	pattern = cairo_pattern_create_linear(0, 0, 0, h / 2.0);
	cairo_pattern_add_color_stop_rgb(pattern, 0.0, 
					 bg_light.red,
					 bg_light.green,
					 bg_light.blue);
	cairo_pattern_add_color_stop_rgb(pattern, 1.0,
					 (bg_light.red + bg_dark.red) / 2.0,
					 (bg_light.green + bg_dark.green) / 2.0,
					 (bg_light.blue + bg_dark.blue) / 2.0);

	cairo_new_path(cairo);

	cairo_move_to(cairo, h / 4.0, h / 2.0);
	cairo_curve_to(cairo, h / 8.0, h / 4.0, 
		       h * 3.0 / 8.0, h / 16.0, h / 2.0, h / 16.0);
	cairo_line_to(cairo, w - h / 2.0, h / 16.0);
	cairo_curve_to(cairo, w - h * 3.0 / 8.0, h / 16.0, 
		       w - h / 8.0, h / 4.0, w - h / 4.0, h / 2.0);
	cairo_close_path(cairo);

	cairo_set_source(cairo, pattern);
	cairo_fill(cairo);
    
	cairo_pattern_destroy(pattern);
	cairo_destroy(cairo);
	cairo_surface_destroy(cairo_surface);
    }

    drawEffectText(display, surface, 
		   text_color, button_draw->param.is_centered, 
		   button_draw->str, 
		   button_draw->param.symbol_count,
		   button_draw->symbols, button_draw->symbol_flags,
	           button_draw->tab_stop_count, button_draw->tab_stops);

    return 0;
}

/*  Free the resources associated with a deferred button draw  */
void buttonDeferredFree(void *param)
{
    BUTTON_DRAW *button_draw = param;

    free(button_draw->symbols);
    free(button_draw->symbol_flags);
    free(button_draw->tab_stops);
    free(button_draw->str);
    free(button_draw);
}

/*  Button effect constructor  */
static void *buttonNew(void)
{
    BUTTON *button;

    button = malloc(sizeof(BUTTON));
    if (button == NULL)
    {
	return NULL;
    }
    memset(button, 0, sizeof(BUTTON));

    return button;
}

/*  Button effect destructor  */
static void buttonDestroy(void *state)
{
    BUTTON *button = state;

    free(button);
}

/*  Presist the drawing parameters for a button.  We've got to 
    allocate memory to store the symbols, as the caller could free them
    before the deferred draw occurs  */
int buttonSetParam(void *state, void *param)
{
    BUTTON *button = state;
    BUTTON_EFFECT_PARAM *button_param = param;

    if (button->param.symbol_count > 0)
    {
	free(button->symbols);
	free(button->symbol_flags);

	button->symbols = NULL;
	button->symbol_flags = NULL;
    }

    button->param = *button_param;

    if (button->param.symbol_count > 0)
    {
	button->symbols = malloc(sizeof(wchar_t) * button->param.symbol_count);
	button->symbol_flags = malloc(
	    sizeof(int) * button->param.symbol_count);

	if (button->symbols == NULL || button->symbol_flags == NULL)
	{
	    free(button->symbols);
	    free(button->symbol_flags);
	    memset(&button->param, 0, sizeof(BUTTON_EFFECT_PARAM));
	    
	    return ENOMEM;
	}

	memcpy(button->symbols, button->param.symbols, 
	       sizeof(wchar_t) * button->param.symbol_count);
	memcpy(button->symbol_flags, button->param.symbol_flags,
	       sizeof(int) * button->param.symbol_count);
    }

    return 0;
}

/*  Generate a deferred draw for a button  */
BROGUE_DEFERRED_DRAW *buttonDraw(
    BROGUE_WINDOW *window, const BROGUE_DRAW_CONTEXT_STATE *context_state, 
    void *effect_state, int x, int y, const wchar_t *str)
{
    BUTTON *button = effect_state;
    BUTTON_DRAW *button_draw;
    BROGUE_DEFERRED_DRAW *draw;

    button_draw = malloc(sizeof(BUTTON_DRAW));
    if (button_draw == NULL)
    {
	return NULL;
    }
    memset(button_draw, 0, sizeof(BUTTON_DRAW));

    button_draw->str = malloc(sizeof(wchar_t) * (wcslen(str) + 1));
    if (button_draw->str == NULL)
    {
	free(button_draw);
	return NULL;
    }

    if (button->param.symbol_count > 0)
    {
	button_draw->symbols = malloc(
	    sizeof(wchar_t) * button->param.symbol_count);
	button_draw->symbol_flags = malloc(
	    sizeof(int) * button->param.symbol_count);

	if (button_draw->symbols == NULL || button_draw->symbol_flags == NULL)
	{
	    free(button_draw->symbols);
	    free(button_draw->symbol_flags);
	    free(button_draw->str);
	    free(button_draw);
	    return NULL;
	}

	memcpy(button_draw->symbols, button->symbols, 
	       sizeof(wchar_t) * button->param.symbol_count);
	memcpy(button_draw->symbol_flags, button->symbol_flags,
	       sizeof(int) * button->param.symbol_count);
    }

    if (context_state->tab_stop_count)
    {
	button_draw->tab_stop_count = context_state->tab_stop_count;
	button_draw->tab_stops = malloc(
	    sizeof(int) * context_state->tab_stop_count);
	if (button_draw->tab_stops == NULL)
	{
	    free(button_draw->symbols);
	    free(button_draw->symbol_flags);
	    free(button_draw->str);
	    free(button_draw);

	    return NULL;
	}
	memcpy(button_draw->tab_stops, context_state->tab_stops,
	       sizeof(int) * context_state->tab_stop_count);
    }

    wcscpy(button_draw->str, str);
    button_draw->param = button->param;

    draw = BrogueDeferredDraw_create(
	window, buttonDeferredDraw, buttonDeferredFree,
	button_draw, x, y, button_draw->param.width, 1);
    if (draw == NULL)
    {
	free(button_draw);
	return NULL;
    }

    return draw;
}

/*  Register our custom effect classes  */
void BrogueEffects_registerAll(BROGUE_DISPLAY *display)
{
    BrogueDisplay_registerEffectClass(
	display, PROGRESS_BAR_EFFECT_NAME, 
	progressBarNew, progressBarDestroy, 
	progressBarSetParam, progressBarDraw);
    BrogueDisplay_registerEffectClass(
	display, BUTTON_EFFECT_NAME,
	buttonNew, buttonDestroy,
	buttonSetParam, buttonDraw);
}
