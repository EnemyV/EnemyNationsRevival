/**************************************************************************

    TIMEWING.H - A timing demo for WinG

 **************************************************************************/
/**************************************************************************

    (C) Copyright 1994 Microsoft Corp.  All rights reserved.

    You have a royalty-free right to use, modify, reproduce and 
    distribute the Sample Files (and/or any modified version) in 
    any way you find useful, provided that you agree that 
    Microsoft has no warranty obligations or liability for any 
    Sample Application Files which are modified. 

 **************************************************************************/

/* Menu Items */
#define MENU_OPEN       1
#define MENU_ABOUT      2
#define MENU_EXIT       3

#define MENU_TIMEALL    4
#define MENU_1TO1       5
#define MENU_1TO2       6
#define MENU_1TOALMOST2 7

#define MENU_TIME 50



/****************************************************************************
****************************************************************************/

#ifdef DEBUG
    extern void FAR CDECL dprintf(LPSTR szFormat, ...);
    #define DPF dprintf
#else
    #define DPF ; / ## /
#endif
