#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include "assert.h"
#include "debug.h"
#include "acmutil.h"

inline void raiseError(MMRESULT mmr, LPCSTR text)
{
	throw CACMException(mmr, text);
}


CACMWaveFormat::CACMWaveFormat(WORD fmt, DWORD sz)
{
	MMRESULT mmr;

	m_fmt = NULL;
	m_size = 0;

	if (0 == sz)
	{
		CACMStruct<ACMFORMATTAGDETAILS>  tagD;

		tagD->dwFormatTag = fmt;
		mmr = acmFormatTagDetails(NULL, 
							tagD,
						ACM_FORMATTAGDETAILSF_LARGESTSIZE
						);

		if (0 != mmr)
			raiseError(mmr, 
			"Error getting format descriptor size");
		
		sz = tagD->cbFormatSize;

		
	}
	m_fmt = (LPWAVEFORMATEX) 
		   GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, sz);
			   
	if (m_fmt)
	{
		m_fmt->wFormatTag = fmt;
		m_size = sz;
	}
	else
	{
		raiseError(MMSYSERR_NOMEM, 
			"No memory for wave format descriptor");
	}
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

	fmtD->dwFormatTag = m_fmt->wFormatTag;
	fmtD->pwfx = m_fmt;
	fmtD->cbwfx = m_size;
	DWORD done = 0;

	MMRESULT mmr = acmFormatEnum(NULL,
			fmtD,
			fmtEnumCallback,
			(DWORD) &done,
			flags);

	if (mmr)
		raiseError(mmr, "Format enumeration error");

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
		m_mmr = acmStreamClose(m_strm, 0);

	if (m_mmr)
		raiseError(m_mmr, "Error cloasing ACM stream"); 
	m_strm = NULL;
}

void CACMStream::Open()
{
	m_mmr = acmStreamOpen(&m_strm, 
						  NULL, 
						  m_srcFmt,
						  m_dstFmt,
						  NULL,
						  0,
						  0,
						  ACM_STREAMOPENF_NONREALTIME);
	
	if (m_mmr)
		raiseError(m_mmr, "Error opening ACM stream");

}


void  CACMStream::Convert(LPVOID src, DWORD size)
{

	m_mmr = 0;

	if (m_hdr->fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED)
	{
		if (src != m_hdr->pbSrc || size > m_hdr->dwSrcUser)
		{
			Unprepare();
			Prepare(src, size);
		}
	}
	else
	{
		Prepare(src, size);
	}


	m_mmr = acmStreamConvert(m_strm, 
							 m_hdr, 
							 ACM_STREAMCONVERTF_END|
							 ACM_STREAMCONVERTF_START);

	if (m_mmr)
		raiseError(m_mmr, "Error during conversion");

}


void CACMStream::Prepare(LPVOID src, DWORD size)
{

 	m_hdr->pbSrc = (LPBYTE) src;
	m_hdr->cbSrcLength = m_hdr->dwSrcUser = size;

	// get the suggested output buffer size
	DWORD dstSize;
	m_mmr = acmStreamSize(m_strm, 
		size, 
		&dstSize, 
		ACM_STREAMSIZEF_SOURCE);

	
	if (m_mmr)
		raiseError(m_mmr, 
		"Error evaluating destination buffer size");



 	m_hdr->pbDst = (LPBYTE) GlobalAlloc(GMEM_FIXED, dstSize);
	if (!m_hdr->pbDst)
	{
		m_mmr = MMSYSERR_NOMEM;
		raiseError(m_mmr, "Error allocating destination buffer");
	}

	m_hdr->cbDstLength = m_hdr->dwDstUser = dstSize;


	m_mmr = acmStreamPrepareHeader(m_strm, m_hdr, 0);

	if (m_mmr)
		raiseError(m_mmr, 
		"Error prparing stream header");
}

void CACMStream::Unprepare()
{
	m_mmr = 0;
	if (m_hdr->fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED)
	{
		m_hdr->cbSrcLength = m_hdr->dwSrcUser;
		m_mmr = acmStreamUnprepareHeader(m_strm, m_hdr, 0);
		GlobalFree(m_hdr->pbDst);
	}

	if (m_mmr)
		raiseError(m_mmr, 
			"Error unpreparing stream header");
}


CADPCMtoPCMConvert::CADPCMtoPCMConvert()
: m_pcmFmt(WAVE_FORMAT_PCM), m_adpcmFmt(WAVE_FORMAT_ADPCM)
{
	const DWORD flags = ACM_FORMATENUMF_WFORMATTAG | 
			ACM_FORMATENUMF_WBITSPERSAMPLE |
			ACM_FORMATENUMF_NCHANNELS |
			ACM_FORMATENUMF_NSAMPLESPERSEC; 

	m_stream = NULL;

	m_adpcmFmt->wBitsPerSample = 4;
	m_adpcmFmt->nChannels = 2;
	m_adpcmFmt->nSamplesPerSec = 44100;

	m_pcmFmt->wBitsPerSample = 16;
	m_pcmFmt->nChannels = 2;
	m_pcmFmt->nSamplesPerSec = 44100;

	if (!m_adpcmFmt.Prepare(flags))
		raiseError(0, 
		"ADPCM format is not supported on this machine");

	if (!m_pcmFmt.Prepare(flags))
		raiseError(0, 
		"PCM format is not supported on this machine");

	m_stream = new CACMStream(m_pcmFmt, m_adpcmFmt);
}

CADPCMtoPCMConvert::~CADPCMtoPCMConvert()
{
	delete m_stream;
}

