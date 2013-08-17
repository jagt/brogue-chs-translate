/* -*- tab-width: 4 -*- */

/*
 *  Movement.c
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

#include "Rogue.h"
#include "IncludeGlobals.h"
#include <math.h>

void playerRuns(short direction) {
	short newX, newY, dir;
	boolean cardinalPassability[4];
	
	rogue.disturbed = (player.status[STATUS_CONFUSED] ? true : false);
	
	for (dir = 0; dir < 4; dir++) {
		newX = player.xLoc + nbDirs[dir][0];
		newY = player.yLoc + nbDirs[dir][1];
		cardinalPassability[dir] = monsterAvoids(&player, newX, newY);
	}
	
	while (!rogue.disturbed) {
		if (!playerMoves(direction)) {
			rogue.disturbed = true;
			break;
		}
		
		newX = player.xLoc + nbDirs[direction][0];
		newY = player.yLoc + nbDirs[direction][1];
		if (!coordinatesAreInMap(newX, newY)
			|| monsterAvoids(&player, newX, newY)) {
			
			rogue.disturbed = true;
		}		
		if (isDisturbed(player.xLoc, player.yLoc)) {
			rogue.disturbed = true;
		} else if (direction < 4) {
			for (dir = 0; dir < 4; dir++) {
				newX = player.xLoc + nbDirs[dir][0];
				newY = player.yLoc + nbDirs[dir][1];
				if (cardinalPassability[dir] != monsterAvoids(&player, newX, newY)
					&& !(nbDirs[dir][0] + nbDirs[direction][0] == 0 &&
						 nbDirs[dir][1] + nbDirs[direction][1] == 0)) {
						// dir is not the x-opposite or y-opposite of direction
					rogue.disturbed = true;
				}
			}
		}
	}
	updateFlavorText();
}

enum dungeonLayers highestPriorityLayer(short x, short y, boolean skipGas) {
	short bestPriority = 10000;
	enum dungeonLayers tt, best = 0;
	
	for (tt = 0; tt < NUMBER_TERRAIN_LAYERS; tt++) {
		if (tt == GAS && skipGas) {
			continue;
		}
		if (pmap[x][y].layers[tt] && tileCatalog[pmap[x][y].layers[tt]].drawPriority < bestPriority) {
			bestPriority = tileCatalog[pmap[x][y].layers[tt]].drawPriority;
			best = tt;
		}
	}
	return best;
}

enum dungeonLayers layerWithTMFlag(short x, short y, unsigned long flag) {
	enum dungeonLayers layer;
	
	for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
		if (tileCatalog[pmap[x][y].layers[layer]].mechFlags & flag) {
			return layer;
		}
	}
	return NO_LAYER;
}

enum dungeonLayers layerWithFlag(short x, short y, unsigned long flag) {
	enum dungeonLayers layer;
	
	for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
		if (tileCatalog[pmap[x][y].layers[layer]].flags & flag) {
			return layer;
		}
	}
	return NO_LAYER;
}

// Retrieves a pointer to the flavor text of the highest-priority terrain at the given location
char *tileFlavor(short x, short y) {
	return tileCatalog[pmap[x][y].layers[highestPriorityLayer(x, y, false)]].flavorText;
}

// Retrieves a pointer to the description text of the highest-priority terrain at the given location
char *tileText(short x, short y) {
	return tileCatalog[pmap[x][y].layers[highestPriorityLayer(x, y, false)]].description;
}

void describedItemCategory(short theCategory, char *buf) {
	unsigned short itemCats[9] = {FOOD, WEAPON, ARMOR, POTION, SCROLL, STAFF, WAND, RING, GOLD};
	
	assureCosmeticRNG;
	if (player.status[STATUS_HALLUCINATING] && !rogue.playbackOmniscience) {
		theCategory = itemCats[rand_range(0, 8)];
	}
	switch (theCategory) {
		case FOOD:
			strcpy(buf, "food");
			break;
		case WEAPON:
			strcpy(buf, "a weapon");
			break;
		case ARMOR:
			strcpy(buf, "a suit of armor");
			break;
		case POTION:
			strcpy(buf, "a potion");
			break;
		case SCROLL:
			strcpy(buf, "a scroll");
			break;
		case STAFF:
			strcpy(buf, "a staff");
			break;
		case WAND:
			strcpy(buf, "a wand");
			break;
		case RING:
			strcpy(buf, "a ring");
			break;
		case CHARM:
			strcpy(buf, "a charm");
			break;
		case AMULET:
			strcpy(buf, "the Amulet of Yendor");
			break;
		case GEM:
			strcpy(buf, "a lumenstone");
			break;
		case KEY:
			strcpy(buf, "a key");
			break;
		case GOLD:
			strcpy(buf, "a pile of gold");
			break;
		default:
			strcpy(buf, "something strange");
			break;
	}
	restoreRNG;
}

// Describes the item in question either by naming it if the player has already seen its name,
// or by tersely identifying its category otherwise.
void describedItemName(item *theItem, char *buf) {
	if (rogue.playbackOmniscience || (!player.status[STATUS_HALLUCINATING])) {
		itemName(theItem, buf, (theItem->category & (WEAPON | ARMOR) ? false : true), true, NULL);
	} else {
		describedItemCategory(theItem->category, buf);
	}
}

void describeLocation(char *buf, short x, short y) {
	creature *monst;
	item *theItem, *magicItem;
	boolean standsInTerrain;
	boolean subjectMoving;
	boolean prepositionLocked = false;
	boolean monsterDormant;
	
	char subject[DCOLS];
	char verb[DCOLS];
	char preposition[DCOLS];
	char object[DCOLS];
	char adjective[DCOLS];
	
	assureCosmeticRNG;
	
	if (x == player.xLoc && y == player.yLoc) {
		if (player.status[STATUS_LEVITATING]) {
			sprintf(buf, "you are hovering above %s.", tileText(x, y));
		} else {
			strcpy(buf, tileFlavor(x, y));
		}
		restoreRNG;
		return;
	}
	
	monst = NULL;
	standsInTerrain = ((tileCatalog[pmap[x][y].layers[highestPriorityLayer(x, y, false)]].mechFlags & TM_STAND_IN_TILE) ? true : false);
	theItem = itemAtLoc(x, y);
	monsterDormant = false;
	if (pmap[x][y].flags & HAS_MONSTER) {
		monst = monsterAtLoc(x, y);
	} else if (pmap[x][y].flags & HAS_DORMANT_MONSTER) {
		monst = dormantMonsterAtLoc(x, y);
		monsterDormant = true;
	}
	
	// detecting magical items
	magicItem = NULL;
	if (theItem && !playerCanSeeOrSense(x, y)
		&& (theItem->flags & ITEM_MAGIC_DETECTED)
		&& itemMagicChar(theItem)) {
		magicItem = theItem;
	} else if (monst && !canSeeMonster(monst)
			   && monst->carriedItem
			   && (monst->carriedItem->flags & ITEM_MAGIC_DETECTED)
			   && itemMagicChar(monst->carriedItem)) {
		magicItem = monst->carriedItem;
	}
	if (magicItem) {
		switch (itemMagicChar(magicItem)) {
			case GOOD_MAGIC_CHAR:
				strcpy(object, "benevolent magic");
				break;
			case BAD_MAGIC_CHAR:
				strcpy(object, "malevolent magic");
				break;
			case AMULET_CHAR:
				strcpy(object, "the Amulet of Yendor");
				break;
			default:
				strcpy(object, "mysterious magic");
				break;
		}
		sprintf(buf, "you can detect the aura of %s here.", object);
		restoreRNG;
		return;
	}
	
	// telepathy
	if (monst
        && !canSeeMonster(monst)
        && monsterRevealed(monst)) {
        
		strcpy(adjective, (((!player.status[STATUS_HALLUCINATING] || rogue.playbackOmniscience) && monst->info.displayChar >= 'a' && monst->info.displayChar <= 'z')
						   || (player.status[STATUS_HALLUCINATING] && !rogue.playbackOmniscience && rand_range(0, 1)) ? "small" : "large"));
		if (pmap[x][y].flags & DISCOVERED) {
			strcpy(object, tileText(x, y));
			if (monst->bookkeepingFlags & MONST_SUBMERGED) {
				strcpy(preposition, "under ");
			} else if (monsterDormant) {
				strcpy(preposition, "coming from within ");
			} else if (standsInTerrain) {
				strcpy(preposition, "in ");
			} else {
				strcpy(preposition, "over ");
			}
		} else {
			strcpy(object, "here");
			strcpy(preposition, "");
		}

		sprintf(buf, "you can sense a %s psychic emanation %s%s.", adjective, preposition, object);
		restoreRNG;
		return;
	}
	
	if (monst && !canSeeMonster(monst) && !rogue.playbackOmniscience	// monster is not visible
										// and not invisible but outlined in a gas cloud
		&& (!pmap[x][y].layers[GAS] || !monst->status[STATUS_INVISIBLE])) {
		monst = NULL;
	}
	
	if (!playerCanSeeOrSense(x, y)) {
		if (pmap[x][y].flags & DISCOVERED) { // memory
			if (pmap[x][y].rememberedItemCategory) {
				describedItemCategory(pmap[x][y].rememberedItemCategory, object);
			} else {
				strcpy(object, tileCatalog[pmap[x][y].rememberedTerrain].description);
			}
			sprintf(buf, "you remember seeing %s here.", object);
			restoreRNG;
			return;
		} else if (pmap[x][y].flags & MAGIC_MAPPED) { // magic mapped
			sprintf(buf, "you expect %s to be here.", tileCatalog[pmap[x][y].rememberedTerrain].description);
			restoreRNG;
			return;
		}
		strcpy(buf, "");
		restoreRNG;
		return;
	}
	
	if (monst) {
		
		monsterName(subject, monst, true);
		
		if (pmap[x][y].layers[GAS] && monst->status[STATUS_INVISIBLE]) { // phantoms in gas
			sprintf(buf, "you can perceive the faint outline of %s in %s.", subject, tileCatalog[pmap[x][y].layers[GAS]].description);
			restoreRNG;
			return;
		}
		
		subjectMoving = (monst->turnsSpentStationary == 0
                         && !(monst->info.flags & MONST_GETS_TURN_ON_ACTIVATION));
		
		if (cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY)) {
			strcpy(verb, "is trapped");
			subjectMoving = false;
		} else if (monst->bookkeepingFlags & MONST_CAPTIVE) {
			strcpy(verb, "is shackled in place");
			subjectMoving = false;
		} else if (monst->status[STATUS_PARALYZED]) {
			strcpy(verb, "is frozen in place");
			subjectMoving = false;
		} else if (monst->status[STATUS_STUCK]) {
			strcpy(verb, "is entangled");
			subjectMoving = false;
		} else if (monst->status[STATUS_LEVITATING]) {
			strcpy(verb, (subjectMoving ? "is flying" : "is hovering"));
			strcpy(preposition, "over");
			prepositionLocked = true;
		} else if (monsterCanSubmergeNow(monst)) {
			strcpy(verb, (subjectMoving ? "is gliding" : "is drifting"));
		} else if (cellHasTerrainFlag(x, y, T_MOVES_ITEMS) && !(monst->info.flags & MONST_SUBMERGES)) {
			strcpy(verb, (subjectMoving ? "is swimming" : "is struggling"));
		} else if (cellHasTerrainFlag(x, y, T_AUTO_DESCENT)) {
			strcpy(verb, "is suspended in mid-air");
			strcpy(preposition, "over");
			prepositionLocked = true;
			subjectMoving = false;
		} else if (monst->status[STATUS_CONFUSED]) {
			strcpy(verb, "is staggering");
		} else if ((monst->info.flags & MONST_RESTRICTED_TO_LIQUID)
				   && !cellHasTMFlag(monst->xLoc, monst->yLoc, TM_ALLOWS_SUBMERGING)) {
			strcpy(verb, "is lying");
			subjectMoving = false;
		} else if (monst->info.flags & MONST_IMMOBILE) {
			strcpy(verb, "is resting");
		} else {
			switch (monst->creatureState) {
				case MONSTER_SLEEPING:
					strcpy(verb, "is sleeping");
					subjectMoving = false;
					break;
				case MONSTER_WANDERING:
					strcpy(verb, subjectMoving ? "is wandering" : "is standing");
					break;
				case MONSTER_FLEEING:
					strcpy(verb, subjectMoving ? "is fleeing" : "is standing");
					break;
				case MONSTER_TRACKING_SCENT:
					strcpy(verb, subjectMoving ? "is moving" : "is standing");
					break;
				case MONSTER_ALLY:
					strcpy(verb, subjectMoving ? "is following you" : "is standing");
					break;
				default:
					strcpy(verb, "is standing");
					break;
			}
		}
		if (monst->status[STATUS_BURNING] && !(monst->info.flags & MONST_FIERY)) {
			strcat(verb, ", burning,");
		}
		
		if (theItem) {
			strcpy(preposition, "over");
			describedItemName(theItem, object);
		} else {
			if (!prepositionLocked) {
				strcpy(preposition, subjectMoving ? (standsInTerrain ? "through" : "across")
					   : (standsInTerrain ? "in" : "on"));
			}
			
			strcpy(object, tileText(x, y));
			
		}
	} else { // no monster
		strcpy(object, tileText(x, y));
		if (theItem) {
			describedItemName(theItem, subject);
			subjectMoving = cellHasTerrainFlag(x, y, T_MOVES_ITEMS);
			
			strcpy(verb, (theItem->quantity > 1 || (theItem->category & GOLD)) ? "are" : "is");
			if (cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY)) {
				strcat(verb, " enclosed");
			} else {
				strcat(verb, subjectMoving ? " drifting" : " lying");
			}
			strcpy(preposition, standsInTerrain ? (subjectMoving ? "through" : "in")
				   : (subjectMoving ? "across" : "on"));
			

		} else { // no item
			sprintf(buf, "you %s %s.", (playerCanDirectlySee(x, y) ? "see" : "sense"), object);
			return;
		}
	}
	
	sprintf(buf, "%s %s %s %s.", subject, verb, preposition, object);
	restoreRNG;
}

void printLocationDescription(short x, short y) {
	char buf[DCOLS*3];
	describeLocation(buf, x, y);
	flavorMessage(buf);
}

void exposeCreatureToFire(creature *monst) {
	char buf[COLS], buf2[COLS];
	if (monst->status[STATUS_IMMUNE_TO_FIRE]
		|| (monst->bookkeepingFlags & MONST_SUBMERGED)
		|| ((!monst->status[STATUS_LEVITATING]) && cellHasTMFlag(monst->xLoc, monst->yLoc, TM_EXTINGUISHES_FIRE))) {
		return;
	}
	if (monst->status[STATUS_BURNING] == 0) {
		if (monst == &player) {
			rogue.minersLight.lightColor = &fireForeColor;
			player.info.foreColor = &torchLightColor;
			refreshDungeonCell(player.xLoc, player.yLoc);
			//updateVision(); // this screws up the firebolt visual effect by erasing it while a message is displayed
			message("you catch fire!", true);
		} else if (canDirectlySeeMonster(monst)) {
			monsterName(buf, monst, true);
			sprintf(buf2, "%s catches fire!", buf);
			message(buf2, false);
		}
	}
	monst->status[STATUS_BURNING] = monst->maxStatus[STATUS_BURNING] = max(monst->status[STATUS_BURNING], 7);
}

void updateFlavorText() {
	char buf[DCOLS * 3];
	if (rogue.disturbed && !rogue.gameHasEnded) {
        if (rogue.armor
            && (rogue.armor->flags & ITEM_RUNIC)
            && rogue.armor->enchant2 == A_RESPIRATION
            && tileCatalog[pmap[player.xLoc][player.yLoc].layers[highestPriorityLayer(player.xLoc, player.yLoc, false)]].flags & T_RESPIRATION_IMMUNITIES) {
            
            flavorMessage("A pocket of cool, clean air swirls around you.");
		} else if (player.status[STATUS_LEVITATING]) {
			describeLocation(buf, player.xLoc, player.yLoc);
			flavorMessage(buf);
		} else {
			flavorMessage(tileFlavor(player.xLoc, player.yLoc));
		}
	}
}

void useKeyAt(item *theItem, short x, short y) {
	short layer, i;
	creature *monst;
	char buf[COLS], buf2[COLS], terrainName[COLS], preposition[10];
	boolean disposable;
	
	strcpy(terrainName, "unknown terrain"); // redundant failsafe
	for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
		if (tileCatalog[pmap[x][y].layers[layer]].mechFlags & TM_PROMOTES_WITH_KEY) {
			if (tileCatalog[pmap[x][y].layers[layer]].description[0] == 'a'
				&& tileCatalog[pmap[x][y].layers[layer]].description[1] == ' ') {
				sprintf(terrainName, "the %s", &(tileCatalog[pmap[x][y].layers[layer]].description[2]));
			} else {
				strcpy(terrainName, tileCatalog[pmap[x][y].layers[layer]].description);
			}
			if (tileCatalog[pmap[x][y].layers[layer]].mechFlags & TM_STAND_IN_TILE) {
				strcpy(preposition, "in");
			} else {
				strcpy(preposition, "on");
			}
			promoteTile(x, y, layer, false);
		}
	}
	
	disposable = false;
	for (i=0; i < KEY_ID_MAXIMUM && (theItem->keyLoc[i].x || theItem->keyLoc[i].machine); i++) {
		if (theItem->keyLoc[i].x == x && theItem->keyLoc[i].y == y && theItem->keyLoc[i].disposableHere) {
			disposable = true;
		} else if (theItem->keyLoc[i].machine == pmap[x][y].machineNumber && theItem->keyLoc[i].disposableHere) {
			disposable = true;
		}
	}
	
	if (disposable) {
		if (removeItemFromChain(theItem, packItems)) {
			itemName(theItem, buf2, true, false, NULL);
			sprintf(buf, "you use your %s %s %s.",
					buf2,
					preposition,
					terrainName);
			messageWithColor(buf, &itemMessageColor, false);
			deleteItem(theItem);
		} else if (removeItemFromChain(theItem, floorItems)) {
			deleteItem(theItem);
			pmap[x][y].flags &= ~HAS_ITEM;
		} else if (pmap[x][y].flags & HAS_MONSTER) {
			monst = monsterAtLoc(x, y);
			if (monst->carriedItem && monst->carriedItem == theItem) {
				monst->carriedItem = NULL;
				deleteItem(theItem);
			}
		}
	}
}

boolean monsterShouldFall(creature *monst) {
	return (!(monst->status[STATUS_LEVITATING])
			&& cellHasTerrainFlag(monst->xLoc, monst->yLoc, T_AUTO_DESCENT)
			&& !cellHasTerrainFlag(monst->xLoc, monst->yLoc, T_ENTANGLES | T_OBSTRUCTS_PASSABILITY)
			&& !(monst->bookkeepingFlags & MONST_PREPLACED));
}

// Called at least every 100 ticks; may be called more frequently.
void applyInstantTileEffectsToCreature(creature *monst) {
	char buf[COLS], buf2[COLS];
	short *x = &(monst->xLoc), *y = &(monst->yLoc), damage;
	enum dungeonLayers layer;
	item *theItem;
	
	if (monst->bookkeepingFlags & MONST_IS_DYING) {
		return; // the monster is already dead.
	}
	
	if (monst == &player) {
		if (!player.status[STATUS_LEVITATING]) {
			pmap[*x][*y].flags |= KNOWN_TO_BE_TRAP_FREE;
		}
	} else if (!player.status[STATUS_HALLUCINATING]
			   && !monst->status[STATUS_LEVITATING]
			   && canSeeMonster(monst)
			   && !(cellHasTerrainFlag(*x, *y, T_IS_DF_TRAP))) {
		pmap[*x][*y].flags |= KNOWN_TO_BE_TRAP_FREE;
	}
	
	// You will discover the secrets of any tile you stand on.
	if (monst == &player
		&& !(monst->status[STATUS_LEVITATING])
		&& cellHasTMFlag(*x, *y, TM_IS_SECRET)
		&& playerCanSee(*x, *y)) {
		
		discover(*x, *y);
	}
	
	// Visual effect for submersion in water.
	if (monst == &player) {
		if (rogue.inWater) {
			if (!cellHasTerrainFlag(player.xLoc, player.yLoc, T_IS_DEEP_WATER) || player.status[STATUS_LEVITATING]
				|| cellHasTerrainFlag(player.xLoc, player.yLoc, (T_ENTANGLES | T_OBSTRUCTS_PASSABILITY))) {
				
                rogue.inWater = false;
				updateMinersLightRadius();
				updateVision(true);
				displayLevel();
			}
		} else {
			if (cellHasTerrainFlag(player.xLoc, player.yLoc, T_IS_DEEP_WATER) && !player.status[STATUS_LEVITATING]
				&& !cellHasTerrainFlag(player.xLoc, player.yLoc, (T_ENTANGLES | T_OBSTRUCTS_PASSABILITY))) {
                
				rogue.inWater = true;
				updateMinersLightRadius();
				updateVision(true);
				displayLevel();
			}
		}
	}
	
	// lava
	if (!(monst->status[STATUS_LEVITATING])
		&& !(monst->status[STATUS_IMMUNE_TO_FIRE])
		&& !cellHasTerrainFlag(*x, *y, (T_ENTANGLES | T_OBSTRUCTS_PASSABILITY))
		&& cellHasTerrainFlag(*x, *y, T_LAVA_INSTA_DEATH)) {
		
		if (monst == &player) {
			sprintf(buf, "you plunge into %s!",
					tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, T_LAVA_INSTA_DEATH)]].description);
			message(buf, true);
			sprintf(buf, "Killed by %s",
					tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, T_LAVA_INSTA_DEATH)]].description);
			gameOver(buf, true);
			return;
		} else { // it's a monster
			if (canSeeMonster(monst)) {
				monsterName(buf, monst, true);
				sprintf(buf2, "%s is consumed by %s instantly!", buf,
						tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, T_LAVA_INSTA_DEATH)]].description);
				messageWithColor(buf2, messageColorFromVictim(monst), false);
			}
			killCreature(monst, false);
			refreshDungeonCell(*x, *y);
			return;
		}
	}
	
	// Water puts out fire.
	if (cellHasTMFlag(*x, *y, TM_EXTINGUISHES_FIRE)
        && monst->status[STATUS_BURNING]
        && !monst->status[STATUS_LEVITATING]
        && !(monst->info.flags & MONST_ATTACKABLE_THRU_WALLS)
		&& !(monst->info.flags & MONST_FIERY)) {
		extinguishFireOnCreature(monst);
	}
	
	// If you see a monster use a secret door, you discover it.
	if (playerCanSee(*x, *y)
        && cellHasTMFlag(*x, *y, TM_IS_SECRET)
		&& (cellHasTerrainFlag(*x, *y, T_OBSTRUCTS_PASSABILITY))) {
		discover(*x, *y);
	}
	
	// Creatures plunge into chasms and through trap doors.
	if (monsterShouldFall(monst)) {
		if (monst == &player) {
			// player falling takes place at the end of the turn
			if (!(monst->bookkeepingFlags & MONST_IS_FALLING)) {
				monst->bookkeepingFlags |= MONST_IS_FALLING;
				return;
			}
		} else { // it's a monster
			monst->bookkeepingFlags |= MONST_IS_FALLING; // handled at end of turn
		}
	}
	
	// Pressure plates.
	if (!(monst->status[STATUS_LEVITATING])
		&& !(monst->bookkeepingFlags & MONST_SUBMERGED)
		&& cellHasTerrainFlag(*x, *y, T_IS_DF_TRAP)
		&& !(pmap[*x][*y].flags & PRESSURE_PLATE_DEPRESSED)) {
		
		pmap[*x][*y].flags |= PRESSURE_PLATE_DEPRESSED;
		if (playerCanSee(*x, *y) && cellHasTMFlag(*x, *y, TM_IS_SECRET)) {
			discover(*x, *y);
			refreshDungeonCell(*x, *y);
		}
		if (canSeeMonster(monst)) {
			monsterName(buf, monst, true);
			sprintf(buf2, "a pressure plate clicks underneath %s!", buf);
			message(buf2, true);
		} else if (playerCanSee(*x, *y)) {
			// usually means an invisible monster
			message("a pressure plate clicks!", false);
		}
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[*x][*y].layers[layer]].flags & T_IS_DF_TRAP) {
				spawnDungeonFeature(*x, *y, &(dungeonFeatureCatalog[tileCatalog[pmap[*x][*y].layers[layer]].fireType]), true, false);
				promoteTile(*x, *y, layer, false);
			}
		}
	}
	
	if (cellHasTMFlag(*x, *y, TM_PROMOTES_ON_STEP)) { // flying creatures activate too
		// Because this uses no pressure plate to keep track of whether it's already depressed,
		// it will trigger every time this function is called while the monster or player is on the tile.
		// Because this function can be called several times per turn, multiple promotions can
        // happen unpredictably if the tile does not promote to a tile without the T_PROMOTES_ON_STEP
        // attribute. That's acceptable for some effects, e.g. doors opening,
        // but not for others, e.g. magical glyphs activating.
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[*x][*y].layers[layer]].mechFlags & TM_PROMOTES_ON_STEP) {
				promoteTile(*x, *y, layer, false);
			}
		}
	}
	
	if (cellHasTMFlag(*x, *y, TM_PROMOTES_ON_PLAYER_ENTRY) && monst == &player) {
		// Subject to same caveats as T_PROMOTES_ON_STEP above.
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[*x][*y].layers[layer]].mechFlags & TM_PROMOTES_ON_PLAYER_ENTRY) {
				promoteTile(*x, *y, layer, false);
			}
		}
	}
	
	// spiderwebs
	if (cellHasTerrainFlag(*x, *y, T_ENTANGLES) && !monst->status[STATUS_STUCK]
		&& !(monst->info.flags & MONST_IMMUNE_TO_WEBS) && !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
		monst->status[STATUS_STUCK] = monst->maxStatus[STATUS_STUCK] = rand_range(3, 7);
		if (monst == &player) {
			sprintf(buf2, "you are stuck fast in %s!",
					tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, T_ENTANGLES)]].description);
			message(buf2, false);
		} else if (canDirectlySeeMonster(monst)) { // it's a monster
			monsterName(buf, monst, true);
			sprintf(buf2, "%s is stuck fast in %s!", buf,
					tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, T_ENTANGLES)]].description);
			message(buf2, false);
		}
	}
	
	// explosions
	if (cellHasTerrainFlag(*x, *y, T_CAUSES_EXPLOSIVE_DAMAGE) && !monst->status[STATUS_EXPLOSION_IMMUNITY]
		&& !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
		damage = rand_range(15, 20);
		damage = max(damage, monst->info.maxHP / 2);
		monst->status[STATUS_EXPLOSION_IMMUNITY] = 5;
		if (monst == &player) {
			rogue.disturbed = true;
			for (layer = 0; layer < NUMBER_TERRAIN_LAYERS && !(tileCatalog[pmap[*x][*y].layers[layer]].flags & T_CAUSES_EXPLOSIVE_DAMAGE); layer++);
			message(tileCatalog[pmap[*x][*y].layers[layer]].flavorText, false);
            if (rogue.armor && (rogue.armor->flags & ITEM_RUNIC) && rogue.armor->enchant2 == A_DAMPENING) {
                itemName(rogue.armor, buf2, false, false, NULL);
                sprintf(buf, "Your %s pulses and absorbs the damage.", buf2);
                messageWithColor(buf, &goodMessageColor, false);
                rogue.armor->flags |= ITEM_RUNIC_IDENTIFIED;
            } else if (inflictDamage(&player, damage, &yellow)) {
				strcpy(buf2, tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, T_CAUSES_EXPLOSIVE_DAMAGE)]].description);
				sprintf(buf, "Killed by %s", buf2);
				gameOver(buf, true);
				return;
			}
		} else { // it's a monster
			if (monst->creatureState == MONSTER_SLEEPING) {
				monst->creatureState = MONSTER_TRACKING_SCENT;
			}
			monsterName(buf, monst, true);
			if (inflictDamage(monst, damage, &yellow)) {
				// if killed
				sprintf(buf2, "%s dies in %s.", buf,
						tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, T_CAUSES_EXPLOSIVE_DAMAGE)]].description);
				messageWithColor(buf2, messageColorFromVictim(monst), false);
				refreshDungeonCell(*x, *y);
				return;
			} else {
				// if survived
				sprintf(buf2, "%s engulfs %s.",
						tileCatalog[pmap[*x][*y].layers[layerWithFlag(*x, *y, T_CAUSES_EXPLOSIVE_DAMAGE)]].description, buf);
				messageWithColor(buf2, messageColorFromVictim(monst), false);
			}
		}
	}
    
    // Toxic gases!
    // If it's the player, and he's wearing armor of respiration, then no effect from toxic gases.
    if (monst == &player
        && cellHasTerrainFlag(*x, *y, T_RESPIRATION_IMMUNITIES)
        && rogue.armor
        && (rogue.armor->flags & ITEM_RUNIC)
        && rogue.armor->enchant2 == A_RESPIRATION) {
        if (!(rogue.armor->flags & ITEM_RUNIC_IDENTIFIED)) {
            message("Your armor trembles and a pocket of clean air swirls around you.", false);
            autoIdentify(rogue.armor);
        }
    } else {
        
        // zombie gas
        if (cellHasTerrainFlag(*x, *y, T_CAUSES_NAUSEA)
            && !(monst->info.flags & MONST_INANIMATE)
            && !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
            if (monst == &player) {
                rogue.disturbed = true;
            }
            if (canDirectlySeeMonster(monst) && !(monst->status[STATUS_NAUSEOUS])) {
                if (monst->creatureState == MONSTER_SLEEPING) {
                    monst->creatureState = MONSTER_TRACKING_SCENT;
                }
                flashMonster(monst, &brown, 100);
                monsterName(buf, monst, true);
                sprintf(buf2, "%s choke%s and gag%s on the stench of rot.", buf,
                        (monst == &player ? "": "s"), (monst == &player ? "": "s"));
                message(buf2, false);
            }
            monst->status[STATUS_NAUSEOUS] = monst->maxStatus[STATUS_NAUSEOUS] = max(monst->status[STATUS_NAUSEOUS], 20);
        }
        
        // confusion gas
        if (cellHasTerrainFlag(*x, *y, T_CAUSES_CONFUSION) && !(monst->info.flags & MONST_INANIMATE)) {
            if (monst == &player) {
                rogue.disturbed = true;
            }
            if (canDirectlySeeMonster(monst) && !(monst->status[STATUS_CONFUSED])) {
                if (monst->creatureState == MONSTER_SLEEPING) {
                    monst->creatureState = MONSTER_TRACKING_SCENT;
                }
                flashMonster(monst, &confusionGasColor, 100);
                monsterName(buf, monst, true);
                sprintf(buf2, "%s %s very confused!", buf, (monst == &player ? "feel": "looks"));
                message(buf2, false);
            }
            monst->status[STATUS_CONFUSED] = monst->maxStatus[STATUS_CONFUSED] = max(monst->status[STATUS_CONFUSED], 25);
        }
        
        // paralysis gas
        if (cellHasTerrainFlag(*x, *y, T_CAUSES_PARALYSIS) && !(monst->info.flags & MONST_INANIMATE) && !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
            if (canDirectlySeeMonster(monst) && !monst->status[STATUS_PARALYZED]) {
                flashMonster(monst, &pink, 100);
                monsterName(buf, monst, true);
                sprintf(buf2, "%s %s paralyzed!", buf, (monst == &player ? "are": "is"));
                message(buf2, (monst == &player));
            }
            monst->status[STATUS_PARALYZED] = monst->maxStatus[STATUS_PARALYZED] = max(monst->status[STATUS_PARALYZED], 20);
            if (monst == &player) {
                rogue.disturbed = true;
            }
        }
    }
    
    // poisonous lichen
    if (cellHasTerrainFlag(*x, *y, T_CAUSES_POISON) && !(monst->info.flags & MONST_INANIMATE) && !monst->status[STATUS_LEVITATING]) {
        if (monst == &player && !player.status[STATUS_POISONED]) {
            rogue.disturbed = true;
        }
        if (canDirectlySeeMonster(monst) && !(monst->status[STATUS_POISONED])) {
            if (monst->creatureState == MONSTER_SLEEPING) {
                monst->creatureState = MONSTER_TRACKING_SCENT;
            }
            flashMonster(monst, &green, 100);
            monsterName(buf, monst, true);
            sprintf(buf2, "the lichen's grasping tendrils poison %s.", buf);
            messageWithColor(buf2, messageColorFromVictim(monst), false);
        }
        monst->status[STATUS_POISONED] = max(monst->status[STATUS_POISONED], 10);
        monst->maxStatus[STATUS_POISONED] = monst->info.maxHP;
    }
	
	// fire
	if (cellHasTerrainFlag(*x, *y, T_IS_FIRE)) {
		exposeCreatureToFire(monst);
	} else if (cellHasTerrainFlag(*x, *y, T_IS_FLAMMABLE)
			   && !cellHasTerrainFlag(*x, *y, T_IS_FIRE)
			   && monst->status[STATUS_BURNING]
               && !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
		exposeTileToFire(*x, *y, true);
	}
	
	// keys
	if (cellHasTMFlag(*x, *y, TM_PROMOTES_WITH_KEY) && (theItem = keyOnTileAt(*x, *y))) {
		useKeyAt(theItem, *x, *y);
	}
}

void applyGradualTileEffectsToCreature(creature *monst, short ticks) {
	short itemCandidates, randItemIndex;
	short x = monst->xLoc, y = monst->yLoc, damage;
	char buf[COLS], buf2[COLS];
	item *theItem;
	enum dungeonLayers layer;
	
	if (!(monst->status[STATUS_LEVITATING])
        && cellHasTerrainFlag(x, y, T_IS_DEEP_WATER)
		&& !cellHasTerrainFlag(x, y, (T_ENTANGLES | T_OBSTRUCTS_PASSABILITY))
		&& !(monst->info.flags & MONST_IMMUNE_TO_WATER)) {
		if (monst == &player) {
			if (!(pmap[x][y].flags & HAS_ITEM) && rand_percent(ticks * 50 / 100)) {
				itemCandidates = numberOfMatchingPackItems(ALL_ITEMS, 0, (ITEM_EQUIPPED), false);
				if (itemCandidates) {
					randItemIndex = rand_range(1, itemCandidates);
					for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
						if (!(theItem->flags & (ITEM_EQUIPPED))) {
							if (randItemIndex == 1) {
								break;
							} else {
								randItemIndex--;
							}
						}
					}
					theItem = dropItem(theItem);
					if (theItem) {
						itemName(theItem, buf2, false, true, NULL);
						sprintf(buf, "%s floats away in the current!", buf2);
						messageWithColor(buf, &itemMessageColor, false);
					}
				}
			}
		} else if (monst->carriedItem && !(pmap[x][y].flags & HAS_ITEM) && rand_percent(ticks * 50 / 100)) { // it's a monster with an item
			makeMonsterDropItem(monst);
		}
	}
	
	if (cellHasTerrainFlag(x, y, T_CAUSES_DAMAGE)
		&& !(monst->info.flags & MONST_INANIMATE)
		&& !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
        
		damage = (monst->info.maxHP / 15) * ticks / 100;
		damage = max(1, damage);
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS && !(tileCatalog[pmap[x][y].layers[layer]].flags & T_CAUSES_DAMAGE); layer++);
		if (monst == &player) {
            if (rogue.armor && (rogue.armor->flags & ITEM_RUNIC) && rogue.armor->enchant2 == A_RESPIRATION) {
                if (!(rogue.armor->flags & ITEM_RUNIC_IDENTIFIED)) {
                    message("Your armor trembles and a pocket of clean air swirls around you.", false);
                    autoIdentify(rogue.armor);
                }
            } else {
                rogue.disturbed = true;
                messageWithColor(tileCatalog[pmap[x][y].layers[layer]].flavorText, &badMessageColor, false);
                if (inflictDamage(&player, damage, tileCatalog[pmap[x][y].layers[layer]].backColor)) {
                    sprintf(buf, "Killed by %s", tileCatalog[pmap[x][y].layers[layer]].description);
                    gameOver(buf, true);
                    return;
                }
            }
		} else { // it's a monster
			if (monst->creatureState == MONSTER_SLEEPING) {
				monst->creatureState = MONSTER_TRACKING_SCENT;
			}
			if (inflictDamage(monst, damage, tileCatalog[pmap[x][y].layers[layer]].backColor)) {
				if (canSeeMonster(monst)) {
					monsterName(buf, monst, true);
					sprintf(buf2, "%s dies.", buf);
					messageWithColor(buf2, messageColorFromVictim(monst), false);
				}
				refreshDungeonCell(x, y);
				return;
			}
		}
	}
    
    if (cellHasTerrainFlag(x, y, T_CAUSES_HEALING)
		&& !(monst->info.flags & MONST_INANIMATE)
		&& !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
        
		damage = (monst->info.maxHP / 15) * ticks / 100;
		damage = max(1, damage);
        if (monst->currentHP < monst->info.maxHP) {
            monst->currentHP += min(damage, monst->info.maxHP - monst->currentHP);
            if (monst == &player) {
                messageWithColor("you feel much better.", &goodMessageColor, false);
            }
        }
    }
}

short randValidDirectionFrom(creature *monst, short x, short y, boolean respectAvoidancePreferences) {
	short i, newX, newY, validDirectionCount = 0, randIndex;
	
#ifdef BROGUE_ASSERTS
	assert(rogue.RNG == RNG_SUBSTANTIVE);
#endif
	
	for (i=0; i<8; i++) {
		newX = x + nbDirs[i][0];
		newY = y + nbDirs[i][1];
		if (coordinatesAreInMap(newX, newY)
			&& !cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)
            && !diagonalBlocked(x, y, newX, newY)
			&& (!respectAvoidancePreferences
				|| (!monsterAvoids(monst, newX, newY))
				|| ((pmap[newX][newY].flags & HAS_PLAYER) && monst->creatureState != MONSTER_ALLY))) {
			validDirectionCount++;
		}
	}
	if (validDirectionCount == 0) {
        // Rare, and important in this case that the function returns BEFORE a random roll is made to avoid OOS.
		return -1;
	}
	randIndex = rand_range(1, validDirectionCount);
	validDirectionCount = 0;
	for (i=0; i<8; i++) {
		newX = x + nbDirs[i][0];
		newY = y + nbDirs[i][1];
		if (coordinatesAreInMap(newX, newY)
			&& !cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)
			&& !diagonalBlocked(x, y, newX, newY)
			&& (!respectAvoidancePreferences
				|| (!monsterAvoids(monst, newX, newY))
				|| ((pmap[newX][newY].flags & HAS_PLAYER) && monst->creatureState != MONSTER_ALLY))) {
			validDirectionCount++;
			if (validDirectionCount == randIndex) {
				return i;
			}
		}
	}
	return -1; // should rarely get here
}

void vomit(creature *monst) {
	char buf[COLS], monstName[COLS];
	spawnDungeonFeature(monst->xLoc, monst->yLoc, &dungeonFeatureCatalog[DF_VOMIT], true, false);
	
	if (canDirectlySeeMonster(monst)) {
		monsterName(monstName, monst, true);
		sprintf(buf, "%s vomit%s profusely", monstName, (monst == &player ? "" : "s"));
		combatMessage(buf, NULL);
	}
}

void moveEntrancedMonsters(enum directions dir) {
	creature *monst;
	
	dir = oppositeDirection(dir);
	
	for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		if (monst->status[STATUS_ENTRANCED]
			&& !monst->status[STATUS_STUCK]
			&& !monst->status[STATUS_PARALYZED]
			&& !(monst->bookkeepingFlags & MONST_CAPTIVE)) {
			
			// && !monsterAvoids(monst, monst->xLoc + nbDirs[dir][0], monst->yLoc + nbDirs[dir][1])
			moveMonster(monst, nbDirs[dir][0], nbDirs[dir][1]);
		}
	}
}

void becomeAllyWith(creature *monst) {
	demoteMonsterFromLeadership(monst);
	// Drop your item.
	if (monst->carriedItem) {
		makeMonsterDropItem(monst);
	}
	// If you're going to change into something, it should be friendly.
	if (monst->carriedMonster) {
		becomeAllyWith(monst->carriedMonster);
	}
	monst->creatureState = MONSTER_ALLY;
	monst->bookkeepingFlags |= MONST_FOLLOWER;
	monst->leader = &player;
	monst->bookkeepingFlags &= ~MONST_CAPTIVE;
	refreshDungeonCell(monst->xLoc, monst->yLoc);
}

void freeCaptive(creature *monst) {
	char buf[COLS * 3], monstName[COLS];
	
	becomeAllyWith(monst);
	monsterName(monstName, monst, false);
	sprintf(buf, "you free the grateful %s and gain a faithful ally.", monstName);
	message(buf, false);
}

boolean freeCaptivesEmbeddedAt(short x, short y) {
	creature *monst;
	
	if (pmap[x][y].flags & HAS_MONSTER) {
		// Free any captives trapped in the tunnelized terrain.
		monst = monsterAtLoc(x, y);
		if ((monst->bookkeepingFlags & MONST_CAPTIVE)
			&& !(monst->info.flags & MONST_ATTACKABLE_THRU_WALLS)
			&& (cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY))) {
			freeCaptive(monst);
			return true;
		}
	}
	return false;
}

// Do we need confirmation so we don't accidently hit an acid mound?
boolean abortAttackAgainstAcidicTarget(creature *hitList[8]) {
    short i;
	char monstName[COLS], weaponName[COLS];
	char buf[COLS*3];
    
    if (rogue.weapon
        && !(rogue.weapon->flags & ITEM_PROTECTED)
        && !player.status[STATUS_HALLUCINATING]
        && !player.status[STATUS_CONFUSED]) {
        
        for (i=0; i<8; i++) {
            if (hitList[i]
                && (hitList[i]->info.flags & MONST_DEFEND_DEGRADE_WEAPON)
                && canSeeMonster(hitList[i])) {
                
                monsterName(monstName, hitList[i], true);
                itemName(rogue.weapon, weaponName, false, false, NULL);
                sprintf(buf, "Degrade your %s by attacking %s?", weaponName, monstName);
                if (confirm(buf, false)) {
                    return false; // Fire when ready!
                } else {
                    return true; // Abort!
                }
            }
        }
    }
    return false;
}

boolean diagonalBlocked(short x1, short y1, short x2, short y2) {
    if (x1 == x2 || y1 == y2) {
        return false; // If it's not a diagonal, it's not diagonally blocked.
    }
    if (cellHasTerrainFlag(x1, y2, T_OBSTRUCTS_DIAGONAL_MOVEMENT)
        || cellHasTerrainFlag(x2, y1, T_OBSTRUCTS_DIAGONAL_MOVEMENT)) {
        return true;
    }
    return false;
}

// Called whenever the player voluntarily tries to move in a given direction.
// Can be called from movement keys, exploration, or auto-travel.
boolean playerMoves(short direction) {
	short initialDirection = direction, i, layer;
	short x = player.xLoc, y = player.yLoc;
	short newX, newY, newestX, newestY, newDir;
	boolean playerMoved = false, alreadyRecorded = false;
	creature *defender = NULL, *tempMonst = NULL, *hitList[8] = {NULL};
	char monstName[COLS];
	char buf[COLS*3];
	const uchar directionKeys[8] = {UP_KEY, DOWN_KEY, LEFT_KEY, RIGHT_KEY, UPLEFT_KEY, DOWNLEFT_KEY, UPRIGHT_KEY, DOWNRIGHT_KEY};
	
#ifdef BROGUE_ASSERTS
	assert(direction >= 0 && direction < 8);
#endif
	
	newX = x + nbDirs[direction][0];
	newY = y + nbDirs[direction][1];
	
	if (!coordinatesAreInMap(newX, newY)) {
		return false;
	}
	
	if (player.status[STATUS_CONFUSED]) {
        // Confirmation dialog if you're moving while confused and you're next to lava and not levitating or immune to fire.
        if (player.status[STATUS_LEVITATING] <= 1
            && player.status[STATUS_IMMUNE_TO_FIRE] <= 1) {
            
            for (i=0; i<8; i++) {
                newestX = x + nbDirs[i][0];
                newestY = y + nbDirs[i][1];
                if (coordinatesAreInMap(newestX, newestY)
                    && (pmap[newestX][newestY].flags & (DISCOVERED | MAGIC_MAPPED))
                    && !diagonalBlocked(x, y, newestX, newestY)
                    && cellHasTerrainFlag(newestX, newestY, T_LAVA_INSTA_DEATH)
                    && !cellHasTerrainFlag(newestX, newestY, T_OBSTRUCTS_PASSABILITY | T_ENTANGLES)
                    && !((pmap[newestX][newestY].flags & HAS_MONSTER)
                         && canSeeMonster(monsterAtLoc(newestX, newestY))
                         && monsterAtLoc(newestX, newestY)->creatureState != MONSTER_ALLY)) {
                    
                    if (!confirm("Risk stumbling into lava?", false)) {
                        return false;
                    } else {
                        break;
                    }
                }
            }
        }
        
		direction = randValidDirectionFrom(&player, x, y, false);
		if (direction == -1) {
			return false;
		} else {
			newX = x + nbDirs[direction][0];
			newY = y + nbDirs[direction][1];
			if (!coordinatesAreInMap(newX, newY)) {
				return false;
			}
			if (!alreadyRecorded) {
				recordKeystroke(directionKeys[initialDirection], false, false);
				alreadyRecorded = true;
			}
		}
	}
	
	if (pmap[newX][newY].flags & HAS_MONSTER) {
		defender = monsterAtLoc(newX, newY);
	}
    
    // If there's no enemy at the movement location that the player is aware of, consider terrain promotions.
    if (!(defender && (canSeeMonster(defender) || monsterRevealed(defender)) && monstersAreEnemies(&player, defender))) {
        
        if (cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY) && cellHasTMFlag(newX, newY, TM_PROMOTES_ON_PLAYER_ENTRY)) {
            layer = layerWithTMFlag(newX, newY, TM_PROMOTES_ON_PLAYER_ENTRY);
            if (tileCatalog[pmap[newX][newY].layers[layer]].flags & T_OBSTRUCTS_PASSABILITY) {
                if (!alreadyRecorded) {
                    recordKeystroke(directionKeys[initialDirection], false, false);
                    alreadyRecorded = true;
                }
                message(tileCatalog[pmap[newX][newY].layers[layer]].flavorText, false);
                promoteTile(newX, newY, layer, false);
                playerTurnEnded();
                return true;
            }
        }
        
        if (player.status[STATUS_STUCK] && cellHasTerrainFlag(x, y, T_ENTANGLES)) {
            if (--player.status[STATUS_STUCK]) {
                message("you struggle but cannot free yourself.", false);
                moveEntrancedMonsters(direction);
                if (!alreadyRecorded) {
                    recordKeystroke(directionKeys[initialDirection], false, false);
                    alreadyRecorded = true;
                }
                playerTurnEnded();
                return true;
            }
            if (tileCatalog[pmap[x][y].layers[SURFACE]].flags & T_ENTANGLES) {
                pmap[x][y].layers[SURFACE] = NOTHING;
            }
        }
        
    }
	
	if (((!cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY) || (cellHasTMFlag(newX, newY, TM_PROMOTES_WITH_KEY) && keyInPackFor(newX, newY)))
         && !diagonalBlocked(x, y, newX, newY)
         && (!cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY) || (cellHasTMFlag(x, y, TM_PROMOTES_WITH_KEY) && keyInPackFor(x, y))))
		|| (defender && defender->info.flags & MONST_ATTACKABLE_THRU_WALLS)) {
		// if the move is not blocked
		
		if (defender) {
			// if there is a monster there
			
			if (defender->bookkeepingFlags & MONST_CAPTIVE) {
				monsterName(monstName, defender, false);
				sprintf(buf, "Free the captive %s?", monstName);
				if (alreadyRecorded || confirm(buf, false)) {
					if (!alreadyRecorded) {
						recordKeystroke(directionKeys[initialDirection], false, false);
						alreadyRecorded = true;
					}
					if (cellHasTMFlag(newX, newY, TM_PROMOTES_WITH_KEY) && keyInPackFor(newX, newY)) {
						useKeyAt(keyInPackFor(newX, newY), newX, newY);
					}
					freeCaptive(defender);
					player.ticksUntilTurn += player.attackSpeed;
					playerTurnEnded();
					return true;
				} else {
					return false;
				}
			}
			
			if (defender->creatureState != MONSTER_ALLY) {
				// Make a hit list of monsters the player is attacking this turn.
				// We separate this tallying phase from the actual attacking phase because sometimes the attacks themselves
				// create more monsters, and those shouldn't be attacked in the same turn.
				
				if (rogue.weapon && (rogue.weapon->flags & ITEM_ATTACKS_PENETRATE)) {
					hitList[0] = defender;
					newestX = newX + nbDirs[direction][0];
					newestY = newY + nbDirs[direction][1];
					if (coordinatesAreInMap(newestX, newestY) && (pmap[newestX][newestY].flags & HAS_MONSTER)) {
						defender = monsterAtLoc(newestX, newestY);
						if (defender
							&& monstersAreEnemies(&player, defender)
                            && defender->creatureState != MONSTER_ALLY
							&& !(defender->bookkeepingFlags & MONST_IS_DYING)
                            && (!cellHasTerrainFlag(defender->xLoc, defender->yLoc, T_OBSTRUCTS_PASSABILITY) || (defender->info.flags & MONST_ATTACKABLE_THRU_WALLS))) {
							
                            // Attack the outermost monster first, so that spears of force can potentially send both of them flying.
                            hitList[1] = hitList[0];
                            hitList[0] = defender;
						}
					}
				} else if (rogue.weapon && (rogue.weapon->flags & ITEM_ATTACKS_ALL_ADJACENT)) {
					for (i=0; i<8; i++) {
						newDir = (direction + i) % 8;
						newestX = x + cDirs[newDir][0];
						newestY = y + cDirs[newDir][1];
						if (coordinatesAreInMap(newestX, newestY) && (pmap[newestX][newestY].flags & HAS_MONSTER)) {
							defender = monsterAtLoc(newestX, newestY);
							if (defender
								&& monstersAreEnemies(&player, defender)
                                && defender->creatureState != MONSTER_ALLY
								&& !(defender->bookkeepingFlags & MONST_IS_DYING)
                                && (!cellHasTerrainFlag(defender->xLoc, defender->yLoc, T_OBSTRUCTS_PASSABILITY) || (defender->info.flags & MONST_ATTACKABLE_THRU_WALLS))) {
                                
								hitList[i] = defender;
							}
						}
					}
				} else {
					hitList[0] = defender;
				}
				
				if (abortAttackAgainstAcidicTarget(hitList)) { // Acid mound attack confirmation.
#ifdef BROGUE_ASSERTS
                    assert(!alreadyRecorded);
#endif
                    return false;
                }
                
                if (player.status[STATUS_NAUSEOUS]) {
                    if (!alreadyRecorded) {
                        recordKeystroke(directionKeys[initialDirection], false, false);
                        alreadyRecorded = true;
                    }
                    if (rand_percent(25)) {
                        vomit(&player);
                        playerTurnEnded();
                        return true;
                    }
                }
				
				// Proceeding with the attack.
				
				if (!alreadyRecorded) {
					recordKeystroke(directionKeys[initialDirection], false, false);
					alreadyRecorded = true;
				}
				
				if (rogue.weapon && (rogue.weapon->flags & ITEM_ATTACKS_SLOWLY)) {
					player.ticksUntilTurn += 2 * player.attackSpeed;
				} else if (rogue.weapon && (rogue.weapon->flags & ITEM_ATTACKS_QUICKLY)) {
					player.ticksUntilTurn += player.attackSpeed / 2;
				} else {
					player.ticksUntilTurn += player.attackSpeed;
				}
				
				// Attack!
				for (i=0; i<8; i++) {
					if (hitList[i]
						&& monstersAreEnemies(&player, hitList[i])
						&& !(hitList[i]->bookkeepingFlags & MONST_IS_DYING)) {
						
						attack(&player, hitList[i], false);
					}
				}
				
				moveEntrancedMonsters(direction);
				
				playerTurnEnded();
				return true;
			}
		}
		
		if (player.bookkeepingFlags & MONST_SEIZED) {
			for (defender = monsters->nextCreature; defender != NULL; defender = defender->nextCreature) {
				if ((defender->bookkeepingFlags & MONST_SEIZING)
					&& monstersAreEnemies(&player, defender)
					&& distanceBetween(player.xLoc, player.yLoc, defender->xLoc, defender->yLoc) == 1
					&& !player.status[STATUS_LEVITATING]
                    && !defender->status[STATUS_ENTRANCED]) {
					
                    monsterName(monstName, defender, true);
                    if (alreadyRecorded || !canSeeMonster(defender)) {
                        if (!alreadyRecorded) {
                            recordKeystroke(directionKeys[initialDirection], false, false);
                            alreadyRecorded = true;
                        }
                        sprintf(buf, "you struggle but %s is holding your legs!", monstName);
                        moveEntrancedMonsters(direction);
                        message(buf, false);
                        playerTurnEnded();
                        return true;
                    } else {
                        sprintf(buf, "you cannot move; %s is holding your legs!", monstName);
                        message(buf, false);
                        return false;
                    }
				}
			}
			player.bookkeepingFlags &= ~MONST_SEIZED; // failsafe
		}
		
		if (pmap[newX][newY].flags & (DISCOVERED | MAGIC_MAPPED)
            && player.status[STATUS_LEVITATING] <= 1
            && !player.status[STATUS_CONFUSED]
            && cellHasTerrainFlag(newX, newY, T_LAVA_INSTA_DEATH)
            && player.status[STATUS_IMMUNE_TO_FIRE] <= 1
            && !cellHasTerrainFlag(newX, newY, T_ENTANGLES)
            && !cellHasTMFlag(newX, newY, TM_IS_SECRET)) {
			message("that would be certain death!", false);
			return false; // player won't willingly step into lava
		} else if (pmap[newX][newY].flags & (DISCOVERED | MAGIC_MAPPED)
				   && player.status[STATUS_LEVITATING] <= 1
				   && !player.status[STATUS_CONFUSED]
				   && cellHasTerrainFlag(newX, newY, T_AUTO_DESCENT)
				   && !cellHasTerrainFlag(newX, newY, T_ENTANGLES)
                   && !cellHasTMFlag(newX, newY, TM_IS_SECRET)
				   && !confirm("Dive into the depths?", false)) {
			return false;
		} else if (playerCanSee(newX, newY)
				   && !player.status[STATUS_CONFUSED]
				   && !player.status[STATUS_BURNING]
				   && player.status[STATUS_IMMUNE_TO_FIRE] <= 1
				   && cellHasTerrainFlag(newX, newY, T_IS_FIRE)
				   && !cellHasTMFlag(newX, newY, TM_EXTINGUISHES_FIRE)
				   && !confirm("Venture into flame?", false)) {
			return false;
		} else if (pmap[newX][newY].flags & (ANY_KIND_OF_VISIBLE | MAGIC_MAPPED)
				   && player.status[STATUS_LEVITATING] <= 1
				   && !player.status[STATUS_CONFUSED]
				   && cellHasTerrainFlag(newX, newY, T_IS_DF_TRAP)
				   && !(pmap[newX][newY].flags & PRESSURE_PLATE_DEPRESSED)
				   && !cellHasTMFlag(newX, newY, TM_IS_SECRET)
				   && !confirm("Step onto the pressure plate?", false)) {
			return false;
		}
        
        if (rogue.weapon && (rogue.weapon->flags & ITEM_LUNGE_ATTACKS)) {
            newestX = player.xLoc + 2*nbDirs[direction][0];
            newestY = player.yLoc + 2*nbDirs[direction][1];
            if (coordinatesAreInMap(newestX, newestY) && (pmap[newestX][newestY].flags & HAS_MONSTER)) {
                tempMonst = monsterAtLoc(newestX, newestY);
                if (tempMonst
                    && canSeeMonster(tempMonst)
                    && monstersAreEnemies(&player, tempMonst)
                    && tempMonst->creatureState != MONSTER_ALLY
                    && !(tempMonst->bookkeepingFlags & MONST_IS_DYING)
                    && (!cellHasTerrainFlag(tempMonst->xLoc, tempMonst->yLoc, T_OBSTRUCTS_PASSABILITY) || (tempMonst->info.flags & MONST_ATTACKABLE_THRU_WALLS))) {
                    
                    hitList[0] = tempMonst;
                    if (abortAttackAgainstAcidicTarget(hitList)) { // Acid mound attack confirmation.
#ifdef BROGUE_ASSERTS
                        assert(!alreadyRecorded);
#endif
                        return false;
                    }
                }
            }
        }
        
        if (player.status[STATUS_NAUSEOUS]) {
            if (!alreadyRecorded) {
                recordKeystroke(directionKeys[initialDirection], false, false);
                alreadyRecorded = true;
            }
            if (rand_percent(25)) {
                vomit(&player);
                playerTurnEnded();
                return true;
            }
        }
		
		// Are we taking the stairs?
		if (rogue.downLoc[0] == newX && rogue.downLoc[1] == newY) {
			if (!alreadyRecorded) {
				recordKeystroke(directionKeys[initialDirection], false, false);
				alreadyRecorded = true;
			}
			useStairs(1);
		} else if (rogue.upLoc[0] == newX && rogue.upLoc[1] == newY) {
			if (!alreadyRecorded) {
				recordKeystroke(directionKeys[initialDirection], false, false);
				alreadyRecorded = true;
			}
			useStairs(-1);
		} else {
			// Okay, we're finally moving!
			if (!alreadyRecorded) {
				recordKeystroke(directionKeys[initialDirection], false, false);
				alreadyRecorded = true;
			}
			
			player.xLoc += nbDirs[direction][0];
			player.yLoc += nbDirs[direction][1];
			pmap[x][y].flags &= ~HAS_PLAYER;
			pmap[player.xLoc][player.yLoc].flags |= HAS_PLAYER;
			pmap[player.xLoc][player.yLoc].flags &= ~IS_IN_PATH;
            if (defender && defender->creatureState == MONSTER_ALLY) { // Swap places with ally.
				pmap[defender->xLoc][defender->yLoc].flags &= ~HAS_MONSTER;
                defender->xLoc = x;
				defender->yLoc = y;
                if (monsterAvoids(defender, x, y)) {
                    getQualifyingPathLocNear(&(defender->xLoc), &(defender->yLoc), player.xLoc, player.yLoc, true, forbiddenFlagsForMonster(&(defender->info)), 0, 0, (HAS_PLAYER | HAS_MONSTER | HAS_STAIRS), false);
                }
                //getQualifyingLocNear(loc, player.xLoc, player.yLoc, true, NULL, forbiddenFlagsForMonster(&(defender->info)) & ~(T_IS_DF_TRAP | T_IS_DEEP_WATER | T_SPONTANEOUSLY_IGNITES), HAS_MONSTER, false, false);
				//defender->xLoc = loc[0];
				//defender->yLoc = loc[1];
				pmap[defender->xLoc][defender->yLoc].flags |= HAS_MONSTER;
			}

			if (pmap[player.xLoc][player.yLoc].flags & HAS_ITEM) {
				pickUpItemAt(player.xLoc, player.yLoc);
				rogue.disturbed = true;
			}
			refreshDungeonCell(x, y);
			refreshDungeonCell(player.xLoc, player.yLoc);
			playerMoved = true;
			
			checkForMissingKeys(x, y);
            if (monsterShouldFall(&player)) {
                player.bookkeepingFlags |= MONST_IS_FALLING;
            }
			moveEntrancedMonsters(direction);
            
            // Perform a lunge attack if appropriate.
            if (hitList[0]) {
                attack(&player, hitList[0], true);
            }
			
			playerTurnEnded();
		}
	} else if (cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)) {
		i = pmap[newX][newY].layers[layerWithFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)];
		if ((tileCatalog[i].flags & T_OBSTRUCTS_PASSABILITY)
            && (!diagonalBlocked(x, y, newX, newY) || !cellHasTMFlag(newX, newY, TM_PROMOTES_WITH_KEY))) {
            
			messageWithColor(tileCatalog[i].flavorText, &backgroundMessageColor, false);
		}
	}
	return playerMoved;
}

// replaced in Dijkstra.c:
/*
// returns true if the cell value changed
boolean updateDistanceCell(short **distanceMap, short x, short y) {
	short dir, newX, newY;
	boolean somethingChanged = false;
	
	if (distanceMap[x][y] >= 0 && distanceMap[x][y] < 30000) {
		for (dir=0; dir<8; dir++) {
			newX = x + nbDirs[dir][0];
			newY = y + nbDirs[dir][1];
			if (coordinatesAreInMap(newX, newY)
				&& distanceMap[newX][newY] >= distanceMap[x][y] + 2
				&& !diagonalBlocked(x, y, newX, newY)) {
				distanceMap[newX][newY] = distanceMap[x][y] + 1;
				somethingChanged = true;
			}
		}
	}
	return somethingChanged;
}

void dijkstraScan(short **distanceMap, char passMap[DCOLS][DROWS], boolean allowDiagonals) {
	short i, j, maxDir;
	enum directions dir;
	boolean somethingChanged;
	
	maxDir = (allowDiagonals ? 8 : 4);
	
	do {
		somethingChanged = false;
		for (i=1; i<DCOLS-1; i++) {
			for (j=1; j<DROWS-1; j++) {
				if (!passMap || passMap[i][j]) {
					for (dir = 0; dir < maxDir; dir++) {
						if (coordinatesAreInMap(i + nbDirs[dir][0], j + nbDirs[dir][1])
							&& (!passMap || passMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]])
							&& distanceMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] >= distanceMap[i][j] + 2) {
							distanceMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] = distanceMap[i][j] + 1;
							somethingChanged = true;
						}
					}
				}
			}
		}
		
		
		for (i = DCOLS - 1; i >= 0; i--) {
			for (j = DROWS - 1; j >= 0; j--) {
				if (!passMap || passMap[i][j]) {
					for (dir = 0; dir < maxDir; dir++) {
						if (coordinatesAreInMap(i + nbDirs[dir][0], j + nbDirs[dir][1])
							&& (!passMap || passMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]])
							&& distanceMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] >= distanceMap[i][j] + 2) {
							distanceMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] = distanceMap[i][j] + 1;
							somethingChanged = true;
						}
					}
				}
			}
		}
	} while (somethingChanged);
}*/

/*void enqueue(short x, short y, short val, distanceQueue *dQ) {
	short *qX2, *qY2, *qVal2;
	
	// if we need to allocate more memory:
	if (dQ->qLen + 1 > dQ->qMaxLen) {
		dQ->qMaxLen *= 2;
		qX2 = realloc(dQ->qX, dQ->qMaxLen);
		if (qX2) {
			free(dQ->qX);
			dQ->qX = qX2;
		} else {
			// out of memory
		}
		qY2 = realloc(dQ->qY, dQ->qMaxLen);
		if (qY2) {
			free(dQ->qY);
			dQ->qY = qY2;
		} else {
			// out of memory
		}
		qVal2 = realloc(dQ->qVal, dQ->qMaxLen);
		if (qVal2) {
			free(dQ->qVal);
			dQ->qVal = qVal2;
		} else {
			// out of memory
		}
	}
	
	dQ->qX[dQ->qLen] = x;
	dQ->qY[dQ->qLen] = y;
	(dQ->qVal)[dQ->qLen] = val;
	
	dQ->qLen++;
	
	if (val < dQ->qMinVal) {
		dQ->qMinVal = val;
		dQ->qMinCount = 1;
	} else if (val == dQ->qMinVal) {
		dQ->qMinCount++;
	}
}

void updateQueueMinCache(distanceQueue *dQ) {
	short i;
	dQ->qMinCount = 0;
	dQ->qMinVal = 30001;
	for (i = 0; i < dQ->qLen; i++) {
		if (dQ->qVal[i] < dQ->qMinVal) {
			dQ->qMinVal = dQ->qVal[i];
			dQ->qMinCount = 1;
		} else if (dQ->qVal[i] == dQ->qMinVal) {
			dQ->qMinCount++;
		}
	}
}

// removes the lowest value from the queue, populates x/y/value variables and updates min caching
void dequeue(short *x, short *y, short *val, distanceQueue *dQ) {
	short i, minIndex;
	
	if (dQ->qMinCount <= 0) {
		updateQueueMinCache(dQ);
	}
	
	*val = dQ->qMinVal;
	
	// find the last instance of the minVal
	for (minIndex = dQ->qLen - 1; minIndex >= 0 && dQ->qVal[minIndex] != *val; minIndex--);
	
	// populate the return variables
	*x = dQ->qX[minIndex];
	*y = dQ->qY[minIndex];
	
	dQ->qLen--;
	
	// delete the minValue queue entry
	for (i = minIndex; i < dQ->qLen; i++) {
		dQ->qX[i] = dQ->qX[i+1];
		dQ->qY[i] = dQ->qY[i+1];
		dQ->qVal[i] = dQ->qVal[i+1];
	}
	
	// update min values
	dQ->qMinCount--;
	if (!dQ->qMinCount && dQ->qLen) {
		updateQueueMinCache(dQ);
	}
	
}

void dijkstraScan(short **distanceMap, char passMap[DCOLS][DROWS], boolean allowDiagonals) {
	short i, j, maxDir, val;
	enum directions dir;
	distanceQueue dQ;
	
	dQ.qMaxLen = DCOLS * DROWS * 1.5;
	dQ.qX = (short *) malloc(dQ.qMaxLen * sizeof(short));
	dQ.qY = (short *) malloc(dQ.qMaxLen * sizeof(short));
	dQ.qVal = (short *) malloc(dQ.qMaxLen * sizeof(short));
	dQ.qLen = 0;
	dQ.qMinVal = 30000;
	dQ.qMinCount = 0;
	
	maxDir = (allowDiagonals ? 8 : 4);
	
	// seed the queue with the entire map
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (!passMap || passMap[i][j]) {
				enqueue(i, j, distanceMap[i][j], &dQ);
			}
		}
	}
	
	// iterate through queue updating lowest entries until the queue is empty
	while (dQ.qLen) {
		dequeue(&i, &j, &val, &dQ);
		if (distanceMap[i][j] == val) { // if it hasn't been improved since joining the queue
			for (dir = 0; dir < maxDir; dir++) {
				if (coordinatesAreInMap(i + nbDirs[dir][0], j + nbDirs[dir][1])
					&& (!passMap || passMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]])
					&& distanceMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] >= distanceMap[i][j] + 2) {
					
					distanceMap[i + nbDirs[dir][0]][j + nbDirs[dir][1]] = distanceMap[i][j] + 1;
					
					enqueue(i + nbDirs[dir][0], j + nbDirs[dir][1], distanceMap[i][j] + 1, &dQ);
				}
			}
		}
	}
	
	free(dQ.qX);
	free(dQ.qY);
	free(dQ.qVal);
}*/

/*
void calculateDistances(short **distanceMap, short destinationX, short destinationY, unsigned long blockingTerrainFlags, creature *traveler) {
	short i, j;
	boolean somethingChanged;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			distanceMap[i][j] = ((traveler && traveler == &player && !(pmap[i][j].flags & (DISCOVERED | MAGIC_MAPPED)))
								 || ((traveler && monsterAvoids(traveler, i, j))
									 || cellHasTerrainFlag(i, j, blockingTerrainFlags))) ? -1 : 30000;
		}
	}
	
	distanceMap[destinationX][destinationY] = 0;
	
//	dijkstraScan(distanceMap);
	do {
		somethingChanged = false;
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				if (updateDistanceCell(distanceMap, i, j)) {
					somethingChanged = true;
				}
			}
		}
		
		
		for (i = DCOLS - 1; i >= 0; i--) {
			for (j = DROWS - 1; j >= 0; j--) {
				if (updateDistanceCell(distanceMap, i, j)) {
					somethingChanged = true;
				}
			}
		}
	} while (somethingChanged);
}*/

// Returns -1 if there are no beneficial moves.
// If preferDiagonals is true, we will prefer diagonal moves.
// Always rolls downhill on the distance map.
// If monst is provided, do not return a direction pointing to
// a cell that the monster avoids.
short nextStep(short **distanceMap, short x, short y, creature *monst, boolean preferDiagonals) {
	short newX, newY, bestScore;
    enum directions dir, bestDir;
    creature *blocker;
    boolean blocked;
    
#ifdef BROGUE_ASSERTS
    assert(coordinatesAreInMap(x, y));
#endif
	
	bestScore = 0;
	bestDir = NO_DIRECTION;
	
	for (dir = (preferDiagonals ? 7 : 0);
		 (preferDiagonals ? dir >= 0 : dir < 8);
		 (preferDiagonals ? dir-- : dir++)) {
		
		newX = x + nbDirs[dir][0];
		newY = y + nbDirs[dir][1];
        
#ifdef BROGUE_ASSERTS
        assert(coordinatesAreInMap(newX, newY));
#endif
        
        blocked = false;
        blocker = monsterAtLoc(newX, newY);
        if (monst
            && monsterAvoids(monst, newX, newY)) {
            
            blocked = true;
        } else if (monst
                   && blocker
                   && !canPass(monst, blocker)
                   && !monstersAreTeammates(monst, blocker)
                   && !monstersAreEnemies(monst, blocker)) {
            blocked = true;
        }
		if (coordinatesAreInMap(newX, newY)
			&& (distanceMap[x][y] - distanceMap[newX][newY]) > bestScore
            && !diagonalBlocked(x, y, newX, newY)
			&& isPassableOrSecretDoor(newX, newY)
            && !blocked) {
			
			bestDir = dir;
			bestScore = distanceMap[x][y] - distanceMap[newX][newY];
		}
	}
	return bestDir;
}

void displayRoute(short **distanceMap, boolean removeRoute) {
	short currentX = player.xLoc, currentY = player.yLoc, dir, newX, newY;
	boolean advanced;
	
	if (distanceMap[player.xLoc][player.yLoc] < 0 || distanceMap[player.xLoc][player.yLoc] == 30000) {
		return;
	}
	do {
		if (removeRoute) {
			refreshDungeonCell(currentX, currentY);
		} else {
			hiliteCell(currentX, currentY, &hiliteColor, 50, true);
		}
		advanced = false;
		for (dir = 7; dir >= 0; dir--) {
			newX = currentX + nbDirs[dir][0];
			newY = currentY + nbDirs[dir][1];
			if (coordinatesAreInMap(newX, newY)
				&& distanceMap[newX][newY] >= 0 && distanceMap[newX][newY] < distanceMap[currentX][currentY]
				&& !diagonalBlocked(currentX, currentY, newX, newY)) {
                
				currentX = newX;
				currentY = newY;
				advanced = true;
				break;
			}
		}
	} while (advanced);
}

void travelRoute(short path[1000][2], short steps) {
	short i;
	short dir;
	
	rogue.disturbed = false;
	rogue.automationActive = true;
	
	for (i=0; i < steps && !rogue.disturbed; i++) {
		for (dir = 0; dir < 8; dir++) {
			if (player.xLoc + nbDirs[dir][0] == path[i][0]
				&& player.yLoc + nbDirs[dir][1] == path[i][1]) {
				
				if (!playerMoves(dir)) {
					rogue.disturbed = true;
				}
				if (pauseBrogue(25)) {
					rogue.disturbed = true;
				}
				break;
			}
		}
	}
	rogue.disturbed = true;
	rogue.automationActive = false;
	updateFlavorText();
}

void travelMap(short **distanceMap) {
	short currentX = player.xLoc, currentY = player.yLoc, dir, newX, newY;
	boolean advanced;
	
	rogue.disturbed = false;
	rogue.automationActive = true;
	
	if (distanceMap[player.xLoc][player.yLoc] < 0 || distanceMap[player.xLoc][player.yLoc] == 30000) {
		return;
	}
	do {
		advanced = false;
		for (dir = 7; dir >= 0; dir--) {
			newX = currentX + nbDirs[dir][0];
			newY = currentY + nbDirs[dir][1];
			if (coordinatesAreInMap(newX, newY)
				&& distanceMap[newX][newY] >= 0
				&& distanceMap[newX][newY] < distanceMap[currentX][currentY]
				&& !diagonalBlocked(currentX, currentY, newX, newY)) {
				
				if (!playerMoves(dir)) {
					rogue.disturbed = true;
				}
				if (pauseBrogue(500)) {
					rogue.disturbed = true;
				}
				currentX = newX;
				currentY = newY;
				advanced = true;
				break;
			}
		}
	} while (advanced && !rogue.disturbed);
	rogue.disturbed = true;
	rogue.automationActive = false;
	updateFlavorText();
}

void travel(short x, short y, boolean autoConfirm) {
	short **distanceMap, i;
	rogueEvent theEvent;
	unsigned short staircaseConfirmKey;
	
	confirmMessages();
    
	if (D_WORMHOLING) {
		recordMouseClick(mapToWindowX(x), mapToWindowY(y), true, false);
		pmap[player.xLoc][player.yLoc].flags &= ~HAS_PLAYER;
		refreshDungeonCell(player.xLoc, player.yLoc);
		player.xLoc = x;
		player.yLoc = y;
		pmap[x][y].flags |= HAS_PLAYER;
		refreshDungeonCell(x, y);
		updateVision(true);
		return;
	}
	
	if (abs(player.xLoc - x) + abs(player.yLoc - y) == 1) {
		// targeting a cardinal neighbor
		for (i=0; i<4; i++) {
			if (nbDirs[i][0] == (x - player.xLoc) && nbDirs[i][1] == (y - player.yLoc)) {
				playerMoves(i);
				break;
			}
		}
		return;
	}
	
	if (!(pmap[x][y].flags & (DISCOVERED | MAGIC_MAPPED))) {
		message("You have not explored that location.", false);
		return;
	}
	
	distanceMap = allocGrid();
	
	calculateDistances(distanceMap, x, y, 0, &player, false, false);
	if (distanceMap[player.xLoc][player.yLoc] < 30000) {
		if (autoConfirm) {
			travelMap(distanceMap);
			//refreshSideBar(-1, -1, false);
		} else {
			if (rogue.upLoc[0] == x && rogue.upLoc[1] == y) {
				staircaseConfirmKey = ASCEND_KEY;
			} else if (rogue.downLoc[0] == x && rogue.downLoc[1] == y) {
				staircaseConfirmKey = DESCEND_KEY;
			} else {
				staircaseConfirmKey = 0;
			}
			displayRoute(distanceMap, false);
			message("Travel this route? (y/n)", false);
			
			do {
				nextBrogueEvent(&theEvent, true, false, false);
			} while (theEvent.eventType != MOUSE_UP && theEvent.eventType != KEYSTROKE);
			
			displayRoute(distanceMap, true); // clear route display
			confirmMessages();
			
			if ((theEvent.eventType == MOUSE_UP && windowToMapX(theEvent.param1) == x && windowToMapY(theEvent.param2) == y)
				|| (theEvent.eventType == KEYSTROKE && (theEvent.param1 == 'Y' || theEvent.param1 == 'y'
														|| theEvent.param1 == RETURN_KEY
														|| theEvent.param1 == ENTER_KEY
														|| (theEvent.param1 == staircaseConfirmKey
															&& theEvent.param1 != 0)))) {
				travelMap(distanceMap);
				//refreshSideBar(-1, -1, false);
			} else if (theEvent.eventType == MOUSE_UP) {
				executeMouseClick(&theEvent);
			}
		}
//		if (player.xLoc == x && player.yLoc == y) {
//			rogue.cursorLoc[0] = rogue.cursorLoc[1] = 0;
//		} else {
//			rogue.cursorLoc[0] = x;
//			rogue.cursorLoc[1] = y;
//		}
	} else {
		rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
		message("No path is available.", false);
	}
	freeGrid(distanceMap);
}

void populateGenericCostMap(short **costMap) {
    short i, j;
    
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
            if (cellHasTerrainFlag(i, j, T_OBSTRUCTS_PASSABILITY)
                && (!cellHasTMFlag(i, j, TM_IS_SECRET) || (discoveredTerrainFlagsAtLoc(i, j) & T_OBSTRUCTS_PASSABILITY))) {
                
				costMap[i][j] = cellHasTerrainFlag(i, j, T_OBSTRUCTS_DIAGONAL_MOVEMENT) ? PDS_OBSTRUCTION : PDS_FORBIDDEN;
            } else if (cellHasTerrainFlag(i, j, T_PATHING_BLOCKER & ~T_OBSTRUCTS_PASSABILITY)) {
				costMap[i][j] = PDS_FORBIDDEN;
            } else {
                costMap[i][j] = 1;
            }
        }
    }
}

void populateCreatureCostMap(short **costMap, creature *monst) {
	short i, j, unexploredCellCost;
    creature *currentTenant;
    item *theItem;
	
	unexploredCellCost = 10 + (clamp(rogue.depthLevel, 5, 15) - 5) * 2;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (monst == &player && !(pmap[i][j].flags & (DISCOVERED | MAGIC_MAPPED))) {
				costMap[i][j] = PDS_OBSTRUCTION;
                continue;
			}
            
            if (cellHasTerrainFlag(i, j, T_OBSTRUCTS_PASSABILITY)
                 && (!cellHasTMFlag(i, j, TM_IS_SECRET) || (discoveredTerrainFlagsAtLoc(i, j) & T_OBSTRUCTS_PASSABILITY) || monst == &player)) {
                
				costMap[i][j] = cellHasTerrainFlag(i, j, T_OBSTRUCTS_DIAGONAL_MOVEMENT) ? PDS_OBSTRUCTION : PDS_FORBIDDEN;
                continue;
            }
            
            if (cellHasTerrainFlag(i, j, T_LAVA_INSTA_DEATH)
                && !(monst->info.flags & (MONST_IMMUNE_TO_FIRE | MONST_FLIES))
                && (monst->status[STATUS_LEVITATING] || monst->status[STATUS_IMMUNE_TO_FIRE])
                && max(monst->status[STATUS_LEVITATING], monst->status[STATUS_IMMUNE_TO_FIRE]) < (rogue.mapToShore[i][j] + distanceBetween(i, j, monst->xLoc, monst->yLoc) * monst->movementSpeed / 100)) {
                // Only a temporary effect will permit the monster to survive the lava, and the remaining duration either isn't
                // enough to get it to the spot, or it won't suffice to let it return to shore if it does get there.
                // Treat these locations as obstacles.
				costMap[i][j] = PDS_FORBIDDEN;
                continue;
			}
            
            if ((cellHasTerrainFlag(i, j, T_AUTO_DESCENT) || cellHasTerrainFlag(i, j, T_IS_DEEP_WATER) && !(monst->info.flags & MONST_IMMUNE_TO_WATER))
                && !(monst->info.flags & MONST_FLIES)
                && (monst->status[STATUS_LEVITATING])
                && monst->status[STATUS_LEVITATING] < (rogue.mapToShore[i][j] + distanceBetween(i, j, monst->xLoc, monst->yLoc) * monst->movementSpeed / 100)) {
                // Only a temporary effect will permit the monster to levitate over the chasm/water, and the remaining duration either isn't
                // enough to get it to the spot, or it won't suffice to let it return to shore if it does get there.
                // Treat these locations as obstacles.
				costMap[i][j] = PDS_FORBIDDEN;
                continue;
			}
            
            if (monsterAvoids(monst, i, j)) {
				costMap[i][j] = PDS_FORBIDDEN;
                continue;
			}
            
            if (pmap[i][j].flags & HAS_MONSTER) {
                currentTenant = monsterAtLoc(i, j);
                if ((currentTenant->info.flags & MONST_IMMUNE_TO_WEAPONS) && !canPass(monst, currentTenant)) {
                    costMap[i][j] = PDS_FORBIDDEN;
                    continue;
                }
			} 
            
            if ((pmap[i][j].flags & KNOWN_TO_BE_TRAP_FREE)
                || (monst != &player && monst->creatureState != MONSTER_ALLY)) {
                
                costMap[i][j] = 10;
            } else {
                // Player and allies give locations that are known to be free of traps
                // an advantage that increases with depth level, based on the depths
                // at which traps are generated.
                costMap[i][j] = unexploredCellCost;
            }
            
            if (cellHasTerrainFlag(i, j, T_CAUSES_NAUSEA)
                || cellHasTMFlag(i, j, TM_PROMOTES_ON_ITEM_PICKUP)
                || cellHasTerrainFlag(i, j, T_ENTANGLES) && !(monst->info.flags & MONST_IMMUNE_TO_WEBS)) {
                
                costMap[i][j] += 20;
            }
            
            if (monst == &player) {
                theItem = itemAtLoc(i, j);
                if (theItem && (theItem->flags & ITEM_PLAYER_AVOIDS)) {
                    costMap[i][j] += 10;
                }
            }
		}
	}
}

#define exploreGoalValue(x, y)	(0 - abs((x) - DCOLS / 2) / 3 - abs((x) - DCOLS / 2) / 4)

void getExploreMap(short **map, boolean headingToStairs) {// calculate explore map
	short i, j;
	short **costMap;
	item *theItem;
	
	costMap = allocGrid();
	populateCreatureCostMap(costMap, &player);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			
			map[i][j] = 30000; // Can be overridden later.
			
            theItem = itemAtLoc(i, j);
			
			if (!(pmap[i][j].flags & DISCOVERED)) {
				costMap[i][j] = 1;
				map[i][j] = exploreGoalValue(i, j);
			} else if (theItem
					   && !monsterAvoids(&player, i, j)) {
				if (theItem->flags & ITEM_PLAYER_AVOIDS) {
					costMap[i][j] = 20;
				} else {
					costMap[i][j] = 1;
					map[i][j] = exploreGoalValue(i, j) - 100;
				}
			}
		}
	}
	
	costMap[rogue.downLoc[0]][rogue.downLoc[1]]	= 100;
	costMap[rogue.upLoc[0]][rogue.upLoc[1]]		= 100;
	
	if (headingToStairs) {
		map[rogue.downLoc[0]][rogue.downLoc[1]] = 0; // head to the stairs
	}
	
	dijkstraScan(map, costMap, true);
	
	//displayGrid(costMap);
	freeGrid(costMap);
}

boolean explore(short frameDelay) {
	short **distanceMap, newX, newY;
	boolean madeProgress, headingToStairs, foundDownStairs, foundUpStairs;
	enum directions dir;
	creature *monst;
	
	clearCursorPath();
	
	madeProgress	= false;
	headingToStairs	= false;
	foundDownStairs	= (pmap[rogue.downLoc[0]][rogue.downLoc[1]].flags & (DISCOVERED | MAGIC_MAPPED)) ? true : false;
	foundUpStairs	= (pmap[rogue.upLoc[0]][rogue.upLoc[1]].flags & (DISCOVERED | MAGIC_MAPPED)) ? true : false;
	
	if (player.status[STATUS_CONFUSED]) {
		message("Not while you're confused.", false);
		return false;
	}
	
	// fight any adjacent enemies
	for (dir = 0; dir < 8; dir++) {
		monst = monsterAtLoc(player.xLoc + nbDirs[dir][0], player.yLoc + nbDirs[dir][1]);
		if (monst && canSeeMonster(monst) && monstersAreEnemies(&player, monst) && !(monst->info.flags & MONST_IMMUNE_TO_WEAPONS)) {
			startFighting(dir, (player.status[STATUS_HALLUCINATING] ? true : false));
			if (rogue.disturbed) {
				return true;
			}
		}
	}
	
	if (!rogue.autoPlayingLevel) {
		message("自动探索中... 按任意键停止。", false);
		// A little hack so the exploring message remains bright while exploring and then auto-dims when
		// another message is displayed:
		confirmMessages();
	}
	rogue.disturbed = false;
	rogue.automationActive = true;
	
	distanceMap = allocGrid();
	do {
		// fight any adjacent enemies
		for (dir = 0; dir < 8 && !rogue.disturbed; dir++) {
            newX = player.xLoc + nbDirs[dir][0];
            newY = player.yLoc + nbDirs[dir][1];
            if (coordinatesAreInMap(newX, newY)) {
                monst = monsterAtLoc(newX, newY);
                if (monst
                    && (!diagonalBlocked(player.xLoc, player.yLoc, newX, newY) || (monst->info.flags & MONST_ATTACKABLE_THRU_WALLS))
                    && canSeeMonster(monst)
                    && monstersAreEnemies(&player, monst)
                    && !(monst->info.flags & MONST_IMMUNE_TO_WEAPONS)) {
                    
                    startFighting(dir, (player.status[STATUS_HALLUCINATING] ? true : false));
                    if (rogue.disturbed) {
                        madeProgress = true;
                        continue;
                    }
                }
            }
		}
        
        if (rogue.disturbed) {
            continue;
        }
		
		getExploreMap(distanceMap, headingToStairs);
		
		// take a step
		dir = nextStep(distanceMap, player.xLoc, player.yLoc, NULL, false);
		
		if (!headingToStairs && rogue.autoPlayingLevel && dir == NO_DIRECTION) {
			headingToStairs = true;
			continue;
		}
		
		refreshSideBar(-1, -1, false);
		
		if (dir == NO_DIRECTION) {
			message("I see no path for further exploration.", false);
			rogue.disturbed = true;
		} else if (!playerMoves(dir)) {
			rogue.disturbed = true;
		} else if (!foundDownStairs && (pmap[rogue.downLoc[0]][rogue.downLoc[1]].flags & (DISCOVERED | MAGIC_MAPPED))) {
            message("you see the stairs down.", false);
			foundDownStairs = true;
			madeProgress = true;
		} else if (!foundUpStairs && (pmap[rogue.upLoc[0]][rogue.upLoc[1]].flags & (DISCOVERED | MAGIC_MAPPED))) {
			message("you see the stairs up.", false);
			foundUpStairs = true;
			madeProgress = true;
		} else {
			madeProgress = true;
			if (pauseBrogue(frameDelay)) {
				rogue.disturbed = true;
				rogue.autoPlayingLevel = false;
			}
		}
	} while (!rogue.disturbed);
	rogue.automationActive = false;
	refreshSideBar(-1, -1, false);
	freeGrid(distanceMap);
	return madeProgress;
}

void autoPlayLevel(boolean fastForward) {
	boolean madeProgress;
	
	rogue.autoPlayingLevel = true;
	
	confirmMessages();
	message("Playing... press any key to stop.", false);
	
	// explore until we are not making progress
	do {
		madeProgress = explore(fastForward ? 1 : 50);
		//refreshSideBar(-1, -1, false);
		
		if (!madeProgress && rogue.downLoc[0] == player.xLoc && rogue.downLoc[1] == player.yLoc) {
			useStairs(1);
			madeProgress = true;
		}
	} while (madeProgress && rogue.autoPlayingLevel);
	
	confirmMessages();
	
	rogue.autoPlayingLevel = false;
}

void updateClairvoyance() {
	short i, j, clairvoyanceRadius, dx, dy;
	boolean cursed;
	unsigned long cFlags;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			
			pmap[i][j].flags &= ~WAS_CLAIRVOYANT_VISIBLE;
			
			if (pmap[i][j].flags & CLAIRVOYANT_VISIBLE) {
				pmap[i][j].flags |= WAS_CLAIRVOYANT_VISIBLE;
			}
			
			pmap[i][j].flags &= ~(CLAIRVOYANT_VISIBLE | CLAIRVOYANT_DARKENED);
		}
	}
	
	cursed = (rogue.clairvoyance < 0);
	if (cursed) {
		clairvoyanceRadius = (rogue.clairvoyance - 1) * -1;
		cFlags = CLAIRVOYANT_DARKENED;
	} else {
		clairvoyanceRadius = (rogue.clairvoyance > 0) ? rogue.clairvoyance + 1 : 0;
		cFlags = CLAIRVOYANT_VISIBLE | DISCOVERED;
	}
	
	for (i = max(0, player.xLoc - clairvoyanceRadius); i < min(DCOLS, player.xLoc + clairvoyanceRadius + 1); i++) {
		for (j = max(0, player.yLoc - clairvoyanceRadius); j < min(DROWS, player.yLoc + clairvoyanceRadius + 1); j++) {
			
			dx = (player.xLoc - i);
			dy = (player.yLoc - j);
			
			if (dx*dx + dy*dy < clairvoyanceRadius*clairvoyanceRadius + clairvoyanceRadius
				&& (pmap[i][j].layers[DUNGEON] != GRANITE || pmap[i][j].flags & DISCOVERED)) {
				if ((cFlags & ~pmap[i][j].flags & DISCOVERED)
					&& !cellHasTerrainFlag(i, j, T_PATHING_BLOCKER)) {
					rogue.xpxpThisTurn++;
				}
				pmap[i][j].flags |= cFlags;
				if (!(pmap[i][j].flags & HAS_PLAYER) && !cursed) {
					pmap[i][j].flags &= ~STABLE_MEMORY;
				}
			}
		}
	}
}

void updateTelepathy() {
	short i, j;
	creature *monst;
	boolean grid[DCOLS][DROWS];
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			pmap[i][j].flags &= ~WAS_TELEPATHIC_VISIBLE;
			if (pmap[i][j].flags & TELEPATHIC_VISIBLE) {
				pmap[i][j].flags |= WAS_TELEPATHIC_VISIBLE;
			}
			pmap[i][j].flags &= ~(TELEPATHIC_VISIBLE);
		}
	}
	
    zeroOutGrid(grid);
    for (monst = monsters->nextCreature; monst; monst = monst->nextCreature) {
        if (monsterRevealed(monst)) {
            getFOVMask(grid, monst->xLoc, monst->yLoc, 1.5, T_OBSTRUCTS_VISION, 0, false);
            pmap[monst->xLoc][monst->yLoc].flags |= (TELEPATHIC_VISIBLE | DISCOVERED);
        }
    }
    for (monst = dormantMonsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
        if (monsterRevealed(monst)) {
            getFOVMask(grid, monst->xLoc, monst->yLoc, 1.5, T_OBSTRUCTS_VISION, 0, false);
            pmap[monst->xLoc][monst->yLoc].flags |= (TELEPATHIC_VISIBLE | DISCOVERED);
        }
    }
    for (i = 0; i < DCOLS; i++) {
        for (j = 0; j < DROWS; j++) {
            if (grid[i][j]) {
                pmap[i][j].flags |= (TELEPATHIC_VISIBLE | DISCOVERED);
            }
        }
    }
}

void updateScent() {
	short i, j;
	char grid[DCOLS][DROWS];
	
	zeroOutGrid(grid);
	
	getFOVMask(grid, player.xLoc, player.yLoc, DCOLS, T_OBSTRUCTS_SCENT, 0, false);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (grid[i][j]) {
				if (abs(player.xLoc - i) > abs(player.yLoc - j)) {
					addScentToCell(i, j, 2 * abs(player.xLoc - i) + abs(player.yLoc - j));
				} else {
					addScentToCell(i, j, abs(player.xLoc - i) + 2 * abs(player.yLoc - j));
				}
			}
		}
	}
	addScentToCell(player.xLoc, player.yLoc, 0);
}

void demoteVisibility() {
	short i, j;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			pmap[i][j].flags &= ~WAS_VISIBLE;
			if (pmap[i][j].flags & VISIBLE) {
				pmap[i][j].flags &= ~VISIBLE;
				pmap[i][j].flags |= WAS_VISIBLE;
			}
		}
	}
}

void updateVision(boolean refreshDisplay) {
	short i, j;
	char grid[DCOLS][DROWS];
	item *theItem;
	creature *monst;
	
    demoteVisibility();
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			pmap[i][j].flags &= ~IN_FIELD_OF_VIEW;
		}
	}
	
	// Calculate player's field of view (distinct from what is visible, as lighting hasn't been done yet).
	zeroOutGrid(grid);
	getFOVMask(grid, player.xLoc, player.yLoc, DCOLS + DROWS, (T_OBSTRUCTS_VISION), 0, false);
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (grid[i][j]) {
				pmap[i][j].flags |= IN_FIELD_OF_VIEW;
			}
		}
	}
	pmap[player.xLoc][player.yLoc].flags |= IN_FIELD_OF_VIEW | VISIBLE;
	
	if (rogue.clairvoyance < 0) {
		pmap[player.xLoc][player.yLoc].flags |= DISCOVERED;
		pmap[player.xLoc][player.yLoc].flags &= ~STABLE_MEMORY;
	}
	
	if (rogue.clairvoyance != 0) {
		updateClairvoyance();
	}
	
    updateTelepathy();
	updateLighting();
	updateFieldOfViewDisplay(true, refreshDisplay);
	displayLevel();
	
//	for (i=0; i<DCOLS; i++) {
//		for (j=0; j<DROWS; j++) {
//			if (pmap[i][j].flags & VISIBLE) {
//				plotCharWithColor(' ', mapToWindowX(i), mapToWindowY(j), &yellow, &yellow);
//			} else if (pmap[i][j].flags & IN_FIELD_OF_VIEW) {
//				plotCharWithColor(' ', mapToWindowX(i), mapToWindowY(j), &blue, &blue);
//			}
//		}
//	}
//	displayMoreSign();
	
	if (player.status[STATUS_HALLUCINATING] > 0) {
		for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
			if ((pmap[theItem->xLoc][theItem->yLoc].flags & DISCOVERED) && refreshDisplay) {
				refreshDungeonCell(theItem->xLoc, theItem->yLoc);
			}
		}
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if ((pmap[monst->xLoc][monst->yLoc].flags & DISCOVERED) && refreshDisplay) {
				refreshDungeonCell(monst->xLoc, monst->yLoc);
			}
		}
	}
}

void checkNutrition() {
	item *theItem;
	char buf[DCOLS*3], foodWarning[DCOLS*3];
	
	if (numberOfMatchingPackItems(FOOD, 0, 0, false) == 0) {
		sprintf(foodWarning, " and have no food");
	} else {
		foodWarning[0] = '\0';
	}
	
	if (player.status[STATUS_NUTRITION] == HUNGER_THRESHOLD) {
        player.status[STATUS_NUTRITION]--;
		sprintf(buf, "you are hungry%s.", foodWarning);
		message(buf, false);
	} else if (player.status[STATUS_NUTRITION] == WEAK_THRESHOLD) {
        player.status[STATUS_NUTRITION]--;
		sprintf(buf, "you feel weak with hunger%s.", foodWarning);
		message(buf, true);
	} else if (player.status[STATUS_NUTRITION] == FAINT_THRESHOLD) {
        player.status[STATUS_NUTRITION]--;
		sprintf(buf, "you feel faint with hunger%s.", foodWarning);
		message(buf, true);
	} else if (player.status[STATUS_NUTRITION] <= 1) {
        // Force the player to eat something if he has it
        for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
            if (theItem->category == FOOD) {
                sprintf(buf, "unable to control your hunger, you eat a %s.", (theItem->kind == FRUIT ? "mango" : "ration of food"));
                messageWithColor(buf, &itemMessageColor, true);
                apply(theItem, false);
                break;
            }
		}
	}
	
	if (player.status[STATUS_NUTRITION] == 1) {	// Didn't manage to eat any food above.
		player.status[STATUS_NUTRITION] = 0;	// So the status bar changes in time for the message:
		message("you are starving to death!", true);
	}
}

void burnItem(item *theItem) {
	short x, y;
	char buf1[DCOLS], buf2[DCOLS];
	itemName(theItem, buf1, false, true, NULL);
	sprintf(buf2, "%s burns up!", buf1);
	
	x = theItem->xLoc;
	y = theItem->yLoc;
	
	removeItemFromChain(theItem, floorItems);
	
	pmap[x][y].flags &= ~(HAS_ITEM | ITEM_DETECTED);
	
	if (pmap[x][y].flags & (ANY_KIND_OF_VISIBLE | DISCOVERED | ITEM_DETECTED)) {	
		refreshDungeonCell(x, y);
	}
	
	if (playerCanSee(x, y)) {
		messageWithColor(buf2, &itemMessageColor, false);
	}
    
    spawnDungeonFeature(x, y, &(dungeonFeatureCatalog[DF_ITEM_FIRE]), true, false);
}

void activateMachine(short machineNumber) {
	short i, j, x, y, layer, sRows[DROWS], sCols[DCOLS], monsterCount, maxMonsters;
    creature **activatedMonsterList, *monst;
	
    fillSequentialList(sCols, DCOLS);
    shuffleList(sCols, DCOLS);
    fillSequentialList(sRows, DROWS);
    shuffleList(sRows, DROWS);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			x = sCols[i];
			y = sRows[j];
			if ((pmap[x][y].flags & IS_IN_MACHINE)
				&& pmap[x][y].machineNumber == machineNumber
				&& !(pmap[x][y].flags & IS_POWERED)
				&& cellHasTMFlag(x, y, TM_IS_WIRED)) {
				
				pmap[x][y].flags |= IS_POWERED;
				for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
					if (tileCatalog[pmap[x][y].layers[layer]].mechFlags & TM_IS_WIRED) {
						promoteTile(x, y, layer, false);
					}
				}
			}
		}
	}
    
    monsterCount = maxMonsters = 0;
    activatedMonsterList = NULL;
    for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
        if (monst->machineHome == machineNumber
            && monst->spawnDepth == rogue.depthLevel
            && (monst->info.flags & MONST_GETS_TURN_ON_ACTIVATION)) {
            
            monsterCount++;
            
            if (monsterCount > maxMonsters) {
                maxMonsters += 10;
                activatedMonsterList = realloc(activatedMonsterList, sizeof(creature *) * maxMonsters);
            }
            activatedMonsterList[monsterCount - 1] = monst;
        }
    }
    for (i=0; i<monsterCount; i++) {
        if (!(activatedMonsterList[i]->bookkeepingFlags & MONST_IS_DYING)) {
            monstersTurn(activatedMonsterList[i]);
        }
    }
    
    if (activatedMonsterList) {
        free(activatedMonsterList);
    }
}

boolean circuitBreakersPreventActivation(short machineNumber) {
    short i, j;
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
            if (pmap[i][j].machineNumber == machineNumber
                && cellHasTMFlag(i, j, TM_IS_CIRCUIT_BREAKER)) {
                
                return true;
            }
        }
    }
    return false;
}

void promoteTile(short x, short y, enum dungeonLayers layer, boolean useFireDF) {
	short i, j;
	enum dungeonFeatureTypes DFType;
	floorTileType *tile;
	
	tile = &(tileCatalog[pmap[x][y].layers[layer]]);
	
	DFType = (useFireDF ? tile->fireType : tile->promoteType);
	
	if ((tile->mechFlags & TM_VANISHES_UPON_PROMOTION)) {
		if (tileCatalog[pmap[x][y].layers[layer]].flags & T_PATHING_BLOCKER) {
			rogue.staleLoopMap = true;
		}
		pmap[x][y].layers[layer] = (layer == DUNGEON ? FLOOR : NOTHING); // even the dungeon layer implicitly has floor underneath it
		if (layer == GAS) {
			pmap[x][y].volume = 0;
		}
		refreshDungeonCell(x, y);
	}
	if (DFType) {
		spawnDungeonFeature(x, y, &dungeonFeatureCatalog[DFType], true, false);
	}
	
	if (!useFireDF && (tile->mechFlags & TM_IS_WIRED)
        && !(pmap[x][y].flags & IS_POWERED)
        && !circuitBreakersPreventActivation(pmap[x][y].machineNumber)) {
		// Send power through all cells in the same machine that are not IS_POWERED,
		// and on any such cell, promote each terrain layer that is T_IS_WIRED.
		// Note that machines need not be contiguous.
		pmap[x][y].flags |= IS_POWERED;
		activateMachine(pmap[x][y].machineNumber); // It lives!!!
		
		// Power fades from the map immediately after we finish.
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				pmap[i][j].flags &= ~IS_POWERED;
			}
		}
	}
}

boolean exposeTileToFire(short x, short y, boolean alwaysIgnite) {
	enum dungeonLayers layer;
	short ignitionChance = 0, bestExtinguishingPriority = 1000, explosiveNeighborCount = 0;
    short newX, newY;
    enum directions dir;
	boolean fireIgnited = false, explosivePromotion = false;
	
	if (!cellHasTerrainFlag(x, y, T_IS_FLAMMABLE)) {
		return false;
	}
	
	// Pick the extinguishing layer with the best priority.
	for (layer=0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
		if ((tileCatalog[pmap[x][y].layers[layer]].mechFlags & TM_EXTINGUISHES_FIRE)
			&& tileCatalog[pmap[x][y].layers[layer]].drawPriority < bestExtinguishingPriority) {
			bestExtinguishingPriority = tileCatalog[pmap[x][y].layers[layer]].drawPriority;
		}
	}
	
	// Pick the fire type of the most flammable layer that is either gas or equal-or-better priority than the best extinguishing layer.
	for (layer=0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
		if ((tileCatalog[pmap[x][y].layers[layer]].flags & T_IS_FLAMMABLE)
			&& (layer == GAS || tileCatalog[pmap[x][y].layers[layer]].drawPriority <= bestExtinguishingPriority)
			&& tileCatalog[pmap[x][y].layers[layer]].chanceToIgnite > ignitionChance) {
			ignitionChance = tileCatalog[pmap[x][y].layers[layer]].chanceToIgnite;
		}
	}
	
	if (alwaysIgnite || (ignitionChance && rand_percent(ignitionChance))) {	// If it ignites...
		fireIgnited = true;
        
        // Count explosive neighbors.
        if (cellHasTMFlag(x, y, TM_EXPLOSIVE_PROMOTE)) {
            for (dir = 0, explosiveNeighborCount = 0; dir < 8; dir++) {
                newX = x + nbDirs[dir][0];
                newY = y + nbDirs[dir][1];
                if (coordinatesAreInMap(newX, newY)
                    && (cellHasTerrainFlag(newX, newY, T_IS_FIRE | T_OBSTRUCTS_GAS) || cellHasTMFlag(newX, newY, TM_EXPLOSIVE_PROMOTE))) {
                    
                    explosiveNeighborCount++;
                }
            }
            if (explosiveNeighborCount >= 8) {
                explosivePromotion = true;
            }
        }
		
		// Flammable layers are consumed.
		for (layer=0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[x][y].layers[layer]].flags & T_IS_FLAMMABLE) {
				if (layer == GAS) {
					pmap[x][y].volume = 0; // Flammable gas burns its volume away.
				}
				promoteTile(x, y, layer, !explosivePromotion);
			}
		}
		refreshDungeonCell(x, y);
	}
	return fireIgnited;
}

// Only the gas layer can be volumetric.
void updateVolumetricMedia() {
	short i, j, newX, newY, numSpaces;
	unsigned long highestNeighborVolume;
	unsigned long sum;
	enum tileType gasType;
	// assume gas layer
	//	enum dungeonLayers layer;
	enum directions dir;
	unsigned short newGasVolume[DCOLS][DROWS];
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			newGasVolume[i][j] = 0;
		}
	}
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (!cellHasTerrainFlag(i, j, T_OBSTRUCTS_GAS)) {
				sum = pmap[i][j].volume;
				numSpaces = 1;
				highestNeighborVolume = pmap[i][j].volume;
				gasType = pmap[i][j].layers[GAS];
				for (dir=0; dir<8; dir++) {
					newX = i + nbDirs[dir][0];
					newY = j + nbDirs[dir][1];
					if (coordinatesAreInMap(newX, newY)
						&& !cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_GAS)) {
						
						sum += pmap[newX][newY].volume;
						numSpaces++;
						if (pmap[newX][newY].volume > highestNeighborVolume) {
							highestNeighborVolume = pmap[newX][newY].volume;
							gasType = pmap[newX][newY].layers[GAS];
						}
					}
				}
				if (cellHasTerrainFlag(i, j, T_AUTO_DESCENT)) { // if it's a chasm tile or trap door
					numSpaces++; // this will allow gas to escape from the level entirely
				}
				newGasVolume[i][j] += sum / max(1, numSpaces);
				if ((unsigned) rand_range(0, numSpaces - 1) < (sum % numSpaces)) {
					newGasVolume[i][j]++; // stochastic rounding
				}
				if (pmap[i][j].layers[GAS] != gasType && newGasVolume[i][j] > 3) {
					if (pmap[i][j].layers[GAS] != NOTHING) {
						newGasVolume[i][j] = min(3, newGasVolume[i][j]); // otherwise interactions between gases are crazy
					}
					pmap[i][j].layers[GAS] = gasType;
				} else if (pmap[i][j].layers[GAS] && newGasVolume[i][j] < 1) {
					pmap[i][j].layers[GAS] = NOTHING;
					refreshDungeonCell(i, j);
				}
				if (pmap[i][j].volume > 0) {
					if (tileCatalog[pmap[i][j].layers[GAS]].mechFlags & TM_GAS_DISSIPATES_QUICKLY) {
						newGasVolume[i][j] -= (rand_percent(50) ? 1 : 0);
					} else if (tileCatalog[pmap[i][j].layers[GAS]].mechFlags & TM_GAS_DISSIPATES) {
						newGasVolume[i][j] -= (rand_percent(20) ? 1 : 0);
					}
				}
			} else if (pmap[i][j].volume > 0) { // if has gas but can't hold gas
				// disperse gas instantly into neighboring tiles that can hold gas
				numSpaces = 0;
				for (dir = 0; dir < 8; dir++) {
					newX = i + nbDirs[dir][0];
					newY = j + nbDirs[dir][1];
					if (coordinatesAreInMap(newX, newY)
						&& !cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_GAS)) {
						
						numSpaces++;
					}
				}
				if (numSpaces > 0) {
					for (dir = 0; dir < 8; dir++) {
						newX = i + nbDirs[dir][0];
						newY = j + nbDirs[dir][1];
						if (coordinatesAreInMap(newX, newY)
							&& !cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_GAS)) {
							
							newGasVolume[newX][newY] += (pmap[i][j].volume / numSpaces);
							if (pmap[i][j].volume / numSpaces) {
								pmap[newX][newY].layers[GAS] = pmap[i][j].layers[GAS];
							}
						}
					}
				}
				newGasVolume[i][j] = 0;
				pmap[i][j].layers[GAS] = NOTHING;
			}
		}
	}
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (pmap[i][j].volume != newGasVolume[i][j]) {
				pmap[i][j].volume = newGasVolume[i][j];
				refreshDungeonCell(i, j);
			}
		}
	}
}

// Monsters who are over chasms or other descent tiles won't fall until this is called.
// This is to avoid having the monster chain change unpredictably in the middle of a turn.
void monstersFall() {
	creature *monst, *previousCreature, *nextCreature;
	short x, y;
	char buf[DCOLS], buf2[DCOLS];
	
	// monsters plunge into chasms at the end of the turn
	for (monst = monsters->nextCreature; monst != NULL; monst = nextCreature) {
		nextCreature = monst->nextCreature;
		if ((monst->bookkeepingFlags & MONST_IS_FALLING) || monsterShouldFall(monst)) {
			x = monst->xLoc;
			y = monst->yLoc;
			
			if (canSeeMonster(monst)) {
				monsterName(buf, monst, true);
				sprintf(buf2, "%s plunges out of sight!", buf);
				messageWithColor(buf2, messageColorFromVictim(monst), false);
			}
			monst->bookkeepingFlags |= MONST_PREPLACED;
			monst->bookkeepingFlags &= ~(MONST_IS_FALLING | MONST_SEIZED | MONST_SEIZING);
            if (monst->info.flags & MONST_GETS_TURN_ON_ACTIVATION) {
                // Guardians and mirrored totems never survive the fall. If they did, they might block the level below.
                killCreature(monst, false);
            } else if (!inflictDamage(monst, randClumpedRange(6, 12, 2), &red)) {
				demoteMonsterFromLeadership(monst);
				
				// remove from monster chain
				for (previousCreature = monsters;
					 previousCreature->nextCreature != monst;
					 previousCreature = previousCreature->nextCreature);
				previousCreature->nextCreature = monst->nextCreature;
				
				// add to next level's chain
				monst->nextCreature = levels[rogue.depthLevel-1 + 1].monsters;
				levels[rogue.depthLevel-1 + 1].monsters = monst;
				
				monst->depth = rogue.depthLevel + 1;
			}
			
			pmap[x][y].flags &= ~HAS_MONSTER;
			refreshDungeonCell(x, y);
		}
	}
}

void updateEnvironment() {
	short i, j, direction, newX, newY, promoteChance, promotions[DCOLS][DROWS];
	enum dungeonLayers layer;
	floorTileType *tile;
	boolean isVolumetricGas = false;
	
	monstersFall();
	
	// update gases twice
	for (i=0; i<DCOLS && !isVolumetricGas; i++) {
		for (j=0; j<DROWS && !isVolumetricGas; j++) {
			if (!isVolumetricGas && pmap[i][j].layers[GAS]) {
				isVolumetricGas = true;
			}
		}
	}
	if (isVolumetricGas) {
		updateVolumetricMedia();
		updateVolumetricMedia();
	}
	
	// Do random tile promotions in two passes to keep generations distinct.
	// First pass, make a note of each terrain layer at each coordinate that is going to promote:
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			promotions[i][j] = 0;
			for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
				tile = &(tileCatalog[pmap[i][j].layers[layer]]);
				if (tile->promoteChance < 0) {
					promoteChance = 0;
					for (direction = 0; direction < 4; direction++) {
						if (coordinatesAreInMap(i + nbDirs[direction][0], j + nbDirs[direction][1])
							&& !cellHasTerrainFlag(i + nbDirs[direction][0], j + nbDirs[direction][1], T_OBSTRUCTS_PASSABILITY)
							&& pmap[i + nbDirs[direction][0]][j + nbDirs[direction][1]].layers[layer] != pmap[i][j].layers[layer]
							&& !(pmap[i][j].flags & CAUGHT_FIRE_THIS_TURN)) {
							promoteChance += -1 * tile->promoteChance;
						}
					}
				} else {
					promoteChance = tile->promoteChance;
				}
				if (promoteChance
					&& !(pmap[i][j].flags & CAUGHT_FIRE_THIS_TURN)
					&& rand_range(0, 10000) < promoteChance) {
					promotions[i][j] |= Fl(layer);
					//promoteTile(i, j, layer, false);
				}
			}
		}
	}
	// Second pass, do the promotions:
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
				if ((promotions[i][j] & Fl(layer))) {
					//&& (tileCatalog[pmap[i][j].layers[layer]].promoteChance != 0)){
					// make sure that it's still a promotable layer
					promoteTile(i, j, layer, false);
				}
			}
		}
	}
	
	// Bookkeeping for fire, pressure plates and key-activated tiles.
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			pmap[i][j].flags &= ~(CAUGHT_FIRE_THIS_TURN);
			if (!(pmap[i][j].flags & (HAS_PLAYER | HAS_MONSTER | HAS_ITEM)) && pmap[i][j].flags & PRESSURE_PLATE_DEPRESSED) {
				pmap[i][j].flags &= ~PRESSURE_PLATE_DEPRESSED;
			}
			if (cellHasTMFlag(i, j, TM_PROMOTES_WITHOUT_KEY) && !keyOnTileAt(i, j)) {
				for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
					if (tileCatalog[pmap[i][j].layers[layer]].mechFlags & TM_PROMOTES_WITHOUT_KEY) {
						promoteTile(i, j, layer, false);
					}
				}
			}
		}
	}
	
	// Update fire.
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (cellHasTerrainFlag(i, j, T_IS_FIRE) && !(pmap[i][j].flags & CAUGHT_FIRE_THIS_TURN)) {
				exposeTileToFire(i, j, false);
				for (direction=0; direction<4; direction++) {
					newX = i + nbDirs[direction][0];
					newY = j + nbDirs[direction][1];
					if (coordinatesAreInMap(newX, newY)) {
						exposeTileToFire(newX, newY, false);
					}
				}
			}
		}
	}
	
    // Terrain that affects items
    updateFloorItems();
}

void updateAllySafetyMap() {
	short i, j;
	short **playerCostMap, **monsterCostMap;
	
	rogue.updatedAllySafetyMapThisTurn = true;
	
	playerCostMap = allocGrid();
	monsterCostMap = allocGrid();
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			allySafetyMap[i][j] = 30000;
			
			playerCostMap[i][j] = monsterCostMap[i][j] = 1;
			
			if (cellHasTerrainFlag(i, j, T_OBSTRUCTS_PASSABILITY)
                && (!cellHasTMFlag(i, j, TM_IS_SECRET) || (discoveredTerrainFlagsAtLoc(i, j) & T_OBSTRUCTS_PASSABILITY))) {
                
				playerCostMap[i][j] = monsterCostMap[i][j] = cellHasTerrainFlag(i, j, T_OBSTRUCTS_DIAGONAL_MOVEMENT) ? PDS_OBSTRUCTION : PDS_FORBIDDEN;
			} else if (cellHasTerrainFlag(i, j, T_PATHING_BLOCKER & ~T_OBSTRUCTS_PASSABILITY)) {
				playerCostMap[i][j] = monsterCostMap[i][j] = PDS_FORBIDDEN;
			} else if ((pmap[i][j].flags & HAS_MONSTER) && monstersAreEnemies(&player, monsterAtLoc(i, j))) {
				playerCostMap[i][j] = 1;
				monsterCostMap[i][j] = PDS_FORBIDDEN;
				allySafetyMap[i][j] = 0;
			}
		}
	}
	
	playerCostMap[player.xLoc][player.yLoc] = PDS_FORBIDDEN;
	monsterCostMap[player.xLoc][player.yLoc] = PDS_FORBIDDEN;
	
	dijkstraScan(allySafetyMap, playerCostMap, false);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (monsterCostMap[i][j] < 0) {
				continue;
			}
			
			if (allySafetyMap[i][j] == 30000) {
				allySafetyMap[i][j] = 150;
			}
			
			allySafetyMap[i][j] = 50 * allySafetyMap[i][j] / (50 + allySafetyMap[i][j]);
			
			allySafetyMap[i][j] *= -3;
			
			if (pmap[i][j].flags & IN_LOOP) {
				allySafetyMap[i][j] -= 10;
			}
		}
	}
	dijkstraScan(allySafetyMap, monsterCostMap, false);
	
	freeGrid(playerCostMap);
	freeGrid(monsterCostMap);
}

void updateSafetyMap() {
	short i, j;
	short **playerCostMap, **monsterCostMap;
	creature *monst;
	
	rogue.updatedSafetyMapThisTurn = true;
	
	playerCostMap = allocGrid();
	monsterCostMap = allocGrid();
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			safetyMap[i][j] = 30000;
			
			playerCostMap[i][j] = monsterCostMap[i][j] = 1; // prophylactic
			
			if (cellHasTerrainFlag(i, j, T_OBSTRUCTS_PASSABILITY)
                && (!cellHasTMFlag(i, j, TM_IS_SECRET) || (discoveredTerrainFlagsAtLoc(i, j) & T_OBSTRUCTS_PASSABILITY))) {
                
				playerCostMap[i][j] = monsterCostMap[i][j] = cellHasTerrainFlag(i, j, T_OBSTRUCTS_DIAGONAL_MOVEMENT) ? PDS_OBSTRUCTION : PDS_FORBIDDEN;
			} else if (cellHasTerrainFlag(i, j, T_LAVA_INSTA_DEATH)) {
                monsterCostMap[i][j] = PDS_FORBIDDEN;
                if (player.status[STATUS_LEVITATING] || !player.status[STATUS_IMMUNE_TO_FIRE]) {
                    playerCostMap[i][j] = 1;
                } else {
                    playerCostMap[i][j] = PDS_FORBIDDEN;
                }
			} else {
				if (pmap[i][j].flags & HAS_MONSTER) {
					monst = monsterAtLoc(i, j);
					if ((monst->creatureState == MONSTER_SLEEPING
						 || monst->turnsSpentStationary > 2
						 || monst->creatureState == MONSTER_ALLY)
						&& monst->creatureState != MONSTER_FLEEING) {
						
						playerCostMap[i][j] = 1;
						monsterCostMap[i][j] = PDS_FORBIDDEN;
						continue;
					}
				}
				
				if (cellHasTerrainFlag(i, j, (T_AUTO_DESCENT | T_IS_DF_TRAP))) {// | T_IS_FIRE))) {
					monsterCostMap[i][j] = PDS_FORBIDDEN;
                    if (player.status[STATUS_LEVITATING]) {
                        playerCostMap[i][j] = 1;
                    } else {
                        playerCostMap[i][j] = PDS_FORBIDDEN;
                    }
				} else if (cellHasTerrainFlag(i, j, T_IS_FIRE)) {
					monsterCostMap[i][j] = PDS_FORBIDDEN;
                    if (player.status[STATUS_IMMUNE_TO_FIRE]) {
                        playerCostMap[i][j] = 1;
                    } else {
                        playerCostMap[i][j] = PDS_FORBIDDEN;
                    }
				} else if (cellHasTerrainFlag(i, j, (T_IS_DEEP_WATER | T_SPONTANEOUSLY_IGNITES))) {
                    if (player.status[STATUS_LEVITATING]) {
                        playerCostMap[i][j] = 1;
                    } else {
                        playerCostMap[i][j] = 5;
                    }
					monsterCostMap[i][j] = 5;
				} else {
					playerCostMap[i][j] = monsterCostMap[i][j] = 1;
				}
			}
		}
	}
	
	safetyMap[player.xLoc][player.yLoc] = 0;
	playerCostMap[player.xLoc][player.yLoc] = 1;
	monsterCostMap[player.xLoc][player.yLoc] = PDS_FORBIDDEN;
	
	playerCostMap[rogue.upLoc[0]][rogue.upLoc[1]] = PDS_FORBIDDEN;
	monsterCostMap[rogue.upLoc[0]][rogue.upLoc[1]] = PDS_FORBIDDEN;
	playerCostMap[rogue.downLoc[0]][rogue.downLoc[1]] = PDS_FORBIDDEN;
	monsterCostMap[rogue.downLoc[0]][rogue.downLoc[1]] = PDS_FORBIDDEN;
	
	dijkstraScan(safetyMap, playerCostMap, false);
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (monsterCostMap[i][j] < 0) {
				continue;
			}
			
			if (safetyMap[i][j] == 30000) {
				safetyMap[i][j] = 150;
			}
			
			safetyMap[i][j] = 50 * safetyMap[i][j] / (50 + safetyMap[i][j]);
			
			safetyMap[i][j] *= -3;
			
			if (pmap[i][j].flags & IN_LOOP) {
				safetyMap[i][j] -= 10;
			}
		}
	}
	dijkstraScan(safetyMap, monsterCostMap, false);
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (monsterCostMap[i][j] < 0) {
				safetyMap[i][j] = 30000;
			}
		}
	}
	freeGrid(playerCostMap);
	freeGrid(monsterCostMap);
}

void updateSafeTerrainMap() {
	short i, j;
	short **costMap;
	creature *monst;
	
	rogue.updatedMapToSafeTerrainThisTurn = true;
	costMap = allocGrid();
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			monst = monsterAtLoc(i, j);
			if (cellHasTerrainFlag(i, j, T_OBSTRUCTS_PASSABILITY)
                && (!cellHasTMFlag(i, j, TM_IS_SECRET) || (discoveredTerrainFlagsAtLoc(i, j) & T_OBSTRUCTS_PASSABILITY))) {
                
				costMap[i][j] = cellHasTerrainFlag(i, j, T_OBSTRUCTS_DIAGONAL_MOVEMENT) ? PDS_OBSTRUCTION : PDS_FORBIDDEN;
				rogue.mapToSafeTerrain[i][j] = 30000; // OOS prophylactic
			} else if ((monst && monst->turnsSpentStationary > 1)
				|| (cellHasTerrainFlag(i, j, T_PATHING_BLOCKER & ~T_HARMFUL_TERRAIN) && !cellHasTMFlag(i, j, TM_IS_SECRET))) {
                
				costMap[i][j] = PDS_FORBIDDEN;
				rogue.mapToSafeTerrain[i][j] = 30000;
			} else if (cellHasTerrainFlag(i, j, T_HARMFUL_TERRAIN) || pmap[i][j].layers[DUNGEON] == DOOR) {
				// The door thing is an aesthetically offensive but necessary hack to make sure
				// that monsters trying to find their way out of poisonous gas do not sprint for
				// the doors. Doors are superficially free of gas, but as soon as they are opened,
				// gas will fill their tile, so they are not actually safe. Without this fix,
				// allies will fidget back and forth in a doorway while they asphyxiate.
				// This will have to do. It's a difficult problem to solve elegantly.
				costMap[i][j] = 1;
				rogue.mapToSafeTerrain[i][j] = 30000;
			} else {
				costMap[i][j] = 1;
				rogue.mapToSafeTerrain[i][j] = 0;
			}
		}
	}
	dijkstraScan(rogue.mapToSafeTerrain, costMap, false);
	freeGrid(costMap);
}

void rechargeItemsIncrementally() {
	item *theItem, *autoIdentifyItems[3] = {rogue.armor, rogue.ringLeft, rogue.ringRight};
	char buf[DCOLS*3], theItemName[DCOLS*3];
	short rechargeIncrement, staffRechargeAmount, i;
	
	if (rogue.wisdomBonus) {
		rechargeIncrement = (int) (10 * pow(1.3, min(27, rogue.wisdomBonus)) + FLOAT_FUDGE); // at level 27, you recharge anything to full in one turn
	} else {
		rechargeIncrement = 10;
	}
	
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (theItem->category & STAFF) {
			if (theItem->enchant2 <= 0) {
				// if it's time for a recharge
                if (theItem->charges < theItem->enchant1) {
                    theItem->charges++;
                }
				// staffs of blinking and obstruction recharge half as fast so they're less powerful
				staffRechargeAmount = (theItem->kind == STAFF_BLINKING || theItem->kind == STAFF_OBSTRUCTION ? 10000 : 5000) / theItem->enchant1;
                theItem->enchant2 = randClumpedRange(max(staffRechargeAmount / 3, 1), staffRechargeAmount * 5 / 3, 3);
			} else if (theItem->charges < theItem->enchant1) {
				theItem->enchant2 -= rechargeIncrement;
			}
		} else if ((theItem->category & CHARM) && (theItem->charges > 0)) {
            theItem->charges--;
            if (theItem->charges == 0) {
				itemName(theItem, theItemName, false, false, NULL);
				sprintf(buf, "your %s has recharged.", theItemName);
                message(buf, false);
            }
        }
	}
	
	for (i=0; i<3; i++) {
		theItem = autoIdentifyItems[i];
		if (theItem
			&& theItem->charges > 0
			&& !(theItem->flags & ITEM_IDENTIFIED)) {
			
			theItem->charges--;
			if (theItem->charges <= 0) {
				itemName(theItem, theItemName, false, false, NULL);
				sprintf(buf, "you are now familiar enough with your %s to identify it.", theItemName);
				messageWithColor(buf, &itemMessageColor, false);
				
				if (theItem->category & ARMOR) {
					// Don't necessarily reveal the armor's runic specifically, just that it has one.
					theItem->flags |= ITEM_IDENTIFIED;
				} else if (theItem->category & RING) {
					identify(theItem);
				}
				updateIdentifiableItems();
				
				itemName(theItem, theItemName, true, true, NULL);
				sprintf(buf, "%s %s.", (theItem->quantity > 1 ? "they are" : "it is"), theItemName);
				messageWithColor(buf, &itemMessageColor, false);
			}
		}
	}
}

void extinguishFireOnCreature(creature *monst) {
	
	monst->status[STATUS_BURNING] = 0;
	if (monst == &player) {
		player.info.foreColor = &white;
		rogue.minersLight.lightColor = &minersLightColor;
		refreshDungeonCell(player.xLoc, player.yLoc);
		updateVision(true);
		message("you are no longer on fire.", false);
	}
}

// n is the monster's depthLevel - 1.
void monsterEntersLevel(creature *monst, short n) {
    creature *prevMonst;
	char monstName[COLS], buf[COLS];
	boolean pit = false;
    
    // place traversing monster near the stairs on this level
    if (monst->bookkeepingFlags & MONST_APPROACHING_DOWNSTAIRS) {
        monst->xLoc = rogue.upLoc[0];
        monst->yLoc = rogue.upLoc[1];
    } else if (monst->bookkeepingFlags & MONST_APPROACHING_UPSTAIRS) {
        monst->xLoc = rogue.downLoc[0];
        monst->yLoc = rogue.downLoc[1];
    } else if (monst->bookkeepingFlags & MONST_APPROACHING_PIT) { // jumping down pit
        pit = true;
        monst->xLoc = levels[n].playerExitedVia[0];
        monst->yLoc = levels[n].playerExitedVia[1];
    } else {
#ifdef BROGUE_ASSERTS
        assert(false);
#endif
    }
    monst->depth = rogue.depthLevel;
    
    if (!pit) {
        getQualifyingPathLocNear(&(monst->xLoc), &(monst->yLoc), monst->xLoc, monst->yLoc, true,
                                 T_DIVIDES_LEVEL & avoidedFlagsForMonster(&(monst->info)), 0,
                                 avoidedFlagsForMonster(&(monst->info)), HAS_STAIRS, false);
    }
    if (!pit
        && (pmap[monst->xLoc][monst->yLoc].flags & (HAS_PLAYER | HAS_MONSTER))
        && !(terrainFlags(monst->xLoc, monst->yLoc) & avoidedFlagsForMonster(&(monst->info)))) {
        // Monsters using the stairs will displace any creatures already located there, to thwart stair-dancing.
        prevMonst = monsterAtLoc(monst->xLoc, monst->yLoc);
#ifdef BROGUE_ASSERTS
        assert(prevMonst);
#endif
        getQualifyingPathLocNear(&(prevMonst->xLoc), &(prevMonst->yLoc), monst->xLoc, monst->yLoc, true,
                                 T_DIVIDES_LEVEL & avoidedFlagsForMonster(&(prevMonst->info)), 0,
                                 avoidedFlagsForMonster(&(prevMonst->info)), (HAS_MONSTER | HAS_PLAYER | HAS_STAIRS), false);
        pmap[monst->xLoc][monst->yLoc].flags &= ~(HAS_PLAYER | HAS_MONSTER);
        pmap[prevMonst->xLoc][prevMonst->yLoc].flags |= (prevMonst == &player ? HAS_PLAYER : HAS_MONSTER);
        refreshDungeonCell(prevMonst->xLoc, prevMonst->yLoc);
        //DEBUG printf("\nBumped a creature (%s) from (%i, %i) to (%i, %i).", prevMonst->info.monsterName, monst->xLoc, monst->yLoc, prevMonst->xLoc, prevMonst->yLoc);
    }
    
    // remove traversing monster from other level monster chain
    if (monst == levels[n].monsters) {
        levels[n].monsters = monst->nextCreature;
    } else {
        for (prevMonst = levels[n].monsters; prevMonst->nextCreature != monst; prevMonst = prevMonst->nextCreature);
        prevMonst->nextCreature = monst->nextCreature;
    }
    
    // prepend traversing monster to current level monster chain
    monst->nextCreature = monsters->nextCreature;
    monsters->nextCreature = monst;
    
    monst->status[STATUS_ENTERS_LEVEL_IN] = 0;
    monst->bookkeepingFlags |= MONST_PREPLACED;
    monst->bookkeepingFlags &= ~MONST_IS_FALLING;
    restoreMonster(monst, NULL, NULL);
    //DEBUG printf("\nPlaced a creature (%s) at (%i, %i).", monst->info.monsterName, monst->xLoc, monst->yLoc);
    monst->ticksUntilTurn = monst->movementSpeed;
    refreshDungeonCell(monst->xLoc, monst->yLoc);
    
    if (pit) {
        monsterName(monstName, monst, true);
        if (!monst->status[STATUS_LEVITATING]) {
            if (inflictDamage(monst, randClumpedRange(6, 12, 2), &red)) {
                if (canSeeMonster(monst)) {
                    sprintf(buf, "%s plummets from above and splatters against the ground!", monstName);
                    messageWithColor(buf, messageColorFromVictim(monst), false);
                }
            } else {
                if (canSeeMonster(monst)) {
                    sprintf(buf, "%s falls from above and crashes to the ground!", monstName);
                    message(buf, false);
                }
            }
        } else if (canSeeMonster(monst)) {
            sprintf(buf, "%s swoops into the cavern from above.", monstName);
            message(buf, false);
        }
    }
}

void monstersApproachStairs() {
	creature *monst, *nextMonst;
	short n;
	
	for (n = rogue.depthLevel - 2; n <= rogue.depthLevel; n += 2) { // cycle through previous and next level
		if (n >= 0 && n < DEEPEST_LEVEL && levels[n].visited) {
			for (monst = levels[n].monsters; monst != NULL;) {
				nextMonst = monst->nextCreature;
				if (monst->status[STATUS_ENTERS_LEVEL_IN] > 1) {
					monst->status[STATUS_ENTERS_LEVEL_IN]--;
				} else if (monst->status[STATUS_ENTERS_LEVEL_IN] == 1) {
                    monsterEntersLevel(monst, n);
                }
				monst = nextMonst;
			}
		}
	}
}

void decrementPlayerStatus() {
	if (player.status[STATUS_NUTRITION] > 0) {
		if (!numberOfMatchingPackItems(AMULET, 0, 0, false) || rand_percent(20)) {
			player.status[STATUS_NUTRITION]--;
		}
	}
    checkNutrition();
	
	if (player.status[STATUS_TELEPATHIC] > 0 && !--player.status[STATUS_TELEPATHIC]) {
		updateVision(true);
		message("your preternatural mental sensitivity fades.", false);
	}
	
	if (player.status[STATUS_DARKNESS] > 0) {
		player.status[STATUS_DARKNESS]--;
		updateMinersLightRadius();
		//updateVision();
	}
	
	if (player.status[STATUS_HALLUCINATING] > 0 && !--player.status[STATUS_HALLUCINATING]) {
		displayLevel();
		message("your hallucinations fade.", false);
	}
	
	if (player.status[STATUS_LEVITATING] > 0 && !--player.status[STATUS_LEVITATING]) {
		message("you are no longer levitating.", false);
	}
	
	if (player.status[STATUS_CONFUSED] > 0 && !--player.status[STATUS_CONFUSED]) {
		message("you no longer feel confused.", false);
	}
	
	if (player.status[STATUS_NAUSEOUS] > 0 && !--player.status[STATUS_NAUSEOUS]) {
		message("you feel less nauseous.", false);
	}
	
	if (player.status[STATUS_PARALYZED] > 0 && !--player.status[STATUS_PARALYZED]) {
		message("you can move again.", false);
	}
	
	if (player.status[STATUS_HASTED] > 0 && !--player.status[STATUS_HASTED]) {
		player.movementSpeed = player.info.movementSpeed;
		player.attackSpeed = player.info.attackSpeed;
        synchronizePlayerTimeState();
		message("your supernatural speed fades.", false);
	}
	
	if (player.status[STATUS_SLOWED] > 0 && !--player.status[STATUS_SLOWED]) {
		player.movementSpeed = player.info.movementSpeed;
		player.attackSpeed = player.info.attackSpeed;
        synchronizePlayerTimeState();
		message("your normal speed resumes.", false);
	}
	
	if (player.status[STATUS_WEAKENED] > 0 && !--player.status[STATUS_WEAKENED]) {
		player.weaknessAmount = 0;
		message("strength returns to your muscles as the weakening toxin wears off.", false);
		updateEncumbrance();
	}
	
	if (player.status[STATUS_IMMUNE_TO_FIRE] > 0 && !--player.status[STATUS_IMMUNE_TO_FIRE]) {
		//player.info.flags &= ~MONST_IMMUNE_TO_FIRE;
		message("you no longer feel immune to fire.", false);
	}
	
	if (player.status[STATUS_STUCK] && !cellHasTerrainFlag(player.xLoc, player.yLoc, T_ENTANGLES)) {
		player.status[STATUS_STUCK] = 0;
	}
	
	if (player.status[STATUS_EXPLOSION_IMMUNITY]) {
		player.status[STATUS_EXPLOSION_IMMUNITY]--;
	}
	
	if (player.status[STATUS_DISCORDANT]) {
		player.status[STATUS_DISCORDANT]--;
	}
	
	if (player.status[STATUS_SHIELDED]) {
		player.status[STATUS_SHIELDED] -= player.maxStatus[STATUS_SHIELDED] / 20;
        if (player.status[STATUS_SHIELDED] <= 0) {
            player.status[STATUS_SHIELDED] = player.maxStatus[STATUS_SHIELDED] = 0;
        }
	}
    
	if (player.status[STATUS_INVISIBLE] > 0 && !--player.status[STATUS_INVISIBLE]) {
		message("you are no longer invisible.", false);
	}
	
	if (rogue.monsterSpawnFuse <= 0) {
		spawnPeriodicHorde();
		rogue.monsterSpawnFuse = rand_range(125, 175);
	}
}

void autoRest() {
	short i = 0;
	boolean initiallyEmbedded; // Stop as soon as we're free from crystal.
	
	rogue.disturbed = false;
	rogue.automationActive = true;
	initiallyEmbedded = cellHasTerrainFlag(player.xLoc, player.yLoc, T_OBSTRUCTS_PASSABILITY);
	
	if ((player.currentHP < player.info.maxHP
		 || player.status[STATUS_HALLUCINATING]
		 || player.status[STATUS_CONFUSED]
		 || player.status[STATUS_NAUSEOUS]
		 || player.status[STATUS_POISONED]
		 || player.status[STATUS_DARKNESS]
		 || initiallyEmbedded)
		&& !rogue.disturbed) {
		while (i++ < TURNS_FOR_FULL_REGEN
			   && (player.currentHP < player.info.maxHP
				   || player.status[STATUS_HALLUCINATING]
				   || player.status[STATUS_CONFUSED]
				   || player.status[STATUS_NAUSEOUS]
				   || player.status[STATUS_POISONED]
				   || player.status[STATUS_DARKNESS]
				   || cellHasTerrainFlag(player.xLoc, player.yLoc, T_OBSTRUCTS_PASSABILITY))
			   && !rogue.disturbed
			   && (!initiallyEmbedded || cellHasTerrainFlag(player.xLoc, player.yLoc, T_OBSTRUCTS_PASSABILITY))) {
			
			recordKeystroke(REST_KEY, false, false);
			rogue.justRested = true;
			playerTurnEnded();
			//refreshSideBar(-1, -1, false);
			if (pauseBrogue(1)) {
				rogue.disturbed = true;
			}
			//refreshSideBar(-1, -1, false);
		}
	} else {
		for (i=0; i<100 && !rogue.disturbed; i++) {
			recordKeystroke(REST_KEY, false, false);
			rogue.justRested = true;
			playerTurnEnded();
			//refreshSideBar(-1, -1, false);
			if (pauseBrogue(1)) {
				rogue.disturbed = true;
			}
			//refreshSideBar(-1, -1, false);
		}
	}
	rogue.automationActive = false;
}

short directionOfKeypress(unsigned short ch) {
	switch (ch) {
		case LEFT_KEY:
		case LEFT_ARROW:
		case NUMPAD_4:
			return LEFT;
		case RIGHT_KEY:
		case RIGHT_ARROW:
		case NUMPAD_6:
			return RIGHT;
		case UP_KEY:
		case UP_ARROW:
		case NUMPAD_8:
			return UP;
		case DOWN_KEY:
		case DOWN_ARROW:
		case NUMPAD_2:
			return DOWN;
		case UPLEFT_KEY:
		case NUMPAD_7:
			return UPLEFT;
		case UPRIGHT_KEY:
		case NUMPAD_9:
			return UPRIGHT;
		case DOWNLEFT_KEY:
		case NUMPAD_1:
			return DOWNLEFT;
		case DOWNRIGHT_KEY:
		case NUMPAD_3:
			return DOWNRIGHT;
		default:
			return -1;
	}
}

void startFighting(enum directions dir, boolean tillDeath) {
	short x, y, expectedDamage;
	creature *monst;
	
	x = player.xLoc + nbDirs[dir][0];
	y = player.yLoc + nbDirs[dir][1];
	
	monst = monsterAtLoc(x, y);
    
    if (monst->info.flags & MONST_IMMUNE_TO_WEAPONS) {
        return;
    }
	
	expectedDamage = monst->info.damage.upperBound * monsterDamageAdjustmentAmount(monst);
	if (rogue.depthLevel == 1) {
		expectedDamage /= 2;
	}
	
	if (rogue.easyMode) {
		expectedDamage /= 5;
	}
	
	rogue.blockCombatText = true;
	rogue.disturbed = false;
	
//	if (monst->creatureState == MONSTER_ALLY) {
//		monst->creatureState = MONSTER_TRACKING_SCENT;
//	}
	
	do {
		if (!playerMoves(dir)) {
			break;
		}
		if (pauseBrogue(1)) {
			break;
		}
	} while (!rogue.disturbed && !rogue.gameHasEnded && (tillDeath || player.currentHP > expectedDamage)
			 && (pmap[x][y].flags & HAS_MONSTER) && monsterAtLoc(x, y) == monst);
	
	rogue.blockCombatText = false;
}

//void autoFight(boolean tillDeath) {
//	short x, y, dir;
//	creature *monst;
//	
//	if (player.status[STATUS_HALLUCINATING] && !tillDeath) {
//		message("Not while you're hallucinating.", false);
//		return;
//	}
//	if (player.status[STATUS_CONFUSED]) {
//		message("Not while you're confused.", false);
//		return;
//	}
//	
//	confirmMessages();
//	temporaryMessage("Fight what? (<hjklyubn> to select direction)", false);
//	dir = directionOfKeypress(nextKeyPress(false));
//	confirmMessages();
//	
//	if (dir == -1) {
//		return;
//	}
//	
//	x = player.xLoc + nbDirs[dir][0];
//	y = player.yLoc + nbDirs[dir][1];
//	
//	monst = monsterAtLoc(x, y);
//	
//	if (!monst
//		|| monst->status[STATUS_INVISIBLE]
//		|| (monst->bookkeepingFlags & MONST_SUBMERGED)
//		|| !playerCanSee(x, y)) {
//		message("I see no monster there.", false);
//		return;
//	}
//	
//	startFighting(dir, tillDeath);
//}

void addXPXPToAlly(short XPXP, creature *monst) {
    char theMonsterName[100], buf[200];
    if (!(monst->info.flags & (MONST_INANIMATE | MONST_IMMOBILE))
        && monst->creatureState == MONSTER_ALLY
        && monst->spawnDepth <= rogue.depthLevel
        && rogue.depthLevel <= AMULET_LEVEL) {
        
        monst->xpxp += XPXP;
        monst->absorbXPXP += XPXP;
        //printf("\n%i xpxp added to your %s this turn.", rogue.xpxpThisTurn, monst->info.monsterName);
        while (monst->xpxp >= XPXP_NEEDED_FOR_LEVELUP) {
            monst->xpxp -= XPXP_NEEDED_FOR_LEVELUP;
            monst->info.maxHP += 5;
            monst->currentHP += (5 * monst->currentHP / (monst->info.maxHP - 5));
            monst->info.defense += 5;
            monst->info.accuracy += 5;
            monst->info.damage.lowerBound += max(1, monst->info.damage.lowerBound / 20);
            monst->info.damage.upperBound += max(1, monst->info.damage.upperBound / 20);
            if (!(monst->bookkeepingFlags & MONST_TELEPATHICALLY_REVEALED)) {
                monst->bookkeepingFlags |= MONST_TELEPATHICALLY_REVEALED;
                updateVision(true);
                monsterName(theMonsterName, monst, false);
                sprintf(buf, "you have developed a bond with your %s.", theMonsterName);
                messageWithColor(buf, &advancementMessageColor, false);
            }
            //				if (canSeeMonster(monst)) {
            //					monsterName(theMonsterName, monst, false);
            //					sprintf(buf, "your %s looks stronger", theMonsterName);
            //					combatMessage(buf, &advancementMessageColor);
            //				}
        }
    }
}

void handleXPXP() {
	creature *monst;
	//char buf[DCOLS*2], theMonsterName[50];
	
	for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
        addXPXPToAlly(rogue.xpxpThisTurn, monst);
	}
    if (rogue.depthLevel > 1) {
        for (monst = levels[rogue.depthLevel - 2].monsters; monst != NULL; monst = monst->nextCreature) {
            addXPXPToAlly(rogue.xpxpThisTurn, monst);
        }
    }
    if (rogue.depthLevel < DEEPEST_LEVEL) {
        for (monst = levels[rogue.depthLevel].monsters; monst != NULL; monst = monst->nextCreature) {
            addXPXPToAlly(rogue.xpxpThisTurn, monst);
        }
    }
	rogue.xpxpThisTurn = 0;
}

void playerFalls() {
	short damage;
	short layer;
    
    if (cellHasTMFlag(player.xLoc, player.yLoc, TM_IS_SECRET)
        && playerCanSee(player.xLoc, player.yLoc)) {
		
		discover(player.xLoc, player.yLoc);
    }
    
    monstersFall();	// Monsters must fall with the player rather than getting suspended on the previous level.
    updateFloorItems(); // Likewise, items should fall with the player rather than getting suspended above.
	
	layer = layerWithFlag(player.xLoc, player.yLoc, T_AUTO_DESCENT);
	if (layer >= 0) {
		message(tileCatalog[pmap[player.xLoc][player.yLoc].layers[layer]].flavorText, true);
	} else if (layer == -1) {
		message("You plunge downward!", true);
	}
	
    player.bookkeepingFlags &= ~(MONST_IS_FALLING | MONST_SEIZED | MONST_SEIZING);
	rogue.disturbed = true;
    
    if (rogue.depthLevel < DEEPEST_LEVEL) {
        rogue.depthLevel++;
        startLevel(rogue.depthLevel - 1, 0);
        damage = randClumpedRange(FALL_DAMAGE_MIN, FALL_DAMAGE_MAX, 2);
        messageWithColor("You are damaged by the fall.", &badMessageColor, false);
        if (inflictDamage(&player, damage, &red)) {
            gameOver("Killed by a fall", true);
        } else if (rogue.depthLevel > rogue.deepestLevel) {
            rogue.deepestLevel = rogue.depthLevel;
        }
    } else {
        message("A strange force seizes you as you fall.", false);
        teleport(&player, -1, -1, true);
    }
    createFlare(player.xLoc, player.yLoc, GENERIC_FLASH_LIGHT);
    animateFlares(rogue.flares, rogue.flareCount);
    rogue.flareCount = 0;
}

void handleAllyHungerAlerts() {
    char buf[COLS*3], name[50];
    creature *monst;
    
    if (!player.status[STATUS_HALLUCINATING]) {
        for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
            if (monst->creatureState == MONSTER_ALLY
                && canSeeMonster(monst)
                && monst->absorbXPXP >= XPXP_NEEDED_FOR_ABSORB
                && !(monst->bookkeepingFlags & MONST_ALLY_ANNOUNCED_HUNGER)) {
                
                monst->bookkeepingFlags |= MONST_ALLY_ANNOUNCED_HUNGER;
                monsterName(name, monst, true);
                sprintf(buf, "%s looks like $HESHE's ready to learn something new.", name);
                resolvePronounEscapes(buf, monst);
                messageWithColor(buf, &advancementMessageColor, false);
            }
        }
    }
}

#define NUMBER_HEALTH_THRESHOLDS	4

void handleHealthAlerts() {
	short i, x, y, currentPercent, thresholds[] = {5, 10, 25, 40};
	char buf[DCOLS];
	
	currentPercent = player.currentHP * 100 / player.info.maxHP;
	
	if (currentPercent < rogue.previousHealthPercent && !rogue.gameHasEnded) {
		for (i=0; i < NUMBER_HEALTH_THRESHOLDS; i++) {
			if (currentPercent < thresholds[i] && rogue.previousHealthPercent >= thresholds[i]) {
				if (player.yLoc > DROWS / 2) {
					y = mapToWindowY(player.yLoc - 2);
				} else {
					y = mapToWindowY(player.yLoc + 2);
				}
				sprintf(buf, " <%i%% health ", thresholds[i]);
				x = mapToWindowX(player.xLoc - strLenWithoutEscapes(buf) / 2);
				if (x > COLS - strLenWithoutEscapes(buf)) {
					x = COLS - strLenWithoutEscapes(buf);
				}
				flashMessage(buf, x, y, (rogue.playbackMode ? 100 : 1000), &badMessageColor, &darkRed);
                rogue.disturbed = true;
				break;
			}
		}
	}
	rogue.previousHealthPercent = currentPercent;
}

// Call this periodically (when haste/slow wears off and when moving between depths)
// to keep environmental updates in sync with player turns.
void synchronizePlayerTimeState() {
    rogue.ticksTillUpdateEnvironment = player.ticksUntilTurn;
}

void playerTurnEnded() {
	short soonestTurn, damage, turnsRequiredToShore, turnsToShore;
	char buf[COLS], buf2[COLS];
	creature *monst, *monst2, *nextMonst;
	boolean fastForward = false;
	
#ifdef BROGUE_ASSERTS
	assert(rogue.RNG == RNG_SUBSTANTIVE);
#endif
	
	handleXPXP();
	resetDFMessageEligibility();
	
	if (player.bookkeepingFlags & MONST_IS_FALLING) {
		playerFalls();
		handleHealthAlerts();
		return;
	}
	
	do {
		if (rogue.gameHasEnded) {
			return;
		}
		
		if (!player.status[STATUS_PARALYZED]) {
			rogue.playerTurnNumber++; // So recordings don't register more turns than you actually have.
		}
        rogue.absoluteTurnNumber++;
		
        if (player.status[STATUS_INVISIBLE]) {
            rogue.scentTurnNumber += 10; // Your scent fades very quickly while you are invisible.
        } else {
            rogue.scentTurnNumber += 3; // this must happen per subjective player time
        }
		if (rogue.scentTurnNumber > 20000) {
			resetScentTurnNumber();
		}
		
		//updateFlavorText();
		
		// Regeneration/starvation:
		if (player.status[STATUS_NUTRITION] <= 0) {
			player.currentHP--;
			if (player.currentHP <= 0) {
				gameOver("Starved to death", true);
				return;
			}
		} else if (player.currentHP < player.info.maxHP
				   && !player.status[STATUS_POISONED]) {
			if ((player.turnsUntilRegen -= 1000) <= 0) {
				player.currentHP++;
				player.turnsUntilRegen += player.info.turnsBetweenRegen;
			}
			if (player.regenPerTurn) {
				player.currentHP += player.regenPerTurn;
			}
		}
		
		if (rogue.awarenessBonus > -30) {
			search(rogue.awarenessBonus + 30);
		}
		
		if (rogue.staleLoopMap) {
			analyzeMap(false); // Don't need to update the chokemap.
		}
		
		for (monst = monsters->nextCreature; monst != NULL; monst = nextMonst) {
			nextMonst = monst->nextCreature;
			if ((monst->bookkeepingFlags & MONST_BOUND_TO_LEADER)
				&& (!monst->leader || !(monst->bookkeepingFlags & MONST_FOLLOWER))
				&& (monst->creatureState != MONSTER_ALLY)) {
				
				killCreature(monst, false);
				if (canSeeMonster(monst)) {
					monsterName(buf2, monst, true);
					sprintf(buf, "%s dissipates into thin air", buf2);
					combatMessage(buf, messageColorFromVictim(monst));
				}
			}
		}
		
		if (player.status[STATUS_BURNING] > 0) {
			damage = rand_range(1, 3);
			if (!(player.status[STATUS_IMMUNE_TO_FIRE]) && inflictDamage(&player, damage, &orange)) {
				gameOver("Burned to death", true);
			}
			if (!--player.status[STATUS_BURNING]) {
				player.status[STATUS_BURNING]++; // ugh
				extinguishFireOnCreature(&player);
			}
		}
		
		if (player.status[STATUS_POISONED] > 0) {
			player.status[STATUS_POISONED]--;
			if (inflictDamage(&player, 1, &green)) {
				gameOver("Died from poison", true);
			}
		}
		
		if (player.ticksUntilTurn == 0) { // attacking adds ticks elsewhere
			player.ticksUntilTurn += player.movementSpeed;
		} else if (player.ticksUntilTurn < 0) { // if he gets a free turn
			player.ticksUntilTurn = 0;
		}
		
		updateScent();
		rogue.updatedSafetyMapThisTurn			= false;
		rogue.updatedAllySafetyMapThisTurn		= false;
		rogue.updatedMapToSafeTerrainThisTurn	= false;
		
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if (D_SAFETY_VISION || monst->creatureState == MONSTER_FLEEING && pmap[monst->xLoc][monst->yLoc].flags & IN_FIELD_OF_VIEW) {	
				updateSafetyMap(); // only if there is a fleeing monster who can see the player
				break;
			}
		}
		
		if (D_BULLET_TIME && !rogue.justRested) {
			player.ticksUntilTurn = 0;
		}
		
		applyGradualTileEffectsToCreature(&player, player.ticksUntilTurn);
		
		if (rogue.gameHasEnded) {
			return;
		}
		
		rogue.heardCombatThisTurn = false;
		
		while (player.ticksUntilTurn > 0) {
			soonestTurn = 10000;
			for(monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				soonestTurn = min(soonestTurn, monst->ticksUntilTurn);
			}
			soonestTurn = min(soonestTurn, player.ticksUntilTurn);
			soonestTurn = min(soonestTurn, rogue.ticksTillUpdateEnvironment);
			for(monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				monst->ticksUntilTurn -= soonestTurn;
			}
			rogue.ticksTillUpdateEnvironment -= soonestTurn;
			if (rogue.ticksTillUpdateEnvironment <= 0) {
				rogue.ticksTillUpdateEnvironment += 100;
				
				// stuff that happens periodically according to an objective time measurement goes here:
				rechargeItemsIncrementally(); // staffs recharge every so often
				rogue.monsterSpawnFuse--; // monsters spawn in the level every so often
				
				for (monst = monsters->nextCreature; monst != NULL;) {
					nextMonst = monst->nextCreature;
					applyInstantTileEffectsToCreature(monst);
					monst = nextMonst; // this weirdness is in case the monster dies in the previous step
				}
				
				for (monst = monsters->nextCreature; monst != NULL;) {
					nextMonst = monst->nextCreature;
					decrementMonsterStatus(monst);
					monst = nextMonst;
				}
				
				// monsters with a dungeon feature spawn it every so often
				for (monst = monsters->nextCreature; monst != NULL;) {
					nextMonst = monst->nextCreature;
					if (monst->info.DFChance
                        && !(monst->info.flags & MONST_GETS_TURN_ON_ACTIVATION)
                        && rand_percent(monst->info.DFChance)) {
                        
						spawnDungeonFeature(monst->xLoc, monst->yLoc, &dungeonFeatureCatalog[monst->info.DFType], true, false);
					}
					monst = nextMonst;
				}
				
				updateEnvironment(); // Update fire and gas, items floating around in water, monsters falling into chasms, etc.
				decrementPlayerStatus();
				applyInstantTileEffectsToCreature(&player);
				if (rogue.gameHasEnded) { // poison gas, lava, trapdoor, etc.
					return;
				}
				monstersApproachStairs();

                // Rolling waypoint refresh:
                rogue.wpRefreshTicker++;
                if (rogue.wpRefreshTicker >= rogue.wpCount) {
                    rogue.wpRefreshTicker = 0;
                }
                refreshWaypoint(rogue.wpRefreshTicker);
			}
			
			for (monst = monsters->nextCreature; (monst != NULL) && (rogue.gameHasEnded == false); monst = monst->nextCreature) {
				if (monst->ticksUntilTurn <= 0) {
                    if (monst->currentHP > monst->info.maxHP) {
                        monst->info.maxHP = monst->currentHP;
                    }
					
                    if ((monst->info.flags & MONST_GETS_TURN_ON_ACTIVATION)
                        || monst->status[STATUS_PARALYZED]
                        || monst->status[STATUS_ENTRANCED]
                        || (monst->bookkeepingFlags & MONST_CAPTIVE)) {
                        
                        // Do not pass go; do not collect 200 gold.
                        monst->ticksUntilTurn = monst->movementSpeed;
                    } else {
                        monstersTurn(monst);
                    }
					
					for(monst2 = monsters->nextCreature; monst2 != NULL; monst2 = monst2->nextCreature) {
						if (monst2 == monst) { // monst still alive and on the level
							applyGradualTileEffectsToCreature(monst, monst->ticksUntilTurn);
							break;
						}
					}
					monst = monsters; // loop through from the beginning to be safe
				}
			}
			
			player.ticksUntilTurn -= soonestTurn;
			
			if (rogue.gameHasEnded) {
				return;
			}
		}
		// DEBUG displayLevel();
		//checkForDungeonErrors();
		
		updateVision(true);
		
		for(monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			if (canSeeMonster(monst) && !(monst->bookkeepingFlags & (MONST_WAS_VISIBLE))) {
				monst->bookkeepingFlags |= MONST_WAS_VISIBLE;
				if (monst->creatureState != MONSTER_ALLY) {
					rogue.disturbed = true;
					if (rogue.cautiousMode || rogue.automationActive) {
						assureCosmeticRNG;
						monsterName(buf2, monst, false);
						sprintf(buf, "you %s a%s %s",
								playerCanDirectlySee(monst->xLoc, monst->yLoc) ? "see" : "sense",
								(isVowelish(buf2) ? "n" : ""),
								buf2);
						if (rogue.cautiousMode) {
							strcat(buf, ".");
							message(buf, true);
						} else {
							combatMessage(buf, 0);
						}
						restoreRNG;
					}
				}
				if (cellHasTerrainFlag(monst->xLoc, monst->yLoc, T_OBSTRUCTS_PASSABILITY)
					&& cellHasTMFlag(monst->xLoc, monst->yLoc, TM_IS_SECRET)) {
					
					discover(monst->xLoc, monst->yLoc);
				}
				if (rogue.weapon && rogue.weapon->flags & ITEM_RUNIC
					&& rogue.weapon->enchant2 == W_SLAYING
					&& !(rogue.weapon->flags & ITEM_RUNIC_HINTED)
					&& rogue.weapon->vorpalEnemy == monst->info.monsterID) {
					
					rogue.weapon->flags |= ITEM_RUNIC_HINTED;
					itemName(rogue.weapon, buf2, false, false, NULL);
					sprintf(buf, "the runes on your %s gleam balefully.", buf2);
					messageWithColor(buf, &itemMessageColor, true);
				}
				if (rogue.armor && rogue.armor->flags & ITEM_RUNIC
					&& rogue.armor->enchant2 == A_IMMUNITY
					&& !(rogue.armor->flags & ITEM_RUNIC_HINTED)
					&& rogue.armor->vorpalEnemy == monst->info.monsterID) {
					
					rogue.armor->flags |= ITEM_RUNIC_HINTED;
					itemName(rogue.armor, buf2, false, false, NULL);
					sprintf(buf, "the runes on your %s glow protectively.", buf2);
					messageWithColor(buf, &itemMessageColor, true);
				}
			} else if (!canSeeMonster(monst)
					   && (monst->bookkeepingFlags & MONST_WAS_VISIBLE)
					   && !(monst->bookkeepingFlags & MONST_CAPTIVE)) {
				monst->bookkeepingFlags &= ~MONST_WAS_VISIBLE;
			}
		}
        
        handleAllyHungerAlerts();
		
		displayCombatText();
		
		if (player.status[STATUS_PARALYZED]) {
			if (!fastForward) {
				fastForward = rogue.playbackFastForward || pauseBrogue(25);
			}
		}

		//checkNutrition(); // Now handled within decrementPlayerStatus().
        if (!rogue.playbackFastForward) {
            shuffleTerrainColors(100, false);
        }
		
		displayAnnotation();
		
		refreshSideBar(-1, -1, false);
		
		applyInstantTileEffectsToCreature(&player);
		if (rogue.gameHasEnded) { // poison gas, lava, trapdoor, etc.
			return;
		}
        
        if (player.currentHP > player.info.maxHP) {
            player.currentHP = player.info.maxHP;
        }
        
        if (player.bookkeepingFlags & MONST_IS_FALLING) {
            playerFalls();
            handleHealthAlerts();
            return;
        }
		
	} while (player.status[STATUS_PARALYZED]);
	
	rogue.justRested = false;
	updateFlavorText();
	
	if (!rogue.updatedMapToShoreThisTurn) {
		updateMapToShore();
	}
	
	// "point of no return" check
	if ((player.status[STATUS_LEVITATING] && cellHasTerrainFlag(player.xLoc, player.yLoc, T_LAVA_INSTA_DEATH | T_IS_DEEP_WATER | T_AUTO_DESCENT))
		|| (player.status[STATUS_IMMUNE_TO_FIRE] && cellHasTerrainFlag(player.xLoc, player.yLoc, T_LAVA_INSTA_DEATH))) {
		if (!rogue.receivedLevitationWarning) {
			turnsRequiredToShore = rogue.mapToShore[player.xLoc][player.yLoc] * player.movementSpeed / 100;
			if (cellHasTerrainFlag(player.xLoc, player.yLoc, T_LAVA_INSTA_DEATH)) {
				turnsToShore = max(player.status[STATUS_LEVITATING], player.status[STATUS_IMMUNE_TO_FIRE]) * 100 / player.movementSpeed;
			} else {
				turnsToShore = player.status[STATUS_LEVITATING] * 100 / player.movementSpeed;
			}
			if (turnsRequiredToShore == turnsToShore || turnsRequiredToShore + 1 == turnsToShore) {
				message("better head back to solid ground!", true);
				rogue.receivedLevitationWarning = true;
			} else if (turnsRequiredToShore > turnsToShore) {
				message("you're past the point of no return!", true);
				rogue.receivedLevitationWarning = true;
			}
		}
	} else {
		rogue.receivedLevitationWarning = false;
	}
	
	emptyGraveyard();
	rogue.playbackBetweenTurns = true;
	RNGCheck();
	handleHealthAlerts();
    
    if (rogue.flareCount > 0) {
        animateFlares(rogue.flares, rogue.flareCount);
        rogue.flareCount = 0;
    }
}

void resetScentTurnNumber() { // don't want player.scentTurnNumber to roll over the short maxint!
	short i, j, d;
	rogue.scentTurnNumber -= 20000;
    for (d = 0; d < DEEPEST_LEVEL; d++) {
        if (levels[d].visited) {
            for (i=0; i<DCOLS; i++) {
                for (j=0; j<DROWS; j++) {
                    if (levels[d].scentMap[i][j] > 20000) {
                        levels[d].scentMap[i][j] -= 20000;
                    } else {
                        levels[d].scentMap[i][j] = 0;
                    }
                }
            }
        }
    }
}

boolean isDisturbed(short x, short y) {
	short i;
	creature *monst;
	for (i=0; i<8; i++) {
		monst = monsterAtLoc(x + nbDirs[i][0], y + nbDirs[i][1]);
		if (pmap[x + nbDirs[i][0]][y + nbDirs[i][1]].flags & (HAS_ITEM)) {
			// Do not trigger for submerged or invisible or unseen monsters.
			return true;
		}
		if (monst
			&& !(monst->creatureState == MONSTER_ALLY)
			&& (canSeeMonster(monst) || monsterRevealed(monst))) {
			// Do not trigger for submerged or invisible or unseen monsters.
			return true;
		}
	}
	return false;
}

void discover(short x, short y) {
	enum dungeonLayers layer;
	dungeonFeature *feat;
	if (cellHasTMFlag(x, y, TM_IS_SECRET)) {
		
		for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
			if (tileCatalog[pmap[x][y].layers[layer]].mechFlags & TM_IS_SECRET) {
				feat = &dungeonFeatureCatalog[tileCatalog[pmap[x][y].layers[layer]].discoverType];
				pmap[x][y].layers[layer] = (layer == DUNGEON ? FLOOR : NOTHING);
				spawnDungeonFeature(x, y, feat, true, false);
			}
		}
		refreshDungeonCell(x, y);
        
        if (playerCanSee(x, y)) {
            rogue.disturbed = true;
        }
	}
}

// returns true if found anything
boolean search(short searchStrength) {
	short i, j, radius, x, y, percent;
	boolean foundSomething = false;
	
	radius = searchStrength / 10;
	x = player.xLoc;
	y = player.yLoc;
	
	for (i = x - radius; i <= x + radius; i++) {
		for (j = y - radius; j <= y + radius; j++) {
			if (coordinatesAreInMap(i, j)
				&& playerCanDirectlySee(i, j)
				&& cellHasTMFlag(i, j, TM_IS_SECRET)) {
                
                percent = searchStrength - distanceBetween(x, y, i, j) * 10;
                if (cellHasTerrainFlag(i, j, T_OBSTRUCTS_PASSABILITY)) {
                    percent = percent * 2/3;
                }
                percent = min(percent, 100);
                
				if (rand_percent(percent)) {
                    discover(i, j);
                    foundSomething = true;
                }
			}
		}	
	}
	return foundSomething;
}

void routeTo(short x, short y, char *failureMessage) {
	if (player.xLoc == x && player.yLoc == y) {
		message("you are already there.", false);
	} else if (pmap[x][y].flags & (DISCOVERED | MAGIC_MAPPED)) {
		if (rogue.cursorLoc[0] == x && rogue.cursorLoc[1] == y) {
			travel(x, y, true);
		} else {
			rogue.cursorLoc[0] = x;
			rogue.cursorLoc[1] = y;
		}
	} else {
		message(failureMessage, false);
	}
}

boolean useStairs(short stairDirection) {
	boolean succeeded = false;
    //cellDisplayBuffer fromBuf[COLS][ROWS], toBuf[COLS][ROWS];
	
	if (stairDirection == 1) {
        if (rogue.depthLevel < DEEPEST_LEVEL) {
            //copyDisplayBuffer(fromBuf, displayBuffer);
            rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
            rogue.depthLevel++;
            message("You descend.", false);
            startLevel(rogue.depthLevel - 1, stairDirection);
            if (rogue.depthLevel > rogue.deepestLevel) {
                rogue.deepestLevel = rogue.depthLevel;
            }
            //copyDisplayBuffer(toBuf, displayBuffer);
            //irisFadeBetweenBuffers(fromBuf, toBuf, mapToWindowX(player.xLoc), mapToWindowY(player.yLoc), 10, false);
        } else if (numberOfMatchingPackItems(AMULET, 0, 0, false)) {
            victory(true);
        } else {
			confirmMessages();
            messageWithColor("the crystal archway repels you with a mysterious force!", &lightBlue, false);
            messageWithColor("(Only the bearer of the Amulet of Yendor may pass.)", &backgroundMessageColor, false);
        }
		succeeded = true;
	} else {
		if (rogue.depthLevel > 1 || numberOfMatchingPackItems(AMULET, 0, 0, false)) {
			rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
			rogue.depthLevel--;
			if (rogue.depthLevel == 0) {
				victory(false);
			} else {
                //copyDisplayBuffer(fromBuf, displayBuffer);
				message("You ascend.", false);
				startLevel(rogue.depthLevel + 1, stairDirection);
                //copyDisplayBuffer(toBuf, displayBuffer);
                //irisFadeBetweenBuffers(fromBuf, toBuf, mapToWindowX(player.xLoc), mapToWindowY(player.yLoc), 10, true);
			}
			succeeded = true;
		} else {
			confirmMessages();
            messageWithColor("The dungeon exit is magically sealed!", &lightBlue, false);
            messageWithColor("(Only the bearer of the Amulet of Yendor may pass.)", &backgroundMessageColor, false);
		}
	}
	
	if (succeeded) {
		rogue.cursorLoc[0] = -1;
		rogue.cursorLoc[1] = -1;
	}
	
	return succeeded;
}

void updateFieldOfViewDisplay(boolean updateDancingTerrain, boolean refreshDisplay) {
	short i, j;
	item *theItem;
    char buf[COLS*3], name[COLS*3];
	
	assureCosmeticRNG;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
			if (pmap[i][j].flags & IN_FIELD_OF_VIEW
				&& (tmap[i][j].light[0] + tmap[i][j].light[1] + tmap[i][j].light[2] > VISIBILITY_THRESHOLD)
				&& !(pmap[i][j].flags & CLAIRVOYANT_DARKENED)) {
				pmap[i][j].flags |= VISIBLE;
			}
			
			if ((pmap[i][j].flags & VISIBLE) && !(pmap[i][j].flags & WAS_VISIBLE)) { // if the cell became visible this move
				if (!(pmap[i][j].flags & DISCOVERED)
					&& !cellHasTerrainFlag(i, j, T_PATHING_BLOCKER)) {
					
					rogue.xpxpThisTurn++;
					if (rogue.automationActive
						&& (pmap[i][j].flags & HAS_ITEM)) {
						
						theItem = itemAtLoc(i, j);
						if (theItem && (theItem->category & KEY)) {
                            itemName(theItem, name, false, true, NULL);
                            sprintf(buf, "you see %s", name);
							combatMessage(buf, NULL);
						}
					}
				}
				pmap[i][j].flags |= DISCOVERED;
				pmap[i][j].flags &= ~STABLE_MEMORY;
			} else if (!(pmap[i][j].flags & CLAIRVOYANT_VISIBLE) && (pmap[i][j].flags & WAS_CLAIRVOYANT_VISIBLE)) { // ceased being clairvoyantly visible
			} else if (!(pmap[i][j].flags & WAS_CLAIRVOYANT_VISIBLE) && (pmap[i][j].flags & CLAIRVOYANT_VISIBLE)) { // became clairvoyantly visible
				pmap[i][j].flags &= ~STABLE_MEMORY;
			} else if (!(pmap[i][j].flags & WAS_TELEPATHIC_VISIBLE) && (pmap[i][j].flags & TELEPATHIC_VISIBLE)) { // became telepathically visible
                if (!(pmap[i][j].flags & DISCOVERED)
					&& !cellHasTerrainFlag(i, j, T_PATHING_BLOCKER)) {
					rogue.xpxpThisTurn++;
                }

				pmap[i][j].flags &= ~STABLE_MEMORY;
			} else if (playerCanSeeOrSense(i, j)
					   && (tmap[i][j].light[0] != tmap[i][j].oldLight[0] ||
						   tmap[i][j].light[1] != tmap[i][j].oldLight[1] ||
						   tmap[i][j].light[2] != tmap[i][j].oldLight[2])) { // if the cell's light color changed this move
					   } else if (updateDancingTerrain
								  && playerCanSee(i, j)
								  && (!rogue.automationActive || !(rogue.playerTurnNumber % 5))
								  && ((tileCatalog[pmap[i][j].layers[DUNGEON]].backColor)       && tileCatalog[pmap[i][j].layers[DUNGEON]].backColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[DUNGEON]].foreColor)    && tileCatalog[pmap[i][j].layers[DUNGEON]].foreColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[LIQUID]].backColor)     && tileCatalog[pmap[i][j].layers[LIQUID]].backColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[LIQUID]].foreColor)     && tileCatalog[pmap[i][j].layers[LIQUID]].foreColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[SURFACE]].backColor)    && tileCatalog[pmap[i][j].layers[SURFACE]].backColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[SURFACE]].foreColor)    && tileCatalog[pmap[i][j].layers[SURFACE]].foreColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[GAS]].backColor)        && tileCatalog[pmap[i][j].layers[GAS]].backColor->colorDances
									  || (tileCatalog[pmap[i][j].layers[GAS]].foreColor)        && tileCatalog[pmap[i][j].layers[GAS]].foreColor->colorDances
									  || player.status[STATUS_HALLUCINATING])) {
									  
									  pmap[i][j].flags &= ~STABLE_MEMORY;
								  }
		}
	}

	if (refreshDisplay) {
		displayLevel();
	}
	restoreRNG;
}

//		   Octants:      //
//			\7|8/        //
//			6\|/1        //
//			--@--        //
//			5/|\2        //
//			/4|3\        //

void betweenOctant1andN(short *x, short *y, short x0, short y0, short n) {
	short x1 = *x, y1 = *y;
	short dx = x1 - x0, dy = y1 - y0;
	switch (n) {
		case 1:
			return;
		case 2:
			*y = y0 - dy;
			return;
		case 5:
			*x = x0 - dx;
			*y = y0 - dy;
			return;
		case 6:
			*x = x0 - dx;
			return;
		case 8:
			*x = x0 - dy;
			*y = y0 - dx;
			return;
		case 3:
			*x = x0 - dy;
			*y = y0 + dx;
			return;
		case 7:
			*x = x0 + dy;
			*y = y0 - dx;
			return;
		case 4:
			*x = x0 + dy;
			*y = y0 + dx;
			return;
	}
}

// Returns a boolean grid indicating whether each square is in the field of view of (xLoc, yLoc).
// forbiddenTerrain is the set of terrain flags that will block vision (but the blocking cell itself is
// illuminated); forbiddenFlags is the set of map flags that will block vision.
// If cautiousOnWalls is set, we will not illuminate blocking tiles unless the tile one space closer to the origin
// is visible to the player; this is to prevent lights from illuminating a wall when the player is on the other
// side of the wall.
void getFOVMask(char grid[DCOLS][DROWS], short xLoc, short yLoc, float maxRadius,
				unsigned long forbiddenTerrain,	unsigned long forbiddenFlags, boolean cautiousOnWalls) {
	short i;
	
	for (i=1; i<=8; i++) {
		scanOctantFOV(grid, xLoc, yLoc, i, maxRadius, 1, LOS_SLOPE_GRANULARITY * -1, 0,
					  forbiddenTerrain, forbiddenFlags, cautiousOnWalls);
	}
}

// This is a custom implementation of recursive shadowcasting.
void scanOctantFOV(char grid[DCOLS][DROWS], short xLoc, short yLoc, short octant, float maxRadius,
				   short columnsRightFromOrigin, long startSlope, long endSlope, unsigned long forbiddenTerrain,
				   unsigned long forbiddenFlags, boolean cautiousOnWalls) {
	
	if (columnsRightFromOrigin >= maxRadius) return;
	
	short i, a, b, iStart, iEnd, x, y, x2, y2; // x and y are temporary variables on which we do the octant transform
	long newStartSlope, newEndSlope;
	boolean cellObstructed;
	
	newStartSlope = startSlope;
	
	a = ((LOS_SLOPE_GRANULARITY / -2 + 1) + startSlope * columnsRightFromOrigin) / LOS_SLOPE_GRANULARITY;
	b = ((LOS_SLOPE_GRANULARITY / -2 + 1) + endSlope * columnsRightFromOrigin) / LOS_SLOPE_GRANULARITY;
	
	iStart = min(a, b);
	iEnd = max(a, b);
	
	// restrict vision to a circle of radius maxRadius
	if ((columnsRightFromOrigin*columnsRightFromOrigin + iEnd*iEnd) >= maxRadius*maxRadius) {
		return;
	}
	if ((columnsRightFromOrigin*columnsRightFromOrigin + iStart*iStart) >= maxRadius*maxRadius) {
		iStart = (int) (-1 * sqrt(maxRadius*maxRadius - columnsRightFromOrigin*columnsRightFromOrigin) + FLOAT_FUDGE);
	}
	
	x = xLoc + columnsRightFromOrigin;
	y = yLoc + iStart;
	betweenOctant1andN(&x, &y, xLoc, yLoc, octant);
	boolean currentlyLit = coordinatesAreInMap(x, y) && !(cellHasTerrainFlag(x, y, forbiddenTerrain) ||
														  (pmap[x][y].flags & forbiddenFlags));
	for (i = iStart; i <= iEnd; i++) {
		x = xLoc + columnsRightFromOrigin;
		y = yLoc + i;
		betweenOctant1andN(&x, &y, xLoc, yLoc, octant);
		if (!coordinatesAreInMap(x, y)) {
			// We're off the map -- here there be memory corruption.
			continue;
		}
		cellObstructed = (cellHasTerrainFlag(x, y, forbiddenTerrain) || (pmap[x][y].flags & forbiddenFlags));
		// if we're cautious on walls and this is a wall:
		if (cautiousOnWalls && cellObstructed) {
			// (x2, y2) is the tile one space closer to the origin from the tile we're on:
			x2 = xLoc + columnsRightFromOrigin - 1;
			y2 = yLoc + i;
			if (i < 0) {
				y2++;
			} else if (i > 0) {
				y2--;
			}
			betweenOctant1andN(&x2, &y2, xLoc, yLoc, octant);
			
			if (pmap[x2][y2].flags & IN_FIELD_OF_VIEW) {
				// previous tile is visible, so illuminate
				grid[x][y] = 1;
			}
		} else {
			// illuminate
			grid[x][y] = 1;
		}
		if (!cellObstructed && !currentlyLit) { // next column slope starts here
			newStartSlope = (long int) ((LOS_SLOPE_GRANULARITY * (i) - LOS_SLOPE_GRANULARITY / 2) / (columnsRightFromOrigin + 0.5));
			currentlyLit = true;
		} else if (cellObstructed && currentlyLit) { // next column slope ends here
			newEndSlope = (long int) ((LOS_SLOPE_GRANULARITY * (i) - LOS_SLOPE_GRANULARITY / 2)
							/ (columnsRightFromOrigin - 0.5));
			if (newStartSlope <= newEndSlope) {
				// run next column
				scanOctantFOV(grid, xLoc, yLoc, octant, maxRadius, columnsRightFromOrigin + 1, newStartSlope, newEndSlope,
							  forbiddenTerrain, forbiddenFlags, cautiousOnWalls);
			}
			currentlyLit = false;
		}
	}
	if (currentlyLit) { // got to the bottom of the scan while lit
		newEndSlope = endSlope;
		if (newStartSlope <= newEndSlope) {
			// run next column
			scanOctantFOV(grid, xLoc, yLoc, octant, maxRadius, columnsRightFromOrigin + 1, newStartSlope, newEndSlope,
						  forbiddenTerrain, forbiddenFlags, cautiousOnWalls);
		}
	}
}

void addScentToCell(short x, short y, short distance) {
    unsigned short value;
	if (!cellHasTerrainFlag(x, y, T_OBSTRUCTS_SCENT) || !cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY)) {
        value = rogue.scentTurnNumber - distance;
		scentMap[x][y] = max(value, (unsigned short) scentMap[x][y]);
	}
}
