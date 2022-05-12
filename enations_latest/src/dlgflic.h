//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------

#ifndef __DLGFLIC_H__
#define __DLGFLIC_H__

// dlgflic.h : header file
//

#include <flcctrl.h>

//------------------------------- C F l i c ---------------------------------

class CFlic
{

public:

	CFlic( char const *pszName );

	CString				GetName() const { return m_strName;    }
	Ptr< CFlcInfo >	GetInfo()       { return m_ptrflcinfo;	}

private:

	CString				m_strName;
	Ptr< CFlcInfo >	m_ptrflcinfo;
};

/////////////////////////////////////////////////////////////////////////////
// CDlgFlic dialog

class CDlgFlic : public CDialog
{
// Construction
public:

	static void	Play( CFlic * apflics[], int nFlics = 1 );
	static void	Play( CFlic * );

protected:

	CDlgFlic( CFlic **, int );

	static	void SavePalette();
	static	void RestorePalette();

// Dialog Data
	//{{AFX_DATA(CDlgFlic)
	enum { IDD = IDD_PLAY_FLIC };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgFlic)
	//}}AFX_VIRTUAL

// Implementation
protected:

	BOOL	OnInitDialog();
	void	OnOK();
	void	OnCancel();
	void	OnFlicStop();
	void	OnFlicKey();
	void	Stop();

	// Generated message map functions
	//{{AFX_MSG(CDlgFlic)
	afx_msg void 		OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void 		OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT	OnCtlColorDlg(	WPARAM, LPARAM );
	afx_msg LRESULT	OnPlay		 (	WPARAM, LPARAM );
	afx_msg void		OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

	CFlcCtrl	  	m_flcctrl;
	BOOL		  	m_bStarted;
	CFlic		** m_ppflic;
	int			m_iFlic;
	int			m_nFlics;
};

#endif
