#ifndef BITBUFFER_H
#define BITBUFFER_H

class BitBuffer
{
	private :

	enum BitBufferMode { bbInvalid, bbRead, bbWrite };

	BYTE *			_pBuf;
	int				_iBufSize;
	int				_iCurrentBufPos;
	BitBufferMode	_bbmMode;

	BYTE			_bMaskByte;
	BYTE			_bDataByte;

	public :

	BitBuffer();
	~BitBuffer();

	void AttachForReading( const void *pBuf, int bufSize );
	void AttachForWriting( void *pBuf, int bufSize );

	void PutBit( int iBit );
	void PutBits( DWORD dwBits, int iBitCount );
	void Flush();
	int GetSizeInBytes() const;

	int GetBit();
	DWORD GetBits( int iBitCount );
};

#endif
