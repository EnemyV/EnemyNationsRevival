//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "_windwrd.h"
#include "io.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CDataFile theDataFile;

char CDataFile::aDatafileMagic[ 4 ] = { 'W','S','D','F' };

CDataFile::CDataFile()
{
	m_countryCode = DEF_COUNTRY_CODE;
	m_pPatchDir = NULL;
	m_pDataFile = NULL;
	m_pFileMap = NULL;
#ifdef _DEBUG
	m_bNegativeSeekCheck = FALSE;
	m_lastPos = 0;
#endif
}

CDataFile::~CDataFile()
{
	//  Call close, just in case the file
	//  wasn't closed.  Close is safe to call
	//  multiple times.
	Close();
}

static BOOL GetFileName ( CString & strFileName )
{

	CString sTmp (strFileName);
	_fullpath (strFileName.GetBuffer (258), sTmp, 256);
	strFileName.ReleaseBuffer (-1);

	// get the drive name
	CString sDrive (strFileName);
	int iInd = sDrive.Find ('\\');
	if (iInd >= 0)
		sDrive.ReleaseBuffer ( iInd + 1 );

	// if it's a CD we prompt them to insert the CD
	if (GetDriveType (sDrive) == DRIVE_CDROM)
		{
		CDlgSelCD dlg;
		dlg.m_sFileName = strFileName;
		dlg.m_strMsg.LoadString (IDS_INSERT_CD);
		char sBuf[2];
		sBuf[0] = toupper (strFileName[0]);
		sBuf[1] = 0;
		csPrintf (&dlg.m_strMsg, (char const *) sBuf);
		if (dlg.DoModal () != IDOK)
			{
			TRAP ();
			return (FALSE);
			}

		strFileName = dlg.m_sFileName;
		}
	else

		// not a CD - prompt for the location
		{
		CString sMsg;
		sMsg.LoadString (IDS_BAD_DATA_FILE);
		csPrintf (&sMsg, (char const *) strFileName);
		if (AfxMessageBox (sMsg, MB_YESNO | MB_ICONSTOP) != IDYES)
			return (FALSE);

		CString sFilters;
		sFilters.LoadString (IDS_DATA_FILTERS);
		CFileDialog dlg (TRUE, "dat", strFileName, OFN_FILEMUSTEXIST, sFilters, NULL);	
		if (dlg.DoModal () != IDOK)
			return (FALSE);
		strFileName = dlg.GetPathName ();
		}

	return (TRUE);
}

BOOL CDataFile::Init( const char *pFilename, int iRifVer, BOOL bErr )
{

	m_iRifVer = iRifVer;

	CString strFileName;
	if ( ( ! bErr ) && ( ptheApp->m_lpCmdLine[0] ) )
		{
		TRAP ();
		strFileName = ptheApp->m_lpCmdLine;
		}
	else
		strFileName = ptheApp->GetProfileString ("Game", "DataFile", CString (".\\") + pFilename);
	CString sPatch = ptheApp->GetProfileString ("Game", "Patch", "data");
	if (! sPatch.IsEmpty ())
		if (sPatch[sPatch.GetLength()-1] == '\\')
			sPatch.ReleaseBuffer (sPatch.GetLength()-1);

	// if it's an error we force looking for a new file
	if (bErr)
		if ( ! GetFileName ( strFileName ) )
			return (FALSE);
			
	// open the data file
	for (; TRUE; )
		{
		try
			{
			theDataFile._Init (strFileName, sPatch, iRifVer);
			ptheApp->WriteProfileString ("Game", "DataFile", strFileName);
			return (TRUE);
			}

		catch (...)
			{
			if ( ! GetFileName ( strFileName ) )
				return (FALSE);
			}
		}

	ASSERT (FALSE);
	return (FALSE);
}

void CDataFile::_Init( const char *pFilename, const char *pPatchDir, int iRifVer )
{

	m_iRifVer = iRifVer;

	if ( pFilename )
	{
		m_pDataFile = new CStdioFile;

		if ( m_pDataFile == NULL )
			ThrowError ( ERR_OUT_OF_MEMORY );
		if ( m_pDataFile->Open( pFilename, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) == FALSE )
			ThrowError( ERR_DATAFILE_OPEN );
		m_sFileName = pFilename;

		m_pFileMap = new CMapStringToPtr;
		if ( m_pFileMap == NULL )
			ThrowError( ERR_OUT_OF_MEMORY );

		//  The two entries in the file should be the magic number of the file and the 
		//  size of the header in bytes.
		struct
		{
			char	aMagicNum[ 4 ];
			long	tableSize;
		} dfHdr;
		if ( m_pDataFile->Read( &dfHdr, sizeof( dfHdr) ) != sizeof( dfHdr ) )
			ThrowError( ERR_DATAFILE_READ );

		if ( strncmp( dfHdr.aMagicNum, aDatafileMagic, 4 ) != 0 )
			ThrowError( ERR_DATAFILE_BAD_MAGIC );

		//  Read in the entire file table before constructing the map.
		//  The input file is buffered, but this will still be faster.
		char *pFileTableBuff = new char[ dfHdr.tableSize ];
		if ( pFileTableBuff == NULL )
			ThrowError( ERR_OUT_OF_MEMORY );
		if ( m_pDataFile->Read( pFileTableBuff, dfHdr.tableSize ) != ( UINT )dfHdr.tableSize )
			ThrowError( ERR_DATAFILE_READ );

		char *pBuff = pFileTableBuff;
		while( pBuff < pFileTableBuff + dfHdr.tableSize )
		{
			//  Get the string length
			long stringLength = *( long * )pBuff;
			pBuff += sizeof( long );

			//  Save a pointer to the string
			char *pStr = pBuff;
			pBuff += stringLength;

			//  Get the offset.
			long fileOffset = *( long * )pBuff;
			pBuff += sizeof( long );

			//  Add the string/offset pair to the map.
			//  Can ThrowError CMemoryException.
			strlwr (pStr);
			(*m_pFileMap)[ pStr ] = ( void * )fileOffset;
		}

		//  Delete the file table buffer, which is no longer
		//  needed.
		delete[] pFileTableBuff;
	}

	if ( pPatchDir )
	{
		m_pPatchDir = new CString( pPatchDir );
		m_pPatchDir->MakeLower ();
		if ( m_pPatchDir == NULL )
			ThrowError( ERR_OUT_OF_MEMORY );
	}
}

void CDataFile::Close()
{
	delete m_pDataFile;
	m_pDataFile = NULL;

	delete m_pFileMap;
	m_pFileMap = NULL;

	delete m_pPatchDir;
	m_pPatchDir = NULL;
}

void CDataFile::SetCountryCode( int countryCode )
{
	m_countryCode = countryCode;
}

CMmio *CDataFile::OpenAsMMIO( const char *pFilename, const char *pRif )
{
	//  Get the relative path of the file for which to search.
	CString file( pFilename );
	CString path;
	if ( pFilename == NULL )
		path.Format( "language\\%d\\%d.rif", m_countryCode, m_countryCode );
	else
		path = file + "\\" + file + ".rif";
	path.MakeLower ();

	//  Check to see if the file exists in the patch directory.
	if ( m_pPatchDir )
	{
		CString patchPath = *m_pPatchDir + "\\" + path;

		CFile	test;
		if ( test.Open( patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) != FALSE )
		{
			//  Close the file so we can re-open it as an mmio file.
			//  Otherwise we'd need to keep the CFile around so it's
			//  destructor wouldn't close the file handle.
			test.Close();
			CMmio *pNewFile = new CMmio( patchPath );
			if ( pNewFile == NULL )
				ThrowError( ERR_OUT_OF_MEMORY );

			// does the version match?
			try
				{
				pNewFile->DescendRiff ( pRif );
				pNewFile->DescendList ('F', 'V', 'E', 'R');
				pNewFile->DescendChunk ('D', 'A', 'T', 'A');
				int iRifVer = pNewFile->ReadShort ();
				pNewFile->AscendChunk ();
				pNewFile->AscendList ();

				if ( iRifVer == m_iRifVer )
					{
					// go back to begining (hack!!)
					delete pNewFile;
					pNewFile = new CMmio( patchPath );
					if ( pNewFile == NULL )
						ThrowError( ERR_OUT_OF_MEMORY );
					return pNewFile;
					}
				}

			catch (...)
				{
				}

			delete pNewFile;
		}

		// we now look in the patch dir (users version)
		if ( pFilename == NULL )
			patchPath.Format( "%s\\%d.rif", (char const *) (*m_pPatchDir), m_countryCode );
		else
			patchPath = *m_pPatchDir + CString ("\\") + file + ".rif";

		if ( test.Open( patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) != FALSE )
		{
			//  Close the file so we can re-open it as an mmio file.
			//  Otherwise we'd need to keep the CFile around so it's
			//  destructor wouldn't close the file handle.
			test.Close();
			CMmio *pNewFile = new CMmio( patchPath );
			if ( pNewFile == NULL )
				ThrowError( ERR_OUT_OF_MEMORY );

			// does the version match?
			try
				{
				pNewFile->DescendRiff ( pRif );
				pNewFile->DescendList ('F', 'V', 'E', 'R');
				pNewFile->DescendChunk ('D', 'A', 'T', 'A');
				int iRifVer = pNewFile->ReadShort ();
				pNewFile->AscendChunk ();
				pNewFile->AscendList ();

				if ( iRifVer == m_iRifVer )
					{
					// go back to begining (hack!!)
					delete pNewFile;
					pNewFile = new CMmio( patchPath );
					if ( pNewFile == NULL )
						ThrowError( ERR_OUT_OF_MEMORY );
					return pNewFile;
					}
				}

			catch (...)
				{
				}

			delete pNewFile;
		}
	}

	//  If here, the file does not exist in the 
	//  patch directory or there is no patch directory.
	//  If a datafile was opened, look for it in the
	//  datafile.
	if ( m_pDataFile )
	{
		//  Look for the file path in the map.  If 
		//  it is not found, if searching for language 
		//  file try searching for US version;  otherwise,
		//  return NULL.
		void *	dummy;
		if ( m_pFileMap->Lookup( path, dummy ) == FALSE )
		{
			if ( pFilename == NULL && m_countryCode != DEF_COUNTRY_CODE )
			{	
				m_countryCode = DEF_COUNTRY_CODE;
				return OpenAsMMIO( NULL, pRif );
			}
			ThrowError( ERR_DATAFILE_NO_ENTRY );
		}

		//  If here, file was found in the map.  Seek to
		//  that position in the file.
		//  If negative seek checks between CMmio's are 
		//  desired, they should be done here.
		long fileOffset = ( long )dummy;
		long	offsetFromHere = fileOffset - ( long )m_pDataFile->GetPosition();

#ifdef _DEBUG
		if ( m_bNegativeSeekCheck == TRUE && fileOffset < m_lastPos )
			WarnNegativeSeek( pFilename, m_lastPos, fileOffset );
#endif

		m_pDataFile->Seek( offsetFromHere, CFile::current );

#ifdef _DEBUG
		m_lastPos = m_pDataFile->GetPosition();
#endif

		//  Create a new CMmioEmbeddedFile object.  It will be positioned 
		//  at the current file offset.
		// BUGBUG - problem
		CMmio *pNewFile = new CMmioEmbeddedFile( m_pDataFile->m_hFile, m_sFileName );
		if ( pNewFile == NULL )
			ThrowError( ERR_OUT_OF_MEMORY );

		return pNewFile;
	}

	//  If here, file was not found in patch dir ( or
	//  no patch dir was given ), and no datafile was
	//  opened, so return NULL ( no file found ).
	ThrowError( ERR_DATAFILE_NO_ENTRY );
	return NULL;
}

CFile *CDataFile::OpenAsFile( const char *pFilename )
{

	// if fully qualified just try it
	if ( (pFilename != NULL) && ( (*(pFilename + 1) == ':') || (*(pFilename + 1) == '\\') ) )
		{
		CFile	test;
		if ( test.Open( pFilename, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) != FALSE )
			{
			//  Close the file so we can allocate a new CFile object to
			//  return to the caller.
			test.Close();

			//  Allocate a new CFile object.  By creating and opening the CFile
			//  this way, the CFile dtor will automagically close the file 
			//  handle for us.
			CFile *pNewFile = new CFile;
			if ( pNewFile == NULL )
				ThrowError( ERR_OUT_OF_MEMORY );

			if ( pNewFile->Open( pFilename, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) == FALSE )
				ThrowError( ERR_PATCHFILE_OPEN );

			return pNewFile;
			}
		}

	// open a FILE
	CString filename( pFilename );
	CString path;
	path = "files\\" + filename;
	path.MakeLower ();

	CFile * pRtn = _OpenAsFile( path );
	if (pRtn != NULL)
		return (pRtn);
		
	// open a RIF
	CString file( pFilename );
	if ( pFilename == NULL )
		path.Format( "language\\%d\\%d.rif", m_countryCode, m_countryCode );
	else
		path = file + "\\" + file + ".rif";
	path.MakeLower ();

	if ((pRtn = _OpenAsFile( path )) != NULL)
		return (pRtn);

	ThrowError( ERR_DATAFILE_NO_ENTRY );
	return NULL;
}

CFile *CDataFile::_OpenAsFile( const char *pFilename )
{
	ASSERT( pFilename );

	//  Check to see if the file exists in the patch directory.
	if ( m_pPatchDir )
	{
		CString patchPath = *m_pPatchDir + "\\" + pFilename;

		CFile	test;
		if ( test.Open( patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) != FALSE )
		{
			//  Close the file so we can allocate a new CFile object to
			//  return to the caller.
			test.Close();

			//  Allocate a new CFile object.  By creating and opening the CFile
			//  this way, the CFile dtor will automagically close the file 
			//  handle for us.
			CFile *pNewFile = new CFile;
			if ( pNewFile == NULL )
				ThrowError( ERR_OUT_OF_MEMORY );

			if ( pNewFile->Open( patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) == FALSE )
				ThrowError( ERR_PATCHFILE_OPEN );

			return pNewFile;
		}

		// we now look in the patch dir (users version)
		const char * pName = strrchr ( pFilename, '\\' );
		if ( pName == NULL )
			pName = pFilename;
		else
			pName ++;
		patchPath = *m_pPatchDir + CString ("\\") + pName;

		if ( test.Open( patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) != FALSE )
		{
			//  Close the file so we can allocate a new CFile object to
			//  return to the caller.
			test.Close();

			//  Allocate a new CFile object.  By creating and opening the CFile
			//  this way, the CFile dtor will automagically close the file 
			//  handle for us.
			CFile *pNewFile = new CFile;
			if ( pNewFile == NULL )
				ThrowError( ERR_OUT_OF_MEMORY );

			if ( pNewFile->Open( patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) == FALSE )
				ThrowError( ERR_PATCHFILE_OPEN );

			return pNewFile;
		}
	}

	//  If here, the file does not exist in the 
	//  patch directory or there is no patch directory.
	//  If a datafile was opened, look for it in the
	//  datafile.
	if ( m_pDataFile )
	{
		//  Look for the file path in the map.  If 
		//  it is not found, if searching for language 
		//  file try searching for US version;  otherwise,
		//  return NULL.
		void *	dummy;
		if ( m_pFileMap->Lookup( pFilename, dummy ) == FALSE )
			return (NULL);

		//  If here, file was found in the map.  Seek to
		//  that position in the file.
		//  If negative seek checks between CMmio's are 
		//  desired, they should be done here.
		long fileOffset = ( long )dummy;
		long	offsetFromHere = fileOffset - ( long )m_pDataFile->GetPosition();

#ifdef _DEBUG
		if ( m_bNegativeSeekCheck == TRUE && fileOffset < m_lastPos )
			WarnNegativeSeek( pFilename, m_lastPos, fileOffset );
#endif

		m_pDataFile->Seek( offsetFromHere, CFile::current );

#ifdef _DEBUG
		m_lastPos = m_pDataFile->GetPosition();
#endif

		//  Create a new CFile object.  It will be positioned 
		//  at the current file offset.  Note that this dtor will
		//  not automagically close the file handle, which is what we
		//  want.
		// DNT - fixed to provide duplicate handle - need to set offset
		CFile * pNewFile = new CFile ();
		if ( pNewFile == NULL )
			ThrowError( ERR_OUT_OF_MEMORY );
		if (pNewFile->Open( m_sFileName, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) == FALSE)
			ThrowError( ERR_OUT_OF_MEMORY );
		pNewFile->Seek (m_pDataFile->GetPosition (), CFile::begin);

		return pNewFile;
	}

	//  If here, file was not found in patch dir ( or
	//  no patch dir was given ), and no datafile was
	//  opened, so return NULL ( no file found ).
	return NULL;
}

CArchive *CDataFile::OpenAsCArchive( const char *pFilename )
{
	ASSERT( pFilename );

	//  Get the relative path of the file for which to search.
	CString filename( pFilename );
	CString path;
	path = "files\\" + filename;
	path.MakeLower ();

	//  Check to see if the file exists in the patch directory.
	if ( m_pPatchDir )
	{
		CString patchPath = *m_pPatchDir + "\\" + path;

		CFile	test;
		if ( test.Open( patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) != FALSE )
		{
			//  Close the file so we can allocate a new CFile object to
			//  return to the caller.
			test.Close();

			//  Allocate a new CFile object.  By creating and opening the CFile
			//  this way, the CFile dtor will automagically close the file 
			//  handle for us.
			CFile *pNewFile = new CFile;
			if ( pNewFile->Open( patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) == FALSE )
				ThrowError( ERR_PATCHFILE_OPEN );
			CArchive *pNewCArchive = new CArchive( pNewFile, CArchive::load );
			if ( pNewCArchive == NULL )
				ThrowError( ERR_OUT_OF_MEMORY );

			return pNewCArchive;
		}
	}

	//  If here, the file does not exist in the 
	//  patch directory or there is no patch directory.
	//  If a datafile was opened, look for it in the
	//  datafile.
	if ( m_pDataFile )
	{
		//  Look for the file path in the map.  If 
		//  it is not found, if searching for language 
		//  file try searching for US version;  otherwise,
		//  return NULL.
		void *	dummy;
		if ( m_pFileMap->Lookup( path, dummy ) == FALSE )
			ThrowError( ERR_DATAFILE_NO_ENTRY );

		//  If here, file was found in the map.  Seek to
		//  that position in the file.
		//  If negative seek checks between CMmio's are 
		//  desired, they should be done here.
		long fileOffset = ( long )dummy;
		long	offsetFromHere = fileOffset - ( long )m_pDataFile->GetPosition();

#ifdef _DEBUG
		if ( m_bNegativeSeekCheck == TRUE && fileOffset < m_lastPos )
			WarnNegativeSeek( pFilename, m_lastPos, fileOffset );
#endif

		m_pDataFile->Seek( offsetFromHere, CFile::current );

#ifdef _DEBUG
		m_lastPos = m_pDataFile->GetPosition();
#endif

		//  Create a new CFile object.  It will be positioned 
		//  at the current file offset.  Note that this dtor will
		//  not automagically close the file handle, which is what we
		//  want.
		CFile * pNewFile = new CFile ();
		if ( pNewFile == NULL )
			ThrowError( ERR_OUT_OF_MEMORY );
		if (pNewFile->Open( m_sFileName, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) == FALSE)
			ThrowError( ERR_OUT_OF_MEMORY );
		pNewFile->Seek (m_pDataFile->GetPosition (), CFile::begin);

		CArchive *pNewCArchive = new CArchive( pNewFile, CArchive::load );
		if ( pNewCArchive == NULL )
			ThrowError( ERR_OUT_OF_MEMORY );

		return pNewCArchive;
	}

	//  If here, file was not found in patch dir ( or
	//  no patch dir was given ), and no datafile was
	//  opened, so return NULL ( no file found ).
	ThrowError( ERR_DATAFILE_NO_ENTRY );
	return NULL;
}

void CDataFile::CloseCArchive( CArchive *pArchive )
{
	ASSERT( pArchive );

	//  Flush the archive and get the file.
	pArchive->Flush();
	CFile *pFile = pArchive->GetFile();

	//  Close the archive.
	pArchive->Close();
	delete pArchive;

	//  Close the file.
	pFile->Close();
	delete pFile;
}

#ifdef TEST_DATAFILE
CDataFile dataFile;

int main( int /*argc*/, char * /*argv*/[] )
{
	//  Create a CDataFile object.
	CString	str;
	CMmio *pFile;

	dataFile.Init( "test.dat", "." );

	pFile = dataFile.OpenAsMMIO( "t1" );
	pFile->DescendRiff( 'T','S','T','1' );
	pFile->DescendList( 'T','S','T','L' );
	pFile->DescendChunk( 'D','A','T','A' );
	pFile->ReadString( str );
	pFile->AscendChunk();
	pFile->DescendChunk( 'D','A','T','A' );
	pFile->ReadString( str );
	delete pFile;

	pFile = dataFile.OpenAsMMIO( "t2" );
	pFile->DescendRiff( 'T','S','T','2' );
	pFile->DescendChunk( 'D','A','T','A' );
	pFile->ReadString( str );
	delete pFile;

	pFile = dataFile.OpenAsMMIO( "units" );
	pFile->DescendRiff( 'U','N','I','T' );
	pFile->DescendList( 'T','E','R','N' );
	pFile->DescendChunk( 'N','M','B','R' );
	short x = ( short )pFile->ReadInt();
	delete pFile;

	CFile *pNormFile;
	pNormFile = dataFile.OpenAsFile( "bk.bat" );
	char	array[ 64 ];
	pNormFile->Read( array, 64 );
	delete pNormFile;

	CArchive *pArchive;
	pArchive = dataFile.OpenAsCArchive( "bk.bat" );
	dataFile.CloseCArchive( pArchive );

	dataFile.SetCountryCode( 1 );
	pFile = dataFile.OpenAsMMIO( NULL );
	delete pFile;

	dataFile.SetCountryCode( 2 );
	pFile = dataFile.OpenAsMMIO( NULL );
	delete pFile;

	dataFile.SetCountryCode( 12 );
	pFile = dataFile.OpenAsMMIO( NULL );
	delete pFile;

	dataFile.Close();

	dataFile.Init( NULL, "." );

	pFile = dataFile.OpenAsMMIO( "t1" );
	pFile->DescendRiff( 'T','S','T','1' );
	pFile->DescendList( 'T','S','T','L' );
	pFile->DescendChunk( 'D','A','T','A' );
	pFile->ReadString( str );
	pFile->AscendChunk();
	pFile->DescendChunk( 'D','A','T','A' );
	pFile->ReadString( str );
	delete pFile;
	
	pNormFile = dataFile.OpenAsFile( "bk2.bat" );
	pNormFile->Read( array, 64 );
	delete pNormFile;

	pArchive = dataFile.OpenAsCArchive( "bk2.bat" );
	dataFile.CloseCArchive( pArchive );

	dataFile.Close();

	return 0;
}

#endif


/////////////////////////////////////////////////////////////////////////////
// CDlgSelCD dialog


CDlgSelCD::CDlgSelCD(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSelCD::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgSelCD)
	m_strMsg = _T("");
	//}}AFX_DATA_INIT
}


void CDlgSelCD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgSelCD)
	DDX_Text(pDX, IDC_SEL_MSG, m_strMsg);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgSelCD, CDialog)
	//{{AFX_MSG_MAP(CDlgSelCD)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgSelCD message handlers

// we let them pick a location
void CDlgSelCD::OnBrowse() 
{
	
	CString sFilters;
	sFilters.LoadString (IDS_DATA_FILTERS);
	CFileDialog dlg (TRUE, "dat", m_sFileName, OFN_FILEMUSTEXIST, sFilters, NULL);	
	if (dlg.DoModal () != IDOK)
		{
		TRAP ();
		EndDialog (IDCANCEL);
		}

	m_sFileName = dlg.GetPathName ();
	EndDialog (IDOK);
}

void CDlgSelCD::OnOK() 
{
	
	UpdateData (TRUE);
	m_strMsg.LoadString ( IDS_CD_CHECKING );
	SetDlgItemText ( IDC_SEL_MSG, m_strMsg );
	UpdateWindow ();

	// this is my attempt to get the system to read the CD.
	CString sCdFile = CString ( m_sFileName [0] ) + ":\\";
	GetDriveType ( sCdFile );
	DWORD dw1, dw2, dw3, dw4;
	GetDiskFreeSpace ( sCdFile, &dw1, &dw2, &dw3, &dw4 );
	::Sleep ( 100 );

	// is it readable yet?
	if ( ! GetVolumeInformation ( sCdFile, NULL, 0, &dw1, &dw2, &dw3, NULL, 0 ) )
		{
		m_strMsg.LoadString ( IDS_CD_NOT_READY );
		UpdateData (FALSE);
		UpdateWindow ();
		return;
		}
  
	CFileStatus fs;
	if ( (CFile::GetStatus ( m_sFileName, fs ) == 0) || (fs.m_size < 1000) )
		{
		m_strMsg.LoadString ( IDS_CD_NO_FILE );
		UpdateData (FALSE);
		UpdateWindow ();
		return;
		}

	m_strMsg.LoadString ( IDS_CD_OK );
	UpdateData (FALSE);
	SetDlgItemText ( IDC_SEL_MSG, m_strMsg );
	UpdateWindow ();

	CDialog::OnOK();
}

#ifdef _DEBUG

void CDataFile::WarnNegativeSeek( const char *pFilename, long oldPos, long newPos )
{

	if (pFilename == NULL)
		pFilename = "NULL";

	char	str[ 256 ];
	wsprintf( str, "WARNING! - Negative seek from %ld to %ld while loading %s\n", oldPos, newPos, pFilename );
	OutputDebugString( str );
}

void CDataFile::EnableNegativeSeekChecking()
{
	m_bNegativeSeekCheck = TRUE;
	m_lastPos = 0;
}

void CDataFile::DisableNegativeSeekChecking()
{
	m_bNegativeSeekCheck = FALSE;
}

#endif


