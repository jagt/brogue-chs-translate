/*
 *  sdl-keymap.h
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

#ifndef SDL_KEYMAP_H
#define SDL_KEYMAP_H

typedef struct SDL_KEYMAP SDL_KEYMAP;

SDL_KEYMAP *SdlKeymap_alloc(void);
void SdlKeymap_free(SDL_KEYMAP *keymap);

int SdlKeymap_addTranslation(SDL_KEYMAP *keymap,
			     const char *src, const char *dst);
void SdlKeymap_translate(SDL_KEYMAP *keymap, SDL_KeyboardEvent *event);

#endif
