//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __EVENT_H__
#define __EVENT_H__

extern COLORREF EventclrOk;
extern COLORREF EventclrWarn;
extern COLORREF EventclrBad;

enum
{

		EVENT_NOTIFY,
		EVENT_WARN,
		EVENT_BAD,
		EVENT_OFF								// if this is the event shown - clear it

};

enum 
{

		EVENT_CONST_LOC,				// pick location to build
		EVENT_CONST_CANT,				// can't start building a building (arrived)
		EVENT_CONST_START,			// start building a building (arrived)
		EVENT_CONST_HALTED,			// building a building
		EVENT_CONST_DONE,				// building completed
		EVENT_REPAIR_DONE,			// building repaired
		EVENT_CONST_UNDER_ATK,	// civ veh under attack

		EVENT_ROAD_START,				// road started
		EVENT_ROAD_HALTED,			// can't build road
		EVENT_ROAD_DONE,				// road completed

		EVENT_BUILD_HALTED,			// building a vehicle
		EVENT_BUILD_DONE,				// vehicle built

		EVENT_MANUF_HALTED,			// manufacturing goods

		EVENT_GOTO_CANT,				// trying to goto a hex it can't go to - STOPPED
		EVENT_BLDG_ENEMY_CLOSE,	// enemy nearby
		EVENT_BLDG_UNDER_ATK,		// building under attack
		EVENT_BLDG_HURTING,			// building 50% destroyed
		EVENT_BLDG_DYING,				// building 90% destroyed
		EVENT_VEH_UNDER_ATK,		// vehicle under attack
		EVENT_ATK_TARGET,				// attacking objective
		EVENT_ATK_DESTROYED,		// objective destroyed

		EVENT_HAVE_MAIL,				// received mail
		EVENT_HAVE_CALL,				// received mail
		EVENT_MINE_EMPTY,				// a mine has run dry

		EVENT_POP_LOW,					// not enough people
		EVENT_GAS_LOW,					// low on gas
		EVENT_FOOD_LOW,					// low on food
		EVENT_POWER_LOW,				// low/out of power
		EVENT_GAS_OUT,					// out of gas
		EVENT_FOOD_OUT,					// out of food

		EVENT_PLAYER_DEAD,			// player died
		EVENT_PLAYER_LEFT,			// player left game - AI taking over
		EVENT_PLAYER_JOINED,		// player joined game - took over from AI

		EVENT_LOW_HOUSING,			// low on apartments
		EVENT_LOW_OFFICE,				// low on office space
		EVENT_NEW_CRANE,				// built a free new crane
		EVENT_NEW_TRUCK,
		EVENT_NEW_RELATIONS,		// changed relations for you

		EVENT_HPR_TRUCKS,			// no trucks available
		EVENT_HPR_NOPICKUP,			// unable to pick up materials
		EVENT_HPR_NOACCESS,			// unable to reach materials
		EVENT_HPR_SEAPORTS,			// not enough seaports
		EVENT_HPR_CARGOSHIP,		// need a cargo ship
		EVENT_HPR_NOREACH,			// unable to reach seaport

		EVENT_CANT_BRIDGE,			// can't build bridges yet
		EVENT_HAVE_RADAR,				// now have radar

		EVENT_ROCKET_CANT,			// can't land rocket here,
		EVENT_EMPT1,
		EVENT_EMPT2,
		EVENT_EMPT3, // "in case i didn't add enough"
		NUM_EVENTS
};

enum
{

		MEVENT_SELECT_COMBAT,		// select combat veh
		MEVENT_SELECT_CRANE,		// select crane
		MEVENT_SELECT_FACTORY,	// select vehicle factory

		MEVENT_GO_COMBAT,			// goto combat veh
		MEVENT_GO_CRANE,			// goto crane
		MEVENT_BUILD_FACTORY,	// start building vehicle
		MEVENT_RSRCH_DONE,				// completed research

		NUM_MEVENTS

};


#endif
