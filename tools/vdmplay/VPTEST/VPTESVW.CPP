// vptesvw.cpp : implementation of the CVptestView class
//

#include "stdafx.h"
#include "vptest.h"

#include "vptesdoc.h"
#include "vptesvw.h"
#include "joindial.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVptestView

IMPLEMENT_DYNCREATE(CVptestView, CView)

BEGIN_MESSAGE_MAP(CVptestView, CView)
	//{{AFX_MSG_MAP(CVptestView)
	ON_COMMAND(ID_VDMPLAY_CLEANUP, OnVdmplayCleanup)
	ON_COMMAND(ID_VDMPLAY_CLOSESESSION, OnVdmplayClosesession)
	ON_COMMAND(ID_VDMPLAY_CREATESESSION, OnVdmplayCreatesession)
	ON_COMMAND(ID_VDMPLAY_ENUMSESSIONS, OnVdmplayEnumsessions)
	ON_COMMAND(ID_VDMPLAY_JOINSESSION, OnVdmplayJoinsession)
	ON_COMMAND(ID_VDMPLAY_SENDMESSAGE, OnVdmplaySendmessage)
	ON_COMMAND(ID_VDMPLAY_STARTUP, OnVdmplayStartup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVptestView construction/destruction

CVptestView::CVptestView()
{
	// TODO: add construction code here

}

CVptestView::~CVptestView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CVptestView drawing

void CVptestView::OnDraw(CDC* pDC)
{
	CVptestDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CVptestView diagnostics

#ifdef _DEBUG
void CVptestView::AssertValid() const
{
	CView::AssertValid();
}

void CVptestView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CVptestDoc* CVptestView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVptestDoc)));
	return (CVptestDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVptestView message handlers

void CVptestView::OnVdmplayCleanup() 
{
	// TODO: Add your command handler code here
	
}

void CVptestView::OnVdmplayClosesession() 
{
	// TODO: Add your command handler code here
	
}

void CVptestView::OnVdmplayCreatesession() 
{
	// TODO: Add your command handler code here
	
}

void CVptestView::OnVdmplayEnumsessions() 
{
	// TODO: Add your command handler code here
	
}

void CVptestView::OnVdmplayJoinsession() 
{
	CJoinDialog dlg;

	dlg.DoModal();

	// TODO: Add your command handler code here
	
}

void CVptestView::OnVdmplaySendmessage() 
{
	// TODO: Add your command handler code here
	
}

void CVptestView::OnVdmplayStartup() 
{
	// TODO: Add your command handler code here
	
}
