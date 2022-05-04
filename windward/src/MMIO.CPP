//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "_windwrd.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

void CMmio::ctor ()
{
	m_hMmio = NULL;
	m_iListDepth = 0;
}

CMmio::CMmio( const char *pFilename )
{
	ctor ();
	Open (pFilename);
}

CMmio::~CMmio ()
{

	if (m_hMmio != NULL)
		Close ();
}

void CMmio::Open (const char *pFile)
{
	ASSERT_VALID (this);

	if (m_hMmio != NULL)
		Close ();

	m_sFileName = pFile;

	m_hMmio = mmioOpen ((char *) pFile, NULL, MMIO_READ |	MMIO_DENYWRITE | MMIO_ALLOCBUF);

	// throw exception if it doesn't exist
	if (m_hMmio == NULL)
		ThrowError (ERR_MMIO_OPEN);
}

void CMmio::Close ()
{

	ASSERT (m_hMmio != NULL);
	ASSERT_VALID (this);

	mmioClose (m_hMmio, 0);
	ctor ();
}

LONG CMmio::GetOffset () const
{

	MMIOINFO mmi;
	memset (&mmi, 0, sizeof (mmi));
	mmioGetInfo (m_hMmio, &mmi, 0);
	return ( mmi.lDiskOffset + (mmi.pchNext - mmi.pchBuffer) - mmi.cchBuffer );
}

DWORD CMmio::DescendRiff (FOURCC chunk)
{

	ASSERT (m_hMmio != NULL);
	ASSERT_VALID (this);

	memset (&m_mckiRiff, 0, sizeof (m_mckiRiff));
	m_mckiRiff.ckid =	mmioFOURCC ('R', 'I', 'F', 'F');
	m_mckiRiff.fccType = chunk;
	if (mmioDescend (m_hMmio, &m_mckiRiff, NULL,	MMIO_FINDRIFF) != 0)
		{
		ThrowError (ERR_MMIO_DESCEND_RIFF);
		}

	return (m_mckiRiff.cksize);
}

DWORD CMmio::DescendList (FOURCC chunk)
{

	ASSERT (m_hMmio != NULL);
	ASSERT_VALID (this);

	if (m_iListDepth >= MMIO_NUM_LIST)
		{
		ThrowError (ERR_MMIO_DESCEND_LIST);
		}

	MMCKINFO * pMckiPar;
	if (m_iListDepth == 0)
		pMckiPar = &m_mckiRiff;
	else
		pMckiPar = &m_mckiList[m_iListDepth-1];

	memset (&m_mckiList[m_iListDepth], 0, sizeof (MMCKINFO));
	m_mckiList[m_iListDepth].ckid =	mmioFOURCC ('L', 'I', 'S', 'T');
	m_mckiList[m_iListDepth].fccType = chunk;
	if (mmioDescend (m_hMmio, &m_mckiList[m_iListDepth], pMckiPar, MMIO_FINDLIST) != 0)
		{
		ThrowError (ERR_MMIO_DESCEND_LIST);
		}

	m_iListDepth++;
	return (m_mckiList[m_iListDepth-1].cksize);
}

DWORD CMmio::DescendChunk (FOURCC chunk)
{

	ASSERT (m_hMmio != NULL);
	ASSERT_VALID (this);

	memset (&m_mckiChunk, 0, sizeof (MMCKINFO));
	m_mckiChunk.ckid =	chunk;
	if (mmioDescend (m_hMmio, &m_mckiChunk, &m_mckiList[m_iListDepth-1], MMIO_FINDCHUNK) != 0)
		{
		ThrowError (ERR_MMIO_DESCEND_CHUNK);
		}

	return (m_mckiChunk.cksize);
}

void CMmio::AscendList ()
{

	ASSERT (m_hMmio != NULL);
	ASSERT_VALID (this);
	ASSERT (m_iListDepth > 0);

	if (mmioAscend (m_hMmio, &m_mckiList[m_iListDepth-1], 0) != 0)
		{
		ThrowError (ERR_MMIO_ASCEND_LIST);
		}

	m_iListDepth--;
}

void CMmio::AscendChunk ()
{

	ASSERT (m_hMmio != NULL);
	ASSERT_VALID (this);

	if (mmioAscend (m_hMmio, &m_mckiChunk, 0) != 0)
		{
		ThrowError (ERR_MMIO_ASCEND_CHUNK);
		}
}

CMmioEmbeddedFile::CMmioEmbeddedFile()
{
	ctor();
}

CMmioEmbeddedFile::CMmioEmbeddedFile( UINT hFile, const char * pFileName )
{
	ctor();
	Open( hFile, pFileName );
}

CMmioEmbeddedFile::~CMmioEmbeddedFile()
{
	if ( m_hMmio != NULL )
		Close();
}

void CMmioEmbeddedFile::Open( UINT hFile, const char * pFileName )
{
	ASSERT_VALID( this );

	if( m_hMmio != NULL )
		Close();

	MMIOINFO	info;
	( void )memset( &info, 0, sizeof( info ) );
	info.fccIOProc = FOURCC_DOS;
	info.pchBuffer = NULL;
	info.adwInfo[ 0 ] = hFile;

	m_hMmio = mmioOpen( NULL, &info, MMIO_READ | MMIO_DENYWRITE | MMIO_ALLOCBUF );

	m_sFileName = pFileName;

	//  ThrowError exception if the open was 
	//  unsuccessful.
	if ( m_hMmio == NULL )
		ThrowError( ERR_MMIO_OPEN );
}

void CMmioEmbeddedFile::Close()
{
	ASSERT( m_hMmio != NULL );
	ASSERT_VALID( this );

	mmioClose( m_hMmio, MMIO_FHOPEN );
	ctor();
}
