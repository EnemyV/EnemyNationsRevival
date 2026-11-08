#include "stdafx.h"
#include <mmsystem.h>

#include "codec.h"
#include "bpecodec.h"
#include "lzsscode.h"
#include "huffmanc.h"
#include "lzwcodec.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

LZSSCoDec		lzss12_4( 12, 4 );
LZSSCoDec		lzss12_6( 12, 6 );
LZSSCoDec		lzss14_4( 14, 4 );
LZSSCoDec		lzss16_4( 16, 4 );
LZSSCoDec		lzss16_6( 16, 6 );
LZSSCoDec		lzss16_8( 16, 8 );
HuffmanCoDec	huff;
LZWCoDec		lzw;
BPECoDec		bpe;

struct
{
	const char *pDesc;
	CoDec *		pCodec;
	int			totalBytesUncompressed;
	int			totalBytesCompressed;
	int			totalDecompTimeMS;
} codecs[] =
{
	"LZSS 12/4", &lzss12_4, 0, 0, 0, 
	"LZSS 12/6", &lzss12_6, 0, 0, 0, 
	"LZSS 14/4", &lzss14_4, 0, 0, 0, 
	"LZSS 16/4", &lzss16_4, 0, 0, 0, 
	"LZSS 16/6", &lzss16_6, 0, 0, 0, 
	"Huffman", &huff, 0, 0, 0, 
	"LZW", &lzw, 0, 0, 0, 
	"LZSS 16/8", &lzss16_8, 0, 0, 0, 
	"BPE", &bpe, 0, 0, 0,
	"none", &bpe, 0, 0, 0,
};
const int nCodecCnt = sizeof( codecs ) / sizeof( codecs[ 0 ] );

int main( int argc, char *argv[] )
{
	printf( "Filename      Comp type  Old Size  New Size  Ratio   Decomp Time  CD Ratio\n" );
	printf( "------------  ---------  --------  --------  ------  -----------  --------\n" );

	int argvIndex;
	for( argvIndex = 1; argvIndex < argc; argvIndex++ )
	{
		CFile	file;
		if ( file.Open( argv[ argvIndex ], CFile::modeRead | CFile::typeBinary ) == FALSE )
		{
			fprintf( stderr, "Error opening file %s\n", argv[ argvIndex ] );
			continue;
		}

		long originalSize = file.GetLength();
		if ( originalSize == 0 )
		{
			file.Close();
			continue;
		}

		BYTE *pFileBuf = new BYTE[ originalSize ];
		if ( pFileBuf == NULL )
		{
			fprintf( stderr, "Error allocating buffer for file %s\n", argv[ argvIndex ] );
			continue;
		}

		if ( file.Read( pFileBuf, originalSize ) != ( UINT )originalSize )
		{
			fprintf( stderr, "Error reading file %s\n", argv[ argvIndex ] );
			continue;
		}

		file.Close();

		for( int codecIndex = 0; codecIndex < nCodecCnt; codecIndex++ )
		{

			int compressedSize;
			BYTE * pCompressedBuf = (BYTE *) CoDec::Compress ( (CoDec::CODEC) codecIndex, pFileBuf, originalSize, compressedSize );

#ifdef BUGBUG
			CoDec *pCodec = codecs[ codecIndex ].pCodec;

			BYTE *pCompressedBuf = ( BYTE * )pCodec->Compress( pFileBuf, originalSize, &compressedSize );
#endif
			if ( pCompressedBuf == NULL )
			{
				fprintf( stderr, "Error compressing file %s\n", argv[ argvIndex ] );
				continue;
			}

			DWORD startTime = timeGetTime();
			int uncompressedSize;

			BYTE *pUncompressedBuf = ( BYTE * ) CoDec::Decompress ( pCompressedBuf, compressedSize, uncompressedSize );

#ifdef BUGBUG
			BYTE *pUncompressedBuf = ( BYTE * )pCodec->Decompress( pCompressedBuf, compressedSize, &uncompressedSize );
#endif

			if ( pUncompressedBuf == NULL )
			{
				fprintf( stderr, "Error uncompressing file %s\n", argv[ argvIndex ] );
				continue;
			}
			DWORD decompTimeMS = timeGetTime() - startTime;

			if ( uncompressedSize != originalSize ||
				memcmp( pFileBuf, pUncompressedBuf, originalSize ) != 0 )
			{
				fprintf( stderr, "Error!  Original and decompressed files not identical!  File %s\n", argv[ argvIndex ] );
				continue;
			}

			CoDec::FreeBuf ( pCompressedBuf );
			CoDec::FreeBuf ( pUncompressedBuf );

			codecs[ codecIndex ].totalBytesUncompressed += uncompressedSize;
			codecs[ codecIndex ].totalBytesCompressed += compressedSize;
			codecs[ codecIndex ].totalDecompTimeMS += decompTimeMS;

			float ratio = ( 1.0f - ( float )compressedSize / ( float )originalSize ) * 100.0f;
			float uncompressedLoadTimeMS = ( float )originalSize / 300.0f;	//  Double-speed CD is 300K/s, or 300 bytes/ms.
			float compressedLoadTimeMS = ( float )compressedSize / 300.0f + decompTimeMS;
			float cdLoadRatio = ( 1.0f - compressedLoadTimeMS / uncompressedLoadTimeMS ) * 100.0f;
			printf( "%-12s  %-9s  %8d  %8d  %5.2f%%  %11d  %5.2f%%\n", argv[ argvIndex ], codecs[ codecIndex ].pDesc, originalSize, compressedSize, ratio, decompTimeMS, cdLoadRatio );
		}

		delete[] pFileBuf;

	}

	printf( "\n\nFinal Statistics\n----------------\n" );
	printf( "Comp type  Old Size  New Size  Ratio   Decomp Time  CD Ratio\n" );
	printf( "---------  --------  --------  ------  -----------  --------\n" );

	for( int codecIndex = 0; codecIndex < nCodecCnt; codecIndex++ )
	{
		float ratio = ( 1.0f - ( float )codecs[ codecIndex ].totalBytesCompressed / ( float )codecs[ codecIndex ].totalBytesUncompressed ) * 100.0f;
		float uncompressedLoadTimeMS = ( float )codecs[ codecIndex ].totalBytesUncompressed / 300.0f;	//  Double-speed CD is 300K/s, or 300 bytes/ms.
		float compressedLoadTimeMS = ( float )codecs[ codecIndex ].totalBytesCompressed / 300.0f + codecs[ codecIndex ].totalDecompTimeMS;
		float cdLoadRatio = ( 1.0f - compressedLoadTimeMS / uncompressedLoadTimeMS ) * 100.0f;
		printf( "%-9s  %8d  %8d  %5.2f%%  %11d  %5.2f%%\n", codecs[ codecIndex ].pDesc, codecs[ codecIndex ].totalBytesUncompressed, codecs[ codecIndex ].totalBytesCompressed, ratio, codecs[ codecIndex ].totalDecompTimeMS, cdLoadRatio );
	}

	return 0;
}