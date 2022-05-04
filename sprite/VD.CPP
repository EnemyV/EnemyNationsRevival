// vd.cpp : implementation file
//

#include "stdafx.h"
#include "vd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSprite

IMPLEMENT_DYNCREATE( CVD, CSprite )

CVD::CVD()
{
}

CVD::~CVD()
{
}


BEGIN_MESSAGE_MAP( CVD, CSprite )
	//{{AFX_MSG_MAP(CVD)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CSprite diagnostics

#ifdef _DEBUG
void CVD::AssertValid() const
{
	CSprite::AssertValid();
}

void CVD::Dump(CDumpContext& dc) const
{
	CSprite::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSprite serialization

void CVD::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// NOTE: add storing code here
	}
	else
	{
		// NOTE: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CVD commands

BOOL CVD::OnNewDocument()
{
	pSprite8 = new VehicleSprite;
	if ( pSprite8 == NULL )
		return FALSE;

#ifdef CREATE15
	pSprite15 = new VehicleSprite;
	if ( pSprite15 == NULL )
		return FALSE;
#endif

#ifdef CREATE16
	pSprite16 = new VehicleSprite;
	if ( pSprite16 == NULL )
		return FALSE;
#endif

	pSprite24 = new VehicleSprite;
	if ( pSprite24 == NULL )
		return FALSE;

#ifdef CREATE32
	pSprite32 = new VehicleSprite;
	if ( pSprite32 == NULL )
		return FALSE;
#endif

	if ( CSprite::OnNewDocument() == FALSE )
		return FALSE;

	return TRUE;
}

BOOL CVD::OnOpenDocument(LPCTSTR lpszPathName) 
{
	ASSERT( lpszPathName != NULL );

	pSprite8 = new VehicleSprite;
	if ( pSprite8 == NULL )
		return FALSE;

#ifdef CREATE15
	pSprite15 = new VehicleSprite;
	if ( pSprite15 == NULL )
		return FALSE;
#endif

#ifdef CREATE16
	pSprite16 = new VehicleSprite;
	if ( pSprite16 == NULL )
		return FALSE;
#endif

	pSprite24 = new VehicleSprite;
	if ( pSprite24 == NULL )
		return FALSE;

#ifdef CREATE32
	pSprite32 = new VehicleSprite;
	if ( pSprite32 == NULL )
		return FALSE;
#endif

	if (!CSprite::OnOpenDocument(lpszPathName))
		return FALSE;
	
	return TRUE;
}
