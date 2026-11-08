#include "stdafx.h"

#include "bpecodec.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

BPECoDec::BPECoDec()
{
}

BPECoDec::~BPECoDec()
{
}

/***************************************************
 *
 *  Compression section
 *
 ***************************************************/

//  Return index of character pair in hash table.
//  Deleted nodes have a count of 1 for hashing
int BPECoDec::Lookup( unsigned char a, unsigned char b )
{
	int index;

	//  Compute hash key from both characters
	index = ( a ^ ( b << 5 ) ) & ( HASHSIZE - 1 );

	//  Search for pair or first empty slot
	int iLimit = HASHSIZE + 2;
	while( ( left[ index ] != a || right[ index ] != b ) &&
		count[ index ] != 0 )
		{
		index = ( index + 1 ) & ( HASHSIZE - 1 );
		if (iLimit-- < 0)
			throw( "infinite loop" );
		}

	//  Store pair in table
	left[ index ] = a;
	right[ index ] = b;

	return index;
}

//  Read next block from input file into buffer
int BPECoDec::GetNextBlock()
{
	int c, index, used = 0;

	//  Reset hash table and pair table
	for( c = 0; c < HASHSIZE; c++ )
		count[ c ] = 0;
	for( c = 0; c < 256; c++ )
	{
		leftcode[ c ] = ( unsigned char )c;
		rightcode[ c ] = 0;
	}
	size = 0;

	//  Read data until full or few unused chars
	while( size < BLOCKSIZE && 
		used < MAXCHARS && 
		nBytesLeftInInputBuf > 0 )
	{
		nBytesLeftInInputBuf--;
		c = *pInputBuf++;

		if ( size > 0 )
		{
			index = Lookup( buffer[ size - 1 ], ( unsigned char )c );
			if ( count[ index ] < 255 )
				++count[ index ];
		}
		buffer[ size++ ] = ( unsigned char )c;

		//  Use rightcode to flag data chars found
		if ( rightcode[ c ] == 0 )
		{
			rightcode[ c ] = 1;
			++used;
		}
	}

	return nBytesLeftInInputBuf == 0;
}

//  Write each pair table and data block to output
void BPECoDec::PutCompressedBlock()
{
	int i, len, c = 0;

	//  For each character 0..255
	while( c < 256 )
	{
		//  If not a pair code, count run of literals
		if ( c == leftcode[ c ] )
		{
			len = 1;
			c++;
			while( len < 127 && c < 256 && c == leftcode[ c ] )
			{
				len++;
				c++;
			}

			*pOutputBuf++ = ( BYTE )( len + 127 );
			nBytesInOutputBuf++;
			len = 0;
			if ( c == 256 )
				break;
		}
		//  else count run of pair codes
		else
		{
			len = 0;
			c++;
			while( ( len < 127 && c < 256 && c != leftcode[ c ] ) || 
				( len < 125 && c < 254 && c + 1 != leftcode[ c + 1 ] ) )
			{
				len++;
				c++;
			}

			*pOutputBuf++ = ( BYTE )len;
			nBytesInOutputBuf++;
			c -= len + 1;
		}

		//  Write range of pairs to output
		for( i = 0; i <= len; i++ )
		{
			*pOutputBuf++ = leftcode[ c ];
			nBytesInOutputBuf++;
			if ( c != leftcode[ c ] )
			{
				*pOutputBuf++ = rightcode[ c ];
				nBytesInOutputBuf++;
			}
			c++;
		}
	}

	//  Write byte sizes and compressed data block
	*pOutputBuf++ = ( BYTE )( size / 256 );
	*pOutputBuf++ = ( BYTE )( size % 256 );
	memcpy( pOutputBuf, buffer, size );
	pOutputBuf += size;
	nBytesInOutputBuf += size + 2;
}

//  Compress from input file to output file
void *BPECoDec::Compress( const void *pUncompressedBuffer, int iUncompressedBufSize, int *piCompressedBufSize )
{
	ASSERT( pUncompressedBuffer );
	ASSERT( 0 < iUncompressedBufSize );
	ASSERT( piCompressedBufSize );

	*piCompressedBufSize = 3 * iUncompressedBufSize / 2;
	BYTE * pCompressedBuffer = (BYTE *) malloc (  1 + *piCompressedBufSize );
	if ( pCompressedBuffer == NULL )
		throw( "Not enough memory" );

	pInputBuf = ( BYTE * )pUncompressedBuffer;
	nBytesLeftInInputBuf = iUncompressedBufSize;
	pOutputBuf = pCompressedBuffer + 1;

	*( ( int * )pOutputBuf ) = iUncompressedBufSize;
	pOutputBuf += 4;
	nBytesInOutputBuf = 4;

	int leftch, rightch, code, oldsize;
	int index, r, w, best, done = 0;

	//  Compress each data block until end of file
	while( !done )
	{
		done = GetNextBlock();
		code = 256;

		//  Compress this block
		for( ;; )
		{
			//  Get next unused char for pair code
			for( code--; code >= 0; code-- )
			{
				if ( code == leftcode[ code ] && !rightcode[ code ] )
					break;
			}

			//  Must quit if no unused chars left
			if ( code < 0 )
				break;

			//  Find most frequent pair of chars
			for( best = 2, index = 0;
				index < HASHSIZE;
				index++ )
			{
				if ( count[ index ] > best )
				{
					best = count[ index ];
					leftch = left[ index ];
					rightch = right[ index ];
				}
			}

			//  Done if no more compression possible
			if ( best < THRESHOLD )
				break;

			//  Replace pairs in data, adjust pair counts
			oldsize = size - 1;
			for( w = 0, r = 0; r < oldsize; r++ )
			{
				if ( buffer[ r ] == leftch && buffer[ r + 1 ] == rightch )
				{
					if ( r > 0 )
					{
						index = Lookup( buffer[ w - 1 ], ( unsigned char )leftch );
						if ( count[ index ] > 1 )
							--count[ index ];
						index = Lookup( buffer[ w - 1 ], ( unsigned char )code );
						if ( count[ index ] < 255 )
							++count[ index ];
					}
					if ( r < oldsize - 1 )
					{
						index = Lookup( ( unsigned char )rightch, buffer[ r + 2 ] );
						if ( count[ index ] > 1 )
							--count[ index ];
						index = Lookup( ( unsigned char )code, buffer[ r + 2 ] );
						if ( count[ index ] < 255 )
							++count[ index ];
					}
					buffer[ w++ ] = ( unsigned char )code;
					r++;
					size--;
				}
				else
					buffer[ w++ ] = buffer[ r ];
			}
			buffer[ w ] = buffer[ r ];

			//  Add to pair substitution table
			leftcode[ code ] = ( unsigned char )leftch;
			rightcode[ code ] = ( unsigned char )rightch;

			//  Delete pair from hash table
			index = Lookup( ( unsigned char )leftch, ( unsigned char )rightch );
			count[ index ] = 1;
		}

		PutCompressedBlock();
	}

	*piCompressedBufSize = nBytesInOutputBuf + 1;

	* pCompressedBuffer = CODEC::BPE;
	pCompressedBuffer = (BYTE *) realloc ( pCompressedBuffer, nBytesInOutputBuf + 1 );

	return pCompressedBuffer;
}

/***************************************************
 *
 *  Decompression section
 *
 ***************************************************/
//  Decompress data from input to output
void *BPECoDec::Decompress( const void *pCompressedBuffer, int iCompressedBufSize, int *piUncompressedBufSize )
{
	ASSERT( pCompressedBuffer );
	ASSERT( 0 < iCompressedBufSize );
	ASSERT( piUncompressedBufSize );

	pInputBuf = ( BYTE * )pCompressedBuffer;

	*piUncompressedBufSize = *( ( int * )pInputBuf );
	pInputBuf += 4;
	iCompressedBufSize -= 4;
	void *pUncompressedBuf = malloc ( *piUncompressedBufSize );
	if ( pUncompressedBuf == NULL )
		throw( "Not enough memory" );

	pOutputBuf = ( BYTE * )pUncompressedBuf;

	unsigned char left[ 256 ], right[ 256 ], stack[ 30 ];
	int c, count, i, size;

	//  Unpack each block until end of file.
	while( iCompressedBufSize > 0 )
	{
		count = *pInputBuf++;
		iCompressedBufSize--;

		//  Set left to itself as literal flag
		for( i = 0; i < 256; i++ )
			left[ i ] = ( unsigned char )i;

		//  Read pair table
		for( c = 0; ; )
		{
			//  Skip range of literal bytes
			if ( count > 127 )
			{
				c += count - 127;
				count = 0;
			}
			if ( c == 256 )
				break;

			//  Read pairs, skip right if literal
			for( i = 0; i <= count; i++, c++ )
			{
				left[ c ] = *pInputBuf++;
				iCompressedBufSize--;
				if ( c != left[ c ] )
				{
					right[ c ] = *pInputBuf++;
					iCompressedBufSize--;
				}
			}
			if ( c == 256 )
				break;
			count = *pInputBuf++;
			iCompressedBufSize--;
		}

		//  Calculate packed data block size
		size = 256 * *pInputBuf + *( pInputBuf + 1 );
		pInputBuf += 2;
		iCompressedBufSize -= 2;

		//  Unpack data block
		for( i = 0;; )
		{
			//  Pop byte from stack or read byte
			if ( i )
				c = stack[ --i ];
			else
			{
				if ( !size-- )
					break;
			
				c = *pInputBuf++;
				iCompressedBufSize--;
			}

			//  Output byte or push pair on stack
			if ( c == left[ c ] )
				*pOutputBuf++ = ( BYTE )c;
			else
			{
				stack[ i++ ] = right[ c ];
				stack[ i++ ] = left[ c ];
			}

		}
	}

	return pUncompressedBuf;
}
