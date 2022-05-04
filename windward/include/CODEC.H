#ifndef CODEC_H
#define CODEC_H

#include "thielen.h"

typedef void (* FNCOMPSTAT) (DWORD dwData, int iBlk);

class CoDec
{
	public :

	enum CODEC { LZSS_12_4, LZSS_12_6, LZSS_14_4, LZSS_16_4, LZSS_16_6, HUFFMAN, LZW, LZSS_16_8, BPE, NONE,
							VFX_8 = BPE, VFX_16 = BPE, SFX_8 = BPE, SFX_16 = BPE, MUS_8 = NONE, MUS_16 = BPE,
							SPRITE_8 = BPE, SPRITE_24 = NONE, GAME = BPE };

	// for compress only
	static int BufSize ( int iUnCompSize ) { return ( 3 * iUnCompSize ) / 2 + 1; }

	// for decompress only
	static CODEC BufType ( void const * pBuf ) { return (CODEC) * ((BYTE *) pBuf); }
	static int BufLength ( void const * pBuf ) { return * ((int *) (((BYTE *) pBuf) + 1)); }

	static void * Compress ( CODEC typ, void const * pSrc, int iSrcSize, int & iDestSize, FNCOMPSTAT fnStat = NULL, DWORD dwData = NULL);
	static void * Decompress ( void const * pSrc, int iSrcSize, int & iDestSize, FNCOMPSTAT fnStat = NULL, DWORD dwData = NULL);

	// give it a pointer from AllocBuf
	static void Compress ( CODEC typ, void const * pSrc, int iSrcSize, void * pDest, int & iDestSize, FNCOMPSTAT fnStat = NULL, DWORD dwData = NULL);
	static void Decompress ( void const * pSrc, int iSrcSize, void * pDest, int & iDestSize, FNCOMPSTAT fnStat = NULL, DWORD dwData = NULL);

	static void * AllocBuf ( int iSize ) { return malloc ( iSize ); }
	static void FreeBuf ( void * pBuf ) { free ( pBuf ); }

	CoDec() {}
	virtual ~CoDec() {}

	virtual void *Compress( const void *pUncompressedBuffer, int iUncompressedBufSize, void *pCompressedBuffer, int *piCompressedBufSize, FNCOMPSTAT fnStat, DWORD dwData ) PURE_FUNC
	virtual void *Decompress( const void *pCompressedBuffer, int iCompressedBufSize, void *pUncompressedBuffer, int *piUncompressedBufSize, FNCOMPSTAT fnStat, DWORD dwData ) PURE_FUNC
};

#endif
