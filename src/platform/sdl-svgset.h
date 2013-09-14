/*
 *  sdl-svgset.h
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

#ifndef SDL_SVGSET_H
#define SDL_SVGSET_H

#include <SDL/SDL.h>

typedef struct SDL_SVGSET SDL_SVGSET;

SDL_SVGSET *SdlSvgset_alloc(char *path, int width, int height);
void SdlSvgset_free(SDL_SVGSET *svgset);

SDL_Surface *SdlSvgset_getGlyph(SDL_SVGSET *svgset, unsigned glyph, int frame);
SDL_Surface *SdlSvgset_render(SDL_SVGSET *svgset, 
			      unsigned glyph, int frame, SDL_Color color);

#endif
