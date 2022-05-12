//--------------------------------------------------------------------------
//	flcanim.h		CFlcAnim class
//
// Copyright (c) 1994-1996 ChromeOcean Software - All rights reserved
// Reuse permission granted to Dave Thielen
//
//--------------------------------------------------------------------------

#ifndef __FLCANIM_H__
#define __FLCANIM_H__

#include "thielen.h"
#include "blt.h"

//--------------------------- C F l c H e a d e r --------------------------

#pragma pack( push, CFlcHeader )
#pragma pack( 1 )

class CFlcHeader
{

public:

	WORD	GetType() 			const	{ ASSERT_STRICT( AssertValid() ); return m_wType; 	}
	int 	GetSpeed() 			const	{ ASSERT_STRICT( AssertValid() ); return m_iSpeed;  }
	int 	GetFrameCount() 	const	{ ASSERT_STRICT( AssertValid() ); return m_nFrames; }
	int 	GetWidth()			const { ASSERT_STRICT( AssertValid() ); return m_iWidth; 	}
	int 	GetHeight()			const { ASSERT_STRICT( AssertValid() ); return m_iHeight; }

	#ifdef _DEBUG
	BOOL	AssertValid() const;
	#endif

	int	m_iSize;
	WORD	m_wType;
	short	m_nFrames;
	short	m_iWidth;
	short	m_iHeight;
	short	m_iBitsPerPixel;
	short	m_iFlags;
	int	m_iSpeed;
	BYTE	m_byReserved1[2];
	int	m_iCreateTime;
	int	m_iCreator;
	int	m_iUpdateTime;
	int	m_iUpdater;
	short	m_iAspectX;
	short	m_iAspectY;
	BYTE	m_byReserved2[ 38 ];
	int	m_iFrame1;
	int	m_iFrame2;
	BYTE	m_byREeserved3[ 40 ];
};

#pragma pack( pop, CFlcHeader )

//------------------------ C F l c F r a m e H e a d e r --------------------

#pragma pack( push, CFlcFrameHeader )
#pragma pack( 1 )

class CFlcFrameHeader
{

public:

	int	GetSize() 			const	{ ASSERT_STRICT( AssertValid() ); return m_iSize;   }
	int	GetChunkCount() 	const	{ ASSERT_STRICT( AssertValid() ); return m_nChunks; }

	#ifdef _DEBUG
	BOOL	AssertValid() const;
	#endif

	int	m_iSize;
	WORD	m_wType;
	short m_nChunks;
	BYTE	m_byReserved[ 8 ];
};

#pragma pack( pop, CFlcFrameHeader )

//----------------------- C F l c C h u n k H e a d e r --------------------

#pragma pack( push, CFlcChunkHeader )
#pragma pack( 1 )

class CFlcChunkHeader
{

public:

	enum CHUNK_TYPE
	{
		PREFIX_CHUNK 	= 0xf100,
		COLOR256_CHUNK = 4,
		SS2_CHUNK      = 7,
      COLOR_CHUNK    = 11,
      LC_CHUNK       = 12,
      BLACK_CHUNK    = 13,
      BRUN_CHUNK     = 15,
      COPY_CHUNK     = 16,
      PSTAMP_CHUNK	= 18
	};

	int	GetSize() const	{ ASSERT_STRICT( AssertValid() ); return m_iSize; }
	WORD	GetType() const	{ ASSERT_STRICT( AssertValid() ); return m_wType; }

	#ifdef _DEBUG
	BOOL	AssertValid() const;
	#endif

	int	m_iSize;
	WORD	m_wType;
};

#pragma pack( pop, CFlcChunkHeader )

//----------------------------- C F l c F r a m e --------------------------

class CFlcFrame
#ifdef _DEBUG
: public CObject
#endif
{

public:

	CFlcFrame( int	iWidth, int	iHeight, Ptr< CFile > const & ptrfile );
  ~CFlcFrame();

	int								GetFrame() 		const { ASSERT_STRICT_VALID( this ); return  m_iFrame; }
	BYTE 		  					*	GetBuffer() 			{ ASSERT_STRICT_VALID( this ); return  m_pbyBuffer; }
	BYTE 		  			const *	GetBufferEnd() const { ASSERT_STRICT_VALID( this ); return  m_pbyBufferEnd; }
	BYTE 		  			const *	GetBufferMax() const { ASSERT_STRICT_VALID( this ); return  m_pbyBufferMax; }
	CFlcFrameHeader 	const * 	GetHeader()		const { ASSERT_STRICT_VALID( this ); return &m_flcframehdr; }

	void	LoadNextFrame();

	#ifdef _DEBUG
	void	AssertValid() const;
	#endif

private:

	int					m_iMaxBuffer;
	int					m_iFrame;
	BYTE			 	 *	m_pbyBuffer;
	BYTE			 	 *	m_pbyBufferEnd;
	BYTE			 	 *	m_pbyBufferMax;

	Ptr< CFile >		m_ptrfile;
	CFlcFrameHeader  	m_flcframehdr;
};

//------------------------------ C F l c A n i m ---------------------------

class CFlcAnim
#ifdef _DEBUG
: public CObject
#endif
{

public:

	CFlcAnim( Ptr< CFile > const & ptrfile );
  ~CFlcAnim();

	void	 		Init( CDIBWnd * pdibwnd );
	BOOL	 		DisplayNextFrame();

	void			SetIgnorePalette( BOOL bIgnorePalette ) { m_bIgnorePalette = bIgnorePalette; }
	BOOL			IsIgnorePalette () const					 { return m_bIgnorePalette; }

	RGBQUAD	 * GetPalette()				{ ASSERT_STRICT_VALID( this ); return m_prgbquad; }

	BOOL	 		IsInitialized()	const	{ ASSERT_STRICT_VALID( this ); return m_bInitialized; }
	int	 		GetSpeed()  		const;
	int	 		GetWidth()  		const	{ ASSERT_STRICT_VALID( this ); return m_flcheader.GetWidth();  }
	int	 		GetHeight() 		const	{ ASSERT_STRICT_VALID( this ); return m_flcheader.GetHeight(); }
	CDIB 		 *	GetDIB()				const { ASSERT_STRICT_VALID( this ); ASSERT_STRICT_VALID( m_pdib ); return m_pdib; }

	#ifdef _DEBUG
	void	AssertValid() const;
	#endif

protected:

	void	DoChunk();
	void	DoSS2Chunk();
	void	DoColorChunk( BOOL );
	void	DoLCChunk();
	void	DoBlackChunk();
	void	DoBRunChunk();
	void	DoCopyChunk();
	void	GetChunk();

	BOOL	GetFrame();

private:

	BYTE				 * m_pby;
	BOOL					m_bInitialized;
	BOOL					m_bIgnorePalette;

	CDIBWnd		 	 * m_pdibwnd;
	CDIB				 * m_pdib;
	CColorFormat		m_colorformat;
	CFlcHeader			m_flcheader;
	CFlcChunkHeader *	m_pchunkhdr;
	RGBQUAD	 		 * m_prgbquad;
	Ptr< CFlcFrame >	m_ptrflcframe;
};

#endif
