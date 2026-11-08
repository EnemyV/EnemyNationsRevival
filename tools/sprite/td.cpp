// td.cpp : implementation file
//

#include "stdafx.h"
#include "td.h"
#include "terrain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSprite

IMPLEMENT_DYNCREATE( CTD, CSprite )

CTD::CTD()
{
}

CTD::~CTD()				 
{
}


BEGIN_MESSAGE_MAP( CTD, CSprite )
	//{{AFX_MSG_MAP(CTD)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CSprite diagnostics

#ifdef _DEBUG
void CTD::AssertValid() const
{
	CSprite::AssertValid();
}

void CTD::Dump(CDumpContext& dc) const
{
	CSprite::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSprite serialization

void CTD::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CBD commands

BOOL CTD::OnNewDocument()
{
	pSprite8 = new TerrainSprite;
	if ( pSprite8 == NULL )
		return FALSE;

#ifdef CREATE15
	pSprite15 = new TerrainSprite;
	if ( pSprite15 == NULL )
		return FALSE;
#endif

#ifdef CREATE16
	pSprite16 = new TerrainSprite;
	if ( pSprite16 == NULL )
		return FALSE;
#endif

	pSprite24 = new TerrainSprite;
	if ( pSprite24 == NULL )
		return FALSE;

#ifdef CREATE32
	pSprite32 = new TerrainSprite;
	if ( pSprite32 == NULL )
		return FALSE;
#endif

	if ( CSprite::OnNewDocument() == FALSE )
		return FALSE;

	return TRUE;
}

BOOL CTD::OnOpenDocument(LPCTSTR lpszPathName) 
{
	ASSERT( lpszPathName != NULL );

	pSprite8 = new TerrainSprite;
	if ( pSprite8 == NULL )
		return FALSE;

#ifdef CREATE15
	pSprite15 = new TerrainSprite;
	if ( pSprite15 == NULL )
		return FALSE;
#endif

#ifdef CREATE16
	pSprite16 = new TerrainSprite;
	if ( pSprite16 == NULL )
		return FALSE;
#endif

	pSprite24 = new TerrainSprite;
	if ( pSprite24 == NULL )
		return FALSE;

#ifdef CREATE32
	pSprite32 = new TerrainSprite;
	if ( pSprite32 == NULL )
		return FALSE;
#endif

	if (!CSprite::OnOpenDocument(lpszPathName))
		return FALSE;

	return TRUE;
}
