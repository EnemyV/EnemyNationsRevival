#ifndef __MUSIC_H__
#define __MUSIC_H__

#include "acmutil.h"


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//  Because I wrote the Music (MIDI) player first this is called Music. But it
//	actually handles all sounds; MIDI, Digital Audio Music, SFX, and voice.
//  Voice is just another sfx and all sfx & voice are stored as mono.
//
//	Digital Audio is stored at 16-bit 22KHz stereo. SFX is stored at both 8-bit
//	11 KHz (if using MIDI for music) and 16-bit 22KHz if playing Digital Audio
//	(so it mixes a lot faster/easier).
//
//	All WAV files belong to a group (some groups have just 1 file). We can load
//	and unload a group. We can also play a sound which will load that sound if
//	it's not loaded (useful for 1 file groups).
//
//	MIDI just rotates through it's sounds - it's there for low-end systems and
//	doesn't take much memory so we don't do all this fancy stuff for it.
//
//	Because we don't have a 1:1 relation between MIDI tracks and Digital Audio
//	we have seperate calls for each that the app can make and this code ignores
//	the irrelevant calls.
//
//	WAV files are marked either pre-load (we don't cache them), cache (read when
//	needed and discarded if not used for awhile), and buffered (primarily music,
//	it's read in in chunks as it's played).
//
//	When you load a group it reads in all the pre-loaded files and reads the offset
//	and length of all the others.
//
//---------------------------------------------------------------------------


class CMusicPlayer;
class CRawChannel;
class CRawData;


const int MAX_SOUND_SAMPLES = 6;
const int DBL_BUF_LEN = 0x20000;	// 128K


/////////////////////////////////////////////////////////////////////////////
// CRawChannel - this is a channel we are mixing & playing

const int BACKGROUND_PRI = 400;
const int FOREGROUND_PRI = 50;

class CRawChannel
{
public:
		CRawChannel ();
		~CRawChannel () { Close (); }

		void				Close ();
		BOOL				AllocDblBuf ();
		void				FreeDblBuf ();
		void				BackgroundRead (UINT iHdl, int iOff, int iLen );

		HSAMPLE					m_hSmp;
		BOOL						m_bKill;				// we are stopping (for callback)
									enum FORE { back, fore, excl, music };
		FORE						m_iIsFore;			// 0 == background sfx, 1 == foreground sfx
																// 2 == exclusive music, 3 == digital music (group)
		int							m_iBackSound;		// background effect for this channel
		int							m_iBackPri;			// background priority
		int							m_iNum;					// number of requests
		int							m_iVol;					// volume set to
		int							m_iPan;
		int							m_iForeSound;		// foreground sound playing
		int							m_iForePri;			// foreground priority
		
		CRawData *			m_pData;				// data we are playing
		void *					m_pDblBuf[3];		// double buffer for music

		CRITICAL_SECTION m_cs;
		BOOL						 m_bCritSec;		// critical section created
		CWinThread *		m_pThrd;				// double buffering read-ahead thread
		DWORD						m_iReadLen;			// how much the read ahead got (end of file)
									enum STAT { _unused, _idle, _reading, _read };
		STAT						m_cStat;
		UINT						m_iHdl;					// file to read from
		int							m_iOff;					// offset to read from
		DWORD						m_iRead;				// how much to read
		BOOL						m_bRunning;			// thread should run
		int							m_iBufOn;				// 0..2

		CMusicPlayer * 	m_pPar;					// who holds us
};


/////////////////////////////////////////////////////////////////////////////
// CRawData - this is a single WAV file

class CRawData
{
public:
		CRawData ();
		~CRawData () { Close (); }

		void			Close ();

		void			InitData (CMmio * pMmio, int iGrp = -1);
		void			LoadBuffer (CFile * pFile);
		void			UnloadBuffer ();

		static void	DblBufCallBack (DWORD dwData);

		void			StartDblBuffer (CRawChannel * pRaw);
		void			LoadNextDblBuffer (CRawChannel * pRaw, int iNeed);

							enum TYPE { preload, cache, buffer };
		TYPE				m_iType;					// if data is loaded, cached, or buffered as played

		LONG				m_lOff;						// offset in data file of WAV file
		LONG				m_lBufOff;				// offset in when double buffering
		int					m_iDataLen;				// length of uncompressed data (set on first read in)
		int					m_iFileLen;				// length of compressed data
		void *			m_pBuf;						// data (NULL if ! allocated)
		DWORD				m_dwLastUsed;			// timeGetTime last used

							enum STAT { unloaded, loading, loaded, dead };
		char				m_iStat;

							enum { mono_11_8, mono_22_16, stereo_22_16 };
		char				m_iFmt;						// 8/16 s/mono 11/22

		char				m_iGroup;					// group it is in

		char				m_iComp;					// compression method (-1 == none)

		char				m_IDBackup;				// WAV to play instead if not in memory
																	// 0 == none, -1 == play when loaded

		char				m_bVoice;					// a voice
		char				m_bKillMe;				// kill sound

		CRawChannel *		m_pRc;				// channel on when doing 
};


/////////////////////////////////////////////////////////////////////////////
// CMusicPlayer

class CMusicPlayer
{
friend class CRawData;
public:
			CMusicPlayer ();
			~CMusicPlayer ();

						enum MUSIC_MODE { midi_only, mixed, wav_only };
			void		InitData (MUSIC_MODE iMode, int iGrp = -1);
			void		Open (int iMusicVol, int iSoundVol, MUSIC_MODE iMode, int iGrp = -1);
			void		Close ();
			BOOL		OnActivate (BOOL bActive);
			void		ShutUp ();			// shuts everything off
			void		SoundsOff ();		// sound effects (but not music) off

			BOOL		SoundsPlaying () const { return (m_hDig != NULL); }
			BOOL		MusicPlaying () const { return (m_hMdi != NULL); }

			BOOL		IsGroupLoaded (int iGrp);
			void		LoadGroup (int iGrp);
			void		UnloadGroup (int iGrp);
			void		UnloadCache (DWORD dwTime);

			// MIDI music (note - we use digital for all non-playing music too)
			void		StartMidiMusic ();
			void		StopMidiMusic ();

			// this is for digital music that is played on a MIDI system
			//   MIDI is shut down while this plays
			void		PlayExclusiveMusic (int iSound);						// by ID - 1 track
			void		EndExclusiveMusic ();		// called by StartMidiMusic

			// this is for digital music - ignored if playing MIDI
			// 	this gets exclusive use of the first channel
			void		PlayMusicGroup (int iGrpFrst, int iNumGrp);		// swap between tracks in group
			void		EndMusicGroup ();

			// these are for sound effects
			void		PlayForegroundSound (int iSound, int iPri, int iPan = 64, int iVol = 100);	// priority over any background sounds
			void		KillForegroundSound (int iSound);

			// clear all sounds (in our counts - is still playing), queue new sounds, update to new set
			void		ClrBackgroundSounds ();
			void		QueueBackgroundSound (int iSound, int iPri = BACKGROUND_PRI, int iPan = 64, int iVol = 100);
			void		UpdateBackgroundSounds ();

			void		IncBackgroundSound (int iSound, int iPan = 64, int iVol = 100);
			void		DecBackgroundSound (int iSound, int iPan = 64, int iVol = 100);

			void		FreeOldSounds ( int iSec  );

			int			GetMusicVolume () const;
			int			GetSoundVolume () const;
			void		SetMusicVolume (int iVol);
			void		SetSoundVolume (int iVol);
			void		SetMusicSoundVolume (int iMusVol, int iSfxVol);

			MUSIC_MODE	GetMode () const { return m_iMode; }
			BOOL		UseDirectSound () const { return m_bUseDS; }
			BOOL		IsRunning () const { return m_bRunning; }
			BOOL		MidiOk () const { return ! m_bNoMidi; }
			BOOL		WavOk () const { return ! m_bNoWav; }
			char const * GetVersion () { return m_sVer; }

			void		YieldPlayer () const;
			HMDIDRIVER	_GetHMidi () { return m_hMdi; }
			HDIGDRIVER	_GetHDig () { return m_hDig; }

protected:
			void		LoadRaw (CMmio * pMmio);
			void		LoadMidi (CMmio * pMmio);

			void		OpenMidi (int iVol);
			void		OpenDigital (int iVol, BOOL bMidi);
			void		CheckDigVol ();
			BOOL		StartRaw (CRawChannel * pRawChannel, CRawData * pRawData);

			void		ToExclMusic ();								// used to convert to/from music mode when have MIDI
			void		FromExclMusic ();

			BOOL		IsWavMusic ();

	static void WINAPI MidiCallback (HSEQUENCE hSeq);
	static void WINAPI RawCallback (HSAMPLE hSmp);
	static void WINAPI RawDblBufCallback (HSAMPLE hSmp);
	static void CacheCallback ( DWORD dwData );

public:
			CADPCMtoPCMConvert *		m_pAdpcmSfx;
			CADPCMtoPCMConvert *		m_pAdpcmMusic;
			char *									m_pBufDecode;
			int											m_iInBuf;
protected:

			CString			m_sVer;
			MUSIC_MODE	m_iMode;													// what we use for music
			BOOL				m_bNoMidi;												// MIDI doesn't work
			BOOL				m_bNoWav;													// WAV doesn't work
			BOOL				m_bRunning;												// we're running (app is active)
			BOOL				m_bExclWav;												// WAV music playing (instead of MIDI)
			BOOL				m_bKillMidi;											// we are stopping MIDI (for callback)
			BOOL				m_bUseDS;													// true if uses DirectSound

			int					m_iMusicVol;											// volume
			int					m_iSfxVol;

			int					m_iMusicGrpFirst;									// ID of first WAV in group we are playing
			int					m_iMusicGrpNum;										// number of WAVs (sequentially in array)

			HMDIDRIVER	m_hMdi;
			HDIGDRIVER	m_hDig;

			// WAV data
			CFile *			m_pFileReg;												// non-lang for reading cached & buffered data
			CFile *			m_pFileVoc;												// voices for reading cached & buffered data
			CRawChannel	m_Channel [MAX_SOUND_SAMPLES];		// channels we are playing on
			CArray <CRawData, CRawData>	m_aRaw;						// array of classes holding RAW data

			// MIDI data
			HSEQUENCE		m_hSeq;														// sequence we are playing
			int					m_iMidiTrk;												// track we are playing

			CArray <void *, void *>	m_aMidi;							// array of pointers to xmidi data
};


void ConstructElements (CRawData * pNew, int nCount);
void DestructElements (CRawData * pNew, int nCount);

extern CMusicPlayer theMusicPlayer;


#endif
