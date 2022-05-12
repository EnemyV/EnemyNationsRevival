#ifndef __MMIO_H__
#define __MMIO_H__


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


const int MMIO_NUM_LIST = 5;

class FourCC
{

private:

	FOURCC	four_cc;

public:

	FourCC( void );
	FourCC( const FOURCC& cc );
	FourCC( const FourCC* cc );
	FourCC( char a, char b, char c, char d );
	FourCC( const char* s );

	void Copy( const FOURCC& cc );
	void Copy( const FourCC& cc );
	void Copy( const char* s );

	FourCC& operator = ( const FOURCC& cc );
	FourCC& operator = ( const FourCC& cc );
	FourCC& operator = ( const char* s );

	BOOL operator == ( const FourCC& cc ) const;

	operator FOURCC() const;
};


class CMmio : public CObject
{
	public:

	CMmio () {ctor ();}
	CMmio (const char *pFile);
	virtual ~CMmio ();

	//  Overridables.
	virtual void		Open( const char *pFilename );
	virtual void		Close();

	LONG						GetOffset () const;

	//  The following shouldn't need to be
	//  overridden.
	DWORD		DescendRiff (char ch0, char ch1, char ch2, char ch3)
												{return DescendRiff (mmioFOURCC (ch0, ch1, ch2, ch3));}
	DWORD		DescendList (char ch0, char ch1, char ch2, char ch3)
												{return DescendList (mmioFOURCC (ch0, ch1, ch2, ch3));}
	DWORD		DescendChunk (char ch0, char ch1, char ch2, char ch3)
												{return DescendChunk (mmioFOURCC (ch0, ch1, ch2, ch3));}
	DWORD		DescendRiff (FOURCC chunk);
	DWORD		DescendList (FOURCC chunk);
	DWORD		DescendChunk (FOURCC chunk);

	//
	//	4/28/96 BobP, accept "0123" style fourcc codes
	//
	DWORD DescendRiff( const FourCC& cc );
	DWORD DescendList( const FourCC& cc );
	DWORD DescendChunk( const FourCC& cc );

	void		AscendList ();
	void		AscendChunk ();

	void		Read (void *pBuf, LONG lNum);
	short int	ReadShort ();
	int			ReadInt ();
	long		ReadLong ();
	float		ReadFloat ();
	void		ReadString (CString & sRtn);

	//
	//	4/28/96 BobP, These are for getting chunk and list sizes
	//
	const MMCKINFO& GetRiffChunkInfo() const;
	const MMCKINFO& GetListChunkInfo( int i ) const;
	const MMCKINFO& GetChunkInfo() const;

	CMmio & operator >> ( SHORT& s );
	CMmio & operator >> ( USHORT& s );
	CMmio & operator >> ( LONG& l );
	CMmio & operator >> ( ULONG& l );
	CMmio & operator >> ( INT& i );
	CMmio & operator >> ( UINT& i );
	CMmio & operator >> ( CString& s );

	const char * GetFileName () const { return m_sFileName; }

protected :
	void		ctor ();

	HMMIO		m_hMmio;
	int			m_iListDepth;
	MMCKINFO	m_mckiRiff;
	MMCKINFO	m_mckiList [MMIO_NUM_LIST];
	MMCKINFO	m_mckiChunk;

	CString		m_sFileName;
};

//  MMIO file embedded in another file.
class CMmioEmbeddedFile : public CMmio
{
	public :

	CMmioEmbeddedFile();
	CMmioEmbeddedFile( UINT hFile, const char * pFileName );
	virtual ~CMmioEmbeddedFile();

	virtual void	Open( UINT hFile, const char * pFileName );
	virtual void	Close();
};

//  Probably want to add CMmioMemoryFile here.

#endif
