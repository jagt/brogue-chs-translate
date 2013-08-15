/*
 *  sdl-draw.c
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
#include <wchar.h>

#if defined(__MMX__)
#include <mmintrin.h>
#endif

#include "Rogue.h"

#include "sdl-display.h"

typedef struct DEFERRED_CHAR DEFERRED_CHAR;
typedef struct DEFERRED_STRING DEFERRED_STRING;
typedef struct DEFERRED_GRAPHIC DEFERRED_GRAPHIC;

/*  A draw context associated with a window, which maintains drawing state  */
struct BROGUE_DRAW_CONTEXT
{
    BROGUE_WINDOW *window;

    GPtrArray *effects;

    BROGUE_DRAW_CONTEXT_STATE state;
    GList *pushed_state;
};

/*  An instance of an effect class  */
struct BROGUE_EFFECT
{
    BROGUE_DRAW_CONTEXT *context;
    int ref_count;
    int is_closed;

    BROGUE_EFFECT_CLASS *effect_class;
    void *state;
};

/*  A graphic, loaded from an external file, such as SVG  */
struct BROGUE_GRAPHIC
{
    char *path;
    int ref_count;

    RsvgHandle *svg;
};

/*  A deferred character which will fill a full display cell  */
struct DEFERRED_CHAR
{
    wchar_t c;
    int tile;

    int is_foreground_blended;
    BROGUE_DRAW_COLOR foreground;
    BROGUE_DRAW_COLOR blended_foreground[4];

    int is_background_blended;
    BROGUE_DRAW_COLOR background;
    BROGUE_DRAW_COLOR blended_background[4];
};

/*  A deferred string, which can fill multiple cells  */
struct DEFERRED_STRING
{
    int str_count;
    Uint16 **str;
    int *str_position;
    BROGUE_DRAW_COLOR *str_color;

    int is_proportional;
    BROGUE_DRAW_COLOR background;
};

/*  A deferred graphic element draw  */
struct DEFERRED_GRAPHIC
{
    BROGUE_GRAPHIC *graphic;
    SDL_Surface *render_cache;
};

/*  Convert a 0.0 - 1.0 value to 0 - 255 for storing in actual pixels  */
static int colorFloat2Byte(float c)
{
    int r = (int)(c * 255.0);

    if (r < 0)
    {
	return 0;
    }
    if (r > 255)
    {
	return 255;
    }

    return r;
}

/*  Multiply two colors together  */
static BROGUE_DRAW_COLOR multiplyColor(
    BROGUE_DRAW_COLOR a, BROGUE_DRAW_COLOR b)
{
    BROGUE_DRAW_COLOR ret;

    ret.red = a.red * b.red;
    ret.green = a.green * b.green;
    ret.blue = a.blue * b.blue;
    ret.alpha = a.alpha * b.alpha;

    return ret;
}

/*  Add two colors  */
static BROGUE_DRAW_COLOR addColor(BROGUE_DRAW_COLOR a, BROGUE_DRAW_COLOR b)
{
    BROGUE_DRAW_COLOR ret;

    ret.red = a.red + b.red;
    ret.green = a.green + b.green;
    ret.blue = a.blue + b.blue;
    ret.alpha = a.alpha + b.alpha;

    return ret;
}

/*  Average two colors, with an increasing weight interpolating toward 'b'  */
static BROGUE_DRAW_COLOR averageColor(
    BROGUE_DRAW_COLOR a, BROGUE_DRAW_COLOR b, float weight)
{
    BROGUE_DRAW_COLOR ret;

    ret.red = a.red * (1.0 - weight) + b.red * weight;
    ret.green = a.green * (1.0 - weight) + b.green * weight;
    ret.blue = a.blue * (1.0 - weight) + b.blue * weight;
    ret.alpha = a.alpha * (1.0 - weight) + b.alpha * weight;

    return ret;
}

/*  Apply the current color transform state, which applied a multiplier,
    followed by an addition and finally an average.  The transform values
    are set in the draw context state.  */
static BROGUE_DRAW_COLOR applyColorTransform(
    BROGUE_DRAW_CONTEXT_STATE *state, 
    BROGUE_DRAW_COLOR color, int is_foreground)
{
    if (is_foreground)
    {
	color = multiplyColor(color, state->foreground_multiplier);
	color = addColor(color, state->foreground_addition);
	color = averageColor(color, 
			     state->foreground_average_target, 
			     state->foreground_average_weight);
    }
    else
    {
	color = multiplyColor(color, state->background_multiplier);
	color = addColor(color, state->background_addition);
	color = averageColor(color, 
			     state->background_average_target, 
			     state->background_average_weight);
    }

    return color;
}

/*  Convert a color to SDL's integer format  */
int BrogueDisplay_colorToSDL(SDL_Surface *surface, BROGUE_DRAW_COLOR *color)
{
    int ret;

    ret = SDL_MapRGBA(surface->format, 
			   colorFloat2Byte(color->red),
			   colorFloat2Byte(color->green),
			   colorFloat2Byte(color->blue),
			   255);
    return ret;
}

/*  Convert a color to SDL's structure format  */
SDL_Color BrogueDisplay_colorToSDLColor(BROGUE_DRAW_COLOR *color)
{
    SDL_Color ret;

    ret.r = colorFloat2Byte(color->red);
    ret.g = colorFloat2Byte(color->green);
    ret.b = colorFloat2Byte(color->blue);

    return ret;
}

/*  Fill a surface with a gradient which is generated by bilinearly 
    interpolating between four corner color values.  Can take
    a source surface and multiply it into the gradient, but if 
    'src' is NULL, it will generate the gradient without multiplying  */
static void fillBlend(
    SDL_Surface *dst, SDL_Surface *src, BROGUE_DRAW_COLOR *color)
{
    int x, y;
    int lr, lg, lb, rr, rg, rb;
    int ldr, ldg, ldb, rdr, rdg, rdb;
    int w, h;
    BROGUE_DRAW_COLOR ul = color[0];
    BROGUE_DRAW_COLOR ur = color[1];
    BROGUE_DRAW_COLOR bl = color[2];
    BROGUE_DRAW_COLOR br = color[3];
#if defined(__MMX__)
    int mmx = SDL_HasMMX();
#endif
    
    w = dst->w;
    h = dst->h;

    if (src != NULL)
    {
	assert(dst->w == src->w);
	assert(dst->h == src->h);
    }

    lr = clamp(ul.red * 0xFFFF, 0, 0xFFFF);
    lg = clamp(ul.green * 0xFFFF, 0, 0xFFFF);
    lb = clamp(ul.blue * 0xFFFF, 0, 0xFFFF);

    rr = clamp(ur.red * 0xFFFF, 0, 0xFFFF);
    rg = clamp(ur.green * 0xFFFF, 0, 0xFFFF);
    rb = clamp(ur.blue * 0xFFFF, 0, 0xFFFF);

    ldr = (clamp(bl.red * 0xFFFF, 0, 0xFFFF) - lr) / h;
    ldg = (clamp(bl.green * 0xFFFF, 0, 0xFFFF) - lg) / h;
    ldb = (clamp(bl.blue * 0xFFFF, 0, 0xFFFF) - lb) / h;

    rdr = (clamp(br.red * 0xFFFF, 0, 0xFFFF) - rr) / h;
    rdg = (clamp(br.green * 0xFFFF, 0, 0xFFFF) - rg) / h;
    rdb = (clamp(br.blue * 0xFFFF, 0, 0xFFFF) - rb) / h;

    for (y = 0; y < h; y++)
    {
	unsigned char *pix;
	int dr, dg, db;
	int rpp, gpp, bpp, raccum, gaccum, baccum;

	pix = (unsigned char *)dst->pixels + dst->pitch * y;

	dr = rr - lr;
	dg = rg - lg;
	db = rb - lb;

	rpp = dr / w;
	gpp = dg / w;
	bpp = db / w;

	raccum = lr;
	gaccum = lg;
	baccum = lb;

	lr += ldr;
	lg += ldg;
	lb += ldb;

	rr += rdr;
	rg += rdg;
	rb += rdb;

	if (src != NULL)
	{
	    unsigned char *src_pix = (unsigned char *)src->pixels 
		+ src->pitch * y;
	    x = w;

#if defined(__MMX__)
	    /*  MMX is significantly faster.  Use it if the CPU supports it  */
	    if (mmx)
	    {
		__m64 mmx_zero = _m_from_int(0);

		long long ll_color = 
		    ((long long)0xFFFF << 48)
		    | ((long long)raccum << 32)
		    | ((long long)gaccum << 16)
		    | ((long long)baccum);
		__m64 mmx_color = *(__m64 *)&ll_color;

		long long ll_pp = 
		    ((long long)(rpp & 0xFFFF) << 32)
		    | ((long long)(gpp & 0xFFFF) << 16)
		    | ((long long)(bpp & 0xFFFF));
		__m64 mmx_pp = *(__m64 *)&ll_pp;

		while (x >= 2)
		{
		    __m64 src_pair = *(__m64 *)src_pix;
		
		    /*  Separate the left pixel and right pixel  */
		    __m64 left_pix = _mm_unpacklo_pi8(src_pair, mmx_zero);
		    __m64 right_pix = _mm_unpackhi_pi8(src_pair, mmx_zero);
		
		    /*  Multiply the left source by the gradient color  */
		    left_pix = _mm_mullo_pi16(left_pix, 
					      _mm_srli_pi16(mmx_color, 8));
		    /*  Advance the gradient color for the next pixel */
		    mmx_color = _mm_add_pi16(mmx_color, mmx_pp);

		    /*  Multiply the right source by the gradient color  */
		    right_pix = _mm_mullo_pi16(right_pix,
					       _mm_srli_pi16(mmx_color, 8));
		    /*  Advance the gradient  */
		    mmx_color = _mm_add_pi16(mmx_color, mmx_pp); 

		    /*  Recombine the pixels  */
		    __m64 result_pix = _mm_packs_pu16(
			_mm_srli_pi16(left_pix, 8),
			_mm_srli_pi16(right_pix, 8));
		    
		    *(__m64 *)pix = result_pix;

		    src_pix += 8;
		    pix += 8;
		    x -= 2;
		}

		/*  Extract the accumulated gradient value for the potential
		    odd remaining pixel  */
		short *s_color = (short *)&mmx_color;
		raccum = s_color[2];
		gaccum = s_color[1];
		baccum = s_color[0];
	    }
#endif

	    /*  The equivalent slow loop for odd pixels or CPUs without MMX  */
	    while (x > 0)
	    {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		pix[3] = src_pix[3];
		pix[2] = (src_pix[2] * raccum) >> 16;
		pix[1] = (src_pix[1] * gaccum) >> 16;
		pix[0] = (src_pix[0] * baccum) >> 16;
#else
		pix[0] = src_pix[0];
		pix[1] = (src_pix[1] * raccum) >> 16;
		pix[2] = (src_pix[2] * gaccum) >> 16;
		pix[3] = (src_pix[3] * baccum) >> 16;
#endif

		raccum += rpp;
		gaccum += gpp;
		baccum += bpp;

		src_pix += 4;
		pix += 4;
		x--;
	    }
	}
	else
	{
	    /*  If no source surface, we'll just generate the gradient  */
	    x = w;

#if defined(__MMX__)
	    /*  MMX is significantly faster.  Use it if the CPU supports it  */
	    if (mmx)
	    {
		long long ll_color = 
		    ((long long)0xFFFF << 48)
		    | ((long long)raccum << 32)
		    | ((long long)gaccum << 16)
		    | ((long long)baccum);
		__m64 mmx_color = *(__m64 *)&ll_color;

		long long ll_pp = 
		    ((long long)(rpp & 0xFFFF) << 32)
		    | ((long long)(gpp & 0xFFFF) << 16)
		    | ((long long)(bpp & 0xFFFF));
		__m64 mmx_pp = *(__m64 *)&ll_pp;

		while (x >= 2)
		{
		    /*  Store the left pixel  */
		    __m64 left_pix = _mm_srli_pi16(mmx_color, 8);
		    /*  Advance the gradient  */
		    mmx_color = _mm_add_pi16(mmx_color, mmx_pp);
		    /*  Store the right pixel  */
		    __m64 right_pix = _mm_srli_pi16(mmx_color, 8);
		    /*  Advance the gradient  */
		    mmx_color = _mm_add_pi16(mmx_color, mmx_pp); 

		    /*  Recombine the pixels  */
		    __m64 result_pix = _mm_packs_pu16(left_pix, right_pix);

		    *(__m64 *)pix = result_pix;
		    
		    pix += 8;
		    x -= 2;
		}

		/*  Extract the accumulated gradient value for the potential
		    odd remaining pixel  */
		short *s_color = (short *)&mmx_color;
		raccum = s_color[2];
		gaccum = s_color[1];
		baccum = s_color[0];
	    }
#endif

	    /*  The slow loop for odd pixels or MMU-less CPUs  */
	    while (x > 0)
	    {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		pix[2] = raccum >> 8;
		pix[1] = gaccum >> 8;
		pix[0] = baccum >> 8;
#else
		pix[1] = raccum >> 8;
		pix[2] = gaccum >> 8;
		pix[3] = baccum >> 8;
#endif

		raccum += rpp;
		gaccum += gpp;
		baccum += bpp;

		x--;
		pix += 4;
	    }
	}
    }

#if defined(__MMX__)
    /*  Empty the MMX state to avoid interfering with future FPU results  */
    if (mmx)
    {
	_mm_empty();
    }
#endif
}

/*  Just-in-time rendering of a character filling a full cell  */
static int deferredChar(
    BROGUE_DISPLAY *display, void *data, SDL_Surface *surface)
{
    DEFERRED_CHAR *param = (DEFERRED_CHAR *)data;
    wchar_t c = param->c;
    int tile = param->tile;
    int need_free_glyph = 0;
    int err;
    BROGUE_DRAW_COLOR fg = param->foreground;
    SDL_Color sdl_fg = { 255, 255, 255, 255 };
    SDL_Rect rect = { 0, 0, surface->w, surface->h };
    SDL_Surface *glyph;
    int bg_color;

    if (param->is_background_blended)
    {
	fillBlend(surface, NULL, param->blended_background);
    }
    else if (param->background.alpha > 0.0)
    {
	bg_color = BrogueDisplay_colorToSDL(surface, &param->background);
	SDL_FillRect(surface, NULL, bg_color);
    }

    if (!param->is_foreground_blended)
    {
	sdl_fg = BrogueDisplay_colorToSDLColor(&fg);
    }

    glyph = NULL;
    if (tile)
    {
	int frame = SDL_GetTicks() / FRAME_TIME;

	if (param->is_foreground_blended)
	{
	    /*  We'll use a 'render_glyph' rather than allocating a 
		new surface for the common case for the dungeon grid
	        where the foreground is gradient blended.  */
	    glyph = SdlSvgset_getGlyph(display->svgset, c, frame);
	    fillBlend(display->render_glyph, glyph, param->blended_foreground);
	    glyph = display->render_glyph;
	}
	else
	{
	    glyph = SdlSvgset_render(display->svgset, 
				     c, frame, sdl_fg);
	    need_free_glyph = 1;
	}
    }

    if (glyph == NULL)
    {
	TTF_Font *font = display->mono_font;
	int minx, maxx, miny, maxy;

	glyph = TTF_RenderGlyph_Blended(font, c, sdl_fg);
	if (glyph == NULL)
	{
	    return EINVAL;
	}

	need_free_glyph = 1;
	err = TTF_GlyphMetrics(font, c,
			       &minx, &maxx, &miny, &maxy, NULL);
	if (err)
	{
	    SDL_FreeSurface(glyph);
	    return err;
	}
	
	/*  Center the glyph horizontally, adjust for descenders vertically. */
	rect.x += (display->font_width - maxx + minx) / 2;
	rect.y += (display->font_height - maxy) + 
	    display->font_descent - 1;

	if (param->is_foreground_blended)
	{
	    fillBlend(glyph, glyph, param->blended_foreground);
	}
    }

    if (glyph != NULL)
    {
	SDL_BlitSurface(glyph, NULL, surface, &rect);
	if (need_free_glyph)
	{
	    SDL_FreeSurface(glyph);
	}
    }

    return 0;
}

/*  Deferred characters don't have internal pointers which need
    freeing, so they can use this generic free method, but other
    structures which have internal pointers use other free methods  */
static void freeParam(void *param)
{
    free(param);
}

/*  A just-in-time rendered string, possibly using multiple display
    cells, and perhaps rendered using a proportional font  */
static int deferredString(
    BROGUE_DISPLAY *display, void *data, SDL_Surface *surface)
{
    DEFERRED_STRING *param = (DEFERRED_STRING *)data;
    SDL_Rect rect = { 0, 0, surface->w, surface->h };
    SDL_Surface *font_surface;
    int bg_color;
    TTF_Font *font = BrogueDisplay_getFont(display, param->is_proportional);
    int i;
    SDL_Color sdl_fg;

    if (param->background.alpha > 0.0)
    {
	bg_color = BrogueDisplay_colorToSDL(surface, &param->background);
	SDL_FillRect(surface, NULL, bg_color);
    }

    for (i = 0; i < param->str_count; i++)
    {
	BROGUE_DRAW_COLOR fg = param->str_color[i];

	if (param->str[i][0] == 0)
	{
	    continue;
	}

	if (param->str_position[i])
	{
	    rect.x = param->str_position[i];
	}

	sdl_fg = BrogueDisplay_colorToSDLColor(&fg);
	font_surface = TTF_RenderUNICODE_Blended(font, param->str[i], sdl_fg);

	if (font_surface == NULL)
	{
	    return ENOMEM;
	}
	
	SDL_BlitSurface(font_surface, NULL, surface, &rect);

	rect.x += font_surface->w;

	SDL_FreeSurface(font_surface);
    }

    return 0;
}

/*  The free routing for a deferred string draw.  Frees all associated
    memory.  */
static void freeDeferredString(void *data)
{
    int i;
    DEFERRED_STRING *param = (DEFERRED_STRING *)data;

    for (i = 0; i < param->str_count; i++)
    {
	free(param->str[i]);
    }

    free(param->str_position);
    free(param->str);
    free(param);
}

/*  A deferred file-loaded graphic element  */
static int deferredGraphic(
    BROGUE_DISPLAY *display, void *data, SDL_Surface *surface)
{
    DEFERRED_GRAPHIC *param = (DEFERRED_GRAPHIC *)data;
    cairo_surface_t *cairo_surface;
    cairo_t *cairo;
    RsvgDimensionData dimensions;
    int need_render = 0;

    if (param->render_cache == NULL)
    {
	need_render = 1;
    }
    else if (param->render_cache->w != surface->w 
	     || param->render_cache->h != surface->h)
    {
	need_render = 1;
    }

    /*  Only regenerate the surface from SVG if the dimensions have changed  */
    if (need_render)
    {
	if (param->render_cache != NULL)
	{
	    SDL_FreeSurface(param->render_cache);
	}

	param->render_cache = SDL_CreateRGBSurface(
	    0, surface->w, surface->h, 32,
	    0x00FF0000, 0x0000FF00, 0x000000FF, 
	    0xFF000000);

	if (param->render_cache == NULL)
	{
	    return ENOMEM;
	}

	cairo_surface = cairo_image_surface_create_for_data(
	    param->render_cache->pixels, CAIRO_FORMAT_ARGB32, 
	    param->render_cache->w, param->render_cache->h, 
	    param->render_cache->pitch);
	if (cairo_surface == NULL)
	{
	    SDL_FreeSurface(param->render_cache);
	    param->render_cache = NULL;
	    return ENOMEM;
	}
	
	cairo = cairo_create(cairo_surface);
	if (cairo == NULL)
	{
	    cairo_surface_destroy(cairo_surface);
	    SDL_FreeSurface(param->render_cache);
	    param->render_cache = NULL;
	    return ENOMEM;
	}
	
	rsvg_handle_get_dimensions(param->graphic->svg, &dimensions);
	cairo_scale(cairo, 
		    (double)param->render_cache->w / (double)dimensions.width,
		    (double)param->render_cache->h / (double)dimensions.height);
	rsvg_handle_render_cairo(param->graphic->svg, cairo);
	
	cairo_destroy(cairo);
	cairo_surface_destroy(cairo_surface);

	Sdl_undoPremultipliedAlpha(param->render_cache);
    }
  
    SDL_BlitSurface(param->render_cache, NULL, surface, NULL);
    
    return 0;
}

/*  Free a deferred graphic element  */
static void freeDeferredGraphic(void *data)
{
    DEFERRED_GRAPHIC *param = (DEFERRED_GRAPHIC *)data;

    if (param->render_cache != NULL)
    {
	SDL_FreeSurface(param->render_cache);
    }

    BrogueGraphic_unref(param->graphic);
    free(param);
}

/*  Open a drawing context for drawing to a window  */
BROGUE_DRAW_CONTEXT *BrogueDrawContext_open(BROGUE_WINDOW *window)
{
    BROGUE_DRAW_CONTEXT *context;
    BROGUE_DRAW_COLOR fg = { 1.0, 1.0, 1.0, 1.0 };

    context = (BROGUE_DRAW_CONTEXT *)malloc(sizeof(BROGUE_DRAW_CONTEXT));
    if (context == NULL)
    {
	return NULL;
    }
    memset(context, 0, sizeof(BROGUE_DRAW_CONTEXT));

    context->window = window;
    context->state.foreground = fg;
    context->state.foreground_multiplier = fg;
    context->state.background_multiplier = fg;
    context->pushed_state = NULL;

    context->effects = g_ptr_array_new();
    if (context->effects == NULL)
    {
	free(context);
	return NULL;
    }

    g_ptr_array_add(window->contexts, context);

    return context;
}

/*  Close the drawing context, freeing all resources  */
void BrogueDrawContext_close(BROGUE_DRAW_CONTEXT *context)
{
    while (g_list_last(context->pushed_state) != NULL)
    {
	BrogueDrawContext_pop(context);
    }
    g_list_free(context->pushed_state);

    while(context->effects->len)
    {
	int count = context->effects->len;

	BROGUE_EFFECT *effect = 
	    (BROGUE_EFFECT *)g_ptr_array_index(context->effects, 0);
	BrogueEffect_close(effect);

	assert(context->effects->len < count);
    }

    free(context->state.tab_stops);

    g_ptr_array_remove(context->window->contexts, context);
    free(context);
}

/*  Save the current state of the drawing context for later
    restoration using pop  */
void BrogueDrawContext_push(BROGUE_DRAW_CONTEXT *context)
{
    BROGUE_DRAW_CONTEXT_STATE *state;

    state = malloc(sizeof(BROGUE_DRAW_CONTEXT_STATE));
    assert(state != NULL);

    memcpy(state, &context->state, sizeof(BROGUE_DRAW_CONTEXT_STATE));
    context->pushed_state = g_list_append(context->pushed_state, state);

    if (context->state.effect != NULL)
    {
	BrogueEffect_ref(context->state.effect);
    }

    if (context->state.tab_stops)
    {
	context->state.tab_stops = malloc(
	    sizeof(int) * context->state.tab_stop_count);
	memcpy(context->state.tab_stops, state->tab_stops, 
	       sizeof(int) * context->state.tab_stop_count);
    }
}

/*  Restore the saved drawing context state  */
void BrogueDrawContext_pop(BROGUE_DRAW_CONTEXT *context)
{
    BROGUE_DRAW_CONTEXT_STATE *state;

    GList *link = g_list_last(context->pushed_state);
    assert(link != NULL);

    /*  Unreference the current effect  */
    if (context->state.effect != NULL)
    {
	BrogueEffect_unref(context->state.effect);
    }

    free(context->state.tab_stops);

    state = (BROGUE_DRAW_CONTEXT_STATE *)link->data;
    memcpy(&context->state, state, sizeof(BROGUE_DRAW_CONTEXT_STATE));
    free(state);

    context->pushed_state = g_list_delete_link(context->pushed_state, link);
}

/*  Find the global screen position of local drawing coordinates by
    traversing the window hierarchy  */
void BrogueDrawContext_getScreenPosition(
    BROGUE_DRAW_CONTEXT *context, int *x, int *y)
{
    BROGUE_WINDOW *window;

    *x = 0;  
    *y = 0;

    window = context->window;
    while (window != NULL)
    {
	*x += window->x;
	*y += window->y;

	window = window->parent;
    }
}

/*  Enable use of the proportional font, as opposed to monospaced  */
void BrogueDrawContext_enableProportionalFont(
    BROGUE_DRAW_CONTEXT *context, int enable)
{
    context->state.proportional_enable = enable;
}

/*  Enable drawing with tiles when drawing single characters  */
void BrogueDrawContext_enableTiles(
    BROGUE_DRAW_CONTEXT *context, int enable)
{
    context->state.tile_enable = enable;
}

/*  Enable automatic wrapping given left and right margins  */
void BrogueDrawContext_enableWrap(
    BROGUE_DRAW_CONTEXT *context, int left_column, int right_column)
{
    context->state.wrap_enable = 1;
    context->state.wrap_left = left_column;
    context->state.wrap_right = right_column;
}

/*  Disable automatic wrapping  */
void BrogueDrawContext_disableWrap(
    BROGUE_DRAW_CONTEXT *context)
{
    context->state.wrap_enable = 0;
    context->state.wrap_left = 0;
    context->state.wrap_right = 0;
}

/*  Enable justification for string drawing  */
void BrogueDrawContext_enableJustify(
    BROGUE_DRAW_CONTEXT *context, int left_column, int right_column, 
    int justify_mode)
{
    context->state.justify_mode = justify_mode;
    context->state.wrap_left = left_column;
    context->state.wrap_right = right_column;
}

/*  Set the current effect for future drawing  */
void BrogueDrawContext_setEffect(
    BROGUE_DRAW_CONTEXT *context, BROGUE_EFFECT *effect)
{
    if (effect != NULL)
    {
	BrogueEffect_ref(effect);
    }
    if (context->state.effect != NULL)
    {
	BrogueEffect_unref(context->state.effect);
    }
    context->state.effect = effect;
}

/*  Set a solid foreground color  */
void BrogueDrawContext_setForeground(
    BROGUE_DRAW_CONTEXT *context, BROGUE_DRAW_COLOR color)
{
    context->state.is_foreground_blended = 0;

    context->state.foreground = applyColorTransform(&context->state, color, 1);
}

/*  Set a solid background color  */
void BrogueDrawContext_setBackground(
    BROGUE_DRAW_CONTEXT *context, BROGUE_DRAW_COLOR color)
{
    context->state.is_background_blended = 0;

    context->state.background = applyColorTransform(&context->state, color, 0);
}

/*  Set a blended foreground color, for use with single character drawing  */
void BrogueDrawContext_blendForeground(
    BROGUE_DRAW_CONTEXT *context,
    BROGUE_DRAW_COLOR upper_left, BROGUE_DRAW_COLOR upper_right,
    BROGUE_DRAW_COLOR lower_left, BROGUE_DRAW_COLOR lower_right)
{
    context->state.is_foreground_blended = 1;

    context->state.blended_foreground[0] = 
	applyColorTransform(&context->state, upper_left, 1);
    context->state.blended_foreground[1] = 
	applyColorTransform(&context->state, upper_right, 1);
    context->state.blended_foreground[2] = 
	applyColorTransform(&context->state, lower_left, 1);
    context->state.blended_foreground[3] = 
	applyColorTransform(&context->state, lower_right, 1);
}

/*  Set a blended background color, for use with single character drawing  */
void BrogueDrawContext_blendBackground(
    BROGUE_DRAW_CONTEXT *context, 
    BROGUE_DRAW_COLOR upper_left, BROGUE_DRAW_COLOR upper_right,
    BROGUE_DRAW_COLOR lower_left, BROGUE_DRAW_COLOR lower_right)
{
    context->state.is_background_blended = 1;

    context->state.blended_background[0] = 
	applyColorTransform(&context->state, upper_left, 0);
    context->state.blended_background[1] = 
	applyColorTransform(&context->state, upper_right, 0);
    context->state.blended_background[2] = 
	applyColorTransform(&context->state, lower_left, 0);
    context->state.blended_background[3] = 
	applyColorTransform(&context->state, lower_right, 0);
}

/*  Set a color multiplier, for either foreground or background  */
void BrogueDrawContext_setColorMultiplier(
    BROGUE_DRAW_CONTEXT *context, BROGUE_DRAW_COLOR color, int is_foreground)
{
    if (is_foreground)
    {
	context->state.foreground_multiplier = color;
    }
    else
    {
	context->state.background_multiplier = color;
    }
}

/*  Set a color addition, to be applied after the multiplier  */
void BrogueDrawContext_setColorAddition(
    BROGUE_DRAW_CONTEXT *context, BROGUE_DRAW_COLOR color, int is_foreground)
{
    if (is_foreground)
    {
	context->state.foreground_addition = color;
    }
    else
    {
	context->state.background_addition = color;
    }
}

/*  Set a color weighted target, to be applied after the color addition  */
void BrogueDrawContext_setColorAverageTarget(
    BROGUE_DRAW_CONTEXT *context, 
    float weight, BROGUE_DRAW_COLOR color, int is_foreground)
{
    if (is_foreground)
    {
	context->state.foreground_average_target = color;
	context->state.foreground_average_weight = weight;
    }
    else
    {
	context->state.background_average_target = color;
	context->state.background_average_weight = weight;
    }
}

/*  Set the tab stops which are used when drawing a string with 
    embeded tab characters  */
int BrogueDrawContext_setTabStops(
    BROGUE_DRAW_CONTEXT *context, int stop_count, int *stops)
{
    free(context->state.tab_stops);
    context->state.tab_stops = NULL;

    context->state.tab_stop_count = stop_count;
    if (stop_count > 0)
    {
	context->state.tab_stops = malloc(sizeof(int) * stop_count);
	if (context->state.tab_stops == NULL)
	{
	    return ENOMEM;
	}
	memcpy(context->state.tab_stops, stops, sizeof(int) * stop_count);
    }

    return 0;
}

/*  Draw an individual character  */
void BrogueDrawContext_drawChar(
    BROGUE_DRAW_CONTEXT *context, int x, int y, wchar_t c)
{
    BROGUE_DEFERRED_DRAW *draw;
    DEFERRED_CHAR *defer;

    defer = (DEFERRED_CHAR *)malloc(sizeof(DEFERRED_CHAR));
    if (defer == NULL)
    {
	return;
    }
    memset(defer, 0, sizeof(DEFERRED_CHAR));

    defer->is_foreground_blended = context->state.is_foreground_blended;
    defer->foreground = context->state.foreground;
    memcpy(defer->blended_foreground, context->state.blended_foreground,
	   sizeof(BROGUE_DRAW_COLOR) * 4);
    defer->is_background_blended = context->state.is_background_blended;
    defer->background = context->state.background;
    memcpy(defer->blended_background, context->state.blended_background,
	   sizeof(BROGUE_DRAW_COLOR) * 4);
    defer->tile = context->state.tile_enable;
    defer->c = c;

    draw = BrogueDeferredDraw_create(
	context->window, deferredChar, freeParam, defer, x, y, 1, 1);
    if (draw == NULL)
    {
	free(defer);
	return;
    }

    BrogueWindow_replaceCell(context->window, x, y, draw);
    BrogueDeferredDraw_unref(draw);
}

/*  Measure the pixel length of a substring  */
static int measureSubstring(
    BROGUE_DRAW_CONTEXT *context, const wchar_t *str, int len, int x)
{
    TTF_Font *font;
    Uint16 *uni;
    int i, w, h, uni_index, tab_index;
    int font_width = context->window->display->font_width;

    if (context->state.proportional_enable)
    {
	font = context->window->display->sans_font;
    }
    else
    {
	font = context->window->display->mono_font;
    }

    uni = (Uint16 *)malloc(sizeof(Uint16) * (len + 1));
    uni_index = 0;
    tab_index = 0;
    for (i = 0; i < len; i++)
    {
	if (str[i] == COLOR_ESCAPE || str[i] == '*' || str[i] == '\t') 
	{
	    uni[uni_index] = 0;
	    TTF_SizeUNICODE(font, uni, &w, &h);
	    x += w;

	    uni_index = 0;

	    if (str[i] == COLOR_ESCAPE)
	    {
		if (i + 3 < len)
		{
		    i += 3;
		}
	    } 
	    else if (str[i] == '*')
	    {
		x += font_width;
	    }
	    else if (str[i] == '\t')
	    {
		if (tab_index < context->state.tab_stop_count)
		{
		    x = context->state.tab_stops[tab_index++] * font_width;
		}
	    }
	}
	else
	{
	    uni[uni_index++] = str[i];
	}
    }
    uni[uni_index] = 0;
    TTF_SizeUNICODE(font, uni, &w, &h);
    x += w;
    
    free(uni);

    return x;
}

/*  Convert a substring into a deferred string drawing representation  */
static void drawSubstring(
    BROGUE_DRAW_CONTEXT *context, const wchar_t *str, int len, int x, int y)
{
    DEFERRED_STRING *defer;
    BROGUE_DEFERRED_DRAW *draw;
    BROGUE_DRAW_COLOR color;
    int i, j, str_begin, start_x, end_x, escape_count, str_index, tab_index;
    int font_width = context->window->display->font_width;

    if (len == 0)
    {
	return;
    }

    defer = (DEFERRED_STRING *)malloc(sizeof(DEFERRED_STRING));
    memset(defer, 0, sizeof(DEFERRED_STRING));

    escape_count = 0;
    for (i = 0; i < len; i++)
    {
	if (str[i] == COLOR_ESCAPE || str[i] == '\t')
	{
	    escape_count++;

	    if (str[i] == COLOR_ESCAPE && i + 3 < len)
	    {
		i += 3;
	    }
	}
    }

    defer->str_count = escape_count + 1;
    defer->str = malloc(sizeof(Uint16 **) * defer->str_count);
    memset(defer->str, 0, sizeof(Uint16 **) * defer->str_count);
    defer->str_color = malloc(sizeof(BROGUE_DRAW_COLOR) * defer->str_count);
    memset(defer->str_color, 0, sizeof(BROGUE_DRAW_COLOR) * defer->str_count);
    defer->str_position = malloc(sizeof(int) * defer->str_count);
    memset(defer->str_position, 0, sizeof(int) * defer->str_count);
    start_x = (x / font_width) * font_width;
    defer->str_position[0] = x - start_x;

    color = context->state.foreground;

    str_index = 0;
    str_begin = 0;
    tab_index = 0;
    for (i = 0; i < len; i++)
    {
	if (str[i] == COLOR_ESCAPE || str[i] == '\t')
	{
	    defer->str[str_index] = malloc(
		sizeof(Uint16) * (i - str_begin + 1));
	    for (j = str_begin; j < i; j++)
	    {
		defer->str[str_index][j - str_begin] = str[j];
	    }
	    defer->str[str_index][j - str_begin] = 0;
	    defer->str_color[str_index] = color;
	    str_index++;

	    if (str[i] == COLOR_ESCAPE)
	    {
		if (i + 3 < len)
		{
		    color.red = (str[i + 1] - COLOR_VALUE_INTERCEPT) / 100.0;
		    color.green = (str[i + 2] - COLOR_VALUE_INTERCEPT) / 100.0;
		    color.blue = (str[i + 3] - COLOR_VALUE_INTERCEPT) / 100.0;

		    color = applyColorTransform(&context->state, color, 1);

		    i += 3;
		}
	    }
	    else if (str[i] == '\t')
	    {
		if (tab_index < context->state.tab_stop_count
		    && str_index < defer->str_count)
		{
		    defer->str_position[str_index] = 
			context->state.tab_stops[tab_index++]
			* font_width - start_x;
		}
	    }

	    str_begin = i + 1;
	}
    }
    assert(str_index == defer->str_count - 1);

    defer->str[str_index] = malloc(
	sizeof(Uint16) * (i - str_begin + 1));
    for (j = str_begin; j < i; j++)
    {
	defer->str[str_index][j - str_begin] = str[j];
    }
    defer->str[str_index][j - str_begin] = 0;
    defer->str_color[str_index] = color;

    context->state.foreground = color;

    defer->background = context->state.background;
    defer->is_proportional = context->state.proportional_enable;

    end_x = measureSubstring(context, str, len, x);
    draw = BrogueDeferredDraw_create(
	context->window, deferredString, freeDeferredString, defer, 
	x / font_width, y, (end_x - start_x + font_width - 1) / font_width, 1);
    if (draw == NULL)
    {
	free(defer->str);
	free(defer->str_color);
	free(defer);
	return;
    }

    BrogueWindow_replaceCell(context->window, x / font_width, y, draw);
    BrogueDeferredDraw_unref(draw);
}

/*  Draw a string, which might be split into multiple deferred string
    draws internally, applying justification and wrapping automatically  */
int BrogueDrawContext_drawString(
    BROGUE_DRAW_CONTEXT *context, int x, int y, const wchar_t *str)
{
    int i, begin_word, begin_line, begin_line_x, font_width;
    BROGUE_EFFECT *effect;

    font_width = context->window->display->font_width;

    effect = context->state.effect;
    if (effect != NULL && effect->effect_class != NULL)
    {
	BROGUE_DEFERRED_DRAW *draw;

	draw = effect->effect_class->draw_string_func(
	    context->window, &context->state, effect->state, x, y, str);
	if (draw != NULL)
	{
	    BrogueWindow_replaceCell(context->window, x, y, draw);
	    BrogueDeferredDraw_unref(draw);

	    return 0;
	}
    }

    if (context->state.justify_mode == BROGUE_JUSTIFY_LEFT)
    {
	begin_word = 0;
	begin_line = 0;
	begin_line_x = x * font_width;

	for (i = 0; str[i]; i++)
	{
	    if (str[i] == '\n')
	    {
		drawSubstring(
		    context, &str[begin_line], i - begin_line,
		    begin_line_x, y);

		begin_line = i + 1;
		begin_line_x = context->state.wrap_left * font_width;
		begin_word = i + 1;
		y++;
	    }
	    else if (str[i] == ' ')
	    {
		begin_word = i + 1;
	    }
	    else
	    {
		x = measureSubstring(
		    context, &str[begin_line], i - begin_line + 1,
		    begin_line_x);
		
		if (context->state.wrap_enable 
		    && x >= context->state.wrap_right * font_width
		    && begin_word != begin_line)
		{
		    drawSubstring(
			context, &str[begin_line], begin_word - begin_line,
			begin_line_x, y);

		    begin_line_x = context->state.wrap_left * font_width;
		    begin_line = begin_word;
		    y++;
		}
	    }
	}

	drawSubstring(
	    context, &str[begin_line], i - begin_line,
	    begin_line_x, y);
    }
    else
    {
	int w = measureSubstring(context, str, wcslen(str), 0);

	if (context->state.justify_mode == BROGUE_JUSTIFY_CENTER)
	{
	    int pos;

	    pos = context->state.wrap_left * font_width +
		((context->state.wrap_right - context->state.wrap_left) 
		 * font_width - w) / 2;
	    drawSubstring(
		context, str, wcslen(str),
		pos, y);
	}
	else
	{
	    assert(context->state.justify_mode == BROGUE_JUSTIFY_RIGHT);

	    drawSubstring(
		context, str, wcslen(str),
		context->state.wrap_right * font_width - w, y);
	}
    }

    return 0;
}

/*  Measure the size of a string, as it would be drawn with current
    draw context settings  */
BROGUE_TEXT_SIZE BrogueDrawContext_measureString(
    BROGUE_DRAW_CONTEXT *context, int x, const wchar_t *str)
{
    BROGUE_TEXT_SIZE size;
    int h = 1;
    int minx, maxx;
    int i, begin_word, begin_line, begin_line_x, font_width;

    font_width = context->window->display->font_width;

    minx = maxx = x * font_width;

    begin_word = 0;
    begin_line = 0;
    begin_line_x = x * font_width;

    for (i = 0; str[i]; i++)
    {
	if (str[i] == '\n')
	{
	    x = measureSubstring(
		context, &str[begin_line], i - begin_line,
		begin_line_x);

	    if (x > maxx)
	    {
		maxx = x;
	    }

	    begin_line = i + 1;
	    begin_line_x = context->state.wrap_left * font_width;
	    begin_word = i + 1;
	    h++;

	    if (begin_line_x < minx)
	    {
		minx = begin_line_x;
	    }
	}
	else if (str[i] == ' ')
	{
	    begin_word = i + 1;
	}
	else
	{
	    x = measureSubstring(
		context, &str[begin_line], i - begin_line + 1,
		begin_line_x);

	    if (context->state.wrap_enable 
		&& x >= context->state.wrap_right * font_width
		&& begin_word != begin_line)
	    {
		x = measureSubstring(
		    context, &str[begin_line], begin_word - begin_line,
		    begin_line_x);

		if (x > maxx)
		{
		    maxx = x;
		}

		begin_line_x = context->state.wrap_left * font_width;
		begin_line = begin_word;
		h++;

		if (begin_line_x < minx)
		{
		    minx = begin_line_x;
		}
	    }
	}
    }

    x = measureSubstring(
	context, &str[begin_line], i - begin_line,
	begin_line_x);
    if (x > maxx)
    {
	maxx = x;
    }

    size.width = (maxx - minx + font_width - 1) / font_width;
    size.height = h;
    size.final_column = (x + font_width - 1) / font_width;

    return size;
}

/*  Draw an ASCII, as opposed to wide-character, string  */
int BrogueDrawContext_drawAsciiString(
    BROGUE_DRAW_CONTEXT *context, int x, int y, const char *str)
{
    wchar_t *ustr;
    int i, err;

    ustr = malloc(sizeof(wchar_t) * (strlen(str) + 1));
    if (ustr == NULL)
    {
	return ENOMEM;
    }

    for (i = 0; str[i]; i++)
    {
	ustr[i] = str[i];
    }
    ustr[i] = 0;

    err = BrogueDrawContext_drawString(context, x, y, ustr);

    free(ustr);

    return err;
}

/*  Measure the drawn size of an ASCII string  */
BROGUE_TEXT_SIZE BrogueDrawContext_measureAsciiString(
    BROGUE_DRAW_CONTEXT *context, int x, const char *str)
{
    wchar_t *ustr;
    int i = 0;
    BROGUE_TEXT_SIZE size = { 0, 0 };

    ustr = malloc(sizeof(wchar_t) * (strlen(str) + 1));
    if (ustr == NULL)
    {
	return size;
    }

    for (i = 0; str[i]; i++)
    {
	ustr[i] = str[i];
    }
    ustr[i] = 0;

    size = BrogueDrawContext_measureString(context, x, ustr);

    free(ustr);
    return size;
}

/*  Draw a graphic which has been loaded from an external SVG  */
int BrogueDrawContext_drawGraphic(
    BROGUE_DRAW_CONTEXT *context, 
    int x, int y, int width, int height, BROGUE_GRAPHIC *graphic)
{
    DEFERRED_GRAPHIC *deferred_graphic;
    BROGUE_DEFERRED_DRAW *draw;

    deferred_graphic = malloc(sizeof(DEFERRED_GRAPHIC));
    if (deferred_graphic == NULL)
    {
	return ENOMEM;
    }
    memset(deferred_graphic, 0, sizeof(DEFERRED_GRAPHIC));
    
    deferred_graphic->graphic = graphic;
    BrogueGraphic_ref(deferred_graphic->graphic);

    draw = BrogueDeferredDraw_create(
	context->window, deferredGraphic, freeDeferredGraphic, 
	deferred_graphic, x, y, width, height);
    if (draw == NULL)
    {
	BrogueGraphic_unref(deferred_graphic->graphic);
	free(deferred_graphic);
	return ENOMEM;
    }

    BrogueWindow_replaceCell(context->window, x, y, draw);

    return 0;
}

/*  Open an effect instance by class name, to be used for custom
    drawing of future primitives  */
BROGUE_EFFECT *BrogueEffect_open(
    BROGUE_DRAW_CONTEXT *context, const wchar_t *effect_name)
{
    BROGUE_EFFECT_CLASS *effect_class;
    BROGUE_EFFECT *effect;

    effect_class = g_hash_table_lookup(
	context->window->display->effect_classes, effect_name);

    effect = malloc(sizeof(BROGUE_EFFECT));
    if (effect == NULL)
    {
	return NULL;
    }
    memset(effect, 0, sizeof(BROGUE_EFFECT));

    effect->effect_class = effect_class;
    effect->context = context;
    effect->ref_count = 1;
    if (effect->effect_class != NULL)
    {
	effect->state = effect->effect_class->new_func();
    }

    g_ptr_array_add(context->effects, effect);

    return effect;
}

/*  Increment the reference count  */
void BrogueEffect_ref(BROGUE_EFFECT *effect)
{
    effect->ref_count++;
}

/*  Decrement the effect reference count, freeing if zero  */
void BrogueEffect_unref(BROGUE_EFFECT *effect)
{
    if (--effect->ref_count == 0)
    {
	if (effect->effect_class != NULL)
	{
	    effect->effect_class->destroy_func(effect->state);
	}
	g_ptr_array_remove(effect->context->effects, effect);
	free(effect);
    }
}

/*  Close the effect.  Assumes that at least our references is still active. */
void BrogueEffect_close(BROGUE_EFFECT *effect)
{
    assert(!effect->is_closed);

    effect->is_closed = 1;
    BrogueEffect_unref(effect);
}

/*  Pass the effect parameter structure to the effect implementation  */
int BrogueEffect_setParameters(BROGUE_EFFECT *effect, void *param)
{
    if (effect->effect_class != NULL)
    {
	return effect->effect_class->set_param_func(
	    effect->state, param);
    }
    else
    {
	return 0;
    }
}

/*  Create a deferred drawing object for later direct rendering  */
BROGUE_DEFERRED_DRAW *BrogueDeferredDraw_create(
    BROGUE_WINDOW *window,
    BROGUE_DEFERRED_DRAW_FUNC draw_func, 
    BROGUE_DEFERRED_FREE_FUNC free_func,
    void *draw_param, 
    int x, int y, int width, int height)
{
    BROGUE_DEFERRED_DRAW *draw;

    draw = malloc(sizeof(BROGUE_DEFERRED_DRAW));
    if (draw == NULL)
    {
	return NULL;
    }
    memset(draw, 0, sizeof(BROGUE_DEFERRED_DRAW));

    draw->window = window;
    draw->ref_count = 1;
    draw->draw_func = draw_func;
    draw->free_func = free_func;
    draw->draw_param = draw_param;
    draw->x = x;
    draw->y = y;
    draw->width = width;
    draw->height = height;

    return draw;
}

/*  Increment the reference count  */
void BrogueDeferredDraw_ref(BROGUE_DEFERRED_DRAW *draw)
{
    draw->ref_count++;
}

/*  Unreference the deferred drawing object, possibly freeing it  */
void BrogueDeferredDraw_unref(BROGUE_DEFERRED_DRAW *draw)
{
    if (!--draw->ref_count)
    {
	if (draw->free_func != NULL)
	{
	    draw->free_func(draw->draw_param);
	}

	free(draw);
    }
}

/*  Perform the deferred draw operation  */
int BrogueDeferredDraw_draw(
    BROGUE_DISPLAY *display, BROGUE_DEFERRED_DRAW *draw, SDL_Surface *surface)
{
    if (draw->drawn)
    {
	return 0;
    }

    return draw->draw_func(display, draw->draw_param, surface);
}

/*  Open a graphic element for rendering from a local file  */
BROGUE_GRAPHIC *BrogueGraphic_open(const char *filename)
{
    BROGUE_GRAPHIC *graphic;

    graphic = malloc(sizeof(BROGUE_GRAPHIC));
    if (graphic == NULL)
    {
	return NULL;
    }
    memset(graphic, 0, sizeof(BROGUE_GRAPHIC));

    graphic->ref_count = 1;
    graphic->path = SdlConsole_getResourcePath(filename);
    if (graphic->path == NULL)
    {
	free(graphic);
	return NULL;
    }

    graphic->svg = rsvg_handle_new_from_file(graphic->path, NULL);
    if (graphic->svg == NULL)
    {
	free(graphic->path);
	free(graphic);

	return NULL;
    }

    return graphic;
}

/*  Increase the reference count  */
void BrogueGraphic_ref(BROGUE_GRAPHIC *graphic)
{
    graphic->ref_count++;
}

/*  Decrease the graphic reference count, possibly freeing it  */
void BrogueGraphic_unref(BROGUE_GRAPHIC *graphic)
{
    if (--graphic->ref_count == 0)
    {
	free(graphic->path);
	g_object_unref(graphic->svg);
	free(graphic);
    }
}

/*  Close an opened graphic element  */
void BrogueGraphic_close(BROGUE_GRAPHIC *graphic)
{
    BrogueGraphic_unref(graphic);
}
