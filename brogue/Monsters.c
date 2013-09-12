/*
 *  Monsters.c
 *  Brogue
 *
 *  Created by Brian Walker on 1/13/09.
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
#include "Rogue.h"
#include "IncludeGlobals.h"

void mutateMonster(creature *monst, short mutationIndex) {
    monst->mutationIndex = mutationIndex;
    const mutation *theMut = &(mutationCatalog[mutationIndex]);
    monst->info.maxHP = monst->info.maxHP * theMut->healthFactor / 100;
    monst->info.movementSpeed = monst->info.movementSpeed * theMut->moveSpeedFactor / 100;
    monst->info.attackSpeed = monst->info.attackSpeed * theMut->attackSpeedFactor / 100;
    monst->info.defense = monst->info.defense * theMut->defenseFactor / 100;
    monst->info.damage.lowerBound = monst->info.damage.lowerBound * theMut->damageFactor / 100;
    monst->info.damage.upperBound = monst->info.damage.upperBound * theMut->damageFactor / 100;
    if (theMut->DFChance >= 0) {
        monst->info.DFChance = theMut->DFChance;
    }
    if (theMut->DFType > 0) {
        monst->info.DFType = theMut->DFType;
    }
    monst->info.flags |= theMut->monsterFlags;
    monst->info.abilityFlags |= theMut->monsterAbilityFlags;
}

// Allocates space, generates a creature of the given type,
// prepends it to the list of creatures, and returns a pointer to that creature. Note that the creature
// is not given a map location here!
creature *generateMonster(short monsterID, boolean itemPossible, boolean mutationPossible) {
	short itemChance, mutationChance, i, mutationAttempt;
	creature *monst;
	
	monst = (creature *) malloc(sizeof(creature));
	memset(monst, '\0', sizeof(creature));
	clearStatus(monst);
	monst->info = monsterCatalog[monsterID];
    
    monst->mutationIndex = -1;
    if (mutationPossible
        && !(monst->info.flags & MONST_NEVER_MUTATED)
        && !(monst->info.abilityFlags & MA_NEVER_MUTATED)
        && rogue.depthLevel > 10) {
        
        mutationChance = clamp(rogue.depthLevel - 10, 1, 10);
        if (rand_percent(mutationChance)) {
            mutationAttempt = rand_range(0, NUMBER_MUTATORS - 1);
            if (!(monst->info.flags & mutationCatalog[mutationAttempt].forbiddenFlags)
                && !(monst->info.abilityFlags & mutationCatalog[mutationAttempt].forbiddenAbilityFlags)) {
                
                mutateMonster(monst, mutationAttempt);
            }
        }
    }
    
	monst->nextCreature = monsters->nextCreature;
	monsters->nextCreature = monst;
	monst->xLoc = monst->yLoc = 0;
	monst->depth = rogue.depthLevel;
	monst->bookkeepingFlags = 0;
	monst->mapToMe = NULL;
	monst->safetyMap = NULL;
	monst->leader = NULL;
	monst->carriedMonster = NULL;
	monst->creatureState = (((monst->info.flags & MONST_NEVER_SLEEPS) || rogue.aggravating || rand_percent(25))
							? MONSTER_TRACKING_SCENT : MONSTER_SLEEPING);
	monst->creatureMode = MODE_NORMAL;
	monst->currentHP = monst->info.maxHP;
	monst->spawnDepth = rogue.depthLevel;
	monst->ticksUntilTurn = monst->info.movementSpeed;
	monst->info.turnsBetweenRegen *= 1000; // tracked as thousandths to prevent rounding errors
	monst->turnsUntilRegen = monst->info.turnsBetweenRegen;
	monst->regenPerTurn = 0;
	monst->movementSpeed = monst->info.movementSpeed;
	monst->attackSpeed = monst->info.attackSpeed;
	monst->turnsSpentStationary = 0;
	monst->xpxp = 0;
    monst->machineHome = 0;
	monst->absorbXPXP = 1500;
	monst->targetCorpseLoc[0] = monst->targetCorpseLoc[1] = 0;
    monst->targetWaypointIndex = -1;
    for (i=0; i < MAX_WAYPOINT_COUNT; i++) {
        monst->waypointAlreadyVisited[i] = rand_range(0, 1);
    }
    
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
	
	if (monst->info.flags & MONST_CARRY_ITEM_100) {
		itemChance = 100;
	} else if (monst->info.flags & MONST_CARRY_ITEM_25) {
		itemChance = 25;
	} else {
		itemChance = 0;
	}
	
    if (ITEMS_ENABLED
        && itemPossible
        && (rogue.depthLevel <= AMULET_LEVEL)
        && monsterItemsHopper->nextItem
        && rand_percent(itemChance)) {
        
        monst->carriedItem = monsterItemsHopper->nextItem;
        monsterItemsHopper->nextItem = monsterItemsHopper->nextItem->nextItem;
        monst->carriedItem->nextItem = NULL;
    } else {
        monst->carriedItem = NULL;
    }
	
	initializeGender(monst);
    
    if (!(monst->info.flags & MONST_INANIMATE) && !monst->status[STATUS_LIFESPAN_REMAINING]) {
        monst->bookkeepingFlags |= MONST_HAS_SOUL;
    }
	
	return monst;
}

boolean monsterRevealed(creature *monst) {
    if (monst == &player) {
        return false;
    } else if (monst->bookkeepingFlags & MONST_TELEPATHICALLY_REVEALED) {
        return true;
    } else if (monst->status[STATUS_ENTRANCED]) {
        return true;
    } else if (player.status[STATUS_TELEPATHIC] && !(monst->info.flags & MONST_INANIMATE)) {
        return true;
    }
    
    return false;
}

boolean monsterIsHidden(creature *monst) {
	return ((monst->status[STATUS_INVISIBLE] && monst->creatureState != MONSTER_ALLY && !pmap[monst->xLoc][monst->yLoc].layers[GAS]) // invisible and not ally
			|| ((monst->bookkeepingFlags & MONST_SUBMERGED) && !rogue.inWater)				// or submerged and player is not underwater
			|| (monst->bookkeepingFlags & MONST_IS_DORMANT)) ? true : false;				// or monster is dormant
}

boolean canSeeMonster(creature *monst) {
	if (monst == &player) {
		return true;
	}
	if (!monsterIsHidden(monst)
        && (playerCanSee(monst->xLoc, monst->yLoc) || monsterRevealed(monst))) {
		return true;
	}
	return false;
}

// This is different from canSeeMonster() in that it counts only physical sight -- not clairvoyance or telepathy.
boolean canDirectlySeeMonster(creature *monst) {
	if (monst == &player) {
		return true;
	}
	if (playerCanDirectlySee(monst->xLoc, monst->yLoc) && !monsterIsHidden(monst)) {
		return true;
	}
	return false;
}

void monsterName(char *buf, creature *monst, boolean includeArticle) {
	
	if (monst == &player) {
		strcpy(buf, "你");
		return;
	}
	if (canSeeMonster(monst) || rogue.playbackOmniscience) {
		if (player.status[STATUS_HALLUCINATING] && !rogue.playbackOmniscience) {
			
			assureCosmeticRNG;
			// article seems meaningless in chs
			sprintf(buf, "%s", monsterCatalog[rand_range(1, NUMBER_MONSTER_KINDS - 1)].monsterName);
			// sprintf(buf, "%s%s", (includeArticle ? "这个" : ""),
			// 		monsterCatalog[rand_range(1, NUMBER_MONSTER_KINDS - 1)].monsterName);
			restoreRNG;
			
			return;
		}
		sprintf(buf, "%s%s", (includeArticle ? (monst->creatureState == MONSTER_ALLY ? "你的" : "") : ""),
				monst->info.monsterName);
		//monsterText[monst->info.monsterID].name);
		return;
	} else {
		strcpy(buf, "某样东西");
		return;
	}
}

boolean monstersAreTeammates(creature *monst1, creature *monst2) {
	// if one follows the other, or the other follows the one, or they both follow the same
	return ((((monst1->bookkeepingFlags & MONST_FOLLOWER) && monst1->leader == monst2)
			 || ((monst2->bookkeepingFlags & MONST_FOLLOWER) && monst2->leader == monst1)
			 || (monst1->creatureState == MONSTER_ALLY && monst2 == &player)
			 || (monst1 == &player && monst2->creatureState == MONSTER_ALLY)
			 || (monst1->creatureState == MONSTER_ALLY && monst2->creatureState == MONSTER_ALLY)
			 || ((monst1->bookkeepingFlags & MONST_FOLLOWER) && (monst2->bookkeepingFlags & MONST_FOLLOWER)
				 && monst1->leader == monst2->leader)) ? true : false);
}

boolean monstersAreEnemies(creature *monst1, creature *monst2) {
	if ((monst1->bookkeepingFlags | monst2->bookkeepingFlags) & MONST_CAPTIVE) {
		return false;
	}
	if (monst1 == monst2) {
		return false; // Can't be enemies with yourself, even if discordant.
	}
	if (monst1->status[STATUS_DISCORDANT] || monst2->status[STATUS_DISCORDANT]) {
		return true;
	}
	// eels and krakens attack anything in deep water
	if (((monst1->info.flags & MONST_RESTRICTED_TO_LIQUID)
		 && !(monst2->info.flags & MONST_IMMUNE_TO_WATER)
		 && !(monst2->status[STATUS_LEVITATING])
		 && cellHasTerrainFlag(monst2->xLoc, monst2->yLoc, T_IS_DEEP_WATER))
		
		|| ((monst2->info.flags & MONST_RESTRICTED_TO_LIQUID)
			&& !(monst1->info.flags & MONST_IMMUNE_TO_WATER)
			&& !(monst1->status[STATUS_LEVITATING])
			&& cellHasTerrainFlag(monst1->xLoc, monst1->yLoc, T_IS_DEEP_WATER))) {
			
			return true;
		}
	return ((monst1->creatureState == MONSTER_ALLY || monst1 == &player)
			!= (monst2->creatureState == MONSTER_ALLY || monst2 == &player));
}

// Every program should have a function called initializeGender().
void initializeGender(creature *monst) {
	if ((monst->info.flags & MONST_MALE) && (monst->info.flags & MONST_FEMALE)) {	// If it's male and female,
		monst->info.flags &= ~(rand_percent(50) ? MONST_MALE : MONST_FEMALE);		// identify as one or the other.
	}
}

// Returns true if either string has a null terminator before they otherwise disagree.
boolean stringsMatch(const char *str1, const char *str2) {
	short i;
	
	for (i=0; str1[i] && str2[i]; i++) {
		if (str1[i] != str2[i]) {
			return false;
		}
	}
	return true;
}

// Genders:
//	0 = [character escape sequence]
//	1 = you
//	2 = male
//	3 = female
//	4 = neuter
void resolvePronounEscapes(char *text, creature *monst) {
	short pronounType, gender, i;
	char *insert, *scan;
	boolean capitalize;
	// Note: Escape sequences MUST be longer than EACH of the possible replacements.
	// That way, the string only contracts, and we don't need a buffer.
	const char pronouns[4][5][20] = {
		{"$HESHE", "你", "他", "她", "它"},
		{"$HIMHER", "你", "他", "她", "它"},
		{"$HISHER", "你的", "他的", "她的", "它的"},
		{"$HIMSELFHERSELF", "你自己", "他自己", "她自己", "它自己"}};
	
	if (monst == &player) {
		gender = 1;
    } else if (!canSeeMonster(monst)) {
        gender = 4;
	} else if (monst->info.flags & MONST_MALE) {
		gender = 2;
	} else if (monst->info.flags & MONST_FEMALE) {
		gender = 3;
	} else {
		gender = 4;
	}
	
	capitalize = false;
	
	for (insert = scan = text; *scan;) {
		if (scan[0] == '$') {
			for (pronounType=0; pronounType<4; pronounType++) {
				if (stringsMatch(pronouns[pronounType][0], scan)) {
					strcpy(insert, pronouns[pronounType][gender]);
					if (capitalize) {
						upperCase(insert);
						capitalize = false;
					}
					scan += strlen(pronouns[pronounType][0]);
					insert += strlen(pronouns[pronounType][gender]);
					break;
				}
			}
			if (pronounType == 4) {
				// Started with a '$' but didn't match an escape sequence; just copy the character and move on.
				*(insert++) = *(scan++);
			}
		} else if (scan[0] == COLOR_ESCAPE) {
			for (i=0; i<4; i++) {
				*(insert++) = *(scan++);
			}
		} else { // Didn't match any of the escape sequences; copy the character instead.
			if (*scan == '.') {
				capitalize = true;
			} else if (*scan != ' ') {
				capitalize = false;
			}

			*(insert++) = *(scan++);
		}
	}
	*insert = '\0';
}

// Pass 0 for summonerType for an ordinary selection.
short pickHordeType(short depth, enum monsterTypes summonerType, unsigned long forbiddenFlags, unsigned long requiredFlags) {
	short i, index, possCount = 0;
	
	if (depth <= 0) {
		depth = rogue.depthLevel;
	}
	
	for (i=0; i<NUMBER_HORDES; i++) {
		if (!(hordeCatalog[i].flags & forbiddenFlags)
			&& !(~(hordeCatalog[i].flags) & requiredFlags)
			&& ((!summonerType && hordeCatalog[i].minLevel <= depth && hordeCatalog[i].maxLevel >= depth)
				|| (summonerType && (hordeCatalog[i].flags & HORDE_IS_SUMMONED) && hordeCatalog[i].leaderType == summonerType))) {
				possCount += hordeCatalog[i].frequency;
			}
	}
	
	if (possCount == 0) {
		return -1;
	}
	
	index = rand_range(1, possCount);
	
	for (i=0; i<NUMBER_HORDES; i++) {
		if (!(hordeCatalog[i].flags & forbiddenFlags)
			&& !(~(hordeCatalog[i].flags) & requiredFlags)
			&& ((!summonerType && hordeCatalog[i].minLevel <= depth && hordeCatalog[i].maxLevel >= depth)
				|| (summonerType && (hordeCatalog[i].flags & HORDE_IS_SUMMONED) && hordeCatalog[i].leaderType == summonerType))) {
				if (index <= hordeCatalog[i].frequency) {
					return i;
				}
				index -= hordeCatalog[i].frequency;
			}
	}
	return 0; // should never happen
}

// If placeClone is false, the clone won't get a location
// and won't set any HAS_MONSTER flags or cause any refreshes;
// it's just generated and inserted into the chains.
creature *cloneMonster(creature *monst, boolean announce, boolean placeClone) {
	creature *newMonst, *nextMonst, *parentMonst;
	char buf[DCOLS*3], monstName[DCOLS*3];
	
	newMonst = generateMonster(monst->info.monsterID, false, false);
	nextMonst = newMonst->nextCreature;
	*newMonst = *monst; // boink!
	newMonst->nextCreature = nextMonst;
	
	if (monst->carriedMonster) {
		parentMonst = cloneMonster(monst->carriedMonster, false, false); // Also clone the carriedMonster
		removeMonsterFromChain(parentMonst, monsters);
		removeMonsterFromChain(parentMonst, dormantMonsters);
	} else {
		parentMonst = NULL;
	}

	initializeGender(newMonst);
	newMonst->bookkeepingFlags &= ~(MONST_LEADER | MONST_CAPTIVE | MONST_HAS_SOUL);
	newMonst->bookkeepingFlags |= MONST_FOLLOWER;
	newMonst->mapToMe = NULL;
	newMonst->safetyMap = NULL;
	newMonst->carriedItem = NULL;
	newMonst->carriedMonster = parentMonst;
	newMonst->ticksUntilTurn = 101;
    if (!(monst->creatureState == MONSTER_ALLY)) {
        newMonst->bookkeepingFlags &= MONST_TELEPATHICALLY_REVEALED;
    }
	if (monst->leader) {
		newMonst->leader = monst->leader;
	} else {
		newMonst->leader = monst;
		monst->bookkeepingFlags |= MONST_LEADER;
	}
    
    if (monst->bookkeepingFlags & MONST_CAPTIVE) {
        // If you clone a captive, the clone will be your ally.
        becomeAllyWith(newMonst);
    }
	
	if (placeClone) {
//		getQualifyingLocNear(loc, monst->xLoc, monst->yLoc, true, 0, forbiddenFlagsForMonster(&(monst->info)), (HAS_PLAYER | HAS_MONSTER), false, false);
//		newMonst->xLoc = loc[0];
//		newMonst->yLoc = loc[1];
        getQualifyingPathLocNear(&(newMonst->xLoc), &(newMonst->yLoc), monst->xLoc, monst->yLoc, true,
                                 T_DIVIDES_LEVEL & avoidedFlagsForMonster(&(newMonst->info)), HAS_PLAYER,
                                 avoidedFlagsForMonster(&(newMonst->info)), (HAS_PLAYER | HAS_MONSTER | HAS_UP_STAIRS | HAS_DOWN_STAIRS), false);
		pmap[newMonst->xLoc][newMonst->yLoc].flags |= HAS_MONSTER;
		refreshDungeonCell(newMonst->xLoc, newMonst->yLoc);
		if (announce && canSeeMonster(newMonst)) {
			monsterName(monstName, newMonst, false);
			sprintf(buf, "突然又出现了一个%s！", monstName);
			message(buf, false);
		}
	}
	
	if (monst == &player) { // Player managed to clone himself.
		newMonst->info.foreColor = &gray;
		newMonst->info.damage.lowerBound = 1;
		newMonst->info.damage.upperBound = 2;
		newMonst->info.damage.clumpFactor = 1;
		newMonst->info.defense = 0;
		strcpy(newMonst->info.monsterName, "的克隆");
		newMonst->creatureState = MONSTER_ALLY;
	}
	return newMonst;
}

unsigned long forbiddenFlagsForMonster(creatureType *monsterType) {
	unsigned long flags;
	
	flags = T_PATHING_BLOCKER;
	if (monsterType->flags & (MONST_IMMUNE_TO_FIRE | MONST_FLIES)) {
		flags &= ~T_LAVA_INSTA_DEATH;
	}
	if (monsterType->flags & MONST_IMMUNE_TO_FIRE) {
		flags &= ~(T_SPONTANEOUSLY_IGNITES | T_IS_FIRE);
	}
	if (monsterType->flags & (MONST_IMMUNE_TO_WATER | MONST_FLIES)) {
		flags &= ~T_IS_DEEP_WATER;
	}
	if (monsterType->flags & (MONST_FLIES)) {
		flags &= ~(T_AUTO_DESCENT | T_IS_DF_TRAP);
	}
	return flags;
}

unsigned long avoidedFlagsForMonster(creatureType *monsterType) {
	unsigned long flags;
	
	flags = forbiddenFlagsForMonster(monsterType) | T_HARMFUL_TERRAIN;
    
	if (monsterType->flags & MONST_INANIMATE) {
		flags &= ~(T_CAUSES_POISON | T_CAUSES_DAMAGE | T_CAUSES_EXPLOSIVE_DAMAGE | T_CAUSES_PARALYSIS | T_CAUSES_CONFUSION);
	}
	if (monsterType->flags & (MONST_IMMUNE_TO_FIRE)) {
		flags &= ~T_IS_FIRE;
	}
	if (monsterType->flags & (MONST_FLIES)) {
		flags &= ~(T_CAUSES_POISON);
	}
	return flags;
}

boolean monsterCanSubmergeNow(creature *monst) {
	return ((monst->info.flags & MONST_SUBMERGES)
			&& cellHasTMFlag(monst->xLoc, monst->yLoc, TM_ALLOWS_SUBMERGING)
			&& !(monst->bookkeepingFlags & (MONST_SEIZING | MONST_SEIZED))
			&& ((monst->info.flags & MONST_IMMUNE_TO_FIRE)
				|| monst->status[STATUS_IMMUNE_TO_FIRE]
				|| !cellHasTerrainFlag(monst->xLoc, monst->yLoc, T_LAVA_INSTA_DEATH)));
}

// Returns true if at least one minion spawned.
boolean spawnMinions(short hordeID, creature *leader, boolean summoned) {
	short iSpecies, iMember, count;
	unsigned long forbiddenTerrainFlags;
	hordeType *theHorde;
	creature *monst;
	short x, y;
	short failsafe;
	boolean atLeastOneMinion = false;
	
	x = leader->xLoc;
	y = leader->yLoc;
	
	theHorde = &hordeCatalog[hordeID];
	
	for (iSpecies = 0; iSpecies < theHorde->numberOfMemberTypes; iSpecies++) {
		count = randClump(theHorde->memberCount[iSpecies]);
		
		forbiddenTerrainFlags = forbiddenFlagsForMonster(&(monsterCatalog[theHorde->memberType[iSpecies]]));
		if (hordeCatalog[hordeID].spawnsIn) {
			forbiddenTerrainFlags &= ~(tileCatalog[hordeCatalog[hordeID].spawnsIn].flags);
		}
		
		for (iMember = 0; iMember < count; iMember++) {
			monst = generateMonster(theHorde->memberType[iSpecies], true, !summoned);
			failsafe = 0;
			do {
                getQualifyingPathLocNear(&(monst->xLoc), &(monst->yLoc), x, y, summoned,
                                         T_DIVIDES_LEVEL & forbiddenTerrainFlags, (HAS_PLAYER | HAS_UP_STAIRS | HAS_DOWN_STAIRS),
                                         forbiddenTerrainFlags, HAS_MONSTER, false);
			} while (theHorde->spawnsIn && !cellHasTerrainType(monst->xLoc, monst->yLoc, theHorde->spawnsIn) && failsafe++ < 20);
			if (failsafe >= 20) {
				// abort
				killCreature(monst, true);
				break;
			}
			if (monsterCanSubmergeNow(monst)) {
				monst->bookkeepingFlags |= MONST_SUBMERGED;
			}
			pmap[monst->xLoc][monst->yLoc].flags |= HAS_MONSTER;
			monst->bookkeepingFlags |= (MONST_FOLLOWER | MONST_JUST_SUMMONED);
			monst->leader = leader;
			monst->creatureState = leader->creatureState;
			monst->mapToMe = NULL;
			if (theHorde->flags & HORDE_DIES_ON_LEADER_DEATH) {
				monst->bookkeepingFlags |= MONST_BOUND_TO_LEADER;
			}
			if (hordeCatalog[hordeID].flags & HORDE_ALLIED_WITH_PLAYER) {
				becomeAllyWith(monst);
			}
			atLeastOneMinion = true;
		}
	}
	
	if (atLeastOneMinion && !(theHorde->flags & HORDE_DIES_ON_LEADER_DEATH)) {
		leader->bookkeepingFlags |= MONST_LEADER;
	}
	
	return atLeastOneMinion;
}

boolean drawManacle(short x, short y, enum directions dir) {
	enum tileType manacles[8] = {MANACLE_T, MANACLE_B, MANACLE_L, MANACLE_R, MANACLE_TL, MANACLE_BL, MANACLE_TR, MANACLE_BR};
	short newX = x + nbDirs[dir][0];
	short newY = y + nbDirs[dir][1];
	if (coordinatesAreInMap(newX, newY)
		&& pmap[newX][newY].layers[DUNGEON] == FLOOR
		&& pmap[newX][newY].layers[LIQUID] == NOTHING) {
		
		pmap[x + nbDirs[dir][0]][y + nbDirs[dir][1]].layers[SURFACE] = manacles[dir];
		return true;
	}
	return false;
}

void drawManacles(short x, short y) {
	enum directions fallback[4][3] = {{UPLEFT, UP, LEFT}, {DOWNLEFT, DOWN, LEFT}, {UPRIGHT, UP, RIGHT}, {DOWNRIGHT, DOWN, RIGHT}};
	short i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 3 && !drawManacle(x, y, fallback[i][j]); j++);
	}
}

// If hordeID is 0, it's randomly assigned based on the depth, with a 10% chance of an out-of-depth spawn from 1-5 levels deeper.
// If x is negative, location is random.
// Returns a pointer to the leader.
creature *spawnHorde(short hordeID, short x, short y, unsigned long forbiddenFlags, unsigned long requiredFlags) {
	short loc[2];
	short i, failsafe, depth;
	hordeType *theHorde;
	creature *leader;
    boolean tryAgain;
	
	if (rogue.depthLevel > 1 && rand_percent(10)) {
		depth = rogue.depthLevel + rand_range(1, min(5, rogue.depthLevel / 2));
		if (depth > AMULET_LEVEL) {
			depth = max(rogue.depthLevel, AMULET_LEVEL);
		}
        forbiddenFlags |= HORDE_NEVER_OOD;
	} else {
		depth = rogue.depthLevel;
	}
	
	if (hordeID <= 0) {
		failsafe = 50;
		do {
            tryAgain = false;
			hordeID = pickHordeType(depth, 0, forbiddenFlags, requiredFlags);
			if (hordeID < 0) {
				return NULL;
			}
            if (x >= 0 && y >= 0) {
                if (cellHasTerrainFlag(x, y, T_PATHING_BLOCKER)
                    && (!hordeCatalog[hordeID].spawnsIn || !cellHasTerrainType(x, y, hordeCatalog[hordeID].spawnsIn))) {
                    
                    // don't spawn a horde in special terrain unless it's meant to spawn there
                    tryAgain = true;
                }
                if (hordeCatalog[hordeID].spawnsIn && !cellHasTerrainType(x, y, hordeCatalog[hordeID].spawnsIn)) {
                    // don't spawn a horde on normal terrain if it's meant for special terrain
                    tryAgain = true;
                }
            }
		} while (--failsafe && tryAgain);
	}
	
//#ifdef BROGUE_ASSERTS
//	assert(!(x > 0 && y > 0 && hordeCatalog[hordeID].spawnsIn == DEEP_WATER && pmap[x][y].layers[LIQUID] != DEEP_WATER)); // Somtimes machines can trip this
//#endif
	
	failsafe = 50;
	
	if (x < 0 || y < 0) {
		i = 0;
		do {
			while (!randomMatchingLocation(&(loc[0]), &(loc[1]), FLOOR, NOTHING, (hordeCatalog[hordeID].spawnsIn ? hordeCatalog[hordeID].spawnsIn : -1))
				   || passableArcCount(loc[0], loc[1]) > 1) {
				if (!--failsafe) {
					return NULL;
				}
				hordeID = pickHordeType(depth, 0, forbiddenFlags, 0);
				
				if (hordeID < 0) {
					return NULL;
				}
			}
			x = loc[0];
			y = loc[1];
			i++;
			
			// This "while" condition should contain IN_FIELD_OF_VIEW, since that is specifically
			// calculated from the entry stairs when the level is generated, and will prevent monsters
			// from spawning within FOV of the entry stairs.
		} while (i < 25 && (pmap[x][y].flags & (ANY_KIND_OF_VISIBLE | IN_FIELD_OF_VIEW)));
	}
	
//	if (hordeCatalog[hordeID].spawnsIn == DEEP_WATER && pmap[x][y].layers[LIQUID] != DEEP_WATER) {
//		message("Waterborne monsters spawned on land!", true);
//	}
	
	theHorde = &hordeCatalog[hordeID];
	
	if (theHorde->machine > 0) {
		// Build the accompanying machine (e.g. a goblin encampment)
		buildAMachine(theHorde->machine, x, y, 0, NULL, NULL, NULL);
	}
	
	leader = generateMonster(theHorde->leaderType, true, true);
	leader->xLoc = x;
	leader->yLoc = y;
	
	if (hordeCatalog[hordeID].flags & HORDE_LEADER_CAPTIVE) {
		leader->bookkeepingFlags |= MONST_CAPTIVE;
		leader->creatureState = MONSTER_WANDERING;
		leader->currentHP = leader->info.maxHP / 4 + 1;
		
		// Draw the manacles unless the horde spawns in weird terrain (e.g. cages).
		if (!hordeCatalog[hordeID].spawnsIn) {
			drawManacles(x, y);
		}
	} else if (hordeCatalog[hordeID].flags & HORDE_ALLIED_WITH_PLAYER) {
		becomeAllyWith(leader);
	}
	
	pmap[x][y].flags |= HAS_MONSTER;
    if (playerCanSeeOrSense(x, y)) {
        refreshDungeonCell(x, y);
    }
	if (monsterCanSubmergeNow(leader)) {
		leader->bookkeepingFlags |= MONST_SUBMERGED;
	}
	
	spawnMinions(hordeID, leader, false);
	
	return leader;
}

boolean removeMonsterFromChain(creature *monst, creature *theChain) {
	creature *previousMonster;
	
	for (previousMonster = theChain;
		 previousMonster->nextCreature;
		 previousMonster = previousMonster->nextCreature) {
		if (previousMonster->nextCreature == monst) {
			previousMonster->nextCreature = monst->nextCreature;
			return true;
		}
	}
	return false;
}

boolean summonMinions(creature *summoner) {
	enum monsterTypes summonerType = summoner->info.monsterID;
	short hordeID = pickHordeType(0, summonerType, 0, 0), seenMinionCount = 0, x, y;
	boolean atLeastOneMinion = false;
	creature *monst, *host;
	char buf[DCOLS*3];
	char monstName[DCOLS*3];
    short **grid;
	
	if (hordeID < 0) {
		return false;
	}
    
    host = NULL;
	
	atLeastOneMinion = spawnMinions(hordeID, summoner, true);
	
    if (hordeCatalog[hordeID].flags & HORDE_SUMMONED_AT_DISTANCE) {
        // Create a grid where "1" denotes a valid summoning location: within DCOLS/2 pathing distance,
        // not in harmful terrain, and outside of the player's field of view.
        grid = allocGrid();
        fillGrid(grid, 0);
        calculateDistances(grid, summoner->xLoc, summoner->yLoc, T_PATHING_BLOCKER, NULL, true, true);
        findReplaceGrid(grid, 1, DCOLS/2, 1);
        findReplaceGrid(grid, 2, 30000, 0);
        getTerrainGrid(grid, 0, (T_PATHING_BLOCKER | T_HARMFUL_TERRAIN), (IN_FIELD_OF_VIEW | CLAIRVOYANT_VISIBLE | HAS_PLAYER | HAS_MONSTER));
    } else {
        grid = NULL;
    }
        
	for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		if (monst != summoner && monstersAreTeammates(monst, summoner)
			&& (monst->bookkeepingFlags & MONST_JUST_SUMMONED)) {
            
            if (hordeCatalog[hordeID].flags & HORDE_SUMMONED_AT_DISTANCE) {
                x = y = -1;
                randomLocationInGrid(grid, &x, &y, 1);
                teleport(monst, x, y, true);
                if (x != -1 && y != -1) {
                    grid[x][y] = 0;
                }
            }
			
			monst->bookkeepingFlags &= ~MONST_JUST_SUMMONED;
			if (canSeeMonster(monst)) {
				seenMinionCount++;
				refreshDungeonCell(monst->xLoc, monst->yLoc);
			}
			monst->ticksUntilTurn = 101;
			monst->leader = summoner;
			if (monst->carriedItem) {
				deleteItem(monst->carriedItem);
				monst->carriedItem = NULL;
			}
			host = monst;
		}
	}
	
	if (canSeeMonster(summoner)) {
		monsterName(monstName, summoner, true);
		if (monsterText[summoner->info.monsterID].summonMessage) {
			sprintf(buf, "%s%s", monstName, monsterText[summoner->info.monsterID].summonMessage);
		} else {
			sprintf(buf, "%s开始吟唱！", monstName);
		}
		message(buf, false);
	}
	
	if (atLeastOneMinion
		&& (summoner->info.abilityFlags & MA_ENTER_SUMMONS)
        && host) {
		
		host->carriedMonster = summoner;
		demoteMonsterFromLeadership(summoner);
		removeMonsterFromChain(summoner, monsters);
		pmap[summoner->xLoc][summoner->yLoc].flags &= ~HAS_MONSTER;
		refreshDungeonCell(summoner->xLoc, summoner->yLoc);
	}
    createFlare(summoner->xLoc, summoner->yLoc, SUMMONING_FLASH_LIGHT);
    
    if (grid) {
        freeGrid(grid);
    }
	
	return atLeastOneMinion;
}

// Generates and places monsters for the level.
void populateMonsters() {
	if (!MONSTERS_ENABLED) {
		return;
	}
	
	short i, numberOfMonsters = min(20, 6 + 3 * max(0, rogue.depthLevel - AMULET_LEVEL)); // almost always 6.
	
	while (rand_percent(60)) {
		numberOfMonsters++;
	}
	for (i=0; i<numberOfMonsters; i++) {
		spawnHorde(0, -1, -1, (HORDE_IS_SUMMONED | HORDE_MACHINE_ONLY), 0); // random horde type, random location
	}
}

void spawnPeriodicHorde() {
	creature *monst, *monst2;
	short x, y, **grid;
	
	if (!MONSTERS_ENABLED) {
		return;
	}
    
    grid = allocGrid();
    fillGrid(grid, 0);
    calculateDistances(grid, player.xLoc, player.yLoc, T_DIVIDES_LEVEL, NULL, true, true);
    getTerrainGrid(grid, 0, (T_PATHING_BLOCKER | T_HARMFUL_TERRAIN), (HAS_PLAYER | HAS_MONSTER | HAS_STAIRS | IN_FIELD_OF_VIEW));
    findReplaceGrid(grid, -30000, DCOLS/2-1, 0);
    findReplaceGrid(grid, 30000, 30000, 0);
    findReplaceGrid(grid, DCOLS/2, 30000-1, 1);
    randomLocationInGrid(grid, &x, &y, 1);
    if (x < 0 || y < 0) {
        fillGrid(grid, 1);
        getTerrainGrid(grid, 0, (T_PATHING_BLOCKER | T_HARMFUL_TERRAIN), (HAS_PLAYER | HAS_MONSTER | HAS_STAIRS | IN_FIELD_OF_VIEW | IS_IN_MACHINE));
        randomLocationInGrid(grid, &x, &y, 1);
        if (x < 0 || y < 0) {
            freeGrid(grid);
            return;
        }
    }
//    DEBUG {
//        dumpLevelToScreen();
//        hiliteGrid(grid, &orange, 50);
//        plotCharWithColor('X', mapToWindowX(x), mapToWindowY(y), &black, &white);
//        temporaryMessage("Periodic horde spawn location possibilities:", true);
//    }
    freeGrid(grid);
    
	monst = spawnHorde(0, x, y, (HORDE_IS_SUMMONED | HORDE_LEADER_CAPTIVE | HORDE_NO_PERIODIC_SPAWN | HORDE_MACHINE_ONLY), 0);
	if (monst) {
        monst->creatureState = MONSTER_WANDERING;
        for (monst2 = monsters->nextCreature; monst2 != NULL; monst2 = monst2->nextCreature) {
            if (monst2->leader == monst) {
                monst2->creatureState = MONSTER_WANDERING;
            }
        }
	}
}

// x and y are optional.
void teleport(creature *monst, short x, short y, boolean respectTerrainAvoidancePreferences) {
	short **grid, i, j;
	char monstFOV[DCOLS][DROWS];
	
    if (!coordinatesAreInMap(x, y)) {
        zeroOutGrid(monstFOV);
        getFOVMask(monstFOV, monst->xLoc, monst->yLoc, DCOLS, T_OBSTRUCTS_VISION, 0, false);
        grid = allocGrid();
        fillGrid(grid, 0);
        calculateDistances(grid, monst->xLoc, monst->yLoc, forbiddenFlagsForMonster(&(monst->info)) & T_DIVIDES_LEVEL, NULL, true, false);
        findReplaceGrid(grid, -30000, DCOLS/2, 0);
        findReplaceGrid(grid, 2, 30000, 1);
        if (validLocationCount(grid, 1) < 1) {
            fillGrid(grid, 1);
        }
        if (respectTerrainAvoidancePreferences) {
            if (monst->info.flags & MONST_RESTRICTED_TO_LIQUID) {
                fillGrid(grid, 0);
                getTMGrid(grid, 1, TM_ALLOWS_SUBMERGING);
            }
            getTerrainGrid(grid, 0, avoidedFlagsForMonster(&(monst->info)), (IS_IN_MACHINE | HAS_PLAYER | HAS_MONSTER | HAS_STAIRS));
        } else {
            getTerrainGrid(grid, 0, forbiddenFlagsForMonster(&(monst->info)), (IS_IN_MACHINE | HAS_PLAYER | HAS_MONSTER | HAS_STAIRS));
        }
        for (i=0; i<DCOLS; i++) {
            for (j=0; j<DROWS; j++) {
                if (monstFOV[i][j]) {
                    grid[i][j] = 0;
                }
            }
        }
        randomLocationInGrid(grid, &x, &y, 1);
//        DEBUG {
//            dumpLevelToScreen();
//            hiliteGrid(grid, &orange, 50);
//            plotCharWithColor('X', mapToWindowX(x), mapToWindowY(y), &white, &red);
//            temporaryMessage("Teleport candidate locations:", true);
//        }
        freeGrid(grid);
        if (x < 0 || y < 0) {
            return; // Failure!
        }
    }
    monst->bookkeepingFlags &= ~(MONST_SEIZED | MONST_SEIZING);
	if (monst == &player) {
		pmap[player.xLoc][player.yLoc].flags &= ~HAS_PLAYER;
		refreshDungeonCell(player.xLoc, player.yLoc);
		monst->xLoc = x;
		monst->yLoc = y;
		pmap[player.xLoc][player.yLoc].flags |= HAS_PLAYER;
		updateVision(true);
		// get any items at the destination location
		if (pmap[player.xLoc][player.yLoc].flags & HAS_ITEM) {
			pickUpItemAt(player.xLoc, player.yLoc);
		}
	} else {
		pmap[monst->xLoc][monst->yLoc].flags &= ~HAS_MONSTER;
		refreshDungeonCell(monst->xLoc, monst->yLoc);
		monst->xLoc = x;
		monst->yLoc = y;
		pmap[monst->xLoc][monst->yLoc].flags |= HAS_MONSTER;
		chooseNewWanderDestination(monst);
	}
	refreshDungeonCell(monst->xLoc, monst->yLoc);
}

boolean isValidWanderDestination(creature *monst, short wpIndex) {
    return (wpIndex >= 0
            && wpIndex < rogue.wpCount
            && !monst->waypointAlreadyVisited[wpIndex]
            && rogue.wpDistance[wpIndex][monst->xLoc][monst->yLoc] >= 0
            && rogue.wpDistance[wpIndex][monst->xLoc][monst->yLoc] < DCOLS/2
            && nextStep(rogue.wpDistance[wpIndex], monst->xLoc, monst->yLoc, monst, false) != NO_DIRECTION);
}

short closestWaypointIndex(creature *monst) {
    short i, closestDistance, closestIndex;
    
    closestDistance = 30000;
    closestIndex = -1;
    for (i=0; i < rogue.wpCount; i++) {
        if (isValidWanderDestination(monst, i)
            && rogue.wpDistance[i][monst->xLoc][monst->yLoc] < closestDistance) {
            
            closestDistance = rogue.wpDistance[i][monst->xLoc][monst->yLoc];
            closestIndex = i;
        }
    }
    return closestIndex;
}

void chooseNewWanderDestination(creature *monst) {
    short i;
    
#ifdef BROGUE_ASSERTS
    assert(monst->targetWaypointIndex < MAX_WAYPOINT_COUNT);
    assert(rogue.wpCount > 0 && rogue.wpCount <= MAX_WAYPOINT_COUNT);
#endif
    
    // Set two checkpoints at random to false (which equilibrates to 50% of checkpoints being active).
    monst->waypointAlreadyVisited[rand_range(0, rogue.wpCount - 1)] = false;
    monst->waypointAlreadyVisited[rand_range(0, rogue.wpCount - 1)] = false;
    // Set the targeted checkpoint to true.
    if (monst->targetWaypointIndex >= 0) {
        monst->waypointAlreadyVisited[monst->targetWaypointIndex] = true;
    }
    
    monst->targetWaypointIndex = closestWaypointIndex(monst); // Will be -1 if no waypoints were available.
    if (monst->targetWaypointIndex == -1) {
        for (i=0; i < rogue.wpCount; i++) {
            monst->waypointAlreadyVisited[i] = 0;
        }
        monst->targetWaypointIndex = closestWaypointIndex(monst);
    }
}

enum subseqDFTypes {
	SUBSEQ_PROMOTE = 0,
	SUBSEQ_BURN,
    SUBSEQ_DISCOVER,
};

// Returns the terrain flags of this tile after it's promoted according to the event corresponding to subseqDFTypes.
unsigned long successorTerrainFlags(enum tileType tile, enum subseqDFTypes promotionType) {
    enum dungeonFeatureTypes DF = 0;
    
    switch (promotionType) {
        case SUBSEQ_PROMOTE:
            DF = tileCatalog[tile].promoteType;
            break;
        case SUBSEQ_BURN:
            DF = tileCatalog[tile].fireType;
            break;
        case SUBSEQ_DISCOVER:
            DF = tileCatalog[tile].discoverType;
            break;
        default:
            break;
    }
    
    if (DF) {
        return tileCatalog[dungeonFeatureCatalog[DF].tile].flags;
    } else {
        return 0;
    }
}

unsigned long burnedTerrainFlagsAtLoc(short x, short y) {
    short layer;
    unsigned long flags = 0;
    
    for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
        if (tileCatalog[pmap[x][y].layers[layer]].flags & T_IS_FLAMMABLE) {
            flags |= successorTerrainFlags(pmap[x][y].layers[layer], SUBSEQ_BURN);
            if (tileCatalog[pmap[x][y].layers[layer]].mechFlags & TM_EXPLOSIVE_PROMOTE) {
                flags |= successorTerrainFlags(pmap[x][y].layers[layer], SUBSEQ_PROMOTE);
            }
        }
    }
    
    return flags;
}

unsigned long discoveredTerrainFlagsAtLoc(short x, short y) {
    short layer;
    unsigned long flags = 0;
    
    for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
        if (tileCatalog[pmap[x][y].layers[layer]].mechFlags & TM_IS_SECRET) {
            flags |= successorTerrainFlags(pmap[x][y].layers[layer], SUBSEQ_DISCOVER);
        }
    }
    
    return flags;
}

boolean monsterAvoids(creature *monst, short x, short y) {
    unsigned long terrainImmunities;
	creature *defender;
	
	// everyone avoids the stairs
	if ((x == rogue.downLoc[0] && y == rogue.downLoc[1])
		|| (x == rogue.upLoc[0] && y == rogue.upLoc[1])) {
		
		return true;
	}
	
	// dry land
	if (monst->info.flags & MONST_RESTRICTED_TO_LIQUID
		&& !cellHasTMFlag(x, y, TM_ALLOWS_SUBMERGING)) {
		return true;
	}
	
	// non-allied monsters can always attack the player
	if (player.xLoc == x && player.yLoc == y && monst != &player && monst->creatureState != MONSTER_ALLY) {
		return false;
	}
	
	// walls
	if (cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY)) {
        if (monst != &player
            && cellHasTMFlag(x, y, TM_IS_SECRET)
            && !(discoveredTerrainFlagsAtLoc(x, y) & avoidedFlagsForMonster(&(monst->info)))) {
            // This is so monsters can use secret doors but won't embed themselves in secret levers.
            return false;
        }
        if (distanceBetween(monst->xLoc, monst->yLoc, x, y) <= 1) {
            defender = monsterAtLoc(x, y);
            if (defender
                && (defender->info.flags & MONST_ATTACKABLE_THRU_WALLS)) {
                return false;
            }
        }
		return true;
	}
    
    // monsters can always attack unfriendly neighboring monsters, unless the monster is aquatic and the neighbor is flying.
    if (distanceBetween(monst->xLoc, monst->yLoc, x, y) <= 1) {
        defender = monsterAtLoc(x, y);
        if (defender
            && monstersAreEnemies(monst, defender)
            && (!(monst->info.flags & MONST_RESTRICTED_TO_LIQUID) || !(defender->status[STATUS_LEVITATING]))) {
            
            return false;
        }
    }
	
	// hidden terrain
	if (cellHasTMFlag(x, y, TM_IS_SECRET) && monst == &player) {
		return false; // player won't avoid what he doesn't know about
	}
    
    // Determine invulnerabilities based only on monster characteristics.
    terrainImmunities = 0;
    if (monst->status[STATUS_IMMUNE_TO_FIRE]) {
        terrainImmunities |= (T_IS_FIRE | T_SPONTANEOUSLY_IGNITES | T_LAVA_INSTA_DEATH);
    }
    if (monst->info.flags & MONST_INANIMATE) {
        terrainImmunities |= (T_CAUSES_DAMAGE | T_CAUSES_PARALYSIS | T_CAUSES_CONFUSION | T_CAUSES_NAUSEA | T_CAUSES_POISON);
    }
    if (monst->status[STATUS_LEVITATING]) {
        terrainImmunities |= (T_AUTO_DESCENT | T_CAUSES_POISON | T_IS_DEEP_WATER | T_IS_DF_TRAP | T_LAVA_INSTA_DEATH);
    }
    if (monst->info.flags & MONST_IMMUNE_TO_WEBS) {
        terrainImmunities |= T_ENTANGLES;
    }
    if (monst->info.flags & MONST_IMMUNE_TO_WATER) {
        terrainImmunities |= T_IS_DEEP_WATER;
    }
    if (monst == &player
        && rogue.armor
        && (rogue.armor->flags & ITEM_RUNIC)
        && rogue.armor->enchant2 == A_RESPIRATION) {
        
        terrainImmunities |= T_RESPIRATION_IMMUNITIES;
    }
	
	// brimstone
	if (!(monst->status[STATUS_IMMUNE_TO_FIRE]) && cellHasTerrainFlag(x, y, (T_SPONTANEOUSLY_IGNITES))
		&& !(pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER))
		&& !cellHasTerrainFlag(monst->xLoc, monst->yLoc, T_IS_FIRE | T_SPONTANEOUSLY_IGNITES)
		&& (monst == &player || (monst->creatureState != MONSTER_TRACKING_SCENT && monst->creatureState != MONSTER_FLEEING))) {
		return true;
	}
	
	// burning wandering monsters avoid flammable terrain out of common courtesy
	if (monst != &player
		&& monst->creatureState == MONSTER_WANDERING
		&& (monst->info.flags & MONST_FIERY)
		&& (cellHasTerrainFlag(x, y, T_IS_FLAMMABLE))) {
        
		return true;
	}
    
    // burning monsters avoid explosive terrain and steam-emitting terrain
    if (monst != &player
        && monst->status[STATUS_BURNING]
        && (burnedTerrainFlagsAtLoc(x, y) & (T_CAUSES_EXPLOSIVE_DAMAGE | T_CAUSES_DAMAGE))) {
        
        return true;
    }
	
	// fire
	if (cellHasTerrainFlag(x, y, T_IS_FIRE & ~terrainImmunities)
		&& !cellHasTerrainFlag(monst->xLoc, monst->yLoc, T_IS_FIRE)
		&& !(pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER))
		&& (monst != &player || rogue.mapToShore[x][y] >= player.status[STATUS_IMMUNE_TO_FIRE])) {
		return true;
	}
	
	// non-fire harmful terrain
	if (cellHasTerrainFlag(x, y, (T_HARMFUL_TERRAIN & ~T_IS_FIRE & ~terrainImmunities))
		&& !cellHasTerrainFlag(monst->xLoc, monst->yLoc, (T_HARMFUL_TERRAIN & ~T_IS_FIRE))) {
		return true;
	}
    
    // chasms or trap doors
    if (cellHasTerrainFlag(x, y, T_AUTO_DESCENT & ~terrainImmunities)
        && (!cellHasTerrainFlag(x, y, T_ENTANGLES) || !(monst->info.flags & MONST_IMMUNE_TO_WEBS))) {
        return true;
    }
    
    // gas or other environmental traps
    if (cellHasTerrainFlag(x, y, T_IS_DF_TRAP & ~terrainImmunities)
        && !(pmap[x][y].flags & PRESSURE_PLATE_DEPRESSED)
        && (monst == &player || monst->creatureState == MONSTER_WANDERING
            || (monst->creatureState == MONSTER_ALLY && !(cellHasTMFlag(x, y, TM_IS_SECRET))))
        && !(monst->status[STATUS_ENTRANCED])
        && (!cellHasTerrainFlag(x, y, T_ENTANGLES) || !(monst->info.flags & MONST_IMMUNE_TO_WEBS))) {
        return true;
    }
    
    // lava
    if (cellHasTerrainFlag(x, y, T_LAVA_INSTA_DEATH & ~terrainImmunities)
        && (!cellHasTerrainFlag(x, y, T_ENTANGLES) || !(monst->info.flags & MONST_IMMUNE_TO_WEBS))
        && (monst != &player || rogue.mapToShore[x][y] >= max(player.status[STATUS_IMMUNE_TO_FIRE], player.status[STATUS_LEVITATING]))) {
        return true;
    }
    
    // deep water
    if (cellHasTerrainFlag(x, y, T_IS_DEEP_WATER & ~terrainImmunities)
        && (!cellHasTerrainFlag(x, y, T_ENTANGLES) || !(monst->info.flags & MONST_IMMUNE_TO_WEBS))
        && !cellHasTerrainFlag(monst->xLoc, monst->yLoc, T_IS_DEEP_WATER)) {
        return true; // avoid only if not already in it
    }
    
    // poisonous lichen
    if (cellHasTerrainFlag(x, y, T_CAUSES_POISON & ~terrainImmunities)
        && !cellHasTerrainFlag(monst->xLoc, monst->yLoc, T_CAUSES_POISON)
        && (monst == &player || monst->creatureState != MONSTER_TRACKING_SCENT || monst->currentHP < 10)) {
        return true;
    }
	return false;
}

boolean moveMonsterPassivelyTowards(creature *monst, short targetLoc[2], boolean willingToAttackPlayer) {
	short x, y, dx, dy, newX, newY;
	
	x = monst->xLoc;
	y = monst->yLoc;
	
	if (targetLoc[0] == x) {
		dx = 0;
	} else {
		dx = (targetLoc[0] < x ? -1 : 1);
	}
	if (targetLoc[1] == y) {
		dy = 0;
	} else {
		dy = (targetLoc[1] < y ? -1 : 1);
	}
	
	if (dx == 0 && dy == 0) { // already at the destination
		return false;
	}
	
	newX = x + dx;
	newY = y + dy;
	
	if (!coordinatesAreInMap(newX, newY)) {
		return false;
	}
	
	if (monst->creatureState != MONSTER_TRACKING_SCENT && dx && dy) {
		if (abs(targetLoc[0] - x) > abs(targetLoc[1] - y) && rand_range(0, abs(targetLoc[0] - x)) > abs(targetLoc[1] - y)) {
			if (!(monsterAvoids(monst, newX, y) || (!willingToAttackPlayer && (pmap[newX][y].flags & HAS_PLAYER)) || !moveMonster(monst, dx, 0))) {
				return true;
			}
		} else if (abs(targetLoc[0] - x) < abs(targetLoc[1] - y) && rand_range(0, abs(targetLoc[1] - y)) > abs(targetLoc[0] - x)) {
			if (!(monsterAvoids(monst, x, newY) || (!willingToAttackPlayer && (pmap[x][newY].flags & HAS_PLAYER)) || !moveMonster(monst, 0, dy))) {
				return true;
			}
		}
	}
	
	// Try to move toward the goal diagonally if possible or else straight.
	// If that fails, try both directions for the shorter coordinate.
	// If they all fail, return false.
	if (monsterAvoids(monst, newX, newY) || (!willingToAttackPlayer && (pmap[newX][newY].flags & HAS_PLAYER)) || !moveMonster(monst, dx, dy)) {
		if (distanceBetween(x, y, targetLoc[0], targetLoc[1]) <= 1 && (dx == 0 || dy == 0)) { // cardinally adjacent
			return false; // destination is blocked
		}
		//abs(targetLoc[0] - x) < abs(targetLoc[1] - y)
		if ((max(targetLoc[0], x) - min(targetLoc[0], x)) < (max(targetLoc[1], y) - min(targetLoc[1], y))) {
			if (monsterAvoids(monst, x, newY) || (!willingToAttackPlayer && pmap[x][newY].flags & HAS_PLAYER) || !moveMonster(monst, 0, dy)) {
				if (monsterAvoids(monst, newX, y) || (!willingToAttackPlayer &&  pmap[newX][y].flags & HAS_PLAYER) || !moveMonster(monst, dx, 0)) {
					if (monsterAvoids(monst, x-1, newY) || (!willingToAttackPlayer && pmap[x-1][newY].flags & HAS_PLAYER) || !moveMonster(monst, -1, dy)) {
						if (monsterAvoids(monst, x+1, newY) || (!willingToAttackPlayer && pmap[x+1][newY].flags & HAS_PLAYER) || !moveMonster(monst, 1, dy)) {
							return false;
						}
					}
				}
			}
		} else {
			if (monsterAvoids(monst, newX, y) || (!willingToAttackPlayer && pmap[newX][y].flags & HAS_PLAYER) || !moveMonster(monst, dx, 0)) {
				if (monsterAvoids(monst, x, newY) || (!willingToAttackPlayer && pmap[x][newY].flags & HAS_PLAYER) || !moveMonster(monst, 0, dy)) {
					if (monsterAvoids(monst, newX, y-1) || (!willingToAttackPlayer && pmap[newX][y-1].flags & HAS_PLAYER) || !moveMonster(monst, dx, -1)) {
						if (monsterAvoids(monst, newX, y+1) || (!willingToAttackPlayer && pmap[newX][y+1].flags & HAS_PLAYER) || !moveMonster(monst, dx, 1)) {
							return false;
						}
					}
				}
			}
		}
	}
	return true;
}

short distanceBetween(short x1, short y1, short x2, short y2) {
	return max(abs(x1 - x2), abs(y1 - y2));
}

void wakeUp(creature *monst) {
	creature *teammate;
	
	if (monst->creatureState != MONSTER_ALLY) {
		monst->creatureState =
		(monst->creatureMode == MODE_PERM_FLEEING ? MONSTER_FLEEING : MONSTER_TRACKING_SCENT);
        updateMonsterState(monst);
	}
	monst->ticksUntilTurn = 100;
	
	for (teammate = monsters->nextCreature; teammate != NULL; teammate = teammate->nextCreature) {
		if (monst != teammate && monstersAreTeammates(monst, teammate) && teammate->creatureMode == MODE_NORMAL) {
			if (monst->creatureState != MONSTER_ALLY) {
				teammate->creatureState =
				(teammate->creatureMode == MODE_PERM_FLEEING ? MONSTER_FLEEING : MONSTER_TRACKING_SCENT);
                updateMonsterState(teammate);
			}
			teammate->ticksUntilTurn = 100;
		}
	}
}

// Assumes that observer is not the player.
short awarenessDistance(creature *observer, creature *target) {
	long perceivedDistance, bonus = 0;
	
	// start with base distance
	
	if ((observer->status[STATUS_LEVITATING] || (observer->info.flags & MONST_RESTRICTED_TO_LIQUID) || (observer->bookkeepingFlags & MONST_SUBMERGED)
		 || ((observer->info.flags & MONST_IMMUNE_TO_WEBS) && (observer->info.abilityFlags & MA_SHOOTS_WEBS)))
		&& ((target == &player && (pmap[observer->xLoc][observer->yLoc].flags & IN_FIELD_OF_VIEW)) ||
			(target != &player && openPathBetween(observer->xLoc, observer->yLoc, target->xLoc, target->yLoc)))) {
			// if monster flies or is waterbound or is underwater or can cross pits with webs:
			perceivedDistance = distanceBetween(observer->xLoc, observer->yLoc, target->xLoc, target->yLoc) * 3;
		} else {
			perceivedDistance = (rogue.scentTurnNumber - scentMap[observer->xLoc][observer->yLoc]); // this value is triple the apparent distance
		}
	
	perceivedDistance = min(perceivedDistance, 1000);
	
	if (perceivedDistance < 0) {
		perceivedDistance = 1000;
	}
	
	// calculate bonus modifiers
    
    if (target->status[STATUS_INVISIBLE]) {
        bonus += 10;
    }
	
	if ((target != &player
		 && tmap[target->xLoc][target->yLoc].light[0] < 0
		 && tmap[target->xLoc][target->yLoc].light[1] < 0
		 && tmap[target->xLoc][target->yLoc].light[2] < 0)
		|| (target == &player && playerInDarkness())) {
		
		// super-darkness
		bonus += 5;
	}
	if (observer->creatureState == MONSTER_SLEEPING) {
		bonus += 3;
	}
	
	if (target == &player) {
		bonus += rogue.stealthBonus;
		if (rogue.justRested) {
			bonus = (bonus + 1) * 2;
		}
		if (observer->creatureState == MONSTER_TRACKING_SCENT) {
			bonus -= 4;
		}
	}
	if (pmap[target->xLoc][target->yLoc].flags & IS_IN_SHADOW
		|| target == &player && playerInDarkness()) {
		bonus = (bonus + 1) * 2;
	}
	
	// apply bonus -- each marginal point increases perceived distance by a compounding 10%
	perceivedDistance *= pow(1.1, bonus);
	if (perceivedDistance < 0 || perceivedDistance > 10000) {
		return 10000;
	}
	return ((short) perceivedDistance);
}

// yes or no -- observer is aware of the target as of this new turn.
// takes into account whether it is ALREADY aware of the target.
boolean awareOfTarget(creature *observer, creature *target) {
	short perceivedDistance = awarenessDistance(observer, target);
	short awareness = observer->info.scentThreshold * 3; // forget sight, it sucks
    boolean retval;
    
#ifdef BROGUE_ASSERTS
    assert(perceivedDistance >= 0 && awareness >= 0);
#endif
	
	if (perceivedDistance > awareness) {
		// out of awareness range
		retval = false;
	} else if (observer->creatureState == MONSTER_TRACKING_SCENT) {
		// already aware of the target
		retval = true;
	} else if (target == &player
		&& !(pmap[observer->xLoc][observer->yLoc].flags & IN_FIELD_OF_VIEW)) {
		// observer not hunting and player-target not in field of view
		retval = false;
	} else {
	// within range but currently unaware
        retval = ((rand_range(0, perceivedDistance) == 0) ? true : false);
    }
    return retval;
}

void updateMonsterState(creature *monst) {
	short x, y, maximumInvisibilityDetectionRadius, shortestDistanceToEnemy;
	boolean awareOfPlayer, lostToInvisibility;
	//char buf[DCOLS*3], monstName[DCOLS];
    creature *monst2;
	
	if ((monst->info.flags & MONST_ALWAYS_HUNTING)
        && monst->creatureState != MONSTER_ALLY) {
        
		monst->creatureState = MONSTER_TRACKING_SCENT;
		return;
	}
    
    if (player.status[STATUS_INVISIBLE]) {
        lostToInvisibility = true;
        if (monst->creatureState == MONSTER_TRACKING_SCENT) {
            maximumInvisibilityDetectionRadius = 2;
        } else {
            maximumInvisibilityDetectionRadius = 1;
        }
        CYCLE_MONSTERS_AND_PLAYERS(monst2) {
            if ((monst2 == &player || monstersAreEnemies(monst, monst2))
                && (monst2 != &player || (pmap[player.xLoc][player.yLoc].flags & IN_FIELD_OF_VIEW))
                && monst != monst2
                && distanceBetween(monst->xLoc, monst->yLoc, monst2->xLoc, monst2->yLoc) <= maximumInvisibilityDetectionRadius) {
                
                lostToInvisibility = false;
                break;
            }
        }
    } else {
        lostToInvisibility = false;
    }
	
	x = monst->xLoc;
	y = monst->yLoc;
	
	awareOfPlayer = awareOfTarget(monst, &player);
	
	if (monst->creatureMode == MODE_PERM_FLEEING
		&& (monst->creatureState == MONSTER_WANDERING || monst->creatureState == MONSTER_TRACKING_SCENT)) {
        
		monst->creatureState = MONSTER_FLEEING;
	}
    
    shortestDistanceToEnemy = DCOLS+DROWS;
    if (monst->info.flags & MONST_MAINTAINS_DISTANCE) {
        CYCLE_MONSTERS_AND_PLAYERS(monst2) {
            if (monstersAreEnemies(monst, monst2)
                && distanceBetween(x, y, monst2->xLoc, monst2->yLoc) < shortestDistanceToEnemy
                && traversiblePathBetween(monst2, x, y)
                && openPathBetween(x, y, monst2->xLoc, monst2->yLoc)) {
                
                shortestDistanceToEnemy = distanceBetween(x, y, monst2->xLoc, monst2->yLoc);
            }
        }
    }
	
	if ((monst->creatureState == MONSTER_WANDERING)
        && awareOfPlayer
        && !lostToInvisibility
        && (pmap[player.xLoc][player.yLoc].flags & IN_FIELD_OF_VIEW)) {
        
		// If wandering, but the scent is stronger than the scent detection threshold, start tracking the scent.
		monst->creatureState = MONSTER_TRACKING_SCENT;
	} else if (monst->creatureState == MONSTER_SLEEPING) {
		// if sleeping, the monster has a chance to awaken
		
		if (awareOfPlayer) {
			wakeUp(monst); // wakes up the whole horde if necessary
			
//			if (canSeeMonster(monst)) {
//				monsterName(monstName, monst, true);
//				sprintf(buf, "%s awakens!", monstName);
//				combatMessage(buf, 0);
//			}
		}
	} else if (monst->creatureState == MONSTER_TRACKING_SCENT && (!awareOfPlayer || lostToInvisibility)) {
		// if tracking scent, but the scent is weaker than the scent detection threshold, begin wandering.
		monst->creatureState = MONSTER_WANDERING;
		chooseNewWanderDestination(monst);
	} else if (monst->creatureState == MONSTER_TRACKING_SCENT
			   && (monst->info.flags & MONST_MAINTAINS_DISTANCE)
			   && shortestDistanceToEnemy < 3) {
		monst->creatureState = MONSTER_FLEEING;
	} else if (monst->creatureMode == MODE_NORMAL
			   && monst->creatureState == MONSTER_FLEEING
			   && (monst->info.flags & MONST_MAINTAINS_DISTANCE)
			   && !(monst->status[STATUS_MAGICAL_FEAR])
			   && shortestDistanceToEnemy >= 3) {
		monst->creatureState = MONSTER_TRACKING_SCENT;
	} else if (monst->creatureMode == MODE_PERM_FLEEING
			   && monst->creatureState == MONSTER_FLEEING
			   && (monst->info.abilityFlags & MA_HIT_STEAL_FLEE)
			   && !(monst->status[STATUS_MAGICAL_FEAR])
			   && !(monst->carriedItem)) {
		monst->creatureState = MONSTER_TRACKING_SCENT;
		monst->creatureMode = MODE_NORMAL;
	} else if (monst->creatureMode == MODE_NORMAL
			   && monst->creatureState == MONSTER_FLEEING
			   && (monst->info.flags & MONST_FLEES_NEAR_DEATH)
			   && !(monst->status[STATUS_MAGICAL_FEAR])
			   && monst->currentHP >= monst->info.maxHP * 3 / 4) {
		monst->creatureState = (((monst->bookkeepingFlags & MONST_FOLLOWER) && monst->leader == &player)
								? MONSTER_ALLY : MONSTER_TRACKING_SCENT);
	}
}

void decrementMonsterStatus(creature *monst) {
	short i, damage;
	char buf[COLS*3], buf2[COLS*3];
	
	monst->bookkeepingFlags &= ~MONST_JUST_SUMMONED;
	
	if (monst->currentHP < monst->info.maxHP && monst->info.turnsBetweenRegen > 0 && !monst->status[STATUS_POISONED]) {
		if ((monst->turnsUntilRegen -= 1000) <= 0) {
			monst->currentHP++;
			monst->turnsUntilRegen += monst->info.turnsBetweenRegen;
		}
	}
	
	for (i=0; i<NUMBER_OF_STATUS_EFFECTS; i++) {
        switch (i) {
            case STATUS_LEVITATING:
                if (monst->status[i] && !(monst->info.flags & MONST_FLIES)) {
                    monst->status[i]--;
                }
                break;
            case STATUS_SLOWED:
                if (monst->status[i] && !--monst->status[i]) {
                    monst->movementSpeed = monst->info.movementSpeed;
                    monst->attackSpeed = monst->info.attackSpeed;
                }
                break;
            case STATUS_WEAKENED:
                if (monst->status[i] && !--monst->status[i]) {
                    monst->weaknessAmount = 0;
                }
                break;
            case STATUS_HASTED:
                if (monst->status[i]) {
                    if (!--monst->status[i]) {
                        monst->movementSpeed = monst->info.movementSpeed;
                        monst->attackSpeed = monst->info.attackSpeed;
                    }
                }
                break;
            case STATUS_BURNING:
                if (monst->status[i]) {
                    if (!(monst->info.flags & MONST_FIERY)) {
                        monst->status[i]--;
                    }
                    damage = rand_range(1, 3);
                    if (!(monst->status[STATUS_IMMUNE_TO_FIRE]) && inflictDamage(monst, damage, &orange)) {
                        if (canSeeMonster(monst)) {
                            monsterName(buf, monst, true);
                            sprintf(buf2, "%s被%s。",
                                    buf,
                                    (monst->info.flags & MONST_INANIMATE) ? "尽了" : "死了");
                            messageWithColor(buf2, messageColorFromVictim(monst), false);
                        }
                        return;
                    }
                    if (monst->status[i] <= 0) {
                        extinguishFireOnCreature(monst);
                    }
                }
                break;
            case STATUS_LIFESPAN_REMAINING:
                if (monst->status[i]) {
                    monst->status[i]--;
                    if (monst->status[i] <= 0) {
                        killCreature(monst, false);
                        if (canSeeMonster(monst)) {
                            monsterName(buf, monst, true);
                            sprintf(buf2, "%s瞬间化成粉尘，消散在了空中。", buf);
                            messageWithColor(buf2, &white, false);
                        }
                        return;
                    }
                }
                break;
            case STATUS_POISONED:
                if (monst->status[i]) {
                    monst->status[i]--;
                    if (inflictDamage(monst, 1, &green)) {
                        if (canSeeMonster(monst)) {
                            monsterName(buf, monst, true);
                            sprintf(buf2, "%s被毒死了。", buf);
                            messageWithColor(buf2, messageColorFromVictim(monst), false);
                        }
                        return;
                    }
                }
                break;
            case STATUS_STUCK:
                if (monst->status[i] && !cellHasTerrainFlag(monst->xLoc, monst->yLoc, T_ENTANGLES)) {
                    monst->status[i] = 0;
                }
                if (monst->status[i]) {
                    if (!--monst->status[i]) {
                        if (tileCatalog[pmap[monst->xLoc][monst->yLoc].layers[SURFACE]].flags & T_ENTANGLES) {
                            pmap[monst->xLoc][monst->yLoc].layers[SURFACE] = NOTHING;
                        }
                    }
                }
                break;
            case STATUS_MAGICAL_FEAR:
                if (monst->status[i]) {
                    if (!--monst->status[i]) {
                        monst->creatureState = (monst->leader == &player ? MONSTER_ALLY : MONSTER_TRACKING_SCENT);
                    }
                }
                break;
            case STATUS_SHIELDED:
                monst->status[i] -= monst->maxStatus[i] / 20;
                if (monst->status[i] <= 0) {
                    monst->status[i] = monst->maxStatus[i] = 0;
                }
                break;
            case STATUS_IMMUNE_TO_FIRE:
                if (monst->status[i] && !(monst->info.flags & MONST_IMMUNE_TO_FIRE)) {	
                    monst->status[i]--;
                }
                break;
            case STATUS_INVISIBLE:
                if (monst->status[i]
                    && !(monst->info.flags & MONST_INVISIBLE)
                    && !--monst->status[i]
                    && playerCanSee(monst->xLoc, monst->yLoc)) {
                    
                    refreshDungeonCell(monst->xLoc, monst->yLoc);
                }
                break;
            default:
                if (monst->status[i]) {
                    monst->status[i]--;
                }
                break;
		}
	}
	
	if (monsterCanSubmergeNow(monst) && !(monst->bookkeepingFlags & MONST_SUBMERGED)) {
		if (rand_percent(20)) {
			monst->bookkeepingFlags |= MONST_SUBMERGED;
			if (!monst->status[STATUS_MAGICAL_FEAR]
                && monst->creatureState == MONSTER_FLEEING
				&& (!(monst->info.flags & MONST_FLEES_NEAR_DEATH) || monst->currentHP >= monst->info.maxHP * 3 / 4)) {
                
				monst->creatureState = MONSTER_TRACKING_SCENT;
			}
			refreshDungeonCell(monst->xLoc, monst->yLoc);
		} else if (monst->info.flags & (MONST_RESTRICTED_TO_LIQUID)
				   && monst->creatureState != MONSTER_ALLY) {
			monst->creatureState = MONSTER_FLEEING;
		}
	}
}

boolean traversiblePathBetween(creature *monst, short x2, short y2) {
	short coords[DCOLS][2], i, x, y, n;
	short originLoc[2] = {monst->xLoc, monst->yLoc};
	short targetLoc[2] = {x2, y2};
	
	n = getLineCoordinates(coords, originLoc, targetLoc);
	
	for (i=0; i<n; i++) {
		x = coords[i][0];
		y = coords[i][1];
		if (x == x2 && y == y2) {
			return true;
		}
		if (monsterAvoids(monst, x, y)) {
			return false;
		}
	}
#ifdef BROGUE_ASSERTS
    assert(false);
#endif
	return true; // should never get here
}

boolean specifiedPathBetween(short x1, short y1, short x2, short y2,
							 unsigned long blockingTerrain, unsigned long blockingFlags) {
	short coords[DCOLS][2], i, x, y, n;
	short originLoc[2] = {x1, y1};
	short targetLoc[2] = {x2, y2};
	n = getLineCoordinates(coords, originLoc, targetLoc);
	
	for (i=0; i<n; i++) {
		x = coords[i][0];
		y = coords[i][1];
		if (cellHasTerrainFlag(x, y, blockingTerrain) || (pmap[x][y].flags & blockingFlags)) {
			return false;
		}
		if (x == x2 && y == y2) {
			return true;
		}
	}
#ifdef BROGUE_ASSERTS
    assert(false);
#endif
	return true; // should never get here
}

boolean openPathBetween(short x1, short y1, short x2, short y2) {
	short returnLoc[2], startLoc[2] = {x1, y1}, targetLoc[2] = {x2, y2};
	
	getImpactLoc(returnLoc, startLoc, targetLoc, DCOLS, false);
	if (returnLoc[0] == targetLoc[0] && returnLoc[1] == targetLoc[1]) {
		return true;
	}
	return false;
}

// will return the player if the player is at (x, y).
creature *monsterAtLoc(short x, short y) {
	creature *monst;
	if (!(pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER))) {
		return NULL;
	}
	if (player.xLoc == x && player.yLoc == y) {
		return &player;
	}
	for (monst = monsters->nextCreature; monst != NULL && (monst->xLoc != x || monst->yLoc != y); monst = monst->nextCreature);
	return monst;
}

creature *dormantMonsterAtLoc(short x, short y) {
	creature *monst;
	if (!(pmap[x][y].flags & HAS_DORMANT_MONSTER)) {
		return NULL;
	}
	for (monst = dormantMonsters->nextCreature; monst != NULL && (monst->xLoc != x || monst->yLoc != y); monst = monst->nextCreature);
	return monst;
}

void moveTowardLeader(creature *monst) {
	short targetLoc[2], dir;
	
	if (monst->creatureState == MONSTER_ALLY && !(monst->leader)) {
		monst->leader = &player;
		monst->bookkeepingFlags |= MONST_FOLLOWER;
	}
	
	if ((monst->bookkeepingFlags & MONST_FOLLOWER)
		&& traversiblePathBetween(monst, monst->leader->xLoc, monst->leader->yLoc)) {
		
		targetLoc[0] = monst->leader->xLoc;
		targetLoc[1] = monst->leader->yLoc;
		moveMonsterPassivelyTowards(monst, targetLoc, (monst->creatureState != MONSTER_ALLY));
		return;
	}
	
	// okay, poor minion can't see his leader.
	
	// is the leader missing his map altogether?
	if (!monst->leader->mapToMe) {
		monst->leader->mapToMe = allocGrid();
		fillGrid(monst->leader->mapToMe, 0);
		calculateDistances(monst->leader->mapToMe, monst->leader->xLoc, monst->leader->yLoc, 0, monst, true, false);
	}
	
	// is the leader map out of date?
	if (monst->leader->mapToMe[monst->leader->xLoc][monst->leader->yLoc] > 3) {
		// it is. recalculate the map.
		calculateDistances(monst->leader->mapToMe, monst->leader->xLoc, monst->leader->yLoc, 0, monst, true, false);
	}
	
	// blink to the leader?
	if ((monst->info.abilityFlags & MA_CAST_BLINK)
		&& distanceBetween(monst->xLoc, monst->yLoc, monst->leader->xLoc, monst->leader->yLoc) > 10
		&& monsterBlinkToPreferenceMap(monst, monst->leader->mapToMe, false)) { // if he actually cast a spell
        
		monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
		return;
	}
	
	// follow the map.
	dir = nextStep(monst->leader->mapToMe, monst->xLoc, monst->yLoc, monst, true);
	targetLoc[0] = monst->xLoc + nbDirs[dir][0];
	targetLoc[1] = monst->yLoc + nbDirs[dir][1];
	if (!moveMonsterPassivelyTowards(monst, targetLoc, (monst->creatureState != MONSTER_ALLY))) {
		// monster is blocking the way
		dir = randValidDirectionFrom(monst, monst->xLoc, monst->yLoc, true);
		if (dir != -1) {
			targetLoc[0] = monst->xLoc + nbDirs[dir][0];
			targetLoc[1] = monst->yLoc + nbDirs[dir][1];
			moveMonsterPassivelyTowards(monst, targetLoc, (monst->creatureState != MONSTER_ALLY));
		}
	}
}

boolean creatureEligibleForSwarming(creature *monst) {
    if ((monst->info.flags & (MONST_IMMOBILE | MONST_GETS_TURN_ON_ACTIVATION | MONST_MAINTAINS_DISTANCE))
        || monst->status[STATUS_ENTRANCED]
        || monst->status[STATUS_CONFUSED]
        || monst->status[STATUS_STUCK]
        || monst->status[STATUS_PARALYZED]
        || monst->status[STATUS_MAGICAL_FEAR]
        || monst->status[STATUS_LIFESPAN_REMAINING] == 1
        || (monst->bookkeepingFlags & (MONST_SEIZED | MONST_SEIZING))) {

        return false;
    }
    if (monst != &player
        && monst->creatureState != MONSTER_ALLY
        && monst->creatureState != MONSTER_TRACKING_SCENT) {
        
        return false;
    }
    return true;
}

// Swarming behavior.
// If you’re adjacent to an enemy and about to strike it, and you’re adjacent to a hunting-mode tribemate
// who is not adjacent to another enemy, and there is no empty space adjacent to the tribemate AND the enemy,
// and there is an empty space adjacent to you AND the enemy, then move into that last space.
// (In each case, "adjacent" excludes diagonal tiles obstructed by corner walls.)
enum directions monsterSwarmDirection(creature *monst, creature *enemy) {
    short newX, newY, i;
    enum directions dir, targetDir;
    short dirList[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    boolean alternateDirectionExists;
    creature *ally, *otherEnemy;
    
    if (monst == &player || !creatureEligibleForSwarming(monst)) {
        return NO_DIRECTION;
    }
    
    if (distanceBetween(monst->xLoc, monst->yLoc, enemy->xLoc, enemy->yLoc) != 1
        || (diagonalBlocked(monst->xLoc, monst->yLoc, enemy->xLoc, enemy->yLoc) || (enemy->info.flags & MONST_ATTACKABLE_THRU_WALLS))
        || !monstersAreEnemies(monst, enemy)) {
        
        return NO_DIRECTION; // Too far from the enemy, diagonally blocked, or not enemies with it.
    }
    
    // Find a location that is adjacent to you and to the enemy.
    targetDir = NO_DIRECTION;
    shuffleList(dirList, 4);
    shuffleList(&(dirList[4]), 4);
    for (i=0; i<8 && targetDir == NO_DIRECTION; i++) {
        dir = dirList[i];
        newX = monst->xLoc + nbDirs[dir][0];
        newY = monst->yLoc + nbDirs[dir][1];
        if (coordinatesAreInMap(newX, newY)
            && distanceBetween(enemy->xLoc, enemy->yLoc, newX, newY) == 1
            && !(pmap[newX][newY].flags & (HAS_PLAYER | HAS_MONSTER))
            && !diagonalBlocked(monst->xLoc, monst->yLoc, newX, newY)
            && (!diagonalBlocked(enemy->xLoc, enemy->yLoc, newX, newY) || (enemy->info.flags & MONST_ATTACKABLE_THRU_WALLS))
            && !monsterAvoids(monst, newX, newY)) {
            
            targetDir = dir;
        }
    }
    if (targetDir == NO_DIRECTION) {
        return NO_DIRECTION; // No open location next to both you and the enemy.
    }
    
    // OK, now we have a place to move toward. Let's analyze the teammates around us to make sure that
    // one of them could take advantage of the space we open.
    CYCLE_MONSTERS_AND_PLAYERS(ally) {
        if (ally != monst
            && ally != enemy
            && monstersAreTeammates(monst, ally)
            && monstersAreEnemies(ally, enemy)
            && creatureEligibleForSwarming(ally)
            && distanceBetween(monst->xLoc, monst->yLoc, ally->xLoc, ally->yLoc) == 1
            && !diagonalBlocked(monst->xLoc, monst->yLoc, ally->xLoc, ally->yLoc)
            && !monsterAvoids(ally, monst->xLoc, monst->yLoc)
            && (distanceBetween(enemy->xLoc, enemy->yLoc, ally->xLoc, ally->yLoc) > 1 || diagonalBlocked(enemy->xLoc, enemy->yLoc, ally->xLoc, ally->yLoc))) {
            
            // Found a prospective ally.
            // Check that there isn't already an open space from which to attack the enemy that is accessible to the ally.
            alternateDirectionExists = false;
            for (dir=0; dir<8 && !alternateDirectionExists; dir++) {
                newX = ally->xLoc + nbDirs[dir][0];
                newY = ally->yLoc + nbDirs[dir][1];
                if (coordinatesAreInMap(newX, newY)
                    && !(pmap[newX][newY].flags & (HAS_PLAYER | HAS_MONSTER))
                    && distanceBetween(enemy->xLoc, enemy->yLoc, newX, newY) == 1
                    && !diagonalBlocked(enemy->xLoc, enemy->yLoc, newX, newY)
                    && !diagonalBlocked(ally->xLoc, ally->yLoc, newX, newY)
                    && !monsterAvoids(ally, newX, newY)) {
                    
                    alternateDirectionExists = true;
                }
            }
            if (!alternateDirectionExists) {
                // OK, no alternative open spaces exist.
                // Check that the ally isn't already occupied with an enemy of its own.
                CYCLE_MONSTERS_AND_PLAYERS(otherEnemy) {
                    if (ally != otherEnemy
                        && monst != otherEnemy
                        && enemy != otherEnemy
                        && monstersAreEnemies(ally, otherEnemy)
                        && distanceBetween(ally->xLoc, ally->yLoc, otherEnemy->xLoc, otherEnemy->yLoc) == 1
                        && (!diagonalBlocked(ally->xLoc, ally->yLoc, otherEnemy->xLoc, otherEnemy->yLoc) || (otherEnemy->info.flags & MONST_ATTACKABLE_THRU_WALLS))) {
                        
                        break; // Ally is already occupied.
                    }
                }
                if (otherEnemy == NULL) {
                    // Success!
                    return targetDir;
                }
            }
        }
    }
    return NO_DIRECTION; // Failure!
}

// Isomorphs a number in [0, 39] to coordinates along the square of radius 5 surrounding (0,0).
// This is used as the sample space for bolt target coordinates, e.g. when reflecting or when
// monsters are deciding where to blink.
void perimeterCoords(short returnCoords[2], short n) {
	if (n <= 10) {			// top edge, left to right
		returnCoords[0] = n - 5;
		returnCoords[1] = -5;
	} else if (n <= 21) {	// bottom edge, left to right
		returnCoords[0] = (n - 11) - 5;
		returnCoords[1] = 5;
	} else if (n <= 30) {	// left edge, top to bottom
		returnCoords[0] = -5;
		returnCoords[1] = (n - 22) - 4;
	} else if (n <= 39) {	// right edge, top to bottom
		returnCoords[0] = 5;
		returnCoords[1] = (n - 31) - 4;		
	} else {
		message("ERROR! Bad perimeter coordinate request!", true);
		returnCoords[0] = returnCoords[1] = 0; // garbage in, garbage out
	}
}

// Tries to make the monster blink to the most desirable square it can aim at, according to the
// preferenceMap argument. "blinkUphill" determines whether it's aiming for higher or lower numbers on
// the preference map -- true means higher. Returns true if the monster blinked; false if it didn't.
boolean monsterBlinkToPreferenceMap(creature *monst, short **preferenceMap, boolean blinkUphill) {
	short i, bestTarget[2], bestPreference, nowPreference, maxDistance, target[2], impact[2], origin[2];
	boolean gotOne;
	char monstName[DCOLS*3];
	char buf[DCOLS*3];
    
	maxDistance = staffBlinkDistance(5);
	gotOne = false;
	
	origin[0] = monst->xLoc;
	origin[1] = monst->yLoc;
		
	bestTarget[0]	= 0;
	bestTarget[1]	= 0;
	bestPreference	= preferenceMap[monst->xLoc][monst->yLoc];
	
	// make sure that we beat the four cardinal neighbors
	for (i = 0; i < 4; i++) {
		nowPreference = preferenceMap[monst->xLoc + nbDirs[i][0]][monst->yLoc + nbDirs[i][1]];
		
		if (((blinkUphill && nowPreference > bestPreference) || (!blinkUphill && nowPreference < bestPreference))
			&& !monsterAvoids(monst, monst->xLoc + nbDirs[i][0], monst->yLoc + nbDirs[i][1])) {
			
			bestPreference = nowPreference;
		}
	}
	
	for (i=0; i<40; i++) {
		perimeterCoords(target, i);
		target[0] += monst->xLoc;
		target[1] += monst->yLoc;
		
		getImpactLoc(impact, origin, target, maxDistance, true);
		nowPreference = preferenceMap[impact[0]][impact[1]];
		
		if (((blinkUphill && (nowPreference > bestPreference))
             || (!blinkUphill && (nowPreference < bestPreference)))
			&& !monsterAvoids(monst, impact[0], impact[1])) {
            
			bestTarget[0]	= target[0];
			bestTarget[1]	= target[1];
			bestPreference	= nowPreference;
			
			if ((abs(impact[0] - origin[0]) > 1 || abs(impact[1] - origin[1]) > 1)
                || (cellHasTerrainFlag(impact[0], origin[1], T_OBSTRUCTS_PASSABILITY))
                || (cellHasTerrainFlag(origin[0], impact[1], T_OBSTRUCTS_PASSABILITY))) {
				gotOne = true;
			} else {
				gotOne = false;
			}
		}
	}
	
	if (gotOne) {
		if (canDirectlySeeMonster(monst)) {
			monsterName(monstName, monst, true);
			sprintf(buf, "%s一瞬间移开了", monstName);
			combatMessage(buf, 0);
		}
		monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
		zap(origin, bestTarget, BOLT_BLINKING, 5, false);
		return true;
	}
	return false;
}

// returns whether the monster did something (and therefore ended its turn)
boolean monsterBlinkToSafety(creature *monst) {
	short **blinkSafetyMap;
	
	if (monst->creatureState == MONSTER_ALLY) {
		if (!rogue.updatedAllySafetyMapThisTurn) {
			updateAllySafetyMap();
		}
		blinkSafetyMap = allySafetyMap;
	} else if (pmap[monst->xLoc][monst->yLoc].flags & IN_FIELD_OF_VIEW) {
		if (monst->safetyMap) {
			freeGrid(monst->safetyMap);
			monst->safetyMap = NULL;
		}
		if (!rogue.updatedSafetyMapThisTurn) {
			updateSafetyMap();
		}
		blinkSafetyMap = safetyMap;
	} else {
		if (!monst->safetyMap) {
			if (!rogue.updatedSafetyMapThisTurn) {
				updateSafetyMap();
			}
			monst->safetyMap = allocGrid();
			copyGrid(monst->safetyMap, safetyMap);
		}
		blinkSafetyMap = monst->safetyMap;
	}
	
	return monsterBlinkToPreferenceMap(monst, blinkSafetyMap, false);
}

// returns whether the monster did something (and therefore ended its turn)
boolean monstUseMagic(creature *monst) {
	short originLoc[2] = {monst->xLoc, monst->yLoc};
	short targetLoc[2];
	short weakestAllyHealthFraction = 100;
	creature *target, *weakestAlly;
	char monstName[DCOLS*3];
	char buf[DCOLS*3];
	short minionCount = 0;
	boolean abortHaste, alwaysUse;
	short listOfCoordinates[MAX_BOLT_LENGTH][2];
    
	memset(listOfCoordinates, 0, sizeof(short) * MAX_BOLT_LENGTH * 2);
    alwaysUse = (monst->info.flags & MONST_ALWAYS_USE_ABILITY) ? true : false;
	
	// abilities that have no particular target:
	if (monst->info.abilityFlags & (MA_CAST_SUMMON)) {
        // Count existing minions.
		for (target = monsters->nextCreature; target != NULL; target = target->nextCreature) {
            if (monst->creatureState == MONSTER_ALLY) {
                if (target->creatureState == MONSTER_ALLY) {
                    minionCount++; // Allied summoners count all allies.
                }
			} else if ((target->bookkeepingFlags & MONST_FOLLOWER) && target->leader == monst) {
				minionCount++; // Enemy summoners count only direct followers, not teammates.
			}
		}
        if (monst->creatureState == MONSTER_ALLY) { // Allied summoners also count monsters on the previous and next depths.
            if (rogue.depthLevel > 1) {
                for (target = levels[rogue.depthLevel - 2].monsters; target != NULL; target = target->nextCreature) {
                    if (target->creatureState == MONSTER_ALLY && !(target->info.flags & MONST_WILL_NOT_USE_STAIRS)) {
                        minionCount++;
                    }
                }
            }
            if (rogue.depthLevel < DEEPEST_LEVEL) {
                for (target = levels[rogue.depthLevel].monsters; target != NULL; target = target->nextCreature) {
                    if (target->creatureState == MONSTER_ALLY && !(target->info.flags & MONST_WILL_NOT_USE_STAIRS)) {
                        minionCount++;
                    }
                }
            }
        }
        if (alwaysUse && minionCount < 50) {
			summonMinions(monst);
			return true;
        } else if ((monst->creatureState != MONSTER_ALLY || minionCount < 5 || (monst->info.abilityFlags & MA_ENTER_SUMMONS))
			&& !rand_range(0, minionCount * minionCount * 3 + (monst->info.abilityFlags & MA_ENTER_SUMMONS ? 5 : 1))) {
            
			summonMinions(monst);
			return true;
		}
	}
	
	// Strong abilities that might target the caster's enemies:
	if (!monst->status[STATUS_CONFUSED] && monst->info.abilityFlags
		& (MA_CAST_NEGATION | MA_CAST_SLOW | MA_CAST_DISCORD | MA_CAST_BECKONING | MA_BREATHES_FIRE | MA_SHOOTS_WEBS | MA_ATTACKS_FROM_DISTANCE)) {
        
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (monstersAreEnemies(monst, target)
				&& !((monst->bookkeepingFlags | target->bookkeepingFlags) & MONST_SUBMERGED) // neither is submerged
				&& !target->status[STATUS_INVISIBLE]
                && (monst->creatureState != MONSTER_ALLY || !(target->info.flags & MONST_REFLECT_4))
				&& openPathBetween(monst->xLoc, monst->yLoc, target->xLoc, target->yLoc)) {
                
				targetLoc[0] = target->xLoc;
				targetLoc[1] = target->yLoc;
                
                // mirrored totems sometimes cast beckoning
				if ((monst->info.abilityFlags & MA_CAST_BECKONING)
					&& !(target->info.flags & MONST_INANIMATE)
                    && distanceBetween(monst->xLoc, monst->yLoc, targetLoc[0], targetLoc[1]) > 1
                    && (alwaysUse || rand_percent(35))) {
                    
                    if (canDirectlySeeMonster(monst)) {
                        monsterName(monstName, monst, true);
                        sprintf(buf, "%s使用了牵引魔法", monstName);
                        combatMessage(buf, 0);
                    }
                    zap(originLoc, targetLoc, BOLT_BECKONING, 10, false);
                    return true;
                }
				
				// dragons sometimes breathe fire
				if ((monst->info.abilityFlags & MA_BREATHES_FIRE)
                    && !target->status[STATUS_IMMUNE_TO_FIRE]
                    && !target->status[STATUS_ENTRANCED]
                    && (alwaysUse || rand_percent(35))) {
                    
                    // Hold your fire if (1) the first cell in the trajectory is flammable and will catch fire,
                    // and (2) the cell we're standing on is flammable and will combust into something we'd rather avoid.
                    if (!(burnedTerrainFlagsAtLoc(listOfCoordinates[0][0], listOfCoordinates[0][1]) & T_IS_FIRE)
                        || !(burnedTerrainFlagsAtLoc(monst->xLoc, monst->yLoc) & avoidedFlagsForMonster(&(monst->info)))) {
                        
                        monsterName(monstName, monst, true);
                        if (canSeeMonster(monst) || canSeeMonster(target)) {
                            sprintf(buf, "%s吐出了火焰！", monstName);
                            message(buf, false);
                        }
                        zap(originLoc, targetLoc, BOLT_FIRE, 18, false);
                        if (player.currentHP <= 0) {
                            gameOver(monsterCatalog[monst->info.monsterID].monsterName, false);
                        }
                        return true;
                    }
				}
				
				// spiders sometimes shoot webs:
				if (monst->info.abilityFlags & MA_SHOOTS_WEBS
					&& !(target->info.flags & (MONST_INANIMATE | MONST_IMMUNE_TO_WEBS))
					&& (!target->status[STATUS_STUCK] || distanceBetween(monst->xLoc, monst->yLoc, targetLoc[0], targetLoc[1]) > 1)
					&& distanceBetween(monst->xLoc, monst->yLoc, targetLoc[0], targetLoc[1]) < 20
					&& ((!target->status[STATUS_STUCK] && distanceBetween(monst->xLoc, monst->yLoc, targetLoc[0], targetLoc[1]) == 1)
						|| (alwaysUse || rand_percent(20)))) {
						
						shootWeb(monst, targetLoc, SPIDERWEB);
						return true;
					}
				
				// centaurs shoot from a distance:
				if ((monst->info.abilityFlags & MA_ATTACKS_FROM_DISTANCE)
                    && !(target->info.flags & MONST_IMMUNE_TO_WEAPONS)
                    && !target->status[STATUS_ENTRANCED]
					&& distanceBetween(monst->xLoc, monst->yLoc, targetLoc[0], targetLoc[1]) < 11
                    && (alwaysUse || !monst->status[STATUS_DISCORDANT] || rand_percent(35))) {
                    
					monsterShoots(monst, targetLoc,
								  ((monst->info.abilityFlags & MA_HIT_DEGRADE_ARMOR) ? '*' : WEAPON_CHAR),
								  ((monst->info.abilityFlags & MA_HIT_DEGRADE_ARMOR) ? &green : &gray));
					return true;
				}
				
				// Dar blademasters sometimes blink -- but it is handled elsewhere.
				
				// Discord.
				if ((monst->info.abilityFlags & MA_CAST_DISCORD)
                    && (!target->status[STATUS_DISCORDANT])
					&& !(target->info.flags & MONST_INANIMATE)
                    && (target != &player)
                    && (alwaysUse || rand_percent(50))) {
                    
					if (canDirectlySeeMonster(monst)) {
						monsterName(monstName, monst, true);
						sprintf(buf, "%s释放了挑拨法术", monstName);
						combatMessage(buf, 0);
					}
					zap(originLoc, targetLoc, BOLT_DISCORD, 10, false);
					return true;
				}
				
				// Opportunity to cast negation cleverly?
				if ((monst->info.abilityFlags & MA_CAST_NEGATION)
					&& (target->status[STATUS_HASTED] || target->status[STATUS_TELEPATHIC] || target->status[STATUS_SHIELDED]
						|| (target->info.flags & (MONST_DIES_IF_NEGATED | MONST_IMMUNE_TO_WEAPONS))
						|| ((target->status[STATUS_IMMUNE_TO_FIRE] || target->status[STATUS_LEVITATING])
							&& cellHasTerrainFlag(target->xLoc, target->yLoc, (T_LAVA_INSTA_DEATH | T_IS_DEEP_WATER | T_AUTO_DESCENT))))
					&& !(target == &player && rogue.armor && (rogue.armor->flags & ITEM_RUNIC) && rogue.armor->enchant2 == A_REFLECTION && netEnchant(rogue.armor) > 0)
                    && (monst->creatureState != MONSTER_ALLY || !(target->info.flags & MONST_REFLECT_4))
					&& (alwaysUse || rand_percent(50))) {
                    
					if (canDirectlySeeMonster(monst)) {
						monsterName(monstName, monst, true);
						sprintf(buf, "%s释放了反魔法法术", monstName);
						combatMessage(buf, 0);
					}
					zap(originLoc, targetLoc, BOLT_NEGATION, 10, false);
					return true;
				}
				
				// Slow.
				if ((monst->info.abilityFlags & MA_CAST_SLOW)
                    && (!target->status[STATUS_SLOWED])
					&& !(target->info.flags & MONST_INANIMATE)
                    && (alwaysUse || rand_percent(50))) {
                    
					if (canDirectlySeeMonster(monst)) {
						monsterName(monstName, monst, true);
						sprintf(buf, "%s释放了减速魔法", monstName);
						combatMessage(buf, 0);
					}
					zap(originLoc, targetLoc, BOLT_SLOW, 10, false);
					return true;
				}
			}
		}
	}
	
	// abilities that might target allies:
	
	if (!monst->status[STATUS_CONFUSED]
        && (monst->info.abilityFlags & MA_CAST_NEGATION)
        && (alwaysUse || rand_percent(75))) {
        
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (target != monst
				&& ((target->status[STATUS_ENTRANCED] && target->creatureState != MONSTER_ALLY)
                     || target->status[STATUS_MAGICAL_FEAR] || target->status[STATUS_DISCORDANT])
				&& monstersAreTeammates(monst, target)
				&& !(target->bookkeepingFlags & MONST_SUBMERGED)
				&& !(target->info.flags & MONST_DIES_IF_NEGATED)
                && (monst->creatureState != MONSTER_ALLY || !(target->info.flags & MONST_REFLECT_4))
				&& openPathBetween(monst->xLoc, monst->yLoc, target->xLoc, target->yLoc)) {
				
				if (canDirectlySeeMonster(monst)) {
					monsterName(monstName, monst, true);
					sprintf(buf, "%s释放了反魔法法术", monstName);
					combatMessage(buf, 0);
				}
				
				targetLoc[0] = target->xLoc;
				targetLoc[1] = target->yLoc;
				zap(originLoc, targetLoc, BOLT_NEGATION, 10, false);
				return true;
			}
		}
	}
	
	if (!monst->status[STATUS_CONFUSED]
        && (monst->info.abilityFlags & MA_CAST_HASTE)
        && (alwaysUse || rand_percent(75))) {
		// Allies shouldn't cast haste unless there is a hostile monster in sight of the player.
		// Otherwise, it's super annoying.
		abortHaste = false;
		if (monst->creatureState == MONSTER_ALLY) {
			abortHaste = true;
			CYCLE_MONSTERS_AND_PLAYERS(target) {
				if (monstersAreEnemies(&player, target)
					&& (pmap[target->xLoc][target->yLoc].flags & IN_FIELD_OF_VIEW)) {
					abortHaste = false;
					break;
				}
			}
		}
		
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (!abortHaste
				&& target != monst
				&& (!target->status[STATUS_HASTED] || target->status[STATUS_HASTED] * 3 < target->maxStatus[STATUS_HASTED])
				&& (target->creatureState == MONSTER_TRACKING_SCENT
					|| target->creatureState == MONSTER_FLEEING
					|| target->creatureState == MONSTER_ALLY
					|| target == &player)
				&& monstersAreTeammates(monst, target)
				&& !monstersAreEnemies(monst, target)
				&& !(target->bookkeepingFlags & MONST_SUBMERGED)
				&& !(target->info.flags & MONST_INANIMATE)
				&& openPathBetween(monst->xLoc, monst->yLoc, target->xLoc, target->yLoc)) {
				
				if (canDirectlySeeMonster(monst)) {
					monsterName(monstName, monst, true);
					sprintf(buf, "%s释放了加速法术", monstName);
					combatMessage(buf, 0);
				}
				
				targetLoc[0] = target->xLoc;
				targetLoc[1] = target->yLoc;
				zap(originLoc, targetLoc, BOLT_HASTE, 2, false);
				return true;
			}
		}
	}
	
	if (!monst->status[STATUS_CONFUSED]
        && (monst->info.abilityFlags & MA_CAST_PROTECTION)
        && (alwaysUse || rand_percent(75))) {
        
		// Generally follows the haste model from above.
		abortHaste = false;
		if (monst->creatureState == MONSTER_ALLY) {
			abortHaste = true;
			CYCLE_MONSTERS_AND_PLAYERS(target) {
				if (monstersAreEnemies(&player, target)
					&& (pmap[target->xLoc][target->yLoc].flags & IN_FIELD_OF_VIEW)
					&& !(target->bookkeepingFlags & MONST_SUBMERGED)) {
					abortHaste = false;
					break;
				}
			}
		}
		
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (!abortHaste
				&& target != monst
				&& (!target->status[STATUS_SHIELDED] || target->status[STATUS_SHIELDED] * 2 < target->maxStatus[STATUS_SHIELDED])
				&& (target->creatureState == MONSTER_TRACKING_SCENT
					|| target->creatureState == MONSTER_FLEEING
					|| target->creatureState == MONSTER_ALLY
					|| target == &player)
				&& monstersAreTeammates(monst, target)
				&& !monstersAreEnemies(monst, target)
				&& !(target->bookkeepingFlags & MONST_SUBMERGED)
				&& !(target->info.flags & MONST_INANIMATE)
				&& openPathBetween(monst->xLoc, monst->yLoc, target->xLoc, target->yLoc)) {
				
				if (canDirectlySeeMonster(monst)) {
					monsterName(monstName, monst, true);
					sprintf(buf, "%s释放了魔法盾", monstName);
					combatMessage(buf, 0);
				}
				
				targetLoc[0] = target->xLoc;
				targetLoc[1] = target->yLoc;
				zap(originLoc, targetLoc, BOLT_SHIELDING, (monst->creatureState == MONSTER_ALLY ? 3 : 5), false);
				return true;
			}
		}
	}
	
	weakestAlly = NULL;
	
	// Heal the weakest ally you can see
	if (!monst->status[STATUS_CONFUSED]
		&& (monst->info.abilityFlags & MA_CAST_HEAL)
		&& !(monst->bookkeepingFlags & (MONST_SUBMERGED | MONST_CAPTIVE))
		&& (alwaysUse || rand_percent(monst->creatureState == MONSTER_ALLY ? 20 : 50))) {
        
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (target != monst
				&& (100 * target->currentHP / target->info.maxHP < weakestAllyHealthFraction)
				&& monstersAreTeammates(monst, target)
				&& !monstersAreEnemies(monst, target)
				&& openPathBetween(monst->xLoc, monst->yLoc, target->xLoc, target->yLoc)) {
				weakestAllyHealthFraction = 100 * target->currentHP / target->info.maxHP;
				weakestAlly = target;
			}
		}
		if (weakestAllyHealthFraction < 100) {
			target = weakestAlly;
			targetLoc[0] = target->xLoc;
			targetLoc[1] = target->yLoc;
			
			if (canDirectlySeeMonster(monst)) {
				monsterName(monstName, monst, true);
				sprintf(buf, "%s释放了回复法术", monstName);
				combatMessage(buf, 0);
			}
			
			zap(originLoc, targetLoc, BOLT_HEALING, (monst->creatureState == MONSTER_ALLY ? 2 : 5), false);
			return true;
		}
	}
	
	// weak direct damage spells against enemies:
	
	if (!monst->status[STATUS_CONFUSED] && monst->info.abilityFlags & (MA_CAST_FIRE | MA_CAST_SPARK)) {
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (monstersAreEnemies(monst, target)
				&& monst != target
				&& !(monst->bookkeepingFlags & MONST_SUBMERGED)
				&& !target->status[STATUS_INVISIBLE]
                && !target->status[STATUS_ENTRANCED]
                && (monst->creatureState != MONSTER_ALLY || !(target->info.flags & MONST_REFLECT_4))
				&& openPathBetween(monst->xLoc, monst->yLoc, target->xLoc, target->yLoc)) {
                
                targetLoc[0] = target->xLoc;
                targetLoc[1] = target->yLoc;
                
                if (monst->info.abilityFlags & (MA_CAST_FIRE)
                    && !target->status[STATUS_IMMUNE_TO_FIRE]
                    && (alwaysUse || rand_percent(50))) {
                    
                    // Hold your fire if (1) the first cell in the trajectory is flammable and will catch fire,
                    // and (2) the cell we're standing on is flammable and will combust into something we'd rather avoid.
                    if (!(burnedTerrainFlagsAtLoc(listOfCoordinates[0][0], listOfCoordinates[0][1]) & T_IS_FIRE)
                        || !(burnedTerrainFlagsAtLoc(monst->xLoc, monst->yLoc) & avoidedFlagsForMonster(&(monst->info)))) {
                        
                        if (canSeeMonster(monst)) {
                            monsterName(monstName, monst, true);
                            sprintf(buf, "%s释放了火球术", monstName);
                            combatMessage(buf, 0);
                        }
                        zap(originLoc, targetLoc, BOLT_FIRE, 4, false);
                        if (player.currentHP <= 0) {
                            monsterName(monstName, monst, false);
                            gameOver(monsterCatalog[monst->info.monsterID].monsterName, false);
                        }
                        return true;
                    }
                }
                
                if ((monst->info.abilityFlags & MA_CAST_SPARK)
                    && (alwaysUse || rand_percent(50))) {
                    
                    if (canDirectlySeeMonster(monst)) {
                        monsterName(monstName, monst, true);
                        sprintf(buf, "%s释放了闪电术", monstName);
                        combatMessage(buf, 0);
                    }
                    zap(originLoc, targetLoc, BOLT_LIGHTNING, 1, false);
                    if (player.currentHP <= 0) {
                        monsterName(monstName, monst, false);
                        gameOver(monsterCatalog[monst->info.monsterID].monsterName, false);
                    }
                    return true;
                }
            }
		}
	}
	
	return false;
}

boolean isLocalScentMaximum(short x, short y) {
    enum directions dir;
    short newX, newY;
    
    const short baselineScent = scentMap[x][y];
    
    for (dir=0; dir<8; dir++) {
        newX = x + nbDirs[dir][0];
        newY = y + nbDirs[dir][1];
        if (coordinatesAreInMap(newX, newY)
            && (scentMap[newX][newY] > baselineScent)
            && !cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)
            && !diagonalBlocked(x, y, newX, newY)) {
            
            return false;
        }
    }
    return false;
}

// Returns the direction the player's scent points to from a given cell. Returns -1 if the nose comes up blank.
enum directions scentDirection(creature *monst) {
	short newX, newY, x, y, newestX, newestY;
    enum directions bestDirection = NO_DIRECTION, dir, dir2;
	unsigned short bestNearbyScent = 0;
	boolean canTryAgain = true;
	creature *otherMonst;
	
	x = monst->xLoc;
	y = monst->yLoc;
	
	for (;;) {
		
		for (dir=0; dir<8; dir++) {
			newX = x + nbDirs[dir][0];
			newY = y + nbDirs[dir][1];
			otherMonst = monsterAtLoc(newX, newY);
			if (coordinatesAreInMap(newX, newY)
				&& (scentMap[newX][newY] > bestNearbyScent)
				&& (!(pmap[newX][newY].flags & HAS_MONSTER) || (otherMonst && canPass(monst, otherMonst)))
				&& !cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)
				&& !diagonalBlocked(x, y, newX, newY)
				&& !monsterAvoids(monst, newX, newY)) {
				
				bestNearbyScent = scentMap[newX][newY];
				bestDirection = dir;
			}
		}
		
		if (bestDirection >= 0 && bestNearbyScent > scentMap[x][y]) {
			return bestDirection;
		}
		
		if (canTryAgain) {
			// Okay, the monster may be stuck in some irritating diagonal.
			// If so, we can diffuse the scent into the offending kink and solve the problem.
			// There's a possibility he's stuck for some other reason, though, so we'll only
			// try once per his move -- hence the failsafe.
			canTryAgain = false;
			for (dir=0; dir<4; dir++) {
				newX = x + nbDirs[dir][0];
				newY = y + nbDirs[dir][1];
				for (dir2=0; dir2<4; dir2++) {
					newestX = newX + nbDirs[dir2][0];
					newestY = newY + nbDirs[dir2][1];
					if (coordinatesAreInMap(newX, newY) && coordinatesAreInMap(newestX, newestY)) {
						scentMap[newX][newY] = max(scentMap[newX][newY], scentMap[newestX][newestY] - 1);
					}
				}
			}
		} else {
			return NO_DIRECTION; // failure!
		}
	}
}

void unAlly(creature *monst) {
	if (monst->creatureState == MONSTER_ALLY) {
		monst->creatureState = MONSTER_TRACKING_SCENT;
		monst->bookkeepingFlags &= ~(MONST_FOLLOWER | MONST_TELEPATHICALLY_REVEALED);
		monst->leader = NULL;
	}
}

boolean allyFlees(creature *ally, creature *closestEnemy) {
    const short x = ally->xLoc;
    const short y = ally->yLoc;
    
    if (!closestEnemy) {
        return false; // No one to flee from.
    }
    
    if (ally->info.maxHP <= 1 || (ally->status[STATUS_LIFESPAN_REMAINING]) > 0) { // Spectral blades and timed allies should never flee.
        return false;
    }
    
    if (distanceBetween(x, y, closestEnemy->xLoc, closestEnemy->yLoc) < 10
        && (100 * ally->currentHP / ally->info.maxHP <= 33)
        && ally->info.turnsBetweenRegen > 0
        && !ally->carriedMonster
        && ((ally->info.flags & MONST_FLEES_NEAR_DEATH) || (100 * ally->currentHP / ally->info.maxHP * 2 < 100 * player.currentHP / player.info.maxHP))) {
        // Flee if you're within 10 spaces, your HP is under 1/3, you're not a phoenix or lich or vampire in bat form,
        // and you either flee near death or your health fraction is less than half of the player's.
        return true;
    }
    
    // so do allies that keep their distance or while in the presence of damage-immune or kamikaze enemies
    if (distanceBetween(x, y, closestEnemy->xLoc, closestEnemy->yLoc) < 4
        && ((ally->info.flags & MONST_MAINTAINS_DISTANCE) || (closestEnemy->info.flags & MONST_IMMUNE_TO_WEAPONS) || (closestEnemy->info.abilityFlags & MA_KAMIKAZE))) {
        // Flee if you're within 3 spaces and you either flee near death or the closest enemy is a bloat, revenant or guardian.
        return true;
    }
    
    return false;
}

void moveAlly(creature *monst) {
	creature *target, *closestMonster = NULL;
	short i, j, x, y, dir, shortestDistance, targetLoc[2], leashLength;
	short **enemyMap, **costMap;
	char buf[DCOLS*3], monstName[DCOLS*3];
	
	x = monst->xLoc;
	y = monst->yLoc;
	
	targetLoc[0] = targetLoc[1] = 0;
	
	// If we're standing in harmful terrain and there is a way to escape it, spend this turn escaping it.
	if (cellHasTerrainFlag(x, y, (T_HARMFUL_TERRAIN & ~(T_IS_FIRE | T_CAUSES_DAMAGE | T_CAUSES_PARALYSIS | T_CAUSES_CONFUSION)))
		|| (cellHasTerrainFlag(x, y, T_IS_FIRE) && !monst->status[STATUS_IMMUNE_TO_FIRE])
		|| (cellHasTerrainFlag(x, y, T_CAUSES_DAMAGE | T_CAUSES_PARALYSIS | T_CAUSES_CONFUSION) && !(monst->info.flags & MONST_INANIMATE))) {
        
		if (!rogue.updatedMapToSafeTerrainThisTurn) {
			updateSafeTerrainMap();
		}
		
		if ((monst->info.abilityFlags & MA_CAST_BLINK)
			&& monsterBlinkToPreferenceMap(monst, rogue.mapToSafeTerrain, false)) {
            
			monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
			return;
		}
		
		dir = nextStep(rogue.mapToSafeTerrain, x, y, monst, true);
		if (dir != -1) {
			targetLoc[0] = x + nbDirs[dir][0];
			targetLoc[1] = y + nbDirs[dir][1];
			if (moveMonsterPassivelyTowards(monst, targetLoc, false)) {
				return;
			}
		}
	}
	
	// Look around for enemies; shortestDistance will be the distance to the nearest.
	shortestDistance = max(DROWS, DCOLS);
	for (target = monsters->nextCreature; target != NULL; target = target->nextCreature) {
		if (target != monst
			&& (!(target->bookkeepingFlags & MONST_SUBMERGED) || (monst->bookkeepingFlags & MONST_SUBMERGED))
			&& monstersAreEnemies(target, monst)
			&& !(target->bookkeepingFlags & MONST_CAPTIVE)
			&& !(target->info.flags & MONST_IMMUNE_TO_WEAPONS)
            && !target->status[STATUS_ENTRANCED]
			&& distanceBetween(x, y, target->xLoc, target->yLoc) < shortestDistance
			&& traversiblePathBetween(monst, target->xLoc, target->yLoc)
			&& (!monsterAvoids(monst, target->xLoc, target->yLoc) || (target->info.flags & MONST_ATTACKABLE_THRU_WALLS))
			&& (!target->status[STATUS_INVISIBLE] || rand_percent(33))) {
			
			shortestDistance = distanceBetween(x, y, target->xLoc, target->yLoc);
			closestMonster = target;
		}
	}
	
	// Weak allies in the presence of enemies seek safety;
	if (allyFlees(monst, closestMonster)) {		
		if ((monst->info.abilityFlags & MA_CAST_BLINK)
			&& ((monst->info.flags & MONST_ALWAYS_USE_ABILITY) || rand_percent(30))
			&& monsterBlinkToSafety(monst)) {
			return;
		}
		if (!rogue.updatedAllySafetyMapThisTurn) {
			updateAllySafetyMap();
		}
		dir = nextStep(allySafetyMap, monst->xLoc, monst->yLoc, monst, true);
		if (dir != -1) {
			targetLoc[0] = x + nbDirs[dir][0];
			targetLoc[1] = y + nbDirs[dir][1];
		}
		if (dir == -1
			|| (allySafetyMap[targetLoc[0]][targetLoc[1]] >= allySafetyMap[x][y])
			|| (!moveMonster(monst, nbDirs[dir][0], nbDirs[dir][1]) && !moveMonsterPassivelyTowards(monst, targetLoc, true))) {
			// ally can't flee; continue below
		} else {
			return;
		}
	}
	
	// Magic users sometimes cast spells.
	if (monst->info.abilityFlags & MAGIC_ATTACK) {
		if (monstUseMagic(monst)) { // if he actually cast a spell
			monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
			return;
		}
	}
    
    if (shortestDistance == 1) {
        leashLength = 11; // If the ally is adjacent to a monster at the end of its leash, it shouldn't be prevented from attacking.
    } else {
        leashLength = 10;
    }
	
	if (closestMonster
		&& (distanceBetween(x, y, player.xLoc, player.yLoc) < leashLength || (monst->bookkeepingFlags & MONST_DOES_NOT_TRACK_LEADER))
		&& !(monst->info.flags & MONST_MAINTAINS_DISTANCE)) {
		
		// Blink toward an enemy?
		if ((monst->info.abilityFlags & MA_CAST_BLINK)
			&& ((monst->info.flags & MONST_ALWAYS_USE_ABILITY) || rand_percent(30))) {
			
			enemyMap = allocGrid();
			costMap = allocGrid();
			
			for (i=0; i<DCOLS; i++) {
				for (j=0; j<DROWS; j++) {
					if (cellHasTerrainFlag(i, j, T_OBSTRUCTS_PASSABILITY)) {
						costMap[i][j] = cellHasTerrainFlag(i, j, T_OBSTRUCTS_DIAGONAL_MOVEMENT) ? PDS_OBSTRUCTION : PDS_FORBIDDEN;
						enemyMap[i][j] = 0; // safeguard against OOS
					} else if (monsterAvoids(monst, i, j)) {
						costMap[i][j] = PDS_FORBIDDEN;
						enemyMap[i][j] = 0; // safeguard against OOS
					} else {
						costMap[i][j] = 1;
						enemyMap[i][j] = 10000;
					}
				}
			}
			
			for (target = monsters->nextCreature; target != NULL; target = target->nextCreature) {
				if (target != monst
					&& (!(target->bookkeepingFlags & MONST_SUBMERGED) || (monst->bookkeepingFlags & MONST_SUBMERGED))
					&& monstersAreEnemies(target, monst)
					&& !(target->bookkeepingFlags & MONST_CAPTIVE)
					&& distanceBetween(x, y, target->xLoc, target->yLoc) < shortestDistance
					&& traversiblePathBetween(monst, target->xLoc, target->yLoc)
					&& (!monsterAvoids(monst, target->xLoc, target->yLoc) || (target->info.flags & MONST_ATTACKABLE_THRU_WALLS))
					&& (!target->status[STATUS_INVISIBLE] || ((monst->info.flags & MONST_ALWAYS_USE_ABILITY) || rand_percent(33)))) {
					
					enemyMap[target->xLoc][target->yLoc] = 0;
					costMap[target->xLoc][target->yLoc] = 1;
				}
			}
			
			dijkstraScan(enemyMap, costMap, true);
			freeGrid(costMap);
			
			if (monsterBlinkToPreferenceMap(monst, enemyMap, false)) { // if he actually cast a spell
				monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
				freeGrid(enemyMap);
				return;
			}
			freeGrid(enemyMap);
		}
		
		targetLoc[0] = closestMonster->xLoc;
		targetLoc[1] = closestMonster->yLoc;
		moveMonsterPassivelyTowards(monst, targetLoc, false);
	} else if (monst->targetCorpseLoc[0]
			   && !monst->status[STATUS_POISONED]
			   && (!monst->status[STATUS_BURNING] || monst->status[STATUS_IMMUNE_TO_FIRE])) { // Going to start eating a corpse.
		moveMonsterPassivelyTowards(monst, monst->targetCorpseLoc, false);
		if (monst->xLoc == monst->targetCorpseLoc[0]
			&& monst->yLoc == monst->targetCorpseLoc[1]
			&& !(monst->bookkeepingFlags & MONST_ABSORBING)) {
			if (canSeeMonster(monst)) {
				monsterName(monstName, monst, true);
				sprintf(buf, "%s%s%s的尸体。", monstName, monsterText[monst->info.monsterID].absorbing, monst->targetCorpseName);
				messageWithColor(buf, &goodMessageColor, false);
			}
			monst->corpseAbsorptionCounter = 20;
			monst->bookkeepingFlags |= MONST_ABSORBING;
		}
	} else if ((monst->bookkeepingFlags & MONST_DOES_NOT_TRACK_LEADER)
			   || (distanceBetween(x, y, player.xLoc, player.yLoc) < 3 && (pmap[x][y].flags & IN_FIELD_OF_VIEW))) {
        
		monst->bookkeepingFlags &= ~MONST_GIVEN_UP_ON_SCENT;
		if (rand_percent(30)) {
			dir = randValidDirectionFrom(monst, x, y, true);
			if (dir != -1) {
				targetLoc[0] = x + nbDirs[dir][0];
				targetLoc[1] = y + nbDirs[dir][1];
				moveMonsterPassivelyTowards(monst, targetLoc, false);
			}
		}
	} else {
		if ((monst->info.abilityFlags & MA_CAST_BLINK)
			&& !(monst->bookkeepingFlags & MONST_GIVEN_UP_ON_SCENT)
			&& distanceBetween(x, y, player.xLoc, player.yLoc) > 10
			&& monsterBlinkToPreferenceMap(monst, scentMap, true)) { // if he actually cast a spell
			
			monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
			return;
		}
		dir = scentDirection(monst);
		if (dir == -1 || (monst->bookkeepingFlags & MONST_GIVEN_UP_ON_SCENT)) {
			monst->bookkeepingFlags |= MONST_GIVEN_UP_ON_SCENT;
			moveTowardLeader(monst);
		} else {
			targetLoc[0] = x + nbDirs[dir][0];
			targetLoc[1] = y + nbDirs[dir][1];
			moveMonsterPassivelyTowards(monst, targetLoc, false);
		}
	}
}

void monstersTurn(creature *monst) {
	short x, y, playerLoc[2], targetLoc[2], dir, shortestDistance;
	boolean alreadyAtBestScent;
	char buf[COLS*3], buf2[COLS*3];
	creature *ally, *target, *closestMonster;
	
	monst->turnsSpentStationary++;
	
	if (monst->corpseAbsorptionCounter) {
		if (monst->xLoc == monst->targetCorpseLoc[0]
			&& monst->yLoc == monst->targetCorpseLoc[1]
			&& (monst->bookkeepingFlags & MONST_ABSORBING)) {
			if (!--monst->corpseAbsorptionCounter) {
				monst->targetCorpseLoc[0] = monst->targetCorpseLoc[1] = 0;
				if (monst->absorbBehavior) {
					monst->info.flags |= monst->absorptionFlags;
				} else {
					monst->info.abilityFlags |= monst->absorptionFlags;
				}
				monst->absorbXPXP -= XPXP_NEEDED_FOR_ABSORB;
				monst->bookkeepingFlags &= ~(MONST_ABSORBING | MONST_ALLY_ANNOUNCED_HUNGER);
				
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
				if (canSeeMonster(monst)) {
					monsterName(buf2, monst, true);
					sprintf(buf, "%s停止%s%s的尸体。", buf2, monsterText[monst->info.monsterID].absorbing, monst->targetCorpseName);
					messageWithColor(buf, &goodMessageColor, false);
					sprintf(buf, "%s%s！", buf2,
							(monst->absorbBehavior ? monsterBehaviorFlagDescriptions[unflag(monst->absorptionFlags)] :
							 monsterAbilityFlagDescriptions[unflag(monst->absorptionFlags)]));
					resolvePronounEscapes(buf, monst);
					messageWithColor(buf, &goodMessageColor, false);
				}
				monst->absorptionFlags = 0;
			}
			monst->ticksUntilTurn = 100;
			return;
		} else if (!--monst->corpseAbsorptionCounter) {
			monst->targetCorpseLoc[0] = monst->targetCorpseLoc[1] = 0; // lost its chance
			monst->bookkeepingFlags &= ~MONST_ABSORBING;
		} else if (monst->bookkeepingFlags & MONST_ABSORBING) {
			monst->bookkeepingFlags &= ~MONST_ABSORBING; // absorbing but not on the corpse
			if (monst->corpseAbsorptionCounter <= 15) {
				monst->targetCorpseLoc[0] = monst->targetCorpseLoc[1] = 0; // lost its chance
			}
		}
	}
    
    if (monst->info.DFChance
        && (monst->info.flags & MONST_GETS_TURN_ON_ACTIVATION)
        && rand_percent(monst->info.DFChance)) {
        
        spawnDungeonFeature(monst->xLoc, monst->yLoc, &dungeonFeatureCatalog[monst->info.DFType], true, false);
    }
    
    applyInstantTileEffectsToCreature(monst); // Paralysis, confusion etc. take effect before the monster can move.
	
	// if the monster is paralyzed, entranced or chained, this is where its turn ends.
	if (monst->status[STATUS_PARALYZED] || monst->status[STATUS_ENTRANCED] || (monst->bookkeepingFlags & MONST_CAPTIVE)) {
		monst->ticksUntilTurn = monst->movementSpeed;
		if ((monst->bookkeepingFlags & MONST_CAPTIVE) && monst->carriedItem) {
			makeMonsterDropItem(monst);
		}
		return;
	}
    
    if (monst->bookkeepingFlags & MONST_IS_DYING) {
        return;
    }
	
	monst->ticksUntilTurn = monst->movementSpeed / 3; // will be later overwritten by movement or attack
	
	x = monst->xLoc;
	y = monst->yLoc;
	
	// Sleepers can awaken, but it takes a whole turn.
	if (monst->creatureState == MONSTER_SLEEPING) {
		monst->ticksUntilTurn = monst->movementSpeed;
		updateMonsterState(monst);
		return;
	}
	
	// Update creature state if appropriate.
	updateMonsterState(monst);
	
	if (monst->creatureState == MONSTER_SLEEPING) {
		monst->ticksUntilTurn = monst->movementSpeed;
		return;
	}
	
	// and move the monster.
	
	// immobile monsters can only use special abilities:
	if (monst->info.flags & MONST_IMMOBILE) {
		if (monst->info.abilityFlags & MAGIC_ATTACK) {
			if (monstUseMagic(monst)) { // if he actually cast a spell
				monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
				return;
			}
		}
		monst->ticksUntilTurn = monst->attackSpeed;
		return;
	}
	
	// discordant monsters
	if (monst->status[STATUS_DISCORDANT] && monst->creatureState != MONSTER_FLEEING) {
		shortestDistance = max(DROWS, DCOLS);
		closestMonster = NULL;
		CYCLE_MONSTERS_AND_PLAYERS(target) {
			if (target != monst
				&& (!(target->bookkeepingFlags & MONST_SUBMERGED) || (monst->bookkeepingFlags & MONST_SUBMERGED))
				&& monstersAreEnemies(target, monst)
				&& !(target->bookkeepingFlags & MONST_CAPTIVE)
				&& distanceBetween(x, y, target->xLoc, target->yLoc) < shortestDistance
				&& traversiblePathBetween(monst, target->xLoc, target->yLoc)
				&& (!monsterAvoids(monst, target->xLoc, target->yLoc) || (target->info.flags & MONST_ATTACKABLE_THRU_WALLS))
				&& (!target->status[STATUS_INVISIBLE] || rand_percent(33))) {
				
				shortestDistance = distanceBetween(x, y, target->xLoc, target->yLoc);
				closestMonster = target;
			}
		}
		if (closestMonster && monstUseMagic(monst)) {
			monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
			return;
		}
		if (closestMonster && !(monst->info.flags & MONST_MAINTAINS_DISTANCE)) {
			targetLoc[0] = closestMonster->xLoc;
			targetLoc[1] = closestMonster->yLoc;
			if (moveMonsterPassivelyTowards(monst, targetLoc, false)) {
				return;
			}
		}
	}
	
	// hunting
	if ((monst->creatureState == MONSTER_TRACKING_SCENT
		|| (monst->creatureState == MONSTER_ALLY && monst->status[STATUS_DISCORDANT]))
		// eels don't charge if you're not in the water
		&& (!(monst->info.flags & MONST_RESTRICTED_TO_LIQUID) || cellHasTMFlag(player.xLoc, player.yLoc, TM_ALLOWS_SUBMERGING))) {
		
		// magic users sometimes cast spells
		if (monst->info.abilityFlags & (MAGIC_ATTACK | MA_CAST_BLINK)) {
			if (monstUseMagic(monst)
				|| ((monst->info.abilityFlags & MA_CAST_BLINK)
					&& ((monst->info.flags & MONST_ALWAYS_USE_ABILITY) || rand_percent(30))
					&& monsterBlinkToPreferenceMap(monst, scentMap, true))) { // if he actually cast a spell
					
					monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
					return;
			}
		}
		
		// if the monster is adjacent to an ally and not adjacent to the player, attack the ally
		if (distanceBetween(x, y, player.xLoc, player.yLoc) > 1
			|| diagonalBlocked(x, y, player.xLoc, player.yLoc)) {
			for (ally = monsters->nextCreature; ally != NULL; ally = ally->nextCreature) {
				if (monstersAreEnemies(monst, ally) && distanceBetween(x, y, ally->xLoc, ally->yLoc) == 1
					&& (!ally->status[STATUS_INVISIBLE] || rand_percent(33))) {
					targetLoc[0] = ally->xLoc;
					targetLoc[1] = ally->yLoc;
					if (moveMonsterPassivelyTowards(monst, targetLoc, true)) { // attack
						return;
					}
				}
			}
		}
		
		if ((monst->status[STATUS_LEVITATING] || monst->info.flags & MONST_RESTRICTED_TO_LIQUID || monst->bookkeepingFlags & MONST_SUBMERGED
			 || (monst->info.flags & MONST_IMMUNE_TO_WEBS && monst->info.abilityFlags & MA_SHOOTS_WEBS))
			&& (distanceBetween(x, y, player.xLoc, player.yLoc)) / 100 < monst->info.sightRadius
			&& pmap[x][y].flags & IN_FIELD_OF_VIEW) {
			playerLoc[0] = player.xLoc;
			playerLoc[1] = player.yLoc;
			moveMonsterPassivelyTowards(monst, playerLoc, true); // attack
			return;
		}
		if (scentMap[x][y] == 0) {
			return;
		}
		
		dir = scentDirection(monst);
		if (dir == NO_DIRECTION) {
			alreadyAtBestScent = isLocalScentMaximum(monst->xLoc, monst->yLoc);
			if (alreadyAtBestScent && monst->creatureState != MONSTER_ALLY) {
				monst->creatureState = MONSTER_WANDERING;
				chooseNewWanderDestination(monst);
			}
		} else {
			moveMonster(monst, nbDirs[dir][0], nbDirs[dir][1]);
		}
	} else if (monst->creatureState == MONSTER_FLEEING) {
		// fleeing
		if ((monst->info.abilityFlags & MA_CAST_BLINK)
			&& ((monst->info.flags & MONST_ALWAYS_USE_ABILITY) || rand_percent(30))
			&& monsterBlinkToSafety(monst)) {
			return;
		}
		
		if (pmap[x][y].flags & IN_FIELD_OF_VIEW) {
			if (monst->safetyMap) {
				freeGrid(monst->safetyMap);
				monst->safetyMap = NULL;
			}
			if (!rogue.updatedSafetyMapThisTurn) {
				updateSafetyMap();
			}
			dir = nextStep(safetyMap, monst->xLoc, monst->yLoc, NULL, true);
		} else {
			if (!monst->safetyMap) {
				monst->safetyMap = allocGrid();
				copyGrid(monst->safetyMap, safetyMap);
			}
			dir = nextStep(monst->safetyMap, monst->xLoc, monst->yLoc, NULL, true);
		}
		if (dir != -1) {
			targetLoc[0] = x + nbDirs[dir][0];
			targetLoc[1] = y + nbDirs[dir][1];
		}
		if (dir == -1 || (!moveMonster(monst, nbDirs[dir][0], nbDirs[dir][1]) && !moveMonsterPassivelyTowards(monst, targetLoc, true))) {
			CYCLE_MONSTERS_AND_PLAYERS(ally) {
				if (!monst->status[STATUS_MAGICAL_FEAR] // Fearful monsters will never attack.
					&& monstersAreEnemies(monst, ally)
					&& ally != monst // Otherwise, discordant cornered fleeing enemies will slit their wrists in grotesque fashion.
					&& distanceBetween(x, y, ally->xLoc, ally->yLoc) <= 1) {
					
					moveMonster(monst, ally->xLoc - x, ally->yLoc - y); // attack the player if cornered
					return;
				}
			}
		}
		return;
	} else if (monst->creatureState == MONSTER_WANDERING
			   // eels wander if you're not in water
			   || ((monst->info.flags & MONST_RESTRICTED_TO_LIQUID) && !cellHasTMFlag(player.xLoc, player.yLoc, TM_ALLOWS_SUBMERGING))) {
		
		// if we're standing in harmful terrain and there is a way to escape it, spend this turn escaping it.
		if (cellHasTerrainFlag(x, y, (T_HARMFUL_TERRAIN & ~T_IS_FIRE))
			|| (cellHasTerrainFlag(x, y, T_IS_FIRE) && !monst->status[STATUS_IMMUNE_TO_FIRE])) {
			if (!rogue.updatedMapToSafeTerrainThisTurn) {
				updateSafeTerrainMap();
			}
			
			if ((monst->info.abilityFlags & MA_CAST_BLINK)
				&& monsterBlinkToPreferenceMap(monst, rogue.mapToSafeTerrain, false)) {
				monst->ticksUntilTurn = monst->attackSpeed * (monst->info.flags & MONST_CAST_SPELLS_SLOWLY ? 2 : 1);
				return;
			}
			
			dir = nextStep(rogue.mapToSafeTerrain, x, y, monst, true);
			if (dir != -1) {
				targetLoc[0] = x + nbDirs[dir][0];
				targetLoc[1] = y + nbDirs[dir][1];
				if (moveMonsterPassivelyTowards(monst, targetLoc, true)) {
					return;
				}
			}
		}
		
		// if a captive leader is captive and healthy enough to withstand an attack, then approach or attack him.
		if ((monst->bookkeepingFlags & MONST_FOLLOWER)
            && (monst->leader->bookkeepingFlags & MONST_CAPTIVE)
			&& monst->leader->currentHP > monst->info.damage.upperBound * monsterDamageAdjustmentAmount(monst)
            && !diagonalBlocked(monst->xLoc, monst->yLoc, monst->leader->xLoc, monst->leader->yLoc)) {
            
            if (distanceBetween(monst->xLoc, monst->yLoc, monst->leader->xLoc, monst->leader->yLoc) == 1) {
                // Attack if adjacent.
                attack(monst, monst->leader, false);
                monst->ticksUntilTurn = monst->attackSpeed;
                return;
            } else {
                // Otherwise, approach.
                moveTowardLeader(monst);
                return;
            }
		}
		
		// if the monster is adjacent to an ally and not fleeing, attack the ally
		if (monst->creatureState == MONSTER_WANDERING) {
			for (ally = monsters->nextCreature; ally != NULL; ally = ally->nextCreature) {
				if (monstersAreEnemies(monst, ally) && distanceBetween(x, y, ally->xLoc, ally->yLoc) == 1
					&& (!ally->status[STATUS_INVISIBLE] || rand_percent(33))) {
                    
					monst->creatureState = MONSTER_TRACKING_SCENT; // this alerts the monster that you're nearby
					targetLoc[0] = ally->xLoc;
					targetLoc[1] = ally->yLoc;
					if (moveMonsterPassivelyTowards(monst, targetLoc, true)) {
						return;
					}
				}
			}
		}
		
		// if you're a follower, don't get separated from the pack
		if ((monst->bookkeepingFlags & MONST_FOLLOWER)
            && distanceBetween(x, y, monst->leader->xLoc, monst->leader->yLoc) > 2) {
            
			moveTowardLeader(monst);
		} else {
            // Step toward the chosen waypoint.
            dir = NO_DIRECTION;
            if (isValidWanderDestination(monst, monst->targetWaypointIndex)) {
                dir = nextStep(rogue.wpDistance[monst->targetWaypointIndex], monst->xLoc, monst->yLoc, monst, false);
            }
            // If there's no path forward, call that waypoint finished and pick a new one.
            if (!isValidWanderDestination(monst, monst->targetWaypointIndex)
                || dir == NO_DIRECTION) {
                
                chooseNewWanderDestination(monst);
                if (isValidWanderDestination(monst, monst->targetWaypointIndex)) {
                    dir = nextStep(rogue.wpDistance[monst->targetWaypointIndex], monst->xLoc, monst->yLoc, monst, false);
                }
            }
            // If there's still no path forward, step randomly as though flitting.
            // (This is how eels wander in deep water.)
            if (dir == NO_DIRECTION) {
                dir = randValidDirectionFrom(monst, x, y, true);
            }
            if (dir != NO_DIRECTION) {
				targetLoc[0] = x + nbDirs[dir][0];
				targetLoc[1] = y + nbDirs[dir][1];
				if (moveMonsterPassivelyTowards(monst, targetLoc, true)) {
					return;
				}
            }
		}
	} else if (monst->creatureState == MONSTER_ALLY) {
		moveAlly(monst);
	}
}

boolean canPass(creature *mover, creature *blocker) {
    
    if (blocker == &player) {
        return false;
    }
	
	if (blocker->status[STATUS_CONFUSED]
		|| blocker->status[STATUS_STUCK]
		|| blocker->status[STATUS_PARALYZED]
		|| blocker->status[STATUS_ENTRANCED]
		|| mover->status[STATUS_ENTRANCED]) {
		
		return false;
	}
	
	if ((blocker->bookkeepingFlags & (MONST_CAPTIVE | MONST_ABSORBING))
		|| (blocker->info.flags & MONST_IMMOBILE)) {
		return false;
	}
	
	if (monstersAreEnemies(mover, blocker)) {
		return false;
	}
	
	if (blocker->leader == mover) {
		return true;
	}
	
	if (mover->leader == blocker) {
		return false;
	}
	
	return (monstersAreTeammates(mover, blocker)
			&& blocker->currentHP < mover->currentHP);
}
boolean isPassableOrSecretDoor(short x, short y) {
	return (!cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY)
            || (cellHasTMFlag(x, y, TM_IS_SECRET) && !(discoveredTerrainFlagsAtLoc(x, y) & T_OBSTRUCTS_PASSABILITY)));
}

void executeMonsterMovement(creature *monst, short newX, short newY) {
    pmap[monst->xLoc][monst->yLoc].flags &= ~HAS_MONSTER;
    refreshDungeonCell(monst->xLoc, monst->yLoc);
    monst->turnsSpentStationary = 0;
    monst->xLoc = newX;
    monst->yLoc = newY;
    pmap[newX][newY].flags |= HAS_MONSTER;
    if ((monst->bookkeepingFlags & MONST_SUBMERGED) && !cellHasTMFlag(newX, newY, TM_ALLOWS_SUBMERGING)) {
        monst->bookkeepingFlags &= ~MONST_SUBMERGED;
    }
    if (playerCanSee(newX, newY)
        && cellHasTMFlag(newX, newY, TM_IS_SECRET)
        && cellHasTerrainFlag(newX, newY, T_OBSTRUCTS_PASSABILITY)) {
        
        discover(newX, newY); // if you see a monster use a secret door, you discover it
    }
    refreshDungeonCell(newX, newY);
    monst->ticksUntilTurn = monst->movementSpeed;
    applyInstantTileEffectsToCreature(monst);
}

// Tries to move the given monster in the given vector; returns true if the move was legal
// (including attacking player, vomiting or struggling in vain)
// Be sure that dx, dy are both in the range [-1, 1] or the move will sometimes fail due to the diagonal check.
boolean moveMonster(creature *monst, short dx, short dy) {
	short x = monst->xLoc, y = monst->yLoc;
	short newX, newY;
	short confusedDirection, swarmDirection;
	creature *defender = NULL;
	
	newX = x + dx;
	newY = y + dy;
	
	if (!coordinatesAreInMap(newX, newY)) {
		//DEBUG printf("\nProblem! Monster trying to move more than one space at a time.");
		return false;
	}
	
	// vomiting
	if (monst->status[STATUS_NAUSEOUS] && rand_percent(25)) {
		vomit(monst);
		monst->ticksUntilTurn = monst->movementSpeed;
		return true;
	}
	
	// move randomly?
	if (!monst->status[STATUS_ENTRANCED]) {
		if (monst->status[STATUS_CONFUSED]) {
			confusedDirection = randValidDirectionFrom(monst, x, y, false);
			if (confusedDirection != -1) {
				dx = nbDirs[confusedDirection][0];
				dy = nbDirs[confusedDirection][1];
			}
		} else if ((monst->info.flags & MONST_FLITS) && !(monst->bookkeepingFlags & MONST_SEIZING) && rand_percent(33)) {
			confusedDirection = randValidDirectionFrom(monst, x, y, true);
			if (confusedDirection != -1) {
				dx = nbDirs[confusedDirection][0];
				dy = nbDirs[confusedDirection][1];
			}
		}
	}
	
	newX = x + dx;
	newY = y + dy;
	
	// Liquid-based monsters should never move or attack outside of liquid.
	if ((monst->info.flags & MONST_RESTRICTED_TO_LIQUID) && !cellHasTMFlag(newX, newY, TM_ALLOWS_SUBMERGING)) {
		return false;
	}
	
	// Caught in spiderweb?
	if (monst->status[STATUS_STUCK] && !(pmap[newX][newY].flags & (HAS_PLAYER | HAS_MONSTER))
		&& cellHasTerrainFlag(x, y, T_ENTANGLES) && !(monst->info.flags & MONST_IMMUNE_TO_WEBS)) {
		if (--monst->status[STATUS_STUCK]) {
			monst->ticksUntilTurn = monst->movementSpeed;
			return true;
		} else if (tileCatalog[pmap[x][y].layers[SURFACE]].flags & T_ENTANGLES) {
			pmap[x][y].layers[SURFACE] = NOTHING;
		}
	}
	
	if (pmap[newX][newY].flags & (HAS_MONSTER | HAS_PLAYER)) {
		defender = monsterAtLoc(newX, newY);
	} else {
		if (monst->bookkeepingFlags & MONST_SEIZED) {
			for (defender = monsters->nextCreature; defender != NULL; defender = defender->nextCreature) {
				if ((defender->bookkeepingFlags & MONST_SEIZING)
					&& monstersAreEnemies(monst, defender)
					&& distanceBetween(monst->xLoc, monst->yLoc, defender->xLoc, defender->yLoc) == 1
					&& !monst->status[STATUS_LEVITATING]) {
					
					monst->ticksUntilTurn = monst->movementSpeed;
					return true;
				}
			}
			monst->bookkeepingFlags &= ~MONST_SEIZED; // failsafe
		}
		if (monst->bookkeepingFlags & MONST_SEIZING) {
			monst->bookkeepingFlags &= ~MONST_SEIZING;
		}
	}
	
	if (((defender && (defender->info.flags & MONST_ATTACKABLE_THRU_WALLS))
		 || (isPassableOrSecretDoor(newX, newY)
             && !diagonalBlocked(x, y, newX, newY)
			 && isPassableOrSecretDoor(x, y)))
		&& (!defender || monst->status[STATUS_CONFUSED] || monst->status[STATUS_ENTRANCED] ||
			canPass(monst, defender) || monstersAreEnemies(monst, defender))) {
			// if it's a legal move
            
			if (defender) {
				if (canPass(monst, defender)) {
                    
                    // swap places
                    pmap[defender->xLoc][defender->yLoc].flags &= ~HAS_MONSTER;
                    refreshDungeonCell(defender->xLoc, defender->yLoc);
                    
                    pmap[monst->xLoc][monst->yLoc].flags &= ~HAS_MONSTER;
                    refreshDungeonCell(monst->xLoc, monst->yLoc);
                    
                    monst->xLoc = newX;
                    monst->yLoc = newY;
                    pmap[monst->xLoc][monst->yLoc].flags |= HAS_MONSTER;
                    
                    if (monsterAvoids(defender, x, y)) { // don't want a flying monster to swap a non-flying monster into lava!
                        getQualifyingPathLocNear(&(defender->xLoc), &(defender->yLoc), x, y, true,
                                                 forbiddenFlagsForMonster(&(defender->info)), HAS_PLAYER,
                                                 forbiddenFlagsForMonster(&(defender->info)), (HAS_PLAYER | HAS_MONSTER | HAS_UP_STAIRS | HAS_DOWN_STAIRS), false);
                    } else {
                        defender->xLoc = x;
                        defender->yLoc = y;
                    }
                    pmap[defender->xLoc][defender->yLoc].flags |= HAS_MONSTER;
                    
                    refreshDungeonCell(monst->xLoc, monst->yLoc);
                    refreshDungeonCell(defender->xLoc, defender->yLoc);
                    
                    monst->ticksUntilTurn = monst->movementSpeed;
                    return true;
                }
                
                // Sights are set on an enemy monster. Would we rather swarm than attack?
                swarmDirection = monsterSwarmDirection(monst, defender);
                if (swarmDirection != NO_DIRECTION) {
                    newX = monst->xLoc + nbDirs[swarmDirection][0];
                    newY = monst->yLoc + nbDirs[swarmDirection][1];
                    executeMonsterMovement(monst, newX, newY);
                    return true;
                } else {
                    // attacking another monster!
                    monst->ticksUntilTurn = monst->attackSpeed;
                    if (!((monst->info.abilityFlags & MA_SEIZES) && !(monst->bookkeepingFlags & MONST_SEIZING))) {
                        // Bog monsters and krakens won't surface on the turn that they seize their target.
                        monst->bookkeepingFlags &= ~MONST_SUBMERGED;
                    }
                    refreshDungeonCell(x, y);
                    attack(monst, defender, false);
                }
                return true;
			} else {
				// okay we're moving!
                executeMonsterMovement(monst, newX, newY);
				return true;
			}
		}
	return false;
}

void clearStatus(creature *monst) {
	short i;
	
	for (i=0; i<NUMBER_OF_STATUS_EFFECTS; i++) {
		monst->status[i] = monst->maxStatus[i] = 0;
	}
}

// Bumps a creature to a random nearby hospitable cell.
void findAlternativeHomeFor(creature *monst, short *x, short *y, boolean chooseRandomly) {
	short sCols[DCOLS], sRows[DROWS], i, j, maxPermissibleDifference, dist;
	
    fillSequentialList(sCols, DCOLS);
    fillSequentialList(sRows, DROWS);
	if (chooseRandomly) {
		shuffleList(sCols, DCOLS);
		shuffleList(sRows, DROWS);
	}
	
	for (maxPermissibleDifference = 1; maxPermissibleDifference < max(DCOLS, DROWS); maxPermissibleDifference++) {
		for (i=0; i < DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				dist = abs(sCols[i] - monst->xLoc) + abs(sRows[j] - monst->yLoc);
				if (dist <= maxPermissibleDifference
					&& dist > 0
					&& !(pmap[sCols[i]][sRows[j]].flags & (HAS_PLAYER | HAS_MONSTER))
					&& !monsterAvoids(monst, sCols[i], sRows[j])
					&& !(monst == &player && cellHasTerrainFlag(sCols[i], sRows[j], T_PATHING_BLOCKER))) {
					
					// Success!
					*x = sCols[i];
					*y = sRows[j];
					return;
				}
			}
		}
	}
	// Failure!
	*x = *y = -1;
}

// blockingMap is optional
boolean getQualifyingLocNear(short loc[2],
							 short x, short y,
							 boolean hallwaysAllowed,
							 char blockingMap[DCOLS][DROWS],
							 unsigned long forbiddenTerrainFlags,
							 unsigned long forbiddenMapFlags,
							 boolean forbidLiquid,
							 boolean deterministic) {
	short i, j, k, candidateLocs, randIndex;
	
	candidateLocs = 0;
	
	// count up the number of candidate locations
	for (k=0; k<max(DROWS, DCOLS) && !candidateLocs; k++) {
		for (i = x-k; i <= x+k; i++) {
			for (j = y-k; j <= y+k; j++) {
				if (coordinatesAreInMap(i, j)
					&& (i == x-k || i == x+k || j == y-k || j == y+k)
					&& (!blockingMap || !blockingMap[i][j])
					&& !cellHasTerrainFlag(i, j, forbiddenTerrainFlags)
					&& !(pmap[i][j].flags & forbiddenMapFlags)
					&& (!forbidLiquid || pmap[i][j].layers[LIQUID] == NOTHING)
					&& (hallwaysAllowed || passableArcCount(i, j) < 2)) {
					candidateLocs++;
				}
			}
		}
	}
	
	if (candidateLocs == 0) {
		return false;
	}
	
	// and pick one
	if (deterministic) {
		randIndex = 1 + candidateLocs / 2;
	} else {
		randIndex = rand_range(1, candidateLocs);
	}
	
	for (k=0; k<max(DROWS, DCOLS); k++) {
		for (i = x-k; i <= x+k; i++) {
			for (j = y-k; j <= y+k; j++) {
				if (coordinatesAreInMap(i, j)
					&& (i == x-k || i == x+k || j == y-k || j == y+k)
					&& (!blockingMap || !blockingMap[i][j])
					&& !cellHasTerrainFlag(i, j, forbiddenTerrainFlags)
					&& !(pmap[i][j].flags & forbiddenMapFlags)
					&& (!forbidLiquid || pmap[i][j].layers[LIQUID] == NOTHING)
					&& (hallwaysAllowed || passableArcCount(i, j) < 2)) {
					if (--randIndex == 0) {
						loc[0] = i;
						loc[1] = j;
						return true;
					}
				}
			}
		}
	}
	
#ifdef BROGUE_ASSERTS
    assert(false);
#endif
	return false; // should never reach this point
}

boolean getQualifyingGridLocNear(short loc[2],
								 short x, short y,
								 boolean grid[DCOLS][DROWS],
								 boolean deterministic) {
	short i, j, k, candidateLocs, randIndex;
	
	candidateLocs = 0;
	
	// count up the number of candidate locations
	for (k=0; k<max(DROWS, DCOLS) && !candidateLocs; k++) {
		for (i = x-k; i <= x+k; i++) {
			for (j = y-k; j <= y+k; j++) {
				if (coordinatesAreInMap(i, j)
					&& (i == x-k || i == x+k || j == y-k || j == y+k)
					&& grid[i][j]) {
					
					candidateLocs++;
				}
			}
		}
	}
	
	if (candidateLocs == 0) {
		return false;
	}
	
	// and pick one
	if (deterministic) {
		randIndex = 1 + candidateLocs / 2;
	} else {
		randIndex = rand_range(1, candidateLocs);
	}
	
	for (k=0; k<max(DROWS, DCOLS); k++) {
		for (i = x-k; i <= x+k; i++) {
			for (j = y-k; j <= y+k; j++) {
				if (coordinatesAreInMap(i, j)
					&& (i == x-k || i == x+k || j == y-k || j == y+k)
					&& grid[i][j]) {
					
					if (--randIndex == 0) {
						loc[0] = i;
						loc[1] = j;
						return true;
					}
				}
			}
		}
	}
    
#ifdef BROGUE_ASSERTS
    assert(false);
#endif
	return false; // should never reach this point
}

void makeMonsterDropItem(creature *monst) {
	short x, y;
    getQualifyingPathLocNear(&x, &y, monst->xLoc, monst->yLoc, true,
                             T_DIVIDES_LEVEL | T_OBSTRUCTS_ITEMS, 0,
                             0, (HAS_PLAYER | HAS_UP_STAIRS | HAS_DOWN_STAIRS | HAS_ITEM), false);
	//getQualifyingLocNear(loc, monst->xLoc, monst->yLoc, true, 0, (T_OBSTRUCTS_ITEMS), (HAS_ITEM), false, false);
	placeItem(monst->carriedItem, x, y);
	monst->carriedItem = NULL;
	refreshDungeonCell(x, y);
}

void demoteMonsterFromLeadership(creature *monst) {
	creature *follower, *newLeader = NULL;
	
	monst->bookkeepingFlags &= ~MONST_LEADER;
	if (monst->mapToMe) {
		freeGrid(monst->mapToMe);
		monst->mapToMe = NULL;
	}
	for (follower = monsters->nextCreature; follower != NULL; follower = follower->nextCreature) {
		if (follower->leader == monst && monst != follower) { // used to use monstersAreTeammates()
			if (follower->bookkeepingFlags & MONST_BOUND_TO_LEADER) {
				// gonna die in playerTurnEnded().
				follower->leader = NULL;
				follower->bookkeepingFlags &= ~MONST_FOLLOWER;
			} else if (newLeader) {
				follower->leader = newLeader;
                follower->targetWaypointIndex = monst->targetWaypointIndex;
                if (follower->targetWaypointIndex >= 0) {
                    follower->waypointAlreadyVisited[follower->targetWaypointIndex] = false;
                }
			} else {
				newLeader = follower;
				follower->bookkeepingFlags |= MONST_LEADER;
				follower->bookkeepingFlags &= ~MONST_FOLLOWER;
				follower->leader = NULL;
			}
		}
	}
	for (follower = dormantMonsters->nextCreature; follower != NULL; follower = follower->nextCreature) {
		if (follower->leader == monst && monst != follower) {
			follower->leader = NULL;
			follower->bookkeepingFlags &= ~MONST_FOLLOWER;
		}
	}
}

// Makes a monster dormant, or awakens it from that state
void toggleMonsterDormancy(creature *monst) {
	creature *prevMonst;
	//short loc[2] = {0, 0};
	
	for (prevMonst = dormantMonsters; prevMonst != NULL; prevMonst = prevMonst->nextCreature) {
		if (prevMonst->nextCreature == monst) {
			// Found it! It's dormant. Wake it up.
			
			// Remove it from the dormant chain.
			prevMonst->nextCreature = monst->nextCreature;
			
			// Add it to the normal chain.
			monst->nextCreature = monsters->nextCreature;
			monsters->nextCreature = monst;
			
			pmap[monst->xLoc][monst->yLoc].flags &= ~HAS_DORMANT_MONSTER;
			
			// Does it need a new location?
			if (pmap[monst->xLoc][monst->yLoc].flags & (HAS_MONSTER | HAS_PLAYER)) { // Occupied!
                getQualifyingPathLocNear(&(monst->xLoc), &(monst->yLoc), monst->xLoc, monst->yLoc, true,
                                         T_DIVIDES_LEVEL & avoidedFlagsForMonster(&(monst->info)), HAS_PLAYER,
                                         avoidedFlagsForMonster(&(monst->info)), (HAS_PLAYER | HAS_MONSTER | HAS_UP_STAIRS | HAS_DOWN_STAIRS), false);
//				getQualifyingLocNear(loc, monst->xLoc, monst->yLoc, true, 0, T_PATHING_BLOCKER, (HAS_PLAYER | HAS_MONSTER), false, false);
//				monst->xLoc = loc[0];
//				monst->yLoc = loc[1];
			}
			
			// Miscellaneous transitional tasks.
			// Don't want it to move before the player has a chance to react.
			monst->ticksUntilTurn = 200;
			
			pmap[monst->xLoc][monst->yLoc].flags |= HAS_MONSTER;
			monst->bookkeepingFlags &= ~MONST_IS_DORMANT;
			return;
		}
	}
	
	for (prevMonst = monsters; prevMonst != NULL; prevMonst = prevMonst->nextCreature) {
		if (prevMonst->nextCreature == monst) {
			// Found it! It's alive. Put it into dormancy.
			// Remove it from the monsters chain.
			prevMonst->nextCreature = monst->nextCreature;
			// Add it to the dormant chain.
			monst->nextCreature = dormantMonsters->nextCreature;
			dormantMonsters->nextCreature = monst;
			// Miscellaneous transitional tasks.
			pmap[monst->xLoc][monst->yLoc].flags &= ~HAS_MONSTER;
			pmap[monst->xLoc][monst->yLoc].flags |= HAS_DORMANT_MONSTER;
			monst->bookkeepingFlags |= MONST_IS_DORMANT;
			return;
		}
	}
}

void monsterDetails(char buf[], creature *monst) {
	char monstName[COLS*3], capMonstName[COLS*3], theItemName[COLS*3], newText[10*COLS];
	short i, j, combatMath, combatMath2, playerKnownAverageDamage, playerKnownMaxDamage, commaCount, realArmorValue;
	boolean anyFlags, printedDominationText = false;
	item *theItem;
	
	buf[0] = '\0';
	commaCount = 0;
	
	monsterName(monstName, monst, true);
	strcpy(capMonstName, monstName);
	upperCase(capMonstName);
	
	if (!(monst->info.flags & MONST_RESTRICTED_TO_LIQUID)
		 || cellHasTMFlag(monst->xLoc, monst->yLoc, TM_ALLOWS_SUBMERGING)) {
		// If the monster is not a beached whale, print the ordinary flavor text.
		sprintf(newText, "     %s\n     ", monsterText[monst->info.monsterID].flavorText);
		strcat(buf, newText);
	}
    
    if (monst->mutationIndex >= 0) {
        i = strlen(buf);
        i = encodeMessageColor(buf, i, mutationCatalog[monst->mutationIndex].textColor);
        strcpy(newText, mutationCatalog[monst->mutationIndex].description);
        resolvePronounEscapes(newText, monst);
        upperCase(newText);
        strcat(newText, "\n     ");
        strcat(buf, newText);
        i = strlen(buf);
        i = encodeMessageColor(buf, i, &white);
    }
	
	if (!(monst->info.flags & MONST_ATTACKABLE_THRU_WALLS)
		&& cellHasTerrainFlag(monst->xLoc, monst->yLoc, T_OBSTRUCTS_PASSABILITY)) {
		// If the monster is trapped in impassible terrain, explain as much.
		sprintf(newText, "%s被%s困住了。\n     ",
				capMonstName,
				//(tileCatalog[pmap[monst->xLoc][monst->yLoc].layers[layerWithFlag(monst->xLoc, monst->yLoc, T_OBSTRUCTS_PASSABILITY)]].mechFlags & TM_STAND_IN_TILE) ? "in" : "on",
				tileCatalog[pmap[monst->xLoc][monst->yLoc].layers[layerWithFlag(monst->xLoc, monst->yLoc, T_OBSTRUCTS_PASSABILITY)]].description);
		strcat(buf, newText);
	}
	
	if (!rogue.armor || (rogue.armor->flags & ITEM_IDENTIFIED)) {
		combatMath2 = hitProbability(monst, &player);
	} else {
		realArmorValue = player.info.defense;
		player.info.defense = (armorTable[rogue.armor->kind].range.upperBound + armorTable[rogue.armor->kind].range.lowerBound) / 2;
		player.info.defense += 10 * strengthModifier(rogue.armor);
		combatMath2 = hitProbability(monst, &player);
		player.info.defense = realArmorValue;
	}
	
	if ((monst->info.flags & MONST_RESTRICTED_TO_LIQUID) && !cellHasTMFlag(monst->xLoc, monst->yLoc, TM_ALLOWS_SUBMERGING)) {
		sprintf(newText, "     %s在地面上无助的扭动。\n     ", capMonstName);
	} else if (rogue.armor
			   && (rogue.armor->flags & ITEM_RUNIC)
			   && (rogue.armor->flags & ITEM_RUNIC_IDENTIFIED)
			   && rogue.armor->enchant2 == A_IMMUNITY
			   && rogue.armor->vorpalEnemy == monst->info.monsterID) {
		
		sprintf(newText, "你的盔甲使你对%s的攻击完全免疫。\n     ", monstName);
	} else if (monst->info.damage.upperBound * monsterDamageAdjustmentAmount(monst) == 0) {
		sprintf(newText, "%s不能对你造成伤害。\n     ", capMonstName);
	} else {
		i = strlen(buf);
		i = encodeMessageColor(buf, i, &badMessageColor);
		combatMath = (player.currentHP + monst->info.damage.upperBound * monsterDamageAdjustmentAmount(monst) - 1) / (monst->info.damage.upperBound * monsterDamageAdjustmentAmount(monst));
        if (combatMath < 1) {
            combatMath = 1;
        }
		sprintf(newText, "%s能以%i%%的几率击中你，大约能消耗你当前生命值的%i%%，最差情况能在%i次攻击后杀死你。\n     ",
				capMonstName,
				combatMath2,
				(int) (100 * (monst->info.damage.lowerBound + monst->info.damage.upperBound) * monsterDamageAdjustmentAmount(monst) / 2 / player.currentHP + FLOAT_FUDGE),
				combatMath);
				//(combatMath > 1 ? "s" : ""));
	}
	upperCase(newText);
	strcat(buf, newText);
	
	if (monst->creatureState == MONSTER_ALLY) {
		i = strlen(buf);
		i = encodeMessageColor(buf, i, &goodMessageColor);
		
		sprintf(newText, "%s现在是你的同伴。", capMonstName);
		if (monst->absorbXPXP > XPXP_NEEDED_FOR_ABSORB) {
			upperCase(newText);
			strcat(buf, newText);
			strcat(buf, "\n     ");
			i = strlen(buf);
			i = encodeMessageColor(buf, i, &advancementMessageColor);
			
			strcpy(newText, "$HESHE看起来学会了些新东西。");
			resolvePronounEscapes(newText, monst); // So that it gets capitalized appropriately.
		}
	} else if (monst->bookkeepingFlags & MONST_CAPTIVE) {
		i = strlen(buf);
		i = encodeMessageColor(buf, i, &goodMessageColor);
		
		sprintf(newText, "%s被笼子困住了。", capMonstName);
	} else {
		
		if (!rogue.weapon || (rogue.weapon->flags & ITEM_IDENTIFIED)) {
			playerKnownAverageDamage = (player.info.damage.upperBound + player.info.damage.lowerBound) / 2;
			playerKnownMaxDamage = player.info.damage.upperBound;
		} else {
			playerKnownAverageDamage = (rogue.weapon->damage.upperBound + rogue.weapon->damage.lowerBound) / 2;
			playerKnownMaxDamage = rogue.weapon->damage.upperBound;
		}
		
		if (playerKnownMaxDamage == 0) {
			i = strlen(buf);
			i = encodeMessageColor(buf, i, &white);
			
			sprintf(newText, "你无法直接造成伤害。");
		} else {
			i = strlen(buf);
			i = encodeMessageColor(buf, i, &goodMessageColor);
			
			combatMath = (monst->currentHP + playerKnownMaxDamage - 1) / playerKnownMaxDamage;
            if (combatMath < 1) {
                combatMath = 1;
            }
			if (rogue.weapon && !(rogue.weapon->flags & ITEM_IDENTIFIED)) {
				realArmorValue = rogue.weapon->enchant1;
				rogue.weapon->enchant1 = 0;
				combatMath2 = hitProbability(&player, monst);
				rogue.weapon->enchant1 = realArmorValue;
			} else {
				combatMath2 = hitProbability(&player, monst);
			}
			sprintf(newText, "你能以%i%%的概率击中%s，能消耗掉$HISHER当前生命值的%i%%，最好情况下你能在%i击内杀死$HIMHER。",
					combatMath2,
					monstName,
					100 * playerKnownAverageDamage / monst->currentHP,
					combatMath);
					// (combatMath > 1 ? "s" : ""));
		}
	}
	upperCase(newText);
	strcat(buf, newText);
	
	anyFlags = false;
	for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		itemName(theItem, theItemName, false, false, NULL);
		anyFlags = false;
		
		if (theItem->category == STAFF
			&& (theItem->flags & (ITEM_MAX_CHARGES_KNOWN | ITEM_IDENTIFIED))
			&& (staffTable[theItem->kind].identified)) {
			switch (theItem->kind) {
				case STAFF_FIRE:
					if (monst->status[STATUS_IMMUNE_TO_FIRE]) {
						sprintf(newText, "\n     你的%s(%c)没有办法对%s造成伤害。",
								theItemName,
								theItem->inventoryLetter,
								monstName);
						anyFlags = true;
						break;
					}
					// otherwise continue to the STAFF_LIGHTNING template
				case STAFF_LIGHTNING:
					sprintf(newText, "\n     你的%s(%c)能对%s有效，会消耗$HISHER当前生命值的%i%%到%i%%。",
							theItemName,
							theItem->inventoryLetter,
							monstName,
							100 * staffDamageLow(theItem->enchant1) / monst->currentHP,
							100 * staffDamageHigh(theItem->enchant1) / monst->currentHP);
					anyFlags = true;
					break;
				case STAFF_POISON:
					if (monst->info.flags & MONST_INANIMATE) {
						sprintf(newText, "\n     你的%s(%c)对%s无效。",
								theItemName,
								theItem->inventoryLetter,
								monstName);
					} else {
						sprintf(newText, "\n     你的%s(%c)能使%s中毒，消耗$HISHER当前生命值的%i%%。",
								theItemName,
								theItem->inventoryLetter,
								monstName,
								100 * staffPoison(theItem->enchant1) / monst->currentHP);
					}
					anyFlags = true;
					break;
				default:
					break;
			}
		} else if (theItem->category == WAND
				   && theItem->kind == WAND_DOMINATION
				   && wandTable[WAND_DOMINATION].identified
				   && !printedDominationText
				   && monst->creatureState != MONSTER_ALLY) {
			printedDominationText = true;
			if (monst->info.flags & MONST_INANIMATE) {
				sprintf(newText, "\n     支配魔棒对像%s这种没有生命的物体是无效的。",
						monstName);
			} else {
				sprintf(newText, "\n     支配魔棒能以%i%%的成功率控制%s，若血量更少则成功率更高。",
						wandDominate(monst),
						monstName);
			}
			anyFlags = true;
		}
		if (anyFlags) {
			i = strlen(buf);
			i = encodeMessageColor(buf, i, &itemMessageColor);
			strcat(buf, newText);
		}
	}
	
	if (monst->carriedItem) {
		i = strlen(buf);
		i = encodeMessageColor(buf, i, &itemMessageColor);
		itemName(monst->carriedItem, theItemName, true, true, NULL);
		sprintf(newText, "%s身上带着%s。", capMonstName, theItemName);
		upperCase(newText);
		strcat(buf, "\n     ");
		strcat(buf, newText);
	}
	
	strcat(buf, "\n     ");
	
	i = strlen(buf);
	i = encodeMessageColor(buf, i, &white);
	
	anyFlags = false;
	sprintf(newText, "%s", capMonstName);
	
	if (monst->attackSpeed < 100) {
		strcat(newText, "攻击速度很快");
		anyFlags = true;
	} else if (monst->attackSpeed > 100) {
		strcat(newText, "攻击速度很慢");
		anyFlags = true;
	}
	
	if (monst->movementSpeed < 100) {
		if (anyFlags) {
			strcat(newText, "&");
			commaCount++;
		}
		strcat(newText, "移动速度很快");
		anyFlags = true;
	} else if (monst->movementSpeed > 100) {
		if (anyFlags) {
			strcat(newText, "&");
			commaCount++;
		}
		strcat(newText, "移动速度很慢");
		anyFlags = true;
	}
	
	if (monst->info.turnsBetweenRegen == 0) {
		if (anyFlags) {
			strcat(newText, "&");
			commaCount++;
		}
		strcat(newText, "不会恢复生命值");
		anyFlags = true;
	} else if (monst->info.turnsBetweenRegen < 5000) {
		if (anyFlags) {
			strcat(newText, "&");
			commaCount++;
		}
		strcat(newText, "能迅速恢复生命值");
		anyFlags = true;
	}
	
	// ability flags
	for (i=0; i<32; i++) {
		// !!!!! DEBUG
		if ((monst->info.abilityFlags & (Fl(i)))
			&& monsterAbilityFlagDescriptions[i][0]) {
			if (anyFlags) {
				strcat(newText, "&");
				commaCount++;
			}
			strcat(newText, monsterAbilityFlagDescriptions[i]);
			anyFlags = true;
		}
	}
	
	// behavior flags
	for (i=0; i<32; i++) {
		if ((monst->info.flags & (Fl(i)))
			&& monsterBehaviorFlagDescriptions[i][0]) {
			if (anyFlags) {
				strcat(newText, "&");
				commaCount++;
			}
			strcat(newText, monsterBehaviorFlagDescriptions[i]);
			anyFlags = true;
		}
	}
	
	// bookkeeping flags
	for (i=0; i<32; i++) {
		if ((monst->bookkeepingFlags & (Fl(i)))
			&& monsterBookkeepingFlagDescriptions[i][0]) {
			if (anyFlags) {
				strcat(newText, "&");
				commaCount++;
			}
			strcat(newText, monsterBookkeepingFlagDescriptions[i]);
			anyFlags = true;
		}
	}
	
	if (anyFlags) {
		strcat(newText, "。 ");
		//strcat(buf, "\n\n");
		j = strlen(buf);
		for (i=0; newText[i] != '\0'; i++) {
			if (newText[i] == '&') {
				if (!--commaCount) {
					buf[j] = '\0';
					strcat(buf, "并且");
					j += strlen("并且");
				} else {
					char uComma[] = "，";
					int iix;
					for (iix = 0; iix < u8_seqlen(uComma); ++iix) {
						buf[j++] = uComma[iix];
					}
				}
			} else {
				buf[j++] = newText[i];
			}
		}
		buf[j] = '\0';
	}
	resolvePronounEscapes(buf, monst);
}
