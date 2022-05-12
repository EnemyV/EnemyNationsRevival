#include "stdafx.h"
#include <thielen.h>
#include "_windwrd.h"


#include "assert.h"
#include "acmutil.h"

static DWORD _dwMaxSize = DBL_BUF_LEN;

extern "C"
{
typedef MMRESULT ( ACMAPI * PROC_acmStreamOpen) (LPHACMSTREAM phas, HACMDRIVER had, 
    LPWAVEFORMATEX pwfxSrc, LPWAVEFORMATEX pwfxDst, LPWAVEFILTER pwfltr,
    DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);
typedef MMRESULT ( ACMAPI * PROC_acmFormatEnumA) (
    HACMDRIVER              had,
    LPACMFORMATDETAILSA     pafd,
    ACMFORMATENUMCBA        fnCallback,
    DWORD                   dwInstance, 
    DWORD                   fdwEnum );
typedef MMRESULT ( ACMAPI * PROC_acmFormatTagDetailsA) (
    HACMDRIVER              had,
    LPACMFORMATTAGDETAILSA  paftd,
    DWORD                   fdwDetails );
typedef MMRESULT ( ACMAPI * PROC_acmMetrics) (
    HACMOBJ                 had,
    UINT										uMetric,
    LPVOID									pData );
typedef MMRESULT ( ACMAPI * PROC_acmStreamClose) (
    HACMSTREAM              has,
    DWORD                   fdwClose );
typedef MMRESULT ( ACMAPI * PROC_acmStreamConvert) (
    HACMSTREAM              has, 
    LPACMSTREAMHEADER       pash,
    DWORD                   fdwConvert );
typedef MMRESULT ( ACMAPI * PROC_acmStreamPrepareHeader) (
    HACMSTREAM          has,
    LPACMSTREAMHEADER   pash,
    DWORD               fdwPrepare );
typedef MMRESULT ( ACMAPI * PROC_acmStreamSize) (
    HACMSTREAM              has,
    DWORD                   cbInput,
    LPDWORD                 pdwOutputBytes,
    DWORD                   fdwSize );
typedef MMRESULT ( ACMAPI * PROC_acmStreamUnprepareHeader) (
    HACMSTREAM          has,
    LPACMSTREAMHEADER   pash,
    DWORD               fdwUnprepare );

static PROC_acmStreamOpen fnacmStreamOpen = NULL;
static PROC_acmFormatEnumA fnacmFormatEnum = NULL;
static PROC_acmFormatTagDetailsA fnacmFormatTagDetails = NULL;
static PROC_acmMetrics fnacmMetrics = NULL;
static PROC_acmStreamClose fnacmStreamClose = NULL;
static PROC_acmStreamConvert fnacmStreamConvert = NULL;
static PROC_acmStreamPrepareHeader fnacmStreamPrepareHeader = NULL;
static PROC_acmStreamSize fnacmStreamSize = NULL;
static PROC_acmStreamUnprepareHeader fnacmStreamUnprepareHeader = NULL;
};

class xAcm
{
public:
	xAcm ();
	~xAcm ();

	BOOL IsLoaded () 	{ return ( m_hLib != NULL ); }
	void IncUse () { iNumUsers ++; }
	void DecUse ();

	HINSTANCE m_hLib;
	int iNumUsers;
};

static xAcm _acm;

xAcm::xAcm ()
{

	iNumUsers = 0;
	if ( m_hLib != NULL )
		return;
		
	UINT uOld = ::SetErrorMode ( SEM_NOOPENFILEERRORBOX );
	if ( ( m_hLib = LoadLibrary ("msacm32.dll") ) == NULL )
		{
		SetErrorMode ( uOld );
		return;
		}
	::SetErrorMode ( uOld );
		
	if ( (fnacmStreamOpen = (PROC_acmStreamOpen) GetProcAddress (m_hLib, "acmStreamOpen")) == NULL)
		{
		FreeLibrary (m_hLib);
		m_hLib = NULL;
		}
	if ( (fnacmFormatEnum = (PROC_acmFormatEnumA) GetProcAddress (m_hLib, "acmFormatEnumA")) == NULL)
		{
		FreeLibrary (m_hLib);
		m_hLib = NULL;
		}
	if ( (fnacmFormatTagDetails = (PROC_acmFormatTagDetailsA) GetProcAddress (m_hLib, "acmFormatTagDetailsA")) == NULL)
		{
		FreeLibrary (m_hLib);
		m_hLib = NULL;
		}
	if ( (fnacmMetrics = (PROC_acmMetrics) GetProcAddress (m_hLib, "acmMetrics")) == NULL)
		{
		FreeLibrary (m_hLib);
		m_hLib = NULL;
		}
	if ( (fnacmStreamClose = (PROC_acmStreamClose) GetProcAddress (m_hLib, "acmStreamClose")) == NULL)
		{
		FreeLibrary (m_hLib);
		m_hLib = NULL;
		}
	if ( (fnacmStreamConvert = (PROC_acmStreamConvert) GetProcAddress (m_hLib, "acmStreamConvert")) == NULL)
		{
		FreeLibrary (m_hLib);
		m_hLib = NULL;
		}
	if ( (fnacmStreamPrepareHeader = (PROC_acmStreamPrepareHeader) GetProcAddress (m_hLib, "acmStreamPrepareHeader")) == NULL)
		{
		FreeLibrary (m_hLib);
		m_hLib = NULL;
		}
	if ( (fnacmStreamSize = (PROC_acmStreamSize) GetProcAddress (m_hLib, "acmStreamSize")) == NULL)
		{
		FreeLibrary (m_hLib);
		m_hLib = NULL;
		}
	if ( (fnacmStreamUnprepareHeader = (PROC_acmStreamUnprepareHeader) GetProcAddress (m_hLib, "acmStreamUnprepareHeader")) == NULL)
		{
		FreeLibrary (m_hLib);
		m_hLib = NULL;
		}
}

xAcm::~xAcm ()
{

	if ( m_hLib == NULL )
		return;

	if ( iNumUsers > 0 )
		iNumUsers--;
	else
		FreeLibrary (m_hLib);
}

void xAcm::DecUse ()
{

	iNumUsers --;
	// can only happen if dtor called
	if ( iNumUsers < 0 )
		FreeLibrary (m_hLib);
}

CACMWaveFormat::CACMWaveFormat(WORD fmt, DWORD sz)
{

	if ( ! _acm.IsLoaded () )
		{
		TRAP ();
		ThrowError ( ERR_ADPCM );
		}

	m_fmt = NULL;
	m_size = 0;

	if (0 == sz)
		fnacmMetrics ( NULL, ACM_METRIC_MAX_SIZE_FORMAT, &sz );
	
	m_fmt = (LPWAVEFORMATEX) GlobalAlloc (GMEM_FIXED|GMEM_ZEROINIT, sz);
			   
	if (m_fmt)
		{
		m_fmt->wFormatTag = fmt;
		m_size = sz;
		}
	else
		ThrowError ( ERR_ADPCM );
}


CACMWaveFormat::~CACMWaveFormat()
{

	if (m_fmt)
		GlobalFree(m_fmt);
}


BOOL PASCAL CACMWaveFormat::fmtEnumCallback(HACMDRIVERID hadid,
    LPACMFORMATDETAILS pafd, DWORD dwInstance, DWORD fdwSupport)
{
	LPDWORD pDw = (LPDWORD) dwInstance;

	*pDw = TRUE;
	return FALSE;
}

BOOL CACMWaveFormat::Prepare(DWORD flags)
{

	CACMStruct<ACMFORMATDETAILS> fmtD;

	// this should be a 1 or 2
	fmtD->dwFormatTag = m_fmt->wFormatTag;
	fmtD->pwfx = m_fmt;
	fmtD->cbwfx = m_size;
	DWORD done = 0;

	MMRESULT mmr = fnacmFormatEnum(NULL,
			fmtD,
			fmtEnumCallback,
			(DWORD) &done,
			flags);

	// BUBUG getting mmr=Invalid paramater, on WAVE_FORMAT_PCM
	// which paramater is the invalid one, or more than one?
	// 
	if (mmr)
		ThrowError ( ERR_ADPCM );

	return done;
}



CACMStream::CACMStream(LPWAVEFORMATEX src, LPWAVEFORMATEX dst)
{

	m_srcFmt = src;
	m_dstFmt = dst;
	m_strm = NULL;
	m_mmr = 0;
}

CACMStream::~CACMStream()
{

	Unprepare();
	Close();
}

void CACMStream::Close()
{

	m_mmr = 0;
	if (m_strm)
		m_mmr = fnacmStreamClose(m_strm, 0);

	m_strm = NULL;
}

void CACMStream::Open()
{

	m_mmr = fnacmStreamOpen(&m_strm, 
						  NULL, 
						  m_srcFmt,
						  m_dstFmt,
						  NULL,
						  0,
						  0,
						  ACM_STREAMOPENF_NONREALTIME);
	
	if (m_mmr)
		ThrowError ( ERR_ADPCM );
}

void  CACMStream::Convert (LPVOID pSrcBuf, DWORD dwSrcSize, LPVOID pDstBuf, DWORD dwDstSize, DWORD dwFlags )
{

	m_mmr = 0;

	if (m_hdr->fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED)
		{
		if ( (pSrcBuf != m_hdr->pbSrc) || (dwSrcSize != m_hdr->dwSrcUser) ||
					(pDstBuf != m_hdr->pbDst) || (dwDstSize != m_hdr->dwDstUser) )
			{
			Unprepare();
			Prepare ( pSrcBuf, dwSrcSize, pDstBuf, dwDstSize );
			}
		}
	else
		Prepare (pSrcBuf, dwSrcSize, pDstBuf, dwDstSize);

	m_mmr = fnacmStreamConvert (m_strm, m_hdr, dwFlags );

	TRAP ( m_mmr );
	if (m_mmr)
		ThrowError ( ERR_ADPCM );
}

void CACMStream::Prepare (LPVOID pSrcBuf, DWORD dwSrcSize, LPVOID pDstBuf, DWORD dwDstSize)
{

 	m_hdr->pbSrc = (LPBYTE) pSrcBuf;

	// get the suggested output buffer size
	if ( dwDstSize == 0 )
		{
		m_mmr = fnacmStreamSize (m_strm, dwSrcSize, &dwDstSize, ACM_STREAMSIZEF_SOURCE);
	
		if (m_mmr)
			ThrowError ( ERR_ADPCM );
		}
	else
		{
		TRAP (dwSrcSize == 0);
		DWORD dwRtn;
		m_mmr = fnacmStreamSize (m_strm, __min (_dwMaxSize, dwDstSize), &dwRtn, 
																								ACM_STREAMSIZEF_DESTINATION);
		TRAP (dwRtn == 0);
		dwSrcSize = __min ( dwRtn, dwSrcSize );
		}
	if ( (m_hdr->cbSrcLength = m_hdr->dwSrcUser = dwSrcSize) == 0 )
		{
		TRAP ();
		m_hdr->cbSrcLengthUsed = m_hdr->cbDstLengthUsed = 0;
		return;
		}

	if ( pDstBuf == NULL )
		{
	 	m_hdr->pbDst = (LPBYTE) MEM_alloc_lock ( dwDstSize + 16 );
		if (!m_hdr->pbDst)
			{
			m_mmr = MMSYSERR_NOMEM;
			ThrowError ( ERR_ADPCM );
			}
		}
	else
	 	m_hdr->pbDst = (LPBYTE) pDstBuf;

	m_hdr->cbDstLength = m_hdr->dwDstUser = dwDstSize;

	m_mmr = fnacmStreamPrepareHeader(m_strm, m_hdr, 0);

	if (m_mmr)
		ThrowError ( ERR_ADPCM );
}

void CACMStream::ReleaseBuffer ()
{

	m_mmr = 0;
	if (m_hdr->fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED)
		{
		m_hdr->cbSrcLength = m_hdr->dwSrcUser;
		m_mmr = fnacmStreamUnprepareHeader (m_strm, m_hdr, 0);
		m_hdr->pbDst = NULL;
		}
}

void CACMStream::Unprepare()
{

	if ( m_hdr->pbDst == NULL )
		return;

	m_mmr = 0;
	if (m_hdr->fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED)
		{
		m_hdr->cbSrcLength = m_hdr->dwSrcUser;
		m_mmr = fnacmStreamUnprepareHeader (m_strm, m_hdr, 0);
		}

	if (m_mmr)
		ThrowError ( ERR_ADPCM );
}


CADPCMtoPCMConvert::CADPCMtoPCMConvert ( int iChannels, int iBits, int iRate )
: m_pcmFmt(WAVE_FORMAT_PCM), m_adpcmFmt(WAVE_FORMAT_ADPCM)
{
	const DWORD flags = ACM_FORMATENUMF_WFORMATTAG | 
			ACM_FORMATENUMF_WBITSPERSAMPLE |
			ACM_FORMATENUMF_NCHANNELS |
			ACM_FORMATENUMF_NSAMPLESPERSEC; 

	m_stream = NULL;

	// can we link?
	if ( ! _acm.IsLoaded () )
		{
		TRAP ();
		ThrowError ( ERR_ADPCM );
		}
	_acm.IncUse ();

	// if not multi-threaded then 2 sector at a time
	if ( ! ptheApp->GetProfileInt ( "Advanced", "MusicThread", 1 ) )
		_dwMaxSize = __min ( 4 * 2 * 2048, DBL_BUF_LEN / 4 );
	else
		_dwMaxSize = DBL_BUF_LEN / 4;

	m_adpcmFmt->wBitsPerSample = 4;
	m_adpcmFmt->nChannels = iChannels;
	m_adpcmFmt->nSamplesPerSec = iRate;

	m_pcmFmt->nChannels = iChannels;
	m_pcmFmt->wBitsPerSample = iBits;
	m_pcmFmt->nSamplesPerSec = iRate;
  m_pcmFmt->nBlockAlign =  m_pcmFmt->nChannels * m_pcmFmt->wBitsPerSample / 8;
  m_pcmFmt->nAvgBytesPerSec =  m_pcmFmt->nSamplesPerSec * m_pcmFmt->nBlockAlign;

	if ( ! m_pcmFmt.Prepare (flags) )
		ThrowError ( ERR_ADPCM );

	if ( ! m_adpcmFmt.Prepare (flags) )
		ThrowError ( ERR_ADPCM );

	m_stream = new CACMStream (m_adpcmFmt, m_pcmFmt);
	m_stream->Open ();
}


CADPCMtoPCMConvert::~CADPCMtoPCMConvert()
{
	delete m_stream;

	_acm.DecUse ();
}

