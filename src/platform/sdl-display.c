/*
 *  sdl-display.c
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

#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <wchar.h>
#include <glib.h>

#include "sdl-display.h"

/*  wide-character-string equivalence for glib's hashes  */
static gboolean wcs_equal(gconstpointer a, gconstpointer b)
{
    return (wcscmp((wchar_t *)a, (wchar_t *)b) == 0);
}

/*  a wide character string hash function for glib's hashes  */
static guint wcs_hash(gconstpointer key)
{
    wchar_t *str = (wchar_t *)key;
    guint value = 0;
    
    while (*str)
    {
	int i;

	value = (value << 31) | (value >> 1);
	i = *str;
	value = value ^ g_int_hash(&i);
	str++;
    }

    return value;
}

/*  Open the global display state for the app  */
BROGUE_DISPLAY *BrogueDisplay_open(int width, int height)
{
    BROGUE_DISPLAY *display;

    display = (BROGUE_DISPLAY *)malloc(sizeof(BROGUE_DISPLAY));
    if (display == NULL)
    {
	return NULL;
    }
    memset(display, 0, sizeof(BROGUE_DISPLAY));

    display->width = width;
    display->height = height;

    display->effect_classes = g_hash_table_new(wcs_hash, wcs_equal);
    if (display->effect_classes == NULL)
    {
	free(display);
	return NULL;
    }

    display->root_window = BrogueWindow_open(NULL, 0, 0, width, height);
    if (display->root_window == NULL)
    {
	BrogueDisplay_close(display);
    }
    display->root_window->display = display;

    BrogueEffects_registerAll(display);

    return display;
}

/*  The iterator for freeing the effect classes store in the display's
    effect hash  */
static void effectClassFree(gpointer key, gpointer value, gpointer data)
{
    free(key);
    free(value);
}

/*  Close down the display.  */
void BrogueDisplay_close(BROGUE_DISPLAY *display)
{
    if (display->render_glyph != NULL)
    {
	SDL_FreeSurface(display->render_glyph);
    }
 
    if (display->root_window)
    {
	BrogueWindow_close(display->root_window);
	display->root_window = NULL;
    }

    g_hash_table_foreach(display->effect_classes, effectClassFree, NULL);
    g_hash_table_destroy(display->effect_classes);

    free(display);
}

/*  Generate the framebuffer from the current state of the display  */
void BrogueDisplay_prepareFrame(BROGUE_DISPLAY *display)
{
    SDL_Surface *surface;
    int err;
    SDL_Rect rect;
    int w, h;
    int screen_compatible = 1;

    if (display->screen->format->Rmask != 0x00FF0000
	|| display->screen->format->Gmask != 0x0000FF00
	|| display->screen->format->Bmask != 0x000000FF)
    {
	screen_compatible = 0;
    }

    w = display->width * display->font_width;
    h = display->height * display->font_height;

    rect.x = (display->screen->w - w) / 2;
    rect.y = (display->screen->h - h) / 2;
    rect.w = w;
    rect.h = h;

    if (!screen_compatible)
    {
	surface = SDL_CreateRGBSurface(
	    0, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);
	if (surface == NULL)
	{
	    return;
	}
    }
    else
    {
	/*  If the screen surface matches our expected ARGB pixel format,
	    we can render to it directly rather than rendering to
	    another surface and converting afterward  */
	surface = SDL_CreateRGBSurfaceFrom(
	    display->screen->pixels 
	    + 4 * rect.x + display->screen->pitch * rect.y, 
	    w, h, 32, display->screen->pitch,
	    0x00FF0000, 0x0000FF00, 0x000000FF, 0);

	memset(display->screen->pixels, 0, 
	       display->screen->pitch * display->screen->h);
    }

    err = BrogueWindow_draw(display->root_window, surface);
    if (err)
    {
	return;
    }

    if (!screen_compatible)
    {
	SDL_BlitSurface(surface, NULL, display->screen, &rect);
    }
    SDL_FreeSurface(surface);
}

/*  Set the target screen surface  */
void BrogueDisplay_setScreen(BROGUE_DISPLAY *display, SDL_Surface *screen)
{
    display->screen = screen;
}

/*  Get one of the fonts associated with the display  */
TTF_Font *BrogueDisplay_getFont(BROGUE_DISPLAY *display, int proportional)
{
    if (proportional)
    {
	return display->sans_font;
    }
    else
    {
	return display->mono_font;
    }
}

/*  Get the current cell size of the display  */
void BrogueDisplay_getFontSize(BROGUE_DISPLAY *display, 
			       int *width, int *height)
{
    *width = display->font_width;
    *height = display->font_height;
}

/*  Set the fonts to be used by the display  */
void BrogueDisplay_setFonts(
    BROGUE_DISPLAY *display, TTF_Font *mono_font, TTF_Font *sans_font,
    int font_width, int font_height)
{
    display->mono_font = mono_font;
    display->sans_font = sans_font;
    display->font_width = font_width;
    display->font_height = font_height;
    display->font_descent = 0;

    if (mono_font != NULL)
    {
	display->font_descent = TTF_FontDescent(mono_font);
    }

    if (display->render_glyph != NULL)
    {
	SDL_FreeSurface(display->render_glyph);
    }
    display->render_glyph = SDL_CreateRGBSurface(
	0, font_width, font_height, 32,
	0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
}

/*  Set the tile-set to be used by the display  */
void BrogueDisplay_setSvgset(BROGUE_DISPLAY *display, SDL_SVGSET *svgset)
{
    display->svgset = svgset;
}

/*  Get the current tile set  */
SDL_SVGSET *BrogueDisplay_getSvgset(BROGUE_DISPLAY *display)
{
    return display->svgset;
}

/*  Register an effect class for custom rendering of elements  */
int BrogueDisplay_registerEffectClass(
    BROGUE_DISPLAY *display,
    const wchar_t *name,
    BROGUE_EFFECT_NEW_FUNC new_func,
    BROGUE_EFFECT_DESTROY_FUNC destroy_func,
    BROGUE_EFFECT_SET_PARAM_FUNC set_param_func,
    BROGUE_EFFECT_DRAW_STRING_FUNC draw_string_func)
{
    wchar_t *persist_name;
    BROGUE_EFFECT_CLASS *effect_class;
    
    persist_name = (wchar_t *)malloc(sizeof(wchar_t) * (wcslen(name) + 1));
    if (persist_name == NULL)
    {
	return ENOMEM;
    }

    wcscpy(persist_name, name);

    effect_class = malloc(sizeof(BROGUE_EFFECT_CLASS));
    if (effect_class == NULL)
    {
	free(persist_name);
	return ENOMEM;
    }
    memset(effect_class, 0, sizeof(BROGUE_EFFECT_CLASS));

    effect_class->new_func = new_func;
    effect_class->destroy_func = destroy_func;
    effect_class->set_param_func = set_param_func;
    effect_class->draw_string_func = draw_string_func;

    g_hash_table_insert(display->effect_classes, persist_name, effect_class);

    return 0;
}

/*  Get the window at the root of the window hierarchy  */
BROGUE_WINDOW *BrogueDisplay_getRootWindow(BROGUE_DISPLAY *display)
{
    return display->root_window;
}

/*  Clip a rectangle against the bounds of the window  */
void BrogueWindow_clip(BROGUE_WINDOW *window,
		       int *x, int *y, int *width, int *height)
{
    if (*x < 0)
    {
	*width += *x;
	*x = 0;
    }

    if (*y < 0)
    {
	*height += *y;
	*y = 0;
    }

    if (*x > window->width)
    {
	*x = window->width;
    }

    if (*y > window->height)
    {
	*y = window->height;
    }

    if (*x + *width > window->width)
    {
	*width = window->width - *x;
    }

    if (*y + *height > window->height)
    {
	*height = window->height - *y;
    }

    if (*width < 0)
    {
	*width = 0;
    }

    if (*height < 0)
    {
	*height = 0;
    }
}

/*  Open a new subwindow  */
BROGUE_WINDOW *BrogueWindow_open(
    BROGUE_WINDOW *parent, int x, int y, int width, int height)
{
    BROGUE_WINDOW *window;

    if (parent != NULL)
    {
	BrogueWindow_clip(parent, &x, &y, &width, &height);
    }

    window = malloc(sizeof(BROGUE_WINDOW));
    if (window == NULL)
    {
	return NULL;
    }
    memset(window, 0, sizeof(BROGUE_WINDOW));

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->visible = 1;

    window->draws = malloc(sizeof(BROGUE_DEFERRED_DRAW *) 
			   * window->width * window->height);
    if (window->draws == NULL)
    {
	free(window);
	return NULL;
    }
    memset(window->draws, 0, 
	   sizeof(BROGUE_DEFERRED_DRAW *) * window->width * window->height);
    
    window->children = g_ptr_array_new();
    if (window->children == NULL)
    {
	free(window->draws);
	free(window);
	return NULL;
    }

    window->contexts = g_ptr_array_new();
    if (window->contexts == NULL)
    {
	g_ptr_array_unref(window->children);
	free(window->draws);
	free(window);
    }

    if (parent != NULL)
    {
	window->display = parent->display;
	window->parent = parent;

	g_ptr_array_add(parent->children, window);
    }

    return window;
}

/*  Close a window, freeing all associated resources  */
void BrogueWindow_close(BROGUE_WINDOW *window)
{
    /*  Unreference all deferred draws  */
    BrogueWindow_clear(window);

    while(window->children->len)
    {
	int count = window->children->len;

	BROGUE_WINDOW *child = 
	    (BROGUE_WINDOW *)g_ptr_array_index(window->children, 0);
	BrogueWindow_close(child);

	assert(window->children->len < count);
    }

    while(window->contexts->len)
    {
	int count = window->contexts->len;

	BROGUE_DRAW_CONTEXT *context = 
	    (BROGUE_DRAW_CONTEXT *)g_ptr_array_index(window->contexts, 0);
	BrogueDrawContext_close(context);

	assert(window->contexts->len < count);
    }

    if (window->parent != NULL)
    {
	g_ptr_array_remove(window->parent->children, window);
    }
    g_ptr_array_unref(window->children);
    g_ptr_array_unref(window->contexts);
    free(window->draws);
    free(window);
}

/*  Blur the contents of a cairo surface along either the horizontal or
    vertical axis  */
int BrogueWindow_blurSurface(
    cairo_surface_t *surface, int amount, int vertical)
{
    int x, y;
    unsigned char *blur_line;
    int pitch = cairo_image_surface_get_stride(surface);
    unsigned char *base_pix = cairo_image_surface_get_data(surface);
    int w = cairo_image_surface_get_width(surface);
    int h = cairo_image_surface_get_height(surface);
    int blur_axis, secondary_axis;

    if (vertical)
    {
	blur_axis = h;
	secondary_axis = w;
    }
    else
    {
	blur_axis = w;
	secondary_axis = h;
    }

    blur_line = malloc((blur_axis + amount) * 4);
    if (blur_line == NULL)
    {
	return ENOMEM;
    }

    /*  Blur the current contents on the framebuffer  */
    for (y = 0; y < secondary_axis; y++)
    {
	int rc, gc, bc, accum_count;
	int pix_offset;
	unsigned char *blur_pix = blur_line;
	unsigned char *line_pix;
	unsigned char *pix;
	unsigned char *back_pix;

	if (vertical)
	{
	    line_pix = base_pix + 4 * y;
	    pix_offset = pitch;
	}
	else
	{
	    line_pix = base_pix + y * pitch;
	    pix_offset = 4;
	}

	pix = back_pix = line_pix;
	rc = gc = bc = accum_count = 0;

	for (x = 0; x < blur_axis + amount; x++)
	{
	    if (x < blur_axis)
	    {
		rc += pix[0];
		gc += pix[1];
		bc += pix[2];

		pix += pix_offset;
	    }
	    else
	    {
		accum_count--;
	    }

	    if (accum_count < amount)
	    {
		accum_count++;
	    }
	    else
	    {
		rc -= back_pix[0];
		gc -= back_pix[1];
		bc -= back_pix[2];

		back_pix += pix_offset;
	    }

	    blur_pix[0] = rc / accum_count;
	    blur_pix[1] = gc / accum_count;
	    blur_pix[2] = bc / accum_count;

	    blur_pix += 4;
	}

	pix = line_pix;
	blur_pix = blur_line + amount / 2 * 4;

	for (x = 0; x < blur_axis; x++)
	{
	    pix[0] = blur_pix[0];
	    pix[1] = blur_pix[1];
	    pix[2] = blur_pix[2];

	    pix += pix_offset;
	    blur_pix += 4;
	}
    }
    free(blur_line);

    return 0;
}

/*  Draw the background, for opaque or translucent windows  */
int BrogueWindow_drawBackground(BROGUE_WINDOW *window, SDL_Surface *surface)
{
    int x, y;
    int r, g, b, a;
    int err;
    int w = surface->w;
    int h = surface->h;
    int blur = 8;
    cairo_surface_t *blurred_surface, *dst_surface;
    cairo_t *cairo;
    double cell_size = window->display->font_width;

    if (window->color.alpha == 0.0)
    {
	return 0;
    }

    blurred_surface = cairo_image_surface_create(
	CAIRO_FORMAT_RGB24, surface->w, surface->h);
    if (blurred_surface == NULL)
    {
	return ENOMEM;
    }

    dst_surface = cairo_image_surface_create_for_data(
	surface->pixels, CAIRO_FORMAT_RGB24, 
	surface->w, surface->h, surface->pitch);
    if (dst_surface == NULL)
    {
	cairo_surface_destroy(blurred_surface);
	return ENOMEM;
    }

    cairo = cairo_create(dst_surface);
    if (cairo == NULL)
    {
	cairo_surface_destroy(blurred_surface);
	cairo_surface_destroy(dst_surface);
	return ENOMEM;
    }

    r = (int)(window->color.red * 255);
    g = (int)(window->color.green * 255);
    b = (int)(window->color.blue * 255);
    a = (int)(window->color.alpha * 256);

    /*  Copy to the cairo surface  */
    for (y = 0; y < surface->h; y++)
    {
	unsigned char *pix = 
	    (unsigned char *)surface->pixels + surface->pitch * y;
	unsigned char *cairo_pix = 
	    cairo_image_surface_get_data(blurred_surface)
	    + y * cairo_image_surface_get_stride(blurred_surface);

	memcpy(cairo_pix, pix, 4 * surface->w);
    }

    /*  Blur the surface in both directions  */
    err = BrogueWindow_blurSurface(blurred_surface, blur, 0);
    if (!err)
    {
	err = BrogueWindow_blurSurface(blurred_surface, blur, 1);
    }
    if (err)
    {
	cairo_destroy(cairo);
	cairo_surface_destroy(blurred_surface);
	cairo_surface_destroy(dst_surface);
	return err;
    }

    /*  Blend the window color  */
    for (y = 0; y < surface->h; y++)
    {
	unsigned char *pix = 
	    cairo_image_surface_get_data(blurred_surface)
	    + y * cairo_image_surface_get_stride(blurred_surface);

	for (x = 0; x < surface->w; x++)
	{	    
	    pix[0] = ((256 - a) * pix[0] + a * b) >> 8;
	    pix[1] = ((256 - a) * pix[1] + a * g) >> 8;
	    pix[2] = ((256 - a) * pix[2] + a * r) >> 8;

	    pix += 4;
	}
    }

    /*  Trace out a bevelled window shape  */
    cairo_move_to(cairo, 1.0, cell_size);
    cairo_curve_to(cairo, 1.0, cell_size / 2, 
		   cell_size / 2, 1.0, cell_size, 1.0);
    cairo_line_to(cairo, w - cell_size, 1.0);
    cairo_curve_to(cairo, w - cell_size / 2, 1.0, 
		   w - 1.0, cell_size / 2, w - 1.0, cell_size);
    cairo_line_to(cairo, w - 1.0, h - cell_size);
    cairo_curve_to(cairo, w - 1.0, h - cell_size / 2, 
		   w - cell_size / 2, h - 1.0, w - cell_size, h - 1.0);
    cairo_line_to(cairo, cell_size, h - 1.0);
    cairo_curve_to(cairo, cell_size / 2, h - 1.0,
		   1.0, h - cell_size / 2, 1.0, h - cell_size);
    cairo_close_path(cairo);

    /*  Fill with the blurred surface  */
    cairo_set_source_surface(cairo, blurred_surface, 0, 0);
    cairo_fill(cairo);

    cairo_destroy(cairo);
    cairo_surface_destroy(dst_surface);
    cairo_surface_destroy(blurred_surface);

    return 0;
}

/*  Draw the current contents of a window (and children) onto 
    a target surface  */
int BrogueWindow_draw(BROGUE_WINDOW *window, SDL_Surface *surface)
{
    int i, err;
    int x, y;
    int font_width, font_height;

    if (!window->visible)
    {
	return 0;
    }

    font_width = window->display->font_width;
    font_height = window->display->font_height;

    for (i = 0; i < window->width * window->height; i++)
    {
	BROGUE_DEFERRED_DRAW *draw = window->draws[i];
	if (draw != NULL)
	{
	    draw->drawn = 0;
	}
    }

    err = BrogueWindow_drawBackground(window, surface);
    if (err)
    {
	return err;
    }
    
    for (y = 0; y < window->height; y++)
    {
	for (x = 0; x < window->width; x++)
	{
	    BROGUE_DEFERRED_DRAW *draw = window->draws[y * window->width + x];
	    SDL_Surface *draw_surface;
	    void *draw_pixels;
	    int x, y, width, height;

	    if (draw == NULL || draw->drawn)
	    {
		continue;
	    }

	    x = draw->x;
	    y = draw->y;
	    width = draw->width;
	    height = draw->height;
	    BrogueWindow_clip(window, &x, &y, &width, &height);
	    
	    draw_pixels = (char *)surface->pixels 
		+ y * font_height * surface->pitch
		+ x * font_width * surface->format->BytesPerPixel;
	    draw_surface = SDL_CreateRGBSurfaceFrom(
		draw_pixels, 
		width * font_width, height * font_height,
		surface->format->BitsPerPixel, surface->pitch,
		surface->format->Rmask,
		surface->format->Gmask,
		surface->format->Bmask,
		surface->format->Amask);
	    if (draw_surface == NULL)
	    {
		return ENOMEM;
	    }

	    err = BrogueDeferredDraw_draw(window->display, draw, draw_surface);
	    SDL_FreeSurface(draw_surface);

	    if (err)
	    {
		return err;
	    }

	    draw->drawn = 1;
	}
    }

    for (i = 0; i < window->children->len; i++)
    {
	SDL_Surface *child_surface;
	void *child_pixels;
	BROGUE_WINDOW *child = 
	    (BROGUE_WINDOW *)g_ptr_array_index(window->children, i);
	int x, y, width, height;

	x = child->x;
	y = child->y;
	width = child->width;
	height = child->height;

	child_pixels = (char *)surface->pixels 
	    + y * font_height * surface->pitch
	    + x * font_width * surface->format->BytesPerPixel;
	child_surface = SDL_CreateRGBSurfaceFrom(
		child_pixels, 
		width * font_width, height * font_height,
		surface->format->BitsPerPixel, surface->pitch,
		surface->format->Rmask,
		surface->format->Gmask,
		surface->format->Bmask,
		surface->format->Amask);
	if (child_surface == NULL)
	{
	    return ENOMEM;
	}

	err = BrogueWindow_draw(child, child_surface);
	SDL_FreeSurface(child_surface);

	if (err)
	{
	    return err;
	}
    }

    return 0;
}

/*  Move or resize a window  */
int BrogueWindow_reposition(
    BROGUE_WINDOW *window, int x, int y, int width, int height)
{
    BROGUE_DEFERRED_DRAW **draws;

    BrogueWindow_clip(window->parent, &x, &y, &width, &height);

    draws = malloc(sizeof(BROGUE_DEFERRED_DRAW *) 
		   * width * height);
    if (draws == NULL)
    {
	return ENOMEM;
    }
    memset(draws, 0, 
	   sizeof(BROGUE_DEFERRED_DRAW *) * width * height);

    /*  Unreference all deferred draws  */
    BrogueWindow_clear(window);

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;

    free(window->draws);
    window->draws = draws;

    return 0;
}

/*  Set the background color  */
void BrogueWindow_setColor(BROGUE_WINDOW *window, BROGUE_DRAW_COLOR color)
{
    window->color = color;
}

/*  Windows are opened as visible, but can be hidden and unhidden 
    using this method  */
void BrogueWindow_setVisible(BROGUE_WINDOW *window, int visible)
{
    window->visible = visible;
}

/*  Replace the deferred draw associated with a screen character cell  */
void BrogueWindow_replaceCell(
    BROGUE_WINDOW *window, int x, int y, BROGUE_DEFERRED_DRAW *draw)
{
    BROGUE_DEFERRED_DRAW **cell;

    if (x < 0 || x >= window->width)
    {
	return;
    }

    if (y < 0 || y >= window->height)
    {
	return;
    }

    cell = &window->draws[y * window->width + x];
    if (*cell != NULL)
    {
	BrogueDeferredDraw_unref(*cell);
    }
    *cell = draw;
    if (*cell != NULL)
    {
	BrogueDeferredDraw_ref(*cell);
    }
}

/*  Clear all deferred draws from the window  */
void BrogueWindow_clear(BROGUE_WINDOW *window)
{
    int x, y;

    for (y = 0; y < window->height; y++)
    {
	for (x = 0; x < window->width; x++)
	{
	    BrogueWindow_replaceCell(window, x, y, NULL);
	}
    }
}

