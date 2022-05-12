/**************************************************************************

    DOGGIE.H - a sprite demo for WinG

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
#define MENU_ABOUT       2
#define MENU_EXIT        4

BOOL TransparentDIBits( BITMAPINFO far *pBufferHeader,
          void huge *pBufferBits,
          int nXOriginDest, int nYOriginDest, void const far *pBits,
          BITMAPINFO const far *pBitmapInfo, int nXOriginSrc, int nYOriginSrc,
          int iUsage, char unsigned TransparentColor );
