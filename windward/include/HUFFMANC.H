#ifndef HUFFMANCODEC_H
#define HUFFMANCODEC_H

#include "bitbuffe.h"
#include "codec.h"

class HuffmanCoDec : public CoDec
{
	enum
	{
		iEND_OF_STREAM = 256,
		iESCAPE = 257,
		iSYMBOL_COUNT = 258,
		iNODE_TABLE_COUNT = 515,	//  ( ( iSYMBOL_COUNT * 2 ) - 1 )
		iROOT_NODE = 0,
		iMAX_WEIGHT = 0x8000
	};

	int		iLeaf[ iSYMBOL_COUNT ];
	int		iNextFreeNode;

	struct node
	{
		DWORD	dwWeight;
		int		iParent;
		int		iChildIsLeaf;
		int		iChild;
	} nodes[ iNODE_TABLE_COUNT ];

	void InitializeTree();
	void EncodeSymbol( int nextSymbol );
	int DecodeSymbol();
	void UpdateModel( int nextSymbol );
	void RebuildTree();
	void SwapNodes( int i, int j );
	void AddNewNode( int newSymbol );
	void VerifyTree();

	BitBuffer	bitBuffer;

	public :

	HuffmanCoDec();
	virtual ~HuffmanCoDec();

	virtual void *Compress( const void *pUncompressedBuffer, int iUncompressedBufSize, void * pCb, int *piCompressedBufSize, FNCOMPSTAT fnStat, DWORD dwData );
	virtual void *Decompress( const void *pCompressedBuffer, int iCompressedBufSize, void * pUb, int *piUncompressedBufSize, FNCOMPSTAT fnStat, DWORD dwData );
};

#endif	//  HUFFMANCODEC_H
