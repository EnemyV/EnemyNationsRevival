// bd.cpp : implementation file
//

#include "stdafx.h"
#include "bd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBD

IMPLEMENT_DYNCREATE( CBD, CSprite )

CBD::CBD()
{
}

CBD::~CBD()
{
}


BEGIN_MESSAGE_MAP( CBD, CSprite )
	//{{AFX_MSG_MAP(CBD)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CBD diagnostics

#ifdef _DEBUG
void CBD::AssertValid() const
{
	CSprite::AssertValid();
}

void CBD::Dump(CDumpContext& dc) const
{
	CSprite::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSprite serialization

void CBD::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// MFC: add storing code here
	}
	else
	{
		// MFC: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CBD commands

BOOL CBD::OnNewDocument()
{
	pSprite8 = new BuildingSprite;
	if ( pSprite8 == NULL )
		return FALSE;

#ifdef CREATE15
	pSprite15 = new BuildingSprite;
	if ( pSprite15 == NULL )
		return FALSE;
#endif

#ifdef CREATE16
	pSprite16 = new BuildingSprite;
	if ( pSprite16 == NULL )
		return FALSE;
#endif

	pSprite24 = new BuildingSprite;
	if ( pSprite24 == NULL )
		return FALSE;

#ifdef CREATE32
	pSprite32 = new BuildingSprite;
	if ( pSprite32 == NULL )
		return FALSE;
#endif

	if ( CSprite::OnNewDocument() == FALSE )
		return FALSE;

	return TRUE;
}

BOOL CBD::OnOpenDocument(LPCTSTR lpszPathName) 
{
	ASSERT( lpszPathName != NULL );

	//  Create the four sprites which will be needed.
	pSprite8 = new BuildingSprite;
	if ( pSprite8 == NULL )
		return FALSE;

#ifdef CREATE15
	pSprite15 = new BuildingSprite;
	if ( pSprite15 == NULL )
		return FALSE;
#endif

#ifdef CREATE16
	pSprite16 = new BuildingSprite;
	if ( pSprite16 == NULL )
		return FALSE;
#endif

	pSprite24 = new BuildingSprite;
	if ( pSprite24 == NULL )
		return FALSE;

#ifdef CREATE32
	pSprite32 = new BuildingSprite;
	if ( pSprite32 == NULL )
		return false;
#endif

	//  Call the base class to load all of the sprites.
	if ( CSprite::OnOpenDocument( lpszPathName ) == FALSE )
		return FALSE;
	
	return TRUE;
}
