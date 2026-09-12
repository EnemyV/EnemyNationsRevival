#ifndef DATA_TEST_STDAFX_H
#define DATA_TEST_STDAFX_H

//---------------------------------------------------------------------------
//
//  tests/data/shim/stdafx.h -- just enough of wind22 to compile the SHIPPED
//  windward/wind22/src/datafile.cpp on its own with cl.exe.
//
//  Why a shim and not the real headers: the real stdafx.h drags in the whole
//  SDL2/CDIB/game stack, which cannot be built without CMake. Everything below
//  is a faithful-but-minimal stand-in for the ONE behaviour datafile.cpp needs
//  from it, and nothing here is compiled into the game.
//
//  Two pieces are deliberately more than stubs, because the loader checks under
//  test depend on them:
//    * CFile does REAL file I/O and records every path it was asked to open
//      (g_openLog), which is how the case-folding checks observe a decision the
//      case-insensitive Windows filesystem would otherwise hide.
//    * CMmio really parses RIFF, so the FVER version gate is exercised rather
//      than simulated.
//
//---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>   // OFN_FILEMUSTEXIST, used by the .dat picker
#include <mmsystem.h>  // MMCKINFO, for CMmio::GetRiffChunkInfo

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <unordered_map>
#include <vector>

//---------------------------------------------------------------------------
//  MFC-isms datafile.cpp spells out.
//---------------------------------------------------------------------------

#define BASED_CODE
#define DEBUG_NEW new
#define TRAP( ) ( (void)0 )
#ifndef ASSERT
#define ASSERT( x ) ( (void)0 )
#endif
#define ASSERT_VALID( x ) ( (void)0 )
#define TRACE( ... ) ( (void)0 )
#define _T( x ) x

typedef void* POSITION;

class CWnd;
class CDataExchange;

//---------------------------------------------------------------------------
//  CString: the subset datafile.cpp uses.
//---------------------------------------------------------------------------

class CString
{
  public:
    CString( ) { }
    CString( const char* p ) : m_s( p ? p : "" ) { }
    CString( char ch ) : m_s( 1, ch ) { }
    CString( const CString& o ) : m_s( o.m_s ) { }

    CString& operator=( const CString& o )
    {
        m_s = o.m_s;
        return *this;
    }
    CString& operator=( const char* p )
    {
        m_s = p ? p : "";
        return *this;
    }

    operator const char*( ) const { return m_s.c_str( ); }

    int  GetLength( ) const { return (int)m_s.size( ); }
    BOOL IsEmpty( ) const { return m_s.empty( ) ? TRUE : FALSE; }
    void MakeLower( )
    {
        for ( size_t i = 0; i < m_s.size( ); i++ ) m_s[i] = (char)tolower( (unsigned char)m_s[i] );
    }
    int  Find( char ch ) const
    {
        size_t n = m_s.find( ch );
        return n == std::string::npos ? -1 : (int)n;
    }
    char operator[]( int i ) const { return m_s[(size_t)i]; }

    // MFC's GetBuffer/ReleaseBuffer pair. ReleaseBuffer(-1) means "measure it
    // with strlen"; ReleaseBuffer(n) truncates to n characters, which is how
    // datafile.cpp trims a trailing separator.
    char* GetBuffer( int cbMin )
    {
        if ( (int)m_s.size( ) < cbMin ) m_s.resize( (size_t)cbMin, '\0' );
        return &m_s[0];
    }
    void ReleaseBuffer( int cb = -1 )
    {
        if ( cb < 0 )
            m_s.resize( strlen( m_s.c_str( ) ) );
        else if ( (size_t)cb < m_s.size( ) )
            m_s.resize( (size_t)cb );
    }

    void Format( const char* pFmt, ... )
    {
        char    buf[2048];
        va_list ap;
        va_start( ap, pFmt );
        _vsnprintf_s( buf, sizeof( buf ), _TRUNCATE, pFmt, ap );
        va_end( ap );
        m_s = buf;
    }
    BOOL LoadString( UINT ) { return FALSE; }  // no resources in the fixture

    CString operator+( const CString& o ) const { return CString( ( m_s + o.m_s ).c_str( ) ); }
    CString operator+( const char* p ) const { return CString( ( m_s + ( p ? p : "" ) ).c_str( ) ); }

    const std::string& str( ) const { return m_s; }

  private:
    std::string m_s;
};

inline CString operator+( const char* p, const CString& o ) { return CString( p ) + o; }
inline void    csPrintf( CString*, const char* ) { }

//---------------------------------------------------------------------------
//  CFile / CStdioFile over real files, with an open log.
//---------------------------------------------------------------------------

extern std::vector<std::string> g_openLog;  // every path Open() was handed
extern std::vector<std::string> g_mmioLog;  // every path a CMmio was built on
extern std::vector<std::string> g_msgBoxes; // every MessageBoxA body text

//  The loader shows a modal box on a fatal data error. A fixture must never
//  block on one, and the TEXT is itself worth checking (phase 2 replaced
//  "ENations.dat could not be found" with a message that names the entry), so
//  the call is captured instead of shown.
int EnTestMessageBox( HWND, const char* pText, const char*, UINT );
#undef MessageBoxA
#define MessageBoxA EnTestMessageBox

class CFileException
{
};

class CFileStatus
{
  public:
    DWORD m_size = 0;
};

class CFile
{
  public:
    enum
    {
        modeRead      = 0x0000,
        modeWrite     = 0x0001,
        shareDenyWrite = 0x0020,
        typeBinary    = 0x0100
    };
    enum
    {
        begin   = 0,
        current = 1,
        end     = 2
    };

    CFile( ) { }
    virtual ~CFile( ) { Close( ); }

    virtual BOOL Open( const char* p, UINT /*flags*/, CFileException* = NULL )
    {
        Close( );
        if ( p == NULL ) return FALSE;
        g_openLog.push_back( p );
        m_fp = NULL;
        fopen_s( &m_fp, p, "rb" );
        if ( m_fp == NULL ) return FALSE;
        m_sPath  = p;
        m_hFile  = (HANDLE)m_fp;
        return TRUE;
    }
    virtual void Close( )
    {
        if ( m_fp ) fclose( m_fp );
        m_fp    = NULL;
        m_hFile = NULL;
    }
    virtual UINT Read( void* pBuf, UINT cb )
    {
        if ( m_fp == NULL ) return 0;
        return (UINT)fread( pBuf, 1, cb, m_fp );
    }
    virtual LONG Seek( LONG off, UINT from )
    {
        if ( m_fp == NULL ) return 0;
        fseek( m_fp, off, from == begin ? SEEK_SET : ( from == current ? SEEK_CUR : SEEK_END ) );
        return ftell( m_fp );
    }
    virtual DWORD GetPosition( ) const { return m_fp ? (DWORD)ftell( m_fp ) : 0; }
    virtual DWORD GetLength( ) const
    {
        if ( m_fp == NULL ) return 0;
        long cur = ftell( m_fp );
        fseek( m_fp, 0, SEEK_END );
        long len = ftell( m_fp );
        fseek( m_fp, cur, SEEK_SET );
        return (DWORD)len;
    }
    const char* PathForTest( ) const { return m_sPath.c_str( ); }

    static int GetStatus( const char*, CFileStatus& ) { return 0; }

    HANDLE m_hFile = NULL;

  protected:
    FILE*       m_fp = NULL;
    std::string m_sPath;
};

class CStdioFile : public CFile
{
};

class CArchive
{
  public:
    enum
    {
        load  = 1,
        store = 2
    };
    CArchive( CFile* pFile, int ) : m_pFile( pFile ) { }
    void   Flush( ) { }
    void   Close( ) { }
    CFile* GetFile( ) const { return m_pFile; }

  private:
    CFile* m_pFile;
};

//---------------------------------------------------------------------------
//  CMapStringToPtr: the two operations datafile.cpp uses plus iteration.
//---------------------------------------------------------------------------

class CMapStringToPtr
{
  public:
    void*& operator[]( const char* pKey ) { return m_map[std::string( pKey )]; }

    BOOL Lookup( const CString& key, void*& rVal ) const
    {
        std::unordered_map<std::string, void*>::const_iterator it = m_map.find( std::string( (const char*)key ) );
        if ( it == m_map.end( ) ) return FALSE;
        rVal = it->second;
        return TRUE;
    }

    POSITION GetStartPosition( ) const
    {
        if ( m_map.empty( ) ) return NULL;
        m_it = m_map.begin( );
        return (POSITION)1;
    }
    void GetNextAssoc( POSITION& rPos, CString& rKey, void*& rVal ) const
    {
        rKey = CString( m_it->first.c_str( ) );
        rVal = m_it->second;
        ++m_it;
        rPos = ( m_it == m_map.end( ) ) ? NULL : (POSITION)1;
    }

  private:
    std::unordered_map<std::string, void*>                          m_map;
    mutable std::unordered_map<std::string, void*>::const_iterator  m_it;
};

//---------------------------------------------------------------------------
//  CMmio: a real (small) RIFF reader, enough for the loader's FVER gate.
//---------------------------------------------------------------------------

//  Which convention CMmio::GetRiffChunkInfo reports for a RIFF chunk's
//  dwDataOffset: 8 = at the form type (what the POSIX mmio shim sets,
//  win32_compat.cpp:1040), 12 = past it (the Win32 documentation's reading).
//  The checks run the reader under BOTH, because the game does.
extern int g_iRiffDataOffsetBias;

class CMmio
{
  public:
    CMmio( ) { }
    CMmio( const char* pFile ) { Open( pFile ); }
    virtual ~CMmio( ) { }

    virtual void Open( const char* pFile )
    {
        m_sFileName = pFile ? pFile : "";
        g_mmioLog.push_back( m_sFileName );
        LoadFrom( m_sFileName.c_str( ), 0 );
    }

    DWORD DescendRiff( const char* pForm ) { return DescendRiff( pForm[0], pForm[1], pForm[2], pForm[3] ); }
    DWORD DescendRiff( char a, char b, char c, char d )
    {
        if ( m_buf.size( ) < 12 || memcmp( &m_buf[0], "RIFF", 4 ) != 0 ) Throw( );
        const char form[4] = { a, b, c, d };
        if ( memcmp( &m_buf[8], form, 4 ) != 0 ) Throw( );
        m_riffEnd  = 8 + (size_t)LE32( 4 );
        if ( m_riffEnd > m_buf.size( ) ) m_riffEnd = m_buf.size( );
        m_listEnd  = m_riffEnd;
        m_pos      = 12;

        memset( &m_mckiRiff, 0, sizeof( m_mckiRiff ) );
        m_mckiRiff.ckid    = mmioFOURCC( 'R', 'I', 'F', 'F' );
        m_mckiRiff.fccType = mmioFOURCC( a, b, c, d );
        m_mckiRiff.cksize  = (DWORD)LE32( 4 );
        //  The RIFF sits at the start of the buffer, which is m_fileBase bytes
        //  into the real file (non-zero for a container entry).
        m_mckiRiff.dwDataOffset = (DWORD)( m_fileBase + (size_t)g_iRiffDataOffsetBias );
        return (DWORD)LE32( 4 );
    }

    const MMCKINFO& GetRiffChunkInfo( ) const { return m_mckiRiff; }
    DWORD DescendList( char a, char b, char c, char d )
    {
        const char form[4] = { a, b, c, d };
        size_t     p       = m_pos;
        while ( p + 8 <= m_riffEnd )
        {
            unsigned long cb = LE32( p + 4 );
            if ( memcmp( &m_buf[p], "LIST", 4 ) == 0 && cb >= 4 && memcmp( &m_buf[p + 8], form, 4 ) == 0 )
            {
                m_listEnd = p + 8 + cb;
                m_pos     = p + 12;
                return (DWORD)cb;
            }
            p += 8 + cb + ( cb & 1 );
        }
        Throw( );
        return 0;
    }
    DWORD DescendChunk( char a, char b, char c, char d )
    {
        const char id[4] = { a, b, c, d };
        size_t     p     = m_pos;
        while ( p + 8 <= m_listEnd )
        {
            unsigned long cb = LE32( p + 4 );
            if ( memcmp( &m_buf[p], id, 4 ) == 0 )
            {
                m_chunkEnd = p + 8 + cb;
                m_pos      = p + 8;
                return (DWORD)cb;
            }
            p += 8 + cb + ( cb & 1 );
        }
        Throw( );
        return 0;
    }
    void AscendChunk( ) { m_pos = m_chunkEnd + ( ( m_chunkEnd & 1 ) ? 1 : 0 ); }
    void AscendList( ) { m_pos = m_listEnd; }

    short ReadShort( )
    {
        if ( m_pos + 2 > m_buf.size( ) ) Throw( );
        short v = (short)( (unsigned char)m_buf[m_pos] | ( (unsigned char)m_buf[m_pos + 1] << 8 ) );
        m_pos += 2;
        return v;
    }

    const char* GetFileName( ) const { return m_sFileName.c_str( ); }

  protected:
    void LoadFrom( const char* pPath, size_t off )
    {
        m_buf.clear( );
        m_fileBase = off;
        FILE* fp = NULL;
        fopen_s( &fp, pPath, "rb" );
        if ( fp == NULL ) Throw( );
        fseek( fp, 0, SEEK_END );
        long len = ftell( fp );
        fseek( fp, (long)off, SEEK_SET );
        if ( (size_t)len > off )
        {
            m_buf.resize( (size_t)len - off );
            size_t got = fread( &m_buf[0], 1, m_buf.size( ), fp );
            m_buf.resize( got );
        }
        fclose( fp );
    }
    static void Throw( ) { throw 1; }
    unsigned long LE32( size_t at ) const
    {
        return (unsigned long)(unsigned char)m_buf[at] | ( (unsigned long)(unsigned char)m_buf[at + 1] << 8 ) |
               ( (unsigned long)(unsigned char)m_buf[at + 2] << 16 ) |
               ( (unsigned long)(unsigned char)m_buf[at + 3] << 24 );
    }

    std::string       m_sFileName;
    std::vector<char> m_buf;
    MMCKINFO          m_mckiRiff = {};
    size_t            m_fileBase = 0;   // where m_buf[0] sits in the real file
    size_t            m_pos      = 0;
    size_t            m_riffEnd  = 0;
    size_t            m_listEnd  = 0;
    size_t            m_chunkEnd = 0;
};

class CMmioEmbeddedFile : public CMmio
{
  public:
    // The game hands the CONTAINER's handle, its path and the entry's offset.
    // The fixture only needs the last two.
    CMmioEmbeddedFile( HANDLE, const char* pFileName, unsigned long long off )
    {
        m_sFileName = pFileName ? pFileName : "";
        g_mmioLog.push_back( m_sFileName );
        LoadFrom( m_sFileName.c_str( ), (size_t)off );
    }
};

//---------------------------------------------------------------------------
//  The "locate the data file" dialog datafile.cpp also defines. None of it is
//  reachable from the checks; it only has to compile and link.
//---------------------------------------------------------------------------

#define IDS_INSERT_CD 1
#define IDS_BAD_DATA_FILE 2
#define IDS_DATA_FILTERS 3
#define IDS_CD_CHECKING 4
#define IDS_CD_NOT_READY 5
#define IDS_CD_NO_FILE 6
#define IDS_CD_OK 7
#define IDC_SEL_MSG 8
#define IDC_BROWSE 9

#define DECLARE_MESSAGE_MAP( )
#define afx_msg
#define IDD_SELECT_CD 100
#define BEGIN_MESSAGE_MAP( cls, base ) void cls##_TestMessageMap( ) {
#define ON_BN_CLICKED( id, fn ) (void)0;
#define END_MESSAGE_MAP( ) }

inline void DDX_Text( CDataExchange*, int, CString& ) { }

#undef SetDlgItemText

class CDialog
{
  public:
    CDialog( int, CWnd* ) { }
    virtual ~CDialog( ) { }
    virtual void DoDataExchange( CDataExchange* ) { }
    virtual void OnOK( ) { }
    int          DoModal( ) { return IDCANCEL; }
    void         EndDialog( int ) { }
    BOOL         UpdateData( BOOL = TRUE ) { return TRUE; }
    void         UpdateWindow( ) { }
    void         SetDlgItemText( int, const char* ) { }
};

class CFileDialog
{
  public:
    CFileDialog( BOOL, const char*, const CString&, DWORD, const CString&, CWnd* ) { }
    int         DoModal( ) { return IDCANCEL; }
    CString     GetPathName( ) { return CString( "" ); }
};

// NOTE: the real windward/wind22/include/datafile.h is NOT included here. Its
// own first line is #include "stdafx.h", and MSVC resolves a quoted include
// from the INCLUDING FILE'S directory first, so it would pull in the real
// wind22 stdafx.h (SDL, CDIB, the whole stack) instead of this shim. The runner
// therefore copies datafile.h into the build directory with that one line
// dropped, and the fixture includes THAT - generated from the shipped header on
// every run, so it cannot drift from it.

#endif  // DATA_TEST_STDAFX_H
