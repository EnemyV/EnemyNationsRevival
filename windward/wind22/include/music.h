#ifndef __MUSIC_H__
#define __MUSIC_H__

#include "acmutil.h"

#include <SDL.h>
#include <SDL_mixer.h>

//---------------------------------------------------------------------------
//
//  Copyright (c) 1995, 1996. Windward Studios, Inc.
//  All Rights Reserved.
//
//  Audio system using SDL2 + SDL_mixer.
//  Handles digital audio music, SFX, and voice playback.
//  Voice is just another sfx and all sfx & voice are stored as mono.
//
//  All audio is opened at 22050 Hz, 16-bit signed, stereo.
//  Sounds in other formats (8-bit 11025Hz mono) are converted at load time.
//
//  WAV files belong to a group. We can load and unload a group.
//  WAV files are marked either pre-load, cache, or buffered (music).
//
//---------------------------------------------------------------------------


class CMusicPlayer;
class CRawChannel;
class CRawData;


const int MAX_SOUND_SAMPLES = 6;
const int DBL_BUF_LEN       = 0x20000;  // 128K

// Audio format constants (replaces MSS32 DIG_F_* constants)
const int AUDIO_FMT_MONO_8    = 0;  // 8-bit unsigned mono 11025Hz
const int AUDIO_FMT_MONO_16   = 1;  // 16-bit signed mono 22050Hz
const int AUDIO_FMT_STEREO_8  = 2;  // 8-bit unsigned stereo 11025Hz (unused in practice)
const int AUDIO_FMT_STEREO_16 = 3;  // 16-bit signed stereo 22050Hz

// Device format we open SDL_mixer with
const int DEVICE_FREQ     = 22050;
const int DEVICE_CHANNELS = 2;
// AUDIO_S16SYS is used at Mix_OpenAudio time


/////////////////////////////////////////////////////////////////////////////
// CRawChannel - this is a channel we are mixing & playing

const int BACKGROUND_PRI = 400;
const int FOREGROUND_PRI = 50;

// Events set by SDL audio thread, consumed by game thread in YieldPlayer()
enum PENDING_EVENT
{
    PENDING_NONE,
    PENDING_FINISHED
};

class CRawChannel
{
  public:
    CRawChannel( );
    ~CRawChannel( ) { Close( ); }

    void Close( );
    BOOL AllocDblBuf( );
    void FreeDblBuf( );
    void BackgroundRead( HANDLE iHdl, int iOff, int iLen );

    int        m_iChannel;       // SDL_mixer channel index (0..MAX_SOUND_SAMPLES-1)
    Mix_Chunk* m_pChunk;         // currently loaded Mix_Chunk for this channel (we own the wrapper, not the data)
    volatile PENDING_EVENT m_pendingEvent;  // set by SDL audio thread, consumed by game thread

    BOOL    m_bKill;  // we are stopping (for callback)
    enum FORE
    {
        back,
        fore,
        excl,
        music
    };
    FORE m_iIsFore;          // 0 == background sfx, 1 == foreground sfx
                             // 2 == exclusive music, 3 == digital music (group)
    int m_iBackSound;        // background effect for this channel
    int m_iBackPri;          // background priority
    int m_iNum;              // number of requests
    int m_iVol;              // volume set to
    int m_iPan;
    int m_iForeSound;        // foreground sound playing
    int m_iForePri;          // foreground priority

    CRawData* m_pData;       // data we are playing
    void*     m_pDblBuf[3];  // double buffer for music

    // Streaming state (used by music channel only, channel 0)
    int           m_iPlayBuf;       // which buffer the hook is reading from (0..2)
    int           m_iPlayPos;       // byte offset within current play buffer
    int           m_iPlayLen[3];    // valid byte count per buffer
    volatile bool m_bBufConsumed;   // signals game thread to kick next read-ahead
    bool          m_bStreamEOF;     // no more data from disk
    volatile bool m_bStreamActive;  // hook is actively streaming

    CRITICAL_SECTION m_cs;
    BOOL             m_bCriticalSectionCreated;  // critical section created
    CWinThread*      m_pThread;     // double buffering read-ahead thread
    DWORD            m_iReadLen;  // how much the read ahead got (end of file)
    enum STAT
    {
        _unused,
        _idle,
        _reading,
        _read
    };
    STAT   m_cStat;
    HANDLE m_iHdl;         // file to read from
    int    m_iOff;         // offset to read from
    DWORD  m_iRead;        // how much to read
    BOOL   m_bRunning;     // thread should run
    int    m_iBufOn;       // 0..2

    CMusicPlayer* m_pPar;  // who holds us
};


/////////////////////////////////////////////////////////////////////////////
// CRawData - this is a single WAV file

class CRawData
{
  public:
    CRawData( );
    ~CRawData( ) { Close( ); }

    void Close( );

    void InitData( CMmio* pMmio, int iGrp = -1 );
    void LoadBuffer( CFile* pFile );
    void UnloadBuffer( );

    void StartDblBuffer( CRawChannel* pRaw );
    void LoadNextDblBuffer( CRawChannel* pRaw, int iNeed );

    enum TYPE
    {
        preload,
        cache,
        buffer
    };
    TYPE m_iType;        // if data is loaded, cached, or buffered as played

    LONG  m_lOff;        // offset in data file of WAV file
    LONG  m_lBufOff;     // offset in when double buffering
    int   m_iDataLen;    // length of data in device format (set on first read in)
    int   m_iFileLen;    // length of compressed data on disk
    void* m_pBuf;        // data in device format (NULL if ! allocated)
    DWORD m_dwLastUsed;  // timeGetTime last used

    enum STAT
    {
        unloaded,
        loading,
        loaded,
        dead
    };
    char m_iStat;

    enum FORMAT
    {
        mono_11_8,
        mono_22_16,
        stereo_22_16
    };
    char m_iFmt;         // original format from data file

    char m_iGroup;       // group it is in

    char m_iComp;        // compression method (-1 == none)

    char m_IDBackup;     // WAV to play instead if not in memory
                         // 0 == none, -1 == play when loaded

    char m_bVoice;       // a voice
    char m_bKillMe;      // kill sound

    CRawChannel* m_pRc;  // channel on when doing
};


/////////////////////////////////////////////////////////////////////////////
// Format conversion: convert raw audio data to device format (22050Hz 16-bit stereo)

void ConvertToDeviceFormat( const void* pSrc, int iSrcLen, int iFmt,
                            void** ppDst, int* piDstLen );


/////////////////////////////////////////////////////////////////////////////
// CMusicPlayer

class CMusicPlayer
{
    friend class CRawData;

  public:
    CMusicPlayer( );
    ~CMusicPlayer( );

    enum class MUSIC_MODE
    {
        midi_only,
        mixed,
        wav_only
    };
    void InitData( MUSIC_MODE iMode, int iGrp = -1 );
    void Open( int iMusicVol, int iSoundVol, MUSIC_MODE iMode, int iGrp = -1 );
    void Close( );
    BOOL OnActivate( BOOL bActive );
    void ShutUp( );     // shuts everything off
    void SoundsOff( );  // sound effects (but not music) off

    BOOL SoundsPlaying( ) const { return m_bDigOpen; }
    BOOL MusicPlaying( ) const { return FALSE; }  // MIDI removed

    BOOL IsGroupLoaded( int iGrp );
    void LoadGroup( int iGrp );
    void UnloadGroup( int iGrp );

    // MIDI stubs (MIDI support removed; these are no-ops for API compatibility)
    void StartMidiMusic( );
    void StopMidiMusic( );

    // digital music - 1 track (exclusive)
    void PlayExclusiveMusic( int iSound );
    void EndExclusiveMusic( );

    // digital music - rotate through tracks in a group
    void PlayMusicGroup( int iGrpFrst, int iNumGrp );
    void EndMusicGroup( );

    // sound effects
    void PlayForegroundSound( int iSound, int iPri, int iPan = 64,
                              int iVol = 100 );
    void KillForegroundSound( int iSound );

    // background ambient sounds
    void ClrBackgroundSounds( );
    void QueueBackgroundSound( int iSound, int iPri = BACKGROUND_PRI, int iPan = 64, int iVol = 100 );
    void UpdateBackgroundSounds( );

    void IncBackgroundSound( int iSound, int iPan = 64, int iVol = 100 );
    void DecBackgroundSound( int iSound, int iPan = 64, int iVol = 100 );

    void FreeOldSounds( int iSec );

    int  GetMusicVolume( ) const;
    int  GetSoundVolume( ) const;
    void SetMusicVolume( int iVol );
    void SetSoundVolume( int iVol );

    MUSIC_MODE  GetMode( ) const { return m_iMode; }
    BOOL        UseDirectSound( ) const { return FALSE; }  // legacy; SDL manages backend
    BOOL        IsRunning( ) const { return m_bRunning; }
    BOOL        MidiOk( ) const { return FALSE; }  // MIDI removed
    BOOL        WavOk( ) const { return !m_bNoWav; }
    char const* GetVersion( ) { return m_sVer; }

    void YieldPlayer( );

    // Diagnostics: replaces _GetHDig() + AIL_digital_configuration()
    void GetDigitalConfig( int* pRate, int* pChannels, CString& sName );

  protected:
    void OpenDigital( int iVol );
    void CheckDigVol( );
    BOOL StartRaw( CRawChannel* pRawChannel, CRawData* pRawData );

    void HandleChannelFinished( CRawChannel* pCh );
    void ProcessMusicStreaming( );

    static void SDLCALL OnChannelFinished( int channel );
    static void SDLCALL MusicHookCallback( void* udata, Uint8* stream, int len );

    void HaltChannel( CRawChannel* pCh );   // stop channel and clean up immediately
    void FreeChunk( CRawChannel* pCh );      // free the Mix_Chunk wrapper if any

    void SetChannelVolume( CRawChannel* pCh );
    void SetChannelPan( CRawChannel* pCh );

    void StartServiceThread( );
    void StopServiceThread( );
    static UINT ServiceThreadProc( LPVOID pParam );

  public:
    CADPCMtoPCMConvert* m_pAdpcmSfx;
    CADPCMtoPCMConvert* m_pAdpcmMusic;
    char*               m_pBufDecode;
    int                 m_iInBuf;

  protected:
    CString    m_sVer;
    MUSIC_MODE m_iMode;      // always wav_only (kept for API compat)
    BOOL       m_bNoWav;     // WAV doesn't work
    BOOL       m_bRunning;   // we're running (app is active)
    bool       m_bDigOpen;   // SDL_mixer audio device is open

    int m_iMusicVol;         // volume 0-127
    int m_iSfxVol;           // volume 0-127

    int m_iMusicGrpFirst;  // ID of first WAV in group we are playing
    int m_iMusicGrpNum;    // number of WAVs (sequentially in array)

    // WAV data
    CFile*                     m_pFileReg;                    // non-lang for reading cached & buffered data
    CFile*                     m_pFileVoc;                    // voices for reading cached & buffered data
    CRawChannel                m_Channel[MAX_SOUND_SAMPLES];  // channels we are playing on
    CArray<CRawData, CRawData> m_aRaw;                        // array of classes holding RAW data

  public:
    // Service thread: processes deferred callbacks and streaming independently of game loop
    CRITICAL_SECTION m_csAudio;           // protects all audio state from concurrent access
    BOOL             m_bCsInitialized;    // critical section initialized

  protected:
    CWinThread*      m_pServiceThread;    // audio service thread
    volatile bool    m_bServiceRunning;   // service thread should keep running
};

// Channel map: maps SDL_mixer channel index -> CRawChannel* (set during Open)
extern CRawChannel* g_channelMap[MAX_SOUND_SAMPLES];

#pragma warning( disable : 4995 )
void ConstructElements( CRawData* pNew, int nCount );
#pragma warning( disable : 4995 )
void DestructElements( CRawData* pNew, int nCount );

extern CMusicPlayer theMusicPlayer;


#endif
