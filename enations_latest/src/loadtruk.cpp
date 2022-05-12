// loadtruk.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "loadtruk.h"
#include "area.h"
#include "chproute.hpp"
#include "unit.inl"

#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgLoadTruck dialog


CDlgLoadTruck::CDlgLoadTruck(CWnd* pParent, CVehicle * pVehPar)
	: CDialog(CDlgLoadTruck::IDD, pParent)
{

	m_pVehPar = pVehPar;
	m_pBldgPar = 0;

	//{{AFX_DATA_INIT(CDlgLoadTruck)
	m_sCoal = "";
	m_sIron = "";
	m_sLumber = "";
	m_sOil = "";
	m_sSteel = "";
	m_sCopper = "";
	m_sMax = "0";
	m_iMaxCoal = 0;
	m_iMaxIron = 0;
	m_iMaxLumber = 0;
	m_iMaxOil = 0;
	m_iMaxSteel = 0;
	m_iMaxCopper = 0;
	//}}AFX_DATA_INIT
}


void CDlgLoadTruck::DoDataExchange(CDataExchange* pDX)
{

	try
		{
		CDialog::DoDataExchange(pDX);
		//{{AFX_DATA_MAP(CDlgLoadTruck)
	DDX_Control(pDX, IDC_TRUCK_AUTO, m_btnAuto);
		DDX_Control(pDX, IDOK, m_btnOk);
		DDX_Control(pDX, IDC_TRUCK_SPIN_XILITIUM, m_spnCopper);
		DDX_Control(pDX, IDC_TRUCK_SPIN_STEEL, m_spnSteel);
		DDX_Control(pDX, IDC_TRUCK_SPIN_OIL, m_spnOil);
		DDX_Control(pDX, IDC_TRUCK_SPIN_LUMBER, m_spnLumber);
		DDX_Control(pDX, IDC_TRUCK_SPIN_IRON, m_spnIron);
		DDX_Control(pDX, IDC_TRUCK_SPIN_COAL, m_spnCoal);
		DDX_Text(pDX, IDC_TRUCK_COAL, m_sCoal);
		DDX_Text(pDX, IDC_TRUCK_IRON, m_sIron);
		DDX_Text(pDX, IDC_TRUCK_LUMBER, m_sLumber);
		DDX_Text(pDX, IDC_TRUCK_OIL, m_sOil);
		DDX_Text(pDX, IDC_TRUCK_STEEL, m_sSteel);
		DDX_Text(pDX, IDC_TRUCK_XILITIUM, m_sCopper);
		DDX_Text(pDX, IDC_TRUCK_HAVE_COAL, m_iMaxCoal);
		DDX_Text(pDX, IDC_TRUCK_HAVE_IRON, m_iMaxIron);
		DDX_Text(pDX, IDC_TRUCK_HAVE_LUMBER, m_iMaxLumber);
		DDX_Text(pDX, IDC_TRUCK_HAVE_OIL, m_iMaxOil);
		DDX_Text(pDX, IDC_TRUCK_HAVE_STEEL, m_iMaxSteel);
		DDX_Text(pDX, IDC_TRUCK_HAVE_XIL, m_iMaxCopper);
		DDX_Text(pDX, IDC_TRUCK_MAX, m_sMax);
	//}}AFX_DATA_MAP
		}

	catch (...)
		{
		}
}


BEGIN_MESSAGE_MAP(CDlgLoadTruck, CDialog)
	//{{AFX_MSG_MAP(CDlgLoadTruck)
	ON_BN_CLICKED(IDC_TRUCK_LOAD_BLDG, OnTruckLoadBldg)
	ON_BN_CLICKED(IDC_TRUCK_LOAD_VEH, OnTruckLoadVeh)
	ON_BN_CLICKED(IDC_TRUCK_AUTO, OnTruckAuto)
	ON_BN_CLICKED(IDC_TRUCK_UNLOAD, OnTruckUnload)
	ON_EN_CHANGE(IDC_TRUCK_COAL, OnChangeItem)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_TRUCK_IRON, OnChangeItem)
	ON_EN_CHANGE(IDC_TRUCK_LUMBER, OnChangeItem)
	ON_EN_CHANGE(IDC_TRUCK_OIL, OnChangeItem)
	ON_EN_CHANGE(IDC_TRUCK_STEEL, OnChangeItem)
	ON_EN_CHANGE(IDC_TRUCK_XILITIUM, OnChangeItem)
	ON_BN_CLICKED(IDC_TRUCK_LOAD, OnTruckLoad)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgLoadTruck message handlers

void CDlgLoadTruck::InitData (CSpinButtonCtrl * pSpn, CString & sCtrl, int idCtrl, int iOn, CBuilding * pBldg, int & iMax)
{

	// set up the spin controls
	pSpn->SetBuddy (GetDlgItem (idCtrl));

	// tell it what the truck has
	sCtrl = IntToCString ( m_pVehPar->GetStore (iOn) );

	// what's our max?
	UpdateItemLimit (pSpn, iOn, pBldg, iMax);
}

void CDlgLoadTruck::UpdateItemLimit (CSpinButtonCtrl * pSpn, int iOn, CBuilding * pBldg, int & iMax)
{

	// what's our max?
	iMax = m_pVehPar->GetStore (iOn);
	if (pBldg != NULL)
		iMax += pBldg->GetStore (iOn);
	pSpn->SetRange (0, iMax);
}

// materials list in building changed
void CDlgLoadTruck::UpdateLimits ()
{

	TRAP ();
	CBuilding * pBldg = theBuildingHex._GetBuilding (m_pVehPar->GetPtHead ());
	UpdateItemLimit (&m_spnCoal, CMaterialTypes::coal, pBldg, m_iMaxCoal );
	UpdateItemLimit (&m_spnIron, CMaterialTypes::iron, pBldg, m_iMaxIron );
	UpdateItemLimit (&m_spnLumber, CMaterialTypes::lumber, pBldg, m_iMaxLumber );
	UpdateItemLimit (&m_spnOil, CMaterialTypes::oil, pBldg, m_iMaxOil );
	UpdateItemLimit (&m_spnSteel, CMaterialTypes::steel, pBldg, m_iMaxSteel );
	UpdateItemLimit (&m_spnCopper, CMaterialTypes::copper, pBldg, m_iMaxCopper );
}

BOOL CDlgLoadTruck::OnInitDialog() 
{

	CDialog::OnInitDialog();

	// change title
	if ( m_pVehPar->GetData ()->IsBoat () )
		{
		TRAP ();
		CString sTitle;
		sTitle.LoadString ( IDS_LOAD_FREIGHTER );
		SetWindowText ( sTitle );
		}

	// lower left of area window
	CWnd * pParWnd = GetParent ();
	if (pParWnd != NULL)	
		{
		CRect rectUs, rectPar;
		pParWnd->GetClientRect (&rectPar);
		pParWnd->ClientToScreen (&rectPar);
		GetWindowRect (&rectUs);
		SetWindowPos (NULL, rectPar.left, rectPar.bottom - rectUs.Height (),
																								0, 0, SWP_NOSIZE | SWP_NOZORDER);
		}

	if ((m_pBldgPar = theBuildingHex._GetBuilding (m_pVehPar->GetPtHead ())) == NULL)
		{
		TRAP ();
		ASSERT (FALSE);
		DestroyWindow ();
		return TRUE;
		}

	// if not ours or alliance we can't load
	if ( m_pBldgPar->GetOwner()->GetTheirRelations () > RELATIONS_ALLIANCE )
		{
		GetDlgItem (IDC_TRUCK_LOAD)->EnableWindow (FALSE);
		GetDlgItem (IDC_TRUCK_LOAD_BLDG)->EnableWindow (FALSE);
		GetDlgItem (IDC_TRUCK_LOAD_VEH)->EnableWindow (FALSE);
		}

	CString sTitle;
	sTitle.LoadString (IDS_LOAD_TRUCK);
	csPrintf ( &sTitle, (char const *) m_pBldgPar->GetData()->GetDesc () );
	SetWindowText ( sTitle );

	// set up the controls
	InitData (&m_spnCoal, m_sCoal, IDC_TRUCK_COAL, CMaterialTypes::coal, m_pBldgPar, m_iMaxCoal);
	InitData (&m_spnIron, m_sIron, IDC_TRUCK_IRON, CMaterialTypes::iron, m_pBldgPar, m_iMaxIron);
	InitData (&m_spnLumber, m_sLumber, IDC_TRUCK_LUMBER, CMaterialTypes::lumber, m_pBldgPar, m_iMaxLumber);
	InitData (&m_spnOil, m_sOil, IDC_TRUCK_OIL, CMaterialTypes::oil, m_pBldgPar, m_iMaxOil);
	InitData (&m_spnSteel, m_sSteel, IDC_TRUCK_STEEL, CMaterialTypes::steel, m_pBldgPar, m_iMaxSteel);
	InitData (&m_spnCopper, m_sCopper, IDC_TRUCK_XILITIUM, CMaterialTypes::copper, m_pBldgPar, m_iMaxCopper);

	m_sMax = IntToCString ( m_pVehPar->GetData()->GetMaxMaterials () );

	UpdateData (FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgLoadTruck::OnTruckLoad() 
{
	
	int iCoal = m_pVehPar->GetStore (CMaterialTypes::coal) + m_pBldgPar->GetStore (CMaterialTypes::coal);
	int iIron = m_pVehPar->GetStore (CMaterialTypes::iron) + m_pBldgPar->GetStore (CMaterialTypes::iron);
	int iLumber = m_pVehPar->GetStore (CMaterialTypes::lumber) + m_pBldgPar->GetStore (CMaterialTypes::lumber);
	int iOil = m_pVehPar->GetStore (CMaterialTypes::oil) + m_pBldgPar->GetStore (CMaterialTypes::oil);
	int iSteel = m_pVehPar->GetStore (CMaterialTypes::steel) + m_pBldgPar->GetStore (CMaterialTypes::steel);
	int iCopper = m_pVehPar->GetStore (CMaterialTypes::copper) + m_pBldgPar->GetStore (CMaterialTypes::copper);

	int iHave = iCoal + iIron + iLumber + iOil + iSteel + iCopper;
	if (iHave > m_pVehPar->GetData()->GetMaxMaterials ())
		{
		float fMul = (float) (m_pVehPar->GetData()->GetMaxMaterials () - 1) / (float) iHave;
		iCoal = (int) ( (float) iCoal * fMul );
		iIron = (int) ( (float) iIron * fMul );
		iLumber = (int) ( (float) iLumber * fMul );
		iOil = (int) ( (float) iOil * fMul );
		iSteel = (int) ( (float) iSteel * fMul );
		iCopper = (int) ( (float) iCopper * fMul );
		}

	m_sCoal = IntToCString ( iCoal );
	m_sIron = IntToCString ( iIron );
	m_sLumber = IntToCString ( iLumber );
	m_sOil = IntToCString ( iOil );
	m_sSteel = IntToCString ( iSteel );
	m_sCopper = IntToCString ( iCopper );

	UpdateData (FALSE);
	OnChangeItem ();
}

void CDlgLoadTruck::OnTruckLoadBldg() 
{

	// none of these
	m_sCoal = m_sIron = m_sOil = m_sCopper = "";

	// initial allocation
	int iSteel = m_pVehPar->GetData()->GetMaxMaterials () / 2;
	int iLumber = m_pVehPar->GetData()->GetMaxMaterials () / 2;

	// make sure we have it
	if (iSteel > m_pVehPar->GetStore (CMaterialTypes::steel) + m_pBldgPar->GetStore (CMaterialTypes::steel)) 
		{
		iSteel = m_pVehPar->GetStore (CMaterialTypes::steel) + m_pBldgPar->GetStore (CMaterialTypes::steel);
		iLumber = m_pVehPar->GetData()->GetMaxMaterials () - iSteel;
		}
	if (iLumber > m_pVehPar->GetStore (CMaterialTypes::lumber) + m_pBldgPar->GetStore (CMaterialTypes::lumber))
		iLumber = m_pVehPar->GetStore (CMaterialTypes::lumber) + m_pBldgPar->GetStore (CMaterialTypes::lumber);

	m_sSteel = IntToCString ( iSteel );
	m_sLumber = IntToCString ( iLumber );

	UpdateData (FALSE);
	OnChangeItem ();
}

void CDlgLoadTruck::OnTruckLoadVeh() 
{

	// none of these
	m_sCoal = m_sIron = m_sLumber = m_sOil = "";

	// initial allocation
	int iSteel = (m_pVehPar->GetData()->GetMaxMaterials () * 4) / 5;
	int iCopper = m_pVehPar->GetData()->GetMaxMaterials () / 5;

	// make sure we have it
	if (iSteel > m_pVehPar->GetStore (CMaterialTypes::steel) + m_pBldgPar->GetStore (CMaterialTypes::steel)) 
		{
		iSteel = m_pVehPar->GetStore (CMaterialTypes::steel) + m_pBldgPar->GetStore (CMaterialTypes::steel);
		iCopper = m_pVehPar->GetData()->GetMaxMaterials () - iSteel;
		}
	if (iCopper > m_pVehPar->GetStore (CMaterialTypes::copper) + m_pBldgPar->GetStore (CMaterialTypes::copper))
		iCopper = m_pVehPar->GetStore (CMaterialTypes::copper) + m_pBldgPar->GetStore (CMaterialTypes::copper);

	m_sSteel = IntToCString ( iSteel );
	m_sCopper = IntToCString ( iCopper );

	UpdateData (FALSE);
	OnChangeItem ();
}

void CDlgLoadTruck::Transfer (int iType, int iAmount)
{

	int iTotal = m_pVehPar->GetStore (iType) + m_pBldgPar->GetStore (iType);
	iAmount = __min ( iAmount, iTotal );
	m_pBldgPar->SetStore (iType, iTotal - iAmount);
	m_pVehPar->SetStore (iType, iAmount);
}

void CDlgLoadTruck::OnOK() 
{

	// TODO: Add extra validation here
	UpdateData (TRUE);

	int iCoal = atoi ( m_sCoal );
	int iIron = atoi ( m_sIron );
	int iLumber = atoi ( m_sLumber );
	int iOil = atoi ( m_sOil );
	int iSteel = atoi ( m_sSteel );
	int iCopper = atoi ( m_sCopper );

	// transfer materials
	Transfer ( CMaterialTypes::coal, iCoal );
	Transfer ( CMaterialTypes::iron, iIron );
	Transfer ( CMaterialTypes::lumber, iLumber );
	Transfer ( CMaterialTypes::oil, iOil );
	Transfer ( CMaterialTypes::steel, iSteel );
	Transfer ( CMaterialTypes::copper, iCopper );

	if ((theGame.GetScenario () == 7) && (m_pBldgPar->GetStore (CMaterialTypes::copper) > 0))
		theGame.m_iScenarioVar++;

	// tell the building it can build
	m_pBldgPar->MaterialMessage ();
	m_pBldgPar->EventOff ();

	// send truck out the exit
	m_pVehPar->ExitBuilding ();

	// put us in the area window with the unit selected
	CWndArea * pWnd = theAreaList.BringToTop ();
	if (pWnd != NULL)
		pWnd->OnlySelectUnit (m_pVehPar);
	
	m_pVehPar->NullLoadWindow ();

	CDialog::OnOK();
	DestroyWindow ();
}

void CDlgLoadTruck::OnCancel() 
{

	// kill it (nothing else because we don't actually move until OnOK)
	m_pVehPar->NullLoadWindow ();
	CDialog::OnCancel();
	DestroyWindow ();
}

void CDlgLoadTruck::OnTruckAuto() 
{

	// TODO: Add your control notification handler code here
	
	POSITION pos = m_pVehPar->GetRouteList().GetHeadPosition ();
	while (pos != NULL)
		{
		CRoute * pR = m_pVehPar->GetRouteList().GetNext (pos);
		delete pR;
		}
	m_pVehPar->GetRouteList().RemoveAll ();
	m_pVehPar->SetRoutePos (NULL);

	// give it back to the router
	m_pVehPar->DumpContents ();

	// close window, etc
	UpdateData (TRUE);
	m_sCoal = m_sIron = m_sLumber = m_sOil = m_sSteel = m_sCopper = "";
	UpdateData (FALSE);

	OnOK ();

	// select nothing
	CWndArea * pWnd = theAreaList.BringToTop ();
	if (pWnd != NULL)
		pWnd->SelectNone ();
}

void CDlgLoadTruck::OnTruckUnload() 
{

	// unload everything
	m_sCoal = m_sIron = m_sLumber = m_sOil = m_sSteel = m_sCopper = "";
	UpdateData (FALSE);

	OnChangeItem ();
}

void CDlgLoadTruck::OnChangeItem() 
{

	if (m_pBldgPar == NULL)
		return;

	UpdateData (TRUE);

	int iCoal = atoi ( m_sCoal );
	int iIron = atoi ( m_sIron );
	int iLumber = atoi ( m_sLumber );
	int iOil = atoi ( m_sOil );
	int iSteel = atoi ( m_sSteel );
	int iCopper = atoi ( m_sCopper );

	// any < 0
	if ((iCoal < 0) || (iIron < 0) || (iLumber < 0) || (iOil < 0) || (iSteel < 0) || (iCopper < 0))
		{
		m_btnOk.EnableWindow (FALSE);
		return;
		}

	// can we carry this much?	
	if (iCoal+iIron+iLumber+iOil+iSteel+iCopper > m_pVehPar->GetData()->GetMaxMaterials ())
		{
		m_btnOk.EnableWindow (FALSE);
		return;
		}

	// does the building have this much?
	if ((iCoal > m_pVehPar->GetStore (CMaterialTypes::coal) + m_pBldgPar->GetStore (CMaterialTypes::coal)) ||
			(iIron > m_pVehPar->GetStore (CMaterialTypes::iron) + m_pBldgPar->GetStore (CMaterialTypes::iron)) ||
			(iLumber > m_pVehPar->GetStore (CMaterialTypes::lumber) + m_pBldgPar->GetStore (CMaterialTypes::lumber)) ||
			(iOil > m_pVehPar->GetStore (CMaterialTypes::oil) + m_pBldgPar->GetStore (CMaterialTypes::oil)) ||
			(iSteel > m_pVehPar->GetStore (CMaterialTypes::steel) + m_pBldgPar->GetStore (CMaterialTypes::steel)) ||
			(iCopper > m_pVehPar->GetStore (CMaterialTypes::copper) + m_pBldgPar->GetStore (CMaterialTypes::copper)))
		{
		m_btnOk.EnableWindow (FALSE);
		return;
		}

	// if it's not alliance
	if ( m_pBldgPar->GetOwner()->GetTheirRelations () > RELATIONS_ALLIANCE )
		if ( ( iCoal > m_pVehPar->GetStore (CMaterialTypes::coal) ) ||
						( iIron > m_pVehPar->GetStore (CMaterialTypes::iron) ) ||
						( iLumber > m_pVehPar->GetStore (CMaterialTypes::lumber) ) ||
						( iOil > m_pVehPar->GetStore (CMaterialTypes::oil) ) ||
						( iSteel > m_pVehPar->GetStore (CMaterialTypes::steel) ) ||
						( iCopper > m_pVehPar->GetStore (CMaterialTypes::copper) ) )
			{
			m_btnOk.EnableWindow (FALSE);
			return;
			}

	// it's ok
	m_btnOk.EnableWindow (TRUE);
}

void CDlgLoadTruck::OnDestroy() 
{

	// take us out of vehicle
	m_pVehPar->NullLoadWindow ();

	CDialog::OnDestroy();
}

