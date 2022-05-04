#include "stdafx.h"

#include "bitbuffe.h"
#include "huffmanc.h"

HuffmanCoDec::HuffmanCoDec() : bitBuffer()
{
}

HuffmanCoDec::~HuffmanCoDec()
{
}

void *HuffmanCoDec::Compress( const void *pUncompressedBuffer, int iUncompressedBufSize, int *piCompressedBufSize )
{
	ASSERT( pUncompressedBuffer );
	ASSERT( 0 < iUncompressedBufSize );
	ASSERT( piCompressedBufSize );

	//  We want to use this as a BYTE array, not a void *.
	BYTE *pUncompBuf = ( BYTE * )pUncompressedBuffer;

	*piCompressedBufSize = 3 * iUncompressedBufSize / 2;
	void *pCompressedBuffer = malloc ( 1 + *piCompressedBufSize );
	if ( pCompressedBuffer == NULL )
		throw( "Not enough memory" );
	bitBuffer.AttachForWriting( ((char *)pCompressedBuffer) + 1, *piCompressedBufSize );

	bitBuffer.PutBits( ( DWORD )iUncompressedBufSize, 32 );

	InitializeTree();

	while( iUncompressedBufSize )
	{
		BYTE	nextSymbol = *pUncompBuf++;
		iUncompressedBufSize--;

		EncodeSymbol( nextSymbol );
		UpdateModel( nextSymbol );
	}
	EncodeSymbol( iEND_OF_STREAM );

	bitBuffer.Flush();
	*piCompressedBufSize = bitBuffer.GetSizeInBytes() + 1;

	* ((char *) pCompressedBuffer) = (char) CODEC::HUFFMAN;
	pCompressedBuffer = (BYTE *) realloc ( pCompressedBuffer, bitBuffer.GetSizeInBytes() + 1 );

	return pCompressedBuffer;
}

void *HuffmanCoDec::Decompress( const void *pCompressedBuffer, int iCompressedBufSize, int *piUncompressedBufSize )
{
	ASSERT( pCompressedBuffer );
	ASSERT( 0 < iCompressedBufSize );
	ASSERT( piUncompressedBufSize );

	bitBuffer.AttachForReading( pCompressedBuffer, iCompressedBufSize );

	*piUncompressedBufSize = ( int )bitBuffer.GetBits( 32 );
	void *pUncompressedBuf = malloc ( *piUncompressedBufSize );
	if ( pUncompressedBuf == NULL )
		throw( "Not enough memory" );
	BYTE *pBufPos = ( BYTE * )pUncompressedBuf;

	InitializeTree();
	for( ;; )
	{
		int nextSymbol = DecodeSymbol();
		if ( nextSymbol == iEND_OF_STREAM )
			break;

		*pBufPos++ = ( BYTE )nextSymbol;
		UpdateModel( nextSymbol );
	}

	return pUncompressedBuf;
}

void HuffmanCoDec::InitializeTree()
{
	nodes[ iROOT_NODE ].iChild = iROOT_NODE + 1;
	nodes[ iROOT_NODE ].iChildIsLeaf = FALSE;
	nodes[ iROOT_NODE ].dwWeight = 2;
	nodes[ iROOT_NODE ].iParent = -1;

	nodes[ iROOT_NODE + 1 ].iChild = iEND_OF_STREAM;
	nodes[ iROOT_NODE + 1 ].iChildIsLeaf = TRUE;
	nodes[ iROOT_NODE + 1 ].dwWeight = 1;
	nodes[ iROOT_NODE + 1 ].iParent = iROOT_NODE;
	iLeaf[ iEND_OF_STREAM ] = iROOT_NODE + 1;

	nodes[ iROOT_NODE + 2 ].iChild = iESCAPE;
	nodes[ iROOT_NODE + 2 ].iChildIsLeaf = TRUE;
	nodes[ iROOT_NODE + 2 ].dwWeight = 1;
	nodes[ iROOT_NODE + 2 ].iParent = iROOT_NODE;
	iLeaf[ iESCAPE ] = iROOT_NODE + 2;
	iNextFreeNode = iROOT_NODE + 3;

	for( int nodeIndex = 0; nodeIndex < iEND_OF_STREAM; nodeIndex++ )
		iLeaf[ nodeIndex ] = -1;
}

void HuffmanCoDec::EncodeSymbol( int nextSymbol )
{
	DWORD code;
	DWORD currentBit;
	int codeSize;
	int currentNode;

	code = 0;
	currentBit = 1;
	codeSize = 0;
	currentNode = iLeaf[ nextSymbol ];

	if ( currentNode == -1 )
		currentNode = iLeaf[ iESCAPE ];

	while( currentNode != iROOT_NODE )
	{
		if ( ( currentNode & 1 ) == 0 )
			code |= currentBit;
		currentBit <<= 1;
		codeSize++;
		currentNode = nodes[ currentNode ].iParent;
	}

	bitBuffer.PutBits( code, codeSize );
	if ( iLeaf[ nextSymbol ] == -1 )
	{
		bitBuffer.PutBits( ( DWORD )nextSymbol, 8 );
		AddNewNode( nextSymbol );
	}

#ifdef _DEBUG
	VerifyTree();
#endif
}

int HuffmanCoDec::DecodeSymbol()
{
	int currentNode;
	int nextSymbol;

	currentNode = iROOT_NODE;
	while( nodes[ currentNode ].iChildIsLeaf == FALSE )
	{
		currentNode = nodes[ currentNode ].iChild;
		currentNode += bitBuffer.GetBit();
	}

	nextSymbol = nodes[ currentNode ].iChild;
	if ( nextSymbol == iESCAPE )
	{
		nextSymbol = ( int )bitBuffer.GetBits( 8 );
		AddNewNode( nextSymbol );
	}

#ifdef _DEBUG
	VerifyTree();
#endif

	return nextSymbol;
}

void HuffmanCoDec::UpdateModel( int nextSymbol )
{
	int currentNode;
	int newNode;

	if ( nodes[ iROOT_NODE ].dwWeight == ( DWORD )iMAX_WEIGHT )
		RebuildTree();

	currentNode = iLeaf[ nextSymbol ];
	while( currentNode != -1 )
	{
		nodes[ currentNode ].dwWeight++;
		for( newNode = currentNode; newNode > iROOT_NODE; newNode-- )
		{
			if ( nodes[ newNode - 1 ].dwWeight >= nodes[ currentNode ].dwWeight )
				break;
		}

		if ( currentNode != newNode )
		{
			SwapNodes( currentNode, newNode );
			currentNode = newNode;
		}

		currentNode = nodes[ currentNode ].iParent;
	}

#ifdef _DEBUG
	VerifyTree();
#endif
}

void HuffmanCoDec::RebuildTree()
{
	int i, j, k;
	DWORD dwWeight;

	j = iNextFreeNode - 1;
	for( i = j; i >= iROOT_NODE; i-- )
	{
		if ( nodes[ i ].iChildIsLeaf == TRUE )
		{
			nodes[ j ] = nodes[ i ];
			nodes[ j ].dwWeight = ( nodes[ j ].dwWeight + 1 ) / 2;
			j--;
		}
	}

	for( i = iNextFreeNode - 2; j >= iROOT_NODE; i -= 2, j-- )
	{
		k = i + 1;
		nodes[ j ].dwWeight = nodes[ i ].dwWeight + nodes[ j ].dwWeight;
		dwWeight = nodes[ j ].dwWeight;
		nodes[ j ].iChildIsLeaf = FALSE;
		for( k = j + 1; dwWeight < nodes[ k ].dwWeight; k++ )
			;
		k--;
		memmove( &nodes[ j ], &nodes[ j + 1 ], ( k - j ) * sizeof( struct node ) );
		nodes[ k ].dwWeight = dwWeight;
		nodes[ k ].iChild = i;
		nodes[ k ].iChildIsLeaf = FALSE;
	}

	for( i = iNextFreeNode - 1; i >= iROOT_NODE; i-- )
	{
		if ( nodes[ i ].iChildIsLeaf )
		{
			k = nodes[ i ].iChild;
			iLeaf[ k ] = i;
		}
		else
		{
			k = nodes[ i ].iChild;
			nodes[ k ].iParent = nodes[ k + 1 ].iParent = i;
		}
	}

#ifdef _DEBUG
	VerifyTree();
#endif
}

void HuffmanCoDec::SwapNodes( int i, int j )
{
	struct node temp;

	if ( nodes[ i ].iChildIsLeaf )
		iLeaf[ nodes[ i ].iChild ] = j;
	else
	{
		nodes[ nodes[ i ].iChild ].iParent = j;
		nodes[ nodes[ i ].iChild + 1 ].iParent = j;
	}

	if ( nodes[ j ].iChildIsLeaf )
		iLeaf[ nodes[ j ].iChild ] = i;
	else
	{
		nodes[ nodes[ j ].iChild ].iParent = i;
		nodes[ nodes[ j ].iChild + 1 ].iParent = i;
	}

	temp = nodes[ i ];
	nodes[ i ] = nodes[ j ];
	nodes[ i ].iParent = temp.iParent;
	temp.iParent = nodes[ j ].iParent;
	nodes[ j ] = temp;

#ifdef _DEBUG
	VerifyTree();
#endif
}

void HuffmanCoDec::AddNewNode( int newSymbol )
{
	int lightestNode;
	int newNode;
	int zeroWeightNode;

	lightestNode = iNextFreeNode - 1;
	newNode = iNextFreeNode;
	zeroWeightNode = iNextFreeNode + 1;
	iNextFreeNode += 2;

	nodes[ newNode ] = nodes[ lightestNode ];
	nodes[ newNode ].iParent = lightestNode;
	iLeaf[ nodes[ newNode ].iChild ] = newNode;

	nodes[ lightestNode ].iChild = newNode;
	nodes[ lightestNode ].iChildIsLeaf = FALSE;

	nodes[ zeroWeightNode ].iChild = newSymbol;
	nodes[ zeroWeightNode ].iChildIsLeaf = TRUE;
	nodes[ zeroWeightNode ].dwWeight = 0;
	nodes[ zeroWeightNode ].iParent = lightestNode;
	iLeaf[ newSymbol ] = zeroWeightNode;

#ifdef _DEBUG
	VerifyTree();
#endif
}

void HuffmanCoDec::VerifyTree()
{
	int nodeIndex;
	for( nodeIndex = 0; nodeIndex < iNextFreeNode; nodeIndex++ )
	{
		if ( nodes[ nodeIndex ].iChildIsLeaf == FALSE )
		{	
			ASSERT( nodes[ nodes[ nodeIndex ].iChild ].iParent == nodeIndex );
			ASSERT( nodes[ nodes[ nodeIndex ].iChild + 1 ].iParent == nodeIndex );
		}
	}

	for( nodeIndex = iNextFreeNode - 1; nodeIndex > iROOT_NODE; nodeIndex-- )
	{
		ASSERT( nodes[ nodeIndex ].iParent < nodeIndex );
	}
}