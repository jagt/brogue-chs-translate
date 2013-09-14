/* -*- tab-width: 4 -*- */

/*
 *  Buttons.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/14/12.
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

#include <math.h>
#include <time.h>
#include <limits.h>

#include "Rogue.h"
#include "IncludeGlobals.h"
#include "DisplayEffect.h"

#define MENU_FLAME_PRECISION_FACTOR		10
#define MENU_FLAME_RISE_SPEED			50
#define MENU_FLAME_SPREAD_SPEED			20
#define MENU_FLAME_COLOR_DRIFT_SPEED	500
#define MENU_FLAME_FADE_SPEED			20
#define MENU_FLAME_UPDATE_DELAY			50
#define MENU_FLAME_ROW_PADDING			2
#define MENU_TITLE_OFFSET_X				(-4)
#define MENU_TITLE_OFFSET_Y				(-1)

#define MENU_FLAME_COLOR_SOURCE_COUNT	1136

#define MENU_FLAME_DENOMINATOR			(100 + MENU_FLAME_RISE_SPEED + MENU_FLAME_SPREAD_SPEED)

void drawMenuFlames(
	BROGUE_DRAW_CONTEXT *context, 
	signed short flames[COLS][(ROWS + MENU_FLAME_ROW_PADDING)][3]) {

	short i, j, versionStringLength;
    char dchar;
    
    versionStringLength = strLenWithoutEscapes(BROGUE_VERSION_STRING);
	
	for (j=0; j<ROWS; j++) {
		for (i=0; i<COLS; i++) {
            if (j == ROWS - 1 && i >= COLS - versionStringLength) {
                dchar = BROGUE_VERSION_STRING[i - (COLS - versionStringLength)];
            } else {
                dchar = ' ';
            }
            
			BrogueDrawContext_setForeground(
				context, colorForDisplay(darkGray));

			if (i < COLS - 1)
			{
				color ul, ur, bl, br;
				memset(&ul, 0, sizeof(color));
				memset(&ur, 0, sizeof(color));
				memset(&bl, 0, sizeof(color));
				memset(&br, 0, sizeof(color));

				ul.red = flames[i][j][0] / MENU_FLAME_PRECISION_FACTOR;
				ul.green = flames[i][j][1] / MENU_FLAME_PRECISION_FACTOR;
				ul.blue = flames[i][j][2] / MENU_FLAME_PRECISION_FACTOR;

				ur.red = flames[i + 1][j][0] / MENU_FLAME_PRECISION_FACTOR;
				ur.green = flames[i + 1][j][1] / MENU_FLAME_PRECISION_FACTOR;
				ur.blue = flames[i + 1][j][2] / MENU_FLAME_PRECISION_FACTOR;

				bl.red = flames[i][j + 1][0] / MENU_FLAME_PRECISION_FACTOR;
				bl.green = flames[i][j + 1][1] / MENU_FLAME_PRECISION_FACTOR;
				bl.blue = flames[i][j + 1][2] / MENU_FLAME_PRECISION_FACTOR;

				br.red = flames[i + 1][j + 1][0] / MENU_FLAME_PRECISION_FACTOR;
				br.green = flames[i + 1][j + 1][1] / MENU_FLAME_PRECISION_FACTOR;
				br.blue = flames[i + 1][j + 1][2] / MENU_FLAME_PRECISION_FACTOR;

				BrogueDrawContext_blendBackground(
					context, colorForDisplay(ul), colorForDisplay(ur),
					colorForDisplay(bl), colorForDisplay(br));
			}
			else
			{
				color upper, lower;

				upper.red = flames[i][j][0] / MENU_FLAME_PRECISION_FACTOR;
				upper.green = flames[i][j][1] / MENU_FLAME_PRECISION_FACTOR;
				upper.blue = flames[i][j][2] / MENU_FLAME_PRECISION_FACTOR;

				lower.red = flames[i][j + 1][0] / MENU_FLAME_PRECISION_FACTOR;
				lower.green = flames[i][j + 1][1] / MENU_FLAME_PRECISION_FACTOR;
				lower.blue = flames[i][j + 1][2] / MENU_FLAME_PRECISION_FACTOR;

				BrogueDrawContext_blendBackground(
					context, colorForDisplay(upper), colorForDisplay(upper),
					colorForDisplay(lower), colorForDisplay(lower));
			}

			BrogueDrawContext_drawChar(context, i, j, dchar);
		}
	}
}

void updateMenuFlames(color *colors[COLS][(ROWS + MENU_FLAME_ROW_PADDING)],
					  signed short colorSources[MENU_FLAME_COLOR_SOURCE_COUNT][4],
					  signed short flames[COLS][(ROWS + MENU_FLAME_ROW_PADDING)][3]) {
	
	short i, j, k, l, x, y;
	signed short tempFlames[COLS][3];
	short colorSourceNumber, rand;
	
	colorSourceNumber = 0;
	for (j=0; j<(ROWS + MENU_FLAME_ROW_PADDING); j++) {
		// Make a temp copy of the current row.
		for (i=0; i<COLS; i++) {
			for (k=0; k<3; k++) {
				tempFlames[i][k] = flames[i][j][k];
			}
		}
		
		for (i=0; i<COLS; i++) {
			// Each cell is the weighted average of the three color values below and itself.
			// Weight of itself: 100
			// Weight of left and right neighbors: MENU_FLAME_SPREAD_SPEED / 2 each
			// Weight of below cell: MENU_FLAME_RISE_SPEED
			// Divisor: 100 + MENU_FLAME_SPREAD_SPEED + MENU_FLAME_RISE_SPEED
			
			// Itself:
			for (k=0; k<3; k++) {
				flames[i][j][k] = 100 * flames[i][j][k] / MENU_FLAME_DENOMINATOR;
			}
			
			// Left and right neighbors:
			for (l = -1; l <= 1; l += 2) {
				x = i + l;
				if (x == -1) {
					x = COLS - 1;
				} else if (x == COLS) {
					x = 0;
				}
				for (k=0; k<3; k++) {
					flames[i][j][k] += MENU_FLAME_SPREAD_SPEED * tempFlames[x][k] / 2 / MENU_FLAME_DENOMINATOR;
				}
			}
			
			// Below:
			y = j + 1;
			if (y < (ROWS + MENU_FLAME_ROW_PADDING)) {
				for (k=0; k<3; k++) {
					flames[i][j][k] += MENU_FLAME_RISE_SPEED * flames[i][y][k] / MENU_FLAME_DENOMINATOR;
				}
			}
			
			// Fade a little:
			for (k=0; k<3; k++) {
				flames[i][j][k] = (1000 - MENU_FLAME_FADE_SPEED) * flames[i][j][k] / 1000;
			}
			
			if (colors[i][j]) {
				// If it's a color source tile:
				
				// First, cause the color to drift a little.
				for (k=0; k<4; k++) {
					colorSources[colorSourceNumber][k] += rand_range(-MENU_FLAME_COLOR_DRIFT_SPEED, MENU_FLAME_COLOR_DRIFT_SPEED);
					colorSources[colorSourceNumber][k] = clamp(colorSources[colorSourceNumber][k], 0, 1000);
				}
				
				// Then, add the color to this tile's flames.
				rand = colors[i][j]->rand * colorSources[colorSourceNumber][0] / 1000;
				flames[i][j][0] += (colors[i][j]->red	+ (colors[i][j]->redRand	* colorSources[colorSourceNumber][1] / 1000) + rand) * MENU_FLAME_PRECISION_FACTOR;
				flames[i][j][1] += (colors[i][j]->green	+ (colors[i][j]->greenRand	* colorSources[colorSourceNumber][2] / 1000) + rand) * MENU_FLAME_PRECISION_FACTOR;
				flames[i][j][2] += (colors[i][j]->blue	+ (colors[i][j]->blueRand	* colorSources[colorSourceNumber][3] / 1000) + rand) * MENU_FLAME_PRECISION_FACTOR;
				
				colorSourceNumber++;
			}
		}
	}
}

// Takes a grid of values, each of which is 0 or 100, and fills in some middle values in the interstices.
void antiAlias(unsigned char mask[COLS][ROWS]) {
	short i, j, x, y, dir, nbCount;
	const short intensity[5] = {0, 0, 35, 50, 60};
	
	for (i=0; i<COLS; i++) {
		for (j=0; j<ROWS; j++) {
			if (mask[i][j] < 100) {
				nbCount = 0;
				for (dir=0; dir<4; dir++) {
					x = i + nbDirs[dir][0];
					y = j + nbDirs[dir][1];
					if (coordinatesAreInWindow(x, y) && mask[x][y] == 100) {
						nbCount++;
					}
				}
				mask[i][j] = intensity[nbCount];
			}
		}
	}
}

#define MENU_TITLE_WIDTH	74
#define MENU_TITLE_HEIGHT	19

void initializeMenuFlames(boolean includeTitle,
						  color *colors[COLS][(ROWS + MENU_FLAME_ROW_PADDING)],
						  signed short colorSources[MENU_FLAME_COLOR_SOURCE_COUNT][4],
						  signed short flames[COLS][(ROWS + MENU_FLAME_ROW_PADDING)][3],
						  unsigned char mask[COLS][ROWS]) {
	short i, j, k, colorSourceCount;
	const char title[MENU_TITLE_HEIGHT][MENU_TITLE_WIDTH+1] = {
		"########   ########       ######         #######   ####     ###  #########",
		" ##   ###   ##   ###    ##     ###     ##      ##   ##       #    ##     #",
		" ##    ##   ##    ##   ##       ###   ##        #   ##       #    ##     #",
		" ##    ##   ##    ##   #    #    ##   #         #   ##       #    ##      ",
		" ##    ##   ##    ##  ##   ##     ## ##             ##       #    ##    # ",
		" ##   ##    ##   ##   ##   ###    ## ##             ##       #    ##    # ",
		" ######     ## ###    ##   ####   ## ##             ##       #    ####### ",
		" ##    ##   ##  ##    ##   ####   ## ##             ##       #    ##    # ",
		" ##     ##  ##   ##   ##    ###   ## ##      #####  ##       #    ##    # ",
		" ##     ##  ##   ##   ###    ##   ## ###       ##   ##       #    ##      ",
		" ##     ##  ##    ##   ##    #    #   ##       ##   ##       #    ##      ",
		" ##     ##  ##    ##   ###       ##   ###      ##   ###      #    ##     #",
		" ##    ##   ##     ##   ###     ##     ###    ###    ###    #     ##     #",
		"########   ####    ###    ######         #####        ######     #########",
		"                            ##                                            ",
		"                        ##########                                        ",
		"                            ##                                            ",
		"                            ##                                            ",
		"                           ####                                           ",
	};
	
	for (i=0; i<COLS; i++) {
		for (j=0; j<ROWS; j++) {
			mask[i][j] = 0;
		}
	}
	
	for (i=0; i<COLS; i++) {
		for (j=0; j<(ROWS + MENU_FLAME_ROW_PADDING); j++) {
			colors[i][j] = NULL;
			for (k=0; k<3; k++) {
				flames[i][j][k] = 0;
			}
		}
	}
	
	// Seed source color random components.
	for (i=0; i<MENU_FLAME_COLOR_SOURCE_COUNT; i++) {
		for (k=0; k<4; k++) {
			colorSources[i][k] = rand_range(0, 1000);
		}
	}
	
	// Put some flame source along the bottom row.
	colorSourceCount = 0;
	for (i=0; i<COLS; i++) {
		colors[i][(ROWS + MENU_FLAME_ROW_PADDING)-1] = &flameSourceColor;
		colorSourceCount++;
	}
	
	if (includeTitle) {
		// Wreathe the title in flames, and mask it in black.
		for (i=0; i<MENU_TITLE_WIDTH; i++) {
			for (j=0; j<MENU_TITLE_HEIGHT; j++) {
				if (title[j][i] != ' ') {
					colors[(COLS - MENU_TITLE_WIDTH)/2 + i + MENU_TITLE_OFFSET_X][(ROWS - MENU_TITLE_HEIGHT)/2 + j + MENU_TITLE_OFFSET_Y] = &flameTitleColor;
					colorSourceCount++;
					mask[(COLS - MENU_TITLE_WIDTH)/2 + i + MENU_TITLE_OFFSET_X][(ROWS - MENU_TITLE_HEIGHT)/2 + j + MENU_TITLE_OFFSET_Y] = 100;
				}
			}
		}
		
		// Anti-alias the mask.
		antiAlias(mask);
	}
	
#ifdef BROGUE_ASSERTS
	assert(colorSourceCount <= MENU_FLAME_COLOR_SOURCE_COUNT);
#endif
	
	// Simulate the background flames for a while
	for (i=0; i<100; i++) {
		updateMenuFlames(colors, colorSources, flames);
	}
	
}

void titleMenu() {
	signed short flames[COLS][(ROWS + MENU_FLAME_ROW_PADDING)][3]; // red, green and blue
	signed short colorSources[MENU_FLAME_COLOR_SOURCE_COUNT][4]; // red, green, blue, and rand, one for each color source (no more than MENU_FLAME_COLOR_SOURCE_COUNT).
	color *colors[COLS][(ROWS + MENU_FLAME_ROW_PADDING)];
	unsigned char mask[COLS][ROWS];
	boolean controlKeyWasDown = false;
	
	short i, b, x, y, button;
	buttonState state;
	brogueButton buttons[6];
	char whiteColorEscape[10] = "";
	char goldColorEscape[10] = "";
	char newGameText[100] = "", customNewGameText[100] = "";
	rogueEvent theEvent;
	enum NGCommands buttonCommands[6] = {NG_NEW_GAME, NG_OPEN_GAME, NG_VIEW_RECORDING, NG_HIGH_SCORES, NG_QUIT};
	BROGUE_WINDOW *root, *window, *title_window, *button_window;
	BROGUE_DRAW_CONTEXT *context, *title_context, *button_context;
	BROGUE_EFFECT *button_effect;
	BROGUE_GRAPHIC *title_graphic;
	
	// Initialize the RNG so the flames aren't always the same.
	
	seedRandomGenerator(0);
	
	// Empty nextGamePath and nextGameSeed so that the buttons don't try to load an old game path or seed.
	rogue.nextGamePath[0] = '\0';
	rogue.nextGameSeed = 0;
	
	// Initialize the title menu buttons.
	encodeMessageColor(whiteColorEscape, 0, &white);
	encodeMessageColor(goldColorEscape, 0, &itemMessageColor);
	sprintf(newGameText, " %s(N)%s 开始新游戏", goldColorEscape, whiteColorEscape);
	sprintf(customNewGameText, " %s(N)%s 生成新游戏", goldColorEscape, whiteColorEscape);
	b = 0;
	button = -1;
	
	initializeButton(&(buttons[b]));
	strcpy(buttons[b].text, newGameText);
	buttons[b].hotkey[0] = 'n';
	buttons[b].hotkey[1] = 'N';
	b++;
	
	initializeButton(&(buttons[b]));
	sprintf(buttons[b].text, " %s(O)%s 读取存档   ", goldColorEscape, whiteColorEscape);
	buttons[b].hotkey[0] = 'o';
	buttons[b].hotkey[1] = 'O';
	b++;
	
	initializeButton(&(buttons[b]));
	sprintf(buttons[b].text, " %s(V)%s 观看录像   ", goldColorEscape, whiteColorEscape);
	buttons[b].hotkey[0] = 'v';
	buttons[b].hotkey[1] = 'V';
	b++;
	
	initializeButton(&(buttons[b]));
	sprintf(buttons[b].text, " %s(H)%s 最高分      ", goldColorEscape, whiteColorEscape);
	buttons[b].hotkey[0] = 'h';
	buttons[b].hotkey[1] = 'H';
	b++;
	
	initializeButton(&(buttons[b]));
	sprintf(buttons[b].text, " %s(Q)%s 退出游戏   ", goldColorEscape, whiteColorEscape);
	buttons[b].hotkey[0] = 'q';
	buttons[b].hotkey[1] = 'Q';
	b++;
	
	x = COLS - 1 - 20 - 2;
	y = ROWS - 3;
	for (i = b-1; i >= 0; i--) {
		y -= 2;

		buttons[i].x = 1;
		buttons[i].y = 1 + 2 * i;
		buttons[i].buttonColor = interfaceButtonColor;
		buttons[i].flags |= B_WIDE_CLICK_AREA;
	}

	title_graphic = BrogueGraphic_open("svg/title.svg");

	root = ioGetRoot();
	window = BrogueWindow_open(root, 0, 0, COLS, ROWS);
	context = BrogueDrawContext_open(window);

	title_window = BrogueWindow_open(window, 0, 0, COLS, ROWS);
	title_context = BrogueDrawContext_open(title_window);
	if (title_graphic != NULL)
	{
		BrogueDrawContext_drawGraphic(title_context, 0, 0, COLS, ROWS, 
									  title_graphic);
	}

	button_window = BrogueWindow_open(window, x, y, 22, b * 2 + 1);
	BrogueWindow_setColor(button_window, windowColor);
	button_context = BrogueDrawContext_open(button_window);
	button_effect = BrogueEffect_open(button_context, BUTTON_EFFECT_NAME);
	
	blackOutScreen();
	initializeButtonState(&state, buttons, b, button_context, button_effect, 
						  x, y, 20, b*2-1);
	drawButtonsInState(button_context, button_effect, &state);

	initializeMenuFlames(true, colors, colorSources, flames, mask);
    rogue.creaturesWillFlashThisTurn = false; // total unconscionable hack
	
	do {
		if (!controlKeyWasDown && controlKeyIsDown()) {
			strcpy(state.buttons[0].text, customNewGameText);
			drawButtonsInState(button_context, button_effect, &state);
			buttonCommands[0] = NG_NEW_GAME_WITH_SEED;
			controlKeyWasDown = true;
		} else if (controlKeyWasDown && !controlKeyIsDown()) {
			strcpy(state.buttons[0].text, newGameText);
			drawButtonsInState(button_context, button_effect, &state);
			buttonCommands[0] = NG_NEW_GAME;
			controlKeyWasDown = false;
		}
		
		// Update the display.
		updateMenuFlames(colors, colorSources, flames);
		drawMenuFlames(context, flames);
		
		// Pause briefly.
		if (pauseBrogue(MENU_FLAME_UPDATE_DELAY)) {
			// There was input during the pause! Get the input.
			nextBrogueEvent(&theEvent, true, false, true);
			
			// Process the input.
			button = processButtonInput(button_context, button_effect, 
										&state, NULL, &theEvent);
		}
	} while (button == -1 && rogue.nextGame == NG_NOTHING);
	drawMenuFlames(context, flames);
	if (button != -1) {
		rogue.nextGame = buttonCommands[button];
	}

	BrogueWindow_close(window);
	BrogueGraphic_close(title_graphic);
}

void dialogAlert(char *message) {
	brogueButton OKButton;
	initializeButton(&OKButton);
	strcpy(OKButton.text, "     OK     ");
	OKButton.hotkey[0] = RETURN_KEY;
	OKButton.hotkey[1] = ENTER_KEY;
	OKButton.hotkey[2] = ACKNOWLEDGE_KEY;
	printTextBox(message, COLS/3, &white, &interfaceBoxColor, &OKButton, 1);
}

boolean stringsExactlyMatch(const char *string1, const char *string2) {
	short i;
	for (i=0; string1[i] && string2[i]; i++) {
		if (string1[i] != string2[i]) {
			return false;
		}
	}
	return string1[i] == string2[i];
}

#define FILES_ON_PAGE_MAX				(min(26, ROWS - 7)) // Two rows (top and bottom) for flames, two rows for border, one for prompt, one for heading.
#define MAX_FILENAME_DISPLAY_LENGTH		53
boolean dialogChooseFile(char *path, const char *suffix, const char *prompt) {
	short i, j, count, x, y, width, height, suffixLength, pathLength, maxPathLength, currentPageStart;
	brogueButton buttons[FILES_ON_PAGE_MAX + 2];
	fileEntry *files;
	boolean retval = false, again;
	color *dialogColor = &interfaceBoxColor;
	char *membuf;
	
	suffixLength = strlen(suffix);
	files = listFiles(&count, &membuf);
//	copyDisplayBuffer(rbuf, displayBuffer);
	maxPathLength = strLenWithoutEscapes(prompt);
	
	// First, we want to filter the list by stripping out any filenames that do not end with suffix.
	// i is the entry we're testing, and j is the entry that we move it to if it qualifies.
	for (i=0, j=0; i<count; i++) {
		pathLength = strlen(files[i].path);
		//printf("\nString 1: %s", &(files[i].path[(max(0, pathLength - suffixLength))]));
		if (stringsExactlyMatch(&(files[i].path[(max(0, pathLength - suffixLength))]), suffix)) {
			
			// This file counts!
			if (i > j) {
				files[j] = files[i];
				//printf("\nMatching file: %s\twith date: %s", files[j].path, files[j].date);
			}
			j++;
			
			// Keep track of the longest length.
			if (min(pathLength, MAX_FILENAME_DISPLAY_LENGTH) + 10 > maxPathLength) {
				maxPathLength = min(pathLength, MAX_FILENAME_DISPLAY_LENGTH) + 10;
			}
		}
	}
	count = j;
	
	currentPageStart = 0;
	
	do { // Repeat to permit scrolling.
		again = false;
		
		for (i=0; i<min(count - currentPageStart, FILES_ON_PAGE_MAX); i++) {
			initializeButton(&(buttons[i]));
			buttons[i].flags &= ~(B_WIDE_CLICK_AREA | B_GRADIENT);
			buttons[i].buttonColor = *dialogColor;
			sprintf(buttons[i].text, "%c)\t", 'a' + i);
			strncat(buttons[i].text, files[currentPageStart+i].path, MAX_FILENAME_DISPLAY_LENGTH);
			
			// Clip off the file suffix from the button text.
			buttons[i].text[strlen(buttons[i].text) - suffixLength] = '\0'; // Snip!
			buttons[i].hotkey[0] = 'a' + i;
			buttons[i].hotkey[1] = 'A' + i;
			
			// Clip the filename length if necessary.
			if (strlen(buttons[i].text) > MAX_FILENAME_DISPLAY_LENGTH) {
				strcpy(&(buttons[i].text[MAX_FILENAME_DISPLAY_LENGTH - 3]), "...");
			}
			
			//printf("\nFound file: %s, with date: %s", files[currentPageStart+i].path, files[currentPageStart+i].date);
		}
		
		x = (COLS - maxPathLength) / 2;
		width = maxPathLength;
		height = min(count - currentPageStart, FILES_ON_PAGE_MAX) + 5;
		y = max(4, (ROWS - height) / 2);
		
		for (i=0; i<min(count - currentPageStart, FILES_ON_PAGE_MAX); i++) {
			strcat(buttons[i].text, "\t");
			strcat(buttons[i].text, files[currentPageStart+i].date);
			buttons[i].x = 1;
			buttons[i].y = 3 + i;
			buttons[i].width = width;
		}
		
		if (count > FILES_ON_PAGE_MAX) {

			// Create up and down arrows.
			initializeButton(&(buttons[i]));
			strcpy(buttons[i].text, "     *     ");
			buttons[i].symbol[0] = UP_ARROW_CHAR;
			if (currentPageStart <= 0) {
				buttons[i].flags &= ~(B_ENABLED | B_DRAW);
			} else {
				buttons[i].hotkey[0] = UP_ARROW;
				buttons[i].hotkey[1] = NUMPAD_8;
				buttons[i].hotkey[2] = PAGE_UP_KEY;
			}
			buttons[i].x = (width - 11)/2;
			buttons[i].y = 2;
			
			i++;
			initializeButton(&(buttons[i]));
			strcpy(buttons[i].text, "     *     ");
			buttons[i].symbol[0] = DOWN_ARROW_CHAR;
			if (currentPageStart + FILES_ON_PAGE_MAX >= count) {
				buttons[i].flags &= ~(B_ENABLED | B_DRAW);
			} else {
				buttons[i].hotkey[0] = DOWN_ARROW;
				buttons[i].hotkey[1] = NUMPAD_2;
				buttons[i].hotkey[2] = PAGE_DOWN_KEY;
			}
			buttons[i].x = (width - 11)/2;
			buttons[i].y = 2 + i;
		}
		
		if (count) {
			BROGUE_WINDOW *root, *window;
			BROGUE_DRAW_CONTEXT *context;
			BROGUE_EFFECT *effect;
			int tabStops[2] = { 3, width - 8 }; 

			root = ioGetRoot();
			window = BrogueWindow_open(
				root, (COLS - width) / 2, (ROWS - height) / 2, 
				width, height);
			context = BrogueDrawContext_open(window);
			effect = BrogueEffect_open(context, BUTTON_EFFECT_NAME);

			BrogueWindow_setColor(window, windowColor);
			BrogueDrawContext_enableProportionalFont(context, 1);
			BrogueDrawContext_setTabStops(context, 2, tabStops);

			BrogueDrawContext_drawAsciiString(context, 1, 1, prompt);

//			for (j=0; j<min(count - currentPageStart, FILES_ON_PAGE_MAX); j++) {
//				printf("\nSanity check BEFORE: %s, with date: %s", files[currentPageStart+j].path, files[currentPageStart+j].date);
//				printf("\n   (button name)Sanity check BEFORE: %s", buttons[j].text);
//			}
			
			i = buttonInputLoop(buttons,
								min(count - currentPageStart, FILES_ON_PAGE_MAX) + (count > FILES_ON_PAGE_MAX ? 2 : 0),
								context, 
								effect, 
								x,
								y,
								width,
								height,
								NULL); 

			BrogueWindow_close(window);
			
//			for (j=0; j<min(count - currentPageStart, FILES_ON_PAGE_MAX); j++) {
//				printf("\nSanity check AFTER: %s, with date: %s", files[currentPageStart+j].path, files[currentPageStart+j].date);
//				printf("\n   (button name)Sanity check AFTER: %s", buttons[j].text);
//			}

			if (i < min(count - currentPageStart, FILES_ON_PAGE_MAX)) {
				if (i >= 0) {
					retval = true;
					strcpy(path, files[currentPageStart+i].path);
				} else { // i is -1
					retval = false;
				}
			} else if (i == min(count - currentPageStart, FILES_ON_PAGE_MAX)) { // Up arrow
				again = true;
				currentPageStart -= FILES_ON_PAGE_MAX;
			} else if (i == min(count - currentPageStart, FILES_ON_PAGE_MAX) + 1) { // Down arrow
				again = true;
				currentPageStart += FILES_ON_PAGE_MAX;
			}
		}
		
	} while (again);
	
	free(files);
	free(membuf);
	
	if (count == 0) {
		dialogAlert("找不到合适的文件。");
		return false;
	} else {
		return retval;
	}
}

void scum(unsigned long startingSeed, short numberOfSeedsToScan, short scanThroughDepth) {
    unsigned long theSeed;
    char path[BROGUE_FILENAME_MAX];
    item *theItem, spareItem;
    char buf[200];
    FILE *logFile;
    
    logFile = fopen("Brogue seed scumming log file.txt", "w");
    rogue.nextGame = NG_NOTHING;
    
    getAvailableFilePath(path, LAST_GAME_NAME, GAME_SUFFIX);
    strcat(path, GAME_SUFFIX);
    
    fprintf(logFile, "Brogue seed catalog, seeds %li to %li, through depth %i.\n\n\
To play one of these seeds, press control-N from the title screen \
and enter the seed number. Knowing which items will appear on \
the first %i depths will, of course, make the game significantly easier.",
            startingSeed, startingSeed + numberOfSeedsToScan - 1, scanThroughDepth, scanThroughDepth);
    
    for (theSeed = startingSeed; theSeed < startingSeed + numberOfSeedsToScan; theSeed++) {
        fprintf(logFile, "\n\nSeed %li:", theSeed);
        printf("\nScanned seed %li.", theSeed);
        rogue.nextGamePath[0] = '\0';
        randomNumbersGenerated = 0;
        
        rogue.playbackMode = false;
        rogue.playbackFastForward = false;
        rogue.playbackBetweenTurns = false;
        
        strcpy(currentFilePath, path);
        initializeRogue(theSeed);
        for (rogue.depthLevel = 1; rogue.depthLevel <= scanThroughDepth; rogue.depthLevel++) {
            startLevel(rogue.depthLevel == 1 ? 1 : rogue.depthLevel - 1, 1); // descending into level n
            fprintf(logFile, "\n    Depth %i:", rogue.depthLevel);
            for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
                spareItem = *theItem;
                identify(&spareItem);
                itemName(&spareItem, buf, true, true, NULL);
                upperCase(buf);
                fprintf(logFile, "\n        %s", buf);
                if (pmap[theItem->xLoc][theItem->yLoc].machineNumber > 0) {
                    fprintf(logFile, " (vault %i)", pmap[theItem->xLoc][theItem->yLoc].machineNumber);
                }
            }
        }
        freeEverything();
        remove(currentFilePath); // Don't add a spurious LastGame file to the brogue folder.
    }
    fclose(logFile);
}

// This is the basic program loop.
// When the program launches, or when a game ends, you end up here.
// If the player has already said what he wants to do next
// (by storing it in rogue.nextGame -- possibilities listed in enum NGCommands),
// we'll do it. The path (rogue.nextGamePath) is essentially a parameter for this command, and
// tells NG_VIEW_RECORDING and NG_OPEN_GAME which file to open. If there is a command but no
// accompanying path, and it's a command that should take a path, then pop up a dialog to have
// the player specify a path. If there is no command (i.e. if rogue.nextGame contains NG_NOTHING),
// then we'll display the title screen so the player can choose.
void mainBrogueJunction() {
	rogueEvent theEvent;
	char path[BROGUE_FILENAME_MAX], buf[100], seedDefault[100];
	char maxSeed[64];
	short i;
	boolean seedTooBig;

	if (ioInitialize() != 0) {
		printf("Failure to initialize display\n");
		exit(1);
	}
	
	initializeLaunchArguments(&rogue.nextGame, rogue.nextGamePath, &rogue.nextGameSeed);
	
	do {
        rogue.gameHasEnded = false;
        rogue.playbackFastForward = false;
        rogue.playbackMode = false;
		switch (rogue.nextGame) {
			case NG_NOTHING:
				// Run the main menu to get a decision out of the player.
				titleMenu();
				break;
			case NG_NEW_GAME:
			case NG_NEW_GAME_WITH_SEED:
				rogue.nextGamePath[0] = '\0';
				randomNumbersGenerated = 0;
				
				rogue.playbackMode = false;
				rogue.playbackFastForward = false;
				rogue.playbackBetweenTurns = false;
                
                getAvailableFilePath(path, LAST_GAME_NAME, GAME_SUFFIX);
                strcat(path, GAME_SUFFIX);
				strcpy(currentFilePath, path);
				
				if (rogue.nextGame == NG_NEW_GAME_WITH_SEED) {
					if (rogue.nextGameSeed == 0) { // Prompt for seed; default is the previous game's seed.
						sprintf(maxSeed, "%lu", ULONG_MAX);
						if (previousGameSeed == 0) {
							seedDefault[0] = '\0';
						} else {
							sprintf(seedDefault, "%lu", previousGameSeed);
						}
						if (getInputTextString(buf, "输入指定的随机数种子来生成地下城：",
											   log10(ULONG_MAX) + 1,
											   seedDefault,
											   "",
											   TEXT_INPUT_NUMBERS,
											   true)
							&& buf[0] != '\0') {
							seedTooBig = false;
							if (strlen(buf) > strlen(maxSeed)) {
								seedTooBig = true;
							} else if (strlen(buf) == strlen(maxSeed)) {
								for (i=0; maxSeed[i]; i++) {
									if (maxSeed[i] > buf[i]) {
										break; // we're good
									} else if (maxSeed[i] < buf[i]) {
										seedTooBig = true;
										break;
									}
								}
							}
							if (seedTooBig) {
								rogue.nextGameSeed = ULONG_MAX;
							} else {
								sscanf(buf, "%lu", &rogue.nextGameSeed);
							}
						} else {
							rogue.nextGame = NG_NOTHING;
							break; // Don't start a new game after all.
						}
					}
				} else {
					rogue.nextGameSeed = 0; // Seed based on clock.
				}
				
				rogue.nextGame = NG_NOTHING;
				initializeRogue(rogue.nextGameSeed);
				startLevel(rogue.depthLevel, 1); // descending into level 1
				
				mainInputLoop();
				freeEverything();
				break;
			case NG_OPEN_GAME:
				rogue.nextGame = NG_NOTHING;
				path[0] = '\0';
				if (rogue.nextGamePath[0]) {
					strcpy(path, rogue.nextGamePath);
					rogue.nextGamePath[0] = '\0';
				} else {
					dialogChooseFile(path, GAME_SUFFIX, "选择中断存档进行读取：");
					//chooseFile(path, "Open saved game: ", "Saved game", GAME_SUFFIX);
				}
					
				if (openFile(path)) {
					loadSavedGame();
					mainInputLoop();
					freeEverything();
				} else {
					//dialogAlert("File not found.");
				}
				rogue.playbackMode = false;
				rogue.playbackOOS = false;
				
				break;
			case NG_VIEW_RECORDING:
				rogue.nextGame = NG_NOTHING;
				
				path[0] = '\0';
				if (rogue.nextGamePath[0]) {
					strcpy(path, rogue.nextGamePath);
					rogue.nextGamePath[0] = '\0';
				} else {
					dialogChooseFile(path, RECORDING_SUFFIX, "选择录像文件进行回放：");
					//chooseFile(path, "View recording: ", "Recording", RECORDING_SUFFIX);
				}
				
				if (openFile(path)) {
					randomNumbersGenerated = 0;
					rogue.playbackMode = true;
					initializeRogue(0); // Seed argument is ignored because we're in playback.
					if (!rogue.gameHasEnded) {
						startLevel(rogue.depthLevel, 1);
						pausePlayback();
						displayAnnotation(); // in case there's an annotation for turn 0
					}
					
					while(!rogue.gameHasEnded && rogue.playbackMode) {
						rogue.RNG = RNG_COSMETIC; // dancing terrain colors can't influence recordings
						rogue.playbackBetweenTurns = true;
						nextBrogueEvent(&theEvent, false, true, false);
						rogue.RNG = RNG_SUBSTANTIVE;
						
						executeEvent(&theEvent);
					}
					
					freeEverything();
				} else {
					// announce file not found
				}
				rogue.playbackMode = false;
				rogue.playbackOOS = false;
				
				break;
			case NG_HIGH_SCORES:
				rogue.nextGame = NG_NOTHING;
				printHighScores(false);
				break;
            case NG_SCUM:
                rogue.nextGame = NG_NOTHING;
                scum(1, 1000, 5);
                break;
			case NG_QUIT:
				// No need to do anything.
				break;
			default:
				break;
		}
	} while (rogue.nextGame != NG_QUIT);
}

