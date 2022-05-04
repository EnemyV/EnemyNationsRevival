//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"

#include <thielen.h>
#include "_windwrd.h"
#include "_res.h"
#include "acmutil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

CMusicPlayer theMusicPlayer;


/////////////////////////////////////////////////////////////////////////////
// CRawChannel

CRawChannel::CRawChannel ()
{

	m_bKill = FALSE;
	m_hSmp = NULL;
	m_iIsFore = back;
	m_iBackSound = 0;
	m_iBackPri = INT_MAX;
	m_iNum = 0;
	m_iVol = 0;
	m_iPan = 64;
	m_pData = NULL;
	m_pDblBuf[0] = m_pDblBuf[1] = NULL;
}

void CRawChannel::Close ()
{

	FreeDblBuf ();
}

BOOL CRawChannel::AllocDblBuf ()
{

	// get our buffer
	for (int iInd=0; iInd<2; iInd++)
	if (m_pDblBuf[iInd] == NULL)
		if ((m_pDblBuf[iInd] = MEM_alloc_lock (DBL_BUF_LEN)) == NULL)
			{
			TRAP ();
			FreeDblBuf ();
			return (FALSE);
			}

	return (TRUE);
}

void CRawChannel::FreeDblBuf ()
{

	for (int iInd=0; iInd<2; iInd++)
		if (m_pDblBuf[iInd] != NULL)
			{
			MEM_free_lock (m_pDblBuf[iInd]);
			m_pDblBuf[iInd] = NULL;
			}
}


/////////////////////////////////////////////////////////////////////////////
// CRawData

CRawData::CRawData ()
{

	m_iGroup = 0;
	m_iType = cache;
	m_iComp = -1;
	m_IDBackup = 0;
	m_pBuf = NULL;
	m_lBufOff = 0;
	m_iFmt = DIG_F_MONO_8;
	m_bVoice = FALSE;
	m_bKillMe = FALSE;
	m_iStat = unloaded;
	m_pRc = NULL;
}

void CRawData::Close ()
{

	if (m_pBuf != NULL)
		{
		MEM_free_lock (m_pBuf);
		m_pBuf = NULL;
		}
}

void ConstructElements (CRawData * pNew, int nCount)
{

	for (int i=0; i<nCount; i++, pNew++)
		pNew->CRawData::CRawData ();
}

void DestructElements (CRawData * pNew, int nCount)
{

	for (int i=0; i<nCount; i++, pNew++)
		pNew->CRawData::~CRawData ();
}

// reads in the data (and buffer if preload) of a wav file
void CRawData::InitData (CMmio * pMmio, int iGrp)
{

	pMmio->DescendChunk ('D', 'A', 'T', 'A');

	m_iGroup = pMmio->ReadShort ();
	m_iType = (CRawData::TYPE) pMmio->ReadShort ();
	m_iComp = pMmio->ReadShort ();
	m_IDBackup = pMmio->ReadShort ();
	short sTyp = pMmio->ReadShort ();
	switch (sTyp)
	  {
		case mono_22_16 :
			m_iFmt = DIG_F_MONO_16;
			break;
		case stereo_22_16 :
			m_iFmt = DIG_F_STEREO_16;
			break;
		default :
			m_iFmt = DIG_F_MONO_8;
			break;
	  }
	m_iDataLen = m_iFileLen = pMmio->ReadLong ();
	m_lOff = pMmio->GetOffset ();

	if ((m_iType == preload) && (m_iGroup == iGrp))
		{
		if (m_iComp == -1)
			{
			if ((m_pBuf = MEM_alloc_lock (m_iDataLen)) != NULL)
				pMmio->Read (m_pBuf, m_iDataLen);
			}
		else if( m_iComp == 8 )
			{
			TRAP ();
			void * pBuf = malloc (m_iFileLen);
			if (pBuf != NULL)
				{
				pMmio->Read (pBuf, m_iFileLen);

				TRAP ();
				m_iDataLen = CoDec::BufLength ( pBuf );
				
				if ((m_pBuf = MEM_alloc_lock (m_iDataLen)) == NULL)
					ThrowError (ERR_OUT_OF_MEMORY);
				CoDec::Decompress ( pBuf, m_iFileLen, m_pBuf, m_iDataLen );

				free (pBuf);
				}
			}
		else if( m_iComp == 9 )
			{
				void * pBuf = malloc (m_iFileLen);
				if( pBuf != NULL )
				{
					pMmio->Read (pBuf, m_iFileLen);

					// instantiate an object to do the decompression
					CADPCMtoPCMConvert adpcmCoDec;

					// now do the actual decompression, with result in m_pBuf
					adpcmCoDec.Convert(pBuf, m_iFileLen);
					m_pBuf = adpcmCoDec.ResultData();
					m_iDataLen = adpcmCoDec.ResultSize();

					free (pBuf);
				}
			}
		}

	pMmio->AscendChunk ();
}

void CRawData::LoadBuffer (CFile * pFile)
{

	// if loaded we're done
	if ( m_iStat == loaded )
		return;

	if (m_iComp == -1)
		{
		// alloc the memory if necessary
		if (m_pBuf == NULL)
			if ((m_pBuf = MEM_alloc_lock (m_iDataLen)) == NULL)
				ThrowError (ERR_OUT_OF_MEMORY);

		pFile->Seek (m_lOff, CFile::begin);
		pFile->Read (m_pBuf, m_iDataLen);
		m_iStat = loaded;
		return;
		}

	if( m_iComp == 8 )
	{
	// need to decompress
	void * pBuf = malloc (m_iFileLen);
	if (!pBuf)
		return;

	pFile->Seek (m_lOff, CFile::begin);
	pFile->Read (pBuf, m_iFileLen);

	m_iDataLen = CoDec::BufLength ( pBuf );
	if ((m_pBuf = MEM_alloc_lock (m_iDataLen)) == NULL)
		ThrowError (ERR_OUT_OF_MEMORY);
	CoDec::Decompress ( pBuf, m_iFileLen, m_pBuf, m_iDataLen );

	free (pBuf);

	m_iStat = loaded;
	}
	else if( m_iComp == 9 )
	{
		// need to decompress
		void * pBuf = malloc (m_iFileLen);
		if (!pBuf)
			return;

		// instantiate an object to do the decompression
		CADPCMtoPCMConvert adpcmCoDec;

		// skip the riff header?
		pFile->Seek (m_lOff, CFile::begin);
		pFile->Read (pBuf, m_iFileLen);

		// instantiate an object to do the decompression
		CADPCMtoPCMConvert adpcmCoDec;

		// now do the actual decompression, with result in m_pBuf
		adpcmCoDec.Convert(pBuf, m_iFileLen);
		m_pBuf = adpcmCoDec.ResultData();
		m_iDataLen = adpcmCoDec.ResultSize();

		free (pBuf);

		m_iStat = loaded;
	}
}

void CRawData::UnloadBuffer ()
{

	// make sure not in use
	if (m_pRc != NULL)
		if (m_pRc->m_pData == this)
			{
			TRAP ();
			return;
			}

	m_iStat = unloaded;
	if (m_pBuf != NULL)
		{
		MEM_free_lock (m_pBuf);
		m_pBuf = NULL;
		}
}

void CRawData::StartDblBuffer (CRawChannel * pRaw)
{

	int iLen;
	if (DBL_BUF_LEN <= m_iDataLen)
		iLen = DBL_BUF_LEN;
	else
		{
		TRAP ();
		iLen = m_iDataLen;
		}

	m_lBufOff = 0;

	theDiskCache.SyncRequest ( m_bVoice ? pRaw->m_pPar->m_pFileVoc->m_hFile : pRaw->m_pPar->m_pFileReg->m_hFile,
																	m_lOff, iLen, pRaw->m_pDblBuf[0]);

	// hand it off
	AIL_load_sample_buffer (pRaw->m_hSmp, 0, (char *) pRaw->m_pDblBuf[0], iLen);
}

void CRawData::LoadNextDblBuffer (CRawChannel * pRaw, int iNeed)
{

	int iLen;
	if (m_lBufOff+DBL_BUF_LEN <= m_iDataLen)
		iLen = DBL_BUF_LEN;
	else
		{
		iLen = m_iDataLen - m_lBufOff;
		if (iLen == 0)
			{
			AIL_end_sample (pRaw->m_hSmp);
			return;
			}
		}

	theDiskCache.SyncRequest ( m_bVoice ? pRaw->m_pPar->m_pFileVoc->m_hFile : pRaw->m_pPar->m_pFileReg->m_hFile,
						m_lBufOff + m_lOff, iLen, pRaw->m_pDblBuf[iNeed]);
						
	// hand it off
	AIL_load_sample_buffer (pRaw->m_hSmp, iNeed, (char *) pRaw->m_pDblBuf[iNeed], iLen);

	m_lBufOff += iLen;
}


/////////////////////////////////////////////////////////////////////////////
// CMusicPlayer

CMusicPlayer::CMusicPlayer ()
{

	m_bNoMidi = m_bNoWav = m_bRunning = m_bExclWav = m_bKillMidi = m_bUseDS = FALSE;
	m_iMode = mixed;

	m_hDig = NULL;
	m_hMdi = NULL;
	m_hSeq = NULL;

	m_pFileReg = NULL;
	m_pFileVoc = NULL;

	for (int iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++)
		m_Channel [iOn].m_pPar = this;
}

CMusicPlayer::~CMusicPlayer ()
{

	Close ();
}

void CMusicPlayer::Open (int iMusicVol, int iSoundVol, MUSIC_MODE iMode, int iGrp)
{

	ASSERT (m_bRunning == FALSE);
	ASSERT ((0 <= iMusicVol) && (iMusicVol <= 100));
	ASSERT ((0 <= iSoundVol) && (iSoundVol <= 100));

	m_iMode = iMode;

	m_iMusicVol = (iMusicVol * 127) / 100;
	m_iSfxVol = (iSoundVol * 127) / 100;

	ptheApp->WriteProfileInt( "Advanced", "MusicModeUsed", iMode );

	AIL_DLL_version ( m_sVer.GetBuffer ( 256 ), 254 );
	m_sVer.ReleaseBuffer ( -1 );

	// if this fails we just don't have sound
	try
		{
		// if nothing playing - don't open
		if ((m_iSfxVol == 0) && (m_iMusicVol == 0))
			return;

		// start us up
		AIL_startup ();
		m_bRunning = TRUE;

		m_bExclWav = iMode != midi_only;

		// open the MIDI device
		if ((iMode == midi_only) && (m_iMusicVol > 0))
			OpenMidi (m_iMusicVol);

		// open the wave device
		int iVol;
		if (iMode == midi_only)
			iVol = m_iSfxVol;
		else
			iVol = __max (m_iMusicVol, m_iSfxVol);
		if (iVol > 0)
			OpenDigital (iVol, ! m_bExclWav);

		AIL_serve ();
		}

	// close it down
	catch (...)
		{
		m_bNoMidi = m_bNoWav = TRUE;
		try
			{
			CDlgMsg dlg;
			dlg.MsgBox (IDS_ERROR_AUDIO, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NoSoundCard" );
			Close ();
			}
		catch (...)
			{
			}
		m_bRunning = FALSE;
		}
}

void CMusicPlayer::InitData (MUSIC_MODE iMode, int iGrp)
{

	#ifdef _GG
		return;
	#endif

	m_iMode = iMode;
	BOOL bMidi = iMode != wav_only;

	CMmio *pMmio = theDataFile.OpenAsMMIO ("music");
	pMmio->DescendRiff ('M', 'U', 'S', 'C');

	// read the midi
	if (bMidi)
		{
		pMmio->DescendList ('M', 'I', 'D', 'I');
		pMmio->DescendChunk ('N', 'U', 'M', 'M');
		m_aMidi.SetSize (pMmio->ReadShort ());
		pMmio->AscendChunk ();

		// read in the MIDI tracks
		for (int iOn=0; iOn<m_aMidi.GetSize (); iOn++)
			{
			pMmio->DescendChunk ('D', 'A', 'T', 'A');
			int iLen = pMmio->ReadLong ();
			if ((m_aMidi[iOn] = MEM_alloc_lock (iLen)) != NULL)
				pMmio->Read (m_aMidi[iOn], iLen);
			pMmio->AscendChunk ();
			}
		pMmio->AscendList ();
		}

	// read the sfx
	char bTyp = bMidi ? '1' : '2';
	pMmio->DescendList ('S', 'F', 'X', bTyp);
	pMmio->DescendChunk ('N', 'U', 'M', 'S');
	m_aRaw.SetSize (pMmio->ReadShort ());
	pMmio->AscendChunk ();

	// ok, read the header info for each
	for (int iOn=0; iOn<m_aRaw.GetSize (); iOn++)
		m_aRaw[iOn].InitData (pMmio, iGrp);
	pMmio->AscendList ();

	// read the voices - this is in the LANGUAGE part
	CMmio *pMmioVoices = theDataFile.OpenAsMMIO (NULL);
	pMmioVoices->DescendRiff ('L', 'A', 'N', 'G');
	pMmioVoices->DescendList ('S', 'F', 'X', bTyp);
	pMmioVoices->DescendChunk ('N', 'U', 'M', 'S');
	int iBase = m_aRaw.GetSize ();
	m_aRaw.SetSize (iBase + pMmioVoices->ReadShort ());
	pMmioVoices->AscendChunk ();

	// ok, read the header info for each
	for (iOn=iBase; iOn<m_aRaw.GetSize (); iOn++)
		{
		m_aRaw[iOn].InitData (pMmioVoices, iGrp);
		m_aRaw[iOn].m_bVoice = TRUE;
		}
	pMmioVoices->AscendList ();
	delete pMmioVoices;

	// if we are not MIDI only read in the digital music headers
	if (m_iMode != midi_only)
		{
		// read the common music
		pMmio->DescendList ('M', 'U', 'S', '0');
		pMmio->DescendChunk ('N', 'U', 'M', 'M');
		iBase = m_aRaw.GetSize ();
		m_aRaw.SetSize (iBase + pMmio->ReadShort ());
		pMmio->AscendChunk ();

		for (iOn=iBase; iOn<m_aRaw.GetSize (); iOn++)
			m_aRaw[iOn].InitData (pMmio, iGrp);
		pMmio->AscendList ();

		// read the music if no MIDI
		if (! bMidi)
			{
			pMmio->DescendList ('M', 'U', 'S', '2');
			pMmio->DescendChunk ('N', 'U', 'M', 'M');
			int iBase = m_aRaw.GetSize ();
			m_aRaw.SetSize (iBase + pMmio->ReadShort ());
			pMmio->AscendChunk ();

			for (int iOn=iBase; iOn<m_aRaw.GetSize (); iOn++)
				m_aRaw[iOn].InitData (pMmio, iGrp);
			pMmio->AscendList ();
			}
		}

	delete pMmio;

	// for cached & buffered items
	ASSERT (m_pFileReg == NULL);
	ASSERT (m_pFileVoc == NULL);
#ifdef _DEBUG
	theDataFile.DisableNegativeSeekChecking ();
#endif

	m_pFileReg = theDataFile.OpenAsFile ("music");
	m_pFileVoc = theDataFile.OpenAsFile (NULL);

#ifdef _DEBUG
	theDataFile.EnableNegativeSeekChecking ();
#endif
}

BOOL CMusicPlayer::IsGroupLoaded (int iGrp)
{

	if (m_hDig == NULL)
		return TRUE;

	// see if ANY not loaded
	for (int iOn=0; iOn<m_aRaw.GetSize (); iOn++)
		if ((m_aRaw[iOn].m_iGroup == iGrp) && (m_aRaw[iOn].m_iType == CRawData::preload))
			if ( m_aRaw[iOn].m_iStat != CRawData::loaded )
				return FALSE;

	return TRUE;
}

void CMusicPlayer::LoadGroup (int iGrp)
{

	if (m_hDig == NULL)
		return;

	// read in the preload ones
	for (int iOn=0; iOn<m_aRaw.GetSize (); iOn++)
		if ((m_aRaw[iOn].m_iGroup == iGrp) && (m_aRaw[iOn].m_iType == CRawData::preload))
			{
			m_aRaw[iOn].LoadBuffer (m_aRaw[iOn].m_bVoice ? m_pFileVoc : m_pFileReg);
			AIL_serve ();
			}
}

void CMusicPlayer::UnloadGroup (int iGrp)
{

	// read in the preload ones
	for (int iOn=0; iOn<m_aRaw.GetSize (); iOn++)
		if (m_aRaw[iOn].m_iGroup == iGrp)
			m_aRaw[iOn].UnloadBuffer ();
}

// sets the system to 22-16-S if we are in MIDI mode
void CMusicPlayer::ToExclMusic ()
{

	// only do this if playing MIDI
	if ((m_iMode != mixed) || (! m_bRunning) || (m_iMusicVol == 0) || 
																				(m_bExclWav) || (m_iMusicVol == 0))
		{
		m_bExclWav = TRUE;
		return;
		}
	m_bExclWav = TRUE;

	StopMidiMusic ();

	if (m_bNoWav)
		return;

	// if we are playing sfx close it
	if (m_hDig != NULL)
		{
		AIL_waveOutClose (m_hDig);
		m_hDig = NULL;

		m_Channel[0].FreeDblBuf ();

		// let it get set up
		AIL_serve ();
		}

	OpenDigital (m_iMusicVol, FALSE);
}

// returns the system to MIDI / 11-8-M mode if using MIDI music
void CMusicPlayer::FromExclMusic ()
{

	// only do this if playing MIDI
	if ((m_iMode != mixed) || (! m_bRunning) || (m_iMusicVol == 0) || (! m_bExclWav))
		{
		m_bExclWav = FALSE;
		return;
		}
	m_bExclWav = FALSE;

	// if we are playing digital audio close it (we should be)
	if (m_hDig != NULL)
		{
		AIL_waveOutClose (m_hDig);
		m_hDig = NULL;

		// let it get set up
		AIL_serve ();
		}

	// if the SFX volume is 0 we're done
	if (m_iSfxVol == 0)
		{
		TRAP ();
		m_Channel[0].FreeDblBuf ();
		return;
		}

	OpenDigital (m_iSfxVol, TRUE);
}

void CMusicPlayer::OpenDigital (int iVol, BOOL bMidi)
{

	ASSERT ((0 <= iVol) && (iVol <= 127));
	ASSERT (m_hDig == NULL);

	if (m_bNoWav)
		return;

	try
		{
		// if we haven't started, start us up
		if (! m_bRunning)
			{
			AIL_startup ();
			m_bRunning = TRUE;
			}

		m_hDig = NULL;
		if ((iVol != 0) && (waveOutGetNumDevs () > 0))
			{
			// can force to ! DirectSound
			if ( ptheApp->GetProfileInt ( "Advanced", "NoDirectSound", 0 ) )
				AIL_set_preference (DIG_USE_WAVEOUT, YES);

			PCMWAVEFORMAT pwf;
			memset (&pwf, 0, sizeof (pwf));
			pwf.wf.wFormatTag = WAVE_FORMAT_PCM;
			if (bMidi)
				{
				// 2 channels for panning
				pwf.wf.nChannels = 2;
				pwf.wf.nSamplesPerSec = 11025;
				pwf.wf.nAvgBytesPerSec = 22050;
				pwf.wf.nBlockAlign = 2;
				pwf.wBitsPerSample = 8;
				}
			else
				{
				pwf.wf.nChannels = 2;
				pwf.wf.nSamplesPerSec = 22050;
				pwf.wf.nAvgBytesPerSec = 88200;
				pwf.wf.nBlockAlign = 4;
				pwf.wBitsPerSample = 16;
				}
			// try - if fails turn DirectSound off
			m_bUseDS = ( W32s != iWinType );
			if (AIL_waveOutOpen (&m_hDig, NULL, 0, (LPWAVEFORMAT) &pwf) != 0)
				{
				m_bUseDS = FALSE;
				AIL_set_preference (DIG_USE_WAVEOUT, YES);
				if (AIL_waveOutOpen (&m_hDig, NULL, 0, (LPWAVEFORMAT) &pwf) != 0)
					{
					CDlgMsg dlg;
					dlg.MsgBox (IDS_ERROR_WAV, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NoDigitalSound" );
					m_hDig = NULL;
					return;
					}
				}

			if (m_hDig == NULL)
				{
				TRAP ();
				return;
				}

			// let it get set up
			AIL_serve ();

			CRawChannel * pRaw = m_Channel;
			for (int iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
				if ((pRaw->m_hSmp = AIL_allocate_sample_handle (m_hDig)) != NULL)
					{
					AIL_init_sample (pRaw->m_hSmp);
					AIL_set_sample_user_data (pRaw->m_hSmp, 0, (long) pRaw);
					}
				else
					{
					pRaw = m_Channel;
					for (int iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
						pRaw->m_hSmp = NULL;
					AIL_waveOutClose (m_hDig);
					m_hDig = NULL;

					// let it get set up
					AIL_serve ();
					return;
					}

			AIL_set_digital_master_volume (m_hDig, iVol);

			// let it get set up
			AIL_serve ();
			}
		}

	catch (...)
		{
		m_bNoWav = TRUE;
		try
			{
			CDlgMsg dlg;
			dlg.MsgBox (IDS_ERROR_WAV, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NoDigitalSound" );
			AIL_waveOutClose (m_hDig);
			m_hDig = NULL;
			}
		catch (...)
			{
			}
		}
}

void CMusicPlayer::OpenMidi (int iVol)
{

	ASSERT ((0 <= iVol) && (iVol <= 127));
	ASSERT (m_hMdi == NULL);

	if (m_bNoMidi)
		return;

	if (m_iMode == wav_only)
		return;

	try
		{
		// if we haven't started, start us up
		if (! m_bRunning)
			{
			AIL_startup ();
			m_bRunning = TRUE;
			}

		m_hMdi = NULL;
		if ((iVol != 0) && (midiOutGetNumDevs () > 0))
			{
			m_bKillMidi = FALSE;
			AIL_midiOutOpen (&m_hMdi, NULL, MIDI_MAPPER);
			if (m_hMdi == NULL)
				{
				int iRes = ( WNT == iWinType ) ? IDS_ERROR_MIDI_NT : IDS_ERROR_MIDI;
				CDlgMsg dlg;
				dlg.MsgBox (iRes, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NoMidiSound" );
				TRAP ();
				return;
				}

			AIL_set_XMIDI_master_volume (m_hMdi, iVol);

			if ((m_hSeq = AIL_allocate_sequence_handle (m_hMdi)) == NULL)
				{
				m_bKillMidi = TRUE;
				AIL_midiOutClose (m_hMdi);
				m_hMdi = NULL;
				return;
				}
			}
		}

	catch (...)
		{
		m_bNoMidi = TRUE;
		try
			{
			int iRes = ( WNT == iWinType ) ? IDS_ERROR_MIDI_NT : IDS_ERROR_MIDI;
			CDlgMsg dlg;
			dlg.MsgBox (iRes, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NoMidiSound" );
			AIL_midiOutClose (m_hMdi);
			m_hMdi = NULL;
			}
		catch (...)
			{
			m_hMdi = NULL;
			}
		}
}

void CMusicPlayer::Close ()
{

	// don't need this anymore
	delete m_pFileReg;
	delete m_pFileVoc;
	m_pFileReg = m_pFileVoc = NULL;

	if (! m_bRunning)
		return;
	m_bKillMidi = TRUE;

	// close it down
	AIL_shutdown ();
	m_bRunning = FALSE;
	m_hMdi = NULL;
	m_hDig = NULL;

	for (int iOn=0; iOn<m_aMidi.GetSize(); iOn++)
		MEM_free_lock (m_aMidi[iOn]);
	m_aMidi.RemoveAll ();

	for (iOn=0; iOn<m_aRaw.GetSize(); iOn++)
		m_aRaw[iOn].Close ();
	m_aRaw.RemoveAll ();

	for (iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++)
		m_Channel[iOn].Close ();
}

void CMusicPlayer::ShutUp ()
{

	if (! m_bRunning)
		return;

	SoundsOff ();
}

void CMusicPlayer::OnActivate (BOOL bActive)
{

	if (! bActive)
		{
		// digital off
		if (m_hDig != NULL)
			AIL_digital_handle_release ( m_hDig );

		// midi off
		if (m_hMdi != NULL)
			AIL_MIDI_handle_release ( m_hMdi );
		return;
		}

	// digital on
	if (m_hDig != NULL)
		AIL_digital_handle_reacquire ( m_hDig );

	// midi on
	if (m_hMdi != NULL)
		AIL_MIDI_handle_reacquire ( m_hMdi );
}

void CMusicPlayer::PlayExclusiveMusic (int iSound)
{

	if ((m_iMusicVol == 0) || (! m_bRunning))
		return;

	if (m_iMode == midi_only)
		{
		StartMidiMusic ();
		return;
		}

	ASSERT ((0 <= iSound) && (iSound < m_aRaw.GetSize ()));

	// turn off MIDI
	StopMidiMusic ();

	if (m_bNoWav)
		return;

	// kill it if in use
	if (m_hDig != NULL)
		if (AIL_sample_status (m_Channel[0].m_hSmp) != SMP_DONE)
			{
			m_Channel[0].m_pData->m_bKillMe = TRUE;
			AIL_end_sample (m_Channel[0].m_hSmp);
			AIL_serve ();
			}

	// set music (in case vol == 0 and turned on later)
	m_Channel[0].m_pData = &(m_aRaw.ElementAt (iSound));

	// put us in 22-16-S
	ToExclMusic ();

	m_Channel[0].m_iIsFore = CRawChannel::excl;
	m_Channel[0].m_iBackPri = 0;
	StartRaw (&m_Channel[0], m_Channel[0].m_pData);
}

void CMusicPlayer::EndExclusiveMusic ()
{

	if ((m_iMusicVol == 0) || (m_iMode == midi_only) || (! m_bRunning))
		return;

	// kill it if in use
	if ((m_hDig != NULL) && (m_Channel[0].m_iIsFore == CRawChannel::excl))
		{
		m_Channel[0].m_iIsFore = CRawChannel::back;

		// close it down
		if (m_hDig != NULL)
			if (AIL_sample_status (m_Channel[0].m_hSmp) != SMP_DONE)
				{
				m_Channel[0].m_pData->m_bKillMe = TRUE;
				AIL_end_sample (m_Channel[0].m_hSmp);
				AIL_serve ();
				}

		m_Channel[0].m_iBackPri = INT_MAX;
		m_Channel[0].m_iNum = 0;
		}

	// put us back in 11-8-M (if MIDI)
	FromExclMusic ();

	// free up the memory if we're MIDI
	if (m_iMode == mixed)
		m_Channel[0].FreeDblBuf ();

	m_Channel[0].m_pData = NULL;

	// start MIDI if off
	if ((m_hMdi == NULL) && (m_iMusicVol > 0) && (m_iMode != wav_only))
		return;

	if (m_bNoMidi)
		return;

	OpenMidi (m_iMusicVol);
}

void CMusicPlayer::EndMusicGroup ()
{

	if ((m_iMusicVol == 0) || (m_iMode != wav_only) || (! m_bRunning))
		return;

	TRAP ();

	// kill it if in use
	if ((m_hDig != NULL) && (m_Channel[0].m_iIsFore == CRawChannel::music))
		{
		TRAP ();
		m_Channel[0].m_iIsFore = CRawChannel::back;

		// close it down
		if (m_hDig != NULL)
			if (AIL_sample_status (m_Channel[0].m_hSmp) != SMP_DONE)
				{
				m_Channel[0].m_pData->m_bKillMe = TRUE;
				AIL_end_sample (m_Channel[0].m_hSmp);
				AIL_serve ();
				}

		m_Channel[0].m_iBackPri = INT_MAX;
		m_Channel[0].m_iNum = 0;
		}

	m_Channel[0].m_pData = NULL;
}

void CMusicPlayer::PlayMusicGroup (int iGrpStrt, int iNumGrp)
{

	// if MIDI we ignore this
	if ((m_hDig == NULL) || (m_iMode != wav_only))
		return;

	m_iMusicGrpFirst = iGrpStrt;
	m_iMusicGrpNum = iNumGrp;
	
	// kill it if in use
	if (AIL_sample_status (m_Channel[0].m_hSmp) != SMP_DONE)
		{
		m_Channel[0].m_pData->m_bKillMe = TRUE;
		AIL_end_sample (m_Channel[0].m_hSmp);
		AIL_serve ();
		}

	m_Channel[0].m_iIsFore = CRawChannel::music;
	m_Channel[0].m_iBackPri = 0;
	StartRaw (&m_Channel[0], &(m_aRaw.ElementAt (m_iMusicGrpFirst + RandNum (m_iMusicGrpNum - 1))));
}

void CMusicPlayer::PlayForegroundSound (int iSound, int iPan, int iVol)
{

	if ((iSound <= 0) || (iSound >= m_aRaw.GetSize ()) || (m_hDig == NULL) || (m_iSfxVol == 0))
		return;

	// find the lowest pri background channel
	CRawChannel * pRawOn;
	int iVal = -1;
	{
	CRawChannel * pRaw = m_Channel;
	for (int iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
		if (pRaw->m_iIsFore == CRawChannel::back)
			if ((iVal == -1) || (pRaw->m_iBackPri > iVal))
				{
				pRawOn = pRaw;
				iVal = pRaw->m_iBackPri;
				if (pRaw->m_iBackPri == INT_MAX)
					break;
				}
	}

	// if all are hi-pri take one
	if (iVal == -1)
		pRawOn = &m_Channel [1 + RandNum (MAX_SOUND_SAMPLES - 2)];

	// kill it if in use
	if (AIL_sample_status (pRawOn->m_hSmp) != SMP_DONE)
		{
		pRawOn->m_pData->m_bKillMe = TRUE;
		AIL_end_sample (pRawOn->m_hSmp);
		AIL_serve ();
		}

	pRawOn->m_iIsFore = CRawChannel::fore;
	pRawOn->m_iVol = iVol;
	pRawOn->m_iPan = iPan;
	StartRaw (pRawOn, &(m_aRaw.ElementAt (iSound)));
}

void CMusicPlayer::KillForegroundSound (int iSound)
{

	if ((iSound <= 0) || (iSound >= m_aRaw.GetSize ()))
		return;

	m_aRaw.ElementAt (iSound).UnloadBuffer ();
}

// leaves sounds playing but clears them from m_iBackground
void CMusicPlayer::ClrBackgroundSounds ()
{

	// if not playing leave (we don't track background sounds)
	if ((m_hDig == NULL) || (m_iSfxVol == 0))
		{
		TRAP ();
		return;
		}

	CRawChannel * pRaw = m_Channel;
	for (int iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
		if ((pRaw->m_iIsFore == CRawChannel::back) || (pRaw->m_iIsFore == CRawChannel::fore))
			{
			pRaw->m_iBackPri = INT_MAX;
			pRaw->m_iNum = 0;
			}
}

void CMusicPlayer::QueueBackgroundSound (int iSound, int iPri, int iPan, int iVol)
{

	// if not playing leave (we don't track background sounds)
	if ((iSound == 0) || (iSound >= m_aRaw.GetSize ()) || (iVol < 10) || (m_hDig == NULL) || (m_iSfxVol == 0))
		{
		TRAP ();
		return;
		}

	// if we're already in there - leave us alone
	CRawChannel * pRaw = m_Channel;
	for (int iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
		if (abs (pRaw->m_iBackSound) == iSound)
			{
			pRaw->m_iBackSound = - iSound;
			pRaw->m_iBackPri = iPri;
			pRaw->m_iNum ++;
			if (pRaw->m_iNum <= 1)
				{
				pRaw->m_iVol = iVol;
				pRaw->m_iPan = iPan;
				}
			else
				{
				pRaw->m_iVol = (pRaw->m_iVol * (pRaw->m_iNum - 1) + iVol) / pRaw->m_iNum;
				pRaw->m_iPan = (pRaw->m_iPan * (pRaw->m_iNum - 1) + iPan) / pRaw->m_iNum;
				}
			return;
			}

	// find the highest # (lowest priority)
	pRaw = m_Channel;
	int iVal = pRaw->m_iBackPri;
	CRawChannel * pRawOn = m_Channel;
	for (iOn=1; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
		if (pRaw->m_iBackPri > iVal)
			{
			pRawOn = pRaw;
			iVal = pRaw->m_iBackPri;
			}

	// if we're higher - forget it
	if (iVal < iPri)
		return;

	pRawOn->m_iBackPri = iPri;
	pRawOn->m_iBackSound = iSound;
	pRawOn->m_iVol = iVol;
	pRawOn->m_iPan = iPan;
	pRawOn->m_iNum = 1;
}

// we switch to the sounds we now have in m_iBackSound
void CMusicPlayer::UpdateBackgroundSounds ()
{

	// if not playing leave (we don't track background sounds)
	if ((m_hDig == NULL) || (m_iSfxVol == 0))
		{
		TRAP ();
		return;
		}

	CRawChannel * pRaw = m_Channel;
	for (int iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
		if (pRaw->m_iIsFore != CRawChannel::back)
			pRaw->m_iBackSound = abs (pRaw->m_iBackSound);
		else
			{
			// we never used it - kill it
			if (pRaw->m_iBackPri == INT_MAX)
				{
				if (AIL_sample_status (pRaw->m_hSmp) != SMP_DONE)
					{
					pRaw->m_pData->m_bKillMe = TRUE;
					AIL_end_sample (pRaw->m_hSmp);
					AIL_serve ();
					}
				pRaw->m_iBackSound = 0;
				pRaw->m_pData = NULL;
				}
			else

				// if its the same leave it alone
				if (pRaw->m_iBackSound < 0)
					pRaw->m_iBackSound = - pRaw->m_iBackSound;
				else
				
					// start it up
					{
					if (AIL_sample_status (pRaw->m_hSmp) != SMP_DONE)
						{
						pRaw->m_pData->m_bKillMe = TRUE;
						AIL_end_sample (pRaw->m_hSmp);
						AIL_serve ();
						}

					StartRaw (pRaw, &(m_aRaw.ElementAt (pRaw->m_iBackSound)));
					}
			}

	AIL_serve ();
}

void CMusicPlayer::IncBackgroundSound (int iSound, int iPan, int iVol)
{

	// if not playing leave (we don't track background sounds)
	if ((iSound == 0) || (iSound >= m_aRaw.GetSize ()) || (iVol < 10) || (m_hDig == NULL) || (m_iSfxVol == 0))
		return;

	// find it
	CRawChannel * pRaw = m_Channel;
	for (int iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
		if (abs (pRaw->m_iBackSound) == iSound)
			{
			pRaw->m_iNum ++;
			if (pRaw->m_iNum <= 1)
				{
				pRaw->m_iVol = iVol;
				pRaw->m_iPan = iPan;
				}
			else
				{
				pRaw->m_iVol = (pRaw->m_iVol * (pRaw->m_iNum - 1) + iVol) / pRaw->m_iNum;
				pRaw->m_iPan = (pRaw->m_iPan * (pRaw->m_iNum - 1) + iPan) / pRaw->m_iNum;
				}
			return;
			}

	// not playing - try to find a channel
	pRaw = m_Channel;
	int iVal = pRaw->m_iBackPri;
	CRawChannel * pRawOn = m_Channel;
	for (iOn=1; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
		if (pRaw->m_iBackPri > iVal)
			{
			pRawOn = pRaw;
			iVal = pRaw->m_iBackPri;
			}

	// if we're higher - forget it
	if (iVal <= BACKGROUND_PRI)
		return;

	pRawOn->m_iBackPri = BACKGROUND_PRI;
	pRawOn->m_iBackSound = iSound;
	pRawOn->m_iVol = iVol;
	pRawOn->m_iPan = iPan;
	pRawOn->m_iNum = 1;

	if (pRawOn->m_iIsFore != CRawChannel::back)
		return;

	// start it up
	if (AIL_sample_status (pRawOn->m_hSmp) != SMP_DONE)
		{
		pRawOn->m_pData->m_bKillMe = TRUE;
		AIL_end_sample (pRawOn->m_hSmp);
		AIL_serve ();
		}

	StartRaw (pRawOn, &(m_aRaw.ElementAt (pRawOn->m_iBackSound)));
}

void CMusicPlayer::DecBackgroundSound (int iSound, int iPan, int iVol)
{

	// if not playing leave (we don't track background sounds)
	if ((iSound == 0) || (iSound >= m_aRaw.GetSize ()) || (iVol < 10) || (m_hDig == NULL) || (m_iSfxVol == 0))
		return;

	// find it
	CRawChannel * pRaw = m_Channel;
	for (int iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
		if (abs (pRaw->m_iBackSound) == iSound)
			{
			// BUGBUG - change vol/pan
			pRaw->m_iNum --;
			if ((pRaw->m_iNum > 0) || (pRaw->m_iIsFore != CRawChannel::back))
				{
				if (pRaw->m_iNum <= 0)
					pRaw->m_iBackSound = 0;
				break;
				}

			// ok - turn it off
			if (AIL_sample_status (pRaw->m_hSmp) != SMP_DONE)
				{
				pRaw->m_pData->m_bKillMe = TRUE;
				AIL_end_sample (pRaw->m_hSmp);
				AIL_serve ();
				}
			pRaw->m_iBackSound = 0;
			pRaw->m_pData = NULL;
			break;
			}
}

void CMusicPlayer::SoundsOff ()
{

	StopMidiMusic ();

	if (m_hDig == NULL)
		return;

	CRawChannel * pRaw = m_Channel;
	for (int iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
		{
		pRaw->m_iIsFore = CRawChannel::back;
		pRaw->m_iBackSound = 0;
		pRaw->m_iBackPri = INT_MAX;
		if (AIL_sample_status (pRaw->m_hSmp) != SMP_DONE)
			{
			pRaw->m_pData->m_bKillMe = TRUE;
			AIL_end_sample (pRaw->m_hSmp);
			AIL_serve ();
			}

		pRaw->FreeDblBuf ();
		}
}

// return TRUE if playing digital music
BOOL CMusicPlayer::IsWavMusic ()
{

	if (m_iMode == midi_only)
		return (FALSE);
		
	if (m_iMode == wav_only)
		return (TRUE);

	return (m_bExclWav);
}

void WINAPI CMusicPlayer::RawDblBufCallback (HSAMPLE hSmp)
{

	CRawChannel * pRaw = (CRawChannel *) AIL_sample_user_data (hSmp, 0);
	ASSERT (pRaw->m_hSmp == hSmp);

	// this means we are done with this
	if (pRaw->m_pData->m_bKillMe)
		{
		pRaw->m_pData->m_bKillMe = FALSE;
		return;
		}

	if ((pRaw->m_iIsFore == CRawChannel::back) && (pRaw->m_iBackPri == INT_MAX))
		return;

	if (pRaw->m_pPar->m_iMusicVol == 0)
		if ((pRaw->m_iIsFore == CRawChannel::excl) || (pRaw->m_iIsFore == CRawChannel::music))
			return;

	// get both
	for (int iInd=2; iInd>0; iInd--)
		{
		int iNeed = AIL_sample_buffer_ready (hSmp);
		if ((iNeed != 0) && (iNeed != 1))
			return;

		// read the data
		pRaw->m_pData->LoadNextDblBuffer (pRaw, iNeed);
		}
}

void WINAPI CMusicPlayer::RawCallback (HSAMPLE hSmp)
{

	CRawChannel * pRaw = (CRawChannel *) AIL_sample_user_data (hSmp, 0);
	ASSERT (pRaw->m_hSmp == hSmp);

	if (pRaw->m_pData->m_bKillMe)
		{
		pRaw->m_pData->m_bKillMe = FALSE;
		pRaw->m_pData = NULL;
		return;
		}

	// if it's group music
	if (pRaw->m_iIsFore == CRawChannel::music)
		{
		TRAP ();
		// pick the new piece
		theMusicPlayer.StartRaw (pRaw, &(pRaw->m_pPar->m_aRaw.ElementAt (pRaw->m_pPar->m_iMusicGrpFirst + 
																			RandNum (pRaw->m_pPar->m_iMusicGrpNum - 1))));
		return;
		}

	// exclusive music
	if (pRaw->m_iIsFore == CRawChannel::excl)
		{
		if (pRaw->m_pPar->m_iMusicVol == 0)
			return;

		// if exclusive being turned off - no replay
		if (! pRaw->m_pPar->m_bExclWav)
			{
			TRAP ();
			return;
			}

		theMusicPlayer.StartRaw (pRaw, pRaw->m_pData);
		return;
		}

	// ok - we're a foreground sound finishing (background sounds don't call this - they loop)

	// kill it if still running???
	pRaw->m_iIsFore = CRawChannel::back;
	if (AIL_sample_status (pRaw->m_hSmp) != SMP_DONE)
		{
		TRAP ();
		pRaw->m_pData->m_bKillMe = TRUE;
		AIL_end_sample (pRaw->m_hSmp);
		}

	// no background sound - we only need to switch from foreground to background mode
	// no sound
	if ( (pRaw->m_iBackPri == INT_MAX) || (pRaw->m_pPar->m_iSfxVol == 0) )
		{
		pRaw->m_pData = NULL;
		return;
		}

	// busy setting new sounds
	if ( pRaw->m_iBackSound < 0 )
		{
		pRaw->m_iBackSound = - pRaw->m_iBackSound;
		return;
		}

	theMusicPlayer.StartRaw (pRaw, &(theMusicPlayer.m_aRaw.ElementAt (pRaw->m_iBackSound)));
}

void CMusicPlayer::StartMidiMusic ()
{

	if (m_hMdi == NULL)
		return;

	if (m_bNoMidi)
		return;

	// turn off WAV music
	EndExclusiveMusic ();

	// end old sequence
	if ((AIL_sequence_status (m_hSeq) == SEQ_PLAYING) || 
																(AIL_sequence_status (m_hSeq) == SEQ_STOPPED))
		{
		m_bKillMidi = TRUE;
		AIL_end_sequence (m_hSeq);
		AIL_serve ();
		m_bKillMidi = FALSE;
		}

	// no music if vol == 0
	if (m_iMusicVol <= 0)
		{
		TRAP ();
		return;
		}

	// start new sequence
	int iSel = RandNum (m_aMidi.GetSize () - 1);
	AIL_init_sequence (m_hSeq, m_aMidi[iSel], 0);
	AIL_register_sequence_callback (m_hSeq, MidiCallback);
	AIL_start_sequence (m_hSeq);

	AIL_serve ();
}

void CMusicPlayer::StopMidiMusic ()
{

	if (m_hMdi == NULL)
		return;
	if ((AIL_sequence_status (m_hSeq) != SEQ_PLAYING) &&
																(AIL_sequence_status (m_hSeq) != SEQ_STOPPED))
		return;

	m_bKillMidi = TRUE;

	// end old sequence
	AIL_end_sequence (m_hSeq);

	AIL_serve ();
}

void WINAPI CMusicPlayer::MidiCallback (HSEQUENCE hSeq)
{

	ASSERT (hSeq == theMusicPlayer.m_hSeq);

	// see if we are turning it off
	if (theMusicPlayer.m_bKillMidi)
		{
		theMusicPlayer.m_bKillMidi = FALSE;
		return;
		}
		
	// time to play another tune in the Grp
	int iSel = RandNum (theMusicPlayer.m_aMidi.GetSize () - 1);

	AIL_end_sequence (theMusicPlayer.m_hSeq);
	AIL_init_sequence (theMusicPlayer.m_hSeq, theMusicPlayer.m_aMidi[iSel], 0);
	AIL_register_sequence_callback (theMusicPlayer.m_hSeq, MidiCallback);
	AIL_start_sequence (theMusicPlayer.m_hSeq);
}

int CMusicPlayer::GetMusicVolume () const
{

	if ( (m_iMode == midi_only) && (m_hMdi == NULL) )
		return (0);
		
	if ( (m_iMode != midi_only) && (m_hDig == NULL) )
		return (0);
		
	return ((m_iMusicVol * 100) / 127);
}

void CMusicPlayer::SetMusicVolume (int iVol)
{

	ASSERT ((0 <= iVol) && (iVol <= 100));

	iVol = (iVol * 127) / 100;
	if (iVol == m_iMusicVol)
		return;
	m_iMusicVol = iVol;

	// if they're both 0 shut it down
	if ((m_iMusicVol == 0) && (m_iSfxVol == 0))
		{
		AIL_shutdown ();
		m_bRunning = FALSE;
		m_hDig = NULL;
		m_hMdi = NULL;
		return;
		}

	// if we're playing digital audio fix the volume
	if (m_bExclWav || (m_iMode == wav_only))
		{
		CheckDigVol ();

		// if we're no MIDI we're done
		if (m_iMode == wav_only)
			return;
		}

	if (m_bNoMidi)
		return;

	// turn it off
	if (iVol == 0)
		{
		if (m_hMdi == NULL)
			return;

		m_bKillMidi = TRUE;

		AIL_end_sequence (m_hSeq);
		AIL_release_sequence_handle (m_hSeq);
		m_hSeq = NULL;

		AIL_midiOutClose (m_hMdi);
		m_hMdi = NULL;
		return;
		}

	// turn it on
	if (m_hMdi == NULL)
		{
		OpenMidi (iVol);

		// start it up if ! exclusive digital mode
		if ((m_hMdi != NULL) && (! m_bExclWav))
			MidiCallback (m_hSeq);
		return;
		}

	// set the volume
	AIL_set_XMIDI_master_volume (m_hMdi, m_iMusicVol);
}

int CMusicPlayer::GetSoundVolume () const
{

	if (m_hDig == NULL)
		return (0);
		
	return ((m_iSfxVol * 100) / 127);
}

void CMusicPlayer::SetSoundVolume (int iVol)
{

	ASSERT ((0 <= iVol) && (iVol <= 100));

	iVol = (iVol * 127) / 100;
	if (iVol == m_iSfxVol)
		return;

	m_iSfxVol = iVol;

	if ( m_bNoWav )
		return;

	// if we're both off - kill it
	if ((m_iMusicVol == 0) && (m_iSfxVol == 0))
		{
		AIL_shutdown ();
		m_bRunning = FALSE;
		m_hDig = NULL;
		m_hMdi = NULL;
		return;
		}

	CheckDigVol ();
}

void CMusicPlayer::CheckDigVol ()
{

	if (m_bNoWav)
		return;

	// figure the new volume for digital
	int iVol = m_iSfxVol;
	if (m_bExclWav || (m_iMode == wav_only))
		iVol = __max (m_iSfxVol, m_iMusicVol);

	// turn it off
	if (iVol == 0)
		{
		if (m_hDig == NULL)
			return;
		AIL_waveOutClose (m_hDig);
		m_hDig = NULL;

		m_Channel[0].FreeDblBuf ();

		// let it get set up
		AIL_serve ();
		return;
		}

	// turn it on
	if (m_hDig == NULL)
		{
		OpenDigital (iVol, (m_iMode == midi_only) || ((m_iMode == mixed) && (! m_bExclWav)));
		if (m_hDig == NULL)
			return;
		}

	AIL_set_digital_master_volume (m_hDig, iVol);

	// restart audio
	CRawChannel * pRaw = m_Channel;
	for (int iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
		{
		if ((pRaw->m_pData != NULL) && ((pRaw->m_iIsFore != CRawChannel::back) || 
													((pRaw->m_iBackPri >= 0) && (pRaw->m_iBackPri != INT_MAX))))
			{
			// if not playing we need to restart
			BOOL bChnlDone = AIL_sample_status (pRaw->m_hSmp) == SMP_DONE;

			// turn off if music vol 0
			if ((m_iMusicVol == 0) && ((pRaw->m_iIsFore == CRawChannel::excl) || (pRaw->m_iIsFore == CRawChannel::music)))
				{
				if (! bChnlDone)
					{
					pRaw->m_pData->m_bKillMe = TRUE;
					AIL_end_sample (pRaw->m_hSmp);
					AIL_serve ();
					}
				continue;
				}

			// turn off if sfx vol 0
			if ((m_iSfxVol == 0) && ((pRaw->m_iIsFore == CRawChannel::back) || (pRaw->m_iIsFore == CRawChannel::fore)))
				{
				if (! bChnlDone)
					{
					pRaw->m_pData->m_bKillMe = TRUE;
					AIL_end_sample (pRaw->m_hSmp);
					AIL_serve ();
					}
				continue;
				}

			// if just a change in volume (non-zero to non-zero) just change the vol
			if (! bChnlDone)
				{
				// set the volume, ( we skipped above if vol == 0)
				if ((pRaw->m_iIsFore == CRawChannel::excl) || (pRaw->m_iIsFore == CRawChannel::music))
					AIL_set_sample_volume (pRaw->m_hSmp, m_iMusicVol);
				else
					AIL_set_sample_volume (pRaw->m_hSmp, m_iSfxVol);
				continue;
				}

			StartRaw (pRaw, pRaw->m_pData);
			}
		}

	AIL_serve ();
}

BOOL CMusicPlayer::StartRaw (CRawChannel * pRawChannel, CRawData * pRawData)
{

	pRawChannel->m_pData = pRawData;

	// if no vol or no hdl -- exit
	if (m_hDig == NULL)
		return (TRUE);
	if ((pRawChannel->m_iIsFore == CRawChannel::excl) || (pRawChannel->m_iIsFore == CRawChannel::music))
		{
		if (m_iMusicVol == 0)
			return (TRUE);
		}
	else
		if (m_iSfxVol == 0)
			return (TRUE);

	if (pRawData->m_iType == CRawData::buffer)
		{
		if (pRawChannel->m_pDblBuf[1] == NULL)
			if (! pRawChannel->AllocDblBuf ())
				{
				TRAP ();
				return (FALSE);
				}
		}
	else
		// read in if not in memory
		{
		if (pRawData->m_pBuf == NULL)
			pRawData->LoadBuffer ( pRawData->m_bVoice ? m_pFileVoc : m_pFileReg );

#ifdef BUGBUG
		// it's already loading
		if ( pRawData->m_iStat == CRawData::loading )
			{
			TRAP ();
			return (FALSE);
			}

		if (pRawData->m_pBuf == NULL)
			{
			// alloc the memory if necessary
			if ((pRawData->m_pBuf = MEM_alloc_lock (pRawData->m_iFileLen)) == NULL)
				{
				TRAP ();
				pRawChannel->m_iBackPri = INT_MAX;
				pRawChannel->m_iIsFore = CRawChannel::back;
				return (FALSE);
				}

			// cache request
			pRawData->m_iStat = CRawData::loading;
			pRawData->m_pRc = pRawChannel;
			theDiskCache.AddRequest (pRawData->m_bVoice ? m_pFileVoc->m_hFile : 
												m_pFileReg->m_hFile, pRawData->m_lOff, 
												pRawData->m_iFileLen, pRawData->m_pBuf, CacheCallback, (DWORD) pRawData );
			::Sleep (0);
			return (TRUE);
			}
#endif
		}

	pRawData->m_bKillMe = FALSE;
	pRawData->m_dwLastUsed = timeGetTime ();

	// set it up
	AIL_init_sample (pRawChannel->m_hSmp);
	AIL_set_sample_loop_count (pRawChannel->m_hSmp, 1);
	AIL_set_sample_type (pRawChannel->m_hSmp, pRawData->m_iFmt, 0);

	// set the volume
	if ((pRawChannel->m_iIsFore == CRawChannel::excl) || (pRawChannel->m_iIsFore == CRawChannel::music))
		AIL_set_sample_volume (pRawChannel->m_hSmp, m_iMusicVol);
	else
		{
		pRawChannel->m_iVol = __minmax ( 10, 127, pRawChannel->m_iVol );
		AIL_set_sample_volume (pRawChannel->m_hSmp, (m_iSfxVol * pRawChannel->m_iVol) / 128);
		pRawChannel->m_iPan = __minmax ( 0, 127, pRawChannel->m_iPan );
		AIL_set_sample_pan (pRawChannel->m_hSmp, pRawChannel->m_iPan);
		}

	// playback rate
	AIL_set_sample_playback_rate (pRawChannel->m_hSmp, pRawData->m_iFmt == DIG_F_MONO_8 ? 11025 : 22050);

	// double buffered
	if (pRawData->m_iType == CRawData::buffer)
		{
		AIL_register_EOB_callback (pRawChannel->m_hSmp, RawDblBufCallback);
		AIL_register_EOS_callback (pRawChannel->m_hSmp, RawCallback);
		AIL_set_sample_loop_count (pRawChannel->m_hSmp, 1);

		if (AIL_sample_buffer_ready (pRawChannel->m_hSmp) == 0)
			pRawData->StartDblBuffer (pRawChannel);
		if (AIL_sample_buffer_ready (pRawChannel->m_hSmp) == 1)
			pRawData->LoadNextDblBuffer (pRawChannel, 1);
		}
	else

		{
		// start it
		if (pRawChannel->m_iIsFore != CRawChannel::back)
			{
			AIL_register_EOS_callback (pRawChannel->m_hSmp, RawCallback);
			AIL_set_sample_loop_count (pRawChannel->m_hSmp, 1);
			}
		else
			AIL_set_sample_loop_count (pRawChannel->m_hSmp, 0);

		AIL_set_sample_address (pRawChannel->m_hSmp, pRawData->m_pBuf, 
																							pRawData->m_iDataLen);
		AIL_start_sample (pRawChannel->m_hSmp);
		}

	AIL_serve ();
	return (TRUE);
}

#ifdef BUGBUG
// disk cache calls us
void CMusicPlayer::CacheCallback ( DWORD dwData )
{

	CRawData * pRawData = (CRawData *) dwData;

	pRawData->m_dwLastUsed = timeGetTime ();
	pRawData->m_iStat = CRawData::loaded;
	pRawData->m_bKillMe = FALSE;

	// if we are compressed, decompress!!
	if (pRawData->m_iComp != -1)
		{
		void * pDeComp = CoDec::Decompress ( pRawData->m_pBuf, pRawData->m_iFileLen, 
																													pRawData->m_iDataLen );
		MEM_free_lock (pRawData->m_pBuf);

		if ((pRawData->m_pBuf = MEM_alloc_lock (pRawData->m_iDataLen)) != NULL)
			memcpy (pRawData->m_pBuf, pDeComp, pRawData->m_iDataLen);
		CoDec::FreeBuf ( pDeComp );
		}

	// our channel may not want us anymore
	if (pRawData->m_pRc->m_pData != pRawData)
		{
		pRawData->m_pRc = NULL;
		return;
		}
	CRawChannel * pRawChannel = pRawData->m_pRc;
	pRawData->m_pRc = NULL;

	// set it up
	AIL_init_sample (pRawChannel->m_hSmp);
	AIL_set_sample_loop_count (pRawChannel->m_hSmp, 1);
	AIL_set_sample_type (pRawChannel->m_hSmp, pRawData->m_iFmt, 0);

	// set the volume
	if ((pRawChannel->m_iIsFore == CRawChannel::excl) || (pRawChannel->m_iIsFore == CRawChannel::music))
		AIL_set_sample_volume (pRawChannel->m_hSmp, pRawChannel->m_pPar->m_iMusicVol);
	else
		{
		pRawChannel->m_iVol = __minmax ( 10, 127, pRawChannel->m_iVol );
		AIL_set_sample_volume (pRawChannel->m_hSmp, (pRawChannel->m_pPar->m_iSfxVol * pRawChannel->m_iVol) / 128);
		pRawChannel->m_iPan = __minmax ( 0, 127, pRawChannel->m_iPan );
		AIL_set_sample_pan (pRawChannel->m_hSmp, pRawChannel->m_iPan);
		}

	// playback rate
	AIL_set_sample_playback_rate (pRawChannel->m_hSmp, pRawData->m_iFmt == DIG_F_MONO_8 ? 11025 : 22050);

	// double buffered
	if (pRawData->m_iType == CRawData::buffer)
		{
		AIL_register_EOB_callback (pRawChannel->m_hSmp, RawDblBufCallback);
		AIL_register_EOS_callback (pRawChannel->m_hSmp, RawCallback);
		AIL_set_sample_loop_count (pRawChannel->m_hSmp, 1);

		if (AIL_sample_buffer_ready (pRawChannel->m_hSmp) == 0)
			pRawData->StartDblBuffer (pRawChannel);
		if (AIL_sample_buffer_ready (pRawChannel->m_hSmp) == 1)
			pRawData->LoadNextDblBuffer (pRawChannel, 1);
		}
	else

		{
		// start it
		AIL_set_sample_address (pRawChannel->m_hSmp, pRawData->m_pBuf, 
																							pRawData->m_iDataLen);
		if (pRawChannel->m_iIsFore != CRawChannel::back)
			{
			AIL_register_EOS_callback (pRawChannel->m_hSmp, RawCallback);
			AIL_set_sample_loop_count (pRawChannel->m_hSmp, 1);
			}
		else
			AIL_set_sample_loop_count (pRawChannel->m_hSmp, 0);
		AIL_start_sample (pRawChannel->m_hSmp);
		}

	AIL_serve ();
}

void CMusicPlayer::FreeOldSounds ( int iSec )
{

	DWORD dwMin = timeGetTime () - iSec * 1000;

	for (int iOn=0; iOn<m_aRaw.GetSize (); iOn++)
		{
		CRawData * pRd = &(m_aRaw[iOn]);
		if (pRd->m_iType == CRawData::cache)
			if (pRd->m_iStat == CRawData::loaded)
				if (pRd->m_dwLastUsed < dwMin)
					{

					// make sure not in use
					CRawChannel * pRaw = m_Channel;
					for (int iOn=0; iOn<MAX_SOUND_SAMPLES; iOn++, pRaw++)
						if (pRaw->m_pData == pRd)
							goto TryNext;
					pRd->UnloadBuffer ();
TryNext:
		;
					}
		}
}
#endif

void CMusicPlayer::YieldPlayer () const
{

	AIL_serve ();
}
