#ifndef BPECODEC_H
#define BPECODEC_H

#include "codec.h"

class BPECoDec : public CoDec
{
	enum
	{
		BLOCKSIZE = 5000,	//  Max block size
		HASHSIZE = 4096,	//  Size of hash table
		MAXCHARS = 200,		//  Char set per block
		THRESHOLD = 3		//  Minimum pair count
	};

	unsigned char buffer[ BLOCKSIZE ];	//  Data block
	unsigned char leftcode[ 256 ];		//  Pair table
	unsigned char rightcode[ 256 ];		//  Pair table
	unsigned char left[ HASHSIZE ];		//  Hash table
	unsigned char right[ HASHSIZE ];	//  Hash table
	unsigned char count[ HASHSIZE ];	//  Pair count
	int size;	//  Size of current data block

	BYTE *	pInputBuf;
	int		nBytesLeftInInputBuf;
	BYTE *	pOutputBuf;
	int		nBytesInOutputBuf;

	int Lookup( unsigned char, unsigned char );
	int GetNextBlock();
	void PutCompressedBlock();

	public :

	BPECoDec();
	virtual ~BPECoDec();

	virtual void *Compress( const void *pUncompressedBuffer, int iUncompressedBufSize, int *piCompressedBufSize );
	virtual void *Decompress( const void *pCompressedBuffer, int iCompressedBufSize, int *piUncompressedBufSize );
};

#endif  //  BPECODEC_H
