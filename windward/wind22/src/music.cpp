//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
// Audio system rewritten to use SDL2 + SDL_mixer, replacing Miles Sound System.
//
//---------------------------------------------------------------------------


#include "stdafx.h"

#include <thielen.h>
#include "_windwrd.h"
#include "w22_settings.h"
#include "_res.h"
#include "acmutil.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

CMusicPlayer theMusicPlayer;
CRawChannel* g_channelMap[MAX_SOUND_SAMPLES] = {};

// RAII lock guard for the audio critical section
class CAudioLock {
  public:
    CAudioLock( CMusicPlayer& player ) : m_player( player ), m_bLocked( FALSE ) {
        if ( player.m_bCsInitialized ) {
            EnterCriticalSection( &player.m_csAudio );
            m_bLocked = TRUE;
        }
    }
    ~CAudioLock() {
        if ( m_bLocked )
            LeaveCriticalSection( &m_player.m_csAudio );
    }
  private:
    CMusicPlayer& m_player;
    BOOL m_bLocked;
};


/////////////////////////////////////////////////////////////////////////////
// Format conversion: convert audio data to device format (22050Hz 16-bit signed stereo)

void ConvertToDeviceFormat( const void* pSrc, int iSrcLen, int iFmt,
                            void** ppDst, int* piDstLen ) {

    if ( iFmt == AUDIO_FMT_STEREO_16 ) {
        // Already in device format - just copy
        *piDstLen = iSrcLen;
        *ppDst = malloc( iSrcLen );
        if ( *ppDst != NULL )
            memcpy( *ppDst, pSrc, iSrcLen );
        return;
    }

    if ( iFmt == AUDIO_FMT_MONO_16 ) {
        // 16-bit signed mono 22050Hz -> 16-bit signed stereo 22050Hz (2x size)
        int iNumSamples = iSrcLen / 2;
        *piDstLen = iNumSamples * 4;  // 2 bytes * 2 channels
        *ppDst = malloc( *piDstLen );
        if ( *ppDst == NULL )
            return;

        const short* pIn = (const short*)pSrc;
        short* pOut = (short*)*ppDst;
        for ( int i = 0; i < iNumSamples; i++ ) {
            pOut[i * 2] = pIn[i];      // left
            pOut[i * 2 + 1] = pIn[i];  // right
        }
        return;
    }

    if ( iFmt == AUDIO_FMT_MONO_8 ) {
        // 8-bit unsigned mono 11025Hz -> 16-bit signed stereo 22050Hz (8x size)
        // Step 1: convert 8-bit unsigned to 16-bit signed
        // Step 2: upsample 11025->22050 by duplicating each sample
        // Step 3: mono to stereo by duplicating each sample
        int iNumSrcSamples = iSrcLen;
        int iNumDstSamples = iNumSrcSamples * 2;  // 2x for upsample
        *piDstLen = iNumDstSamples * 4;  // 2 bytes * 2 channels per sample
        *ppDst = malloc( *piDstLen );
        if ( *ppDst == NULL )
            return;

        const unsigned char* pIn = (const unsigned char*)pSrc;
        short* pOut = (short*)*ppDst;
        for ( int i = 0; i < iNumSrcSamples; i++ ) {
            short s16 = (short)( ( (int)pIn[i] - 128 ) << 8 );
            // Each source sample produces 2 output frames (upsample 2x)
            // Each frame has 2 channels (stereo)
            int base = i * 4;  // 4 shorts per source sample
            pOut[base]     = s16;  // frame 0 left
            pOut[base + 1] = s16;  // frame 0 right
            pOut[base + 2] = s16;  // frame 1 left
            pOut[base + 3] = s16;  // frame 1 right
        }
        return;
    }

    // AUDIO_FMT_STEREO_8 - not used in practice but handle defensively
    // 8-bit unsigned stereo 11025Hz -> 16-bit signed stereo 22050Hz (4x size)
    int iNumFrames = iSrcLen / 2;  // 2 bytes per frame (L+R)
    int iNumDstFrames = iNumFrames * 2;  // upsample 2x
    *piDstLen = iNumDstFrames * 4;  // 2 channels * 2 bytes per sample
    *ppDst = malloc( *piDstLen );
    if ( *ppDst == NULL )
        return;

    const unsigned char* pIn = (const unsigned char*)pSrc;
    short* pOut = (short*)*ppDst;
    for ( int i = 0; i < iNumFrames; i++ ) {
        short sL = (short)( ( (int)pIn[i * 2] - 128 ) << 8 );
        short sR = (short)( ( (int)pIn[i * 2 + 1] - 128 ) << 8 );
        int b = i * 4;  // 4 shorts per source frame (2 output frames * 2 channels)
        pOut[b]     = sL;  // frame 0 left
        pOut[b + 1] = sR;  // frame 0 right
        pOut[b + 2] = sL;  // frame 1 left
        pOut[b + 3] = sR;  // frame 1 right
    }
}


/////////////////////////////////////////////////////////////////////////////
// CRawChannel

UINT fnMusicReadAhead( LPVOID pParam ) {

    CRawChannel* pRawChn = (CRawChannel*)pParam;

    try {
        // read into the buffer
        while ( TRUE ) {
            // wait for next request
            pRawChn->m_pThread->SuspendThread();

            if ( !pRawChn->m_bRunning )
                break;

            ::SetFilePointer( pRawChn->m_iHdl, pRawChn->m_iOff, NULL, FILE_BEGIN );
            ::ReadFile( pRawChn->m_iHdl, pRawChn->m_pPar->m_pBufDecode + pRawChn->m_pPar->m_iInBuf, pRawChn->m_iRead, &( pRawChn->m_iReadLen ), NULL );
            if ( pRawChn->m_iRead != pRawChn->m_iReadLen )
                ThrowError( ERR_CACHE_READ );

            if ( pRawChn->m_pData->m_iComp == 9 ) {
                int iLen = pRawChn->m_iReadLen + theMusicPlayer.m_iInBuf;
                theMusicPlayer.m_pAdpcmMusic->Convert( theMusicPlayer.m_pBufDecode,
                                                       iLen, pRawChn->m_pDblBuf[pRawChn->m_iBufOn], DBL_BUF_LEN,
                                                       ACM_STREAMCONVERTF_BLOCKALIGN );
                if ( ( theMusicPlayer.m_iInBuf = iLen - theMusicPlayer.m_pAdpcmMusic->SrcSize() ) > 0 )
                    memcpy( theMusicPlayer.m_pBufDecode,
                            theMusicPlayer.m_pBufDecode + theMusicPlayer.m_pAdpcmMusic->SrcSize(),
                            theMusicPlayer.m_iInBuf );
                pRawChn->m_iReadLen = theMusicPlayer.m_pAdpcmMusic->ResultSize();
            }

            pRawChn->m_cStat = CRawChannel::_read;
        }
    }

    catch ( ... ) {
        if ( pRawChn->m_bCriticalSectionCreated )
            EnterCriticalSection( &( pRawChn->m_cs ) );
        pRawChn->m_cStat = CRawChannel::_unused;
        pRawChn->m_pThread->m_hThread = NULL;
        pRawChn->m_pThread = NULL;
        if ( pRawChn->m_bCriticalSectionCreated )
            LeaveCriticalSection( &( pRawChn->m_cs ) );
        return ( 0 );
    }

    if ( pRawChn->m_bCriticalSectionCreated )
        EnterCriticalSection( &( pRawChn->m_cs ) );
    pRawChn->m_cStat = CRawChannel::_unused;
    pRawChn->m_pThread->m_hThread = NULL;
    pRawChn->m_pThread            = NULL;
    if ( pRawChn->m_bCriticalSectionCreated )
        LeaveCriticalSection( &( pRawChn->m_cs ) );

    return ( 0 );
}

void CRawChannel::BackgroundRead( HANDLE iHdl, int iOff, int iLen ) {

    // special case
    if ( iLen <= 0 ) {
        m_iReadLen = 0;
        m_cStat = _read;
        return;
    }

    for ( int iWait = 0; ( m_cStat == _reading ) && ( iWait < 10 ); iWait++ )
        ::Sleep( iWait * 8 );
    // if locked up - just give up
    if ( m_cStat == _reading ) {
        TRAP();
        return;
    }

    m_cStat = _reading;
    m_iHdl = iHdl;
    m_iOff = iOff;
    m_iRead = iLen;

    m_iBufOn++;
    if ( m_iBufOn > 2 )
        m_iBufOn = 0;

    // start it
    m_pThread->ResumeThread();
}

CRawChannel::CRawChannel() {

    m_bKill = FALSE;
    m_iChannel = -1;
    m_pChunk = NULL;
    m_pendingEvent = PENDING_NONE;
    m_iIsFore = back;
    m_iBackSound = 0;
    m_iBackPri = INT_MAX;
    m_iNum = 0;
    m_iVol = 0;
    m_iPan = 64;
    m_pData = NULL;
    m_pDblBuf[0] = m_pDblBuf[1] = m_pDblBuf[2] = NULL;
    m_pThread = NULL;
    m_cStat = _unused;
    m_iBufOn = 0;

    // Streaming state
    m_iPlayBuf = 0;
    m_iPlayPos = 0;
    m_iPlayLen[0] = m_iPlayLen[1] = m_iPlayLen[2] = 0;
    m_bBufConsumed = false;
    m_bStreamEOF = false;
    m_bStreamActive = false;

    memset( &m_cs, 0, sizeof( m_cs ) );
    m_bCriticalSectionCreated = FALSE;
}

void CRawChannel::Close() {

    FreeDblBuf();
}

BOOL CRawChannel::AllocDblBuf() {

    // get our buffer (always 3 buffers, Win32s check removed)
    for ( int iInd = 0; iInd < 3; iInd++ )
        if ( m_pDblBuf[iInd] == NULL )
            if ( ( m_pDblBuf[iInd] = malloc( DBL_BUF_LEN ) ) == NULL ) {
                TRAP();
                FreeDblBuf();
                return ( FALSE );
            }

    // get a thread for reading ahead
    if ( m_pPar != NULL )
        if ( w22::GetProfileInt( "Advanced", "MusicThread", 1 ) ) {
            m_iBufOn = 0;
            m_cStat = _idle;
            m_bRunning = TRUE;

            memset( &m_cs, 0, sizeof( m_cs ) );
            InitializeCriticalSection( &m_cs );
            m_bCriticalSectionCreated = TRUE;

            m_pThread = AfxBeginThread( fnMusicReadAhead, this, THREAD_PRIORITY_HIGHEST,
                                        0, CREATE_SUSPENDED );
            m_pThread->ResumeThread();
        }

    return ( TRUE );
}

void CRawChannel::FreeDblBuf() {

    // Stop the music hook and lock SDL audio to ensure the audio callback
    // is not mid-execution reading from m_pDblBuf before we free them.
    bool bNeedUnlock = false;
    if ( m_bStreamActive ) {
        Mix_HookMusic( NULL, NULL );
        m_bStreamActive = false;
    }
    // Only lock SDL audio if the device is still open (SDL_LockAudio after Mix_CloseAudio is undefined)
    if ( SDL_WasInit( SDL_INIT_AUDIO ) ) {
        SDL_LockAudio();
        bNeedUnlock = true;
    }

    if ( ( m_pThread != NULL ) && m_bCriticalSectionCreated ) {
        EnterCriticalSection( &m_cs );
        if ( ( m_pThread != NULL ) && AfxIsValidAddress( m_pThread, sizeof( CWinThread ) ) ) {
            m_bRunning = FALSE;
            m_pThread->ResumeThread();
            for ( int iWait = 0; ( m_pThread->m_hThread != NULL ) && ( iWait < 10 ); iWait++ ) {
                LeaveCriticalSection( &m_cs );
                ::Sleep( iWait * 8 );
                EnterCriticalSection( &m_cs );
                if ( !AfxIsValidAddress( m_pThread, sizeof( CWinThread ) ) )
                    break;
            }

            if ( AfxIsValidAddress( m_pThread, sizeof( CWinThread ) ) )
                if ( m_pThread->m_hThread != NULL )
                    TerminateThread( m_pThread->m_hThread, 1 );
        }
        m_pThread = NULL;
        LeaveCriticalSection( &m_cs );
    }

    for ( int iInd = 0; iInd < 3; iInd++ )
        if ( m_pDblBuf[iInd] != NULL ) {
            free( m_pDblBuf[iInd] );
            m_pDblBuf[iInd] = NULL;
        }

    if ( bNeedUnlock )
        SDL_UnlockAudio();

    if ( m_bCriticalSectionCreated ) {
        DeleteCriticalSection( &m_cs );
        m_bCriticalSectionCreated = FALSE;
    }

    m_bStreamActive = false;
}


/////////////////////////////////////////////////////////////////////////////
// CRawData

CRawData::CRawData() {

    m_iGroup = 0;
    m_iType = cache;
    m_iComp = -1;
    m_IDBackup = 0;
    m_pBuf = NULL;
    m_lBufOff = 0;
    m_iFmt = AUDIO_FMT_MONO_8;
    m_bVoice = FALSE;
    m_bKillMe = FALSE;
    m_iStat = unloaded;
    m_pRc = NULL;
}

void CRawData::Close() {

    if ( m_pBuf != NULL ) {
        free( m_pBuf );
        m_pBuf = NULL;
    }
}

void ConstructElements( CRawData* pNew, int nCount ) {

    for ( int i = 0; i < nCount; i++, pNew++ )
#ifdef _WIN32
        pNew->CRawData::CRawData();      // MSVC: explicit ctor re-init
#else
        new ( pNew ) CRawData();         // portable placement-new equivalent
#endif
}

void DestructElements( CRawData* pNew, int nCount ) {

    for ( int i = 0; i < nCount; i++, pNew++ )
        pNew->CRawData::~CRawData();
}

// reads in the data (and buffer if preload) of a wav file
void CRawData::InitData( CMmio* pMmio, int iGrp ) {

    pMmio->DescendChunk( 'D', 'A', 'T', 'A' );

    try {
        m_iGroup = pMmio->ReadShort();
        m_iType = (CRawData::TYPE)pMmio->ReadShort();
        m_iComp = pMmio->ReadShort();
        m_IDBackup = pMmio->ReadShort();
        short sTyp = pMmio->ReadShort();
        switch ( sTyp ) {
        case mono_22_16:
            m_iFmt = AUDIO_FMT_MONO_16;
            break;
        case stereo_22_16:
            m_iFmt = AUDIO_FMT_STEREO_16;
            break;
        default:
            m_iFmt = AUDIO_FMT_MONO_8;
            break;
        }
        m_iDataLen = m_iFileLen = pMmio->ReadLong();
        m_lOff = pMmio->GetOffset();

        if ( ( m_iType == preload ) && ( m_iGroup == iGrp ) ) {
            switch ( m_iComp ) {
            case -1:
            {
                void* pRaw = malloc( m_iDataLen + 16 );
                if ( pRaw != NULL ) {
                    pMmio->Read( pRaw, m_iDataLen );
                    // Convert to device format
                    if ( m_iFmt != AUDIO_FMT_STEREO_16 ) {
                        void* pConverted = NULL;
                        int iConvLen = 0;
                        ConvertToDeviceFormat( pRaw, m_iDataLen, m_iFmt, &pConverted, &iConvLen );
                        free( pRaw );
                        m_pBuf = pConverted;
                        m_iDataLen = iConvLen;
                    } else {
                        m_pBuf = pRaw;
                    }
                }
                break;
            }

            case 8:
            {
                TRAP();
                void* pBuf = malloc( m_iFileLen );
                if ( pBuf != NULL ) {
                    pMmio->Read( pBuf, m_iFileLen );

                    TRAP();
                    int iRawLen = CoDec::BufLength( pBuf );
                    void* pRaw = malloc( iRawLen + 16 );
                    if ( pRaw == NULL )
                        ThrowError( ERR_OUT_OF_MEMORY );
                    CoDec::Decompress( pBuf, m_iFileLen, pRaw, iRawLen );

                    // Convert to device format
                    if ( m_iFmt != AUDIO_FMT_STEREO_16 ) {
                        void* pConverted = NULL;
                        int iConvLen = 0;
                        ConvertToDeviceFormat( pRaw, iRawLen, m_iFmt, &pConverted, &iConvLen );
                        free( pRaw );
                        m_pBuf = pConverted;
                        m_iDataLen = iConvLen;
                    } else {
                        m_pBuf = pRaw;
                        m_iDataLen = iRawLen;
                    }

                    free( pBuf );
                }
                break;
            }

            case 9:
            {
                TRAP();
                void* pBuf = malloc( m_iFileLen );
                if ( pBuf != NULL ) {
                    pMmio->Read( pBuf, m_iFileLen );
                    TRAP();

                    // ADPCM decompression
                    theMusicPlayer.m_pAdpcmSfx->Convert( pBuf, m_iFileLen );
                    void* pRaw = theMusicPlayer.m_pAdpcmSfx->ResultData();
                    int iRawLen = theMusicPlayer.m_pAdpcmSfx->ResultSize();
                    theMusicPlayer.m_pAdpcmSfx->ReleaseBuffer();

                    // Convert to device format
                    if ( m_iFmt != AUDIO_FMT_STEREO_16 ) {
                        void* pConverted = NULL;
                        int iConvLen = 0;
                        ConvertToDeviceFormat( pRaw, iRawLen, m_iFmt, &pConverted, &iConvLen );
                        // pRaw was allocated by ADPCM with MEM_alloc_lock (now malloc via acmutil)
                        // but ReleaseBuffer detached ownership, so we must free it
                        free( pRaw );
                        m_pBuf = pConverted;
                        m_iDataLen = iConvLen;
                    } else {
                        m_pBuf = pRaw;
                        m_iDataLen = iRawLen;
                    }

                    free( pBuf );
                }
                break;
            }
            }
        }
    }

    catch ( ... ) {
        m_pBuf = NULL;
        m_iType = cache;
        m_iStat = dead;
    }

    pMmio->AscendChunk();
}

void CRawData::LoadBuffer( CFile* pFile ) {

    // if loaded we're done
    if ( m_iStat == loaded )
        return;

    // Same class of bug as StartDblBuffer (see its comment): the caller resolves
    // pFile as m_bVoice ? m_pFileVoc : m_pFileReg (CMusicPlayer::LoadGroup), and
    // either can be NULL when CMusicPlayer::InitData's SFX/voice/music load
    // partially failed (e.g. a demo-layout .dat). All three switch cases below
    // deref pFile unconditionally, and a NULL deref is a hard crash that skips
    // the catch(...) below meant to make this non-fatal — reproduced via `newgame`
    // (CreateNewWorld -> LoadGroup -> here), SIGSEGV at pFile->Seek. Treat it the
    // same way the exception path already does: dead, not loaded.
    if ( pFile == NULL ) {
        m_pBuf = NULL;
        m_iType = cache;
        m_iStat = dead;
        return;
    }

    try {
        switch ( m_iComp ) {
        case -1:
        {
            void* pRaw = malloc( m_iFileLen + 16 );
            if ( pRaw == NULL )
                ThrowError( ERR_OUT_OF_MEMORY );
            pFile->Seek( m_lOff, CFile::begin );
            pFile->Read( pRaw, m_iFileLen );

            // Convert to device format
            if ( m_iFmt != AUDIO_FMT_STEREO_16 ) {
                void* pConverted = NULL;
                int iConvLen = 0;
                ConvertToDeviceFormat( pRaw, m_iFileLen, m_iFmt, &pConverted, &iConvLen );
                free( pRaw );
                m_pBuf = pConverted;
                m_iDataLen = iConvLen;
            } else {
                m_pBuf = pRaw;
                m_iDataLen = m_iFileLen;
            }
            break;
        }

        case 8:
        {
            void* pBuf = malloc( m_iFileLen );
            if ( pBuf != NULL ) {
                pFile->Seek( m_lOff, CFile::begin );
                pFile->Read( pBuf, m_iFileLen );

                int iRawLen = CoDec::BufLength( pBuf );
                void* pRaw = malloc( iRawLen + 16 );
                if ( pRaw == NULL )
                    ThrowError( ERR_OUT_OF_MEMORY );
                CoDec::Decompress( pBuf, m_iFileLen, pRaw, iRawLen );

                if ( m_iFmt != AUDIO_FMT_STEREO_16 ) {
                    void* pConverted = NULL;
                    int iConvLen = 0;
                    ConvertToDeviceFormat( pRaw, iRawLen, m_iFmt, &pConverted, &iConvLen );
                    free( pRaw );
                    m_pBuf = pConverted;
                    m_iDataLen = iConvLen;
                } else {
                    m_pBuf = pRaw;
                    m_iDataLen = iRawLen;
                }

                free( pBuf );
            }
            break;
        }

        case 9:
        {
            void* pBuf = malloc( m_iFileLen );
            if ( pBuf != NULL ) {
                pFile->Seek( m_lOff, CFile::begin );
                pFile->Read( pBuf, m_iFileLen );

                theMusicPlayer.m_pAdpcmSfx->Convert( pBuf, m_iFileLen );
                void* pRaw = theMusicPlayer.m_pAdpcmSfx->ResultData();
                int iRawLen = theMusicPlayer.m_pAdpcmSfx->ResultSize();
                theMusicPlayer.m_pAdpcmSfx->ReleaseBuffer();

                if ( m_iFmt != AUDIO_FMT_STEREO_16 ) {
                    void* pConverted = NULL;
                    int iConvLen = 0;
                    ConvertToDeviceFormat( pRaw, iRawLen, m_iFmt, &pConverted, &iConvLen );
                    free( pRaw );
                    m_pBuf = pConverted;
                    m_iDataLen = iConvLen;
                } else {
                    m_pBuf = pRaw;
                    m_iDataLen = iRawLen;
                }

                free( pBuf );
            }
            break;
        }
        }

        if ( m_pBuf != NULL )
            m_iStat = loaded;
        else
            m_iStat = dead;
    }

    catch ( ... ) {
        m_pBuf = NULL;
        m_iType = cache;
        m_iStat = dead;
    }
}

void CRawData::UnloadBuffer() {

    // make sure not in use
    if ( m_pRc != NULL )
        if ( m_pRc->m_pData == this ) {
            TRAP();
            return;
        }

    m_iStat = unloaded;
    if ( m_pBuf != NULL ) {
        free( m_pBuf );
        m_pBuf = NULL;
    }
}

void CRawData::StartDblBuffer( CRawChannel* pRaw ) {

    try {
        int iLen;
        if ( DBL_BUF_LEN / 4 <= m_iDataLen )
            iLen = DBL_BUF_LEN / 4;
        else {
            TRAP();
            iLen = m_iDataLen;
        }

        // CMusicPlayer::InitData's SFX/voice/music read is one big try/catch (by
        // design — a demo-layout .dat legitimately lacks 16-bit SFX/voice data,
        // and that's meant to be non-fatal, "continuing without full audio"). But
        // an exception partway through leaves m_pFileVoc/m_pFileReg NULL for
        // entries beyond whatever loaded before the throw, while m_aRaw is
        // already sized big enough that PlayExclusiveMusic's bounds check
        // (m_aRaw.GetSize() <= iSound) passes anyway. A NULL HANDLE deref is a
        // hard crash, not a C++ exception, so it skipped the catch below entirely
        // (segfault in CConquerApp::InitInstance on startup). Turn it into the
        // catchable failure this function already expects.
        CFile* pFile = m_bVoice ? pRaw->m_pPar->m_pFileVoc : pRaw->m_pPar->m_pFileReg;
        if ( pFile == NULL )
            ThrowError( ERR_CACHE_READ );

        // read it
        ::SetFilePointer( (HANDLE)( pFile->m_hFile ), m_lOff, NULL, FILE_BEGIN );
        m_lBufOff = iLen;
        DWORD dwRead;
        ::ReadFile( (HANDLE)( pFile->m_hFile ), pRaw->m_pPar->m_pBufDecode,
                    iLen, &dwRead, NULL );
        if ( dwRead != (DWORD)iLen )
            ThrowError( ERR_CACHE_READ );

        int iDecLen;
        if ( m_iComp == 9 ) {
            theMusicPlayer.m_pAdpcmMusic->Convert( theMusicPlayer.m_pBufDecode, iLen,
                                                   pRaw->m_pDblBuf[0], DBL_BUF_LEN,
                                                   ACM_STREAMCONVERTF_BLOCKALIGN | ACM_STREAMCONVERTF_START );
            iDecLen = theMusicPlayer.m_pAdpcmMusic->ResultSize();
            if ( ( theMusicPlayer.m_iInBuf = iLen - theMusicPlayer.m_pAdpcmMusic->SrcSize() ) > 0 )
                memcpy( theMusicPlayer.m_pBufDecode,
                        theMusicPlayer.m_pBufDecode + theMusicPlayer.m_pAdpcmMusic->SrcSize(),
                        theMusicPlayer.m_iInBuf );
        } else
            iDecLen = iLen;

        // start background thread on next buffer
        if ( pRaw->m_pThread != nullptr ) {
            int iNextLen = DBL_BUF_LEN / 4 - pRaw->m_pPar->m_iInBuf;
            iNextLen = __min( iNextLen, m_iDataLen - m_lBufOff );
            pRaw->m_iBufOn = 0;
            pRaw->BackgroundRead( pFile->m_hFile, m_lOff + m_lBufOff, iNextLen );
            m_lBufOff += iNextLen;
            if ( iNextLen <= 0 )
                pRaw->m_bStreamEOF = true;
        }

        // Set up streaming state
        iDecLen = __min( iDecLen, DBL_BUF_LEN );
        pRaw->m_iPlayBuf = 0;
        pRaw->m_iPlayPos = 0;
        pRaw->m_iPlayLen[0] = iDecLen;
        pRaw->m_iPlayLen[1] = 0;
        pRaw->m_iPlayLen[2] = 0;
        pRaw->m_bBufConsumed = false;
        pRaw->m_bStreamActive = true;
    }

    catch ( ... ) {
        pRaw->m_bStreamActive = false;
    }
}

void CRawData::LoadNextDblBuffer( CRawChannel* pRaw, int iNeed ) {

    try {
        // Same NULL-file class of bug as StartDblBuffer/LoadBuffer (see their
        // comments): m_pFileVoc/m_pFileReg can be NULL when CMusicPlayer::InitData's
        // SFX/voice/music load partially failed. Both branches below (background
        // and synchronous read) deref whichever one m_bVoice selects, unguarded.
        // Resolve once and throw so the catch(...) below handles it, matching the
        // graceful path this function already has for every other failure.
        CFile* pFile = m_bVoice ? pRaw->m_pPar->m_pFileVoc : pRaw->m_pPar->m_pFileReg;
        if ( pFile == NULL )
            ThrowError( ERR_CACHE_READ );

        int iLen = DBL_BUF_LEN / 4 - pRaw->m_pPar->m_iInBuf;
        iLen = __min( iLen, m_iDataLen - m_lBufOff );
        if ( ( iLen == 0 ) && ( pRaw->m_pPar->m_iInBuf == 0 ) ) {
            // End of track
            pRaw->m_bStreamActive = false;
            return;
        }

        // have a background thread - it's read it
        if ( pRaw->m_pThread != NULL ) {
            int iDelay = 10;
            while ( pRaw->m_cStat == CRawChannel::_reading ) {
                ::Sleep( iDelay );
                iDelay += iDelay / 2;

                if ( pRaw->m_pThread == NULL ) {
                    TRAP();
                    ThrowError( ERR_CACHE_READ );
                }
            }

            int iLastBuf = pRaw->m_iBufOn;
            int iLastLen = pRaw->m_iReadLen;

            if ( iLastLen == 0 ) {
                TRAP();
                pRaw->m_bStreamActive = false;
                return;
            }

            // start the next buffer
            if ( iLen > 0 ) {
                pRaw->BackgroundRead( pFile->m_hFile, m_lBufOff + m_lOff, iLen );
                m_lBufOff += iLen;
            } else {
                if ( m_iComp == 9 ) {
                    pRaw->m_iBufOn++;
                    if ( pRaw->m_iBufOn > 2 )
                        pRaw->m_iBufOn = 0;
                    theMusicPlayer.m_pAdpcmMusic->Convert( theMusicPlayer.m_pBufDecode,
                                                           theMusicPlayer.m_iInBuf, pRaw->m_pDblBuf[pRaw->m_iBufOn],
                                                           DBL_BUF_LEN, ACM_STREAMCONVERTF_BLOCKALIGN );
                    if ( ( theMusicPlayer.m_iInBuf -= theMusicPlayer.m_pAdpcmMusic->SrcSize() ) > 0 )
                        memcpy( theMusicPlayer.m_pBufDecode,
                                theMusicPlayer.m_pBufDecode + theMusicPlayer.m_pAdpcmMusic->SrcSize(),
                                theMusicPlayer.m_iInBuf );
                    pRaw->m_iReadLen = theMusicPlayer.m_pAdpcmMusic->ResultSize();
                    pRaw->m_cStat = CRawChannel::_read;
                    theMusicPlayer.m_iInBuf = 0;
                }
                pRaw->m_bStreamEOF = true;
            }

            // store the buffer length
            iLastLen = __min( iLastLen, DBL_BUF_LEN );
            pRaw->m_iPlayLen[iLastBuf] = iLastLen;
            return;
        }

        // no background thread - read synchronously
        if ( iLen > 0 ) {
            ::SetFilePointer( (HANDLE)( pFile->m_hFile ), m_lBufOff + m_lOff, NULL, FILE_BEGIN );
            m_lBufOff += iLen;
            DWORD dwRead;
            ::ReadFile( (HANDLE)( pFile->m_hFile ),
                        pRaw->m_pPar->m_pBufDecode + pRaw->m_pPar->m_iInBuf,
                        iLen, &dwRead, NULL );
            if ( dwRead != (DWORD)iLen )
                ThrowError( ERR_CACHE_READ );
        }

        int iDecLen;
        if ( m_iComp == 9 ) {
            iLen += theMusicPlayer.m_iInBuf;
            DWORD dwFlags = iLen >= DBL_BUF_LEN / 4 ? ACM_STREAMCONVERTF_BLOCKALIGN : ACM_STREAMCONVERTF_END;
            theMusicPlayer.m_pAdpcmMusic->Convert( theMusicPlayer.m_pBufDecode, iLen,
                                                   pRaw->m_pDblBuf[iNeed], DBL_BUF_LEN, dwFlags );
            if ( ( iDecLen = theMusicPlayer.m_pAdpcmMusic->ResultSize() ) == 0 ) {
                TRAP();
                pRaw->m_bStreamActive = false;
                return;
            }
            if ( ( theMusicPlayer.m_iInBuf = iLen - theMusicPlayer.m_pAdpcmMusic->SrcSize() ) > 0 )
                memcpy( theMusicPlayer.m_pBufDecode,
                        theMusicPlayer.m_pBufDecode + theMusicPlayer.m_pAdpcmMusic->SrcSize(),
                        theMusicPlayer.m_iInBuf );
        } else
            iDecLen = iLen;

        iDecLen = __min( iDecLen, DBL_BUF_LEN );
        pRaw->m_iPlayLen[iNeed] = iDecLen;
    }

    catch ( ... ) {
        pRaw->m_bStreamActive = false;
    }
}


/////////////////////////////////////////////////////////////////////////////
// CMusicPlayer

CMusicPlayer::CMusicPlayer() {

    m_pAdpcmSfx = NULL;
    m_pAdpcmMusic = NULL;
    m_pBufDecode = NULL;

    m_bNoWav = FALSE;
    m_bRunning = FALSE;
    m_bDigOpen = false;
    m_iMode = CMusicPlayer::MUSIC_MODE::wav_only;

    m_iMusicVol = 0;
    m_iSfxVol = 0;
    m_iMusicGrpFirst = 0;
    m_iMusicGrpNum = 0;

    m_pFileReg = NULL;
    m_pFileVoc = NULL;

    m_iInBuf = 0;

    m_pServiceThread = NULL;
    m_bServiceRunning = false;
    m_bCsInitialized = FALSE;
    memset( &m_csAudio, 0, sizeof( m_csAudio ) );
    InitializeCriticalSection( &m_csAudio );
    m_bCsInitialized = TRUE;

    for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++ )
        m_Channel[iOn].m_pPar = this;

    // Get SDL_mixer version
    const SDL_version* pVer = Mix_Linked_Version();
    if ( pVer != NULL )
        m_sVer.Format( "SDL_mixer %d.%d.%d", pVer->major, pVer->minor, pVer->patch );
    else
        m_sVer = "SDL_mixer";
}

CMusicPlayer::~CMusicPlayer() {

    try {
        Close();

        if ( m_pBufDecode != NULL )
            free( m_pBufDecode );
        delete m_pAdpcmMusic;
        delete m_pAdpcmSfx;

        SDL_QuitSubSystem( SDL_INIT_AUDIO );
    } catch ( ... ) {
        TRAP();
    }

    if ( m_bCsInitialized ) {
        DeleteCriticalSection( &m_csAudio );
        m_bCsInitialized = FALSE;
    }
}

void CMusicPlayer::Open( int iMusicVol, int iSoundVol, MUSIC_MODE iMode, int iGrp ) {

    ASSERT( m_bRunning == FALSE );
    ASSERT( ( 0 <= iMusicVol ) && ( iMusicVol <= 100 ) );
    ASSERT( ( 0 <= iSoundVol ) && ( iSoundVol <= 100 ) );

    // Force wav_only mode (MIDI removed)
    m_iMode = CMusicPlayer::MUSIC_MODE::wav_only;

    m_iMusicVol = ( iMusicVol * 127 ) / 100;
    m_iSfxVol = ( iSoundVol * 127 ) / 100;

    w22::WriteProfileInt( "Advanced", "MusicModeUsed", static_cast<int>( m_iMode ) );

    // if this fails we just don't have sound
    try {
        // if nothing playing - don't open
        if ( ( m_iSfxVol == 0 ) && ( m_iMusicVol == 0 ) )
            return;

        // Initialize SDL audio subsystem
        if ( SDL_InitSubSystem( SDL_INIT_AUDIO ) < 0 ) {
            TRACE( "SDL_InitSubSystem(AUDIO) failed: %s\n", SDL_GetError() );
            m_bNoWav = TRUE;
            return;
        }

        // Open the wave device
        int iVol = __max( m_iMusicVol, m_iSfxVol );
        if ( iVol > 0 )
            OpenDigital( iVol );

        m_bRunning = TRUE;

        // Start the audio service thread (processes callbacks/streaming independently of game loop)
        StartServiceThread();
    }

    catch ( ... ) {
        m_bNoWav = TRUE;
        try {
            CDlgMsg dlg;
            dlg.MsgBox( IDS_ERROR_AUDIO, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NoSoundCard" );
            Close();
        } catch ( ... ) {
        }
        m_bRunning = FALSE;
    }
}

void CMusicPlayer::InitData( MUSIC_MODE iMode, int iGrp ) {

    // Force wav_only
    iMode = CMusicPlayer::MUSIC_MODE::wav_only;

    // if 16-bit, see if we can open the CODEC
    try {
        m_pAdpcmSfx = new CADPCMtoPCMConvert( 1, 16, 22050 );
        m_pAdpcmMusic = new CADPCMtoPCMConvert( 2, 16, 22050 );
        m_pBufDecode = (char*)malloc( DBL_BUF_LEN / 4 );
    } catch ( ... ) {
        CDlgMsg dlg;
        dlg.MsgBox( IDS_NO_ADPCM, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NoADPCM" );
    }

    m_iMode = iMode;

    CMmio* pMmio = theDataFile.OpenAsMMIO( "music", "MUSC" );
    CString sMusicFile = pMmio->GetFileName();
    if ( sMusicFile.IsEmpty() )
        sMusicFile = "MUSIC";
    pMmio->DescendRiff( 'M', 'U', 'S', 'C' );

    // Skip MIDI data (we don't use it anymore)
    // But we still need to read past it if present in the file
    try {
        pMmio->DescendList( 'M', 'I', 'D', 'I' );
        // Read and discard MIDI tracks
        pMmio->DescendChunk( 'N', 'U', 'M', 'M' );
        int iNumMidi = pMmio->ReadShort();
        pMmio->AscendChunk();
        for ( int iOn = 0; iOn < iNumMidi; iOn++ ) {
            pMmio->DescendChunk( 'D', 'A', 'T', 'A' );
            pMmio->AscendChunk();  // skip the data
        }
        pMmio->AscendList();
    } catch ( ... ) {
        // MIDI section may not exist - that's fine
    }

    // read the sfx (always use '2' = 16-bit format since we're wav_only).
    // Wrapped in try/catch (like the MIDI section above): the demo data ships
    // 8-bit SFX1 and no voice/LANG layout, so the retail 16-bit reads here can
    // legitimately fail — missing/incompatible sound is non-fatal, the game just
    // runs without (some of) the audio.
    try {
    char bTyp = '2';
    pMmio->DescendList( 'S', 'F', 'X', bTyp );
    pMmio->DescendChunk( 'N', 'U', 'M', 'S' );
    m_aRaw.SetSize( pMmio->ReadShort() );
    pMmio->AscendChunk();

    // ok, read the header info for each
    for ( int iOn = 0; iOn < m_aRaw.GetSize(); iOn++ )
        m_aRaw[iOn].InitData( pMmio, iGrp );
    pMmio->AscendList();

    // read the voices - this is in the LANGUAGE part
    CMmio* pMmioVoices = theDataFile.OpenAsMMIO( NULL, "LANG" );
    CString sVoiceFile = pMmioVoices->GetFileName();

    pMmioVoices->DescendRiff( 'L', 'A', 'N', 'G' );
    pMmioVoices->DescendList( 'S', 'F', 'X', bTyp );
    pMmioVoices->DescendChunk( 'N', 'U', 'M', 'S' );
    int iBase = m_aRaw.GetSize();
    m_aRaw.SetSize( iBase + pMmioVoices->ReadShort() );
    pMmioVoices->AscendChunk();

    // ok, read the header info for each
    for ( int iOn = iBase; iOn < m_aRaw.GetSize(); iOn++ ) {
        m_aRaw[iOn].InitData( pMmioVoices, iGrp );
        m_aRaw[iOn].m_bVoice = TRUE;
    }
    pMmioVoices->AscendList();
    delete pMmioVoices;

    // read the digital music headers (always, since we're wav_only)
    // read the common music
    pMmio->DescendList( 'M', 'U', 'S', '0' );
    pMmio->DescendChunk( 'N', 'U', 'M', 'M' );
    iBase = m_aRaw.GetSize();
    m_aRaw.SetSize( iBase + pMmio->ReadShort() );
    pMmio->AscendChunk();

    for ( int iOn = iBase; iOn < m_aRaw.GetSize(); iOn++ )
        m_aRaw[iOn].InitData( pMmio, iGrp );
    pMmio->AscendList();

    // read the non-MIDI music
    pMmio->DescendList( 'M', 'U', 'S', '2' );
    pMmio->DescendChunk( 'N', 'U', 'M', 'M' );
    iBase = m_aRaw.GetSize();
    m_aRaw.SetSize( iBase + pMmio->ReadShort() );
    pMmio->AscendChunk();

    for ( int iOn = iBase; iOn < m_aRaw.GetSize(); iOn++ )
        m_aRaw[iOn].InitData( pMmio, iGrp );
    pMmio->AscendList();

    delete pMmio;

    // for cached & buffered items
    ASSERT( m_pFileReg == NULL );
    ASSERT( m_pFileVoc == NULL );
#ifdef _DEBUG
    theDataFile.DisableNegativeSeekChecking();
#endif

    m_pFileReg = theDataFile.OpenAsFile( sMusicFile );
    m_pFileVoc = theDataFile.OpenAsFile( sVoiceFile );

#ifdef _DEBUG
    theDataFile.EnableNegativeSeekChecking();
#endif
    } catch ( ... ) {
        OutputDebugString( "CMusicPlayer::InitData: sound data incomplete (demo layout?) - continuing without full audio\n" );
    }
}

BOOL CMusicPlayer::IsGroupLoaded( int iGrp ) {

    if ( !m_bDigOpen )
        return TRUE;

    for ( int iOn = 0; iOn < m_aRaw.GetSize(); iOn++ )
        if ( ( m_aRaw[iOn].m_iGroup == iGrp ) && ( m_aRaw[iOn].m_iType == CRawData::preload ) )
            if ( m_aRaw[iOn].m_iStat != CRawData::loaded )
                return FALSE;

    return TRUE;
}

void CMusicPlayer::LoadGroup( int iGrp ) {

    if ( !m_bDigOpen )
        return;

    for ( int iOn = 0; iOn < m_aRaw.GetSize(); iOn++ )
        if ( ( m_aRaw[iOn].m_iGroup == iGrp ) && ( m_aRaw[iOn].m_iType == CRawData::preload ) )
            m_aRaw[iOn].LoadBuffer( m_aRaw[iOn].m_bVoice ? m_pFileVoc : m_pFileReg );
}

void CMusicPlayer::UnloadGroup( int iGrp ) {

    for ( int iOn = 0; iOn < m_aRaw.GetSize(); iOn++ )
        if ( m_aRaw[iOn].m_iGroup == iGrp )
            m_aRaw[iOn].UnloadBuffer();
}

void CMusicPlayer::Close() {

    // Stop the service thread first (before tearing down audio state)
    StopServiceThread();

    // don't need this anymore
    delete m_pFileReg;
    delete m_pFileVoc;
    m_pFileReg = m_pFileVoc = NULL;

    if ( !m_bRunning )
        return;

    // Stop streaming music
    Mix_HookMusic( NULL, NULL );

    // Stop all channels
    if ( m_bDigOpen ) {
        Mix_HaltChannel( -1 );

        // Free all chunks
        for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++ )
            FreeChunk( &m_Channel[iOn] );

        Mix_CloseAudio();
        m_bDigOpen = false;
    }

    m_bRunning = FALSE;

    for ( int iOn = 0; iOn < m_aRaw.GetSize(); iOn++ )
        m_aRaw[iOn].Close();
    m_aRaw.RemoveAll();

    for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++ )
        m_Channel[iOn].Close();
}

void CMusicPlayer::ShutUp() {

    if ( !m_bRunning )
        return;

    SoundsOff();
}

BOOL CMusicPlayer::OnActivate( BOOL bActive ) {

    CAudioLock lock( *this );

    if ( !bActive ) {
        if ( m_bDigOpen ) {
            Mix_Pause( -1 );
            Mix_HookMusic( NULL, NULL );  // pause streaming too
        }
        return ( TRUE );
    }

    if ( m_bDigOpen ) {
        Mix_Resume( -1 );
        // Re-enable streaming if music channel was streaming
        CRawChannel* pMusicCh = &m_Channel[0];
        if ( pMusicCh->m_bStreamActive && pMusicCh->m_pData != NULL )
            Mix_HookMusic( MusicHookCallback, pMusicCh );
    }
    return ( TRUE );
}

void CMusicPlayer::OpenDigital( int iVol ) {

    ASSERT( ( 0 <= iVol ) && ( iVol <= 127 ) );
    ASSERT( !m_bDigOpen );

    if ( m_bNoWav )
        return;

    try {
        if ( ( iVol != 0 ) && ( waveOutGetNumDevs() > 0 ) ) {
            if ( Mix_OpenAudio( DEVICE_FREQ, AUDIO_S16SYS, DEVICE_CHANNELS, 2048 ) < 0 ) {
                TRACE( "Mix_OpenAudio failed: %s\n", Mix_GetError() );
                CDlgMsg dlg;
                dlg.MsgBox( IDS_ERROR_WAV, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NoDigitalSound" );
                return;
            }

            m_bDigOpen = true;

            // Allocate channels
            Mix_AllocateChannels( MAX_SOUND_SAMPLES );

            // Set up channel map and assign channel indices
            for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++ ) {
                m_Channel[iOn].m_iChannel = iOn;
                m_Channel[iOn].m_pChunk = NULL;
                m_Channel[iOn].m_pendingEvent = PENDING_NONE;
                g_channelMap[iOn] = &m_Channel[iOn];
            }

            // Register the channel-finished callback
            Mix_ChannelFinished( OnChannelFinished );

            // Set master volume
            Mix_MasterVolume( iVol * 128 / 127 );
        }
    }

    catch ( ... ) {
        m_bNoWav = TRUE;
        try {
            CDlgMsg dlg;
            dlg.MsgBox( IDS_ERROR_WAV, MB_OK | MB_ICONSTOP | MB_TASKMODAL, "Warnings", "NoDigitalSound" );
            if ( m_bDigOpen ) {
                Mix_CloseAudio();
                m_bDigOpen = false;
            }
        } catch ( ... ) {
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// SDL Callbacks (audio thread - minimal work only!)

void SDLCALL CMusicPlayer::OnChannelFinished( int channel ) {

    // Called from SDL audio thread - only set a flag
    if ( channel >= 0 && channel < MAX_SOUND_SAMPLES && g_channelMap[channel] != NULL )
        g_channelMap[channel]->m_pendingEvent = PENDING_FINISHED;
}

void SDLCALL CMusicPlayer::MusicHookCallback( void* udata, Uint8* stream, int len ) {

    // Called from SDL audio thread - copy data from triple buffers, apply volume
    CRawChannel* pRaw = (CRawChannel*)udata;
    if ( pRaw == NULL || !pRaw->m_bStreamActive ) {
        memset( stream, 0, len );
        return;
    }

    int iMusicVol = theMusicPlayer.m_iMusicVol;  // atomic read of int
    int iBytesWritten = 0;

    while ( iBytesWritten < len ) {
        int iBuf = pRaw->m_iPlayBuf;
        int iAvail = pRaw->m_iPlayLen[iBuf] - pRaw->m_iPlayPos;

        if ( iAvail <= 0 ) {
            // Current buffer exhausted, signal game thread
            pRaw->m_bBufConsumed = true;

            // Advance to next buffer
            int iNextBuf = ( iBuf + 1 ) % 3;
            if ( pRaw->m_iPlayLen[iNextBuf] > 0 ) {
                pRaw->m_iPlayBuf = iNextBuf;
                pRaw->m_iPlayPos = 0;
                continue;
            }

            // No next buffer ready - fill with silence
            memset( stream + iBytesWritten, 0, len - iBytesWritten );

            // If EOF and no more data, signal stream done
            if ( pRaw->m_bStreamEOF )
                pRaw->m_bStreamActive = false;
            return;
        }

        int iToCopy = __min( iAvail, len - iBytesWritten );
        const char* pSrc = (const char*)pRaw->m_pDblBuf[iBuf] + pRaw->m_iPlayPos;

        // Copy with volume scaling
        if ( iMusicVol >= 127 ) {
            // Full volume - straight copy
            memcpy( stream + iBytesWritten, pSrc, iToCopy );
        } else if ( iMusicVol <= 0 ) {
            memset( stream + iBytesWritten, 0, iToCopy );
        } else {
            // Scale 16-bit samples by volume
            const short* pIn = (const short*)pSrc;
            short* pOut = (short*)( stream + iBytesWritten );
            int iNumSamples = iToCopy / 2;
            for ( int i = 0; i < iNumSamples; i++ )
                pOut[i] = (short)( ( (int)pIn[i] * iMusicVol ) / 127 );
        }

        iBytesWritten += iToCopy;
        pRaw->m_iPlayPos += iToCopy;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Deferred work processing (game thread)

void CMusicPlayer::YieldPlayer() {

    if ( !m_bDigOpen )
        return;

    CAudioLock lock( *this );

    // Process pending channel-finished events
    for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++ ) {
        CRawChannel* pCh = &m_Channel[iOn];
        if ( pCh->m_pendingEvent == PENDING_FINISHED ) {
            pCh->m_pendingEvent = PENDING_NONE;
            HandleChannelFinished( pCh );
        }
    }

    // Process streaming music
    ProcessMusicStreaming();
}


/////////////////////////////////////////////////////////////////////////////
// Audio service thread: ensures audio processing continues during modal loops (resize, menus, etc.)

UINT CMusicPlayer::ServiceThreadProc( LPVOID pParam ) {

    CMusicPlayer* pPlayer = (CMusicPlayer*)pParam;

    while ( pPlayer->m_bServiceRunning ) {
        ::Sleep( 50 );  // ~20Hz service rate

        if ( !pPlayer->m_bServiceRunning )
            break;

        if ( !pPlayer->m_bDigOpen || !pPlayer->m_bRunning )
            continue;

        // Same work as YieldPlayer, protected by the same critical section
        if ( !pPlayer->m_bCsInitialized )
            continue;

        EnterCriticalSection( &pPlayer->m_csAudio );

        try {
            for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++ ) {
                CRawChannel* pCh = &pPlayer->m_Channel[iOn];
                if ( pCh->m_pendingEvent == PENDING_FINISHED ) {
                    pCh->m_pendingEvent = PENDING_NONE;
                    pPlayer->HandleChannelFinished( pCh );
                }
            }

            pPlayer->ProcessMusicStreaming();
        } catch ( ... ) {
        }

        LeaveCriticalSection( &pPlayer->m_csAudio );
    }

    return 0;
}

void CMusicPlayer::StartServiceThread() {

    if ( m_pServiceThread != NULL )
        return;

    m_bServiceRunning = true;
    m_pServiceThread = AfxBeginThread( ServiceThreadProc, this, THREAD_PRIORITY_ABOVE_NORMAL, 0, CREATE_SUSPENDED );
    if ( m_pServiceThread != NULL ) {
        m_pServiceThread->m_bAutoDelete = FALSE;  // prevent MFC from deleting the object
        m_pServiceThread->ResumeThread();
    }
}

void CMusicPlayer::StopServiceThread() {

    if ( m_pServiceThread == NULL )
        return;

    m_bServiceRunning = false;

    // Wait for thread to finish (it checks m_bServiceRunning every 50ms)
    if ( m_pServiceThread->m_hThread != NULL )
        ::WaitForSingleObject( m_pServiceThread->m_hThread, 2000 );

    delete m_pServiceThread;
    m_pServiceThread = NULL;
}

void CMusicPlayer::HandleChannelFinished( CRawChannel* pCh ) {

    // Free the chunk wrapper (data owned by CRawData)
    FreeChunk( pCh );

    if ( pCh->m_pData == NULL )
        return;

    if ( pCh->m_pData->m_bKillMe ) {
        pCh->m_pData->m_bKillMe = FALSE;
        pCh->m_pData = NULL;
        return;
    }

    // Group music - pick next track
    if ( pCh->m_iIsFore == CRawChannel::music ) {
        StartRaw( pCh, &( m_aRaw.ElementAt( m_iMusicGrpFirst + RandNum( m_iMusicGrpNum - 1 ) ) ) );
        return;
    }

    // Exclusive music - replay
    if ( pCh->m_iIsFore == CRawChannel::excl ) {
        if ( m_iMusicVol == 0 )
            return;
        StartRaw( pCh, pCh->m_pData );
        return;
    }

    // Foreground sound finishing - switch back to background mode
    pCh->m_iIsFore = CRawChannel::back;

    if ( ( pCh->m_iBackPri == INT_MAX ) || ( m_iSfxVol == 0 ) ) {
        pCh->m_pData = NULL;
        return;
    }

    // busy setting new sounds
    if ( pCh->m_iBackSound < 0 ) {
        pCh->m_iBackSound = -pCh->m_iBackSound;
        return;
    }

    StartRaw( pCh, &( m_aRaw.ElementAt( pCh->m_iBackSound ) ) );
}

void CMusicPlayer::ProcessMusicStreaming() {

    CRawChannel* pMusicCh = &m_Channel[0];

    // Not streaming? Nothing to do
    if ( !pMusicCh->m_bStreamActive && !pMusicCh->m_bBufConsumed )
        return;

    // Buffer consumed by hook - kick next read-ahead
    if ( pMusicCh->m_bBufConsumed ) {
        pMusicCh->m_bBufConsumed = false;

        if ( pMusicCh->m_pData != NULL && !pMusicCh->m_bStreamEOF ) {
            // Clear the consumed buffer length so hook knows it's not ready
            int iConsumedBuf = ( pMusicCh->m_iPlayBuf + 2 ) % 3;  // the one that was just consumed
            pMusicCh->m_iPlayLen[iConsumedBuf] = 0;

            // Load next buffer
            pMusicCh->m_pData->LoadNextDblBuffer( pMusicCh, iConsumedBuf );
        }
    }

    // Stream ended - handle next track
    if ( !pMusicCh->m_bStreamActive && pMusicCh->m_pData != NULL ) {
        Mix_HookMusic( NULL, NULL );

        if ( pMusicCh->m_pData->m_bKillMe ) {
            pMusicCh->m_pData->m_bKillMe = FALSE;
            pMusicCh->m_pData = NULL;
            return;
        }

        // Pick next track based on channel type
        if ( pMusicCh->m_iIsFore == CRawChannel::music ) {
            StartRaw( pMusicCh, &( m_aRaw.ElementAt( m_iMusicGrpFirst + RandNum( m_iMusicGrpNum - 1 ) ) ) );
        } else if ( pMusicCh->m_iIsFore == CRawChannel::excl ) {
            if ( m_iMusicVol > 0 )
                StartRaw( pMusicCh, pMusicCh->m_pData );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// Channel helpers

void CMusicPlayer::HaltChannel( CRawChannel* pCh ) {

    if ( !m_bDigOpen || pCh->m_iChannel < 0 )
        return;

    // For streaming channel, stop the hook first
    if ( pCh->m_bStreamActive ) {
        Mix_HookMusic( NULL, NULL );
        pCh->m_bStreamActive = false;
    }

    if ( Mix_Playing( pCh->m_iChannel ) )
        Mix_HaltChannel( pCh->m_iChannel );

    // Clear any pending event since we're handling this synchronously
    pCh->m_pendingEvent = PENDING_NONE;

    FreeChunk( pCh );
}

void CMusicPlayer::FreeChunk( CRawChannel* pCh ) {

    if ( pCh->m_pChunk != NULL ) {
        Mix_FreeChunk( pCh->m_pChunk );
        pCh->m_pChunk = NULL;
    }
}

void CMusicPlayer::SetChannelVolume( CRawChannel* pCh ) {

    if ( !m_bDigOpen || pCh->m_iChannel < 0 )
        return;

    int iVol;
    if ( ( pCh->m_iIsFore == CRawChannel::excl ) || ( pCh->m_iIsFore == CRawChannel::music ) )
        iVol = m_iMusicVol;
    else {
        pCh->m_iVol = __minmax( 10, 127, pCh->m_iVol );
        iVol = ( m_iSfxVol * pCh->m_iVol ) / 128;
    }

    Mix_Volume( pCh->m_iChannel, iVol * 128 / 127 );
}

void CMusicPlayer::SetChannelPan( CRawChannel* pCh ) {

    if ( !m_bDigOpen || pCh->m_iChannel < 0 )
        return;

    pCh->m_iPan = __minmax( 0, 127, pCh->m_iPan );

    // MSS pan: 0=left, 64=center, 127=right
    // SDL_mixer SetPanning: left=0-255, right=0-255
    int iRight = pCh->m_iPan * 2;
    if ( iRight > 255 ) iRight = 255;
    int iLeft = 255 - iRight;
    Mix_SetPanning( pCh->m_iChannel, (Uint8)iLeft, (Uint8)iRight );
}


/////////////////////////////////////////////////////////////////////////////
// Music playback

void CMusicPlayer::PlayExclusiveMusic( int iSound ) {

    CAudioLock lock( *this );

    if ( ( m_iMusicVol == 0 ) || ( !m_bRunning ) )
        return;

    if ( ( iSound < 0 ) || ( m_aRaw.GetSize() <= iSound ) )
        return;

    if ( m_bNoWav )
        return;

    // kill channel 0 if in use
    if ( m_bDigOpen ) {
        HaltChannel( &m_Channel[0] );
        if ( m_Channel[0].m_pData != NULL )
            m_Channel[0].m_pData->m_bKillMe = TRUE;
    }

    // set music
    m_Channel[0].m_pData = &( m_aRaw.ElementAt( iSound ) );
    m_Channel[0].m_iIsFore = CRawChannel::excl;
    m_Channel[0].m_iBackPri = 0;
    StartRaw( &m_Channel[0], m_Channel[0].m_pData );
}

void CMusicPlayer::EndExclusiveMusic() {

    CAudioLock lock( *this );

    if ( ( m_iMusicVol == 0 ) || ( !m_bRunning ) )
        return;

    if ( ( m_bDigOpen ) && ( m_Channel[0].m_iIsFore == CRawChannel::excl ) ) {
        m_Channel[0].m_iIsFore = CRawChannel::back;

        HaltChannel( &m_Channel[0] );
        if ( m_Channel[0].m_pData != NULL )
            m_Channel[0].m_pData->m_bKillMe = TRUE;

        m_Channel[0].m_iBackPri = INT_MAX;
        m_Channel[0].m_iNum = 0;
    }

    m_Channel[0].FreeDblBuf();
}

void CMusicPlayer::EndMusicGroup() {

    CAudioLock lock( *this );

    if ( ( m_iMusicVol == 0 ) || ( m_iMode != CMusicPlayer::MUSIC_MODE::wav_only ) || ( !m_bRunning ) )
        return;

    if ( ( m_bDigOpen ) && ( m_Channel[0].m_iIsFore == CRawChannel::music ) ) {
        m_Channel[0].m_iIsFore = CRawChannel::back;

        HaltChannel( &m_Channel[0] );
        if ( m_Channel[0].m_pData != NULL )
            m_Channel[0].m_pData->m_bKillMe = TRUE;

        m_Channel[0].m_iBackPri = INT_MAX;
        m_Channel[0].m_iNum = 0;
    }
}

void CMusicPlayer::PlayMusicGroup( int iGrpStrt, int iNumGrp ) {

    CAudioLock lock( *this );

    if ( !m_bDigOpen || ( m_iMode != CMusicPlayer::MUSIC_MODE::wav_only ) )
        return;

    m_iMusicGrpFirst = iGrpStrt;
    m_iMusicGrpNum = iNumGrp;

    // kill channel 0 if in use
    HaltChannel( &m_Channel[0] );
    if ( m_Channel[0].m_pData != NULL )
        m_Channel[0].m_pData->m_bKillMe = TRUE;

    m_Channel[0].m_iIsFore = CRawChannel::music;
    m_Channel[0].m_iBackPri = 0;
    StartRaw( &m_Channel[0], &( m_aRaw.ElementAt( m_iMusicGrpFirst + RandNum( m_iMusicGrpNum - 1 ) ) ) );
}

// MIDI stubs (no-ops for API compatibility)
void CMusicPlayer::StartMidiMusic() {
    // MIDI removed - start digital music if available
    // Game code typically calls PlayMusicGroup() after this
}

void CMusicPlayer::StopMidiMusic() {
    // MIDI removed - no-op
}


/////////////////////////////////////////////////////////////////////////////
// Sound effects

void CMusicPlayer::PlayForegroundSound( int iSound, int iPri, int iPan, int iVol ) {

    CAudioLock lock( *this );

    if ( ( iSound <= 0 ) || ( iSound >= m_aRaw.GetSize() ) || ( !m_bDigOpen ) || ( m_iSfxVol == 0 ) )
        return;

    // find the lowest pri background channel
    CRawChannel* pRawOn = nullptr;
    int iVal = -1;
    {
        CRawChannel* pRaw = m_Channel;
        for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++, pRaw++ )
            if ( pRaw->m_iIsFore == CRawChannel::back )
                if ( ( iVal == -1 ) || ( pRaw->m_iBackPri > iVal ) ) {
                    pRawOn = pRaw;
                    iVal = pRaw->m_iBackPri;
                    if ( pRaw->m_iBackPri == INT_MAX )
                        break;
                }
    }

    // if all are background, see if a foreground is available
    if ( iVal == -1 ) {
        CRawChannel* pRaw = m_Channel;
        for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++, pRaw++ )
            if ( pRaw->m_iIsFore == CRawChannel::fore ) {
                if ( pRaw->m_iForeSound == iSound )
                    return;

                if ( ( iVal == -1 ) || ( pRaw->m_iForePri > iVal ) ) {
                    pRawOn = pRaw;
                    iVal = pRaw->m_iForePri;
                }
            }

        if ( iVal == -1 ) {
            TRAP();
            return;
        }
    }

    // kill it if in use
    if ( pRawOn != nullptr ) {
        HaltChannel( pRawOn );
        if ( pRawOn->m_pData != NULL )
            pRawOn->m_pData->m_bKillMe = TRUE;

        pRawOn->m_iIsFore = CRawChannel::fore;
        pRawOn->m_iVol = iVol;
        pRawOn->m_iPan = iPan;
        pRawOn->m_iForeSound = iSound;
        pRawOn->m_iForePri = iPri;
        StartRaw( pRawOn, &( m_aRaw.ElementAt( iSound ) ) );
    }
}

void CMusicPlayer::KillForegroundSound( int iSound ) {

    CAudioLock lock( *this );

    if ( ( iSound <= 0 ) || ( iSound >= m_aRaw.GetSize() ) )
        return;

    // Halt any channel currently playing this sound before freeing the buffer,
    // because Mix_QuickLoad_RAW chunks reference the buffer directly.
    CRawData* pRd = &m_aRaw.ElementAt( iSound );
    if ( m_bDigOpen ) {
        CRawChannel* pRaw = m_Channel;
        for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++, pRaw++ )
            if ( pRaw->m_pData == pRd ) {
                HaltChannel( pRaw );
                pRaw->m_pData = NULL;
            }
    }

    pRd->UnloadBuffer();
}

void CMusicPlayer::ClrBackgroundSounds() {

    if ( ( !m_bDigOpen ) || ( m_iSfxVol == 0 ) ) {
        TRAP();
        return;
    }

    CRawChannel* pRaw = m_Channel;
    for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++, pRaw++ )
        if ( ( pRaw->m_iIsFore == CRawChannel::back ) || ( pRaw->m_iIsFore == CRawChannel::fore ) ) {
            pRaw->m_iBackPri = INT_MAX;
            pRaw->m_iNum = 0;
        }
}

void CMusicPlayer::QueueBackgroundSound( int iSound, int iPri, int iPan, int iVol ) {

    if ( ( iSound == 0 ) || ( iSound >= m_aRaw.GetSize() ) || ( iVol < 10 ) || ( !m_bDigOpen ) || ( m_iSfxVol == 0 ) ) {
        TRAP();
        return;
    }

    // if we're already in there
    CRawChannel* pRaw = m_Channel;
    for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++, pRaw++ )
        if ( abs( pRaw->m_iBackSound ) == iSound ) {
            pRaw->m_iBackSound = -iSound;
            pRaw->m_iBackPri = iPri;
            pRaw->m_iNum++;
            if ( pRaw->m_iNum <= 1 ) {
                pRaw->m_iVol = iVol;
                pRaw->m_iPan = iPan;
            } else {
                pRaw->m_iVol = ( pRaw->m_iVol * ( pRaw->m_iNum - 1 ) + iVol ) / pRaw->m_iNum;
                pRaw->m_iPan = ( pRaw->m_iPan * ( pRaw->m_iNum - 1 ) + iPan ) / pRaw->m_iNum;
            }
            return;
        }

    // find the highest # (lowest priority)
    pRaw = m_Channel;
    int iVal = pRaw->m_iBackPri;
    CRawChannel* pRawOn = m_Channel;
    for ( int iOn = 1; iOn < MAX_SOUND_SAMPLES; iOn++, pRaw++ )
        if ( pRaw->m_iBackPri > iVal ) {
            pRawOn = pRaw;
            iVal = pRaw->m_iBackPri;
        }

    if ( iVal < iPri )
        return;

    pRawOn->m_iBackPri = iPri;
    pRawOn->m_iBackSound = iSound;
    pRawOn->m_iVol = iVol;
    pRawOn->m_iPan = iPan;
    pRawOn->m_iNum = 1;
}

void CMusicPlayer::UpdateBackgroundSounds() {

    CAudioLock lock( *this );

    if ( ( !m_bDigOpen ) || ( m_iSfxVol == 0 ) ) {
        TRAP();
        return;
    }

    CRawChannel* pRaw = m_Channel;
    for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++, pRaw++ )
        if ( pRaw->m_iIsFore != CRawChannel::back )
            pRaw->m_iBackSound = abs( pRaw->m_iBackSound );
        else {
            // we never used it - kill it
            if ( pRaw->m_iBackPri == INT_MAX ) {
                HaltChannel( pRaw );
                if ( pRaw->m_pData != NULL )
                    pRaw->m_pData->m_bKillMe = TRUE;
                pRaw->m_iBackSound = 0;
            } else

                // if its the same leave it alone
                if ( pRaw->m_iBackSound < 0 )
                    pRaw->m_iBackSound = -pRaw->m_iBackSound;
                else

                    // start it up
                {
                    HaltChannel( pRaw );
                    if ( pRaw->m_pData != NULL )
                        pRaw->m_pData->m_bKillMe = TRUE;

                    StartRaw( pRaw, &( m_aRaw.ElementAt( pRaw->m_iBackSound ) ) );
                }
        }
}

void CMusicPlayer::IncBackgroundSound( int iSound, int iPan, int iVol ) {

    CAudioLock lock( *this );

    if ( ( iSound == 0 ) || ( iSound >= m_aRaw.GetSize() ) || ( iVol < 10 ) || ( !m_bDigOpen ) || ( m_iSfxVol == 0 ) )
        return;

    CRawChannel* pRaw = m_Channel;
    for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++, pRaw++ )
        if ( abs( pRaw->m_iBackSound ) == iSound ) {
            pRaw->m_iNum++;
            if ( pRaw->m_iNum <= 1 ) {
                pRaw->m_iVol = iVol;
                pRaw->m_iPan = iPan;
            } else {
                pRaw->m_iVol = ( pRaw->m_iVol * ( pRaw->m_iNum - 1 ) + iVol ) / pRaw->m_iNum;
                pRaw->m_iPan = ( pRaw->m_iPan * ( pRaw->m_iNum - 1 ) + iPan ) / pRaw->m_iNum;
            }
            return;
        }

    // not playing - try to find a channel
    pRaw = m_Channel;
    int iVal = pRaw->m_iBackPri;
    CRawChannel* pRawOn = m_Channel;
    for ( int iOn = 1; iOn < MAX_SOUND_SAMPLES; iOn++, pRaw++ )
        if ( pRaw->m_iBackPri > iVal ) {
            pRawOn = pRaw;
            iVal = pRaw->m_iBackPri;
        }

    if ( iVal <= BACKGROUND_PRI )
        return;

    pRawOn->m_iBackPri = BACKGROUND_PRI;
    pRawOn->m_iBackSound = iSound;
    pRawOn->m_iVol = iVol;
    pRawOn->m_iPan = iPan;
    pRawOn->m_iNum = 1;

    if ( pRawOn->m_iIsFore != CRawChannel::back )
        return;

    HaltChannel( pRawOn );
    if ( pRawOn->m_pData != NULL )
        pRawOn->m_pData->m_bKillMe = TRUE;

    StartRaw( pRawOn, &( m_aRaw.ElementAt( pRawOn->m_iBackSound ) ) );
}

void CMusicPlayer::DecBackgroundSound( int iSound, int iPan, int iVol ) {

    CAudioLock lock( *this );

    if ( ( iSound == 0 ) || ( iSound >= m_aRaw.GetSize() ) || ( iVol < 10 ) || ( !m_bDigOpen ) || ( m_iSfxVol == 0 ) )
        return;

    CRawChannel* pRaw = m_Channel;
    for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++, pRaw++ )
        if ( abs( pRaw->m_iBackSound ) == iSound ) {
            pRaw->m_iNum--;
            if ( ( pRaw->m_iNum > 0 ) || ( pRaw->m_iIsFore != CRawChannel::back ) ) {
                if ( pRaw->m_iNum <= 0 )
                    pRaw->m_iBackSound = 0;
                break;
            }

            // ok - turn it off
            HaltChannel( pRaw );
            if ( pRaw->m_pData != NULL )
                pRaw->m_pData->m_bKillMe = TRUE;
            pRaw->m_iBackSound = 0;
            break;
        }
}

void CMusicPlayer::SoundsOff() {

    CAudioLock lock( *this );

    if ( !m_bDigOpen )
        return;

    // Stop streaming music
    Mix_HookMusic( NULL, NULL );

    CRawChannel* pRaw = m_Channel;
    for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++, pRaw++ ) {
        pRaw->m_iIsFore = CRawChannel::back;
        pRaw->m_iBackSound = 0;
        pRaw->m_iBackPri = INT_MAX;
        HaltChannel( pRaw );
        if ( pRaw->m_pData != NULL )
            pRaw->m_pData->m_bKillMe = TRUE;
        pRaw->FreeDblBuf();
    }
}


/////////////////////////////////////////////////////////////////////////////
// Volume

int CMusicPlayer::GetMusicVolume() const {

    if ( !m_bDigOpen )
        return ( 0 );

    return ( ( m_iMusicVol * 100 ) / 127 );
}

void CMusicPlayer::SetMusicVolume( int iVol ) {

    CAudioLock lock( *this );

    ASSERT( ( 0 <= iVol ) && ( iVol <= 100 ) );

    iVol = ( iVol * 127 ) / 100;
    if ( iVol == m_iMusicVol )
        return;
    m_iMusicVol = iVol;

    // if they're both 0 shut it down
    if ( ( m_iMusicVol == 0 ) && ( m_iSfxVol == 0 ) ) {
        if ( m_bDigOpen ) {
            Mix_HookMusic( NULL, NULL );
            Mix_HaltChannel( -1 );
            for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++ )
                FreeChunk( &m_Channel[iOn] );
            Mix_CloseAudio();
            m_bDigOpen = false;
        }
        m_bRunning = FALSE;
        return;
    }

    CheckDigVol();
}

int CMusicPlayer::GetSoundVolume() const {

    if ( !m_bDigOpen )
        return ( 0 );

    return ( ( m_iSfxVol * 100 ) / 127 );
}

void CMusicPlayer::SetSoundVolume( int iVol ) {

    CAudioLock lock( *this );

    ASSERT( ( 0 <= iVol ) && ( iVol <= 100 ) );

    iVol = ( iVol * 127 ) / 100;
    if ( iVol == m_iSfxVol )
        return;

    m_iSfxVol = iVol;

    if ( m_bNoWav )
        return;

    // if we're both off - kill it
    if ( ( m_iMusicVol == 0 ) && ( m_iSfxVol == 0 ) ) {
        if ( m_bDigOpen ) {
            Mix_HookMusic( NULL, NULL );
            Mix_HaltChannel( -1 );
            for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++ )
                FreeChunk( &m_Channel[iOn] );
            Mix_CloseAudio();
            m_bDigOpen = false;
        }
        m_bRunning = FALSE;
        return;
    }

    CheckDigVol();
}

void CMusicPlayer::CheckDigVol() {

    if ( m_bNoWav )
        return;

    int iVol = __max( m_iSfxVol, m_iMusicVol );

    // turn it off
    if ( iVol == 0 ) {
        if ( !m_bDigOpen )
            return;
        Mix_HookMusic( NULL, NULL );
        Mix_HaltChannel( -1 );
        for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++ )
            FreeChunk( &m_Channel[iOn] );
        Mix_CloseAudio();
        m_bDigOpen = false;
        m_Channel[0].FreeDblBuf();
        return;
    }

    // turn it on
    if ( !m_bDigOpen ) {
        OpenDigital( iVol );
        if ( !m_bDigOpen )
            return;
    }

    // Update volumes on playing channels
    CRawChannel* pRaw = m_Channel;
    for ( int iOn = 0; iOn < MAX_SOUND_SAMPLES; iOn++, pRaw++ ) {
        if ( ( pRaw->m_pData != NULL ) && ( pRaw->m_pData->m_bKillMe != TRUE ) &&
             ( ( pRaw->m_iIsFore != CRawChannel::back ) ||
               ( ( pRaw->m_iBackPri >= 0 ) && ( pRaw->m_iBackPri != INT_MAX ) ) ) ) {

            BOOL bPlaying = Mix_Playing( pRaw->m_iChannel );

            // turn off if music vol 0
            if ( ( m_iMusicVol == 0 ) && ( ( pRaw->m_iIsFore == CRawChannel::excl ) || ( pRaw->m_iIsFore == CRawChannel::music ) ) ) {
                if ( bPlaying || pRaw->m_bStreamActive ) {
                    HaltChannel( pRaw );
                    pRaw->m_pData->m_bKillMe = TRUE;
                }
                continue;
            }

            // turn off if sfx vol 0
            if ( ( m_iSfxVol == 0 ) && ( ( pRaw->m_iIsFore == CRawChannel::back ) || ( pRaw->m_iIsFore == CRawChannel::fore ) ) ) {
                if ( bPlaying ) {
                    HaltChannel( pRaw );
                    pRaw->m_pData->m_bKillMe = TRUE;
                }
                continue;
            }

            // if just a change in volume - update it
            if ( bPlaying || pRaw->m_bStreamActive ) {
                // Streaming music volume is applied in the hook callback (reads m_iMusicVol directly)
                // For non-streaming channels, update SDL_mixer volume
                if ( !pRaw->m_bStreamActive )
                    SetChannelVolume( pRaw );
                continue;
            }

            // Not playing - restart
            StartRaw( pRaw, pRaw->m_pData );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// StartRaw - the core playback function

BOOL CMusicPlayer::StartRaw( CRawChannel* pRawChannel, CRawData* pRawData ) {

    pRawChannel->m_pData = pRawData;

    // if no vol or no device - exit
    if ( !m_bDigOpen )
        return ( TRUE );
    if ( ( pRawChannel->m_iIsFore == CRawChannel::excl ) || ( pRawChannel->m_iIsFore == CRawChannel::music ) ) {
        if ( m_iMusicVol == 0 )
            return ( TRUE );
    } else
        if ( m_iSfxVol == 0 )
            return ( TRUE );

    if ( pRawData->m_iType == CRawData::buffer ) {
        if ( pRawChannel->m_pDblBuf[1] == NULL )
            if ( !pRawChannel->AllocDblBuf() ) {
                TRAP();
                return ( FALSE );
            }
    } else
    {
        // if dead forget it
        if ( pRawData->m_iStat == CRawData::dead ) {
            TRAP();
            return ( FALSE );
        }

        if ( pRawData->m_pBuf == NULL ) {
            pRawData->LoadBuffer( pRawData->m_bVoice ? m_pFileVoc : m_pFileReg );
            if ( pRawData->m_iStat != CRawData::loaded ) {
                TRAP();
                return ( FALSE );
            }
        }
    }

    pRawData->m_bKillMe = FALSE;
    pRawData->m_dwLastUsed = timeGetTime();

    // Double buffered (streaming music)
    if ( pRawData->m_iType == CRawData::buffer ) {
        // Reset streaming state
        pRawChannel->m_bStreamEOF = false;
        pRawChannel->m_bStreamActive = false;
        pRawChannel->m_bBufConsumed = false;
        pRawChannel->m_iPlayBuf = 0;
        pRawChannel->m_iPlayPos = 0;
        pRawChannel->m_iPlayLen[0] = pRawChannel->m_iPlayLen[1] = pRawChannel->m_iPlayLen[2] = 0;

        // Start filling buffers
        pRawData->StartDblBuffer( pRawChannel );

        // Start the music hook
        if ( pRawChannel->m_bStreamActive )
            Mix_HookMusic( MusicHookCallback, pRawChannel );
    } else {
        // Non-streaming: create Mix_Chunk and play

        // Free old chunk if any
        FreeChunk( pRawChannel );

        // Create a Mix_Chunk pointing to the data buffer
        // Mix_QuickLoad_RAW does not copy or free the data
        pRawChannel->m_pChunk = Mix_QuickLoad_RAW( (Uint8*)pRawData->m_pBuf, pRawData->m_iDataLen );
        if ( pRawChannel->m_pChunk == NULL ) {
            TRAP();
            return ( FALSE );
        }

        // Set volume
        SetChannelVolume( pRawChannel );

        // Set pan for SFX
        if ( ( pRawChannel->m_iIsFore != CRawChannel::excl ) && ( pRawChannel->m_iIsFore != CRawChannel::music ) )
            SetChannelPan( pRawChannel );

        // Play: background loops infinitely, foreground plays once
        int iLoops;
        if ( pRawChannel->m_iIsFore == CRawChannel::back )
            iLoops = -1;  // infinite loop
        else
            iLoops = 0;  // play once

        Mix_PlayChannel( pRawChannel->m_iChannel, pRawChannel->m_pChunk, iLoops );
    }

    return ( TRUE );
}


/////////////////////////////////////////////////////////////////////////////
// Utility

void CMusicPlayer::FreeOldSounds( int iSec ) {

    DWORD dwMin = timeGetTime() - iSec * 1000;

    for ( int iOn = 0; iOn < m_aRaw.GetSize(); iOn++ ) {
        CRawData* pRd = &( m_aRaw[iOn] );
        if ( pRd->m_iType == CRawData::cache )
            if ( pRd->m_iStat == CRawData::loaded )
                if ( pRd->m_dwLastUsed < dwMin ) {

                    // make sure not in use
                    CRawChannel* pRaw = m_Channel;
                    for ( int iCh = 0; iCh < MAX_SOUND_SAMPLES; iCh++, pRaw++ )
                        if ( pRaw->m_pData == pRd )
                            goto TryNext;
                    pRd->UnloadBuffer();
                TryNext:
                    ;
                }
    }
}

void CMusicPlayer::GetDigitalConfig( int* pRate, int* pChannels, CString& sName ) {

    std::string s;
    GetDigitalConfig( pRate, pChannels, s );
    sName = s.c_str();
}

void CMusicPlayer::GetDigitalConfig( int* pRate, int* pChannels, std::string& sName ) {

    if ( m_bDigOpen ) {
        int iFreq, iChannels;
        Uint16 uFmt;
        Mix_QuerySpec( &iFreq, &uFmt, &iChannels );
        if ( pRate ) *pRate = iFreq;
        if ( pChannels ) *pChannels = iChannels;

        const char* pDriver = SDL_GetCurrentAudioDriver();
        sName = pDriver ? pDriver : "SDL Audio";
    } else {
        if ( pRate ) *pRate = 0;
        if ( pChannels ) *pChannels = 0;
        sName = "No audio device";
    }
}
