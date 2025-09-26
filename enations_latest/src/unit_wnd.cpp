//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// vehicle.cpp : vehicles
//

#include "stdafx.h"

#include <blt.h>

#include "unit_wnd.h"
#include "lastplnt.h"
#include "player.h"
#include "error.h"
#include "event.h"
#include "CHPRoute.hpp"
#include "bitmaps.h"
#include "area.h"
#include "sfx.h"

#include "terrain.inl"
#include "ui.inl"
#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"
#include "msgs.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


/////////////////////////////////////////////////////////////////////////////
// CWndListUnits

CWndListUnits::CWndListUnits() {

    m_pDib = m_pDibBack = m_pDibUnit = NULL;
    m_iStatHt = 28;
}

BEGIN_MESSAGE_MAP(CWndListUnits, CWndBase)
                    //{{AFX_MSG_MAP(CWndListUnits)
                    ON_WM_CREATE()
                    ON_WM_SIZE()
                    ON_WM_MEASUREITEM()
                    ON_WM_SYSCOMMAND()
                    ON_WM_DRAWITEM()
                    ON_WM_PAINT()
                    ON_WM_DESTROY()
                    ON_WM_ERASEBKGND()
                    ON_WM_GETMINMAXINFO()
                    ON_LBN_SELCHANGE (101, OnLbnClk)
                    ON_LBN_DBLCLK (101, OnLbnDblClk)
                    ON_WM_COMPAREITEM()
                    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWndListUnits message handlers

int CWndListUnits::OnCreate(LPCREATESTRUCT lpCreateStruct) {

    if (CWndBase::OnCreate(lpCreateStruct) == -1)
        return -1;

    // button background
    m_pDibBack = theBitmaps.GetByIndex(DIB_LIST_UNIT_BACK);

    m_iStatHt = theIcons.GetByIndex(ICON_DAMAGE)->m_cyBack;

    // create the list box
    CRect rect;
    GetClientRect(&rect);
    m_ListBox.Create(WS_CHILD | WS_VISIBLE | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | LBS_SORT |
                     LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_MULTIPLESEL | LBS_EXTENDEDSEL,
                     rect, this, 101);

    return 0;
}

void CWndListUnits::OnGetMinMaxInfo(MINMAXINFO FAR *lpMMI) {

    if (theApp.m_wndBar.m_hWnd != NULL) {
        CRect rect;
        theApp.m_wndBar.GetWindowRect(&rect);
        lpMMI->ptMaxTrackSize.y = __min (lpMMI->ptMaxTrackSize.y, rect.top);
        lpMMI->ptMaxSize.y = __min (lpMMI->ptMaxSize.y, rect.top);
    }

    CWndBase::OnGetMinMaxInfo(lpMMI);
}

void CWndListUnits::OnPaint() {

    ASSERT_VALID (this);

    CPaintDC dc(this); // device context for painting
    thePal.Paint(dc.m_hDC);

    // paint grey for behind scroll bar
    CRect rect;
    GetClientRect(&rect);

    CDIB *pDibBkgnd = theBitmaps.GetByIndex(DIB_GOLD);

    CRect rectSrc = CGlobalSubClass::GetBackgroundSrcRect(rect, rect, pDibBkgnd->GetRect());

    pDibBkgnd->StretchBlt(dc, rect, rectSrc);
    //	GG pDibBkgnd->StretchBlt ( dc, rect, pDibBkgnd->GetRect () );

    thePal.EndPaint(dc.m_hDC);
    // Do not call CWndBase::OnPaint() for painting messages
}

void CWndListUnits::OnSysCommand(UINT nID, LPARAM lParam) {

    ASSERT_VALID (this);

    // for minimize hide it
    if ((nID == SC_MINIMIZE) || (nID == SC_CLOSE)) {
        ShowWindow(SW_HIDE);
        return;
    }

    CWndBase::OnSysCommand(nID, lParam);
}

void CWndListUnits::OnSize(UINT nType, int cx, int cy) {

    ASSERT_VALID (this);

    CWndBase::OnSize(nType, cx, cy);

    // re-size the listbox
    if (m_ListBox.GetSafeHwnd() == NULL)
        return;
    m_ListBox.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

    // bitmap for painting
    CRect rect;
    m_ListBox.GetClientRect(&rect);
    delete m_pDib;
    m_pDib = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                      ptrthebltformat->GetDirection(), rect.Width(), 64);
}

void CWndListUnits::OnMeasureItem(int, LPMEASUREITEMSTRUCT lpMIS) {

    ASSERT_VALID (this);

    lpMIS->itemHeight = 64;
}

BOOL CWndListUnits::OnEraseBkgnd(CDC *) {

    return (1);
}

// by type first, ID second (so oldest always first in a type)
int CWndListUnits::OnCompareItem(int nIDCtl, LPCOMPAREITEMSTRUCT lpCis) {

    // put bogus data last, 0 on both bogus
    if (lpCis->itemID1 < 0) {
        TRAP();
        return (lpCis->itemID2 < 0 ? 0 : 1);
    }
    if (lpCis->itemID2 < 0) {
        TRAP();
        return (-1);
    }
    if (lpCis->itemData1 == 0) {
        TRAP();
        return (lpCis->itemData2 == 0 ? 0 : 1);
    }
    if (lpCis->itemData2 == 0) {
        TRAP();
        return (-1);
    }

    int iTyp1, iTyp2;
    CUnit *pUnit = (CUnit *) lpCis->itemData1;
    ASSERT_VALID (pUnit);
    if (pUnit->GetUnitType() == CUnit::vehicle)
        iTyp1 = ((CVehicle *) pUnit)->GetData()->GetType();
    else if (pUnit->GetUnitType() == CUnit::building)
        iTyp1 = ((CBuilding *) pUnit)->GetData()->GetType();
    else
        iTyp1 = -1;

    pUnit = (CUnit *) lpCis->itemData2;
    ASSERT_VALID (pUnit);
    if (pUnit->GetUnitType() == CUnit::vehicle)
        iTyp2 = ((CVehicle *) pUnit)->GetData()->GetType();
    else if (pUnit->GetUnitType() == CUnit::building)
        iTyp2 = ((CBuilding *) pUnit)->GetData()->GetType();
    else
        iTyp2 = -1;

    if (iTyp1 != iTyp2)
        return (iTyp1 < iTyp2 ? -1 : 1);

    return (((CUnit *) lpCis->itemData1)->GetID() < ((CUnit *) lpCis->itemData2)->GetID() ? -1 : 1);
}

void CWndListUnits::OnDrawItem(int, LPDRAWITEMSTRUCT pDis) {

    ASSERT_VALID (this);

    if (ODT_BUTTON == pDis->CtlType)    // GG
    {
        Default();
        return;
    }

    if ((int) pDis->itemID < 0)
        return;

    // Win32s bug
    if (pDis->itemData == -1) {
        ASSERT (iWinType == W32s);
        ::ExtTextOut(pDis->hDC, pDis->rcItem.left, pDis->rcItem.top,
                     ETO_CLIPPED, &(pDis->rcItem), "Win32s bug requires this", 24, NULL);
        return;
    }

    // set the palette
    thePal.Paint(pDis->hDC);

    // draw the button
    // set U.L. to 0,0 (for our CDIB)
    CRect rect(pDis->rcItem);
    rect.OffsetRect(-rect.left, -rect.top);

    // draw the background (center if smaller)
    CRect rSrc(0, 0, m_pDibBack->GetWidth(), m_pDibBack->GetHeight() / 2);
    if (pDis->itemState & ODS_SELECTED)
        rSrc.OffsetRect(0, m_pDibBack->GetHeight() / 2);
    m_pDibBack->StretchBlt(m_pDib, rect, rSrc);

    // if not a unit - draw & done
    if ((pDis->itemID < 0) || (pDis->itemData == NULL)) {
        m_pDib->BitBlt(pDis->hDC, &(pDis->rcItem), CPoint(0, 0));
        thePal.EndPaint(pDis->hDC);
        return;
    }

    // if we're a unit - draw it
    CUnit *pUnit = (CUnit *) pDis->itemData;
    ASSERT_VALID (pUnit);
    int iTyp;
    if (pUnit->GetUnitType() == CUnit::vehicle)
        iTyp = ((CVehicle *) pUnit)->GetData()->GetType();
    else if (pUnit->GetUnitType() == CUnit::building)
        iTyp = ((CBuilding *) pUnit)->GetData()->GetType();
    else {
        m_pDib->BitBlt(pDis->hDC, &(pDis->rcItem), CPoint(0, 0));
        thePal.EndPaint(pDis->hDC);
        return;
    }

    CRect rDest(4, 0, __min (88, m_pDibUnit->GetWidth()), 64);
    CPoint ptSrc(20, 64 * iTyp);
    rDest.right = __min (rDest.right, rect.Width());
    m_pDibUnit->TranBlt(m_pDib, rDest, ptSrc);

    rect.left = rDest.right + 4;
    rect.right -= 4;

    // may be out of room
    if (rect.Width() > 0) {
        // now the text
        rect.top = 4;
        rect.bottom = 60 - m_iStatHt - 4;
        CDC *pDc = CDC::FromHandle(m_pDib->GetDC());
        if (pDc == NULL) {
            thePal.EndPaint(pDis->hDC);
            return;
        }
        thePal.Paint(pDc->m_hDC);
        pDc->SetBkMode(TRANSPARENT);
        CFont *pOldFont = pDc->SelectObject(&theApp.TextFont());

        CString sText = pUnit->GetData()->GetDesc();
        // put dest/location if dest is a building
        if (pUnit->GetUnitType() == CUnit::vehicle) {
            sText += " ";
            CVehicle *pVeh = (CVehicle *) pUnit;
            if (pVeh->GetData()->IsTransport()) {
                if (!pVeh->IsHpControl())
                    sText += CTransportData::m_sAuto;
                else if (pVeh->GetEvent() == CVehicle::route)
                    sText += CTransportData::m_sRoute;
            }

            CBuilding *pBldg = theBuildingHex.GetBuilding(pVeh->GetPtHead());
            if ((pBldg == NULL) || (pVeh->GetHexOwnership()))
                pBldg = theBuildingHex.GetBuilding(pVeh->GetHexDest());
            if ((pBldg != NULL) && (pBldg->GetOwner()->IsMe()))
                sText += "[" + pBldg->GetData()->GetDesc() + "]";
            else if (pVeh->GetData()->IsTransport()) {
                if (pVeh->GetRouteMode() == CVehicle::stop)
                    sText += CTransportData::m_sIdle;
                else
                    sText += CTransportData::m_sTravel;
            }
        }

        // if text too long, do the ... thing
        CRect rFit;
        BOOL bShrink = FALSE;
        while (TRUE) {
            rFit = rect;
            pDc->DrawText(sText, -1, &rFit, DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_VCENTER);
            if (rFit.right < rect.right)
                break;
            if (sText.GetLength() < 5)
                break;
            sText.ReleaseBuffer(sText.GetLength() - 1);
            bShrink = TRUE;
        }
        if (bShrink) {
            sText.ReleaseBuffer(sText.GetLength() - 2);
            sText += "...";
        }

        rect.OffsetRect(1, 1);
        pDc->SetTextColor(RGB (0, 0, 0));
        pDc->DrawText(sText, -1, &rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        rect.OffsetRect(-2, -2);
        pDc->SetTextColor(RGB (255, 255, 255));
        pDc->DrawText(sText, -1, &rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

        // now the status
        rect.top = 60 - m_iStatHt;
        rect.bottom = 60;
        _UnitShowStatus(FALSE, pUnit, pDc, rect, m_pDibBack, CPoint(0, 0));

        pDc->SelectObject(pOldFont);
        thePal.EndPaint(pDc->m_hDC);
    }

    // BLT to the screen DC
    thePal.Paint(pDis->hDC);
    m_pDib->BitBlt(pDis->hDC, &(pDis->rcItem), CPoint(0, 0));

    thePal.EndPaint(pDis->hDC);
    if (m_pDib->IsBitmapSelected())
        m_pDib->ReleaseDC();
}

void CWndListUnits::AddToList(CUnit *pUnit) {

    ASSERT_VALID (this);
    ASSERT_VALID (pUnit);

    if (m_ListBox.m_hWnd == NULL)
        return;

    int iInd = m_ListBox.AddString((LPCSTR) pUnit);
    m_ListBox.SetItemDataPtr(iInd, (void *) pUnit);
}

int CWndListUnits::FindItem(CUnit const *pUnit) {

    ASSERT_VALID (this);
    ASSERT_VALID (pUnit);

    if (m_ListBox.m_hWnd == NULL)
        return (-1);

    int iNum = m_ListBox.GetCount();
    for (int iInd = 0; iInd < iNum; iInd++)
        if (m_ListBox.GetItemDataPtr(iInd) == (void *) pUnit)
            return (iInd);

    return (-1);
}

void CWndListUnits::OnDestroy() {

    CWndBase::OnDestroy();

    delete m_pDib;
    m_pDib = NULL;
}

void CWndListUnits::OnLbnClk() {

    ASSERT_VALID (this);

    CWndArea *pWndArea = theAreaList.GetTop();
    if (pWndArea == NULL)
        return;
    ASSERT_VALID (pWndArea);

    // all selected units need to be selected in the area window
    pWndArea->m_lstUnits.RemoveAllUnits(FALSE);
    int iNum = m_ListBox.GetSelCount();
    if (iNum > 0) {
        int *pSel = new int[iNum + 2];
        iNum = m_ListBox.GetSelItems(iNum + 2, pSel);
        int *pSelOn = pSel;
        while (iNum--) {
            CUnit *pUnit = (CUnit *) m_ListBox.GetItemData(*pSelOn);
            if (pUnit != NULL) {
                ASSERT_VALID (pUnit);
                pWndArea->m_lstUnits.AddUnit(pUnit, FALSE);
            }
            pSelOn++;
        }
        delete[] pSel;
    }

    // fix up the area window global data
    if (pWndArea->m_lstUnits.GetCount() == 1)
        pWndArea->m_pUnit = pWndArea->m_lstUnits.GetHead();
    else
        pWndArea->m_pUnit = NULL;

    // set button states
    pWndArea->SetButtonState();

    // repaint it
    pWndArea->InvalidateStatus();
    pWndArea->InvalidateWindow();

    // On Win95 it doesn't always repaint otherwise
    SetRedraw(TRUE);
    InvalidateRect(NULL, FALSE);
}

void CWndListUnits::OnLbnDblClk() {

    ASSERT_VALID (this);

    // first we find which vehicle it is
    int iIndex = m_ListBox.GetCurSel();
    if (iIndex < 0)
        return;

    CUnit *pUnit = (CUnit *) m_ListBox.GetItemData(iIndex);
    if (pUnit == NULL)
        return;
    ASSERT_VALID (pUnit);

    m_ListBox.SetCurSel(-1);
    pUnit->ShowWindow();
}


/////////////////////////////////////////////////////////////////////////////
// CWndListBuildings

BEGIN_MESSAGE_MAP(CWndListBuildings, CWndListUnits)
                    //{{AFX_MSG_MAP(CWndListBuildings)
                    ON_WM_CREATE()
                    ON_WM_SIZE()
                    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWndListBuildings message handlers

void CWndListBuildings::Create() {

    // units
    m_pDibUnit = theBitmaps.GetByIndex(DIB_LIST_UNIT_BUILDINGS);

    CString sTitle;
    sTitle.LoadString(IDS_TITLE_BUILDINGS);
    if (CreateEx(0, theApp.m_sWndCls, sTitle, dwPopWndStyle,
                 theApp.GetProfileInt(theApp.m_sResIni, "BuildX", theApp.m_iCol2),
                 theApp.GetProfileInt(theApp.m_sResIni, "BuildY", theApp.m_iRow4),
                 __max (256, theApp.GetProfileInt(theApp.m_sResIni, "BuildEX", theApp.m_iScrnX - theApp.m_iCol2 + 1)),
                 theApp.GetProfileInt(theApp.m_sResIni, "BuildEY", (theApp.m_iRow3 - theApp.m_iRow4 + 1) / 2),
                 theApp.m_pMainWnd->m_hWnd, NULL, NULL) == 0)
        ThrowError(ERR_RES_CREATE_WND);
}

int CWndListBuildings::OnCreate(LPCREATESTRUCT lpCreateStruct) {

    if (CWndListUnits::OnCreate(lpCreateStruct) == -1)
        return -1;

    // if created after game starts
    POSITION pos = theBuildingMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CBuilding *pBldg;
        theBuildingMap.GetNextAssoc(pos, dwID, pBldg);
        if (pBldg->GetOwner()->IsMe())
            AddToList(pBldg);
    }

    // if rebuilding use old pos
    if (theGame.m_wpBldgs.length == 0) {
        // save position
        theGame.m_wpBldgs.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(&(theGame.m_wpBldgs));
    }

    return 0;
}

void CWndListBuildings::OnSize(UINT nType, int cx, int cy) {

    ASSERT_VALID (this);

    CWndListUnits::OnSize(nType, cx, cy);

    // save position
    theGame.m_wpBldgs.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(&(theGame.m_wpBldgs));
}


/////////////////////////////////////////////////////////////////////////////
// CWndListVehicles

BEGIN_MESSAGE_MAP(CWndListVehicles, CWndListUnits)
                    //{{AFX_MSG_MAP(CWndListVehicles)
                    ON_WM_CREATE()
                    ON_WM_SIZE()
                    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWndListVehicles message handlers

void CWndListVehicles::Create() {

    // units
    m_pDibUnit = theBitmaps.GetByIndex(DIB_LIST_UNIT_VEHICLES);

    CString sTitle;
    sTitle.LoadString(IDS_TITLE_VEHICLES);
    int y = (theApp.m_iRow3 + theApp.m_iRow4) / 2 - 1;
    if (CreateEx(0, theApp.m_sWndCls, sTitle, dwPopWndStyle,
                 theApp.GetProfileInt(theApp.m_sResIni, "VehicleX", theApp.m_iCol2),
                 theApp.GetProfileInt(theApp.m_sResIni, "VehicleY", y),
                 __max (256, theApp.GetProfileInt(theApp.m_sResIni, "VehicleEX", theApp.m_iScrnX - theApp.m_iCol2 + 1)),
                 theApp.GetProfileInt(theApp.m_sResIni, "VehicleEY", theApp.m_iRow3 - y + 1),
                 theApp.m_pMainWnd->m_hWnd, NULL, NULL) == 0)
        ThrowError(ERR_RES_CREATE_WND);
}

int CWndListVehicles::OnCreate(LPCREATESTRUCT lpCreateStruct) {

    if (CWndListUnits::OnCreate(lpCreateStruct) == -1)
        return -1;

    // if created after game starts
    POSITION pos = theVehicleMap.GetStartPosition();
    while (pos != NULL) {
        DWORD dwID;
        CVehicle *pVeh;
        theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
        if (pVeh->GetOwner()->IsMe())
            AddToList(pVeh);
    }

    // if rebuilding use old pos
    if (theGame.m_wpVehicles.length == 0) {
        // save position
        theGame.m_wpVehicles.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(&(theGame.m_wpVehicles));
    }

    return 0;
}

void CWndListVehicles::OnSize(UINT nType, int cx, int cy) {

    ASSERT_VALID (this);

    CWndListUnits::OnSize(nType, cx, cy);

    // save position
    theGame.m_wpVehicles.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(&(theGame.m_wpVehicles));
}


/////////////////////////////////////////////////////////////////////////////
// CUnitButton

CUnitButton::CUnitButton() {

    m_pDib = m_pBackDib = m_pBtnDib = m_pOvrlyDib = NULL;
    m_iOvrlyNum = 0;
    m_cState = 0;
}

BOOL CUnitButton::Create(char const *psText, CRect &rect, CWnd *pPar, int ID, int IDgroup, CDIB *pBack, CDIB *pBtn,
                         CDIB *pOvrly) {

    m_pDib = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                      CBLTFormat::DIR_BOTTOMUP, rect.Width(), rect.Height());
    m_pBackDib = pBack;
    m_pBtnDib = pBtn;
    m_pOvrlyDib = pOvrly;
    m_IDGroup = IDgroup;

    return (CButton::Create(psText, BS_PUSHBUTTON | BS_OWNERDRAW | WS_CHILD | WS_VISIBLE, rect, pPar, ID));
}

BEGIN_MESSAGE_MAP(CUnitButton, CButton)
                    //{{AFX_MSG_MAP(CUnitButton)
                    ON_WM_DESTROY()
                    ON_WM_ERASEBKGND()
                    ON_WM_LBUTTONDOWN()
                    ON_WM_LBUTTONDBLCLK()
                    ON_WM_PALETTECHANGED()
                    ON_WM_QUERYNEWPALETTE()
                    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CUnitButton::OnDestroy() {

    delete m_pDib;
    m_pDib = NULL;

    CButton::OnDestroy();
}

BOOL CUnitButton::OnEraseBkgnd(CDC *) {

    return (1);
}

void CUnitButton::OnLButtonDown(UINT nFlags, CPoint point) {

    // toggle it down
    if (!(m_cState & 0x01)) {
        m_cState = 0x01;
        theMusicPlayer.PlayForegroundSound(SOUNDS::GetID(SOUNDS::button), SFXPRIORITY::selected_pri);

        // tell the parent
        GetParent()->PostMessage(MSG_BTN_CLICKED, 0, GetDlgCtrlID());
    }

    CButton::OnLButtonDown(nFlags, point);
}

void CUnitButton::OnLButtonDblClk(UINT nFlags, CPoint point) {

    // toggle it down
    m_cState = 0x01;

    // tell the parent
    GetParent()->PostMessage(MSG_BTN_DBLCLK, 0, GetDlgCtrlID());

    CButton::OnLButtonDblClk(nFlags, point);
}

void CUnitButton::SetNum(int iNum) {

    m_iOvrlyNum = iNum;
}

void CUnitButton::SetToggleState(BOOL bDown) {

    m_cState = bDown ? 0x01 : 0;
    InvalidateRect(NULL);
}

void CUnitButton::OnPaletteChanged(CWnd *pFocusWnd) {
    static BOOL bInFunc = FALSE;

    CButton::OnPaletteChanged(pFocusWnd);

    // Win32s locks up if we do the below code
    if (iWinType == W32s)
        return;

    // stop infinite recursion
    if (bInFunc)
        return;
    bInFunc = TRUE;

    CClientDC dc(this);
    int iRtn = thePal.PalMsg(dc.m_hDC, m_hWnd, WM_PALETTECHANGED, (WPARAM) pFocusWnd->m_hWnd, 0);

    // invalidate the window
    if (iRtn)
        InvalidateRect(NULL);

    SendMessage(WM_NCPAINT, 0, 0);

    bInFunc = FALSE;
}

BOOL CUnitButton::OnQueryNewPalette() {

    if (iWinType == W32s)
        return CUnitButton::OnQueryNewPalette();

    CClientDC dc(this);
    thePal.PalMsg(dc.m_hDC, m_hWnd, WM_QUERYNEWPALETTE, 0, 0);

    SendMessage(WM_NCPAINT, 0, 0);
    return CButton::OnQueryNewPalette();
}

void CUnitButton::DrawItem(LPDRAWITEMSTRUCT pDis) {

    // can we draw yet?
    if (m_pDib == NULL) {
        TRAP();
        return;
    }

    // put up the button art
    CRect rect(m_pDib->GetRect());
    CRect rSrc(m_pBtnDib->GetRect());
    rSrc.right /= 3;
    BOOL bShift = FALSE;
    if (pDis->itemState & (ODS_DISABLED | ODS_GRAYED))
        rSrc.OffsetRect(rSrc.Width() * 2, 0);
    else if ((pDis->itemState & ODS_SELECTED) || (m_cState & 0x01)) {
        rSrc.OffsetRect(rSrc.Width(), 0);
        bShift = TRUE;
    }
    m_pBtnDib->StretchBlt(m_pDib, rect, rSrc);

    // keep it on the face of the button
    rect.InflateRect(-6, -6);

    // if we're a bitmap - blt it in
    if (m_pOvrlyDib != NULL) {
        // shift it if pressed
        if (bShift)
            rect.OffsetRect(2, 2);

        CRect rSrc(0, 64 * m_iOvrlyNum, m_pOvrlyDib->GetWidth(), 64 * m_iOvrlyNum + 64);
        CRect rDest(0, rect.top, m_pOvrlyDib->GetWidth(), rect.bottom);
        if (rect.Width() >= m_pOvrlyDib->GetWidth())
            rDest.OffsetRect((rect.Width() - m_pOvrlyDib->GetWidth()) / 2, 0);
        else
            rDest = rect;
        if (rDest.Height() > rSrc.Height()) {
            rDest.top += (rDest.Height() - rSrc.Height()) / 2;
            rDest.bottom = rDest.top + rSrc.Height();
        }
        m_pOvrlyDib->StretchTranBlt(m_pDib, rDest, rSrc);
    }

    // if we have text
    CString sText;
    GetWindowText(sText);
    if (!sText.IsEmpty()) {
        CDC *pDc = CDC::FromHandle(m_pDib->GetDC());
        if (pDc == NULL)
            return;

        pDc->SetBkMode(TRANSPARENT);
        pDc->SetTextColor(CLR_UNIT_BUILD);
        CFont *pOldFont;
        if (m_pOvrlyDib != NULL)
            pOldFont = pDc->SelectObject(&theApp.CostFont());
        else
            pOldFont = pDc->SelectObject(&theApp.TextFont());

        pDc->SetTextColor(RGB (0, 0, 0));
        int iHt = rect.Height();
        int iWid = rect.Width();
        pDc->DrawText(sText, -1, &rect, DT_CALCRECT | DT_CENTER | DT_NOPREFIX | DT_WORDBREAK);

        if (m_pOvrlyDib != NULL)
            rect.OffsetRect((iWid - rect.Width()) / 2, (iHt - rect.Height()) - 2);
        else
            rect.OffsetRect((iWid - rect.Width()) / 2, (iHt - rect.Height()) / 2);

        pDc->DrawText(sText, -1, &rect, DT_CENTER | DT_NOPREFIX | DT_WORDBREAK | DT_NOPREFIX);
        rect.OffsetRect(-1, -1);
        pDc->SetTextColor(RGB (255, 255, 255));
        pDc->DrawText(sText, -1, &rect, DT_CENTER | DT_NOPREFIX | DT_WORDBREAK | DT_NOPREFIX);

        pDc->SelectObject(pOldFont);
    }

    // BLT to the screen DC
    thePal.Paint(pDis->hDC);
    m_pDib->BitBlt(pDis->hDC, &(pDis->rcItem), CPoint(0, 0));

    if (m_pDib->IsBitmapSelected())
        m_pDib->ReleaseDC();
    thePal.EndPaint(pDis->hDC);
}


/////////////////////////////////////////////////////////////////////////////
// CDlgBuildStructure dialog

const int FIRST_CAT_BUTTON = 100;
const int FIRST_BLDG_BUTTON = 110;

CDlgBuildStructure::CDlgBuildStructure(CWnd *pParent, CVehicle *pVeh)
        : CDialog(CDlgBuildStructure::IDD, pParent) {

    m_pVehPar = pVeh;
    m_pDibBtn = m_pDibBkgnd = NULL;

    //{{AFX_DATA_INIT(CDlgBuildStructure)
    //}}AFX_DATA_INIT
}

void CDlgBuildStructure::DoDataExchange(CDataExchange *pDX) {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgBuildStructure)
    DDX_Control(pDX, IDOK, m_btnBuild);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgBuildStructure, CDialog)
                    //{{AFX_MSG_MAP(CDlgBuildStructure)
                    ON_WM_DESTROY()
                    ON_MESSAGE (MSG_BTN_CLICKED, OnSelChange)
                    ON_MESSAGE (MSG_BTN_DBLCLK, OnDblClk)
                    ON_WM_CREATE()
                    ON_WM_ERASEBKGND()
                    ON_WM_PAINT()
                    ON_WM_DRAWITEM()
                    ON_WM_PALETTECHANGED()
                    ON_WM_QUERYNEWPALETTE()
                    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CDlgBuildStructure::Create(CWnd *pPar) {

    ASSERT_VALID (pPar);

    m_pDibBtn = m_pDibBkgnd = NULL;

    CDialog::Create(IDD, pPar);
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBuildStructure message handlers

const CRect rectText(246, 19, 457, 286);

BOOL CDlgBuildStructure::OnInitDialog() {

    CDialog::OnInitDialog();

    m_pSd = NULL;

    m_sCost.LoadString(IDS_COST);
    m_sHave.LoadString(IDS_HAVE);
    m_sNeed.LoadString(IDS_NEED);
    m_sTime.LoadString(IDS_TIME);
    m_sOper.LoadString(IDS_OPERATING);
    m_sPeople.LoadString(IDS_PEOPLE);
    m_sPower.LoadString(IDS_POWER);

    m_pDibBkgnd = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                           CBLTFormat::DIR_BOTTOMUP, 465, 345);

    m_pDibBtn = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                         CBLTFormat::DIR_BOTTOMUP, 98, 23);

    // set the controls
    CRect rWin, rClnt;
    GetWindowRect(&rWin);
    GetClientRect(&rClnt);
    SetWindowPos(NULL, 0, 0, 465 + rWin.Width() - rClnt.Width(),
                 345 + rWin.Height() - rClnt.Height(), SWP_NOMOVE | SWP_NOZORDER);
    m_btnBuild.SetWindowPos(NULL, 249, 300, 98, 23, SWP_NOZORDER);
    GetDlgItem(IDCANCEL)->SetWindowPos(NULL, 359, 300, 98, 23, SWP_NOZORDER);

    ASSERT(ptrthebltformat.Value());

    // create the buttons
    m_iCatOn = -1;
    CRect rect(11, 22, 115, 71);
    for (int iOn = 0; iOn < 6; iOn++) {
        m_btnCat[iOn].Create(theStructureType.GetDesc(iOn),
                             rect, this, FIRST_CAT_BUTTON + iOn, IDC_BUILD_LIST_CAT,
                             theBitmaps.GetByIndex(DIB_STRUCTURE_BKGND),
                             theBitmaps.GetByIndex(DIB_STRUCTURE_BTNS_1), NULL);
        rect.OffsetRect(0, 50);
    }

    m_iBldgOn = -1;
    rect = CRect(129, 22, 233, 71);
    for (int iOn = 0; iOn < 6; iOn++) {
        m_btnBldg[iOn].Create("",
                              rect, this, FIRST_BLDG_BUTTON + iOn, IDC_BUILD_LIST_BLDGS,
                              theBitmaps.GetByIndex(DIB_STRUCTURE_BKGND),
                              theBitmaps.GetByIndex(DIB_STRUCTURE_BTNS_2),
                              theBitmaps.GetByIndex(DIB_LIST_UNIT_BUILDINGS));
        rect.OffsetRect(0, 50);
    }

    CenterWindow(&theApp.m_wndMain);

    // init controls
    OnSelchangeBuildListCat();

    ASSERT_VALID (this);
    return (TRUE);
}

const int NUM_CIV_BLDG = 3;

// returns TRUE if can build this building
BOOL CDlgBuildStructure::CanBuild(int iIndex, CStructureData const *pSd) {

    if ((pSd->GetCat() != iIndex) || (!pSd->IsDiscovered()))
        return (FALSE);

    if ((theGame.GetScenario() != -1) && (pSd->GetScenario() > theGame.GetScenario()))
        return (FALSE);

    // special stuff for apt/office
    if (pSd->GetBldgType() == CStructureData::apartment) {
        int iMax = theApp.IsShareware() ? CStructureData::num_shareware_civ : CStructureData::num_apartments;
        // check for researched enough of them
        for (int iOn = CStructureData::apartment_base; iOn < CStructureData::apartment_base + iMax; iOn++)
            if (!theStructures.GetData(iOn)->IsDiscovered())
                iMax--;

        int iBase = theGame.GetMe()->GetPplTotal() / 200;
        iBase = __min (iBase, iMax - 3);
        int iNum = __min (NUM_CIV_BLDG, iMax - iBase);
        int iStrt = CStructureData::apartment_base + iBase;
        if ((pSd->GetType() < iStrt) || (pSd->GetType() >= iStrt + iNum))
            return (FALSE);
    }

    if (pSd->GetBldgType() == CStructureData::office) {
        int iMax = theApp.IsShareware() ? CStructureData::num_shareware_civ : CStructureData::num_offices;
        // check for researched enough of them
        for (int iOn = CStructureData::office_base; iOn < CStructureData::office_base + iMax; iOn++)
            if (!theStructures.GetData(iOn)->IsDiscovered())
                iMax--;

        int iBase = theGame.GetMe()->GetPplBldg() / 100;
        iBase = __min (iBase, iMax - 3);
        int iNum = __min (NUM_CIV_BLDG, iMax - iBase);
        int iStrt = CStructureData::office_base + iBase;
        if ((pSd->GetType() < iStrt) || (pSd->GetType() >= iStrt + iNum))
            return (FALSE);
    }

    if ((pSd->GetUnionType() != CStructureData::UTvehicle) && (pSd->GetUnionType() != CStructureData::UTshipyard))
        return (TRUE);

    // can't build a factory unless it can build vehicles
    CBuildVehicle const *pBv = pSd->GetBldVehicle();
    for (int iOn = 0, iInd = 0; iOn < pBv->GetSize(); iOn++) {
        CTransportData const *pTd = theTransports.GetData(pBv->GetUnit(iOn)->m_iVehType);
        if ((pTd->IsDiscovered()) &&
            ((theGame.GetScenario() == -1) || (pTd->GetScenario() <= theGame.GetScenario())))
            return (TRUE);
    }

    return (FALSE);
}

LRESULT CDlgBuildStructure::OnSelChange(WPARAM, LPARAM ID) {

    // is it a category?
    if ((FIRST_CAT_BUTTON <= ID) && (ID < FIRST_CAT_BUTTON + 6)) {
        // clear the other toggles
        for (int iOn = 0; iOn < 6; iOn++)
            if (iOn + FIRST_CAT_BUTTON != (int) ID)
                if (m_btnCat[iOn].GetToggleState())
                    m_btnCat[iOn].SetToggleState(FALSE);

        m_iCatOn = ID - FIRST_CAT_BUTTON;
        OnSelchangeBuildListCat();
        return (0);
    }

    // it's a building
    m_iBldgOn = ID - FIRST_BLDG_BUTTON;

    // clear the other toggles
    for (int iOn = 0; iOn < 6; iOn++)
        if (iOn + FIRST_BLDG_BUTTON != (int) ID)
            if (m_btnBldg[iOn].GetToggleState())
                m_btnBldg[iOn].SetToggleState(FALSE);

    OnSelchangeBuildListBldgs();

    return (0);
}

LRESULT CDlgBuildStructure::OnDblClk(WPARAM, LPARAM ID) {

    // is it a category?
    if ((FIRST_CAT_BUTTON <= ID) && (ID < FIRST_CAT_BUTTON + 6))
        return (0);

    // it's a building
    m_iBldgOn = ID - FIRST_BLDG_BUTTON;

    OnDblclkBuildListBldgs();
    return (0);
}

void CDlgBuildStructure::OnSelchangeBuildListCat() {

    // nothing selected
    m_iBldgOn = -1;

    m_pSd = NULL;
    m_strDesc = "";
    InvalidateRect(&rectText, FALSE);

    // if nothing selected leave it empty
    if (m_iCatOn < 0) {
        m_btnBuild.EnableWindow(FALSE);
        // clear out what is in there
        for (int iOn = 0; iOn < 6; iOn++) {
            m_btnBldg[iOn].SetNum(-1);
            m_btnBldg[iOn].m_pData = NULL;
            m_btnBldg[iOn].InvalidateRect(NULL);
            m_btnBldg[iOn].SetWindowText("");
            m_btnBldg[iOn].SetToggleState(FALSE);
            m_btnBldg[iOn].EnableWindow(FALSE);
        }
        return;
    }

    // add all the buildings of this type to the list box
    int iBtnNum = 0;
    for (int iOn = 0; iOn < theStructures.GetNumBuildings(); iOn++) {
        CStructureData const *pSd = theStructures.GetData(iOn);
        if (CanBuild(m_iCatOn, pSd)) {
            m_btnBldg[iBtnNum].SetNum(iOn);
            m_btnBldg[iBtnNum].m_pData = (void *) pSd;
            m_btnBldg[iBtnNum].SetWindowText(pSd->GetDesc());
            m_btnBldg[iBtnNum].InvalidateRect(NULL);
            m_btnBldg[iBtnNum].SetToggleState(FALSE);
            m_btnBldg[iBtnNum].EnableWindow(TRUE);
            iBtnNum++;
            if (iBtnNum >= 6)
                break;
        }
    }

    // clear out what is in there
    for (int iOn = iBtnNum; iOn < 6; iOn++) {
        m_btnBldg[iOn].SetNum(-1);
        m_btnBldg[iOn].m_pData = NULL;
        m_btnBldg[iOn].InvalidateRect(NULL);
        m_btnBldg[iOn].SetWindowText("");
        m_btnBldg[iOn].SetToggleState(FALSE);
        m_btnBldg[iOn].EnableWindow(FALSE);
    }

    OnSelchangeBuildListBldgs();
}

void CDlgBuildStructure::UpdateChoices() {

    // let's see if it's changed - we check each building that should be in the list
    // against the building at that point in the list. If they don't match - redo it
    if (m_iCatOn >= 0)
        for (int iOn = 0, iInd = 0; iOn < theStructures.GetNumBuildings(); iOn++) {
            CStructureData const *pSd = theStructures.GetData(iOn);
            if (CanBuild(m_iCatOn, pSd)) {
                if (m_btnBldg[iInd].m_pData != (void *) pSd) {
                    // re initializes list
                    OnSelchangeBuildListCat();
                    return;
                }
                iInd++;
            }
        }
}

void CDlgBuildStructure::OnDblclkBuildListBldgs() {

    OnSelchangeBuildListBldgs();
    OnOK();
}

void CDlgBuildStructure::OnSelchangeBuildListBldgs() {

    ASSERT_VALID (this);

    m_pSd = NULL;
    m_strDesc = "";
    InvalidateRect(&rectText, FALSE);

    if (m_iBldgOn < 0) {
        m_btnBuild.EnableWindow(FALSE);
        return;
    }
    m_btnBuild.EnableWindow(TRUE);

    UpdateData(TRUE);

    CStructureData const *pSd = (CStructureData *) m_btnBldg[m_iBldgOn].m_pData;
    if (pSd == NULL)
        return;
    ASSERT_VALID (pSd);
    m_strDesc = pSd->GetText();
    m_pSd = pSd;

    UpdateData(FALSE);
}

void CDlgBuildStructure::OnCancel() {

    DestroyWindow();
}

void CDlgBuildStructure::OnOK() {

    ASSERT_VALID (this);

    if (!m_btnBuild.IsWindowEnabled())
        return;

    // which item
    if (m_iBldgOn < 0)
        return;

    CStructureData const *pSd = (CStructureData *) m_btnBldg[m_iBldgOn].m_pData;
    ASSERT_VALID (pSd);

    // find the index
    int iIndex = 0;
    for ( ; iIndex < theStructures.GetNumBuildings(); iIndex++)
        if (theStructures.GetData(iIndex) == pSd)
            break;
    if (iIndex >= theStructures.GetNumBuildings())
        return;

    // find location
    CWndArea *pWndArea = theAreaList.GetTop();
    if (pWndArea == NULL)
        return;
    ASSERT_VALID (pWndArea);
    pWndArea->BuildOn(iIndex);

    // activate area window
    pWndArea->SetFocus();

    // delete it
    DestroyWindow();
}

void CDlgBuildStructure::OnDestroy() {

    // get rid of the pointer cause we're about to be destroyed
    m_pVehPar->m_pDlgStructure = NULL;

    delete m_pDibBkgnd;
    m_pDibBkgnd = NULL;

    delete m_pDibBtn;
    m_pDibBtn = NULL;

    CDialog::OnDestroy();
}

void CDlgBuildStructure::OnPaletteChanged(CWnd *pFocusWnd) {
    static BOOL bInFunc = FALSE;

    CDialog::OnPaletteChanged(pFocusWnd);

    // Win32s locks up if we do the below code
    if (iWinType == W32s)
        return;

    // stop infinite recursion
    if (bInFunc)
        return;
    bInFunc = TRUE;

    CClientDC dc(this);
    int iRtn = thePal.PalMsg(dc.m_hDC, m_hWnd, WM_PALETTECHANGED, (WPARAM) pFocusWnd->m_hWnd, 0);

    // invalidate the window
    if (iRtn)
        InvalidateRect(NULL);

    SendMessage(WM_NCPAINT, 0, 0);

    bInFunc = FALSE;
}

BOOL CDlgBuildStructure::OnQueryNewPalette() {

    if (iWinType == W32s)
        return CDialog::OnQueryNewPalette();

    CClientDC dc(this);
    thePal.PalMsg(dc.m_hDC, m_hWnd, WM_QUERYNEWPALETTE, 0, 0);

    SendMessage(WM_NCPAINT, 0, 0);
    return CDialog::OnQueryNewPalette();
}

BOOL CDlgBuildStructure::OnEraseBkgnd(CDC *) {

    return (1);
}

void CDlgBuildStructure::OnPaint() {

    ASSERT_VALID (this);

    CPaintDC dc(this); // device context for painting
    thePal.Paint(dc.m_hDC);

    // paint background (stretch in case...)
    CRect rect;
    GetClientRect(&rect);
    CDIB *pdib = theBitmaps.GetByIndex(DIB_STRUCTURE_BKGND);
    pdib->StretchBlt(m_pDibBkgnd, rect, pdib->GetRect());

    // set up for text
    CDC *pDcTxt = CDC::FromHandle(m_pDibBkgnd->GetDC());
    if (pDcTxt == NULL) {
        thePal.EndPaint(dc.m_hDC);
        return;
    }
    thePal.Paint(pDcTxt->m_hDC);
    pDcTxt->SetBkMode(TRANSPARENT);
    pDcTxt->SetTextColor(PALETTERGB (41, 255, 8));

    // paint the description
    rect = CRect(252, 22, 450, 144);
    FitDrawText(pDcTxt, theApp.DescFont(), m_strDesc, rect);

    // paint the cost
    if (m_pSd != NULL) {
        CFont *pOldFont = pDcTxt->SelectObject(&theApp.CostFont());

        // header
        rect = CRect(264, 158, 366, 280);
        pDcTxt->DrawText(m_sCost, &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
        rect.right = 402;
        pDcTxt->DrawText(m_sHave, &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
        rect.right = 439;
        rect.top += pDcTxt->DrawText(m_sNeed, &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);

        CPen pen(PS_SOLID, 1, PALETTERGB (41, 255, 8));
        CPen *pOldPen = pDcTxt->SelectObject(&pen);
        rect.top++;
        pDcTxt->MoveTo(264, rect.top);
        pDcTxt->LineTo(439, rect.top);
        rect.top += 2;

        // time
        pDcTxt->DrawText(m_sTime, &rect, DT_LEFT | DT_SINGLELINE | DT_TOP);
        rect.right = 366;
        int iHt = pDcTxt->DrawText(IntToCString(m_pSd->m_iTimeBuild / 24), &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
        rect.top += iHt;

        // materials
        for (int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes(); iInd++)
            if (m_pSd->m_aiBuild[iInd] > 0) {
                pDcTxt->DrawText(CMaterialTypes::GetDesc(iInd), &rect, DT_LEFT | DT_SINGLELINE | DT_TOP);
                rect.right = 366;
                pDcTxt->DrawText(IntToCString(m_pSd->m_aiBuild[iInd]), &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
                rect.right = 402;
                iHt = pDcTxt->DrawText(IntToCString(theGame.GetMe()->GetMaterialHave(iInd)), &rect,
                                       DT_RIGHT | DT_SINGLELINE | DT_TOP);
                int iLoss = theGame.GetMe()->GetMaterialHave(iInd) - m_pSd->m_aiBuild[iInd];
                if (iLoss < 0) {
                    rect.right = 439;
                    pDcTxt->SetTextColor(PALETTERGB (255, 41, 8));
                    pDcTxt->DrawText("(" + IntToCString(iLoss) + ")", &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
                    pDcTxt->SetTextColor(PALETTERGB (41, 255, 8));
                }
                rect.top += iHt;
            }

        // operating costs
        pDcTxt->SetTextColor(PALETTERGB (71, 71, 225));
        rect.top += iHt / 2;
        rect.top += pDcTxt->DrawText(m_sOper, &rect, DT_LEFT | DT_SINGLELINE | DT_TOP);

        pDcTxt->DrawText(m_sPeople, &rect, DT_LEFT | DT_SINGLELINE | DT_TOP);
        rect.right = 366;
        pDcTxt->DrawText(IntToCString(m_pSd->GetPeople()), &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
        rect.right = 402;
        iHt = pDcTxt->DrawText(IntToCString(theGame.GetMe()->GetPplTotal()), &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
        int iLoss = theGame.GetMe()->GetPplBldg() - theGame.GetMe()->GetPplNeedBldg() - m_pSd->GetPeople();
        if (iLoss < 0) {
            rect.right = 439;
            pDcTxt->SetTextColor(PALETTERGB (255, 41, 8));
            pDcTxt->DrawText("(" + IntToCString(iLoss) + ")", &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
            pDcTxt->SetTextColor(PALETTERGB (71, 71, 225));
        }
        rect.top += iHt;

        pDcTxt->DrawText(m_sPower, &rect, DT_LEFT | DT_SINGLELINE | DT_TOP);
        rect.right = 366;
        pDcTxt->DrawText(IntToCString(m_pSd->m_iPower), &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
        rect.right = 402;
        iHt = pDcTxt->DrawText(IntToCString(theGame.GetMe()->GetPwrHave()), &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
        iLoss = theGame.GetMe()->GetPwrHave() - theGame.GetMe()->GetPwrNeed() - m_pSd->m_iPower;
        if (iLoss < 0) {
            rect.right = 439;
            pDcTxt->SetTextColor(PALETTERGB (255, 41, 8));
            pDcTxt->DrawText("(" + IntToCString(iLoss) + ")", &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
            pDcTxt->SetTextColor(PALETTERGB (71, 71, 225));
        }
        rect.top += iHt;

        pDcTxt->MoveTo(329, 158);
        pDcTxt->LineTo(329, rect.top);

        pDcTxt->SelectObject(pOldPen);
        pDcTxt->SelectObject(pOldFont);
    }

//	m_pDibBkgnd->ReleaseDC ();

    m_pDibBkgnd->BitBlt(dc, m_pDibBkgnd->GetRect(), CPoint(0, 0));

    thePal.EndPaint(pDcTxt->m_hDC);
    if (m_pDibBkgnd->IsBitmapSelected())
        m_pDibBkgnd->ReleaseDC();

    thePal.EndPaint(dc.m_hDC);
    // Do not call CWndBase::OnPaint() for painting messages
}

void CDlgBuildStructure::OnDrawItem(int, LPDRAWITEMSTRUCT pDis) {

    if (ODT_BUTTON == pDis->CtlType)    // GG
    {
        // category button
        for (int iOn = 0; iOn < 6; iOn++)
            if (iOn + FIRST_CAT_BUTTON == (int) pDis->CtlID) {
                m_btnCat[iOn].DrawItem(pDis);
                return;
            }

        // building button
        for (int iOn = 0; iOn < 6; iOn++)
            if (iOn + FIRST_BLDG_BUTTON == (int) pDis->CtlID) {
                m_btnBldg[iOn].DrawItem(pDis);
                return;
            }

        // ok/cancel button
        CDIB *pDibBmp = theBitmaps.GetByIndex(DIB_STRUCTURE_BTNS_3);
        CRect rSrc(pDibBmp->GetRect());
        rSrc.right /= 3;
        CRect rDest(m_pDibBtn->GetRect());
        if (pDis->itemState & (ODS_DISABLED | ODS_GRAYED))
            rSrc.OffsetRect(rSrc.Width() * 2, 0);
        else if (pDis->itemState & ODS_SELECTED) {
            rSrc.OffsetRect(rSrc.Width(), 0);
            rDest.OffsetRect(2, 1);
        }
        pDibBmp->StretchBlt(m_pDibBtn, m_pDibBtn->GetRect(), rSrc);

        // paint the text
        CDC *pDc = CDC::FromHandle(m_pDibBtn->GetDC());
        if (pDc == NULL)
            return;
        pDc->SetBkMode(TRANSPARENT);
        pDc->SetTextColor(CLR_UNIT_BUILD);
        pDc->SetTextColor(RGB (255, 255, 255));
        CString sText;
        GetDlgItem(pDis->CtlID)->GetWindowText(sText);
        pDc->DrawText(sText, -1, &rDest,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_WORDBREAK | DT_NOPREFIX);

        // BLT to the screen DC
        thePal.Paint(pDis->hDC);
        m_pDibBtn->BitBlt(pDis->hDC, &(pDis->rcItem), CPoint(0, 0));

        thePal.EndPaint(pDis->hDC);
        if (m_pDibBtn->IsBitmapSelected())
            m_pDibBtn->ReleaseDC();
        return;
    }
}


/////////////////////////////////////////////////////////////////////////////
// CDlgBuildTransport dialog

const int FIRST_VEH_BUTTON = 110;

CDlgBuildTransport::CDlgBuildTransport(CWnd *pParent, CVehicleBuilding *pBldg)
        : CDialog(CDlgBuildTransport::IDD, pParent) {

    m_pBldgPar = pBldg;
    m_pDibBtn = m_pDibBkgnd = NULL;
    m_iNum = 1;

    //{{AFX_DATA_INIT(CDlgBuildTransport)
    m_sNum = "1";
    //}}AFX_DATA_INIT
}

void CDlgBuildTransport::DoDataExchange(CDataExchange *pDX) {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgBuildTransport)
    DDX_Control(pDX, IDC_BUILD_NUM, m_edtNum);
    DDX_Control(pDX, IDC_BUILD_SPIN, m_spnNum);
    DDX_Control(pDX, IDOK, m_btnBuild);
    DDX_Text(pDX, IDC_BUILD_NUM, m_sNum);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgBuildTransport, CDialog)
                    //{{AFX_MSG_MAP(CDlgBuildTransport)
                    ON_EN_CHANGE(IDC_BUILD_NUM, OnChangeNum)
                    ON_MESSAGE (MSG_BTN_CLICKED, OnSelChange)
                    ON_MESSAGE (MSG_BTN_DBLCLK, OnDblClk)
                    ON_WM_DESTROY()
                    ON_WM_CREATE()
                    ON_WM_ERASEBKGND()
                    ON_WM_PAINT()
                    ON_WM_DRAWITEM()
                    ON_WM_PALETTECHANGED()
                    ON_WM_QUERYNEWPALETTE()
                    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgBuildTransport message handlers

const int TRANS_LIST_ITEM_HT = 47;
const CRect rectTranText(128, 20, 369, 251);
const CRect rectTranStat(134, 292, 253, 318);

void CDlgBuildTransport::Create(CWnd *pBldg) {

    ASSERT_VALID (pBldg);
    m_pDibBtn = m_pDibBkgnd = NULL;

    CDialog::Create(IDD, pBldg);
}

BOOL CDlgBuildTransport::OnInitDialog() {

    CDialog::OnInitDialog();

    m_pBu = NULL;

    m_sCost.LoadString(IDS_COST);
    m_sHave.LoadString(IDS_HAVE);
    m_sNeed.LoadString(IDS_NEED);
    m_sTime.LoadString(IDS_TIME);
    m_sPeople.LoadString(IDS_PEOPLE);

    m_pDibBkgnd = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                           CBLTFormat::DIR_BOTTOMUP, 380, 332);

    m_pDibBtn = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                         CBLTFormat::DIR_BOTTOMUP, 84, 23);

    // set the controls
    CRect rWin, rClnt;
    GetWindowRect(&rWin);
    GetClientRect(&rClnt);
    SetWindowPos(NULL, 0, 0, 380 + rWin.Width() - rClnt.Width(),
                 332 + rWin.Height() - rClnt.Height(), SWP_NOMOVE | SWP_NOZORDER);
    m_btnBuild.SetWindowPos(NULL, 133, 258, 84, 23, SWP_NOZORDER);
    GetDlgItem(IDCANCEL)->SetWindowPos(NULL, 282, 258, 84, 23, SWP_NOZORDER);
    m_edtNum.SetWindowPos(NULL, 282, 295, 40, 22, SWP_NOZORDER);
    m_spnNum.SetWindowPos(NULL, 307, 295, 15, 22, SWP_NOZORDER);

    m_iVehOn = -1;
    CRect rect(10, 24, 118, 72);
    for (int iOn = 0; iOn < 6; iOn++) {
        m_btnVeh[iOn].Create("",
                             rect, this, FIRST_VEH_BUTTON + iOn, IDC_BUILD_LIST,
                             theBitmaps.GetByIndex(DIB_VEHICLE_BKGND),
                             theBitmaps.GetByIndex(DIB_VEHICLE_BTNS_2),
                             theBitmaps.GetByIndex(DIB_LIST_UNIT_VEHICLES));
        rect.OffsetRect(0, 48);
    }

    // unit building
    m_statInst.Attach(&theIcons, ICON_BUILD_VEH);
    m_statInst.SetSize(rectTranStat);
    m_statInst.SetPer(__max (0, m_pBldgPar->GetBuildPer()));

    // int
    m_spnNum.SetBuddy(&m_edtNum);
    m_spnNum.SetRange(1, UD_MAXVAL);
    UpdateData(TRUE);
    m_sNum = "1";
    m_iNum = 1;
    UpdateData(FALSE);

    // if its troops we change the title
    if (m_pBldgPar->GetData()->GetBldgType() == CStructureData::barracks) {
        CString sTitle;
        sTitle.LoadString(IDS_BUILD_PEOPLE);
        SetWindowText(sTitle);
    }

    CenterWindow(&theApp.m_wndMain);

    UpdateChoices();

    ASSERT_VALID (this);

    
    // now it's safe for handlers to call UpdateData(TRUE)
    m_bInitComplete = true;

    return (TRUE);
}

void CDlgBuildTransport::OnChangeNum() {
    if ( m_bInitComplete )
    {
        UpdateData( TRUE );
        m_iNum = atoi( m_sNum );
        InvalidateRect( &rectTranText, FALSE );
    }
}

void CDlgBuildTransport::UpdateChoices() {

    // let's walk through - and set all buttons
    ASSERT_VALID (m_pBldgPar);
    CBuildVehicle const *pBv = m_pBldgPar->GetData()->GetBldVehicle();
    int iBtn = 0;
    for (int iOn = 0; iOn < pBv->GetSize(); iOn++) {
        CTransportData const *pTd = theTransports.GetData(pBv->GetUnit(iOn)->m_iVehType);
        if ((pTd->IsDiscovered()) &&
            ((theGame.GetScenario() == -1) || (pTd->GetScenario() <= theGame.GetScenario()))) {
            m_btnVeh[iBtn].SetNum(pBv->GetUnit(iOn)->m_iVehType);
            m_btnVeh[iBtn].m_pData = (void *) pBv->GetUnit(iOn);
            m_btnVeh[iBtn].InvalidateRect(NULL);
            m_btnVeh[iBtn].SetWindowText(pTd->GetDesc());
            m_btnVeh[iBtn].SetToggleState(iBtn == m_iVehOn);
            m_btnVeh[iBtn].EnableWindow(TRUE);


            iBtn++;
            if (iBtn >= 6)
                break;
        }
    }

    // the rest are disabled
    for (int iOn = iBtn; iOn < 6; iOn++) {
        m_btnVeh[iOn].SetNum(-1);
        m_btnVeh[iOn].m_pData = NULL;
        m_btnVeh[iOn].InvalidateRect(NULL);
        m_btnVeh[iOn].SetWindowText("");
        m_btnVeh[iOn].SetToggleState(FALSE);
        m_btnVeh[iOn].EnableWindow(FALSE);
    }

    // init controls
    OnSelchangeBuildList();
}

LRESULT CDlgBuildTransport::OnSelChange(WPARAM, LPARAM ID) {

    m_iVehOn = ID - FIRST_VEH_BUTTON;

    // clear the other toggles
    for (int iOn = 0; iOn < 6; iOn++)
        if (iOn + FIRST_VEH_BUTTON != (int) ID)
            if (m_btnVeh[iOn].GetToggleState())
                m_btnVeh[iOn].SetToggleState(FALSE);

    OnSelchangeBuildList();

    return (0);
}

LRESULT CDlgBuildTransport::OnDblClk(WPARAM, LPARAM ID) {

    m_iVehOn = ID - FIRST_VEH_BUTTON;

    OnOK();
    return (0);
}

void CDlgBuildTransport::OnSelchangeBuildList() {

    ASSERT_VALID (this);

    if (m_iVehOn < 0) {
        m_btnBuild.EnableWindow(FALSE);
        return;
    }
    CBuildUnit *pBu = (CBuildUnit *) m_btnVeh[m_iVehOn].m_pData;

    ASSERT_VALID (m_pBldgPar);
    int iVehIndex = pBu->m_iVehType;
    ASSERT (iVehIndex < theTransports.GetNumTransports());

    if ((iVehIndex < 0) || (iVehIndex >= theTransports.GetNumTransports())) {
        m_btnBuild.EnableWindow(FALSE);
        return;
    }
    m_btnBuild.EnableWindow(TRUE);

    UpdateData(TRUE);
    m_iNum = atoi(m_sNum);

    CTransportData const *pTd = theTransports.GetData(iVehIndex);
    m_strDesc = pTd->GetText();
    m_pBu = pBu;

    InvalidateRect(&rectTranText, FALSE);
}

BOOL CDlgBuildTransport::OnEraseBkgnd(CDC *) {

    return (1);
}

void CDlgBuildTransport::UpdateStatus(int iPer) {

    // must exist
    if (m_hWnd == NULL)
        return;

    m_statInst.SetPer(iPer);
    InvalidateRect(&(m_statInst.m_rDest), FALSE);

    // set the title
    CString sTitle;
    if (iPer != 0) {
        sTitle.LoadString(IDS_BUILD_UNIT);
        CBuildUnit const *pBu = m_pBldgPar->GetBldUnt();
        if (pBu == NULL)
            csPrintf(&sTitle, (char const *) "");
        else
            csPrintf(&sTitle, (char const *) theTransports.GetData(pBu->GetVehType())->GetDesc());
    } else if (m_pBldgPar->GetData()->GetBldgType() == CStructureData::barracks)
        sTitle.LoadString(IDS_BUILD_PEOPLE);
    else
        sTitle.LoadString(IDS_BUILD_VEHICLE);
    SetWindowText(sTitle);

    // the below we do only if we aren't active
    if (CWnd::GetActiveWindow() == this)
        return;

    // update the count
    UpdateData(TRUE);
    m_iNum = atoi(m_sNum);
    if (m_iNum != m_pBldgPar->GetNum()) {
        m_iNum = __max (1, m_pBldgPar->GetNum());
        m_sNum = IntToCString(m_iNum);
        UpdateData(FALSE);
    }
    InvalidateRect(&rectTranText, FALSE);
}

void CDlgBuildTransport::OnPaletteChanged(CWnd *pFocusWnd) {
    static BOOL bInFunc = FALSE;

    CDialog::OnPaletteChanged(pFocusWnd);

    // Win32s locks up if we do the below code
    if (iWinType == W32s)
        return;

    // stop infinite recursion
    if (bInFunc)
        return;
    bInFunc = TRUE;

    CClientDC dc(this);
    int iRtn = thePal.PalMsg(dc.m_hDC, m_hWnd, WM_PALETTECHANGED, (WPARAM) pFocusWnd->m_hWnd, 0);

    // invalidate the window
    if (iRtn)
        InvalidateRect(NULL);

    SendMessage(WM_NCPAINT, 0, 0);

    bInFunc = FALSE;
}

BOOL CDlgBuildTransport::OnQueryNewPalette() {

    if (iWinType == W32s)
        return CDialog::OnQueryNewPalette();

    CClientDC dc(this);
    thePal.PalMsg(dc.m_hDC, m_hWnd, WM_QUERYNEWPALETTE, 0, 0);

    SendMessage(WM_NCPAINT, 0, 0);
    return CDialog::OnQueryNewPalette();
}

void CDlgBuildTransport::OnPaint() {

    ASSERT_VALID (this);

    CPaintDC dc(this); // device context for painting
    thePal.Paint(dc.m_hDC);

    // paint background (stretch in case...)
    CRect rect;
    GetClientRect(&rect);
    CDIB *pdib = theBitmaps.GetByIndex(DIB_VEHICLE_BKGND);
    pdib->StretchBlt(m_pDibBkgnd, rect, pdib->GetRect());

    // paint the vehicle being built
    m_statInst.m_pDib->Clear(NULL, 253);
    m_statInst.DrawStatDone();
    m_statInst.m_pDib->TranBlt(m_pDibBkgnd, m_statInst.m_rDest, CPoint(0, 0));

    // set up for text
    CDC *pDcTxt = CDC::FromHandle(m_pDibBkgnd->GetDC());
    if (pDcTxt == NULL) {
        thePal.EndPaint(dc.m_hDC);
        return;
    }
    thePal.Paint(pDcTxt->m_hDC);
    pDcTxt->SetBkMode(TRANSPARENT);
    pDcTxt->SetTextColor(PALETTERGB (41, 255, 8));

    // paint the description
    rect = CRect(136, 26, 361, 128);
    FitDrawText(pDcTxt, theApp.DescFont(), m_strDesc, rect);

    // paint the cost
    if (m_pBu != NULL) {
        CFont *pOldFont = pDcTxt->SelectObject(&theApp.CostFont());

        // header
        rect = CRect(136, 142, 270, 245);
        pDcTxt->DrawText(m_sCost, &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
        rect.right = 315;
        pDcTxt->DrawText(m_sHave, &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
        rect.right = 360;
        rect.top += pDcTxt->DrawText(m_sNeed, &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP) + 1;

        CPen pen(PS_SOLID, 1, PALETTERGB (41, 255, 8));
        CPen *pOldPen = pDcTxt->SelectObject(&pen);
        pDcTxt->MoveTo(136, rect.top);
        pDcTxt->LineTo(360, rect.top);
        rect.top += 2;

        // time
        pDcTxt->DrawText(m_sTime, &rect, DT_LEFT | DT_SINGLELINE | DT_TOP);
        rect.right = 270;
        CString sTime = IntToCString(m_pBu->m_iTime / 24);
        int iHt = pDcTxt->DrawText(sTime, &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
        if (m_iNum > 1) {
            sTime = "(" + IntToCString((m_iNum * m_pBu->m_iTime) / 24) + ")";
            rect.right = 315;
            pDcTxt->DrawText(sTime, &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
        }
        rect.top += iHt;

        // materials
        for (int iInd = 0; iInd < CMaterialTypes::GetNumBuildTypes(); iInd++)
            if (m_pBu->m_aiInput[iInd] > 0) {
                pDcTxt->DrawText(CMaterialTypes::GetDesc(iInd), &rect, DT_LEFT | DT_SINGLELINE | DT_TOP);
                rect.right = 270;
                pDcTxt->DrawText(IntToCString(m_pBu->m_aiInput[iInd] * m_iNum), &rect,
                                 DT_RIGHT | DT_SINGLELINE | DT_TOP);
                rect.right = 315;
                int iHt = pDcTxt->DrawText(IntToCString(m_pBldgPar->GetStore(iInd)), &rect,
                                           DT_RIGHT | DT_SINGLELINE | DT_TOP);
                int iLoss = m_pBldgPar->GetStore(iInd) - (m_pBu->m_aiInput[iInd] * m_iNum);
                if (iLoss < 0) {
                    rect.right = 360;
                    pDcTxt->SetTextColor(PALETTERGB (255, 41, 8));
                    pDcTxt->DrawText("(" + IntToCString(iLoss) + ")", &rect, DT_RIGHT | DT_SINGLELINE | DT_TOP);
                    pDcTxt->SetTextColor(PALETTERGB (41, 255, 8));
                }
                rect.top += iHt;
            }

        // people to operate
        pDcTxt->SetTextColor(PALETTERGB (71, 71, 225));
        rect.top += iHt / 2;
        rect.right = 270;
        pDcTxt->DrawText(m_sPeople, &rect, DT_LEFT | DT_SINGLELINE | DT_TOP);
        rect.right = 315;
        CTransportData const *pData = theTransports.GetData(m_pBu->GetVehType());
        rect.top += pDcTxt->DrawText(IntToCString(m_iNum * pData->GetPeople()), &rect,
                                     DT_RIGHT | DT_SINGLELINE | DT_TOP);

        pDcTxt->MoveTo(210, 142);
        pDcTxt->LineTo(210, rect.top);

        pDcTxt->SelectObject(pOldPen);
        pDcTxt->SelectObject(pOldFont);
    }

    m_pDibBkgnd->BitBlt(dc, m_pDibBkgnd->GetRect(), CPoint(0, 0));

    thePal.EndPaint(pDcTxt->m_hDC);
    if (m_pDibBkgnd->IsBitmapSelected())
        m_pDibBkgnd->ReleaseDC();

    thePal.EndPaint(dc.m_hDC);
    // Do not call CWndBase::OnPaint() for painting messages
}

void CDlgBuildTransport::OnDrawItem(int, LPDRAWITEMSTRUCT pDis) {

    if (ODT_BUTTON == pDis->CtlType)    // GG
    {
        // building button
        for (int iOn = 0; iOn < 6; iOn++)
            if (iOn + FIRST_VEH_BUTTON == (int) pDis->CtlID) {
                m_btnVeh[iOn].DrawItem(pDis);
                return;
            }

        if ((pDis->CtlID != IDOK) && (pDis->CtlID != IDCANCEL)) {
            Default();
            return;
        }

        // ok/cancel button
        CDIB *pDibBmp = theBitmaps.GetByIndex(DIB_VEHICLE_BTNS_1);
        CRect rSrc(pDibBmp->GetRect());
        rSrc.right /= 3;
        CRect rDest(m_pDibBtn->GetRect());
        if (pDis->itemState & (ODS_DISABLED | ODS_GRAYED))
            rSrc.OffsetRect(rSrc.Width() * 2, 0);
        else if (pDis->itemState & ODS_SELECTED) {
            rSrc.OffsetRect(rSrc.Width(), 0);
            rDest.OffsetRect(2, 1);
        }
        pDibBmp->StretchBlt(m_pDibBtn, m_pDibBtn->GetRect(), rSrc);

        // paint the text
        CDC *pDc = CDC::FromHandle(m_pDibBtn->GetDC());
        if (pDc == NULL)
            return;
        pDc->SetBkMode(TRANSPARENT);
        pDc->SetTextColor(CLR_UNIT_BUILD);
        pDc->SetTextColor(RGB (255, 255, 255));
        CString sText;
        GetDlgItem(pDis->CtlID)->GetWindowText(sText);
        pDc->DrawText(sText, -1, &rDest,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_WORDBREAK | DT_NOPREFIX);

        // BLT to the screen DC
        thePal.Paint(pDis->hDC);
        m_pDibBtn->BitBlt(pDis->hDC, &(pDis->rcItem), CPoint(0, 0));

        if (m_pDibBtn->IsBitmapSelected())
            m_pDibBtn->ReleaseDC();
        thePal.EndPaint(pDis->hDC);
        return;
    }
}

void CDlgBuildTransport::OnCancel() {

    DestroyWindow();
}

void CDlgBuildTransport::OnOK() {

    if ((!m_btnBuild.IsWindowEnabled()) || (m_iVehOn < 0))
        return;

    ASSERT_VALID (this);
    UpdateData(TRUE);
    m_iNum = atoi(m_sNum);

    // which item
    CBuildUnit *pBu = (CBuildUnit *) m_btnVeh[m_iVehOn].m_pData;

    ASSERT_VALID (m_pBldgPar);
    int iIndex = pBu->m_iVehType;
    ASSERT (iIndex < theTransports.GetNumTransports());

    // turn it on
    m_pBldgPar->ResumeUnit();

    // start the construction (for our display)
    m_pBldgPar->StartVehicle(iIndex, m_iNum);
    UpdateStatus(1);

    m_statInst.SetPer(1);
    InvalidateRect(&(m_statInst.m_rDest), FALSE);

    // tell the server (it decides when const is done)
    if (!theGame.AmServer()) {
        CMsgBuildVeh msg(m_pBldgPar, iIndex);
        theGame.PostToServer(&msg, sizeof(msg));
    }

    // leave it up but go to area map
    CWndArea *pWndArea = theAreaList.GetTop();
    if (pWndArea != NULL)
        pWndArea->SetFocus();
}

void CDlgBuildTransport::OnDestroy() {

    // get rid of the pointer cause we're about to be destroyed
    m_pBldgPar->m_pDlgTransport = NULL;

    delete m_pDibBkgnd;
    m_pDibBkgnd = NULL;
    delete m_pDibBtn;
    m_pDibBtn = NULL;

    CDialog::OnDestroy();
}


/////////////////////////////////////////////////////////////////////////////
// CWndRoute

BEGIN_MESSAGE_MAP(CWndRoute, CWndBase)
                    //{{AFX_MSG_MAP(CWndRoute)
                    ON_WM_CREATE()
                    ON_WM_PAINT()
                    ON_WM_SIZE()
                    ON_BN_CLICKED (IDS_ROUTE_WAYPOINT, Waypoint)
                    ON_BN_CLICKED (IDS_ROUTE_UNLOAD, Unload)
                    ON_BN_CLICKED (IDS_ROUTE_LOAD, Load)
                    ON_BN_CLICKED (IDS_ROUTE_GOTO, Goto)
                    ON_BN_CLICKED (IDS_ROUTE_DELETE, Delete)
                    ON_BN_CLICKED (IDS_ROUTE_OK, Start)
                    ON_BN_CLICKED (IDS_ROUTE_AUTO, Auto)
                    ON_LBN_SELCHANGE (101, OnLbnClk)
                    ON_WM_GETMINMAXINFO()
                    ON_WM_DESTROY()
                    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWndRoute message handlers

CWndRoute::CWndRoute(CVehicle *pVeh) {

    ASSERT_VALID (pVeh);

    m_pVeh = pVeh;
    m_iXmin = theApp.TextWid() * 10 * NUM_ROUTE_BTNS;
    m_iYmin = theApp.TextHt() + 5 * theApp.BevelDimen() +
              3 * theApp.TextHt();
}

void CWndRoute::OnGetMinMaxInfo(MINMAXINFO FAR *lpMMI) {

    if (lpMMI->ptMinTrackSize.x < m_iXmin)
        lpMMI->ptMinTrackSize.x = m_iXmin;
    if (lpMMI->ptMinTrackSize.y < m_iYmin)
        lpMMI->ptMinTrackSize.y = m_iYmin;

    CWndBase::OnGetMinMaxInfo(lpMMI);
}

void CWndRoute::Create(CWndArea *pPar) {

    ASSERT_VALID (pPar);

    // cascade off LR of our parent
    CRect rect;
    pPar->GetClientRect(&rect);
    pPar->ClientToScreen(&rect);

    CreateEx(0, theApp.m_sWndCls, m_pVeh->GetData()->GetDesc(), dwPopWndStyle,
             rect.right - m_iXmin, rect.bottom - m_iYmin * 2, m_iXmin, m_iYmin * 2,
             pPar->m_hWnd, NULL, NULL);
}

int CWndRoute::OnCreate(LPCREATESTRUCT lpCreateStruct) {

    if (CWndBase::OnCreate(lpCreateStruct) == -1)
        return -1;

    // make the list box
    CRect rect;
    GetClientRect(&rect);
    rect.top = theApp.BevelDimen();
    rect.left = theApp.BevelDimen();
    rect.bottom -= (theApp.TextHt() + 4 * theApp.BevelDimen());
    rect.right -= theApp.BevelDimen();
    m_listbox.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_USETABSTOPS, rect, this, 101);

    m_iYmin = theApp.TextHt() + 5 * theApp.BevelDimen() +
              3 * m_listbox.GetItemHeight(0);
    int iTabs[3] = {16, 112, 160};
    m_listbox.SetTabStops(3, iTabs);

    // buttons
    static int aiBtns[NUM_ROUTE_BTNS] = {IDS_ROUTE_WAYPOINT, IDS_ROUTE_UNLOAD, IDS_ROUTE_LOAD, IDS_ROUTE_GOTO,
                                         IDS_ROUTE_DELETE, IDS_ROUTE_OK, IDS_ROUTE_AUTO};
    static int aiHlp[NUM_ROUTE_BTNS] = {IDH_ROUTE_WAYPOINT, IDH_ROUTE_UNLOAD, IDH_ROUTE_LOAD, IDH_ROUTE_GOTO,
                                        IDH_ROUTE_DELETE, IDH_ROUTE_OK, IDH_ROUTE_AUTO};

    GetClientRect(&rect);
    rect.left += 4;
    rect.right -= 4;
    rect.bottom -= 4;
    rect.top += 4;
    int iWid = rect.Width() / NUM_ROUTE_BTNS;
    rect.right = rect.left + iWid - theApp.BevelDimen();

    for (int iOn = 0; iOn < NUM_ROUTE_BTNS; iOn++) {
        m_Btns[iOn].Create(aiBtns[iOn], aiHlp[iOn], &theTextBtnData, rect, theBitmaps.GetByIndex(DIB_GOLD), this,
                           aiBtns[iOn]);
        rect.left += iWid;
        rect.right += iWid;
        if (iOn < 3)
            m_Btns[iOn].SetToggleMode(TRUE);
    }
    OnLbnClk();

    CString sTitle;
    sTitle.LoadString(IDS_ROUTE_TITLE);
    csPrintf(&sTitle, (char const *) m_pVeh->GetData()->GetDesc());
    SetWindowText(sTitle);

    // fill the listbox
    NewRoute(m_pVeh);

    // no un/load if not a transport
    if (!m_pVeh->GetData()->IsTransport()) {
        m_Btns[1].EnableWindow(FALSE);
        m_Btns[2].EnableWindow(FALSE);
    }

    return 0;
}

void CWndRoute::OnPaint() {

    if (!theApp.m_bSubClass) {
        CWndBase::OnPaint();
        return;
    }

    ASSERT_VALID (this);
    CPaintDC dc(this); // device context for painting

    CRect rect;
    CWndBase::GetClientRect(&rect);

    thePal.Paint(dc.m_hDC);
    theBitmaps.GetByIndex(DIB_GOLD)->Tile(dc, rect);
    thePal.EndPaint(dc.m_hDC);

    // Do not call CWndBase::OnPaint() for painting messages
}

void CWndRoute::OnSize(UINT nType, int cx, int cy) {

    ASSERT_VALID (this);
    CWndBase::OnSize(nType, cx, cy);

    int iBtnHt = theApp.TextHt() + theApp.TextHt() / 2;

    // re-locate the listbox
    CRect rect;
    GetClientRect(&rect);
    rect.top = theApp.BevelDimen();
    rect.left = theApp.BevelDimen();
    rect.bottom -= (iBtnHt + 4 * theApp.BevelDimen());
    rect.right -= theApp.BevelDimen();
    m_listbox.SetWindowPos(NULL, rect.left, rect.top, rect.Width(),
                           rect.Height(), SWP_NOACTIVATE | SWP_NOZORDER);

    // set tabs for listbox
    int iWid = LOWORD (GetDialogBaseUnits());
    int iTab = rect.Width() / 10;
    int iTabs[3];
    iTabs[0] = ((iTab * 1) * 4) / iWid;
    iTabs[1] = ((iTab * 6) * 4) / iWid;
    iTabs[2] = ((iTab * 3) * 4) / iWid;
    m_listbox.SetTabStops(3, iTabs);

    // resize the buttons
    GetClientRect(&rect);
    rect.left += theApp.BevelDimen();
    rect.right -= theApp.BevelDimen();
    rect.bottom -= theApp.BevelDimen();
    rect.top = rect.bottom - (2 * theApp.BevelDimen() + iBtnHt);
    iWid = rect.Width() / NUM_ROUTE_BTNS;
    rect.right = rect.left + iWid - theApp.BevelDimen();

    for (int iOn = 0; iOn < NUM_ROUTE_BTNS; iOn++) {
        m_Btns[iOn].SetWindowPos(NULL, rect.left, rect.top, rect.Width(),
                                 rect.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
        rect.left += iWid;
        rect.right += iWid;
    }
}

void CWndRoute::OnLbnClk() {

    ASSERT_VALID (this);

    if (m_listbox.GetCurSel() >= 0) {
        m_Btns[3].EnableWindow(TRUE);
        m_Btns[4].EnableWindow(TRUE);
    } else {
        m_Btns[3].EnableWindow(FALSE);
        m_Btns[4].EnableWindow(FALSE);
    }

    m_Btns[6].EnableWindow(m_pVeh->m_bFlags & CVehicle::hp_controls);
}

void CWndRoute::Waypoint() {

    CallGotoOn(CRoute::waypoint);
}

void CWndRoute::Unload() {

    CallGotoOn(CRoute::unload);
}

void CWndRoute::Load() {

    CallGotoOn(CRoute::load);
}

void CWndRoute::CallGotoOn(int iMode) {

    ASSERT_VALID (this);

    if (!m_Btns[iMode].GetToggleState())
        return;

    for (int iOn = 0; iOn < 3; iOn++)
        if (iOn != iMode)
            m_Btns[iOn].SetToggleState(FALSE);

    // get the position selected
    int iNum = m_listbox.GetCurSel();
    POSITION pos;
    if (iNum < 0)
        pos = NULL;
    else {
        pos = m_pVeh->GetRouteList().GetHeadPosition();
        while ((iNum > 0) && (pos != NULL)) {
            m_pVeh->GetRouteList().GetNext(pos);
            iNum--;
        }
    }

    ((CWndArea *) GetParent())->GotoOn(m_pVeh, CWndArea::veh_route, iMode, pos);
    ((CWndArea *) GetParent())->SetButtonState();
}

void CWndRoute::Goto() {

    ASSERT_VALID (this);

    BtnPressed();

    int iSel = m_listbox.GetCurSel();
    POSITION pos = m_pVeh->GetRouteList().GetHeadPosition();
    while ((iSel > 0) && (pos != NULL)) {
        m_pVeh->GetRouteList().GetNext(pos);
        iSel--;
    }
    m_pVeh->SetRoutePos(pos);

    NewRoute(m_pVeh);
}

void CWndRoute::Delete() {

    ASSERT_VALID (this);

    BtnPressed();

    int iSel = m_listbox.GetCurSel();
    int iOldSel = iSel;
    POSITION pos = m_pVeh->GetRouteList().GetHeadPosition();
    while ((iSel > 0) && (pos != NULL)) {
        m_pVeh->GetRouteList().GetNext(pos);
        iSel--;
    }

    if (pos != NULL) {
        if (pos == m_pVeh->GetRoutePos()) {
            POSITION pos_next = pos;
            m_pVeh->GetRouteList().GetNext(pos_next);
            if (pos_next == NULL)
                pos_next = m_pVeh->GetRouteList().GetHeadPosition();
            m_pVeh->SetRoutePos(pos_next);
        }
        CRoute *pR = m_pVeh->GetRouteList().GetAt(pos);
        m_pVeh->GetRouteList().RemoveAt(pos);
        delete pR;
    }

    // rebuild the list
    NewRoute(m_pVeh);

    m_listbox.SetCurSel(iOldSel);
    OnLbnClk();
}

void CWndRoute::Start() {

    ASSERT_VALID (this);

    // ok, if no route set it to auto
    if (m_pVeh->GetRouteList().GetCount() <= 0) {
        Auto();
        return;
    }

    m_pVeh->SetEvent(CVehicle::route);
    ((CWndArea *) GetParent())->SetButtonState();

    // take it from the router
    theGame.m_pHpRtr->MsgTakeVeh(m_pVeh);
    m_pVeh->m_bFlags |= CVehicle::hp_controls;

    if (m_listbox.GetCount() <= 0) {
        m_pVeh->GetRouteList().RemoveAll();
        m_pVeh->SetRoutePos(NULL);
    }

    // get it going
    if (m_pVeh->m_pos) {
        CRoute *pR = m_pVeh->m_route.GetAt(m_pVeh->m_pos);
        if (!pR) {
            TRAP();
            m_pVeh->m_pos = m_pVeh->m_route.GetHeadPosition();
            pR = m_pVeh->m_route.GetAt(m_pVeh->m_pos);
        }
        if (pR)
            m_pVeh->SetDest(pR->GetCoord());
    }

    // we're done
    m_pVeh->m_pWndRoute = NULL;
    DestroyWindow();
}

void CWndRoute::Auto() {

    ASSERT_VALID (this);

    POSITION pos = m_pVeh->GetRouteList().GetHeadPosition();
    while (pos != NULL) {
        CRoute *pR = m_pVeh->GetRouteList().GetNext(pos);
        delete pR;
    }
    m_pVeh->GetRouteList().RemoveAll();
    m_pVeh->SetRoutePos(NULL);

    // rebuild the list
    NewRoute(m_pVeh);

    // give it back to the router
    m_pVeh->DumpContents();

    // we're done
    m_pVeh->m_pWndRoute = NULL;
    DestroyWindow();
}

void CWndRoute::VehDesc(CVehicle *pVeh, POSITION &pos, CString &sLine) {

    ASSERT_VALID (this);
    ASSERT_VALID (pVeh);

    if (pos == pVeh->GetRoutePos())
        sLine = ">>\t";
    else
        sLine = "\t";

    CRoute *pR = pVeh->GetRouteList().GetNext(pos);

    CString sName;
    CBuilding *pBldg = theBuildingHex._GetBuilding(pR->GetCoord());
    if (pBldg != NULL)
        pBldg->GetDesc(sName);
    if (sName.GetLength() > 0)
        sLine += sName;
    else
        sLine += IntToCString(pR->GetCoord().X()) + "," + IntToCString(pR->GetCoord().Y());

    int ID = 0;
    switch (pR->GetRouteType()) {
        case 0 :
            ID = IDS_WAYPOINT;
            break;
        case 1 :
            ID = IDS_UNLOAD;
            break;
        case 2 :
            ID = IDS_LOAD;
            break;
#ifdef _DEBUG
            default:
                TRAP ();
                break;
#endif
    }

    CString sRes;
    sRes.LoadString(ID);
    sLine += "\t" + sRes;
}

void CWndRoute::NewRoute(CVehicle *pVeh) {

    ASSERT_VALID (this);
    ASSERT_VALID (pVeh);

    // fill the listbox
    int iSel = m_listbox.GetCurSel();
    m_listbox.ResetContent();
    POSITION pos = pVeh->GetRouteList().GetHeadPosition();
    while (pos != NULL) {
        CString sLine;
        VehDesc(pVeh, pos, sLine);
        m_listbox.AddString(sLine);
    }
    m_listbox.SetCurSel(iSel + 1);

    // reset the buttons
    OnLbnClk();

    for (int iOn = 0; iOn < 3; iOn++)
        if (m_Btns[iOn].GetToggleState()) {
            CallGotoOn(iOn);
            break;
        }

//BUGBUG	m_Btns[5].EnableWindow (m_listbox.GetCount () > 0);
}

void CWndRoute::BtnPressed() {

    ASSERT_VALID (this);

    for (int iOn = 0; iOn < 3; iOn++)
        m_Btns[iOn].SetToggleState(FALSE);

    ((CWndArea *) GetParent())->SelectOff();
    ((CWndArea *) GetParent())->SetButtonState();
}

void CWndRoute::Invalidate() {

    ASSERT_VALID (this);
    if (m_pVeh == NULL)
        return;

    POSITION pos = m_pVeh->GetRouteList().GetHeadPosition();
    int iIndex = m_listbox.GetCurSel();
    m_listbox.ResetContent();

    while (pos != NULL) {
        CString sLine;
        VehDesc(m_pVeh, pos, sLine);
        m_listbox.AddString(sLine);
    }
    m_listbox.SetCurSel(iIndex);
}

void CWndRoute::OnDestroy() {

    ASSERT_VALID (m_pVeh);
    m_pVeh->m_pWndRoute = NULL;

    // if any area windows have us selected kill it
    POSITION pos;
    for (pos = theAreaList.GetHeadPosition(); pos != NULL;) {
        CWndArea *pWndArea = theAreaList.GetNext(pos);
        ASSERT_VALID (pWndArea);
        if ((pWndArea->GetUnit() == m_pVeh) && (pWndArea->GetMode() == CWndArea::veh_route))
            pWndArea->SelectOff();
    }

    // ok, if no route set it to auto
    if (m_pVeh->GetRouteList().GetCount() <= 0) {
        m_pVeh->SetRoutePos(NULL);
        m_pVeh->DumpContents();
    }

    CWndBase::OnDestroy();
}

