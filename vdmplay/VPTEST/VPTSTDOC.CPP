// vptstdoc.cpp : implementation of the CVptstDoc class
//

#include "stdafx.h"
#include "vptst.h"

#include "vptstdoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVptstDoc

IMPLEMENT_DYNCREATE(CVptstDoc, CDocument)

BEGIN_MESSAGE_MAP(CVptstDoc, CDocument)
	//{{AFX_MSG_MAP(CVptstDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVptstDoc construction/destruction

CVptstDoc::CVptstDoc()
{
	// TODO: add one-time construction code here
}

CVptstDoc::~CVptstDoc()
{
}

BOOL CVptstDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CVptstDoc serialization

void CVptstDoc::Serialize(CArchive& ar)
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
// CVptstDoc diagnostics

#ifdef _DEBUG
void CVptstDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CVptstDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVptstDoc commands
