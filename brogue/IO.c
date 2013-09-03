/* -*- tab-width: 4 -*- */

/*
 *  IO.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/10/09.
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

#include <assert.h>
#include <math.h>
#include <time.h>

#include "Rogue.h"
#include "IncludeGlobals.h"
#include "Display.h"
#include "DisplayEffect.h"

/*  We'll maintain some windows and draw contexts for drawing to regions
	of the display  */
typedef struct IO_STATE IO_STATE;
struct IO_STATE
{
	BROGUE_DISPLAY *display;

	BROGUE_WINDOW *root;
	BROGUE_WINDOW *dungeon_window;
	BROGUE_WINDOW *message_window;
	BROGUE_WINDOW *flavor_window;
	BROGUE_WINDOW *button_window;
	BROGUE_WINDOW *sidebar_window;

	BROGUE_DRAW_CONTEXT *root_context;
	BROGUE_DRAW_CONTEXT *dungeon_context;
	BROGUE_DRAW_CONTEXT *message_context;
	BROGUE_DRAW_CONTEXT *flavor_context;
	BROGUE_DRAW_CONTEXT *button_context;
	BROGUE_DRAW_CONTEXT *sidebar_context;

	BROGUE_EFFECT *button_effect;
	BROGUE_EFFECT *progress_bar_effect;

	BROGUE_WINDOW *alert_window;
};

IO_STATE io_state;

/*  Open the windows and draw contexts used by the main game display  */
int ioInitialize(void)
{
	io_state.display = brogue_display;
	if (io_state.display == NULL)
	{
		return -1;
	}

	io_state.root = BrogueDisplay_getRootWindow(io_state.display);
	io_state.dungeon_window = BrogueWindow_open(
		io_state.root, STAT_BAR_WIDTH + 1, MESSAGE_LINES, DCOLS, DROWS);
	io_state.message_window = BrogueWindow_open(
		io_state.root, STAT_BAR_WIDTH + 1, 0, DCOLS, MESSAGE_LINES);
	io_state.flavor_window = BrogueWindow_open(
		io_state.root, STAT_BAR_WIDTH + 1, MESSAGE_LINES + DROWS, DCOLS, 1);
	io_state.button_window = BrogueWindow_open(
		io_state.root, STAT_BAR_WIDTH + 1, MESSAGE_LINES + DROWS + 1, DCOLS, 1);
	io_state.sidebar_window = BrogueWindow_open(
		io_state.root, 0, 0, STAT_BAR_WIDTH, ROWS);
	if (io_state.dungeon_window == NULL
		|| io_state.message_window == NULL
		|| io_state.flavor_window == NULL
		|| io_state.button_window == NULL
		|| io_state.sidebar_window == NULL)
	{
		BrogueDisplay_close(io_state.display);
		return -1;
	}

	io_state.root_context = BrogueDrawContext_open(io_state.root);
	io_state.dungeon_context = BrogueDrawContext_open(io_state.dungeon_window);
	io_state.message_context = BrogueDrawContext_open(io_state.message_window);
	io_state.flavor_context = BrogueDrawContext_open(io_state.flavor_window);
	io_state.button_context = BrogueDrawContext_open(io_state.button_window);
	io_state.sidebar_context = BrogueDrawContext_open(io_state.sidebar_window);
	if (io_state.dungeon_context == NULL
		|| io_state.message_context == NULL
		|| io_state.flavor_context == NULL
		|| io_state.button_context == NULL
		|| io_state.sidebar_context == NULL)
	{
		BrogueDisplay_close(io_state.display);
		return -1;
	}

	io_state.progress_bar_effect = 
		BrogueEffect_open(io_state.sidebar_context, PROGRESS_BAR_EFFECT_NAME);
	io_state.button_effect =
		BrogueEffect_open(io_state.button_context, BUTTON_EFFECT_NAME);
	if (io_state.progress_bar_effect == NULL
		|| io_state.button_effect == NULL)
	{
		BrogueDisplay_close(io_state.display);
		return -1;
	}

	BrogueDrawContext_enableProportionalFont(io_state.message_context, 1);
	BrogueDrawContext_enableProportionalFont(io_state.flavor_context, 1);
	BrogueDrawContext_enableTiles(io_state.dungeon_context, 1);

	return 0;
}

/*  Return the root window of the display  */
BROGUE_WINDOW *ioGetRoot(void)
{
	return BrogueDisplay_getRootWindow(io_state.display);
}

/*  Convert a color to the floating point format used by the display
	subsystem  */
BROGUE_DRAW_COLOR colorForDisplay(color c)
{
	BROGUE_DRAW_COLOR r;

	r.red = c.red / 100.0;
	r.green = c.green / 100.0;
	r.blue = c.blue / 100.0;
	r.alpha = 1.0;

	return r;
}

/*  Weight a color value while converting to the display color format  */
BROGUE_DRAW_COLOR weightedColorForDisplay(color c, int weight)
{
	BROGUE_DRAW_COLOR r;

	r.red = c.red / 100.0 * weight / 100.0;
	r.green = c.green / 100.0 * weight / 100.0;
	r.blue = c.blue / 100.0 * weight / 100.0;
	r.alpha = 1.0;

	return r;
}

// Populates path[][] with a list of coordinates starting at origin and traversing down the map. Returns the number of steps in the path.
short getPathOnMap(short path[1000][2], short **map, short originX, short originY) {
	short dir, x, y, steps;
	
	x = originX;
	y = originY;
	
	dir = 0;
	
	for (steps = 0; dir != -1;) {
		dir = nextStep(map, x, y, NULL, false);
		if (dir != -1) {
			x += nbDirs[dir][0];
			y += nbDirs[dir][1];
			path[steps][0] = x;
			path[steps][1] = y;
			steps++;
#ifdef BROGUE_ASSERTS
			assert(coordinatesAreInMap(x, y));
#endif
		}
	}
	return steps;
}

void reversePath(short path[1000][2], short steps) {
	short i, x, y;
	
	for (i=0; i<steps / 2; i++) {
		x = path[steps - i - 1][0];
		y = path[steps - i - 1][1];
		
		path[steps - i - 1][0] = path[i][0];
		path[steps - i - 1][1] = path[i][1];
		
		path[i][0] = x;
		path[i][1] = y;
	}
}

void hilitePath(short path[1000][2], short steps, boolean unhilite) {
	short i;
	if (unhilite) {
		for (i=0; i<steps; i++) {
#ifdef BROGUE_ASSERTS
            assert(coordinatesAreInMap(path[i][0], path[i][1]));
#endif
			pmap[path[i][0]][path[i][1]].flags &= ~IS_IN_PATH;
			refreshDungeonCell(path[i][0], path[i][1]);
		}
	} else {
		for (i=0; i<steps; i++) {
#ifdef BROGUE_ASSERTS
            assert(coordinatesAreInMap(path[i][0], path[i][1]));
#endif
			pmap[path[i][0]][path[i][1]].flags |= IS_IN_PATH;
			refreshDungeonCell(path[i][0], path[i][1]);
		}
	}
}

// More expensive than hilitePath(__, __, true), but you don't need access to the path itself.
void clearCursorPath() {
	short i, j;
	
	if (!rogue.playbackMode) { // There are no cursor paths during playback.
		for (i=1; i<DCOLS; i++) {
			for (j=1; j<DROWS; j++) {
				if (pmap[i][j].flags & IS_IN_PATH) {
					pmap[i][j].flags &= ~IS_IN_PATH;
					refreshDungeonCell(i, j);
				}
			}
		}
	}
}

void getClosestValidLocationOnMap(short loc[2], short **map, short x, short y) {
	short i, j, dist, closestDistance, lowestMapScore;
	
	closestDistance = 10000;
	lowestMapScore = 10000;
	for (i=1; i<DCOLS-1; i++) {
		for (j=1; j<DROWS-1; j++) {
			if (map[i][j] >= 0
				&& map[i][j] < 30000) {
				
				dist = (i - x)*(i - x) + (j - y)*(j - y);
				//hiliteCell(i, j, &purple, min(dist / 2, 100), false);
				if (dist < closestDistance
					|| dist == closestDistance && map[i][j] < lowestMapScore) {
					
					loc[0] = i;
					loc[1] = j;
					closestDistance = dist;
					lowestMapScore = map[i][j];
				}
			}
		}
	}
}

// Displays a menu of buttons for various commands.
// Buttons will be disabled if not permitted based on the playback state.
// Returns the keystroke to effect the button's command, or -1 if canceled.
short actionMenu(short x, short y, boolean playingBack) {
	short buttonCount;
	BROGUE_WINDOW *root, *window;
	BROGUE_DRAW_CONTEXT *context;
	BROGUE_EFFECT *effect;
	brogueButton buttons[ROWS] = {{{0}}};
	char yellowColorEscape[5] = "", whiteColorEscape[5] = "", grayColorEscape[5] = "";
	short i, longestName = 0, buttonChosen;
	
	root = BrogueDisplay_getRootWindow(io_state.display);

	encodeMessageColor(yellowColorEscape, 0, &itemMessageColor);
	encodeMessageColor(whiteColorEscape, 0, &white);
	encodeMessageColor(grayColorEscape, 0, &gray);
	
	for (i=0; i<ROWS; i++) {
		initializeButton(&(buttons[i]));
		buttons[i].flags &= ~B_GRADIENT;
		buttons[i].buttonColor = interfaceBoxColor;
		buttons[i].opacity = INTERFACE_OPACITY;
		buttons[i].x = 1;
		buttons[i].y = i + 1;
	}
	
	buttonCount = 0;
	
	if (playingBack) {
		sprintf(buttons[buttonCount].text,	"  %sk: %sFaster playback  ", yellowColorEscape, whiteColorEscape);
		buttons[buttonCount].hotkey[0] = UP_KEY;
		buttons[buttonCount].hotkey[1] = UP_ARROW;
		buttons[buttonCount].hotkey[2] = NUMPAD_8;
		buttonCount++;
		sprintf(buttons[buttonCount].text,	"  %sj: %sSlower playback  ", yellowColorEscape, whiteColorEscape);
		buttons[buttonCount].hotkey[0] = DOWN_KEY;
		buttons[buttonCount].hotkey[1] = DOWN_ARROW;
		buttons[buttonCount].hotkey[2] = NUMPAD_2;
		buttonCount++;
		sprintf(buttons[buttonCount].text, "%s---", grayColorEscape);
		buttons[buttonCount].flags &= ~B_ENABLED;
		buttons[buttonCount].flags |= B_FORCE_CENTERED;
		buttonCount++;
		
		sprintf(buttons[buttonCount].text,	"%s0-9: %sFast forward to turn  ", yellowColorEscape, whiteColorEscape);
		buttons[buttonCount].hotkey[0] = '0';
		buttonCount++;
		sprintf(buttons[buttonCount].text,	"  %s>:%s Next Level  ", yellowColorEscape, whiteColorEscape);
		buttons[buttonCount].hotkey[0] = DESCEND_KEY;
		buttonCount++;
		sprintf(buttons[buttonCount].text, "%s---", grayColorEscape);
		buttons[buttonCount].flags &= ~B_ENABLED;
		buttons[buttonCount].flags |= B_FORCE_CENTERED;
		buttonCount++;
	} else {
		sprintf(buttons[buttonCount].text, "  %sZ: %sSleep until better  ",		yellowColorEscape, whiteColorEscape);
		buttons[buttonCount].hotkey[0] = AUTO_REST_KEY;
		buttonCount++;
		sprintf(buttons[buttonCount].text, "%s---", grayColorEscape);
		buttons[buttonCount].flags &= ~B_ENABLED;
		buttons[buttonCount].flags |= B_FORCE_CENTERED;
		buttonCount++;
		
		sprintf(buttons[buttonCount].text, "  %sA: %sAutopilot  ",				yellowColorEscape, whiteColorEscape);
		buttons[buttonCount].hotkey[0] = AUTOPLAY_KEY;
		buttonCount++;
		sprintf(buttons[buttonCount].text, "%s---", grayColorEscape);
		buttons[buttonCount].flags &= ~B_ENABLED;
		buttons[buttonCount].flags |= B_FORCE_CENTERED;
		buttonCount++;
		
		sprintf(buttons[buttonCount].text, "  %sS: %sSuspend game and quit  ",	yellowColorEscape, whiteColorEscape);
		buttons[buttonCount].hotkey[0] = SAVE_GAME_KEY;
		buttonCount++;
		sprintf(buttons[buttonCount].text, "  %sO: %sOpen suspended game  ",		yellowColorEscape, whiteColorEscape);
		buttons[buttonCount].hotkey[0] = LOAD_SAVED_GAME_KEY;
		
	}
	sprintf(buttons[buttonCount].text, "  %sV: %sView saved recording  ",		yellowColorEscape, whiteColorEscape);
	buttons[buttonCount].hotkey[0] = VIEW_RECORDING_KEY;
	buttonCount++;
	sprintf(buttons[buttonCount].text, "%s---", grayColorEscape);
	buttons[buttonCount].flags &= ~B_ENABLED;
	buttons[buttonCount].flags |= B_FORCE_CENTERED;
	buttonCount++;
	
	sprintf(buttons[buttonCount].text, "  %sD: %sDiscovered items  ",	yellowColorEscape, whiteColorEscape);
	buttons[buttonCount].hotkey[0] = DISCOVERIES_KEY;
	buttonCount++;
	sprintf(buttons[buttonCount].text, "  %s\\: %s%s color effects  ",	yellowColorEscape, whiteColorEscape, rogue.trueColorMode ? "Enable" : "Disable");
	buttons[buttonCount].hotkey[0] = TRUE_COLORS_KEY;
	buttonCount++;
	sprintf(buttons[buttonCount].text, "  %s?: %sHelp  ",						yellowColorEscape, whiteColorEscape);
	buttons[buttonCount].hotkey[0] = HELP_KEY;
	buttonCount++;
	sprintf(buttons[buttonCount].text, "%s---", grayColorEscape);
	buttons[buttonCount].flags &= ~B_ENABLED;
	buttons[buttonCount].flags |= B_FORCE_CENTERED;
	buttonCount++;
	
	sprintf(buttons[buttonCount].text, "  %sQ: %sQuit %s  ",	yellowColorEscape, whiteColorEscape, (playingBack ? "to title screen" : "without saving"));
	buttons[buttonCount].hotkey[0] = QUIT_KEY;
	buttonCount++;
	
	strcpy(buttons[buttonCount].text, " ");
	buttons[buttonCount].flags &= ~B_ENABLED;
	buttonCount++;
	
	window = BrogueWindow_open(
		root, STAT_BAR_WIDTH + x - 1, ROWS - buttonCount - 2, 
		1, buttonCount + 1);
	BrogueWindow_setColor(window, windowColor);
	context = BrogueDrawContext_open(window);
	BrogueDrawContext_enableProportionalFont(context, 1);
	effect = BrogueEffect_open(context, BUTTON_EFFECT_NAME);

	longestName = 0;
	for (i = 0; i < buttonCount; i++)
	{
		BROGUE_TEXT_SIZE text_size;
		
		text_size = BrogueDrawContext_measureAsciiString(
			context, 0, buttons[i].text);
		if (text_size.width + 2 > longestName)
		{
			longestName = text_size.width + 2;
		}
	}

	for (i = 0; i < buttonCount; i++)
	{
		buttons[i].width = longestName - 2;
	}

	BrogueWindow_reposition(
		window, STAT_BAR_WIDTH + x - 1, ROWS - buttonCount - 2,
		longestName, buttonCount + 1);

	buttonChosen = buttonInputLoop(
		buttons, buttonCount, context, effect, 
		x - 1, y, longestName, buttonCount, NULL); 

	BrogueWindow_close(window);

	if (buttonChosen == -1) {
		return -1;
	} else {
		return buttons[buttonChosen].hotkey[0];
	}
}

#define MAX_MENU_BUTTON_COUNT 5

void initializeMenuButtons(buttonState *state, brogueButton buttons[5]) {
	short i, x, buttonCount;
	char goldTextEscape[MAX_MENU_BUTTON_COUNT] = "";
	char whiteTextEscape[MAX_MENU_BUTTON_COUNT] = "";
	
	encodeMessageColor(goldTextEscape, 0, &yellow);
	encodeMessageColor(whiteTextEscape, 0, &white);
	
	for (i=0; i<MAX_MENU_BUTTON_COUNT; i++) {
		initializeButton(&(buttons[i]));
		buttons[i].opacity = 75;
		buttons[i].buttonColor = interfaceButtonColor;
		buttons[i].y = 0;
		buttons[i].flags |= B_WIDE_CLICK_AREA;
		buttons[i].flags &= ~B_KEYPRESS_HIGHLIGHT;
	}
	
	buttonCount = 0;
	
	if (rogue.playbackMode) {
		sprintf(buttons[buttonCount].text,	" Unpause (%sspace%s) ", goldTextEscape, whiteTextEscape);
		buttons[buttonCount].hotkey[0] = ACKNOWLEDGE_KEY;
		buttonCount++;
		
		sprintf(buttons[buttonCount].text,	"Omniscience (%stab%s)", goldTextEscape, whiteTextEscape);
		buttons[buttonCount].hotkey[0] = TAB_KEY;
		buttonCount++;
		
		sprintf(buttons[buttonCount].text,	" Next Turn (%sl%s) ", goldTextEscape, whiteTextEscape);
		buttons[buttonCount].hotkey[0] = RIGHT_KEY;
		buttons[buttonCount].hotkey[1] = RIGHT_ARROW;
		buttons[buttonCount].hotkey[2] = NUMPAD_6;
		buttonCount++;
		
		strcpy(buttons[buttonCount].text,	"    菜单    ");
		buttonCount++;
	} else {
		sprintf(buttons[buttonCount].text,	"(x) 自动探索", goldTextEscape, whiteTextEscape);
		buttons[buttonCount].hotkey[0] = EXPLORE_KEY;
		buttons[buttonCount].hotkey[1] = 'X';
		buttonCount++;
		
		sprintf(buttons[buttonCount].text,	"  (z) 休息  ", goldTextEscape, whiteTextEscape);
		buttons[buttonCount].hotkey[0] = REST_KEY;
		buttonCount++;
		
		sprintf(buttons[buttonCount].text,	"(s) 搜索周围", goldTextEscape, whiteTextEscape);
		buttons[buttonCount].hotkey[0] = SEARCH_KEY;
		buttonCount++;
		
		strcpy(buttons[buttonCount].text,   "    菜单    ");
		buttonCount++;
	}
	
	sprintf(buttons[4].text,	" (i) 物品栏 ", goldTextEscape, whiteTextEscape);
	buttons[4].hotkey[0] = INVENTORY_KEY;
	buttons[4].hotkey[1] = 'I';
	
	x = 0;
	for (i=0; i<5; i++) {
		buttons[i].x = x;
		x += strLenWithoutEscapes(buttons[i].text) + 2; // Gap between buttons.
	}
	
	initializeButtonState(state,
						  buttons,
						  5,
						  io_state.button_context,
						  io_state.button_effect,
						  0,
						  0,
						  DCOLS,
						  1);
	
	for (i=0; i < 5; i++) {
		drawButton(io_state.button_context, io_state.button_effect,
				   &(state->buttons[i]), BUTTON_NORMAL);
	}
}

// This is basically the main loop for the game.
void mainInputLoop() {
	short originLoc[2], oldTargetLoc[2], pathDestination[2], path[1000][2], steps, oldRNG, dir, newX, newY;
	creature *monst;
	item *theItem;
	
	boolean canceled, targetConfirmed, tabKey, focusedOnMonster, focusedOnItem, focusedOnTerrain,
	playingBack, doEvent, textDisplayed, cursorMode;
	BROGUE_WINDOW *details_window;
	
	rogueEvent theEvent;
	short **costMap;
	brogueButton buttons[5] = {{{0}}};
	buttonState state;
	short buttonInput;
	
	short *cursor = rogue.cursorLoc; // shorthand
	
	canceled = false;
	cursorMode = false; // Controls whether the keyboard moves the cursor or the character.
	steps = 0;
	
	rogue.cursorPathIntensity = (cursorMode ? 50 : 20);
	
	// Initialize buttons.
	initializeMenuButtons(&state, buttons);
	
	playingBack = rogue.playbackMode;
	rogue.playbackMode = false;
	costMap = allocGrid();
	
	cursor[0] = cursor[1] = -1;
	oldTargetLoc[0] = oldTargetLoc[1] = -1;
	
	while (!rogue.gameHasEnded && (!playingBack || !canceled)) { // repeats until the game ends
		
		oldRNG = rogue.RNG;
		rogue.RNG = RNG_COSMETIC;
		
		focusedOnMonster = focusedOnItem = focusedOnTerrain = false;
		steps = 0;
		clearCursorPath();
		
		originLoc[0] = player.xLoc;
		originLoc[1] = player.yLoc;
		
		if (playingBack && cursorMode) {
			temporaryMessage("请选取查看的目标 (使用 <hjklyubn>, <tab> 键或者鼠标来进行选择)", false);
		}
		
		if (!playingBack
			&& player.xLoc == cursor[0]
			&& player.yLoc == cursor[1]
			&& oldTargetLoc[0] == cursor[0]
			&& oldTargetLoc[1] == cursor[1]) {
			
			// Path hides when you reach your destination.
			cursorMode = false;
			rogue.cursorPathIntensity = (cursorMode ? 50 : 20);
			cursor[0] = -1;
			cursor[1] = -1;
		}
		
		oldTargetLoc[0] = cursor[0];
		oldTargetLoc[1] = cursor[1];
		
		populateCreatureCostMap(costMap, &player);
		costMap[rogue.downLoc[0]][rogue.downLoc[1]] = 100;
		costMap[rogue.upLoc[0]][rogue.upLoc[1]] = 100;
		fillGrid(playerPathingMap, 30000);
		playerPathingMap[player.xLoc][player.yLoc] = 0;
		dijkstraScan(playerPathingMap, costMap, true);
        
		
		do {
			textDisplayed = false;
			
			// Draw the cursor and path
			if (coordinatesAreInMap(oldTargetLoc[0], oldTargetLoc[1])) {
				refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);				// Remove old cursor.
			}
			if (!playingBack) {
				if (coordinatesAreInMap(oldTargetLoc[0], oldTargetLoc[1])) {
					hilitePath(path, steps, true);									// Unhilite old path.
				}
				if (coordinatesAreInMap(cursor[0], cursor[1])) {
					if (playerPathingMap[cursor[0]][cursor[1]] >= 0
						&& playerPathingMap[cursor[0]][cursor[1]] < 30000) {
						
						pathDestination[0] = cursor[0];
						pathDestination[1] = cursor[1];
					} else {
						// If the cursor is aimed at an inaccessible area, find the nearest accessible area to path toward.
						getClosestValidLocationOnMap(pathDestination, playerPathingMap, cursor[0], cursor[1]);
					}
					steps = getPathOnMap(path, playerPathingMap, pathDestination[0], pathDestination[1]) - 1;	// Get new path.
					reversePath(path, steps);												// Flip it around, back-to-front.
                    if (steps >= 0) {
                        path[steps][0] = pathDestination[0];
                        path[steps][1] = pathDestination[1];
                    }
					steps++;
					if (playerPathingMap[cursor[0]][cursor[1]] != 1
						|| pathDestination[0] != cursor[0]
						|| pathDestination[1] != cursor[1]) {
						hilitePath(path, steps, false);		// Hilite new path.
					}
				}
			}
            
			if (coordinatesAreInMap(cursor[0], cursor[1])) {				
				oldTargetLoc[0] = cursor[0];
				oldTargetLoc[1] = cursor[1];
				
				monst = monsterAtLoc(cursor[0], cursor[1]);
				theItem = itemAtLoc(cursor[0], cursor[1]);
				if (monst != NULL && (canSeeMonster(monst) || rogue.playbackOmniscience)) {
					rogue.playbackMode = playingBack;
					refreshSideBar(cursor[0], cursor[1], false);
					rogue.playbackMode = false;
					
					focusedOnMonster = true;
					if (monst != &player && (!player.status[STATUS_HALLUCINATING] || rogue.playbackOmniscience)) {
						details_window = printMonsterDetails(monst);
						textDisplayed = true;
					}
				} else if (theItem != NULL && playerCanSeeOrSense(cursor[0], cursor[1])) {
					rogue.playbackMode = playingBack;
					refreshSideBar(cursor[0], cursor[1], false);
					rogue.playbackMode = false;
					
					focusedOnItem = true;
					if (!player.status[STATUS_HALLUCINATING] || rogue.playbackOmniscience) {
						details_window = printFloorItemDetails(theItem);
						textDisplayed = true;
					}
				} else if (cellHasTMFlag(cursor[0], cursor[1], TM_LIST_IN_SIDEBAR) && playerCanSeeOrSense(cursor[0], cursor[1])) {
					rogue.playbackMode = playingBack;
					refreshSideBar(cursor[0], cursor[1], false);
					rogue.playbackMode = false;
                    focusedOnTerrain = true;
				}
				
				printLocationDescription(cursor[0], cursor[1]);
			}
			
			// Get the input!
			rogue.playbackMode = playingBack;
			doEvent = moveCursor(
				&targetConfirmed, &canceled, &tabKey, cursor, &theEvent, 
				&state, io_state.button_context, io_state.button_effect,
				!textDisplayed, cursorMode, true);
			rogue.playbackMode = false;
			
			if (state.buttonChosen == 3) { // Actions menu button.
				buttonInput = actionMenu(buttons[3].x, buttons[3].y - (playingBack ? 14 : 13), playingBack); // Returns the corresponding keystroke.
				if (buttonInput == -1) { // Canceled.
					doEvent = false;
				} else {
					theEvent.eventType = KEYSTROKE;
					theEvent.param1 = buttonInput;
					theEvent.param2 = 0;
					theEvent.shiftKey = theEvent.controlKey = false;
					doEvent = true;
				}
			} else if (state.buttonChosen > -1) {
				theEvent.eventType = KEYSTROKE;
				theEvent.param1 = buttons[state.buttonChosen].hotkey[0];
				theEvent.param2 = 0;
			}
			state.buttonChosen = -1;
			
			if (playingBack) {
				if (canceled) {
					cursorMode = false;
					rogue.cursorPathIntensity = (cursorMode ? 50 : 20);
				}
				
				if (theEvent.eventType == KEYSTROKE
					&& theEvent.param1 == ACKNOWLEDGE_KEY) { // To unpause by button during playback.
					canceled = true;
				} else {
					canceled = false;
				}
			}

			if (focusedOnMonster || focusedOnItem || focusedOnTerrain) {
				focusedOnMonster = false;
				focusedOnItem = false;
                focusedOnTerrain = false;
				if (textDisplayed) {
					BrogueWindow_close(details_window);
					details_window = NULL;
				}
				rogue.playbackMode = playingBack;
				refreshSideBar(-1, -1, false);
				rogue.playbackMode = false;
			}
            
			if (tabKey && !playingBack) { // The tab key cycles the cursor through monsters, items and terrain features.
				if (nextTargetAfter(&newX, &newY, cursor[0], cursor[1], true, true, true, true, false)) {
                    cursor[0] = newX;
                    cursor[1] = newY;
                }
			}
			
			if (theEvent.eventType == KEYSTROKE
				&& (theEvent.param1 == ASCEND_KEY && cursor[0] == rogue.upLoc[0] && cursor[1] == rogue.upLoc[1]
					|| theEvent.param1 == DESCEND_KEY && cursor[0] == rogue.downLoc[0] && cursor[1] == rogue.downLoc[1])) {
					
					targetConfirmed = true;
					doEvent = false;
				}
		} while (!targetConfirmed && !canceled && !doEvent && !rogue.gameHasEnded);
		
		if (coordinatesAreInMap(oldTargetLoc[0], oldTargetLoc[1])) {
			refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);						// Remove old cursor.
		}
        
		restoreRNG;
		
		if (canceled && !playingBack) {
			// Drop out of cursor mode if we're in it, and hide the path either way.
			confirmMessages();
			cursorMode = false;
			rogue.cursorPathIntensity = (cursorMode ? 50 : 20);
			cursor[0] = -1;
			cursor[1] = -1;
		} else if (targetConfirmed && !playingBack && coordinatesAreInMap(cursor[0], cursor[1])) {
			if (theEvent.eventType == MOUSE_UP
				&& theEvent.controlKey
				&& steps > 1) {
				// Control-clicking moves the player one step along the path.
				
				for (dir=0;
					 dir<8 && (player.xLoc + nbDirs[dir][0] != path[0][0] || player.yLoc + nbDirs[dir][1] != path[0][1]);
					 dir++);
				playerMoves(dir);
			} else if (D_WORMHOLING) {
				travel(cursor[0], cursor[1], true);
			} else {
				confirmMessages();
				
				if (originLoc[0] == cursor[0]
					&& originLoc[1] == cursor[1]) {
					
					confirmMessages();
				} else if (abs(player.xLoc - cursor[0]) + abs(player.yLoc - cursor[1]) == 1 // horizontal or vertical
						   || (distanceBetween(player.xLoc, player.yLoc, cursor[0], cursor[1]) == 1 // includes diagonals
							   && (!diagonalBlocked(player.xLoc, player.yLoc, cursor[0], cursor[1])
                                   || ((pmap[cursor[0]][cursor[1]].flags & HAS_MONSTER) && (monsterAtLoc(cursor[0], cursor[1])->info.flags & MONST_ATTACKABLE_THRU_WALLS)) // there's a turret there
                                   || ((terrainFlags(cursor[0], cursor[1]) & T_OBSTRUCTS_PASSABILITY) && (terrainMechFlags(cursor[0], cursor[1]) & TM_PROMOTES_ON_PLAYER_ENTRY))))) { // there's a lever there
							   // Clicking one space away will cause the player to try to move there directly irrespective of path.
							   for (dir=0;
									dir<8 && (player.xLoc + nbDirs[dir][0] != cursor[0] || player.yLoc + nbDirs[dir][1] != cursor[1]);
									dir++);
							   playerMoves(dir);
						   } else if (steps) {
							   travelRoute(path, steps);
						   }
			}
		}
		
		if (theEvent.eventType == KEYSTROKE
			&& (theEvent.param1 == RETURN_KEY || theEvent.param1 == ENTER_KEY)) {
			
			// Return or enter turns on cursor mode. When the path is hidden, move the cursor to the player.
			if (!coordinatesAreInMap(cursor[0], cursor[1])) {
				cursor[0] = player.xLoc;
				cursor[1] = player.yLoc;
				cursorMode = true;
				rogue.cursorPathIntensity = (cursorMode ? 50 : 20);
			} else if (playingBack && cursorMode) {
				cursorMode = false;
				rogue.cursorPathIntensity = (cursorMode ? 50 : 20);
				cursor[0] = cursor[1] = -1;
				updateMessageDisplay();
			} else {
				cursorMode = true;
				rogue.cursorPathIntensity = (cursorMode ? 50 : 20);
			}
		}
		
		if (doEvent) {
			// If the player entered input during moveCursor() that wasn't a cursor movement command.
			// Mainly, we want to filter out directional keystrokes when we're in cursor mode, since
			// those should move the cursor but not the player.
			if (playingBack) {
				rogue.playbackMode = true;
				executePlaybackInput(&theEvent);
				playingBack = rogue.playbackMode;
				rogue.playbackMode = false;
			} else {
				executeEvent(&theEvent);
				if (rogue.playbackMode) {
					playingBack = true;
					rogue.playbackMode = false;
					confirmMessages();
					break;
				}
			}
		}
	}
	
	rogue.playbackMode = playingBack;
	refreshSideBar(-1, -1, false);
	freeGrid(costMap);
}

// accuracy depends on how many clock cycles occur per second
#define MILLISECONDS	(clock() * 1000 / CLOCKS_PER_SEC)

#define MILLISECONDS_FOR_CAUTION	100

void considerCautiousMode() {
	/*
	signed long oldMilliseconds = rogue.milliseconds;
	rogue.milliseconds = MILLISECONDS;
	clock_t i = clock();
	printf("\n%li", i);
	if (rogue.milliseconds - oldMilliseconds < MILLISECONDS_FOR_CAUTION) {
		rogue.cautiousMode = true;
	}*/
}

// higher-level redraw
void displayLevel() {
	short i, j;
	
	if (!rogue.playbackFastForward) {
		if (io_state.alert_window) {
			BrogueWindow_close(io_state.alert_window);
			io_state.alert_window = NULL;
		}

		for( i=0; i<DCOLS; i++ ) {
			for( j=0; j<DROWS; j++ ) {
				refreshDungeonCell(i, j);
			}
		}
	}
}

// converts colors into components
void storeColorComponents(char components[3], const color *theColor) {
	components[0] = theColor->red;
	components[1] = theColor->green;
	components[2] = theColor->blue;
}

void bakeTerrainColors(color *foreColor, color *backColor, short x, short y) {
    const short *vals;
    const short nf = 1000;
    const short nb = 0;
    const short neutralColors[8] = {nf, nf, nf, nf, nb, nb, nb, nb};

    if (rogue.trueColorMode) {
        vals = neutralColors;
    } else {
        vals = &(terrainRandomValues[x][y][0]);
    }
    
	const short foreRand = foreColor->rand * vals[6] / 1000;
	const short backRand = backColor->rand * vals[7] / 1000;
	
	foreColor->red += foreColor->redRand * vals[0] / 1000 + foreRand;
	foreColor->green += foreColor->greenRand * vals[1] / 1000 + foreRand;
	foreColor->blue += foreColor->blueRand * vals[2] / 1000 + foreRand;
	foreColor->redRand = foreColor->greenRand = foreColor->blueRand = foreColor->rand = 0;
	
	backColor->red += backColor->redRand * vals[3] / 1000 + backRand;
	backColor->green += backColor->greenRand * vals[4] / 1000 + backRand;
	backColor->blue += backColor->blueRand * vals[5] / 1000 + backRand;
	backColor->redRand = backColor->greenRand = backColor->blueRand = backColor->rand = 0;
	
	if (foreColor->colorDances || backColor->colorDances) {
		pmap[x][y].flags |= TERRAIN_COLORS_DANCING;
	} else {
		pmap[x][y].flags &= ~TERRAIN_COLORS_DANCING;
	}
}

void bakeColor(color *theColor) {
	short rand;
	rand = rand_range(0, theColor->rand);
	theColor->red += rand_range(0, theColor->redRand) + rand;
	theColor->green += rand_range(0, theColor->greenRand) + rand;
	theColor->blue += rand_range(0, theColor->blueRand) + rand;
	theColor->redRand = theColor->greenRand = theColor->blueRand = theColor->rand = 0;
}

void shuffleTerrainColors(short percentOfCells, boolean refreshCells) {
	short i, j, k;
	
	assureCosmeticRNG;
	
	for (i=0; i<DCOLS; i++) {
		for(j=0; j<DROWS; j++) {
			if (playerCanSeeOrSense(i, j)
				&& (!rogue.automationActive || !(rogue.playerTurnNumber % 5))
				&& ((pmap[i][j].flags & TERRAIN_COLORS_DANCING)
					|| (player.status[STATUS_HALLUCINATING] && playerCanDirectlySee(i, j)))
				&& (i != rogue.cursorLoc[0] || j != rogue.cursorLoc[1])
				&& (percentOfCells >= 100 || rand_range(1, 100) <= percentOfCells)) {
					
					for (k=0; k<8; k++) {
						terrainRandomValues[i][j][k] += rand_range(-600, 600);
						terrainRandomValues[i][j][k] = clamp(terrainRandomValues[i][j][k], 0, 1000);
					}
				}
		}
	}

	if (refreshCells)
	{
		displayLevel();
	}
	restoreRNG;
}

/*  Pick a random value, as seeded by the terrain values for the cell  */
int rand_range_cell(int x, int y, int min, int max)
{
	return min + (max - min) * terrainRandomValues[x][y][0] / 1000;
}

// if forecolor is too similar to back, darken or lighten it and return true.
// Assumes colors have already been baked (no random components).
boolean separateColors(color *fore, color *back) {
	color f, b, *modifier;
	short failsafe;
	boolean madeChange;
	
	f = *fore;
	b = *back;
	f.red		= max(0, min(100, f.red));
	f.green		= max(0, min(100, f.green));
	f.blue		= max(0, min(100, f.blue));
	b.red		= max(0, min(100, b.red));
	b.green		= max(0, min(100, b.green));
	b.blue		= max(0, min(100, b.blue));
	
	if (f.red + f.blue + f.green > 50 * 3) {
		modifier = &black;
	} else {
		modifier = &white;
	}
	
	madeChange = false;
	failsafe = 10;
	
	while(COLOR_DIFF(f, b) < MIN_COLOR_DIFF && --failsafe) {
		applyColorAverage(&f, modifier, 20);
		madeChange = true;
	}
	
	if (madeChange) {
		*fore = f;
		return true;
	} else {
		return false;
	}
}

uchar transformCellCharByNeighbors(uchar cellChar, short x, short y)
{
    /*  Transform to vertical wall characters if the terrain character
		below us is the same as ours.  */
    if (cellChar == 0xE1 || cellChar == WALL_CHAR)
    {
		if (y + 1 < DROWS)
		{
			enum tileType lowerTile = pmap[x][y + 1].layers[DUNGEON];
			uchar lowerChar = tileCatalog[lowerTile].displayChar;
			int flags = pmap[x][y + 1].flags;

			if ((flags & DISCOVERED) == 0)
			{
				if (!playerCanSeeOrSense(x, y + 1))
				{
					return cellChar;
				}
			}
			
			if (lowerChar == cellChar)
			{
				cellChar = 0xB1;
			}
		}
    }

    return cellChar;
}

/*  Apply color tweaks to lighting values used for a dungeon cell  */
void applyLighting(
	int x, int y, color *cellForeColor, color *cellBackColor, 
	color lightMultiplierColor, color lightAugment, boolean needDistinctness)
{
	if (!rogue.trueColorMode || !needDistinctness) {
		applyColorMultiplier(cellForeColor, &lightMultiplierColor);
		applyColorAugment(cellForeColor, &lightAugment, 100);
	}
	applyColorMultiplier(cellBackColor, &lightMultiplierColor);
	applyColorAugment(cellBackColor, &lightAugment, 100);
		
	if (player.status[STATUS_HALLUCINATING] && !rogue.trueColorMode) {
		randomizeColor(x, y, cellForeColor, 40 * player.status[STATUS_HALLUCINATING] / 300 + 20);
		randomizeColor(x, y, cellBackColor, 40 * player.status[STATUS_HALLUCINATING] / 300 + 20);
	}
	if (rogue.inWater) {
		applyColorMultiplier(cellForeColor, &deepWaterLightColor);
		applyColorMultiplier(cellBackColor, &deepWaterLightColor);
	}
}

// okay, this is kind of a beast...
void getCellAppearance(
	BROGUE_DRAW_CONTEXT *context, short x, short y, color *foreColor,
	uchar *returnChar) {

	short bestBCPriority, bestFCPriority, bestCharPriority;
	uchar cellChar = 0;
	color cellForeColor, cellBackColor, gasAugmentColor;
	color cornerBackColor[4], cornerForeColor[4];
	boolean monsterWithDetectedItem = false, needDistinctness = false;
	boolean bypassLighting = false;
	short gasAugmentWeight = 0;
	creature *monst = NULL;
	item *theItem = NULL;
    enum tileType tile = NOTHING;
	uchar itemChars[] = {POTION_CHAR, SCROLL_CHAR, FOOD_CHAR, WAND_CHAR,
						STAFF_CHAR, GOLD_CHAR, ARMOR_CHAR, WEAPON_CHAR, RING_CHAR, CHARM_CHAR};
	enum dungeonLayers layer, maxLayer;
	int i;
	
	assureCosmeticRNG;

#ifdef BROGUE_ASSERTS
	assert(coordinatesAreInMap(x, y));
#endif
	
	if (pmap[x][y].flags & HAS_MONSTER) {
		monst = monsterAtLoc(x, y);
	} else if (pmap[x][y].flags & HAS_DORMANT_MONSTER) {
		monst = dormantMonsterAtLoc(x, y);
	}
	if (monst) {
		monsterWithDetectedItem = (monst->carriedItem && (monst->carriedItem->flags & ITEM_MAGIC_DETECTED)
								   && itemMagicChar(monst->carriedItem) && !canSeeMonster(monst));
	}
	
	if (monsterWithDetectedItem) {
		theItem = monst->carriedItem;
	} else {
		theItem = itemAtLoc(x, y);
	}
	
	if (!playerCanSeeOrSense(x, y)
		&& !(pmap[x][y].flags & (ITEM_DETECTED | HAS_PLAYER))
		&& (!monst || !monsterRevealed(monst))
		&& !monsterWithDetectedItem
		&& (pmap[x][y].flags & (DISCOVERED | MAGIC_MAPPED))
		&& (pmap[x][y].flags & STABLE_MEMORY)) {
		
		// restore memory
		cellChar = pmap[x][y].rememberedAppearance.character;
		cellForeColor = colorFromComponents(pmap[x][y].rememberedAppearance.foreColorComponents);
		cellBackColor = colorFromComponents(pmap[x][y].rememberedAppearance.backColorComponents);
	} else {
		// Find the highest-priority fore color, back color and character.
		bestFCPriority = bestBCPriority = bestCharPriority = 10000;
        
        // Default to the appearance of floor.
        cellForeColor = *(tileCatalog[FLOOR].foreColor);
        cellBackColor = *(tileCatalog[FLOOR].backColor);
        cellChar = tileCatalog[FLOOR].displayChar;

		if ((pmap[x][y].flags & MAGIC_MAPPED) && !(pmap[x][y].flags & DISCOVERED) && !rogue.playbackOmniscience) {
			maxLayer = LIQUID + 1; // Can see only dungeon and liquid layers with magic mapping.
		} else {
			maxLayer = NUMBER_TERRAIN_LAYERS;
		}
		
		for (layer = 0; layer < maxLayer; layer++) {
			// Gas shows up as a color average, not directly.
			if (pmap[x][y].layers[layer] && layer != GAS) {
                tile = pmap[x][y].layers[layer];
                if (rogue.playbackOmniscience && (tileCatalog[tile].mechFlags & TM_IS_SECRET)) {
                    tile = dungeonFeatureCatalog[tileCatalog[tile].discoverType].tile;
                }
				
				if (tileCatalog[tile].drawPriority < bestFCPriority
					&& tileCatalog[tile].foreColor) {
					
					cellForeColor = *(tileCatalog[tile].foreColor);
					bestFCPriority = tileCatalog[tile].drawPriority;
				}
				if (tileCatalog[tile].drawPriority < bestBCPriority
					&& tileCatalog[tile].backColor) {
					
					cellBackColor = *(tileCatalog[tile].backColor);
					bestBCPriority = tileCatalog[tile].drawPriority;
				}
				if (tileCatalog[tile].drawPriority < bestCharPriority
					&& tileCatalog[tile].displayChar) {
					
					cellChar = tileCatalog[tile].displayChar;
					bestCharPriority = tileCatalog[tile].drawPriority;
                    needDistinctness = (tileCatalog[tile].mechFlags & TM_VISUALLY_DISTINCT) ? true : false;
				}
			}
		}
		
		if (pmap[x][y].layers[GAS]
			&& tileCatalog[pmap[x][y].layers[GAS]].backColor
			&& playerCanSeeOrSense(x, y)) {
			
			gasAugmentColor = *(tileCatalog[pmap[x][y].layers[GAS]].backColor);
			if (rogue.trueColorMode) {
				gasAugmentWeight = 30;
			} else {
				gasAugmentWeight = min(90, 30 + pmap[x][y].volume);
			}
		}
		
		if (pmap[x][y].flags & HAS_PLAYER) {
			cellChar = player.info.displayChar;
			cellForeColor = *(player.info.foreColor);
			needDistinctness = true;
		} else if (((pmap[x][y].flags & HAS_ITEM) && (pmap[x][y].flags & ITEM_DETECTED)
					&& itemMagicChar(theItem)
					&& (!playerCanSeeOrSense(x, y) || cellHasTerrainFlag(x, y, T_OBSTRUCTS_ITEMS)))
				   || monsterWithDetectedItem){
			cellChar = itemMagicChar(theItem);
			cellForeColor = white;
			needDistinctness = true;
			if (cellChar == GOOD_MAGIC_CHAR) {
				cellForeColor = goodMessageColor;
			} else if (cellChar == BAD_MAGIC_CHAR) {
				cellForeColor = badMessageColor;
			}
			cellBackColor = black;
			bypassLighting = true;
		} else if ((pmap[x][y].flags & HAS_MONSTER)
				   && (playerCanSeeOrSense(x, y) || ((monst->info.flags & MONST_IMMOBILE) && (pmap[x][y].flags & DISCOVERED)))
				   && (!monst->status[STATUS_INVISIBLE] || monst->creatureState == MONSTER_ALLY || rogue.playbackOmniscience)
				   && (!(monst->bookkeepingFlags & MONST_SUBMERGED) || rogue.inWater || rogue.playbackOmniscience)) {
			needDistinctness = true;
			if (player.status[STATUS_HALLUCINATING] > 0 && !(monst->info.flags & MONST_INANIMATE) && !rogue.playbackOmniscience) {
				cellChar = rand_range_cell(x, y, 'a', 'z') + (rand_range_cell(x, y, 0, 1) ? 'A' - 'a' : 0);
				cellForeColor = *(monsterCatalog[rand_range_cell(x, y, 1, NUMBER_MONSTER_KINDS - 1)].foreColor);
			} else {
				cellChar = monst->info.displayChar;
				if (monst->status[STATUS_INVISIBLE] || (monst->bookkeepingFlags & MONST_SUBMERGED)) {
                    // Invisible allies show up on the screen with a transparency effect.
					cellForeColor = cellBackColor;
				} else {
					cellForeColor = *(monst->info.foreColor);
					if (monst->creatureState == MONSTER_ALLY
						&& (monst->info.displayChar >= 'a' && monst->info.displayChar <= 'z' || monst->info.displayChar >= 'A' && monst->info.displayChar <= 'Z')) {
						//applyColorAverage(&cellForeColor, &blue, 50);
						applyColorAverage(&cellForeColor, &pink, 50);
					}
				}
				//DEBUG if (monst->bookkeepingFlags & MONST_LEADER) applyColorAverage(&cellBackColor, &purple, 50);
			}
		} else if (monst
                   && monsterRevealed(monst)
				   && !canSeeMonster(monst)) {
			if (player.status[STATUS_HALLUCINATING] && !rogue.playbackOmniscience) {
				cellChar = (rand_range_cell(x, y, 0, 1) ? 'X' : 'x');
			} else {
				cellChar = (monst->info.displayChar >= 'a' && monst->info.displayChar <= 'z' ? 'x' : 'X');
			}
			cellForeColor = white;
			if (!(pmap[x][y].flags & DISCOVERED)) {
				cellBackColor = black;
				gasAugmentColor = black;
			}
		} else if ((pmap[x][y].flags & HAS_ITEM) && !cellHasTerrainFlag(x, y, T_OBSTRUCTS_ITEMS)
				   && (playerCanSeeOrSense(x, y) || ((pmap[x][y].flags & DISCOVERED) && !cellHasTerrainFlag(x, y, T_MOVES_ITEMS)))) {
			needDistinctness = true;
			if (player.status[STATUS_HALLUCINATING] && !rogue.playbackOmniscience) {
				cellChar = itemChars[rand_range_cell(x, y, 0, 9)];
				cellForeColor = itemColor;
			} else {
				theItem = itemAtLoc(x, y);
				cellChar = theItem->displayChar;
				cellForeColor = *(theItem->foreColor);
			}
		} else if (playerCanSeeOrSense(x, y) || (pmap[x][y].flags & (DISCOVERED | MAGIC_MAPPED))) {
			// just don't want these to be plotted as black
		} else {
			*returnChar = ' ';
			
			BrogueDrawContext_setForeground(context, 
											colorForDisplay(black));
			BrogueDrawContext_setBackground(context, 
											colorForDisplay(undiscoveredColor));

			restoreRNG;
			return;
		}
		
		if (gasAugmentWeight) {
			if (!rogue.trueColorMode || !needDistinctness) {
				applyColorAverage(&cellForeColor, &gasAugmentColor, gasAugmentWeight);
			}
			// phantoms create sillhouettes in gas clouds
			if ((pmap[x][y].flags & HAS_MONSTER)
				&& monst->status[STATUS_INVISIBLE]
				&& (playerCanSeeOrSense(x, y) || !monsterRevealed(monst))) {
				
				if (player.status[STATUS_HALLUCINATING] && !rogue.playbackOmniscience) {
					cellChar = monsterCatalog[rand_range_cell(x, y, 1, NUMBER_MONSTER_KINDS - 1)].displayChar;
				} else {
					cellChar = monst->info.displayChar;
				}
				cellForeColor = cellBackColor;
			}
			applyColorAverage(&cellBackColor, &gasAugmentColor, gasAugmentWeight);
		}
		
		if (!(pmap[x][y].flags & (ANY_KIND_OF_VISIBLE | ITEM_DETECTED | HAS_PLAYER))
			&& !playerCanSeeOrSense(x, y)
			&& (!monst || monsterRevealed(monst)) && !monsterWithDetectedItem) {			
			// store memory
			storeColorComponents(pmap[x][y].rememberedAppearance.foreColorComponents, &cellForeColor);
			storeColorComponents(pmap[x][y].rememberedAppearance.backColorComponents, &cellBackColor);
						
			pmap[x][y].rememberedAppearance.character = cellChar;
			pmap[x][y].flags |= STABLE_MEMORY;
			pmap[x][y].rememberedTerrain = pmap[x][y].layers[highestPriorityLayer(x, y, false)];
			if (pmap[x][y].flags & HAS_ITEM) {
                theItem = itemAtLoc(x, y);
				pmap[x][y].rememberedItemCategory = theItem->category;
			} else {
				pmap[x][y].rememberedItemCategory = 0;
			}
		}
	}
	
	cellChar = transformCellCharByNeighbors(cellChar, x, y);

	/*  Independently generate color values for each of the four corners of
		the cell we are about to draw.  */
	for (i = 0; i < 4; i++) {
		color cornerFore, cornerBack;
		color lightMultiplier, lightAugment;
		int lx = x;
		int ly = y;

		if ((i == 1 || i == 3) && lx < DCOLS - 1)
		{
			lx++;
		}
		if ((i == 2 || i == 3) && ly < DROWS - 1)
		{
			ly++;
		}

		if (!bypassLighting) {
			colorMultiplierFromDungeonLight(lx, ly, 
											&lightMultiplier, &lightAugment);
		} else {
			lightMultiplier = white;
			lightAugment = black;
		}

		if (foreColor != NULL)
		{
			cornerFore = *foreColor;
		}
		else
		{
			cornerFore = cellForeColor;
		}
		cornerBack = cellBackColor;
		
		applyLighting(lx, ly, &cornerFore, &cornerBack, 
					  lightMultiplier, lightAugment, needDistinctness);
		
		if (pmap[x][y].flags & IS_IN_PATH) {
			if (!rogue.trueColorMode || !needDistinctness) {
				applyColorAverage(&cornerFore, 
								  &yellow, rogue.cursorPathIntensity);
			}
			applyColorAverage(&cornerBack, &yellow, rogue.cursorPathIntensity);
		}

		if (x == rogue.cursorLoc[0] && y == rogue.cursorLoc[1])
		{
			applyColorAverage(&cornerFore, &white, 50);
			applyColorAverage(&cornerBack, &white, 50);
		}
	
		bakeTerrainColors(&cornerFore, &cornerBack, lx, ly);	
		if (needDistinctness) {
			separateColors(&cornerFore, &cornerBack);
		} 

		cornerBackColor[i] = cornerBack;
		cornerForeColor[i] = cornerFore;
	}

	*returnChar = cellChar;
	BrogueDrawContext_blendForeground(
		context, 
		colorForDisplay(cornerForeColor[0]),
		colorForDisplay(cornerForeColor[1]),
		colorForDisplay(cornerForeColor[2]),
		colorForDisplay(cornerForeColor[3]));
	BrogueDrawContext_blendBackground(
		context, 
		colorForDisplay(cornerBackColor[0]),
		colorForDisplay(cornerBackColor[1]),
		colorForDisplay(cornerBackColor[2]),
		colorForDisplay(cornerBackColor[3]));

	restoreRNG;
}

void refreshDungeonCell(short x, short y) {
	uchar cellChar;
#ifdef BROGUE_ASSERTS
	assert(coordinatesAreInMap(x, y));
#endif

	if (!rogue.playbackFastForward) {
		getCellAppearance(io_state.dungeon_context, x, y, NULL, &cellChar);
		BrogueDrawContext_drawChar(io_state.dungeon_context, x, y, cellChar);
	}
}

/*  Draw an item character at a particular grid cell, for effect purposes  */
void plotItemChar(wchar_t c, color *charColor, short x, short y) {
	uchar cellChar;

	if (!rogue.playbackFastForward) {
		getCellAppearance(io_state.dungeon_context, x, y, charColor, &cellChar);
		BrogueDrawContext_drawChar(io_state.dungeon_context, x, y, c);
	}
}

void applyColorMultiplier(color *baseColor, const color *multiplierColor) {
	baseColor->red = baseColor->red * multiplierColor->red / 100;
	baseColor->redRand = baseColor->redRand * multiplierColor->redRand / 100;
	baseColor->green = baseColor->green * multiplierColor->green / 100;
	baseColor->greenRand = baseColor->greenRand * multiplierColor->greenRand / 100;
	baseColor->blue = baseColor->blue * multiplierColor->blue / 100;
	baseColor->blueRand = baseColor->blueRand * multiplierColor->blueRand / 100;
	baseColor->rand = baseColor->rand * multiplierColor->rand / 100;
	//baseColor->colorDances *= multiplierColor->colorDances;
	return;
}

void applyColorAverage(color *baseColor, const color *newColor, short averageWeight) {
	short weightComplement = 100 - averageWeight;
	baseColor->red = (baseColor->red * weightComplement + newColor->red * averageWeight) / 100;
	baseColor->redRand = (baseColor->redRand * weightComplement + newColor->redRand * averageWeight) / 100;
	baseColor->green = (baseColor->green * weightComplement + newColor->green * averageWeight) / 100;
	baseColor->greenRand = (baseColor->greenRand * weightComplement + newColor->greenRand * averageWeight) / 100;
	baseColor->blue = (baseColor->blue * weightComplement + newColor->blue * averageWeight) / 100;
	baseColor->blueRand = (baseColor->blueRand * weightComplement + newColor->blueRand * averageWeight) / 100;
	baseColor->rand = (baseColor->rand * weightComplement + newColor->rand * averageWeight) / 100;
	baseColor->colorDances = (baseColor->colorDances || newColor->colorDances);
	return;
}

void applyColorAugment(color *baseColor, const color *augmentingColor, short augmentWeight) {
	baseColor->red += (augmentingColor->red * augmentWeight) / 100;
	baseColor->redRand += (augmentingColor->redRand * augmentWeight) / 100;
	baseColor->green += (augmentingColor->green * augmentWeight) / 100;
	baseColor->greenRand += (augmentingColor->greenRand * augmentWeight) / 100;
	baseColor->blue += (augmentingColor->blue * augmentWeight) / 100;
	baseColor->blueRand += (augmentingColor->blueRand * augmentWeight) / 100;
	baseColor->rand += (augmentingColor->rand * augmentWeight) / 100;
	return;
}

void applyColorScalar(color *baseColor, short scalar) {
	baseColor->red          = baseColor->red        * scalar / 100;
	baseColor->redRand      = baseColor->redRand    * scalar / 100;
	baseColor->green        = baseColor->green      * scalar / 100;
	baseColor->greenRand    = baseColor->greenRand  * scalar / 100;
	baseColor->blue         = baseColor->blue       * scalar / 100;
	baseColor->blueRand     = baseColor->blueRand   * scalar / 100;
	baseColor->rand         = baseColor->rand       * scalar / 100;
}

void applyColorBounds(color *baseColor, short lowerBound, short upperBound) {
	baseColor->red          = clamp(baseColor->red, lowerBound, upperBound);
	baseColor->redRand      = clamp(baseColor->redRand, lowerBound, upperBound);
	baseColor->green        = clamp(baseColor->green, lowerBound, upperBound);
	baseColor->greenRand    = clamp(baseColor->greenRand, lowerBound, upperBound);
	baseColor->blue         = clamp(baseColor->blue, lowerBound, upperBound);
	baseColor->blueRand     = clamp(baseColor->blueRand, lowerBound, upperBound);
	baseColor->rand         = clamp(baseColor->rand, lowerBound, upperBound);
}

void desaturate(color *baseColor, short weight) {
	short avg;
	avg = (baseColor->red + baseColor->green + baseColor->blue) / 3 + 1;
	baseColor->red = baseColor->red * (100 - weight) / 100 + (avg * weight / 100);
	baseColor->green = baseColor->green * (100 - weight) / 100 + (avg * weight / 100);
	baseColor->blue = baseColor->blue * (100 - weight) / 100 + (avg * weight / 100);
	
	avg = (baseColor->redRand + baseColor->greenRand + baseColor->blueRand);
	baseColor->redRand = baseColor->redRand * (100 - weight) / 100;
	baseColor->greenRand = baseColor->greenRand * (100 - weight) / 100;
	baseColor->blueRand = baseColor->blueRand * (100 - weight) / 100;
	
	baseColor->rand += avg * weight / 3 / 100;
}

short randomizeByPercent(short input, short percent) {
	return (rand_range(input * (100 - percent) / 100, input * (100 + percent) / 100));
}

void randomizeColor(int x, int y, color *baseColor, short randomizePercent) {
	int nonRandom = 100 - randomizePercent / 2;

	baseColor->red = baseColor->red * nonRandom / 100 
		+ baseColor->red * randomizePercent 
		* terrainRandomValues[x][y][0] / 100000;
	baseColor->green = baseColor->green * nonRandom / 100 
		+ baseColor->red * randomizePercent 
		* terrainRandomValues[x][y][0] / 100000;
	baseColor->blue = baseColor->blue * nonRandom / 100 
		+ baseColor->blue * randomizePercent 
		* terrainRandomValues[x][y][0] / 100000;
}

// Assumes colors are pre-baked.
void blendAppearances(const color *fromForeColor, const color *fromBackColor, const uchar fromChar, const int fromFlags,
                      const color *toForeColor, const color *toBackColor, const uchar toChar, const int toFlags,
                      color *retForeColor, color *retBackColor, uchar *retChar, int *retFlags,
                      const short percent) {
    // Straight average of the back color:
    *retBackColor = *fromBackColor;
    applyColorAverage(retBackColor, toBackColor, percent);
    
    // Pick the character:
    if (percent >= 50) {
        *retChar = toChar;
	*retFlags = toFlags;
    } else {
        *retChar = fromChar;
	*retFlags = fromFlags;
    }
    
    // Pick the method for blending the fore color.
    if (fromChar == toChar) {
        // If the character isn't changing, do a straight average.
        *retForeColor = *fromForeColor;
        applyColorAverage(retForeColor, toForeColor, percent);
    } else {
        // If it is changing, the first half blends to the current back color, and the second half blends to the final back color.
        if (percent >= 50) {
            *retForeColor = *retBackColor;
            applyColorAverage(retForeColor, toForeColor, (percent - 50) * 2);
        } else {
            *retForeColor = *fromForeColor;
            applyColorAverage(retForeColor, retBackColor, percent * 2);
        }
    }
}

// takes dungeon coordinates
void colorBlendCell(short x, short y, color *hiliteColor, short hiliteStrength) {
	uchar displayChar;
	
	if (!rogue.playbackFastForward) {
		BrogueDrawContext_push(io_state.dungeon_context);
		BrogueDrawContext_setColorAverageTarget(
			io_state.dungeon_context, 
			hiliteStrength / 100.0, colorForDisplay(*hiliteColor), 0);
		BrogueDrawContext_setColorAverageTarget(
			io_state.dungeon_context,
			hiliteStrength / 100.0, colorForDisplay(*hiliteColor), 1);

		getCellAppearance(io_state.dungeon_context, x, y, NULL, &displayChar);

		BrogueDrawContext_drawChar(io_state.dungeon_context, x, y, displayChar);
		BrogueDrawContext_pop(io_state.dungeon_context);
	}
}

// takes dungeon coordinates
void hiliteCell(short x, short y, const color *hiliteColor, short hiliteStrength, boolean distinctColors) {
	uchar displayChar;
	
	assureCosmeticRNG;
	
	if (!rogue.playbackFastForward) {
		BrogueDrawContext_push(io_state.dungeon_context);
		BrogueDrawContext_setColorAddition(
			io_state.dungeon_context, 
			weightedColorForDisplay(*hiliteColor, hiliteStrength), 0);
		BrogueDrawContext_setColorAddition(
			io_state.dungeon_context, 
			weightedColorForDisplay(*hiliteColor, hiliteStrength), 1);  
		
		getCellAppearance(io_state.dungeon_context, x, y, NULL, &displayChar);
		
		BrogueDrawContext_drawChar(io_state.dungeon_context, x, y, displayChar);
		BrogueDrawContext_pop(io_state.dungeon_context);
	}
	
	restoreRNG;
}

short adjustedLightValue(short x) {
    if (x <= LIGHT_SMOOTHING_THRESHOLD) {
        return x;
    } else {
        return (short) (sqrt(((float) x)/LIGHT_SMOOTHING_THRESHOLD)*LIGHT_SMOOTHING_THRESHOLD + FLOAT_FUDGE);
    }
}

/*  Generate the color value for the corner of a dungeon cell, taking
	into account the visibility of neighbor cells  */
void colorMultiplierFromDungeonLight(
	short x, short y, color *editColor, color *augment) {

	short x1 = x - 1;
	short x2 = x;
	short y1 = y - 1;
	short y2 = y;

	boolean ul = (x1 >= 0 && y1 >= 0);
	boolean ur = (x2 < DCOLS && y1 >= 0);
	boolean bl = (x1 >= 0 && y2 < DROWS);
	boolean br = (x2 < DCOLS && y2 < DROWS);

	boolean ul_vis = (ul && (playerCanSeeOrSense(x1, y1) || pmap[x1][y1].flags & (DISCOVERED | MAGIC_MAPPED)));
	boolean ur_vis = (ur && (playerCanSeeOrSense(x2, y1) || pmap[x2][y1].flags & (DISCOVERED | MAGIC_MAPPED)));
	boolean bl_vis = (bl && (playerCanSeeOrSense(x1, y2) || pmap[x1][y2].flags & (DISCOVERED | MAGIC_MAPPED)));
	boolean br_vis = (br && (playerCanSeeOrSense(x2, y2) || pmap[x2][y2].flags & (DISCOVERED | MAGIC_MAPPED)));

	boolean ul_map = (ul_vis && !(pmap[x1][y1].flags & DISCOVERED));
	boolean ur_map = (ur_vis && !(pmap[x2][y1].flags & DISCOVERED));
	boolean bl_map = (bl_vis && !(pmap[x1][y2].flags & DISCOVERED));
	boolean br_map = (br_vis && !(pmap[x2][y2].flags & DISCOVERED));

	boolean ul_see = (ul && playerCanSeeOrSense(x1, y1));
	boolean ur_see = (ur && playerCanSeeOrSense(x2, y1));
	boolean bl_see = (bl && playerCanSeeOrSense(x1, y2));
	boolean br_see = (br && playerCanSeeOrSense(x2, y2));

	boolean ul_anyvis = (ul && (pmap[x1][y1].flags & ANY_KIND_OF_VISIBLE));
	boolean ur_anyvis = (ur && (pmap[x2][y1].flags & ANY_KIND_OF_VISIBLE));
	boolean bl_anyvis = (bl && (pmap[x1][y2].flags & ANY_KIND_OF_VISIBLE));
	boolean br_anyvis = (br && (pmap[x2][y2].flags & ANY_KIND_OF_VISIBLE));

	boolean ul_clair = (ul && !(pmap[x1][y1].flags & VISIBLE) 
						&& (pmap[x1][y1].flags & CLAIRVOYANT_VISIBLE));
	boolean ur_clair = (ur && !(pmap[x2][y1].flags & VISIBLE) 
						&& (pmap[x2][y1].flags & CLAIRVOYANT_VISIBLE));
	boolean bl_clair = (bl && !(pmap[x1][y2].flags & VISIBLE) 
						&& (pmap[x1][y2].flags & CLAIRVOYANT_VISIBLE));
	boolean br_clair = (br && !(pmap[x2][y2].flags & VISIBLE) 
						&& (pmap[x2][y2].flags & CLAIRVOYANT_VISIBLE));

	boolean ul_tele = (ul && !(pmap[x1][y1].flags & VISIBLE) 
						&& (pmap[x1][y1].flags & TELEPATHIC_VISIBLE));
	boolean ur_tele = (ur && !(pmap[x2][y1].flags & VISIBLE) 
						&& (pmap[x2][y1].flags & TELEPATHIC_VISIBLE));
	boolean bl_tele = (bl && !(pmap[x1][y2].flags & VISIBLE) 
						&& (pmap[x1][y2].flags & TELEPATHIC_VISIBLE));
	boolean br_tele = (br && !(pmap[x2][y2].flags & VISIBLE) 
						&& (pmap[x2][y2].flags & TELEPATHIC_VISIBLE));

	*editColor = black;
	*augment = black;

	if (!ul_vis || !ur_vis || !bl_vis || !br_vis) {
		return;
	}

	if (rogue.playbackOmniscience 
		&& (!ul_anyvis || !ur_anyvis || !bl_anyvis || !br_anyvis)) {

		*editColor = omniscienceColor;
		return;
	}

	if (ul_map && ur_map && bl_map && br_map) {
		*editColor = magicMapColor;
		return;
	}

	if (!ul_see || !ur_see || !bl_see || !br_see)
	{
		*editColor = memoryColor;
		*augment = memoryOverlay;
		applyColorScalar(augment, 25);

		return;
	}

	if (ul_clair || ur_clair || bl_clair || br_clair) {
		*editColor = clairvoyanceColor;
		return;
	}

	if (ul_tele || ur_tele || bl_tele || br_tele)
	{
		*editColor = telepathyMultiplier;
		return;
	}

	editColor->red		= adjustedLightValue(max(0, lightmap[x][y].light[0]));
	editColor->green	= adjustedLightValue(max(0, lightmap[x][y].light[1]));
	editColor->blue		= adjustedLightValue(max(0, lightmap[x][y].light[2]));
	
	editColor->rand = adjustedLightValue(max(0, lightmap[x][y].light[0] + lightmap[x][y].light[1] + lightmap[x][y].light[2]) / 6);
	editColor->colorDances = false;
}

void blackOutScreen() {
	BrogueWindow_clear(io_state.root);
	BrogueWindow_clear(io_state.dungeon_window);
	BrogueWindow_clear(io_state.message_window);
	BrogueWindow_clear(io_state.flavor_window);
	BrogueWindow_clear(io_state.button_window);
	BrogueWindow_clear(io_state.sidebar_window);
}

void colorOverDungeon(const color *color) {
	BrogueWindow_clear(io_state.dungeon_window);
}

color colorFromComponents(char rgb[3]) {
	color theColor = black;
	theColor.red	= rgb[0];
	theColor.green	= rgb[1];
	theColor.blue	= rgb[2];
	return theColor;
}

// Takes a list of locations, a color and a list of strengths and flashes the foregrounds of those locations.
// Strengths are percentages measuring how hard the color flashes at its peak.
void flashForeground(short *x, short *y, color **flashColor, short *flashStrength, short count, short frames) {
	short i, j, percent;

	if (count <= 0) {
		return;
	}
	
	assureCosmeticRNG;
	
	for (j=frames; j>= 0; j--) {
		for (i=0; i<count; i++) {
			uchar c;

			percent = flashStrength[i] * j / frames;
			
			BrogueDrawContext_push(io_state.dungeon_context);
			BrogueDrawContext_setColorAverageTarget(
				io_state.dungeon_context, percent / 100.0,
				colorForDisplay(*flashColor[i]), true);
			getCellAppearance(io_state.dungeon_context,
 							  x[i], y[i], NULL, &c);
			BrogueDrawContext_drawChar(
				io_state.dungeon_context, x[i], y[i], c);
			BrogueDrawContext_pop(io_state.dungeon_context);
		}
		if (j) {
			if (pauseBrogue(1)) {
				j = 1;
			}
		}
	}
	
	restoreRNG;
}

void flash(color *theColor, short frames, short x, short y) {
	short i;
	boolean interrupted = false;
	
	for (i=0; i<frames && !interrupted; i++) {
		colorBlendCell(x, y, theColor, 100 - 100 * i / frames);
		interrupted = pauseBrogue(50);
	}
	
	refreshDungeonCell(x, y);
}

// special effect expanding flash of light at dungeon coordinates (x, y) restricted to tiles with matching flags
void colorFlash(const color *theColor, unsigned long reqTerrainFlags,
				unsigned long reqTileFlags, short frames, short maxRadius, short x, short y) {
	short i, j, k, intensity, currentRadius, fadeOut;
	short localRadius[DCOLS][DROWS];
	boolean tileQualifies[DCOLS][DROWS], aTileQualified, fastForward;
	
	aTileQualified = false;
	fastForward = false;
	
	for (i = max(x - maxRadius, 0); i < min(x + maxRadius, DCOLS); i++) {
		for (j = max(y - maxRadius, 0); j < min(y + maxRadius, DROWS); j++) {
			if ((!reqTerrainFlags || cellHasTerrainFlag(reqTerrainFlags, i, j))
				&& (!reqTileFlags || (pmap[i][j].flags & reqTileFlags))
				&& (i-x) * (i-x) + (j-y) * (j-y) <= maxRadius * maxRadius) {
				
				tileQualifies[i][j] = true;
				localRadius[i][j] = sqrt((i-x) * (i-x) + (j-y) * (j-y));
				aTileQualified = true;
			} else {
				tileQualifies[i][j] = false;
			}
		}
	}
	
	if (!aTileQualified) {
		return;
	}
	
	for (k = 1; k <= frames; k++) {
		currentRadius = max(1, maxRadius * k / frames);
		fadeOut = min(100, (frames - k) * 100 * 5 / frames);
		for (i = max(x - maxRadius, 0); i < min(x + maxRadius, DCOLS); i++) {
			for (j = max(y - maxRadius, 0); j < min(y + maxRadius, DROWS); j++) {
				if (tileQualifies[i][j] && (localRadius[i][j] <= currentRadius)) {
					
					intensity = 100 - 100 * (currentRadius - localRadius[i][j] - 2) / currentRadius;
					intensity = fadeOut * intensity / 100;
					
					hiliteCell(i, j, theColor, intensity, false);
				}
			}
		}
		if (!fastForward && (rogue.playbackFastForward || pauseBrogue(50))) {
			k = frames - 1;
			fastForward = true;
		}
	}
}

#define bCurve(x)	(((x) * (x) + 11) / (10 * ((x) * (x) + 1)) - 0.1)

void displayWaypoints() {
    short i, j, w, lowestDistance;
    
    for (i=0; i<DCOLS; i++) {
        for (j=0; j<DROWS; j++) {
            lowestDistance = 30000;
            for (w=0; w<rogue.wpCount; w++) {
                if (rogue.wpDistance[w][i][j] < lowestDistance) {
                    lowestDistance = rogue.wpDistance[w][i][j];
                }
            }
            if (lowestDistance < 10) {
                hiliteCell(i, j, &white, clamp(100 - lowestDistance*15, 0, 100), true);
            }
        }
    }
    temporaryMessage("Waypoints:", true);
}

boolean pauseBrogue(short milliseconds) {
	boolean interrupted;
	
	if (rogue.playbackMode && rogue.playbackFastForward) {
        interrupted = true;
	} else {
		interrupted = pauseForMilliseconds(milliseconds);
	}
	pausingTimerStartsNow();
	return interrupted;
}

void nextBrogueEvent(rogueEvent *returnEvent, boolean textInput, boolean colorsDance, boolean realInputEvenInPlayback) {
	rogueEvent recordingInput;
	boolean repeatAgain;
	short pauseDuration;
	
	returnEvent->eventType = EVENT_ERROR;
	
	if (rogue.playbackMode && !realInputEvenInPlayback) {
		do {
			repeatAgain = false;
			if ((!rogue.playbackFastForward && rogue.playbackBetweenTurns)
				|| rogue.playbackOOS) {
				
				pauseDuration = (rogue.playbackPaused ? DEFAULT_PLAYBACK_DELAY : rogue.playbackDelayThisTurn);
				if (pauseDuration && pauseBrogue(pauseDuration)) {
					// if the player did something during playback
					nextBrogueEvent(&recordingInput, false, false, true);
					executePlaybackInput(&recordingInput);
					repeatAgain = true;
				}
			}
		} while ((rogue.playbackPaused || repeatAgain || rogue.playbackOOS) && !rogue.gameHasEnded);
		rogue.playbackDelayThisTurn = rogue.playbackDelayPerTurn;
		recallEvent(returnEvent);
	} else {
		if (rogue.creaturesWillFlashThisTurn) {
			displayMonsterFlashes(true);
		}
		do {
			nextKeyOrMouseEvent(returnEvent, textInput, colorsDance); // No mouse clicks outside of the window will register.
		} while (returnEvent->eventType == MOUSE_UP && !coordinatesAreInWindow(returnEvent->param1, returnEvent->param2));
		// recording done elsewhere
	}
	
	pausingTimerStartsNow();
	
	if (returnEvent->eventType == EVENT_ERROR) {
		rogue.playbackPaused = rogue.playbackMode; // pause if replaying
		message("Event error!", true);
	}
}

void executeMouseClick(rogueEvent *theEvent) {
	short x, y;
	boolean autoConfirm;
	x = theEvent->param1;
	y = theEvent->param2;
	autoConfirm = theEvent->controlKey;
	
	if (theEvent->eventType == RIGHT_MOUSE_UP) {
		displayInventory(ALL_ITEMS, 0, 0, true, true);
	} else if (coordinatesAreInMap(windowToMapX(x), windowToMapY(y))) {
		if (autoConfirm) {
			travel(windowToMapX(x), windowToMapY(y), autoConfirm);
		} else {
			rogue.cursorLoc[0] = windowToMapX(x);
			rogue.cursorLoc[1] = windowToMapY(y);
			mainInputLoop();
		}
		
	} else if (windowToMapX(x) >= 0 && windowToMapX(x) < DCOLS && y >= 0 && y < MESSAGE_LINES) {
		// If the click location is in the message block, display the message archive.
		displayMessageArchive();
	}
}

void executeKeystroke(signed long keystroke, boolean controlKey, boolean shiftKey) {
	char path[BROGUE_FILENAME_MAX];
	short direction = -1;
	
	confirmMessages();
	stripShiftFromMovementKeystroke(&keystroke);
	
	switch (keystroke) {
		case UP_KEY:
		case UP_ARROW:
		case NUMPAD_8:
			direction = UP;
			break;
		case DOWN_KEY:
		case DOWN_ARROW:
		case NUMPAD_2:
			direction = DOWN;
			break;
		case LEFT_KEY:
		case LEFT_ARROW:
		case NUMPAD_4:
			direction = LEFT;
			break;
		case RIGHT_KEY:
		case RIGHT_ARROW:
		case NUMPAD_6:
			direction = RIGHT;
			break;
		case NUMPAD_7:
		case UPLEFT_KEY:
			direction = UPLEFT;
			break;
		case UPRIGHT_KEY:
		case NUMPAD_9:
			direction = UPRIGHT;
			break;
		case DOWNLEFT_KEY:
		case NUMPAD_1:
			direction = DOWNLEFT;
			break;
		case DOWNRIGHT_KEY:
		case NUMPAD_3:
			direction = DOWNRIGHT;
			break;
		case DESCEND_KEY:
			considerCautiousMode();
			if (D_WORMHOLING) {
				recordKeystroke(DESCEND_KEY, false, false);
				useStairs(1);
			} else {
				routeTo(rogue.downLoc[0], rogue.downLoc[1], "我还没找到往下一层的路。");
			}
			//refreshSideBar(-1, -1, false);
			break;
		case ASCEND_KEY:
			considerCautiousMode();
			if (D_WORMHOLING) {
				recordKeystroke(ASCEND_KEY, false, false);
				useStairs(-1);
			} else {
				routeTo(rogue.upLoc[0], rogue.upLoc[1], "我还没找到往上一层的路。");
			}
			//refreshSideBar(-1, -1, false);
			break;
		case REST_KEY:
		case PERIOD_KEY:
		case NUMPAD_5:
			considerCautiousMode();
			rogue.justRested = true;
			recordKeystroke(REST_KEY, false, false);
			playerTurnEnded();
			//refreshSideBar(-1, -1, false);
			break;
		case AUTO_REST_KEY:
			rogue.justRested = true;
			autoRest();
			break;
		case SEARCH_KEY:
			recordKeystroke(SEARCH_KEY, false, false);
			considerCautiousMode();
			search(rogue.awarenessBonus < 0 ? 40 : 80);
			playerTurnEnded();
			//refreshSideBar(-1, -1, false);
			break;
		case INVENTORY_KEY:
			displayInventory(ALL_ITEMS, 0, 0, true, true);
			break;
		case EQUIP_KEY:
			equip(NULL);
			//refreshSideBar(-1, -1, false);
			break;
		case UNEQUIP_KEY:
			unequip(NULL);
			//refreshSideBar(-1, -1, false);
			break;
		case DROP_KEY:
			drop(NULL);
			//refreshSideBar(-1, -1, false);
			break;
		case APPLY_KEY:
			apply(NULL, true);
			//refreshSideBar(-1, -1, false);
			break;
		case THROW_KEY:
			throwCommand(NULL);
			//refreshSideBar(-1, -1, false);
			break;
		case TRUE_COLORS_KEY:
			rogue.trueColorMode = !rogue.trueColorMode;
			displayLevel();
			refreshSideBar(-1, -1, false);
			if (rogue.trueColorMode) {
				messageWithColor("已禁用颜色特效。 再次按下 <\\> 键重新启用。", &teal, false);
			} else {
				messageWithColor("已启用颜色特效。 再次按下 <\\> 键重新禁用。", &teal, false);
			}
			break;
//		case FIGHT_KEY:
//			autoFight(false);
//			refreshSideBar(-1, -1, false);
//			break;
//		case FIGHT_TO_DEATH_KEY:
//			autoFight(true);
//			refreshSideBar(-1, -1, false);
//			break;
		case CALL_KEY:
			call(NULL);
			break;
		case EXPLORE_KEY:
			considerCautiousMode();
			explore(controlKey ? 1 : 20);
			break;
		case AUTOPLAY_KEY:
			autoPlayLevel(controlKey);
			break;
		case MESSAGE_ARCHIVE_KEY:
			displayMessageArchive();
			break;
		case HELP_KEY:
			printHelpScreen();
			break;
		case DISCOVERIES_KEY:
			printDiscoveriesScreen();
			break;
		case VIEW_RECORDING_KEY:
			if (rogue.playbackMode) {
				return;
			}
			confirmMessages();
			if ((rogue.playerTurnNumber < 50 || confirm("是否终止当前游戏并播放录像？", false))
				&& dialogChooseFile(path, RECORDING_SUFFIX, "请输入录像文件名字: ")) {
				if (fileExists(path)) {
					strcpy(rogue.nextGamePath, path);
					rogue.nextGame = NG_VIEW_RECORDING;
					rogue.gameHasEnded = true;
				} else {
					message("无法找到该录像文件。", false);
				}
			}
			break;
		case LOAD_SAVED_GAME_KEY:
			if (rogue.playbackMode) {
				return;
			}
			confirmMessages();
			if ((rogue.playerTurnNumber < 50 || confirm("是否终止当前游戏并载入另外的游戏存档？", false))
				&& dialogChooseFile(path, GAME_SUFFIX, "请输入存档文件名字: ")) {
				if (fileExists(path)) {
					strcpy(rogue.nextGamePath, path);
					rogue.nextGame = NG_OPEN_GAME;
					rogue.gameHasEnded = true;
				} else {
					message("无法找到该存档文件。", false);
				}
			}
			break;
		case SAVE_GAME_KEY:
			if (rogue.playbackMode) {
				return;
			}
			if (confirm("中断当前游戏并存档？(存档功能并不完善，有可能会丢失进度。)", false)) {
				saveGame();
			}
			break;
		case NEW_GAME_KEY:
			if (rogue.playerTurnNumber < 50 || confirm("是否终止当前游戏并重新开始新游戏？", false)) {
				rogue.nextGame = NG_NEW_GAME;
				rogue.gameHasEnded = true;
			}
			break;
		case QUIT_KEY:
			if (confirm("是否强制终止当前游戏？本次游戏的进度将丢失。", false)) {
				recordKeystroke(QUIT_KEY, false, false);
				rogue.quit = true;
				gameOver("主动退出游戏。", true);
			}
			break;
		case SEED_KEY:
			printSeed();
			break;
		case EASY_MODE_KEY:
			if (shiftKey) {
				enableEasyMode();
			}
			break;
		default:
			break;
	}
	if (direction >= 0) { // if it was a movement command
		considerCautiousMode();
		if (controlKey || shiftKey) {
			playerRuns(direction);
		} else {
			playerMoves(direction);
		}
		refreshSideBar(-1, -1, false);
	}
    
    if (D_EMPHASIZE_LIGHTING_LEVELS || D_SCENT_VISION) {
        displayLevel();
    }
	
	rogue.cautiousMode = false;
}

boolean getInputTextString(char *inputText,
						   const char *prompt,
						   short maxLength,
						   const char *defaultEntry,
						   const char *promptSuffix,
						   short textEntryType,
						   boolean useDialogBox) {
	short charNum, i, x, y, pos;
	rogueEvent event;
	char keystroke, suffix[100];
	int cursorBlink = 0;
	const short textEntryBounds[TEXT_INPUT_TYPES][2] = {{' ', '~'}, {'0', '9'}};
	BROGUE_WINDOW *window = NULL, *inner;
	BROGUE_DRAW_CONTEXT *context, *window_context;

	// x and y mark the origin for text entry.
	if (useDialogBox) {
		int width = maxLength + 4;
		if (u8_strlen(prompt) + 4 > width) {
			width = u8_strlen(prompt) + 4;
		}
		
		window = BrogueWindow_open(
			io_state.root, (COLS - width) / 2, ROWS / 2 - 2, 
			width, 4);
		BrogueWindow_setColor(window, windowColor);
		window_context = BrogueDrawContext_open(window);
		BrogueDrawContext_enableProportionalFont(window_context, 1);

		BrogueDrawContext_drawAsciiString(window_context, 1, 1, prompt);

		inner = BrogueWindow_open(
			window, 1, 2, width - 2, 1);
		BrogueWindow_setColor(inner, inputFieldColor);
		context = BrogueDrawContext_open(inner);
		BrogueDrawContext_enableProportionalFont(context, 1);
		
		x = 1;
		y = 0;
	} else {
		x = 0;
		y = MESSAGE_LINES - 1;

		message("", false);
		context = io_state.message_context;
	}

	
	maxLength = min(maxLength, COLS - x);	
	
	strcpy(inputText, defaultEntry);
	charNum = strLenWithoutEscapes(inputText);
	for (i = charNum; i < maxLength; i++) {
		inputText[i] = ' ';
	}
	
	if (promptSuffix[0] == '\0') { // empty suffix
		strcpy(suffix, " "); // so that deleting doesn't leave a white trail
	} else {
		strcpy(suffix, promptSuffix);
	}
	
	do {
		inputText[charNum] = 0;

		wchar_t *str = malloc(sizeof(wchar_t) * 
						   (strlen(prompt) + strlen(suffix) + charNum + 2));

		pos = 0;
		if (!useDialogBox) {
			// for (i = 0; prompt[i]; i++) {
			// 	str[pos++] = prompt[i];
			// }
			pos = T_unpack(prompt, str, u8_strlen(prompt));
		}
		for (i = 0; inputText[i]; i++) {
			str[pos++] = inputText[i];
		}
		/*  Use vertical bar as cursor character  */
		if (cursorBlink)
		{
			str[pos++] = ' ';
		}
		else
		{
			str[pos++] = '|';
		}
		for (i = 0; suffix[i]; i++) {
			str[pos++] = suffix[i];
		}
		str[pos] = 0;

		BrogueDrawContext_setColorMultiplier(
			context, colorForDisplay(white), 1);
		BrogueDrawContext_setForeground(context, colorForDisplay(white));
		BrogueDrawContext_drawString(context, x, y, str);

		keystroke = 0;
		if (pauseBrogue(500)) {
			nextBrogueEvent(&event, true, false, false);
			if (event.eventType == KEYSTROKE) {
				keystroke = event.param1;
			}

			cursorBlink = 0;
		}
		else
		{
			cursorBlink = !cursorBlink;
		}

		if (keystroke == DELETE_KEY && charNum > 0) {
			charNum--;
			inputText[charNum] = ' ';
		} else if (keystroke >= textEntryBounds[textEntryType][0]
				   && keystroke <= textEntryBounds[textEntryType][1]) { // allow only permitted input
			
			inputText[charNum] = keystroke;
			if (charNum < maxLength) {
				charNum++;
			}
		}
	} while (keystroke != RETURN_KEY && keystroke != ESCAPE_KEY 
			 && keystroke != ENTER_KEY);
	
	inputText[charNum] = '\0';

	if (window != NULL) {
		BrogueWindow_close(window);
	}
	
	if (keystroke == ESCAPE_KEY) {
		return false;
	}

	return true;
}

BROGUE_WINDOW *openAlert(
	char *message, short x, short y, color *fColor, color *bColor) {

	BROGUE_WINDOW *window;
	BROGUE_DRAW_CONTEXT *context;
	BROGUE_TEXT_SIZE size;

	window = BrogueWindow_open(io_state.root, x, y, 1, 1);
	BrogueWindow_setColor(window, colorForDisplay(*bColor));
	context = BrogueDrawContext_open(window);
	BrogueDrawContext_enableProportionalFont(context, 1);
	
	size = BrogueDrawContext_measureAsciiString(context, 0, message);
	BrogueWindow_reposition(window, x - size.width / 2 - 1, y, 
							size.width + 2, 1);
	BrogueDrawContext_setForeground(context, colorForDisplay(*fColor));
	BrogueDrawContext_drawAsciiString(context, 1, 0, message);

	return window;
}

void flashMessage(
	char *message, short x, short y, int time, color *fColor, color *bColor) {

	BROGUE_WINDOW *window;
	int i;

	window = openAlert(message, x, y, fColor, bColor);
	for (i = 0; i < time; i++) {
		if (pauseBrogue(1)) {
			break;
		}
	}

	BrogueWindow_close(window);
}

/*  Center an alert message which persists until the dungeon is next 
	redrawn  */
void displayCenteredAlert(char *message) {
	if (io_state.alert_window != NULL) {
		BrogueWindow_close(io_state.alert_window);
	}
	io_state.alert_window = openAlert(
		message, COLS / 2, ROWS / 2, &white, &darkBlue);
}

void flashTemporaryAlert(char *message, int time) {
	flashMessage(message, COLS / 2, ROWS / 2, time, &white, &darkRed);
}

void waitForAcknowledgment() {
	rogueEvent theEvent;
	
	if (rogue.autoPlayingLevel || (rogue.playbackMode && !rogue.playbackOOS)) {
		return;
	}
	
	do {
		nextBrogueEvent(&theEvent, false, false, false);
		if (theEvent.eventType == KEYSTROKE && theEvent.param1 != ACKNOWLEDGE_KEY && theEvent.param1 != ESCAPE_KEY) {
			flashTemporaryAlert(" -- 按空格或点击 -- ", 500);
		}
	} while (!(theEvent.eventType == KEYSTROKE && (theEvent.param1 == ACKNOWLEDGE_KEY || theEvent.param1 == ESCAPE_KEY)
			   || theEvent.eventType == MOUSE_UP));
}

void waitForKeystrokeOrMouseClick() {
	rogueEvent theEvent;
	do {
		nextBrogueEvent(&theEvent, false, false, false);
	} while (theEvent.eventType != KEYSTROKE && theEvent.eventType != MOUSE_UP);
}

boolean confirm(char *prompt, boolean alsoDuringPlayback) {
	short retVal;
	brogueButton buttons[2] = {{{0}}};
	char whiteColorEscape[20] = "";
	char yellowColorEscape[20] = "";
	
	if (rogue.autoPlayingLevel || (!alsoDuringPlayback && rogue.playbackMode)) {
		return true; // oh yes he did
	}
	
	encodeMessageColor(whiteColorEscape, 0, &white);
	encodeMessageColor(yellowColorEscape, 0, &yellow);
	
	initializeButton(&(buttons[0]));
	sprintf(buttons[0].text, "     %sY%ses     ", yellowColorEscape, whiteColorEscape);
	buttons[0].hotkey[0] = 'y';
	buttons[0].hotkey[1] = 'Y';
	buttons[0].hotkey[2] = RETURN_KEY;
	buttons[0].hotkey[2] = ENTER_KEY;
	buttons[0].flags |= (B_WIDE_CLICK_AREA | B_KEYPRESS_HIGHLIGHT);
	
	initializeButton(&(buttons[1]));
	sprintf(buttons[1].text, "     %sN%so      ", yellowColorEscape, whiteColorEscape);
	buttons[1].hotkey[0] = 'n';
	buttons[1].hotkey[1] = 'N';
	buttons[1].hotkey[2] = ACKNOWLEDGE_KEY;
	buttons[1].hotkey[3] = ESCAPE_KEY;
	buttons[1].flags |= (B_WIDE_CLICK_AREA | B_KEYPRESS_HIGHLIGHT);
	
	retVal = printTextBox(
		prompt, COLS/3, &white, &interfaceBoxColor, buttons, 2);
	
	if (retVal == -1 || retVal == 1) { // If they canceled or pressed no.
		return false;
	} else {
		return true;
	}

	confirmMessages();
	return retVal;
}

void clearMonsterFlashes() {
    
}

void displayMonsterFlashes(boolean flashingEnabled) {
	creature *monst;
	short x[100], y[100], strength[100], count = 0;
	color *flashColor[100];
	
	rogue.creaturesWillFlashThisTurn = false;
	
	if (rogue.autoPlayingLevel || rogue.blockCombatText) {
		return;
	}
	
	assureCosmeticRNG;
	
	CYCLE_MONSTERS_AND_PLAYERS(monst) {
		if (monst->bookkeepingFlags & MONST_WILL_FLASH) {
			monst->bookkeepingFlags &= ~MONST_WILL_FLASH;
			if (flashingEnabled && canSeeMonster(monst) && count < 100) {
				x[count] = monst->xLoc;
				y[count] = monst->yLoc;
				strength[count] = monst->flashStrength;
				flashColor[count] = &(monst->flashColor);
				count++;
			}
		}
	}
	flashForeground(x, y, flashColor, strength, count, 250);
	restoreRNG;
}

void dequeueEvent() {
	rogueEvent returnEvent;
	nextBrogueEvent(&returnEvent, false, false, true);
}

void displayMessageArchive() {
	BROGUE_WINDOW *root, *window;
	BROGUE_DRAW_CONTEXT *context;
	int i;
	int lines = 0;

	for (i = 0; i < MESSAGE_ARCHIVE_LINES; i++)
	{
		if (strlen(messageArchive[i]) > 0)
		{
			lines++;
		}
	}

	if (lines > MESSAGE_LINES)
	{
		root = BrogueDisplay_getRootWindow(io_state.display);
		window = BrogueWindow_open(
			root, STAT_BAR_WIDTH, 0, DCOLS, ROWS);
		BrogueWindow_setColor(window, windowColor);
		context = BrogueDrawContext_open(window);
		BrogueDrawContext_enableProportionalFont(context, 1);

		for (i = 1; i < ROWS - 2; i++)
		{
			int ix = (messageArchivePosition - (ROWS - 2 - i) 
					  + MESSAGE_ARCHIVE_LINES) % MESSAGE_ARCHIVE_LINES;
			
			BrogueDrawContext_drawAsciiString(
				context, 1, i, messageArchive[ix]);
		}

		BrogueDrawContext_setForeground(
			context, colorForDisplay(itemMessageColor));
		BrogueDrawContext_enableJustify(
			context, 0, DCOLS, BROGUE_JUSTIFY_CENTER);
		BrogueDrawContext_drawAsciiString(
			context, 0, ROWS - 1,
			"-- 按任意键继续 --");

		waitForAcknowledgment();

		BrogueDrawContext_close(context);
		BrogueWindow_close(window);	
	}

}

// Clears the message area and prints the given message in the area.
// It will disappear when messages are refreshed and will not be archived.
// This is primarily used to display prompts.
void temporaryMessage(char *msg, boolean requireAcknowledgment) {
	char message[COLS];
	short i;
	
	if (rogue.playbackFastForward) {
		return;
	}

	assureCosmeticRNG;
	strcpy(message, msg);
	
	for (i=0; message[i] == COLOR_ESCAPE; i += 4) {
		upperCase(&(message[i]));
	}
	
	refreshSideBar(-1, -1, false);

	BrogueWindow_clear(io_state.message_window);

	BrogueDrawContext_drawAsciiString(
		io_state.message_context, 0, MESSAGE_LINES - 1, message);

	if (requireAcknowledgment) {
		waitForAcknowledgment();
		updateMessageDisplay();
	}
	restoreRNG;
}

void messageWithColor(char *msg, color *theColor, boolean requireAcknowledgment) {
	char buf[COLS*2] = "";
	short i;
	
	i=0;
	i = encodeMessageColor(buf, i, theColor);
	strcpy(&(buf[i]), msg);
	message(buf, requireAcknowledgment);
}

void flavorMessage(char *msg) {
	short i;
	char text[COLS*20];
	
	if (rogue.playbackFastForward) {
		return;
	}

	for (i=0; i < COLS*2 && msg[i] != '\0'; i++) {
		text[i] = msg[i];
	}
	text[i] = '\0';
	
	for(i=0; text[i] == COLOR_ESCAPE; i+=4);
	upperCase(&(text[i]));
	
	BrogueWindow_clear(io_state.flavor_window);
	BrogueDrawContext_setForeground(
		io_state.flavor_context, colorForDisplay(flavorTextColor));
	BrogueDrawContext_drawAsciiString(io_state.flavor_context, 0, 0, text);
}

void messageWithoutCaps(char *msg, boolean requireAcknowledgment) {
	short i;

#ifdef BROGUE_ASSERTS
	assert(msg[0]);
#endif
	
	// need to confirm the oldest message? (Disabled!)
	/*if (!messageConfirmed[MESSAGE_LINES - 1]) {
		//refreshSideBar(-1, -1, false);
		displayMoreSign();
		for (i=0; i<MESSAGE_LINES; i++) {
			messageConfirmed[i] = true;
		}
	}*/
	
	for (i = MESSAGE_LINES - 1; i >= 1; i--) {
		messageConfirmed[i] = messageConfirmed[i-1];
		strcpy(displayedMessage[i], displayedMessage[i-1]);
	}
	messageConfirmed[0] = false;
	strcpy(displayedMessage[0], msg);
	
	// Add the message to the archive.
	strcpy(messageArchive[messageArchivePosition], msg);
	messageArchivePosition = (messageArchivePosition + 1) % MESSAGE_ARCHIVE_LINES;
	
	// display the message:
	updateMessageDisplay();
	
	if (requireAcknowledgment || rogue.cautiousMode) {
		displayMoreSign();
		confirmMessages();
		rogue.cautiousMode = false;
	}
	
	if (rogue.playbackMode) {
		rogue.playbackDelayThisTurn += rogue.playbackDelayPerTurn * 5;
	}
}


void message(const char *msg, boolean requireAcknowledgment) {
	char text[COLS*20], *msgPtr;
	short i, lines;
	
	assureCosmeticRNG;
	
	rogue.disturbed = true;
	if (requireAcknowledgment) {
		refreshSideBar(-1, -1, false);
	}
	displayCombatText();
	
	lines = wrapText(text, msg, DCOLS);
	msgPtr = &(text[0]);
	
	// for(i=0; text[i] == COLOR_ESCAPE; i+=4);
	// upperCase(&(text[i]));
	
	if (lines > 1) {
		for (i=0; text[i] != '\0'; i++) {
			if (text[i] == '\n') {
				text[i] = '\0';
				
				messageWithoutCaps(msgPtr, false);
				msgPtr = &(text[i+1]);
			}
		}
	}
	
	messageWithoutCaps(msgPtr, requireAcknowledgment);
	restoreRNG;
}

// Only used for the "you die..." message, to enable posthumous inventory viewing.
void displayMoreSignWithoutWaitingForAcknowledgment() {
	BrogueDrawContext_setForeground(
		io_state.message_context, colorForDisplay(white));
	BrogueDrawContext_drawAsciiString(
		io_state.message_context, DCOLS - 8, MESSAGE_LINES - 1, "--继续--");
}

void displayMoreSign() {
	BrogueDrawContext_setForeground(
		io_state.message_context, colorForDisplay(white));
	BrogueDrawContext_drawAsciiString(
		io_state.message_context, DCOLS - 8, MESSAGE_LINES - 1, "--继续--");
	waitForAcknowledgment();
	BrogueDrawContext_drawAsciiString(
		io_state.message_context, DCOLS - 8, MESSAGE_LINES - 1, "");
}

/*  Center an after-game result message on an otherwise cleared screen  */
void centerResultMessage(char *msg) {
	blackOutScreen();

	BrogueDrawContext_push(io_state.root_context);
	BrogueDrawContext_enableJustify(
		io_state.root_context, 0, COLS, BROGUE_JUSTIFY_CENTER);
	BrogueDrawContext_enableProportionalFont(io_state.root_context, 1);
	BrogueDrawContext_drawAsciiString(io_state.root_context, 0, ROWS / 2, msg);
	BrogueDrawContext_pop(io_state.root_context);
}

// Inserts a four-character color escape sequence into a string at the insertion point.
// Does NOT check string lengths, so it could theoretically write over the null terminator.
// Returns the new insertion point.
short encodeMessageColor(char *msg, short i, const color *theColor) {
	boolean needTerminator = false;
	color col = *theColor;
	
	assureCosmeticRNG;
	
	bakeColor(&col);
	
	col.red		= clamp(col.red, 0, 100);
	col.green	= clamp(col.green, 0, 100);
	col.blue	= clamp(col.blue, 0, 100);
	
	needTerminator = !msg[i] || !msg[i + 1] || !msg[i + 2] || !msg[i + 3];
	
	msg[i++] = COLOR_ESCAPE;
	msg[i++] = (char) (COLOR_VALUE_INTERCEPT + col.red);
	msg[i++] = (char) (COLOR_VALUE_INTERCEPT + col.green);
	msg[i++] = (char) (COLOR_VALUE_INTERCEPT + col.blue);
	
	if (needTerminator) {
		msg[i] = '\0';
	}
	
	restoreRNG;
	
	return i;
}

// Call this when the i'th character of msg is COLOR_ESCAPE.
// It will return the encoded color, and will advance i past the color escape sequence.
short decodeMessageColor(const char *msg, short i, color *returnColor) {
	
	if (msg[i] != COLOR_ESCAPE) {
		printf("\nAsked to decode a color escape that didn't exist!");
		*returnColor = white;
	} else {
		i++;
		*returnColor = black;
		returnColor->red	= (short) (msg[i++] - COLOR_VALUE_INTERCEPT);
		returnColor->green	= (short) (msg[i++] - COLOR_VALUE_INTERCEPT);
		returnColor->blue	= (short) (msg[i++] - COLOR_VALUE_INTERCEPT);
		
		returnColor->red	= clamp(returnColor->red, 0, 100);
		returnColor->green	= clamp(returnColor->green, 0, 100);
		returnColor->blue	= clamp(returnColor->blue, 0, 100);
	}
	return i;
}

// Returns a color for combat text based on the identity of the victim.
color *messageColorFromVictim(creature *monst) {
	if (monstersAreEnemies(&player, monst)) {
		return &goodMessageColor;
	} else if (monst == &player || monst->creatureState == MONSTER_ALLY) {
		return &badMessageColor;
	} else {
		return &white;
	}
}

void updateMessageDisplay() {
	short i;
	color messageColor;

	if (rogue.playbackFastForward) {
		return;
	}

	BrogueWindow_clear(io_state.message_window);
	BrogueDrawContext_push(io_state.message_context);

	for (i=0; i<MESSAGE_LINES; i++) {
		char *substr = malloc(
			sizeof(char) * (strlen(displayedMessage[i]) + 1));
		if (substr == NULL)
		{
			continue;
		}

		messageColor = white;		
		if (messageConfirmed[i]) {
			applyColorAverage(&messageColor, &black, 50);
			applyColorAverage(&messageColor, &black, 75 * i / MESSAGE_LINES);
		}

		BrogueDrawContext_setColorMultiplier(
			io_state.message_context, colorForDisplay(messageColor), 1);
		BrogueDrawContext_setForeground(
			io_state.message_context, colorForDisplay(white));
		
		BrogueDrawContext_drawAsciiString(
			io_state.message_context, 0, MESSAGE_LINES - i - 1,
			displayedMessage[i]);										  
	}
	BrogueDrawContext_pop(io_state.message_context);
}

// Does NOT clear the message archive.
void deleteMessages() {
	short i;
	for (i=0; i<MESSAGE_LINES; i++) {
		displayedMessage[i][0] = '\0';
	}
	confirmMessages();
}

void confirmMessages() {
	short i;
	for (i=0; i<MESSAGE_LINES; i++) {
		messageConfirmed[i] = true;
	}
	updateMessageDisplay();
}

void stripShiftFromMovementKeystroke(signed long *keystroke) {
	const unsigned short newKey = *keystroke - ('A' - 'a');
	if (newKey == LEFT_KEY
		|| newKey == RIGHT_KEY
		|| newKey == DOWN_KEY
		|| newKey == UP_KEY
		|| newKey == UPLEFT_KEY
		|| newKey == UPRIGHT_KEY
		|| newKey == DOWNLEFT_KEY
		|| newKey == DOWNRIGHT_KEY) {
		*keystroke -= 'A' - 'a';
	}
}

void upperCase(char *theChar) {
	if (*theChar >= 'a' && *theChar <= 'z') {
		(*theChar) += ('A' - 'a');
	}
}

enum entityDisplayTypes {
	EDT_NOTHING = 0,
	EDT_CREATURE,
	EDT_ITEM,
    EDT_TERRAIN,
};

// Refreshes the sidebar.
// Progresses from the closest visible monster to the farthest.
// If a monster, item or terrain is focused, then display the sidebar with that monster/item highlighted,
// in the order it would normally appear. If it would normally not fit on the sidebar at all,
// then list it first.
// Also update rogue.sidebarLocationList[ROWS][2] list of locations so that each row of
// the screen is mapped to the corresponding entity, if any.
// FocusedEntityMustGoFirst should usually be false when called externally. This is because
// we won't know if it will fit on the screen in normal order until we try.
// So if we try and fail, this function will call itself again, but with this set to true.
void refreshSideBar(short focusX, short focusY, boolean focusedEntityMustGoFirst) {
	short printY, oldPrintY, shortestDistance, i, j, k, px, py, x, y, displayEntityCount;
	creature *monst, *closestMonst;
	item *theItem, *closestItem;
	char buf[COLS];
	void *entityList[ROWS] = {0}, *focusEntity = NULL;
	enum entityDisplayTypes entityType[ROWS] = {0}, focusEntityType = EDT_NOTHING;
    short terrainLocationMap[ROWS][2];
	boolean gotFocusedEntityOnScreen = (focusX >= 0 ? false : true);
	char addedEntity[DCOLS][DROWS];
	
	if (rogue.gameHasEnded || rogue.playbackFastForward) {
		return;
	}
	
	assureCosmeticRNG;
	
	if (focusX < 0) {
		focusedEntityMustGoFirst = false; // just in case!
	} else {
		if (pmap[focusX][focusY].flags & (HAS_MONSTER | HAS_PLAYER)) {
			monst = monsterAtLoc(focusX, focusY);
			if (canSeeMonster(monst) || rogue.playbackOmniscience) {
				focusEntity = monst;
				focusEntityType = EDT_CREATURE;
			}
		}
		if (!focusEntity && (pmap[focusX][focusY].flags & HAS_ITEM)) {
			theItem = itemAtLoc(focusX, focusY);
			if (playerCanSeeOrSense(focusX, focusY)) {
				focusEntity = theItem;
				focusEntityType = EDT_ITEM;
			}
		}
        if (!focusEntity
            && cellHasTMFlag(focusX, focusY, TM_LIST_IN_SIDEBAR)
            && playerCanSeeOrSense(focusX, focusY)) {
            
            focusEntity = tileCatalog[pmap[focusX][focusY].layers[layerWithTMFlag(focusX, focusY, TM_LIST_IN_SIDEBAR)]].description;
            focusEntityType = EDT_TERRAIN;
        }
	}
	
	printY = 0;
	
	px = player.xLoc;
	py = player.yLoc;
	
	zeroOutGrid(addedEntity);
	BrogueWindow_clear(io_state.sidebar_window);
	
	// Header information for playback mode.
	if (rogue.playbackMode) {
		BrogueDrawContext_push(io_state.sidebar_context);
		BrogueDrawContext_enableJustify(
			io_state.sidebar_context, 0, STAT_BAR_WIDTH, 
			BROGUE_JUSTIFY_CENTER);
		BrogueDrawContext_enableProportionalFont(io_state.sidebar_context, 1);
		BrogueDrawContext_drawAsciiString(
			io_state.sidebar_context, 0, printY++, "   -- PLAYBACK --   ");

		if (rogue.howManyTurns > 0) {
			sprintf(buf, "Turn %li/%li", rogue.playerTurnNumber, rogue.howManyTurns);
			printProgressBar(0, printY++, buf, rogue.playerTurnNumber, rogue.howManyTurns, &darkPurple, false);
		}
		if (rogue.playbackOOS) {
			BrogueDrawContext_setForeground(io_state.sidebar_context,
										    colorForDisplay(badMessageColor));
			BrogueDrawContext_drawAsciiString(
				io_state.sidebar_context, 0, printY++, "    [OUT OF SYNC]   ");
		} else if (rogue.playbackPaused) {
			BrogueDrawContext_setForeground(io_state.sidebar_context,
										    colorForDisplay(gray));
			BrogueDrawContext_drawAsciiString(
				io_state.sidebar_context, 0, printY++, "      [PAUSED]      ");
		}
		BrogueDrawContext_pop(io_state.sidebar_context);
	}
	
	// Now list the monsters that we'll be displaying in the order of their proximity to player (listing the focused first if required).
	
	// Initialization.
	displayEntityCount = 0;
	for (i=0; i<ROWS; i++) {
		rogue.sidebarLocationList[i][0] = -1;
		rogue.sidebarLocationList[i][1] = -1;
	}
	
	// Player always goes first.
	entityList[displayEntityCount] = &player;
	entityType[displayEntityCount] = EDT_CREATURE;
	displayEntityCount++;
	addedEntity[player.xLoc][player.yLoc] = true;
	
	// Focused entity, if it must go first.
	if (focusedEntityMustGoFirst && !addedEntity[focusX][focusY]) {
		addedEntity[focusX][focusY] = true;
		entityList[displayEntityCount] = focusEntity;
		entityType[displayEntityCount] = focusEntityType;
        terrainLocationMap[displayEntityCount][0] = focusX;
        terrainLocationMap[displayEntityCount][1] = focusY;
		displayEntityCount++;
	}
	
	// Non-focused monsters.
	do {
		shortestDistance = 10000;
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if ((canSeeMonster(monst) || rogue.playbackOmniscience)
				&& !addedEntity[monst->xLoc][monst->yLoc]
                && !(monst->info.flags & MONST_NOT_LISTED_IN_SIDEBAR)
				&& (px - monst->xLoc) * (px - monst->xLoc) + (py - monst->yLoc) * (py - monst->yLoc) < shortestDistance) {
				
				shortestDistance = (px - monst->xLoc) * (px - monst->xLoc) + (py - monst->yLoc) * (py - monst->yLoc);
				closestMonst = monst;
			}
		}
		if (shortestDistance < 10000) {
			addedEntity[closestMonst->xLoc][closestMonst->yLoc] = true;
			entityList[displayEntityCount] = closestMonst;
			entityType[displayEntityCount] = EDT_CREATURE;
			displayEntityCount++;
		}
	} while (shortestDistance < 10000 && displayEntityCount * 2 < ROWS); // Because each entity takes at least 2 rows in the sidebar.
	
	// Non-focused items.
	do {
		shortestDistance = 10000;
		for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
			if ((playerCanSeeOrSense(theItem->xLoc, theItem->yLoc) || rogue.playbackOmniscience)
				&& !addedEntity[theItem->xLoc][theItem->yLoc]
				&& (px - theItem->xLoc) * (px - theItem->xLoc) + (py - theItem->yLoc) * (py - theItem->yLoc) < shortestDistance) {
				
				shortestDistance = (px - theItem->xLoc) * (px - theItem->xLoc) + (py - theItem->yLoc) * (py - theItem->yLoc);
				closestItem = theItem;
			}
		}
		if (shortestDistance < 10000) {
			addedEntity[closestItem->xLoc][closestItem->yLoc] = true;
			entityList[displayEntityCount] = closestItem;
			entityType[displayEntityCount] = EDT_ITEM;
			displayEntityCount++;
		}
	} while (shortestDistance < 10000 && displayEntityCount * 2 < ROWS); // Because each entity takes at least 2 rows in the sidebar.
    
    // Non-focused terrain.
    
	// count up the number of candidate locations
	for (k=0; k<max(DROWS, DCOLS); k++) {
		for (i = px-k; i <= px+k; i++) {
			for (j = py-k; j <= py+k; j++) {
				if (coordinatesAreInMap(i, j)
					&& (i == px-k || i == px+k || j == py-k || j == py+k)
					&& !addedEntity[i][j]
                    && playerCanSeeOrSense(i, j)
                    && cellHasTMFlag(i, j, TM_LIST_IN_SIDEBAR)
                    && displayEntityCount < ROWS - 1) {
                    
                    addedEntity[i][j] = true;
                    entityList[displayEntityCount] = tileCatalog[pmap[i][j].layers[layerWithTMFlag(i, j, TM_LIST_IN_SIDEBAR)]].description;
                    entityType[displayEntityCount] = EDT_TERRAIN;
                    terrainLocationMap[displayEntityCount][0] = i;
                    terrainLocationMap[displayEntityCount][1] = j;
                    displayEntityCount++;
				}
			}
		}
	}
	
	// Entities are now listed. Start printing.
	
	for (i=0; i<displayEntityCount && printY < ROWS - 1; i++) { // Bottom line is reserved for the depth.
		oldPrintY = printY;
		BrogueDrawContext_push(io_state.sidebar_context);
		x = y = 0;
		if (entityType[i] == EDT_CREATURE) {
			x = ((creature *) entityList[i])->xLoc;
			y = ((creature *) entityList[i])->yLoc;
			printY = printMonsterInfo((creature *) entityList[i],
									  printY,
									  (focusEntity && (x != focusX || y != focusY)),
									  (x == focusX && y == focusY));
			
		} else if (entityType[i] == EDT_ITEM) {
			x = ((item *) entityList[i])->xLoc;
			y = ((item *) entityList[i])->yLoc;
			printY = printItemInfo((item *) entityList[i],
								   printY,
								   (focusEntity && (x != focusX || y != focusY)),
								   (x == focusX && y == focusY));
		} else if (entityType[i] == EDT_TERRAIN) {
            x = terrainLocationMap[i][0];
            y = terrainLocationMap[i][1];
            printY = printTerrainInfo(x, y,
                                      printY,
                                      ((const char *) entityList[i]),
                                      (focusEntity && (x != focusX || y != focusY)),
                                      (x == focusX && y == focusY));
        }
		if (focusEntity && (x == focusX && y == focusY) && printY < ROWS) {
			gotFocusedEntityOnScreen = true;
		}
		for (j=oldPrintY; j<printY; j++) {
			rogue.sidebarLocationList[j][0] = x;
			rogue.sidebarLocationList[j][1] = y;
		}
		BrogueDrawContext_pop(io_state.sidebar_context);
	}
	
	if (gotFocusedEntityOnScreen) {
		sprintf(buf, "  -- 第 %i 层 --%s   ", rogue.depthLevel, (rogue.depthLevel < 10 ? " " : ""));

		BrogueDrawContext_push(io_state.sidebar_context);
		BrogueDrawContext_enableJustify(
			io_state.sidebar_context, 0, STAT_BAR_WIDTH, 
			BROGUE_JUSTIFY_CENTER);
		BrogueDrawContext_enableProportionalFont(io_state.sidebar_context, 1);
		BrogueDrawContext_drawAsciiString(
			io_state.sidebar_context, 0, ROWS - 1, buf);
		BrogueDrawContext_pop(io_state.sidebar_context);
	} else if (!focusedEntityMustGoFirst) {
		// Failed to get the focusMonst printed on the screen. Try again, this time with the focus first.
		refreshSideBar(focusX, focusY, true);
	}
	
	restoreRNG;
}

// Inserts line breaks into really long words. Optionally adds a hyphen, but doesn't do anything
// clever regarding hyphen placement. Plays nicely with color escapes.
void breakUpLongWordsIn(char *sourceText, short width, boolean useHyphens) {
	char buf[COLS * ROWS * 2] = "";
	short i, m, nextChar, wordWidth, u8clen;
	//const short maxLength = useHyphens ? width - 1 : width;
	
	// i iterates over characters in sourceText; m keeps track of the length of buf.
	wordWidth = 0;
	for (i=0, m=0; sourceText[i] != 0;) {
		if (sourceText[i] == COLOR_ESCAPE) {
			strncpy(&(buf[m]), &(sourceText[i]), 4);
			i += 4;
			m += 4;
		} else if (sourceText[i] == ' ' || sourceText[i] == '\n') {
			wordWidth = 0;
			buf[m++] = sourceText[i++];
		} else {
			u8clen = u8_seqlen(sourceText + i);
			if (u8clen == 1) {
				if (!useHyphens && wordWidth >= width) {
					buf[m++] = '\n';
					wordWidth = 0;
				} else if (useHyphens && wordWidth >= width - 1) {
					nextChar = i+1;
					while (sourceText[nextChar] == COLOR_ESCAPE) {
						nextChar += 4;
					}
					if (sourceText[nextChar] && sourceText[nextChar] != ' ' && sourceText[nextChar] != '\n') {
						buf[m++] = '-';
						buf[m++] = '\n';
						wordWidth = 0;
					}
				}
				buf[m++] = sourceText[i++];
				wordWidth++;
			} else {
				// do not break up unicode characters
				while (u8clen--) {
					buf[m++] = sourceText[i++];
				}
			}
		}
	}
	buf[m] = '\0';
	strcpy(sourceText, buf);
}

// Returns the number of lines, including the newlines already in the text.
// Puts the output in "to" only if we receive a "to" -- can make it null and just get a line count.
short wrapText(char *to, const char *sourceText, short width) {
	short i, w, textLength, lineCount;
	char printString[COLS * ROWS * 10];
	short u8clen, writeCount, currentLineLength;
	
	// ! write when wraping
	textLength = strlen(sourceText); // do NOT discount escape sequences
	lineCount = 1;
	
	// ! Since now we have long unicode lines, replacing spaces is not fesible
	// ! so don't break on spaces now. just hard break anything longer than the width
	w = currentLineLength = 0;
	while (i < textLength) {
		if (sourceText[i] == COLOR_ESCAPE) {
			writeCount = 4;
		} else if (sourceText[i] == '\n') {
			// string itself contains a new line
			++lineCount;
			currentLineLength = 0;
			writeCount = 1;
		} else {
			u8clen = u8_seqlen(sourceText + i);
			if (u8clen == 1) {
				++currentLineLength;
			} else {
				currentLineLength += 2; // count unicode as 2 chars wide
			}
			writeCount = u8clen;

			// write an additional line break when too long
			if (currentLineLength + 1 > width) {
				printString[w++] = '\n';
				++lineCount;
				currentLineLength = 0;
			}
		}

		while (writeCount--) {
			printString[w++] = sourceText[i++];
		}
	}

	// end the string
	printString[w] = '\0';
	
	if (to) {
		strcpy(to, printString);
	}

	return lineCount;
}

char nextKeyPress(boolean textInput) {
	rogueEvent theEvent;
	do {
		nextBrogueEvent(&theEvent, textInput, false, false);
	} while (theEvent.eventType != KEYSTROKE);
	return theEvent.param1;
}

#define BROGUE_HELP_LINE_COUNT	32

void printHelpScreen() {
	char helpText[BROGUE_HELP_LINE_COUNT][DCOLS*3] = {
		"",
		"-- 帮助 --",
		"-- (大写字母代表需要按住 SHIFT ，比如 D 需要输入 SHIFT + D) --",
		"",
		"          鼠标  ****移动鼠标控制光标位置。悬浮在感兴趣的地方，可以查看相关信息",
		"      左键单击  ****移动到光标位置，若点击敌人则进行攻击",
		" CTRL+左键单击  ****向光标方向移动一格",
		"          回车  ****使用键盘来控制光标",
		"   空格或ESC键  ****禁用键盘控制光标",
		"<hjklyubn>, 方向键及小键盘  ****移动和攻击。按住 CTRL 或 SHIFT 朝方向进行连续移动",
		"",
		"   a/e/r/t/d/c  ****分别对应 使用/装备/卸下装备/投掷/丢弃/命名 物品的菜单",
		"   i, 右键单击  ****查看物品栏",
		"             D  ****显示当前游戏中已经鉴定过的物品列表",
        "",
		"             z  ****原地休息一回合",
		"             Z  ****原地休息100回合，或直到被打断",
		"             s  ****搜索附近的隐藏门和陷阱",
		"          <, >  ****移动到向上/向下的楼梯处",
		"             x  ****自动探索，自动控制角色探索未知地形，拾取物品，按任意键停止。(CTRL+x: 加快显示速度)",
		"             A  ****自动游戏，包括自动探索的功能，遇到怪物会一直进行攻击，按任意键停止。(CTRL+A: 加快显示速度)",
		"             M  ****显示所有信息列表",
        "",
		"             S  ****中断游戏，保存游戏进度并退出",
		"             O  ****读取存档",
		"             V  ****观看已保存的录像",
		"             Q  ****强制退出游戏",
        "",
		"             \\  ****关闭/开启颜色特效",
		"    空格或ESC键  ****确认信息",
		"",
		"-- 按任意键继续 --"
	};
	char *helpPtr[BROGUE_HELP_LINE_COUNT];
	int i;

	for (i = 0; i < BROGUE_HELP_LINE_COUNT; i++)
	{
		helpPtr[i] = (char *)&helpText[i];
	}

	helpDialog(helpPtr, BROGUE_HELP_LINE_COUNT);
}

/*  Draw a help dialog with text from either the in-game help screen 
	or the playback help screen  */
void helpDialog(char **helpText, int helpLineCount) {
	BROGUE_WINDOW *root, *window;
	BROGUE_DRAW_CONTEXT *context;
	int i;
	int tab_col = 16;

	root = BrogueDisplay_getRootWindow(io_state.display);
	window = BrogueWindow_open(
		root, STAT_BAR_WIDTH + 2, (ROWS - helpLineCount - 2) / 2, 
		DCOLS - 2, helpLineCount + 2);
	BrogueWindow_setColor(window, windowColor);
	context = BrogueDrawContext_open(window);
	BrogueDrawContext_enableProportionalFont(context, 1);

	for (i = 0; i < helpLineCount; i++)
	{
		BROGUE_TEXT_SIZE text_size;
		const char *line = helpText[i];
		const char *asterisks = strstr(line, "****");

		char *substr = malloc(strlen(line) + 1);

		if (asterisks)
		{
			memcpy(substr, line, asterisks - line);
			substr[asterisks - line] = 0;

			text_size = BrogueDrawContext_measureAsciiString(
				context, 2, substr);

			if (text_size.final_column < tab_col - 1)
			{
				BrogueDrawContext_push(context);

				BrogueDrawContext_setForeground(
					context, colorForDisplay(itemMessageColor));
				BrogueDrawContext_enableJustify(
					context, 2, tab_col, BROGUE_JUSTIFY_RIGHT);
				BrogueDrawContext_drawAsciiString(context, 2, i, substr);
				
				BrogueDrawContext_pop(context);

				BrogueDrawContext_setForeground(
					context, colorForDisplay(white));
				BrogueDrawContext_drawAsciiString(
  					context, tab_col, i, asterisks + 4);
			}
			else
			{
				BrogueDrawContext_setForeground(
					context, colorForDisplay(itemMessageColor));
				BrogueDrawContext_drawAsciiString(context, 2, i, substr);

				BrogueDrawContext_setForeground(
					context, colorForDisplay(white));
				BrogueDrawContext_drawAsciiString(
					context, text_size.final_column, i, asterisks + 4);
			}
		}
		else
		{
			BrogueDrawContext_push(context);

			BrogueDrawContext_setForeground(
				context, colorForDisplay(itemMessageColor));
			BrogueDrawContext_enableJustify(
				context, 0, DCOLS - 2, BROGUE_JUSTIFY_CENTER);
			BrogueDrawContext_drawAsciiString(context, 2, i, line);

			BrogueDrawContext_pop(context);
		}

		free(substr);
	}

	waitForKeystrokeOrMouseClick();

	BrogueDrawContext_close(context);
	BrogueWindow_close(window);
}

/*  Draw the text for an individual item in the discoveries screen  */
void printDiscoveries(BROGUE_DRAW_CONTEXT *context, 
					  short category, short count, wchar_t glyph, 
					  int x, int y) {
	color goodColor, badColor;
	itemTable *table = tableForItemCategory(category);
	int i, magic, magic_x;

	goodColor = goodMessageColor;
	applyColorAverage(&goodColor, &black, 50);
	badColor = badMessageColor;
	applyColorAverage(&badColor, &black, 50);

	for (i = 0; i < count; i++)
	{
		magic = magicCharDiscoverySuffix(category, i);

		if (table[i].identified)
		{
			int icon_x = x;
			if (magic != 0)
			{
				icon_x++;
			}

			BrogueDrawContext_setForeground(
				context, colorForDisplay(itemColor));
			BrogueDrawContext_enableTiles(context, 1);
			BrogueDrawContext_drawChar(context, icon_x, y + i, glyph);
			BrogueDrawContext_enableTiles(context, 0);

			BrogueDrawContext_setForeground(
				context, colorForDisplay(white));
		}
		else
		{
			BrogueDrawContext_setForeground(
				context, colorForDisplay(darkGray));
		}

		char *name = malloc(strlen(table[i].name) + 1);
		strcpy(name, table[i].name);
		upperCase(name);

		BrogueDrawContext_drawAsciiString(context, x + 5, y + i, name);
		free(name);

		magic_x = x + 2;
		if (magic != 0)
		{
			magic_x++;
		}
		
		BrogueDrawContext_push(context);
		BrogueDrawContext_enableTiles(context, 1);

		if (magic != -1)
		{
			BrogueDrawContext_setForeground(
				context, colorForDisplay(goodColor));
			BrogueDrawContext_drawChar(context, magic_x++, y + i, 
									   GOOD_MAGIC_CHAR);
		}
		if (magic != 1)
		{
			BrogueDrawContext_setForeground(
				context, colorForDisplay(badColor));
			BrogueDrawContext_drawChar(context, magic_x++, y + i, 
									   BAD_MAGIC_CHAR);
		}

		BrogueDrawContext_pop(context);
	}
}

/*  Catalog the discovered and undiscovered items  */
void printDiscoveriesScreen(void) {
	BROGUE_WINDOW *root, *window;
	BROGUE_DRAW_CONTEXT *context;
	int col_width = (DCOLS - 4) / 3;
	int x, y;
	int h = 7 + NUMBER_STAFF_KINDS + NUMBER_WAND_KINDS;

	root = BrogueDisplay_getRootWindow(io_state.display);
	window = BrogueWindow_open(
		root, STAT_BAR_WIDTH + 2, (ROWS - h) / 2, DCOLS - 2, h);
	BrogueWindow_setColor(window, windowColor);
	context = BrogueDrawContext_open(window);
	BrogueDrawContext_enableProportionalFont(context, 1);

	x = 1;
	y = 1;

	BrogueDrawContext_push(context);
	BrogueDrawContext_setForeground(
		context, colorForDisplay(flavorTextColor));
	BrogueDrawContext_enableJustify(
		context, x, x + col_width, BROGUE_JUSTIFY_CENTER);

	BrogueDrawContext_drawAsciiString(context, x, y, "卷轴");	
	y += NUMBER_SCROLL_KINDS + 2;

	BrogueDrawContext_drawAsciiString(context, x, y, "戒指");
	x += col_width;
	y = 1;
	BrogueDrawContext_enableJustify(
		context, x, x + col_width, BROGUE_JUSTIFY_CENTER);

	BrogueDrawContext_drawAsciiString(context, x, y, "药剂");
	x += col_width;
	y = 1;
	BrogueDrawContext_enableJustify(
		context, x, x + col_width, BROGUE_JUSTIFY_CENTER);

	BrogueDrawContext_drawAsciiString(context, x, y, "法杖");
	y += NUMBER_STAFF_KINDS + 2;
	
	BrogueDrawContext_drawAsciiString(context, x, y, "魔棒");
	BrogueDrawContext_pop(context);

	printDiscoveries(
		context, SCROLL, NUMBER_SCROLL_KINDS, SCROLL_CHAR, 1, 2);
	printDiscoveries(
		context, RING, NUMBER_RING_KINDS, RING_CHAR, 
		1, 2 + NUMBER_SCROLL_KINDS + 2);
	printDiscoveries(
		context, POTION, NUMBER_POTION_KINDS, POTION_CHAR,
		1 + col_width, 2);
	printDiscoveries(
		context, STAFF, NUMBER_STAFF_KINDS, STAFF_CHAR,
		1 + 2 * col_width, 2);
	printDiscoveries(
		context, WAND, NUMBER_WAND_KINDS, WAND_CHAR,
		1 + 2 * col_width, 2 + NUMBER_STAFF_KINDS + 2);

	BrogueDrawContext_setForeground(
		context, colorForDisplay(itemMessageColor));
	BrogueDrawContext_enableJustify(
		context, 1, DCOLS - 4, BROGUE_JUSTIFY_CENTER);
	BrogueDrawContext_drawAsciiString(
		context, 1, 5 + NUMBER_STAFF_KINDS + NUMBER_WAND_KINDS,
		"-- 按任意键继续 --");

	waitForKeystrokeOrMouseClick();

	BrogueDrawContext_close(context);
	BrogueWindow_close(window);
}

void printHighScores(boolean hiliteMostRecent) {
	short i, hiliteLineNum, maxLength = 0, leftOffset;
	rogueHighScoresEntry list[HIGH_SCORES_COUNT] = {{0}};
	char buf[DCOLS*3];
	color scoreColor;
	BROGUE_WINDOW *root, *window;
	BROGUE_DRAW_CONTEXT *context;
	int tabStops[3];
	
	hiliteLineNum = getHighScoresList(list);
	
	if (!hiliteMostRecent) {
		hiliteLineNum = -1;
	}

	blackOutScreen();

	root = BrogueDisplay_getRootWindow(io_state.display);
	window = BrogueWindow_open(root, 0, 0, COLS, ROWS);
	context = BrogueDrawContext_open(window);
	BrogueDrawContext_enableProportionalFont(context, 1);
	
	for (i = 0; i < HIGH_SCORES_COUNT && list[i].score > 0; i++) {
		BROGUE_TEXT_SIZE text_size;

		text_size = BrogueDrawContext_measureAsciiString(
			context, 0, list[i].description);

		if (text_size.width > maxLength) {
			maxLength = text_size.width;
		}
	}
	
	leftOffset = min(COLS - maxLength - 21 - 1, COLS/5);
	
	scoreColor = black;
	applyColorAverage(&scoreColor, &itemMessageColor, 100);

	BrogueDrawContext_push(context);
	BrogueDrawContext_setForeground(
		context, colorForDisplay(scoreColor));
	BrogueDrawContext_enableJustify(context, 0, COLS, BROGUE_JUSTIFY_CENTER);
	BrogueDrawContext_drawAsciiString(context, 0, 0, "最高分");
	BrogueDrawContext_pop(context);

	tabStops[0] = leftOffset + 3;
	tabStops[1] = tabStops[0] + 6;
	tabStops[2] = tabStops[1] + 7;
	BrogueDrawContext_setTabStops(context, 3, tabStops);
	for (i = 0; i < HIGH_SCORES_COUNT && list[i].score > 0; i++) {
		scoreColor = black;
		if (i == hiliteLineNum) {
			applyColorAverage(&scoreColor, &itemMessageColor, 100);
		} else {
			applyColorAverage(&scoreColor, &white, 100);
			applyColorAverage(&scoreColor, &black, (i * 50 / 24));
		}
		BrogueDrawContext_setForeground(
			context, colorForDisplay(scoreColor));
		
		// rank
		sprintf(buf, "%i)\t%li\t%s\t%s", i + 1, list[i].score, list[i].date, list[i].description);
		BrogueDrawContext_drawAsciiString(context, leftOffset, i + 2, buf);
	}
	
	scoreColor = black;
	applyColorAverage(&scoreColor, &goodMessageColor, 100);
	BrogueDrawContext_push(context);
	BrogueDrawContext_enableJustify(context, 0, COLS, BROGUE_JUSTIFY_CENTER);
	BrogueDrawContext_drawAsciiString(context, 0, ROWS - 1, 
									  "按空格键继续。");
	BrogueDrawContext_pop(context);

	waitForAcknowledgment();

	BrogueWindow_close(window);
}

void printSeed() {
	char buf[COLS];
	sprintf(buf, "Dungeon seed #%lu; turn #%lu", rogue.seed, rogue.playerTurnNumber);
	message(buf, false);	
}

/*  Draw a progress bar  */
void drawProgress(
	BROGUE_DRAW_CONTEXT *context, BROGUE_EFFECT *effect,
	short x, short y, const char barLabel[COLS],
	long amtFilled, long amtMax, color *fillColor, boolean dim) {

	color progressBarColor;
	PROGRESS_BAR_EFFECT_PARAM progress;
	
	if (y >= ROWS - 1) { // don't write over the depth number
		return;
	}
	
	if (amtFilled > amtMax) {
		amtFilled = amtMax;
	}
	
	if (amtMax <= 0) {
		amtMax = 1;
	}
	
	progressBarColor = *fillColor;
	if (!(y % 2)) {
		applyColorAverage(&progressBarColor, &black, 25);
	}
	
	if (dim) {
		applyColorAverage(&progressBarColor, &black, 50);
	}

	progress.size = 20;
	progress.amount = (double)amtFilled / (double)amtMax;
	
	BrogueDrawContext_push(context);
	BrogueDrawContext_setForeground(
		context, colorForDisplay(dim ? gray : white));
	BrogueDrawContext_setBackground(
		context, colorForDisplay(progressBarColor));
	BrogueDrawContext_setEffect(context, effect);
	BrogueEffect_setParameters(effect, &progress);
	BrogueDrawContext_drawAsciiString(context, x, y, barLabel);
	BrogueDrawContext_pop(context);
}

void printProgressBar(
	short x, short y, const char barLabel[COLS], 
	long amtFilled, long amtMax, color *fillColor, boolean dim) {

	drawProgress(io_state.sidebar_context, io_state.progress_bar_effect,
				 x, y, barLabel, amtFilled, amtMax, fillColor, dim);
}

// returns the y-coordinate after the last line printed
short printMonsterInfo(creature *monst, short y, boolean dim, boolean highlight) {
	char buf[COLS], monstName[COLS];
	uchar monstChar;
	color monstForeColor, healthBarColor, tempColor;
	short i, displayedArmor;
	short x;

	monstForeColor = white;
	
	const char hallucinationStrings[10][COLS] = {
		"     (Dancing)      ",
		"     (Singing)      ",
		"  (Pontificating)   ",
		"     (Skipping)     ",
		"     (Spinning)     ",
		"      (Crying)      ",
		"     (Laughing)     ",
		"     (Humming)      ",
		"    (Whistling)     ",
		"    (Quivering)     ",
	};
	const char statusStrings[NUMBER_OF_STATUS_EFFECTS][COLS] = {
		"Weakened: -",
		"Telepathic",
		"Hallucinating",
		"Levitating",
		"Slowed",
		"Hasted",
		"Confused",
		"Burning",
		"Paralyzed",
		"Poisoned",
		"Stuck",
		"Nauseous",
		"Discordant",
		"Immune to Fire",
		"", // STATUS_EXPLOSION_IMMUNITY,
		"", // STATUS_NUTRITION,
		"", // STATUS_ENTERS_LEVEL_IN,
		"Frightened",
		"Entranced",
		"Darkened",
		"Lifespan",
		"Shielded",
        "Invisible",
	};
	
	if (y >= ROWS - 1) {
		return ROWS - 1;
	}
	
	assureCosmeticRNG;	
	BrogueDrawContext_push(io_state.sidebar_context);

	if (y < ROWS - 1) {
		// Unhighlight if it's highlighted as part of the path.
		pmap[monst->xLoc][monst->yLoc].flags &= ~IS_IN_PATH;

		BrogueDrawContext_push(io_state.sidebar_context);
		getCellAppearance(io_state.sidebar_context, 
						  monst->xLoc, monst->yLoc, NULL, &monstChar);

		BrogueDrawContext_enableTiles(io_state.sidebar_context, 1);
		BrogueDrawContext_drawChar(
			io_state.sidebar_context, 0, y, monstChar);
		BrogueDrawContext_pop(io_state.sidebar_context);

		monsterName(monstName, monst, false);
		upperCase(monstName);
        
        sprintf(buf, ": %s", monstName);

		BrogueDrawContext_setForeground(
			io_state.sidebar_context, colorForDisplay(dim ? gray : white));
		BrogueDrawContext_enableProportionalFont(io_state.sidebar_context, 1);
        BrogueDrawContext_drawAsciiString(
			io_state.sidebar_context, 1, y, buf);
		x = 1 + BrogueDrawContext_measureAsciiString(
			io_state.sidebar_context, 0, buf).width;

        if (monst == &player) {
            if (player.status[STATUS_INVISIBLE]) {
				BrogueDrawContext_setForeground(
					io_state.sidebar_context, 
					colorForDisplay(playerInvisibleColor));
				BrogueDrawContext_drawAsciiString(
					io_state.sidebar_context, x, y, " (隐形)");
            } else if (playerInDarkness()) {
				BrogueDrawContext_setForeground(
					io_state.sidebar_context, colorForDisplay(monstForeColor));
				BrogueDrawContext_drawAsciiString(
					io_state.sidebar_context, x, y, " (在暗处)");
            } else if (pmap[player.xLoc][player.yLoc].flags & IS_IN_SHADOW) {
                
            } else {
				BrogueDrawContext_setForeground(
					io_state.sidebar_context, colorForDisplay(monstForeColor));
				BrogueDrawContext_drawAsciiString(
					io_state.sidebar_context, x, y, " (在明处)");
            }
        }

		y++;
	}
    
    // mutation, if any
    if (y < ROWS - 1
        && monst->mutationIndex >= 0) {
        sprintf(buf, "(%s)", mutationCatalog[monst->mutationIndex].title);
        tempColor = *mutationCatalog[monst->mutationIndex].textColor;
        if (dim) {
            applyColorAverage(&tempColor, &black, 50);
        }

		BrogueDrawContext_setForeground(
			io_state.sidebar_context, colorForDisplay(tempColor));
		BrogueDrawContext_drawAsciiString(
			io_state.sidebar_context, 0, y++, buf);
	} 
	
	// hit points
	if (monst->info.maxHP > 1) {
		if (monst == &player) {
			healthBarColor = redBar;
			applyColorAverage(&healthBarColor, &blueBar, min(100, 100 * player.currentHP / player.info.maxHP));
		} else {
			healthBarColor = blueBar;
		}
		printProgressBar(0, y++, "体力", monst->currentHP, monst->info.maxHP, &healthBarColor, dim);
	}
	
	if (monst == &player) {
		// nutrition
		if (player.status[STATUS_NUTRITION] > HUNGER_THRESHOLD) {
			printProgressBar(0, y++, "食物", player.status[STATUS_NUTRITION], STOMACH_SIZE, &blueBar, dim);
		} else if (player.status[STATUS_NUTRITION] > WEAK_THRESHOLD) {
			printProgressBar(0, y++, "食物 (饥饿)", player.status[STATUS_NUTRITION], STOMACH_SIZE, &blueBar, dim);
		} else if (player.status[STATUS_NUTRITION] > FAINT_THRESHOLD) {
			printProgressBar(0, y++, "食物 (虚弱)", player.status[STATUS_NUTRITION], STOMACH_SIZE, &blueBar, dim);
		} else if (player.status[STATUS_NUTRITION] > 0) {
			printProgressBar(0, y++, "食物 (饿晕)", player.status[STATUS_NUTRITION], STOMACH_SIZE, &blueBar, dim);
		} else if (y < ROWS - 1) {
			BrogueDrawContext_setForeground(
				io_state.sidebar_context, colorForDisplay(badMessageColor));
			BrogueDrawContext_drawAsciiString(
				io_state.sidebar_context, 0, y++, "      急需食物      ");
		}
	}
	
	if (!player.status[STATUS_HALLUCINATING] || rogue.playbackOmniscience || monst == &player) {
		
		for (i=0; i<NUMBER_OF_STATUS_EFFECTS; i++) {
			if (i == STATUS_WEAKENED && monst->status[i] > 0) {
				sprintf(buf, "%s%i", statusStrings[STATUS_WEAKENED], monst->weaknessAmount);
				printProgressBar(0, y++, buf, monst->status[i], monst->maxStatus[i], &redBar, dim);
			} else if (i == STATUS_LEVITATING && monst->status[i] > 0) {
				printProgressBar(0, y++, (monst == &player ? "悬浮" : "飞行"), monst->status[i], monst->maxStatus[i], &redBar, dim);
			} else if (i == STATUS_POISONED
					   && monst->status[i] > 0
					   && monst->status[i] >= monst->currentHP) {
				printProgressBar(0, y++, "已中剧毒", monst->status[i], monst->maxStatus[i], &redBar, dim);
			} else if (statusStrings[i][0] && monst->status[i] > 0) {
				printProgressBar(0, y++, statusStrings[i], monst->status[i], monst->maxStatus[i], &redBar, dim);
			}
		}
		if (monst->targetCorpseLoc[0] == monst->xLoc && monst->targetCorpseLoc[1] == monst->yLoc) {
			printProgressBar(0, y++,  monsterText[monst->info.monsterID].absorbStatus, monst->corpseAbsorptionCounter, 20, &redBar, dim);
		}
	}
	
	BrogueDrawContext_setForeground(
		io_state.sidebar_context, colorForDisplay(dim ? darkGray : gray));

	if (monst != &player
		&& (!(monst->info.flags & MONST_INANIMATE)
			|| monst->creatureState == MONSTER_ALLY)) {

			BrogueDrawContext_push(io_state.sidebar_context);
			BrogueDrawContext_enableJustify(
				io_state.sidebar_context, 0, STAT_BAR_WIDTH, 
				BROGUE_JUSTIFY_CENTER);
			
			if (y < ROWS - 1) {
				if (player.status[STATUS_HALLUCINATING] && !rogue.playbackOmniscience && y < ROWS - 1) {
				BrogueDrawContext_drawAsciiString(
					io_state.sidebar_context, 0, y++, 
					hallucinationStrings[rand_range(0, 9)]);
				} else if (monst->bookkeepingFlags & MONST_CAPTIVE && y < ROWS - 1) {
					BrogueDrawContext_drawAsciiString(
						io_state.sidebar_context, 0, y++,
						"     (Captive)      ");
				} else if ((monst->info.flags & MONST_RESTRICTED_TO_LIQUID)
						   && !cellHasTMFlag(monst->xLoc, monst->yLoc, TM_ALLOWS_SUBMERGING)) {
					BrogueDrawContext_drawAsciiString(
						io_state.sidebar_context, 0, y++,
						"     (Helpless)     ");
				} else if (monst->creatureState == MONSTER_SLEEPING && y < ROWS - 1) {
					BrogueDrawContext_drawAsciiString(
						io_state.sidebar_context, 0, y++,
						"     (Sleeping)     ");
                } else if ((monst->creatureState == MONSTER_ALLY) && y < ROWS - 1) {
                    BrogueDrawContext_drawAsciiString(
						io_state.sidebar_context, 0, y++,
						"       (Ally)       ");
                } else if (monst->ticksUntilTurn > player.ticksUntilTurn + player.movementSpeed) {
                    BrogueDrawContext_drawAsciiString(
						io_state.sidebar_context, 0, y++,
						"   (Off balance)    ");
				} else if (monst->creatureState == MONSTER_FLEEING && y < ROWS - 1) {
					BrogueDrawContext_drawAsciiString(
						io_state.sidebar_context, 0, y++,
						"     (Fleeing)      ");
				} else if ((monst->creatureState == MONSTER_TRACKING_SCENT) && y < ROWS - 1) {
					BrogueDrawContext_drawAsciiString(
						io_state.sidebar_context, 0, y++,
						"     (Hunting)      ");
				} else if ((monst->creatureState == MONSTER_WANDERING) && y < ROWS - 1) {
					if ((monst->bookkeepingFlags & MONST_FOLLOWER) && monst->leader && (monst->leader->info.flags & MONST_IMMOBILE)) {
						// follower of an immobile leader -- i.e. a totem
						BrogueDrawContext_drawAsciiString(
							io_state.sidebar_context, 0, y++,
							"    (Worshiping)    ");
					} else if ((monst->bookkeepingFlags & MONST_FOLLOWER) && monst->leader && (monst->leader->bookkeepingFlags & MONST_CAPTIVE)) {
						// actually a captor/torturer
						BrogueDrawContext_drawAsciiString(
							io_state.sidebar_context, 0, y++,
							"     (Guarding)     ");
					} else {
						BrogueDrawContext_drawAsciiString(
							io_state.sidebar_context, 0, y++,
							"    (Wandering)     ");
					}
				}
			}

			BrogueDrawContext_pop(io_state.sidebar_context);
		} else if (monst == &player) {
			char strColorEscape[5] = { 0 };

			if (y < ROWS - 1) {
				BrogueDrawContext_push(io_state.sidebar_context);
				tempColor = dim ? darkGray : gray;
				if (player.status[STATUS_WEAKENED]) {
					tempColor = red;
					if (dim) {
						applyColorAverage(&tempColor, &black, 50);
					}
				}
				encodeMessageColor(strColorEscape, 0, &tempColor);
				sprintf(buf, "力量: %s%i", strColorEscape,
						rogue.strength - player.weaknessAmount);
				BrogueDrawContext_drawAsciiString(
					io_state.sidebar_context, 4, y, buf);
				BrogueDrawContext_pop(io_state.sidebar_context);
                
                displayedArmor = displayedArmorValue();
				
				if (!rogue.armor || rogue.armor->flags & ITEM_IDENTIFIED || rogue.playbackOmniscience) {
					
					sprintf(buf, "  护甲: %i", displayedArmor);
				} else {
					sprintf(buf, "  护甲: %i?", 
							max(0, (short) (((armorTable[rogue.armor->kind].range.upperBound + armorTable[rogue.armor->kind].range.lowerBound) / 2) / 10 + strengthModifier(rogue.armor))));
				}
				BrogueDrawContext_drawAsciiString(
					io_state.sidebar_context, STAT_BAR_WIDTH / 2, y, buf);

				y++;
			}
			if (y < ROWS - 1 && rogue.gold) {
				sprintf(buf, "Gold: %li", rogue.gold);

				BrogueDrawContext_push(io_state.sidebar_context);
				BrogueDrawContext_enableJustify(
					io_state.sidebar_context, 0, STAT_BAR_WIDTH, 
					BROGUE_JUSTIFY_CENTER);
												
				BrogueDrawContext_drawAsciiString(
					io_state.sidebar_context, 0, y++, buf);
				BrogueDrawContext_pop(io_state.sidebar_context);
			}
		}
	
	if (y < ROWS - 1) {
		y++;
	}
	
	restoreRNG;
	BrogueDrawContext_pop(io_state.sidebar_context);

	return y;
}

// Returns the y-coordinate after the last line printed.
short printItemInfo(item *theItem, short y, boolean dim, boolean highlight) {
	char name[COLS];
	uchar itemChar;
	BROGUE_TEXT_SIZE size;
	
	if (y >= ROWS - 1) {
		return ROWS - 1;
	}
	
	assureCosmeticRNG;
	
	if (y < ROWS - 1) {
		// Unhighlight if it's highlighted as part of the path.
		pmap[theItem->xLoc][theItem->yLoc].flags &= ~IS_IN_PATH;

		BrogueDrawContext_push(io_state.sidebar_context);
		getCellAppearance(io_state.sidebar_context, 
						  theItem->xLoc, theItem->yLoc, NULL, &itemChar);

		BrogueDrawContext_enableTiles(io_state.sidebar_context, 1);
		BrogueDrawContext_drawChar(
			io_state.sidebar_context, 0, y, itemChar);
		BrogueDrawContext_pop(io_state.sidebar_context);

		BrogueDrawContext_setForeground(
			io_state.sidebar_context, colorForDisplay(dim ? gray : white));
		BrogueDrawContext_drawChar(
			io_state.sidebar_context, 1, y, ':');

		if (rogue.playbackOmniscience || !player.status[STATUS_HALLUCINATING]) {
			itemName(theItem, name, true, true, (dim ? &gray : &white));
		} else {
			describedItemCategory(theItem->category, name);
		}
		upperCase(name);

		BrogueDrawContext_push(io_state.sidebar_context);
		BrogueDrawContext_enableProportionalFont(io_state.sidebar_context, 1);
		BrogueDrawContext_enableWrap(
			io_state.sidebar_context, 2, STAT_BAR_WIDTH);
		BrogueDrawContext_drawAsciiString(
			io_state.sidebar_context, 2, y, name);
		size = BrogueDrawContext_measureAsciiString(
			io_state.sidebar_context, 2, name);
		y += size.height;
		BrogueDrawContext_pop(io_state.sidebar_context);
	}

	y++;
	
	restoreRNG;
	return y;
}

// Returns the y-coordinate after the last line printed.
short printTerrainInfo(short x, short y, short py, const char *description, boolean dim, boolean highlight) {
	uchar displayChar;
    char name[DCOLS*2];
    color textColor;
	
	if (py >= ROWS - 1) {
		return ROWS - 1;
	}
	
	assureCosmeticRNG;
	
	if (py < ROWS - 1) {
		// Unhighlight if it's highlighted as part of the path.
		pmap[x][y].flags &= ~IS_IN_PATH;

		BrogueDrawContext_push(io_state.sidebar_context);
		getCellAppearance(io_state.sidebar_context, x, y, NULL, &displayChar);

		BrogueDrawContext_enableTiles(io_state.sidebar_context, 1);
		BrogueDrawContext_drawChar(
			io_state.sidebar_context, 0, py, displayChar);
		BrogueDrawContext_pop(io_state.sidebar_context);

		BrogueDrawContext_setForeground(
			io_state.sidebar_context, colorForDisplay(dim ? gray : white));
		BrogueDrawContext_drawChar(
			io_state.sidebar_context, 1, py, ':');

		strcpy(name, description);
		upperCase(name);

        textColor = flavorTextColor;
        if (dim) {
            applyColorScalar(&textColor, 50);
        }

		BrogueDrawContext_push(io_state.sidebar_context);
		BrogueDrawContext_setForeground(
			io_state.sidebar_context, colorForDisplay(textColor));
		BrogueDrawContext_enableWrap(
			io_state.sidebar_context, 2, STAT_BAR_WIDTH);
		BrogueDrawContext_enableProportionalFont(io_state.sidebar_context, 1);
		BrogueDrawContext_drawAsciiString(
			io_state.sidebar_context, 2, py, name);
		py += BrogueDrawContext_measureAsciiString(
			io_state.sidebar_context, 2, name).height - 1;
		BrogueDrawContext_pop(io_state.sidebar_context);
	}

	py += 2;
	
	restoreRNG;
	return py;
}

#define MIN_DEFAULT_INFO_PANEL_WIDTH	33

BROGUE_WINDOW *openTextBox(
	BROGUE_DRAW_CONTEXT **out_context, BROGUE_EFFECT **out_effect,
	char *textBuf, int x, short width,
	color *foreColor,
	brogueButton *buttons, short buttonCount) {

	BROGUE_WINDOW *root;
	BROGUE_WINDOW *window;
	BROGUE_DRAW_CONTEXT *context;
	BROGUE_EFFECT *effect;
	short i, bx, by;
	BROGUE_DRAW_COLOR windowColor = { 0.05, 0.05, 0.20, 0.8 };
	BROGUE_TEXT_SIZE size;
	int w;
	int h;

	if (width <= 0)
	{
		width = MIN_DEFAULT_INFO_PANEL_WIDTH;
	}

	w = width;
	h = 3;

	root = BrogueDisplay_getRootWindow(io_state.display);

	window = BrogueWindow_open(
		root, x, MESSAGE_LINES + (DROWS - h) / 2, w, h);
	context = BrogueDrawContext_open(window);
	effect = BrogueEffect_open(context, BUTTON_EFFECT_NAME);

	BrogueDrawContext_enableProportionalFont(context, 1);
	BrogueDrawContext_enableWrap(context, 1, w - 1);
	BrogueDrawContext_setForeground(context, colorForDisplay(*foreColor));

	size = BrogueDrawContext_measureAsciiString(context, 1, textBuf);

	h = size.height + 2;
	w = size.width + 2;

	if (buttonCount > 0) {
		int bw = 2;

		for (i=0; i<buttonCount; i++) {
			bw += strLenWithoutEscapes(buttons[i].text) + 2;
		}

		if (bw > w)
		{
			if (bw > width)
			{
				w = width;
			}
			else
			{
				w = bw;
			}

			BrogueDrawContext_enableWrap(context, 1, w - 1);
			size = BrogueDrawContext_measureAsciiString(context, 1, textBuf);
			h = size.height + 2;
		}

		bx = w;
		by = h;
		h += 2;
		for (i=0; i<buttonCount; i++) {
			if (buttons[i].flags & B_DRAW) {
				bx -= strLenWithoutEscapes(buttons[i].text) + 2;
				buttons[i].x = bx;
				buttons[i].y = by;
				if (bx < 1) {
					// Buttons can wrap to the next line (though are double-spaced).
					bx = width - (strLenWithoutEscapes(buttons[i].text) + 2);
					by += 2;
					h += 2;
					buttons[i].x = bx;
					buttons[i].y = by;
				}
			}
		}
	}

	if (x < 0)
	{
		x = STAT_BAR_WIDTH + (DCOLS - w) / 2;
	}

	// BrogueWindow_reposition(
	// 	window, x, MESSAGE_LINES + (DROWS - h) / 2,	w, h);
	
	// +2 is hack to add a little padding to button text boxes so text can be shown complete and it just works
	BrogueWindow_reposition(
		window, x, MESSAGE_LINES + (DROWS - h) / 2,	w + 2, h);
	
	BrogueWindow_setColor(window, windowColor);

	BrogueDrawContext_drawAsciiString(context, 1, 1, textBuf);

	if (out_context != NULL)
	{
		*out_context = context;
	}
	if (out_effect != NULL)
	{
		*out_effect = effect;
	}

	return window;
}

// If buttons are provided, we'll extend the text box downward, re-position the buttons,
// run a button input loop and return the result.
// (Returns -1 for canceled; otherwise the button index number.)
short printTextBox(char *textBuf, short width,
				   color *foreColor, color *backColor,
				   brogueButton *buttons, short buttonCount) {
	BROGUE_WINDOW *window;
	BROGUE_DRAW_CONTEXT *context;
	BROGUE_EFFECT *effect;
	int ret = -1;
	
	window = openTextBox(&context, &effect,
						 textBuf, -1, width, foreColor, 
						 buttons, buttonCount);

	if (buttonCount > 0)
	{
		ret = buttonInputLoop(buttons, buttonCount, context, effect,
							  0, 0, 0, 0, NULL);
	}
	else
	{
		waitForKeystrokeOrMouseClick();
	}

	BrogueWindow_close(window);

	return ret;
}

/*  Show the details dialog for a monster  */
BROGUE_WINDOW *printMonsterDetails(creature *monst) {
	char textBuf[COLS * 100];
	int x;
	
	monsterDetails(textBuf, monst);
	if (monst->xLoc < DCOLS / 2)
	{
		x = COLS - MIN_DEFAULT_INFO_PANEL_WIDTH - 2;
	}
	else
	{
		x = STAT_BAR_WIDTH + 2;
	}

	return openTextBox(NULL, NULL, textBuf, x, 0, &white, NULL, 0);
}

// Displays the item info box with the dark blue background.
// If includeButtons is true, we include buttons for item actions.
// Returns the key of an action to take, if any; otherwise -1.
unsigned long printCarriedItemDetails(item *theItem,
									  short width,
									  boolean includeButtons) {
	char textBuf[COLS * 100], goldColorEscape[5] = "", whiteColorEscape[5] = "";
	brogueButton buttons[20] = {{{0}}};
	short b;
	
	itemDetails(textBuf, theItem);
	
	for (b=0; b<20; b++) {
		initializeButton(&(buttons[b]));
		buttons[b].flags |= B_WIDE_CLICK_AREA;
	}
	
	b = 0;
	if (includeButtons) {
		encodeMessageColor(goldColorEscape, 0, &yellow);
		encodeMessageColor(whiteColorEscape, 0, &white);
		
		if (theItem->category & (FOOD | SCROLL | POTION | WAND | STAFF | CHARM)) {
			sprintf(buttons[b].text, "   %sa%spply   ", goldColorEscape, whiteColorEscape);
			buttons[b].hotkey[0] = APPLY_KEY;
			b++;
		}
		if (theItem->category & (ARMOR | WEAPON | RING)) {
			if (theItem->flags & ITEM_EQUIPPED) {
				sprintf(buttons[b].text, "  %sr%semove   ", goldColorEscape, whiteColorEscape);
				buttons[b].hotkey[0] = UNEQUIP_KEY;
				b++;
			} else {
				sprintf(buttons[b].text, "   %se%squip   ", goldColorEscape, whiteColorEscape);
				buttons[b].hotkey[0] = EQUIP_KEY;
				b++;
			}
		}
		sprintf(buttons[b].text, "   %sd%srop    ", goldColorEscape, whiteColorEscape);
		buttons[b].hotkey[0] = DROP_KEY;
		b++;
		
		sprintf(buttons[b].text, "   %st%shrow   ", goldColorEscape, whiteColorEscape);
		buttons[b].hotkey[0] = THROW_KEY;
		b++;
		
		if (itemCanBeCalled(theItem)) {
			sprintf(buttons[b].text, "   %sc%sall    ", goldColorEscape, whiteColorEscape);
			buttons[b].hotkey[0] = CALL_KEY;
			b++;
		}
		
		// Add invisible previous and next buttons, so up and down arrows can page through items.
		// Previous
		buttons[b].flags = B_ENABLED; // clear everything else
		buttons[b].hotkey[0] = UP_KEY;
		buttons[b].hotkey[1] = NUMPAD_8;
		buttons[b].hotkey[2] = UP_ARROW;
		b++;
		// Next
		buttons[b].flags = B_ENABLED; // clear everything else
		buttons[b].hotkey[0] = DOWN_KEY;
		buttons[b].hotkey[1] = NUMPAD_2;
		buttons[b].hotkey[2] = DOWN_ARROW;
		b++;
	}	
	b = printTextBox(textBuf, width, &white, &interfaceBoxColor, buttons, b);
	
	if (!includeButtons) {
		waitForKeystrokeOrMouseClick();
		return -1;
	}
	
	if (b >= 0) {
		return buttons[b].hotkey[0];
	} else {
		return -1;
	}
}

/*  Show the details dialog for a floor item  */
BROGUE_WINDOW *printFloorItemDetails(item *theItem) {
	char textBuf[COLS * 100];
	int x;
	
	itemDetails(textBuf, theItem);
	if (theItem->xLoc < DCOLS / 2)
	{
		x = COLS - MIN_DEFAULT_INFO_PANEL_WIDTH - 2;
	}
	else
	{
		x = STAT_BAR_WIDTH + 2;
	}

	return openTextBox(NULL, NULL, textBuf, x, 0, &white, NULL, 0);
}


/***********************************************/
// utf8 routines from
// http://www.cprogramming.com/tutorial/utf8.c
static unsigned int offsetsFromUTF8[6] = {
    0x00000000UL, 0x00003080UL, 0x000E2080UL,
    0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

static const char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/* conversions without error checking
   only works for valid UTF-8, i.e. no 5- or 6-byte sequences
   srcsz = source size in bytes, or -1 if 0-terminated
   sz = dest size in # of wide characters

   returns # characters converted
   dest will always be L'\0'-terminated, even if there isn't enough room
   for all the characters.
   if sz = srcsz+1 (i.e. 4*srcsz+4 bytes), there will always be enough space.
*/
static int u8_toucs(wchar_t *dest, int sz, const char *src, int srcsz)
{
    wchar_t ch;
    const char *src_end = src + srcsz;
    int nb;
    int i=0;

    while (i < sz-1) {
        nb = trailingBytesForUTF8[(unsigned char)*src];
        if (srcsz == -1) {
            if (*src == 0)
                goto done_toucs;
        }
        else {
            if (src + nb >= src_end)
                goto done_toucs;
        }
        ch = 0;
        switch (nb) {
            /* these fall through deliberately */
        case 3: ch += (unsigned char)*src++; ch <<= 6;
        case 2: ch += (unsigned char)*src++; ch <<= 6;
        case 1: ch += (unsigned char)*src++; ch <<= 6;
        case 0: ch += (unsigned char)*src++;
        }
        ch -= offsetsFromUTF8[nb];
        dest[i++] = ch;
    }
 done_toucs:
    dest[i] = 0;
    return i;
}

static int u8_wc_toutf8(char *dest, wchar_t ch)
{
    if (ch < 0x80) {
        dest[0] = (char)ch;
        return 1;
    }
    if (ch < 0x800) {
        dest[0] = (ch>>6) | 0xC0;
        dest[1] = (ch & 0x3F) | 0x80;
        return 2;
    }
    if (ch < 0x10000) {
        dest[0] = (ch>>12) | 0xE0;
        dest[1] = ((ch>>6) & 0x3F) | 0x80;
        dest[2] = (ch & 0x3F) | 0x80;
        return 3;
    }
    if (ch < 0x110000) {
        dest[0] = (ch>>18) | 0xF0;
        dest[1] = ((ch>>12) & 0x3F) | 0x80;
        dest[2] = ((ch>>6) & 0x3F) | 0x80;
        dest[3] = (ch & 0x3F) | 0x80;
        return 4;
    }
    return 0;
}

#define PACKED_BUF_SIZE 128
#define PACKED_LINE_SIZE 1024
static char packed_strings[PACKED_BUF_SIZE][PACKED_LINE_SIZE] = {{0}};
static char packed_ix = 0;

char* T(const wchar_t *ws) {
    // char* packed = calloc((wcslen(ws)*3 + 1), sizeof(char));
    char* packed = packed_strings[packed_ix++];
    char* ptr = packed;
    int ix;
    boolean stored_flag;
    while (*ws != 0) {
        ptr += u8_wc_toutf8(ptr, *ws);
        ++ws;
    }
    *ptr = '\0';

    // stored_flag = false;
    // for (ix = 0; ix < PACKED_BUF_SIZE; ++ix) {
    // 	if (packed_strings[ix] == 0) {
    // 		packed_strings[ix] = packed;
    // 		stored_flag = true;
    // 		break;
    // 	}
    // }
    // assert(stored_flag);
    
    return packed;
}


int T_unpack(const char* s, wchar_t *ws, int ws_size) {
	return u8_toucs(ws, ws_size, s, strlen(s)+1);
}

void T_free() {
 //    int ix;
 //    // this seems really dangerous
 //    for (ix = 0; ix < PACKED_BUF_SIZE; ++ix) {
 //    	if (packed_strings[ix] == 0) {
 //    		return;
 //    	}
	// 	free(packed_strings[ix]);
	// 	packed_strings[ix] = 0;
	// }
}

/* reads the next utf-8 sequence out of a string, updating an index */
wchar_t u8_nextchar(const char *s, int *i) {
    wchar_t ch = 0;
    int sz = 0;

    do {
        ch <<= 6;
        ch += (unsigned char)s[(*i)++];
        sz++;
    } while (s[*i] && !isutf(s[*i]));
    ch -= offsetsFromUTF8[sz-1];

    return ch;
}

/* number of characters */
int u8_strlen(const char *s) {
    int count = 0;
    int i = 0;

    while (u8_nextchar(s, &i) != 0)
        count++;

    return count;
}

/* display lengh, in which any unicode char count as 2 char long */
int u8_displen(const char *s) {
	short len, u8clen;
	len = 0;

	while (*s) {
		u8clen = u8_seqlen(s);
		len += 1 + (u8clen > 1); // unicode char all count as two char long
		s += u8clen;
	}

	return len;
}

/* returns length of next utf-8 sequence */
int u8_seqlen(const char *s)
{
    return trailingBytesForUTF8[(unsigned int)(unsigned char)s[0]] + 1;
}

