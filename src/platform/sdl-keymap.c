/*
 *  sdl-keymap.c
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

#include <SDL/SDL.h>

#include "platform.h"
#include "sdl-keymap.h"

/*  We will associate strings with SDL key codes and unicode characters.  */
struct SDL_CONSOLE_KEY_DEFINITION
{
    char *name;
    SDLKey key;
    uchar sym;
};
typedef struct SDL_CONSOLE_KEY_DEFINITION SDL_CONSOLE_KEY_DEFINITION;

/*  When we map keys, we will compare the SDL key code, but subsitute 
    both a replacement key code and a unicode character.  */
struct SDL_KEY_MAPPING
{
    SDLKey src_key;
    SDLKey dst_key;
    uchar dst_sym;
};
typedef struct SDL_KEY_MAPPING SDL_KEY_MAPPING;

/*  Our keymap consists of a list of mappings.  */
struct SDL_KEYMAP
{
    SDL_KEY_MAPPING *mapping;
    unsigned mapping_count;
};

static SDL_CONSOLE_KEY_DEFINITION key_defs[] = 
{
    { "NONE", SDLK_UNKNOWN, 0 },
    { "ESCAPE", SDLK_ESCAPE, '\x1b' },
    { "BACKSPACE", SDLK_BACKSPACE, '\b' },
    { "TAB", SDLK_TAB, '\t' },
    { "ENTER", SDLK_RETURN, '\r' },
    { "SHIFT", SDLK_LSHIFT, 0 },
    { "SHIFT", SDLK_RSHIFT, 0 },
    { "CONTROL", SDLK_LCTRL, 0 },
    { "CONTROL", SDLK_RCTRL, 0 },
    { "ALT", SDLK_LALT, 0 },
    { "ALT", SDLK_RALT, 0 },
    { "PAUSE", SDLK_PAUSE, 0 },
    { "CAPSLOCK", SDLK_CAPSLOCK, 0 },
    { "PAGEUP", SDLK_PAGEUP, 0 },
    { "PAGEDOWN", SDLK_PAGEDOWN, 0 },
    { "END", SDLK_END, 0 },
    { "HOME", SDLK_HOME, 0 },
    { "UP", SDLK_UP, 0 },
    { "LEFT", SDLK_LEFT, 0 },
    { "RIGHT", SDLK_RIGHT, 0 },
    { "DOWN", SDLK_DOWN, 0 },
    { "PRINTSCREEN", SDLK_PRINT, 0 },
    { "INSERT", SDLK_INSERT, 0 },
    { "DELETE", SDLK_DELETE, '\b' },
    { "LWIN", SDLK_LSUPER, 0 },
    { "RWIN", SDLK_RSUPER, 0 },
    { "APPS", SDLK_MODE, 0 },
    { "KP0", SDLK_KP0, 0 },
    { "KP1", SDLK_KP1, 0 },
    { "KP2", SDLK_KP2, 0 },
    { "KP3", SDLK_KP3, 0 },
    { "KP4", SDLK_KP4, 0 },
    { "KP5", SDLK_KP5, 0 },
    { "KP6", SDLK_KP6, 0 },
    { "KP7", SDLK_KP7, 0 },
    { "KP8", SDLK_KP8, 0 },
    { "KP9", SDLK_KP9, 0 },
    { "KPADD", SDLK_KP_PLUS, 0 },
    { "KPSUB", SDLK_KP_MINUS, 0 },
    { "KPDIV", SDLK_KP_DIVIDE, 0 },
    { "KPMUL", SDLK_KP_MULTIPLY, 0 },
    { "KPDEC", SDLK_KP_PERIOD, 0 },
    { "KPENTER", SDLK_KP_ENTER, 0 },
    { "F1", SDLK_F1, 0 },
    { "F2", SDLK_F2, 0 },
    { "F3", SDLK_F3, 0 },
    { "F4", SDLK_F4, 0 },
    { "F5", SDLK_F5, 0 },
    { "F6", SDLK_F6, 0 },
    { "F7", SDLK_F7, 0 },
    { "F8", SDLK_F8, 0 },
    { "F9", SDLK_F9, 0 },
    { "F10", SDLK_F10, 0 },
    { "F11", SDLK_F11, 0 },
    { "F12", SDLK_F12, 0 },
    { "NUMLOCK", SDLK_NUMLOCK, 0 },
    { "SCROLLLOCK", SDLK_SCROLLOCK, 0 },
    { "SPACE", SDLK_SPACE, ' ' },
    { NULL, SDLK_UNKNOWN }
};

/*  Allocate a new keymap object.  */
SDL_KEYMAP *SdlKeymap_alloc(void)
{
    SDL_KEYMAP *keymap;

    keymap = malloc(sizeof(SDL_KEYMAP));
    if (keymap == NULL)
    {
	return NULL;
    }

    memset(keymap, 0, sizeof(SDL_KEYMAP));
    return keymap;
}

/*  Free a previously allocated keymap.  */
void SdlKeymap_free(SDL_KEYMAP *keymap)
{
    if (keymap->mapping)
    {
	free(keymap->mapping);
	keymap->mapping = NULL;
    }

    free(keymap);
}

/*  Find a key definition from a string.  For keys with a well-known 
    ASCII code, we'll generate the key definition from a one-character
    name string.  */
int SdlKeymap_findDefinition(SDL_CONSOLE_KEY_DEFINITION *def,
			     const char *name)
{
    int i = 0;

    while (key_defs[i].name != NULL)
    {
	if (!strcmp(name, key_defs[i].name))
	{
	    *def = key_defs[i];

	    return 0;
	}

	i++;
    }

    if (strlen(name) == 1)
    {
	uchar c = name[0];

	if (c > ' ' && c <= 'z')
	{
	    def->name = NULL;
	    def->key = c;
	    def->sym = c;

	    return 0;
	}
    }

    return EINVAL;
}

/*  Add a new translation entry to the keymap.  Returns an errno 
    error code or zero for success.  */
int SdlKeymap_addTranslation(SDL_KEYMAP *keymap,
			     const char *src, const char *dst)
{
    SDL_CONSOLE_KEY_DEFINITION src_key, dst_key;
    int err;
    SDL_KEY_MAPPING *mapping, *new_mapping;
 
    err = SdlKeymap_findDefinition(&src_key, src);
    if (err)
    {
	return err;
    }

    err = SdlKeymap_findDefinition(&dst_key, dst);
    if (err)
    {
	return err;
    }

    mapping = realloc(keymap->mapping, 
		      sizeof(SDL_KEY_MAPPING) * (keymap->mapping_count + 1));
    if (mapping == NULL)
    {
	return ENOMEM;
    }
    keymap->mapping = mapping;
    new_mapping = &mapping[keymap->mapping_count++];

    memset(new_mapping, 0, sizeof(SDL_KEY_MAPPING));
    
    new_mapping->src_key = src_key.key;
    new_mapping->dst_key = dst_key.key;
    new_mapping->dst_sym = dst_key.sym;

    return 0;
}

/*  Translate an SDL key event in-place.  */
void SdlKeymap_translate(SDL_KEYMAP *keymap, SDL_KeyboardEvent *event)
{
    int i;

    for (i = 0; i < keymap->mapping_count; i++)
    {
	SDL_KEY_MAPPING *mapping = &keymap->mapping[i];

	if (mapping->src_key == event->keysym.sym)
	{
	    event->keysym.sym = mapping->dst_key;
	    event->keysym.unicode = mapping->dst_sym;
	    
	    return;
	}
    }
}
