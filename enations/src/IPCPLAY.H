//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __IPCPLAY_HPP__
#define __IPCPLAY_HPP__

////////////////////////////////////////////////////////////////////////////
//
//  IPCPlay.h :  CIPCPlayer, CIPCPlayerList, CPlyrMsgStatusDlg
//               
//  Last update:    08/25/95
//
////////////////////////////////////////////////////////////////////////////

class CChatWnd;


//
// flags to use in m_wStatus to reflect player's
// status for IPC messages
//
#define IPC_IGNORE_EMAIL	0x0001
#define IPC_IGNORE_CHAT		0x0002
#define IPC_IGNORE_VOICE	0x0004
#define IPC_ACCEPT_EMAIL	0x0008
#define IPC_ACCEPT_CHAT		0x0010
#define IPC_ACCEPT_VOICE	0x0020

/////////////////////////////////////////////////////////////////////////

class CIPCPlayer : public CObject
{
	DECLARE_SERIAL( CIPCPlayer );
public:
	WORD m_wID;		// id of player
	CString m_sName;// name of player
	WORD m_wStatus;	// bitmap of current status
	CPlayer * m_pPlyr;	// player
	CChatWnd *	m_pwndChat;	// chat window to this person

	CIPCPlayer() { m_pwndChat = NULL; };
	//CIPCPlayer( WORD wID );
	CIPCPlayer( const char *pszName, WORD wID );
	~CIPCPlayer();

	virtual void Serialize( CArchive& archive );
};

class CIPCPlayerList : public CObList
{
	DECLARE_SERIAL( CIPCPlayerList );
public:
	CIPCPlayerList() {};
	~CIPCPlayerList();

	CIPCPlayer *GetPlayer( WORD wID );
	CIPCPlayer *GetPlayer( CString& sName );

	void InitPlayers( void );
	void RemovePlayer( WORD wID );
	void RemovePlayer( CString& sName );
	void DeleteList( void );
};

/////////////////////////////////////////////////////////////////////////////
// CPlyrMsgStatusDlg dialog

class CPlyrMsgStatusDlg : public CDialog
{
// Construction
public:
	CPlyrMsgStatusDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPlyrMsgStatusDlg)
	enum { IDD = IDD_MSG_STATUS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPlyrMsgStatusDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPlyrMsgStatusDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangePlayerList();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


extern CIPCPlayerList *plIPCPlayers;

#endif // __IPCPLAY_HPP__
