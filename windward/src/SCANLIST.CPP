//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// scanlist.cpp : class for row-by-row scan conversion
//

#include "stdafx.h"
#include "_windwrd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#include "scanlist.h"

//---------------------------- S c a n L i s t ------------------------------

//--------------------------------------------------------------------------
// ScanList::ScanList
//--------------------------------------------------------------------------
ScanList::ScanList(
	int	iMaxHeight )
	:
		m_iMaxHeight( iMaxHeight ),
		m_piLeft 	( NULL ),
		m_piRight 	( NULL ),
		m_iTopY     ( 0 ),
		m_iBottomY  ( 0 )
{
	ASSERT_STRICT( iMaxHeight > 0 );

	m_piLeft  = new int [ iMaxHeight ];
	m_piRight = new int [ iMaxHeight ];

	m_aiList[ 0 ] = m_piLeft;
	m_aiList[ 1 ] = m_piRight;

	ASSERT_STRICT_VALID( this );
}

//--------------------------------------------------------------------------
// ScanList::~ScanList
//--------------------------------------------------------------------------
ScanList::~ScanList()
{
	delete [] m_piLeft;
	delete [] m_piRight;
}

//--------------------------------------------------------------------------
// ScanList::CheckPoly
//--------------------------------------------------------------------------
BOOL
ScanList::CheckPoly(
	CPoint const	apt[],		// Array of points in clockwise order
	int				iCount )		// # points
{
	for ( int i = 0; i < iCount; ++i )
	{
		int	j1 = i - 1;
		int	j2 = i + 1;

		if ( j1 < 0 )
			j1 = iCount - 1;

		if ( j2 == iCount )
			j2 = 0;

		CPoint	pt1 = CPoint( apt[j1] - apt[i] );
		CPoint	pt2 = CPoint( apt[j2] - apt[i] );

		int	iZ = pt1.y * pt2.x - pt1.x * pt2.y;

		if ( iZ <= 0 )
			return FALSE;
	}

	return TRUE;
}

//--------------------------------------------------------------------------
// ScanList::ScanPoly
//--------------------------------------------------------------------------
void
ScanList::ScanPoly(	
	CPoint const	apt[],		// Array of points in clockwise order
	int				iCount )		// # points
{
	ASSERT_STRICT_VALID( this );
	ASSERT_STRICT( iCount > 2 );
	ASSERT_STRICT( AfxIsValidAddress( apt, iCount * sizeof( CPoint )));

	#ifdef _DEBUG
	CheckPoly( apt, iCount );
	#endif

	m_iTopY   	= apt[0].y;
	m_iBottomY	= apt[0].y;

	for ( int i = 1; i < iCount; ++i )
		if ( apt[i].y < m_iTopY )
			m_iTopY = apt[i].y;
		else if ( apt[i].y > m_iBottomY )
			m_iBottomY = apt[i].y;

	for ( i = 0; i < iCount; ++i )
	{
		int j = i + 1;

		if ( j == iCount )
			j = 0;

		// MakeLine( apt[i], apt[j] );

		int		iSide;
		CPoint	ptTop;
		CPoint	ptBot;

		if ( apt[i].y > apt[j].y )
		{
			iSide = 0;

			ptTop = apt[j];
			ptBot = apt[i];
		}
		else
		{
			iSide = 1;

			ptTop = apt[i];
			ptBot = apt[j];
		}

		int	iDelY	= ptBot.y - ptTop.y;

		if ( 0 == iDelY )
			continue;

		int	iDelX		= ptBot.x - ptTop.x;
		int	iX 		= ptTop.x;
		int * pi 		= m_aiList[ iSide ] + ptTop.y - m_iTopY;
		int	iCount 	= iDelY + 1;

		if ( 0 == iDelX )
		{
			__asm
			{
				; Fill the column with the same x-coordinate

				mov	eax,[iX]
				mov	edi,[pi]
				mov	ecx,[iCount]
				rep	stosd
			}
		}
		else
		{
			__asm
			{
				; Calc fixed-point delta-x

				mov	ebx, [iDelY]		; integer divisor
				mov 	eax, [iDelX]		; integer dividend
				shl	ebx, 16				; fixed-point divisor
				shl	eax, 16				; fixed-point dividend
				cdq							; convert to 64 bits
				shld	edx, eax, 16
				shl	eax, 16
				idiv	ebx					; divide - eax contains now fixed-point delta-x

				; eax	- whole part of delta-x (in ax)
				; ebx	- current x-coordinate
				; ecx	- loop counter
				; edx	- fractional part of delta-x (in upper 2 bytes)
				; esi	- accumulated fractional part of delta-x (in upper 2 bytes)
				; edi	- destination pointer

				; Initialize loop

				mov	edi, [pi]			; destination pointer
				mov	ebx, [iX]			; initial x-coordinate
				mov	ecx, [iCount]		; # of rows
				mov	edx, eax				; assign 16.16 delta-x to edx
				mov	esi, 80000000H		; initial fractional accumulation (1/2 starts in the middle of the pixel)
				sar	eax, 16				; whole part of delta-x
				shl	edx, 16				; fractional part of delta-x stored in upper 2 bytes

				push	ebp					; extra register

				; Can't access stack frame

				; Determine entry point into loop (jump table is easier, but can't dw with inline asm)

				mov	ebp, ecx				; # of rows
				add	ecx, 3				; unrolled loop count is 
				shr	ecx, 2				; 	( count + 3 ) / 4
				bt		ebp, 0				; 1 or 3 extra rows?
				jnc	EvenRows				; no, 2 or 4
				bt		ebp, 1				; 3 extra rows?
				jc		LoopEntry3			; yes
				jmp	LoopEntry1			; 1 extra row

				EvenRows:

				bt		ebp, 1				; 2 extra rows?
				jc		LoopEntry2			; yes

				; Calculate the x-coordinate for each row (loop unrolled 4x)
				; Should be 2 cycles per pixel on a Pentium

				LoopEntry4:

				mov	[edi],	ebx		; assign the x-coordinate (mov/add faster than stosd on Pentium)
				add	esi, 		edx		; add fractional step to accumulated fractional step
				adc	ebx,		eax		; add delta whole step to current x-coord (plus 1 if accumulated fractional step >= 1)
				add	edi,		4 			; bump dest pointer for next x-coordinate

				LoopEntry3:

				mov	[edi],	ebx		; assign the x-coordinate (mov/add faster than stosd on Pentium)
				add	esi, 		edx		; add fractional step to accumulated fractional step
				adc	ebx,		eax		; add delta whole step to current x-coord (plus 1 if accumulated fractional step >= 1)
				add	edi,		4 			; bump dest pointer for next x-coordinate

				LoopEntry2:

				mov	[edi],	ebx		; assign the x-coordinate (mov/add faster than stosd on Pentium)
				add	esi, 		edx		; add fractional step to accumulated fractional step
				adc	ebx,		eax		; add delta whole step to current x-coord (plus 1 if accumulated fractional step >= 1)
				add	edi,		4 			; bump dest pointer for next x-coordinate

				LoopEntry1:

				mov	[edi],	ebx		; assign the x-coordinate (mov/add faster than stosd on Pentium)
				add	esi, 		edx		; add fractional step to accumulated fractional step
				adc	ebx,		eax		; add delta whole step to current x-coord (plus 1 if accumulated fractional step >= 1)
				add	edi,		4 			; bump dest pointer for next x-coordinate

				dec	ecx					; decrement loop counter (dec/jnz faster than loopnz)

				jnz	LoopEntry4			; next iteration - ggtodo: unroll for 486?

				pop	ebp

				; Can access stack frame
			}

//			while ( iCount-- > 0 )
//			{
//				*pi++ = fixX >> 16;
//				fixX += fixDelta;
//			}
		}
	}

	ASSERT_STRICT_VALID( this );
}

//--------------------------------------------------------------------------
// ScanList::ScanPolyFixed
//--------------------------------------------------------------------------
void
ScanList::ScanPolyFixed(	
	CPoint const	apt[],		// Array of points in clockwise order
	int				iCount )		// # points
{
	ASSERT_STRICT_VALID( this );
	ASSERT_STRICT( iCount > 2 );
	ASSERT_STRICT( AfxIsValidAddress( apt, iCount * sizeof( CPoint )));

	#ifdef _DEBUG
	CheckPoly( apt, iCount );
	#endif

	m_iTopY   	= apt[0].y;
	m_iBottomY	= apt[0].y;

	for ( int i = 1; i < iCount; ++i )
		if ( apt[i].y < m_iTopY )
			m_iTopY = apt[i].y;
		else if ( apt[i].y > m_iBottomY )
			m_iBottomY = apt[i].y;

	for ( i = 0; i < iCount; ++i )
	{
		int j = i + 1;

		if ( j == iCount )
			j = 0;

		// MakeLine( apt[i], apt[j] );

		int		iSide;
		CPoint	ptTop;
		CPoint	ptBot;

		if ( apt[i].y > apt[j].y )
		{
			iSide = 0;

			ptTop = apt[j];
			ptBot = apt[i];
		}
		else
		{
			iSide = 1;

			ptTop = apt[i];
			ptBot = apt[j];
		}

		int	iDelY	= ptBot.y - ptTop.y;

		if ( 0 == iDelY )
			continue;

		int	iDelX		= ptBot.x - ptTop.x;
		int	fixX 		= ( ptTop.x << 16 ) + 0x00008000;
//		int	iX 		= ptTop.x;
		int * pi 		= m_aiList[ iSide ] + ptTop.y - m_iTopY;
		int	iCount 	= iDelY + 1;

		if ( 0 == iDelX )
		{
			__asm
			{
				; Fill the column with the same x-coordinate

				mov	eax,[fixX]
				mov	edi,[pi]
				mov	ecx,[iCount]
				rep	stosd
			}
		}
		else
		{
			__asm
			{
				; Calc fixed-point delta-x

				mov	ebx, [iDelY]		; integer divisor
				mov 	eax, [iDelX]		; integer dividend
				shl	ebx, 16				; fixed-point divisor
				shl	eax, 16				; fixed-point dividend
				cdq							; convert to 64 bits
				shld	edx, eax, 16
				shl	eax, 16
				idiv	ebx					; divide - eax contains now fixed-point delta-x

				; eax	- whole part of delta-x (in ax)
				; ebx	- current x-coordinate
				; ecx	- loop counter
				; edx	- fractional part of delta-x (in upper 2 bytes)
				; esi	- accumulated fractional part of delta-x (in upper 2 bytes)
				; edi	- destination pointer

				; Initialize loop

				mov	edi, [pi]			; destination pointer
				mov	ebx, [fixX]			; initial x-coordinate
				mov	ecx, [iCount]		; # of rows

				; Determine entry point into loop (jump table is easier, but can't dw with inline asm)

				mov	edx, ecx				; # of rows
				add	ecx, 3				; unrolled loop count is 
				shr	ecx, 2				; 	( count + 3 ) / 4
				bt		edx, 0				; 1 or 3 extra rows?
				jnc	EvenRows				; no, 2 or 4
				bt		edx, 1				; 3 extra rows?
				jc		LoopEntry3			; yes
				jmp	LoopEntry1			; 1 extra row

				EvenRows:

				bt		edx, 1				; 2 extra rows?
				jc		LoopEntry2			; yes

				; Calculate the x-coordinate for each row (loop unrolled 4x)
				; Should be 2 cycles per pixel on a Pentium

				LoopEntry4:

				mov	[edi],	ebx		; assign the x-coordinate (mov/add faster than stosd on Pentium)
				add	ebx,		eax		; add delta whole step to current x-coord (plus 1 if accumulated fractional step >= 1)
				add	edi,		4 			; bump dest pointer for next x-coordinate

				LoopEntry3:

				mov	[edi],	ebx		; assign the x-coordinate (mov/add faster than stosd on Pentium)
				add	ebx,		eax		; add delta whole step to current x-coord (plus 1 if accumulated fractional step >= 1)
				add	edi,		4 			; bump dest pointer for next x-coordinate

				LoopEntry2:

				mov	[edi],	ebx		; assign the x-coordinate (mov/add faster than stosd on Pentium)
				add	ebx,		eax		; add delta whole step to current x-coord (plus 1 if accumulated fractional step >= 1)
				add	edi,		4 			; bump dest pointer for next x-coordinate

				LoopEntry1:

				mov	[edi],	ebx		; assign the x-coordinate (mov/add faster than stosd on Pentium)
				add	ebx,		eax		; add delta whole step to current x-coord (plus 1 if accumulated fractional step >= 1)
				add	edi,		4 			; bump dest pointer for next x-coordinate

				dec	ecx					; decrement loop counter (dec/jnz faster than loopnz)

				jnz	LoopEntry4			; next iteration - ggtodo: unroll for 486?
			}

//			while ( iCount-- > 0 )
//			{
//				*pi++ = fixX;
//				fixX += fixDelta;
//			}
		}
	}

	ASSERT_STRICT_VALID( this );
}

//-------------------------------------------------------------------------
// ScanList::AssertValid
//-------------------------------------------------------------------------
#ifdef _DEBUG
void ScanList::AssertValid() const
{
	CObject::AssertValid();

	ASSERT_STRICT( 0 <= m_iMaxHeight && m_iMaxHeight < 2000 );

	ASSERT_STRICT( AfxIsValidAddress( m_prowends, m_iMaxHeight * sizeof( RowEnds )));

	ASSERT_STRICT( m_iBottomY >= m_iTopY );
	ASSERT_STRICT( m_iBottomY - m_iTopY < m_iMaxHeight );
}

#endif
