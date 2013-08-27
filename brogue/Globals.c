/*
 *  Globals.c
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
#include "Display.h"

BROGUE_DISPLAY *brogue_display;
BROGUE_DRAW_COLOR windowColor = { 0.05, 0.05, 0.20, 0.8 };
BROGUE_DRAW_COLOR inputFieldColor = { 0.0, 0.0, 0.10, 0.8 };
tcell tmap[DCOLS][DROWS];						// grids with info about the map
lightcorner lightmap[DCOLS + 1][DROWS + 1];
pcell pmap[DCOLS][DROWS];
short **scentMap;
short terrainRandomValues[DCOLS][DROWS][8];
short **safetyMap;								// used to help monsters flee
short **allySafetyMap;							// used to help allies flee
short **chokeMap;								// used to assess the importance of the map's various chokepoints
short **playerPathingMap;						// used to calculate routes for mouse movement
const short nbDirs[8][2] = {{0,-1}, {0,1}, {-1,0}, {1,0}, {-1,-1}, {-1,1}, {1,-1}, {1,1}};
const short cDirs[8][2] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};
short numberOfWaypoints;
levelData levels[DEEPEST_LEVEL+1];
creature player;
playerCharacter rogue;
creature *monsters;
creature *dormantMonsters;
creature *graveyard;
item *floorItems;
item *packItems;
item *monsterItemsHopper;

// expand all message related store since using utf8
char displayedMessage[MESSAGE_LINES][COLS*2 * 6];
boolean messageConfirmed[MESSAGE_LINES];
char combatText[COLS * 2 * 6];
short messageArchivePosition;
char messageArchive[MESSAGE_ARCHIVE_LINES][COLS*2 * 6];

char currentFilePath[BROGUE_FILENAME_MAX];

char displayDetail[DCOLS][DROWS];		// used to make certain per-cell data accessible to external code (e.g. terminal adaptations)

#ifdef AUDIT_RNG
FILE *RNGLogFile;
#endif

unsigned char inputRecordBuffer[INPUT_RECORD_BUFFER + 10];
unsigned short locationInRecordingBuffer;
unsigned long randomNumbersGenerated;
unsigned long positionInPlaybackFile;
unsigned long lengthOfPlaybackFile;
unsigned long recordingLocation;
unsigned long maxLevelChanges;
char annotationPathname[BROGUE_FILENAME_MAX];	// pathname of annotation file
unsigned long previousGameSeed;

#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma mark Colors
//									Red		Green	Blue	RedRand	GreenRand	BlueRand	Rand	Dances?
// basic colors
const color white =					{100,	100,	100,	0,		0,			0,			0,		false};
const color gray =					{50,	50,		50,		0,		0,			0,			0,		false};
const color darkGray =				{30,	30,		30,		0,		0,			0,			0,		false};
const color veryDarkGray =			{15,	15,		15,		0,		0,			0,			0,		false};
const color black =					{0,		0,		0,		0,		0,			0,			0,		false};
const color yellow =				{100,	100,	0,		0,		0,			0,			0,		false};
const color darkYellow =			{50,	50,		0,		0,		0,			0,			0,		false};
const color teal =					{30,	100,	100,	0,		0,			0,			0,		false};
const color purple =				{100,	0,		100,	0,		0,			0,			0,		false};
const color darkPurple =			{50,	0,		50,		0,		0,			0,			0,		false};
const color brown =					{60,	40,		0,		0,		0,			0,			0,		false};
const color green =					{0,		100,	0,		0,		0,			0,			0,		false};
const color darkGreen =				{0,		50,		0,		0,		0,			0,			0,		false};
const color orange =				{100,	50,		0,		0,		0,			0,			0,		false};
const color darkOrange =			{50,	25,		0,		0,		0,			0,			0,		false};
const color blue =					{0,		0,		100,	0,		0,			0,			0,		false};
const color darkBlue =				{0,		0,		50,		0,		0,			0,			0,		false};
const color darkTurquoise =         {0,		40,		65,		0,		0,			0,			0,		false};
const color lightBlue =				{40,	40,		100,	0,		0,			0,			0,		false};
const color pink =					{100,	60,		66,		0,		0,			0,			0,		false};
const color red  =					{100,	0,		0,		0,		0,			0,			0,		false};
const color darkRed =				{50,	0,		0,		0,		0,			0,			0,		false};
const color tanColor =				{80,	67,		15,		0,		0,			0,			0,		false};

// bolt colors
const color rainbow =				{-70,	-70,	-70,	170,	170,		170,		0,		true};
// const color rainbow =			{0,		0,		50,		100,	100,		0,			0,		true};
const color descentBoltColor =      {-40,   -40,    -40,    0,      0,          80,         80,     true};
const color discordColor =			{25,	0,		25,		66,		0,			0,			0,		true};
const color poisonColor =			{0,		0,		0,		10,		50,			10,			0,		true};
const color beckonColor =			{10,	10,		10,		5,		5,			5,			50,		true};
const color invulnerabilityColor =	{25,	0,		25,		0,		0,			66,			0,		true};
const color dominationColor =		{0,		0,		100,	80,		25,			0,			0,		true};
const color fireBoltColor =			{500,	150,	0,		45,		30,			0,			0,		true};
const color flamedancerCoronaColor ={500,	150,	100,	45,		30,			0,			0,		true};
//const color shieldingColor =		{100,	50,		0,		0,		50,			100,		0,		true};
const color shieldingColor =		{150,	75,		0,		0,		50,			175,		0,		true};

// tile colors
const color undiscoveredColor =		{0,		0,		0,		0,		0,			0,			0,		false};

const color wallForeColor =			{7,		7,		7,		3,		3,			3,			0,		false};

color wallBackColor;
const color wallBackColorStart =	{45,	40,		40,		15,		0,			5,			20,		false};
const color wallBackColorEnd =		{40,	30,		35,		0,		20,			30,			20,		false};

const color graniteBackColor =		{10,	10,		10,		0,		0,			0,			0,		false};

const color floorForeColor =		{30,	30,		30,		0,		0,			0,			35,		false};

color floorBackColor;
const color floorBackColorStart =	{2,		2,		10,		2,		2,			0,			0,		false};
const color floorBackColorEnd =		{5,		5,		5,		2,		2,			0,			0,		false};

const color stairsBackColor =		{15,	15,		5,		0,		0,			0,			0,		false};
const color firstStairsBackColor =	{10,	10,		25,		0,		0,			0,			0,		false};

const color refuseBackColor =		{6,		5,		3,		2,		2,			0,			0,		false};
const color rubbleBackColor =		{7,		7,		8,		2,		2,			1,			0,		false};
const color bloodflowerForeColor =  {30,    5,      40,     5,      1,          3,          0,      false};
const color bloodflowerPodForeColor = {50,  5,      25,     5,      1,          3,          0,      false};
const color bloodflowerBackColor =  {15,    3,      10,     3,      1,          3,          0,      false};

const color obsidianBackColor =		{6,		0,		8,		2,		0,			3,			0,		false};
const color carpetForeColor =		{23,	30,		38,		0,		0,			0,			0,		false};
const color carpetBackColor =		{15,	8,		5,		0,		0,			0,			0,		false};
const color doorForeColor =			{70,	35,		15,		0,		0,			0,			0,		false};
const color doorBackColor =			{30,	10,		5,		0,		0,			0,			0,		false};
//const color ironDoorForeColor =		{40,	40,		40,		0,		0,			0,			0,		false};
const color ironDoorForeColor =		{500,	500,	500,	0,		0,			0,			0,		false};
const color ironDoorBackColor =		{15,	15,		30,		0,		0,			0,			0,		false};
const color bridgeFrontColor =		{33,	12,		12,		12,		7,			2,			0,		false};
const color bridgeBackColor =		{12,	3,		2,		3,		2,			1,			0,		false};
const color statueBackColor =		{20,	20,		20,		0,		0,			0,			0,		false};
const color glyphColor =            {20,    5,      5,      50,     0,          0,          0,      true};
const color glyphLightColor =       {150,   0,      0,      150,    0,          0,          0,      true};

//const color deepWaterForeColor =	{5,		5,		40,		0,		0,			10,			10,		true};
//color deepWaterBackColor;
//const color deepWaterBackColorStart = {5,	5,		55,		5,		5,			10,			10,		true};
//const color deepWaterBackColorEnd =	{5,		5,		45,		2,		2,			5,			5,		true};
//const color shallowWaterForeColor =	{40,	40,		90,		0,		0,			10,			10,		true};
//color shallowWaterBackColor;
//const color shallowWaterBackColorStart ={30,30,		80,		0,		0,			10,			10,		true};
//const color shallowWaterBackColorEnd ={20,	20,		60,		0,		0,			5,			5,		true};

const color deepWaterForeColor =	{5,		8,		20,		0,		4,			15,			10,		true};
color deepWaterBackColor;
const color deepWaterBackColorStart = {5,	10,		31,		5,		5,			5,			6,		true};
const color deepWaterBackColorEnd =	{5,		8,		20,		2,		3,			5,			5,		true};
const color shallowWaterForeColor =	{28,	28,		60,		0,		0,			10,			10,		true};
color shallowWaterBackColor;
const color shallowWaterBackColorStart ={20,20,		60,		0,		0,			10,			10,		true};
const color shallowWaterBackColorEnd ={12,	15,		40,		0,		0,			5,			5,		true};

const color mudForeColor =			{18,	14,		5,		5,		5,			0,			0,		false};
const color mudBackColor =			{23,	17,		7,		5,		5,			0,			0,		false};
const color chasmForeColor =		{7,		7,		15,		4,		4,			8,			0,		false};
color chasmEdgeBackColor;
const color chasmEdgeBackColorStart ={5,	5,		25,		2,		2,			2,			0,		false};
const color chasmEdgeBackColorEnd =	{8,		8,		20,		2,		2,			2,			0,		false};
const color fireForeColor =			{70,	20,		0,		15,		10,			0,			0,		true};
const color lavaForeColor =			{20,	20,		20,		100,	10,			0,			0,		true};
const color brimstoneForeColor =	{100,	50,		10,		0,		50,			40,			0,		true};
const color brimstoneBackColor =	{18,	12,		9,		0,		0,			5,			0,		false};

const color lavaBackColor =			{50,	15,		0,		15,		10,			0,			0,		true};
const color acidBackColor =			{15,	80,		25,		5,		15,			10,			0,		true};

const color lightningColor =		{100,	150,	500,	50,		50,			0,			50,		true};
const color fungusLightColor =		{2,		11,		11,		4,		3,			3,			0,		true};
const color lavaLightColor =		{47,	13,		0,		10,		7,			0,			0,		true};
const color deepWaterLightColor =	{10,	30,		100,	0,		30,			100,		0,		true};

const color grassColor =			{15,	40,		15,		15,		50,			15,			10,		false};
const color deadGrassColor =		{20,	13,		0,		20,		10,			5,			10,		false};
const color fungusColor =			{15,	50,		50,		0,		25,			0,			30,		true};
const color grayFungusColor =		{30,	30,		30,		5,		5,			5,			10,		false};
const color foliageColor =			{25,	100,	25,		15,		0,			15,			0,		false};
const color deadFoliageColor =		{20,	13,		0,		30,		15,			0,			20,		false};
const color lichenColor =			{50,	5,		25,		10,		0,			5,			0,		true};
const color hayColor =				{70,	55,		5,		0,		20,			20,			0,		false};
const color ashForeColor =			{20,	20,		20,		0,		0,			0,			20,		false};
const color bonesForeColor =		{80,	80,		30,		5,		5,			35,			5,		false};
const color ectoplasmColor =		{45,	20,		55,		25,		0,			25,			5,		false};
const color forceFieldColor =		{0,		25,		25,		0,		25,			25,			0,		true};
const color forceFieldForeColor =		{0,		35,		35,		0,		25,			25,			0,		true};
const color wallCrystalColor =		{40,	40,		60,		20,		20,			40,			0,		true};
const color wallCrystalForeColor =	{50,	50,		80,		20,		20,			40,			0,		true};
const color altarForeColor =		{5,		7,		9,		0,		0,			0,			0,		false};
const color altarBackColor =		{35,	18,		18,		0,		0,			0,			0,		false};
const color pedestalBackColor =		{10,	5,		20,		0,		0,			0,			0,		false};

// monster colors
const color goblinColor =			{40,	30,		20,		0,		0,			0,			0,		false};
const color jackalColor =			{60,	42,		27,		0,		0,			0,			0,		false};
const color ogreColor =				{60,	25,		25,		0,		0,			0,			0,		false};
const color eelColor =				{30,	12,		12,		0,		0,			0,			0,		false};
const color goblinConjurerColor =	{67,	10,		100,	0,		0,			0,			0,		false};
const color spectralBladeColor =	{15,	15,		60,		0,		0,			70,			50,		true};
const color spectralImageColor =	{13,	0,		0,		25,		0,			0,			0,		true};
const color toadColor =				{40,	65,		30,		0,		0,			0,			0,		false};
const color trollColor =			{40,	60,		15,		0,		0,			0,			0,		false};
const color centipedeColor =		{75,	25,		85,		0,		0,			0,			0,		false};
const color dragonColor =			{20,	80,		15,		0,		0,			0,			0,		false};
const color krakenColor =			{100,	55,		55,		0,		0,			0,			0,		false};
const color salamanderColor =		{40,	10,		0,		8,		5,			0,			0,		true};
const color pixieColor =			{60,	60,		60,		40,		40,			40,			0,		true};
const color darPriestessColor =		{0,		50,		50,		0,		0,			0,			0,		false};
const color darMageColor =			{50,	50,		0,		0,		0,			0,			0,		false};
const color wraithColor =			{66,	66,		25,		0,		0,			0,			0,		false};
const color pinkJellyColor =		{100,	40,		40,		5,		5,			5,			20,		true};
const color wormColor =				{80,	60,		40,		0,		0,			0,			0,		false};
const color sentinelColor =			{3,		3,		30,		0,		0,			10,			0,		true};
const color goblinMysticColor =		{10,	67,		100,	0,		0,			0,			0,		false};
const color ifritColor =			{50,	10,		100,	75,		0,			20,			0,		true};
const color phoenixColor =			{100,	0,		0,		0,		100,		0,			0,		true};

// light colors
color minersLightColor;
const color minersLightStartColor =	{180,	180,	180,	0,		0,			0,			0,		false};
const color minersLightEndColor =	{90,	90,		120,	0,		0,			0,			0,		false};
const color torchColor =			{150,	75,		30,		0,		30,			20,			0,		true};
const color torchLightColor =		{75,	38,		15,		0,		15,			7,			0,		true};
//const color hauntedTorchColor =     {75,	30,		150,	30,		20,			0,			0,		true};
const color hauntedTorchColor =     {75,	20,		40,     30,		10,			0,			0,		true};
//const color hauntedTorchLightColor ={19,     7,		37,		8,		4,			0,			0,		true};
const color hauntedTorchLightColor ={67,    10,		10,		20,		4,			0,			0,		true};
const color ifritLightColor =		{0,		10,		150,	100,	0,			100,		0,		true};
//const color unicornLightColor =		{-50,	-50,	-50,	200,	200,		200,		0,		true};
const color unicornLightColor =		{-50,	-50,	-50,	250,	250,		250,		0,		true};
const color wispLightColor =		{75,	100,	250,	33,		10,			0,			0,		true};
const color summonedImageLightColor ={200,	0,		75,		0,		0,			0,			0,		true};
const color spectralBladeLightColor ={40,	0,		230,	0,		0,			0,			0,		true};
const color ectoplasmLightColor =	{23,	10,		28,		13,		0,			13,			3,		false};
const color explosionColor =		{10,	8,		2,		0,		2,			2,			0,		true};
const color dartFlashColor =		{500,	500,	500,	0,		2,			2,			0,		true};
const color lichLightColor =		{-50,	80,		30,		0,		0,			20,			0,		true};
const color forceFieldLightColor =	{10,	10,		10,		0,		50,			50,			0,		true};
const color crystalWallLightColor =	{10,	10,		10,		0,		0,			50,			0,		true};
const color sunLightColor =			{100,	100,	75,		0,		0,			0,			0,		false};
const color fungusForestLightColor ={30,	40,		60,		0,		0,			0,			40,		true};
const color fungusTrampledLightColor ={10,	10,		10,		0,		50,			50,			0,		true};
const color redFlashColor =			{100,	10,		10,		0,		0,			0,			0,		false};
const color darknessPatchColor =	{-10,	-10,	-10,	0,		0,			0,			0,		false};
const color darknessCloudColor =	{-20,	-20,	-20,	0,		0,			0,			0,		false};
const color magicMapFlashColor =	{60,	20,		60,		0,		0,			0,			0,		false};
const color sentinelLightColor =	{20,	20,		120,	10,		10,			60,			0,		true};
const color telepathyColor =		{30,	30,		130,	0,		0,			0,			0,		false};
const color confusionLightColor =	{10,	10,		10,		10,		10,			10,			0,		true};
const color portalActivateLightColor ={300,	400,	500,	0,		0,			0,			0,		true};
const color descentLightColor =     {20,    20,     70,     0,      0,          0,          0,      false};
const color algaeBlueLightColor =   {20,    15,     50,     0,      0,          0,          0,      false};
const color algaeGreenLightColor =  {15,    50,     20,     0,      0,          0,          0,      false};

// flare colors
const color scrollProtectionColor =	{375,	750,	0,		0,		0,			0,          0,		true};
const color scrollEnchantmentColor ={250,	225,	300,	0,		0,			450,        0,		true};
const color potionStrengthColor =   {1000,  0,      400,	600,	0,			0,          0,		true};
const color genericFlashColor =     {800,   800,    800,    0,      0,          0,          0,      false};
const color summoningFlashColor =   {0,     0,      0,      600,    0,          1200,       0,      true};
const color fireFlashColor =		{750,	225,	0,		100,	50,			0,			0,		true};
const color explosionFlareColor =   {10000, 6000,   1000,   0,      0,          0,          0,      false};

// color multipliers
const color colorDim25 =			{25,	25,		25,		25,		25,			25,			25,		false};
const color colorMultiplier100 =	{100,	100,	100,	100,	100,		100,		100,	false};
const color memoryColor =			{25,	25,		50,		20,		20,			20,			0,		false};
const color memoryOverlay =			{25,	25,		50,		0,		0,			0,			0,		false};
const color magicMapColor =			{60,	20,		60,		60,		20,			60,			0,		false};
const color clairvoyanceColor =		{50,	90,		50,		50,		90,			50,			66,		false};
const color telepathyMultiplier =	{30,	30,		130,	30,		30,			130,		66,		false};
const color omniscienceColor =		{140,	100,	60,		140,	100,		60,			90,		false};
const color basicLightColor =		{180,	180,	180,	180,	180,		180,		180,	false};

// blood colors
const color humanBloodColor =		{60,	20,		10,		15,		0,			0,			15,		false};
const color insectBloodColor =		{10,	60,		20,		0,		15,			0,			15,		false};
const color vomitColor =			{60,	50,		5,		0,		15,			15,			0,		false};
const color urineColor =			{70,	70,		40,		0,		0,			0,			10,		false};
const color methaneColor =			{45,	60,		15,		0,		0,			0,			0,		false};

// gas colors
const color poisonGasColor =		{75,	25,		85,		0,		0,			0,			0,		false};
const color confusionGasColor =		{60,	60,		60,		40,		40,			40,			0,		true};

// interface colors
const color itemColor =				{100,	95,		-30,	0,		0,			0,			0,		false};
const color blueBar =				{15,	10,		50,		0,		0,			0,			0,		false};
const color redBar =				{45,	10,		15,		0,		0,			0,			0,		false};
const color hiliteColor =			{100,	100,	0,		0,		0,			0,			0,		false};
const color interfaceBoxColor =		{7,		6,		15,		0,		0,			0,			0,		false};
const color interfaceButtonColor =	{10,	20,		50,		0,		0,			0,			0,		false};
const color buttonHoverColor =		{10,	20,		 70,		0,		0,			0,			0,		false};
const color titleButtonColor =		{23,	15,		30,		0,		0,			0,			0,		false};

const color playerInvisibleColor =  {20,    20,     30,     0,      0,          80,         0,      true};
const color playerInLightColor =	{100,	90,     30,		0,		0,			0,			0,		false};
const color playerInShadowColor =	{60,	60,		100,	0,		0,			0,			0,		false};
const color playerInDarknessColor =	{30,	30,		65,		0,		0,			0,			0,		false};

const color inLightMultiplierColor ={150,   150,    75,     150,    150,        75,         100,    true};
const color inDarknessMultiplierColor={66,  66,     120,    66,     66,         120,        66,     true};

const color goodMessageColor =		{60,	50,		100,	0,		0,			0,			0,		false};
const color badMessageColor =		{100,	50,		60,		0,		0,			0,			0,		false};
const color advancementMessageColor ={50,	100,	60,		0,		0,			0,			0,		false};
const color itemMessageColor =		{100,	100,	50,		0,		0,			0,			0,		false};
const color flavorTextColor =		{50,	40,		90,		0,		0,			0,			0,		false};
const color backgroundMessageColor ={60,	20,		70,		0,		0,			0,			0,		false};

const color superVictoryColor =     {150,	100,	300,	0,		0,			0,			0,		false};

//const color flameSourceColor = {0, 0, 0, 65, 40, 100, 0, true}; // 1
//const color flameSourceColor = {0, 0, 0, 80, 50, 100, 0, true}; // 2
//const color flameSourceColor = {25, 13, 25, 50, 25, 50, 0, true}; // 3
//const color flameSourceColor = {20, 20, 20, 60, 20, 40, 0, true}; // 4
//const color flameSourceColor = {30, 18, 18, 70, 36, 36, 0, true}; // 7**
const color flameSourceColor = {20, 7, 7, 60, 40, 40, 0, true}; // 8

//const color flameTitleColor = {0, 0, 0, 17, 10, 6, 0, true}; // pale orange
//const color flameTitleColor = {0, 0, 0, 7, 7, 10, 0, true}; // *pale blue*
const color flameTitleColor = {0, 0, 0, 9, 9, 15, 0, true}; // *pale blue**
//const color flameTitleColor = {0, 0, 0, 11, 11, 18, 0, true}; // *pale blue*
//const color flameTitleColor = {0, 0, 0, 15, 15, 9, 0, true}; // pale yellow
//const color flameTitleColor = {0, 0, 0, 15, 9, 15, 0, true}; // pale purple

#pragma mark Dynamic color references

const color *dynamicColors[NUMBER_DYNAMIC_COLORS][3] = {
	// used color			shallow color				deep color
	{&minersLightColor,		&minersLightStartColor,		&minersLightEndColor},
	{&wallBackColor,		&wallBackColorStart,		&wallBackColorEnd},
	{&deepWaterBackColor,	&deepWaterBackColorStart,	&deepWaterBackColorEnd},
	{&shallowWaterBackColor,&shallowWaterBackColorStart,&shallowWaterBackColorEnd},
	{&floorBackColor,		&floorBackColorStart,		&floorBackColorEnd},
	{&chasmEdgeBackColor,	&chasmEdgeBackColorStart,	&chasmEdgeBackColorEnd},
};

#pragma mark Autogenerator definitions

const autoGenerator autoGeneratorCatalog[NUMBER_AUTOGENERATORS] = {
//	 terrain					layer	DF							Machine						reqDungeon  reqLiquid   >Depth	<Depth          freq	minIncp	minSlope	maxNumber
	{0,							0,		DF_GRANITE_COLUMN,			0,							FLOOR,		NOTHING,    1,		DEEPEST_LEVEL,	60,		100,	0,			4},
	{0,							0,		DF_CRYSTAL_WALL,			0,							WALL,       NOTHING,    14,		DEEPEST_LEVEL,	15,		-325,	25,			5},
	{0,							0,		DF_LUMINESCENT_FUNGUS,		0,							FLOOR,		NOTHING,    7,		DEEPEST_LEVEL,	15,		-300,	70,			14},
	{0,							0,		DF_GRASS,					0,							FLOOR,		NOTHING,    0,		10,             0,		1000,	-80,        10},
	{0,							0,		DF_DEAD_GRASS,				0,							FLOOR,		NOTHING,    4,		9,              0,		-200,	80,         10},
	{0,							0,		DF_DEAD_GRASS,				0,							FLOOR,		NOTHING,    9,		14,             0,		1200,	-80,        10},
	{0,							0,		DF_BONES,					0,							FLOOR,		NOTHING,    12,		DEEPEST_LEVEL-1,30,		0,		0,			4},
	{0,							0,		DF_RUBBLE,					0,							FLOOR,		NOTHING,    0,		DEEPEST_LEVEL-1,30,		0,		0,			4},
	{0,							0,		DF_FOLIAGE,					0,							FLOOR,		NOTHING,    0,		8,              15,		1000,	-333,       10},
	{0,							0,		DF_FUNGUS_FOREST,			0,							FLOOR,		NOTHING,    13,		DEEPEST_LEVEL,	30,		-600,	50,			12},
	{0,							0,		DF_BUILD_ALGAE_WELL,		0,							FLOOR,      DEEP_WATER, 10,		DEEPEST_LEVEL,	50,		0,      0,			2},
	{STATUE_INERT,				DUNGEON,0,							0,							WALL,       NOTHING,    6,		DEEPEST_LEVEL-1,5,		-100,	35,			3},
	{STATUE_INERT,				DUNGEON,0,							0,							FLOOR,		NOTHING,    10,		DEEPEST_LEVEL-1,50,		0,		0,			3},
	{TORCH_WALL,				DUNGEON,0,							0,							WALL,       NOTHING,    6,		DEEPEST_LEVEL-1,5,		-200,	70,			12},
	{GAS_TRAP_POISON_HIDDEN,	DUNGEON,0,							0,							FLOOR,		NOTHING,    5,		DEEPEST_LEVEL-1,30,		100,	0,			3},
	{0,                         0,      0,							MT_PARALYSIS_TRAP_AREA,		FLOOR,		NOTHING,    7,		DEEPEST_LEVEL-1,30,		100,	0,			3},
	{TRAP_DOOR_HIDDEN,			DUNGEON,0,							0,							FLOOR,		NOTHING,    9,		DEEPEST_LEVEL-1,30,		100,	0,			2},
	{GAS_TRAP_CONFUSION_HIDDEN,	DUNGEON,0,							0,							FLOOR,		NOTHING,    11,		DEEPEST_LEVEL-1,30,		100,	0,			3},
	{FLAMETHROWER_HIDDEN,		DUNGEON,0,							0,							FLOOR,		NOTHING,    13,		DEEPEST_LEVEL-1,30,		100,	0,			3},
	{FLOOD_TRAP_HIDDEN,			DUNGEON,0,							0,							FLOOR,		NOTHING,    15,		DEEPEST_LEVEL-1,30,		100,	0,			3},
	{0,							0,		0,							MT_SWAMP_AREA,				FLOOR,		NOTHING,    1,		DEEPEST_LEVEL-1,30,		0,		0,			2},
	{0,							0,		DF_SUNLIGHT,				0,							FLOOR,		NOTHING,    0,		5,              15,		500,	-150,       10},
	{0,							0,		DF_DARKNESS,				0,							FLOOR,		NOTHING,    1,		15,             15,		500,	-50,        10},
	{STEAM_VENT,				DUNGEON,0,							0,							FLOOR,		NOTHING,    16,		DEEPEST_LEVEL-1,30,		100,	0,			3},
	{CRYSTAL_WALL,              DUNGEON,0,                          0,                          WALL,       NOTHING,    DEEPEST_LEVEL,DEEPEST_LEVEL,100,0,      0,          600},

	{0,							0,		DF_LUMINESCENT_FUNGUS,		0,							FLOOR,		NOTHING,    DEEPEST_LEVEL,DEEPEST_LEVEL,100,0,      0,			200},
	{0,                         0,      0,                          MT_BLOODFLOWER_AREA,        FLOOR,      NOTHING,    1,      30,             25,     140,    -10,        3},
	{0,							0,		0,							MT_IDYLL_AREA,				FLOOR,		NOTHING,    1,		5,              15,		0,		0,          1},
	{0,							0,		0,							MT_REMNANT_AREA,			FLOOR,		NOTHING,    10,		DEEPEST_LEVEL,	15,		0,		0,			2},
	{0,							0,		0,							MT_DISMAL_AREA,				FLOOR,		NOTHING,    7,		DEEPEST_LEVEL,	12,		0,		0,			5},
	{0,							0,		0,							MT_BRIDGE_TURRET_AREA,		FLOOR,		NOTHING,    5,		DEEPEST_LEVEL-1,6,		0,		0,			2},
	{0,							0,		0,							MT_LAKE_PATH_TURRET_AREA,	FLOOR,		NOTHING,    5,		DEEPEST_LEVEL-1,6,		0,		0,			2},
	{0,							0,		0,							MT_TRICK_STATUE_AREA,		FLOOR,		NOTHING,    6,		DEEPEST_LEVEL-1,15,		0,		0,			3},
	{0,							0,		0,							MT_SENTINEL_AREA,			FLOOR,		NOTHING,    12,		DEEPEST_LEVEL-1,10,		0,		0,			2},
	{0,							0,		0,							MT_WORM_AREA,				FLOOR,		NOTHING,    12,		DEEPEST_LEVEL-1,12,		0,		0,			3},
};

#pragma mark Terrain definitions

const floorTileType tileCatalog[NUMBER_TILETYPES] = {
	
	// promoteChance is in hundredths of a percent per turn
	
	//	char		fore color				back color		priority	ignit	fireType	discovType	promoteType		promoteChance	glowLight		flags																								description			flavorText
	
	// dungeon layer (this layer must have all of fore color, back color and char)
	{	' ',		&black,					&black,					100,0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0, 0,																								"什么都没有",		""},
	{0xe1,		&wallBackColor,			&graniteBackColor,		0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE),														"花岗岩墙壁",	"墙壁表面的覆满了崎岖不平的岩石。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0, 0,                                                                                               "地面",			""},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0, 0,                                                                                               "地面",			""},
	{0xe2,	&carpetForeColor,		&carpetBackColor,		85,	0,	DF_EMBERS,		0,			0,				0,				NO_LIGHT,		(T_IS_FLAMMABLE), (TM_VANISHES_UPON_PROMOTION),                                                     "地毯",			"华丽的地毯铺满了整个房间，象征着过去的繁华。"},
	{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE),														"岩石墙壁",			"坚硬的墙壁看上去非常稳固。"},
	{DOOR_CHAR,		&doorForeColor,			&doorBackColor,			25,	50,	DF_EMBERS,		0,			DF_OPEN_DOOR,	0,				NO_LIGHT,		(T_OBSTRUCTS_VISION | T_OBSTRUCTS_GAS | T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_PROMOTES_ON_STEP | TM_VISUALLY_DISTINCT), "一扇木门",	"你正站在门口。"},
	{OPEN_DOOR_CHAR,&doorForeColor,			&doorBackColor,			25,	50,	DF_EMBERS,		0,			DF_CLOSED_DOOR,	10000,			NO_LIGHT,		(T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_VISUALLY_DISTINCT),           "已被打开的门",			"你正站在门口。"},
	{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	50,	DF_EMBERS,		DF_SHOW_DOOR,0,				0,				NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING | T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_SECRET),	"岩石墙壁",		"坚硬的墙壁看上去非常稳固。"},
	{0xe3,		&ironDoorForeColor,		&ironDoorBackColor,		15,	50,	DF_EMBERS,		0,			DF_OPEN_IRON_DOOR_INERT,0,		NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_PROMOTES_WITH_KEY | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT | TM_BRIGHT_MEMORY),	"被锁住了的铁门",	"你在兜里找了找，但发现没有匹配的钥匙。"},
	{0xe4,&white,                 &ironDoorBackColor,		90,	50,	DF_EMBERS,		0,			0,				0,				NO_LIGHT,		(T_OBSTRUCTS_SURFACE_EFFECTS), (TM_STAND_IN_TILE | TM_VISUALLY_DISTINCT),                           "已被打开的铁门",	"你正站在门口。"},
	{DESCEND_CHAR,	&itemColor,				&stairsBackColor,		30,	0,	DF_PLAIN_FIRE,	0,			DF_REPEL_CREATURES, 0,			NO_LIGHT,		(T_OBSTRUCTS_ITEMS | T_OBSTRUCTS_SURFACE_EFFECTS), (TM_PROMOTES_ON_STEP | TM_STAND_IN_TILE | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT | TM_BRIGHT_MEMORY), "通往更深处的楼梯",	"阶梯盘旋而下，指向地牢更深处。"},
	{ASCEND_CHAR,	&itemColor,				&stairsBackColor,		30,	0,	DF_PLAIN_FIRE,	0,			DF_REPEL_CREATURES, 0,			NO_LIGHT,		(T_OBSTRUCTS_ITEMS | T_OBSTRUCTS_SURFACE_EFFECTS), (TM_PROMOTES_ON_STEP | TM_STAND_IN_TILE | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT | TM_BRIGHT_MEMORY), "通往上层的楼梯",	"螺旋状的阶梯通往更高处"},
	{OMEGA_CHAR,	&lightBlue,				&firstStairsBackColor,	30,	0,	DF_PLAIN_FIRE,	0,			DF_REPEL_CREATURES, 0,			NO_LIGHT,		(T_OBSTRUCTS_ITEMS | T_OBSTRUCTS_SURFACE_EFFECTS), (TM_PROMOTES_ON_STEP | TM_STAND_IN_TILE | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT | TM_BRIGHT_MEMORY), "地牢出口",		"离开地牢的大门被莫名的一股魔法的力量紧紧封锁了起来。"},
	{OMEGA_CHAR,	&wallCrystalColor,		&firstStairsBackColor,	30,	0,	DF_PLAIN_FIRE,	0,			DF_REPEL_CREATURES, 0,			INCENDIARY_DART_LIGHT,		(T_OBSTRUCTS_ITEMS | T_OBSTRUCTS_SURFACE_EFFECTS), (TM_PROMOTES_ON_STEP | TM_STAND_IN_TILE | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT | TM_BRIGHT_MEMORY), "传送门",		"传送门里发出不断变动的光芒。"},
	{0xe5,		&torchColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				TORCH_LIGHT,	(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE),														"固定在墙上的火把",	"火把被安稳的固定在了墙上，你能听到石炭燃烧的声音。"},
	{0xe6,		&wallCrystalForeColor,		&wallCrystalColor,		0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				CRYSTAL_WALL_LIGHT,(T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_ITEMS | T_OBSTRUCTS_GAS | T_OBSTRUCTS_SURFACE_EFFECTS | T_OBSTRUCTS_DIAGONAL_MOVEMENT), (TM_STAND_IN_TILE | TM_REFLECTS_BOLTS),"水晶色法阵", "光滑的表面下发出璀璨的光芒。"},
	{0xe7,		&gray,					&floorBackColor,		10,	0,	DF_PLAIN_FIRE,	0,			DF_OPEN_PORTCULLIS,	0,			NO_LIGHT,		(T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_ITEMS), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT), "铁栅门",	"上面的钢筋看起参差不齐，但仍是被坚挺的固定住。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_PLAIN_FIRE,	0,			DF_ACTIVATE_PORTCULLIS,0,		NO_LIGHT,		(0), (TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED),                                                    "地面",			""},
	{0xe8,		&doorForeColor,			&floorBackColor,		10,	100,DF_WOODEN_BARRICADE_BURN,0,	DF_ADD_WOODEN_BARRICADE,10000,	NO_LIGHT,		(T_OBSTRUCTS_ITEMS | T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_VISUALLY_DISTINCT),"木质栅栏", "这些木质的栅栏看起来很坚固，但上面覆满了由于干燥而裂开的痕迹。感觉很容易就会烧起来。"},
	{0xe8,		&doorForeColor,			&floorBackColor,		10,	100,DF_WOODEN_BARRICADE_BURN,0,	0,				0,				NO_LIGHT,		(T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_ITEMS | T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_VISUALLY_DISTINCT),"木质栅栏","这些木质的栅栏看起来很坚固，但上面覆满了由于干燥而裂开的痕迹。感觉很容易就会烧起来。"},
	{0xe9,		&torchLightColor,		&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			DF_PILOT_LIGHT,	0,				TORCH_LIGHT,	(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED),			"固定在墙上的火把",	"火把被安稳的固定在了墙上，你能听到石炭燃烧的声音。"},
	{0xea,		&fireForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				TORCH_LIGHT,	(T_OBSTRUCTS_EVERYTHING | T_IS_FIRE), (TM_STAND_IN_TILE | TM_LIST_IN_SIDEBAR),						"掉落在地上的火把",		"火把掉落在墙边，溅出四散的火花。"},
	{0xe9,		&torchColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			DF_HAUNTED_TORCH_TRANSITION,0,	TORCH_LIGHT,	(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED),			"固定在墙上的火把",	"火把被安稳的固定在了墙上，你能听到石炭燃烧的声音。"},
	{0xe9,		&torchColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			DF_HAUNTED_TORCH,2000,			TORCH_LIGHT,	(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                          "固定在墙上的火把",	"火把被安稳的固定在了墙上，你能听到石炭燃烧的声音。"},
	{0xe9,		&hauntedTorchColor,		&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				HAUNTED_TORCH_LIGHT,(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE),                                                   "剧烈燃烧的火把",	"你能看到火焰顶端有明显的焰心。"},
	{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	DF_REVEAL_LEVER,0,			0,				NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_SECRET),			"岩石墙壁",			"坚硬的墙壁看上去非常稳固。"},
	{0xd0,	&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			DF_PULL_LEVER,  0,				NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_PROMOTES_ON_PLAYER_ENTRY | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),"一个杠杆", "这个杠杆看起来可以被拉动。"},
	{0xd1,&wallForeColor,		&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE),                                                       "被固定的杠杆",    "这个杠杆看起来没法再被拉动。"},
	{WALL_CHAR,		&wallForeColor,         &wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,          DF_CREATE_LEVER,0,				NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE | TM_IS_WIRED),											"岩石墙壁",			"坚硬的墙壁看上去非常稳固。"},
	{STATUE_CHAR,	&wallBackColor,			&statueBackColor,		0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_ITEMS | T_OBSTRUCTS_GAS | T_OBSTRUCTS_SURFACE_EFFECTS), (TM_STAND_IN_TILE),	"大理石的雕像",	"乳白色的雕像经历了岁月的历练，但光泽仍然闪耀。"},
	{STATUE_CHAR,	&wallBackColor,			&statueBackColor,		0,	0,	DF_PLAIN_FIRE,	0,			DF_CRACKING_STATUE,0,			NO_LIGHT,		(T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_ITEMS | T_OBSTRUCTS_GAS | T_OBSTRUCTS_SURFACE_EFFECTS), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED),"大理石的雕像",	"乳白色的雕像经历了岁月的历练，但光泽仍然闪耀。"},
	{0xeb,	&wallBackColor,			&statueBackColor,		0,	0,	DF_PLAIN_FIRE,	0,			DF_STATUE_SHATTER,3500,			NO_LIGHT,		(T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_ITEMS | T_OBSTRUCTS_GAS | T_OBSTRUCTS_SURFACE_EFFECTS), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_LIST_IN_SIDEBAR),"破损的雕像",	"你看到雕像沿着裂口慢慢倒下。"},
	{OMEGA_CHAR,	&wallBackColor,			&floorBackColor,		17,	0,	DF_PLAIN_FIRE,	0,			DF_PORTAL_ACTIVATE,0,			NO_LIGHT,		(T_OBSTRUCTS_ITEMS), (TM_STAND_IN_TILE | TM_IS_WIRED | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),  "岩石拱门",		"岩石上覆满了青苔，拱门里传出微弱而阴森的微光。"},
	{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			DF_TURRET_EMERGE,0,				NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED),			"岩石墙壁",			"坚硬的墙壁看上去非常稳固。"},
	{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			DF_WALL_SHATTER,0,				NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED),			"岩石墙壁",			"坚硬的墙壁看上去非常稳固。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_PLAIN_FIRE,	0,			DF_DARKENING_FLOOR,	0,			NO_LIGHT,		(0), (TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED),                                                    "地面",			""},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_PLAIN_FIRE,	0,			DF_DARK_FLOOR,	1500,			NO_LIGHT,		(0), (TM_VANISHES_UPON_PROMOTION),                                                                  "地面",			""},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_PLAIN_FIRE,	0,			0,				0,				DARKNESS_CLOUD_LIGHT, 0, 0,                                                                                         "地面",			""},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(0), (TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_PROMOTES_ON_PLAYER_ENTRY),                      "地面",			""},
	{0xec,	&altarForeColor,		&altarBackColor,		17, 0,	0,				0,			0,				0,				CANDLE_LIGHT,	(T_OBSTRUCTS_SURFACE_EFFECTS), (TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),							"祭坛",	"镀金的祭坛上摆满了蜡烛，烛光随着空气的流动在摆动。"},
	{0xed,		&altarForeColor,		&altarBackColor,		17, 0,	0,				0,			0,				0,				CANDLE_LIGHT,	(T_OBSTRUCTS_SURFACE_EFFECTS), (TM_PROMOTES_WITH_KEY | TM_IS_WIRED | TM_LIST_IN_SIDEBAR),           "祭坛",	"镀金的祭坛上摆满了蜡烛，烛光随着空气的流动在摆动。"},
	{0xef,	&altarForeColor,		&altarBackColor,		17, 0,	0,				0,			DF_ITEM_CAGE_CLOSE,	0,			CANDLE_LIGHT,	(T_OBSTRUCTS_SURFACE_EFFECTS), (TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_PROMOTES_WITHOUT_KEY | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),"祭坛",	"祭坛的正上方吊着一个笼子，笼子的底部已经被打开。"},
	{0xf0,		&altarBackColor,		&veryDarkGray,			17, 0,	0,				0,			DF_ITEM_CAGE_OPEN,	0,			CANDLE_LIGHT,	(T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_SURFACE_EFFECTS), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_PROMOTES_WITH_KEY | TM_IS_WIRED | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),"铁笼","你必须将之前从这里拿起的东西放回来，才能获取房间里其他物品。"},
	{0xf1,	&altarForeColor,		&altarBackColor,		17, 0,	0,				0,			DF_ALTAR_INERT,	0,				CANDLE_LIGHT,	(T_OBSTRUCTS_SURFACE_EFFECTS), (TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_PROMOTES_ON_ITEM_PICKUP | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),	"祭坛",	"石制的祭坛上摆满了蜡烛，烛光随着空气的流动在摆动。"},
	{0xf1,	&altarForeColor,		&altarBackColor,		17, 0,	0,				0,			DF_ALTAR_RETRACT,0,				CANDLE_LIGHT,	(T_OBSTRUCTS_SURFACE_EFFECTS), (TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_PROMOTES_ON_ITEM_PICKUP | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),	"祭坛",	"石制的祭坛上摆满了蜡烛，烛光随着空气的流动在摆动。"},
	{0xf0,		&altarBackColor,		&veryDarkGray,			17, 0,	0,				0,			DF_CAGE_DISAPPEARS,	0,			CANDLE_LIGHT,	(T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_SURFACE_EFFECTS), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),"铁笼","笼子一动也不动，也许附近有别的机关能打开它。"},
	{0xf2,	&altarForeColor,		&pedestalBackColor,		17, 0,	0,				0,			0,				0,				CANDLE_LIGHT,	(T_OBSTRUCTS_SURFACE_EFFECTS), 0,																	"石制基座",		"基座表面雕刻着精美的花纹。"},
	{0xf3,	&floorBackColor,		&veryDarkGray,			17, 0,	0,				0,			0,				0,				NO_LIGHT,		(0), (TM_STAND_IN_TILE),																			"已打开的铁笼",			"笼子内沾满了难闻的粘液，散发出恶心的味道。"},
	{0xf0,		&gray,					&darkGray,				17, 0,	0,				0,			DF_MONSTER_CAGE_OPENS,	0,		NO_LIGHT,		(T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_SURFACE_EFFECTS | T_OBSTRUCTS_GAS), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_PROMOTES_WITH_KEY | TM_LIST_IN_SIDEBAR),"被锁上的铁笼","笼子上的钢筋被坚实的固定在了一起。"},
	{0xf4,	&bridgeFrontColor,		&bridgeBackColor,		17,	20,	DF_COFFIN_BURNS,0,			DF_COFFIN_BURSTS,0,				NO_LIGHT,		(T_IS_FLAMMABLE), (TM_IS_WIRED | TM_VANISHES_UPON_PROMOTION | TM_LIST_IN_SIDEBAR),                  "木质棺材",		"厚木板制的棺材躺在一层深绿色的苔藓上。"},
	{0xf5,	&black,					&bridgeBackColor,		17,	20,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_LIST_IN_SIDEBAR),				"空的棺材",		"厚木板制的棺材躺在一层深绿色的苔藓上，但是顶上的盖子已被推开。"},
	
	// traps (part of dungeon layer):
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_POISON_GAS_CLOUD, DF_SHOW_POISON_GAS_TRAP, 0, 0,			NO_LIGHT,		(T_IS_DF_TRAP), (TM_IS_SECRET),                                                                     "地面",			""},
	{TRAP_CHAR,		&centipedeColor,		0,                      30,	0,	DF_POISON_GAS_CLOUD, 0,		0,				0,				NO_LIGHT,		(T_IS_DF_TRAP), (TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),                                        "毒气陷阱",	"你发现地板下有一个隐藏的机关，连接着一旁的毒气罐。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_POISON_GAS_CLOUD, DF_SHOW_TRAPDOOR,0,	0,				NO_LIGHT,		(T_AUTO_DESCENT), (TM_IS_SECRET),                                                                   "地面",			"你不小心踩到了一个隐藏的机关！"},
	{0xf6,	&chasmForeColor,		&black,					30,	0,	DF_POISON_GAS_CLOUD,0,      0,				0,				NO_LIGHT,		(T_AUTO_DESCENT), 0,                                                                                "一个洞",				"你不小心踏进了地上的一个洞！"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	0,              DF_SHOW_PARALYSIS_GAS_TRAP, 0, 0,           NO_LIGHT,		(T_IS_DF_TRAP), (TM_IS_SECRET | TM_IS_WIRED),                                                       "地面",			""},
	{TRAP_CHAR,		&pink,					0,              		30,	0,	0,              0,          0,				0,				NO_LIGHT,		(T_IS_DF_TRAP), (TM_IS_WIRED | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),                          "麻痹陷阱",	"你发现地板下有一个隐藏的机关。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_PLAIN_FIRE,	DF_DISCOVER_PARALYSIS_VENT, DF_PARALYSIS_VENT_SPEW,0,NO_LIGHT,	(0), (TM_VANISHES_UPON_PROMOTION | TM_IS_SECRET | TM_IS_WIRED),                                 "地面",			""},
	{VENT_CHAR,		&pink,                  0,              		30,	0,	DF_PLAIN_FIRE,	0,			DF_PARALYSIS_VENT_SPEW,0,		NO_LIGHT,		(0), (TM_IS_WIRED | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),                                     "已失效的排气孔",	"一个已经失效的排气孔，连接着一旁的毒气罐。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_CONFUSION_GAS_TRAP_CLOUD,DF_SHOW_CONFUSION_GAS_TRAP, 0,0,NO_LIGHT,		(T_IS_DF_TRAP), (TM_IS_SECRET),                                                                     "地面",			""},
	{TRAP_CHAR,		&confusionGasColor,		0,              		30,	0,	DF_CONFUSION_GAS_TRAP_CLOUD,0,	0,			0,				NO_LIGHT,		(T_IS_DF_TRAP), (TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),                                        "混乱陷阱",		"你发现地板下有一个隐藏的机关，连接着一旁的混乱毒气罐。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_FLAMETHROWER,	DF_SHOW_FLAMETHROWER_TRAP, 0,	0,		NO_LIGHT,		(T_IS_DF_TRAP), (TM_IS_SECRET),                                                                     "地面",			""},
	{TRAP_CHAR,		&fireForeColor,			0,              		30,	0,	DF_FLAMETHROWER,	0,		0,				0,				NO_LIGHT,		(T_IS_DF_TRAP), (TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),										"火焰陷阱",			"你发现地板下有一个隐藏的机关，连接着一旁的一个原始的火焰喷射器。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_FLOOD,		DF_SHOW_FLOOD_TRAP, 0,		0,				NO_LIGHT,		(T_IS_DF_TRAP), (TM_IS_SECRET),                                                                     "地面",			""},
	{TRAP_CHAR,		&shallowWaterForeColor,	0,              		58,	0,	DF_FLOOD,		0,			0,				0,				NO_LIGHT,		(T_IS_DF_TRAP), (TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),										"洪水陷阱",			"你发现地板下有一个隐藏的机关，连接着一旁的闸门。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_PLAIN_FIRE,	DF_SHOW_POISON_GAS_VENT, DF_POISON_GAS_VENT_OPEN, 0, NO_LIGHT, (0), (TM_VANISHES_UPON_PROMOTION | TM_IS_SECRET | TM_IS_WIRED),                                  "地面",			""},
	{VENT_CHAR,		&floorForeColor,		0,              		30,	0,	DF_PLAIN_FIRE,	0,			DF_POISON_GAS_VENT_OPEN,0,		NO_LIGHT,		(0), (TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),		"已失效的排气孔",	"一个已经失效的排气孔隐藏在地面的缝隙中。"},
	{VENT_CHAR,		&floorForeColor,		0,              		30,	0,	DF_PLAIN_FIRE,	0,			DF_VENT_SPEW_POISON_GAS,10000,	NO_LIGHT,		0, (TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),														"排气孔",			"难闻的气体从地面的排气孔里不断喷出。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_PLAIN_FIRE,	DF_SHOW_METHANE_VENT, DF_METHANE_VENT_OPEN,0,NO_LIGHT,		(0), (TM_VANISHES_UPON_PROMOTION | TM_IS_SECRET | TM_IS_WIRED),                                     "地面",			""},
	{VENT_CHAR,		&floorForeColor,		0,              		30,	0,	DF_PLAIN_FIRE,	0,			DF_METHANE_VENT_OPEN,0,			NO_LIGHT,		(0), (TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),		"已失效的排气孔",	"一个已经失效的排气孔隐藏在地面的缝隙中。"},
	{VENT_CHAR,		&floorForeColor,		0,              		30,	15,	DF_EMBERS,		0,			DF_VENT_SPEW_METHANE,5000,		NO_LIGHT,		(T_IS_FLAMMABLE), (TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),										"排气孔",			"难闻的气体从地面的排气孔里不断喷出。"},
	{VENT_CHAR,		&gray,					0,              		15,	15,	DF_EMBERS,		0,			DF_STEAM_PUFF,	250,			NO_LIGHT,		T_OBSTRUCTS_ITEMS, (TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),										"冒出蒸汽的排气孔",			"大量蒸汽从地面的缝隙中不断涌出。"},
	{TRAP_CHAR,		&white,					&chasmEdgeBackColor,	15,	0,	0,				0,			DF_MACHINE_PRESSURE_PLATE_USED,0,NO_LIGHT,      (T_IS_DF_TRAP), (TM_VANISHES_UPON_PROMOTION | TM_PROMOTES_ON_STEP | TM_IS_WIRED | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),"地面上的机关",		"很明显的一个机关。丢点东西上去似乎能激活它。"},
	{TRAP_CHAR,		&darkGray,				&chasmEdgeBackColor,	15,	0,	0,				0,			0,				0,				NO_LIGHT,		0, (TM_LIST_IN_SIDEBAR),                                                                            "已失效的机关", "这个机关看起来已经失效了。"},
	{0xf7,	&glyphColor,            0,                      42,	0,	0,              0,          DF_INACTIVE_GLYPH,0,			GLYPH_LIGHT_DIM,(0), (TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_PROMOTES_ON_PLAYER_ENTRY | TM_VISUALLY_DISTINCT),"地面上的雕文",      "地面上被深深的刻下了奇怪的文字，发出魔法的闪光。"},
	{0xf7,	&glyphColor,            0,                      42,	0,	0,              0,          DF_ACTIVE_GLYPH,10000,			GLYPH_LIGHT_BRIGHT,(0), (TM_VANISHES_UPON_PROMOTION | TM_VISUALLY_DISTINCT),                                        "发光的雕文",      "地面上被深深的刻下了奇怪的文字，发出明亮的光芒。"},
	
	// liquid layer
	{LIQUID_CHAR,	&deepWaterForeColor,	&deepWaterBackColor,	40,	100,DF_STEAM_ACCUMULATION,	0,	0,				0,				NO_LIGHT,		(T_IS_FLAMMABLE | T_IS_DEEP_WATER), (TM_ALLOWS_SUBMERGING | TM_STAND_IN_TILE | TM_EXTINGUISHES_FIRE),"泥泞的水",		"水淹没了你的膝盖，你觉得有点冷。"},
	{0,             &shallowWaterForeColor, &shallowWaterBackColor, 55, 0,  DF_STEAM_ACCUMULATION,  0,  0,              0,              NO_LIGHT,       (0), (TM_STAND_IN_TILE | TM_EXTINGUISHES_FIRE | TM_ALLOWS_SUBMERGING),                              "浅水",        "水淹没了你的膝盖，你觉得有点冷。"},    
	{0xf8,		&mudForeColor,			&mudBackColor,			55,	0,	DF_PLAIN_FIRE,	0,			DF_METHANE_GAS_PUFF, 100,		NO_LIGHT,		(0), (TM_STAND_IN_TILE | TM_ALLOWS_SUBMERGING),                                                     "泥潭",				"你踩在了半人深的泥巴里。"},
	{CHASM_CHAR,	&chasmForeColor,		&black,					40,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(T_AUTO_DESCENT), (TM_STAND_IN_TILE),																"断层",				"你从洞里掉了下去。"},
	{FLOOR_CHAR,	&white,					&chasmEdgeBackColor,	80,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0, 0,                                                                                               "悬崖",	"阴冷的风沿着悬崖吹到你脸上。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_PLAIN_FIRE,	0,			DF_SPREADABLE_COLLAPSE,0,		NO_LIGHT,		(0), (TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED),                                                    "地面",			""},
	{FLOOR_CHAR,	&white,					&chasmEdgeBackColor,	45,	0,	DF_PLAIN_FIRE,	0,			DF_COLLAPSE_SPREADS,2500,		NO_LIGHT,		(0), (TM_VANISHES_UPON_PROMOTION),                                                                  "坍塌的地面",	"脚下的地面正在被莫名的力量撕裂！"},
	{LIQUID_CHAR,	&fireForeColor,			&lavaBackColor,			40,	0,	DF_OBSIDIAN,	0,			0,				0,				LAVA_LIGHT,		(T_LAVA_INSTA_DEATH), (TM_STAND_IN_TILE | TM_ALLOWS_SUBMERGING),									"岩浆",					"一股热流从岩浆处传来。"},
	{LIQUID_CHAR,	&fireForeColor,			&lavaBackColor,			40,	0,	DF_OBSIDIAN,	0,			DF_RETRACTING_LAVA,	0,			LAVA_LIGHT,		(T_LAVA_INSTA_DEATH), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_ALLOWS_SUBMERGING),"岩浆","一股热流从岩浆处传来。"},
	{LIQUID_CHAR,	&fireForeColor,			&lavaBackColor,			40,	0,	DF_OBSIDIAN,	0,			DF_OBSIDIAN_WITH_STEAM,	-1500,	LAVA_LIGHT,		(T_LAVA_INSTA_DEATH), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_ALLOWS_SUBMERGING),		"正在冷却的岩浆",         "一股热流从岩浆处传来。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		90,	0,	DF_PLAIN_FIRE,	0,			0,				0,				SUN_LIGHT,		(0), (TM_STAND_IN_TILE),																			"一束阳光",	"阳光从天花板的间隙间射到地面上。"},
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		90,	0,	DF_PLAIN_FIRE,	0,			0,				0,				DARKNESS_PATCH_LIGHT,	(0), 0,																						"一小块阴影",	"这里正好被阴影盖住，也许是一个藏身的好地方。"},
	{0xf9,		&brimstoneForeColor,	&brimstoneBackColor,	40, 100,DF_INERT_BRIMSTONE,	0,		DF_INERT_BRIMSTONE,	10,			NO_LIGHT,		(T_IS_FLAMMABLE | T_SPONTANEOUSLY_IGNITES), 0,                                                      "硫磺",	"一块块硫磺石被你踩碎，发出让人担心的嘶嘶声。"},
	{0xf9,		&brimstoneForeColor,	&brimstoneBackColor,	40, 0,	DF_INERT_BRIMSTONE,	0,		DF_ACTIVE_BRIMSTONE, 800,		NO_LIGHT,		(T_SPONTANEOUSLY_IGNITES), 0,                                                                       "硫磺",	"一块块硫磺石被你踩碎，发出让人担心的嘶嘶声。"},
	{FLOOR_CHAR,	&darkGray,				&obsidianBackColor,		50,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0, 0,                                                                                               "黑曜石地面",	"此处的地面看起来已经异常坚硬。"},
	{0xfa,	&bridgeFrontColor,		&bridgeBackColor,		45,	50,	DF_BRIDGE_FIRE,	0,			0,				0,				NO_LIGHT,		(T_IS_FLAMMABLE), (TM_VANISHES_UPON_PROMOTION),                                                     "木桥","桥上的木板一直发出咔哧的声音。"},
	{0xfa,	&bridgeFrontColor,		&bridgeBackColor,		45,	50,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(T_IS_FLAMMABLE), (TM_VANISHES_UPON_PROMOTION),                                                     "木桥","走在桥上就让人很担心会掉下去。"},
	{FLOOR_CHAR,	&white,					&chasmEdgeBackColor,	20,	50,	DF_BRIDGE_FIRE,	0,			0,				0,				NO_LIGHT,		0, 0,                                                                                               "石制桥",		"石制的路面跨过断层上方。"},
	{0,				&shallowWaterForeColor,	&shallowWaterBackColor,	60,	0,	DF_STEAM_ACCUMULATION,	0,	DF_SPREADABLE_WATER,0,			NO_LIGHT,		(0), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_EXTINGUISHES_FIRE | TM_ALLOWS_SUBMERGING),	"一滩潜水",	"水淹没了你的膝盖，你觉得有点冷。"},
	{0,				&shallowWaterForeColor,	&shallowWaterBackColor,	60,	0,	DF_STEAM_ACCUMULATION,	0,	DF_WATER_SPREADS,2500,			NO_LIGHT,		(0), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_EXTINGUISHES_FIRE | TM_ALLOWS_SUBMERGING),	"一滩潜水",		"水淹没了你的膝盖，你觉得有点冷。"},
	{MUD_CHAR,		&mudForeColor,			&mudBackColor,			55,	0,	DF_PLAIN_FIRE,	0,			DF_MUD_ACTIVATE,0,				NO_LIGHT,		(0), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED | TM_ALLOWS_SUBMERGING),			"泥潭",				"你踩在了半人深的泥巴里。"},
		
	// surface layer
	{0xf6,	&chasmForeColor,		&black,					9,	0,	DF_PLAIN_FIRE,	0,			DF_HOLE_DRAIN,	-1000,			NO_LIGHT,		(T_AUTO_DESCENT), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                                  "一个洞",				"你不小心踏进了地上的一个洞！"},
	{0xf6,	&chasmForeColor,		&black,					9,	0,	DF_PLAIN_FIRE,	0,			DF_HOLE_DRAIN,	-1000,			DESCENT_LIGHT,	(T_AUTO_DESCENT), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                                  "一个洞",				"你不小心踏进了地上的一个洞！"},
	{FLOOR_CHAR,	&white,					&chasmEdgeBackColor,	50,	0,	DF_PLAIN_FIRE,	0,			0,				-500,			NO_LIGHT,		(0), (TM_VANISHES_UPON_PROMOTION),																	"透风的地面",	"地板上整齐的间隙里一直有风从下面吹来。"},
	{LIQUID_CHAR,	&deepWaterForeColor,	&deepWaterBackColor,	41,	100,DF_STEAM_ACCUMULATION,	0,	DF_FLOOD_DRAIN,	-200,			NO_LIGHT,		(T_IS_FLAMMABLE | T_IS_DEEP_WATER), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_EXTINGUISHES_FIRE | TM_ALLOWS_SUBMERGING), "水流", "汹涌的水流正在充满房间。"},
	{0,				&shallowWaterForeColor,	&shallowWaterBackColor,	50,	0,	DF_STEAM_ACCUMULATION,	0,	DF_PUDDLE,		-100,			NO_LIGHT,		(0), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_EXTINGUISHES_FIRE | TM_ALLOWS_SUBMERGING),	"浅水",		"你能看见齐膝深的水正从地板上的一个漩涡处被吸走。"},
	{GRASS_CHAR,	&grassColor,			0,						60,	15,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                                  "杂草",	"杂乱的绿色植物，仔细看看好像是菌类。"},
	{GRASS_CHAR,	&deadGrassColor,		0,						60,	40,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                                  "枯萎的植物",		"枯萎的植物覆满了整个地面。"},
	{GRASS_CHAR,	&grayFungusColor,		0,						51,	10,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                                  "枯萎的植物",		"泥土间隐约可以看到一一缕缕的蘑菇。"},
	{GRASS_CHAR,	&fungusColor,			0,						60,	10,	DF_PLAIN_FIRE,	0,			0,				0,				FUNGUS_LIGHT,	(T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                                  "荧光蘑菇",	"荧光的蘑菇散发出微弱的亮光。"},
	{GRASS_CHAR,	&lichenColor,			0,						60,	50,	DF_PLAIN_FIRE,	0,			DF_LICHEN_GROW,	10000,			NO_LIGHT,		(T_CAUSES_POISON | T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                "剧毒苔藓",		"地上铺满了带着倒刺的苔藓。"},
	{GRASS_CHAR,	&hayColor,				&refuseBackColor,		57,	50,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                                  "干草堆",			"一叠干草上铺着一层泥土，看起来是被当做简易的床在使用。"},
	{0xf9,	&humanBloodColor,		0,						80,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(0), (TM_STAND_IN_TILE),                                                                            "一滩血迹",		"地板上沾满了血迹。"},
	{0xf9,	&insectBloodColor,		0,						80,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(0), (TM_STAND_IN_TILE),                                                                            "一滩绿色的血迹", "地板上沾满了绿色的血迹。"},
	{0xf9,	&poisonGasColor,		0,						80,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(0), (TM_STAND_IN_TILE),                                                                            "一滩紫色的血迹", "地板上沾满了紫色的血迹。"},
	{0xf9,	&acidBackColor,			0,						80,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		0, 0,                                                                                               "一滩黄色的液体", "地板上沾上了黄色的液体，看起来像是硫酸。"},
	{0xf9,	&vomitColor,			0,						80,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(0), (TM_STAND_IN_TILE),                                                                            "一滩呕吐物",	"地板上有一滩恶心的东西。"},
	{0xf9,	&urineColor,			0,						80,	0,	DF_PLAIN_FIRE,	0,			0,				100,			NO_LIGHT,		(0), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                                               "一滩尿",	"不知道哪里来的一滩尿。"},
	{0xfa,	&white,					0,						80,	0,	DF_PLAIN_FIRE,	0,			0,				0,				UNICORN_POOP_LIGHT,(0), (TM_STAND_IN_TILE),                                                                         "独角兽之屎",			"你看到一坨独角兽的屎散发出彩虹色的光芒，闻起来还有薰衣草味道。"},
	{0xfa,	&wormColor,				0,						80,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(0), (TM_STAND_IN_TILE),                                                                            "一滩内脏", "隐约看的出来是什么东西的内脏掉在了地上。"},
	{0xfb,		&ashForeColor,			0,						80,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(0), (TM_STAND_IN_TILE),                                                                            "一堆灰尘",		"你踩在了什么东西烧剩下的木炭上，溅起了些许灰尘。"},
	{0xfb,		&ashForeColor,			0,						87,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(0), (TM_STAND_IN_TILE),                                                                            "烧毁的地毯",		"地毯看起来很多年前就已经被烧焦了。"},
	{0xf9,	&shallowWaterBackColor,	0,						80,	20,	0,				0,			0,				100,			NO_LIGHT,		(T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                                  "一滩水",	"地面上有一滩水。"},
	{0xfc,	&bonesForeColor,		0,						70,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(0), (TM_STAND_IN_TILE),                                                                            "一堆骨头",		"发黄的骨头已经看不出来是什么年代的了。"},
	{0xfd,	&gray,					0,						70,	0,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(0), (TM_STAND_IN_TILE),                                                                            "一堆石头",		"地上铺满了零碎的石头"},
	{0xfe,	&mudBackColor,			&refuseBackColor,		50,	20,	DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(0), (TM_STAND_IN_TILE),                                                                            "一堆零散的东西","一些原始的工具，装饰品和雕刻堆积在一起。"},
	{0xf9,	&ectoplasmColor,		0,						70,	0,	DF_PLAIN_FIRE,	0,			0,				0,				ECTOPLASM_LIGHT,(0), (TM_STAND_IN_TILE),                                                                            "一坨胶状的东西",	"一层发热的凝胶铺在地面上。"},
	{'0',		&fireForeColor,			0,						70,	0,	DF_PLAIN_FIRE,	0,			DF_ASH,			300,			EMBER_LIGHT,	(0), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                                               "余烬",	"地上都是火烧过的痕迹。"},
	{']',		&white,					0,						19,	100,DF_PLAIN_FIRE,	0,			0,				0,				NO_LIGHT,		(T_ENTANGLES | T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_VISUALLY_DISTINCT),"蜘蛛网",          "厚实的蜘蛛网铺满了整个区域。"},
	{FOLIAGE_CHAR,	&foliageColor,			0,						45,	15,	DF_PLAIN_FIRE,	0,			DF_TRAMPLED_FOLIAGE, 0,			NO_LIGHT,		(T_OBSTRUCTS_VISION | T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_PROMOTES_ON_STEP), "厚实的枝叶",   "不知名的植物铺满整个地面，视线几乎全被挡住。"},
	{FOLIAGE_CHAR,	&deadFoliageColor,		0,						45,	80,	DF_PLAIN_FIRE,	0,			DF_SMALL_DEAD_GRASS, 0,			NO_LIGHT,		(T_OBSTRUCTS_VISION | T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_PROMOTES_ON_STEP), "枯萎的枝叶",    "枯萎的植物上脱落的树叶和枝干铺满了地面。"},
	{TRAMPLED_FOLIAGE_CHAR,&foliageColor,	0,						60,	15,	DF_PLAIN_FIRE,	0,			DF_FOLIAGE_REGROW, 100,			NO_LIGHT,		(T_IS_FLAMMABLE), (TM_VANISHES_UPON_PROMOTION),                                                     "被踏过的枝叶",		"不知名的植物铺满整个区域，视线几乎全被挡住。"},
	{FOLIAGE_CHAR,	&fungusForestLightColor,0,						45,	15,	DF_PLAIN_FIRE,	0,			DF_TRAMPLED_FUNGUS_FOREST, 0,	FUNGUS_FOREST_LIGHT,(T_OBSTRUCTS_VISION | T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_PROMOTES_ON_STEP),"一片荧光蘑菇", "整个地区长满了荧光的蘑菇，连土壤都发出油光。"},
	{TRAMPLED_FOLIAGE_CHAR,&fungusForestLightColor,0,				60,	15,	DF_PLAIN_FIRE,	0,			DF_FUNGUS_FOREST_REGROW, 100,	FUNGUS_LIGHT,	(T_IS_FLAMMABLE), (TM_VANISHES_UPON_PROMOTION),                                                     "被踏过的蘑菇", "整个地区长满了荧光的蘑菇，连土壤都发出油光。"},
	{0xe6,		&forceFieldForeColor,		&forceFieldColor,		0,	0,	0,				0,			0,				-200,			FORCEFIELD_LIGHT, (T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_GAS | T_OBSTRUCTS_DIAGONAL_MOVEMENT), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),		"绿水晶",		"翠绿的水晶看起来像正在融化一般。"},
	{'1',&gray,					0,						20,	0,	0,				0,			0,				0,				NO_LIGHT,		0, 0,																								"铁扣",		"一个厚重的铁扣卡在天花板上。"},
	{'2', &gray,				0,						20,	0,	0,				0,			0,				0,				NO_LIGHT,		0, 0,																								"铁扣",		"一个厚重的铁扣卡在地板上。"},
	{'3', &gray,				0,						20,	0,	0,				0,			0,				0,				NO_LIGHT,		0, 0,																								"铁扣",		"一个厚重的铁扣卡在天花板上。"},
	{'4', &gray,				0,						20,	0,	0,				0,			0,				0,				NO_LIGHT,		0, 0,																								"铁扣",		"一个厚重的铁扣卡在地板上。"},
	{'5',		&gray,					0,						20,	0,	0,				0,			0,				0,				NO_LIGHT,		0, 0,																								"铁扣",		"一个厚重的铁扣卡在墙上。"},
	{'6',	&gray,					0,						20,	0,	0,				0,			0,				0,				NO_LIGHT,		0, 0,																								"铁扣",		"一个厚重的铁扣卡在墙上。"},
	{'7',	&gray,					0,						20,	0,	0,				0,			0,				0,				NO_LIGHT,		0, 0,																								"铁扣",		"一个厚重的铁扣卡在墙上。"},
	{'8',	&gray,					0,						20,	0,	0,				0,			0,				0,				NO_LIGHT,		0, 0,																								"铁扣",		"一个厚重的铁扣卡在墙上。"},
	{0,				0,						0,						1,	0,	0,				0,			0,				10000,			PORTAL_ACTIVATE_LIGHT,(0), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),											"一束强光",		"从门里射出炫目的光芒。"},
	{0,				0,						0,						100,0,	0,				0,			0,				10000,			GLYPH_LIGHT_BRIGHT,(0), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION),                                            "红色的亮光",           "这片区域布满红色的光芒。"},
	
	// fire tiles
	{FIRE_CHAR,		&fireForeColor,			0,						10,	0,	0,				0,			DF_EMBERS,		500,			FIRE_LIGHT,		(T_IS_FIRE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_VISUALLY_DISTINCT),				"翻滚的火焰",		"火苗一直向上蹿。"},
	{FIRE_CHAR,		&fireForeColor,			0,						10,	0,	0,				0,			0,				2500,			BRIMSTONE_FIRE_LIGHT,(T_IS_FIRE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_VISUALLY_DISTINCT),           "橙黄色的火焰",		"炙热的硫磺石上附着橙色的火焰。"},
	{FIRE_CHAR,		&fireForeColor,			0,						10,	0,	0,				0,			DF_OBSIDIAN,	5000,			FIRE_LIGHT,		(T_IS_FIRE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_VISUALLY_DISTINCT),				"地狱之火", "汹涌的地狱之火铺满地面。"},
	{FIRE_CHAR,		&fireForeColor,			0,						10,	0,	0,              0,			0,              8000,			FIRE_LIGHT,		(T_IS_FIRE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_VISUALLY_DISTINCT),				"一团火雾", "一团味道的刺鼻的烟雾悬浮着，里面还夹杂着零星的火焰。"},
	{FIRE_CHAR,		&yellow,				0,						10,	0,	0,				0,			0,              10000,			EXPLOSION_LIGHT,(T_IS_FIRE | T_CAUSES_EXPLOSIVE_DAMAGE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_VISUALLY_DISTINCT), "剧烈的爆炸",	"爆炸产生的冲击力把你震开。"},
	{FIRE_CHAR,		&white,					0,						10,	0,	0,				0,			0,				10000,			INCENDIARY_DART_LIGHT ,(T_IS_FIRE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_VISUALLY_DISTINCT),			"一束火光",		"突然喷涌出的一束火光。"},
	{FIRE_CHAR,		&white,                 0,						10,	0,	0,				0,			DF_EMBERS,		3000,			FIRE_LIGHT,		(T_IS_FIRE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_VISUALLY_DISTINCT),				"烧着的东西",		"什么东西被烧着了"},
	
	// gas layer
	{	' ',		0,						&poisonGasColor,		35,	100,DF_GAS_FIRE,	0,			0,				0,				NO_LIGHT,		(T_IS_FLAMMABLE | T_CAUSES_DAMAGE), (TM_STAND_IN_TILE | TM_GAS_DISSIPATES),							"一团酸雾", "你感觉这紫色的酸雾把你的骨头都要融化了。"},
	{	' ',		0,						&confusionGasColor,		35,	100,DF_GAS_FIRE,	0,			0,				0,				CONFUSION_GAS_LIGHT,(T_IS_FLAMMABLE | T_CAUSES_CONFUSION), (TM_STAND_IN_TILE | TM_GAS_DISSIPATES_QUICKLY),			"一团混乱毒气", "彩虹色的烟雾让你有点不知道自己是谁了。"},
	{	' ',		0,						&vomitColor,			35,	100,DF_GAS_FIRE,	0,			0,				0,				NO_LIGHT,		(T_IS_FLAMMABLE | T_CAUSES_NAUSEA), (TM_STAND_IN_TILE | TM_GAS_DISSIPATES_QUICKLY),					"一团恶臭的气体", "恶臭的气体让人非常想吐。"},
	{	' ',		0,						&pink,					35,	100,DF_GAS_FIRE,	0,			0,				0,				NO_LIGHT,		(T_IS_FLAMMABLE | T_CAUSES_PARALYSIS), (TM_STAND_IN_TILE | TM_GAS_DISSIPATES_QUICKLY),				"一团麻痹毒气", "浅色的气体让你的肌肉突然紧绷起来。"},
	{	' ',		0,						&methaneColor,			35,	100,DF_GAS_FIRE,    0,          DF_EXPLOSION_FIRE, 0,			NO_LIGHT,		(T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_EXPLOSIVE_PROMOTE),                                        "一团沼气",	"空气中充满了沼气的味道。"},
	{	' ',		0,						&white,					35,	0,	DF_GAS_FIRE,	0,			0,				0,				NO_LIGHT,		(T_CAUSES_DAMAGE), (TM_STAND_IN_TILE | TM_GAS_DISSIPATES_QUICKLY),									"一团炙热的整齐", "炙热的水蒸气到处都是。"},
	{	' ',		0,						0,						35,	0,	DF_GAS_FIRE,	0,			0,				0,				DARKNESS_CLOUD_LIGHT,	(0), (TM_STAND_IN_TILE),                                                                    "一团诡异的黑雾", "在深黑色的雾里什么看不太清了。"},
	{	' ',		0,						&darkRed,				35,	0,	DF_GAS_FIRE,	0,			0,				0,				NO_LIGHT,		(T_CAUSES_HEALING), (TM_STAND_IN_TILE | TM_GAS_DISSIPATES_QUICKLY),                                 "一团愈合孢子", "血根草的孢子漂浮在空中。它们好像有治疗的效果。"},
	
	// bloodwort pods
	{0xd2,  &bloodflowerForeColor,  0,  10, 20, DF_PLAIN_FIRE,  0,          DF_BLOODFLOWER_PODS_GROW, 100, NO_LIGHT,       (T_OBSTRUCTS_PASSABILITY | T_IS_FLAMMABLE), (TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT),             "血根草的枝干",  "这种枝干细长的植物会发散出有治疗效果的孢子。"},
	{0xd3,     &bloodflowerForeColor, 0,                    11, 20, DF_BLOODFLOWER_POD_BURST,0, DF_BLOODFLOWER_POD_BURST, 0,   NO_LIGHT,       (T_OBSTRUCTS_PASSABILITY | T_IS_FLAMMABLE), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_PROMOTES_ON_PLAYER_ENTRY | TM_LIST_IN_SIDEBAR | TM_VISUALLY_DISTINCT), "血根草荚", "充满了学根草种子的荚裂开，散发出带有治疗效果的孢子。"},
	
	// algae
	{FLOOR_CHAR,	&floorForeColor,		&floorBackColor,		95,	0,	DF_PLAIN_FIRE,	0,			DF_ALGAE_1,		100,			NO_LIGHT,		0, 0,                                                                                               "地面",			""},
	{LIQUID_CHAR,	&deepWaterForeColor,    &deepWaterBackColor,	40,	100,DF_STEAM_ACCUMULATION,	0,	DF_ALGAE_1,     500,			LUMINESCENT_ALGAE_BLUE_LIGHT,(T_IS_FLAMMABLE | T_IS_DEEP_WATER), (TM_STAND_IN_TILE | TM_EXTINGUISHES_FIRE | TM_ALLOWS_SUBMERGING),	"荧光的水",	"水里有荧光的水藻不断飘动。"},
	{LIQUID_CHAR,	&deepWaterForeColor,    &deepWaterBackColor,	39,	100,DF_STEAM_ACCUMULATION,	0,	DF_ALGAE_REVERT,300,			LUMINESCENT_ALGAE_GREEN_LIGHT,(T_IS_FLAMMABLE | T_IS_DEEP_WATER), (TM_STAND_IN_TILE | TM_EXTINGUISHES_FIRE | TM_ALLOWS_SUBMERGING),	"荧光的水",	"水里有荧光的水藻不断飘动。"},
	
	// extensible stone bridge    
	{CHASM_CHAR,	&chasmForeColor,		&black,					40,	0,	DF_PLAIN_FIRE,	0,			0,              0,              NO_LIGHT,		(T_AUTO_DESCENT), (TM_STAND_IN_TILE),                                                               "断层",				"你从洞里掉了下去。"},
	{FLOOR_CHAR,	&white,					&chasmEdgeBackColor,	40,	0,	DF_PLAIN_FIRE,	0,			DF_BRIDGE_ACTIVATE,6000,        NO_LIGHT,		(0), (TM_VANISHES_UPON_PROMOTION),                                                                  "石制桥",		"石制的路面跨过断层上方。"},
	{FLOOR_CHAR,	&white,					&chasmEdgeBackColor,	80,	0,	DF_PLAIN_FIRE,	0,			DF_BRIDGE_ACTIVATE_ANNOUNCE,0,	NO_LIGHT,		(0), (TM_IS_WIRED),                                                                                 "悬崖",	"阴冷的风沿着悬崖吹到你脸上。"},
	
	// rat trap
	{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			DF_WALL_CRACK,  0,              NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED),			"岩石墙壁",			"坚硬的墙壁看上去非常稳固。"},
	{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			DF_WALL_SHATTER,500,			NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_LIST_IN_SIDEBAR),     "有裂痕的墙壁",		"坚硬的墙壁上有一道深深的裂痕，从底部延展上来。"},
	
	// worm tunnels
	{0,				0,						0,						100,0,	0,				0,			DF_WORM_TUNNEL_MARKER_ACTIVE,0, NO_LIGHT,       (0), (TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED),                                                    "",                     ""},
	{0,				0,						0,						100,0,	0,				0,			DF_GRANITE_CRUMBLES,-2000,		NO_LIGHT,		(0), (TM_VANISHES_UPON_PROMOTION),                                                                  "花岗岩墙壁",	"墙壁表面的覆满了崎岖不平的岩石。"},
	{WALL_CHAR,		&wallForeColor,			&wallBackColor,			0,	0,	DF_PLAIN_FIRE,	0,			DF_WALL_SHATTER,0,				NO_LIGHT,		(T_OBSTRUCTS_EVERYTHING), (TM_STAND_IN_TILE | TM_VANISHES_UPON_PROMOTION | TM_IS_WIRED),			"岩石墙壁",			"坚硬的墙壁看上去非常稳固。"},
	
	// zombie crypt
	{FIRE_CHAR,		&fireForeColor,			&statueBackColor,       0,	0,	DF_PLAIN_FIRE,	0,			0,				0,				BURNING_CREATURE_LIGHT,	(T_OBSTRUCTS_PASSABILITY | T_OBSTRUCTS_ITEMS | T_IS_FIRE), (TM_STAND_IN_TILE | TM_LIST_IN_SIDEBAR),"黄铜火盆",		"古老的火盆里燃烧着深红色的火焰。"},
};

#pragma mark Dungeon Feature definitions

// Features in the gas layer use the startprob as volume, ignore probdecr, and spawn in only a single point.
// Intercepts and slopes are in units of 0.01.
dungeonFeature dungeonFeatureCatalog[NUMBER_DUNGEON_FEATURES] = {
	// tileType					layer		start	decr	fl	txt	 fCol fRad	propTerrain	subseqDF		
	{0}, // nothing
	{GRANITE,					DUNGEON,	80,		70,		DFF_CLEAR_OTHER_TERRAIN},
	{CRYSTAL_WALL,				DUNGEON,	200,	50,		DFF_CLEAR_OTHER_TERRAIN},
	{LUMINESCENT_FUNGUS,		SURFACE,	60,		8,		DFF_BLOCKED_BY_OTHER_LAYERS},
	{GRASS,						SURFACE,	75,		5,		DFF_BLOCKED_BY_OTHER_LAYERS},
	{DEAD_GRASS,				SURFACE,	75,		5,		DFF_BLOCKED_BY_OTHER_LAYERS,	"", 0,	0,	0,		0,			DF_DEAD_FOLIAGE},
	{BONES,						SURFACE,	75,		23,		0},
	{RUBBLE,					SURFACE,	45,		23,		0},
	{FOLIAGE,					SURFACE,	100,	33,		(DFF_BLOCKED_BY_OTHER_LAYERS)},
	{FUNGUS_FOREST,				SURFACE,	100,	45,		(DFF_BLOCKED_BY_OTHER_LAYERS)},
	{DEAD_FOLIAGE,				SURFACE,	50,		30,		(DFF_BLOCKED_BY_OTHER_LAYERS)},
	
	// misc. liquids
	{SUNLIGHT_POOL,				LIQUID,		65,		6,		0},
	{DARKNESS_PATCH,			LIQUID,		65,		11,		0},
	
	// Dungeon features spawned during gameplay:
	
	// revealed secrets
	{DOOR,						DUNGEON,	0,		0,		0, "", GENERIC_FLASH_LIGHT},
	{GAS_TRAP_POISON,			DUNGEON,	0,		0,		0, "", GENERIC_FLASH_LIGHT},
	{GAS_TRAP_PARALYSIS,		DUNGEON,	0,		0,		0, "", GENERIC_FLASH_LIGHT},
	{CHASM_EDGE,				LIQUID,		100,	100,	0, "", GENERIC_FLASH_LIGHT},
	{TRAP_DOOR,					LIQUID,		0,		0,		DFF_CLEAR_OTHER_TERRAIN, "", GENERIC_FLASH_LIGHT, 0, 0, 0, DF_SHOW_TRAPDOOR_HALO},
	{GAS_TRAP_CONFUSION,		DUNGEON,	0,		0,		0, "", GENERIC_FLASH_LIGHT},
	{FLAMETHROWER,				DUNGEON,	0,		0,		0, "", GENERIC_FLASH_LIGHT},
	{FLOOD_TRAP,				DUNGEON,	0,		0,		0, "", GENERIC_FLASH_LIGHT},
	
	// bloods
	// Start probability is actually a percentage for bloods.
	// Base probability is 15 + (damage * 2/3), and then take the given percentage of that.
	// If it's a gas, we multiply the base by an additional 100.
	// Thus to get a starting gas volume of a poison potion (1000), with a hit for 10 damage, use a starting probability of 48.
	{RED_BLOOD,					SURFACE,	100,	25,		0},
	{GREEN_BLOOD,				SURFACE,	100,	25,		0},
	{PURPLE_BLOOD,				SURFACE,	100,	25,		0},
	{WORM_BLOOD,				SURFACE,	100,	25,		0},
	{ACID_SPLATTER,				SURFACE,	200,	25,		0},
	{ASH,						SURFACE,	50,		25,		0},
	{EMBERS,					SURFACE,	125,	25,		0},
	{ECTOPLASM,					SURFACE,	110,	25,		0},
	{RUBBLE,					SURFACE,	33,		25,		0},
	{ROT_GAS,					GAS,		12,		0,		0},
	
	// monster effects
	{VOMIT,						SURFACE,	30,		10,		0},
	{POISON_GAS,				GAS,		2000,	0,		0},
	{GAS_EXPLOSION,				SURFACE,	350,	100,	0,	"", EXPLOSION_FLARE_LIGHT},
	{RED_BLOOD,					SURFACE,	150,	30,		0},
	{FLAMEDANCER_FIRE,			SURFACE,	200,	75,		0},
	
	// mutation effects
	{GAS_EXPLOSION,				SURFACE,	350,	100,	0,	"尸体突然产生了剧烈的爆炸！", EXPLOSION_FLARE_LIGHT},
	{LICHEN,					SURFACE,	70,		60,		0,  "剧毒的孢子从尸体里喷了出来！"},
	
	// misc
	{NOTHING,					GAS,		0,		0,		DFF_EVACUATE_CREATURES_FIRST},
	{ROT_GAS,					GAS,		15,		0,		0},
	{STEAM,						GAS,		325,	0,		0},
	{STEAM,						GAS,		15,		0,		0},
	{METHANE_GAS,				GAS,		2,		0,		0},
	{EMBERS,					SURFACE,	0,		0,		0},
	{URINE,						SURFACE,	65,		25,		0},
	{UNICORN_POOP,				SURFACE,	65,		40,		0},
	{PUDDLE,					SURFACE,	13,		25,		0},
	{ASH,						SURFACE,	0,		0,		0},
	{ECTOPLASM,					SURFACE,	0,		0,		0},
	{FORCEFIELD,				SURFACE,	100,	50,		0},
	{LICHEN,					SURFACE,	2,		100,	(DFF_BLOCKED_BY_OTHER_LAYERS)}, // Lichen won't spread through lava.
	{RUBBLE,					SURFACE,	45,		23,		(DFF_ACTIVATE_DORMANT_MONSTER)},
	
	// foliage
	{TRAMPLED_FOLIAGE,			SURFACE,	0,		0,		0},
	{DEAD_GRASS,				SURFACE,	75,		75,		0},
	{FOLIAGE,					SURFACE,	0,		0,		(DFF_BLOCKED_BY_OTHER_LAYERS)},
	{TRAMPLED_FUNGUS_FOREST,	SURFACE,	0,		0,		0},
	{FUNGUS_FOREST,				SURFACE,	0,		0,		(DFF_BLOCKED_BY_OTHER_LAYERS)},
	
	// brimstone
	{ACTIVE_BRIMSTONE,			LIQUID,		0,		0,		0},
	{INERT_BRIMSTONE,			LIQUID,		0,		0,		0,	"", 0,	0,	0,		0,			DF_BRIMSTONE_FIRE},
	
	// bloodwort
	{BLOODFLOWER_POD,           SURFACE,    60,     60,     DFF_EVACUATE_CREATURES_FIRST},
	{BLOODFLOWER_POD,           SURFACE,    10,     10,     DFF_EVACUATE_CREATURES_FIRST},
	{HEALING_CLOUD,				GAS,		350,	0,		0},
	
	// algae
	{DEEP_WATER_ALGAE_WELL,     DUNGEON,    0,      0,      DFF_SUPERPRIORITY},
	{DEEP_WATER_ALGAE_1,		LIQUID,		50,		100,	0,  "", 0,  0,   0,     DEEP_WATER, DF_ALGAE_2},
	{DEEP_WATER_ALGAE_2,        LIQUID,     0,      0,      0},
	{DEEP_WATER,                LIQUID,     0,      0,      DFF_SUPERPRIORITY},
	
	// doors, item cages, altars, glyphs, guardians -- reusable machine components
	{OPEN_DOOR,					DUNGEON,	0,		0,		0},
	{DOOR,						DUNGEON,	0,		0,		0},
	{OPEN_IRON_DOOR_INERT,		DUNGEON,	0,		0,		0,  "", GENERIC_FLASH_LIGHT},
	{ALTAR_CAGE_OPEN,			DUNGEON,	0,		0,		0,	"罩住祭坛的笼子突然被升了起来。", GENERIC_FLASH_LIGHT},
	{ALTAR_CAGE_CLOSED,			DUNGEON,	0,		0,		(DFF_EVACUATE_CREATURES_FIRST), "祭坛上方的笼子突然降了下去。", GENERIC_FLASH_LIGHT},
	{ALTAR_INERT,				DUNGEON,	0,		0,		0},
	{FLOOR_FLOODABLE,			DUNGEON,	0,		0,		0,	"伴随着摩擦的声音，祭坛逐渐陷入地下。", GENERIC_FLASH_LIGHT},
	{PORTAL_LIGHT,				SURFACE,	0,		0,		(DFF_EVACUATE_CREATURES_FIRST | DFF_ACTIVATE_DORMANT_MONSTER), "拱门里突然闪现过另外一个世界的画面。"},
	{MACHINE_GLYPH_INACTIVE,    DUNGEON,    0,      0,      0},
	{MACHINE_GLYPH,             DUNGEON,    0,      0,      0},
	{GUARDIAN_GLOW,             SURFACE,    0,      0,      0,  ""},
	{GUARDIAN_GLOW,             SURFACE,    0,      0,      0,  "你脚下的雕文突然闪起了光，一旁的守卫开始有了反应。"},
	{GUARDIAN_GLOW,             SURFACE,    0,      0,      0,  "一旁反光的图腾突然闪起了光，反射出你脚下暗红色的雕文。"},
	{MACHINE_GLYPH,             DUNGEON,    200,    95,     DFF_BLOCKED_BY_OTHER_LAYERS},
	{WALL_LEVER,                DUNGEON,    0,      0,      0,  "你注意到墙壁里有一个隐藏的机关。", GENERIC_FLASH_LIGHT},
	{WALL_LEVER_PULLED,         DUNGEON,    0,      0,      0},
	{WALL_LEVER_HIDDEN,         DUNGEON,    0,      0,      0},
	
	// fire
	{PLAIN_FIRE,				SURFACE,	0,		0,		0},
	{GAS_FIRE,					SURFACE,	0,		0,		0},
	{GAS_EXPLOSION,				SURFACE,	60,		17,		0},
	{DART_EXPLOSION,			SURFACE,	0,		0,		0},
	{BRIMSTONE_FIRE,			SURFACE,	0,		0,		0},
	{CHASM,						LIQUID,		0,		0,		0,	"", 0,	0,	0,		0,			DF_PLAIN_FIRE},
	{PLAIN_FIRE,				SURFACE,	100,	37,		0},
	{EMBERS,					SURFACE,	0,		0,		0},
	{EMBERS,					SURFACE,	100,	94,		0},
	{OBSIDIAN,					SURFACE,	0,		0,		0},
	{ITEM_FIRE,                 SURFACE,    0,      0,      0,  "", FALLEN_TORCH_FLASH_LIGHT},
	
	{FLOOD_WATER_SHALLOW,		SURFACE,	225,	37,		0,	"", 0,	0,	0,		0,			DF_FLOOD_2},
	{FLOOD_WATER_DEEP,			SURFACE,	175,	37,		0,	"你发现周围的水面逐渐升起，地板间细小的缝隙里不断有水流注入。"},
	{FLOOD_WATER_SHALLOW,		SURFACE,	10,		25,		0},
	{HOLE,						SURFACE,	200,	100,	0},
	{HOLE_EDGE,					SURFACE,	0,		0,		0},
	
	// gas trap effects
	{POISON_GAS,				GAS,		1000,	0,		0,	"一团酸雾从地板的间隙中喷出！"},
	{CONFUSION_GAS,				GAS,		300,	0,		0,	"一团混乱毒气从地板的间隙中喷出！"},
	{METHANE_GAS,				GAS,		10000,	0,		0}, // debugging toy
	
	// potions
	{POISON_GAS,				GAS,		1000,	0,		0,	"", 0,	&poisonGasColor,4},
	{PARALYSIS_GAS,				GAS,		1000,	0,		0,	"", 0,	&pink,4},
	{CONFUSION_GAS,				GAS,		1000,	0,		0,	"", 0,	&confusionGasColor, 4},
	{PLAIN_FIRE,				SURFACE,	100,	37,		0,	"", EXPLOSION_FLARE_LIGHT},
	{DARKNESS_CLOUD,			GAS,		200,	0,		0},
	{HOLE_EDGE,					SURFACE,	300,	100,	0,	"", 0,	&darkBlue,3,0,			DF_HOLE_2},
	{LICHEN,					SURFACE,	70,		60,		0},
	
	// other items
	{PLAIN_FIRE,				SURFACE,	100,	45,		0,	"", 0,	&yellow,3},
	{HOLE_GLOW,					SURFACE,	200,	100,	DFF_SUBSEQ_EVERYWHERE,	"", 0,	&darkBlue,3,0,			DF_STAFF_HOLE_EDGE},
	{HOLE_EDGE,					SURFACE,	100,	100,	0},
	
	// machine components
	
	// coffin bursts open to reveal vampire:
	{COFFIN_OPEN,				DUNGEON,	0,		0,		DFF_ACTIVATE_DORMANT_MONSTER,	"棺材突然自己打开了，一个黑影慢慢的站了起来。", 0, &darkGray, 3},
	{PLAIN_FIRE,				SURFACE,	0,		0,		DFF_ACTIVATE_DORMANT_MONSTER,	"棺材被火点着了，原来躺在里面的吸血鬼不知怎么也烧了起来。", 0, 0, 0, 0, DF_EMBERS_PATCH},
	{MACHINE_TRIGGER_FLOOR,		DUNGEON,	200,	100,	0},
	
	// throwing tutorial:
	{ALTAR_INERT,				DUNGEON,	0,		0,		0,	"罩住祭坛的笼子突然被升了起来。", GENERIC_FLASH_LIGHT},
	{TRAP_DOOR,					LIQUID,		225,	100,	(DFF_CLEAR_OTHER_TERRAIN | DFF_SUBSEQ_EVERYWHERE), "", 0, 0, 0, 0, DF_SHOW_TRAPDOOR_HALO},
	{LAVA,						LIQUID,		225,	100,	(DFF_CLEAR_OTHER_TERRAIN)},
	{MACHINE_PRESSURE_PLATE_USED,DUNGEON,   0,      0,      0},
	
	// rat trap:
	{RAT_TRAP_WALL_CRACKING,    DUNGEON,    0,      0,      0,  "附近的墙壁里传出一阵剧烈摩擦的声音。", 0, 0, 0, 0, DF_RUBBLE},
	
	// wooden barricade at entrance:
	{WOODEN_BARRICADE,			DUNGEON,	0,		0,		0},
	{PLAIN_FIRE,				SURFACE,	0,		0,		0,	"火焰迅速吞没了木质栅栏。"},
	
	// wooden barricade around altar:
	{ADD_WOODEN_BARRICADE,		DUNGEON,	220,	100,	(DFF_TREAT_AS_BLOCKING | DFF_SUBSEQ_EVERYWHERE), "", 0, 0, 0, 0, DF_SMALL_DEAD_GRASS},
	
	// shallow water flood machine:
	{MACHINE_FLOOD_WATER_SPREADING,	LIQUID,	0,		0,		0,	"你听到了清脆的响声，随后附近的水流开始涌向这里。"},
	{SHALLOW_WATER,				LIQUID,		0,		0,		0},
	{MACHINE_FLOOD_WATER_SPREADING,LIQUID,	100,	100,	0,	"", 0,	0,	0,		FLOOR_FLOODABLE,			DF_SHALLOW_WATER},
	{MACHINE_FLOOD_WATER_DORMANT,LIQUID,	250,	100,	(DFF_TREAT_AS_BLOCKING), "", 0, 0, 0, 0,            DF_SPREADABLE_DEEP_WATER_POOL},
	{DEEP_WATER,				LIQUID,		90,		100,	(DFF_CLEAR_OTHER_TERRAIN | DFF_PERMIT_BLOCKING)},
	
	// unstable floor machine:
	{MACHINE_COLLAPSE_EDGE_SPREADING,LIQUID,0,		0,		0,	"伴随着剧烈的震动附近的地面开始坍塌！"},
	{CHASM,						LIQUID,		0,		0,		DFF_CLEAR_OTHER_TERRAIN, "", 0, 0, 0, 0, DF_SHOW_TRAPDOOR_HALO},
	{MACHINE_COLLAPSE_EDGE_SPREADING,LIQUID,100,	100,	0,	"", 0,	0,	0,	FLOOR_FLOODABLE,	DF_COLLAPSE},
	{MACHINE_COLLAPSE_EDGE_DORMANT,LIQUID,	0,		0,		0},
	
	// levitation bridge machine:
	{CHASM_WITH_HIDDEN_BRIDGE_ACTIVE,LIQUID,100,    100,    0,  "", 0, 0,  0,  CHASM_WITH_HIDDEN_BRIDGE,  DF_BRIDGE_APPEARS},
	{CHASM_WITH_HIDDEN_BRIDGE_ACTIVE,LIQUID,100,    100,    0,  "一作石桥在某种机关的作用下延伸了出来。", 0, 0,  0,  CHASM_WITH_HIDDEN_BRIDGE,  DF_BRIDGE_APPEARS},
	{STONE_BRIDGE,				LIQUID,		0,		0,		0},
	{MACHINE_CHASM_EDGE,        LIQUID,     100,	100,	0},
	
	// retracting lava pool:
	{LAVA_RETRACTABLE,          LIQUID,     100,    100,     0,  "", 0, 0,  0,  LAVA},
	{LAVA_RETRACTING,			LIQUID,		0,		0,		0,	"岩浆逐渐冷却下来变成了黑色的石块。"},
	{OBSIDIAN,					SURFACE,	0,		0,		0,	"", 0,	0,	0,		0,			DF_STEAM_ACCUMULATION},
	
	// hidden poison vent machine:
	{MACHINE_POISON_GAS_VENT_DORMANT,DUNGEON,0,		0,		0,	"你发现地板的间隙里有一个已失效的排气孔。", GENERIC_FLASH_LIGHT},
	{MACHINE_POISON_GAS_VENT,	DUNGEON,	0,		0,		0,	"致命的紫色毒气从地板的间隙中涌出！"},
	{PORTCULLIS_CLOSED,			DUNGEON,	0,		0,		DFF_EVACUATE_CREATURES_FIRST,	"伴随着金属的敲击声，一扇铁栅栏降了下来。", GENERIC_FLASH_LIGHT},
	{PORTCULLIS_DORMANT,		DUNGEON,	0,		0,		0,  "栅栏慢慢的升起到天花板里。", GENERIC_FLASH_LIGHT},
	{POISON_GAS,				GAS,		25,		0,		0},
	
	// hidden methane vent machine:
	{MACHINE_METHANE_VENT_DORMANT,DUNGEON,0,		0,		0,	"你发现地板的间隙里有一个已失效的排气孔。", GENERIC_FLASH_LIGHT},
	{MACHINE_METHANE_VENT,		DUNGEON,	0,		0,		0,	"一股沼气从地板的间隙中涌出！", 0, 0, 0, 0, DF_VENT_SPEW_METHANE},
	{METHANE_GAS,				GAS,		60,		0,		0},
	{PILOT_LIGHT,				DUNGEON,	0,		0,		0,	"一个火把从墙上掉落了下来。", FALLEN_TORCH_FLASH_LIGHT},
	
	// paralysis trap:
	{MACHINE_PARALYSIS_VENT,    DUNGEON,    0,		0,		0,	"你发现地板的间隙里有一个已失效的排气孔。", GENERIC_FLASH_LIGHT},
	{PARALYSIS_GAS,				GAS,		350,	0,		0,	"麻痹毒气从地板的间隙中涌出！", 0, 0, 0, 0, DF_REVEAL_PARALYSIS_VENT_SILENTLY},
	{MACHINE_PARALYSIS_VENT,    DUNGEON,    0,		0,		0},
	
	// thematic dungeon:
	{RED_BLOOD,					SURFACE,	75,     25,		0},
	
	// statuary:
	{STATUE_CRACKING,			DUNGEON,	0,		0,		0,	"你看到雕像上出现了许多裂痕。", 0, 0, 0, 0, DF_RUBBLE},
	{RUBBLE,					SURFACE,	120,	100,	DFF_ACTIVATE_DORMANT_MONSTER,	"雕像碎裂了开来。", 0, &darkGray, 3, 0, DF_RUBBLE},
	
	// hidden turrets:
	{WALL,					DUNGEON,	0,		0,		DFF_ACTIVATE_DORMANT_MONSTER,	"你听到了清脆的响声，墙壁里突然出现一个炮塔。", 0, 0, 0, 0, DF_RUBBLE},
	
	// worm tunnels:
	{WORM_TUNNEL_MARKER_DORMANT,LIQUID,     5,      5,      0,  "", 0,  0,  GRANITE},
	{WORM_TUNNEL_MARKER_ACTIVE, LIQUID,     0,      0,      0},
	{FLOOR,                     DUNGEON,    0,      0,      (DFF_SUPERPRIORITY | DFF_ACTIVATE_DORMANT_MONSTER),  "", 0, 0,  0,  0,  DF_TUNNELIZE},
	{FLOOR,                     DUNGEON,    0,      0,      0,  "周围的墙壁在剧烈的震动中倒下了。", 0, &darkGray,  5,  0,  DF_TUNNELIZE},
	
	// haunted room:
	{DARK_FLOOR_DARKENING,		DUNGEON,	0,		0,		0,	"房间里的突然暗了一下，你突然感到空气中有一丝凉意。"},
	{DARK_FLOOR,				DUNGEON,	0,		0,		DFF_ACTIVATE_DORMANT_MONSTER,	"", 0, 0, 0, 0, DF_ECTOPLASM_DROPLET},
	{HAUNTED_TORCH_TRANSITIONING,DUNGEON,   0,      0,      0},
	{HAUNTED_TORCH,             DUNGEON,    0,      0,      0},
	
	// mud pit:
	{MACHINE_MUD_DORMANT,		LIQUID,		100,	100,	0},
	{MUD,						LIQUID,		0,		0,		DFF_ACTIVATE_DORMANT_MONSTER,	"沼泽里不断有大块的气泡从底部升起。"},
	
	// idyll:
	{SHALLOW_WATER,				LIQUID,		250,	100,	(DFF_TREAT_AS_BLOCKING), "", 0, 0, 0, 0, DF_DEEP_WATER_POOL},
	{DEEP_WATER,				LIQUID,		90,		100,	(DFF_CLEAR_OTHER_TERRAIN)},
	
	// swamp:
	{SHALLOW_WATER,				LIQUID,		20,		100,	0},
	{GRAY_FUNGUS,				SURFACE,	80,		50,		0,	"", 0, 0, 0, 0, DF_SWAMP_MUD},
	{MUD,						LIQUID,		75,		5,		0,	"", 0, 0, 0, 0, DF_SWAMP_WATER},
	
	// camp:
	{HAY,						SURFACE,	90,		87,		0},
	{JUNK,						SURFACE,	20,		20,		0},
	
	// remnants:
	{CARPET,					DUNGEON,	110,	20,		DFF_SUBSEQ_EVERYWHERE,	"", 0, 0, 0, 0, DF_REMNANT_ASH},
	{BURNED_CARPET,				SURFACE,	120,	100,	0},
	
	// chasm catwalk:
	{CHASM,						LIQUID,		0,		0,		DFF_CLEAR_OTHER_TERRAIN, "", 0, 0, 0, 0, DF_SHOW_TRAPDOOR_HALO},
	{STONE_BRIDGE,				LIQUID,		0,		0,		DFF_CLEAR_OTHER_TERRAIN},

	// lake catwalk:
	{DEEP_WATER,				LIQUID,		0,		0,		DFF_CLEAR_OTHER_TERRAIN, "", 0, 0, 0, 0, DF_LAKE_HALO},
	{SHALLOW_WATER,				LIQUID,		160,	100,	0},
	
	// worms pop out of walls:
	{RUBBLE,					SURFACE,	120,	100,	DFF_ACTIVATE_DORMANT_MONSTER,	"附近的墙壁突然裂开，地面上四处散落着碎石。", 0, &darkGray, 3, 0, DF_RUBBLE},
	
	// monster cages open:
	{MONSTER_CAGE_OPEN,			DUNGEON,	0,		0,		0},
};

#pragma mark Lights

// radius is in units of 0.01
const lightSource lightCatalog[NUMBER_LIGHT_KINDS] = {
	//color					radius range			fade%	passThroughCreatures
	{0},																// NO_LIGHT
	{&minersLightColor,		{0, 0, 1},				35,		true},		// miners light
	{&fireBoltColor,		{300, 400, 1},			0,		false},		// burning creature light
	{&wispLightColor,		{400, 800, 1},			0,		false},		// will-o'-the-wisp light
	{&fireBoltColor,		{300, 400, 1},			0,		false},		// salamander glow
	{&pink,					{600, 600, 1},			0,		true},		// imp light
	{&pixieColor,			{400, 600, 1},			50,		false},		// pixie light
	{&lichLightColor,		{1500, 1500, 1},		0,		false},		// lich light
	{&flamedancerCoronaColor,{1000, 2000, 1},		0,		false},		// flamedancer light
	{&sentinelLightColor,	{300, 500, 1},			0,		false},		// sentinel light
	{&unicornLightColor,	{300, 400, 1},			0,		false},		// unicorn light
	{&ifritLightColor,		{300, 600, 1},			0,		false},		// ifrit light
	{&fireBoltColor,		{400, 600, 1},			0,		false},		// phoenix light
	{&fireBoltColor,		{150, 300, 1},			0,		false},		// phoenix egg light
	{&spectralBladeLightColor,{350, 350, 1},		0,		false},		// spectral blades
	{&summonedImageLightColor,{350, 350, 1},		0,		false},		// weapon images
	{&lightningColor,		{250, 250, 1},			35,		false},		// lightning turret light
	{&lightningColor,		{300, 300, 1},			0,		false},		// bolt glow
	{&telepathyColor,		{200, 200, 1},			0,		true},		// telepathy light
	
	// flares:
	{&scrollProtectionColor,{600, 600, 1},			0,		true},		// scroll of protection flare
	{&scrollEnchantmentColor,{600, 600, 1},			0,		true},		// scroll of enchantment flare
	{&potionStrengthColor,  {600, 600, 1},			0,		true},		// potion of strength flare
	{&genericFlashColor,    {300, 300, 1},			0,		true},		// generic flash flare
	{&fireFlashColor,		{800, 800, 1},			0,		false},		// fallen torch flare
	{&summoningFlashColor,  {600, 600, 1},			0,		true},		// summoning flare
	{&explosionFlareColor,  {5000, 5000, 1},        0,      false},     // explosion (explosive bloat or incineration potion)
	
	// glowing terrain:
	{&torchLightColor,		{1000, 1000, 1},		50,		false},		// torch
	{&lavaLightColor,		{300, 300, 1},			50,		false},		// lava
	{&sunLightColor,		{200, 200, 1},			25,		true},		// sunlight
	{&darknessPatchColor,	{400, 400, 1},			0,		true},		// darkness patch
	{&fungusLightColor,		{300, 300, 1},			50,		false},		// luminescent fungus
	{&fungusForestLightColor,{500, 500, 1},			0,		false},		// luminescent forest
	{&algaeBlueLightColor,	{300, 300, 1},			0,		false},		// luminescent algae blue
	{&algaeGreenLightColor,	{300, 300, 1},			0,		false},		// luminescent algae green
	{&ectoplasmColor,		{200, 200, 1},			50,		false},		// ectoplasm
	{&unicornLightColor,	{200, 200, 1},			0,		false},		// unicorn poop light
	{&lavaLightColor,		{200, 200, 1},			50,		false},		// embers
	{&lavaLightColor,		{500, 1000, 1},			0,		false},		// fire
	{&lavaLightColor,		{200, 300, 1},			0,		false},		// brimstone fire
	{&explosionColor,		{DCOLS*100,DCOLS*100,1},100,	false},		// explosions
	{&dartFlashColor,		{15*100,15*100,1},		0,		false},		// incendiary darts
	{&portalActivateLightColor,	{DCOLS*100,DCOLS*100,1},0,	false},		// portal activation
	{&confusionLightColor,	{300, 300, 1},			100,	false},		// confusion gas
	{&darknessCloudColor,	{500, 500, 1},			0,		true},		// darkness cloud
	{&forceFieldLightColor,	{200, 200, 1},			50,		false},		// forcefield
	{&crystalWallLightColor,{300, 500, 1},			50,		false},		// crystal wall
	{&torchLightColor,		{200, 400, 1},			0,		false},		// candle light
	{&hauntedTorchColor,	{400, 600, 1},          0,		false},		// haunted torch
	{&glyphLightColor,      {100, 100, 1},          0,      false},     // glyph dim light
	{&glyphLightColor,      {300, 300, 1},          0,      false},     // glyph bright light
	{&descentLightColor,    {600, 600, 1},          0,      false},     // magical pit light
};

#pragma mark Blueprints

const blueprint blueprintCatalog[NUMBER_BLUEPRINTS] = {
	{{0}}, // nothing
	//BLUEPRINTS:
	//depths			roomSize	freq	featureCt	flags	(features on subsequent lines)
	
		//FEATURES:
		//DF		terrain		layer		instanceCtRange	minInsts	itemCat		itemKind	itemValMin		monsterKind		reqSpace		hordeFl		itemFlags	featureFlags
	
	// -- REWARD ROOMS --
	
	// Mixed item library -- can check one item out at a time
	{{1, 12},           {30, 50},	30,		5,			(BP_ROOM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS | BP_OPEN_INTERIOR | BP_IMPREGNABLE | BP_REWARD),	{
		{0,			CARPET,		DUNGEON,		{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)},
		{0,			0,          0,              {1,1},		1,			0,          0,          0,				0,				2,				0,			0,          (MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_BUILD_VESTIBULE)},
		{0,			ALTAR_CAGE_OPEN,DUNGEON,	{3,3},		3,			(WEAPON|ARMOR|WAND),-1,	0,				0,				2,				0,			(ITEM_IS_KEY | ITEM_KIND_AUTO_ID | ITEM_PLAYER_AVOIDS),	(MF_GENERATE_ITEM | MF_NO_THROWING_WEAPONS | MF_TREAT_AS_BLOCKING | MF_IMPREGNABLE)},
		{0,			ALTAR_CAGE_OPEN,DUNGEON,	{2,3},		2,			(STAFF|RING|CHARM),-1,	0,				0,				2,				0,			(ITEM_IS_KEY | ITEM_KIND_AUTO_ID | ITEM_MAX_CHARGES_KNOWN | ITEM_PLAYER_AVOIDS),	(MF_GENERATE_ITEM | MF_NO_THROWING_WEAPONS | MF_TREAT_AS_BLOCKING | MF_IMPREGNABLE)},
		{0,			STATUE_INERT,DUNGEON,		{2,3},		0,			0,			-1,			0,				0,				2,				0,          0,          (MF_TREAT_AS_BLOCKING | MF_BUILD_IN_WALLS | MF_IMPREGNABLE)}}},
	// Single item category library -- can check one item out at a time
	{{1, 15},           {30, 50},	15,		5,			(BP_ROOM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS | BP_OPEN_INTERIOR | BP_IMPREGNABLE | BP_REWARD),	{
		{0,			CARPET,		DUNGEON,		{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)},
		{0,			0,          0,              {1,1},		1,			0,          0,          0,				0,				2,				0,			0,          (MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_BUILD_VESTIBULE)},
		{0,			ALTAR_CAGE_OPEN,DUNGEON,	{3,4},		3,			(RING),		-1,			0,				0,				2,				0,			(ITEM_IS_KEY | ITEM_KIND_AUTO_ID | ITEM_MAX_CHARGES_KNOWN | ITEM_PLAYER_AVOIDS),	(MF_GENERATE_ITEM | MF_TREAT_AS_BLOCKING | MF_ALTERNATIVE | MF_IMPREGNABLE)},
		{0,			ALTAR_CAGE_OPEN,DUNGEON,	{4,5},		4,			(STAFF),	-1,			0,				0,				2,				0,			(ITEM_IS_KEY | ITEM_KIND_AUTO_ID | ITEM_MAX_CHARGES_KNOWN | ITEM_PLAYER_AVOIDS),	(MF_GENERATE_ITEM | MF_TREAT_AS_BLOCKING | MF_ALTERNATIVE | MF_IMPREGNABLE)},
		{0,			STATUE_INERT,DUNGEON,		{2,3},		0,			0,			-1,			0,				0,				2,				0,          0,          (MF_TREAT_AS_BLOCKING | MF_BUILD_IN_WALLS | MF_IMPREGNABLE)}}},
	// Treasure room -- apothecary or archive (potions or scrolls)
	{{8, AMULET_LEVEL},	{20, 40},	20,		6,			(BP_ROOM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS | BP_OPEN_INTERIOR | BP_IMPREGNABLE | BP_REWARD),	{
		{0,			CARPET,		DUNGEON,		{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)},
		{0,			0,			0,				{5,7},		2,			(POTION),	-1,			0,				0,				2,				0,			0,			(MF_GENERATE_ITEM | MF_ALTERNATIVE | MF_TREAT_AS_BLOCKING)},
		{0,			0,			0,				{4,6},		2,			(SCROLL),	-1,			100,			0,				2,				0,			0,			(MF_GENERATE_ITEM | MF_ALTERNATIVE | MF_TREAT_AS_BLOCKING)},
		{0,			FUNGUS_FOREST,SURFACE,		{3,4},		0,			0,			-1,			0,				0,				2,				0,			0,			0},
		{0,			0,          0,              {1,1},		1,			0,          0,          0,				0,				2,				0,			0,          (MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_BUILD_VESTIBULE)},
		{0,			STATUE_INERT,DUNGEON,		{2,3},		0,			0,			-1,			0,				0,				2,				0,          0,          (MF_TREAT_AS_BLOCKING | MF_BUILD_IN_WALLS | MF_IMPREGNABLE)}}},
	// Guaranteed good permanent item on a glowing pedestal (runic weapon/armor, 2 staffs or 2 charms)
	{{5, AMULET_LEVEL},	{10, 30},	30,		6,			(BP_ROOM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS | BP_OPEN_INTERIOR | BP_IMPREGNABLE | BP_REWARD),	{
		{0,			CARPET,		DUNGEON,		{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)},
		{0,			STATUE_INERT,DUNGEON,		{2,3},		0,			0,			-1,			0,				0,				2,				0,          0,          (MF_TREAT_AS_BLOCKING | MF_BUILD_IN_WALLS | MF_IMPREGNABLE)},
		{0,			PEDESTAL,	DUNGEON,		{1,1},		1,			(WEAPON),	-1,			500,			0,				2,				0,			ITEM_IDENTIFIED,(MF_GENERATE_ITEM | MF_ALTERNATIVE | MF_REQUIRE_GOOD_RUNIC | MF_NO_THROWING_WEAPONS | MF_TREAT_AS_BLOCKING)},
		{0,			PEDESTAL,	DUNGEON,		{1,1},		1,			(ARMOR),	-1,			500,			0,				2,				0,			ITEM_IDENTIFIED,(MF_GENERATE_ITEM | MF_ALTERNATIVE | MF_REQUIRE_GOOD_RUNIC | MF_TREAT_AS_BLOCKING)},
		{0,			PEDESTAL,	DUNGEON,		{2,2},		2,			(STAFF),	-1,			1000,			0,				2,				0,			(ITEM_KIND_AUTO_ID | ITEM_MAX_CHARGES_KNOWN),	(MF_GENERATE_ITEM | MF_ALTERNATIVE | MF_TREAT_AS_BLOCKING)},
		{0,			0,          0,              {1,1},		1,			0,          0,          0,				0,				2,				0,			0,          (MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_BUILD_VESTIBULE)}}},
	// Guaranteed good consumable item on glowing pedestals (scrolls of enchanting, potion of life)
	{{10, AMULET_LEVEL},{10, 30},	30,		7,			(BP_ROOM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS | BP_OPEN_INTERIOR | BP_IMPREGNABLE | BP_REWARD),	{
		{0,			CARPET,		DUNGEON,		{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)},
		{0,			STATUE_INERT,DUNGEON,		{1,3},		0,			0,			-1,			0,				0,				2,				0,          0,          (MF_TREAT_AS_BLOCKING | MF_BUILD_IN_WALLS | MF_IMPREGNABLE)},
		{0,			PEDESTAL,	DUNGEON,		{1,1},		2,			(SCROLL),	SCROLL_ENCHANTING,0,		0,				2,				0,			(ITEM_KIND_AUTO_ID),	(MF_GENERATE_ITEM | MF_ALTERNATIVE | MF_TREAT_AS_BLOCKING)},
		{0,			PEDESTAL,	DUNGEON,		{1,1},		1,			(POTION),	POTION_LIFE,0,              0,              2,				0,			(ITEM_KIND_AUTO_ID),	(MF_GENERATE_ITEM | MF_ALTERNATIVE | MF_TREAT_AS_BLOCKING)},
		{0,			0,          0,              {1,1},		1,			0,          0,          0,				0,				2,				0,			0,          (MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_BUILD_VESTIBULE)}}},
	// Outsourced item -- same item possibilities as in the good permanent item reward room, but directly adopted by 1-2 key machines.
	{{5, 17},           {0, 0},     20,		4,			(BP_REWARD | BP_NO_INTERIOR_FLAG),	{
		{0,			0,			0,				{1,1},		1,			(WEAPON),	-1,			500,			0,				0,				0,			ITEM_IDENTIFIED,(MF_GENERATE_ITEM | MF_ALTERNATIVE | MF_REQUIRE_GOOD_RUNIC | MF_NO_THROWING_WEAPONS | MF_OUTSOURCE_ITEM_TO_MACHINE | MF_BUILD_ANYWHERE_ON_LEVEL)},
		{0,			0,			0,				{1,1},		1,			(ARMOR),	-1,			500,			0,				0,				0,			ITEM_IDENTIFIED,(MF_GENERATE_ITEM | MF_ALTERNATIVE | MF_REQUIRE_GOOD_RUNIC | MF_OUTSOURCE_ITEM_TO_MACHINE | MF_BUILD_ANYWHERE_ON_LEVEL)},
		{0,			0,			0,				{2,2},		2,			(STAFF),	-1,			1000,			0,				0,				0,			ITEM_KIND_AUTO_ID, (MF_GENERATE_ITEM | MF_ALTERNATIVE | MF_OUTSOURCE_ITEM_TO_MACHINE | MF_BUILD_ANYWHERE_ON_LEVEL)},
		{0,			0,			0,				{1,2},		1,			(CHARM),	-1,			0,              0,				0,				0,			ITEM_KIND_AUTO_ID, (MF_GENERATE_ITEM | MF_ALTERNATIVE | MF_OUTSOURCE_ITEM_TO_MACHINE | MF_BUILD_ANYWHERE_ON_LEVEL)}}},
	// Dungeon -- two allies chained up for the taking
	{{5, AMULET_LEVEL},	{30, 80},	12,		6,			(BP_ROOM | BP_REWARD),	{
		{0,			VOMIT,		SURFACE,		{2,2},		2,			0,			-1,			0,				0,				2,				(HORDE_MACHINE_CAPTIVE | HORDE_LEADER_CAPTIVE), 0, (MF_GENERATE_HORDE | MF_TREAT_AS_BLOCKING)},
		{DF_AMBIENT_BLOOD,MANACLE_T,SURFACE,	{1,2},		1,			0,			-1,			0,				0,				1,				0,			0,			0},
		{DF_AMBIENT_BLOOD,MANACLE_L,SURFACE,	{1,2},		1,			0,			-1,			0,				0,				1,				0,			0,			0},
		{DF_BONES,	0,			0,				{2,3},		1,			0,			-1,			0,				0,				1,				0,			0,			0},
		{DF_VOMIT,	0,			0,				{2,3},		1,			0,			-1,			0,				0,				1,				0,			0,			0},
		{0,			0,          0,              {1,1},		1,			0,          0,          0,				0,				2,				0,			0,          (MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_BUILD_VESTIBULE)}}},
	// Kennel -- allies locked in cages in an open room; choose one or two to unlock and take with you.
	{{5, AMULET_LEVEL},	{30, 80},	20,		5,			(BP_ROOM | BP_REWARD),	{
		{0,			MONSTER_CAGE_CLOSED,DUNGEON,{3,5},		3,			0,			-1,			0,				0,				2,				(HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE), 0, (MF_GENERATE_HORDE | MF_TREAT_AS_BLOCKING | MF_IMPREGNABLE)},
		{0,			0,			0,				{1,2},		1,			KEY,		KEY_CAGE,	0,				0,				1,				0,			(ITEM_IS_KEY | ITEM_PLAYER_AVOIDS),(MF_PERMIT_BLOCKING | MF_GENERATE_ITEM | MF_OUTSOURCE_ITEM_TO_MACHINE | MF_SKELETON_KEY | MF_KEY_DISPOSABLE)},
		{DF_AMBIENT_BLOOD, 0,	0,				{3,5},		3,			0,			-1,			0,				0,				1,				0,			0,			0},
		{DF_BONES,	0,			0,				{3,5},		3,			0,			-1,			0,				0,				1,				0,			0,			0},
		{0,			TORCH_WALL,	DUNGEON,		{2,3},		2,			0,			0,			0,				0,				1,				0,			0,			(MF_BUILD_IN_WALLS)}
	}},
	// Vampire lair -- allies locked in cages and chained in a hidden room with a vampire in a coffin; vampire has one cage key.
	{{10, AMULET_LEVEL},{50, 80},	5,		4,			(BP_ROOM | BP_REWARD | BP_SURROUND_WITH_WALLS | BP_PURGE_INTERIOR),	{
		{DF_AMBIENT_BLOOD,0,	0,				{1,2},		1,			0,			-1,			0,				0,				2,				(HORDE_MACHINE_CAPTIVE | HORDE_LEADER_CAPTIVE), 0, (MF_GENERATE_HORDE | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)},
		{DF_AMBIENT_BLOOD,MONSTER_CAGE_CLOSED,DUNGEON,{2,4},2,			0,			-1,			0,				0,				2,				(HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE), 0, (MF_GENERATE_HORDE | MF_TREAT_AS_BLOCKING | MF_IMPREGNABLE | MF_NOT_IN_HALLWAY)},
		{DF_TRIGGER_AREA,COFFIN_CLOSED,0,		{1,1},		1,			KEY,		KEY_CAGE,	0,				MK_VAMPIRE,		1,				0,			(ITEM_IS_KEY | ITEM_PLAYER_AVOIDS),(MF_GENERATE_MONSTER | MF_GENERATE_ITEM | MF_SKELETON_KEY | MF_MONSTER_TAKE_ITEM | MF_MONSTERS_DORMANT | MF_FAR_FROM_ORIGIN | MF_KEY_DISPOSABLE)},
		{DF_AMBIENT_BLOOD,SECRET_DOOR,DUNGEON,	{1,1},		1,			0,			0,			0,				0,				1,				0,			0,			(MF_PERMIT_BLOCKING | MF_BUILD_AT_ORIGIN)}}},
	// Legendary ally -- approach the altar with the crystal key to activate a portal and summon a legendary ally.
	{{8, AMULET_LEVEL},{30, 50},	15,		2,			(BP_ROOM | BP_REWARD),	{
		{DF_LUMINESCENT_FUNGUS,	ALTAR_KEYHOLE, DUNGEON,	{1,1}, 1,		KEY,		KEY_PORTAL,	0,				0,				2,				0,			(ITEM_IS_KEY | ITEM_PLAYER_AVOIDS),(MF_GENERATE_ITEM | MF_NOT_IN_HALLWAY | MF_NEAR_ORIGIN | MF_OUTSOURCE_ITEM_TO_MACHINE | MF_KEY_DISPOSABLE)},
		{DF_LUMINESCENT_FUNGUS,	PORTAL,	DUNGEON,{1,1},		1,			0,			-1,			0,				0,				2,				HORDE_MACHINE_LEGENDARY_ALLY,0,	(MF_GENERATE_HORDE | MF_MONSTERS_DORMANT | MF_FAR_FROM_ORIGIN)}}},
	
	// -- VESTIBULES --
	
	// Plain locked door, key guarded by an adoptive room
	{{1, AMULET_LEVEL},	{1, 1},     100,		1,		(BP_VESTIBULE),	{
		{0,			LOCKED_DOOR, DUNGEON,		{1,1},		1,			KEY,		KEY_DOOR,	0,				0,				1,				0,			(ITEM_IS_KEY | ITEM_PLAYER_AVOIDS), (MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_GENERATE_ITEM | MF_OUTSOURCE_ITEM_TO_MACHINE | MF_KEY_DISPOSABLE | MF_IMPREGNABLE)}}},
	// Plain secret door
	{{2, AMULET_LEVEL},	{1, 1},     1,		1,			(BP_VESTIBULE),	{
		{0,			SECRET_DOOR, DUNGEON,		{1,1},		1,			0,          0,          0,				0,				1,				0,			0,          (MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING)}}},
	// Lever and either an exploding wall or a portcullis
	{{4, AMULET_LEVEL},	{1, 1},     10,		3,			(BP_VESTIBULE),	{
		{0,         WORM_TUNNEL_OUTER_WALL,DUNGEON,{1,1},	1,			0,			-1,			0,				0,				1,				0,			0,			(MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_IMPREGNABLE | MF_ALTERNATIVE)},
		{0,			PORTCULLIS_CLOSED,DUNGEON,  {1,1},      1,			0,			0,			0,				0,				3,				0,			0,			(MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_IMPREGNABLE | MF_ALTERNATIVE)},
		{0,			WALL_LEVER_HIDDEN,DUNGEON,  {1,1},      1,			0,			-1,			0,				0,				1,				0,			0,			(MF_BUILD_IN_WALLS | MF_IN_PASSABLE_VIEW_OF_ORIGIN | MF_BUILD_ANYWHERE_ON_LEVEL)}}},
	// Flammable barricade in the doorway -- burn the wooden barricade to enter
	{{1, 6},			{1, 1},     8,		3,			(BP_VESTIBULE), {
		{0,			ADD_WOODEN_BARRICADE,DUNGEON,{1,1},		1,			0,			0,			0,				0,				1,				0,			0,			(MF_PERMIT_BLOCKING | MF_BUILD_AT_ORIGIN)},
		{0,			0,			0,				{1,1},		1,			WEAPON,		INCENDIARY_DART,0,			0,				1,				0,			0,			(MF_GENERATE_ITEM | MF_BUILD_ANYWHERE_ON_LEVEL | MF_NOT_IN_HALLWAY | MF_ALTERNATIVE)},
		{0,			0,			0,				{1,1},		1,			POTION,		POTION_INCINERATION,0,		0,				1,				0,			0,			(MF_GENERATE_ITEM | MF_BUILD_ANYWHERE_ON_LEVEL | MF_NOT_IN_HALLWAY | MF_ALTERNATIVE)}}},
	// Statue in the doorway -- use a scroll of shattering to enter
	{{1, AMULET_LEVEL},	{1, 1},     6,		2,			(BP_VESTIBULE), {
		{0,			STATUE_INERT,DUNGEON,       {1,1},		1,			0,			0,			0,				0,				1,				0,			0,			(MF_PERMIT_BLOCKING | MF_BUILD_AT_ORIGIN)},
		{0,			0,			0,				{1,1},		1,			SCROLL,		SCROLL_SHATTERING,0,		0,				1,				0,			0,			(MF_GENERATE_ITEM | MF_BUILD_ANYWHERE_ON_LEVEL | MF_NOT_IN_HALLWAY)}}},
	// Statue in the doorway -- bursts to reveal monster
	{{5, AMULET_LEVEL},	{2, 2},     6,		2,			(BP_VESTIBULE), {
		{0,			STATUE_DORMANT,DUNGEON,		{1, 1},		1,			0,			-1,			0,				0,				1,				HORDE_MACHINE_STATUE,0,	(MF_PERMIT_BLOCKING | MF_GENERATE_HORDE | MF_MONSTERS_DORMANT | MF_BUILD_AT_ORIGIN | MF_ALTERNATIVE)},
		{0,			MACHINE_TRIGGER_FLOOR,DUNGEON,{0,0},	1,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)}}},
	// Throwing tutorial -- toss an item onto the pressure plate to retract the portcullis
	{{1, 4},			{70, 70},	10,     3,          (BP_VESTIBULE | BP_NO_INTERIOR_FLAG), {
		{DF_MEDIUM_HOLE, MACHINE_PRESSURE_PLATE, LIQUID, {1,1}, 1,		0,			0,			0,				0,				1,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)},
		{0,			PORTCULLIS_CLOSED,DUNGEON,  {1,1},      1,			0,			0,			0,				0,				3,				0,			0,			(MF_IMPREGNABLE | MF_PERMIT_BLOCKING | MF_BUILD_AT_ORIGIN | MF_ALTERNATIVE)},
		{0,         WORM_TUNNEL_OUTER_WALL,DUNGEON,{1,1},	1,			0,			-1,			0,				0,				1,				0,			0,			(MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_IMPREGNABLE | MF_ALTERNATIVE)}}},
	// Pit traps -- area outside entrance is full of pit traps
	{{1, AMULET_LEVEL},	{30, 60},	10,		3,			(BP_VESTIBULE | BP_OPEN_INTERIOR | BP_NO_INTERIOR_FLAG),	{
		{0,			DOOR,       DUNGEON,        {1,1},      1,			0,			0,			0,				0,				1,				0,			0,			(MF_PERMIT_BLOCKING | MF_BUILD_AT_ORIGIN | MF_ALTERNATIVE)},
		{0,			SECRET_DOOR,DUNGEON,        {1,1},      1,			0,			0,			0,				0,				1,				0,			0,			(MF_IMPREGNABLE | MF_PERMIT_BLOCKING | MF_BUILD_AT_ORIGIN | MF_ALTERNATIVE)},
		{0,			TRAP_DOOR_HIDDEN,DUNGEON,	{60, 60},	1,			0,			-1,			0,				0,				1,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_REPEAT_UNTIL_NO_PROGRESS)}}},
	// Beckoning obstacle -- a mirrored totem guards the door, and glyph are around the doorway.
	{{5, AMULET_LEVEL}, {15, 30},	10,		3,			(BP_VESTIBULE | BP_PURGE_INTERIOR | BP_OPEN_INTERIOR), {
		{0,         DOOR,       DUNGEON,        {1,1},		1,			0,			-1,			0,				0,				1,				0,			0,			(MF_BUILD_AT_ORIGIN)},
		{0,         0,          0,              {1,1},		1,			0,			-1,			0,				MK_MIRRORED_TOTEM,3,			0,			0,			(MF_GENERATE_MONSTER | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY | MF_IN_VIEW_OF_ORIGIN | MF_FAR_FROM_ORIGIN)},
		{0,         MACHINE_GLYPH,DUNGEON,      {1,1},		0,			0,			-1,			0,				0,				1,				0,			0,			(MF_NEAR_ORIGIN | MF_EVERYWHERE)},
		{0,         MACHINE_GLYPH,DUNGEON,      {3,5},      2,          0,          -1,         0,              0,              2,              0,          0,          (MF_TREAT_AS_BLOCKING)}}},
	// Guardian obstacle -- a guardian is in the door on a glyph, with other glyphs scattered around.
	{{6, AMULET_LEVEL}, {25, 25},	10,		4,          (BP_VESTIBULE | BP_OPEN_INTERIOR),	{
		{0,			DOOR,       DUNGEON,        {1,1},		1,			0,			0,			0,				MK_GUARDIAN,	2,				0,			0,			(MF_GENERATE_MONSTER | MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_ALTERNATIVE)},
		{0,			DOOR,       DUNGEON,        {1,1},		1,			0,			0,			0,				MK_WINGED_GUARDIAN,2,           0,			0,			(MF_GENERATE_MONSTER | MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_ALTERNATIVE)},
		{0,         MACHINE_GLYPH,DUNGEON,      {10,10},    3,          0,          -1,         0,              0,              1,              0,          0,          (MF_PERMIT_BLOCKING| MF_NEAR_ORIGIN)},
		{0,         MACHINE_GLYPH,DUNGEON,      {1,1},      0,          0,          -1,         0,              0,              2,              0,          0,          (MF_EVERYWHERE | MF_PERMIT_BLOCKING | MF_NOT_IN_HALLWAY)}}},
	
	// -- KEY HOLDERS --
	
	// Nested item library -- can check one item out at a time, and one is a disposable key to another reward room
	{{1, AMULET_LEVEL},	{30, 50},	35,		7,			(BP_ROOM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS | BP_OPEN_INTERIOR | BP_ADOPT_ITEM | BP_IMPREGNABLE),	{
		{0,			CARPET,		DUNGEON,		{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)},
		{0,			WALL,       DUNGEON,		{0,0},      0,			0,			-1,			0,				0,				0,				0,          0,          (MF_TREAT_AS_BLOCKING | MF_BUILD_IN_WALLS | MF_IMPREGNABLE | MF_EVERYWHERE)},
		{0,			0,          0,              {1,1},		1,			0,          0,          0,				0,				2,				0,			0,          (MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING | MF_BUILD_VESTIBULE)},
		{0,			ALTAR_CAGE_OPEN,DUNGEON,	{1,2},		1,			(WEAPON|ARMOR|WAND),-1,	0,				0,				2,				0,			(ITEM_IS_KEY | ITEM_KIND_AUTO_ID | ITEM_PLAYER_AVOIDS),	(MF_GENERATE_ITEM | MF_NO_THROWING_WEAPONS | MF_TREAT_AS_BLOCKING | MF_IMPREGNABLE)},
		{0,			ALTAR_CAGE_OPEN,DUNGEON,	{1,2},		1,			(STAFF|RING|CHARM),-1,	0,				0,				2,				0,			(ITEM_IS_KEY | ITEM_KIND_AUTO_ID | ITEM_MAX_CHARGES_KNOWN | ITEM_PLAYER_AVOIDS),	(MF_GENERATE_ITEM | MF_NO_THROWING_WEAPONS | MF_TREAT_AS_BLOCKING | MF_IMPREGNABLE)},
		{0,			ALTAR_CAGE_OPEN,DUNGEON,	{1,1},		1,			0,			-1,			0,				0,				2,				0,			(ITEM_IS_KEY | ITEM_PLAYER_AVOIDS | ITEM_MAX_CHARGES_KNOWN),	(MF_ADOPT_ITEM | MF_TREAT_AS_BLOCKING | MF_IMPREGNABLE)},
		{0,			STATUE_INERT,DUNGEON,		{1,3},		0,			0,			-1,			0,				0,				2,				0,          0,          (MF_TREAT_AS_BLOCKING | MF_BUILD_IN_WALLS | MF_IMPREGNABLE)}}},
	// Secret room -- key on an altar in a secret room
	{{1, AMULET_LEVEL},	{15, 100},	1,		2,			(BP_ROOM | BP_ADOPT_ITEM), {
		{0,			ALTAR_INERT,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				1,				0,			ITEM_PLAYER_AVOIDS,	(MF_ADOPT_ITEM | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)},
		{0,			SECRET_DOOR,DUNGEON,		{1,1},		1,			0,			0,			0,				0,				1,				0,			0,			(MF_PERMIT_BLOCKING | MF_BUILD_AT_ORIGIN)}}},
	// Throwing tutorial -- toss an item onto the pressure plate to retract the cage and reveal the key
	{{1, 4},			{70, 120},	10,		2,			(BP_ADOPT_ITEM), {
		{0,			ALTAR_CAGE_RETRACTABLE,DUNGEON,{1,1},	1,			0,			-1,			0,				0,				3,				0,			0,			(MF_ADOPT_ITEM | MF_IMPREGNABLE | MF_NOT_IN_HALLWAY)},
		{DF_MEDIUM_HOLE, MACHINE_PRESSURE_PLATE, LIQUID, {1,1}, 1,		0,			0,			0,				0,				1,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)}}},
	// Rat trap -- getting the key triggers paralysis vents nearby and also causes rats to burst out of the walls
	{{1,8},             {30, 70},	7,		3,          (BP_ADOPT_ITEM | BP_ROOM),	{
		{0,			ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				1,				0,			0,			(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)},
		{0,			MACHINE_PARALYSIS_VENT_HIDDEN,DUNGEON,{1,1},1,		0,			-1,			0,				0,				2,				0,			0,			(MF_FAR_FROM_ORIGIN | MF_NOT_IN_HALLWAY)},
		{0,			RAT_TRAP_WALL_DORMANT,DUNGEON,{10,20},	5,			0,			-1,			0,				MK_RAT,         1,				0,			0,			(MF_GENERATE_MONSTER | MF_MONSTERS_DORMANT | MF_BUILD_IN_WALLS | MF_NOT_ON_LEVEL_PERIMETER)}}},
	// Fun with fire -- trigger the fire trap and coax the fire over to the wooden barricade surrounding the altar and key
	{{3, 10},			{80, 100},	10,		6,			(BP_ROOM | BP_ADOPT_ITEM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS | BP_OPEN_INTERIOR), {
		{DF_SURROUND_WOODEN_BARRICADE,ALTAR_INERT,DUNGEON,{1,1},1,		0,			-1,			0,				0,				3,				0,			0,			(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING)},
		{0,			GRASS,		SURFACE,		{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE | MF_ALTERNATIVE)},
		{DF_SWAMP,	0,			0,				{4,4},		2,			0,			-1,			0,				0,				2,				0,			0,			(MF_ALTERNATIVE | MF_FAR_FROM_ORIGIN)},
		{0,			FLAMETHROWER_HIDDEN,DUNGEON,{1,1},		1,			0,			0,			0,				0,				1,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_NEAR_ORIGIN)},
		{0,			GAS_TRAP_POISON_HIDDEN,DUNGEON,{3, 3},	1,			0,			-1,			0,				0,				5,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_ALTERNATIVE)},
		{0,			0,			0,				{2,2},		1,			POTION,		POTION_LICHEN,0,			0,				3,				0,			0,			(MF_GENERATE_ITEM | MF_BUILD_ANYWHERE_ON_LEVEL | MF_NOT_IN_HALLWAY | MF_ALTERNATIVE)}}},
	// Flood room -- key on an altar in a room with pools of eel-infested waters; take key to flood room with shallow water
	{{3, AMULET_LEVEL},	{80, 180},	10,		4,			(BP_ROOM | BP_SURROUND_WITH_WALLS | BP_PURGE_LIQUIDS | BP_PURGE_PATHING_BLOCKERS | BP_ADOPT_ITEM),	{
		{0,			FLOOR_FLOODABLE,LIQUID,		{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)},
		{0,			ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				5,				0,			0,			(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)},
		{DF_SPREADABLE_WATER_POOL,0,0,          {2, 4},		1,			0,			-1,			0,				0,				5,				HORDE_MACHINE_WATER_MONSTER,0,MF_GENERATE_HORDE},
		{DF_GRASS,	FOLIAGE,	SURFACE,		{3, 4},		3,			0,			-1,			0,				0,				1,				0,			0,			0}}},
	// Fire trap room -- key on an altar, pools of water, fire traps all over the place.
	{{4, AMULET_LEVEL},	{80, 180},	10,		5,			(BP_ROOM | BP_SURROUND_WITH_WALLS | BP_PURGE_LIQUIDS | BP_PURGE_PATHING_BLOCKERS | BP_ADOPT_ITEM),	{
		{0,			ALTAR_INERT,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				1,				0,			0,			(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)},
		{0,         0,          0,              {1, 1},     1,          0,          -1,         0,              0,              4,              0,          0,          MF_BUILD_AT_ORIGIN},
		{0,         FLAMETHROWER_HIDDEN,DUNGEON,{40, 60},   20,         0,          -1,         0,              0,              1,              0,          0,          (MF_TREAT_AS_BLOCKING)},
		{DF_WATER_POOL,0,0,                     {4, 4},		1,			0,			-1,			0,				0,				4,				HORDE_MACHINE_WATER_MONSTER,0,MF_GENERATE_HORDE},
		{DF_GRASS,	FOLIAGE,	SURFACE,		{3, 4},		3,			0,			-1,			0,				0,				1,				0,			0,			0}}},
	// Thief area -- empty altar, monster with item, permanently fleeing.
	{{3, AMULET_LEVEL},	{15, 20},	10,		2,			(BP_ADOPT_ITEM),	{
		{DF_LUMINESCENT_FUNGUS,	ALTAR_INERT,DUNGEON,{1,1},	1,			0,			-1,			0,				0,				2,				HORDE_MACHINE_THIEF,0,			(MF_ADOPT_ITEM | MF_BUILD_AT_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY | MF_GENERATE_HORDE | MF_MONSTER_TAKE_ITEM | MF_MONSTER_FLEEING)},
		{0,         STATUE_INERT,0,             {3, 5},     2,          0,          -1,         0,              0,              2,              0,          0,          (MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)}}},
	// Collapsing floor area -- key on an altar in an area; take key to cause the floor of the area to collapse
	{{1, AMULET_LEVEL},	{45, 65},	13,		3,			(BP_ADOPT_ITEM | BP_TREAT_AS_BLOCKING),	{
		{0,			FLOOR_FLOODABLE,DUNGEON,	{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)},
		{0,			ALTAR_SWITCH_RETRACTING,DUNGEON,{1,1},	1,			0,			-1,			0,				0,				3,				0,			0,			(MF_ADOPT_ITEM | MF_NEAR_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)},
		{DF_ADD_MACHINE_COLLAPSE_EDGE_DORMANT,0,0,{3, 3},	2,			0,			-1,			0,				0,				3,				0,			0,			(MF_FAR_FROM_ORIGIN | MF_NOT_IN_HALLWAY)}}},
	// Pit traps -- key on an altar, room full of pit traps
	{{1, AMULET_LEVEL},	{30, 100},	10,		3,			(BP_ROOM | BP_ADOPT_ITEM),	{
		{0,			ALTAR_INERT,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				2,				0,			0,			(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING)},
		{0,			TRAP_DOOR_HIDDEN,DUNGEON,	{30, 40},	1,			0,			-1,			0,				0,				1,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_REPEAT_UNTIL_NO_PROGRESS)},
		{0,			SECRET_DOOR,DUNGEON,		{1,1},		1,			0,			0,			0,				0,				1,				0,			0,			(MF_PERMIT_BLOCKING | MF_BUILD_AT_ORIGIN)}}},
	// Levitation challenge -- key on an altar, room filled with pit, levitation or lever elsewhere on level, bridge appears when you grab the key/lever.
	{{1, 13},			{75, 120},	10,		9,			(BP_ROOM | BP_ADOPT_ITEM | BP_PURGE_INTERIOR | BP_OPEN_INTERIOR | BP_SURROUND_WITH_WALLS),	{
		{0,			ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				3,				0,			0,			(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING)},
		{0,			TORCH_WALL,	DUNGEON,		{1,4},		0,			0,			0,			0,				0,				1,				0,			0,			(MF_BUILD_IN_WALLS)},
		{0,			0,			0,				{1,1},		1,			0,			0,			0,				0,				3,				0,			0,			MF_BUILD_AT_ORIGIN},
		{DF_ADD_DORMANT_CHASM_HALO,	CHASM,LIQUID,{120, 120},1,			0,			-1,			0,				0,				1,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_REPEAT_UNTIL_NO_PROGRESS)},
		{DF_ADD_DORMANT_CHASM_HALO,	CHASM_WITH_HIDDEN_BRIDGE,LIQUID,{1,1},1,0,		0,			0,				0,				1,				0,			0,			(MF_PERMIT_BLOCKING | MF_EVERYWHERE)},
		{0,			0,			0,				{1,1},		1,			POTION,		POTION_LEVITATION,0,		0,				1,				0,			0,			(MF_GENERATE_ITEM | MF_BUILD_ANYWHERE_ON_LEVEL | MF_ALTERNATIVE)},
		{0,			WALL_LEVER_HIDDEN,DUNGEON,  {1,1},      1,			0,			-1,			0,				0,				1,				0,			0,			(MF_BUILD_IN_WALLS | MF_IN_PASSABLE_VIEW_OF_ORIGIN | MF_BUILD_ANYWHERE_ON_LEVEL | MF_ALTERNATIVE)}}},
	// Web climbing -- key on an altar, room filled with pit, spider at altar to shoot webs, bridge appears when you grab the key
	{{7, AMULET_LEVEL},	{55, 90},	10,		7,			(BP_ROOM | BP_ADOPT_ITEM | BP_PURGE_INTERIOR | BP_OPEN_INTERIOR | BP_SURROUND_WITH_WALLS),	{
		{0,			ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				MK_SPIDER,		3,				0,			0,			(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING | MF_IN_VIEW_OF_ORIGIN | MF_GENERATE_MONSTER)},
		{0,			TORCH_WALL,	DUNGEON,		{1,4},		0,			0,			0,			0,				0,				1,				0,			0,			(MF_BUILD_IN_WALLS)},
		{0,			0,			0,				{1,1},		1,			0,			0,			0,				0,				3,				0,			0,			MF_BUILD_AT_ORIGIN},
		{DF_ADD_DORMANT_CHASM_HALO,	CHASM,LIQUID,	{120, 120},	1,		0,			-1,			0,				0,				1,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_REPEAT_UNTIL_NO_PROGRESS)},
		{DF_ADD_DORMANT_CHASM_HALO,	CHASM_WITH_HIDDEN_BRIDGE,LIQUID,{1,1},1,0,		0,			0,				0,				1,				0,			0,			(MF_PERMIT_BLOCKING | MF_EVERYWHERE)}}},
	// Lava moat room -- key on an altar, room filled with lava, levitation/fire immunity/lever elsewhere on level, lava retracts when you grab the key/lever
	{{3, 13},			{75, 120},	7,		7,			(BP_ROOM | BP_ADOPT_ITEM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS | BP_OPEN_INTERIOR),	{
		{0,			ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				2,				0,			0,			(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING)},
		{0,			0,			0,				{1,1},		1,			0,			0,			0,				0,				2,				0,			0,			(MF_BUILD_AT_ORIGIN)},
		{0,			LAVA,       LIQUID,         {60,60},	1,			0,			0,			0,				0,				1,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_REPEAT_UNTIL_NO_PROGRESS)},
		{DF_LAVA_RETRACTABLE, 0, 0,             {1,1},		1,			0,			0,			0,				0,				1,				0,			0,			(MF_PERMIT_BLOCKING | MF_EVERYWHERE)},
		{0,			0,			0,				{1,1},		1,			POTION,		POTION_LEVITATION,0,		0,				1,				0,			0,			(MF_GENERATE_ITEM | MF_BUILD_ANYWHERE_ON_LEVEL | MF_NOT_IN_HALLWAY | MF_ALTERNATIVE)},
		{0,			0,			0,				{1,1},		1,			POTION,		POTION_FIRE_IMMUNITY,0,		0,				1,				0,			0,			(MF_GENERATE_ITEM | MF_BUILD_ANYWHERE_ON_LEVEL | MF_NOT_IN_HALLWAY | MF_ALTERNATIVE)},
		{0,			WALL_LEVER_HIDDEN,DUNGEON,  {1,1},      1,			0,			-1,			0,				0,				1,				0,			0,			(MF_BUILD_IN_WALLS | MF_IN_PASSABLE_VIEW_OF_ORIGIN | MF_BUILD_ANYWHERE_ON_LEVEL | MF_ALTERNATIVE)}}},
	// Lava moat area -- key on an altar, surrounded with lava, levitation/fire immunity elsewhere on level, lava retracts when you grab the key
	{{3, 13},			{40, 60},	3,		5,			(BP_ADOPT_ITEM | BP_PURGE_INTERIOR | BP_OPEN_INTERIOR | BP_TREAT_AS_BLOCKING),	{
		{0,			ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				2,				0,			0,			(MF_ADOPT_ITEM | MF_BUILD_AT_ORIGIN | MF_TREAT_AS_BLOCKING)},
		{0,			LAVA,       LIQUID,         {60,60},	1,			0,			0,			0,				0,				1,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_REPEAT_UNTIL_NO_PROGRESS)},
		{DF_LAVA_RETRACTABLE, 0, 0,             {1,1},		1,			0,			0,			0,				0,				1,				0,			0,			(MF_PERMIT_BLOCKING | MF_EVERYWHERE)},
		{0,			0,			0,				{1,1},		1,			POTION,		POTION_LEVITATION,0,		0,				1,				0,			0,			(MF_GENERATE_ITEM | MF_BUILD_ANYWHERE_ON_LEVEL | MF_NOT_IN_HALLWAY | MF_ALTERNATIVE)},
		{0,			0,			0,				{1,1},		1,			POTION,		POTION_FIRE_IMMUNITY,0,		0,				1,				0,			0,			(MF_GENERATE_ITEM | MF_BUILD_ANYWHERE_ON_LEVEL | MF_NOT_IN_HALLWAY | MF_ALTERNATIVE)}}},
	// Poison gas -- key on an altar; take key to cause a poison gas vent to appear and the door to be blocked; there is a hidden trapdoor or an escape item somewhere inside
	{{4, AMULET_LEVEL},	{35, 60},	7,		7,			(BP_ROOM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS | BP_ADOPT_ITEM),	{
		{0,			ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				2,				0,			0,			(MF_ADOPT_ITEM | MF_TREAT_AS_BLOCKING)},
		{0,			MACHINE_POISON_GAS_VENT_HIDDEN,DUNGEON,{1,2}, 1,	0,			-1,			0,				0,				2,				0,			0,			0},
		{0,			TRAP_DOOR_HIDDEN,DUNGEON,	{1,1},		1,			0,			-1,			0,				0,				2,				0,			0,			MF_ALTERNATIVE},
		{0,			0,			0,				{1,1},		1,			SCROLL,		SCROLL_TELEPORT,0,			0,				2,				0,			0,			(MF_GENERATE_ITEM | MF_ALTERNATIVE)},
		{0,			0,			0,				{1,1},		1,			POTION,		POTION_DESCENT,0,			0,				2,				0,			0,			(MF_GENERATE_ITEM | MF_ALTERNATIVE)},
		{0,			WALL_LEVER_HIDDEN_DORMANT,DUNGEON,{1,1},1,			0,			-1,			0,				0,				1,				0,			0,			(MF_BUILD_IN_WALLS)},
		{0,			PORTCULLIS_DORMANT,DUNGEON,{1,1},       1,          0,			0,			0,              0,				1,				0,			0,			(MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING)}}},
	// Explosive situation -- key on an altar; take key to cause a methane gas vent to appear and a pilot light to ignite
	{{7, AMULET_LEVEL},	{80, 90},	10,		5,			(BP_ROOM | BP_PURGE_LIQUIDS | BP_SURROUND_WITH_WALLS | BP_ADOPT_ITEM),	{
		{0,			DOOR,       DUNGEON,		{1,1},		1,			0,			0,			0,				0,				1,				0,			0,			(MF_BUILD_AT_ORIGIN)},
		{0,			FLOOR,		DUNGEON,		{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)},
		{0,			ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				1,				0,			0,			(MF_ADOPT_ITEM | MF_TREAT_AS_BLOCKING | MF_FAR_FROM_ORIGIN)},
		{0,			MACHINE_METHANE_VENT_HIDDEN,DUNGEON,{1,1}, 1,		0,			-1,			0,				0,				1,				0,			0,			MF_NEAR_ORIGIN},
		{0,			PILOT_LIGHT_DORMANT,DUNGEON,{1,1},		1,			0,			-1,			0,				0,				1,				0,			0,			(MF_FAR_FROM_ORIGIN | MF_BUILD_IN_WALLS)}}},
	// Burning grass -- key on an altar; take key to cause pilot light to ignite grass in room
	{{1, 7},			{40, 110},	10,		6,			(BP_ROOM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS | BP_ADOPT_ITEM | BP_OPEN_INTERIOR),	{
		{DF_SMALL_DEAD_GRASS,ALTAR_SWITCH_RETRACTING,DUNGEON,{1,1},1,	0,			-1,			0,				0,				1,				0,			0,			(MF_ADOPT_ITEM | MF_TREAT_AS_BLOCKING | MF_FAR_FROM_ORIGIN)},
		{DF_DEAD_FOLIAGE,0,		SURFACE,		{2,3},		0,			0,			-1,			0,				0,				1,				0,			0,			0},
		{0,			FOLIAGE,	SURFACE,		{1,4},		0,			0,			-1,			0,				0,				1,				0,			0,			0},
		{0,			GRASS,		SURFACE,		{10,25},	0,			0,			-1,			0,				0,				1,				0,			0,			0},
		{0,			DEAD_GRASS,	SURFACE,		{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)},
		{0,			PILOT_LIGHT_DORMANT,DUNGEON,{1,1},		1,			0,			-1,			0,				0,				1,				0,			0,			MF_NEAR_ORIGIN | MF_BUILD_IN_WALLS}}},
	// Statuary -- key on an altar, area full of statues; take key to cause statues to burst and reveal monsters
	{{10, AMULET_LEVEL},{35, 90},	10,		2,			(BP_ADOPT_ITEM | BP_NO_INTERIOR_FLAG),	{
		{0,			ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				2,				0,			0,			(MF_ADOPT_ITEM | MF_NEAR_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)},
		{0,			STATUE_DORMANT,DUNGEON,		{3,5},		3,			0,			-1,			0,				0,				2,				HORDE_MACHINE_STATUE,0,	(MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY | MF_GENERATE_HORDE | MF_MONSTERS_DORMANT | MF_FAR_FROM_ORIGIN)}}},
	// Guardian water puzzle -- key held by a guardian, flood trap in the room, glyphs scattered. Lure the guardian into the water to have him drop the key.
	{{4, AMULET_LEVEL}, {35, 70},	8,		4,			(BP_ROOM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS | BP_ADOPT_ITEM),	{
		{0,         0,          0,              {1,1},      1,          0,          -1,         0,              0,              2,              0,          0,          (MF_BUILD_AT_ORIGIN)},
		{0,			0,          0,              {1,1},		1,			0,			-1,			0,				MK_GUARDIAN,	2,				0,			0,			(MF_GENERATE_MONSTER | MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY | MF_MONSTER_TAKE_ITEM)},
		{0,			FLOOD_TRAP,DUNGEON,         {1,1},		1,			0,			-1,			0,				0,				2,				0,          0,          (MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)},
		{0,         MACHINE_GLYPH,DUNGEON,      {1,1},      4,          0,          -1,         0,              0,              2,              0,          0,          (MF_EVERYWHERE | MF_NOT_IN_HALLWAY)}}},
	// Guardian gauntlet -- key in a room full of guardians, glyphs scattered and unavoidable.
	{{6, AMULET_LEVEL}, {50, 95},	10,		6,			(BP_ROOM | BP_ADOPT_ITEM),	{
		{DF_GLYPH_CIRCLE,ALTAR_INERT,DUNGEON,	{1,1},		1,			0,			-1,			0,				0,				1,				0,			0,			(MF_ADOPT_ITEM | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY | MF_FAR_FROM_ORIGIN)},
		{0,			DOOR,       DUNGEON,        {1,1},		1,			0,			0,			0,				0,				3,				0,			0,			(MF_PERMIT_BLOCKING | MF_BUILD_AT_ORIGIN)},
		{0,			0,          0,              {3,6},		3,			0,			-1,			0,				MK_GUARDIAN,	2,				0,			0,			(MF_GENERATE_MONSTER | MF_NEAR_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY | MF_ALTERNATIVE)},
		{0,			0,          0,              {1,2},		1,			0,			-1,			0,				MK_WINGED_GUARDIAN,2,           0,			0,			(MF_GENERATE_MONSTER | MF_NEAR_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY | MF_ALTERNATIVE)},
		{0,         MACHINE_GLYPH,DUNGEON,      {10,15},   10,          0,          -1,         0,              0,              1,              0,          0,          (MF_PERMIT_BLOCKING | MF_NOT_IN_HALLWAY)},
		{0,         MACHINE_GLYPH,DUNGEON,      {1,1},      0,          0,          -1,         0,              0,              2,              0,          0,          (MF_EVERYWHERE | MF_PERMIT_BLOCKING | MF_NOT_IN_HALLWAY)}}},
	// Guardian corridor -- key in a small room, with a connecting corridor full of glyphs, one guardian blocking the corridor.
	{{4, AMULET_LEVEL}, {85, 100},   5,     7,          (BP_ROOM | BP_ADOPT_ITEM | BP_PURGE_INTERIOR | BP_OPEN_INTERIOR | BP_SURROUND_WITH_WALLS),        {
		{DF_GLYPH_CIRCLE,ALTAR_INERT,DUNGEON,   {1,1},      1,          0,          -1,         0,              MK_GUARDIAN,    3,              0,          0,          (MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_GENERATE_MONSTER | MF_ALTERNATIVE)},
		{DF_GLYPH_CIRCLE,ALTAR_INERT,DUNGEON,   {1,1},      1,          0,          -1,         0,              MK_WINGED_GUARDIAN,3,           0,          0,          (MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_GENERATE_MONSTER | MF_ALTERNATIVE)},
		{0,         MACHINE_GLYPH,DUNGEON,      {3,5},      2,          0,          0,          0,              0,              2,              0,          0,          MF_NEAR_ORIGIN | MF_NOT_IN_HALLWAY},
		{0,         0,          0,              {1,1},      1,          0,          0,          0,              0,              3,              0,          0,          MF_BUILD_AT_ORIGIN},
		{0,         WALL,DUNGEON,           {80,80},    1,          0,          -1,         0,              0,              1,              0,          0,          (MF_TREAT_AS_BLOCKING | MF_REPEAT_UNTIL_NO_PROGRESS)},
		{0,         MACHINE_GLYPH,DUNGEON,      {1,1},      1,          0,          0,          0,              0,              1,              0,          0,          (MF_PERMIT_BLOCKING | MF_EVERYWHERE)},
		{0,			MACHINE_GLYPH,DUNGEON,      {1,1},      1,			0,			-1,			0,				0,				1,				0,			0,			(MF_IN_PASSABLE_VIEW_OF_ORIGIN | MF_NOT_IN_HALLWAY | MF_BUILD_ANYWHERE_ON_LEVEL)}}},
	// Summoning circle -- key in a room with an eldritch totem, glyphs unavoidable. // DISABLED. (Not fun enough.)
	{{12, AMULET_LEVEL}, {50, 100},	0,		2,			(BP_ROOM | BP_OPEN_INTERIOR | BP_ADOPT_ITEM),	{
		{DF_GLYPH_CIRCLE,ALTAR_INERT,DUNGEON,	{1,1},		1,			0,			-1,			0,				0,				3,				0,			0,			(MF_ADOPT_ITEM | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY | MF_FAR_FROM_ORIGIN)},
		{DF_GLYPH_CIRCLE,0,     0,              {1,1},		1,			0,			-1,			0,				MK_ELDRITCH_TOTEM,3,			0,			0,			(MF_GENERATE_MONSTER | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)}}},
	// Beckoning obstacle -- key surrounded by glyphs in a room with a mirrored totem.
	{{5, AMULET_LEVEL}, {60, 100},	10,		4,			(BP_ROOM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS | BP_OPEN_INTERIOR | BP_ADOPT_ITEM), {
		{DF_GLYPH_CIRCLE,ALTAR_INERT,DUNGEON,	{1,1},		1,			0,			-1,			0,				0,				3,				0,			0,			(MF_ADOPT_ITEM | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY | MF_FAR_FROM_ORIGIN | MF_IN_VIEW_OF_ORIGIN)},
		{0,         0,          0,              {1,1},		1,			0,			-1,			0,				MK_MIRRORED_TOTEM,3,			0,			0,			(MF_GENERATE_MONSTER | MF_NEAR_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY | MF_IN_VIEW_OF_ORIGIN)},
		{0,         0,          0,              {1,1},      1,          0,          -1,         0,              0,              2,              0,          0,          (MF_BUILD_AT_ORIGIN)},
		{0,         MACHINE_GLYPH,DUNGEON,      {3,5},      2,          0,          -1,         0,              0,              2,              0,          0,          (MF_TREAT_AS_BLOCKING)}}},
	// Worms in the walls -- key on altar; take key to cause underworms to burst out of the walls
	{{12,AMULET_LEVEL},	{7, 7},		7,		2,			(BP_ADOPT_ITEM),	{
		{0,			ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				2,				0,			0,			(MF_ADOPT_ITEM | MF_NEAR_ORIGIN | MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY)},
		{0,			WALL_MONSTER_DORMANT,DUNGEON,{5,8},		5,			0,			-1,			0,				MK_UNDERWORM,	1,				0,			0,			(MF_GENERATE_MONSTER | MF_MONSTERS_DORMANT | MF_BUILD_IN_WALLS | MF_NOT_ON_LEVEL_PERIMETER)}}},
	// Mud pit -- key on an altar, room full of mud, take key to cause bog monsters to spawn in the mud
	{{12, AMULET_LEVEL},{40, 90},	10,		3,			(BP_ROOM | BP_ADOPT_ITEM | BP_SURROUND_WITH_WALLS | BP_PURGE_LIQUIDS),	{
		{DF_SWAMP,		0,		0,				{5,5},		0,			0,			-1,			0,				0,				1,				0,			0,			0},
		{DF_SWAMP,	ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				2,				0,			0,			(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING)},
		{DF_MUD_DORMANT,0,		0,				{3,4},		3,			0,			-1,			0,				0,				1,				HORDE_MACHINE_MUD,0,	(MF_GENERATE_HORDE | MF_MONSTERS_DORMANT)}}},
	// Zombie crypt -- key on an altar; coffins scattered around; brazier in the room; take key to cause zombies to burst out of all of the coffins
	{{12, AMULET_LEVEL},{60, 90},	10,		8,			(BP_ROOM | BP_ADOPT_ITEM | BP_SURROUND_WITH_WALLS | BP_PURGE_INTERIOR),	{
		{0,         DOOR,       DUNGEON,        {1,1},		1,			0,			-1,			0,				0,				1,				0,			0,			(MF_BUILD_AT_ORIGIN)},
		{DF_BONES,  0,          0,              {3,4},      2,          0,          -1,         0,              0,              1,              0,          0,          0},
		{DF_ASH,    0,          0,              {3,4},      2,          0,          -1,         0,              0,              1,              0,          0,          0},
		{DF_AMBIENT_BLOOD,0,    0,              {1,2},		1,			0,			-1,			0,				0,				1,				0,			0,			0},
		{DF_AMBIENT_BLOOD,0,    0,              {1,2},		1,			0,			-1,			0,				0,				1,				0,			0,			0},
		{0,         ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				2,				0,			0,			(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING)},
		{0,         BRAZIER,    DUNGEON,        {1,1},      1,          0,          -1,         0,              0,              2,              0,          0,          (MF_NEAR_ORIGIN | MF_TREAT_AS_BLOCKING)},
		{0,         COFFIN_CLOSED, DUNGEON,		{6,8},		1,			0,          0,          0,				MK_ZOMBIE,		2,				0,			0,          (MF_TREAT_AS_BLOCKING | MF_NOT_IN_HALLWAY | MF_GENERATE_MONSTER | MF_MONSTERS_DORMANT)}}},
	// Haunted house -- key on an altar; take key to cause the room to darken, ectoplasm to cover everything and phantoms to appear
	{{16, AMULET_LEVEL},{45, 150},	10,		4,			(BP_ROOM | BP_ADOPT_ITEM | BP_PURGE_INTERIOR | BP_SURROUND_WITH_WALLS),	{
		{0,			ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				2,				0,			0,			(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING)},
		{0,			DARK_FLOOR_DORMANT,DUNGEON,	{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)},
		{0,			DARK_FLOOR_DORMANT,DUNGEON,	{4,5},		4,			0,			-1,			0,				MK_PHANTOM,		1,				0,			0,			(MF_GENERATE_MONSTER | MF_MONSTERS_DORMANT)},
		{0,         HAUNTED_TORCH_DORMANT,DUNGEON,{5,10},   3,          0,          -1,         0,              0,              2,              0,          0,          (MF_BUILD_IN_WALLS)}}},
	// Worm tunnels -- hidden lever causes tunnels to open up revealing worm areas and a key
	{{8, AMULET_LEVEL},{80, 175},	10,		6,			(BP_ROOM | BP_ADOPT_ITEM | BP_PURGE_INTERIOR | BP_MAXIMIZE_INTERIOR | BP_SURROUND_WITH_WALLS),	{        
		{0,			ALTAR_INERT,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				2,				0,			0,			(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_TREAT_AS_BLOCKING)},
		{0,			0,          0,              {3,6},		3,			0,			-1,			0,				MK_UNDERWORM,	1,				0,			0,			(MF_GENERATE_MONSTER)},
		{0,			GRANITE,    DUNGEON,        {150,150},  1,			0,			-1,			0,				0,				1,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_REPEAT_UNTIL_NO_PROGRESS)},
		{DF_WORM_TUNNEL_MARKER_DORMANT,GRANITE,DUNGEON,{0,0},0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE | MF_PERMIT_BLOCKING)},
		{DF_TUNNELIZE,WORM_TUNNEL_OUTER_WALL,DUNGEON,{1,1},	1,			0,			-1,			0,				0,				1,				0,			0,			(MF_BUILD_AT_ORIGIN | MF_PERMIT_BLOCKING)},
		{0,			WALL_LEVER_HIDDEN,DUNGEON,  {1,1},      1,			0,			-1,			0,				0,				1,				0,			0,			(MF_BUILD_IN_WALLS | MF_IN_PASSABLE_VIEW_OF_ORIGIN | MF_BUILD_ANYWHERE_ON_LEVEL)}}},
	// Gauntlet -- key on an altar; take key to cause turrets to emerge
	{{5, 24},			{35, 90},	10,		2,			(BP_ADOPT_ITEM | BP_NO_INTERIOR_FLAG),	{
		{0,			ALTAR_SWITCH,DUNGEON,		{1,1},		1,			0,			-1,			0,				0,				2,				0,			0,			(MF_ADOPT_ITEM | MF_NEAR_ORIGIN | MF_NOT_IN_HALLWAY | MF_TREAT_AS_BLOCKING)},
		{0,			TURRET_DORMANT,DUNGEON,		{4,6},		4,			0,			-1,			0,				0,				2,				HORDE_MACHINE_TURRET,0,	(MF_TREAT_AS_BLOCKING | MF_GENERATE_HORDE | MF_MONSTERS_DORMANT | MF_BUILD_IN_WALLS | MF_IN_VIEW_OF_ORIGIN)}}},
	// Boss -- key is held by a boss atop a pile of bones in a secret room. A few fungus patches light up the area.
	{{5, AMULET_LEVEL},	{40, 100},	18,		3,			(BP_ROOM | BP_ADOPT_ITEM | BP_SURROUND_WITH_WALLS | BP_PURGE_LIQUIDS), {
		{DF_BONES,	SECRET_DOOR,DUNGEON,		{1,1},		1,			0,			0,			0,				0,				3,				0,			0,			(MF_PERMIT_BLOCKING | MF_BUILD_AT_ORIGIN)},
		{DF_LUMINESCENT_FUNGUS,	STATUE_INERT,DUNGEON,{7,7},	0,			0,			-1,			0,				0,				2,				0,			0,			(MF_TREAT_AS_BLOCKING)},
		{DF_BONES,	0,			0,				{1,1},		1,			0,			-1,			0,				0,				1,				HORDE_MACHINE_BOSS,	0,	(MF_ADOPT_ITEM | MF_FAR_FROM_ORIGIN | MF_MONSTER_TAKE_ITEM | MF_GENERATE_HORDE | MF_MONSTER_SLEEPING)}}},
	
	// -- FLAVOR MACHINES --
	
	// Bloodwort -- bloodwort stalk, some pods, and surrounding grass
	{{1,DEEPEST_LEVEL},	{5, 5},     0,          2,			(BP_TREAT_AS_BLOCKING), {
		{DF_GRASS,	BLOODFLOWER_STALK, SURFACE,	{1, 1},		1,			0,			-1,			0,				0,				0,				0,			0,			(MF_BUILD_AT_ORIGIN | MF_NOT_IN_HALLWAY)},
		{DF_BLOODFLOWER_PODS_GROW_INITIAL,0, 0, {1, 1},     1,			0,			-1,			0,				0,				1,				0,			0,          (MF_BUILD_AT_ORIGIN | MF_TREAT_AS_BLOCKING)}}},
	// Idyll -- ponds and some grass and forest
	{{1,DEEPEST_LEVEL},	{80, 120},	0,		2,			BP_NO_INTERIOR_FLAG, {
		{DF_GRASS,	FOLIAGE,	SURFACE,		{3, 4},		3,			0,			-1,			0,				0,				1,				0,			0,			0},
		{DF_WATER_POOL,	0,		0,				{2, 3},		2,			0,			-1,			0,				0,				5,				0,			0,			(MF_NOT_IN_HALLWAY)}}},
	// Swamp -- mud, grass and some shallow water
	{{1,DEEPEST_LEVEL},	{50, 65},	0,		2,			BP_NO_INTERIOR_FLAG, {
		{DF_SWAMP,	0,			0,				{6, 8},		3,			0,			-1,			0,				0,				1,				0,			0,			0},
		{DF_WATER_POOL,	0,		0,				{0, 1},		0,			0,			-1,			0,				0,				3,				0,			0,			(MF_NOT_IN_HALLWAY | MF_TREAT_AS_BLOCKING)}}},
	// Camp -- hay, junk, urine, vomit
	{{1,DEEPEST_LEVEL},	{40, 50},	0,		4,			BP_NO_INTERIOR_FLAG, {
		{DF_HAY,	0,			0,				{1, 3},		1,			0,			-1,			0,				0,				1,				0,			0,			(MF_NOT_IN_HALLWAY | MF_IN_VIEW_OF_ORIGIN)},
		{DF_JUNK,	0,			0,				{1, 2},		1,			0,			-1,			0,				0,				3,				0,			0,			(MF_NOT_IN_HALLWAY | MF_IN_VIEW_OF_ORIGIN)},
		{DF_URINE,	0,			0,				{3, 5},		1,			0,			-1,			0,				0,				1,				0,			0,			MF_IN_VIEW_OF_ORIGIN},
		{DF_VOMIT,	0,			0,				{0, 2},		0,			0,			-1,			0,				0,				1,				0,			0,			MF_IN_VIEW_OF_ORIGIN}}},
	// Remnant -- carpet surrounded by ash and with some statues
	{{1,DEEPEST_LEVEL},	{80, 120},	0,		2,			BP_NO_INTERIOR_FLAG, {
		{DF_REMNANT, 0,			0,				{6, 8},		3,			0,			-1,			0,				0,				1,				0,			0,			0},
		{0,			STATUE_INERT,DUNGEON,       {3, 5},		2,			0,			-1,			0,				0,				1,				0,			0,			(MF_NOT_IN_HALLWAY | MF_TREAT_AS_BLOCKING)}}},
	// Dismal -- blood, bones, charcoal, some rubble
	{{1,DEEPEST_LEVEL},	{60, 70},	0,		3,			BP_NO_INTERIOR_FLAG, {
		{DF_AMBIENT_BLOOD, 0,	0,				{5,10},		3,			0,			-1,			0,				0,				1,				0,			0,			MF_NOT_IN_HALLWAY},
		{DF_ASH,	0,			0,				{4, 8},		2,			0,			-1,			0,				0,				1,				0,			0,			MF_NOT_IN_HALLWAY},
		{DF_BONES,	0,			0,				{3, 5},		2,			0,			-1,			0,				0,				1,				0,			0,			MF_NOT_IN_HALLWAY}}},
	// Chasm catwalk -- narrow bridge over a chasm, possibly under fire from a turret or two
	{{1,DEEPEST_LEVEL-1},{40, 80},	0,		4,			(BP_REQUIRE_BLOCKING | BP_OPEN_INTERIOR | BP_NO_INTERIOR_FLAG), {
		{DF_CHASM_HOLE,	0,		0,				{80, 80},	1,			0,			-1,			0,				0,				1,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_REPEAT_UNTIL_NO_PROGRESS)},
		{DF_CATWALK_BRIDGE,0,	0,				{0,0},		0,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)},
		{0,			MACHINE_TRIGGER_FLOOR, DUNGEON, {0,1},	0,			0,			0,			0,				0,				1,				0,			0,			(MF_NEAR_ORIGIN | MF_PERMIT_BLOCKING)},
		{0,			TURRET_DORMANT,DUNGEON,		{1, 2},		1,			0,			-1,			0,				0,				2,				HORDE_MACHINE_TURRET,0,	(MF_TREAT_AS_BLOCKING | MF_GENERATE_HORDE | MF_MONSTERS_DORMANT | MF_BUILD_IN_WALLS | MF_IN_VIEW_OF_ORIGIN)}}},
	// Lake walk -- narrow bridge of shallow water through a lake, possibly under fire from a turret or two
	{{1,DEEPEST_LEVEL},	{40, 80},	0,		3,			(BP_REQUIRE_BLOCKING | BP_OPEN_INTERIOR | BP_NO_INTERIOR_FLAG), {
		{DF_LAKE_CELL,	0,		0,				{80, 80},	1,			0,			-1,			0,				0,				1,				0,			0,			(MF_TREAT_AS_BLOCKING | MF_REPEAT_UNTIL_NO_PROGRESS)},
		{0,			MACHINE_TRIGGER_FLOOR, DUNGEON, {0,1},	0,			0,			0,			0,				0,				1,				0,			0,			(MF_NEAR_ORIGIN | MF_PERMIT_BLOCKING)},
		{0,			TURRET_DORMANT,DUNGEON,		{1, 2},		1,			0,			-1,			0,				0,				2,				HORDE_MACHINE_TURRET,0,	(MF_TREAT_AS_BLOCKING | MF_GENERATE_HORDE | MF_MONSTERS_DORMANT | MF_BUILD_IN_WALLS | MF_IN_VIEW_OF_ORIGIN)}}},
	// Paralysis trap -- hidden pressure plate with a few vents nearby.
	{{1,DEEPEST_LEVEL},	{35, 40},	0,		2,			(BP_NO_INTERIOR_FLAG), {
		{0,         GAS_TRAP_PARALYSIS_HIDDEN, DUNGEON, {1,2},1,0,		0,			0,			0,				3,				0,			0,			(MF_NEAR_ORIGIN | MF_NOT_IN_HALLWAY)},
		{0,			MACHINE_PARALYSIS_VENT_HIDDEN,DUNGEON,{3, 4},2,		0,			0,			0,				0,				3,				0,          0,          (MF_FAR_FROM_ORIGIN | MF_NOT_IN_HALLWAY)}}},
	// Statue comes alive -- innocent-looking statue that bursts to reveal a monster when the player approaches
	{{1,DEEPEST_LEVEL},	{5, 5},		0,		3,			(BP_NO_INTERIOR_FLAG), {
		{0,			STATUE_DORMANT,DUNGEON,		{1, 1},		1,			0,			-1,			0,				0,				1,				HORDE_MACHINE_STATUE,0,	(MF_GENERATE_HORDE | MF_MONSTERS_DORMANT | MF_BUILD_AT_ORIGIN | MF_ALTERNATIVE)},
		{0,			STATUE_DORMANT,DUNGEON,		{1, 1},		1,			0,			-1,			0,				0,				1,				HORDE_MACHINE_STATUE,0,	(MF_GENERATE_HORDE | MF_MONSTERS_DORMANT | MF_BUILD_IN_WALLS | MF_ALTERNATIVE | MF_NOT_ON_LEVEL_PERIMETER)},
		{0,			MACHINE_TRIGGER_FLOOR,DUNGEON,{0,0},	2,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)}}},
	// Worms in the walls -- step on trigger region to cause underworms to burst out of the walls
	{{1,DEEPEST_LEVEL},	{7, 7},		0,		2,			(BP_NO_INTERIOR_FLAG), {
		{0,			WALL_MONSTER_DORMANT,DUNGEON,{1, 3},	1,			0,			-1,			0,				MK_UNDERWORM,	1,				0,			0,			(MF_GENERATE_MONSTER | MF_MONSTERS_DORMANT | MF_BUILD_IN_WALLS | MF_NOT_ON_LEVEL_PERIMETER)},
		{0,			MACHINE_TRIGGER_FLOOR,DUNGEON,{0,0},	2,			0,			-1,			0,				0,				0,				0,			0,			(MF_EVERYWHERE)}}},	
	// Sentinels
	{{1,DEEPEST_LEVEL},	{40, 40},	0,		2,			(BP_NO_INTERIOR_FLAG), {
		{0,			STATUE_DORMANT,DUNGEON,		{3, 3},		3,			0,			-1,			0,				MK_SENTINEL,	2,				0,			0,			(MF_GENERATE_MONSTER | MF_NOT_IN_HALLWAY | MF_TREAT_AS_BLOCKING | MF_IN_VIEW_OF_ORIGIN)},
		{DF_ASH,	0,			0,				{2, 3},		0,			0,			-1,			0,				0,				0,				0,			0,			0}}},
};


#pragma mark Monster definitions

// Defines all creatures, which include monsters and the player:
creatureType monsterCatalog[NUMBER_MONSTER_KINDS] = {
	//	name			ch		color			HP		def		acc		damage			reg	sight	scent	move	attack	blood			light	DFChance DFType		behaviorF, abilityF
	{0,	"你",	PLAYER_CHAR,	&playerInLightColor,30,	0,		100,	{1, 2, 1},		20,	DCOLS,	30,		100,	100,	DF_RED_BLOOD,	0,		0,		0,
		(MONST_MALE | MONST_FEMALE)},
	
	{0, "老鼠",			'r',	&gray,			6,		0,		80,		{1, 3, 1},		20,	20,		30,		100,	100,	DF_RED_BLOOD,	0,		1,		DF_URINE},
	{0, "狗头人",		'k',	&goblinColor,	7,		0,		80,		{1, 4, 1},		20,	30,		30,		100,	100,	DF_RED_BLOOD,	0,		0,		0},
	{0,	"豺狼",		'j',	&jackalColor,	8,		0,		70,		{2, 4, 1},		20,	50,		50,		50,		100,	DF_RED_BLOOD,	0,		1,		DF_URINE},
	{0,	"电鳗",			'e',	&eelColor,		18,		27,		100,	{3, 7, 2},		5,	DCOLS,	20,		50,		100,	0,              0,		0,		0,
		(MONST_RESTRICTED_TO_LIQUID | MONST_IMMUNE_TO_WATER | MONST_SUBMERGES | MONST_FLITS | MONST_NEVER_SLEEPS | MONST_WILL_NOT_USE_STAIRS)},
	{0,	"猴子",		'm',	&ogreColor,		12,		17,		100,	{1, 3, 1},		20,	DCOLS,	100,	100,	100,	DF_RED_BLOOD,	0,		1,		DF_URINE,
		(0), (MA_HIT_STEAL_FLEE)},
	{0, "飞鱼",		'b',	&poisonGasColor,4,		0,		100,	{0, 0, 0},		5,	DCOLS,	100,	100,	100,	DF_PURPLE_BLOOD,0,		0,		DF_BLOAT_DEATH,
		(MONST_FLIES | MONST_FLITS), (MA_KAMIKAZE | MA_DF_ON_DEATH)},
	{0, "坑洞飞鱼",	'b',	&blue,          4,		0,		100,	{0, 0, 0},		5,	DCOLS,	100,	100,	100,	DF_PURPLE_BLOOD,0,		0,		DF_HOLE_POTION,
		(MONST_FLIES | MONST_FLITS), (MA_KAMIKAZE | MA_DF_ON_DEATH)},
	{0, "地精",		'g',	&goblinColor,	15,		10,		70,		{2, 5, 1},		20,	30,		20,		100,	100,	DF_RED_BLOOD,	0,		0,		0},
	{0, "地精巫师", 0xc2,	&goblinConjurerColor, 10,10,	70,		{2, 4, 1},		20,	30,		20,		100,	100,	DF_RED_BLOOD,	0,		0,		0,
		(MONST_MAINTAINS_DISTANCE | MONST_CAST_SPELLS_SLOWLY | MONST_CARRY_ITEM_25), (MA_CAST_SUMMON)},
	{0, "地精大法师", 0xc2,	&goblinMysticColor, 10,	10,		70,		{2, 4, 1},		20,	30,		20,		100,	100,	DF_RED_BLOOD,	0,		0,		0,
		(MONST_MAINTAINS_DISTANCE | MONST_CARRY_ITEM_25), (MA_CAST_PROTECTION)},
	{0, "地精图腾",	TOTEM_CHAR,	&orange,	30,		0,		0,		{0, 0, 0},		0,	DCOLS,	200,	100,	300,	DF_RUBBLE_BLOOD,IMP_LIGHT,0,	0,
		(MONST_IMMUNE_TO_WEBS | MONST_NEVER_SLEEPS | MONST_INTRINSIC_LIGHT | MONST_IMMOBILE | MONST_INANIMATE | MONST_ALWAYS_HUNTING | MONST_WILL_NOT_USE_STAIRS), (MA_CAST_HASTE | MA_CAST_SPARK)},
	{0, "史莱姆",	'J',	&pinkJellyColor,50,		0,		85,     {1, 3, 1},		0,	20,		20,		100,	100,	DF_PURPLE_BLOOD,0,		0,		0,
		(MONST_NEVER_SLEEPS), (MA_CLONE_SELF_ON_DEFEND)},
	{0, "蟾蜍",			't',	&toadColor,		18,		0,		90,		{1, 4, 1},		10,	15,		15,		100,	100,	DF_GREEN_BLOOD,	0,		0,		0,
		(0), (MA_HIT_HALLUCINATE)},
	{0, "吸血蝙蝠",	'v',	&gray,			18,		25,		100,	{2, 6, 1},		20,	DCOLS,	50,		50,		100,	DF_RED_BLOOD,	0,		0,		0,
		(MONST_FLIES | MONST_FLITS), (MA_TRANSFERENCE)},
	{0, "箭塔", TURRET_CHAR,&black,		30,		0,		90,		{2, 6, 1},		0,	DCOLS,	50,		100,	250,	0,              0,		0,		0,
		(MONST_TURRET), (MA_ATTACKS_FROM_DISTANCE)},
	{0, "粘液怪",	'a',	&acidBackColor,	15,		10,		70,		{1, 3, 1},		5,	15,		15,		100,	100,	DF_ACID_BLOOD,	0,		0,		0,
		(MONST_DEFEND_DEGRADE_WEAPON), (MA_HIT_DEGRADE_ARMOR)},
	{0, "蜈蚣",	'c',	&centipedeColor,20,		20,		80,		{4, 12, 1},		20,	20,		50,		100,	100,	DF_GREEN_BLOOD,	0,		0,		0,
		(0), (MA_CAUSES_WEAKNESS)},
	{0,	"食人魔",			'O',	&ogreColor,		55,		60,		125,	{9, 13, 2},		20,	30,		30,		100,	200,	DF_RED_BLOOD,	0,		0,		0,
		(MONST_MALE | MONST_FEMALE)},
	{0,	"沼泽怪",	'B',	&krakenColor,	55,		60,		5000,	{3, 4, 1},		3,	30,		30,		200,	100,	0,              0,		0,		0,
		(MONST_RESTRICTED_TO_LIQUID | MONST_SUBMERGES | MONST_FLITS | MONST_FLEES_NEAR_DEATH | MONST_WILL_NOT_USE_STAIRS), (MA_SEIZES)},
	{0, "食人魔图腾",	TOTEM_CHAR,	&green,		70,		0,		0,		{0, 0, 0},		0,	DCOLS,	200,	100,	400,	DF_RUBBLE_BLOOD,LICH_LIGHT,0,	0,
		(MONST_IMMUNE_TO_WEBS | MONST_NEVER_SLEEPS | MONST_INTRINSIC_LIGHT | MONST_IMMOBILE | MONST_INANIMATE | MONST_ALWAYS_HUNTING | MONST_WILL_NOT_USE_STAIRS), (MA_CAST_HEAL | MA_CAST_SLOW)},
	{0, "蜘蛛",		's',	&white,			20,		70,		90,		{6, 11, 2},		20,	50,		20,		100,	100,	DF_GREEN_BLOOD,	0,		0,		0,
		(MONST_IMMUNE_TO_WEBS | MONST_CAST_SPELLS_SLOWLY), (MA_SHOOTS_WEBS | MA_POISONS)},
	{0, "闪电塔", TURRET_CHAR, &lightningColor,80,0,		100,	{0, 0, 0},		0,	DCOLS,	50,		100,	150,	0,              SPARK_TURRET_LIGHT,	0,	0,
		(MONST_TURRET | MONST_INTRINSIC_LIGHT), (MA_CAST_SPARK)},
	{0,	"火焰精灵",'w',	&wispLightColor,10,		90,     100,	{5,	8, 2},		5,	90,		15,		100,	100,	DF_ASH_BLOOD,	WISP_LIGHT,	0,	0,
		(MONST_IMMUNE_TO_FIRE | MONST_FLIES | MONST_FLITS | MONST_NEVER_SLEEPS | MONST_FIERY | MONST_INTRINSIC_LIGHT | MONST_DIES_IF_NEGATED)},
	{0, "怨灵",		'W',	&wraithColor,	50,		60,		120,	{6, 13, 2},		5,	DCOLS,	100,	50,		100,	DF_GREEN_BLOOD,	0,		0,		0,
		(MONST_FLEES_NEAR_DEATH)},
	{0, "僵尸",		'Z',	&vomitColor,	80,		0,		120,	{7, 12, 1},		0,	50,		200,	100,	100,	DF_ROT_GAS_BLOOD,0,		100,	DF_ROT_GAS_PUFF, (0)},
	{0, "巨魔",		'T',	&trollColor,	65,		70,		125,	{10, 15, 3},	1,	DCOLS,	20,		100,	100,	DF_RED_BLOOD,	0,		0,		0,
		(MONST_MALE | MONST_FEMALE)},
	{0,	"食人魔萨满",	0xc4,	&green,			45,		40,		100,	{5, 9, 1},		20,	DCOLS,	30,		100,	200,	DF_RED_BLOOD,	0,		0,		0,
		(MONST_MAINTAINS_DISTANCE | MONST_CAST_SPELLS_SLOWLY | MONST_MALE | MONST_FEMALE), (MA_CAST_HASTE | MA_CAST_SPARK | MA_CAST_SUMMON)},
	{0, "娜迦",			'N',	&trollColor,	75,		70,     150,	{7, 11, 4},		10,	DCOLS,	100,	100,	100,	DF_GREEN_BLOOD,	0,		100,	DF_PUDDLE,
		(MONST_IMMUNE_TO_WATER | MONST_SUBMERGES | MONST_NEVER_SLEEPS | MONST_FEMALE)},
	{0, "火焰蜥蜴",	'S',	&salamanderColor,60,	70,     150,	{7, 13, 3},		10,	DCOLS,	100,	100,	100,	DF_ASH_BLOOD,	SALAMANDER_LIGHT, 100, DF_SALAMANDER_FLAME,
		(MONST_IMMUNE_TO_FIRE | MONST_SUBMERGES | MONST_NEVER_SLEEPS | MONST_FIERY | MONST_INTRINSIC_LIGHT | MONST_MALE)},
	{0, "火焰飞鱼",'b',	&orange,		10,		0,		100,	{0, 0, 0},		5,	DCOLS,	100,	100,	100,	DF_RED_BLOOD,	EMBER_LIGHT,0,	DF_BLOAT_EXPLOSION,
		(MONST_FLIES | MONST_FLITS| MONST_INTRINSIC_LIGHT), (MA_KAMIKAZE | MA_DF_ON_DEATH)},
	{0, "地底精灵剑客",'d',	&purple,		35,		70,     160,	{5, 9, 2},		20,	DCOLS,	100,	100,	100,	DF_RED_BLOOD,	0,		0,		0,
		(MONST_CARRY_ITEM_25 | MONST_MALE | MONST_FEMALE), (MA_CAST_BLINK)},
	{0, "地底精灵牧师", 0xc0,	&darPriestessColor,20,	60,		100,	{2, 5, 1},		20,	DCOLS,	100,	100,	100,	DF_RED_BLOOD,   0,		0,		0,
		(MONST_MAINTAINS_DISTANCE | MONST_CARRY_ITEM_25 | MONST_FEMALE), (MA_CAST_HEAL | MA_CAST_SPARK | MA_CAST_HASTE | MA_CAST_NEGATION)},
	{0, "地底精灵战斗法师", 0xc1,	&darMageColor,	20,		60,		100,	{1, 3, 1},		20,	DCOLS,	100,	100,	100,	DF_RED_BLOOD,	0,		0,		0,
		(MONST_MAINTAINS_DISTANCE | MONST_CARRY_ITEM_25 | MONST_MALE | MONST_FEMALE), (MA_CAST_FIRE | MA_CAST_SLOW | MA_CAST_DISCORD)},
	{0, "酸性史莱姆",	'J',	&acidBackColor,	60,		0,		115,	{2, 6, 1},		0,	20,		20,		100,	100,	DF_ACID_BLOOD,	0,		0,		0,
		(MONST_DEFEND_DEGRADE_WEAPON), (MA_HIT_DEGRADE_ARMOR | MA_CLONE_SELF_ON_DEFEND)},
	{0,	"半人马",		'C',	&tanColor,		35,		50,		175,	{4, 8, 2},		20,	DCOLS,	30,		50,		100,	DF_RED_BLOOD,	0,		0,		0,
		(MONST_MAINTAINS_DISTANCE | MONST_MALE), (MA_ATTACKS_FROM_DISTANCE)},
	{0, "巨虫",	'U',	&wormColor,		80,		40,		160,	{18, 22, 2},	3,	20,		20,		150,	200,	DF_WORM_BLOOD,	0,		0,		0,
		(MONST_NEVER_SLEEPS)},
	{0, "雕像守卫",		STATUE_CHAR, &sentinelColor, 50,0,		0,		{0, 0, 0},		0,	DCOLS,	100,	100,	175,	DF_RUBBLE_BLOOD,SENTINEL_LIGHT,0,0,
		(MONST_TURRET | MONST_INTRINSIC_LIGHT | MONST_CAST_SPELLS_SLOWLY | MONST_DIES_IF_NEGATED), (MA_CAST_HEAL | MA_CAST_SPARK)},
	{0, "酸雾塔", TURRET_CHAR,	&acidBackColor,35,	0,		250,	{1, 2, 1},      0,	DCOLS,	50,		100,	250,	0,              0,		0,		0,
		(MONST_TURRET), (MA_ATTACKS_FROM_DISTANCE | MA_HIT_DEGRADE_ARMOR)},
	{0, "飞镖塔", TURRET_CHAR,	&centipedeColor,20,	0,		140,	{1, 2, 1},      0,	DCOLS,	50,		100,	250,	0,              0,		0,		0,
		(MONST_TURRET), (MA_ATTACKS_FROM_DISTANCE | MA_CAUSES_WEAKNESS)},
	{0,	"湖底巨妖",		'K',	&krakenColor,	120,	0,		150,	{15, 20, 3},	1,	DCOLS,	20,		50,		100,	0,              0,		0,		0,
		(MONST_RESTRICTED_TO_LIQUID | MONST_IMMUNE_TO_WATER | MONST_SUBMERGES | MONST_FLITS | MONST_NEVER_SLEEPS | MONST_FLEES_NEAR_DEATH | MONST_WILL_NOT_USE_STAIRS), (MA_SEIZES)},
	{0,	"巫妖",			'L',	&white,			35,		80,     175,	{2, 6, 1},		0,	DCOLS,	100,	100,	100,	DF_ASH_BLOOD,	LICH_LIGHT,	0,	0,
		(MONST_MAINTAINS_DISTANCE | MONST_CARRY_ITEM_25 | MONST_INTRINSIC_LIGHT | MONST_NO_POLYMORPH), (MA_CAST_SUMMON | MA_CAST_FIRE)},
	{0, "黑暗护符",	0xc9,&lichLightColor,30,	0,		0,		{0, 0, 0},		0,	DCOLS,	50,		100,	150,	DF_RUBBLE_BLOOD,LICH_LIGHT,	0,	0,
		(MONST_IMMUNE_TO_WEBS | MONST_NEVER_SLEEPS | MONST_IMMOBILE | MONST_INANIMATE | MONST_ALWAYS_HUNTING | MONST_WILL_NOT_USE_STAIRS | MONST_INTRINSIC_LIGHT | MONST_DIES_IF_NEGATED), (MA_CAST_SUMMON | MA_ENTER_SUMMONS)},
	{0, "黑暗精灵",		'p',	&pixieColor,	10,		90,     100,	{1, 3, 1},		20,	100,	100,	50,		100,	DF_GREEN_BLOOD,	PIXIE_LIGHT, 0,	0,
		(MONST_MAINTAINS_DISTANCE | MONST_INTRINSIC_LIGHT | MONST_FLIES | MONST_FLITS | MONST_MALE | MONST_FEMALE), (MA_CAST_SPARK | MA_CAST_SLOW | MA_CAST_NEGATION | MA_CAST_DISCORD)},
	{0,	"幻影",		'P',	&ectoplasmColor,35,		70,     160,	{12, 18, 4},		0,	30,		30,		50,		200,	DF_ECTOPLASM_BLOOD,	0,	2,		DF_ECTOPLASM_DROPLET,
		(MONST_INVISIBLE | MONST_FLITS | MONST_FLIES | MONST_IMMUNE_TO_WEBS)},
	{0, "火焰塔", TURRET_CHAR, &lavaForeColor,40,	0,		150,	{1, 2, 1},		0,	DCOLS,	50,		100,	250,	0,              LAVA_LIGHT,	0,	0,
		(MONST_TURRET | MONST_INTRINSIC_LIGHT), (MA_CAST_FIRE)},
	{0, "小恶魔",			'i',	&pink,			35,		90,     225,	{4, 9, 2},		10,	10,		15,		100,	100,	DF_GREEN_BLOOD,	IMP_LIGHT,	0,	0,
		(MONST_INTRINSIC_LIGHT), (MA_HIT_STEAL_FLEE | MA_CAST_BLINK)},
	{0,	"巨型蝙蝠",			'f',	&darkRed,		19,		90,     200,	{6, 11, 4},		20,	40,		30,		50,		100,	DF_RED_BLOOD,	0,		0,		0,
		(MONST_NEVER_SLEEPS | MONST_FLIES)},
	{0, "亡灵",		'R',	&ectoplasmColor,30,		0,		200,	{15, 20, 5},	0,	DCOLS,	20,		100,	100,	DF_ECTOPLASM_BLOOD,	0,	0,		0,
		(MONST_IMMUNE_TO_WEAPONS)},
	{0, "触手怪",'H',	&centipedeColor,120,	95,     225,	{25, 35, 3},	1,	DCOLS,	50,		100,	100,	DF_PURPLE_BLOOD,0,		0,		0,			(0)},
	{0, "傀儡",		'G',	&gray,			400,	70,     225,	{4, 8, 1},		0,	DCOLS,	200,	100,	100,	DF_RUBBLE_BLOOD,0,		0,		0,
		(MONST_REFLECT_4 | MONST_DIES_IF_NEGATED)},
	{0, "龙",		'D',	&dragonColor,	150,	90,     250,	{25, 50, 4},	20,	DCOLS,	120,	50,		200,	DF_GREEN_BLOOD,	0,		0,		0,
		(MONST_IMMUNE_TO_FIRE | MONST_CARRY_ITEM_100), (MA_BREATHES_FIRE)},
	
	// bosses
	{0, "地精军阀", 0xc3,	&blue,			30,		17,		100,	{3, 6, 1},		20,	30,		20,		100,	100,	DF_RED_BLOOD,	0,		0,		0,
		(MONST_MAINTAINS_DISTANCE | MONST_CARRY_ITEM_25), (MA_CAST_SUMMON)},
	{0,	"黑史莱姆",	'J',	&black,			120,	0,		130,	{3, 8, 1},		0,	20,		20,		100,	100,	DF_PURPLE_BLOOD,0,		0,		0,
		(0), (MA_CLONE_SELF_ON_DEFEND)},
	{0, "吸血鬼",		'V',	&white,			75,		60,     120,	{4, 15, 2},		6,	DCOLS,	100,	50,		100,	DF_RED_BLOOD,	0,		0,		DF_BLOOD_EXPLOSION,
		(MONST_FLEES_NEAR_DEATH | MONST_MALE), (MA_CAST_BLINK | MA_CAST_DISCORD | MA_TRANSFERENCE | MA_DF_ON_DEATH | MA_CAST_SUMMON | MA_ENTER_SUMMONS)},
	{0, "火焰之灵",	'F',	&white,			65,		80,     120,	{3, 8, 2},		0,	DCOLS,	100,	100,	100,	DF_EMBER_BLOOD,	FLAMEDANCER_LIGHT,100,DF_FLAMEDANCER_CORONA,
		(MONST_MAINTAINS_DISTANCE | MONST_IMMUNE_TO_FIRE | MONST_FIERY | MONST_INTRINSIC_LIGHT), (MA_CAST_FIRE)},
	
	// special effect monsters
	{0, "奥术之刃",0xc5, &spectralBladeColor,1, 0,	150,	{1, 1, 1},		0,	50,		50,		50,		100,	0,              SPECTRAL_BLADE_LIGHT,0,0,
		(MONST_INANIMATE | MONST_NEVER_SLEEPS | MONST_FLIES | MONST_WILL_NOT_USE_STAIRS | MONST_INTRINSIC_LIGHT | MONST_DOES_NOT_TRACK_LEADER | MONST_DIES_IF_NEGATED | MONST_IMMUNE_TO_WEBS)},
	{0, "奥术之剑",0xc5, &spectralImageColor, 1,0,	150,	{1, 1, 1},		0,	50,		50,		50,		100,	0,              SPECTRAL_IMAGE_LIGHT,0,0,
		(MONST_INANIMATE | MONST_NEVER_SLEEPS | MONST_FLIES | MONST_WILL_NOT_USE_STAIRS | MONST_INTRINSIC_LIGHT | MONST_DOES_NOT_TRACK_LEADER | MONST_DIES_IF_NEGATED | MONST_IMMUNE_TO_WEBS)},
	{0, "岩石守卫", 0xc6, &white,   1000,   0,		150,	{5, 15, 2},		0,	50,		100,	100,	100,	DF_RUBBLE,      0,      100,      DF_GUARDIAN_STEP,
		(MONST_INANIMATE | MONST_NEVER_SLEEPS | MONST_ALWAYS_HUNTING | MONST_IMMUNE_TO_FIRE | MONST_IMMUNE_TO_WEAPONS | MONST_WILL_NOT_USE_STAIRS | MONST_DIES_IF_NEGATED | MONST_REFLECT_4 | MONST_ALWAYS_USE_ABILITY | MONST_GETS_TURN_ON_ACTIVATION)},
	{0, "飞行守卫", 0xc7, &lightBlue,1000, 0,		150,	{5, 15, 2},		0,	50,		100,	100,	100,	DF_RUBBLE,      0,      100,      DF_SILENT_GLYPH_GLOW,
		(MONST_INANIMATE | MONST_NEVER_SLEEPS | MONST_ALWAYS_HUNTING | MONST_IMMUNE_TO_FIRE | MONST_IMMUNE_TO_WEAPONS | MONST_WILL_NOT_USE_STAIRS | MONST_DIES_IF_NEGATED | MONST_REFLECT_4 | MONST_GETS_TURN_ON_ACTIVATION | MONST_ALWAYS_USE_ABILITY), (MA_CAST_BLINK)},
	{0, "可怕的图腾",TOTEM_CHAR, &glyphColor,80,    0,		0,		{0, 0, 0},		0,	DCOLS,	200,	100,	100,	DF_RUBBLE_BLOOD,0,0,	0,
		(MONST_IMMUNE_TO_WEBS | MONST_NEVER_SLEEPS | MONST_IMMOBILE | MONST_INANIMATE | MONST_ALWAYS_HUNTING | MONST_WILL_NOT_USE_STAIRS | MONST_GETS_TURN_ON_ACTIVATION | MONST_ALWAYS_USE_ABILITY), (MA_CAST_SUMMON)},
	{0, "反射图腾",TOTEM_CHAR, &beckonColor,80,	0,		0,		{0, 0, 0},		0,	DCOLS,	200,	100,	100,	DF_RUBBLE_BLOOD,0,      100,	DF_MIRROR_TOTEM_STEP,
		(MONST_IMMUNE_TO_WEBS | MONST_NEVER_SLEEPS | MONST_IMMOBILE | MONST_INANIMATE | MONST_ALWAYS_HUNTING | MONST_WILL_NOT_USE_STAIRS | MONST_GETS_TURN_ON_ACTIVATION | MONST_ALWAYS_USE_ABILITY | MONST_REFLECT_4 | MONST_IMMUNE_TO_WEAPONS | MONST_IMMUNE_TO_FIRE), (MA_CAST_BECKONING)},
	
	// legendary allies
	{0,	"独角兽",		UNICORN_CHAR, &white,   40,		60,		175,	{2, 10, 2},		20,	DCOLS,	30,		50,		100,	DF_RED_BLOOD,	UNICORN_LIGHT,1,DF_UNICORN_POOP,
		(MONST_MAINTAINS_DISTANCE | MONST_INTRINSIC_LIGHT | MONST_MALE | MONST_FEMALE), (MA_CAST_HEAL | MA_CAST_PROTECTION)},
	{0,	"伊夫利特",		'I',	&ifritColor,	40,		75,     175,	{5, 13, 2},		1,	DCOLS,	30,		50,		100,	DF_ASH_BLOOD,	IFRIT_LIGHT,0,	0,
		(MONST_IMMUNE_TO_FIRE | MONST_INTRINSIC_LIGHT | MONST_FLIES | MONST_MALE), (MA_CAST_DISCORD)},
	{0,	"凤凰",		'{',	&phoenixColor,	30,		70,     175,	{4, 10, 2},		0,	DCOLS,	30,		50,		100,	DF_ASH_BLOOD,	PHOENIX_LIGHT,0,0,
		(MONST_IMMUNE_TO_FIRE| MONST_FLIES | MONST_INTRINSIC_LIGHT)},
	{0, "凤凰蛋",	0xc8,&phoenixColor,	100,	0,		0,		{0, 0, 0},		0,	DCOLS,	50,		100,	150,	DF_ASH_BLOOD,	PHOENIX_EGG_LIGHT,	0,	0,
		(MONST_IMMUNE_TO_FIRE| MONST_IMMUNE_TO_WEBS | MONST_NEVER_SLEEPS | MONST_IMMOBILE | MONST_INANIMATE | MONST_WILL_NOT_USE_STAIRS | MONST_INTRINSIC_LIGHT | MONST_NO_POLYMORPH), (MA_CAST_SUMMON | MA_ENTER_SUMMONS)},
};

#pragma mark Monster words

const monsterWords monsterText[NUMBER_MONSTER_KINDS] = {
	{"地牢里的一位冒险者，对周围的环境感觉很迷茫。",
		"在观察", "正在观察",
		{"攻击了", {0}}},
	{"老鼠是地下的常见生物，总是在四处寻找腐朽的东西。",
		"咬着", "正在吃着",
		{"抓到了", "咬了", {0}}},
	{"狗头人是一种人形生物，在地牢上层很常见。",
		"在拨弄", "正在观察",
		{"用锤子敲", "猛击", {0}}},
	{"豺狼总是在洞穴中四处徘徊寻找猎物。",
		"正撕咬着", "正在吃着",
		{"用爪子攻击了", "咬了", "撕咬了", {0}}},
	{"电鳗潜藏在湖中深处，等待这它的猎物踏入水中。",
		"正在吃着", "在吃着",
		{"电击了", "咬了", {0}}},
	{"猴子很喜欢从路过的冒险者身上盗取发光的东西。",
		"正在观察", "正在观察",
		{"拧了", "咬了", "用拳头攻击", {0}}},
	{"飞鱼看起来是半透明的，里面的剧毒气体只似乎随时都要喷出来。",
		"正看着", "在看着",
		{"撞击了", {0}},
		"突然炸开了，一股酸性气体随之喷出！"},
	{"这种罕见的飞鱼里面充满了一种奇特的气体，释放出来的话能将附近的地板融化。",
		"正在看着", "在看着",
		{"撞击了", {0}},
		"突然炸开了，附近的地板正开始随之消失！bursts, causing the floor underneath $HIMHER to disappear!"},
	{"一种肮脏的类猴生物。它们形成了部落，总是一堆一起行动。",
		"吼着", "正在叫唤",
		{"猛击", "挥砍", "刺到了", {0}}},
	{"地精巫师身上纹着发着光的符文。他能召唤出奥术刃来攻击它的敌人。",
		"在跪拜着", "跪在地上",
		{"重击", "用力攻击", "猛击", {0}},
		{0},
		"在比划着不详的手势！"},
	{"地精大法师的眼睛闪出金色的光，他能召出强力的魔法护盾来来保护它的同伴。",
		"在跪拜着", "跪在地上",
		{"扇到了", "拳击了", "踢到了", {0}}},
	{"地精搭起来这些图腾，并赋予了它萨满的力量。",
		"在凝视着", "在凝视着",
		{"攻击了", {0}}},
	{"全身都是粉红色的史莱姆在地面上蠕动。",
		"正在吸收", "在吞噬着",
		{"弄脏了", "粘到了", "弄湿了"}},
	{"这种巨大的蟾蜍可以分泌出一种能让人产生幻觉的毒素。",
		"在吃着", "正在吃",
		{"弄湿了", "冲撞了", {0}}},
	{"这些蝙蝠经常一起活动，他们在黑暗中总是能准确的找到他们的猎物。",
		"在吸取", "在吃着",
		{"用爪子抓", "咬到了", {0}}},
	{"嵌在墙壁里的一种机械装置，里面的弹簧能很快速的发出弓箭。",
		"凝视着", "在凝视着",
		{"射到了", {0}}},
	{"沾液怪会在在爬行过的地面上留下一道酸液的痕迹。",
		"在溶解", "在吞噬着",
		{"弄脏了", "把液体撒到了", "弄湿了", {0}}},
	{"蜈蚣的毒牙带有剧毒，能缓慢的杀死它的猎物。",
		"正在吃", "在吃着",
		{"刺扎了", "戳伤了", {0}}},
	{"食人魔十分有力，随身携带一根巨大的木棒。",
		"在观察", "正在观察",
		{"用棒打", "猛击", "棒击了", {0}}},
	{"这种怪物隐藏在沼泽中，当它的猎物不小心踩到泥地里沼泽怪就会用它的触手把它们困住。",
		"在吸取", "正在吃着",
		{"拧压了", "扼住了", "压打了", {0}}},
	{"食人魔念念有词的围在这些图腾周围，给它注入了神秘的力量。",
		"在凝视着", "正在凝视着",
		{"攻击了", {0}}},
	{"这种蜘蛛红色的眼睛在黑暗中发着光，一旦找到猎物就会用带有剧毒的蜘蛛网困住他。",
		"在吞噬", "正在吃着",
		{"咬到了", "针刺了", {0}}},
	{"这个造型奇特的机关由奇特的水晶组成，它能发出强力的弧形闪电击向它的敌人。",
		"在凝视着", "凝视着",
		{"发出闪电击中了", {0}}},
	{"一团造型奇特的火焰飞舞在空中，以某种奇特的方式四处移动着。",
		"吞没了", "正在吃着",
		{"烤焦了", "烧到了", {0}}},
	{"怨灵总是用它空洞的眼睛饥渴的看着这个世界，带着血迹的爪子不断挥舞着，等待下一个猎物。",
		"狼吞虎咽着", "正在吃着",
		{"抓住了", "用爪子攻击了", "咬到了", {0}}},
	{"僵尸是某种早已被遗忘的仪式的副产品。它的身体总是在腐烂，骨头露在外面。它发出的恶臭能使附近的生物反胃呕吐。",
		"正在撕咬着", "正在吃着",
		{"攻击力", "咬到了", {0}}},
	{"巨魔身形巨大，它们有着天然的迅速恢复的能力且力大无穷。不知有多少的冒险在它们的利爪下划下了句号。",
		"正在吃着", "在吃着",
		{"用木棒锤打", "用棒打", "不断的敲打", "不断用拳头打", "猛击"}},
	{"食人魔萨满在按它们的年纪算都是老头了，虽然体力不如前但却获得了更多神奇的力量。",
		"在跪拜着", "跪在地上",
		{"用木棒打到了", "不断的敲打", {0}},
		{0},
		"开始用低沉的声音吟唱施法！"},
	{"半蛇半人的娜迦平常总是潜伏在水中，等待着机会攻击粗心的冒险者。",
		"正在看着", "在看着",
		{"用爪子抓了", "咬到了", "用尾巴扇打", {0}}},
	{"这种火焰蜥蜴总是潜伏在岩浆里，当猎物出现时它们就会爬出来，留下一路烧焦的痕迹。",
		"正在看着", "在看着",
		{"用爪子攻击了", "鞭打了", "抽打了", {0}}},
	{"这种罕见的飞鱼都可以算作是一个移动的沼气罐。一点细微的刺激就会让里面的沼气喷发出来。",
		"正在凝视着", "在凝视着",
		{"撞击了", {0}},
		"里面的沼气剧烈地喷了出来！"},
	{"这种地底精灵常用的战术是先以极快的速度接近敌人，再用致命的剑术战胜他。",
		"正在看着", "在看着",
		{"用剑刮伤了", "砍到了", "切到了", "斩到了", "刺伤了"}},
	{"地底精灵牧师身上带满了法器，以至于走路的时候都会叮当作响。",
		"跪拜着", "正在祈祷",
		{"砍到了", "切到了", {0}}},
	{"地底精灵战斗法师血红色的眼睛里透出的都是愤怒，其手臂都被一团神秘的火焰包裹着。The dar battlemage's eyes glow an angry shade of red, and $HISHER hands radiate an occult heat.",
		"正在炼制着", "正在炼金",
		{"砍到了", {0}}},
	{"这种酸性的史莱姆虽然看起来弱小但可能是冒险者最讨厌见到的敌人之一。他们会腐蚀任何碰到它的武器和防具，让人非常头疼。",
		"正在吞噬", "吞噬着",
		{"用酸液灼烧到了", {0}}},
	{"半人马非常擅长使用弓箭，就像是猎人和坐骑融合在了一起。",
		"正在看着", "在看着",
		{"射箭击中了", {0}}},
	{"这种奇怪的生物来自地底最深处，虽然比食人魔个头还大但却能从窄小的地方挤过去。它们能连续几天甚至几个月静止在一处，等待猎物走上门。",
		"在吞噬着", "吞噬着",
		{"猛击", "咬到了", "用尾巴扇打", {0}}},	
	{"这些上古的雕像有着模糊的人形，但其手中的水晶武器仍然闪亮着。它们总是三个一组，任何一个受到了伤害其他的两个都会将它修补好。",
		"正在盯着", "在盯着",
		{"攻击了", {0}}},
	{"一个绿色的斑点状盆都嵌入在墙壁内，每当有人胆敢走进就会喷射出剧毒的酸雾。",
		"正在凝视着", "凝视着",
		{"将酸液喷到了", "把大量的酸液射到了", {0}}},
	{"这种机关能自动用弹簧将涂满麻痹毒药的飞镖弹射出来。",
		"正在凝视着", "凝视着",
		{"射出的飞镖扎到了", {0}}},
	{"每到有愚蠢的生物踏入湖中的时候，湖底巨妖就会伸出触手让他们再也回不去。",
		"狼吞虎咽着", "在吃着",
		{"用触手扇", "重击了", "连续猛击", {0}}},
	{"巫妖是由远古的巫术和对力量的欲望召唤出来的可怕生物。它能召唤并控制地狱的各种怪物。它体内的有一个绿色的符石，要杀死必须要摧毁这些符石。",
		"施法于", "正在施法",
		{"攻击了", {0}},
		{0},
		"掉出了一个符石。!"},
	{"这符石是几个世纪前某种的仪式的产物，连接着一个古老而可怕的巫师。只有摧毁它才能阻止巫妖的复活。",
		"施法于", "正在施法",
		{"攻击了", {0}},
		{0},
		"卷起一阵黑色的雾，巫妖重新复活了！"},
	{"黑暗精灵是一种飞在空中的人形生物。它们虽然力量比较弱，但能熟练使用各种魔法来解决它的敌人。",
		"把灰尘卷到了", "卷起一阵风",
		{"戳到了", {0}}},
	{"幻影行踪诡秘而且一般人很难看清，它总是随着地牢里一阵阴风到来，离开时只留下其猎物的残骸。",
		"穿透了", "正在穿透",
		{"攻击了", {0}}},
	{"这个冒着火的机关能向入侵者发射火焰。",
		"烧着了", "正在灼烧",
		{"射出的火焰烫到了", {0}}},
	{"这种狡诈的恶魔总是超高速的接近它的敌人，偷取了东西再迅速离开。",
		"在观察着", "正在观察",
		{"切到了", "砍到了", {0}}},
	{"巨型蝙蝠有着很身体不般配的巨型翅膀，挥舞起来会发出响亮的声音。",
		"鞭打着", "正在鞭打",
		{"攻击了", "撞击了", "重击了", {0}}},
	{"亡灵总是守候在地底深处，它们特殊的存在使得物理攻击对它们完全没有效果。",
		"正玷污着", "正在亵渎",
		{"攻击了", {0}}},
	{"触手怪看起来就是一团黏在一起的触手。它们令人恐惧的力量和惊人的恢复能力使其成为了地下城中最让人恐惧的生物之一。",
		"正在触摸", "在吞噬着",
		{"用触手扇", "猛击了", "用力的扇到了", {0}}},
	{"这种傀儡是某种上古法术的产物。它不会恢复攻击力也不强，但是要想摧毁它要花很大的功夫。",
		"在摇晃着", "正在摇晃",
		{"反手攻击了", "用拳头攻击", "用脚踢", {0}}},
	{"世界上任何的地牢里都会有龙这种生物。但这里的龙行动飞快，还能吐出能融化一切的白色火焰，着实是人见人怕的怪物。",
		"在吃着", "正在吃着",
		{"用爪子攻击了", "撕咬了", {0}}},
	
	{"地精军阀比一般的地精块头要大一圈，它能召唤它忠实的手下来帮助它战斗。",
		"吼着", "正在叫唤",
		{"猛击", "砍到了", "刺到了", {0}},
		{0},
		"最后发出一声惨叫！"},
	{"这种全身漆黑的史莱姆罕见且致命。很少有生物能承受它的剧毒攻击。",
		"吞噬着", "正在吞噬",
		{"弄脏了", "粘到了", "弄湿了"}},
	{"吸血鬼总是独自行动，它会抓住从任何路过的生物上补给营养。",
		"在吸取", "在吃着",
		{"擦伤了", "咬到了", "用它尖利的牙齿咬了", {0}},
		{0},
		"收起它的披风化成了一群蝙蝠！"},
	{"火焰之灵是来自另一个世界的生物，它几乎就是一团有意识的火焰。",
		"在灼烧着", "正在吞噬",
		{"的火焰擦到了", "烧到了", "灼烧了", {0}}},
	
	{"某种邪恶的力量具现成了这种闪光的飞刃。",
		"凝视着", "正在凝视",
		{"割伤了",  {0}}},
	{"某种神秘的力量从你的装备上涌出，行程了这把以太之剑。",
		"凝视着", "正在凝视",
		{"攻击了",  {0}}},
	{"房间里有一座骑士的雕像，它手上拿着一把巨斧，底座连接着地上闪光的符文。",
		"凝视着", "正在凝视",
		{"猛击了",  {0}}},
	{"房间里有一座持剑的天使塑像，底座连接着地上闪光的符文。",
		"凝视着", "正在凝视",
		{"猛击了",  {0}}},
	{"一人高的图腾被放在圆形的召唤阵中央，发出不详的能量。",
		"凝视着", "正在凝视",
		{"猛击了",  {0}},
		{0},
		"迸发出了强烈的能流！"},
	{"图腾光滑的表面在黑暗中都闪着光。",
		"凝视着", "正在凝视",
		{"猛击了",  {0}}},
	
	{"独角兽的鬃毛和尾巴都闪亮着彩虹的颜色。海螺状的角能释放恢复魔法。它的眼神好像在催促你永远要追逐自己的梦想！传说独角兽容易被处子吸引 - 它看你的眼神里是否有一丝职责的神色呢？",
		"在祝福着", "正在祝福",
		{"戳到了", "刺到了", "用角撞击", {0}}},
	{"伊夫利特看起来就像一团人形的沙尘暴，它手持两把弯刀，眼睛闪着红色的火焰。",
		"吞噬着", "正在吞噬",
		{"砍到了", "挥砍", "撕裂了", {0}}},
	{"传奇的凤凰全身闪着卓越的光芒，健壮的翅膀煽动着火花。同传说中的一样，凤凰不会死去，在收到伤害时它会化作一个蛋，随后会有一只新的凤凰飞起来。",
		"灼烧着", "正在灼烧",
		{"撕咬", "抓伤了", "用爪子猛击", {0}}},
	{"在一滩灰烬中，你能透过薄膜看到凤凰蛋里面的核心每秒都在变得更明亮。",
		"灼烧着", "正在灼烧",
		{"烧到了", {0}},
		{0},
		"裂开了，一只崭新的凤凰飞了出来！"},
};

#pragma mark Mutation definitions

const mutation mutationCatalog[NUMBER_MUTATORS] = {
	//Title         textColor       healthFactor    moveSpdMult attackSpdMult   defMult damMult DF% DFtype  monstFlags  abilityFlags    forbiddenFlags      forbiddenAbilities
	{"explosive",   &orange,        50,             100,        100,            50,     100,    0,  DF_MUTATION_EXPLOSION, 0, MA_DF_ON_DEATH, 0,            0,
		"A rare mutation will cause $HIMHER to explode violently when $HESHE dies."},
	{"infested",    &lichenColor,   50,             100,        100,            50,     100,    0,  DF_MUTATION_LICHEN, 0, MA_DF_ON_DEATH, 0,               0,
		"$HESHE has been infested by deadly lichen spores; poisonous fungus will spread from $HISHER corpse when $HESHE dies."},
	{"agile",       &lightBlue,     100,            50,         100,            150,    100,    -1, 0,      MONST_FLEES_NEAR_DEATH, MA_CAST_BLINK, MONST_FLEES_NEAR_DEATH, MA_CAST_BLINK,
		"A rare mutation greatly enhances $HISHER mobility."},
	{"juggernaut",  &brown,         300,            200,        200,            75,     200,    -1, 0,      0,          0,              MONST_MAINTAINS_DISTANCE, 0,
		"A rare mutation has hardened $HISHER flesh, increasing $HISHER health and power but compromising $HISHER speed."},
	{"grappling",   &tanColor,      100,            100,        100,            50,     100,    -1, 0,      0,          MA_SEIZES,      0,                  MA_SEIZES,
		"A rare mutation has caused suckered tentacles to sprout from $HISHER frame, increasing $HISHER health and allowing $HIMHER to grapple with $HISHER prey."},
	{"vampiric",    &red,           100,            100,        100,            100,    100,    -1, 0,      0,          MA_TRANSFERENCE, MONST_MAINTAINS_DISTANCE, MA_TRANSFERENCE,
		"A rare mutation allows $HIMHER to heal $HIMSELFHERSELF with every attack."},
	{"toxic",       &green,         100,            100,        100,            100,    100,    -1, 0,      0,          (MA_CAUSES_WEAKNESS | MA_POISONS), 0, (MA_CAUSES_WEAKNESS | MA_POISONS),
		"A rare mutation causes $HIMHER to poison $HISHER victims and sap their strength with every attack."},
	{"reflective",  &darkTurquoise, 100,            100,        100,            100,    100,    -1, 0,      MONST_REFLECT_4, 0,         (MONST_REFLECT_4 | MONST_ALWAYS_USE_ABILITY), 0,
		"A rare mutation has coated $HISHER flesh with a strange reflective material."},
};

#pragma mark Horde definitions

const hordeType hordeCatalog[NUMBER_HORDES] = {
	// leader		#members	member list								member numbers					minL	maxL	freq	spawnsIn		machine			flags
	{MK_RAT,			0,		{0},									{{0}},							1,		5,		10},
	{MK_KOBOLD,			0,		{0},									{{0}},							1,		6,		10},
	{MK_JACKAL,			0,		{0},									{{0}},							1,		3,		10},
	{MK_JACKAL,			1,		{MK_JACKAL},							{{1, 3, 1}},					3,		7,		5},
	{MK_EEL,			0,		{0},									{{0}},							2,		17,		10,		DEEP_WATER},
	{MK_MONKEY,			0,		{0},									{{0}},							2,		9,		7},
	{MK_BLOAT,			0,		{0},									{{0}},							2,		13,		3},
	{MK_PIT_BLOAT,		0,		{0},									{{0}},							2,		13,		1},
	{MK_BLOAT,			1,		{MK_BLOAT},								{{0, 2, 1}},					14,		26,		3},
	{MK_PIT_BLOAT,		1,		{MK_PIT_BLOAT},							{{0, 2, 1}},					14,		26,		1},
	{MK_EXPLOSIVE_BLOAT,0,		{0},									{{0}},							10,		26,		1},
	{MK_GOBLIN,			0,		{0},									{{0}},							3,		10,		10},
	{MK_GOBLIN_CONJURER,0,		{0},									{{0}},							3,		10,		6},
	{MK_TOAD,			0,		{0},									{{0}},							4,		11,		10},
	{MK_PINK_JELLY,		0,		{0},									{{0}},							4,		13,		10},
	{MK_GOBLIN_TOTEM,	1,		{MK_GOBLIN},							{{2,4,1}},						5,		13,		10,		0,				MT_CAMP_AREA,	HORDE_NO_PERIODIC_SPAWN},
	{MK_ARROW_TURRET,	0,		{0},									{{0}},							5,		13,		10,		WALL,	0,                      HORDE_NO_PERIODIC_SPAWN},
	{MK_MONKEY,			0,		{0},									{{0}},							5,		12,		5},
	{MK_MONKEY,			1,		{MK_MONKEY},							{{2,4,1}},						5,		13,		2},
	{MK_VAMPIRE_BAT,	0,		{0},                                    {{0}},                          6,		13,		3},
	{MK_VAMPIRE_BAT,	1,		{MK_VAMPIRE_BAT},						{{1,2,1}},						6,		13,		7,      0,          0,                  HORDE_NEVER_OOD},
	{MK_ACID_MOUND,		0,		{0},									{{0}},							6,		13,		10},
	{MK_GOBLIN,			3,		{MK_GOBLIN, MK_GOBLIN_MYSTIC, MK_JACKAL},{{2, 3, 1}, {1,2,1}, {1,2,1}},	6,		12,		4},
	{MK_GOBLIN_CONJURER,2,		{MK_GOBLIN_CONJURER, MK_GOBLIN_MYSTIC},	{{0,1,1}, {1,1,1}},				7,		15,		4},
	{MK_CENTIPEDE,		0,		{0},									{{0}},							7,		14,		10},
	{MK_BOG_MONSTER,	0,		{0},									{{0}},							7,		14,		8,		MUD,        0,                  HORDE_NEVER_OOD},
	{MK_OGRE,			0,		{0},									{{0}},							7,		13,		10},
	{MK_EEL,			1,		{MK_EEL},								{{2, 4, 1}},					8,		22,		7,		DEEP_WATER},
	{MK_ACID_MOUND,		1,		{MK_ACID_MOUND},						{{2, 4, 1}},					9,		13,		3},
	{MK_SPIDER,			0,		{0},									{{0}},							9,		16,		10},
	{MK_DAR_BLADEMASTER,1,		{MK_DAR_BLADEMASTER},					{{0, 1, 1}},					10,		14,		10},
	{MK_WILL_O_THE_WISP,0,		{0},									{{0}},							10,		17,		10},
	{MK_WRAITH,			0,		{0},									{{0}},							10,		17,		10},
	{MK_GOBLIN_TOTEM,	4,		{MK_GOBLIN_TOTEM, MK_GOBLIN_CONJURER, MK_GOBLIN_MYSTIC, MK_GOBLIN}, {{1,2,1},{1,2,1},{1,2,1},{3,5,1}},10,17,8,0,MT_CAMP_AREA,	HORDE_NO_PERIODIC_SPAWN},
	{MK_SPARK_TURRET,	0,		{0},									{{0}},							11,		18,		10,		WALL,	0,                      HORDE_NO_PERIODIC_SPAWN},
	{MK_ZOMBIE,			0,		{0},									{{0}},							11,		18,		10},
	{MK_TROLL,			0,		{0},									{{0}},							12,		19,		10},
	{MK_OGRE_TOTEM,		1,		{MK_OGRE},								{{2,4,1}},						12,		19,		6,		0,			0,					HORDE_NO_PERIODIC_SPAWN},
	{MK_BOG_MONSTER,	1,		{MK_BOG_MONSTER},						{{2,4,1}},						12,		26,		10,		MUD},
	{MK_NAGA,			0,		{0},									{{0}},							13,		20,		10,		DEEP_WATER},
	{MK_SALAMANDER,		0,		{0},									{{0}},							13,		20,		10,		LAVA},
	{MK_OGRE_SHAMAN,	1,		{MK_OGRE},								{{1, 3, 1}},					14,		20,		10},
	{MK_CENTAUR,		1,		{MK_CENTAUR},							{{1, 1, 1}},					14,		21,		10},
	{MK_ACID_JELLY,		0,		{0},									{{0}},							14,		21,		10},
	{MK_ACID_TURRET,	0,		{0},									{{0}},							15,		22,		10,		WALL,	0,                      HORDE_NO_PERIODIC_SPAWN},
	{MK_DART_TURRET,	0,		{0},									{{0}},							15,		22,		10,		WALL,	0,                      HORDE_NO_PERIODIC_SPAWN},
	{MK_PIXIE,			0,		{0},									{{0}},							14,		21,		8},
	{MK_FLAME_TURRET,	0,		{0},									{{0}},							14,		24,		10,		WALL,	0,                      HORDE_NO_PERIODIC_SPAWN},
	{MK_DAR_BLADEMASTER,2,		{MK_DAR_BLADEMASTER, MK_DAR_PRIESTESS},	{{0, 1, 1}, {0, 1, 1}},			15,		17,		10},
	{MK_PINK_JELLY,     2,		{MK_PINK_JELLY, MK_DAR_PRIESTESS},      {{0, 1, 1}, {1, 2, 1}},			17,		23,		7},
	{MK_KRAKEN,			0,		{0},									{{0}},							15,		30,		10,		DEEP_WATER},
	{MK_PHANTOM,		0,		{0},									{{0}},							16,		23,		10},
	{MK_WRAITH,			1,		{MK_WRAITH},							{{1, 4, 1}},					16,		23,		8},
	{MK_IMP,			0,		{0},									{{0}},							17,		24,		10},
	{MK_DAR_BLADEMASTER,3,		{MK_DAR_BLADEMASTER, MK_DAR_PRIESTESS, MK_DAR_BATTLEMAGE},{{1,2,1},{1,1,1},{1,1,1}},18,25,10},
	{MK_FURY,			1,		{MK_FURY},								{{2, 4, 1}},					18,		26,		8},
	{MK_REVENANT,		0,		{0},									{{0}},							19,		27,		10},
	{MK_GOLEM,			0,		{0},									{{0}},							21,		30,		10},
	{MK_TENTACLE_HORROR,0,		{0},									{{0}},							22,		DEEPEST_LEVEL-1,		10},
	{MK_PHYLACTERY,		0,		{0},									{{0}},							22,		DEEPEST_LEVEL-1,		10},
	{MK_DRAGON,			0,		{0},									{{0}},							24,		DEEPEST_LEVEL-1,		7},
	{MK_DRAGON,			1,		{MK_DRAGON},							{{1,1,1}},						27,		DEEPEST_LEVEL-1,		3},
	{MK_GOLEM,			3,		{MK_GOLEM, MK_DAR_PRIESTESS, MK_DAR_BATTLEMAGE}, {{1, 2, 1}, {0,1,1},{0,1,1}},27,DEEPEST_LEVEL-1,	8},
	{MK_GOLEM,			1,		{MK_GOLEM},								{{5, 10, 2}},					30,		DEEPEST_LEVEL-1,    2},
	{MK_KRAKEN,			1,		{MK_KRAKEN},							{{5, 10, 2}},					30,		DEEPEST_LEVEL-1,    10,		DEEP_WATER},
	{MK_TENTACLE_HORROR,2,		{MK_TENTACLE_HORROR, MK_REVENANT},		{{1, 3, 1}, {2, 4, 1}},			32,		DEEPEST_LEVEL-1,    2},
	{MK_DRAGON,			1,		{MK_DRAGON},							{{3, 5, 1}},					34,		DEEPEST_LEVEL-1,    2},
	
	// summons
	{MK_GOBLIN_CONJURER,1,		{MK_SPECTRAL_BLADE},					{{3, 5, 1}},					0,		0,		10,		0,			0,					HORDE_IS_SUMMONED | HORDE_DIES_ON_LEADER_DEATH},
	{MK_OGRE_SHAMAN,	1,		{MK_OGRE},								{{1, 1, 1}},					0,		0,		10,		0,			0,					HORDE_IS_SUMMONED},
	{MK_VAMPIRE,		1,		{MK_VAMPIRE_BAT},						{{3, 3, 1}},					0,		0,		10,		0,			0,					HORDE_IS_SUMMONED},
	{MK_LICH,			1,		{MK_PHANTOM},							{{2, 3, 1}},					0,		0,		10,		0,			0,					HORDE_IS_SUMMONED},
	{MK_LICH,			1,		{MK_FURY},								{{2, 3, 1}},					0,		0,		10,		0,			0,					HORDE_IS_SUMMONED},
	{MK_PHYLACTERY,		1,		{MK_LICH},								{{1,1,1}},						0,		0,		10,		0,			0,					HORDE_IS_SUMMONED},
	{MK_GOBLIN_CHIEFTAN,2,		{MK_GOBLIN_CONJURER, MK_GOBLIN},		{{1,1,1}, {3,4,1}},				0,		0,		10,		0,			0,					HORDE_IS_SUMMONED | HORDE_SUMMONED_AT_DISTANCE},
	{MK_PHOENIX_EGG,	1,		{MK_PHOENIX},							{{1,1,1}},						0,		0,		10,		0,			0,					HORDE_IS_SUMMONED},
	{MK_ELDRITCH_TOTEM, 1,		{MK_SPECTRAL_BLADE},					{{4, 7, 1}},					0,		0,		10,		0,			0,					HORDE_IS_SUMMONED | HORDE_DIES_ON_LEADER_DEATH},
	{MK_ELDRITCH_TOTEM, 1,		{MK_FURY},                              {{2, 3, 1}},					0,		0,		10,		0,			0,					HORDE_IS_SUMMONED | HORDE_DIES_ON_LEADER_DEATH},
	
	// captives
	{MK_MONKEY,			1,		{MK_KOBOLD},							{{1, 2, 1}},					1,		5,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_GOBLIN,			1,		{MK_GOBLIN},							{{1, 2, 1}},					3,		7,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_OGRE,			1,		{MK_GOBLIN},							{{3, 5, 1}},					4,		10,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_GOBLIN_MYSTIC,	1,		{MK_KOBOLD},							{{3, 7, 1}},					5,		11,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_OGRE,			1,		{MK_OGRE},								{{1, 2, 1}},					8,		15,		2,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_TROLL,			1,		{MK_TROLL},								{{1, 2, 1}},					12,		19,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_CENTAUR,		1,		{MK_TROLL},								{{1, 2, 1}},					12,		19,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_TROLL,			2,		{MK_OGRE, MK_OGRE_SHAMAN},				{{2, 3, 1}, {0, 1, 1}},			14,		19,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_DAR_BLADEMASTER,1,		{MK_TROLL},								{{1, 2, 1}},					12,		19,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_NAGA,			1,		{MK_SALAMANDER},						{{1, 2, 1}},					13,		20,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_SALAMANDER,		1,		{MK_NAGA},								{{1, 2, 1}},					13,		20,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_TROLL,			1,		{MK_SALAMANDER},						{{1, 2, 1}},					13,		19,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_IMP,			1,		{MK_FURY},								{{2, 4, 1}},					18,		26,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_PIXIE,			1,		{MK_IMP, MK_PHANTOM},					{{1, 2, 1}, {1, 2, 1}},			14,		21,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_DAR_BLADEMASTER,1,		{MK_FURY},								{{2, 4, 1}},					18,		26,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_DAR_BLADEMASTER,1,		{MK_IMP},								{{2, 3, 1}},					18,		26,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_DAR_PRIESTESS,	1,		{MK_FURY},								{{2, 4, 1}},					18,		26,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_DAR_BATTLEMAGE,	1,		{MK_IMP},								{{2, 3, 1}},					18,		26,		1,		0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_TENTACLE_HORROR,3,		{MK_DAR_BLADEMASTER, MK_DAR_PRIESTESS, MK_DAR_BATTLEMAGE},{{1,2,1},{1,1,1},{1,1,1}},20,26,1,	0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	{MK_GOLEM,			3,		{MK_DAR_BLADEMASTER, MK_DAR_PRIESTESS, MK_DAR_BATTLEMAGE},{{1,2,1},{1,1,1},{1,1,1}},18,25,1,	0,			0,					HORDE_LEADER_CAPTIVE | HORDE_NEVER_OOD},
	
	// bosses
	{MK_GOBLIN_CHIEFTAN,2,		{MK_GOBLIN_MYSTIC, MK_GOBLIN, MK_GOBLIN_TOTEM}, {{1,1,1}, {2,3,1}, {2,2,1}},2,	10,		5,		0,			0,					HORDE_MACHINE_BOSS},
	{MK_BLACK_JELLY,	0,		{0},									{{0}},							5,		15,		5,		0,			0,					HORDE_MACHINE_BOSS},
	{MK_VAMPIRE,		0,		{0},									{{0}},							10,		DEEPEST_LEVEL,	5,  0,		0,					HORDE_MACHINE_BOSS},
	{MK_FLAMESPIRIT,	0,		{0},									{{0}},							10,		DEEPEST_LEVEL,	5,  0,		0,					HORDE_MACHINE_BOSS},
	
	// machine water monsters
	{MK_EEL,			0,		{0},									{{0}},							2,		7,		10,		DEEP_WATER,	0,					HORDE_MACHINE_WATER_MONSTER},
	{MK_EEL,			1,		{MK_EEL},								{{2, 4, 1}},					5,		15,		10,		DEEP_WATER,	0,					HORDE_MACHINE_WATER_MONSTER},
	{MK_KRAKEN,			0,		{0},									{{0}},							12,		DEEPEST_LEVEL,	10,	DEEP_WATER,	0,				HORDE_MACHINE_WATER_MONSTER},
	{MK_KRAKEN,			1,		{MK_EEL},								{{1, 2, 1}},					12,		DEEPEST_LEVEL,	8,	DEEP_WATER,	0,				HORDE_MACHINE_WATER_MONSTER},
	
	// dungeon captives -- no captors
	{MK_OGRE,			0,		{0},									{{0}},							1,		5,		10,		0,			0,					HORDE_MACHINE_CAPTIVE | HORDE_LEADER_CAPTIVE},
	{MK_NAGA,			0,		{0},									{{0}},							2,		8,		10,		0,			0,					HORDE_MACHINE_CAPTIVE | HORDE_LEADER_CAPTIVE},
	{MK_GOBLIN_MYSTIC,	0,		{0},									{{0}},							2,		8,		10,		0,			0,					HORDE_MACHINE_CAPTIVE | HORDE_LEADER_CAPTIVE},
	{MK_TROLL,			0,		{0},									{{0}},							10,		20,		10,		0,			0,					HORDE_MACHINE_CAPTIVE | HORDE_LEADER_CAPTIVE},
	{MK_DAR_BLADEMASTER,0,		{0},									{{0}},							8,		14,		10,		0,			0,					HORDE_MACHINE_CAPTIVE | HORDE_LEADER_CAPTIVE},
	{MK_DAR_PRIESTESS,	0,		{0},									{{0}},							8,		14,		10,		0,			0,					HORDE_MACHINE_CAPTIVE | HORDE_LEADER_CAPTIVE},
	{MK_WRAITH,			0,		{0},									{{0}},							11,		17,		10,		0,			0,					HORDE_MACHINE_CAPTIVE | HORDE_LEADER_CAPTIVE},
	{MK_GOLEM,			0,		{0},									{{0}},							17,		23,		10,		0,			0,					HORDE_MACHINE_CAPTIVE | HORDE_LEADER_CAPTIVE},
	{MK_TENTACLE_HORROR,0,		{0},									{{0}},							20,		AMULET_LEVEL,10,0,			0,					HORDE_MACHINE_CAPTIVE | HORDE_LEADER_CAPTIVE},
	{MK_DRAGON,			0,		{0},									{{0}},							23,		AMULET_LEVEL,10,0,			0,					HORDE_MACHINE_CAPTIVE | HORDE_LEADER_CAPTIVE},
	
	// machine statue monsters
	{MK_GOBLIN,			0,		{0},									{{0}},							1,		6,		10,		STATUE_DORMANT, 0,				HORDE_MACHINE_STATUE},
	{MK_OGRE,			0,		{0},									{{0}},							6,		12,		10,		STATUE_DORMANT, 0,				HORDE_MACHINE_STATUE},
	{MK_WRAITH,			0,		{0},									{{0}},							10,		17,		10,		STATUE_DORMANT, 0,				HORDE_MACHINE_STATUE},
	{MK_NAGA,			0,		{0},									{{0}},							12,		19,		10,		STATUE_DORMANT, 0,				HORDE_MACHINE_STATUE},
	{MK_TROLL,			0,		{0},									{{0}},							14,		21,		10,		STATUE_DORMANT, 0,				HORDE_MACHINE_STATUE},
	{MK_GOLEM,			0,		{0},									{{0}},							21,		30,		10,		STATUE_DORMANT, 0,				HORDE_MACHINE_STATUE},
	{MK_DRAGON,			0,		{0},									{{0}},							29,		DEEPEST_LEVEL,	10,	STATUE_DORMANT, 0,			HORDE_MACHINE_STATUE},
	{MK_TENTACLE_HORROR,0,		{0},									{{0}},							29,		DEEPEST_LEVEL,	10,	STATUE_DORMANT, 0,			HORDE_MACHINE_STATUE},
	
	// machine turrets
	{MK_ARROW_TURRET,	0,		{0},									{{0}},							5,		13,		10,		TURRET_DORMANT, 0,				HORDE_MACHINE_TURRET},
	{MK_SPARK_TURRET,	0,		{0},									{{0}},							11,		18,		10,		TURRET_DORMANT, 0,				HORDE_MACHINE_TURRET},
	{MK_ACID_TURRET,	0,		{0},									{{0}},							15,		22,		10,		TURRET_DORMANT, 0,				HORDE_MACHINE_TURRET},
	{MK_DART_TURRET,	0,		{0},									{{0}},							15,		22,		10,		TURRET_DORMANT, 0,				HORDE_MACHINE_TURRET},
	{MK_FLAME_TURRET,	0,		{0},									{{0}},							17,		24,		10,		TURRET_DORMANT, 0,				HORDE_MACHINE_TURRET},
	
	// machine mud monsters
	{MK_BOG_MONSTER,	0,		{0},									{{0}},							12,		26,		10,		MACHINE_MUD_DORMANT, 0,			HORDE_MACHINE_MUD},
	{MK_KRAKEN,			0,		{0},									{{0}},							17,		26,		3,		MACHINE_MUD_DORMANT, 0,			HORDE_MACHINE_MUD},
	
	// kennel monsters
	{MK_MONKEY,			0,		{0},									{{0}},							1,		5,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},
	{MK_GOBLIN,			0,		{0},									{{0}},							1,		8,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},
	{MK_GOBLIN_CONJURER,0,		{0},									{{0}},							2,		9,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},
	{MK_GOBLIN_MYSTIC,	0,		{0},									{{0}},							2,		9,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},
	{MK_OGRE,			0,		{0},									{{0}},							5,		15,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},
	{MK_TROLL,			0,		{0},									{{0}},							10,		19,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},
	{MK_NAGA,			0,		{0},									{{0}},							9,		20,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},
	{MK_SALAMANDER,		0,		{0},									{{0}},							9,		20,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},
	{MK_IMP,			0,		{0},									{{0}},							15,		26,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},
	{MK_PIXIE,			0,		{0},									{{0}},							11,		21,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},	
	{MK_DAR_BLADEMASTER,0,		{0},									{{0}},							9,		AMULET_LEVEL, 10, MONSTER_CAGE_CLOSED, 0,		HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},
	{MK_DAR_PRIESTESS,	0,		{0},									{{0}},							12,		AMULET_LEVEL, 10, MONSTER_CAGE_CLOSED, 0,		HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},
	{MK_DAR_BATTLEMAGE,	0,		{0},									{{0}},							13,		AMULET_LEVEL, 10, MONSTER_CAGE_CLOSED, 0,		HORDE_MACHINE_KENNEL | HORDE_LEADER_CAPTIVE},
	
	// vampire bloodbags
	{MK_MONKEY,			0,		{0},									{{0}},							1,		5,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE},
	{MK_GOBLIN,			0,		{0},									{{0}},							1,		8,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE},
	{MK_GOBLIN_CONJURER,0,		{0},									{{0}},							2,		9,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE},
	{MK_GOBLIN_MYSTIC,	0,		{0},									{{0}},							2,		9,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE},
	{MK_OGRE,			0,		{0},									{{0}},							5,		15,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE},
	{MK_TROLL,			0,		{0},									{{0}},							10,		19,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE},
	{MK_NAGA,			0,		{0},									{{0}},							9,		20,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE},
	{MK_IMP,			0,		{0},									{{0}},							15,		AMULET_LEVEL,10,MONSTER_CAGE_CLOSED, 0,			HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE},
	{MK_PIXIE,			0,		{0},									{{0}},							11,		21,		10,		MONSTER_CAGE_CLOSED, 0,			HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE},	
	{MK_DAR_BLADEMASTER,0,		{0},									{{0}},							9,		AMULET_LEVEL,10,MONSTER_CAGE_CLOSED, 0,			HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE},
	{MK_DAR_PRIESTESS,	0,		{0},									{{0}},							12,		AMULET_LEVEL,10,MONSTER_CAGE_CLOSED, 0,			HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE},
	{MK_DAR_BATTLEMAGE,	0,		{0},									{{0}},							13,		AMULET_LEVEL,10,MONSTER_CAGE_CLOSED, 0,			HORDE_VAMPIRE_FODDER | HORDE_LEADER_CAPTIVE},
	
	// key thieves
	{MK_MONKEY,			0,		{0},									{{0}},							1,		14,		10,     0,          0,                  HORDE_MACHINE_THIEF},
	{MK_IMP,			0,		{0},									{{0}},							15,		DEEPEST_LEVEL,	10, 0,      0,                  HORDE_MACHINE_THIEF},
	
	// legendary allies
	{MK_UNICORN,		0,		{0},									{{0}},							1,		DEEPEST_LEVEL,	10, 0,		0,					HORDE_MACHINE_LEGENDARY_ALLY | HORDE_ALLIED_WITH_PLAYER},
	{MK_IFRIT,			0,		{0},									{{0}},							1,		DEEPEST_LEVEL,	10,	0,		0,					HORDE_MACHINE_LEGENDARY_ALLY | HORDE_ALLIED_WITH_PLAYER},
	{MK_PHOENIX_EGG,	0,		{0},									{{0}},							1,		DEEPEST_LEVEL,	10,	0,		0,					HORDE_MACHINE_LEGENDARY_ALLY | HORDE_ALLIED_WITH_PLAYER},
};

// ITEMS

#pragma mark Item flavors

char itemTitles[NUMBER_SCROLL_KINDS][30];

const char titlePhonemes[NUMBER_TITLE_PHONEMES][30] = {
	"glorp",
	"snarg",
	"gana",
	"flin",
	"herba",
	"pora",
	"nuglo",
	"greep",
	"nur",
	"lofa",
	"poder",
	"nidge",
	"pus",
	"wooz",
	"flem",
	"bloto",
	"porta"
};

char itemColors[NUMBER_ITEM_COLORS][30];

const char itemColorsRef[NUMBER_ITEM_COLORS][30] = {
	"深红色",
	"绯红色",
	"橙色",
	"黄色",
	"绿色",
	"蓝色",
	"湛蓝色",
	"紫罗兰色",
	"深褐色",
	"浅紫色",
	"紫红色",
	"松绿色",
	"海蓝色",
	"灰色",
	"粉红色",
	"白色",
	"薰衣草色",
	"褐色",
	"棕色",
	"青色",
	"黑色"
};

char itemWoods[NUMBER_ITEM_WOODS][30];

const char itemWoodsRef[NUMBER_ITEM_WOODS][30] = {
	"柚木",
	"栎木",
	"红杉木",
	"花楸木",
	"柳木",
	"红木",
	"松木",
	"枫木",
	"竹制",
	"硬木",
	"梨木",
	"桦木",
	"樱木",
	"桉木",
	"胡桃木",
	"雪松木",
	"蔷薇木",
	"紫杉木",
	"檀香木",
	"胡桃木",
	"毒芹木",
};

char itemMetals[NUMBER_ITEM_METALS][30];

const char itemMetalsRef[NUMBER_ITEM_METALS][30] = {
	"铜质",
	"铁质",
	"黄铜",
	"白镴",
	"镍制",
	"镀铜",
	"铝制",
	"钨制",
	"钛合金",
};

char itemGems[NUMBER_ITEM_GEMS][30];

const char itemGemsRef[NUMBER_ITEM_GEMS][30] = {
	"钻石",
	"猫眼石",
	"石榴石",
	"红宝石",
	"紫水晶",
	"黄玉",
	"爪石",
	"电气石",
	"蓝宝石",
	"黑曜石",
	"孔雀石",
	"蓝晶",
	"绿宝石",
	"翡翠",
	"紫翠玉",
	"玛瑙",
	"血玉",
	"碧玉"
};

#pragma mark Item definitions

//typedef struct itemTable {
//	char *name;
//	char *flavor;
//	short frequency;
//	short marketValue;
//	short number;
//	randomRange range;
//} itemTable;

const itemTable keyTable[NUMBER_KEY_TYPES] = {
	{"door key",			"", "", 1, 0,	0, {0,0,0}, true, false, "The notches on this ancient iron key are well worn; its leather lanyard is battered by age. What door might it open?"},
	{"cage key",			"", "", 1, 0,	0, {0,0,0}, true, false, "The rust accreted on this iron key has been stained with flecks of blood; it must have been used recently. What cage might it open?"},
	{"crystal orb",			"", "", 1, 0,	0, {0,0,0}, true, false, "A faceted orb, seemingly cut from a single crystal, sparkling and perpetually warm to the touch. What manner of device might such an object activate?"},
};

const itemTable foodTable[NUMBER_FOOD_KINDS] = {
	{"ration of food",		"", "", 3, 25,	1800, {0,0,0}, true, false, "A ration of food. Was it left by former adventurers? Is it a curious byproduct of the subterranean ecosystem?"},
	{"mango",				"", "", 1, 15,	1550, {0,0,0}, true, false, "An odd fruit to be found so deep beneath the surface of the earth, but only slightly less filling than a ration of food."}
};

const itemTable weaponTable[NUMBER_WEAPON_KINDS] = {
	{"dagger",				"", "", 10, 190,		10,	{3,	4,	1},		true, false, "A simple iron dagger with a well-worn wooden handle. 一二三四五六七八九一二三四五六七八九一二三四五六七八九一二三四五六七八九一二三四五六七八九一二三四五六七八九一二三四五六七八九一二三四五六七八九一二三四五六七八九一二三四五六七八九一二三四五六七八九一二三四五六七八九一二三四五六七八九一二三四五六七八九"},
	{"sword",				"", "", 10, 440,		14, {6,	10,	1},		true, false, "The razor-sharp length of steel blade shines reassuringly. "},
	{"broadsword",			"", "", 10, 990,		19,	{14, 22, 1},	true, false, "This towering blade inflicts heavy damage by investing its heft into every cut. "},
	
	{"rapier",				"", "", 10, 440,		15, {3,	5,	1},		true, false, "This blade is thin and flexible, intended for deft and rapid maneuvers. It inflicts less damage than comparable weapons, but permits you to attack twice as quickly. If there is one space between you and an enemy and you step directly toward it, you will perform a devastating lunge attack, which deals treble damage and never misses. "},
	
	{"mace",				"", "", 10, 660,		16, {18, 30, 1},	true, false, "The symmetrical iron flanges at the head of this weapon inflict substantial damage, but attacking takes two turns because of its weight. "},
	{"war hammer",			"", "", 10, 1100,		20, {30, 50, 1},	true, false, "Few creatures can withstand the crushing blow of this towering mass of lead and steel, but only the strongest of adventurers can use it effectively, and attacking takes two turns because of its weight. "},
	
	{"spear",				"", "", 10, 330,		13, {4, 5, 1},		true, false, "A slender wooden rod tipped with sharpened iron. The reach of the spear permits you to simultaneously attack an adjacent enemy and the enemy directly behind it. "},
	{"war pike",			"", "", 10, 880,		18, {9, 15, 1},		true, false, "A long steel pole ending in a razor-sharp point. The reach of the pike permits you to simultaneously attack an adjacent enemy and the enemy directly behind it. "},
	
	{"axe",					"", "", 10, 550,		15, {6, 9, 1},		true, false, "The blunt iron edge on this axe glints in the darkness. The arc of its swing permits you to attack all adjacent enemies simultaneously. "},
	{"war axe",				"", "", 10, 990,		19, {10, 17, 1},	true, false, "The enormous steel head of this war axe puts considerable heft behind each stroke. The arc of its swing permits you to attack all adjacent enemies simultaneously. "},

	{"dart",				"", "",	0,	15,			10,	{2,	4,	1},		true, false, "These simple metal spikes are weighted to fly true and sting their prey with a flick of the wrist. "},
	{"incendiary dart",		"", "",	10, 25,			12,	{1,	2,	1},		true, false, "The spike on each of these darts is designed to pin it to its target while the unstable compounds strapped to its length burst into brilliant flames. "},
	{"javelin",				"", "",	10, 40,			15,	{3, 11, 3},		true, false, "This length of metal is weighted to keep the spike at its tip foremost as it sails through the air. "},
};

const itemTable armorTable[NUMBER_ARMOR_KINDS] = {
	{"leather armor",	"", "", 10,	250,		10,	{30,30,0},		true, false, "This lightweight armor offers basic protection. "},
	{"scale mail",		"", "", 10, 350,		12, {40,40,0},		true, false, "Bronze scales cover the surface of treated leather, offering greater protection than plain leather with minimal additional weight. "},
	{"chain mail",		"", "", 10, 500,		13, {50,50,0},		true, false, "Interlocking metal links make for a tough but flexible suit of armor. "},
	{"banded mail",		"", "", 10, 800,		15, {70,70,0},		true, false, "Overlapping strips of metal horizontally encircle a chain mail base, offering an additional layer of protection at the cost of greater weight. "},
	{"splint mail",		"", "", 10, 1000,		17, {90,90,0},		true, false, "Thick plates of metal are embedded into a chain mail base, providing the wearer with substantial protection. "},
	{"plate armor",		"", "", 10, 1300,		19, {110,110,0},	true, false, "Enormous plates of metal are joined together into a suit that provides unmatched protection to any adventurer strong enough to bear its staggering weight. "}
};

const char weaponRunicNames[NUMBER_WEAPON_RUNIC_KINDS][30] = {
	"speed",
	"quietus",
	"paralysis",
	"multiplicity",
	"slowing",
	"confusion",
	"force",
	"slaying",
	"mercy",
	"plenty"
};

const char armorRunicNames[NUMBER_ARMOR_ENCHANT_KINDS][30] = {
	"multiplicity",
	"mutuality",
	"absorption",
	"reprisal",
	"immunity",
	"reflection",
	"respiration",
	"dampening",
	"burden",
	"vulnerability",
	"immolation",
};

itemTable scrollTable[NUMBER_SCROLL_KINDS] = {
	{"enchanting",			itemTitles[0], "",	0,	550,	0,{0,0,0}, false, false, "This indispensable scroll will imbue a single item with a powerful and permanent magical charge. A staff will increase in power and in number of charges; a weapon will inflict more damage or find its mark more frequently; a suit of armor will deflect additional blows; the effect of a ring on its wearer will intensify; and a wand will gain expendable charges in the least amount that such a wand can be found with. Weapons and armor will also require less strength to use, and any curses on the item will be lifted."}, // frequency is dynamically adjusted
	{"identify",			itemTitles[1], "",	30,	300,	0,{0,0,0}, false, false, "The scrying magic on this parchment will permanently reveals all of the secrets of a single item."},
	{"teleportation",		itemTitles[2], "",	10,	500,	0,{0,0,0}, false, false, "The spell on this parchment instantly transports the reader to a random location on the dungeon level. It can be used to escape a dangerous situation, but the unlucky reader might find himself in an even more dangerous place."},
	{"remove curse",		itemTitles[3], "",	15,	150,	0,{0,0,0}, false, false, "The incantation on this scroll will instantly strip from the reader's weapon, armor, rings and carried items any evil enchantments that might prevent the wearer from removing them."},
	{"recharging",			itemTitles[4], "",	12,	375,	0,{0,0,0}, false, false, "The power bound up in this parchment will recharge all of your staffs and charms and will add a charge to each of your wands."},
	{"protect armor",		itemTitles[5], "",	10,	400,	0,{0,0,0}, false, false, "The armor worn by the reader of this scroll will be permanently proofed against degradation from acid."},
	{"protect weapon",		itemTitles[6], "",	10,	400,	0,{0,0,0}, false, false, "The weapon held by the reader of this scroll will be permanently proofed against degradation from acid."},
	{"magic mapping",		itemTitles[7], "",	12,	500,	0,{0,0,0}, false, false, "When this scroll is read, a purple-hued image of crystal clarity will be etched into your memory, alerting you to the precise layout of the level and revealing all hidden secrets. The locations of items and creatures will remain unknown."},
//	{"cause fear",			itemTitles[ ], "",	8,	500,	0,{0,0,0}, false, false, "A flash of red light will overwhelm all creatures in your field of view with terror, and they will turn and flee. Attacking a fleeing enemy will dispel the effect, and even fleeing creatures will turn to fight when they are cornered. Any allies caught within its blast will return to your side after the effect wears off, provided that you do not attack them in the interim."},
	{"negation",			itemTitles[8], "",	8,	400,	0,{0,0,0}, false, false, "This scroll contains a powerful anti-magic. When it is released, all creatures (including yourself) and all items lying on the ground within your field of view will be exposed to its blast and stripped of magic -- and creatures animated purely by magic will die. Potions, scrolls, items being held by other creatures and items in your inventory will not be affected."},
	{"shattering",			itemTitles[9],"",	8,	500,	0,{0,0,0}, false, false, "The blast of sorcery unleashed by this scroll will alter the physical structure of nearby stone, causing it to dissolve away over the ensuing minutes."},
	{"aggravate monsters",	itemTitles[10], "",	15,	50,		0,{0,0,0}, false, false, "When read aloud, this scroll will unleash a piercing shriek that will awaken all monsters and alert them to the reader's location."},
	{"summon monsters",		itemTitles[11], "",	10,	50,		0,{0,0,0}, false, false, "The incantation on this scroll will call out to creatures in other planes of existence, drawing them through the fabric of reality to confront the reader."},
};

itemTable potionTable[NUMBER_POTION_KINDS] = {
	{"life",				itemColors[1], "",	0,	500,	0,{0,0,0}, false, false, "A swirling elixir that will instantly heal you, cure you of ailments, and permanently increase your maximum health."}, // frequency is dynamically adjusted
	{"strength",			itemColors[2], "",	0,	400,	0,{0,0,0}, false, false, "This powerful medicine will course through your muscles, permanently increasing your strength by one point."}, // frequency is dynamically adjusted
	{"telepathy",			itemColors[3], "",	20,	350,	0,{0,0,0}, false, false, "After drinking this, your mind will become attuned to the psychic signature of distant creatures, enabling you to sense biological presences through walls. Its effects will not reveal inanimate objects, such as totems, turrets and traps."},
	{"levitation",			itemColors[4], "",	15,	250,	0,{0,0,0}, false, false, "Drinking this curious liquid will cause you to hover five to ten feet in the air, able to drift effortlessly over lava, water, chasms and traps. Flames, gases and spiderwebs fill the air, however, and cannot be bypassed while airborne. Creatures that dwell in water or mud will be unable to attack you while you levitate."},
	{"detect magic",		itemColors[5], "",	20,	500,	0,{0,0,0}, false, false, "This drink will sensitize your mind to the radiance of magic. Items imbued with helpful enchantments will be marked on the map with a full magical sigil; items corrupted by curses or intended to inflict harm will be marked on the map with a hollow sigil. The Amulet of Yendor, if in the vicinity, will be revealed by its unique aura."},
	{"speed",				itemColors[6], "",	10,	500,	0,{0,0,0}, false, false, "Quaffing the contents of this flask will enable you to move at blinding speed for several minutes."},
	{"fire immunity",		itemColors[7], "",	15,	500,	0,{0,0,0}, false, false, "This potion will render you impervious to heat and permit you to wander through fire and lava and ignore otherwise deadly bolts of flame. It will not guard against the concussive impact of an explosion, however."},
	{"invisibility",		itemColors[8], "",	15,	400,	0,{0,0,0}, false, false, "Drinking this potion will render you temporarily invisible. While invisible, enemies will track you only with great difficulty, and will be completely unable to detect you from more than two spaces away."},
	{"poisonous gas",		itemColors[9], "",	15,	200,	0,{0,0,0}, false, false, "Uncorking or shattering this pressurized glass will cause its contents to explode into a deadly cloud of caustic purple gas. You might choose to fling this potion at distant enemies instead of uncorking it by hand."},
	{"paralysis",			itemColors[10], "",	10, 250,	0,{0,0,0}, false, false, "Upon exposure to open air, the liquid in this flask will vaporize into a numbing pink haze. Anyone who inhales the cloud will be paralyzed instantly, unable to move for some time after the cloud dissipates. This item can be thrown at distant enemies to catch them within the effect of the gas."},
	{"hallucination",		itemColors[11], "",	10,	500,	0,{0,0,0}, false, false, "This flask contains a vicious and long-lasting hallucinogen. Under its dazzling effect, you will wander through a rainbow wonderland, unable to discern the form of any creatures or items you see."},
	{"confusion",			itemColors[12], "",	15,	450,	0,{0,0,0}, false, false, "This unstable chemical will quickly vaporize into a glittering cloud upon contact with open air, causing any creature that inhales it to lose control of the direction of its movements until the effect wears off (although its ability to aim projectile attacks will not be affected). Its vertiginous intoxication can cause creatures and adventurers to careen into one another or into chasms or lava pits, so extreme care should be taken when under its effect. Its contents can be weaponized by throwing the flask at distant enemies."},
	{"incineration",		itemColors[13], "",	15,	500,	0,{0,0,0}, false, false, "This flask contains an unstable compound which will burst violently into flame upon exposure to open air. You might throw the flask at distant enemies -- or into a deep lake, to cleanse the cavern with scalding steam."},
	{"darkness",			itemColors[14], "",	7,	150,	0,{0,0,0}, false, false, "Drinking this potion will plunge you into darkness. At first, you will be completely blind to anything not illuminated by an independent light source, but over time your vision will regain its former strength. Throwing the potion will create a cloud of supernatural darkness, and enemies will have difficulty seeing or following you if you take refuge under its cover."},
	{"descent",				itemColors[15], "",	15,	500,	0,{0,0,0}, false, false, "When this flask is uncorked by hand or shattered by being thrown, the fog that seeps out will temporarily cause the ground in the vicinity to vanish."},
	{"creeping death",		itemColors[16], "",	7,	450,	0,{0,0,0}, false, false, "When the cork is popped or the flask is thrown, tiny spores will spill across the ground and begin to grow a deadly lichen. Anything that touches the lichen will be poisoned by its clinging tendrils, and the lichen will slowly grow to fill the area. Fire will purge the infestation."},
};

itemTable wandTable[NUMBER_WAND_KINDS] = {
	{"teleportation",	itemMetals[0], "",	1,	800,	0,{2,5,1}, false, false, "A blast from this wand will teleport a creature to a random place on the level. This can be particularly effective against aquatic or mud-bound creatures, which are helpless on dry land."},
	{"slowness",		itemMetals[1], "",	1,	800,	0,{2,5,1}, false, false, "This wand will cause a creature to move at half its ordinary speed for several turns."},
	{"polymorphism",	itemMetals[2], "",	1,	700,	0,{3,5,1}, false, false, "This mischievous magic can transform any creature into another creature at random. Beware: the tamest of creatures might turn into the most fearsome. The horror of the transformation will turn any affected allies against you."},
	{"negation",		itemMetals[3], "",	1,	550,	0,{4,6,1}, false, false, "This powerful anti-magic will strip a creature of a host of magical traits, including flight, invisibility, acidic corrosiveness, telepathy, magical speed or slowness, hypnosis, magical fear, immunity to physical attack, fire resistance and the ability to blink at will. Spellcasters will lose their magical abilities and magical totems will be rendered inert. Creatures animated purely by magic will die."},
	{"domination",		itemMetals[4], "",	1,	1000,	0,{1,2,1}, false, false, "This wand can forever bind an enemy to the caster's will, turning it into a steadfast ally. However, the magic works only against enemies that are near death."},
	{"beckoning",		itemMetals[5], "",	1,	500,	0,{2,4,1}, false, false, "The force of this wand will yank the targeted creature into direct proximity."},
	{"plenty",			itemMetals[6], "",	1,	700,	0,{1,2,1}, false, false, "The creature at the other end of this mischievous bit of metal will be beside itself -- literally! Cloning an enemy is ill-advised, but the effect can be invaluable on a powerful ally."},
	{"invisibility",	itemMetals[7], "",	1,	100,	0,{3,5,1}, false, false, "A charge from this wand will render a creature temporarily invisible to the naked eye. Only with telepathy or in the silhouette of a thick gas will an observer discern the creature's hazy outline."},
};

itemTable staffTable[NUMBER_STAFF_KINDS] = {
	{"lightning",		itemWoods[0], "",	15,	1300,	0,{2,4,1}, false, false, "This staff conjures forth deadly arcs of electricity, which deal damage to any number of creatures in a straight line."},
	{"firebolt",		itemWoods[1], "",	15,	1300,	0,{2,4,1}, false, false, "This staff unleashes bursts of magical fire. It will ignite flammable terrain, and will damage and burn a creature that it hits. Creatures with an immunity to fire will be unaffected by the bolt."},
	{"poison",			itemWoods[2], "",	10,	1200,	0,{2,4,1}, false, false, "The vile blast of this twisted bit of wood will imbue its target with a deadly venom. A creature that is poisoned will suffer one point of damage per turn and will not regenerate lost health until the effect ends. The duration of the effect increases exponentially with the level of the staff, and a level 10 staff can fell even a deadly dragon with a single use -- eventually."},
	{"tunneling",		itemWoods[3], "",	10,	1000,	0,{2,4,1}, false, false, "Bursts of magic from this staff will pass harmlessly through creatures but will reduce walls and other inanimate obstructions to rubble."},
	{"blinking",		itemWoods[4], "",	11,	1200,	0,{2,4,1}, false, false, "This staff will allow you to teleport in the chosen direction. Creatures and inanimate obstructions will block the teleportation. Be careful around dangerous terrain, as nothing will prevent you from teleporting to a fiery death in a lake of lava."},
	{"entrancement",	itemWoods[5], "",	5,	1000,	0,{2,4,1}, false, false, "This curious staff will send creatures into a deep but temporary trance, in which they will mindlessly mirror your movements. You can use the effect to cause one creature to attack another or to step into lava or other hazardous terrain, but the spell will be broken if you attack the creature under the effect."},
	{"obstruction",		itemWoods[6], "",	10,	1000,	0,{2,4,1}, false, false, "A mass of impenetrable green crystal will spring forth from the point at which this staff is aimed, obstructing any who wish to move through the affected area and temporarily entombing any who are already there. The crystal will dissolve into the air as time passes. Higher level staffs will create larger obstructions."},
	{"discord",			itemWoods[7], "",	10,	1000,	0,{2,4,1}, false, false, "The purple light from this staff will alter the perceptions of all creatures to think the target is their enemy. Strangers and allies alike will turn on an affected creature."},
	{"conjuration",		itemWoods[8], "",	8,	1000,	0,{2,4,1}, false, false, "A flick of this staff summons a number of phantom blades to fight on your behalf."},
	{"healing",			itemWoods[9], "",	6,	1100,	0,{2,4,1}, false, false, "The crimson bolt from this staff will heal the injuries of any creature it touches. This can be counterproductive against enemies but can prove useful when aimed at your allies. Unfortunately, you cannot use this or any staff on yourself."},
	{"haste",			itemWoods[10], "",	6,	900,	0,{2,4,1}, false, false, "The magical bolt from this staff will temporarily double the speed of any monster it hits. This can be counterproductive against enemies but can prove useful when aimed at your allies. Unfortunately, you cannot use this or any staff on yourself."},
	{"protection",		itemWoods[11], "",	6,	900,	0,{2,4,1}, false, false, "A charge from this staff will bathe a creature in protective light, absorbing all damage until depleted. This can be counterproductive against enemies but can prove useful when aimed at your allies. Unfortunately, you cannot use this or any staff on yourself."},
};

itemTable ringTable[NUMBER_RING_KINDS] = {
	{"clairvoyance",	itemGems[0], "",	1,	900,	0,{1,3,1}, false, false, "Wearing this ring will permit you to see through nearby walls and doors, within a radius determined by the level of the ring. A cursed ring of clairvoyance will blind you to your immediate surroundings."},
	{"stealth",			itemGems[2], "",	1,	800,	0,{1,3,1}, false, false, "Enemies will be less likely to notice you if you wear this ring. Staying motionless and lurking in the shadows will make you even harder to spot. At very high levels, even enemies giving chase may sometimes lose track of you. Cursed rings of stealth will alert enemies who might otherwise not have noticed your presence."},
	{"regeneration",	itemGems[3], "",	1,	750,	0,{1,3,1}, false, false, "This ring increases the body's regenerative properties, allowing one to recover lost health at an accelerated rate. Cursed rings will decrease or even halt one's natural regeneration."},
	{"transference",	itemGems[4], "",	1,	750,	0,{1,3,1}, false, false, "Landing a melee attack while wearing this ring will cause a proportion of the inflicted damage to transfer to benefit of the attacker's own health. Cursed rings will cause you to lose health with each attack you land."},
	{"light",			itemGems[5], "",	1,	600,	0,{1,3,1}, false, false, "This ring subtly enhances your vision, enabling you to see farther in the dimming light of the deeper dungeon levels. It will not make you more visible to enemies."},
	{"awareness",		itemGems[6], "",	1,	700,	0,{1,3,1}, false, false, "Wearing this ring will allow the wearer to notice hidden secrets -- traps and secret doors -- without taking time to search. Cursed rings of awareness will dull your senses, making it harder to notice secrets even when actively searching for them."},
	{"wisdom",			itemGems[7], "",	1,	700,	0,{1,3,1}, false, false, "Your staffs will recharge at an accelerated rate in the energy field that radiates from this ring. Cursed rings of wisdom will instead cause your staffs to recharge more slowly."},
};

itemTable charmTable[NUMBER_CHARM_KINDS] = {
	{"health",          "", "",	1,	900,	0,{1,2,1}, true, false, "This handful of dried bloodwort and mandrake root has been bound together with leather cord and imbued with a powerful healing magic."},
	{"protection",		"", "",	1,	800,	0,{1,2,1}, true, false, "Four copper rings have been joined into a tetrahedron. The construct is oddly warm to the touch."},
	{"haste",           "", "",	1,	750,	0,{1,2,1}, true, false, "Various animals have been etched into the surface of this brass bangle. It emits a barely audible hum."},
	{"fire immunity",	"", "",	1,	750,	0,{1,2,1}, true, false, "Eldritch flames flicker within this polished crystal bauble."},
	{"invisibility",	"", "",	1,	700,	0,{1,2,1}, true, false, "This intricate figurine depicts a strange humanoid creature. It has a face on both sides of its head, but all four eyes are closed."},
	{"telepathy",		"", "",	1,	700,	0,{1,2,1}, true, false, "Seven tiny glass eyes roll freely within this glass sphere. Somehow, they always come to rest facing outward."},
	{"levitation",      "", "",	1,	700,	0,{1,2,1}, true, false, "Sparkling dust and fragments of feather waft and swirl endlessly inside this small glass sphere."},
	{"shattering",      "", "",	1,	700,	0,{1,2,1}, true, false, "This turquoise crystal, fixed to a leather lanyard, hums with arcane energy."},
//    {"fear",            "", "",	1,	700,	0,{1,2,1}, true, false, "When you gaze into the murky interior of this obsidian cube, you feel as though something predatory is watching you."},
	{"teleportation",   "", "",	1,	700,	0,{1,2,1}, true, false, "The surface of this nickel sphere has been etched with a perfect grid pattern. Somehow, the squares of the grid are all exactly the same size."},
	{"recharging",      "", "",	1,	700,	0,{1,2,1}, true, false, "A strip of bronze has been wound around a rough wooden sphere. Each time you touch it, you feel a tiny electric shock."},
	{"negation",        "", "",	1,	700,	0,{1,2,1}, true, false, "A featureless gray disc hangs from a leather lanyard. When you touch it, your hand briefly goes numb."},
};

#pragma mark Miscellaneous definitions

const color *boltColors[NUMBER_BOLT_KINDS] = {
	&blue,				// teleport other
	&green,				// slow
	&purple,			// polymorph
	&pink,				// negation
	&dominationColor,	// domination
	&beckonColor,		// beckoning
	&rainbow,			// plenty
	&darkBlue,			// invisibility
	&lightningColor,	// lightning
	&fireBoltColor,		// fire
	&poisonColor,		// poison
	&brown,				// tunneling
	&white,				// blinking
	&yellow,			// entrancement
	&forceFieldColor,	// obstruction
	&discordColor,		// discord
	&spectralBladeColor,// conjuration
	&darkRed,			// healing
	&orange,			// haste
	&shieldingColor,	// shielding
};

const char monsterBehaviorFlagDescriptions[32][COLS] = {
	"是隐形的",									// MONST_INVISIBLE
	"不是生物",									// MONST_INANIMATE
	"无法移动",									// MONST_IMMOBILE
	"",                                         // MONST_CARRY_ITEM_100
	"",                                         // MONST_CARRY_ITEM_25
	"总是在寻找猎物",								// MONST_ALWAYS_HUNTING
	"在危险的时候会逃跑",							// MONST_FLEES_NEAR_DEATH
	"",											// MONST_ATTACKABLE_THRU_WALLS
	"攻击时会腐蚀敌人的武器",						// MONST_DEFEND_DEGRADE_WEAPON
	"对物理攻击免疫",								// MONST_IMMUNE_TO_WEAPONS
	"是飞行单位",									// MONST_FLIES
	"行动非常不规律",								// MONST_FLITS
	"对火焰免疫",									// MONST_IMMUNE_TO_FIRE
	"",											// MONST_CAST_SPELLS_SLOWLY
	"不会被网困住",								// MONST_IMMUNE_TO_WEBS
	"能反射法术",									// MONST_REFLECT_4
	"不需要睡眠",									// MONST_NEVER_SLEEPS
	"全身都是火",									// MONST_FIERY
	"",											// MONST_INTRINSIC_LIGHT
	"能再水中移动自如",							// MONST_IMMUNE_TO_WATER
	"不能在陆地上移动",							// MONST_RESTRICTED_TO_LIQUID
	"能潜入水底",									// MONST_SUBMERGES
	"总是和敌人保持着距离",						// MONST_MAINTAINS_DISTANCE
	"",											// MONST_WILL_NOT_USE_STAIRS
	"是魔法生物",									// MONST_DIES_IF_NEGATED
	"",                                         // MONST_MALE
	"",                                         // MONST_FEMALE
	"",                                         // MONST_NOT_LISTED_IN_SIDEBAR
	"仅在被激活时行动"	,							// MONST_GETS_TURN_ON_ACTIVATION
};

const char monsterAbilityFlagDescriptions[33][COLS] = {
	"能让敌人产生幻觉",							// MA_HIT_HALLUCINATE
	"能盗取敌人的物品",							// MA_HIT_STEAL_FLEE
	"会召集 $HISHER 的同胞",						// MA_ENTER_SUMMONS
	"攻击会腐蚀敌人的盔甲",							// MA_HIT_DEGRADE_ARMOR
	"能治疗 $HISHER 的盟友",						// MA_CAST_HEAL
	"能加速 $HISHER 的盟友",						// MA_CAST_HASTE
	"能释放保护法术",								// MA_CAST_PROTECTION
	"能释放召唤法术",								// MA_CAST_SUMMON
	"会朝着目标短距离闪烁",						// MA_CAST_BLINK
	"能释放净化法术",								// MA_CAST_NEGATION
	"能释放闪电",									// MA_CAST_SPARK
	"能释放火球术",								// MA_CAST_FIRE
	"能释放减速",									// MA_CAST_SLOW
	"能释放换乱法术",								// MA_CAST_DISCORD
	"能释放魅惑法术",								// MA_CAST_BECKONING
	"能喷出炙热的的火焰",							// MA_BREATHES_FIRE
	"可以发射黏人的网",							// MA_SHOOTS_WEBS
	"能使用距离攻击",								// MA_ATTACKS_FROM_DISTANCE
	"会束缚 $HISHER 敌人",						// MA_SEIZES
	"攻击附带剧毒",								// MA_POISONS
	"",											// MA_DF_ON_DEATH
	"受到攻击时会分裂成两个",						// MA_CLONE_SELF_ON_DEFEND
	"会使用自杀攻击",								// MA_KAMIKAZE
	"攻击时回复体力",								// MA_TRANSFERENCE
	"攻击时偷取力量",								// MA_CAUSE_WEAKNESS
};

const char monsterBookkeepingFlagDescriptions[32][COLS] = {
	"",											// MONST_WAS_VISIBLE
	"",											// unused
	"",											// MONST_PREPLACED
	"",											// MONST_APPROACHING_UPSTAIRS
	"",											// MONST_APPROACHING_DOWNSTAIRS
	"",											// MONST_APPROACHING_PIT
	"",											// MONST_LEADER
	"",											// MONST_FOLLOWER
	"",											// MONST_CAPTIVE
	"has been immobilized",						// MONST_SEIZED
	"is currently holding $HISHER prey immobile",// MONST_SEIZING
	"is submerged",								// MONST_SUBMERGED
	"",											// MONST_JUST_SUMMONED
	"",											// MONST_WILL_FLASH
	"is anchored to reality by $HISHER summoner",// MONST_BOUND_TO_LEADER
};
