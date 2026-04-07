// DlgMsg.h : header file
//
// CDlgModelessMsg — notification popup that auto-closes.
// No longer inherits CDialog. Uses a simple Win32 popup window.

/////////////////////////////////////////////////////////////////////////////
// CDlgModelessMsg

class CDlgModelessMsg
{
// Construction
public:
	CDlgModelessMsg(CWnd* pParent = NULL);
	~CDlgModelessMsg();

	void Create ( const char * pMsg );

	CString	m_sMsg;

private:
	HWND m_hWnd;

	static LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static const char* s_className;
	static bool s_classRegistered;
};
