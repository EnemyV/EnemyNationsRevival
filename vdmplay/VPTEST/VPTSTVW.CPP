// vptstvw.cpp : implementation of the CVptstView class
//

#include "stdafx.h"
#include "vptst.h"

#include "vptstdoc.h"
#include "vptstvw.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVptstView

IMPLEMENT_DYNCREATE(CVptstView, CView)

BEGIN_MESSAGE_MAP(CVptstView, CView)
	//{{AFX_MSG_MAP(CVptstView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVptstView construction/destruction

CVptstView::CVptstView()
{
	// TODO: add construction code here
}

CVptstView::~CVptstView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CVptstView drawing

void CVptstView::OnDraw(CDC* pDC)
{
	CVptstDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CVptstView diagnostics

#ifdef _DEBUG
void CVptstView::AssertValid() const
{
	CView::AssertValid();
}

void CVptstView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CVptstDoc* CVptstView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVptstDoc)));
	return (CVptstDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVptstView message handlers
