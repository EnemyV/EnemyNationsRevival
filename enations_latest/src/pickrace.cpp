//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// pickrace.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "pickrace.h"
#include "player.h"
#include "scenario.h"
#include "cutscene.h"
#include "bmbutton.h"
#include "bitmaps.h"
#include "help.h"

#include "creatmul.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CDlgPickRace dialog

CDlgPickRace::CDlgPickRace(CWnd *pParent /*=NULL*/)
        : CDialog(CDlgPickRace::IDD, pParent) {
    //{{AFX_DATA_INIT(CDlgPickRace)
    m_sName = _T("");
    //}}AFX_DATA_INIT

    m_sDesc = _T("");
    m_pPicture = NULL;
}


void CDlgPickRace::DoDataExchange(CDataExchange *pDX) {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgPickRace)
    DDX_Control(pDX, IDOK, m_btnOk);
    DDX_Control(pDX, IDC_RACE_LIST, m_lstRace);
    DDX_Text(pDX, IDC_RACE_NAME, m_sName);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPickRace, CDialog)
                    //{{AFX_MSG_MAP(CDlgPickRace)
                    ON_BN_CLICKED(IDX_HELP, OnHelp)
                    ON_LBN_SELCHANGE(IDC_RACE_LIST, OnSelchangeRaceList)
                    ON_LBN_DBLCLK(IDC_RACE_LIST, OnDblclkRaceList)
                    ON_EN_CHANGE(IDC_RACE_NAME, OnChangeRaceName)
                    ON_BN_CLICKED(IDC_RACE_PREV, OnRacePrev)
                    ON_WM_DESTROY()
                    ON_WM_MEASUREITEM()
                    ON_WM_DRAWITEM()
                    ON_WM_ERASEBKGND()
                    ON_WM_PAINT()
                    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgPickRace message handlers

void CDlgPickRace::Create(CCreateBase *pCb, CDialog *pDlgPrev, UINT id, CWnd *pPar) {

    ASSERT_VALID (this);

    m_pdlgPrev = pDlgPrev;
    m_pCb = pCb;
    CDialog::Create(id, pPar);
}

void CDlgPickRace::OnSelchangeRaceList() {

    ASSERT_VALID (this);

    int iSel = m_lstRace.GetCurSel();
    m_btnOk.EnableWindow((m_sName.GetLength() > 0) && (iSel >= 0));

    if (iSel < 0) {
        TRAP();
        return;
    }
    CRaceDef *pRd = (CRaceDef *) m_lstRace.GetItemDataPtr(iSel);
    ASSERT_VALID (pRd);

    m_sDesc = pRd->GetDesc();
    m_pPicture = pRd->GetPicture();
    InvalidateRect(&m_rPicture);
    InvalidateRect(&m_rText);
}

void CDlgPickRace::OnDblclkRaceList() {

    ASSERT_VALID (this);

    OnSelchangeRaceList();
    OnOK();
}

void CDlgPickRace::OnChangeRaceName() {

    ASSERT_VALID (this);

    UpdateData(TRUE);
    m_btnOk.EnableWindow((m_sName.GetLength() > 0) && (m_lstRace.GetCurSel() >= 0));
}

void CDlgPickRace::OnOK() {

    ASSERT_VALID (this);
    theApp.Log("Picked race");

    if (!m_btnOk.IsWindowEnabled())
        return;

    int iSel = m_lstRace.GetCurSel();
    if (iSel < 0) {
        TRAP();
        return;
    }
    CRaceDef *pRaceDef = (CRaceDef *) m_lstRace.GetItemDataPtr(iSel);
    ASSERT_VALID (pRaceDef);

    // save the parameters we use
    UpdateData(TRUE);
    m_pCb->m_sName = m_sName;
    m_pCb->m_sRace = pRaceDef->GetLine();

    theApp.WriteProfileString("Create", "Name", m_sName);

    // copy the attributes
    theGame.GetMe()->SetName(m_sName);
    theGame.GetMe()->m_InitData.Set(pRaceDef, m_pCb->m_iPos);
    m_pCb->GetNew()->m_InitData.Set(pRaceDef, m_pCb->m_iPos);

    // if it's a scenario we play the cut scene here
    if (theGame.GetScenario() >= 0) {
        // hide this window
        ShowWindow(SW_HIDE);
        theApp.m_wndMain.UpdateWindow();

        // play the cut scene
        if (theCutScene.PlayCutScene(theGame.GetScenario(), FALSE) != CUT_OK) {
            theGame.Close();
            CDialog::OnCancel();
            theApp.CreateMain();
            return;
        }

        // they saw it - let's load & go
    }

    // only if a net game
    if (m_pCb->m_iTyp == CCreateBase::create_net) {

#ifdef LOGGINGON
        OutputDebugStringA( "create_net\n" );
#endif

        theGame.SetNetJoin(m_pCb->m_iJoinUntil);
        m_pCb->UpdateBtns();
    }

    try {
        theGame.IncTry();
        // tell the server that we're ready
        if (m_pCb->m_iTyp == CCreateBase::join_net) {
            CNetReady msg(&(m_pCb->GetNew()->m_InitData));
            if (theNet.Send(theGame.GetServerNetNum(), &msg, sizeof(msg)))
                return;
            theApp.ReadyToJoin();
        } else
            theApp.ReadyToCreate();

        theGame.DecTry();
    }

    catch (int iNum) {
        CatchNum(iNum);
        theApp.CloseWorld();
        return;
    }
    catch (SE_Exception e) {
        CatchSE(e);
        theApp.CloseWorld();
        return;
    }
    catch (...) {
        CatchOther();
        theApp.CloseWorld();
        return;
    }

    // put up final approval window
    if ((m_pCb != NULL) && (m_pCb->m_iTyp == CCreateBase::create_net) && (m_pCb->m_iJoinUntil == 1))
        ((CCreateMultiBase *) m_pCb)->m_wndPlyrList.SetActiveWindow();
}

void CDlgPickRace::OnCancel() {

    ASSERT_VALID (this);

    theGame.Close();
    theNet.Close(FALSE);
    CDialog::OnCancel();
    theApp.CreateMain();
}

void CDlgPickRace::OnRacePrev() {

    m_pdlgPrev->ShowWindow(SW_SHOW);
    ShowWindow(SW_HIDE);
}

BOOL CDlgPickRace::OnInitDialog() {

    CDialog::OnInitDialog();

    CenterWindow(theApp.m_pMainWnd);

    m_pPicture = NULL;
    CRect rect;
    GetClientRect(&rect);
    m_pDib = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                      ptrthebltformat->GetMemDirection(), rect.Width(), rect.Height());

    // rects for painting
    GetDlgItem(IDC_RACE_DESC)->GetClientRect(&m_rFrame);
    GetDlgItem(IDC_RACE_DESC)->MapWindowPoints(this, &m_rFrame);
    m_rText = m_rFrame;
    m_rText.InflateRect(-theBitmaps.GetByIndex(DIB_BORDER_VERT)->GetWidth(),
                        -theBitmaps.GetByIndex(DIB_BORDER_HORZ)->GetHeight());
    m_rPicture = m_rText;
    m_rPicture.left = m_rPicture.right - (130 * m_rText.Height()) / 140;
    m_rText.right = m_rPicture.left - 6;

    // init the radio buttons
    UpdateData(TRUE);
    m_sName = theApp.GetProfileString("Create", "Name");
    UpdateData(FALSE);
    m_btnOk.EnableWindow(FALSE);

    // put them up on the screen
    for (int iOn = 0; iOn < CRaceDef::GetNumRaces(); iOn++) {
        int iSel = m_lstRace.AddString(ptheRaces[iOn].GetLine());
        m_lstRace.SetItemDataPtr(iSel, &(ptheRaces[iOn]));
    }

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgPickRace::OnMeasureItem(int, LPMEASUREITEMSTRUCT lpMIS) {

    ASSERT_VALID (this);

    lpMIS->itemHeight = __max (lpMIS->itemHeight, (UINT) theApp.TextHt());
}

void CDlgPickRace::OnDrawItem(int, LPDRAWITEMSTRUCT pDis) {

    // buttons
    if (ODT_BUTTON == pDis->CtlType) {
        Default();
        return;
    }

    if ((ODT_LISTBOX != pDis->CtlType) || (pDis->itemID < 0))
        return;

    // listbox
    CDC *pDc = CDC::FromHandle(pDis->hDC);
    if (pDc == NULL)
        return;
    thePal.Paint(pDc->m_hDC);

    CFont *pOld = pDc->SelectObject(&(theApp.TextFont()));
    if (pDis->itemState & ODS_SELECTED) {
        pDc->SetTextColor(PALETTERGB (255, 255, 255));
        pDc->SetBkColor(RGB (0, 0, 255));
    } else {
        pDc->SetTextColor(PALETTERGB (0, 0, 255));
        pDc->SetBkColor(RGB (255, 255, 255));
    }

    CRaceDef *pRd = (CRaceDef *) pDis->itemData;

    pDc->ExtTextOut(pDis->rcItem.left, pDis->rcItem.top, ETO_CLIPPED | ETO_OPAQUE,
                    &(pDis->rcItem), pRd->GetLine(), strlen(pRd->GetLine()), NULL);
    pDc->SelectObject(pOld);
    thePal.EndPaint(pDc->m_hDC);
}

BOOL CDlgPickRace::OnEraseBkgnd(CDC *pDc) {

    return (1);
}

void CDlgPickRace::OnPaint() {

    ASSERT_VALID (this);

    CPaintDC dc(this); // device context for painting
    thePal.Paint(dc.m_hDC);

    CRect rect;
    GetClientRect(&rect);

    // paint background (stretch in case...)
    CDIB *pdib = theBitmaps.GetByIndex(DIB_GOLD);

    // GG
    CRect rectSrc = CGlobalSubClass::GetBackgroundSrcRect(rect, rect, pdib->GetRect());

    // pdib->StretchBlt ( m_pDib, rect, pdib->GetRect () );
    pdib->StretchBlt(m_pDib, rect, rectSrc);

    // the border for the text & pic
    PaintBorder(m_pDib, m_rFrame, TRUE);

    // the picture
    if (m_pPicture != NULL)
        m_pPicture->StretchBlt(m_pDib, m_rPicture, m_pPicture->GetRect());

    // text
    CDC *pDc = CDC::FromHandle(m_pDib->GetDC());
    thePal.Paint(pDc->m_hDC);
    pDc->SetBkMode(TRANSPARENT);
    CFont *pOld = pDc->SelectObject(&theApp.TextFont());
    pDc->SetTextColor(PALETTERGB (0, 0, 255));
    pDc->SetBkColor(PALETTERGB (0, 0, 0));
    FitDrawText(pDc, theApp.TextFont(), m_sDesc, m_rText);

    // BLT to the screen
    m_pDib->BitBlt(dc, m_pDib->GetRect(), CPoint(0, 0));

    pDc->SelectObject(pOld);
    thePal.EndPaint(dc.m_hDC);
    if (m_pDib->IsBitmapSelected())
        m_pDib->ReleaseDC();

    // Do not call CWndBase::OnPaint() for painting messages
}

void CDlgPickRace::OnDestroy() {

    delete m_pDib;
    m_pDib = NULL;

    CDialog::OnDestroy();
}

void CDlgPickRace::OnHelp() {

    theApp.WinHelp(HLP_PICK_RACE, HELP_CONTEXT);
}


/////////////////////////////////////////////////////////////////////////////
// CDlgPickPlayer dialog

CDlgPickPlayer::CDlgPickPlayer(CWnd *pParent /*=NULL*/)
        : CDialog(CDlgPickPlayer::IDD, pParent) {
    //{{AFX_DATA_INIT(CDlgPickPlayer)
    m_sDesc = _T("");
    m_sName = _T("");
    //}}AFX_DATA_INIT
}


void CDlgPickPlayer::DoDataExchange(CDataExchange *pDX) {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgPickPlayer)
    DDX_Control(pDX, IDOK, m_btnOk);
    DDX_Control(pDX, IDC_RACE_LIST, m_lstRace);
    DDX_Text(pDX, IDC_RACE_DESC, m_sDesc);
    DDX_Text(pDX, IDC_RACE_NAME, m_sName);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPickPlayer, CDialog)
                    //{{AFX_MSG_MAP(CDlgPickPlayer)
                    ON_BN_CLICKED(IDX_HELP, OnHelp)
                    ON_LBN_SELCHANGE(IDC_RACE_LIST, OnSelchangeRaceList)
                    ON_LBN_DBLCLK(IDC_RACE_LIST, OnDblclkRaceList)
                    ON_EN_CHANGE(IDC_RACE_NAME, OnChangeRaceName)
                    ON_BN_CLICKED(IDC_RACE_PREV, OnRacePrev)
                    ON_WM_DESTROY()
                    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgPickPlayer message handlers

void CDlgPickPlayer::OnSelchangeRaceList() {

    ASSERT_VALID (this);

    UpdateData(TRUE);

    int iSel = m_lstRace.GetCurSel();
    if (iSel < 0) {
        TRAP();
        m_sName = "";
        UpdateData(FALSE);
        m_btnOk.EnableWindow(FALSE);
        return;
    }

    CNetPlyrJoin *pData = (CNetPlyrJoin *) m_lstRace.GetItemDataPtr(iSel);

    if (pData->m_bAvail)
        m_sDesc = "";
    else {
        m_sDesc.LoadString(IDS_PLYR_TAKEN);
        m_sDesc += "\r\n";
    }

    CString sTmp;
    sTmp.LoadString(IDS_BUILDINGS);
    m_sDesc += sTmp + " " + IntToCString(pData->m_iNumBldgs) + "\r\n";
    sTmp.LoadString(IDS_VEHICLES);
    m_sDesc += sTmp + " " + IntToCString(pData->m_iNumVeh) + "\r\n";
    for (int iOn = 0; iOn < CMaterialTypes::GetNumTypes(); iOn++)
        if (pData->m_iMat[iOn] > 0)
            m_sDesc += CMaterialTypes::GetDesc(iOn) + ": " + IntToCString(pData->m_iMat[iOn]) + "\r\n";

    UpdateData(FALSE);

    m_btnOk.EnableWindow((m_sName.GetLength() > 0) && (pData->m_bAvail));
}

void CDlgPickPlayer::OnDblclkRaceList() {

    ASSERT_VALID (this);

    OnSelchangeRaceList();
    OnOK();
}

void CDlgPickPlayer::OnChangeRaceName() {

    ASSERT_VALID (this);

    UpdateData(TRUE);

    int iSel = m_lstRace.GetCurSel();
    if (iSel < 0) {
        m_btnOk.EnableWindow(FALSE);
        return;
    }

    CNetPlyrJoin *pData = (CNetPlyrJoin *) m_lstRace.GetItemDataPtr(iSel);

    m_btnOk.EnableWindow((m_sName.GetLength() > 0) && (pData->m_bAvail));
}

void CDlgPickPlayer::OnOK() {

    ASSERT_VALID (this);
    theApp.Log("Picked player");

    OnChangeRaceName();
    if (!m_btnOk.IsWindowEnabled())
        return;

    int iSel = m_lstRace.GetCurSel();
    if (iSel < 0) {
        TRAP();
        return;
    }
    CNetPlyrJoin *pData = (CNetPlyrJoin *) m_lstRace.GetItemDataPtr(iSel);

    // save the parameters we use
    UpdateData(TRUE);
    m_pCb->m_sName = m_sName;
    theApp.WriteProfileString("Create", "Name", m_sName);

    // we handle join here because it hasn't loaded the game yet
    // ask if we can have this player
    if (m_pCb->m_iTyp == CCreateBase::load_join) {
        // shut this window down while we wait
        m_dlgWait.Create(IDD_PICK_WAIT, this);
        EnableWindow(FALSE);

        // ask for it
        CNetSelectPlyr msg(theGame.GetMyNetNum(), pData->m_iPlyrNum, m_sName);
        theGame.PostToServer(&msg, sizeof(msg));
        return;
    }

    CPlayer *pPlr = theGame.GetPlayerByPlyr(pData->m_iPlyrNum);
    ASSERT_VALID (pPlr);

    // set that we have a Human Player (don't come here if we don't)
    theGame.SetHP(TRUE);
    // turn off scenarios (only get here with multi-player load)
    theGame.SetScenario(-1);

    // switch who's ME
    CPlayer *pPlrWasMe = theGame._GetMe();
    if (pPlr != theGame._GetMe()) {
        theGame._SetMe(pPlr);
        if (theGame.AmServer())
            theGame._SetServer(pPlr);
    }
    pPlr->SetState(CPlayer::ready);
    pPlr->SetName(m_sName);

    // set relations
    POSITION pos;
    for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL;) {
        CPlayer *pPlr = theGame.GetAll().GetNext(pos);
        if (pPlr->IsMe()) {
            pPlr->SetRelations(RELATIONS_ALLIANCE);
            pPlr->SetTheirRelations(RELATIONS_ALLIANCE);
        } else {
            pPlr->SetRelations(RELATIONS_NEUTRAL);
            pPlr->SetTheirRelations(RELATIONS_NEUTRAL);
        }
    }

    try {
        theGame.IncTry();

        switch (m_pCb->m_iTyp) {
            // single player - we can go
            case CCreateBase::load_single :
                m_pCb->ClosePick();
                if (theGame.StartGame(FALSE) != IDOK) {
                    TRAP();
                    throw (0);
                }
                break;

                // server - load game
            case CCreateBase::load_multi : {
                // now we can publish
                pPlr->m_iPerInit = 100;
                if (((CCreateMultiBase *) m_pCb)->m_dlgCreatePublish.m_hWnd == NULL)
                    ((CCreateMultiBase *) m_pCb)->m_dlgCreatePublish.Create((CCreateMultiBase *) m_pCb, NULL,
                                                                            CDlgCreatePublish::IDD, theApp.m_pMainWnd);
                ((CCreateMultiBase *) m_pCb)->m_dlgCreatePublish.ShowWindow(SW_SHOW);
                ShowWindow(SW_HIDE);
                break;
            }

        }

        theGame.DecTry();
    }

    catch (int iNum) {
        TRAP();
        CatchNum(iNum);
        theApp.CloseWorld();
        return;
    }
    catch (SE_Exception e) {
        TRAP();
        CatchSE(e);
        theApp.CloseWorld();
        return;
    }
    catch (...) {
        TRAP();
        CatchOther();
        theApp.CloseWorld();
        return;
    }
}

void CDlgPickPlayer::OnCancel() {

    ASSERT_VALID (this);

    theGame.Close();
    theNet.Close(FALSE);
    CDialog::OnCancel();
    theApp.CloseWorld();
}

void CDlgPickPlayer::OnRacePrev() {

    if (m_pdlgPrev != NULL) {
        m_pdlgPrev->ShowWindow(SW_SHOW);
        ShowWindow(SW_HIDE);
    }
}

BOOL CDlgPickPlayer::OnInitDialog() {

    CDialog::OnInitDialog();

    if (m_pdlgPrev == NULL) {
        GetDlgItem(IDC_RACE_PREV)->EnableWindow(FALSE);
        GetDlgItem(IDC_RACE_PREV)->ShowWindow(SW_HIDE);
    }

    CenterWindow(theApp.m_pMainWnd);

    UpdateData(TRUE);
    m_sName = theApp.GetProfileString("Create", "Name");
    UpdateData(FALSE);

    BOOL bEnb = FALSE;

    if (theGame.AmServer()) {
        // list the players
        POSITION pos;
        for (pos = theGame.GetAll().GetHeadPosition(); pos != NULL;) {
            CPlayer *pPlr = theGame.GetAll().GetNext(pos);
            ASSERT_VALID (pPlr);
            int iSel = m_lstRace.AddString(pPlr->GetName());

            CNetPlyrJoin *pData = CNetPlyrJoin::Alloc(pPlr);
            m_lstRace.SetItemDataPtr(iSel, pData);

            // if it's us select it
            if (pPlr == theGame._GetMe()) {
                bEnb = TRUE;
                m_lstRace.SetCurSel(iSel);
                m_sName = theGame.GetMe()->GetName();
                UpdateData(FALSE);
                OnSelchangeRaceList();
            }
        }
    }

    UpdateData(FALSE);
    m_btnOk.EnableWindow(bEnb);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgPickPlayer::Create(CCreateBase *pCb, UINT id, CWnd *pPar) {

    m_pCb = pCb;
    CDialog::Create(id, pPar);
}

void CDlgPickPlayer::OnDestroy() {

    // delete the attached objects
    int iNum = m_lstRace.GetCount();
    for (int iOn = 0; iOn < iNum; iOn++) {
        char *pData = (char *) m_lstRace.GetItemDataPtr(iOn);
        m_lstRace.SetItemDataPtr(iOn, NULL);
        delete[] pData;
    }

    CDialog::OnDestroy();
}

void CDlgPickPlayer::OnHelp() {

    theApp.WinHelp(HLP_PICK_PLAYER, HELP_CONTEXT);
}


/////////////////////////////////////////////////////////////////////////////
// CDlgPickWait dialog


CDlgPickWait::CDlgPickWait(CWnd *pParent /*=NULL*/)
        : CDialog(CDlgPickWait::IDD, pParent) {
    //{{AFX_DATA_INIT(CDlgPickWait)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CDlgPickWait::DoDataExchange(CDataExchange *pDX) {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgPickWait)
    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPickWait, CDialog)
                    //{{AFX_MSG_MAP(CDlgPickWait)
                    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgPickWait message handlers

void CDlgPickWait::OnCancel() {
    // TODO: Add extra


}
