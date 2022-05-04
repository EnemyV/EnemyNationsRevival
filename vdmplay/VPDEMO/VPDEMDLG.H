// vpdemdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVpdemoDlg dialog
class CVPTransfer;
class CLoadTest;

class CVpdemoDlg : public CDialog
{
// Construction
public:
	CVpdemoDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CVpdemoDlg)
	enum { IDD = IDD_VPDEMO_DIALOG };
	CStatic	m_textFieldTitle;
	CListBox	m_eventLog;
	CEdit	m_playerNameField;
	CEdit	m_textField;
	CListBox	m_sessionBox;
	CListBox	m_playersBox;
	CButton	m_sendMsg;
	CButton	m_join;
	CButton	m_createSession;
	CString	m_selectedSession;
	CString	m_selectedPlayer;
	CString	m_playerName;
	BOOL	m_mustDeliver;
	BOOL	m_broadcast;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVpdemoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CVpdemoDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSendMsg();
	afx_msg void OnCreateSession();
	afx_msg void OnJoin();
	afx_msg LONG OnVpNotify(WPARAM w, LPARAM l);
	afx_msg LONG OnVpFlowOFF(WPARAM w, LPARAM l);
	afx_msg LONG OnVpFlowON(WPARAM w, LPARAM l);
	virtual void OnOK();
	afx_msg void OnRescan();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSendbig();
	afx_msg void OnLoadTest();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:

	void InitVpStuff();
	BOOL EnumSessions();
	void Log(LPCSTR str);
	void    GetServerAddress();

protected:

	void OnSessionEnumNotify(LPCVPMESSAGE );
	void OnPlayerEnumNotify(LPCVPMESSAGE );
	void OnJoinNotify(LPCVPMESSAGE );
	void OnLeaveNotify(LPCVPMESSAGE );
	void OnReadDataNotify(LPCVPMESSAGE );
	void OnSendDataNotify(LPCVPMESSAGE );
	void OnCloseSessionNotify(LPCVPMESSAGE );
	void OnServerDownNotify(LPCVPMESSAGE m);
	void OnNetDownNotify(LPCVPMESSAGE m);

	void DoJoin();
	void DoDisconnect();
	void DoAddPlayer();

	void ClearSessionList();
	void ClearPlayerList();
	BOOL GetPlayerName(VPPLAYERID id, CString& pName);
	VPPLAYERID GetPlayerId(const CString& pName);

	CString GetAddressString(LPCVPNETADDRESS a);
	CString GetStationAddress();

	int GetTransportKind();


	struct SessionData
	{
		int index;
		VPSESSIONID id;
	};

	struct PlayerData
	{
		int index;
		VPPLAYERID id;
	};

	enum { STARTED, ENUMERATING, CLIENT, SERVER };

	int        m_vpstate;
	VPHANDLE   m_vpH;
	VPSESSIONHANDLE  m_vpSession;
	CTypedPtrMap<CMapStringToPtr, CString, SessionData*>  m_sessions;
	CTypedPtrMap<CMapStringToPtr, CString, PlayerData*>  m_players;
	CVPTransfer* m_xfer;
	short*		 m_bigBuf;
	CLoadTest*	m_loadTest;
	VPPLAYERID m_myId;
	BOOL	   m_flowOff;
};
	   
