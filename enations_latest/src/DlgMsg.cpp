// DlgMsg.cpp : implementation file
//
// CDlgModelessMsg — No longer CDialog-based.
// Creates a simple Win32 popup window with the message text.
// Auto-deletes itself when the window is closed.

#include "stdafx.h"
#include "lastplnt.h"
#include "DlgMsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const char* CDlgModelessMsg::s_className = "ENModelessMsg";
bool CDlgModelessMsg::s_classRegistered = false;

/////////////////////////////////////////////////////////////////////////////
// CDlgModelessMsg

CDlgModelessMsg::CDlgModelessMsg(CWnd* pParent /*=NULL*/)
	: m_hWnd( NULL )
{
	m_sMsg.clear();
}

CDlgModelessMsg::~CDlgModelessMsg()
{
	if ( m_hWnd && ::IsWindow( m_hWnd ) )
		::DestroyWindow( m_hWnd );
}

LRESULT CALLBACK CDlgModelessMsg::WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
	case WM_CREATE:
	{
		CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
		::SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pcs->lpCreateParams );
		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint( hwnd, &ps );
		CDlgModelessMsg* pThis = (CDlgModelessMsg*)::GetWindowLongPtr( hwnd, GWLP_USERDATA );
		if ( pThis )
		{
			RECT rc;
			::GetClientRect( hwnd, &rc );
			::InflateRect( &rc, -10, -10 );
			::DrawTextA( hdc, pThis->m_sMsg.c_str(), -1, &rc, DT_WORDBREAK | DT_CENTER | DT_VCENTER );
		}
		::EndPaint( hwnd, &ps );
		return 0;
	}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_KEYDOWN:
		::DestroyWindow( hwnd );
		return 0;
	case WM_DESTROY:
	{
		CDlgModelessMsg* pThis = (CDlgModelessMsg*)::GetWindowLongPtr( hwnd, GWLP_USERDATA );
		if ( pThis )
		{
			pThis->m_hWnd = NULL;
			delete pThis;
		}
		return 0;
	}
	}
	return ::DefWindowProc( hwnd, msg, wParam, lParam );
}

void CDlgModelessMsg::Create( const char* pMsg )
{
	m_sMsg = pMsg;

	if ( !s_classRegistered )
	{
		WNDCLASSEXA wc = {};
		wc.cbSize        = sizeof( wc );
		wc.lpfnWndProc   = WndProc;
		wc.hInstance      = ::GetModuleHandle( NULL );
		wc.hCursor        = ::LoadCursor( NULL, IDC_ARROW );
		wc.hbrBackground  = (HBRUSH)( COLOR_BTNFACE + 1 );
		wc.lpszClassName  = s_className;
		::RegisterClassExA( &wc );
		s_classRegistered = true;
	}

	// Center on screen
	int w = 320, h = 120;
	int x = ( ::GetSystemMetrics( SM_CXSCREEN ) - w ) / 2;
	int y = ( ::GetSystemMetrics( SM_CYSCREEN ) - h ) / 2;

	m_hWnd = ::CreateWindowExA(
		WS_EX_TOPMOST,
		s_className,
		"Enemy Nations",
		WS_POPUP | WS_BORDER | WS_VISIBLE,
		x, y, w, h,
		NULL, NULL,
		::GetModuleHandle( NULL ),
		this );
}
