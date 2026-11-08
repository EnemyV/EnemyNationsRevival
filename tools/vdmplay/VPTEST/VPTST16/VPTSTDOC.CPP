// vptstdoc.cpp : implementation of the CVptst16Doc class
//

#include "stdafx.h"
#include "vptst16.h"

#include "vptstdoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVptst16Doc

IMPLEMENT_DYNCREATE(CVptst16Doc, CDocument)

BEGIN_MESSAGE_MAP(CVptst16Doc, CDocument)
	//{{AFX_MSG_MAP(CVptst16Doc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVptst16Doc construction/destruction

CVptst16Doc::CVptst16Doc()
{
	// TODO: add one-time construction code here
}

CVptst16Doc::~CVptst16Doc()
{
}

BOOL CVptst16Doc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CVptst16Doc serialization

void CVptst16Doc::Serialize(CArchive& ar)
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
// CVptst16Doc diagnostics

#ifdef _DEBUG
void CVptst16Doc::AssertValid() const
{
	CDocument::AssertValid();
}

void CVptst16Doc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVptst16Doc commands
