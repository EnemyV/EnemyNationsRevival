#ifndef __SCANLIST_H__
#define __SCANLIST_H__


//---------------------------------------------------------------------------
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc.
//  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "thielen.h"

// scanlist.h : Scan convert a polygon
//

// Phase 1g step 2: the original PCH guard checked __AFXWIN_H__ to make sure
// stdafx.h was the first include. Under the stub gates we don't include
// afxwin.h at all, so accept either the legacy marker or windows.h's _WINDOWS_.
#if !defined(__AFXWIN_H__) && !defined(_WINDOWS_)
#error include 'stdafx.h' before including this file for PCH
#endif

//---------------------------- S c a n L i s t ------------------------------

//#ifdef _DEBUG
// class ScanList: public CObject
//#else
//class ScanList
//#endif
class ScanList {
public:

    ScanList() = default;
    ScanList( int iMaxHeight );
    ~ScanList();

    static BOOL CheckPoly( CPoint const apt[], int iCount );

    void ScanPoly( CPoint const apt[], int iCount );
    void ScanPolyNew( CPoint const apt[], int iCount );
    void ScanPolyOld( CPoint const apt[], int iCount );
    void ScanPolyFixed( CPoint const apt[], int iCount );
    void ScanPolyFixedOld( CPoint const apt[], int iCount );
    void ScanPolyFixedNew( CPoint const apt[], int iCount );

    int TopY() const { return m_iTopY; }
    int BottomY() const { return m_iBottomY; }
    int MaxHeight() const { return m_iMaxHeight; }
    int LeftX( int iY ) const {
        ASSERT_STRICT( iY >= m_iTopY );
        ASSERT_STRICT( iY <= m_iBottomY );
        return m_piLeft[iY - m_iTopY];
    }
    int RightX( int iY ) const {
        ASSERT_STRICT( iY >= m_iTopY );
        ASSERT_STRICT( iY <= m_iBottomY );
        return m_piRight[iY - m_iTopY];
    }

#ifdef _DEBUG
    void AssertValid() const;
#endif

//protected:
// 
//    void MakeLine( CPoint ptTop, CPoint ptBottom );  // does not exist!

private:

    ScanList( const ScanList& ) = default;
    ScanList& operator=( const ScanList& ) = default;

public:

    int* m_aiList[2];
    int* m_piLeft;
    int* m_piRight;
    int  m_iTopY;
    int  m_iBottomY;
    int  m_iMaxHeight;
};

#endif
