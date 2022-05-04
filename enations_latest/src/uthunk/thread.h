#ifndef __THREAD_H__
#define __THREAD_H__


// commands to the 16-bit DLL entry point func
const int		THRDS_VER = 1;
const int		DLL32_VER = 2;
const int		DLL16_VER = 3;

const int		THRDS_END = 4;
const int		THRDS_START = 5;
const int		THRDS_YIELD = 6;

// commands to the 32-bit entry point it calls.
const int		THRDS_START_AI = 7;

#endif
