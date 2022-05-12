// vptesdoc.cpp : implementation of the CVptestDoc class
//

#include "stdafx.h"
#include "vptest.h"

#include "vptesdoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVptestDoc

IMPLEMENT_DYNCREATE(CVptestDoc, CDocument)

BEGIN_MESSAGE_MAP(CVptestDoc, CDocument)
	//{{AFX_MSG_MAP(CVptestDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVptestDoc construction/destruction

CVptestDoc::CVptestDoc()
{
	// TODO: add one-time construction code here

}

CVptestDoc::~CVptestDoc()
{
}

BOOL CVptestDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CVptestDoc serialization

void CVptestDoc::Serialize(CArchive& ar)
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
// CVptestDoc diagnostics

#ifdef _DEBUG
void CVptestDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CVptestDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVptestDoc commands
