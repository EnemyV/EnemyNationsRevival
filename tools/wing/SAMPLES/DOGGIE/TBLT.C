/**************************************************************************

    TBLT.C - transparent blt

 **************************************************************************/
/**************************************************************************

    (C) Copyright 1994 Microsoft Corp.  All rights reserved.

    You have a royalty-free right to use, modify, reproduce and 
    distribute the Sample Files (and/or any modified version) in 
    any way you find useful, provided that you agree that 
    Microsoft has no warranty obligations or liability for any 
    Sample Application Files which are modified. 

 **************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <wing.h>

#include "..\utils\utils.h"

extern  void PASCAL far TransCopyDIBBits( WORD DestSelector, DWORD DestOffset,
  void const far *pSource, DWORD Width, DWORD Height, long DestWidth,
  long SourceWidth, char unsigned TransparentColor );


/*----------------------------------------------------------------------------

This function ignores the source origin and blts the entire source.

*/

BOOL TransparentDIBits( BITMAPINFO far *pBufferHeader,
  void huge *pBufferBits,
  int nXOriginDest, int nYOriginDest, void const far *pBits,
  BITMAPINFO const far *pBitmapInfo, int nXOriginSrc, int nYOriginSrc,
  int iUsage, char unsigned TransparentColor )
{
  BOOL ReturnValue = FALSE;

  int NewDestinationXOrigin;
  int NewDestinationYOrigin;
  int DestinationDeltaX;
  int DestinationDeltaY;
  int Width;
  int Height;
  int NewSourceXOrigin;
  int NewSourceYOrigin;
  int Orientation = 1;
  int DestinationHeight = DibHeight(&pBufferHeader->bmiHeader);
  int SourceWidth = DibWidth(&pBitmapInfo->bmiHeader);
  int SourceHeight = DibHeight(&pBitmapInfo->bmiHeader);

  char unsigned huge *pSource;
  WORD DestSelector;
  DWORD DestOffset;

  RECT SourceRectangle = { nXOriginDest, nYOriginDest,
    nXOriginDest + SourceWidth, nYOriginDest + SourceHeight };

  RECT DestinationRectangle;

  RECT ClippedRectangle;

  if(DestinationHeight < 0)   // check for top-down DIB
  {
    Orientation = -1;
    DestinationHeight = -DestinationHeight;
  }

  DestinationRectangle.top = 0;
  DestinationRectangle.left = 0;
  DestinationRectangle.bottom = DestinationHeight;
  DestinationRectangle.right = DibWidth(&pBufferHeader->bmiHeader);

  // intersect the destination rectangle with the destination DIB

  if(IntersectRect(&ClippedRectangle,&SourceRectangle,
    &DestinationRectangle))
  {
    // default delta scan to width in bytes
    long DestinationScanDelta = DibWidthBytes(&pBufferHeader->bmiHeader);


    NewDestinationXOrigin = ClippedRectangle.left;
    NewDestinationYOrigin = ClippedRectangle.top;

    DestinationDeltaX = NewDestinationXOrigin - nXOriginDest;
    DestinationDeltaY = NewDestinationYOrigin - nYOriginDest;

    Width = ClippedRectangle.right - ClippedRectangle.left;
    Height = ClippedRectangle.bottom - ClippedRectangle.top;

    pSource = (char unsigned huge *)pBits;

    NewSourceXOrigin = DestinationDeltaX;
    NewSourceYOrigin = DestinationDeltaY;

    pSource += ((long)SourceHeight - (NewSourceYOrigin + Height)) *
      (long)DibWidthBytes(&pBitmapInfo->bmiHeader)
      + NewSourceXOrigin;

    // now we'll calculate the starting destination pointer taking into
    // account we may have a top-down destination

    DestSelector = HIWORD(pBufferBits);

    if(Orientation < 0)
    {
      // destination is top-down

      DestinationScanDelta *= -1;

      DestOffset = (long)(NewDestinationYOrigin + Height - 1) *
        (long)DibWidthBytes(&pBufferHeader->bmiHeader) +
        NewDestinationXOrigin;
    }
    else
    {
      // destination is bottom-up

      DestOffset = ((long)DestinationHeight -
        (NewDestinationYOrigin + Height))
        * (long)DibWidthBytes(&pBufferHeader->bmiHeader) +
        NewDestinationXOrigin;
    }

    TransCopyDIBBits(DestSelector,DestOffset,pSource,Width,Height,
      DestinationScanDelta,
      DibWidthBytes(&pBitmapInfo->bmiHeader),
      TransparentColor);

    ReturnValue = TRUE;
  }

  return ReturnValue;
}

