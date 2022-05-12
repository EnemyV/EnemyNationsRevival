#ifndef MULTPROG_H
#define MULTPROG_H

class CMultiProgress : public CDialog
{
// Construction
public:
	CMultiProgress(CWnd* pParent = NULL);   // standard constructor
	CMultiProgress( CWnd *pParent, int dirCnt, int fileCnt );

	void UpdateDir( const char *pDirName );
	void StepFile();
	void UpdateFile( const char *pFilename );
	void ResetFile();

// Dialog Data
	//{{AFX_DATA(CMultiProgress)
	enum { IDD = IDD_PROGRESS2 };
	CProgressCtrl	m_fileProgress;
	CProgressCtrl	m_dirProgress;
	CEdit	m_currentFile;
	CEdit	m_currentDir;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMultiProgress)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMultiProgress)
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
