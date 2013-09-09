/* -*- tab-width: 4; -*- */

/*
 *  Items.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/17/09.
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

// Allocates space, generates a specified item (or random category/kind if -1)
// and returns a pointer to that item. The item is not given a location here
// and is not inserted into the item chain!
item *generateItem(unsigned short theCategory, short theKind) {
	short i;
	item *theItem;
	
	theItem = (item *) malloc(sizeof(item));
	memset(theItem, '\0', sizeof(item) );
	
	theItem->category = 0;
	theItem->kind = 0;
	theItem->flags = 0;
	theItem->displayChar = '&';
	theItem->foreColor = &itemColor;
	theItem->inventoryColor = &white;
	theItem->armor = 0;
	theItem->strengthRequired = 0;
	theItem->enchant1 = 0;
	theItem->enchant2 = 0;
	theItem->vorpalEnemy = 0;
	theItem->charges = 0;
	theItem->quantity = 1;
	theItem->quiverNumber = 0;
	theItem->keyZ = 0;
	theItem->inscription[0] = '\0';
	theItem->nextItem = NULL;
	
	for (i=0; i < KEY_ID_MAXIMUM; i++) {
		theItem->keyLoc[i].x = 0;
		theItem->keyLoc[i].y = 0;
		theItem->keyLoc[i].machine = 0;
		theItem->keyLoc[i].disposableHere = false;
	}
	
	makeItemInto(theItem, theCategory, theKind);
	
	return theItem;
}

unsigned long pickItemCategory(unsigned long theCategory) {
	short i, sum, randIndex;
	short probabilities[13] =						{50,	42,		52,		3,		3,		10,		8,		2,		3,      2,        0,		0,		0};
	unsigned short correspondingCategories[13] =	{GOLD,	SCROLL,	POTION,	STAFF,	WAND,	WEAPON,	ARMOR,	FOOD,	RING,   CHARM,    AMULET,	GEM,	KEY};
	
	sum = 0;
	
	for (i=0; i<13; i++) {
		if (theCategory <= 0 || theCategory & correspondingCategories[i]) {
			sum += probabilities[i];
		}
	}
	
	if (sum == 0) {
		return theCategory; // e.g. when you pass in AMULET or GEM, since they have no frequency
	}
	
	randIndex = rand_range(1, sum);
	
	for (i=0; ; i++) {
		if (theCategory <= 0 || theCategory & correspondingCategories[i]) {
			if (randIndex <= probabilities[i]) {
				return correspondingCategories[i];
			}
			randIndex -= probabilities[i];
		}
	}
}

// Sets an item to the given type and category (or chooses randomly if -1) with all other stats
item *makeItemInto(item *theItem, unsigned long itemCategory, short itemKind) {
	itemTable *theEntry = NULL;

	if (itemCategory <= 0) {
		itemCategory = ALL_ITEMS;
	}
	
	itemCategory = pickItemCategory(itemCategory);
	
	theItem->category = itemCategory;
	
	switch (itemCategory) {
			
		case FOOD:
			if (itemKind < 0) {
				itemKind = chooseKind(foodTable, NUMBER_FOOD_KINDS);
			}
			theEntry = &foodTable[itemKind];
			theItem->displayChar = FOOD_CHAR;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
			
		case WEAPON:
			if (itemKind < 0) {
				itemKind = chooseKind(weaponTable, NUMBER_WEAPON_KINDS);
			}
			theEntry = &weaponTable[itemKind];
			theItem->damage = weaponTable[itemKind].range;
			theItem->strengthRequired = weaponTable[itemKind].strengthRequired;
			theItem->displayChar = WEAPON_CHAR;
			
			switch (itemKind) {
				case MACE:
				case HAMMER:
					theItem->flags |= ITEM_ATTACKS_SLOWLY;
					break;
				case RAPIER:
					theItem->flags |= (ITEM_ATTACKS_QUICKLY | ITEM_LUNGE_ATTACKS);
					break;
				case SPEAR:
				case PIKE:
					theItem->flags |= ITEM_ATTACKS_PENETRATE;
					break;
				case AXE:
				case WAR_AXE:
					theItem->flags |= ITEM_ATTACKS_ALL_ADJACENT;
					break;
				default:
					break;
			}
			
			if (rand_percent(40)) {
				theItem->enchant1 += rand_range(1, 3);
				if (rand_percent(50)) {
					// cursed
					theItem->enchant1 *= -1;
					theItem->flags |= ITEM_CURSED;
					if (rand_percent(33)) { // give it a bad runic
						theItem->enchant2 = rand_range(NUMBER_GOOD_WEAPON_ENCHANT_KINDS, NUMBER_WEAPON_RUNIC_KINDS - 1);
						theItem->flags |= ITEM_RUNIC;
					}
				} else if (rand_range(3, 10)
                           * ((theItem->flags & ITEM_ATTACKS_SLOWLY) ? 2 : 1)
                           / ((theItem->flags & ITEM_ATTACKS_QUICKLY) ? 2 : 1)
                           > theItem->damage.lowerBound) {
					// give it a good runic; lower damage items are more likely to be runic
					theItem->enchant2 = rand_range(0, NUMBER_GOOD_WEAPON_ENCHANT_KINDS - 1);
					theItem->flags |= ITEM_RUNIC;
					if (theItem->enchant2 == W_SLAYING) {
						theItem->vorpalEnemy = chooseVorpalEnemy();
					}
				}
			}
			if (itemKind == DART || itemKind == INCENDIARY_DART || itemKind == JAVELIN) {
				if (itemKind == INCENDIARY_DART) {
					theItem->quantity = rand_range(3, 6);
				} else {
					theItem->quantity = rand_range(5, 18);
				}
				theItem->quiverNumber = rand_range(1, 60000);
				theItem->flags &= ~(ITEM_CURSED | ITEM_RUNIC); // throwing weapons can't be cursed or runic
				theItem->enchant1 = 0; // throwing weapons can't be magical
			}
			theItem->charges = WEAPON_KILLS_TO_AUTO_ID; // kill 20 enemies to auto-identify
			break;
			
		case ARMOR:
			if (itemKind < 0) {
				itemKind = chooseKind(armorTable, NUMBER_ARMOR_KINDS);
			}
			theEntry = &armorTable[itemKind];
			theItem->armor = randClump(armorTable[itemKind].range);
			theItem->strengthRequired = armorTable[itemKind].strengthRequired;
			theItem->displayChar = ARMOR_CHAR;
			theItem->charges = ARMOR_DELAY_TO_AUTO_ID; // this many turns until it reveals its enchants and whether runic
			if (rand_percent(40)) {
				theItem->enchant1 += rand_range(1, 3);
				if (rand_percent(50)) {
					// cursed
					theItem->enchant1 *= -1;
					theItem->flags |= ITEM_CURSED;
					if (rand_percent(33)) { // give it a bad runic
						theItem->enchant2 = rand_range(NUMBER_GOOD_ARMOR_ENCHANT_KINDS, NUMBER_ARMOR_ENCHANT_KINDS - 1);
						theItem->flags |= ITEM_RUNIC;
					}
				} else if (rand_range(0, 95) > theItem->armor) { // give it a good runic
					theItem->enchant2 = rand_range(0, NUMBER_GOOD_ARMOR_ENCHANT_KINDS - 1);
					theItem->flags |= ITEM_RUNIC;
					if (theItem->enchant2 == A_IMMUNITY) {
						theItem->vorpalEnemy = chooseVorpalEnemy();
					}
				}
			}
			break;
		case SCROLL:
			if (itemKind < 0) {
				itemKind = chooseKind(scrollTable, NUMBER_SCROLL_KINDS);
			}
			theEntry = &scrollTable[itemKind];
			theItem->displayChar = SCROLL_CHAR;
			theItem->flags |= ITEM_FLAMMABLE;
			break;
		case POTION:
			if (itemKind < 0) {
				itemKind = chooseKind(potionTable, NUMBER_POTION_KINDS);
			}
			theEntry = &potionTable[itemKind];
			theItem->displayChar = POTION_CHAR;
			break;
		case STAFF:
			if (itemKind < 0) {
				itemKind = chooseKind(staffTable, NUMBER_STAFF_KINDS);
			}
			theEntry = &staffTable[itemKind];
			theItem->displayChar = STAFF_CHAR;
			theItem->charges = 2;
			if (rand_percent(50)) {
				theItem->charges++;
				if (rand_percent(15)) {
					theItem->charges++;
				}
			}
			theItem->enchant1 = theItem->charges;
			theItem->enchant2 = (itemKind == STAFF_BLINKING || itemKind == STAFF_OBSTRUCTION ? 1000 : 500); // start with no recharging mojo
			break;
		case WAND:
			if (itemKind < 0) {
				itemKind = chooseKind(wandTable, NUMBER_WAND_KINDS);
			}
			theEntry = &wandTable[itemKind];
			theItem->displayChar = WAND_CHAR;
			theItem->charges = randClump(wandTable[itemKind].range);
			break;
		case RING:
			if (itemKind < 0) {
				itemKind = chooseKind(ringTable, NUMBER_RING_KINDS);
			}
			theEntry = &ringTable[itemKind];
			theItem->displayChar = RING_CHAR;
			theItem->enchant1 = randClump(ringTable[itemKind].range);
			theItem->charges = RING_DELAY_TO_AUTO_ID; // how many turns of being worn until it auto-identifies
			if (rand_percent(16)) {
				// cursed
				theItem->enchant1 *= -1;
				theItem->flags |= ITEM_CURSED;
			}
            theItem->enchant2 = 1;
			break;
        case CHARM:
			if (itemKind < 0) {
				itemKind = chooseKind(charmTable, NUMBER_CHARM_KINDS);
			}
            theItem->displayChar = CHARM_CHAR;
            theItem->charges = 0; // Charms are initially ready for use.
            theItem->enchant1 = randClump(charmTable[itemKind].range);
			theItem->flags |= ITEM_IDENTIFIED;
            break;
		case GOLD:
			theEntry = NULL;
			theItem->displayChar = GOLD_CHAR;
			theItem->quantity = rand_range(50 + rogue.depthLevel * 10, 100 + rogue.depthLevel * 15);
			break;
		case AMULET:
			theEntry = NULL;
			theItem->displayChar = AMULET_CHAR;
			itemKind = 0;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
		case GEM:
			theEntry = NULL;
			theItem->displayChar = GEM_CHAR;
			itemKind = 0;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
		case KEY:
			theEntry = NULL;
			theItem->displayChar = KEY_CHAR;
			theItem->flags |= ITEM_IDENTIFIED;
			break;
		default:
			theEntry = NULL;
			message("something has gone terribly wrong!", true);
			break;
	}
	if (theItem
		&& !(theItem->flags & ITEM_IDENTIFIED)
		&& (!(theItem->category & (POTION | SCROLL) ) || (theEntry && !theEntry->identified))) {
        
		theItem->flags |= ITEM_CAN_BE_IDENTIFIED;
	}
	theItem->kind = itemKind;
	
	return theItem;
}

short chooseKind(itemTable *theTable, short numKinds) {
	short i, totalFrequencies = 0, randomFrequency;
	for (i=0; i<numKinds; i++) {
		totalFrequencies += max(0, theTable[i].frequency);
	}
	randomFrequency = rand_range(1, totalFrequencies);
	for (i=0; randomFrequency > theTable[i].frequency; i++) {
		randomFrequency -= max(0, theTable[i].frequency);
	}
	return i;
}

// Places an item at (x,y) if provided or else a random location if they're 0. Inserts item into the floor list.
item *placeItem(item *theItem, short x, short y) {
	short loc[2];
	enum dungeonLayers layer;
	char theItemName[DCOLS], buf[DCOLS];
	if (x <= 0 || y <= 0) {
		randomMatchingLocation(&(loc[0]), &(loc[1]), FLOOR, NOTHING, -1);
		theItem->xLoc = loc[0];
		theItem->yLoc = loc[1];
	} else {
		theItem->xLoc = x;
		theItem->yLoc = y;
	}
	
	removeItemFromChain(theItem, floorItems); // just in case; double-placing an item will result in game-crashing loops in the item list
	
	theItem->nextItem = floorItems->nextItem;
	floorItems->nextItem = theItem;
	pmap[theItem->xLoc][theItem->yLoc].flags |= HAS_ITEM;
	if ((theItem->flags & ITEM_MAGIC_DETECTED) && itemMagicChar(theItem)) {
		pmap[theItem->xLoc][theItem->yLoc].flags |= ITEM_DETECTED;
	}
	if (cellHasTerrainFlag(x, y, T_IS_DF_TRAP)
		&& !(pmap[x][y].flags & PRESSURE_PLATE_DEPRESSED)) {
		
		pmap[x][y].flags |= PRESSURE_PLATE_DEPRESSED;
		if (playerCanSee(x, y)) {
			if (cellHasTMFlag(x, y, TM_IS_SECRET)) {
				discover(x, y);
				refreshDungeonCell(x, y);
			}
			itemName(theItem, theItemName, false, false, NULL);
			sprintf(buf, "%s压到了地面下的机关，发出了一声响声！", theItemName);
			message(buf, true);
		}
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[x][y].layers[layer]].flags & T_IS_DF_TRAP) {
				spawnDungeonFeature(x, y, &(dungeonFeatureCatalog[tileCatalog[pmap[x][y].layers[layer]].fireType]), true, false);
				promoteTile(x, y, layer, false);
			}
		}
	}
	return theItem;
}

void fillItemSpawnHeatMap(unsigned short heatMap[DCOLS][DROWS], unsigned short heatLevel, short x, short y) {
	enum directions dir;
	short newX, newY;
	
	if (pmap[x][y].layers[DUNGEON] == DOOR) {
		heatLevel += 10;
	} else if (pmap[x][y].layers[DUNGEON] == SECRET_DOOR) {
		heatLevel += 3000;
	}
	if (heatMap[x][y] > heatLevel) {
		heatMap[x][y] = heatLevel;
	}
	for (dir = 0; dir < 4; dir++) {
		newX = x + nbDirs[dir][0];
		newY = y + nbDirs[dir][1];
		if (coordinatesAreInMap(newX, newY)
			&& (!cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY | T_IS_DEEP_WATER | T_LAVA_INSTA_DEATH | T_AUTO_DESCENT)
				|| cellHasTMFlag(newX, newY, TM_IS_SECRET))
			&& heatLevel < heatMap[newX][newY]) {
			fillItemSpawnHeatMap(heatMap, heatLevel, newX, newY);
		}
	}
}

void coolHeatMapAt(unsigned short heatMap[DCOLS][DROWS], short x, short y, unsigned long *totalHeat) {
	short k, l;
	unsigned short currentHeat;
	
	currentHeat = heatMap[x][y];
	*totalHeat -= heatMap[x][y];
	heatMap[x][y] = 0;
	
	// lower the heat near the chosen location
	for (k = -5; k <= 5; k++) {
		for (l = -5; l <= 5; l++) {
			if (coordinatesAreInMap(x+k, y+l) && heatMap[x+k][y+l] == currentHeat) {
				heatMap[x+k][y+l] = max(1, heatMap[x+k][y+l]/10);
				*totalHeat -= (currentHeat - heatMap[x+k][y+l]);
			}
		}
	}
}

// Returns false if no place could be found.
// That should happen only if the total heat is zero.
boolean getItemSpawnLoc(unsigned short heatMap[DCOLS][DROWS], short *x, short *y, unsigned long *totalHeat) {
	unsigned long randIndex;
	unsigned short currentHeat;
	short i, j;
	
	if (*totalHeat <= 0) {
		return false;
	}
	
	randIndex = rand_range(1, *totalHeat);
	
	//printf("\nrandIndex: %i", randIndex);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			currentHeat = heatMap[i][j];
			if (randIndex <= currentHeat) { // this is the spot!
				*x = i;
				*y = j;
				return true;
			}
			randIndex -= currentHeat;
		}
	}
#ifdef BROGUE_ASSERTS
	assert(0); // should never get here!
#endif
	return false;
}

#define aggregateGoldLowerBound(d)	(pow((double) (d), 3.05) + 320 * (d) + FLOAT_FUDGE)
#define aggregateGoldUpperBound(d)	(pow((double) (d), 3.05) + 420 * (d) + FLOAT_FUDGE)

// Generates and places items for the level. Must pass the location of the up-stairway on the level.
void populateItems(short upstairsX, short upstairsY) {
	if (!ITEMS_ENABLED) {
		return;
	}
	item *theItem;
	unsigned short itemSpawnHeatMap[DCOLS][DROWS];
	short i, j, numberOfItems, numberOfGoldPiles, goldBonusProbability, x = 0, y = 0;
	unsigned long totalHeat;
	short theCategory, theKind;
	
#ifdef AUDIT_RNG
	char RNGmessage[100];
#endif
	
	if (rogue.depthLevel > AMULET_LEVEL) {
        if (rogue.depthLevel - AMULET_LEVEL - 1 >= 8) {
            numberOfItems = 1;
        } else {
            const short lumenstoneDistribution[8] = {3, 3, 3, 2, 2, 2, 2, 2};
            numberOfItems = lumenstoneDistribution[rogue.depthLevel - AMULET_LEVEL - 1];
        }
		numberOfGoldPiles = 0;
	} else {
        rogue.lifePotionFrequency += 34;
		rogue.strengthPotionFrequency += 17;
		rogue.enchantScrollFrequency += 30;
		numberOfItems = 3;
		while (rand_percent(60)) {
			numberOfItems++;
		}
		if (rogue.depthLevel <= 3) {
			numberOfItems += 2; // 6 extra items to kickstart your career as a rogue
		} else if (rogue.depthLevel <= 5) {
			numberOfItems++; // and 2 more here
		}

		numberOfGoldPiles = min(5, (int) (rogue.depthLevel / 4 + FLOAT_FUDGE));
		for (goldBonusProbability = 60;
			 rand_percent(goldBonusProbability) && numberOfGoldPiles <= 10;
			 goldBonusProbability -= 15) {
			
			numberOfGoldPiles++;
		}
		// Adjust the amount of gold if we're past depth 5 and we were below or above
		// the production schedule as of the previous depth.
		if (rogue.depthLevel > 5) {
			if (rogue.goldGenerated < aggregateGoldLowerBound(rogue.depthLevel - 1)) {
				numberOfGoldPiles += 2;
			} else if (rogue.goldGenerated > aggregateGoldUpperBound(rogue.depthLevel - 1)) {
				numberOfGoldPiles -= 2;
			}
		}
	}
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			itemSpawnHeatMap[i][j] = 50000;
		}
	}
	fillItemSpawnHeatMap(itemSpawnHeatMap, 5, upstairsX, upstairsY);
	totalHeat = 0;
	
#ifdef AUDIT_RNG
	sprintf(RNGmessage, "\n\nInitial heat map for level %i:\n", rogue.currentTurnNumber);
	RNGLog(RNGmessage);
#endif
	
	for (j=0; j<DROWS; j++) {
		for (i=0; i<DCOLS; i++) {
			
			if (cellHasTerrainFlag(i, j, T_OBSTRUCTS_ITEMS | T_PATHING_BLOCKER)
				|| (pmap[i][j].flags & (IS_CHOKEPOINT | IN_LOOP | IS_IN_MACHINE))
				|| passableArcCount(i, j) > 1) { // not in hallways or quest rooms, please
				itemSpawnHeatMap[i][j] = 0;
			} else if (itemSpawnHeatMap[i][j] == 50000) {
				itemSpawnHeatMap[i][j] = 0;
				pmap[i][j].layers[DUNGEON] = WALL; // due to a bug that created occasional isolated one-cell islands;
				// not sure if it's still around, but this is a good-enough failsafe
			}
#ifdef AUDIT_RNG
			sprintf(RNGmessage, "%u%s%s\t%s",
					itemSpawnHeatMap[i][j],
					((pmap[i][j].flags & IS_CHOKEPOINT) ? " (C)": ""), // chokepoint
					((pmap[i][j].flags & IN_LOOP) ? " (L)": ""), // loop
					(i == DCOLS-1 ? "\n" : ""));
			RNGLog(RNGmessage);
#endif
			totalHeat += itemSpawnHeatMap[i][j];
		}
	}

	if (D_INSPECT_LEVELGEN) {
		short **map = allocGrid();
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				map[i][j] = itemSpawnHeatMap[i][j] * -1;
			}
		}
		dumpLevelToScreen();
		freeGrid(map);
		temporaryMessage("Item spawn heat map:", true);
	}
	
	for (i=0; i<numberOfItems; i++) {
		theCategory = ALL_ITEMS & ~GOLD; // gold is placed separately, below, so it's not a punishment
		theKind = -1;
		
		scrollTable[SCROLL_ENCHANTING].frequency = rogue.enchantScrollFrequency;
		potionTable[POTION_STRENGTH].frequency = rogue.strengthPotionFrequency;
        potionTable[POTION_LIFE].frequency = rogue.lifePotionFrequency;
		
		// Adjust the desired item category if necessary.
		if ((rogue.foodSpawned + foodTable[RATION].strengthRequired / 2) * 4
			<= pow(rogue.depthLevel, 1.3) * foodTable[RATION].strengthRequired * 0.45) {
			// Guarantee a certain nutrition minimum of the approximate equivalent of one ration every four levels,
			// with more food on deeper levels since they generally take more turns to complete.
			theCategory = FOOD;
			if (rogue.depthLevel > AMULET_LEVEL) {
				numberOfItems++; // Food isn't at the expense of gems.
			}
		} else if (rogue.depthLevel > AMULET_LEVEL) {
			theCategory = GEM;
		} else if (rogue.lifePotionsSpawned * 4 + 3 < rogue.depthLevel) {
            theCategory = POTION;
            theKind = POTION_LIFE;
        }
		
		// Generate the item.
		theItem = generateItem(theCategory, theKind);
		
		if (theItem->category & FOOD) {
			rogue.foodSpawned += foodTable[theItem->kind].strengthRequired;
            //DEBUG printf("\n *** Depth %i: generated food!", rogue.depthLevel);
		}
		
		// Choose a placement location not in a hallway.
		do {
			if ((theItem->category & FOOD) || ((theItem->category & POTION) && theItem->kind == POTION_STRENGTH)) {
				randomMatchingLocation(&x, &y, FLOOR, NOTHING, -1); // food and gain strength don't follow the heat map
			} else {
				getItemSpawnLoc(itemSpawnHeatMap, &x, &y, &totalHeat);
			}
		} while (passableArcCount(x, y) > 1);
#ifdef BROGUE_ASSERTS
		assert(coordinatesAreInMap(x, y));
#endif
		// Cool off the item spawning heat map at the chosen location:
		coolHeatMapAt(itemSpawnHeatMap, x, y, &totalHeat);
		
		// Regulate the frequency of enchantment scrolls and strength/life potions.
		if (theItem->category & SCROLL && theItem->kind == SCROLL_ENCHANTING) {
			rogue.enchantScrollFrequency -= 50;
			//DEBUG printf("\ndepth %i: %i enchant scrolls generated so far", rogue.depthLevel, ++enchantScrolls);
		} else if (theItem->category & POTION && theItem->kind == POTION_LIFE) {
			//DEBUG printf("\n*** Depth %i: generated a life potion at %i frequency!", rogue.depthLevel, rogue.lifePotionFrequency);
			rogue.lifePotionFrequency -= 150;
            rogue.lifePotionsSpawned++;
		} else if (theItem->category & POTION && theItem->kind == POTION_STRENGTH) {
			//DEBUG printf("\n*** Depth %i: generated a strength potion at %i frequency!", rogue.depthLevel, rogue.strengthPotionFrequency);
			rogue.strengthPotionFrequency -= 50;
		}
		
		// Place the item.
		placeItem(theItem, x, y); // Random valid location already obtained according to heat map.
	}
	
	// Now generate gold.
	for (i=0; i<numberOfGoldPiles; i++) {
		theItem = generateItem(GOLD, -1);
		getItemSpawnLoc(itemSpawnHeatMap, &x, &y, &totalHeat);
		coolHeatMapAt(itemSpawnHeatMap, x, y, &totalHeat);
		placeItem(theItem, x, y);
		rogue.goldGenerated += theItem->quantity;
	}
	
	if (D_INSPECT_LEVELGEN) {
		dumpLevelToScreen();
		temporaryMessage("Added gold.", true);
	}
	
	scrollTable[SCROLL_ENCHANTING].frequency	= 0;	// No enchant scrolls or strength/life potions can spawn except via initial
	potionTable[POTION_STRENGTH].frequency      = 0;	// item population or blueprints that create them specifically.
    potionTable[POTION_LIFE].frequency          = 0;
	
	//DEBUG printf("\nD:%i: %lu gold generated so far.", rogue.depthLevel, rogue.goldGenerated);
}

// Name of this function is a bit misleading -- basically returns true iff the item will stack without consuming an extra slot
// i.e. if it's a throwing weapon with a sibling already in your pack. False for potions and scrolls.
boolean itemWillStackWithPack(item *theItem) {
	item *tempItem;
	if (!(theItem->quiverNumber)) {
		return false;
	} else {
		for (tempItem = packItems->nextItem;
			 tempItem != NULL && tempItem->quiverNumber != theItem->quiverNumber;
			 tempItem = tempItem->nextItem);
		return (tempItem ? true : false);
	}
}

void removeItemFrom(short x, short y) {
	short layer;
	
	pmap[x][y].flags &= ~HAS_ITEM;
	
	if (cellHasTMFlag(x, y, TM_PROMOTES_ON_ITEM_PICKUP)) {
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[x][y].layers[layer]].mechFlags & TM_PROMOTES_ON_ITEM_PICKUP) {
				promoteTile(x, y, layer, false);
			}
		}
	}
}

// adds the item at (x,y) to the pack
void pickUpItemAt(short x, short y) {
	item *theItem;
	char buf[COLS*3], buf2[COLS*3];
	
	rogue.disturbed = true;
	
	// find the item
	theItem = itemAtLoc(x, y);
	
	if (!theItem) {
		message("Error: Expected item; item not found.", true);
		return;
	}
	
	if ((theItem->flags & ITEM_KIND_AUTO_ID)
        && tableForItemCategory(theItem->category)
		&& !(tableForItemCategory(theItem->category)[theItem->kind].identified)) {
        
        identifyItemKind(theItem);
	}
	
	if (numberOfItemsInPack() < MAX_PACK_ITEMS || (theItem->category & GOLD) || itemWillStackWithPack(theItem)) {
		// remove from floor chain
		pmap[x][y].flags &= ~ITEM_DETECTED;
		
#ifdef BROGUE_ASSERTS
		assert(removeItemFromChain(theItem, floorItems));
#else
		removeItemFromChain(theItem, floorItems);
#endif
		
		if (theItem->category & GOLD) {
			rogue.gold += theItem->quantity; 
			sprintf(buf, "你找到了%i个金币。", theItem->quantity);
			messageWithColor(buf, &itemMessageColor, false);
			deleteItem(theItem);
			removeItemFrom(x, y); // triggers tiles with T_PROMOTES_ON_ITEM_PICKUP
			return;
		}
		
		if ((theItem->category & AMULET) && numberOfMatchingPackItems(AMULET, 0, 0, false)) {
			message("你已经拿到了Amulet of Yendor。", false); 
			deleteItem(theItem);
			return;
		}
		
		theItem = addItemToPack(theItem);
		
		itemName(theItem, buf2, true, true, NULL); // include suffix, article
		
		sprintf(buf, "你获得了%s（%c）。", buf2, theItem->inventoryLetter);
		messageWithColor(buf, &itemMessageColor, false);
		
		removeItemFrom(x, y); // triggers tiles with T_PROMOTES_ON_ITEM_PICKUP
	} else {
		theItem->flags |= ITEM_PLAYER_AVOIDS; // explore shouldn't try to pick it up more than once.
		itemName(theItem, buf2, false, true, NULL); // include article
		sprintf(buf, "你的身上已没有位置来存放%s。", buf2);
		message(buf, false);
	}
}

void conflateItemCharacteristics(item *newItem, item *oldItem) {
    
    // let magic detection and other flags propagate to the new stack...
    newItem->flags |= (oldItem->flags & (ITEM_MAGIC_DETECTED | ITEM_IDENTIFIED | ITEM_PROTECTED | ITEM_RUNIC
                                         | ITEM_RUNIC_HINTED | ITEM_CAN_BE_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN));
    
    // keep the higher enchantment and lower strength requirement...
    if (oldItem->enchant1 > newItem->enchant1) {
        newItem->enchant1 = oldItem->enchant1;
    }
    if (oldItem->strengthRequired < newItem->strengthRequired) {
        newItem->strengthRequired = oldItem->strengthRequired;
    }
    // Copy the inscription.
    if (oldItem->inscription && !newItem->inscription) {
        strcpy(newItem->inscription, oldItem->inscription);
    }
}

void stackItems(item *newItem, item *oldItem) {
    //Increment the quantity of the old item...
    newItem->quantity += oldItem->quantity;
    
    // ...conflate attributes...
    conflateItemCharacteristics(newItem, oldItem);
    
    // ...and delete the new item.
    deleteItem(oldItem);
}

item *addItemToPack(item *theItem) {
	item *previousItem, *tempItem;
	char itemLetter;
	
	// Can the item stack with another in the inventory?
	if (theItem->category & (FOOD|POTION|SCROLL|GEM)) {
		for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
			if (theItem->category == tempItem->category && theItem->kind == tempItem->kind) {
				// We found a match!
                stackItems(tempItem, theItem);
				
				// Pass back the incremented (old) item. No need to add it to the pack since it's already there.
				return tempItem;
			}
		}
	} else if (theItem->category & WEAPON && theItem->quiverNumber > 0) {
		for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
			if (theItem->category == tempItem->category && theItem->kind == tempItem->kind
				&& theItem->quiverNumber == tempItem->quiverNumber) {
				// We found a match!
                stackItems(tempItem, theItem);
				
				// Pass back the incremented (old) item. No need to add it to the pack since it's already there.
				return tempItem;
			}
		}
	}
	
	// assign a reference letter to the item
	itemLetter = nextAvailableInventoryCharacter();
	if (itemLetter) {
		theItem->inventoryLetter = itemLetter;
	}
	
	// insert at proper place in pack chain
	for (previousItem = packItems;
		 previousItem->nextItem != NULL && previousItem->nextItem->category <= theItem->category;
		 previousItem = previousItem->nextItem);
	theItem->nextItem = previousItem->nextItem;
	previousItem->nextItem = theItem;
	
	return theItem;
}

short numberOfItemsInPack() {
	short theCount = 0;
	item *theItem;
	for(theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		theCount += (theItem->category & WEAPON ? 1 : theItem->quantity);
	}
	return theCount;
}

char nextAvailableInventoryCharacter() {
	boolean charTaken[26];
	short i;
	item *theItem;
	char c;
	for(i=0; i<26; i++) {
		charTaken[i] = false;
	}
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		c = theItem->inventoryLetter;
		if (c >= 'a' && c <= 'z') {
			charTaken[c - 'a'] = true;
		}
	}
	for(i=0; i<26; i++) {
		if (!charTaken[i]) {
			return ('a' + i);
		}
	}
	return 0;
}

void updateFloorItems() {
    short x, y, loc[2];
    char buf[DCOLS*3], buf2[DCOLS*3];
    enum dungeonLayers layer;
    item *theItem, *nextItem;
    
	for (theItem=floorItems->nextItem; theItem != NULL; theItem = nextItem) {
		nextItem = theItem->nextItem;
        x = theItem->xLoc;
        y = theItem->yLoc;
        if ((cellHasTerrainFlag(x, y, T_IS_FIRE) && theItem->flags & ITEM_FLAMMABLE)
            || (cellHasTerrainFlag(x, y, T_LAVA_INSTA_DEATH) && theItem->category != AMULET)) {
            
            burnItem(theItem);
            continue;
        }
        if (cellHasTerrainFlag(x, y, T_MOVES_ITEMS)) {
            getQualifyingLocNear(loc, x, y, true, 0, (T_OBSTRUCTS_ITEMS | T_OBSTRUCTS_PASSABILITY), (HAS_ITEM), false, false);
            removeItemFrom(x, y);
            pmap[loc[0]][loc[1]].flags |= HAS_ITEM;
            if (pmap[x][y].flags & ITEM_DETECTED) {
                pmap[x][y].flags &= ~ITEM_DETECTED;
                pmap[loc[0]][loc[1]].flags |= ITEM_DETECTED;
            }
            theItem->xLoc = loc[0];
            theItem->yLoc = loc[1];
            refreshDungeonCell(x, y);
            refreshDungeonCell(loc[0], loc[1]);
            continue;
        }
        if (cellHasTerrainFlag(x, y, T_AUTO_DESCENT)) {
            
            if (playerCanSeeOrSense(x, y)) {
                itemName(theItem, buf, false, false, NULL);
                sprintf(buf2, "这件%s被冲到了你视野之外！", buf);
                messageWithColor(buf2, &itemMessageColor, false);
            }
            theItem->flags |= ITEM_PREPLACED;
            
            // Remove from item chain.
            removeItemFromChain(theItem, floorItems);
            
            pmap[x][y].flags &= ~(HAS_ITEM | ITEM_DETECTED);
            
            if (theItem->category == POTION || rogue.depthLevel == DEEPEST_LEVEL) {
                // Potions don't survive the fall.
                deleteItem(theItem);
            } else {
                // Add to next level's chain.
                theItem->nextItem = levels[rogue.depthLevel-1 + 1].items;
                levels[rogue.depthLevel-1 + 1].items = theItem;
            }
            refreshDungeonCell(x, y);
            continue;
        }
        if (cellHasTMFlag(x, y, TM_PROMOTES_ON_STEP)) {
            for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
                if (tileCatalog[pmap[x][y].layers[layer]].mechFlags & TM_PROMOTES_ON_STEP) {
                    promoteTile(x, y, layer, false);
                }
            }
            continue;
        }
        if (pmap[x][y].machineNumber
            && pmap[x][y].machineNumber == pmap[player.xLoc][player.yLoc].machineNumber
            && (theItem->flags & ITEM_KIND_AUTO_ID)) {
            
            identifyItemKind(theItem);
        }
    }
}

boolean inscribeItem(item *theItem) {
	char itemText[30*3], buf[COLS*3], nameOfItem[COLS*3], oldInscription[COLS*3];
	
	strcpy(oldInscription, theItem->inscription);
	theItem->inscription[0] = '\0';
	itemName(theItem, nameOfItem, true, true, NULL);
	strcpy(theItem->inscription, oldInscription);
	
	sprintf(buf, "命名: %s \"", nameOfItem);
	if (getInputTextString(itemText, buf, min(29, DCOLS - strLenWithoutEscapes(buf) - 1), "", "\"", TEXT_INPUT_NORMAL, false)) {
		strcpy(theItem->inscription, itemText);
		confirmMessages();
		itemName(theItem, nameOfItem, true, true, NULL);
		nameOfItem[strlen(nameOfItem) - 1] = '\0';
		sprintf(buf, "%s%s。\"", (theItem->quantity > 1 ? "这些现在被叫做" : "它现在被叫做"), nameOfItem);
		messageWithColor(buf, &itemMessageColor, false);
		return true;
	} else {
		confirmMessages();
		return false;
	}
}

boolean itemCanBeCalled(item *theItem) {
	if ((theItem->flags & ITEM_IDENTIFIED) || theItem->category & (WEAPON|ARMOR|CHARM|FOOD|GOLD|AMULET|GEM)) {
		if (theItem->category & (WEAPON | ARMOR | CHARM | STAFF | WAND | RING)) {
			return true;
		} else {
			return false;
		}
	}
	if ((theItem->category & (POTION|SCROLL|WAND|STAFF|RING))
		&& !(tableForItemCategory(theItem->category)[theItem->kind].identified)) {
		return true;
	}
	return false;
}

void call(item *theItem) {
	char itemText[30], buf[COLS];
	short c;
	unsigned char command[100];
    item *tempItem;
    
	c = 0;
	command[c++] = CALL_KEY;
	if (theItem == NULL) {
        // Need to gray out known potions and scrolls from inventory selection.
        // Hijack the "item can be identified" flag for this purpose,
        // and then reset it immediately afterward.
        for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
            if ((tempItem->category & (POTION | SCROLL))
                && tableForItemCategory(tempItem->category)[tempItem->kind].identified) {
                
                tempItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
            } else {
                tempItem->flags |= ITEM_CAN_BE_IDENTIFIED;
            }
        }
		theItem = promptForItemOfType((WEAPON|ARMOR|SCROLL|RING|POTION|STAFF|WAND), ITEM_CAN_BE_IDENTIFIED, 0,
									  "选择要命名的物品：（a-z, 按住<shift>键弹出物品菜单。<esc>键取消）", true);
        updateIdentifiableItems(); // Reset the flags.
	}
	if (theItem == NULL) {
		return;
	}
	
	command[c++] = theItem->inventoryLetter;
	
	confirmMessages();
	
	if ((theItem->flags & ITEM_IDENTIFIED) || theItem->category & (WEAPON|ARMOR|CHARM|FOOD|GOLD|AMULET|GEM)) {
		if (theItem->category & (WEAPON | ARMOR | CHARM | STAFF | WAND | RING)) {
			if (inscribeItem(theItem)) {
				command[c++] = '\0';
				strcat((char *) command, theItem->inscription);
				recordKeystrokeSequence(command);
				recordKeystroke(RETURN_KEY, false, false);
			}
		} else {
			message("你已经知道了这种物品的真实功效。", false);
		}
		return;
	}
	
	if (theItem->category & (WEAPON | ARMOR | STAFF | WAND | RING)) {
        if (tableForItemCategory(theItem->category)[theItem->kind].identified) {
			if (inscribeItem(theItem)) {
				command[c++] = '\0';
				strcat((char *) command, theItem->inscription);
				recordKeystrokeSequence(command);
				recordKeystroke(RETURN_KEY, false, false);
			}
        } else if (confirm("命名这单件物品，而不是所有类似的物品？", true)) {
			command[c++] = 'y'; // y means yes, since the recording also needs to negotiate the above confirmation prompt.
			if (inscribeItem(theItem)) {
				command[c++] = '\0';
				strcat((char *) command, theItem->inscription);
				recordKeystrokeSequence(command);
				recordKeystroke(RETURN_KEY, false, false);
			}
			return;
		} else {
			command[c++] = 'n'; // n means no
		}
	}
	
	if (tableForItemCategory(theItem->category)
        && !(tableForItemCategory(theItem->category)[theItem->kind].identified)
        && getInputTextString(itemText, "把它们命名为： \"", 29, "", "\"", TEXT_INPUT_NORMAL, false)) {
        
		command[c++] = '\0';
		strcat((char *) command, itemText);
		recordKeystrokeSequence(command);
		recordKeystroke(RETURN_KEY, false, false);
		if (itemText[0]) {
			strcpy(tableForItemCategory(theItem->category)[theItem->kind].callTitle, itemText);
			tableForItemCategory(theItem->category)[theItem->kind].called = true;
		} else {
			tableForItemCategory(theItem->category)[theItem->kind].callTitle[0] = '\0';
			tableForItemCategory(theItem->category)[theItem->kind].called = false;
		}
		confirmMessages();
		itemName(theItem, buf, false, true, NULL);
		messageWithColor(buf, &itemMessageColor, false);
	} else {
        message("你已经知道了这种物品的真实功效。", false);
	}
}

// Generates the item name and returns it in the "root" string.
// IncludeDetails governs things such as enchantment, charges, strength requirement, times used, etc.
// IncludeArticle governs the article -- e.g. "some" food, "5" darts, "a" pink potion.
// If baseColor is provided, then the suffix will be in gray, flavor portions of the item name (e.g. a "pink" potion,
//	a "sandalwood" staff, a "ruby" ring) will be in dark purple, and the Amulet of Yendor and lumenstones will be in yellow.
//  BaseColor itself will be the color that the name reverts to outside of these colored portions.
void itemName(item *theItem, char *root, boolean includeDetails, boolean includeArticle, color *baseColor) {
	char buf[DCOLS*3], pluralization[10], article[10*3] = "",
	grayEscapeSequence[5], purpleEscapeSequence[5], yellowEscapeSequence[5], baseEscapeSequence[5];
	color tempColor;
	
	// strcpy(pluralization, (theItem->quantity > 1 ? "s" : ""));
	strcpy(pluralization, "");
	
	grayEscapeSequence[0] = '\0';
	purpleEscapeSequence[0] = '\0';
	yellowEscapeSequence[0] = '\0';
	baseEscapeSequence[0] = '\0';
	if (baseColor) {
		tempColor = backgroundMessageColor;
		applyColorMultiplier(&tempColor, baseColor); // To gray out the purle if necessary.
		encodeMessageColor(purpleEscapeSequence, 0, &tempColor);
		
		tempColor = gray;
		//applyColorMultiplier(&tempColor, baseColor);
		encodeMessageColor(grayEscapeSequence, 0, &tempColor);
        
		tempColor = itemMessageColor;
		applyColorMultiplier(&tempColor, baseColor);
		encodeMessageColor(yellowEscapeSequence, 0, &tempColor);
		
		encodeMessageColor(baseEscapeSequence, 0, baseColor);
	}
	
	switch (theItem -> category) {
		case FOOD:
			if (theItem -> kind == FRUIT) {
				sprintf(root, "芒果");
			} else {
				if (theItem->quantity == 1) {
					sprintf(article, "一份");
					sprintf(root, "食物");
				} else {
					sprintf(root, "粮食");
				}
			}
			break;
		case WEAPON:
			sprintf(root, "%s%s", weaponTable[theItem->kind].name, pluralization);
			if (includeDetails) {
				if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
					sprintf(buf, " %s%i %s", (theItem->enchant1 < 0 ? "" : "+"), theItem->enchant1, root);
					strcpy(root, buf);
				}
				
				if (theItem->flags & ITEM_RUNIC) {
					if ((theItem->flags & ITEM_RUNIC_IDENTIFIED) || rogue.playbackOmniscience) {
						if (theItem->enchant2 == W_SLAYING) {
							sprintf(root, "%s(屠戳%s)%s",
									root,
									monsterCatalog[theItem->vorpalEnemy].monsterName,
									grayEscapeSequence);
						} else {
							sprintf(root, "%s(%s)%s",
									root,
									weaponRunicNames[theItem->enchant2],
									grayEscapeSequence);
						}
					} else if (theItem->flags & (ITEM_IDENTIFIED | ITEM_RUNIC_HINTED)) {
						if (grayEscapeSequence[0]) {
							strcat(root, grayEscapeSequence);
						}
						strcat(root, "(未知效果)");
					}
				}
				sprintf(root, "%s%s <%i>", root, grayEscapeSequence, theItem->strengthRequired);
			}
			break;
		case ARMOR:
			sprintf(root, "%s", armorTable[theItem->kind].name);
			if (includeDetails) {
				
				if ((theItem->flags & ITEM_RUNIC)
					&& ((theItem->flags & ITEM_RUNIC_IDENTIFIED)
						|| rogue.playbackOmniscience)) {
						if (theItem->enchant2 == A_IMMUNITY) {
							sprintf(root, "%s(免疫%s)", root, monsterCatalog[theItem->vorpalEnemy].monsterName);
						} else {
							sprintf(root, "%s(%s)", root, armorRunicNames[theItem->enchant2]);
						}
					}
				
				if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
					if (theItem->enchant1 == 0) {
						sprintf(buf, "%s%s [%i]<%i>", root, grayEscapeSequence, theItem->armor/10, theItem->strengthRequired);
					} else {
						sprintf(buf, " %s%i %s%s [%i]<%i>",
								(theItem->enchant1 < 0 ? "" : "+"),
								theItem->enchant1,
								root,
								grayEscapeSequence,
								theItem->armor/10 + theItem->enchant1,
								theItem->strengthRequired);
					}
					strcpy(root, buf);
				} else {
					sprintf(root, "%s%s <%i>", root, grayEscapeSequence, theItem->strengthRequired);
				}
				
				if ((theItem->flags & ITEM_RUNIC)
					&& (theItem->flags & (ITEM_IDENTIFIED | ITEM_RUNIC_HINTED))
					&& !(theItem->flags & ITEM_RUNIC_IDENTIFIED)
					&& !rogue.playbackOmniscience) {
					strcat(root, "(未知效果)");
				}
			}
			break;
		case SCROLL:
			if (scrollTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "%s卷轴", scrollTable[theItem->kind].name);
			} else if (scrollTable[theItem->kind].called) {
				sprintf(root, "%s%s%s",
						purpleEscapeSequence,
						scrollTable[theItem->kind].callTitle,
						baseEscapeSequence);
			} else {
				sprintf(root, "%s\"%s\"%s卷轴",
						purpleEscapeSequence,
						scrollTable[theItem->kind].flavor,
						baseEscapeSequence);
			}
			break;
		case POTION:
			if (potionTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "%s药剂", potionTable[theItem->kind].name);
			} else if (potionTable[theItem->kind].called) {
				sprintf(root, "%s%s%s药剂",
						purpleEscapeSequence,
						potionTable[theItem->kind].callTitle,
						baseEscapeSequence);
			} else {
				sprintf(root, "%s%s%s药剂",
						purpleEscapeSequence,
						potionTable[theItem->kind].flavor,
						baseEscapeSequence
						);
			}
			break;
		case WAND:
			if (wandTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "%s魔棒",
						wandTable[theItem->kind].name);
			} else if (wandTable[theItem->kind].called) {
				sprintf(root, "%s%s%s魔棒",
						purpleEscapeSequence,
						wandTable[theItem->kind].callTitle,
						baseEscapeSequence);
			} else {
				sprintf(root, "%s%s%s魔棒",
						purpleEscapeSequence,
						wandTable[theItem->kind].flavor,
						baseEscapeSequence
						);
			}
			if (includeDetails) {
				if (theItem->flags & (ITEM_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN) || rogue.playbackOmniscience) {
					sprintf(root, "%s%s [%i]",
							root,
							grayEscapeSequence,
							theItem->charges);
				} else if (theItem->enchant2) {
					sprintf(root, "%s%s (已使用%i次)",
							root,
							grayEscapeSequence,
							theItem->enchant2);
				// } else if (theItem->enchant2) {
				// 	sprintf(root, "%s%s (已使用 %s)",
				// 			root,
				// 			grayEscapeSequence,
				// 			(theItem->enchant2 == 2 ? "twice" : "once"));
				}
			}
			break;
		case STAFF:
			if (staffTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "%s法杖", staffTable[theItem->kind].name);
			} else if (staffTable[theItem->kind].called) {
				sprintf(root, "%s%s%s法杖",
						purpleEscapeSequence,
						staffTable[theItem->kind].callTitle,
						baseEscapeSequence);
			} else {
				sprintf(root, "%s%s%s法杖",
						purpleEscapeSequence,
						staffTable[theItem->kind].flavor,
						baseEscapeSequence
						);
			}
			if (includeDetails) {
				if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
					sprintf(root, "%s%s [%i/%i]", root, grayEscapeSequence, theItem->charges, theItem->enchant1);
				} else if (theItem->flags & ITEM_MAX_CHARGES_KNOWN) {
					sprintf(root, "%s%s [?/%i]", root, grayEscapeSequence, theItem->enchant1);
				}
			}
			break;
		case RING:
			if (ringTable[theItem->kind].identified || rogue.playbackOmniscience) {
				sprintf(root, "%s指环", ringTable[theItem->kind].name);
			} else if (ringTable[theItem->kind].called) {
				sprintf(root, "%s%s%s指环",
						purpleEscapeSequence,
						ringTable[theItem->kind].callTitle,
						baseEscapeSequence);
			} else {
				sprintf(root, "%s%s%s指环",
						purpleEscapeSequence,
						ringTable[theItem->kind].flavor,
						baseEscapeSequence
						);
			}
			if (includeDetails && ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience)) {
				sprintf(buf, " %s%i %s", (theItem->enchant1 < 0 ? "" : "+"), theItem->enchant1, root);
				strcpy(root, buf);
			}
			break;
		case CHARM:
            sprintf(root, "%s魔导器", charmTable[theItem->kind].name);
            
			if (includeDetails) {
				sprintf(buf, " %s%i %s", (theItem->enchant1 < 0 ? "" : "+"), theItem->enchant1, root);
				strcpy(root, buf);
                
                if (theItem->charges) {
                    sprintf(buf, "%s %s(%i%%)",
                            root,
                            grayEscapeSequence,
                            (charmRechargeDelay(theItem->kind, theItem->enchant1) - theItem->charges) * 100 / charmRechargeDelay(theItem->kind, theItem->enchant1));
                    strcpy(root, buf);
                } else {
                    strcat(root, grayEscapeSequence);
                    strcat(root, " (可以使用)");
                }
			}
			break;
		case GOLD:
			sprintf(root, "金币");
			break;
		case AMULET:
			sprintf(root, "%sAmulet%s of Yendor%s", yellowEscapeSequence, pluralization, baseEscapeSequence);
			break;
		case GEM:
			sprintf(root, "%s宝石%s", yellowEscapeSequence, baseEscapeSequence);
			break;
		case KEY:
			if (includeDetails && theItem->keyZ > 0 && theItem->keyZ != rogue.depthLevel) {
				sprintf(root, "%s%s(在第%i层捡到)",
						keyTable[theItem->kind].name,
						grayEscapeSequence,
						theItem->keyZ);
			} else {
				sprintf(root,
						keyTable[theItem->kind].name,
						"%s%s",
						pluralization);
			}
			break;
		default:
			sprintf(root, "未知物品");
			break;
	}
	
	if (includeArticle) {
		// prepend number if quantity is over 1
		if (theItem->quantity > 1) {
			sprintf(article, "%i", theItem->quantity);
		} else if (theItem->category & AMULET) {
			sprintf(article, "");
		} else if (!(theItem->category & ARMOR) && !(theItem->category & FOOD && theItem->kind == RATION)) {
			// otherwise prepend a/an if the item is not armor and not a ration of food;
			// armor gets no article, and "some food" was taken care of above.
			sprintf(article, "一");
		}
		switch (theItem -> category)
		{
			case WEAPON:
				strcat(article, "把"); break;
			case SCROLL:
				strcat(article, "张"); break;
			case POTION:
				strcat(article, "瓶"); break;
			case WAND:
				strcat(article, "只"); break;
			case STAFF:
				strcat(article, "把"); break;
			case RING:
				strcat(article, "只"); break;
			case CHARM:
				strcat(article, "件"); break;
			case GOLD:
				strcat(article, "个"); break;
			case KEY:
				strcat(article, "把"); break;
			case AMULET:
			case ARMOR:
			case FOOD:
			default:
				// pass
				break;
		}
	}
	// strcat(buf, suffixID);
	if (includeArticle) {
		sprintf(buf, "%s%s", article, root);
		strcpy(root, buf);
	}
	
	if (includeDetails && theItem->inscription[0]) {
        sprintf(buf, "%s \"%s\"", root, theItem->inscription);
        strcpy(root, buf);
	}
	return;
}

itemTable *tableForItemCategory(enum itemCategory theCat) {
	switch (theCat) {
		case FOOD:
			return foodTable;
		case WEAPON:
			return weaponTable;
		case ARMOR:
			return armorTable;
		case POTION:
			return potionTable;
		case SCROLL:
			return scrollTable;
		case RING:
			return ringTable;
		case WAND:
			return wandTable;
		case STAFF:
			return staffTable;
		case CHARM:
			return charmTable;
		default:
			return NULL;
	}
}

boolean isVowelish(char *theChar) {
    short i;
    
	while (*theChar == COLOR_ESCAPE) {
		theChar += 4;
	}
    char str[30];
    strncpy(str, theChar, 29);
    for (i = 0; i < 30; i++) {
        upperCase(&(str[1]));
    }
    if (stringsMatch(str, "UNI")        // Words that start with "uni" aren't treated like vowels; e.g., "a" unicorn.
        || stringsMatch(str, "EU")) {   // Words that start with "eu" aren't treated like vowels; e.g., "a" eucalpytus staff.
        
        return false;
    } else {
        return (str[0] == 'A'
                || str[0] == 'E'
                || str[0] == 'I'
                || str[0] == 'O'
                || str[0] == 'U');
    }
}

short charmEffectDuration(short charmKind, short enchant) {
    const short duration[NUMBER_CHARM_KINDS] = {
        3,  // Health
        20, // Protection
        7,  // Haste
        10, // Fire immunity
        5,  // Invisibility
        25, // Telepathy
        10, // Levitation
        0,  // Shattering
        0,  // Teleportation
        0,  // Recharging
        0,  // Negation
    };
    const short increment[NUMBER_CHARM_KINDS] = {
        0,  // Health
        0,  // Protection
        20, // Haste
        25, // Fire immunity
        20, // Invisibility
        25, // Telepathy
        25, // Levitation
        0,  // Shattering
        0,  // Teleportation
        0,  // Recharging
        0,  // Negation
    };
    
    return duration[charmKind] * (pow((double) (100 + (increment[charmKind])) / 100, enchant) + FLOAT_FUDGE);
}

short charmRechargeDelay(short charmKind, short enchant) {
    const short duration[NUMBER_CHARM_KINDS] = {
        2500,   // Health
        1000,   // Protection
        800,    // Haste
        800,    // Fire immunity
        800,    // Invisibility
        800,    // Telepathy
        800,    // Levitation
        2500,   // Shattering
        1000,   // Teleportation
        10000,  // Recharging
        2500,   // Negation
    };
    const short increment[NUMBER_CHARM_KINDS] = {
//        35, // Health
//        30, // Protection
//        25, // Haste
//        25, // Fire immunity
//        20, // Invisibility
//        30, // Telepathy
//        25, // Levitation
//        40, // Shattering
//        20, // Teleportation
//        30, // Recharging
//        25, // Negation
        45, // Health
        40, // Protection
        35, // Haste
        40, // Fire immunity
        35, // Invisibility
        35, // Telepathy
        35, // Levitation
        40, // Shattering
        45, // Teleportation
        40, // Recharging
        40, // Negation
    };
    
    return charmEffectDuration(charmKind, enchant) + duration[charmKind] * (pow((double) (100 - (increment[charmKind])) / 100, enchant) + FLOAT_FUDGE);
}

float enchantIncrement(item *theItem) {
	if (theItem->category & (WEAPON | ARMOR)) {
		if (theItem->strengthRequired == 0) {
			return 1 + 0;
		} else if (rogue.strength - player.weaknessAmount < theItem->strengthRequired) {
			return 1 + 2.5;
		} else {
			return 1 + 0.25;
		}
	} else {
		return 1 + 0;
	}
}

boolean itemIsCarried(item *theItem) {
	item *tempItem;
	
	for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
		if (tempItem == theItem) {
			return true;
		}
	}
	return false;
}

void itemDetails(char *buf, item *theItem) {
	char buf2[1000], buf3[1000], theName[100], goodColorEscape[20], badColorEscape[20], whiteColorEscape[20];
	boolean singular, carried;
	float enchant;
	short nextLevelState = 0, new;
	float accuracyChange, damageChange, current, currentDamage, newDamage;
	const char weaponRunicEffectDescriptions[NUMBER_WEAPON_RUNIC_KINDS][DCOLS*3] = {
		"在一回合以进行两次攻击",
		"秒杀目标",
		"麻痹目标",
		"[]", // never used
		"减速目标",
		"使目标陷入混乱状态",
        "击飞目标",
		"[]", // never used
		"使目标恢复体力",
		"复制目标"
	};
    
    goodColorEscape[0] = badColorEscape[0] = whiteColorEscape[0] = '\0';
    encodeMessageColor(goodColorEscape, 0, &goodMessageColor);
    encodeMessageColor(badColorEscape, 0, &badMessageColor);
    encodeMessageColor(whiteColorEscape, 0, &white);
	
	singular = (theItem->quantity == 1 ? true : false);
	carried = itemIsCarried(theItem);
	
	// Name
	itemName(theItem, theName, true, true, NULL);
	buf[0] = '\0';
	encodeMessageColor(buf, 0, &itemMessageColor);
	upperCase(theName);
	strcat(buf, theName);
	if (carried) {
		sprintf(buf2, " (%c)", theItem->inventoryLetter);
		strcat(buf, buf2);
	}
	encodeMessageColor(buf, strlen(buf), &white);
	strcat(buf, "\n\n");
	
	itemName(theItem, theName, false, false, NULL);
	
	// introductory text
	if (tableForItemCategory(theItem->category)
		&& (tableForItemCategory(theItem->category)[theItem->kind].identified || rogue.playbackOmniscience)) {
		
		strcat(buf, tableForItemCategory(theItem->category)[theItem->kind].description);
        
        if (theItem->category == POTION && theItem->kind == POTION_LIFE) {
            sprintf(buf2, "\n\n能够提高你%s%i%%%s的生命值上限。",
                    goodColorEscape,
                    (player.info.maxHP + 10) * 100 / player.info.maxHP - 100,
                    whiteColorEscape);
            strcat(buf, buf2);
        }
	} else {
		switch (theItem->category) {
			case POTION:
				sprintf(buf2, "这种烧瓶里装着%s的液体。 \
不知道喝下去或者投掷出去会有什么效果。",
						tableForItemCategory(theItem->category)[theItem->kind].flavor
						);
				break;
			case SCROLL:
				sprintf(buf2, "这种卷轴上写满了熟悉的文字，但却难以看懂是什么意思。卷轴抬头上写着\"%s.\" \
不知道把这些文字大声读出来会有什么效果。",
						tableForItemCategory(theItem->category)[theItem->kind].flavor
						);
				break;
			case STAFF:
				sprintf(buf2, "由整根树干制成的%s法杖握在手里能感觉到微热\
不知道使用后会有什么效果。",
						tableForItemCategory(theItem->category)[theItem->kind].flavor);
				break;
			case WAND:
				sprintf(buf2, "这种细长的%s魔棒握在手里能感觉到微热\
不知道使用后会有什么效果。",
						tableForItemCategory(theItem->category)[theItem->kind].flavor);
				break;
			case RING:
				sprintf(buf2, "这种指环上镶嵌一块%s，在黑暗都闪着微弱的光芒\
不知道戴上后会有什么效果。",
						tableForItemCategory(theItem->category)[theItem->kind].flavor);
				break;
			case CHARM: // Should never be displayed.
				strcat(buf2, "一件神器的法器！");
				break;
			case AMULET:
				strcpy(buf2, "这就是传说中的护身符。\
成千上万的冒险者为了需找它丢了性命，传说只要能把它带到地面上\
就能享用无尽的荣华富贵。");
				break;
			case GEM:
				sprintf(buf2, "半透明的宝石内不断闪动着神奇的光芒。\
传说这种宝石内蕴藏着神奇的力量，但对你来说更大的意义是它能卖很多钱。");
				break;
			case KEY:
				strcpy(buf2, keyTable[theItem->kind].description);
				break;
			case GOLD:
				sprintf(buf2, "%i个闪闪发光的金币，摞成了一堆。", theItem->quantity);
				break;
			default:
				break;
		}
		strcat(buf, buf2);
	}
	
	// detailed description
	switch (theItem->category) {
			
		case FOOD:
			sprintf(buf2, "\n\n你还没有那么饿，现在吃掉它的话有些浪费了。");
			strcat(buf, buf2);
			break;
			
		case WEAPON:
		case ARMOR:
			// enchanted? strength modifier?
			if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
				if (theItem->enchant1) {
					sprintf(buf2, "\n\n这种%s带有%s%i点%s%s%s",
							theName,
                            (theItem->enchant1 > 0 ? goodColorEscape : badColorEscape),
							theItem->enchant1,
							whiteColorEscape,
							(theItem->enchant1 > 0 ? "的增强效果" : "的负面效果")
                            );
				} else {
					sprintf(buf2, "\n\n这种%s没有附加效果",
							theName);
				}
				strcat(buf, buf2);
				if (strengthModifier(theItem)) {
					sprintf(buf2, "，%s由于你的力量%s，其%s效果%s%s了%.2f%s。",
							(theItem->enchant1 ? "而且" : "但是"),
							(strengthModifier(theItem) > 0 ? "富余" : "不足"),
							(theItem->category == WEAPON ? "攻击": "防御"),
                            (strengthModifier(theItem) > 0 ? goodColorEscape : badColorEscape),
							(strengthModifier(theItem) > 0 ? "增加" : "减少"),
							strengthModifier(theItem),
                            whiteColorEscape
							);
					strcat(buf, buf2);
				} else {
					strcat(buf, "，");
				}
			} else {
				if ((theItem->enchant1 > 0) && (theItem->flags & ITEM_MAGIC_DETECTED)) {
					sprintf(buf2, "\n\n你能感觉到一股%s神奇的力量%s不断从%s中涌出。",
                            goodColorEscape,
                            whiteColorEscape,
							theName);
					strcat(buf, buf2);
				}
				if (strengthModifier(theItem)) {
					sprintf(buf2, "\n\n这件%s由于你的力量%s，其%s效果%s%s了%.2f%s。",
							theName,
							(strengthModifier(theItem) > 0 ? "富余" : "不足"),
							(theItem->category == WEAPON ? "攻击": "防御"),
                            (strengthModifier(theItem) > 0 ? goodColorEscape : badColorEscape),
							(strengthModifier(theItem) > 0 ? "增加" : "减少"),
							strengthModifier(theItem),
                            whiteColorEscape);
					strcat(buf, buf2);
				}
				
				if (theItem->category & WEAPON) {
					sprintf(buf2, "这件武器的真实力量在你用它杀死%i个敌人后会显现出来。",
							theItem->charges);
				} else {
					sprintf(buf2, "这件装备的真是力量在你穿着它%i回合后会显现出来。",
							theItem->charges);
				}
				strcat(buf, buf2);
			}
			
			// Display the known percentage by which the armor/weapon will increase/decrease accuracy/damage/defense if not already equipped.
			if (!(theItem->flags & ITEM_EQUIPPED)) {
				if (theItem->category & WEAPON) {
					current = player.info.accuracy;
					if (rogue.weapon) {
                        currentDamage = (((float) rogue.weapon->damage.lowerBound) + ((float) rogue.weapon->damage.upperBound)) / 2;
						if ((rogue.weapon->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
							current *= pow(WEAPON_ENCHANT_ACCURACY_FACTOR, netEnchant(rogue.weapon));
							currentDamage *= pow(WEAPON_ENCHANT_DAMAGE_FACTOR, netEnchant(rogue.weapon));
						} else {
							current *= pow(WEAPON_ENCHANT_ACCURACY_FACTOR, strengthModifier(rogue.weapon));
							currentDamage *= pow(WEAPON_ENCHANT_DAMAGE_FACTOR, strengthModifier(rogue.weapon));
						}
					} else {
                        currentDamage = (((float) player.info.damage.lowerBound) + ((float) player.info.damage.upperBound)) / 2;
                    }
					
					new = player.info.accuracy;
					newDamage = (((float) theItem->damage.lowerBound) + ((float) theItem->damage.upperBound)) / 2;
					if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
						new *= pow(WEAPON_ENCHANT_ACCURACY_FACTOR, netEnchant(theItem));
						newDamage *= pow(WEAPON_ENCHANT_DAMAGE_FACTOR, netEnchant(theItem));
					} else {
						new *= pow(WEAPON_ENCHANT_ACCURACY_FACTOR, strengthModifier(theItem));
						newDamage *= pow(WEAPON_ENCHANT_DAMAGE_FACTOR, strengthModifier(theItem));
					}
					accuracyChange	= (new * 100 / current) - 100 + FLOAT_FUDGE;
					damageChange	= (newDamage * 100 / currentDamage) - 100 + FLOAT_FUDGE;
					sprintf(buf2, "使用这件%s%s将%s你%s%i%%%s的命中率, 同时%s%s%i%%%s的攻击伤害。",
							theName,
							((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) ? "" : "（不考虑隐藏属性的影响）",
							(((short) accuracyChange) < 0) ? "减少" : "增加",
                            (((short) accuracyChange) < 0) ? badColorEscape : (accuracyChange > 0 ? goodColorEscape : ""),
							abs((short) accuracyChange),
                            whiteColorEscape,
							(((short) damageChange) < 0) ? "减少" : "增加",
                            (((short) damageChange) < 0) ? badColorEscape : (damageChange > 0 ? goodColorEscape : ""),
							abs((short) damageChange),
                            whiteColorEscape);
				} else {
					new = theItem->armor;
					if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
						new += 10 * netEnchant(theItem);
					} else {
						new += 10 * strengthModifier(theItem);
					}
					new = max(0, new);
                    new /= 10;
					sprintf(buf2, "这件%s%s的护甲等级为%s%i%s。",
							theName,
							((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) ? "" : "（不考虑隐藏属性的影响）",
                            (new > displayedArmorValue() ? goodColorEscape : (new < displayedArmorValue() ? badColorEscape : whiteColorEscape)),
							(int) (new + FLOAT_FUDGE),
                            whiteColorEscape);
				}
				strcat(buf, buf2);
			}
			
			// protected?
			if (theItem->flags & ITEM_PROTECTED) {
				sprintf(buf2, "%s这件%s不会被酸液腐蚀。%s",
                        goodColorEscape,
						theName,
                        whiteColorEscape);
				strcat(buf, buf2);
			}
			
			if (theItem->category & WEAPON) {
				
				// runic?
				if (theItem->flags & ITEM_RUNIC) {
					if ((theItem->flags & ITEM_RUNIC_IDENTIFIED) || rogue.playbackOmniscience) {
						sprintf(buf2, "\n\n闪亮的%s符文覆盖着这件%s。",
								weaponRunicNames[theItem->enchant2],
								theName);
						strcat(buf, buf2);
						
						// W_SPEED, W_QUIETUS, W_PARALYSIS, W_MULTIPLICITY, W_SLOWING, W_CONFUSION, W_FORCE, W_SLAYING, W_MERCY, W_PLENTY
						
						enchant = netEnchant(theItem);
						if (theItem->enchant2 == W_SLAYING) {
							sprintf(buf2, "它只需要一击就能杀死%s。",
									monsterCatalog[theItem->vorpalEnemy].monsterName);
							strcat(buf, buf2);
						} else if (theItem->enchant2 == W_MULTIPLICITY) {
							if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
								sprintf(buf2, "攻击敌人时你有%i%%的机会, 召唤出%i个%s的奥术幻象。它们带有与该武器相同的属性，在%i回合后会消失。（如果这把%s被增强了, 会以%i%%机会产生%i个额外的镜像，并持续%i回合。）",
										runicWeaponChance(theItem, false, 0),
										weaponImageCount(enchant),
										theName,
										weaponImageDuration(enchant),
										theName,
										runicWeaponChance(theItem, true, (float) (enchant + enchantIncrement(theItem))),
										weaponImageCount((float) (enchant + enchantIncrement(theItem))),
										weaponImageDuration((float) (enchant + enchantIncrement(theItem))));
							} else {
								sprintf(buf2, "在攻击到敌人的时候, %s会召唤出有相同属性的奥术幻象武器来帮助你战斗，它们会在一小段时间后消失。",
										theName);
							}
							strcat(buf, buf2);
						} else {
							if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
                                if (runicWeaponChance(theItem, false, 0) < 2
                                    && rogue.strength - player.weaknessAmount < theItem->strengthRequired) {
                                    
                                    strcpy(buf2, "由于你力量不足，这件武器的特殊效果几乎不会被触发。但在极少的情况下此武器能");
                                } else {
                                    sprintf(buf2, "这件武器能以%i%%的概率",
                                            runicWeaponChance(theItem, false, 0));
                                }
								strcat(buf, buf2);
							} else {
								strcat(buf, "某些情况下这件武器");
							}
							sprintf(buf2, "能%s",
									weaponRunicEffectDescriptions[theItem->enchant2]);
							strcat(buf, buf2);
							
							if ((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience) {
								switch (theItem->enchant2) {
									case W_SPEED:
										strcat(buf, "。");
										break;
									case W_PARALYSIS:
										sprintf(buf2, "，效果持续%i个回合。",
												(int) (weaponParalysisDuration(enchant)));
										strcat(buf, buf2);
										nextLevelState = (int) (weaponParalysisDuration((float) (enchant + enchantIncrement(theItem))) + FLOAT_FUDGE);
										break;
									case W_SLOWING:
										sprintf(buf2, "，效果持续%i个回合。",
												weaponSlowDuration(enchant));
										strcat(buf, buf2);
										nextLevelState = weaponSlowDuration((float) (enchant + enchantIncrement(theItem)));
										break;
									case W_CONFUSION:
										sprintf(buf2, "，效果持续%i个回合。",
												weaponConfusionDuration(enchant));
										strcat(buf, buf2);
										nextLevelState = weaponConfusionDuration((float) (enchant + enchantIncrement(theItem)));
										break;
									case W_FORCE:
										sprintf(buf2, "，使其后退最多%i格。如果敌人撞到了墙壁则会根据被击飞的距离受到伤害。",
												weaponForceDistance(enchant));
										strcat(buf, buf2);
										nextLevelState = weaponForceDistance((float) (enchant + enchantIncrement(theItem)));
										break;
									case W_MERCY:
										strcpy(buf2, "，最多回复其生命值上限的50%%。");
										strcat(buf, buf2);
										break;
									default:
										strcpy(buf2, "。");
										strcat(buf, buf2);
										break;
								}
								
								if (((theItem->flags & ITEM_IDENTIFIED) || rogue.playbackOmniscience)
									&& runicWeaponChance(theItem, false, 0) < runicWeaponChance(theItem, true, (float) (enchant + enchantIncrement(theItem)))){
									sprintf(buf2, "（如果%s被增强，其特效触发几率将增加到%i%%，",
											theName,
											runicWeaponChance(theItem, true, (float) (enchant + enchantIncrement(theItem))));
									strcat(buf, buf2);
									if (nextLevelState) {
                                        if (theItem->enchant2 == W_FORCE) {
                                            sprintf(buf2, "，击飞距离会被提升至%i。）",
                                                    nextLevelState);
                                        } else {
                                            sprintf(buf2, "，效果持续时间会增加至%i回合。）",
                                                    nextLevelState);
                                        }
									} else {
										strcpy(buf2, "。）");
									}
									strcat(buf, buf2);
								}
							} else {
								strcat(buf, "。");
							}
						}
						
					} else if (theItem->flags & ITEM_IDENTIFIED) {
						sprintf(buf2, "\n\n某种神秘的符文刻满了%s。",
								theName);
						strcat(buf, buf2);
					}
				}
				
				// equipped? cursed?
				if (theItem->flags & ITEM_EQUIPPED) {
					sprintf(buf2, "\n\n你正手握着这把%s。",
							theName,
							((theItem->flags & ITEM_CURSED) ? "，而且由于它是被诅咒的，你现在没有办法放开它" : ""));
					strcat(buf, buf2);
				} else if (((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAGIC_DETECTED)) || rogue.playbackOmniscience)
						   && (theItem->flags & ITEM_CURSED)) {
					sprintf(buf2, "\n\n%s你能感觉到这%s里有一股险恶的魔法能量。%s",
                            badColorEscape,
                            theName,
                            whiteColorEscape);
					strcat(buf, buf2);
				}
				
			} else if (theItem->category & ARMOR) {
				
				// runic?
				if (theItem->flags & ITEM_RUNIC) {
					if ((theItem->flags & ITEM_RUNIC_IDENTIFIED) || rogue.playbackOmniscience) {
						sprintf(buf2, "\n\n闪亮的%s符文覆盖着这件%s。",
								armorRunicNames[theItem->enchant2],
								theName);
						strcat(buf, buf2);
						
						// A_MULTIPLICITY, A_MUTUALITY, A_ABSORPTION, A_REPRISAL, A_IMMUNITY, A_REFLECTION, A_BURDEN, A_VULNERABILITY, A_IMMOLATION
						enchant = netEnchant(theItem);
						switch (theItem->enchant2) {
							case A_MULTIPLICITY:
								sprintf(buf2, "这件护甲在被敌人攻击时能以33%%的几率召唤出%i个敌人的幻象来帮助你攻击。幻象持续3回合。",
										armorImageCount(enchant));
								if (armorImageCount((float) enchant + enchantIncrement(theItem)) > armorImageCount(enchant)) {
									sprintf(buf3, "（如果%s被增强了，幻象的数量会增加到%i。）",
											theName,
											(armorImageCount((float) enchant + enchantIncrement(theItem))));
									strcat(buf2, buf3);
								}
								break;
							case A_MUTUALITY:
								strcpy(buf2, "这件护甲能使你受到的物理伤害被溅射到附近的敌人。");
								break;
							case A_ABSORPTION:
								sprintf(buf2, "这件护甲能随机吸收0到%i的伤害，相当于你当前最大生命值的%i%%（如果这件%s被增强了，吸收伤害的最大值会%s%i。）",
										(int) armorAbsorptionMax(enchant),
										(int) (100 * armorAbsorptionMax(enchant) / player.info.maxHP),
										theName,
										(armorAbsorptionMax(enchant) == armorAbsorptionMax((float) (enchant + enchantIncrement(theItem))) ? "维持在" : "增加到"),
										(int) armorAbsorptionMax((float) (enchant + enchantIncrement(theItem))));
								break;
							case A_REPRISAL:
								sprintf(buf2, "任何用对你造成物理伤害的敌人都会被这件护甲反弹%i%%的伤害。（如果这件%s被增强了，反射的百分比会上升到%i%%。）",
										armorReprisalPercent(enchant),
										theName,
										armorReprisalPercent((float) (enchant + enchantIncrement(theItem))));
								break;
							case A_IMMUNITY:
								sprintf(buf2, "这件护甲能免疫来自%s的任何攻击。",
										monsterCatalog[theItem->vorpalEnemy].monsterName);
								break;
							case A_REFLECTION:
								if (theItem->enchant1 > 0) {
									short reflectChance = reflectionChance(enchant);
									short reflectChance2 = reflectionChance(enchant + enchantIncrement(theItem));
									sprintf(buf2, "这件护甲能阻挡%i%%指向你的法术，并能以%i%%的几率将法术反射回施法者。（如果这件%s被增强了，概率会分别上升到%i%%和%i%%）",
											reflectChance,
											reflectChance * reflectChance / 100,
											reflectChance2,
											reflectChance2 * reflectChance2 / 100);
								} else if (theItem->enchant1 < 0) {
									short reflectChance = reflectionChance(enchant);
									short reflectChance2 = reflectionChance(enchant + enchantIncrement(theItem));
									sprintf(buf2, "这件护甲会以%i%%的几率使你的施法失效，并会以%i%%的概率把你发出的法术反射回你自己（如果这件%s被增强了，概率会分别降低到%i%%和%i%%）",
											reflectChance,
											reflectChance * reflectChance / 100,
											reflectChance2,
											reflectChance2 * reflectChance2 / 100);
								}
								break;
                            case A_RESPIRATION:
                                strcpy(buf2, "穿上这件护甲后，它会释放一股新鲜空气包围在你周围，使你免疫于蒸汽和其他毒气。");
                                break;
                            case A_DAMPENING:
                                strcpy(buf2, "这件护甲能吸吸收爆炸产生的伤害，但你仍然会被烧到。");
                                break;
							case A_BURDEN:
								strcpy(buf2, "每次你受到攻击这件护甲都会有10%%的概率变得更重。");
								break;
							case A_VULNERABILITY:
								strcpy(buf2, "这件护甲会使你受到的伤害提高两倍。");
								break;
                            case A_IMMOLATION:
								strcpy(buf2, "每次你受到伤害这件护甲都有10%%的概率产生爆炸。");
								break;
							default:
								break;
						}
						strcat(buf, buf2);
					} else if (theItem->flags & ITEM_IDENTIFIED) {
						sprintf(buf2, "\n\n某种神秘的符文刻满了%s。",
								theName);
						strcat(buf, buf2);
					}
				}
				
				// equipped? cursed?
				if (theItem->flags & ITEM_EQUIPPED) {
					sprintf(buf2, "\n\n你正身着这件%s%s。",
							theName,
							((theItem->flags & ITEM_CURSED) ? "，而且由于它是被诅咒的，你现在没有办法脱掉它" : ""));
					strcat(buf, buf2);
				} else if (((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAGIC_DETECTED)) || rogue.playbackOmniscience)
						   && (theItem->flags & ITEM_CURSED)) {
					sprintf(buf2, "\n\n%s你能感觉到这%s里有一股险恶的魔法能量。%s",
                            badColorEscape,
                            theName,
                            whiteColorEscape);
					strcat(buf, buf2);
				}
				
			}
			break;
			
		case STAFF:
			enchant = theItem->enchant1;
			
			// charges
			if ((theItem->flags & ITEM_IDENTIFIED)  || rogue.playbackOmniscience) {
				sprintf(buf2, "\n\n这件%s还剩下%i发的能量（最多能保存%i发）。所有法杖的能量都会随时间回复。",
						theName,
						theItem->charges,
						theItem->enchant1);
				strcat(buf, buf2);
			} else if (theItem->flags & ITEM_MAX_CHARGES_KNOWN) {
				sprintf(buf2, "\n\n这件%s最多存有%i发的能量。所有法杖的能量都会随时间回复。",
						theName,
						theItem->enchant1);
				strcat(buf, buf2);
			}
			
			// effect description
			if (((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN)) && staffTable[theItem->kind].identified)
				|| rogue.playbackOmniscience) {
				switch (theItem->kind) {
						// STAFF_LIGHTNING, STAFF_FIRE, STAFF_POISON, STAFF_TUNNELING, STAFF_BLINKING, STAFF_ENTRANCEMENT, STAFF_HEALING,
						// STAFF_HASTE, STAFF_OBSTRUCTION, STAFF_DISCORD, STAFF_CONJURATION
					case STAFF_LIGHTNING:
						sprintf(buf2, "当使用时这件法杖会对其释放路线上的所有目标造成伤害。没有怪物能免疫闪电攻击。（如果这件法杖被增强，其伤害会被增强到%i%%。）。",
								(int) (100 * (staffDamageLow(enchant + 1) + staffDamageHigh(enchant + 1)) / (staffDamageLow(enchant) + staffDamageHigh(enchant)) - 100));
						break;
					case STAFF_FIRE:
						sprintf(buf2, "当使用时这件法杖会对其释放路线上的所有目标造成伤害。如果怪物对火焰免疫则不会对其有伤害。（如果这件法杖被增强，其伤害会被增强到%i%%。）。",
								(int) (100 * (staffDamageLow(enchant + 1) + staffDamageHigh(enchant + 1)) / (staffDamageLow(enchant) + staffDamageHigh(enchant)) - 100));
						break;
					case STAFF_POISON:
						sprintf(buf2, "这件法杖能发出魔法的毒箭攻击目标，使其中毒%i回合。（如果这件法杖被增强，中毒效果会延长到%i回合）。",
								staffPoison(enchant),
								staffPoison(enchant + 1));
						break;
					case STAFF_TUNNELING:
						sprintf(buf2, "这件法杖能发出销毁墙壁的法球，造成目标方向的%i层墙壁被销毁。（如果这件法杖被增强，摧毁墙壁的层数会提高到%i）",
								theItem->enchant1,
								theItem->enchant1 + 1);
						break;
					case STAFF_BLINKING:
						sprintf(buf2, "这件法杖能使你最多向指向的方向闪烁最多%i格的距离。（如果这件法杖被增强，闪烁距离上限会提高到%i格）。闪烁法杖的充能速度只有常见法杖的一半。",
								staffBlinkDistance(theItem->enchant1),
								staffBlinkDistance(theItem->enchant1 + 1));
						break;
					case STAFF_ENTRANCEMENT:
						sprintf(buf2, "这件法杖会让目标模仿你的行动，效果持续%i回合。（如果这件法杖被增强，效果持续时间会延长到%i回合）。",
								staffEntrancementDuration(theItem->enchant1),
								staffEntrancementDuration(theItem->enchant1 + 1));
						break;
					case STAFF_HEALING:
						if (enchant < 10) {
							sprintf(buf2, "这件法杖会回复其目标最大生命值的%i%%。（如果这件法杖被增强，回复效果会提升到%i%%）",
									theItem->enchant1 * 10,
									(theItem->enchant1 + 1) * 10);
						} else {
							strcpy(buf2, "这件法杖能完全回复其目标的生命值。");	
						}
						break;
					case STAFF_HASTE:
						sprintf(buf2, "这件法杖能使其目标移动速度翻倍，效果持续%i回合。（如果这件法杖被增强，加速效果会延长到%i回合）。",
								staffHasteDuration(theItem->enchant1),
								staffHasteDuration(theItem->enchant1 + 1));
						break;
					case STAFF_OBSTRUCTION:
						strcpy(buf2, "这件法杖充能速度是一般法杖的一半。");
						break;
					case STAFF_DISCORD:
						sprintf(buf2, "这件法杖会挑拨目标，使其攻击附近的其他敌人，同时敌人也会攻击它。效果持续%i回合。（如果这件法杖被增强，挑拨效果会延长到%i回合）。",
								staffDiscordDuration(theItem->enchant1),
								staffDiscordDuration(theItem->enchant1 + 1));
						break;
					case STAFF_CONJURATION:
						sprintf(buf2, "使用这件法杖能召唤出%i个奥术之剑来帮助你战斗。（如果这件法杖被增强，其数量会提升到%i）。",
								staffBladeCount(theItem->enchant1),
								staffBladeCount(theItem->enchant1 + 1));
						break;
					case STAFF_PROTECTION:
						sprintf(buf2, "这件法杖能对目标施加魔法盾，最多能吸收%i点伤害。魔法盾最多持续20回合。（如果这件法杖被增强，吸收伤害上限将提升到%i）。",
								staffProtection(theItem->enchant1) / 10,
								staffProtection(theItem->enchant1 + 1) / 10);
						break;
					default:
						strcpy(buf2, "没有人知道这件法杖到底是用来干啥的。");
						break;
				}
				strcat(buf, "\n\n");
				strcat(buf, buf2);
			}
			break;
			
		case WAND:
			strcat(buf, "\n\n");
			if ((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN)) || rogue.playbackOmniscience) {
				if (theItem->charges) {
					sprintf(buf2, "剩余%i发的能量。使用充能卷轴能回复一发的能量，使用强化卷轴能回复%i发的能量。",
							theItem->charges,
                            wandTable[theItem->kind].range.lowerBound);
				} else {
					sprintf(buf2, "已没有能量剩余。使用充能卷轴能回复一发的能量，使用强化卷轴能够回复%i发的能量。",
                            wandTable[theItem->kind].range.lowerBound);
				}
			} else {
				if (theItem->enchant2) {
					sprintf(buf2, "这根魔棒你已经使用了%i次，你也不知道还剩下多少能量。",
							theItem->enchant2);
				} else {
					strcpy(buf2, "你还没有使用过这根魔棒。");
				}
				
				if (wandTable[theItem->kind].identified) {
					strcat(buf, buf2);
					sprintf(buf2, "这种类型的魔棒一般有%i到%i发的上限，使用强化卷轴能够回复其%i发的能量。",
							wandTable[theItem->kind].range.lowerBound,
							wandTable[theItem->kind].range.upperBound,
                            wandTable[theItem->kind].range.lowerBound);
				}
			}
			strcat(buf, buf2);
			break;
			
		case RING:
			if (((theItem->flags & ITEM_IDENTIFIED) && ringTable[theItem->kind].identified) || rogue.playbackOmniscience) {
                if (theItem->enchant1) {
                    switch (theItem->kind) {
                        case RING_CLAIRVOYANCE:
                            if (theItem->enchant1 > 0) {
                                sprintf(buf2, "\n\n这件指环能用魔法提供附近%i格内的视野。（如果这件指环被增强，范围会提升到%i）。",
                                        theItem->enchant1 + 1,
                                        theItem->enchant1 + 2);
                            } else {
                                sprintf(buf2, "\n\n这件指环以魔法的力量使你的视野减少到%i格。（如果这件指环被增强，致盲范围会减少到%i）。",
                                        (theItem->enchant1 * -1) + 1,
                                        (theItem->enchant1 * -1));
                            }
                            strcat(buf, buf2);
                            break;
                        case RING_REGENERATION:
                            sprintf(buf2, "\n\n装备着这件指环你的生命力能在%li回合内回复（正常回复需要%li回合）。（如果这件指环被增强，完全恢复所需的回合数将下降到%li）",
                                    (long) (turnsForFullRegen(theItem->enchant1) / 1000),
                                    (long) TURNS_FOR_FULL_REGEN,
                                    (long) (turnsForFullRegen(theItem->enchant1 + 1) / 1000));
                            strcat(buf, buf2);
                            break;
                        case RING_TRANSFERENCE:
                            sprintf(buf2, "\n\n这件指环会以你每次所造成伤害的%i%%%s你的生命值。（如果这件指环被增强，比例会将变为%i%%）。",
                                    abs(theItem->enchant1) * 10,
                                    (theItem->enchant1 > 0 ? "增加" : "减少"),
                                    abs(theItem->enchant1 + 1) * 10);
                            strcat(buf, buf2);
                            break;
                        case RING_WISDOM:
                            sprintf(buf2, "\n\n装备着这件指环，你携带的法杖会以%i%%的速度充能。（如果这件指环被增强，比例会变为%i%%）。",
                                    (int) (100 * pow(1.3, min(27, theItem->enchant1)) + FLOAT_FUDGE),
                                    (int) (100 * pow(1.3, min(27, (theItem->enchant1 + 1))) + FLOAT_FUDGE));
                            strcat(buf, buf2);
                            break;
                        default:
                            break;
                    }
                }
			} else {
				sprintf(buf2, "\n\n这件指环的隐藏属性将在装备%i回合后被发现。",
						theItem->charges);
				strcat(buf, buf2);
                
                if ((theItem->charges < RING_DELAY_TO_AUTO_ID || (theItem->flags & (ITEM_MAGIC_DETECTED | ITEM_IDENTIFIED)))
                    && theItem->enchant1 > 0) { // Mention the unknown-positive-ring footnote only if it's good magic and you know it.
                    
                    sprintf(buf2, "，到此之前你可以把它当做一个普通的+%i指环。", theItem->enchant2);
                    strcat(buf, buf2);
                } else {
                    strcat(buf, "。");
                }
			}
			
			// equipped? cursed?
			if (theItem->flags & ITEM_EQUIPPED) {
				sprintf(buf2, "\n\n你的手指上正带着%s。%s。",
						theName,
						((theItem->flags & ITEM_CURSED) ? "，而且由于它是被诅咒的，你现在没有办法将它取下来。" : ""));
				strcat(buf, buf2);
			} else if (((theItem->flags & (ITEM_IDENTIFIED | ITEM_MAGIC_DETECTED)) || rogue.playbackOmniscience)
					   && (theItem->flags & ITEM_CURSED)) {
				sprintf(buf2, "\n\n%s你能感觉到这%s里有一股险恶的魔法能量。%s",
                        badColorEscape,
                        theName,
                        whiteColorEscape);
				strcat(buf, buf2);
			}
			break;
        case CHARM:
			enchant = theItem->enchant1;
            switch (theItem->kind) {
                case CHARM_HEALTH:
                    sprintf(buf2, "\n\n使用后，它能在你%i%%的生命值，在%i回合后才能重新使用。（如果这件法器被增强，回复效果变为%i%%，冷却回合数将变为%i）。",
                            charmHealing(enchant),
                            charmRechargeDelay(theItem->kind, enchant),
                            charmHealing(enchant + 1),
                            charmRechargeDelay(theItem->kind, enchant + 1));
                    break;
                case CHARM_PROTECTION:
                    sprintf(buf2, "\n\n使用后，它能对你释放最多持续20回合的魔法护盾，最多能吸收%i点伤害，在%i回合后才能重新使用。（如果这件法器被增强，吸收伤害提升到%i，冷却回合数将变为%i）。",
                            charmProtection(enchant) / 10,
                            charmRechargeDelay(theItem->kind, enchant),
                            charmProtection(enchant + 1) / 10,
                            charmRechargeDelay(theItem->kind, enchant + 1));
                    break;
                case CHARM_HASTE:
                    sprintf(buf2, "\n\n使用后，它能对你释放持续%i回合的加速法术，在%i回合后才能重新使用。（如果这件法器被增强，持续时间将提升到%i回合，冷却回合数将变为%i）。",
                            charmEffectDuration(theItem->kind, enchant),
                            charmRechargeDelay(theItem->kind, enchant),
                            charmEffectDuration(theItem->kind, enchant + 1),
                            charmRechargeDelay(theItem->kind, enchant + 1));
                    break;
                case CHARM_FIRE_IMMUNITY:
                    sprintf(buf2, "\n\n使用后，它能使你在%i回合内对火焰免疫，在%i回合后才能重新使用。（如果这件法器被增强，持续时间将提升到%i回合，冷却回合数将变为%i）。",
                            charmEffectDuration(theItem->kind, enchant),
                            charmRechargeDelay(theItem->kind, enchant),
                            charmEffectDuration(theItem->kind, enchant + 1),
                            charmRechargeDelay(theItem->kind, enchant + 1));
                    break;
                case CHARM_INVISIBILITY:
                    sprintf(buf2, "\n\n使用后，它能使你在%i回合内维持隐身，期间在两格以外的怪物无法发现你的踪迹，在%i回合后才能重新使用。（如果这件法器被增强，持续时间将提升到%i回合，冷却回合数将变为%i）。",
                            charmEffectDuration(theItem->kind, enchant),
                            charmRechargeDelay(theItem->kind, enchant),
                            charmEffectDuration(theItem->kind, enchant + 1),
                            charmRechargeDelay(theItem->kind, enchant + 1));
                    break;
                case CHARM_TELEPATHY:
                    sprintf(buf2, "\n\n使用后，它能让你在%i回合内保持心灵感应效果，在%i回合后才能重新使用。（如果这件法器被增强，持续时间将提升到%i回合，冷却回合数将变为%i）。",
                            charmEffectDuration(theItem->kind, enchant),
                            charmRechargeDelay(theItem->kind, enchant),
                            charmEffectDuration(theItem->kind, enchant + 1),
                            charmRechargeDelay(theItem->kind, enchant + 1));
                    break;
                case CHARM_LEVITATION:
                    sprintf(buf2, "\n\n使用后，它能让你悬浮在空中，效果持续%i回合，在%i回合后才能重新使用。（如果这件法器被增强，持续时间将提升到%i回合，冷却回合数将变为%i）。",
                            charmEffectDuration(theItem->kind, enchant),
                            charmRechargeDelay(theItem->kind, enchant),
                            charmEffectDuration(theItem->kind, enchant + 1),
                            charmRechargeDelay(theItem->kind, enchant + 1));
                    break;
                case CHARM_SHATTERING:
                    sprintf(buf2, "\n\n使用后，它会将附近的墙壁溶解掉，在%i回合后才能重新使用。（如果这件法器被增强，冷却回合数将变为%i）",
                            charmRechargeDelay(theItem->kind, enchant),
                            charmRechargeDelay(theItem->kind, enchant + 1));
                    break;
//                case CHARM_CAUSE_FEAR:
//                    sprintf(buf2, "\n\nWhen used, the charm will terrify all visible creatures and recharge in %i turns. (If the charm is enchanted, it will recharge in %i turns.)",
//                            charmRechargeDelay(theItem->kind, enchant),
//                            charmRechargeDelay(theItem->kind, enchant + 1));
//                    break;
                case CHARM_TELEPORTATION:
                    sprintf(buf2, "\n\n使用后，它将把你传送到当前地牢的一个随机位置，在%i回合后才能重新使用。（如果这件法器被增强，冷却回合数将变为%i）。",
                            charmRechargeDelay(theItem->kind, enchant),
                            charmRechargeDelay(theItem->kind, enchant + 1));
                    break;
                case CHARM_RECHARGING:
                    sprintf(buf2, "\n\n使用后，它将对你身上的法杖进行充能（不包括法器和魔棒），在%i回合后才能重新使用。（如果这件法器被增强，冷却回合数将变为%i）。",
                            charmRechargeDelay(theItem->kind, enchant),
                            charmRechargeDelay(theItem->kind, enchant + 1));
                    break;
                case CHARM_NEGATION:
                    sprintf(buf2, "\n\n使用后，它将对你视野范围内的生物和地上的物品产生反魔法效果，在%i回合后才能重新使用。（如果这件法器被增强，冷却回合数将变为%i）。",
                            charmRechargeDelay(theItem->kind, enchant),
                            charmRechargeDelay(theItem->kind, enchant + 1));
                    break;
                default:
                    break;
            }
            strcat(buf, buf2);
            break;
		default:
			break;
	}
}

boolean displayMagicCharForItem(item *theItem) {
	if (!(theItem->flags & ITEM_MAGIC_DETECTED)
		|| (theItem->category & PRENAMED_CATEGORY)) {
		return false;
	} else {
        return true;
    }
}

char displayInventory(unsigned short categoryMask,
					  unsigned long requiredFlags,
					  unsigned long forbiddenFlags,
					  boolean waitForAcknowledge,
					  boolean includeButtons) {
	item *theItem;
	short i, maxLength = 0, itemNumber, itemCount, equippedItemCount;
	int w, h;
	short extraLineCount = 0;
	item *itemList[DROWS];
	char buf[DCOLS*3];
	char theKey;
	rogueEvent theEvent;
	boolean magicDetected, repeatDisplay;
	short highlightItemLine, itemSpaceRemaining;
	brogueButton buttons[50] = {{{0}}};
	short actionKey;
	color darkItemColor;
	BROGUE_WINDOW *root, *window;
	BROGUE_DRAW_CONTEXT *context;
	BROGUE_EFFECT *effect;
	BROGUE_DRAW_COLOR windowColor = { 0.05, 0.05, 0.20, 0.8 };
	char whiteColorEscapeSequence[20],
		grayColorEscapeSequence[20],
		yellowColorEscapeSequence[20],
		darkYellowColorEscapeSequence[20],
		goodColorEscapeSequence[20],
		badColorEscapeSequence[20];
	char *magicEscapePtr;
	
	assureCosmeticRNG;
	
	clearCursorPath();
		
	whiteColorEscapeSequence[0] = '\0';
	encodeMessageColor(whiteColorEscapeSequence, 0, &white);
	grayColorEscapeSequence[0] = '\0';
	encodeMessageColor(grayColorEscapeSequence, 0, &gray);
	yellowColorEscapeSequence[0] = '\0';
	encodeMessageColor(yellowColorEscapeSequence, 0, &itemColor);
	darkItemColor = itemColor;
	applyColorAverage(&darkItemColor, &black, 50);
	darkYellowColorEscapeSequence[0] = '\0';
	encodeMessageColor(darkYellowColorEscapeSequence, 0, &darkItemColor);
	goodColorEscapeSequence[0] = '\0';
	encodeMessageColor(goodColorEscapeSequence, 0, &goodMessageColor);
	badColorEscapeSequence[0] = '\0';
	encodeMessageColor(badColorEscapeSequence, 0, &badMessageColor);

	if (packItems->nextItem == NULL) {
		confirmMessages();
		message("你目前没有任何物品！", false);
		restoreRNG;
		return 0;
	}
	
	magicDetected = false;
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (displayMagicCharForItem(theItem) && (theItem->flags & ITEM_MAGIC_DETECTED)) {
			magicDetected = true;
		}
	}
	
	// List the items in the order we want to display them, with equipped items at the top.
	itemNumber = 0;
	equippedItemCount = 0;
	// First, the equipped weapon if any.
	if (rogue.weapon) {
		itemList[itemNumber] = rogue.weapon;
		itemNumber++;
		equippedItemCount++;
	}
	// Now, the equipped armor if any.
	if (rogue.armor) {
		itemList[itemNumber] = rogue.armor;
		itemNumber++;
		equippedItemCount++;
	}
	// Now, the equipped rings, if any.
	if (rogue.ringLeft) {
		itemList[itemNumber] = rogue.ringLeft;
		itemNumber++;
		equippedItemCount++;
	}
	if (rogue.ringRight) {
		itemList[itemNumber] = rogue.ringRight;
		itemNumber++;
		equippedItemCount++;
	}
	// Now all of the non-equipped items.
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (!(theItem->flags & ITEM_EQUIPPED)) {
			itemList[itemNumber] = theItem;
			itemNumber++;
		}
	}
	
	// Initialize the buttons:
	for (i=0; i < max(MAX_PACK_ITEMS, ROWS); i++) {
		buttons[i].y = 1 + i + (equippedItemCount && i >= equippedItemCount ? 1 : 0);
		buttons[i].buttonColor = black;
		buttons[i].opacity = INTERFACE_OPACITY;
		buttons[i].flags |= B_DRAW;
	}
	// Now prepare the buttons.
	//for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
	for (i=0; i<itemNumber; i++) {
		theItem = itemList[i];
		// Set button parameters for the item:
		buttons[i].flags |= (B_DRAW | B_ENABLED);
		if (!waitForAcknowledge) {
			buttons[i].flags |= B_KEYPRESS_HIGHLIGHT;
		}
		buttons[i].hotkey[0] = theItem->inventoryLetter;
		buttons[i].hotkey[1] = theItem->inventoryLetter + 'A' - 'a';
		
		if ((theItem->category & categoryMask) &&
			!(~(theItem->flags) & requiredFlags) &&
			!(theItem->flags & forbiddenFlags)) {
			
			buttons[i].flags |= (B_HOVER_ENABLED);
		}
		
		// Set the text for the button:
		itemName(theItem, buf, true, true, (buttons[i].flags & B_HOVER_ENABLED) ? &white : &gray);
		upperCase(buf);
		
		if ((theItem->flags & ITEM_MAGIC_DETECTED)
			&& !(theItem->category & AMULET)) { // Won't include food, keys, lumenstones or amulet.
			buttons[i].symbol[0] = (itemMagicChar(theItem) ? itemMagicChar(theItem) : '-');
			if (buttons[i].symbol[0] != '-') {
				buttons[i].symbol_flags[0] = PLOT_CHAR_TILE;
			}
			if (buttons[i].symbol[0] == '-') {
				magicEscapePtr = yellowColorEscapeSequence;
			} else if (buttons[i].symbol[0] == GOOD_MAGIC_CHAR) {
				magicEscapePtr = goodColorEscapeSequence;
			} else {
				magicEscapePtr = badColorEscapeSequence;
			}
			
			// The first '*' is the magic detection symbol, e.g. '-' for non-magical.
			// The second '*' is the item character, e.g. ':' for food.
			sprintf(buttons[i].text, "%c%s\t%s*\t%s*\t%s%s%s%s",
					theItem->inventoryLetter,
					(theItem->flags & ITEM_PROTECTED ? ">" : "."),
					magicEscapePtr,
					(buttons[i].flags & B_HOVER_ENABLED) ? yellowColorEscapeSequence : darkYellowColorEscapeSequence,
					(buttons[i].flags & B_HOVER_ENABLED) ? whiteColorEscapeSequence : grayColorEscapeSequence,
					buf,
					grayColorEscapeSequence,
					(theItem->flags & ITEM_EQUIPPED ? " (已装备) " : ""));
			buttons[i].symbol[1] = theItem->displayChar;
			buttons[i].symbol_flags[1] = PLOT_CHAR_TILE;
		} else {
			sprintf(buttons[i].text, "%c%s\t%s%s*\t%s%s%s%s", // The '*' is the item character, e.g. ':' for food.
					theItem->inventoryLetter,
					(theItem->flags & ITEM_PROTECTED ? ">" : "."),
					(magicDetected ? "\t" : ""), // For proper spacing when this item is not detected but another is.
					(buttons[i].flags & B_HOVER_ENABLED) ? yellowColorEscapeSequence : darkYellowColorEscapeSequence,
					(buttons[i].flags & B_HOVER_ENABLED) ? whiteColorEscapeSequence : grayColorEscapeSequence,
					buf,
					grayColorEscapeSequence,
					(theItem->flags & ITEM_EQUIPPED ? " (已装备) " : ""));
			buttons[i].symbol[0] = theItem->displayChar;
			buttons[i].symbol_flags[0] = PLOT_CHAR_TILE;
		}
	}
	//printf("\nMaxlength: %i", maxLength);
	itemCount = itemNumber;
	if (!itemNumber) {
		confirmMessages();
		message("Nothing of that type!", false);
		restoreRNG;
		return 0;
	}
	if (waitForAcknowledge) {
		// Add the two extra lines as disabled buttons.
		itemSpaceRemaining = MAX_PACK_ITEMS - numberOfItemsInPack();
		if (itemSpaceRemaining) {
			sprintf(buttons[itemNumber + extraLineCount].text, "%s还装得下%i件物品。",
					grayColorEscapeSequence,
					itemSpaceRemaining);
					// (itemSpaceRemaining == 1 ? "" : "s"));
		} else {
			sprintf(buttons[itemNumber + extraLineCount].text, "%s已经没有地方放东西了。",
					grayColorEscapeSequence);
		}
		buttons[itemNumber + extraLineCount].flags |= B_FORCE_CENTERED;
		extraLineCount++;
		
		sprintf(buttons[itemNumber + extraLineCount].text, "%s -- 按 (a-z) 键选择物品 -- ",
				grayColorEscapeSequence);
		buttons[itemNumber + extraLineCount].flags |= B_FORCE_CENTERED;
		extraLineCount++;
	}
	if (equippedItemCount) {
		// Add a separator button to fill in the blank line between equipped and unequipped items.
		sprintf(buttons[itemNumber + extraLineCount].text, "%s---",
				grayColorEscapeSequence);
		buttons[itemNumber + extraLineCount].flags |= B_FORCE_CENTERED;
		buttons[itemNumber + extraLineCount].y = equippedItemCount + 1;
		extraLineCount++;
	}

	root = BrogueDisplay_getRootWindow(brogue_display);

	/*  Open a 1x1 window -- we'll measure the size needed based the string
		lengths below.  */
	window = BrogueWindow_open(root, 0, 0, 1, 1);
	BrogueWindow_setColor(window, windowColor);
	context = BrogueDrawContext_open(window);
	BrogueDrawContext_enableProportionalFont(context, 1);
	if (magicDetected)
	{
		int stops[] = { 2, 4, 6 };
		
		BrogueDrawContext_setTabStops(context, 3, stops);
	}
	else
	{
		int stops[] = { 2, 4 };

		BrogueDrawContext_setTabStops(context, 2, stops);
	}								  
	effect = BrogueEffect_open(context, BUTTON_EFFECT_NAME);

	w = 2;
	for (i=0; i < itemNumber + extraLineCount; i++) {
		BROGUE_TEXT_SIZE text_size;
		
		text_size = BrogueDrawContext_measureAsciiString(
			context, 0, buttons[i].text);
		if (text_size.width + 2 > w)
		{
			w = text_size.width + 2;
		}
	}
	
	for (i = 0; i < itemNumber + extraLineCount; i++) {
		// Position the button.
		buttons[i].x = 1;
		buttons[i].width = w - 2;

		drawButton(context, effect, &(buttons[i]), BUTTON_NORMAL);
	}

	h = itemNumber + extraLineCount + 2;
	BrogueWindow_reposition(window, 
		STAT_BAR_WIDTH + (DCOLS - w) / 2, MESSAGE_LINES + (DROWS - h) / 2, 
		w, h);
	
	// Add invisible previous and next buttons, so up and down arrows can select items.
	// Previous
	buttons[itemNumber + extraLineCount + 0].flags = B_ENABLED; // clear everything else
	buttons[itemNumber + extraLineCount + 0].hotkey[0] = NUMPAD_8;
	buttons[itemNumber + extraLineCount + 0].hotkey[1] = UP_ARROW;
	// Next
	buttons[itemNumber + extraLineCount + 1].flags = B_ENABLED; // clear everything else
	buttons[itemNumber + extraLineCount + 1].hotkey[0] = NUMPAD_2;
	buttons[itemNumber + extraLineCount + 1].hotkey[1] = DOWN_ARROW;
	
	do {
		repeatDisplay = false;
		
		// Do the button loop.
		highlightItemLine = -1;
		
		highlightItemLine = buttonInputLoop(buttons,
											itemCount + extraLineCount + 2, // the 2 is for up/down hotkeys
											context, 
											effect,
											COLS - maxLength,
											mapToWindowY(0),
											maxLength,
											itemNumber + extraLineCount,
											&theEvent); 
		if (highlightItemLine == itemNumber + extraLineCount + 0) {
			// Up key
			highlightItemLine = itemNumber - 1;
			theEvent.shiftKey = true;
		} else if (highlightItemLine == itemNumber + extraLineCount + 1) {
			// Down key
			highlightItemLine = 0;
			theEvent.shiftKey = true;
		}
		
		if (highlightItemLine >= 0) {
			theKey = itemList[highlightItemLine]->inventoryLetter;
			theItem = itemList[highlightItemLine];
		} else {
			theKey = ESCAPE_KEY;
		}
		
		// Was an item selected?
		if (highlightItemLine > -1 && (waitForAcknowledge || theEvent.shiftKey || theEvent.controlKey)) {
			
			actionKey = 0;
			do {
				// Yes. Highlight the selected item. Do this by changing the button color and re-displaying it.
				
				//buttons[highlightItemLine].buttonColor = interfaceBoxColor;
				drawButton(context, effect, 
						   &(buttons[highlightItemLine]),
						   BUTTON_PRESSED);
				//buttons[highlightItemLine].buttonColor = black;
				
				if (theEvent.shiftKey || theEvent.controlKey || waitForAcknowledge) {
					BrogueWindow_setVisible(window, 0);

					// Display an information window about the item.
					actionKey = printCarriedItemDetails(
						theItem, 40, includeButtons);
					
					if (actionKey == -1) {
						BrogueWindow_setVisible(window, 1);
						repeatDisplay = true;
					} else {
						restoreRNG;
						repeatDisplay = false;
					}
					
					switch (actionKey) {
						case APPLY_KEY:
							apply(theItem, true);
							break;
						case EQUIP_KEY:
							equip(theItem);
							break;
						case UNEQUIP_KEY:
							unequip(theItem);
							break;
						case DROP_KEY:
							drop(theItem);
							break;
						case THROW_KEY:
							throwCommand(theItem);
							break;
						case CALL_KEY:
							call(theItem);
							break;
						case UP_KEY:
							highlightItemLine = highlightItemLine - 1;
							if (highlightItemLine < 0) {
								highlightItemLine = itemNumber - 1;
							}
							break;
						case DOWN_KEY:
							highlightItemLine = highlightItemLine + 1;
							if (highlightItemLine >= itemNumber) {
								highlightItemLine = 0;
							}
							break;
						default:
							break;
					}
					
					if (actionKey == UP_KEY || actionKey == DOWN_KEY) {
						theKey = itemList[highlightItemLine]->inventoryLetter;
						theItem = itemList[highlightItemLine];
					} else if (actionKey > -1) {
						// Player took an action directly from the item screen; we're done here.
						restoreRNG;

						BrogueEffect_close(effect);
						BrogueDrawContext_close(context);
						BrogueWindow_close(window);
						return 0;
					}
				}
			} while (actionKey == UP_KEY || actionKey == DOWN_KEY);
		}
	} while (repeatDisplay); // so you can get info on multiple items sequentially

	BrogueEffect_close(effect);
	BrogueDrawContext_close(context);
	BrogueWindow_close(window);
	
	restoreRNG;
	return theKey;
}

short numberOfMatchingPackItems(unsigned short categoryMask,
								unsigned long requiredFlags, unsigned long forbiddenFlags,
								boolean displayErrors) {
	item *theItem;
	short matchingItemCount = 0;
	
	if (packItems->nextItem == NULL) {
		if (displayErrors) {
			confirmMessages();
			message("你身上还没有任何道具！", false);
		}
		return 0;
	}
	
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		
		if (theItem->category & categoryMask &&
			!(~(theItem->flags) & requiredFlags) &&
			!(theItem->flags & forbiddenFlags)) {
			
			matchingItemCount++;
		}
	}
	
	if (matchingItemCount == 0) {
		if (displayErrors) {
			confirmMessages();
			message("没有找到合适的物品。", false);
		}
		return 0;
	}
	
	return matchingItemCount;
}

void updateEncumbrance() {
	short moveSpeed, attackSpeed;
	
	moveSpeed = player.info.movementSpeed;
	attackSpeed = player.info.attackSpeed;
	
	if (player.status[STATUS_HASTED]) {
		moveSpeed /= 2;
		attackSpeed /= 2;
	} else if (player.status[STATUS_SLOWED]) {
		moveSpeed *= 2;
		attackSpeed *= 2;
	}
	
	player.movementSpeed = moveSpeed;
	player.attackSpeed = attackSpeed;
	
	recalculateEquipmentBonuses();
}

short displayedArmorValue() {
    if (!rogue.armor || (rogue.armor->flags & ITEM_IDENTIFIED)) {
        return player.info.defense / 10;
    } else {
        return (short) (((armorTable[rogue.armor->kind].range.upperBound + armorTable[rogue.armor->kind].range.lowerBound) / 2) / 10 + strengthModifier(rogue.armor) + FLOAT_FUDGE);
    }
}

void strengthCheck(item *theItem) {
	char buf1[COLS], buf2[COLS*2];
	short strengthDeficiency;
	
	updateEncumbrance();
	if (theItem) {
		if (theItem->category & WEAPON && theItem->strengthRequired > rogue.strength - player.weaknessAmount) {
			strengthDeficiency = theItem->strengthRequired - max(0, rogue.strength - player.weaknessAmount);
			strcpy(buf1, "");
			itemName(theItem, buf1, false, false, NULL);
			sprintf(buf2, "你几乎没办法把这件%s拿起来；需要额外%i点力量。", buf1, strengthDeficiency);
			message(buf2, false);
		}
		
		if (theItem->category & ARMOR && theItem->strengthRequired > rogue.strength - player.weaknessAmount) {
			strengthDeficiency = theItem->strengthRequired - max(0, rogue.strength - player.weaknessAmount);
			strcpy(buf1, "");
			itemName(theItem, buf1, false, false, NULL);
			sprintf(buf2, "你在%s的重量下感觉步履阑珊；需要额外%i点力量。",
					buf1, strengthDeficiency);
			message(buf2, false);
		}
	}
}

boolean canEquip(item *theItem) {
	item *previouslyEquippedItem = NULL;
	
	if (theItem->category & WEAPON) {
		previouslyEquippedItem = rogue.weapon;
	} else if (theItem->category & ARMOR) {
		previouslyEquippedItem = rogue.armor;
	}
	if (previouslyEquippedItem && (previouslyEquippedItem->flags & ITEM_CURSED)) {
		return false; // already using a cursed item
	}
	
	if ((theItem->category & RING) && rogue.ringLeft && rogue.ringRight) {
		return false;
	}
	return true;
}

// Will prompt for an item if none is given.
// Equips the item and records input if successful.
// Player's failure to select an item will result in failure.
// Failure does not record input.
void equip(item *theItem) {
	char buf1[COLS*3], buf2[COLS*3];
	unsigned char command[10];
	short c = 0;
	item *theItem2;
	
	command[c++] = EQUIP_KEY;
	if (!theItem) {
		theItem = promptForItemOfType((WEAPON|ARMOR|RING), 0, ITEM_EQUIPPED, "Equip what? (a-z, shift for more info; or <esc> to cancel)", true);
	}
	if (theItem == NULL) {
		return;
	}
	
	command[c++] = theItem->inventoryLetter;
	
	if (theItem->category & (WEAPON|ARMOR|RING)) {
		
		if (theItem->category & RING) {
			if (theItem->flags & ITEM_EQUIPPED) {
				confirmMessages();
				message("你已经戴着这件戒指了。", false);
				return;
			} else if (rogue.ringLeft && rogue.ringRight) {
				confirmMessages();
				theItem2 = promptForItemOfType((RING), ITEM_EQUIPPED, 0,
											   "你已经装备了两个戒指，必须先取下来一个：", true);
				if (!theItem2 || theItem2->category != RING || !(theItem2->flags & ITEM_EQUIPPED)) {
					if (theItem2) { // No message if canceled or did an inventory action instead.
						message("无效的输入。", false);
					}
					return;
				} else {
					if (theItem2->flags & ITEM_CURSED) {
						itemName(theItem2, buf1, false, false, NULL);
						sprintf(buf2, "你发现自己没法取下来这件%s：它好像是被诅咒的。", buf1);
						confirmMessages();
						messageWithColor(buf2, &itemMessageColor, false);
						return;
					}
					unequipItem(theItem2, false);
					command[c++] = theItem2->inventoryLetter;
				}
			}
		}
		
		if (theItem->flags & ITEM_EQUIPPED) {
			confirmMessages();
			message("已经装备上了这件物品。", false);
			return;
		}
		
		if (!canEquip(theItem)) {
			// equip failed because current item is cursed
			if (theItem->category & WEAPON) {
				itemName(rogue.weapon, buf1, false, false, NULL);
			} else if (theItem->category & ARMOR) {
				itemName(rogue.armor, buf1, false, false, NULL);
			} else {
				sprintf(buf1, "物品");
			}
			sprintf(buf2, "你无法完成这个动作；你已经装备的%s似乎是被诅咒的。", buf1);
			confirmMessages();
			messageWithColor(buf2, &itemMessageColor, false);
			return;
		}
		command[c] = '\0';
		recordKeystrokeSequence(command);
		
		equipItem(theItem, false);
		
		itemName(theItem, buf2, true, true, NULL);
		sprintf(buf1, "你%s%s。", (theItem->category & WEAPON ? "装备上了" : "穿上了"), buf2);
		confirmMessages();
		messageWithColor(buf1, &itemMessageColor, false);
		
		strengthCheck(theItem);
		
		if (theItem->flags & ITEM_CURSED) {
			itemName(theItem, buf2, false, false, NULL);
			switch(theItem->category) {
				case WEAPON:
					sprintf(buf1, "你无法控制的紧紧的握住了这把%s。", buf2);
					break;
				case ARMOR:
					sprintf(buf1, "这件%s紧紧的裹住了你，让你感觉很难受。", buf2);
					break;
				case RING:
					sprintf(buf1, "这件%s紧紧的楛住了你的手指，让你感觉很难受。", buf2);
					break;
				default:
					sprintf(buf1, "这件%s紧紧的黏住了你，怎么弄也弄不掉。", buf2);
					break;
			}
			messageWithColor(buf1, &itemMessageColor, false);
		}
		playerTurnEnded();
	} else {
		confirmMessages();
		message("无法装备此道具。", false);
	}
}

// Returns whether the given item is a key that can unlock the given location.
// An item qualifies if:
// (1) it's a key (has ITEM_IS_KEY flag),
// (2) its keyZ matches the depth, and
// (3) either its key (x, y) location matches (x, y), or its machine number matches the machine number at (x, y).
boolean keyMatchesLocation(item *theItem, short x, short y) {
	short i;
	
	if ((theItem->flags & ITEM_IS_KEY)
		&& theItem->keyZ == rogue.depthLevel) {
		
		for (i=0; i < KEY_ID_MAXIMUM && (theItem->keyLoc[i].x || theItem->keyLoc[i].machine); i++) {
			if (theItem->keyLoc[i].x == x && theItem->keyLoc[i].y == y) {
				return true;
			} else if (theItem->keyLoc[i].machine == pmap[x][y].machineNumber) {
				return true;
			}
		}
	}
	return false;
}

item *keyInPackFor(short x, short y) {
	item *theItem;
	
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (keyMatchesLocation(theItem, x, y)) {
			return theItem;
		}
	}
	return NULL;
}

item *keyOnTileAt(short x, short y) {
	item *theItem;
	creature *monst;
	
	if ((pmap[x][y].flags & HAS_PLAYER)
		&& player.xLoc == x
		&& player.yLoc == y
		&& keyInPackFor(x, y)) {
		
		return keyInPackFor(x, y);
	}
	if (pmap[x][y].flags & HAS_ITEM) {
		theItem = itemAtLoc(x, y);
		if (keyMatchesLocation(theItem, x, y)) {
			return theItem;
		}
	}
	if (pmap[x][y].flags & HAS_MONSTER) {
		monst = monsterAtLoc(x, y);
		if (monst->carriedItem) {
			theItem = monst->carriedItem;
			if (keyMatchesLocation(theItem, x, y)) {
				return theItem;
			}
		}
	}
	return NULL;
}

void aggravateMonsters() {
	creature *monst;
	short i, j, **grid;
	for (monst=monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		if (monst->creatureState == MONSTER_SLEEPING) {
			wakeUp(monst);
		}
		if (monst->creatureState != MONSTER_ALLY && monst->leader != &player) {
			monst->creatureState = MONSTER_TRACKING_SCENT;
		}
	}
	
	grid = allocGrid();
	fillGrid(grid, 0);
	
	calculateDistances(grid, player.xLoc, player.yLoc, T_PATHING_BLOCKER, NULL, true, false);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (grid[i][j] >= 0 && grid[i][j] < 30000) {
				scentMap[i][j] = 0;
				addScentToCell(i, j, 2 * grid[i][j]);
			}
		}
	}
	
	freeGrid(grid);
}

// returns the number of entries in the list;
// also includes (-1, -1) as an additional terminus indicator after the end of the list
short getLineCoordinates(short listOfCoordinates[][2], short originLoc[2], short targetLoc[2]) {
	float targetVector[2], error[2];
	short largerTargetComponent, currentVector[2], previousVector[2], quadrantTransform[2], i;
	short currentLoc[2];
	short cellNumber = 0;
	
//#ifdef BROGUE_ASSERTS
//	assert(originLoc[0] != targetLoc[0] || originLoc[1] != targetLoc[1]);
//#else
	if (originLoc[0] == targetLoc[0] && originLoc[1] == targetLoc[1]) {
		return 0;
	}
//#endif
	
	// Neither vector is negative. We keep track of negatives with quadrantTransform.
	for (i=0; i<= 1; i++) {
		targetVector[i] = targetLoc[i] - originLoc[i];
		if (targetVector[i] < 0) {
			targetVector[i] *= -1;
			quadrantTransform[i] = -1;
		} else {
			quadrantTransform[i] = 1;
		}
		currentVector[i] = previousVector[i] = error[i] = 0;
		currentLoc[i] = originLoc[i];
	}
	
	// normalize target vector such that one dimension equals 1 and the other is in [0, 1].
	largerTargetComponent = max(targetVector[0], targetVector[1]);
	targetVector[0] /= largerTargetComponent;
	targetVector[1] /= largerTargetComponent;
	
	do {
		for (i=0; i<= 1; i++) {
			currentVector[i] += targetVector[i];
			error[i] += (targetVector[i] == 1 ? 0 : targetVector[i]);
			
			if (error[i] >= 0.5) {
				currentVector[i]++;
				error[i] -= 1;
			}
			
			currentLoc[i] = quadrantTransform[i]*currentVector[i] + originLoc[i];
			
			listOfCoordinates[cellNumber][i] = currentLoc[i];
		}
		
		//DEBUG printf("\ncell %i: (%i, %i)", cellNumber, listOfCoordinates[cellNumber][0], listOfCoordinates[cellNumber][1]);
		cellNumber++;
		
	} while (coordinatesAreInMap(currentLoc[0], currentLoc[1]));
	
	cellNumber--;
	
	listOfCoordinates[cellNumber][0] = listOfCoordinates[cellNumber][1] = -1; // demarcates the end of the list
	return cellNumber;
}

// Should really use getLineCoordinates instead of calculating from scratch.
void getImpactLoc(short returnLoc[2], short originLoc[2], short targetLoc[2],
				  short maxDistance, boolean returnLastEmptySpace) {
	float targetVector[2], error[2];
	short largerTargetComponent, currentVector[2], previousVector[2], quadrantTransform[2], i;
	short currentLoc[2], previousLoc[2];
	creature *monst;
	
	monst = NULL;
	
	// Neither vector is negative. We keep track of negatives with quadrantTransform.
	for (i=0; i<= 1; i++) {
		targetVector[i] = targetLoc[i] - originLoc[i];
		if (targetVector[i] < 0) {
			targetVector[i] *= -1;
			quadrantTransform[i] = -1;
		} else {
			quadrantTransform[i] = 1;
		}
		currentVector[i] = previousVector[i] = error[i] = 0;
		currentLoc[i] = originLoc[i];
	}
	
	// normalize target vector such that one dimension equals 1 and the other is in [0, 1].
	largerTargetComponent = max(targetVector[0], targetVector[1]);
	targetVector[0] /= largerTargetComponent;
	targetVector[1] /= largerTargetComponent;
	
	do {
		for (i=0; i<= 1; i++) {
			
			previousLoc[i] = currentLoc[i];
			
			currentVector[i] += targetVector[i];
			error[i] += (targetVector[i] == 1 ? 0 : targetVector[i]);
			
			if (error[i] >= 0.5) {
				currentVector[i]++;
				error[i] -= 1;
			}
			
			currentLoc[i] = quadrantTransform[i]*currentVector[i] + originLoc[i];
		}
		
		if (!coordinatesAreInMap(currentLoc[0], currentLoc[1])) {
			break;
		}
		
		if (pmap[currentLoc[0]][currentLoc[1]].flags & HAS_MONSTER) {
			monst = monsterAtLoc(currentLoc[0], currentLoc[1]);
		}
		
	} while ((!(pmap[currentLoc[0]][currentLoc[1]].flags & HAS_MONSTER)
			  || !monst
			  || (monst->status[STATUS_INVISIBLE] || (monst->bookkeepingFlags & MONST_SUBMERGED)))
			 && !(pmap[currentLoc[0]][currentLoc[1]].flags & HAS_PLAYER)
			 && !cellHasTerrainFlag(currentLoc[0], currentLoc[1], (T_OBSTRUCTS_VISION | T_OBSTRUCTS_PASSABILITY))
			 && max(currentVector[0], currentVector[1]) <= maxDistance);
	
	if (returnLastEmptySpace) {
		returnLoc[0] = previousLoc[0];
		returnLoc[1] = previousLoc[1];
	} else {
		returnLoc[0] = currentLoc[0];
		returnLoc[1] = currentLoc[1];
	}
}

boolean tunnelize(short x, short y) {
	enum dungeonLayers layer;
	boolean didSomething = false;
	creature *monst;
	
	if (pmap[x][y].flags & IMPREGNABLE) {
		return false;
	}
	
	freeCaptivesEmbeddedAt(x, y);
	
    if (x == 0 || x == DCOLS - 1 || y == 0 || y == DROWS - 1) {
        pmap[x][y].layers[DUNGEON] = CRYSTAL_WALL; // don't dissolve the boundary walls
        didSomething = true;
    } else {
        for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
            if (tileCatalog[pmap[x][y].layers[layer]].flags & (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_VISION)) {
                pmap[x][y].layers[layer] = (layer == DUNGEON ? FLOOR : NOTHING);
                didSomething = true;
            }
        }
        if (didSomething) {
            spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_TUNNELIZE], true, false);
            if (pmap[x][y].flags & HAS_MONSTER) {
                // Kill turrets and sentinels if you tunnelize them.
                monst = monsterAtLoc(x, y);
                if (monst->info.flags & MONST_ATTACKABLE_THRU_WALLS) {
                    inflictDamage(monst, monst->currentHP, NULL);
                }
            }
        }
    }
	return didSomething;
}

void negate(creature *monst) {
	if (monst->info.flags & MONST_DIES_IF_NEGATED) {
		char buf[DCOLS * 3], monstName[DCOLS];
		monsterName(monstName, monst, true);
		if (monst->status[STATUS_LEVITATING]) {
			sprintf(buf, "%s瞬间化为粉末，消散在空中。", monstName);
		} else if (monst->info.flags & MONST_INANIMATE) {
            sprintf(buf, "%s瞬间粉碎了，小块的岩石散落一地。", monstName);
        } else {
			sprintf(buf, "%s突然掉落在了地面上，似乎从来就不会动一样。", monstName);
		}
		killCreature(monst, false);
		combatMessage(buf, messageColorFromVictim(monst));
	} else {
		// works on inanimates
		monst->info.abilityFlags = 0; // negated monsters lose all special abilities
		monst->status[STATUS_IMMUNE_TO_FIRE] = 0;
		monst->status[STATUS_SLOWED] = 0;
		monst->status[STATUS_HASTED] = 0;
		monst->status[STATUS_CONFUSED] = 0;
		monst->status[STATUS_ENTRANCED] = 0;
		monst->status[STATUS_DISCORDANT] = 0;
		monst->status[STATUS_SHIELDED] = 0;
		monst->status[STATUS_INVISIBLE] = 0;
		if (monst == &player) {
			monst->status[STATUS_TELEPATHIC] = min(monst->status[STATUS_TELEPATHIC], 1);
			monst->status[STATUS_MAGICAL_FEAR] = min(monst->status[STATUS_MAGICAL_FEAR], 1);
			monst->status[STATUS_LEVITATING] = min(monst->status[STATUS_LEVITATING], 1);
			if (monst->status[STATUS_DARKNESS]) {
				monst->status[STATUS_DARKNESS] = 0;
				updateMinersLightRadius();
				updateVision(true);
			}
		} else {
			monst->status[STATUS_TELEPATHIC] = 0;
			monst->status[STATUS_MAGICAL_FEAR] = 0;
			monst->status[STATUS_LEVITATING] = 0;
		}
		monst->info.flags &= ~MONST_IMMUNE_TO_FIRE;
		monst->movementSpeed = monst->info.movementSpeed;
		monst->attackSpeed = monst->info.attackSpeed;
		if (monst != &player && monst->info.flags & NEGATABLE_TRAITS) {
			if ((monst->info.flags & MONST_FIERY) && monst->status[STATUS_BURNING]) {
				extinguishFireOnCreature(monst);
			}
			monst->info.flags &= ~NEGATABLE_TRAITS;
			refreshDungeonCell(monst->xLoc, monst->yLoc);
			refreshSideBar(-1, -1, false);
		}
		applyInstantTileEffectsToCreature(monst); // in case it should immediately die or fall into a chasm
	}
}

short monsterAccuracyAdjusted(const creature *monst) {
    short retval = monst->info.accuracy * (pow(WEAPON_ENCHANT_ACCURACY_FACTOR, -2.5 * (float) monst->weaknessAmount) + FLOAT_FUDGE);
    return max(retval, 0);
}

float monsterDamageAdjustmentAmount(const creature *monst) {
    if (monst == &player) {
        return 1.0 + FLOAT_FUDGE; // Handled through player strength routines elsewhere.
    } else {
        return pow(WEAPON_ENCHANT_DAMAGE_FACTOR, -2.5 * (float) monst->weaknessAmount) + FLOAT_FUDGE;
    }
}

short monsterDefenseAdjusted(const creature *monst) {
    short retval = monst->info.defense - 25 * monst->weaknessAmount;
    return max(retval, 0);
}

// Adds one to the creature's weakness, sets the weakness status duration to maxDuration.
void weaken(creature *monst, short maxDuration) {
    if (monst->weaknessAmount < 10) {
        monst->weaknessAmount++;
    }
	monst->status[STATUS_WEAKENED] = max(monst->status[STATUS_WEAKENED], maxDuration);
	monst->maxStatus[STATUS_WEAKENED] = max(monst->maxStatus[STATUS_WEAKENED], maxDuration);
	if (monst == &player) {
        messageWithColor("在毒气的影响下你感觉自己变的很虚弱。", &badMessageColor, false);
		strengthCheck(rogue.weapon);
		strengthCheck(rogue.armor);
	}
}

// True if the creature polymorphed; false if not.
boolean polymorph(creature *monst) {
	short previousDamageTaken;
	float healthFraction;
	
	if (monst == &player || (monst->info.flags & MONST_INANIMATE)) {
		return false; // Sorry, this is not Nethack.
	}
    
    if (monst->creatureState == MONSTER_FLEEING
        && (monst->info.flags & (MONST_MAINTAINS_DISTANCE | MONST_FLEES_NEAR_DEATH)) || (monst->info.flags & MA_HIT_STEAL_FLEE)) {
        
        monst->creatureState = MONSTER_TRACKING_SCENT;
        monst->creatureMode = MODE_NORMAL;
    }
	
	unAlly(monst); // Sorry, no cheap dragon allies.
	healthFraction = monst->currentHP / monst->info.maxHP;
	previousDamageTaken = monst->info.maxHP - monst->currentHP;
	
	do {
		monst->info = monsterCatalog[rand_range(1, NUMBER_MONSTER_KINDS - 1)]; // Presto change-o!
	} while (monst->info.flags & (MONST_INANIMATE | MONST_NO_POLYMORPH)); // Can't turn something into an inanimate object or lich/phoenix.
	
    monst->info.turnsBetweenRegen *= 1000;
	monst->currentHP = max(1, max(healthFraction * monst->info.maxHP, monst->info.maxHP - previousDamageTaken));
	
	monst->movementSpeed = monst->info.movementSpeed;
	monst->attackSpeed = monst->info.attackSpeed;
	if (monst->status[STATUS_HASTED]) {
		monst->movementSpeed /= 2;
		monst->attackSpeed /= 2;
	}
	if (monst->status[STATUS_SLOWED]) {
		monst->movementSpeed *= 2;
		monst->attackSpeed *= 2;
	}
	
	clearStatus(monst);
	
	if (monst->info.flags & MONST_FIERY) {
		monst->status[STATUS_BURNING] = monst->maxStatus[STATUS_BURNING] = 1000; // won't decrease
	}
	if (monst->info.flags & MONST_FLIES) {
		monst->status[STATUS_LEVITATING] = monst->maxStatus[STATUS_LEVITATING] = 1000; // won't decrease
	}
	if (monst->info.flags & MONST_IMMUNE_TO_FIRE) {
		monst->status[STATUS_IMMUNE_TO_FIRE] = monst->maxStatus[STATUS_IMMUNE_TO_FIRE] = 1000; // won't decrease
	}
	if (monst->info.flags & MONST_INVISIBLE) {
		monst->status[STATUS_INVISIBLE] = monst->maxStatus[STATUS_INVISIBLE] = 1000; // won't decrease
	}
	monst->status[STATUS_NUTRITION] = monst->maxStatus[STATUS_NUTRITION] = 1000;
	
	if (monst->bookkeepingFlags & MONST_CAPTIVE) {
		demoteMonsterFromLeadership(monst);
		monst->creatureState = MONSTER_TRACKING_SCENT;
		monst->bookkeepingFlags &= ~MONST_CAPTIVE;
	}
    monst->bookkeepingFlags &= ~(MONST_SEIZING | MONST_SEIZED);
	
	monst->ticksUntilTurn = max(monst->ticksUntilTurn, 101);
	
	refreshDungeonCell(monst->xLoc, monst->yLoc);
	flashMonster(monst, boltColors[BOLT_POLYMORPH], 100);
	return true;
}

void slow(creature *monst, short turns) {
	if (!(monst->info.flags & MONST_INANIMATE)) {
		monst->status[STATUS_SLOWED] = monst->maxStatus[STATUS_SLOWED] = turns;
		monst->status[STATUS_HASTED] = 0;
		if (monst == &player) {
			updateEncumbrance();
			message("你感觉自己的移动速度变慢了。", false);
		} else {
			monst->movementSpeed = monst->info.movementSpeed * 2;
			monst->attackSpeed = monst->info.attackSpeed * 2;
		}
	}
}

void haste(creature *monst, short turns) {
	if (monst && !(monst->info.flags & MONST_INANIMATE)) {
		monst->status[STATUS_SLOWED] = 0;
		monst->status[STATUS_HASTED] = monst->maxStatus[STATUS_HASTED] = turns;
		if (monst == &player) {
			updateEncumbrance();
			message("你感觉自己的移动速度变快了。", false);
		} else {
			monst->movementSpeed = monst->info.movementSpeed / 2;
			monst->attackSpeed = monst->info.attackSpeed / 2;
		}
	}
}

void heal(creature *monst, short percent) {	
	char buf[COLS], monstName[COLS];
	monst->currentHP = min(monst->info.maxHP, monst->currentHP + percent * monst->info.maxHP / 100);
	if (canDirectlySeeMonster(monst) && monst != &player) {
		monsterName(monstName, monst, true);
		sprintf(buf, "%s看起来更健康了。", monstName);
		combatMessage(buf, NULL);
	}
}

void makePlayerTelepathic(short duration) {
    creature *monst;
    
    player.status[STATUS_TELEPATHIC] = player.maxStatus[STATUS_TELEPATHIC] = duration;
    for (monst=monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
        refreshDungeonCell(monst->xLoc, monst->yLoc);
    }
    if (monsters->nextCreature == NULL) {
        message("不知怎么的你突然很确定附近什么生物都没有。", false);
    } else {
        message("不知怎么的你突然能感觉到附近其他生物的存在！", false);
    }
}

void rechargeItems(unsigned long categories) {
    item *tempItem;
    short x, y, z, i, categoryCount;
    char buf[DCOLS * 3];
    
    x = y = z = 0; // x counts staffs, y counts wands, z counts charms
    for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
        if (tempItem->category & categories & STAFF) {
            x++;
            tempItem->charges = tempItem->enchant1;
            tempItem->enchant2 = (tempItem->kind == STAFF_BLINKING || tempItem->kind == STAFF_OBSTRUCTION ? 10000 : 5000) / tempItem->enchant1;
        }
        if (tempItem->category & categories & WAND) {
            y++;
            tempItem->charges++;
        }
        if (tempItem->category & categories & CHARM) {
            z++;
            tempItem->charges = 0;
        }
    }
    
    categoryCount = (x ? 1 : 0) + (y ? 1 : 0) + (z ? 1 : 0);
    
    if (categoryCount) {
        i = 0;
        strcpy(buf, "一股强力的能量流向你的背包里，将能量充入了你的");
        if (x) {
            i++;
            strcat(buf, "法杖");
            if (i == categoryCount - 1) {
                strcat(buf, "和");
            } else if (i <= categoryCount - 2) {
                strcat(buf, "，");
            }
        }
        if (y) {
            i++;
            strcat(buf, "魔棒");
            if (i == categoryCount - 1) {
                strcat(buf, "和");
            } else if (i <= categoryCount - 2) {
                strcat(buf, "，");
            }
        }
        if (z) {
            strcat(buf, "法器");
        }
        strcat(buf, "。");
        message(buf, false);
    } else {
        message("一股强力的能量流向你的背包里，但什么都没有发生。", false);
    }
}

//void causeFear(const char *emitterName) {
//    creature *monst;
//    short numberOfMonsters = 0;
//    char buf[DCOLS*3], mName[DCOLS];
//    
//    for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
//        if (pmap[monst->xLoc][monst->yLoc].flags & IN_FIELD_OF_VIEW
//            && monst->creatureState != MONSTER_FLEEING
//            && !(monst->info.flags & MONST_INANIMATE)) {
//            
//            monst->status[STATUS_MAGICAL_FEAR] = monst->maxStatus[STATUS_MAGICAL_FEAR] = rand_range(150, 225);
//            monst->creatureState = MONSTER_FLEEING;
//            if (canSeeMonster(monst)) {
//                numberOfMonsters++;
//                monsterName(mName, monst, true);
//            }
//        }
//    }
//    if (numberOfMonsters > 1) {
//        sprintf(buf, "%s emits a brilliant flash of red light, and the monsters flee!", emitterName);
//    } else if (numberOfMonsters == 1) {
//        sprintf(buf, "%s emits a brilliant flash of red light, and %s flees!", emitterName, mName);
//    } else {
//        sprintf(buf, "%s emits a brilliant flash of red light!", emitterName);
//    }
//    message(buf, false);
//    colorFlash(&redFlashColor, 0, IN_FIELD_OF_VIEW, 15, DCOLS, player.xLoc, player.yLoc);
//}

void negationBlast(const char *emitterName) {
    creature *monst, *nextMonst;
    item *theItem;
    char buf[DCOLS*3];
    
    sprintf(buf, "%s释放出一股强大的反魔法能流！", emitterName);
    messageWithColor(buf, &itemMessageColor, false);
    colorFlash(&pink, 0, IN_FIELD_OF_VIEW, 15, DCOLS, player.xLoc, player.yLoc);
    negate(&player);
    flashMonster(&player, &pink, 100);
    for (monst = monsters->nextCreature; monst != NULL;) {
        nextMonst = monst->nextCreature;
        if (pmap[monst->xLoc][monst->yLoc].flags & IN_FIELD_OF_VIEW) {
            if (canSeeMonster(monst)) {
                flashMonster(monst, &pink, 100);
            }
            negate(monst); // This can be fatal.
        }
        monst = nextMonst;
    }
    for (theItem = floorItems; theItem != NULL; theItem = theItem->nextItem) {
        if (pmap[theItem->xLoc][theItem->yLoc].flags & IN_FIELD_OF_VIEW) {
            theItem->flags &= ~(ITEM_MAGIC_DETECTED | ITEM_CURSED);
            switch (theItem->category) {
                case WEAPON:
                case ARMOR:
                    theItem->enchant1 = theItem->enchant2 = theItem->charges = 0;
                    theItem->flags &= ~(ITEM_RUNIC | ITEM_RUNIC_HINTED | ITEM_RUNIC_IDENTIFIED | ITEM_PROTECTED);
                    identify(theItem);
                    break;
                case STAFF:
                    theItem->charges = 0;
                    break;
                case WAND:
                    theItem->charges = 0;
                    theItem->flags |= ITEM_MAX_CHARGES_KNOWN;
                    break;
                case RING:
                    theItem->enchant1 = 0;
                    theItem->flags |= ITEM_IDENTIFIED; // Reveal that it is (now) +0, but not necessarily which kind of ring it is.
                    updateIdentifiableItems();
                case CHARM:
                    theItem->charges = charmRechargeDelay(theItem->kind, theItem->enchant1);
                    break;
                default:
                    break;
            }
        }
    }
}

void crystalize(short radius) {
	extern color forceFieldColor;
	short i, j;
	creature *monst;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j < DROWS; j++) {
			if ((player.xLoc - i) * (player.xLoc - i) + (player.yLoc - j) * (player.yLoc - j) <= radius * radius
				&& !(pmap[i][j].flags & IMPREGNABLE)) {
				
				if (i == 0 || i == DCOLS - 1 || j == 0 || j == DROWS - 1) {
					pmap[i][j].layers[DUNGEON] = CRYSTAL_WALL; // don't dissolve the boundary walls
				} else if (tileCatalog[pmap[i][j].layers[DUNGEON]].flags & (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_VISION)) {
					
					pmap[i][j].layers[DUNGEON] = FORCEFIELD;
					
					if (pmap[i][j].flags & HAS_MONSTER) {
						monst = monsterAtLoc(i, j);
						if (monst->info.flags & MONST_ATTACKABLE_THRU_WALLS) {
							inflictDamage(monst, monst->currentHP, NULL);
						} else {
							freeCaptivesEmbeddedAt(i, j);
						}
					}
				}
			}
		}
	}
	updateVision(false);
	colorFlash(&forceFieldColor, 0, 0, radius, radius, player.xLoc, player.yLoc);
	displayLevel();
	refreshSideBar(-1, -1, false);
}

boolean imbueInvisibility(creature *monst, short duration, boolean hideDetails) {
    boolean autoID = false;
    
    if (monst && !(monst->info.flags & (MONST_INANIMATE | MONST_INVISIBLE))) {
        if (monst == &player || monst->creatureState == MONSTER_ALLY) {
            autoID = true;
        }
        monst->status[STATUS_INVISIBLE] = monst->maxStatus[STATUS_INVISIBLE] = duration;
        refreshDungeonCell(monst->xLoc, monst->yLoc);
        refreshSideBar(-1, -1, false);
        if (!hideDetails) {
            flashMonster(monst, boltColors[BOLT_INVISIBILITY], 100);	
        }
    }
    return autoID;
}

boolean projectileReflects(creature *attacker, creature *defender) {	
	short prob;
    float netReflectionLevel;
	
	// immunity armor always reflects its vorpal enemy's projectiles
	if (defender == &player && rogue.armor && (rogue.armor->flags & ITEM_RUNIC) && rogue.armor->enchant2 == A_IMMUNITY
		&& rogue.armor->vorpalEnemy == attacker->info.monsterID
        && monstersAreEnemies(attacker, defender)) {
		return true;
	}
	
	if (defender == &player && rogue.armor && (rogue.armor->flags & ITEM_RUNIC) && rogue.armor->enchant2 == A_REFLECTION) {
		netReflectionLevel = netEnchant(rogue.armor);
	} else {
		netReflectionLevel = 0;
	}
	
	if (defender && (defender->info.flags & MONST_REFLECT_4)) {
        if (defender->info.flags & MONST_ALWAYS_USE_ABILITY) {
            return true;
        }
		netReflectionLevel += 4;
	}
	
	if (netReflectionLevel <= 0) {
		return false;
	}
	
	prob = reflectionChance(netReflectionLevel);
	
	return rand_percent(prob);
}

// returns the path length of the reflected path, alters listOfCoordinates to describe reflected path
short reflectBolt(short targetX, short targetY, short listOfCoordinates[][2], short kinkCell, boolean retracePath) {
	short k, target[2], origin[2], newPath[DCOLS][2], newPathLength, failsafe, finalLength;
	boolean needRandomTarget;
	
	needRandomTarget = (targetX < 0 || targetY < 0
						|| (targetX == listOfCoordinates[kinkCell][0] && targetY == listOfCoordinates[kinkCell][1]));
	
	if (retracePath) {
		// if reflecting back at caster, follow precise trajectory until we reach the caster
		for (k = 1; k <= kinkCell && kinkCell + k < MAX_BOLT_LENGTH; k++) {
			listOfCoordinates[kinkCell + k][0] = listOfCoordinates[kinkCell - k][0];
			listOfCoordinates[kinkCell + k][1] = listOfCoordinates[kinkCell - k][1];
		}
		
		// Calculate a new "extension" path, with an origin at the caster, and a destination at
		// the caster's location translated by the vector from the reflection point to the caster.
		// 
		// For example, if the player is at (0,0), and the caster is at (2,3), then the newpath
		// is from (2,3) to (4,6):
		// (2,3) + ((2,3) - (0,0)) = (4,6).
		
		origin[0] = listOfCoordinates[2 * kinkCell][0];
		origin[1] = listOfCoordinates[2 * kinkCell][1];
		target[0] = targetX + (targetX - listOfCoordinates[kinkCell][0]);
		target[1] = targetY + (targetY - listOfCoordinates[kinkCell][1]);
		newPathLength = getLineCoordinates(newPath, origin, target);
		
		for (k=0; k<=newPathLength; k++) {
			listOfCoordinates[2 * kinkCell + k + 1][0] = newPath[k][0];
			listOfCoordinates[2 * kinkCell + k + 1][1] = newPath[k][1];
		}
		finalLength = 2 * kinkCell + newPathLength + 1;
	} else {
		failsafe = 50;
		do {
			if (needRandomTarget) {
				// pick random target
				perimeterCoords(target, rand_range(0, 39));
				target[0] += listOfCoordinates[kinkCell][0];
				target[1] += listOfCoordinates[kinkCell][1];
			} else {
				target[0] = targetX;
				target[1] = targetY;
			}
			
			newPathLength = getLineCoordinates(newPath, listOfCoordinates[kinkCell], target);
			
			if (newPathLength > 0 && !cellHasTerrainFlag(newPath[0][0], newPath[0][1], (T_OBSTRUCTS_VISION | T_OBSTRUCTS_PASSABILITY))) {
				needRandomTarget = false;
			}
			
		} while (needRandomTarget && --failsafe);
		
		for (k = 0; k < newPathLength; k++) {
			listOfCoordinates[kinkCell + k + 1][0] = newPath[k][0];
			listOfCoordinates[kinkCell + k + 1][1] = newPath[k][1];
		}
		
		finalLength = kinkCell + newPathLength + 1;
	}
	
	listOfCoordinates[finalLength][0] = -1;
	listOfCoordinates[finalLength][1] = -1;
	return finalLength;
}

// Update stuff that promotes without keys so players can't abuse item libraries with blinking/haste shenanigans
void checkForMissingKeys(short x, short y) {
	short layer;

	if (cellHasTMFlag(x, y, TM_PROMOTES_WITHOUT_KEY) && !keyOnTileAt(x, y)) {
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[x][y].layers[layer]].mechFlags & TM_PROMOTES_WITHOUT_KEY) {
				promoteTile(x, y, layer, false);
			}
		}
	}
}

// returns whether the bolt effect should autoID any staff or wand it came from, if it came from a staff or wand
boolean zap(short originLoc[2], short targetLoc[2], enum boltType bolt, short boltLevel, boolean hideDetails) {
	short listOfCoordinates[MAX_BOLT_LENGTH][2];
	short i, j, k, x, y, x2, y2, numCells, blinkDistance, boltLength, initialBoltLength, newLoc[2];
	LIGHTING_STATE *lights;
	short poisonDamage;
	creature *monst = NULL, *shootingMonst, *newMonst;
	char buf[COLS*3], monstName[COLS*3];
	boolean autoID = false;
	boolean fastForward = false;
	boolean beckonedMonster = false;
	boolean alreadyReflected = false;
	boolean boltInView;
	color *boltColor;
    //color boltImpactColor;
	dungeonFeature feat;
	
#ifdef BROGUE_ASSERTS
	assert(originLoc[0] != targetLoc[0] || originLoc[1] != targetLoc[1]);
#else
	if (originLoc[0] == targetLoc[0] && originLoc[1] == targetLoc[1]) {
		return false;
	}
#endif
	
	x = originLoc[0];
	y = originLoc[1];
	
	initialBoltLength = boltLength = 5 * boltLevel;
    
	lightSource boltLights[initialBoltLength];
	color boltLightColors[initialBoltLength];
    
	numCells = getLineCoordinates(listOfCoordinates, originLoc, targetLoc);
	
	shootingMonst = monsterAtLoc(originLoc[0], originLoc[1]);
	
	if (!hideDetails) {
		boltColor = boltColors[bolt];
	} else {
		boltColor = &gray;
	}
    
    //boltImpactColor = *boltColor;
    //applyColorScalar(&boltImpactColor, 5000);
	
	refreshSideBar(-1, -1, false);
    displayCombatText(); // To announce who fired the bolt while the animation plays.
	
	blinkDistance = 0;
	if (bolt == BOLT_BLINKING) {
		if (cellHasTerrainFlag(listOfCoordinates[0][0], listOfCoordinates[0][1], (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_VISION))
			|| (pmap[listOfCoordinates[0][0]][listOfCoordinates[0][1]].flags & (HAS_PLAYER | HAS_MONSTER)
				&& !(monsterAtLoc(listOfCoordinates[0][0], listOfCoordinates[0][1])->bookkeepingFlags & MONST_SUBMERGED))) {
				// shooting blink point-blank into an obstruction does nothing.
				return false;
			}
		pmap[originLoc[0]][originLoc[1]].flags &= ~(HAS_PLAYER | HAS_MONSTER);
		refreshDungeonCell(originLoc[0], originLoc[1]);
		blinkDistance = boltLevel * 2 + 1;
		checkForMissingKeys(originLoc[0], originLoc[1]);
	}
	
	for (i=0; i<initialBoltLength; i++) {
		boltLightColors[i] = *boltColor;
		boltLights[i] = lightCatalog[BOLT_LIGHT_SOURCE];
		boltLights[i].lightColor = &boltLightColors[i];
		boltLights[i].lightRadius.lowerBound = boltLights[i].lightRadius.upperBound = 50 * (3 + boltLevel * 1.33) * (initialBoltLength - i) / initialBoltLength;
	}
	
	if (bolt == BOLT_TUNNELING) {
		tunnelize(originLoc[0], originLoc[1]);
	}
	
	lights = backUpLighting();
	boltInView = true;
	for (i=0; i<numCells; i++) {
		
		x = listOfCoordinates[i][0];
		y = listOfCoordinates[i][1];
		
		monst = monsterAtLoc(x, y);
		
		// Player travels inside the bolt when it is blinking.
		if (bolt == BOLT_BLINKING && shootingMonst == &player) {
			player.xLoc = x;
			player.yLoc = y;
			updateVision(true);

			freeLightingState(lights);
			lights = backUpLighting();
		}
		
		// Firebolts light things on fire, and the effect is updated in realtime.
		if (!monst && bolt == BOLT_FIRE) {
			if (exposeTileToFire(x, y, true)) {
				updateVision(true);

				freeLightingState(lights);
				lights = backUpLighting();
				autoID = true;
			}
		}
		
		// Update the visual effect of the bolt. This lighting effect is expensive; do it only if the player can see the bolt.
		if (boltInView) {
			demoteVisibility();
			restoreLighting(lights);
			for (k = min(i, boltLength + 2); k >= 0; k--) {
				if (k < initialBoltLength) {
					paintLight(&boltLights[k], listOfCoordinates[i-k][0], listOfCoordinates[i-k][1], false, false);
				}
			}
		}
		boltInView = false;
		updateFieldOfViewDisplay(false, true);
		for (k = min(i, boltLength + 2); k >= 0; k--) {
			if (playerCanSeeOrSense(listOfCoordinates[i-k][0], listOfCoordinates[i-k][1])) {
				hiliteCell(listOfCoordinates[i-k][0], listOfCoordinates[i-k][1], boltColor, max(0, 100 - k * 100 / (boltLength)), false);
			}
            if (pmap[listOfCoordinates[i-k][0]][listOfCoordinates[i-k][1]].flags & IN_FIELD_OF_VIEW) {
                boltInView = true;
            }
		}
		if (!fastForward && (boltInView || rogue.playbackOmniscience)) {
			fastForward = rogue.playbackFastForward || pauseBrogue(5);
		}
		
		// Handle bolt reflection off of creatures (reflection off of terrain is handled further down).
		if (monst && projectileReflects(shootingMonst, monst) && i < DCOLS*2) {
			if (projectileReflects(shootingMonst, monst)) { // if it scores another reflection roll, reflect at caster
				numCells = reflectBolt(originLoc[0], originLoc[1], listOfCoordinates, i, !alreadyReflected);
			} else {
				numCells = reflectBolt(-1, -1, listOfCoordinates, i, false); // otherwise reflect randomly
			}
			
			alreadyReflected = true;
			
			if (boltInView) {
				monsterName(monstName, monst, true);
				sprintf(buf, "%s反射了魔法效果", monstName);
				combatMessage(buf, 0);
				
				if (monst == &player
					&& rogue.armor
					&& rogue.armor->enchant2 == A_REFLECTION
					&& !(rogue.armor->flags & ITEM_RUNIC_IDENTIFIED)) {
					
					rogue.armor->flags |= (ITEM_RUNIC_IDENTIFIED | ITEM_RUNIC_HINTED);
				}
			}
			continue;
		}
		
		if (bolt == BOLT_BLINKING) {
			boltLevel = (blinkDistance - i) / 2 + 1;
			boltLength = boltLevel * 5;
			for (j=0; j<i; j++) {
				refreshDungeonCell(listOfCoordinates[j][0], listOfCoordinates[j][1]);
			}
			if (i >= blinkDistance) {
				break;
			}
		}
		
		// Some bolts halt at the square before they hit something.
		if ((bolt == BOLT_BLINKING || bolt == BOLT_OBSTRUCTION || bolt == BOLT_CONJURATION)
			&& i + 1 < numCells
			&& (cellHasTerrainFlag(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1],
								   (T_OBSTRUCTS_VISION | T_OBSTRUCTS_PASSABILITY))
				|| (pmap[listOfCoordinates[i+1][0]][listOfCoordinates[i+1][1]].flags & (HAS_PLAYER | HAS_MONSTER)))) {
				if (pmap[listOfCoordinates[i+1][0]][listOfCoordinates[i+1][1]].flags & HAS_MONSTER) {
					monst = monsterAtLoc(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1]);
					if (!(monst->bookkeepingFlags & MONST_SUBMERGED)) {
						break;
					}
				} else {
					break;
				}
			}
		
		// Lightning hits monsters as it travels.
		if (monst && (pmap[x][y].flags & (HAS_PLAYER | HAS_MONSTER)) && (bolt == BOLT_LIGHTNING) && (!monst || !(monst->bookkeepingFlags & MONST_SUBMERGED))) {
			monsterName(monstName, monst, true);
			
			autoID = true;
			
			if (inflictDamage(monst, staffDamage(boltLevel), &lightningColor)) {
				// killed monster
				if (player.currentHP <= 0) {
					if (shootingMonst == &player) {
						gameOver("被反弹的闪电魔法杀死。", true);
					}
					return false;
				}
				if (boltInView || canSeeMonster(monst)) {
					sprintf(buf, "%s闪电%s%s",
							canSeeMonster(shootingMonst) ? "" : "一道",
							((monst->info.flags & MONST_INANIMATE) ? "摧毁了" : "杀死了"),
							monstName);
					combatMessage(buf, messageColorFromVictim(monst));
				} else {
					sprintf(buf, "你听到%s%s", monstName, ((monst->info.flags & MONST_INANIMATE) ? "被摧毁了" : "死掉了"));
					combatMessage(buf, messageColorFromVictim(monst));
				}
			} else {
				// monster lives
				if (monst->creatureMode != MODE_PERM_FLEEING
					&& monst->creatureState != MONSTER_ALLY
					&& (monst->creatureState != MONSTER_FLEEING || monst->status[STATUS_MAGICAL_FEAR])) {
                    
					monst->creatureState = MONSTER_TRACKING_SCENT;
					monst->status[STATUS_MAGICAL_FEAR] = 0;
				}
				if (boltInView) {
					sprintf(buf, "%s闪电击中了%s",
							canSeeMonster(shootingMonst) ? "" : "一道",
							monstName);
					combatMessage(buf, messageColorFromVictim(monst));
				}
				
				moralAttack(shootingMonst, monst);
			}
		}
		
        // Stop when we hit something -- a wall or a non-submerged creature.
        // However, lightning and tunneling don't stop when they hit creatures, and tunneling continues through walls too.
		if (cellHasTerrainFlag(x, y, (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_VISION))
			|| (((pmap[x][y].flags & HAS_PLAYER) || ((pmap[x][y].flags & HAS_MONSTER) && monst && !(monst->bookkeepingFlags & MONST_SUBMERGED))) && bolt != BOLT_LIGHTNING)) {
			
			if (bolt == BOLT_TUNNELING && x > 0 && y > 0 && x < DCOLS - 1 && y < DROWS - 1) { // don't tunnelize the outermost walls
				tunnelize(x, y);
				if (i > 0 && x != listOfCoordinates[i-1][0] && y != listOfCoordinates[i-1][1]) {
					if ((pmap[x][listOfCoordinates[i-1][1]].flags & IMPREGNABLE)
                        || (rand_percent(50) && !(pmap[listOfCoordinates[i-1][0]][y].flags & IMPREGNABLE))) {
						tunnelize(listOfCoordinates[i-1][0], y);
					} else {
						tunnelize(x, listOfCoordinates[i-1][1]);
					}
				} else if (i == 0 && x > 0 && y > 0 && x < DCOLS - 1 && y < DROWS - 1) {
					if (rand_percent(50)) {
						tunnelize(originLoc[0], y);
					} else {
						tunnelize(x, originLoc[1]);
					}
				}
				updateVision(true);

				freeLightingState(lights);
				lights = backUpLighting();
				autoID = true;
				boltLength = --boltLevel * 5;
				for (j=0; j<i; j++) {
					refreshDungeonCell(listOfCoordinates[j][0], listOfCoordinates[j][1]);
				}
				if (!boltLevel) {
					refreshDungeonCell(listOfCoordinates[i-1][0], listOfCoordinates[i-1][1]);
					refreshDungeonCell(x, y);
					break;
				}
			} else {
				break;
			}
		}
		
		// does the bolt bounce off the wall?
		// Can happen with a cursed deflection ring or a reflective terrain target, or when shooting a tunneling bolt into an impregnable wall.
		if (i + 1 < numCells
			&& cellHasTerrainFlag(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1],
								  (T_OBSTRUCTS_VISION | T_OBSTRUCTS_PASSABILITY))
			&& (projectileReflects(shootingMonst, NULL)
                || cellHasTMFlag(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1], TM_REFLECTS_BOLTS)
				|| (bolt == BOLT_TUNNELING && (pmap[listOfCoordinates[i+1][0]][listOfCoordinates[i+1][1]].flags & IMPREGNABLE)))
			&& i < DCOLS*2) {
			
			sprintf(buf, "%s将魔法效果弹开了", tileText(listOfCoordinates[i+1][0], listOfCoordinates[i+1][1]));
			
			if (projectileReflects(shootingMonst, NULL)) { // if it scores another reflection roll, reflect at caster
				numCells = reflectBolt(originLoc[0], originLoc[1], listOfCoordinates, i, !alreadyReflected);
			} else {
				numCells = reflectBolt(-1, -1, listOfCoordinates, i, false); // otherwise reflect randomly
			}
			
			alreadyReflected = true;
			
			if (boltInView) {
				combatMessage(buf, 0);
			}
			continue;
		}
	}
	
	if (bolt == BOLT_BLINKING) {
		if (pmap[x][y].flags & HAS_MONSTER) { // We're blinking onto an area already occupied by a submerged monster.
			// Make sure we don't get the shooting monster by accident.
			shootingMonst->xLoc = shootingMonst->yLoc = -1; // Will be set back to the destination in a moment.
			monst = monsterAtLoc(x, y);
			findAlternativeHomeFor(monst, &x2, &y2, true);
			if (x2 >= 0) {
				// Found an alternative location.
				monst->xLoc = x2;
				monst->yLoc = y2;
				pmap[x][y].flags &= ~HAS_MONSTER;
				pmap[x2][y2].flags |= HAS_MONSTER;
			} else {
				// No alternative location?? Hard to imagine how this could happen.
				// Just bury the monster and never speak of this incident again.
				killCreature(monst, true);
				pmap[x][y].flags &= ~HAS_MONSTER;
				monst = NULL;
			}
		}
		shootingMonst->bookkeepingFlags &= ~MONST_SUBMERGED;
		pmap[x][y].flags |= (shootingMonst == &player ? HAS_PLAYER : HAS_MONSTER);
		shootingMonst->xLoc = x;
		shootingMonst->yLoc = y;
		applyInstantTileEffectsToCreature(shootingMonst);
		
		if (shootingMonst == &player) {
			updateVision(true);
		}
		autoID = true;
	} else if (bolt == BOLT_BECKONING) {
		if (monst && !(monst->info.flags & MONST_IMMOBILE)
			&& distanceBetween(originLoc[0], originLoc[1], monst->xLoc, monst->yLoc) > 1) {
			beckonedMonster = true;
			fastForward = true;
		}
	}
	
	if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
		monst = monsterAtLoc(x, y);
		monsterName(monstName, monst, true);
	} else {
		monst = NULL;
	}
	
	switch(bolt) {
		case BOLT_TELEPORT:
			if (monst && !(monst->info.flags & MONST_IMMOBILE)) {
				if (monst->bookkeepingFlags & MONST_CAPTIVE) {
					freeCaptive(monst);
				}
				teleport(monst, -1, -1, false);
				//refreshSideBar(-1, -1, false);
			}
			break;
		case BOLT_BECKONING:
			if (beckonedMonster && monst) {
				if (canSeeMonster(monst)) {
					autoID = true;
				}
				if (monst->bookkeepingFlags & MONST_CAPTIVE) {
					freeCaptive(monst);
				}
				newLoc[0] = monst->xLoc;
				newLoc[1] = monst->yLoc;
				zap(newLoc, originLoc, BOLT_BLINKING, max(1, (distanceBetween(originLoc[0], originLoc[1], newLoc[0], newLoc[1]) - 2) / 2), false);
				if (monst->ticksUntilTurn < player.attackSpeed+1) {
					monst->ticksUntilTurn = player.attackSpeed+1;
				}
				if (canSeeMonster(monst)) {
					autoID = true;
				}
			}
			break;
		case BOLT_SLOW:
			if (monst) {
				slow(monst, boltLevel);
				//refreshSideBar(-1, -1, false);
				flashMonster(monst, boltColors[BOLT_SLOW], 100);
				autoID = true;
			}
			break;
		case BOLT_HASTE:
			if (monst) {
				haste(monst, staffHasteDuration(boltLevel));
				//refreshSideBar(-1, -1, false);
				flashMonster(monst, boltColors[BOLT_HASTE], 100);
				autoID = true;
			}
			break;
		case BOLT_POLYMORPH:
			if (monst && monst != &player && !(monst->info.flags & MONST_INANIMATE)) {
				polymorph(monst);
				//refreshSideBar(-1, -1, false);
				if (!monst->status[STATUS_INVISIBLE]) {
					autoID = true;
				}
			}
			break;
		case BOLT_INVISIBILITY:
            autoID = imbueInvisibility(monst, 150, hideDetails);
			break;
		case BOLT_DOMINATION:
			if (monst && monst != &player && !(monst->info.flags & MONST_INANIMATE)) {
				if (rand_percent(wandDominate(monst))) {
					// domination succeeded
					monst->status[STATUS_DISCORDANT] = 0;
					becomeAllyWith(monst);
					//refreshSideBar(-1, -1, false);
					refreshDungeonCell(monst->xLoc, monst->yLoc);
					if (canSeeMonster(monst)) {
						autoID = true;
						sprintf(buf, "你支配了%s的意识！", monstName);
						message(buf, false);
						flashMonster(monst, boltColors[BOLT_DOMINATION], 100);
					}
				} else if (canSeeMonster(monst)) {
					autoID = true;
					sprintf(buf, "%s抵抗了支配魔法。", monstName);
					message(buf, false);
				}
			}
			break;
		case BOLT_NEGATION:
			if (monst) { // works on inanimates
				negate(monst);
				//refreshSideBar(-1, -1, false);
				flashMonster(monst, boltColors[BOLT_NEGATION], 100);
			}
			break;
		case BOLT_LIGHTNING:
			// already handled above
			break;
		case BOLT_POISON:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				poisonDamage = staffPoison(boltLevel);
				monst->status[STATUS_POISONED] += poisonDamage;
				monst->maxStatus[STATUS_POISONED] = monst->info.maxHP;
				//refreshSideBar(-1, -1, false);
				if (canSeeMonster(monst)) {
					flashMonster(monst, boltColors[BOLT_POISON], 100);
					autoID = true;
					if (monst != &player) {
						sprintf(buf, "%s%s%s",
								monstName,
								(monst == &player ? "感觉自己" : "看起来"),
								(monst->status[STATUS_POISONED] > monst->currentHP && !player.status[STATUS_HALLUCINATING] ? "中了致命的毒" : "中毒了"));
						combatMessage(buf, messageColorFromVictim(monst));
					}
				}
			}
			break;
		case BOLT_FIRE:
			if (monst) {
				autoID = true;
				
				if (monst->status[STATUS_IMMUNE_TO_FIRE] > 0) {
					if (canSeeMonster(monst)) {
						sprintf(buf, "%s完全无视了%s火球",
								monstName,
								canSeeMonster(shootingMonst) ? "这个" : "");
						combatMessage(buf, 0);
					}
				} else if (inflictDamage(monst, staffDamage(boltLevel), &orange)) {
					// killed creature
					
					if (player.currentHP <= 0) {
						if (shootingMonst == &player) {
							gameOver("被反弹的火球杀死。", true);
						}
						return false;
					}
					
					if (boltInView || canSeeMonster(monst)) {
						sprintf(buf, "%s火球%s%s",
								canSeeMonster(shootingMonst) ? "" : "一个",
								((monst->info.flags & MONST_INANIMATE) ? "摧毁了" : "杀死了"),
								monstName);
						combatMessage(buf, messageColorFromVictim(monst));
					} else {
						sprintf(buf, "你听到%s%s", monstName, ((monst->info.flags & MONST_INANIMATE) ? "被摧毁了" : "死掉了"));
						combatMessage(buf, messageColorFromVictim(monst));
					}

				} else {
					// monster lives
					if (monst->creatureMode != MODE_PERM_FLEEING
						&& monst->creatureState != MONSTER_ALLY
						&& (monst->creatureState != MONSTER_FLEEING || monst->status[STATUS_MAGICAL_FEAR])) {
                        
						monst->creatureState = MONSTER_TRACKING_SCENT;
					}
					if (boltInView) {
						sprintf(buf, "%s火球击中了%s",
								canSeeMonster(shootingMonst) ? "" : "一个",
								monstName);
						combatMessage(buf, messageColorFromVictim(monst));
					}
					exposeCreatureToFire(monst);
					
					moralAttack(shootingMonst, monst);
				}
				//refreshSideBar(-1, -1, false);
			}
			exposeTileToFire(x, y, true); // burninate
			break;
		case BOLT_BLINKING:
			if (shootingMonst == &player) {
				// handled above for visual effect (i.e. before contrail fades)
				// increase scent turn number so monsters don't sniff around at the old cell like idiots
				rogue.scentTurnNumber += 30;
				// get any items at the destination location
				if (pmap[player.xLoc][player.yLoc].flags & HAS_ITEM) {
					pickUpItemAt(player.xLoc, player.yLoc);
				}
			}
			break;
		case BOLT_ENTRANCEMENT:
			if (monst && monst == &player) {
				flashMonster(monst, &confusionGasColor, 100);
				monst->status[STATUS_CONFUSED] = staffEntrancementDuration(boltLevel);
				monst->maxStatus[STATUS_CONFUSED] = max(monst->status[STATUS_CONFUSED], monst->maxStatus[STATUS_CONFUSED]);
				//refreshSideBar(-1, -1, false);
				message("法术击中了你，你突然感觉失去了方向。", true);
				autoID = true;
			} else if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				monst->status[STATUS_ENTRANCED] = monst->maxStatus[STATUS_ENTRANCED] = staffEntrancementDuration(boltLevel);
				//refreshSideBar(-1, -1, false);
				wakeUp(monst);
				if (canSeeMonster(monst)) {
					flashMonster(monst, boltColors[BOLT_ENTRANCEMENT], 100);
					autoID = true;
					sprintf(buf, "%s被法术迷惑了！", monstName);
					message(buf, false);
				}
			}
			break;
		case BOLT_HEALING:
			if (monst) {
				heal(monst, boltLevel * 10);
				//refreshSideBar(-1, -1, false);
				if (canSeeMonster(monst)) {
					autoID = true;
				}
			}
			break;
		case BOLT_OBSTRUCTION:
			feat = dungeonFeatureCatalog[DF_FORCEFIELD];
			feat.probabilityDecrement = max(1, 75 * pow(0.8, boltLevel));
			spawnDungeonFeature(x, y, &feat, true, false);
			autoID = true;
			break;
		case BOLT_TUNNELING:
			if (autoID) { // i.e. if it did something
				setUpWaypoints(); // recompute based on the new situation
			}
			break;
		case BOLT_PLENTY:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				newMonst = cloneMonster(monst, true, true);
				if (newMonst) {
					newMonst->currentHP = (newMonst->currentHP + 1) / 2;
					monst->currentHP = (monst->currentHP + 1) / 2;
					//refreshSideBar(-1, -1, false);
					flashMonster(monst, boltColors[BOLT_PLENTY], 100);
					flashMonster(newMonst, boltColors[BOLT_PLENTY], 100);
					autoID = true;
				}
			}
			break;
		case BOLT_DISCORD:
			if (monst && !(monst->info.flags & MONST_INANIMATE)) {
				monst->status[STATUS_DISCORDANT] = monst->maxStatus[STATUS_DISCORDANT] = max(staffDiscordDuration(boltLevel), monst->status[STATUS_DISCORDANT]);
				if (canSeeMonster(monst)) {
					flashMonster(monst, boltColors[BOLT_DISCORD], 100);
					autoID = true;
				}
			}
			break;
		case BOLT_CONJURATION:
			
			for (j = 0; j < (staffBladeCount(boltLevel)); j++) {
				monst = generateMonster(MK_SPECTRAL_BLADE, true, false);
                getQualifyingPathLocNear(&(monst->xLoc), &(monst->yLoc), x, y, true,
                                         T_DIVIDES_LEVEL & avoidedFlagsForMonster(&(monst->info)) & ~T_SPONTANEOUSLY_IGNITES, HAS_PLAYER,
                                         avoidedFlagsForMonster(&(monst->info)) & ~T_SPONTANEOUSLY_IGNITES, (HAS_PLAYER | HAS_MONSTER | HAS_UP_STAIRS | HAS_DOWN_STAIRS), false);
				monst->bookkeepingFlags |= (MONST_FOLLOWER | MONST_BOUND_TO_LEADER | MONST_DOES_NOT_TRACK_LEADER);
				monst->bookkeepingFlags &= ~MONST_JUST_SUMMONED;
				monst->leader = &player;
				monst->creatureState = MONSTER_ALLY;
				monst->ticksUntilTurn = monst->info.attackSpeed + 1; // So they don't move before the player's next turn.
				pmap[monst->xLoc][monst->yLoc].flags |= HAS_MONSTER;
				//refreshDungeonCell(monst->xLoc, monst->yLoc);
			}
            updateVision(true);
			//refreshSideBar(-1, -1, false);
			monst = NULL;
			autoID = true;
			break;
		case BOLT_SHIELDING:
			if (monst) {
                if (staffProtection(boltLevel) > monst->status[STATUS_SHIELDED]) {
                    monst->status[STATUS_SHIELDED] = staffProtection(boltLevel);
                }
                monst->maxStatus[STATUS_SHIELDED] = monst->status[STATUS_SHIELDED];
				flashMonster(monst, boltColors[BOLT_SHIELDING], 100);
				autoID = true;
			}
			break;
		default:
			break;
	}
	
	updateLighting();
	freeLightingState(lights);
	lights = backUpLighting();
	refreshSideBar(-1, -1, false);
	if (boltLength > 0) {
		// j is where the front tip of the bolt would be if it hadn't collided at i
		for (j=i; j < i + boltLength + 2; j++) { // j can imply a bolt tip position that is off the map
			
			demoteVisibility();
			restoreLighting(lights);
                
			for (k = min(j, boltLength + 2); k >= j-i; k--) {
				if (k < initialBoltLength) {
					paintLight(&boltLights[k], listOfCoordinates[j-k][0], listOfCoordinates[j-k][1], false, false);
				}
			}
			updateFieldOfViewDisplay(false, true);
			
			
			// beam graphic
			// k iterates from the tail tip of the visible portion of the bolt to the head
			for (k = min(j, boltLength + 2); k >= j-i; k--) {
				if (playerCanSee(listOfCoordinates[j-k][0], listOfCoordinates[j-k][1])) {
					hiliteCell(listOfCoordinates[j-k][0], listOfCoordinates[j-k][1], boltColor, max(0, 100 - k * 100 / (boltLength)), false);
				}
			}
			
			if (!fastForward && boltInView) {
				fastForward = rogue.playbackFastForward || pauseBrogue(5);
			}
		}
	}
	freeLightingState(lights);

	return autoID;
}

// Relies on the sidebar entity list. If one is already selected, select the next qualifying. Otherwise, target the first qualifying.
boolean nextTargetAfter(short *returnX,
                        short *returnY,
                        short targetX,
                        short targetY,
                        boolean targetEnemies,
                        boolean targetAllies,
                        boolean targetItems,
                        boolean targetTerrain,
                        boolean requireOpenPath) {
    short i, n;
    short selectedIndex = -1;
    creature *monst;
    item *theItem;
    
    for (i=0; i<ROWS; i++) {
        if (rogue.sidebarLocationList[i][0] == targetX
            && rogue.sidebarLocationList[i][1] == targetY
            && (i == ROWS - 1 || rogue.sidebarLocationList[i+1][0] != targetX || rogue.sidebarLocationList[i+1][1] != targetY)) {
            
            selectedIndex = i;
            break;
        }
    }
    
    for (i=1; i<=ROWS; i++) {
        n = (selectedIndex + i) % ROWS;
        targetX = rogue.sidebarLocationList[n][0];
        targetY = rogue.sidebarLocationList[n][1];
        if (targetX != -1
            && (targetX != player.xLoc || targetY != player.yLoc)
            && (!requireOpenPath || openPathBetween(player.xLoc, player.yLoc, targetX, targetY))) {

#ifdef BROGUE_ASSERTS
            assert(coordinatesAreInMap(targetX, targetY));
#endif

            monst = monsterAtLoc(targetX, targetY);
            if (monst) {
                
                if (monstersAreEnemies(&player, monst)) {
                    if (targetEnemies) {
                        *returnX = targetX;
                        *returnY = targetY;
                        return true;
                    }
                } else {
                    if (targetAllies) {
                        *returnX = targetX;
                        *returnY = targetY;
                        return true;
                    }
                }
            }
            
            theItem = itemAtLoc(targetX, targetY);
            if (!monst && theItem && targetItems) {
                *returnX = targetX;
                *returnY = targetY;
                return true;
            }
            
            if (!monst && !theItem && targetTerrain) {
                *returnX = targetX;
                *returnY = targetY;
                return true;
            }
        }
    }
    return false;
}
    
//	creature *currentTarget, *monst, *returnMonst = NULL;
//	short currentDistance, shortestDistance;
//	
//	currentTarget = monsterAtLoc(targetX, targetY);
//	
//	if (!currentTarget || currentTarget == &player) {
//		currentTarget = monsters;
//		currentDistance = 0;
//	} else {
//		currentDistance = distanceBetween(player.xLoc, player.yLoc, targetX, targetY);
//	}
//	
//	// first try to find a monster with the same distance later in the chain.
//	for (monst = currentTarget->nextCreature; monst != NULL; monst = monst->nextCreature) {
//		if (distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc) == currentDistance
//			&& canSeeMonster(monst)
//			&& (targetAllies == (monst->creatureState == MONSTER_ALLY || (monst->bookkeepingFlags & MONST_CAPTIVE)))
//			&& (!requireOpenPath || openPathBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc))) {
//			
//			// got one!
//			returnMonst = monst;
//			break;
//		}
//	}
//	
//	if (!returnMonst) {
//		// okay, instead pick the qualifying monster (excluding the current target)
//		// with the shortest distance greater than currentDistance.
//		shortestDistance = max(DCOLS, DROWS);
//		for (monst = currentTarget->nextCreature;; monst = monst->nextCreature) {
//			if (monst == NULL) {
//				monst = monsters;
//			} else if (distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc) < shortestDistance
//					   && distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc) > currentDistance
//					   && canSeeMonster(monst)
//					   && (targetAllies == (monst->creatureState == MONSTER_ALLY || (monst->bookkeepingFlags & MONST_CAPTIVE)))
//					   && (!requireOpenPath || openPathBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc))) {
//				// potentially this one
//				shortestDistance = distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc);
//				returnMonst = monst;
//			}
//			if (monst == currentTarget) {
//				break;
//			}
//		}
//	}
//	
//	if (!returnMonst) {
//		// okay, instead pick the qualifying monster (excluding the current target)
//		// with the shortest distance period.
//		shortestDistance = max(DCOLS, DROWS);
//		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
//			if (distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc) < shortestDistance
//				&& canSeeMonster(monst)
//				&& (targetAllies == (monst->creatureState == MONSTER_ALLY || (monst->bookkeepingFlags & MONST_CAPTIVE)))
//				&& (!requireOpenPath || openPathBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc))) {
//				// potentially this one
//				shortestDistance = distanceBetween(player.xLoc, player.yLoc, monst->xLoc, monst->yLoc);
//				returnMonst = monst;
//			}
//		}
//	}
//	
//	if (returnMonst) {
//		plotCharWithColor(returnMonst->info.displayChar, mapToWindowX(returnMonst->xLoc),
//						  mapToWindowY(returnMonst->yLoc), returnMonst->info.foreColor, &white);
//	}
//	
//	return returnMonst;
//}

// Returns how far it went before hitting something.
short hiliteTrajectory(short coordinateList[DCOLS][2], short numCells, boolean eraseHiliting, boolean passThroughMonsters) {
	short x, y, i;
	creature *monst;
    
	for (i=0; i<numCells; i++) {
		x = coordinateList[i][0];
		y = coordinateList[i][1];
		if (eraseHiliting) {
			refreshDungeonCell(x, y);
		} else {
			//hiliteCell(x, y, &hiliteColor, 50, true); // yellow
			hiliteCell(x, y, &red, 50, true); // red
		}
		
		if (cellHasTerrainFlag(x, y, (T_OBSTRUCTS_VISION | T_OBSTRUCTS_PASSABILITY))
			|| pmap[x][y].flags & (HAS_PLAYER)) {
			i++;
			break;
		} else if (!(pmap[x][y].flags & DISCOVERED)) {
			break;
		} else if (!passThroughMonsters && pmap[x][y].flags & (HAS_MONSTER)
				   && (playerCanSee(x, y) || player.status[STATUS_TELEPATHIC])) {
			monst = monsterAtLoc(x, y);
			if (!(monst->bookkeepingFlags & MONST_SUBMERGED)
				&& (!monst->status[STATUS_INVISIBLE] || player.status[STATUS_TELEPATHIC])) {
                
				i++;
				break;
			}
		}
	}
	return i;
}

// Event is optional. Returns true if the event should be executed by the parent function.
boolean moveCursor(boolean *targetConfirmed,
				   boolean *canceled,
				   boolean *tabKey,
				   short targetLoc[2],
				   rogueEvent *event,
				   buttonState *state,
				   BROGUE_DRAW_CONTEXT *button_context,
				   BROGUE_EFFECT *button_effect,
				   boolean colorsDance,
				   boolean keysMoveCursor,
				   boolean targetCanLeaveMap) {
	signed long keystroke;
	short moveIncrement;
	short buttonInput;
	boolean cursorMovementCommand, again, movementKeystroke, sidebarHighlighted;
	rogueEvent theEvent;
	
	short *cursor = rogue.cursorLoc; // shorthand
	
	cursor[0] = targetLoc[0];
	cursor[1] = targetLoc[1];
	
	*targetConfirmed = *canceled = *tabKey = false;
	sidebarHighlighted = false;
	
	do {
		again = false;
		cursorMovementCommand = false;
		movementKeystroke = false;
		
		assureCosmeticRNG;
		
		if (state) { // Also running a button loop.
			// Get input.
			nextBrogueEvent(&theEvent, false, colorsDance, true);
			
			// Process the input.
			buttonInput = processButtonInput(
				button_context, button_effect, state, NULL, &theEvent);
			
			if (buttonInput != -1) {
				state->buttonDepressed = state->buttonFocused = -1;
				drawButtonsInState(button_context, button_effect, state);
			}
		} else { // No buttons to worry about.
			nextBrogueEvent(&theEvent, false, colorsDance, true);
		}
		restoreRNG;
		
		if (theEvent.eventType == MOUSE_UP || theEvent.eventType == MOUSE_ENTERED_CELL) {
			if (theEvent.param1 >= 0
				&& theEvent.param1 < mapToWindowX(0)
				&& theEvent.param2 >= 0
				&& theEvent.param2 < ROWS - 1
				&& rogue.sidebarLocationList[theEvent.param2][0] > -1) {
				
				// If the cursor is on an entity in the sidebar.
				cursor[0] = rogue.sidebarLocationList[theEvent.param2][0];
				cursor[1] = rogue.sidebarLocationList[theEvent.param2][1];
				sidebarHighlighted = true;
				cursorMovementCommand = true;
				refreshSideBar(cursor[0], cursor[1], false);
				if (theEvent.eventType == MOUSE_UP) {
					*targetConfirmed = true;
				}
			} else if (coordinatesAreInMap(windowToMapX(theEvent.param1), windowToMapY(theEvent.param2))
				|| targetCanLeaveMap && theEvent.eventType != MOUSE_UP) {
				
				// If the cursor is in the map area, or is allowed to leave the map and it isn't a click.
				if (theEvent.eventType == MOUSE_UP
					&& !theEvent.shiftKey
					&& (theEvent.controlKey || (cursor[0] == windowToMapX(theEvent.param1) && cursor[1] == windowToMapY(theEvent.param2)))) {
					
					*targetConfirmed = true;
				}
				cursor[0] = windowToMapX(theEvent.param1);
				cursor[1] = windowToMapY(theEvent.param2);
				cursorMovementCommand = true;
			} else {
				cursorMovementCommand = false;
				again = theEvent.eventType != MOUSE_UP;
			}
		} else if (theEvent.eventType == KEYSTROKE) {
			keystroke = theEvent.param1;
			moveIncrement = ( (theEvent.controlKey || theEvent.shiftKey) ? 5 : 1 );
			stripShiftFromMovementKeystroke(&keystroke);
			switch(keystroke) {
				case LEFT_ARROW:
				case LEFT_KEY:
				case NUMPAD_4:
					if (keysMoveCursor && cursor[0] > 0) {
						cursor[0] -= moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case RIGHT_ARROW:
				case RIGHT_KEY:
				case NUMPAD_6:
					if (keysMoveCursor && cursor[0] < DCOLS - 1) {
						cursor[0] += moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case UP_ARROW:
				case UP_KEY:
				case NUMPAD_8:
					if (keysMoveCursor && cursor[1] > 0) {
						cursor[1] -= moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case DOWN_ARROW:
				case DOWN_KEY:
				case NUMPAD_2:
					if (keysMoveCursor && cursor[1] < DROWS - 1) {
						cursor[1] += moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case UPLEFT_KEY:
				case NUMPAD_7:
					if (keysMoveCursor && cursor[0] > 0 && cursor[1] > 0) {
						cursor[0] -= moveIncrement;
						cursor[1] -= moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case UPRIGHT_KEY:
				case NUMPAD_9:
					if (keysMoveCursor && cursor[0] < DCOLS - 1 && cursor[1] > 0) {
						cursor[0] += moveIncrement;
						cursor[1] -= moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case DOWNLEFT_KEY:
				case NUMPAD_1:
					if (keysMoveCursor && cursor[0] > 0 && cursor[1] < DROWS - 1) {
						cursor[0] -= moveIncrement;
						cursor[1] += moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case DOWNRIGHT_KEY:
				case NUMPAD_3:
					if (keysMoveCursor && cursor[0] < DCOLS - 1 && cursor[1] < DROWS - 1) {
						cursor[0] += moveIncrement;
						cursor[1] += moveIncrement;
					}
					cursorMovementCommand = movementKeystroke = keysMoveCursor;
					break;
				case TAB_KEY:
				case NUMPAD_0:
					*tabKey = true;
					break;
				case RETURN_KEY:
				case ENTER_KEY:
					*targetConfirmed = true;
					break;
				case ESCAPE_KEY:
				case ACKNOWLEDGE_KEY:
					*canceled = true;
					break;
				default:
					break;
			}
		} else if (theEvent.eventType == RIGHT_MOUSE_UP) {
			// do nothing
		} else {
			again = true;
		}
		
		if (sidebarHighlighted
			&& (!(pmap[cursor[0]][cursor[1]].flags & (HAS_PLAYER | HAS_MONSTER))
								   || !canSeeMonster(monsterAtLoc(cursor[0], cursor[1])))
			&& (!(pmap[cursor[0]][cursor[1]].flags & HAS_ITEM) || !playerCanSeeOrSense(cursor[0], cursor[1]))
			&& (!cellHasTMFlag(cursor[0], cursor[1], TM_LIST_IN_SIDEBAR) || !playerCanSeeOrSense(cursor[0], cursor[1]))) {
			
			// The sidebar is highlighted but the cursor is not on a visible item, monster or terrain. Un-highlight the sidebar.
			refreshSideBar(-1, -1, false);
			sidebarHighlighted = false;
		}
		
		if (targetCanLeaveMap && !movementKeystroke) {
			// permit it to leave the map by up to 1 space in any direction if mouse controlled.
			cursor[0] = clamp(cursor[0], -1, DCOLS);
			cursor[1] = clamp(cursor[1], -1, DROWS);
		} else {
			cursor[0] = clamp(cursor[0], 0, DCOLS - 1);
			cursor[1] = clamp(cursor[1], 0, DROWS - 1);
		}
	} while (again && (!event || !cursorMovementCommand));
	
	if (event) {
		*event = theEvent;
	}
	
	if (sidebarHighlighted) {
		// Don't leave the sidebar highlighted when we exit.
		refreshSideBar(-1, -1, false);
		sidebarHighlighted = false;
	}
	
	targetLoc[0] = cursor[0];
	targetLoc[1] = cursor[1];
	
	return !cursorMovementCommand;
}

void pullMouseClickDuringPlayback(short loc[2]) {
	rogueEvent theEvent;
	
#ifdef BROGUE_ASSERTS
	assert(rogue.playbackMode);
#endif
	nextBrogueEvent(&theEvent, false, false, false);
	loc[0] = windowToMapX(theEvent.param1);
	loc[1] = windowToMapY(theEvent.param2);
}

// Return true if a target is chosen, or false if canceled.
boolean chooseTarget(short returnLoc[2],
					 short maxDistance,
					 boolean stopAtTarget,
					 boolean autoTarget,
					 boolean targetAllies,
					 boolean passThroughCreatures) {
	short originLoc[2], targetLoc[2], oldTargetLoc[2], coordinates[DCOLS][2], numCells, i, distance, newX, newY;
	creature *monst;
	boolean canceled, targetConfirmed, tabKey, cursorInTrajectory, focusedOnSomething = false;
	rogueEvent event;
		
	if (rogue.playbackMode) {
		// In playback, pull the next event (a mouseclick) and use that location as the target.
		pullMouseClickDuringPlayback(returnLoc);
		rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
		return true;
	}
	
	assureCosmeticRNG;
	
	originLoc[0] = player.xLoc;
	originLoc[1] = player.yLoc;
	
	targetLoc[0] = oldTargetLoc[0] = player.xLoc;
	targetLoc[1] = oldTargetLoc[1] = player.yLoc;
	
	if (autoTarget) {
		if (rogue.lastTarget
			&& canSeeMonster(rogue.lastTarget)
			&& (targetAllies == (rogue.lastTarget->creatureState == MONSTER_ALLY))
			&& rogue.lastTarget->depth == rogue.depthLevel
			&& !(rogue.lastTarget->bookkeepingFlags & MONST_IS_DYING)
			&& openPathBetween(player.xLoc, player.yLoc, rogue.lastTarget->xLoc, rogue.lastTarget->yLoc)) {
			
			monst = rogue.lastTarget;
		} else {
			//rogue.lastTarget = NULL;
			if (nextTargetAfter(&newX, &newY, targetLoc[0], targetLoc[1], !targetAllies, targetAllies, false, false, true)) {
                targetLoc[0] = newX;
                targetLoc[1] = newY;
            }
            monst = monsterAtLoc(targetLoc[0], targetLoc[1]);
		}
		if (monst) {
			targetLoc[0] = monst->xLoc;
			targetLoc[1] = monst->yLoc;
			refreshSideBar(monst->xLoc, monst->yLoc, false);
			focusedOnSomething = true;
		}
	}
	
	numCells = getLineCoordinates(coordinates, originLoc, targetLoc);
	if (maxDistance > 0) {
		numCells = min(numCells, maxDistance);
	}
	if (stopAtTarget) {
		numCells = min(numCells, distanceBetween(player.xLoc, player.yLoc, targetLoc[0], targetLoc[1]));
	}
	
	targetConfirmed = canceled = tabKey = false;
	
	do {
		printLocationDescription(targetLoc[0], targetLoc[1]);
		
		if (canceled) {
			refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);
			hiliteTrajectory(coordinates, numCells, true, passThroughCreatures);
			confirmMessages();
			rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
			restoreRNG;
			return false;
		}
		
		if (tabKey) {
			if (nextTargetAfter(&newX, &newY, targetLoc[0], targetLoc[1], !targetAllies, targetAllies, false, false, true)) {
                targetLoc[0] = newX;
                targetLoc[1] = newY;
            }
		}
		
		monst = monsterAtLoc(targetLoc[0], targetLoc[1]);
		if (monst != NULL && monst != &player && canSeeMonster(monst)) {
			focusedOnSomething = true;
        } else if (playerCanSeeOrSense(targetLoc[0], targetLoc[1])
                   && (pmap[targetLoc[0]][targetLoc[1]].flags & HAS_ITEM) || cellHasTMFlag(targetLoc[0], targetLoc[1], TM_LIST_IN_SIDEBAR)) {
            focusedOnSomething = true;
		} else if (focusedOnSomething) {
			refreshSideBar(-1, -1, false);
			focusedOnSomething = false;
		}
        if (focusedOnSomething) {
			refreshSideBar(targetLoc[0], targetLoc[1], false);
        }
		
		refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);
		hiliteTrajectory(coordinates, numCells, true, passThroughCreatures);
		
		if (!targetConfirmed) {
			numCells = getLineCoordinates(coordinates, originLoc, targetLoc);
			if (maxDistance > 0) {
				numCells = min(numCells, maxDistance);
			}
			
			if (stopAtTarget) {
				numCells = min(numCells, distanceBetween(player.xLoc, player.yLoc, targetLoc[0], targetLoc[1]));
			}
			distance = hiliteTrajectory(coordinates, numCells, false, passThroughCreatures);
			cursorInTrajectory = false;
			for (i=0; i<distance; i++) {
				if (coordinates[i][0] == targetLoc[0] && coordinates[i][1] == targetLoc[1]) {
					cursorInTrajectory = true;
					break;
				}
			}
			hiliteCell(targetLoc[0], targetLoc[1], &white, (cursorInTrajectory ? 100 : 35), true);	
		}
		
		oldTargetLoc[0] = targetLoc[0];
		oldTargetLoc[1] = targetLoc[1];
		moveCursor(&targetConfirmed, &canceled, &tabKey, targetLoc, &event, NULL, NULL, NULL, false, true, false);
		if (event.eventType == RIGHT_MOUSE_UP) { // Right mouse cancels.
			canceled = true;
		}
	} while (!targetConfirmed);
	if (maxDistance > 0) {
		numCells = min(numCells, maxDistance);
	}
	hiliteTrajectory(coordinates, numCells, true, passThroughCreatures);
	refreshDungeonCell(oldTargetLoc[0], oldTargetLoc[1]);
	
	if (originLoc[0] == targetLoc[0] && originLoc[1] == targetLoc[1]) {
		confirmMessages();
		restoreRNG;
		rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
		return false;
	}
	
	monst = monsterAtLoc(targetLoc[0], targetLoc[1]);
	if (monst && monst != &player && canSeeMonster(monst)) {
		rogue.lastTarget = monst;
	}
	
	returnLoc[0] = targetLoc[0];
	returnLoc[1] = targetLoc[1];
	restoreRNG;
	rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
	return true;
}

void identifyItemKind(item *theItem) {
    itemTable *theTable;
	short tableCount, i, lastItem;
    
    theTable = tableForItemCategory(theItem->category);
    if (theTable) {
		theItem->flags &= ~ITEM_KIND_AUTO_ID;
        
        tableCount = 0;
        lastItem = -1;
        
        switch (theItem->category) {
            case SCROLL:
                tableCount = NUMBER_SCROLL_KINDS;
                break;
            case POTION:
                tableCount = NUMBER_POTION_KINDS;
                break;
            case WAND:
                tableCount = NUMBER_WAND_KINDS;
                break;
            case STAFF:
                tableCount = NUMBER_STAFF_KINDS;
                break;
            case RING:
                tableCount = NUMBER_RING_KINDS;
                break;
            default:
                break;
        }
        if ((theItem->category & RING)
            && theItem->enchant1 <= 0) {
            
            theItem->flags |= ITEM_IDENTIFIED;
        }
        if (tableCount) {
            theTable[theItem->kind].identified = true;
            for (i=0; i<tableCount; i++) {
                if (!(theTable[i].identified)) {
                    if (lastItem != -1) {
                        return; // at least two unidentified items remain
                    }
                    lastItem = i;
                }
            }
            if (lastItem != -1) {
                // exactly one unidentified item remains
                theTable[lastItem].identified = true;
            }
        }
    }
}

void autoIdentify(item *theItem) {
	short quantityBackup;
	char buf[COLS * 3], oldName[COLS * 3], newName[COLS * 3];
	
    if (tableForItemCategory(theItem->category)
        && !tableForItemCategory(theItem->category)[theItem->kind].identified) {
        
        identifyItemKind(theItem);
        quantityBackup = theItem->quantity;
        theItem->quantity = 1;
        itemName(theItem, newName, false, true, NULL);
        theItem->quantity = quantityBackup;
        sprintf(buf, "（你发现这是%s。）",
                newName);
        messageWithColor(buf, &itemMessageColor, false);
    }
    
    if ((theItem->category & (WEAPON | ARMOR))
        && (theItem->flags & ITEM_RUNIC)
        && !(theItem->flags & ITEM_RUNIC_IDENTIFIED)) {
        
        itemName(theItem, oldName, false, false, NULL);
        theItem->flags |= ITEM_RUNIC_IDENTIFIED;
        itemName(theItem, newName, true, true, NULL);
        sprintf(buf, "（你发现这件%s其实是%s。）", oldName, newName);
        messageWithColor(buf, &itemMessageColor, false);
    }
}

// returns whether the item disappeared
boolean hitMonsterWithProjectileWeapon(creature *thrower, creature *monst, item *theItem) {
	char buf[DCOLS], theItemName[DCOLS], targetName[DCOLS], armorRunicString[DCOLS];
	boolean thrownWeaponHit;
	item *equippedWeapon;
	short damage;
	
	if (!(theItem->category & WEAPON)) {
		return false;
	}
	
	armorRunicString[0] = '\0';
	
	itemName(theItem, theItemName, false, false, NULL);
	monsterName(targetName, monst, true);
	
	monst->status[STATUS_ENTRANCED] = 0;
	
	if (monst != &player
		&& monst->creatureMode != MODE_PERM_FLEEING
		&& (monst->creatureState != MONSTER_FLEEING || monst->status[STATUS_MAGICAL_FEAR])
		&& !(monst->bookkeepingFlags & MONST_CAPTIVE)
        && monst->creatureState != MONSTER_ALLY) {
        
		monst->creatureState = MONSTER_TRACKING_SCENT;
		if (monst->status[STATUS_MAGICAL_FEAR]) {
			monst->status[STATUS_MAGICAL_FEAR] = 1;
		}
	}
	
	if (thrower == &player) {
		equippedWeapon = rogue.weapon;
		equipItem(theItem, true);
		thrownWeaponHit = attackHit(&player, monst);
		if (equippedWeapon) {
			equipItem(equippedWeapon, true);
		} else {
			unequipItem(theItem, true);
		}
	} else {
		thrownWeaponHit = attackHit(thrower, monst);
	}
	
	if (thrownWeaponHit) {
		damage = (monst->info.flags & MONST_IMMUNE_TO_WEAPONS ? 0 :
				  randClump(theItem->damage)) * pow(WEAPON_ENCHANT_DAMAGE_FACTOR, netEnchant(theItem));
		
		if (monst == &player) {
			applyArmorRunicEffect(armorRunicString, thrower, &damage, false);
		}
		
		if (inflictDamage(monst, damage, &red)) { // monster killed
			sprintf(buf, "%s%s%s。",
                    theItemName,
                    (monst->info.flags & MONST_INANIMATE) ? "摧毁了" : "杀死了",
                    targetName);
			messageWithColor(buf, messageColorFromVictim(monst), false);
		} else {
			sprintf(buf, "%s击中了%s。", theItemName, targetName);
			if (theItem->flags & ITEM_RUNIC) {
				magicWeaponHit(monst, theItem, false);
			}
			messageWithColor(buf, messageColorFromVictim(monst), false);
			
			moralAttack(thrower, monst);
		}
		if (armorRunicString[0]) {
			message(armorRunicString, false);
		}
		return true;
	} else {
		theItem->flags &= ~ITEM_PLAYER_AVOIDS; // Don't avoid thrown weapons that missed.
		sprintf(buf, "%s没有击中%s。", theItemName, targetName);
		message(buf, false);
		return false;
	}
}

void throwItem(item *theItem, creature *thrower, short targetLoc[2], short maxDistance) {
	short listOfCoordinates[MAX_BOLT_LENGTH][2], originLoc[2];
	short i, x, y, numCells;
	creature *monst = NULL;
	char buf[COLS*3], buf2[COLS*3], buf3[COLS*3];
	short dropLoc[2];
	boolean hitSomethingSolid = false, fastForward = false;
    enum dungeonLayers layer;
	
	theItem->flags |= ITEM_PLAYER_AVOIDS; // Avoid thrown items, unless it's a weapon that misses a monster.
	
	x = originLoc[0] = thrower->xLoc;
	y = originLoc[1] = thrower->yLoc;
	
	numCells = getLineCoordinates(listOfCoordinates, originLoc, targetLoc);
	
	thrower->ticksUntilTurn = thrower->attackSpeed;
	
	if (thrower != &player && pmap[originLoc[0]][originLoc[1]].flags & IN_FIELD_OF_VIEW) {
		monsterName(buf2, thrower, true);
		itemName(theItem, buf3, false, true, NULL);
		sprintf(buf, "%s投出了%s。", buf2, buf3);
		message(buf, false);
	}
	
	for (i=0; i<numCells && i < maxDistance; i++) {
		
		x = listOfCoordinates[i][0];
		y = listOfCoordinates[i][1];
		
		if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
			monst = monsterAtLoc(x, y);
			
//			if (projectileReflects(thrower, monst) && i < DCOLS*2) {
//				if (projectileReflects(thrower, monst)) { // if it scores another reflection roll, reflect at caster
//					numCells = reflectBolt(originLoc[0], originLoc[1], listOfCoordinates, i, true);
//				} else {
//					numCells = reflectBolt(-1, -1, listOfCoordinates, i, false); // otherwise reflect randomly
//				}
//				
//				monsterName(buf2, monst, true);
//				itemName(theItem, buf3, false, false, NULL);
//				sprintf(buf, "%s deflect%s the %s", buf2, (monst == &player ? "" : "s"), buf3);
//				combatMessage(buf, 0);
//				continue;
//			}
			
			if ((theItem->category & WEAPON)
				&& theItem->kind != INCENDIARY_DART
				&& hitMonsterWithProjectileWeapon(thrower, monst, theItem)) {
				return;
			}
			
			break;
		}
		
		// We hit something!
		if (cellHasTerrainFlag(x, y, (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_VISION))) {
			if ((theItem->category & WEAPON)
				&& (theItem->kind == INCENDIARY_DART)
				&& (cellHasTerrainFlag(x, y, T_IS_FLAMMABLE) || (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)))) {
				// Incendiary darts thrown at flammable obstructions (foliage, wooden barricades, doors) will hit the obstruction
				// instead of bursting a cell earlier.
            } else if (cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY)
                       && cellHasTMFlag(x, y, TM_PROMOTES_ON_PLAYER_ENTRY)
                       && tileCatalog[pmap[x][y].layers[layerWithTMFlag(x, y, TM_PROMOTES_ON_PLAYER_ENTRY)]].flags & T_OBSTRUCTS_PASSABILITY) {
                layer = layerWithTMFlag(x, y, TM_PROMOTES_ON_PLAYER_ENTRY);
                if (tileCatalog[pmap[x][y].layers[layer]].flags & T_OBSTRUCTS_PASSABILITY) {
                    message(tileCatalog[pmap[x][y].layers[layer]].flavorText, false);
                    promoteTile(x, y, layer, false);
                }
			} else {
				i--;
				if (i >= 0) {
					x = listOfCoordinates[i][0];
					y = listOfCoordinates[i][1];
				} else { // it was aimed point-blank into an obstruction
					x = thrower->xLoc;
					y = thrower->yLoc;
				}
			}
			hitSomethingSolid = true;
			break;
		}
		
		if (playerCanSee(x, y)) { // show the graphic
			plotItemChar(theItem->displayChar, theItem->foreColor, x, y);
			
			if (!fastForward) {
				fastForward = rogue.playbackFastForward || pauseBrogue(25);
			}
			
			refreshDungeonCell(x, y);
		}
		
		if (x == targetLoc[0] && y == targetLoc[1]) { // reached its target
			break;
		}
	}	
	
	if ((theItem->category & POTION) && (hitSomethingSolid || !cellHasTerrainFlag(x, y, T_AUTO_DESCENT))) {
		if (theItem->kind == POTION_CONFUSION || theItem->kind == POTION_POISON
			|| theItem->kind == POTION_PARALYSIS || theItem->kind == POTION_INCINERATION
			|| theItem->kind == POTION_DARKNESS || theItem->kind == POTION_LICHEN
			|| theItem->kind == POTION_DESCENT) {
			switch (theItem->kind) {
				case POTION_POISON:
					strcpy(buf, "瓶子碎开了，一阵紫色的毒气飘散开来！");
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_POISON_GAS_CLOUD_POTION], true, false);
					message(buf, false);
					break;
				case POTION_CONFUSION:
					strcpy(buf, "瓶子碎开了，一阵颜色奇怪的烟雾飘散开来！");
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_CONFUSION_GAS_CLOUD_POTION], true, false);
					message(buf, false);
					break;
				case POTION_PARALYSIS:
					strcpy(buf, "瓶子碎开了，一阵粉红色的烟雾飘散开来！");
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_PARALYSIS_GAS_CLOUD_POTION], true, false);
					message(buf, false);
					break;
				case POTION_INCINERATION:
					strcpy(buf, "瓶子碎开了，产生了一团火焰！");
					message(buf, false);
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_INCINERATION_POTION], true, false);
					break;
				case POTION_DARKNESS:
					strcpy(buf, "瓶子碎开了，附近的光开始变得暗淡下来！");
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_DARKNESS_POTION], true, false);
					message(buf, false);
					break;
				case POTION_DESCENT:
					strcpy(buf, "瓶子碎开了，附近的地面正开始消失！");
					message(buf, false);
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_HOLE_POTION], true, false);
					break;
				case POTION_LICHEN:
					strcpy(buf, "瓶子碎开了，致命的孢子在地面上扩散开来！");
					message(buf, false);
					spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_LICHEN_PLANTED], true, false);
					break;
			}
			
			autoIdentify(theItem);
			
			refreshDungeonCell(x, y);
			
			//if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
			//	monst = monsterAtLoc(x, y);
			//	applyInstantTileEffectsToCreature(monst);
			//}
		} else {
			// if (cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY)) {
			// 	strcpy(buf2, "against");
			// } else if (tileCatalog[pmap[x][y].layers[highestPriorityLayer(x, y, false)]].mechFlags & TM_STAND_IN_TILE) {
			// 	strcpy(buf2, "into");
			// } else {
			// 	strcpy(buf2, "on");
			// }
			sprintf(buf, "瓶子碎开了，里面%s的液体溅了开来。",
					potionTable[theItem->kind].flavor);
			message(buf, false);
			if (theItem->kind == POTION_HALLUCINATION && (theItem->flags & ITEM_MAGIC_DETECTED)) {
				autoIdentify(theItem);
			}
		}
		return; // potions disappear when they break
	}
	if ((theItem->category & WEAPON) && theItem->kind == INCENDIARY_DART) {
		spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DF_DART_EXPLOSION], true, false);
		if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
			exposeCreatureToFire(monsterAtLoc(x, y));
		}
		return;
	}
	getQualifyingLocNear(dropLoc, x, y, true, 0, (T_OBSTRUCTS_ITEMS | T_OBSTRUCTS_PASSABILITY), (HAS_ITEM), false, false);
	placeItem(theItem, dropLoc[0], dropLoc[1]);
	refreshDungeonCell(dropLoc[0], dropLoc[1]);
}

void throwCommand(item *theItem) {
	item *thrownItem;
	char buf[COLS*3], theName[COLS*3];
	unsigned char command[10];
	short maxDistance, zapTarget[2], quantity;
	boolean autoTarget;
	
	command[0] = THROW_KEY;
	if (theItem == NULL) {
		theItem = promptForItemOfType((ALL_ITEMS), 0, 0, "选择投掷的物品：（a-z, 按住<shift>键弹出物品菜单。<esc>键取消）", true);
	}
	if (theItem == NULL) {
		return;
	}
	
	quantity = theItem->quantity;
	theItem->quantity = 1;
	itemName(theItem, theName, false, false, NULL);
	theItem->quantity = quantity;
	
	command[1] = theItem->inventoryLetter;
	confirmMessages();
	
	if ((theItem->flags & ITEM_EQUIPPED) && theItem->quantity <= 1) {
		sprintf(buf, "确定要投掷这件%s吗？", theName);
		if (!confirm(buf, false)) {
			return;
		}
        if (theItem->flags & ITEM_CURSED) {
            sprintf(buf, "你无法取下这件%s；它似乎是被诅咒的。", theName);
            messageWithColor(buf, &itemMessageColor, false);
            return;
        }
	}
	
	sprintf(buf, "把%s投掷到哪里？（<hjklyubn>键或者鼠标选择位置, 按<tab>键切换敌人目标）",
			theName);
	temporaryMessage(buf, false);
	maxDistance = (12 + 2 * max(rogue.strength - player.weaknessAmount - 12, 2));
	autoTarget = (theItem->category & (WEAPON | POTION)) ? true : false;
	if (chooseTarget(zapTarget, maxDistance, true, autoTarget, false, false)) {
        if ((theItem->flags & ITEM_EQUIPPED) && theItem->quantity <= 1) {
            unequipItem(theItem, false);
        }
		command[2] = '\0';
		recordKeystrokeSequence(command);
		recordMouseClick(mapToWindowX(zapTarget[0]), mapToWindowY(zapTarget[1]), true, false);
		
		confirmMessages();
		
		thrownItem = generateItem(ALL_ITEMS, -1);
		*thrownItem = *theItem; // clone the item
		thrownItem->flags &= ~ITEM_EQUIPPED;
		thrownItem->quantity = 1;
		
		itemName(thrownItem, theName, false, false, NULL);
		
		throwItem(thrownItem, &player, zapTarget, maxDistance);
	} else {
		return;
	}
	
	// Now decrement or delete the thrown item out of the inventory.
	if (theItem->quantity > 1) {
		theItem->quantity--;
	} else {
		removeItemFromChain(theItem, packItems);
		deleteItem(theItem);
	}
	playerTurnEnded();
}

boolean useStaffOrWand(item *theItem, boolean *commandsRecorded) {
	char buf[COLS*3], buf2[COLS*3];
	unsigned char command[10];
	short zapTarget[2], originLoc[2], maxDistance, c;
	boolean autoTarget, targetAllies, autoID, passThroughCreatures;
    
	c = 0;
	command[c++] = APPLY_KEY;
	command[c++] = theItem->inventoryLetter;
    
    if (theItem->charges <= 0 && (theItem->flags & ITEM_IDENTIFIED)) {
        itemName(theItem, buf2, false, false, NULL);
        sprintf(buf, "%s没有能量了。", buf2);
        messageWithColor(buf, &itemMessageColor, false);
        return false;
    }
    temporaryMessage("选择施法方向：（<hjklyubn>键或者鼠标选择位置, 按<tab>键切换敌人目标，回车键确认）", false);
    itemName(theItem, buf2, false, false, NULL);

    if ((theItem->category & STAFF) && theItem->kind == STAFF_BLINKING
        && theItem->flags & (ITEM_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN)) {
        maxDistance = staffBlinkDistance(theItem->enchant1);
    } else {
        maxDistance = -1;
    }
    autoTarget = true;
    if (theItem->category & STAFF && staffTable[theItem->kind].identified &&
        (theItem->kind == STAFF_BLINKING
         || theItem->kind == STAFF_TUNNELING)) {
            autoTarget = false;
        }
    targetAllies = false;
    if (((theItem->category & STAFF) && staffTable[theItem->kind].identified &&
         (theItem->kind == STAFF_HEALING || theItem->kind == STAFF_HASTE || theItem->kind == STAFF_PROTECTION))
        || ((theItem->category & WAND) && wandTable[theItem->kind].identified &&
            (theItem->kind == WAND_INVISIBILITY || theItem->kind == WAND_PLENTY))) {
            targetAllies = true;
        }
    passThroughCreatures = false;
    if ((theItem->category & STAFF) && staffTable[theItem->kind].identified &&
        theItem->kind == STAFF_LIGHTNING) {
        passThroughCreatures = true;
    }
    if (chooseTarget(zapTarget, maxDistance, false, autoTarget, targetAllies, passThroughCreatures)) {
        command[c] = '\0';
        if (!(*commandsRecorded)) {
            recordKeystrokeSequence(command);
            recordMouseClick(mapToWindowX(zapTarget[0]), mapToWindowY(zapTarget[1]), true, false);
            *commandsRecorded = true;
        }
        confirmMessages();
        
        originLoc[0] = player.xLoc;
        originLoc[1] = player.yLoc;
        
        if (theItem->charges > 0) {
            autoID = (zap(originLoc, zapTarget,
                          (theItem->kind + (theItem->category == STAFF ? NUMBER_WAND_KINDS : 0)),		// bolt type
                          (theItem->category == STAFF ? theItem->enchant1 : 10),						// bolt level
                          !(((theItem->category & WAND) && wandTable[theItem->kind].identified)
                            || ((theItem->category & STAFF) && staffTable[theItem->kind].identified))));	// hide bolt details
            if (autoID) {
                if (!tableForItemCategory(theItem->category)[theItem->kind].identified) {
                    identifyItemKind(theItem);
                }
            }
        } else {
            itemName(theItem, buf2, false, false, NULL);
            if (theItem->category == STAFF) {
                sprintf(buf, "%s发出了嘶嘶的声音；看起来它现在没有能量了，需要过一段时间才能使用。", buf2);
            } else {
                sprintf(buf, "%s发出了嘶嘶的声音；看起来里面的能量已经用完了。", buf2);
            }
            messageWithColor(buf, &itemMessageColor, false);
            theItem->flags |= ITEM_MAX_CHARGES_KNOWN;
            playerTurnEnded();
            return false;
        }
    } else {
        return false;
    }
    return true;
}

void useCharm(item *theItem) {
    switch (theItem->kind) {
        case CHARM_HEALTH:
            heal(&player, charmHealing(theItem->enchant1));
            message("你感觉自己更健康了。", false);
            break;
        case CHARM_PROTECTION:
            if (charmProtection(theItem->enchant1) > player.status[STATUS_SHIELDED]) {
                player.status[STATUS_SHIELDED] = charmProtection(theItem->enchant1);
            }
            player.maxStatus[STATUS_SHIELDED] = player.status[STATUS_SHIELDED];
            flashMonster(&player, boltColors[BOLT_SHIELDING], 100);
            message("一个发亮的魔法护盾笼罩了你。", false);
            break;
        case CHARM_HASTE:
            haste(&player, charmEffectDuration(theItem->kind, theItem->enchant1));
            break;
        case CHARM_FIRE_IMMUNITY:
            player.status[STATUS_IMMUNE_TO_FIRE] = player.maxStatus[STATUS_IMMUNE_TO_FIRE] = charmEffectDuration(theItem->kind, theItem->enchant1);
            if (player.status[STATUS_BURNING]) {
                extinguishFireOnCreature(&player);
            }
            message("你不再惧怕火焰。", false);
            break;
        case CHARM_INVISIBILITY:
            imbueInvisibility(&player, charmEffectDuration(theItem->kind, theItem->enchant1), false);
            message("你打了个哆嗦，发现自己都看不见自己的手了。", false);
            break;
        case CHARM_TELEPATHY:
            makePlayerTelepathic(charmEffectDuration(theItem->kind, theItem->enchant1));
            break;
        case CHARM_LEVITATION:
            player.status[STATUS_LEVITATING] = player.maxStatus[STATUS_LEVITATING] = charmEffectDuration(theItem->kind, theItem->enchant1);
            player.bookkeepingFlags &= ~MONST_SEIZED; // break free of holding monsters
            message("你突然飘起来，悬浮在了半空中！", false);
            break;
        case CHARM_SHATTERING:
            messageWithColor("这件法器突然发出一阵蓝色的光，击穿了附近的墙壁！", &itemMessageColor, false);
            crystalize(charmShattering(theItem->enchant1));
            break;
//        case CHARM_CAUSE_FEAR:
//            causeFear("your charm");
//            break;
        case CHARM_TELEPORTATION:
            teleport(&player, -1, -1, true);
            break;
        case CHARM_RECHARGING:
            rechargeItems(STAFF);
            break;
        case CHARM_NEGATION:
            negationBlast("这件法器");
            break;
        default:
            break;
    }
}

void apply(item *theItem, boolean recordCommands) {
	char buf[COLS*3], buf2[COLS*3];
	boolean commandsRecorded, revealItemType;
	unsigned char command[10];
	short c;
	
	commandsRecorded = !recordCommands;
	c = 0;
	command[c++] = APPLY_KEY;
	
	revealItemType = false;
	
	if (!theItem) {
		theItem = promptForItemOfType((SCROLL|FOOD|POTION|STAFF|WAND|CHARM), 0, 0,
									  "选择要使用的物品：（a-z, 按住<shift>键弹出物品菜单。<esc>键取消）", true);
	}
	
	if (theItem == NULL) {
		return;
	}
    
    if ((theItem->category == SCROLL || theItem->category == POTION)
        && magicCharDiscoverySuffix(theItem->category, theItem->kind) == -1
        && ((theItem->flags & ITEM_MAGIC_DETECTED) || tableForItemCategory(theItem->category)[theItem->kind].identified)) {
        
        if (tableForItemCategory(theItem->category)[theItem->kind].identified) {
            sprintf(buf,
                    "确认要%s%s%s吗？",
                    theItem->category == SCROLL ? "使用这件" : "喝下这瓶",
                    tableForItemCategory(theItem->category)[theItem->kind].name,
                    theItem->category == SCROLL ? "卷轴" : "药剂");
        } else {
            sprintf(buf,
                    "确认要%s被诅咒的%s吗？",
                    theItem->category == SCROLL ? "使用这件" : "喝下这瓶",
                    theItem->category == SCROLL ? "卷轴" : "药剂");
        }
        if (!confirm(buf, false)) {
            return;
        }
    }
    
	command[c++] = theItem->inventoryLetter;
	confirmMessages();
	switch (theItem->category) {
		case FOOD:
			if (STOMACH_SIZE - player.status[STATUS_NUTRITION] < foodTable[theItem->kind].strengthRequired) { // Not hungry enough.
				sprintf(buf, "你还没有那么饿，现在吃掉%s有些浪费了。还是要吃掉它么？",
						(theItem->kind == RATION ? "这份粮食" : "这个芒果"));
				if (!confirm(buf, false)) {
					return;
				}
			}
			player.status[STATUS_NUTRITION] = min(foodTable[theItem->kind].strengthRequired + player.status[STATUS_NUTRITION], STOMACH_SIZE);
			if (theItem->kind == RATION) {
				messageWithColor("嗯味道很不错。", &itemMessageColor, false);
			} else {
				messageWithColor("这个芒果相当美味！", &itemMessageColor, false);
			}
			break;
		case POTION:
			command[c] = '\0';
			recordKeystrokeSequence(command);
			commandsRecorded = true;
			if (!potionTable[theItem->kind].identified) {
				revealItemType = true;
			}
			drinkPotion(theItem);
			break;
		case SCROLL:
			command[c] = '\0';
			recordKeystrokeSequence(command);
			commandsRecorded = true; // have to record in case further keystrokes are necessary (e.g. enchant scroll)
			if (!scrollTable[theItem->kind].identified
				&& theItem->kind != SCROLL_ENCHANTING
				&& theItem->kind != SCROLL_IDENTIFY) {
				
				revealItemType = true;
			}
			readScroll(theItem);
			break;
		case STAFF:
		case WAND:
            if (!useStaffOrWand(theItem, &commandsRecorded)) {
                return;
            }
			break;
        case CHARM:
			if (theItem->charges > 0) {
				itemName(theItem, buf2, false, false, NULL);
				sprintf(buf, "你的%s还没有完成充能。", buf2);
				messageWithColor(buf, &itemMessageColor, false);
				return;
			}
			command[c] = '\0';
			recordKeystrokeSequence(command);
			commandsRecorded = true;
            useCharm(theItem);
            break;
		default:
			itemName(theItem, buf2, false, true, NULL);
			sprintf(buf, "你没法使用%s。", buf2);
			message(buf, false);
			return;
	}
	
	if (!commandsRecorded) { // to make sure we didn't already record the keystrokes above with staff/wand targeting
		command[c] = '\0';
		recordKeystrokeSequence(command);
	}
	
	// Reveal the item type if appropriate.
	if (revealItemType) {
		autoIdentify(theItem);
	}
	
	if (theItem->category & CHARM) {
        theItem->charges = charmRechargeDelay(theItem->kind, theItem->enchant1);
    } else if (theItem->charges > 0) {
		theItem->charges--;
		if (theItem->category == WAND) {
			theItem->enchant2++; // keeps track of how many times the wand has been discharged for the player's convenience
		}
	} else if (theItem->quantity > 1) {
		theItem->quantity--;
	} else {
		removeItemFromChain(theItem, packItems);
		deleteItem(theItem);
	}
	playerTurnEnded();
}

void identify(item *theItem) {
	
	theItem->flags |= ITEM_IDENTIFIED;
	theItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
	if (theItem->flags & ITEM_RUNIC) {
		theItem->flags |= (ITEM_RUNIC_IDENTIFIED | ITEM_RUNIC_HINTED);
	}
    
    if (theItem->category & RING) {
        updateRingBonuses();
    }
    
	identifyItemKind(theItem);
}

enum monsterTypes chooseVorpalEnemy() {
	short i, index, possCount = 0, deepestLevel = 0, deepestHorde = 0, chosenHorde = 0, failsafe = 25;
	enum monsterTypes candidate;
    
    for (i=0; i<NUMBER_HORDES; i++) {
        if (hordeCatalog[i].minLevel >= rogue.depthLevel && !hordeCatalog[i].flags) {
            possCount += hordeCatalog[i].frequency;
        }
        if (hordeCatalog[i].minLevel > deepestLevel) {
            deepestHorde = i;
            deepestLevel = hordeCatalog[i].minLevel;
        }
    }
	
	do {
		if (possCount == 0) {
			chosenHorde = deepestHorde;
		} else {
			index = rand_range(1, possCount);
			for (i=0; i<NUMBER_HORDES; i++) {
				if (hordeCatalog[i].minLevel >= rogue.depthLevel && !hordeCatalog[i].flags) {
					if (index <= hordeCatalog[i].frequency) {
						chosenHorde = i;
						break;
					}
					index -= hordeCatalog[i].frequency;
				}
			}
		}
		
		index = rand_range(-1, hordeCatalog[chosenHorde].numberOfMemberTypes - 1);
		if (index == -1) {
			candidate = hordeCatalog[chosenHorde].leaderType;
		} else {
			candidate = hordeCatalog[chosenHorde].memberType[index];
		}
	} while (((monsterCatalog[candidate].flags & MONST_NEVER_VORPAL_ENEMY)
              || (monsterCatalog[candidate].abilityFlags & MA_NEVER_VORPAL_ENEMY))
             && --failsafe > 0);
	return candidate;
}

void updateIdentifiableItem(item *theItem) {
	if ((theItem->category & SCROLL) && scrollTable[theItem->kind].identified) {
        theItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
    } else if ((theItem->category & POTION) && potionTable[theItem->kind].identified) {
		theItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
	} else if ((theItem->category & (RING | STAFF | WAND))
			   && (theItem->flags & ITEM_IDENTIFIED)
			   && tableForItemCategory(theItem->category)[theItem->kind].identified) {
		
		theItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
	} else if ((theItem->category & (WEAPON | ARMOR))
			   && (theItem->flags & ITEM_IDENTIFIED)
			   && (!(theItem->flags & ITEM_RUNIC) || (theItem->flags & ITEM_RUNIC_IDENTIFIED))) {
		
		theItem->flags &= ~ITEM_CAN_BE_IDENTIFIED;
	}
}

void updateIdentifiableItems() {
	item *theItem;
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		updateIdentifiableItem(theItem);
	}
	for (theItem = floorItems; theItem != NULL; theItem = theItem->nextItem) {
		updateIdentifiableItem(theItem);
	}
}

void readScroll(item *theItem) {
	short i, j, x, y, numberOfMonsters = 0;
	item *tempItem;
	creature *monst;
	boolean hadEffect = false;
	char buf[2*COLS*3], buf2[COLS*3];
	
	switch (theItem->kind) {
		case SCROLL_IDENTIFY:
			identify(theItem);
			updateIdentifiableItems();
			messageWithColor("这是一张鉴定卷轴。", &itemMessageColor, true);
			if (numberOfMatchingPackItems(ALL_ITEMS, ITEM_CAN_BE_IDENTIFIED, 0, false) == 0) {
				message("你身上的所有东西都已经被鉴定过了。", false);
				break;
			}
			do {
				theItem = promptForItemOfType((ALL_ITEMS), ITEM_CAN_BE_IDENTIFIED, 0,
											  "选择要鉴定的物品：（a-z, 按住<shift>键弹出物品菜单。<esc>键取消）", false);
				if (rogue.gameHasEnded) {
					return;
				}
				if (theItem && !(theItem->flags & ITEM_CAN_BE_IDENTIFIED)) {
					confirmMessages();
					itemName(theItem, buf2, true, true, NULL);
					sprintf(buf, "你早已知道%s%s。", (theItem->quantity > 1 ? "它们是" : "这是"), buf2);
					messageWithColor(buf, &itemMessageColor, false);
				}
			} while (theItem == NULL || !(theItem->flags & ITEM_CAN_BE_IDENTIFIED));
			recordKeystroke(theItem->inventoryLetter, false, false);
			confirmMessages();
			identify(theItem);
			itemName(theItem, buf, true, true, NULL);
			sprintf(buf2, "%s%s.", (theItem->quantity == 1 ? "这是一件" : "这些是"), buf);
			messageWithColor(buf2, &itemMessageColor, false);
			break;
		case SCROLL_TELEPORT:
			teleport(&player, -1, -1, true);
			break;
		case SCROLL_REMOVE_CURSE:
			for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
				if (tempItem->flags & ITEM_CURSED) {
					hadEffect = true;
					tempItem->flags &= ~ITEM_CURSED;
				}
			}
			if (hadEffect) {
				message("你的背包里闪出一阵光，你能感觉到险恶的能量被驱散了开来。你当前身上的物品都不再是被诅咒的了。", false);
			} else {
				message("你的背包里闪出一阵光，但什么都没有发生。", false);
			}
			break;
		case SCROLL_ENCHANTING:
			identify(theItem);
			messageWithColor("这是一件增强卷轴。", &itemMessageColor, true);
			if (!numberOfMatchingPackItems((WEAPON | ARMOR | RING | STAFF | WAND | CHARM), 0, 0, false)) {
				confirmMessages();
				message("目前身上没有可以增强的物品。", false);
				break;
			}
			do {
				theItem = promptForItemOfType((WEAPON | ARMOR | RING | STAFF | WAND | CHARM), 0, 0,
											  "选择要增强的物品：（a-z, 按住<shift>键弹出物品菜单。<esc>键取消）", false);
				confirmMessages();
				if (theItem == NULL || !(theItem->category & (WEAPON | ARMOR | RING | STAFF | WAND | CHARM))) {
					message("没有办法对这件物品进行增强。", true);
				}
				if (rogue.gameHasEnded) {
					return;
				}
			} while (theItem == NULL || !(theItem->category & (WEAPON | ARMOR | RING | STAFF | WAND | CHARM)));
			recordKeystroke(theItem->inventoryLetter, false, false);
			confirmMessages();
			switch (theItem->category) {
				case WEAPON:
					theItem->strengthRequired = max(0, theItem->strengthRequired - 1);
					theItem->enchant1++;
                    if (theItem->quiverNumber) {
                        theItem->quiverNumber = rand_range(1, 60000);
                    }
					break;
				case ARMOR:
					theItem->strengthRequired = max(0, theItem->strengthRequired - 1);
					theItem->enchant1++;
					break;
				case RING:
					theItem->enchant1++;
                    theItem->enchant2++;
					updateRingBonuses();
					if (theItem->kind == RING_CLAIRVOYANCE) {
						updateClairvoyance();
						displayLevel();
					}
					break;
				case STAFF:
					theItem->enchant1++;
					theItem->charges++;
					theItem->enchant2 = 500 / theItem->enchant1;
					break;
				case WAND:
					//theItem->charges++;
                    theItem->charges += wandTable[theItem->kind].range.lowerBound;
					break;
				case CHARM:
                    theItem->enchant1++;
					
                    theItem->charges = min(0, theItem->charges); // Enchanting instantly recharges charms.
                    
//                    theItem->charges = theItem->charges
//                    * charmRechargeDelay(theItem->kind, theItem->enchant1)
//                    / charmRechargeDelay(theItem->kind, theItem->enchant1 - 1);
                    
					break;
				default:
					break;
			}
			if (theItem->flags & ITEM_EQUIPPED) {
				equipItem(theItem, true);
			}
			itemName(theItem, buf, false, false, NULL);
			sprintf(buf2, "你的%s发出一阵金色的闪光。", buf);
			messageWithColor(buf2, &itemMessageColor, false);
			if (theItem->flags & ITEM_CURSED) {
				sprintf(buf2, "一阵险恶的能量离开了这件%s。它不再是被诅咒的了。", buf);
				messageWithColor(buf2, &itemMessageColor, false);
				theItem->flags &= ~ITEM_CURSED;
			}
            createFlare(player.xLoc, player.yLoc, SCROLL_ENCHANTMENT_LIGHT);
			break;
		case SCROLL_RECHARGING:
            rechargeItems(STAFF | WAND | CHARM);
			break;
		case SCROLL_PROTECT_ARMOR:
			if (rogue.armor) {
				tempItem = rogue.armor;
				tempItem->flags |= ITEM_PROTECTED;
				itemName(tempItem, buf2, false, false, NULL);
				sprintf(buf, "一道金色的光亮覆盖了你的%s。", buf2);
				messageWithColor(buf, &itemMessageColor, false);
				if (tempItem->flags & ITEM_CURSED) {
					sprintf(buf, "一阵险恶的能流离开了这件%s。它不再是被诅咒的了。", buf2);
					messageWithColor(buf, &itemMessageColor, false);
					tempItem->flags &= ~ITEM_CURSED;
				}
			} else {
				message("一阵金色的光照亮了你的身体，但很快就消散了。", false);
			}
            createFlare(player.xLoc, player.yLoc, SCROLL_PROTECTION_LIGHT);
			break;
		case SCROLL_PROTECT_WEAPON:
			if (rogue.weapon) {
				tempItem = rogue.weapon;
				tempItem->flags |= ITEM_PROTECTED;
				itemName(tempItem, buf2, false, false, NULL);
				sprintf(buf, "一道金色的光亮覆盖了你的%s.", buf2);
				messageWithColor(buf, &itemMessageColor, false);
				if (tempItem->flags & ITEM_CURSED) {
					sprintf(buf, "一阵险恶的能流离开了这件%s。它不再是被诅咒的了。", buf2);
					messageWithColor(buf, &itemMessageColor, false);
					tempItem->flags &= ~ITEM_CURSED;
				}
                if (rogue.weapon->quiverNumber) {
                    rogue.weapon->quiverNumber = rand_range(1, 60000);
                }
			} else {
				message("一阵金色的光照亮了你的双手，但很快就消散了。", false);
			}
            createFlare(player.xLoc, player.yLoc, SCROLL_PROTECTION_LIGHT);
			break;
		case SCROLL_MAGIC_MAPPING:
			confirmMessages();
			messageWithColor("你自己看看发现卷轴上画着地图！", &itemMessageColor, false);
			for (i=0; i<DCOLS; i++) {
				for (j=0; j<DROWS; j++) {
					if (!(pmap[i][j].flags & DISCOVERED) && pmap[i][j].layers[DUNGEON] != GRANITE) {
						pmap[i][j].flags |= MAGIC_MAPPED;
					}
				}
			}
			for (i=0; i<DCOLS; i++) {
				for (j=0; j<DROWS; j++) {
					if (cellHasTMFlag(i, j, TM_IS_SECRET)) {
						discover(i, j);
						pmap[i][j].flags |= MAGIC_MAPPED;
						pmap[i][j].flags &= ~STABLE_MEMORY;
					}
				}
			}
			colorFlash(&magicMapFlashColor, 0, MAGIC_MAPPED, 15, DCOLS, player.xLoc, player.yLoc);
			break;
		case SCROLL_AGGRAVATE_MONSTER:
			aggravateMonsters();
			colorFlash(&gray, 0, (DISCOVERED | MAGIC_MAPPED), 10, DCOLS / 2, player.xLoc, player.yLoc);
			message("卷轴发出了巨大的响声，回声在空挡的地牢里荡漾。", false);
			break;
		case SCROLL_SUMMON_MONSTER:
			for (j=0; j<25 && numberOfMonsters < 3; j++) {
				for (i=0; i<8; i++) {
					x = player.xLoc + nbDirs[i][0];
					y = player.yLoc + nbDirs[i][1];
					if (!cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY) && !(pmap[x][y].flags & HAS_MONSTER)
						&& rand_percent(10) && (numberOfMonsters < 3)) {
						monst = spawnHorde(0, x, y, (HORDE_LEADER_CAPTIVE | HORDE_NO_PERIODIC_SPAWN | HORDE_IS_SUMMONED | HORDE_MACHINE_ONLY), 0);
						if (monst) {
							// refreshDungeonCell(x, y);
							// monst->creatureState = MONSTER_TRACKING_SCENT;
							// monst->ticksUntilTurn = player.movementSpeed;
							wakeUp(monst);
							numberOfMonsters++;
						}
					}
				}
			}
			if (numberOfMonsters > 1) {
				message("你看到眼前的景象开始扭曲，然后突然出现了几只怪物！", false);
			} else if (numberOfMonsters == 1) {
				message("你看到眼前的景象开始扭曲，然后突然出现了一只怪物！", false);
			} else {
				message("你看到眼前的景象开始扭曲，但是随后什么都没有发生。", false);
			}
			break;
//		case SCROLL_CAUSE_FEAR:
//            causeFear("the scroll");
//			break;
		case SCROLL_NEGATION:
            negationBlast("这个卷轴");
			break;
		case SCROLL_SHATTERING:
			messageWithColor("这件卷轴突然发出一阵蓝色的光，击穿了附近的墙壁！", &itemMessageColor, false);
			crystalize(9);
			break;
	}
}

void detectMagicOnItem(item *theItem) {
    theItem->flags |= ITEM_MAGIC_DETECTED;
    if ((theItem->category & (WEAPON | ARMOR))
        && theItem->enchant1 == 0
        && !(theItem->flags & ITEM_RUNIC)) {
        
        identify(theItem);
    }
}

void drinkPotion(item *theItem) {
	item *tempItem;
	creature *monst;
	boolean hadEffect = false;
	boolean hadEffect2 = false;
    char buf[1000];
	
	switch (theItem->kind) {
		case POTION_LIFE:
			if (player.status[STATUS_HALLUCINATING] > 1) {
				player.status[STATUS_HALLUCINATING] = 1;
			}
			if (player.status[STATUS_CONFUSED] > 1) {
				player.status[STATUS_CONFUSED] = 1;
			}
			if (player.status[STATUS_NAUSEOUS] > 1) {
				player.status[STATUS_NAUSEOUS] = 1;
			}
			if (player.status[STATUS_SLOWED] > 1) {
				player.status[STATUS_SLOWED] = 1;
			}
			if (player.status[STATUS_WEAKENED] > 1) {
                player.weaknessAmount = 0;
				player.status[STATUS_WEAKENED] = 1;
			}
			if (player.status[STATUS_POISONED]) {
				player.status[STATUS_POISONED] = 0;
			}
			if (player.status[STATUS_DARKNESS] > 0) {
				player.status[STATUS_DARKNESS] = 0;
				updateMinersLightRadius();
				updateVision(true);
			}
            sprintf(buf, "%s你的生命值上限提高了%i%%。",
                    ((player.currentHP < player.info.maxHP) ? "你的生命力已完全恢复，同时" : ""),
                    (player.info.maxHP + 10) * 100 / player.info.maxHP - 100);
            
            player.info.maxHP += 10;
            player.currentHP = player.info.maxHP;
            updatePlayerRegenerationDelay();
            messageWithColor(buf, &advancementMessageColor, false);
			break;
		case POTION_HALLUCINATION:
			player.status[STATUS_HALLUCINATING] = player.maxStatus[STATUS_HALLUCINATING] = 300;
			message("你发现四周到处都是绚丽的彩色！墙壁在摇晃着唱着歌！", false);
			break;
		case POTION_INCINERATION:
			colorFlash(&darkOrange, 0, IN_FIELD_OF_VIEW, 4, 4, player.xLoc, player.yLoc);
			message("你打开瓶盖，里面突然涌出一阵火焰！", false);
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_FLAMETHROWER], true, false);
			exposeCreatureToFire(&player);
			break;
		case POTION_DARKNESS:
			player.status[STATUS_DARKNESS] = max(400, player.status[STATUS_DARKNESS]);
			player.maxStatus[STATUS_DARKNESS] = max(400, player.maxStatus[STATUS_DARKNESS]);
			updateMinersLightRadius();
			updateVision(true);
			message("你的视野被一片突然而来的黑暗覆盖！", false);
			break;
		case POTION_DESCENT:
			colorFlash(&darkBlue, 0, IN_FIELD_OF_VIEW, 3, 3, player.xLoc, player.yLoc);
			message("瓶中泻出的气体使得附近的地面开始消失！", false);
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_HOLE_POTION], true, false);
			break;
		case POTION_STRENGTH:
			rogue.strength++;
			if (player.status[STATUS_WEAKENED]) {
				player.status[STATUS_WEAKENED] = 1;
			}
			updateEncumbrance();
			messageWithColor("一股力量充满了你的身体。", &advancementMessageColor, false);
            createFlare(player.xLoc, player.yLoc, POTION_STRENGTH_LIGHT);
			break;
		case POTION_POISON:
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_POISON_GAS_CLOUD_POTION], true, false);
			message("剧毒的气体从瓶口处涌出！", false);
			break;
		case POTION_PARALYSIS:
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_PARALYSIS_GAS_CLOUD_POTION], true, false);
			message("瓶子中涌出的气体让你的肌肉突然紧绷起来，你感觉被麻痹了！", false);
			break;
		case POTION_TELEPATHY:
            makePlayerTelepathic(300);
			break;
		case POTION_LEVITATION:
			player.status[STATUS_LEVITATING] = player.maxStatus[STATUS_LEVITATING] = 100;
			player.bookkeepingFlags &= ~MONST_SEIZED; // break free of holding monsters
			message("你缓慢的悬浮到了空中！", false);
			break;
		case POTION_CONFUSION:
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_CONFUSION_GAS_CLOUD_POTION], true, false);
			message("一股彩虹色的闪亮气体充满空气中，你感觉有些不知道自己在哪里。", false);
			break;
		case POTION_LICHEN:
			message("打开瓶子的一瞬间一些奇怪的孢子掉落在了地上。", false);
			spawnDungeonFeature(player.xLoc, player.yLoc, &dungeonFeatureCatalog[DF_LICHEN_PLANTED], true, false);
			break;
		case POTION_DETECT_MAGIC:
			hadEffect = false;
			hadEffect2 = false;
			for (tempItem = floorItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
				if (tempItem->category & CAN_BE_DETECTED) {
                    detectMagicOnItem(tempItem);
					if (itemMagicChar(tempItem)) {
						pmap[tempItem->xLoc][tempItem->yLoc].flags |= ITEM_DETECTED;
						hadEffect = true;
						refreshDungeonCell(tempItem->xLoc, tempItem->yLoc);
					}
				}
			}
			for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				if (monst->carriedItem && (monst->carriedItem->category & CAN_BE_DETECTED)) {
                    detectMagicOnItem(monst->carriedItem);
					if (itemMagicChar(monst->carriedItem)) {
						hadEffect = true;
						refreshDungeonCell(monst->xLoc, monst->yLoc);
					}
				}
			}
			for (tempItem = packItems->nextItem; tempItem != NULL; tempItem = tempItem->nextItem) {
				if (tempItem->category & CAN_BE_DETECTED) {
                    detectMagicOnItem(tempItem);
					if (itemMagicChar(tempItem)) {
						if (tempItem->flags & ITEM_MAGIC_DETECTED) {
							hadEffect2 = true;
						}
					}
				}
			}
			if (hadEffect || hadEffect2) {
				if (hadEffect && hadEffect2) {
					message("不知怎么的你能感觉到来自附近的，以及你包裹中的魔法力量。", false);
				} else if (hadEffect) {
					message("不知怎么的你能感觉到来自附近的魔法力量。", false);
				} else {
					message("不知怎么的你能感觉到来自你包裹中的魔法力量。", false);
				}
			} else {
				message("不知怎么的你能感觉到你包裹里以及附近都没有什么魔法的痕迹。", false);
			}
			break;
		case POTION_HASTE_SELF:
			haste(&player, 25);
			break;
		case POTION_FIRE_IMMUNITY:
			player.status[STATUS_IMMUNE_TO_FIRE] = player.maxStatus[STATUS_IMMUNE_TO_FIRE] = 150;
			if (player.status[STATUS_BURNING]) {
				extinguishFireOnCreature(&player);
			}
			message("你感觉到喝下去的药剂很冰爽，一段时间内对火焰免疫。", false);
			break;
		case POTION_INVISIBILITY:
			player.status[STATUS_INVISIBLE] = player.maxStatus[STATUS_INVISIBLE] = 75;
			message("一阵冰凉的感觉从你的脊背窜下。", false);
			break;
		default:
			message("你感觉到很奇怪，你的身体都不知道应该如何反应。", true);
	}
}

// Used for the Discoveries screen. Returns a number: 1 == good, -1 == bad, 0 == could go either way.
short magicCharDiscoverySuffix(short category, short kind) {
	short result = 0;
	
	switch (category) {
		case SCROLL:
			switch (kind) {
				case SCROLL_AGGRAVATE_MONSTER:
				case SCROLL_SUMMON_MONSTER:
					result = -1;
					break;
				default:
					result = 1;
					break;
			}
			break;
		case POTION:
			switch (kind) {
				case POTION_HALLUCINATION:
				case POTION_INCINERATION:
				case POTION_DESCENT:
				case POTION_POISON:
				case POTION_PARALYSIS:
				case POTION_CONFUSION:
				case POTION_LICHEN:
				case POTION_DARKNESS:
					result = -1;
					break;
				default:
					result = 1;
					break;
			}
			break;
		case WAND:
			switch (kind) {
				case WAND_PLENTY:
				case WAND_INVISIBILITY:
				case WAND_BECKONING:
					result = -1;
					break;
				default:
					result = 1;
					break;
			}
			break;
		case STAFF:
			switch (kind) {
				case STAFF_HEALING:
				case STAFF_HASTE:
				case STAFF_PROTECTION:
					result = -1;
					break;
				default:
					result = 1;
					break;
			}
			break;
		case RING:
			result = 0;
            break;
		case CHARM:
			result = 1;
			break;
	}
	return result;
}

uchar itemMagicChar(item *theItem) {
	switch (theItem->category) {
		case WEAPON:
		case ARMOR:
			if ((theItem->flags & ITEM_CURSED) || theItem->enchant1 < 0) {
				return BAD_MAGIC_CHAR;
			} else if (theItem->enchant1 > 0) {
				return GOOD_MAGIC_CHAR;
			}
			return 0;
			break;
		case SCROLL:
			switch (theItem->kind) {
				case SCROLL_AGGRAVATE_MONSTER:
				case SCROLL_SUMMON_MONSTER:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case POTION:
			switch (theItem->kind) {
				case POTION_HALLUCINATION:
				case POTION_INCINERATION:
				case POTION_DESCENT:
				case POTION_POISON:
				case POTION_PARALYSIS:
				case POTION_CONFUSION:
				case POTION_LICHEN:
				case POTION_DARKNESS:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case WAND:
			if (theItem->charges == 0) {
				return 0;
			}
			switch (theItem->kind) {
				case WAND_PLENTY:
				case WAND_INVISIBILITY:
				case WAND_BECKONING:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case STAFF:
			switch (theItem->kind) {
				case STAFF_HEALING:
				case STAFF_HASTE:
				case STAFF_PROTECTION:
					return BAD_MAGIC_CHAR;
				default:
					return GOOD_MAGIC_CHAR;
			}
		case RING:
			if (theItem->flags & ITEM_CURSED || theItem->enchant1 < 0) {
				return BAD_MAGIC_CHAR;
			} else if (theItem->enchant1 > 0) {
				return GOOD_MAGIC_CHAR;
			} else {
				return 0;
			}
        case CHARM:
            return GOOD_MAGIC_CHAR;
            break;
		case AMULET:
			return AMULET_CHAR;
	}
	return 0;
}

void unequip(item *theItem) {
	char buf[COLS*3], buf2[COLS*3];
	unsigned char command[3];
	
	command[0] = UNEQUIP_KEY;
	if (theItem == NULL) {
		theItem = promptForItemOfType(ALL_ITEMS, ITEM_EQUIPPED, 0,
									  "选择要取下的物品：（a-z, 按住<shift>键弹出物品菜单。<esc>键取消）", true);
	}
	if (theItem == NULL) {
		return;
	}
	
	command[1] = theItem->inventoryLetter;
	command[2] = '\0';
	
	if (!(theItem->flags & ITEM_EQUIPPED)) {
		itemName(theItem, buf2, false, false, NULL);
		sprintf(buf, "你的%s当前并没有被装备。", buf2);
		confirmMessages();
		messageWithColor(buf, &itemMessageColor, false);
		return;
	} else if (theItem->flags & ITEM_CURSED) { // this is where the item gets unequipped
		itemName(theItem, buf2, false, false, NULL);
		sprintf(buf, "你发现自己没法取下来这件%s；它好像是被诅咒的。", buf2);
		confirmMessages();
		messageWithColor(buf, &itemMessageColor, false);
		return;
	} else {
		recordKeystrokeSequence(command);
		unequipItem(theItem, false);
		if (theItem->category & RING) {
			updateRingBonuses();
		}
		itemName(theItem, buf2, true, true, NULL);
		if (strLenWithoutEscapes(buf2) > 52) {
			itemName(theItem, buf2, false, true, NULL);
		}
		confirmMessages();
		updateEncumbrance();
		sprintf(buf, "现在你不再%s这件%s。", (theItem->category & WEAPON ? "装备着" : "穿着"), buf2);
		messageWithColor(buf, &itemMessageColor, false);
	}
	playerTurnEnded();
}

boolean canDrop() {
	if (cellHasTerrainFlag(player.xLoc, player.yLoc, T_OBSTRUCTS_ITEMS)) {
		return false;
	}
	return true;
}

void drop(item *theItem) {
	char buf[COLS*3], buf2[COLS*3];
	unsigned char command[3];
	
	command[0] = DROP_KEY;
	if (theItem == NULL) {
		theItem = promptForItemOfType(ALL_ITEMS, 0, 0,
									  "选择要丢弃的物品：（a-z, 按住<shift>键弹出物品菜单。<esc>键取消）", true);
	}
	if (theItem == NULL) {
		return;
	}
	command[1] = theItem->inventoryLetter;
	command[2] = '\0';
	
	if ((theItem->flags & ITEM_EQUIPPED) && (theItem->flags & ITEM_CURSED)) {
		itemName(theItem, buf2, false, false, NULL);
		sprintf(buf, "你发现自己没丢掉这件%s；它好像是被诅咒的。", buf2);
		confirmMessages();
		messageWithColor(buf, &itemMessageColor, false);
	} else if (canDrop()) {
		recordKeystrokeSequence(command);
		if (theItem->flags & ITEM_EQUIPPED) {
			unequipItem(theItem, false);
		}
		theItem = dropItem(theItem); // This is where it gets dropped.
		theItem->flags |= ITEM_PLAYER_AVOIDS; // Try not to pick up stuff you've already dropped.
		itemName(theItem, buf2, true, true, NULL);
		sprintf(buf, "你把%s丢到了地面上。", buf2);
		messageWithColor(buf, &itemMessageColor, false);
		playerTurnEnded();
	} else {
		confirmMessages();
		message("你站着的地上已经有些别的东西了。不能再丢在这里。", false);
	}
}

item *promptForItemOfType(unsigned short category,
						  unsigned long requiredFlags,
						  unsigned long forbiddenFlags,
						  char *prompt,
						  boolean allowInventoryActions) {
	char keystroke;
	item *theItem;
	
	if (!numberOfMatchingPackItems(ALL_ITEMS, requiredFlags, forbiddenFlags, true)) {
		return NULL;
	}
	
	temporaryMessage(prompt, false);
	
	keystroke = displayInventory(category, requiredFlags, forbiddenFlags, false, allowInventoryActions);
	
	if (!keystroke) {
		// This can happen if the player does an action with an item directly from the inventory screen via a button.
		return NULL;
	}
	
	if (keystroke < 'a' || keystroke > 'z') {
		confirmMessages();
		if (keystroke != ESCAPE_KEY && keystroke != ACKNOWLEDGE_KEY) {
			message("无效的输入。", false);
		}
		return NULL;
	}
	
	theItem = itemOfPackLetter(keystroke);
	if (theItem == NULL) {
		confirmMessages();
		message("没有找到对应的物品。", false);
		return NULL;
	}
	
	return theItem;
}

item *itemOfPackLetter(char letter) {
	item *theItem;
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (theItem->inventoryLetter == letter) {
			return theItem;
		}
	}
	return NULL;
}

item *itemAtLoc(short x, short y) {
	item *theItem;
	
	if (!(pmap[x][y].flags & HAS_ITEM)) {
		return NULL; // easy optimization
	}
	for (theItem = floorItems->nextItem; theItem != NULL && (theItem->xLoc != x || theItem->yLoc != y); theItem = theItem->nextItem);
	if (theItem == NULL) {
		pmap[x][y].flags &= ~HAS_ITEM;
		hiliteCell(x, y, &white, 75, true);
		rogue.automationActive = false;
		message("ERROR: An item was supposed to be here, but I couldn't find it.", true);
		refreshDungeonCell(x, y);
	}
	return theItem;
}

item *dropItem(item *theItem) {
	item *itemFromTopOfStack, *itemOnFloor;
	
	if (cellHasTerrainFlag(player.xLoc, player.yLoc, T_OBSTRUCTS_ITEMS)) {
		return NULL;
	}
	
	itemOnFloor = itemAtLoc(player.xLoc, player.yLoc);
	
	if (theItem->quantity > 1 && !(theItem->category & WEAPON)) { // peel off the top item and drop it
		itemFromTopOfStack = generateItem(ALL_ITEMS, -1);
		*itemFromTopOfStack = *theItem; // clone the item
		theItem->quantity--;
		itemFromTopOfStack->quantity = 1;
		if (itemOnFloor) {
			itemOnFloor->inventoryLetter = theItem->inventoryLetter; // just in case all letters are taken
			pickUpItemAt(player.xLoc, player.yLoc);
		}
		placeItem(itemFromTopOfStack, player.xLoc, player.yLoc);
		return itemFromTopOfStack;
	} else { // drop the entire item
		removeItemFromChain(theItem, packItems);
		if (itemOnFloor) {
			itemOnFloor->inventoryLetter = theItem->inventoryLetter;
			pickUpItemAt(player.xLoc, player.yLoc);
		}
		placeItem(theItem, player.xLoc, player.yLoc);
		return theItem;
	}
}

void recalculateEquipmentBonuses() {
	float enchant;
	item *theItem;
	
	if (rogue.weapon) {
		theItem = rogue.weapon;
		enchant = netEnchant(theItem);
		player.info.damage = theItem->damage;
		player.info.damage.lowerBound *= pow(WEAPON_ENCHANT_DAMAGE_FACTOR, enchant);
		player.info.damage.upperBound *= pow(WEAPON_ENCHANT_DAMAGE_FACTOR, enchant);
		if (player.info.damage.lowerBound < 1) {
			player.info.damage.lowerBound = 1;
		}
		if (player.info.damage.upperBound < 1) {
			player.info.damage.upperBound = 1;
		}
	}
	
	if (rogue.armor) {
		theItem = rogue.armor;
		enchant = netEnchant(theItem);
		player.info.defense = theItem->armor + enchant * 10;
		if (player.info.defense < 0) {
			player.info.defense = 0;
		}
	}
}

void equipItem(item *theItem, boolean force) {
	item *previouslyEquippedItem = NULL;
	
	if ((theItem->category & RING) && (theItem->flags & ITEM_EQUIPPED)) {
		return;
	}
	
	if (theItem->category & WEAPON) {
		previouslyEquippedItem = rogue.weapon;
	} else if (theItem->category & ARMOR) {
		previouslyEquippedItem = rogue.armor;
	}
	if (previouslyEquippedItem) {
		if (!force && (previouslyEquippedItem->flags & ITEM_CURSED)) {
			return; // already using a cursed item
		} else {
			unequipItem(previouslyEquippedItem, force);
		}
	}
	if (theItem->category & WEAPON) {
		rogue.weapon = theItem;
		recalculateEquipmentBonuses();
	} else if (theItem->category & ARMOR) {
		rogue.armor = theItem;
		recalculateEquipmentBonuses();
	} else if (theItem->category & RING) {
		if (rogue.ringLeft && rogue.ringRight) {
			return;
		}
		if (rogue.ringLeft) {
			rogue.ringRight = theItem;
		} else {
			rogue.ringLeft = theItem;
		}
		updateRingBonuses();
		if (theItem->kind == RING_CLAIRVOYANCE) {
			updateClairvoyance();
			displayLevel();
            identifyItemKind(theItem);
		} else if (theItem->kind == RING_LIGHT) {
            identifyItemKind(theItem);
		}
	}
	theItem->flags |= ITEM_EQUIPPED;
	return;
}

void unequipItem(item *theItem, boolean force) {
	
	if (theItem == NULL || !(theItem->flags & ITEM_EQUIPPED)) {
		return;
	}
	if ((theItem->flags & ITEM_CURSED) && !force) {
		return;
	}
	theItem->flags &= ~ITEM_EQUIPPED;
	if (theItem->category & WEAPON) {
		player.info.damage.lowerBound = 1;
		player.info.damage.upperBound = 2;
		player.info.damage.clumpFactor = 1;
		rogue.weapon = NULL;
	}
	if (theItem->category & ARMOR) {
		player.info.defense = 0;
		rogue.armor = NULL;
	}
	if (theItem->category & RING) {
		if (rogue.ringLeft == theItem) {
			rogue.ringLeft = NULL;
		} else if (rogue.ringRight == theItem) {
			rogue.ringRight = NULL;
		}
		updateRingBonuses();
		if (theItem->kind == RING_CLAIRVOYANCE) {
			updateClairvoyance();
			updateClairvoyance(); // Yes, we have to call this twice.
			displayLevel();
		}
	}
	updateEncumbrance();
	return;
}

short ringEnchant(item *theItem) {
    if (theItem->category != RING) {
        return 0;
    }
    if (!(theItem->flags & ITEM_IDENTIFIED)
        && theItem->enchant1 > 0) {
        
        return theItem->enchant2; // Unidentified positive rings act as +1 until identified.
    }
    return theItem->enchant1;
}

void updateRingBonuses() {
	short i;
	item *rings[2] = {rogue.ringLeft, rogue.ringRight};
	
	rogue.clairvoyance = rogue.aggravating = rogue.stealthBonus = rogue.transference
	= rogue.awarenessBonus = rogue.regenerationBonus = rogue.wisdomBonus = 0;
	rogue.lightMultiplier = 1;
	
	for (i=0; i<= 1; i++) {
		if (rings[i]) {
			switch (rings[i]->kind) {
				case RING_CLAIRVOYANCE:
					rogue.clairvoyance += ringEnchant(rings[i]);
					break;
				case RING_STEALTH:
					rogue.stealthBonus += ringEnchant(rings[i]);
					break;
				case RING_REGENERATION:
					rogue.regenerationBonus += ringEnchant(rings[i]);
					break;
				case RING_TRANSFERENCE:
					rogue.transference += ringEnchant(rings[i]);
					break;
				case RING_LIGHT:
					rogue.lightMultiplier += ringEnchant(rings[i]);
					break;
				case RING_AWARENESS:
					rogue.awarenessBonus += 20 * ringEnchant(rings[i]);
					break;
				case RING_WISDOM:
					rogue.wisdomBonus += ringEnchant(rings[i]);
					break;
			}
		}
	}
	
	if (rogue.lightMultiplier <= 0) {
		rogue.lightMultiplier--; // because it starts at positive 1 instead of 0
	}
	
	updateMinersLightRadius();
	updatePlayerRegenerationDelay();
	
	if (rogue.stealthBonus < 0) {
		rogue.stealthBonus *= 4;
		rogue.aggravating = true;
	}
	
	if (rogue.aggravating) {
		aggravateMonsters();
	}
}

void updatePlayerRegenerationDelay() {
	short maxHP;
	long turnsForFull; // In thousandths of a turn.
	maxHP = player.info.maxHP;
	turnsForFull = turnsForFullRegen(rogue.regenerationBonus);
	
	player.regenPerTurn = 0;
	while (maxHP > turnsForFull / 1000) {
		player.regenPerTurn++;
		maxHP -= turnsForFull / 1000;
	}
	
	player.info.turnsBetweenRegen = (turnsForFull / maxHP);
	// DEBUG printf("\nTurnsForFull: %i; regenPerTurn: %i; (thousandths of) turnsBetweenRegen: %i", turnsForFull, player.regenPerTurn, player.info.turnsBetweenRegen);
}

boolean removeItemFromChain(item *theItem, item *theChain) {
	item *previousItem;
	
	for (previousItem = theChain;
		 previousItem->nextItem;
		 previousItem = previousItem->nextItem) {
		if (previousItem->nextItem == theItem) {
			previousItem->nextItem = theItem->nextItem;
			return true;
		}
	}
	return false;
}

void deleteItem(item *theItem) {
	free(theItem);
}

void resetItemTableEntry(itemTable *theEntry) {
	theEntry->identified = false;
	theEntry->called = false;
	theEntry->callTitle[0] = '\0';
}

void shuffleFlavors() {
	short i, j, randIndex, randNumber;
	char buf[COLS];
	
	//	for (i=0; i<NUMBER_FOOD_KINDS; i++) {
	//		resetItemTableEntry(foodTable + i);
	//	}
	for (i=0; i<NUMBER_POTION_KINDS; i++) {
		resetItemTableEntry(potionTable + i);
	}
	//	for (i=0; i<NUMBER_WEAPON_KINDS; i++) {
	//		resetItemTableEntry(weaponTable + i);
	//	}
	//	for (i=0; i<NUMBER_ARMOR_KINDS; i++) {
	//		resetItemTableEntry(armorTable + i);
	//	}
	for (i=0; i<NUMBER_STAFF_KINDS; i++) {
		resetItemTableEntry(staffTable+ i);
	}
	for (i=0; i<NUMBER_WAND_KINDS; i++) {
		resetItemTableEntry(wandTable + i);
	}
	for (i=0; i<NUMBER_SCROLL_KINDS; i++) {
		resetItemTableEntry(scrollTable + i);
	}
	for (i=0; i<NUMBER_RING_KINDS; i++) {
		resetItemTableEntry(ringTable + i);
	}
	
	for (i=0; i<NUMBER_ITEM_COLORS; i++) {
		strcpy(itemColors[i], itemColorsRef[i]);
	}
	for (i=0; i<NUMBER_ITEM_COLORS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_COLORS - 1);
		strcpy(buf, itemColors[i]);
		strcpy(itemColors[i], itemColors[randIndex]);
		strcpy(itemColors[randIndex], buf);
	}
	
	for (i=0; i<NUMBER_ITEM_WOODS; i++) {
		strcpy(itemWoods[i], itemWoodsRef[i]);
	}
	for (i=0; i<NUMBER_ITEM_WOODS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_WOODS - 1);
		strcpy(buf, itemWoods[i]);
		strcpy(itemWoods[i], itemWoods[randIndex]);
		strcpy(itemWoods[randIndex], buf);
	}
	
	for (i=0; i<NUMBER_ITEM_GEMS; i++) {
		strcpy(itemGems[i], itemGemsRef[i]);
	}
	for (i=0; i<NUMBER_ITEM_GEMS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_GEMS - 1);
		strcpy(buf, itemGems[i]);
		strcpy(itemGems[i], itemGems[randIndex]);
		strcpy(itemGems[randIndex], buf);
	}
	
	for (i=0; i<NUMBER_ITEM_METALS; i++) {
		strcpy(itemMetals[i], itemMetalsRef[i]);
	}
	for (i=0; i<NUMBER_ITEM_METALS; i++) {
		randIndex = rand_range(0, NUMBER_ITEM_METALS - 1);
		strcpy(buf, itemMetals[i]);
		strcpy(itemMetals[i], itemMetals[randIndex]);
		strcpy(itemMetals[randIndex], buf);
	}
	
	for (i=0; i<NUMBER_SCROLL_KINDS; i++) {
		itemTitles[i][0] = '\0';
		randNumber = rand_range(3, 4);
		for (j=0; j<randNumber; j++) {
			randIndex = rand_range(0, NUMBER_TITLE_PHONEMES - 1);
			strcpy(buf, itemTitles[i]);
			// no space between phonemes in chinese
			sprintf(itemTitles[i], "%s%s", buf, titlePhonemes[randIndex]);
		}
	}
}

unsigned long itemValue(item *theItem) {
	const short weaponRunicIntercepts[] = {
		700,	//W_SPEED,
		1000,	//W_QUIETUS,
		900,	//W_PARALYSIS,
		800,	//W_MULTIPLICITY,
		700,	//W_SLOWING,
		750,	//W_CONFUSION,
        850,    //W_FORCE,
		500,	//W_SLAYING,
		-1000,	//W_MERCY,
		-1000,	//W_PLENTY,
	};
	const short armorRunicIntercepts[] = {
		900,	//A_MULTIPLICITY,
		700,	//A_MUTUALITY,
		900,	//A_ABSORPTION,
		900,	//A_REPRISAL,
		500,	//A_IMMUNITY,
		900,	//A_REFLECTION,
        750,    //A_RESPIRATION
        500,    //A_DAMPENING
		-1000,	//A_BURDEN,
		-1000,	//A_VULNERABILITY,
        -1000,  //A_IMMOLATION,
	};
	
	signed long value;
	
	switch (theItem->category) {
		case FOOD:
			return foodTable[theItem->kind].marketValue * theItem->quantity;
			break;
		case WEAPON:
			value = (signed long) (weaponTable[theItem->kind].marketValue * theItem->quantity
								   * (1 + 0.15 * (theItem->enchant1 + (theItem->flags & ITEM_PROTECTED ? 1 : 0))));
			if (theItem->flags & ITEM_RUNIC) {
				value += weaponRunicIntercepts[theItem->enchant2];
				if (value < 0) {
					value = 0;
				}
			}
			return (unsigned long) value;
			break;
		case ARMOR:
			value = (signed long) (armorTable[theItem->kind].marketValue * theItem->quantity
								   * (1 + 0.15 * (theItem->enchant1 + ((theItem->flags & ITEM_PROTECTED) ? 1 : 0))));
			if (theItem->flags & ITEM_RUNIC) {
				value += armorRunicIntercepts[theItem->enchant2];
				if (value < 0) {
					value = 0;
				}
			}
			return (unsigned long) value;
			break;
		case SCROLL:
			return scrollTable[theItem->kind].marketValue * theItem->quantity;
			break;
		case POTION:
			return potionTable[theItem->kind].marketValue * theItem->quantity;
			break;
		case STAFF:
			return staffTable[theItem->kind].marketValue * theItem->quantity
			* (float) (1 + 0.15 * (theItem->enchant1 - 1));
			break;
		case WAND:
			return wandTable[theItem->kind].marketValue * theItem->quantity;
			break;
		case RING:
			return ringTable[theItem->kind].marketValue * theItem->quantity
			* (float) (1 + 0.15 * (theItem->enchant1 - 1));
			break;
		case CHARM:
			return charmTable[theItem->kind].marketValue * theItem->quantity
			* (float) (1 + 0.15 * (theItem->enchant1 - 1));
			break;
		case AMULET:
			return 10000;
			break;
		case GEM:
			return 5000 * theItem->quantity;
			break;
		case KEY:
			return 0;
			break;
		default:
			return 0;
			break;
	}
}
