/*
 *  DisplayEffect.h
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

#ifndef DISPLAY_EFFECT_H
#define DISPLAY_EFFECT_H

typedef struct PROGRESS_BAR_EFFECT_PARAM PROGRESS_BAR_EFFECT_PARAM;
typedef struct BUTTON_EFFECT_PARAM BUTTON_EFFECT_PARAM;

/*  Parameter structure for progress bar drawing  */
#define PROGRESS_BAR_EFFECT_NAME L"ProgressBar"
struct PROGRESS_BAR_EFFECT_PARAM
{
    int size;
    double amount;
};

/*  Parameter structure for button effect drawing  */
#define BUTTON_EFFECT_NAME L"Button"
struct BUTTON_EFFECT_PARAM
{
    int width;
    int is_shaded;
    int is_centered;

    BROGUE_DRAW_COLOR color;
    BROGUE_DRAW_COLOR highlight_color;

    /*  Symbols to be substituted for '*' characters when drawing  */
    int symbol_count;
    wchar_t *symbols;
    int *symbol_flags;
};

#endif
