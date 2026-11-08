// NamngDlg.h : header file
//

#ifndef NAMNGDLG_H
#define NAMNGDLG_H

#include "common.h"

////////////////////////////////////////////////////////////////////////////
// CNamingDlg dialog

class CNamingDlg : public CDialog
{
// Construction
public:
	CNamingDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNamingDlg)
	enum { IDD = IDD_NAMING };
	CEdit	m_underAnim2Cnt;
	CEdit	m_underAnim1Cnt;
	CEdit	m_anim2cnt;
	CEdit	m_basecnt;
	CEdit	m_anim1cnt;
	CEdit	m_filename;
	//}}AFX_DATA

	int		_stage, _buildType;
	int		_dir, _tilt;
	int		_damage;
	bool	_overlay, _anim1, _anim2, _base, _underAnim1, _underAnim2;
	int		_baseindex, _anim1index, _anim2index, _underAnim1Index, _underAnim2Index;
	bool	_bEditing;

	void UpdateFilename();
	void ParseFilename();
	void ClearEdits();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNamingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNamingDlg)
	afx_msg void OnConstruction();
	afx_msg void OnDone();
	afx_msg void OnFoundation();
	afx_msg void OnSkeleton();
	afx_msg void OnEast();
	afx_msg void OnNorth();
	afx_msg void OnNortheast();
	afx_msg void OnNorthwest();
	afx_msg void OnSouth();
	afx_msg void OnSoutheast();
	afx_msg void OnSouthwest();
	afx_msg void OnWest();
	afx_msg void OnDestroyed();
	afx_msg void OnHeavydamage();
	afx_msg void OnMinordamage();
	afx_msg void OnNodamage();
	afx_msg void OnAnim1();
	afx_msg void OnAnim2();
	afx_msg void OnBase();
	afx_msg void OnChangeAnim1cnt();
	afx_msg void OnSetfocusAnim1cnt();
	afx_msg void OnChangeBasecnt();
	afx_msg void OnSetfocusBasecnt();
	afx_msg void OnChangeAnim2cnt();
	afx_msg void OnSetfocusAnim2cnt();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeFilename();
	afx_msg void OnSetfocusFilename();
	afx_msg void OnKillfocusFilename();
	afx_msg void OnFlat();
	afx_msg void OnTiltaway();
	afx_msg void OnTilttoward();
	afx_msg void OnOnepiece();
	afx_msg void OnTwopiecefore();
	afx_msg void OnTwopieceback();
	afx_msg void OnOverlay();
	afx_msg void OnUnderanim1();
	afx_msg void OnUnderanim2();
	afx_msg void OnChangeUnderanim1cnt();
	afx_msg void OnSetfocusUnderanim1cnt();
	afx_msg void OnChangeUnderanim2cnt();
	afx_msg void OnSetfocusUnderanim2cnt();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
