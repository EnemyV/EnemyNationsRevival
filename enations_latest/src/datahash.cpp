//---------------------------------------------------------------------------
//
//  datahash.cpp - the gameplay data hash (015 area 3, phase 3).
//
//  See datahash.h for what it covers, datahash_walk.h for the byte-level rules
//  and datahash_read.h for how the bytes are pulled out of the loader. This
//  file is only the LIST: which resources are gameplay and in what order.
//
//---------------------------------------------------------------------------

#include "datahash.h"

#include "datahash_read.h"
#include "datahash_walk.h"
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
    kWholeRiff,  //  a RIFF entry hashed end to end
    kChunkRiff,  //  a RIFF entry, only its named top-level elements
    kWholeFile   //  a plain FILE entry (not RIFF) hashed end to end
};

struct GameplayFile
{
    EKind              eKind;
    const char*        pEntry;    //  the loader's name for it; NULL = the language file
    const char*        pForm;     //  RIFF form type; unused for kWholeFile
    const char* const* ppChunks;  //  wanted element names; NULL = the whole image
    int                cChunks;
};

//  9.rif's gameplay text lists. Everything else in that entry - about 10 MB of
//  SFX voices - is presentation and is not hashed.
const char* const kLangChunks[] = { "LEGL", "RSRH", "MTRL", "RACE", "TERN", "TYPE", "BLDG", "VEHL", "SCEN" };

//  create.rif's RACE list. The rest of that entry is the create-screen bitmaps
//  and the credits.
const char* const kCreateChunks[] = { "RACE" };

//  THE GAMEPLAY SET, in the fixed order the hash feeds it (015 plan section 3
//  and spec section A). Something the simulation reads that is not here gets a
//  ROW of its own; never widen a row to a whole file that also carries art.
const GameplayFile kGameplaySet[] = {
    { kWholeRiff, "units", "UNIT", NULL, 0 },
    { kWholeRiff, "research", "RSRH", NULL, 0 },
    { kWholeRiff, "version", "VERN", NULL, 0 },
    { kWholeFile, "stdgta.dat", NULL, NULL, 0 },
    { kChunkRiff, "create", "CRAT", kCreateChunks, (int)( sizeof( kCreateChunks ) / sizeof( kCreateChunks[0] ) ) },
    { kChunkRiff, NULL, "LANG", kLangChunks, (int)( sizeof( kLangChunks ) / sizeof( kLangChunks[0] ) ) },
};
const int kGameplayCount = (int)( sizeof( kGameplaySet ) / sizeof( kGameplaySet[0] ) );

void LogDataHash( const char* pMsg )
{
    OutputDebugStringA( pMsg );
    OutputDebugStringA( "\n" );
}

}  // namespace

DWORD EnComputeGameplayDataHash( )
{
    unsigned long              h = endatahash::kFnvOffset;
    std::vector<unsigned char> buf;

    for ( int iOn = 0; iOn < kGameplayCount; iOn++ )
    {
        const GameplayFile& gf = kGameplaySet[iOn];

        buf.clear( );
        const bool bRead = ( gf.eKind == kWholeFile )
                               ? endataread::ReadFileEntry( theDataFile, gf.pEntry, buf )
                               : endataread::ReadRiffEntry( theDataFile, gf.pEntry, gf.pForm, buf );

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
