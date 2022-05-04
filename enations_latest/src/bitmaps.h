//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __BITMAPS_H__
#define __BITMAPS_H__



extern CBmBtnData theBmBtnData;
extern CTextBtnData theTextBtnData, theLargeTextBtnData, theCutTextBtnData;
extern CIcons theIcons;
extern CBitmapLib theBitmaps;

const int ICON_RESEARCH = 0;
const int ICON_GAS = 1;
const int ICON_POWER = 2;
const int ICON_PEOPLE = 3;
const int ICON_FOOD = 4;
const int ICON_CLOCK = 5;
const int ICON_BAR_TEXT = 6;
const int ICON_MATERIALS = 7;
const int ICON_DAMAGE = 8;
const int ICON_DENSITY = 9;
const int ICON_CONSTRUCTION = 10;
const int ICON_BUILD_VEH = 11;
const int ICON_REPAIR_VEH = 12;
const int ICON_BUILD_ROAD = 13;
const int ICON_VEHICLES = 14;

const int DIB_TOOLBAR = 0;
const int DIB_AREA_BAR = 1;
const int DIB_RADAR = 2;
const int DIB_LIST_UNIT_BACK = 3;
const int DIB_LIST_UNIT_BUILDINGS = 4;
const int DIB_LIST_UNIT_VEHICLES = 5;

const int	DIB_BORDER_HORZ = 10;
const int	DIB_BORDER_VERT = 11;

const int DIB_GOLD = 24;
const int DIB_RADAR_BUTTONS = 25;
const int DIB_RADAR_MASK = 26;
const int	DIB_RSRCH_BKGND = 27;
const int	DIB_VEHICLE_BKGND = 28;
const int	DIB_STRUCTURE_BKGND = 29;

const int DIB_WORLD = 30;
const int DIB_WORLD_MASK = 31;
const int DIB_WORLD_BUTTONS = 32;

const int	DIB_STRUCTURE_BTNS_1 = 33;
const int	DIB_STRUCTURE_BTNS_2 = 34;
const int	DIB_STRUCTURE_BTNS_3 = 35;
const int	DIB_VEHICLE_BTNS_1 = 36;
const int	DIB_VEHICLE_BTNS_2 = 37;
const int	DIB_RESEARCH_BTNS = 38;

const COLORREF CLR_CONST = PALETTERGB (255, 255, 255);
const COLORREF CLR_UNIT_BUILD = PALETTERGB (57, 48, 36);
const COLORREF CLR_STATUS_TEXT_OK = PALETTERGB (255, 255, 255);
const COLORREF CLR_STATUS_TEXT_WARN = PALETTERGB (255, 255, 0);
const COLORREF CLR_STATUS_TEXT_BAD = PALETTERGB (255, 0, 0);
const COLORREF CLR_CLOCK = PALETTERGB (255, 255, 255);


#endif
