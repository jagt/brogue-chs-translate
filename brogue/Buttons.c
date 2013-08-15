/* -*- tab-width: 4 -*- */

/*
 *  Buttons.c
 *  Brogue
 *
 *  Created by Brian Walker on 11/18/11.
 *  Copyright 2012. All rights reserved.
 *
 *  This file is part of Brogue.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Rogue.h"
#include "IncludeGlobals.h"
#include "DisplayEffect.h"
#include <math.h>
#include <time.h>

// Draws the button to the screen, or to a display buffer if one is given.
// Button back color fades from -50% intensity at the edges to the back color in the middle.
// Text is white, but can use color escapes.
//		Hovering highlight augments fore and back colors with buttonHoverColor by 20%.
//		Pressed darkens the middle color (or turns it the hover color if the button is black).
void drawButton(
    BROGUE_DRAW_CONTEXT *context, BROGUE_EFFECT *effect,
    brogueButton *button, enum buttonDrawStates highlight) {

	short opacity;
	color bColor, fColorBase, bColorBase, bColorEdge, bColorMid;
	int width;
	int i;
	BUTTON_EFFECT_PARAM param;
	
	if (!(button->flags & B_DRAW)) {
		return;
	}
	
	width = button->width;
	if (width == 0)
	{
		width = strLenWithoutEscapes(button->text);
	}
	bColorBase = button->buttonColor;
	fColorBase = ((button->flags & B_ENABLED) ? white : gray);
	
	if (highlight == BUTTON_HOVER && (button->flags & B_HOVER_ENABLED)) {
		applyColorAverage(&fColorBase, &buttonHoverColor, 50);
		bColorBase = buttonHoverColor;
	}
	
	bColorEdge	= bColorBase;
	bColorMid	= bColorBase;
	applyColorAverage(&bColorEdge, &black, 50);
	
	if (highlight == BUTTON_PRESSED) {
		applyColorAverage(&bColorMid, &black, 75);
		if (COLOR_DIFF(bColorMid, bColorBase) < 50) {
			bColorMid	= bColorBase;
			applyColorAverage(&bColorMid, &buttonHoverColor, 50);
		}
	}
	bColor = bColorMid;
	
	opacity = button->opacity;
	if (highlight == BUTTON_HOVER || highlight == BUTTON_PRESSED) {
		opacity = 100 - ((100 - opacity) * opacity / 100); // Apply the opacity twice.
	}

	memset(&param, 0, sizeof(BUTTON_EFFECT_PARAM));
	param.width = width;

	if (button->flags & B_GRADIENT)
	{
		param.is_shaded = 1;
		param.is_centered = 1;
	}
	if (highlight != BUTTON_NORMAL)
	{
		param.is_shaded = 1;
	}
	if (button->flags & B_FORCE_CENTERED)
	{
		param.is_centered = 1;
	}

	param.color = colorForDisplay(bColorEdge);
	param.highlight_color = colorForDisplay(bColor);

	for (i = 0; button->text[i]; i++)
	{
		if (button->text[i] == '*')
		{
			param.symbol_count++;
		}
	}
	if (param.symbol_count > 0)
	{
		param.symbols = malloc(sizeof(wchar_t) * param.symbol_count);
		param.symbol_flags = malloc(sizeof(int) * param.symbol_count);

		for (i = 0; i < param.symbol_count; i++)
		{
			param.symbols[i] = button->symbol[i];
			param.symbol_flags[i] = button->symbol_flags[i];
		}
	}

	BrogueDrawContext_push(context);

	BrogueDrawContext_setEffect(context, effect);
	BrogueEffect_setParameters(effect, &param);
	BrogueDrawContext_drawAsciiString(context, 
									  button->x, button->y, button->text);

	BrogueDrawContext_pop(context);

	if (param.symbol_count > 0)
	{
		free(param.symbols);
		free(param.symbol_flags);
	}
}

void initializeButton(brogueButton *button) {
	
	memset((void *) button, 0, sizeof( brogueButton ));
	button->text[0] = '\0';
	button->flags |= (B_ENABLED | B_GRADIENT | B_HOVER_ENABLED | B_DRAW | B_KEYPRESS_HIGHLIGHT);
	button->buttonColor = interfaceButtonColor;
	button->opacity = 100;
	button->width = 0;
}

void drawButtonsInState(BROGUE_DRAW_CONTEXT *context, BROGUE_EFFECT *effect,
						buttonState *state) {
	short i;
	
	// Draw the buttons to the dbuf:
	for (i=0; i < state->buttonCount; i++) {
		if (state->buttons[i].flags & B_DRAW) {
		    drawButton(context, effect, &(state->buttons[i]), BUTTON_NORMAL);
		}
	}
}

void initializeButtonState(buttonState *state,
						   brogueButton *buttons,
						   short buttonCount,
						   BROGUE_DRAW_CONTEXT *context, 
						   BROGUE_EFFECT *effect,
						   short winX,
						   short winY,
						   short winWidth,
						   short winHeight) {
	short i;
	
	// Initialize variables for the state struct:
	state->buttonChosen = state->buttonFocused = state->buttonDepressed = -1;
	state->buttonCount	= buttonCount;
	state->winX			= winX;
	state->winY			= winY;
	state->winWidth		= winWidth;
	state->winHeight	= winHeight;
	for (i=0; i < state->buttonCount; i++) {
		state->buttons[i] = buttons[i];
	}
	
	drawButtonsInState(context, effect, state);
}

// Processes one round of user input, and bakes the necessary graphical changes into state->dbuf.
// Does NOT display the buttons or revert the display afterward.
// Assumes that the display has already been updated (via overlayDisplayBuffer(state->dbuf, NULL))
// and that input has been solicited (via nextBrogueEvent(event, ___, ___, ___)).
// Also relies on the buttonState having been initialized with initializeButtonState() or otherwise.
// Returns the index of a button if one is chosen.
// Otherwise, returns -1. That can be if the user canceled (in which case *canceled is true),
// or, more commonly, if the user's input in this particular split-second round was not decisive.
short processButtonInput(
	BROGUE_DRAW_CONTEXT *context, BROGUE_EFFECT *effect,
	buttonState *state, boolean *canceled, rogueEvent *event) {

	int i, k, x, y;
	boolean buttonUsed = false;
	
	// Mouse event:
	if (event->eventType == MOUSE_DOWN
		|| event->eventType == MOUSE_UP
		|| event->eventType == MOUSE_ENTERED_CELL) {
		
		BrogueDrawContext_getScreenPosition(context, &x, &y);
		x = event->param1 - x;
		y = event->param2 - y;
		
		// Revert the button with old focus, if any.
		if (state->buttonFocused >= 0) {
			drawButton(context, effect, &(state->buttons[state->buttonFocused]), BUTTON_NORMAL);
			state->buttonFocused = -1;
		}
		
		// Find the button with new focus, if any.
		for (i=0; i < state->buttonCount; i++) {
			if ((state->buttons[i].flags & B_DRAW)
				&& (state->buttons[i].flags & B_ENABLED)
				&& (state->buttons[i].y == y || ((state->buttons[i].flags & B_WIDE_CLICK_AREA) && abs(state->buttons[i].y - y) <= 1))
				&& x >= state->buttons[i].x
				&& x < state->buttons[i].x + strLenWithoutEscapes(state->buttons[i].text)) {
				
				state->buttonFocused = i;
				if (event->eventType == MOUSE_DOWN) {
					state->buttonDepressed = i; // Keeps track of which button is down at the moment. Cleared on mouseup.
				}
				break;
			}
		}
		if (i == state->buttonCount) { // No focus this round.
			state->buttonFocused = -1;
		}
		
		if (state->buttonDepressed >= 0) {
			if (state->buttonDepressed == state->buttonFocused) {
				drawButton(context, effect, &(state->buttons[state->buttonDepressed]), BUTTON_PRESSED);
			}
		} else if (state->buttonFocused >= 0) {
			// If no button is depressed, then update the appearance of the button with the new focus, if any.
			drawButton(context, effect, &(state->buttons[state->buttonFocused]), BUTTON_HOVER);
		}
		
		// Mouseup:
		if (event->eventType == MOUSE_UP) {
			if (state->buttonDepressed == state->buttonFocused && state->buttonFocused >= 0) {
				// If a button is depressed, and the mouseup happened on that button, it has been chosen and we're done.
				buttonUsed = true;
			} else {
				// Otherwise, no button is depressed. If one was previously depressed, redraw it.
				if (state->buttonDepressed >= 0) {
					drawButton(context, effect, &(state->buttons[state->buttonDepressed]), BUTTON_NORMAL);
				} else if (!(x >= state->winX && x < state->winX + state->winWidth
							 && y >= state->winY && y < state->winY + state->winHeight)) {
					// Clicking outside of a button means canceling.
					if (canceled) {
						*canceled = true;
					}
				}
				
				if (state->buttonFocused >= 0) {
					// Buttons don't hover-highlight when one is depressed, so we have to fix that when the mouse is up.
					drawButton(context, effect, &(state->buttons[state->buttonFocused]), BUTTON_HOVER);
				}
				state->buttonDepressed = -1;
			}
		}
	}
	
	// Keystroke:
	if (event->eventType == KEYSTROKE) {
		
		// Cycle through all of the hotkeys of all of the buttons.
		for (i=0; i < state->buttonCount; i++) {
			for (k = 0; k < 10 && state->buttons[i].hotkey[k]; k++) {
				if (event->param1 == state->buttons[i].hotkey[k]) {
					// This button was chosen.
					
					if (state->buttons[i].flags & B_DRAW) {
						// Restore the depressed and focused buttons.
						if (state->buttonDepressed >= 0) {
							drawButton(context, effect, &(state->buttons[state->buttonDepressed]), BUTTON_NORMAL);
						}
						if (state->buttonFocused >= 0) {
							drawButton(context, effect, &(state->buttons[state->buttonFocused]), BUTTON_NORMAL);
						}
						
						// If the button likes to flash when keypressed:
						if (state->buttons[i].flags & B_KEYPRESS_HIGHLIGHT) {
							// Depress the chosen button.
							drawButton(context, effect, &(state->buttons[i]), BUTTON_PRESSED);
							
							// Wait for a little; then we're done.
							pauseBrogue(50);
						}
					}
					
					state->buttonDepressed = i;
					buttonUsed = true;
					break;
				}
			}
		}
		
		if (!buttonUsed
			&& (event->param1 == ESCAPE_KEY || event->param1 == ACKNOWLEDGE_KEY)) {
			// If the player pressed escape, we're done.
			if (canceled) {
				*canceled = true;
			}
		}
	}
	
	if (buttonUsed) {
		state->buttonChosen = state->buttonDepressed;
		return state->buttonChosen;
	} else {
		return -1;
	}
}

// Displays a bunch of buttons and collects user input.
// Returns the index number of the chosen button, or -1 if the user cancels.
// A window region is described by winX, winY, winWidth and winHeight.
// Clicking outside of that region will constitute canceling.
short buttonInputLoop(brogueButton *buttons,
					  short buttonCount,
					  BROGUE_DRAW_CONTEXT *context,
					  BROGUE_EFFECT *effect, 
					  short winX,
					  short winY,
					  short winWidth,
					  short winHeight,
					  rogueEvent *returnEvent) {
	short button; 
	boolean canceled;
	rogueEvent theEvent;
	buttonState state = {0};
	
	assureCosmeticRNG;
	
	canceled = false;
	
	initializeButtonState(&state, buttons, buttonCount, context, effect, winX, winY, winWidth, winHeight);
	
	do {
		// Get input.
		nextBrogueEvent(&theEvent, true, false, false);
		
		// Process the input.
		button = processButtonInput(context, effect, 
									&state, &canceled, &theEvent);

	} while (button == -1 && !canceled);
	
	if (returnEvent) {
		*returnEvent = theEvent;
	}
	
	restoreRNG;
	
	return button;
}
