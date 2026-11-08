#ifndef CODEC_H
#define CODEC_H

class CoDec
{
	public :

	enum CODEC { LZSS_12_4, LZSS_12_6, LZSS_14_4, LZSS_16_4, LZSS_16_6, HUFFMAN, LZW, LZSS_16_8, BPE, NONE,
							VFX_8 = BPE, VFX_16 = BPE, SFX_8 = BPE, SFX_16 = BPE, MUS_8 = NONE, MUS_16 = BPE,
							SPRITE_8 = BPE, SPRITE_24 = NONE, GAME = BPE };

	static void * Compress ( CODEC typ, void const * pSrc, int iSrcSize, int & iDestSize );
	static void * Decompress ( void const * pSrc, int iSrcSize, int & iDestSize );
	static void FreeBuf ( void * pBuf ) { free ( pBuf ); }

	CoDec() {}
	virtual ~CoDec() {}

	virtual void *Compress( const void *pUncompressedBuffer, int iUncompressedBufSize, int *piCompressedBufSize ) = 0;
	virtual void *Decompress( const void *pCompressedBuffer, int iCompressedBufSize, int *piUncompressedBufSize ) = 0;
};

#endif
