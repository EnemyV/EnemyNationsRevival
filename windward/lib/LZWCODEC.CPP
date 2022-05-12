#include "stdafx.h"

#include "lzwcodec.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

void LZWCoDec::InitializeDictionary()
{
	int tableIndex;
	for( tableIndex = 0; tableIndex < iTABLE_SIZE; tableIndex++ )
		dictionary[ tableIndex ].iCodeValue = iUNUSED;

	dwNextCode = iFIRST_CODE;
	iCurrentCodeBits = 9;
	dwNextBumpCode = 511;
}

void *LZWCoDec::Compress( const void *pUncompressedBuffer, int iUncompressedBufSize, void *pCompressedBuffer, int *piCompressedBufSize, FNCOMPSTAT fnStat, DWORD dwData )
{
	ASSERT( pUncompressedBuffer );
	ASSERT( 0 < iUncompressedBufSize );
	ASSERT( piCompressedBufSize );

	//  We want to use this as a BYTE array, not a void *.
	BYTE *pUncompBuf = ( BYTE * )pUncompressedBuffer;

	*piCompressedBufSize = 3 * iUncompressedBufSize / 2;
	if ( pCompressedBuffer == NULL )
		pCompressedBuffer = malloc ( 1 + *piCompressedBufSize );
	if ( pCompressedBuffer == NULL )
		throw( "Not enough memory" );
	bitBuffer.AttachForWriting( ((char *)pCompressedBuffer) + 1, *piCompressedBufSize );

	bitBuffer.PutBits( ( DWORD )iUncompressedBufSize, 32 );

	InitializeDictionary();
	int stringCode = iEND_OF_STREAM;
	if ( iUncompressedBufSize != 0 )
	{
		stringCode = *pUncompBuf++;
		iUncompressedBufSize--;
	}

	while( iUncompressedBufSize != 0 )
	{
		int character = *pUncompBuf++;
		iUncompressedBufSize--;

		DWORD index = FindChildNode( stringCode, character );
		if ( dictionary[ index ].iCodeValue != -1 )
		{
			stringCode = dictionary[ index ].iCodeValue;
		}
		else
		{
			dictionary[ index ].iCodeValue = dwNextCode++;
			dictionary[ index ].iParentCode = stringCode;
			dictionary[ index ].cChar = ( char )character;

			bitBuffer.PutBits( stringCode, iCurrentCodeBits );

			stringCode = character;
			if ( dwNextCode > iMAX_CODE )
			{
				bitBuffer.PutBits( iFLUSH_CODE, iCurrentCodeBits );
				InitializeDictionary();
			}
			else if ( dwNextCode > dwNextBumpCode )
			{
				bitBuffer.PutBits( iBUMP_CODE, iCurrentCodeBits );
				iCurrentCodeBits++;
				dwNextBumpCode <<= 1;
				dwNextBumpCode |= 1;
			}
		}
	}

	bitBuffer.PutBits( stringCode, iCurrentCodeBits );
	bitBuffer.PutBits( iEND_OF_STREAM, iCurrentCodeBits );
	bitBuffer.Flush();

	*piCompressedBufSize = bitBuffer.GetSizeInBytes() + 1;

	* ((char *) pCompressedBuffer) = (char) CODEC::LZW;
	pCompressedBuffer = realloc ( pCompressedBuffer, bitBuffer.GetSizeInBytes() + 1 );

	return pCompressedBuffer;
}

void *LZWCoDec::Decompress( const void *pCompressedBuffer, int iCompressedBufSize, void *pUncompressedBuf, int *piUncompressedBufSize, FNCOMPSTAT fnStat, DWORD dwData )
{
	ASSERT( pCompressedBuffer );
	ASSERT( 0 < iCompressedBufSize );
	ASSERT( piUncompressedBufSize );

	bitBuffer.AttachForReading( pCompressedBuffer, iCompressedBufSize );

	*piUncompressedBufSize = ( int )bitBuffer.GetBits( 32 );
	if ( pUncompressedBuf == NULL )
		pUncompressedBuf = malloc ( *piUncompressedBufSize );
	if ( pUncompressedBuf == NULL )
		throw( "Not enough memory" );
	BYTE *pBufPos = ( BYTE * )pUncompressedBuf;

	for( ;; )
	{
		InitializeDictionary();
		DWORD oldCode = bitBuffer.GetBits( iCurrentCodeBits );
		if ( oldCode == iEND_OF_STREAM )
			goto DecompDone;
		int character = oldCode;

		*pBufPos++ = ( BYTE )oldCode;

		for( ;; )
		{
			DWORD newCode = bitBuffer.GetBits( iCurrentCodeBits );
			if ( newCode == iEND_OF_STREAM )
				goto DecompDone;
			else if ( newCode == iFLUSH_CODE )
				break;
			else if ( newCode == iBUMP_CODE )
			{
				iCurrentCodeBits++;
				continue;
			}

			int count;
			if ( newCode >= dwNextCode )
			{
				decodeStack[ 0 ] = ( char )character;
				count = DecodeString( 1, oldCode );
			}
			else
				count = DecodeString( 0, newCode );
			character = decodeStack[ count - 1 ];
			while( count > 0 )
				*pBufPos++ = decodeStack[ --count ];

			dictionary[ dwNextCode ].iParentCode = oldCode;
			dictionary[ dwNextCode ].cChar = ( char )character;
			dwNextCode++;
			oldCode = newCode;
		}
	}

DecompDone:;

	return pUncompressedBuf;
}

DWORD LZWCoDec::FindChildNode( int parentCode, int childCharacter )
{
	DWORD index;
	DWORD offset;

	index = ( childCharacter << ( iMAX_CODE_BITS - 8 ) ) ^ parentCode;
	if ( index == 0 )
		offset = 1;
	else
		offset = iTABLE_SIZE - index;

	for( ;; )
	{
		if ( dictionary[ index ].iCodeValue == iUNUSED )
			return index;
		if ( dictionary[ index ].iParentCode == parentCode && 
			dictionary[ index ].cChar == ( char )childCharacter )
			return index;
		if ( index >= offset )
			index -= offset;
		else
			index += iTABLE_SIZE - offset;
	}
}

DWORD LZWCoDec::DecodeString( DWORD count, DWORD code )
{
	while( code > 255 )
	{
		decodeStack[ count++ ] = dictionary[ code ].cChar;
		code = dictionary[ code ].iParentCode;
	}

	decodeStack[ count++ ] = ( char )code;
	return count;
}
