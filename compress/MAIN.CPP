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

	printf ( "Compress version 1.00.001 %s %s\n", __DATE__, __TIME__);

	if ((argc != 3) && (argc != 4))
		{
		printf ( "usuage: type, inFile, [outFile]\n" );
		printf ( "           1  LZSS 12/4\n" );
		printf ( "           2  LZSS 12/6\n" );
		printf ( "           3  LZSS 14/4\n" );
		printf ( "           4  LZSS 16/4\n" );
		printf ( "           5  LZSS 16/6\n" );
		printf ( "           6  Huffman\n" );
		printf ( "           7  LZW\n" );
		printf ( "           8  LZSS 16/8\n" );
		printf ( "           9  BPE\n" );
		return (0);
		}

	int iTyp = atoi ( argv [1] );
	CString sInFile ( argv [2] );
	CString sOutFile;
	if (argc == 4)
		sOutFile = argv [3];
	else
		{
		sOutFile = sInFile;
		sOutFile.ReleaseBuffer ( sOutFile.ReverseFind ('.') );
		sOutFile += ".cmp";
		}

	if ((iTyp < 1) || (9 < iTyp))
		{
		printf ("illegal type %d\n", iTyp);
		return (0);
		}

	printf ("compressing [%d] %s to %s\n", iTyp, (char const *) sInFile, (char const *) sOutFile );

	CFile	file;
	if ( file.Open( sInFile, CFile::modeRead | CFile::typeBinary ) == FALSE )
		{
		printf ( "Error opening file %s\n", (char const *) sInFile );
		return (0);
		}

	long originalSize = file.GetLength();
	if ( originalSize == 0 )
		{
		printf ( "file 0 bytes %s\n", (char const *) sInFile );
		file.Close();
		return (0);
		}

	BYTE *pFileBuf = new BYTE[ originalSize ];
	if ( pFileBuf == NULL )
		{
		printf ( "memory allocation error\n");
		file.Close();
		return (0);
		}

	if ( file.Read( pFileBuf, originalSize ) != ( UINT )originalSize )
		{
		printf ( "error reading %s\n", (char const *) sInFile );
		file.Close();
		return (0);
		}

	file.Close();

	int compressedSize;
	BYTE * pCompressedBuf = (BYTE *) CoDec::Compress ( (CoDec::CODEC) (iTyp - 1), pFileBuf, originalSize, compressedSize );

	if ( pCompressedBuf == NULL )
		{
		printf ( "Error compressing file %s\n", (char const *) sInFile );
		file.Close();
		return (0);
		}

	free (pFileBuf);

	if ( file.Open( sOutFile, CFile::modeWrite | CFile::typeBinary | CFile::modeCreate ) == FALSE )
		{
		printf ( "Error opening file %s\n", (char const *) sOutFile );
		return (0);
		}

	file.Write( pCompressedBuf, compressedSize );

	file.Close();
	CoDec::FreeBuf ( pCompressedBuf );

	return 0;
}