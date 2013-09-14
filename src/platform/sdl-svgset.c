/*
 *  sdl-svgset.c
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
#ifdef __APPLE__
#include <cairo.h>
#else
#include <cairo/cairo.h>
#endif
#include <librsvg/rsvg.h>
#include <glib.h>

#if defined(__MMX__)
#include <mmintrin.h>
#endif

#include "sdl-svgset.h"

#define MAX_FRAME_COUNT 30

typedef struct SDL_SVGANIM SDL_SVGANIM;

/*  A set of frames for a glyph.  */
struct SDL_SVGANIM
{
    int frame_count;
    SDL_Surface **frame;
};

/*  A set of SVG animation sequences at a particular size  */
struct SDL_SVGSET
{
    int width;
    int height;

    int count;
    SDL_SVGANIM **anim;
};

/*  Allocate a new animation set  */
SDL_SVGANIM *SdlSvgset_allocAnim(int frame_count)
{
    SDL_SVGANIM *anim;

    anim = malloc(sizeof(SDL_SVGANIM));
    if (anim == NULL)
    {
	return NULL;
    }
    memset(anim, 0, sizeof(SDL_SVGANIM));

    anim->frame_count = frame_count;
    anim->frame = malloc(sizeof(SDL_Surface *) * anim->frame_count);
    if (anim->frame == NULL)
    {
	free(anim);
	return NULL;
    }
    memset(anim->frame, 0, sizeof(SDL_Surface *) * anim->frame_count);

    return anim;
}

/*  Free an animation set  */
void SdlSvgset_freeAnim(SDL_SVGANIM *anim)
{
    int i;

    for (i = 0; i < anim->frame_count; i++)
    {
	if (anim->frame[i] != NULL)
	{
	    SDL_FreeSurface(anim->frame[i]);
	    anim->frame[i] = NULL;
	}
    }

    free(anim->frame);
    free(anim);
}

/*  Extend an animation set to a specified number of frames if it is 
    currently shorter than that count.  */
int SdlSvgset_extendAnim(SDL_SVGANIM *anim, int new_count)
{
    if (new_count > anim->frame_count)
    {
	SDL_Surface **new_frame;

	new_frame = malloc(sizeof(SDL_Surface *) * new_count);
	if (new_frame == NULL)
	{
	    return 0;
	}
	memset(new_frame, 0, sizeof(SDL_Surface *) * new_count);
	memcpy(new_frame, anim->frame, 
	       sizeof(SDL_Surface *) * anim->frame_count);
	free(anim->frame);

	anim->frame_count = new_count;
	anim->frame = new_frame;
    }

    return 1;
}

/*  
    Given a filename, return a glyph code, assuming the base part of
    the filename is a hex code.  

    Returns -1 if the filename doesn't match the expected pattern.
*/
int SdlSvgset_getGlyphNumberFromFilename(
    const gchar *filename, int *glyph, int *frame)
{
    gchar *ext;

    *glyph = -1;
    *frame = 0;

    *glyph = g_ascii_strtoll(filename, &ext, 16);
    if (ext[0] == '_')
    {
	ext++;
	*frame = g_ascii_strtoll(ext, &ext, 16);
    }

    if (strcmp(ext, ".svg") != 0)
    {
	return 0;
    }

    return 1;
}

/*  
    Cairo renders with premultiplied alpha.  
    SDL expects un-premultiplied alpha.
    We'll undo the alpha multiplication here, because we will use SDL
    to composite.  We've lost information in the RGB channels, but it
    shouldn't matter because the SDL composite will multiply the channel
    value down again when compositing.
*/
void Sdl_undoPremultipliedAlpha(SDL_Surface *surface)
{
    int x, y;

    for (y = 0; y < surface->h; y++)
    {
	char *pixel = (char *)surface->pixels + surface->pitch * y;
	SDL_PixelFormat *format = surface->format;
	int bpp = format->BytesPerPixel;

	for (x = 0; x < surface->w; x++)
	{
	    unsigned value = *(unsigned *)pixel;
	    int a = (value & format->Amask) >> format->Ashift;
	    int r = (value & format->Rmask) >> format->Rshift;
	    int g = (value & format->Gmask) >> format->Gshift;
	    int b = (value & format->Bmask) >> format->Bshift;
	    
	    if (a > 0)
	    {
		r = r * 255 / a;
		g = g * 255 / a;
		b = b * 255 / a;
	    }

	    value = (a << format->Ashift) | 
		(r << format->Rshift) |
		(g << format->Gshift) |
		(b << format->Bshift);

	    *(unsigned *)pixel = value;
	    pixel += bpp;
	}
    }
}

/*  Rasterize the SVG content to the dimensions expected for the svgset  */
SDL_Surface *SdlSvgset_convertSvgToGlyph(SDL_SVGSET *svgset, 
					 RsvgHandle *svg)
{
    SDL_Surface *surface;
    cairo_surface_t *cairo_surface;
    cairo_t *cairo;
    RsvgDimensionData dimensions;

    surface = SDL_CreateRGBSurface(0, svgset->width, svgset->height, 32,
				   0x00FF0000, 0x0000FF00, 0x000000FF, 
				   0xFF000000);
    if (surface == NULL)
    {
	return NULL;
    }

    cairo_surface = cairo_image_surface_create_for_data(
	surface->pixels, CAIRO_FORMAT_ARGB32, 
	surface->w, surface->h, surface->pitch);
    if (cairo_surface == NULL)
    {
	SDL_FreeSurface(surface);
	return NULL;
    }

    cairo = cairo_create(cairo_surface);
    if (cairo == NULL)
    {
	cairo_surface_destroy(cairo_surface);
	SDL_FreeSurface(surface);
	return NULL;
    }
    
    rsvg_handle_get_dimensions(svg, &dimensions);
    cairo_scale(cairo, 
		(double)svgset->width / (double)dimensions.width,
		(double)svgset->height / (double)dimensions.height);
    rsvg_handle_render_cairo(svg, cairo);

    cairo_destroy(cairo);
    cairo_surface_destroy(cairo_surface);

    Sdl_undoPremultipliedAlpha(surface);

    return surface;
}

/*  Load a particular SVG file into the set  */
void SdlSvgset_loadGlyph(SDL_SVGSET *svgset, char *path, const gchar *filename)
{
    int glyph, frame;
    char *svgpath;
    RsvgHandle *svg;
    SDL_SVGANIM *anim;

    if (!SdlSvgset_getGlyphNumberFromFilename(filename, &glyph, &frame))
    {
	return;
    }

    if (glyph < 0 || glyph >= svgset->count)
    {
	return;
    }

    if (frame < 0 || frame >= MAX_FRAME_COUNT)
    {
	return;
    } 

    if (svgset->anim[glyph] == NULL)
    {
	anim = SdlSvgset_allocAnim(frame + 1);
	if (anim == NULL)
	{
	    return;
	}
	svgset->anim[glyph] = anim;
    }
    else
    {
	anim = svgset->anim[glyph];

	if (!SdlSvgset_extendAnim(anim, frame + 1))
	{
	    return;
	}
    }
    
    assert(frame < anim->frame_count);

    svgpath = malloc(strlen(path) + strlen(filename) + 2);
    if (svgpath == NULL)
    {
	return;
    }

    sprintf(svgpath, "%s/%s", path, filename);
		
    svg = rsvg_handle_new_from_file(svgpath, NULL);
    free(svgpath);

    if (svg == NULL)
    {
	return;
    }

    svgset->anim[glyph]->frame[frame] = 
	SdlSvgset_convertSvgToGlyph(svgset, svg);
    g_object_unref(svg);		
}

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
/*  Load an SVG set with an explicit target width and height  */
SDL_SVGSET *SdlSvgset_alloc(char *path, int width, int height)
{
    SDL_SVGSET *svgset;
    GDir *svgdir;
    const gchar *filename;

    g_type_init();

    svgset = malloc(sizeof(SDL_SVGSET));
    if (svgset == NULL)
    {
	return NULL;
    }
    memset(svgset, 0, sizeof(SDL_SVGSET));

    svgset->width = width;
    svgset->height = height;
    
    svgdir = g_dir_open(path, 0, NULL);
    if (svgdir == NULL)
    {
	free(svgset);
	return NULL;
    }
    
    /*  Find the maximum glyph code present  */
    filename = g_dir_read_name(svgdir);
    while (filename != NULL)
    {
	int glyph, frame;

	if (SdlSvgset_getGlyphNumberFromFilename(filename, &glyph, &frame))
	{
	    if (glyph >= 0 && glyph >= svgset->count)
	    {
		svgset->count = glyph + 1;
	    }
	}
	
	filename = g_dir_read_name(svgdir);
    }

    svgset->anim = malloc(sizeof(SDL_SVGANIM *) * svgset->count);
    if (svgset->anim == NULL)
    {
	free(svgset);
	return NULL;
    }
    memset(svgset->anim, 0, sizeof(SDL_Surface *) * svgset->count);

    g_dir_rewind(svgdir);
    filename = g_dir_read_name(svgdir);
    while (filename != NULL)
    {
	SdlSvgset_loadGlyph(svgset, path, filename);

	filename = g_dir_read_name(svgdir);
    }
    g_dir_close(svgdir);

    return svgset;
}
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

/*  Free the resources associated with an svgset  */
void SdlSvgset_free(SDL_SVGSET *svgset)
{
    int i;

    for (i = 0; i < svgset->count; i++)
    {
	if (svgset->anim[i] != NULL)
	{
	    SdlSvgset_freeAnim(svgset->anim[i]);
	    svgset->anim[i] = NULL;
	}
    }

    free(svgset->anim);
    free(svgset);
}

/*  Multiply the surface by a particular color value  */
int SdlSvgset_applyColor(SDL_Surface *surface, SDL_Color color)
{
    int x, y;
    unsigned char r = color.r;
    unsigned char g = color.g;
    unsigned char b = color.b;
#if defined(__MMX__)
    int mmx = SDL_HasMMX();
#endif

    assert(surface->format->Rmask == 0x00FF0000);
    assert(surface->format->Gmask == 0x0000FF00);
    assert(surface->format->Bmask == 0x000000FF);

    if (SDL_LockSurface(surface))
    {
	return 0;
    }

    for (y = 0; y < surface->h; y++)
    {
	unsigned char *pixel = 
	    (unsigned char *)surface->pixels + surface->pitch * y;
	x = surface->w;

#if defined(__MMX__)
	/*  
	    We'd really like to use the MMX path for the bulk of the pixels
	    on x86 and x86_64, as it is much faster than the more
	    straightforward blend below.  
	*/
	if (mmx)
	{
	    __m64 mmx_zero = _m_from_int(0);
	    long long ll_color = 
		((long long)0x0100 << 48)
		| ((long long)(r * 256 / 255) << 32)
		| ((long long)(g * 256 / 255) << 16)
		| (long long)(b * 256 / 255);
	    __m64 mmx_color = *(__m64 *)&ll_color;

	    while (x >= 2)
	    {
		__m64 src_pix = *(__m64 *)pixel;

		/*  Separate the left pixel and right pixel  */
		__m64 left_pix = _mm_unpacklo_pi8(src_pix, mmx_zero);
		__m64 right_pix = _mm_unpackhi_pi8(src_pix, mmx_zero);

		/*  Multiply by the color value  */
		left_pix = _mm_mullo_pi16(left_pix, mmx_color);
		right_pix = _mm_mullo_pi16(right_pix, mmx_color);

		/*  Divide by 256  */
		left_pix = _mm_srli_pi16(left_pix, 8);
		right_pix = _mm_srli_pi16(right_pix, 8);

		/*  Recombine the pixels  */
		__m64 result_pix = _mm_packs_pu16(left_pix, right_pix);
		*(__m64 *)pixel = result_pix;

		pixel += 8;
		x -= 2;
	    }
	}
#endif

	while (x--)
	{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	    pixel[2] = pixel[2] * r / 255;
	    pixel[1] = pixel[1] * g / 255;
	    pixel[0] = pixel[0] * b / 255;
#else
	    pixel[1] = pixel[1] * r / 255;
	    pixel[2] = pixel[2] * g / 255;
	    pixel[3] = pixel[3] * b / 255;
#endif

	    pixel += 4;
	}
    }

#if defined(__MMX__)
    /*  Empty the MMX state to avoid interfering with future FPU results  */
    if (mmx)
    {
	_mm_empty();
    }
#endif

    SDL_UnlockSurface(surface);
    return 1;
}

SDL_Surface *SdlSvgset_getGlyph(SDL_SVGSET *svgset, 
				unsigned glyph, int frame)
{
    SDL_SVGANIM *anim;
    int frame_index;

    if (glyph >= svgset->count)
    {
	return NULL;
    }

    anim = svgset->anim[glyph];
    if (anim == NULL || anim->frame_count == 0)
    {
	return NULL;
    }

    frame_index = (frame % MAX_FRAME_COUNT) 
	* anim->frame_count / MAX_FRAME_COUNT;
    return anim->frame[frame_index];
}

/*  
    Render one of the glyphs in the svgset at the target dimensions
    and with a particular color.
*/
SDL_Surface *SdlSvgset_render(SDL_SVGSET *svgset, 
			      unsigned glyph, int frame, SDL_Color color)
{
    SDL_Surface *source, *surface;

    source = SdlSvgset_getGlyph(svgset, glyph, frame);
    if (source == NULL)
    {
	return NULL;
    }

    surface = SDL_CreateRGBSurface(
	0, source->w, source->h, 32,
	0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (surface == NULL)
    {
	return NULL;
    }
    memcpy(surface->pixels, source->pixels, source->h * source->pitch);

    if (!SdlSvgset_applyColor(surface, color))
    {
	SDL_FreeSurface(surface);
	return NULL;
    } 

    return surface;
}
