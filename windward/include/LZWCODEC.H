#ifndef LZWCODEC_H
#define LZWCODEC_H

#include "bitbuffe.h"
#include "codec.h"

class LZWCoDec : public CoDec
{
	enum
	{
		iMAX_CODE_BITS = 15,
		iMAX_CODE = 32767,		// ( ( 1 << iMAX_CODE_BITS ) - 1 )
		iTABLE_SIZE = 35023,
		iEND_OF_STREAM = 256,
		iBUMP_CODE = 257,
		iFLUSH_CODE = 258,
		iFIRST_CODE = 259,
		iUNUSED = -1
	};

	DWORD	FindChildNode( int parentCode, int childCharacter );
	DWORD	DecodeString( DWORD offset, DWORD code );

	struct dictionary
	{
		int		iCodeValue;
		int		iParentCode;
		char	cChar;
	} dictionary[ iTABLE_SIZE ];

	char	decodeStack[ iTABLE_SIZE ];
	DWORD	dwNextCode;
	int		iCurrentCodeBits;
	DWORD	dwNextBumpCode;

	BitBuffer	bitBuffer;

	void InitializeDictionary();

	public :

	LZWCoDec() {}
	virtual ~LZWCoDec() {}

	virtual void *Compress( const void *pUncompressedBuffer, int iUncompressedBufSize, void * pCb, int *piCompressedBufSize, FNCOMPSTAT fnStat, DWORD dwData );
	virtual void *Decompress( const void *pCompressedBuffer, int iCompressedBufSize, void * pUb, int *piUncompressedBufSize, FNCOMPSTAT fnStat, DWORD dwData );
};

#endif	//  LZWCODEC_H
