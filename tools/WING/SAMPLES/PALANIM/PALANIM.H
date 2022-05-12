/**************************************************************************

    PALANIM.H - A palette animation demo for WinG

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
#define MENU_ABOUT      1
#define MENU_EXIT       2

#define MENU_RED        3
#define MENU_GREEN      4
#define MENU_BLUE       5
#define MENU_STATIC     6

#define MENU_PALETTE    7
#define MENU_CURTAIN    8

/****************************************************************************
****************************************************************************/

#ifdef DEBUG
    extern void FAR CDECL dprintf(LPSTR szFormat, ...);
    #define DPF dprintf
#else
    #define DPF ; / ## /
#endif
