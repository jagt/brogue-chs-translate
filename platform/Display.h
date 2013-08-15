/*
 *  Display.h
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

#ifndef DISPLAY_H
#define DISPLAY_H

/*  Justification modes for BrogueDrawContext_enableJustify  */
#define BROGUE_JUSTIFY_LEFT 0
#define BROGUE_JUSTIFY_CENTER 1
#define BROGUE_JUSTIFY_RIGHT 2

/*  Some opaque structures used as parameters to display methods  */
typedef struct BROGUE_DISPLAY BROGUE_DISPLAY;
typedef struct BROGUE_WINDOW BROGUE_WINDOW;
typedef struct BROGUE_EFFECT BROGUE_EFFECT;
typedef struct BROGUE_DRAW_CONTEXT BROGUE_DRAW_CONTEXT;
typedef struct BROGUE_DRAW_COLOR BROGUE_DRAW_COLOR;
typedef struct BROGUE_TEXT_SIZE BROGUE_TEXT_SIZE;
typedef struct BROGUE_GRAPHIC BROGUE_GRAPHIC;

/*  Colors, where the expected range for each component is 0.0 - 1.0  */
struct BROGUE_DRAW_COLOR
{
    float red;
    float green;
    float blue;
    float alpha;
};

/*  Text size as measured by the text rendering subsystem  */
struct BROGUE_TEXT_SIZE
{
    int width;
    int height;

    int final_column;
};

/*  Methods for the display itself  */
BROGUE_DISPLAY *BrogueDisplay_open(int width, int height);
void BrogueDisplay_close(BROGUE_DISPLAY *display);
BROGUE_WINDOW *BrogueDisplay_getRootWindow(BROGUE_DISPLAY *display);

/*  Methods for windows of the display  */
BROGUE_WINDOW *BrogueWindow_open(
    BROGUE_WINDOW *parent, int x, int y, int width, int height);
void BrogueWindow_close(BROGUE_WINDOW *window);
int BrogueWindow_reposition(
    BROGUE_WINDOW *window, int x, int y, int width, int height);
void BrogueWindow_setColor(BROGUE_WINDOW *window, BROGUE_DRAW_COLOR color);
void BrogueWindow_setVisible(BROGUE_WINDOW *window, int visible);
void BrogueWindow_clear(BROGUE_WINDOW *window);

/*  Methods for draw contexts associated with particular windows and 
    maintaining drawing state  */
BROGUE_DRAW_CONTEXT *BrogueDrawContext_open(BROGUE_WINDOW *window);
void BrogueDrawContext_close(BROGUE_DRAW_CONTEXT *context);
void BrogueDrawContext_push(BROGUE_DRAW_CONTEXT *context);
void BrogueDrawContext_pop(BROGUE_DRAW_CONTEXT *context);
void BrogueDrawContext_getScreenPosition(
    BROGUE_DRAW_CONTEXT *context, int *x, int *y);
void BrogueDrawContext_enableProportionalFont(
    BROGUE_DRAW_CONTEXT *context, int enable);
void BrogueDrawContext_enableTiles(
    BROGUE_DRAW_CONTEXT *context, int enable);
void BrogueDrawContext_enableWrap(
    BROGUE_DRAW_CONTEXT *context, int left_column, int right_column);
void BrogueDrawContext_enableJustify(
    BROGUE_DRAW_CONTEXT *context, int left_column, int right_column, 
    int justify_mode);
void BrogueDrawContext_disableWrap(
    BROGUE_DRAW_CONTEXT *context);
void BrogueDrawContext_setEffect(
    BROGUE_DRAW_CONTEXT *context, BROGUE_EFFECT *effect);
void BrogueDrawContext_setForeground(
    BROGUE_DRAW_CONTEXT *context, BROGUE_DRAW_COLOR color);
void BrogueDrawContext_setBackground(
    BROGUE_DRAW_CONTEXT *context, BROGUE_DRAW_COLOR color);
void BrogueDrawContext_blendForeground(
    BROGUE_DRAW_CONTEXT *context, 
    BROGUE_DRAW_COLOR upper_left, BROGUE_DRAW_COLOR upper_right,
    BROGUE_DRAW_COLOR lower_left, BROGUE_DRAW_COLOR lower_right);
void BrogueDrawContext_blendBackground(
    BROGUE_DRAW_CONTEXT *context, 
    BROGUE_DRAW_COLOR upper_left, BROGUE_DRAW_COLOR upper_right,
    BROGUE_DRAW_COLOR lower_left, BROGUE_DRAW_COLOR lower_right);
void BrogueDrawContext_setColorMultiplier(
    BROGUE_DRAW_CONTEXT *context, BROGUE_DRAW_COLOR color, int is_foreground);
void BrogueDrawContext_setColorAddition(
    BROGUE_DRAW_CONTEXT *context, BROGUE_DRAW_COLOR color, int is_foreground);
void BrogueDrawContext_setColorAverageTarget(
    BROGUE_DRAW_CONTEXT *context, 
    float weight, BROGUE_DRAW_COLOR color, int is_foreground);
int BrogueDrawContext_setTabStops(
    BROGUE_DRAW_CONTEXT *context, int stop_count, int *stops);
void BrogueDrawContext_drawChar(
    BROGUE_DRAW_CONTEXT *context, int x, int y, wchar_t c);
int BrogueDrawContext_drawString(
    BROGUE_DRAW_CONTEXT *context, int x, int y, const wchar_t *str);
int BrogueDrawContext_drawAsciiString(
    BROGUE_DRAW_CONTEXT *context, int x, int y, const char *str);
int BrogueDrawContext_drawGraphic(
    BROGUE_DRAW_CONTEXT *context, 
    int x, int y, int width, int height, BROGUE_GRAPHIC *graphic);
BROGUE_TEXT_SIZE BrogueDrawContext_measureString(
    BROGUE_DRAW_CONTEXT *context, int x, const wchar_t *str);
BROGUE_TEXT_SIZE BrogueDrawContext_measureAsciiString(
    BROGUE_DRAW_CONTEXT *context, int x, const char *str);

/*  Methods for display "effects" used to render in a non-standard way  */
BROGUE_EFFECT *BrogueEffect_open(
    BROGUE_DRAW_CONTEXT *context, const wchar_t *effect_name);
void BrogueEffect_close(BROGUE_EFFECT *effect);
int BrogueEffect_setParameters(BROGUE_EFFECT *effect, void *param);

/*  Methods for external graphics which can be drawn.  
    (i.e. loaded from SVG)  */
BROGUE_GRAPHIC *BrogueGraphic_open(const char *filename);
void BrogueGraphic_close(BROGUE_GRAPHIC *graphic);

#endif
