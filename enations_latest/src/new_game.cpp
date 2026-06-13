//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// new_game.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "new_game.h"
#include "SDL2CreateStatus.h"
#include "GameWindow.h"
#include "player.h"
#include "error.h"
#include "bitmaps.h"
#include "help.h"
#include "sfx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


// find the index who's data ptr matches
int ListBoxFindDataPtr(CListBox const &lst, void const *pData) {

    int iMax = lst.GetCount();
    for (int iInd = 0; iInd < iMax; iInd++)
        if (lst.GetItemDataPtr(iInd) == pData)
            return (iInd);

    return (-1);
}


LRESULT CALLBACK PerBarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
// scratch DIB for painting (this works well because we only use this in 1 place)
    static CStatInst si;
    static CStatData sd;

    switch (uMsg) {
        case WM_CREATE : {
            CMmio *pMmio = theDataFile.OpenAsMMIO("misc", "MISC");
            pMmio->DescendRiff('M', 'I', 'S', 'C');

            // get the proper bitmap
            pMmio->DescendList('C', 'R', theApp.m_szOtherBPS[0], theApp.m_szOtherBPS[1]);

            // load it
            sd.Init(pMmio);
            si.m_pStatData = &sd;
            si.SetBack(theBitmaps.GetByIndex(DIB_GOLD), CPoint(0, 0));

            delete pMmio;
            break;
        }

        case WM_ERASEBKGND :
            return (1);

        case WM_PAINT : {
            // check pDib
            CRect rect;
            ::GetClientRect(hWnd, &rect);
            si.SetSize(rect);

            // set the percentage
            si.SetPer(::GetWindowWord(hWnd, 0));

            // draw the frame -- raw HDC pattern (no MFC CWnd* needed)
            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(hWnd, &ps);
            thePal.Paint(hdc);
            CDC* pdc = CDC::FromHandle(hdc);
            si.DrawIcon(pdc);
            thePal.EndPaint(hdc);
            ::EndPaint(hWnd, &ps);
            return (0);
        }
    }

    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}


/////////////////////////////////////////////////////////////////////////////
// CCreateBase

CCreateBase::CCreateBase(int iTyp) {

    ASSERT ((0 <= iTyp) && (iTyp < num_types));
    m_iTyp = iTyp;
    m_iNet = -1;
    m_iJoinUntil = 0;
    m_iNumPlayers = 0;
    m_iWorldType = 0;  // WORLD_DEFAULT (EWorldType in terrain.h)
    m_iRivers = 60;    // river density slider baseline
    m_pAdvNet = NULL;

    memset(&m_ID, 0, sizeof(m_ID));

    // read in the races
    ptheRaces = CRaceDef::Init(ptheRaces);
}

void CCreateBase::CloseAll() {

    ASSERT_VALID (this);

    ClosePick();

    // Destroy SDL2 status dialog
    if (m_psdlStatus != NULL) {
        if (theApp.m_gameWindow)
            theApp.m_gameWindow->SetCreateStatus(nullptr);
        delete m_psdlStatus;
        m_psdlStatus = NULL;
    }
}

void CCreateBase::ShowDlgStatus() {

    if (m_psdlStatus == NULL) {
        m_psdlStatus = new SDL2CreateStatus(theApp.m_gameWindow.get());
        theApp.m_gameWindow->SetCreateStatus(m_psdlStatus);
    }
    m_psdlStatus->Show();
}

void CCreateBase::CreateDlgStatus() {

    if (m_psdlStatus == NULL) {
        m_psdlStatus = new SDL2CreateStatus(theApp.m_gameWindow.get());
        theApp.m_gameWindow->SetCreateStatus(m_psdlStatus);
    }
}

void CCreateBase::HideDlgStatus() {

    if (m_psdlStatus)
        m_psdlStatus->Hide();
}

void CCreateBase::ToWorld() {

    ASSERT_VALID (this);

    if (m_psdlStatus == NULL) {
        m_psdlStatus = new SDL2CreateStatus(theApp.m_gameWindow.get());
        theApp.m_gameWindow->SetCreateStatus(m_psdlStatus);
    }
    m_psdlStatus->Show();

    ClosePick();
}

#ifdef _DEBUG
void CCreateBase::AssertValid() const
{

    CObject::AssertValid ();
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CMultiBase
// CDlgPlayerList removed (Phase 2d). CreatePlyrList / CloseAll are inlined no-ops
// in the header now. AssertValid stays minimal.

#ifdef _DEBUG
void CMultiBase::AssertValid() const
{
    CCreateBase::AssertValid ();
};
#endif


/////////////////////////////////////////////////////////////////////////////
// CCreateNewBase / CCreateLoadBase
// MFC dialogs CDlgPickRace/CDlgPickPlayer removed (Phase 2d) — nothing to
// destroy in ClosePick now; SDL2 dialogs are local DoModal scopes.

void CCreateNewBase::ClosePick() {
}

void CCreateLoadBase::ClosePick() {
}

