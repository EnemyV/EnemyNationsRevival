#ifndef __ACMUTIL_H__
#define __ACMUTIL_H__

// ACM (Audio Compression Manager) is the legacy Windows audio-decode path.
// On Linux audio is SDL_mixer and nothing references these CACM* types, so the
// whole header is Windows-only (acmutil.cpp is also excluded from the Linux
// build). This avoids pulling <MSAcm.h>/<mmreg.h> on Linux.
#ifdef _WIN32

#include "stdafx.h"

#include "thielen.h"
#include <MSAcm.h>
#include <mmreg.h>

template<class S> class CACMStruct {
public:
    S FAR* operator->() { return &s; }
    const S FAR* operator->() const { return &s; }
    operator S FAR* ( ) { return &s; }
    CACMStruct() {
        memset( &s, 0, sizeof( s ) );
        s.cbStruct = sizeof( s );
    }
public:
    S s;
};

class CACMWaveFormat {
public:
    CACMWaveFormat( WORD fmt = WAVE_FORMAT_UNKNOWN, DWORD sz = 0 );
    ~CACMWaveFormat();

    LPWAVEFORMATEX  operator->() { return m_fmt; }
    operator LPWAVEFORMATEX() { return m_fmt; }

    BOOL Prepare( DWORD flags );
    DWORD Size() const {
        ASSERT( m_fmt );
        return sizeof( *m_fmt ) + m_fmt->cbSize;
    }

protected:

    static BOOL PASCAL fmtEnumCallback(
        HACMDRIVERID hadid,
        LPACMFORMATDETAILS pafd,
        DWORD_PTR dwInstance,   // x64: must match ACMFORMATENUMCB's pointer-width param
        DWORD fdwSupport );


public:
    LPWAVEFORMATEX m_fmt;
    DWORD     m_size;
};

class CACMStream {

    LPWAVEFORMATEX m_srcFmt;
    LPWAVEFORMATEX m_dstFmt;
    HACMSTREAM    m_strm;
    CACMStruct<ACMSTREAMHEADER> m_hdr;
    MMRESULT       m_mmr;

public:
    CACMStream( LPWAVEFORMATEX src, LPWAVEFORMATEX dst );
    ~CACMStream();


    void  Open();
    void   Close();
    void  Convert( LPVOID pSrc, DWORD dwSrc, LPVOID pDst = NULL, DWORD dwDst = 0,
                   DWORD dwFlags = ACM_STREAMCONVERTF_END | ACM_STREAMCONVERTF_START );

    // return the src size
    DWORD  SrcSize() const {
        ASSERT( m_strm );
        ASSERT( m_hdr->fdwStatus &
                ACMSTREAMHEADER_STATUSF_DONE );

        return m_hdr->cbSrcLengthUsed;
    }

    // return the actual size of the destination buffer
    DWORD  ResultSize() const {
        ASSERT( m_strm );
        ASSERT( m_hdr->fdwStatus &
                ACMSTREAMHEADER_STATUSF_DONE );

        return m_hdr->cbDstLengthUsed;
    }

    // return a pointer to the actual converted data
    LPVOID  ResultData() {
        ASSERT( m_strm );
        ASSERT( m_hdr->fdwStatus &
                ACMSTREAMHEADER_STATUSF_DONE );
        return m_hdr->pbDst;
    }

    void  ReleaseBuffer();

    MMRESULT LastError() const { return m_mmr; }
    void Prepare( LPVOID pSrcBuf, DWORD dwSrcSize, LPVOID pDstBuf, DWORD dwDstSize );
    void Unprepare();
};


// convert raw ADPCM data to raw PCM data
// Usage:
//  Create the object
//   Call Convert
//   Call ResultData and ResultSize to get the converted data
//  Destroy the object
//  Notes:
//   Convert may be called multiple times
//   however the converted data should be read IMMIDIATELY after
//   the invocation of Convert and the invocation of Convert 
//   should be done only after you've completely done with the
//   result data buffer
//   Also next invocations of Convert will be more efficient
//   if the input buffer pointer is always the same one
class CADPCMtoPCMConvert {
public:

    CADPCMtoPCMConvert( int iChannels, int iBits, int Rate );
    ~CADPCMtoPCMConvert();


    LPVOID ResultData() {
        ASSERT( m_stream );
        return m_stream->ResultData();
    }

    DWORD SrcSize() {
        ASSERT( m_stream );
        return m_stream->SrcSize();
    }

    DWORD ResultSize() {
        ASSERT( m_stream );
        return m_stream->ResultSize();
    }

    void  ReleaseBuffer() {
        m_stream->ReleaseBuffer();
    }

    void Convert( LPVOID inBuf, DWORD inSize, LPVOID outBuf = NULL, DWORD outSize = 0,
                  DWORD dwFlags = ACM_STREAMCONVERTF_END | ACM_STREAMCONVERTF_START ) {
        ASSERT( m_stream );
        m_stream->Convert( inBuf, inSize, outBuf, outSize, dwFlags );
    }

private:
    CACMWaveFormat m_pcmFmt;
    CACMWaveFormat m_adpcmFmt;
    CACMStream* m_stream;
};

#else  // !_WIN32 — no ACM on POSIX. Native MS-ADPCM (WAVE_FORMAT_ADPCM) decoder
       // with the same block-based Convert semantics the ACM stream had: with
       // ACM_STREAMCONVERTF_BLOCKALIGN only whole blocks are consumed (SrcSize
       // reports how much; the caller carries the remainder into the next call),
       // otherwise a trailing partial block is decoded too. The game's music and
       // most SFX in ENations.dat are comp==9 == MS-ADPCM, 4-bit, 22050 Hz —
       // the old always-empty stub here is why POSIX builds had no music.

#include <cstdint>
#include <cstdlib>

#define ACM_STREAMCONVERTF_BLOCKALIGN 0x00000004
#define ACM_STREAMCONVERTF_START      0x00000010
#define ACM_STREAMCONVERTF_END        0x00000020

class CADPCMtoPCMConvert {
public:
    CADPCMtoPCMConvert( int iChannels, int /*iBits*/, int iRate ) {
        m_iCh = ( iChannels == 2 ) ? 2 : 1;
        // Standard ACM MS-ADPCM block sizes: 256/512/1024 bytes per channel for
        // 11/22/44 kHz — the same formats acmFormatEnum picks on Windows, and
        // what the 1996 assets were encoded with.
        int perCh = iRate <= 11025 ? 256 : ( iRate <= 22050 ? 512 : 1024 );
        m_iBlockAlign = perCh * m_iCh;
        m_iSamplesPerBlock = ( ( m_iBlockAlign - 7 * m_iCh ) * 2 ) / m_iCh + 2;
        m_pResult = NULL;
        m_dwSrcUsed = m_dwDstUsed = 0;
    }

    LPVOID ResultData()  { return m_pResult; }
    DWORD  SrcSize()     { return m_dwSrcUsed; }
    DWORD  ResultSize()  { return m_dwDstUsed; }

    // Windows ACM handed ownership of an internally-malloc'd destination buffer
    // to the caller on ReleaseBuffer (LoadBuffer free()s / keeps it as m_pBuf).
    // Convert(outBuf==NULL) below mallocs a fresh buffer the same way, so
    // there is nothing to do here.
    void   ReleaseBuffer() {}

    void Convert( LPVOID inBuf, DWORD inSize, LPVOID outBuf = NULL,
                  DWORD outSize = 0,
                  DWORD dwFlags = ACM_STREAMCONVERTF_END | ACM_STREAMCONVERTF_START ) {
        const uint8_t* src = (const uint8_t*)inBuf;
        m_dwSrcUsed = m_dwDstUsed = 0;
        m_pResult = outBuf;
        if ( src == NULL || inSize == 0 )
            return;

        const bool  bBlockOnly = ( dwFlags & ACM_STREAMCONVERTF_BLOCKALIGN ) != 0;
        const DWORD dwBlockOut = (DWORD)m_iSamplesPerBlock * m_iCh * 2;
        const DWORD nFull      = inSize / m_iBlockAlign;
        const DWORD dwTail     = inSize % m_iBlockAlign;

        uint8_t* dst;
        DWORD    dwCap;
        if ( outBuf == NULL ) {
            dwCap = ( nFull + ( dwTail ? 1 : 0 ) ) * dwBlockOut + 16;
            dst = (uint8_t*)malloc( dwCap );
            if ( dst == NULL )
                return;
            m_pResult = dst;
        } else {
            dst = (uint8_t*)outBuf;
            dwCap = outSize;
        }

        DWORD dwIn = 0, dwOut = 0;
        for ( DWORD b = 0; b < nFull; b++ ) {
            if ( dwOut + dwBlockOut > dwCap )
                break;  // destination full - leave the rest unconsumed
            int iFrames = DecodeBlock( src + dwIn, m_iBlockAlign, (int16_t*)( dst + dwOut ) );
            dwIn  += m_iBlockAlign;
            dwOut += (DWORD)iFrames * m_iCh * 2;
        }

        // Final partial block: only when the caller says this is the end of the
        // stream (no BLOCKALIGN) and everything before it was consumed.
        if ( !bBlockOnly && dwTail != 0 && dwIn == nFull * (DWORD)m_iBlockAlign ) {
            if ( dwTail >= (DWORD)( 7 * m_iCh ) && dwOut + dwBlockOut <= dwCap ) {
                int iFrames = DecodeBlock( src + dwIn, (int)dwTail, (int16_t*)( dst + dwOut ) );
                dwOut += (DWORD)iFrames * m_iCh * 2;
            }
            dwIn = inSize;  // partial tail is consumed either way
        }

        m_dwSrcUsed = dwIn;
        m_dwDstUsed = dwOut;
    }

private:
    static int16_t ClampS16( int v ) {
        if ( v > 32767 )  return 32767;
        if ( v < -32768 ) return -32768;
        return (int16_t)v;
    }

    // Decode one (possibly truncated) MS-ADPCM block into interleaved S16 PCM.
    // Returns the number of complete frames (samples per channel) written.
    int DecodeBlock( const uint8_t* src, int iSrcLen, int16_t* dst ) {
        static const int aCoef1[7] = { 256, 512, 0, 192, 240, 460, 392 };
        static const int aCoef2[7] = { 0, -256, 0, 64, 0, -208, -232 };
        static const int aAdapt[16] = { 230, 230, 230, 230, 307, 409, 512, 614,
                                        768, 614, 512, 409, 307, 230, 230, 230 };
        const int ch = m_iCh;
        if ( iSrcLen < 7 * ch )
            return 0;

        int iCoef1[2], iCoef2[2], iDelta[2], iS1[2], iS2[2];
        int p = 0;
        for ( int c = 0; c < ch; c++ ) {
            int idx = src[p++];
            if ( idx > 6 ) idx = 0;
            iCoef1[c] = aCoef1[idx];
            iCoef2[c] = aCoef2[idx];
        }
        for ( int c = 0; c < ch; c++ ) { iDelta[c] = (int16_t)( src[p] | ( src[p + 1] << 8 ) ); p += 2; }
        for ( int c = 0; c < ch; c++ ) { iS1[c]    = (int16_t)( src[p] | ( src[p + 1] << 8 ) ); p += 2; }
        for ( int c = 0; c < ch; c++ ) { iS2[c]    = (int16_t)( src[p] | ( src[p + 1] << 8 ) ); p += 2; }

        // The two header samples are emitted first, oldest first
        int iOut = 0;
        for ( int c = 0; c < ch; c++ ) dst[iOut++] = (int16_t)iS2[c];
        for ( int c = 0; c < ch; c++ ) dst[iOut++] = (int16_t)iS1[c];

        int iFrames = 2;
        const int nNibbles = ( iSrcLen - p ) * 2;
        int c = 0;
        for ( int i = 0; i < nNibbles && iFrames < m_iSamplesPerBlock; i++ ) {
            int nib = ( i & 1 ) ? ( src[p + ( i >> 1 )] & 0x0F ) : ( src[p + ( i >> 1 )] >> 4 );
            int sNib = ( nib >= 8 ) ? nib - 16 : nib;

            int iPred = ( ( iS1[c] * iCoef1[c] + iS2[c] * iCoef2[c] ) >> 8 ) + sNib * iDelta[c];
            int16_t s = ClampS16( iPred );

            iS2[c] = iS1[c];
            iS1[c] = s;
            iDelta[c] = ( aAdapt[nib] * iDelta[c] ) >> 8;
            if ( iDelta[c] < 16 ) iDelta[c] = 16;

            dst[iOut++] = s;
            if ( ++c == ch ) { c = 0; iFrames++; }
        }

        // iFrames only increments when the last channel of a frame is written,
        // so a truncated stereo block ending mid-frame is not over-counted.
        return iFrames;
    }

private:
    int   m_iCh;
    int   m_iBlockAlign;
    int   m_iSamplesPerBlock;
    void* m_pResult;
    DWORD m_dwSrcUsed;
    DWORD m_dwDstUsed;
};

#endif // _WIN32

#endif
