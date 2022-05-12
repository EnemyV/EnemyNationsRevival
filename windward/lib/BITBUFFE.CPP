#include "stdafx.h"

#include "bitbuffe.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

BitBuffer::BitBuffer()
{
	_pBuf = NULL;
	_iBufSize = 0;
	_iCurrentBufPos = 1;	//  Larger than iBufSize, to force ASSERT if not attached before using.
	_bMaskByte = 0;
	_bbmMode = bbInvalid;
}

BitBuffer::~BitBuffer()
{
}

void BitBuffer::AttachForReading( const void *pBuf, int iBufSize )
{
	ASSERT( pBuf != NULL );
	ASSERT( 0 < iBufSize );

	_pBuf = ( BYTE * )pBuf;
	_iBufSize = iBufSize;
	_iCurrentBufPos = 0;
	_bbmMode = bbRead;

	_bDataByte = 0;

	//  Initial mask needs to be 0 for reading.
	_bMaskByte = 0;
}

void BitBuffer::AttachForWriting( void *pBuf, int iBufSize )
{
	ASSERT( pBuf != NULL );
	ASSERT( 0 < iBufSize );

	_pBuf = ( BYTE * )pBuf;
	_iBufSize = iBufSize;
	_iCurrentBufPos = 0;
	_bbmMode = bbWrite;

	_bDataByte = 0;

	//  Initial mask needs to be 0x80 for writing.
	_bMaskByte = 0x80;
}

void BitBuffer::PutBit( int iBit )
{
	ASSERT( _bbmMode == bbWrite );

	if ( iBit )
		_bDataByte |= _bMaskByte;

	_bMaskByte >>= 1;
	if ( _bMaskByte == 0 )
	{
		ASSERT( _iCurrentBufPos < _iBufSize );
		_pBuf[ _iCurrentBufPos++ ] = _bDataByte;
		_bDataByte = 0;
		_bMaskByte = 0x80;
	}
}

void BitBuffer::PutBits( DWORD dwBits, int iBitCount )
{
	ASSERT( _bbmMode == bbWrite );
	ASSERT( 0 <= iBitCount && iBitCount <= 32 );

	DWORD	dwMask = 1 << ( iBitCount - 1 );
	while( dwMask )
	{
		if ( dwMask & dwBits )
			_bDataByte |= _bMaskByte;

		_bMaskByte >>= 1;
		if ( _bMaskByte == 0 )
		{
			ASSERT( _iCurrentBufPos < _iBufSize );
			_pBuf[ _iCurrentBufPos++ ] = _bDataByte;
			_bDataByte = 0;
			_bMaskByte = 0x80;
		}

		dwMask >>= 1;
	}
}

//  Flushes any remaining bits out to the buffer.
void BitBuffer::Flush()
{
	ASSERT( _bbmMode == bbWrite );

	if ( _bMaskByte != 0x80 )
	{
		ASSERT( _iCurrentBufPos < _iBufSize );
		_pBuf[ _iCurrentBufPos++ ] = _bDataByte;
		_bDataByte = 0;
		_bMaskByte = 0x80;
	}

}

int BitBuffer::GetSizeInBytes() const
{
	ASSERT( _bbmMode == bbWrite );
	
	return _iCurrentBufPos + ( _bMaskByte != 0x80 ? 1 : 0 );
}

int BitBuffer::GetBit()
{
	ASSERT( _bbmMode == bbRead );

	if ( _bMaskByte == 0 )
	{
		ASSERT( _iCurrentBufPos < _iBufSize );
		_bDataByte =_pBuf[ _iCurrentBufPos++ ];
		_bMaskByte = 0x80;
	}

	int iBit = _bDataByte & _bMaskByte;

	_bMaskByte >>= 1;

	return iBit ? 1 : 0;
}

DWORD BitBuffer::GetBits( int iBitCount )
{
	ASSERT( _bbmMode == bbRead );
	ASSERT( 0 < iBitCount && iBitCount <= 32 );

	DWORD	dwReturnBits = 0;
	DWORD	dwMask = 1 << ( iBitCount - 1 );
	while( dwMask )
	{
		if ( _bMaskByte == 0 )
		{
			ASSERT( _iCurrentBufPos < _iBufSize );
			_bDataByte = _pBuf[ _iCurrentBufPos++ ];
			_bMaskByte = 0x80;
		}

		if ( _bMaskByte & _bDataByte )
			dwReturnBits |= dwMask;

		_bMaskByte >>= 1;

		dwMask >>= 1;
	}

	return dwReturnBits;
}
