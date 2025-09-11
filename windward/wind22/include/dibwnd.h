#ifndef __DIBWND_H__
#define __DIBWND_H__

//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "stdafx.h"

#include "thielen.h"
#include "dib.h"
#include "ptr.h"

//----------------------------- C D I B W n d -----------------------------
// CDIB/window combination
//

class CDIBWnd
#ifdef _DEBUG
    : public CObject
#endif
{

public:

    //---------------------- I n i t i a l i z i n g -----------------------

    CDIBWnd() { ctor(); }
    ~CDIBWnd() { Exit(); }

    BOOL  Init( HWND hWnd, Ptr< CDIB > const&, int cx = 0, int cy = 0 );
    void  Exit();
    BOOL  Size( LPARAM lParam );
    BOOL  Size( int cx, int cy );

    //--------------------------- D r a w i n g ----------------------------

    void  Paint( CRect );

    void  Invalidate( RECT const* pRect = NULL ) const;
    void  Invalidate( int iLeft, int iTop, int iRight, int iBottom ) const;
    void  Update() const;

    //-------------------------- A c c e s s o r s --------------------------

    CSize  GetWinSize() const { return CSize( m_iWinWid, m_iWinHt ); }
    HWND   GetHWND() { return m_hWnd; }
    HDC    GetHDC() { return m_hDC; }
    CDIB*  GetDIB()  const { return m_ptrdib.Value(); }
    CRect  GetRect() const { return CRect( 0, 0, m_iWinWid, m_iWinHt ); }

#ifdef _DEBUG
    virtual void AssertValid() const;
#endif

protected:

    void ctor();

private:

    HWND        m_hWnd;
    HDC         m_hDC;  // we keep a hDC to the window
    int         m_iWinWid; // the size of the window client area
    int         m_iWinHt;
    Ptr< CDIB > m_ptrdib;
    HRESULT     m_hRes;

    LPDIRECTDRAWCLIPPER m_pddclipper;
};

//--------------------------- C D i r t y R e c t s -----------------------------
//
// Handles dirty rects for a CDibWnd

class CDirtyRects
#ifdef _DEBUG
    : public CObject
#endif
{

public:

    enum class RECT_LIST {
        LIST_PAINT_CUR,
        LIST_PAINT_NEXT,
        LIST_PAINT_BOTH,
        LIST_BLT
    };

    CDirtyRects( CDIBWnd* );
    virtual ~CDirtyRects();

    // Copy the LIST_PAINT_NEXT list to the LIST_PAINT_CUR list 
    // and empty the LIST_PAINT_NEXT list

    void UpdateLists();

    // Blt the rects in the LIST_BLT list, to the screen

    void BltRects();

    // Add rect to the cur or cur/next lists. Coalesce with overlapping rects

    virtual void AddRect( CRect const* prect, RECT_LIST eList = RECT_LIST::LIST_PAINT_CUR );

#ifdef _DEBUG
    virtual void AssertValid() const;
#endif

protected:

    void AddRect( CRect const* prect, CRect arect[], int& );


public:

    CDIBWnd* m_pdibwnd;

    int      m_nRectPaintCur;
    int      m_nRectPaintNext;
    int      m_nRectBlt;
    CRect*   m_prectPaintCur;
    CRect*   m_prectPaintNext;
    CRect*   m_prectBlt;
};

#endif

