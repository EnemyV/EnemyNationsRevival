#ifndef __THREAD_H__
#define __THREAD_H__


// commands to the 16-bit DLL entry point func
const int		THRDS_VER = 1;
const int		DLL32_VER = 2;
const int		DLL16_VER = 3;

const int		THRDS_END = 4;
const int		THRDS_START = 5;
const int		THRDS_YIELD = 6;

const int		THRDS_PRI_LOW = 7;
const int		THRDS_PRI_NORMAL = 8;
const int		THRDS_PRI_HIGH = 9;
const int		THRDS_END_HDL = 10;
const int		THRDS_ID = 11;
const int		THRDS_PAUSE = 12;
const int		THRDS_RESUME = 13;

// commands to the 32-bit entry point it calls.
const int		THRDS_START_AI = 14;

#endif
