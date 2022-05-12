#ifndef __SCANLIST_H__
#define __SCANLIST_H__


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "thielen.h"

// scanlist.h : Scan convert a polygon
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

//---------------------------- S c a n L i s t ------------------------------

class ScanList
#ifdef _DEBUG
: public CObject
#endif
{

public:

	static BOOL	CheckPoly( CPoint const apt[], int iCount );

	ScanList( int iMaxHeight );
  ~ScanList();

	void	ScanPoly		 ( CPoint const apt[], int iCount );
	void	ScanPolyFixed( CPoint const apt[], int iCount );

	int	TopY 	 	()				const { return m_iTopY;    	}
	int	BottomY	()				const { return m_iBottomY; 	}
	int	MaxHeight()				const { return m_iMaxHeight; 	}
	int	LeftX		( int iY )	const {
												  ASSERT_STRICT( iY >= m_iTopY );
												  ASSERT_STRICT( iY <= m_iBottomY );
												  return m_piLeft[ iY - m_iTopY ];
												}
	int	RightX	( int iY )	const {
												  ASSERT_STRICT( iY >= m_iTopY );
												  ASSERT_STRICT( iY <= m_iBottomY );
												  return m_piRight[ iY - m_iTopY ];
												}

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

protected:

	void	MakeLine( CPoint ptTop, CPoint ptBottom );

private:

	ScanList( const ScanList & );
	void operator = ( const ScanList & );

public:

	int	* m_aiList[ 2 ];
	int	* m_piLeft;
	int	* m_piRight;
	int	  m_iTopY;
	int	  m_iBottomY;
	int	  m_iMaxHeight;
};

#endif
