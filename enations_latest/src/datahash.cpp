//---------------------------------------------------------------------------
//
//  datahash.cpp - the gameplay data hash (015 area 3, phase 3).
//
//  See datahash.h for what it covers and datahash_walk.h for the byte-level
//  rules. This file is the half that knows about CDataFile: it asks the loader
//  for each gameplay resource and hands the raw bytes to the walker.
//
//---------------------------------------------------------------------------

#include "datahash.h"

#include "datahash_walk.h"
#include "lastplnt.h"
#include "stdafx.h"

#include <vector>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

namespace {

//  How one member of the set is read.
enum EKind
{
    kWholeRiff,   //  a RIFF entry hashed end to end
    kChunkRiff,   //  a RIFF entry, only its named top-level elements
    kWholeFile    //  a plain FILE entry (not RIFF) hashed end to end
};

struct GameplayFile
{
    EKind              eKind;
    const char*        pEntry;    //  the loader's name for it; NULL = the language file
    const char*        pForm;     //  RIFF form type; unused for kWholeFile
    const char* const* ppChunks;  //  wanted element names, kChunkRiff only
    int                cChunks;
};

//  9.rif's gameplay text lists. Everything else in that entry - about 10 MB of
//  SFX voices - is presentation and is not hashed.
const char* const kLangChunks[] = { "LEGL", "RSRH", "MTRL", "RACE", "TERN", "TYPE", "BLDG", "VEHL", "SCEN" };

//  create.rif's RACE list. The rest of that entry is the create-screen bitmaps
//  and the credits.
const char* const kCreateChunks[] = { "RACE" };

//  THE GAMEPLAY SET, in the fixed order the hash feeds it (015 plan section 3
//  and spec section A). Adding something the simulation reads means adding a
//  ROW here, never widening a row to a whole file that also carries art.
const GameplayFile kGameplaySet[] = {
    { kWholeRiff, "units", "UNIT", NULL, 0 },
    { kWholeRiff, "research", "RSRH", NULL, 0 },
    { kWholeRiff, "version", "VERN", NULL, 0 },
    { kWholeFile, "stdgta.dat", NULL, NULL, 0 },
    { kChunkRiff, "create", "CRAT", kCreateChunks, (int)( sizeof( kCreateChunks ) / sizeof( kCreateChunks[0] ) ) },
    { kChunkRiff, NULL, "LANG", kLangChunks, (int)( sizeof( kLangChunks ) / sizeof( kLangChunks[0] ) ) },
};
const int kGameplayCount = (int)( sizeof( kGameplaySet ) / sizeof( kGameplaySet[0] ) );

//  Sanity bound on a single gameplay resource. The largest today is 9.rif at
//  about 10 MB; this only stops a corrupt header from asking for a huge
//  allocation.
const DWORD kMaxEntryBytes = 64UL * 1024UL * 1024UL;

//  Reads the bytes of a RIFF entry exactly as the loader resolves it.
//
//  CMmio cannot hand back a raw span (its Read asserts against the current
//  chunk), so this takes the two facts the descend leaves behind - which file
//  the resolver picked and where inside it the form starts - and reads that
//  range with a plain CFile. Loose or container, the range is the same bytes,
//  which is why a loose install and a container install hash alike.
bool ReadRiffEntry( const char* pEntry, const char* pForm, std::vector<unsigned char>& buf )
{
    CMmio*  pMmio  = NULL;
    CString sPath;
    DWORD   dwStart = 0;
    DWORD   dwLen   = 0;

    try
    {
        pMmio = theDataFile.OpenAsMMIO( pEntry, pForm );
        if ( pMmio == NULL ) return false;

        pMmio->DescendRiff( pForm );
        const MMCKINFO& mck = pMmio->GetRiffChunkInfo( );

        //  mmioDescend leaves dwDataOffset just past the form type, so the file
        //  image starts 12 bytes earlier ( "RIFF", the size, the form type )
        //  and runs 8 + cksize bytes.
        if ( mck.dwDataOffset < 12 )
        {
            delete pMmio;
            return false;
        }
        dwStart = (DWORD)mck.dwDataOffset - 12;
        dwLen   = (DWORD)mck.cksize + 8;
        sPath   = pMmio->GetFileName( );
    }
    catch ( ... )
    {
        delete pMmio;
        return false;
    }
    delete pMmio;

    if ( ( dwLen < 12 ) || ( dwLen > kMaxEntryBytes ) || sPath.IsEmpty( ) ) return false;

    CFile file;
    if ( file.Open( sPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) == FALSE ) return false;

    bool bOk = false;
    try
    {
        file.Seek( (LONG)dwStart, CFile::begin );
        buf.resize( (size_t)dwLen );
        bOk = ( file.Read( &buf[0], dwLen ) == dwLen );
    }
    catch ( ... )
    {
        bOk = false;
    }
    file.Close( );
    return bOk;
}

//  Reads a plain FILE entry (stdgta.dat) as the loader resolves it.
bool ReadFileEntry( const char* pEntry, std::vector<unsigned char>& buf )
{
    CFile* pFile = NULL;
    try
    {
        pFile = theDataFile.OpenAsFile( pEntry );
    }
    catch ( ... )
    {
        return false;
    }
    if ( pFile == NULL ) return false;

    bool bOk = false;
    try
    {
        const DWORD dwPos = pFile->GetPosition( );
        DWORD       dwLen = 0;

        if ( dwPos == 0 )
        {
            //  A loose file: the file IS the entry.
            dwLen = pFile->GetLength( );
        }
        else
        {
            //  Inside the container: the CFile spans the whole 540 MB, so the
            //  end has to come from the TOC.
            CString sKey = CString( "files\\" ) + pEntry;
            if ( theDataFile.GetContainerEntrySize( sKey, dwLen ) == FALSE ) dwLen = 0;
        }

        if ( ( dwLen > 0 ) && ( dwLen <= kMaxEntryBytes ) )
        {
            buf.resize( (size_t)dwLen );
            bOk = ( pFile->Read( &buf[0], dwLen ) == dwLen );
        }
    }
    catch ( ... )
    {
        bOk = false;
    }

    pFile->Close( );
    delete pFile;
    return bOk;
}

void LogDataHash( const char* pMsg )
{
    OutputDebugStringA( pMsg );
    OutputDebugStringA( "\n" );
    theApp.Log( pMsg );
}

}  // namespace

DWORD EnComputeGameplayDataHash( )
{
    unsigned long             h = endatahash::kFnvOffset;
    std::vector<unsigned char> buf;

    for ( int iOn = 0; iOn < kGameplayCount; iOn++ )
    {
        const GameplayFile& gf = kGameplaySet[iOn];

        buf.clear( );
        const bool bRead =
            ( gf.eKind == kWholeFile ) ? ReadFileEntry( gf.pEntry, buf ) : ReadRiffEntry( gf.pEntry, gf.pForm, buf );

        const unsigned char* pBytes = ( bRead && !buf.empty( ) ) ? &buf[0] : NULL;
        const size_t         cbytes = pBytes != NULL ? buf.size( ) : 0;

        if ( !endatahash::HashSetMember( h, iOn, pBytes, cbytes, gf.ppChunks, gf.cChunks ) )
        {
            //  Unreadable or malformed. HashSetMember has folded in its miss
            //  marker, so the value stays deterministic; say which one it was,
            //  because an install in this state will not match anybody.
            CString sMsg;
            sMsg.Format( "[DATAHASH] could not read gameplay entry '%s'",
                         gf.pEntry != NULL ? gf.pEntry : "(language)" );
            LogDataHash( sMsg );
        }
    }

    return (DWORD)( h & endatahash::kMask32 );
}
