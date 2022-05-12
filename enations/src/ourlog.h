#ifndef __OURLOG_H__


const int LOG_VEH_MOVE = 		0x00000008;
const int LOG_AI_ROUTER = 	0x00000010;
const int LOG_HP_ROUTER = 	0x00000020;
const int LOG_AI_MISC = 		0x00000040;
const int LOG_UNIT_STAT = 	0x00000080;
const int LOG_VEH_PATH = 		0x00000100;
const int LOG_TIME = 				0x00000200;
const int LOG_VEH_LOAD = 		0x00000400;
const int LOG_ATTACK =			0x00000800;
const int LOG_TEMP =				0x00001000;	// don't merge with this one


// for asserts
const int ASSERT_VEH_MOVE = 		0x00000040;

#endif
