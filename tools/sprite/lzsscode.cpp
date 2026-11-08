#include "stdafx.h"

#include "bitbuffe.h"
#include "lzsscode.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

LZSSCoDec::LZSSCoDec( int iIndexBitCount, int iLengthBitCount ) : 
	_iIndexBitCount( iIndexBitCount ),
	_iLengthBitCount( iLengthBitCount )
{
	_iWindowSize = 1 << _iIndexBitCount;
	_iRawLookAheadSize = 1 << _iLengthBitCount;
	_iBreakEven = ( 1 + _iIndexBitCount + _iLengthBitCount ) / 9;
	_iLookAheadSize = _iRawLookAheadSize + _iBreakEven;
	_iTreeRoot = _iWindowSize;
	_iEndOfStream = 0;
	_iUnusedNode = 0;

	_pWindow = new BYTE[ _iWindowSize ];
	_pTree = new TreeStruct[ _iWindowSize + 1 ];
	if ( _pWindow == NULL || _pTree == NULL )
	{
		delete[] _pWindow;
		delete[] _pTree;
		throw( "Not enough memory" );
	}
}

LZSSCoDec::~LZSSCoDec()
{
	delete[] _pWindow;
	delete[] _pTree;
}

//  Make sure the tree is initialized to 0 
//  ( which is the UNUSED_NODE node ).  Add 
//  a single phrase to the tree so it has a
//  root node.
void LZSSCoDec::InitTree( int iInitialPhrase )
{
	( void )memset( _pTree, 0, sizeof( TreeStruct ) * _iWindowSize );

	_pTree[ _iTreeRoot ].iLargerChild = iInitialPhrase;
	_pTree[ iInitialPhrase ].iParent = _iTreeRoot;
}

//  This routine is used when a node is being deleted.
//  The link to it's descendant is broken by pulling 
//  the descendant in to overlay the existing link.
void LZSSCoDec::ContractNode( int iOldNode, int iNewNode )
{
	_pTree[ iNewNode ].iParent = _pTree[ iOldNode ].iParent;
	if ( _pTree[ _pTree[ iOldNode ].iParent ].iLargerChild == iOldNode )
		_pTree[ _pTree[ iOldNode ].iParent ].iLargerChild = iNewNode;
	else
		_pTree[ _pTree[ iOldNode ].iParent ].iSmallerChild = iNewNode;

	_pTree[ iOldNode ].iParent = _iUnusedNode;
}

//  This routine is also used when a node 
//  is being deleted.  However, in this case,
//  it is being replaced by a node that was 
//  not previously in the tree.
void LZSSCoDec::ReplaceNode( int iOldNode, int iNewNode )
{
	int iParent = _pTree[ iOldNode ].iParent;

	if ( _pTree[ iParent ].iSmallerChild == iOldNode )
		_pTree[ iParent ].iSmallerChild = iNewNode;
	else
		_pTree[ iParent ].iLargerChild = iNewNode;

	_pTree[ iNewNode ] = _pTree[ iOldNode ];
	_pTree[ _pTree[ iNewNode ].iSmallerChild ].iParent = iNewNode;
	_pTree[ _pTree[ iNewNode ].iLargerChild ].iParent = iNewNode;
	
	_pTree[ iOldNode ].iParent = _iUnusedNode;
}

//  This routine is used to find the next 
//  smaller node after the node argument.  
//  It assumes that the node has a smaller 
//  child.  We find the next smallest child 
//  by going to the iSmallerChild node, then 
//  going to the end of the iLargerChild 
//  descendant chain.
int LZSSCoDec::FindNextNode( int iCurrentNode )
{
	int iNextNode = _pTree[ iCurrentNode ].iSmallerChild;
	while( _pTree[ iNextNode ].iLargerChild != _iUnusedNode )
		iNextNode = _pTree[ iNextNode ].iLargerChild;

	return iNextNode;
}

//  This routine performs the classic binary 
//  tree deletion algorithm.  If the node to 
//  be deleted has a NULL link in either direction,
//  we just pull the non-null link up one to replace 
//  the existing link.  If both links exist, we 
//  instead delete the next link in order, which
//  is guaranteed to have a NULL link, then replace 
//  the node to be deleted with the next link.
void LZSSCoDec::DeleteString( int iDeleteStringIndex )
{
	int iReplacementStringIndex;

	if ( _pTree[ iDeleteStringIndex ].iParent == _iUnusedNode )
		return;
	
	if ( _pTree[ iDeleteStringIndex ].iLargerChild == _iUnusedNode )
		ContractNode( iDeleteStringIndex , _pTree[ iDeleteStringIndex ].iSmallerChild );
	else if ( _pTree[ iDeleteStringIndex ].iSmallerChild == _iUnusedNode )
		ContractNode( iDeleteStringIndex, _pTree[ iDeleteStringIndex ].iLargerChild );
	else
	{
		iReplacementStringIndex = FindNextNode( iDeleteStringIndex );
		DeleteString( iReplacementStringIndex );
		ReplaceNode( iDeleteStringIndex, iReplacementStringIndex );
	}
}

//  This is where most of the work done by the 
//  encoder takes place.  This routine is responsible 
//  for adding the new node to the binary tree.  It 
//  also has to find the best match among all of the 
//  existing nodes in the tree, and return that to 
//  the calling routine.  To make matters even more 
//  complicated, if the new node has a duplicate in 
//  the tree, the old_node is deleted, for reasons of 
//  efficiency.
int LZSSCoDec::AddString( int iNewNode, int *piMatchPosition )
{
	int iTestNode, iMatchLength;
	int *piChildIndex;

	if ( iNewNode == _iEndOfStream )
		return 0;

	iTestNode = _pTree[ _iTreeRoot ].iLargerChild;
	iMatchLength = 0;
	for( ; ; )
	{
		int iMatchIndex, iDelta = 0;
		for( iMatchIndex = 0; iMatchIndex < _iLookAheadSize; iMatchIndex++ )
		{
			iDelta = _pWindow[ ModWindow( iNewNode + iMatchIndex ) ] - _pWindow[ ModWindow( iTestNode + iMatchIndex ) ];
			if ( iDelta != 0 )
				break;
		}

		if ( iMatchIndex >= iMatchLength )
		{
			iMatchLength = iMatchIndex;
			*piMatchPosition = iTestNode;
			if ( iMatchLength >= _iLookAheadSize )
			{
				ReplaceNode( iTestNode, iNewNode );
				return iMatchLength;
			}
		}

		if ( iDelta >= 0 )
			piChildIndex = &_pTree[ iTestNode ].iLargerChild;
		else
			piChildIndex = &_pTree[ iTestNode ].iSmallerChild;

		if ( *piChildIndex == _iUnusedNode )
		{
			*piChildIndex = iNewNode;
			_pTree[ iNewNode ].iParent = iTestNode;
			_pTree[ iNewNode ].iLargerChild = _iUnusedNode;
			_pTree[ iNewNode ].iSmallerChild = _iUnusedNode;
			return iMatchLength;
		}

		iTestNode = *piChildIndex;
	}


	//  Shouldn't ever get here.
	ASSERT( FALSE );
	return -1;	//  Just so no warnings during compile.
}

//  This is the compression routine.  It has to 
//  first load up the lookahead buffer, then go 
//  into the main compression loop.  The main loop 
//  decides whether to output a single character or 
//  an index/length token that defines a phrase.  
//  Once the character or phrase has been sent out, 
//  another loop has to run.  The second loop reads 
//  in new characters, deletes the strings that are 
//  overwritten by the new character, and adds the 
//  strings that are created by the new character.
void *LZSSCoDec::Compress( const void *pUncompressedBuffer, int iUncompressedBufSize, int *piCompressedBufSize )
{
	ASSERT( pUncompressedBuffer );
	ASSERT( 0 < iUncompressedBufSize );
	ASSERT( piCompressedBufSize );

	//  We want to use this as a BYTE array, not a void *.
	BYTE *pUncompBuf = ( BYTE * )pUncompressedBuffer;

	*piCompressedBufSize = 3 * iUncompressedBufSize / 2;
	void *pCompressedBuffer = new BYTE[ 1 + *piCompressedBufSize ];
	if ( pCompressedBuffer == NULL )
		throw( "Not enough memory" );
	BitBuffer outputBuf;
	outputBuf.AttachForWriting( ((char *)pCompressedBuffer) + 1, *piCompressedBufSize );

	outputBuf.PutBits( ( DWORD )iUncompressedBufSize, 32 );

	int iCurrentPosition = 1;
	( void )memcpy( &_pWindow[ iCurrentPosition ], pUncompBuf, _iLookAheadSize );
	iUncompressedBufSize -= _iLookAheadSize;
	pUncompBuf += _iLookAheadSize;

	InitTree( iCurrentPosition );

	int iLookAheadBytes = _iLookAheadSize;
	int iMatchLength = 0;
	int iMatchPosition = 0;
	while( iLookAheadBytes > 0 )
	{
		if ( iMatchLength > iLookAheadBytes )
			iMatchLength = iLookAheadBytes;

		int iReplaceCount;
		if ( iMatchLength <= _iBreakEven )
		{
			iReplaceCount = 1;
			outputBuf.PutBit( 1 );
			outputBuf.PutBits( ( DWORD )_pWindow[ iCurrentPosition ], 8 );
		}
		else
		{
			iReplaceCount = iMatchLength;
			outputBuf.PutBit( 0 );
			outputBuf.PutBits( ( DWORD )iMatchPosition, _iIndexBitCount );
			outputBuf.PutBits( ( DWORD )( iMatchLength - ( _iBreakEven + 1 ) ), _iLengthBitCount );
		}

		int iReplacesBytesIndex;
		for( iReplacesBytesIndex = 0; iReplacesBytesIndex < iReplaceCount; iReplacesBytesIndex++ )
		{
			DeleteString( ModWindow( iCurrentPosition + _iLookAheadSize ) );
			if ( iUncompressedBufSize == 0 )
				iLookAheadBytes--;
			else
			{
				_pWindow[ ModWindow( iCurrentPosition + _iLookAheadSize ) ] = *pUncompBuf++;
				iUncompressedBufSize--;
			}
			iCurrentPosition = ModWindow( iCurrentPosition + 1 );
			if ( iLookAheadBytes ) 
				iMatchLength = AddString( iCurrentPosition, &iMatchPosition );
		}
	}

	outputBuf.PutBit( 0 );
	outputBuf.PutBits( ( DWORD )_iEndOfStream, _iIndexBitCount );
	outputBuf.Flush();

	*piCompressedBufSize = outputBuf.GetSizeInBytes() + 5;

	switch (_iIndexBitCount)
	  {
		case 12 :
			switch (_iLengthBitCount)
			  {
				case 4 :
					* ((char *) pCompressedBuffer) = (char) CODEC::LZSS_12_4;
					break;
				default:
					* ((char *) pCompressedBuffer) = (char) CODEC::LZSS_12_6;
					break;
			  }
			break;
		case 14 :
			* ((char *) pCompressedBuffer) = (char) CODEC::LZSS_14_4;
			break;
		case 16 :
			switch (_iLengthBitCount)
			  {
				case 4 :
					* ((char *) pCompressedBuffer) = (char) CODEC::LZSS_16_4;
					break;
				case 6 :
					* ((char *) pCompressedBuffer) = (char) CODEC::LZSS_16_6;
					break;
				default:
					* ((char *) pCompressedBuffer) = (char) CODEC::LZSS_16_8;
					break;
			  }
			break;
	  }

	pCompressedBuffer = (BYTE *) realloc ( pCompressedBuffer, outputBuf.GetSizeInBytes() + 1 );

	return pCompressedBuffer;
}

//  This is the expansion routine for the LZSS 
//  algorithm.  All it has to do is read in flag 
//  bits, decide whether to read in a character 
//  or an index/length pair, and take the appropriate 
//  action.
void *LZSSCoDec::Decompress( const void *pCompressedBuffer, int iCompressedBufSize, int *piUncompressedBufSize )
{
	ASSERT( pCompressedBuffer );
	ASSERT( 0 < iCompressedBufSize );
	ASSERT( piUncompressedBufSize );

	BitBuffer inputBuf;
	inputBuf.AttachForReading( pCompressedBuffer, iCompressedBufSize );

	*piUncompressedBufSize = ( int )inputBuf.GetBits( 32 );
	void *pUncompressedBuf = malloc ( *piUncompressedBufSize );
	if ( pUncompressedBuf == NULL )
		throw( "Not enough memory" );
	BYTE *pBufPos = ( BYTE * )pUncompressedBuf;

	int iCurrentPosition = 1;
	for( ;; )
	{
		if ( inputBuf.GetBit() )
		{
			DWORD	dwMatchBits = inputBuf.GetBits( 8 );

			*pBufPos++ = ( BYTE )dwMatchBits;
			_pWindow[ iCurrentPosition ] = ( BYTE )dwMatchBits;
			iCurrentPosition = ModWindow( iCurrentPosition + 1 );
		}
		else
		{
			int iMatchPosition = ( int )inputBuf.GetBits( _iIndexBitCount );
			if ( iMatchPosition == _iEndOfStream )
				break;
			int iMatchLength = ( int )inputBuf.GetBits( _iLengthBitCount );
			iMatchLength += _iBreakEven;
			int iMatchIndex;
			for( iMatchIndex = 0; iMatchIndex <= iMatchLength; iMatchIndex++ )
			{
				BYTE bMatchBYTE = _pWindow[ ModWindow( iMatchPosition + iMatchIndex ) ];
				*pBufPos++ = bMatchBYTE;
				_pWindow[ iCurrentPosition ] = bMatchBYTE;
				iCurrentPosition = ModWindow( iCurrentPosition + 1 );
			}
		}
	}

	return pUncompressedBuf;
}
