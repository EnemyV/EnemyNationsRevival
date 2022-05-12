// vptstvw.cpp : implementation of the CVptst16View class
//

#include "stdafx.h"
#include "vptst16.h"

#include "vptstdoc.h"
#include "vptstvw.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CVptst16View

IMPLEMENT_DYNCREATE(CVptst16View, CView)

BEGIN_MESSAGE_MAP(CVptst16View, CView)
	//{{AFX_MSG_MAP(CVptst16View)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVptst16View construction/destruction

CVptst16View::CVptst16View()
{
	// TODO: add construction code here
}

CVptst16View::~CVptst16View()
{
}

/////////////////////////////////////////////////////////////////////////////
// CVptst16View drawing

void CVptst16View::OnDraw(CDC* pDC)
{
	CVptst16Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CVptst16View diagnostics

#ifdef _DEBUG
void CVptst16View::AssertValid() const
{
	CView::AssertValid();
}

void CVptst16View::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CVptst16Doc* CVptst16View::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVptst16Doc)));
	return (CVptst16Doc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVptst16View message handlers



