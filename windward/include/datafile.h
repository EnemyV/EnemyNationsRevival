#ifndef DATAFILE_H
#define DATAFILE_H


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "..\lib\_res.h"


const int DEF_COUNTRY_CODE = 9;	// English

class CDataFile : public CObject
{
	private :

	//  The country code which is used when trying to open the 
	//  default file ( which is the language file ).
	int				m_countryCode;

	//  The patch dir which gets searched first.
	CString			*m_pPatchDir;

	//  The actual opened data file.
	CFile		*m_pDataFile;

	// name of the file
	CString		m_sFileName;

	//  A map of files in the data file.
	CMapStringToPtr	*m_pFileMap;

	static char aDatafileMagic[ 4 ];

	int				m_iRifVer;

#ifdef _DEBUG
	long			m_lastPos;				//  Last position seeked to.  For negative seek checking.
	BOOL			m_bNegativeSeekCheck;	//  Whether or not negative seek checking is enabled.

	void			WarnNegativeSeek( const char *pFilename, long oldPos, long newPos );
#endif

	public :

	//  ctor/dtor do nothing.
	CDataFile();
	~CDataFile();

	// get the file name
	char const * GetName () const { return m_sFileName; }

	//  Opens a data file.  pFilename cannot be NULL.  pPatchDir, if
	//  it is not NULL, points to a directory which should be searched
	//  first for files before checking the main data file.
	BOOL	Init( const char *pFilename, int iRifVer, BOOL bErr );
	void _Init( const char *pFilename, const char *pPatchDir, int iRifVer );

	//  Closes an open datafile.
	void Close();

	//  Sets the country code.
	void SetCountryCode( int countryCode );

	//  Opens the file with the given name and returns a pointer to
	//  a CMmio object which points to that file.  If pFilename is NULL,
	//  this routine uses the previously set country code and attempts
	//  to load a file called #.rif, where # is the country code.  If
	//  that fails, it then loads 1.rif, where 1 is the country code
	//  for the US.  Caller is responsible for deleting the CMmio object.
	//  Returns NULL if no file is found.
	CMmio *OpenAsMMIO( const char *pFilename, const char *pRif );
	
	//  Opens the file with the given name and returns a pointer to
	//  a CFile object which points to that file.  Caller is responsible
	//  for deleting the CFile object.  Returns NULL if no file is found.
	CFile *OpenAsFile( const char *pFilename );

	//  Opens the file with the given name and returns a pointer to
	//  a CArchive object which points to that file.  Caller must call
	//  CloseCArchive to close and delete the file - caller should _NOT_
	//  delete the file itself.  Returns NULL if no file is found.
	CArchive *OpenAsCArchive( const char *pFilename );
	
	//  Closes a CArchive returned from OpenAsCArchive.
	void CloseCArchive( CArchive *pArchive );

protected:
	CFile *_OpenAsFile( const char *pFilename );

#ifdef _DEBUG
public:
	void EnableNegativeSeekChecking();
	void DisableNegativeSeekChecking();
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CDlgSelCD dialog

class CDlgSelCD : public CDialog
{
// Construction
public:
	CDlgSelCD(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgSelCD)
	enum { IDD = IDD_SELECT_CD };
	CString	m_strMsg;
	//}}AFX_DATA


// data
	CString		m_sFileName;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgSelCD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgSelCD)
	afx_msg void OnBrowse();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


extern CDataFile theDataFile;

#endif
