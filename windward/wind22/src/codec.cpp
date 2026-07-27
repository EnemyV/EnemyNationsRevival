

#include "codec.h"

#include "_windwrd.h"
#include "bpecodec.h"
#include "huffmanc.h"
#include "lzsscode.h"
#include "lzwcodec.h"
#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

LZSSCoDec    _lzss12_4( 12, 4 );
LZSSCoDec    _lzss12_6( 12, 6 );
LZSSCoDec    _lzss14_4( 14, 4 );
LZSSCoDec    _lzss16_4( 16, 4 );
LZSSCoDec    _lzss16_6( 16, 6 );
LZSSCoDec    _lzss16_8( 16, 8 );
HuffmanCoDec _huff;
LZWCoDec     _lzw;
BPECoDec     _bpe;

struct
{
    CoDec* pCodec;
} _codecs[] = {
    &_lzss12_4, &_lzss12_6, &_lzss14_4, &_lzss16_4, &_lzss16_6, &_huff, &_lzw, &_lzss16_8, &_bpe,
};

const int nCodecCnt = sizeof( _codecs ) / sizeof( _codecs[0] );

void* CoDec::Compress( CODEC typ, void const* pSrc, int iSrcSize, int& iDestSize, FNCOMPSTAT fnStat, DWORD_PTR dwData )
{

    char* pDest;
    auto int_typ = static_cast<int>( typ );

    if ( ( 0 <= int_typ ) && ( int_typ < static_cast<int>( CODEC::NONE ) ) )
    {
        CoDec* pCodec = _codecs[int_typ].pCodec;

        try
        {
            pDest = ( (char*)pCodec->Compress( pSrc, iSrcSize, nullptr, &iDestSize, fnStat, dwData ) );
        }
        catch ( ... )
        {
            // force to uncompressed
            iDestSize = iSrcSize;
            pDest     = nullptr;
        }

        if ( iDestSize < iSrcSize - iSrcSize / 8 )
            return pDest;
    }
    else
        pDest = nullptr;

    // ain't worth it
    if ( pDest == nullptr )
        pDest = (char*)malloc( iSrcSize + 5 );
    else
        pDest = (char*)realloc( pDest, iSrcSize + 5 );

    *pDest                   = static_cast<BYTE>( CODEC::NONE );
    *( (int*)( pDest + 1 ) ) = iSrcSize;

    memcpy( pDest + 5, pSrc, iSrcSize );

    iDestSize = iSrcSize + 5;

    return pDest;
}

void* CoDec::Decompress( void const* pSrc, int iSrcSize, int& iDestSize, FNCOMPSTAT fnStat, DWORD_PTR dwData )
{

    auto pcSrc = static_cast<char const*>( pSrc );
    auto typ   = static_cast<CODEC>( *pcSrc );

    // see if none
    if ( ( static_cast<int>( typ ) < 0 ) || ( static_cast<int>( CODEC::NONE ) <= static_cast<int>( typ ) ) )
    {
        char* pDest = static_cast<char*>( malloc( iSrcSize - 5 ) );
        ASSERT( pDest != 0 );
        memcpy( pDest, pcSrc + 5, iSrcSize - 5 );

        iDestSize = iSrcSize - 5;
        return ( pDest );
    }

    CoDec* pCodec = _codecs[static_cast<int>( typ )].pCodec;

    return ( (void*)pCodec->Decompress( pcSrc + 1, iSrcSize - 1, NULL, &iDestSize, fnStat, dwData ) );
}

void CoDec::Decompress( void const* pSrc, int iSrcSize, void* pDest, int& iDestSize, FNCOMPSTAT fnStat, DWORD_PTR dwData )
{

    auto pcSrc = static_cast<char const*>( pSrc );
    auto typ   = static_cast<CODEC>( *pcSrc );

    // see if none
    if ( ( static_cast<int>( typ ) < 0 ) || ( static_cast<int>( CODEC::NONE ) <= static_cast<int>( typ ) ) )
    {
        if ( pDest == NULL )
            pDest = (char*)malloc( iSrcSize - 5 );
        memcpy( pDest, pcSrc + 5, iSrcSize - 5 );

        iDestSize = iSrcSize - 5;
        return;
    }

    CoDec* pCodec = _codecs[static_cast<int>( typ )].pCodec;

    pCodec->Decompress( pcSrc + 1, iSrcSize - 1, pDest, &iDestSize, fnStat, dwData );
}
