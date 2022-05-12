#ifndef LZSSCODEC_H
#define LZSSCODEC_H

#include "codec.h"

class LZSSCoDec : public CoDec
{
	private :

	struct TreeStruct
	{
		int iParent;
		int iSmallerChild;
		int iLargerChild;
	};

	int _iIndexBitCount;
	int _iLengthBitCount;
	int _iWindowSize;
	int _iRawLookAheadSize;
	int _iBreakEven;
	int _iLookAheadSize;
	int _iTreeRoot;
	int _iEndOfStream;
	int _iUnusedNode;

	BYTE *_pWindow;
	TreeStruct *_pTree;

	int ModWindow( int index )	{ return index & ( _iWindowSize - 1 ); }

	void InitTree( int iInitialPhrase );
	void ContractNode( int iOldNode, int iNewNode );
	void ReplaceNode( int iOldNode, int iNewNode );
	int FindNextNode( int iNode );
	void DeleteString( int iDeleteStringIndex );
	int AddString( int iNewNode, int *piMatchPosition );
	
	public :

	LZSSCoDec( int iIndexBitCount = 12, int iLengthBitCount = 4 );
	virtual ~LZSSCoDec();

	virtual void *Compress( const void *pUncompressedBuffer, int iUncompressedBufSize, void * pCb, int *piCompressedBufSize, FNCOMPSTAT fnStat, DWORD dwData );
	virtual void *Decompress( const void *pCompressedBuffer, int iCompressedBufSize, void * pUb, int *piUncompressedBufSize, FNCOMPSTAT fnStat, DWORD dwData );
};

#endif
