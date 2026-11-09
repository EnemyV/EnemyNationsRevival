// test_data_hash.cpp -- the gameplay data hash walker (015 area 3, phase 3).
//
// Compiles the SHIPPED header, enations_latest/src/datahash_walk.h, with cl.exe
// alone: no CMake, no game objects, no data files. Everything it hashes is a
// synthetic RIFF built here, so the checks state properties of the algorithm,
// not of one particular ENATIONS.DAT.
//
// The five properties the spec asks for (015 plan, "Phase 2+3 specification"
// section A and WinAstra spec refinement (i)):
//   1. reordering named elements among UNNAMED ones leaves the hash unchanged
//   2. reordering named elements AMONG THEMSELVES changes it
//   3. editing a gameplay record (a whole-file member) changes it
//   4. editing an art element (an unnamed neighbour) does not
//   5. the same bytes always hash to the same number, on any build

#include "../ai/microtest.h"
#include "../../enations_latest/src/datahash_walk.h"

#include <string>
#include <vector>

namespace {

typedef std::vector<unsigned char> Bytes;

void PutU32LE( Bytes& b, unsigned long v )
{
    b.push_back( (unsigned char)( v & 0xff ) );
    b.push_back( (unsigned char)( ( v >> 8 ) & 0xff ) );
    b.push_back( (unsigned char)( ( v >> 16 ) & 0xff ) );
    b.push_back( (unsigned char)( ( v >> 24 ) & 0xff ) );
}

void PutName( Bytes& b, const char* p4 )
{
    for ( int i = 0; i < 4; i++ ) b.push_back( (unsigned char)p4[i] );
}

// One top-level element of a RIFF: either a plain chunk (id + payload) or a
// LIST (the form type names it).
struct Element
{
    bool        bList;
    std::string sName;     // chunk id, or the LIST's form type
    Bytes       payload;   // for a LIST this is what follows the form type
};

Element Chunk( const char* pId, const std::string& sBody )
{
    Element e;
    e.bList = false;
    e.sName = pId;
    e.payload.assign( sBody.begin( ), sBody.end( ) );
    return e;
}

Element List( const char* pForm, const std::string& sBody )
{
    Element e;
    e.bList = true;
    e.sName = pForm;
    e.payload.assign( sBody.begin( ), sBody.end( ) );
    return e;
}

void AppendElement( Bytes& out, const Element& e )
{
    if ( e.bList )
    {
        PutName( out, "LIST" );
        PutU32LE( out, (unsigned long)( 4 + e.payload.size( ) ) );
        PutName( out, e.sName.c_str( ) );
    }
    else
    {
        PutName( out, e.sName.c_str( ) );
        PutU32LE( out, (unsigned long)e.payload.size( ) );
    }
    out.insert( out.end( ), e.payload.begin( ), e.payload.end( ) );
    // RIFF pads every element to an even length.
    size_t cb = e.bList ? 4 + e.payload.size( ) : e.payload.size( );
    if ( cb & 1 ) out.push_back( 0 );
}

Bytes BuildRiff( const char* pForm, const std::vector<Element>& els )
{
    Bytes body;
    for ( size_t i = 0; i < els.size( ); i++ ) AppendElement( body, els[i] );

    Bytes out;
    PutName( out, "RIFF" );
    PutU32LE( out, (unsigned long)( 4 + body.size( ) ) );
    PutName( out, pForm );
    out.insert( out.end( ), body.begin( ), body.end( ) );
    return out;
}

unsigned long HashChunks( const Bytes& img, const char* const* ppNames, int cNames )
{
    unsigned long h = endatahash::kFnvOffset;
    endatahash::HashNamedChunks( h, img.empty( ) ? NULL : &img[0], img.size( ), ppNames, cNames );
    return h;
}

unsigned long HashMember( int iIndex, const Bytes& img, const char* const* ppNames, int cNames, bool* pbOk = NULL )
{
    unsigned long h  = endatahash::kFnvOffset;
    bool          ok = endatahash::HashSetMember( h, iIndex, img.empty( ) ? NULL : &img[0], img.size( ), ppNames,
                                                 cNames );
    if ( pbOk ) *pbOk = ok;
    return h;
}

// The LANG text lists, exactly as datahash.cpp lists them.
const char* const kLangChunks[] = { "LEGL", "RSRH", "MTRL", "RACE", "TERN", "TYPE", "BLDG", "VEHL", "SCEN" };
const int         kLangCount    = (int)( sizeof( kLangChunks ) / sizeof( kLangChunks[0] ) );

void TestFnvIsTheStandardFunction( )
{
    // FNV-1a 32 over "a" and over "foobar" -- the published test vectors. If
    // this drifts, every other number in this file drifts with it silently.
    unsigned long h = endatahash::kFnvOffset;
    endatahash::FnvBytes( h, (const unsigned char*)"a", 1 );
    CHECK_EQ( h, 0xe40c292cUL );

    h = endatahash::kFnvOffset;
    endatahash::FnvBytes( h, (const unsigned char*)"foobar", 6 );
    CHECK_EQ( h, 0xbf9cf968UL );
}

void TestReorderAmongUnnamed( )
{
    // A language file: two gameplay lists with art (an unnamed DIBB chunk and
    // an unnamed SFX list) interleaved. Moving the art around must not move the
    // hash: an art mod is not a gameplay change.
    std::vector<Element> a;
    a.push_back( Chunk( "DIBB", "...bitmap bytes..." ) );
    a.push_back( List( "RSRH", "research text" ) );
    a.push_back( List( "SFXX", "voice samples" ) );
    a.push_back( List( "BLDG", "building text" ) );

    std::vector<Element> b;
    b.push_back( List( "RSRH", "research text" ) );
    b.push_back( List( "SFXX", "voice samples" ) );
    b.push_back( Chunk( "DIBB", "...bitmap bytes..." ) );
    b.push_back( List( "BLDG", "building text" ) );

    const Bytes imgA = BuildRiff( "LANG", a );
    const Bytes imgB = BuildRiff( "LANG", b );

    CHECK( imgA != imgB );  // the instrument must be able to tell them apart
    CHECK_EQ( HashChunks( imgA, kLangChunks, kLangCount ), HashChunks( imgB, kLangChunks, kLangCount ) );
}

void TestReorderAmongNamed( )
{
    // Swapping two GAMEPLAY lists is a gameplay change and must move the hash.
    std::vector<Element> a;
    a.push_back( List( "RSRH", "research text" ) );
    a.push_back( List( "BLDG", "building text" ) );

    std::vector<Element> b;
    b.push_back( List( "BLDG", "building text" ) );
    b.push_back( List( "RSRH", "research text" ) );

    CHECK( HashChunks( BuildRiff( "LANG", a ), kLangChunks, kLangCount ) !=
           HashChunks( BuildRiff( "LANG", b ), kLangChunks, kLangCount ) );
}

void TestNamedPayloadEditMoves( )
{
    std::vector<Element> a;
    a.push_back( Chunk( "DIBB", "art" ) );
    a.push_back( List( "TYPE", "cost 100" ) );

    std::vector<Element> b;
    b.push_back( Chunk( "DIBB", "art" ) );
    b.push_back( List( "TYPE", "cost 101" ) );

    CHECK( HashChunks( BuildRiff( "LANG", a ), kLangChunks, kLangCount ) !=
           HashChunks( BuildRiff( "LANG", b ), kLangChunks, kLangCount ) );
}

void TestArtEditDoesNotMove( )
{
    // Same gameplay lists, different art payload AND a different art length.
    std::vector<Element> a;
    a.push_back( Chunk( "DIBB", "original sprite bytes" ) );
    a.push_back( List( "TYPE", "cost 100" ) );

    std::vector<Element> b;
    b.push_back( Chunk( "DIBB", "a REPLACEMENT sprite, longer than the original" ) );
    b.push_back( List( "TYPE", "cost 100" ) );

    CHECK_EQ( HashChunks( BuildRiff( "LANG", a ), kLangChunks, kLangCount ),
              HashChunks( BuildRiff( "LANG", b ), kLangChunks, kLangCount ) );
}

void TestCreateRaceListOnly( )
{
    // create.rif is 98% bitmaps and credits; only its RACE list is gameplay.
    const char* const kCreateChunks[] = { "RACE" };

    std::vector<Element> a;
    a.push_back( List( "RACE", "race table" ) );
    a.push_back( List( "BM24", "898 KB of create-screen bitmaps" ) );
    a.push_back( Chunk( "CRDT", "credits" ) );

    std::vector<Element> b;
    b.push_back( List( "RACE", "race table" ) );
    b.push_back( List( "BM24", "REPLACED create-screen bitmaps" ) );
    b.push_back( Chunk( "CRDT", "different credits" ) );

    std::vector<Element> c;
    c.push_back( List( "RACE", "race table EDITED" ) );
    c.push_back( List( "BM24", "898 KB of create-screen bitmaps" ) );
    c.push_back( Chunk( "CRDT", "credits" ) );

    CHECK_EQ( HashChunks( BuildRiff( "CRAT", a ), kCreateChunks, 1 ),
              HashChunks( BuildRiff( "CRAT", b ), kCreateChunks, 1 ) );
    CHECK( HashChunks( BuildRiff( "CRAT", a ), kCreateChunks, 1 ) !=
           HashChunks( BuildRiff( "CRAT", c ), kCreateChunks, 1 ) );
}

void TestWholeFileMemberEdit( )
{
    // units.rif is hashed end to end, so a one-byte edit to a unit record moves
    // the number. Member index 0, matching datahash.cpp's list.
    std::vector<Element> a;
    a.push_back( List( "UNIT", "unit record: hp 100" ) );
    std::vector<Element> b;
    b.push_back( List( "UNIT", "unit record: hp 101" ) );

    const Bytes imgA = BuildRiff( "UNIT", a );
    const Bytes imgB = BuildRiff( "UNIT", b );

    CHECK( HashMember( 0, imgA, NULL, 0 ) != HashMember( 0, imgB, NULL, 0 ) );
    CHECK_EQ( HashMember( 0, imgA, NULL, 0 ), HashMember( 0, imgA, NULL, 0 ) );
}

void TestMemberIndexIsFramed( )
{
    // The same bytes at a different position in the list hash differently, so
    // bytes cannot migrate between members unnoticed.
    std::vector<Element> a;
    a.push_back( List( "UNIT", "payload" ) );
    const Bytes img = BuildRiff( "UNIT", a );

    CHECK( HashMember( 0, img, NULL, 0 ) != HashMember( 1, img, NULL, 0 ) );
}

void TestMalformedIsAMissAndFeedsNothingElse( )
{
    bool  bOk = true;
    Bytes junk;
    junk.push_back( 'N' );
    junk.push_back( 'O' );
    junk.push_back( 'P' );
    junk.push_back( 'E' );
    for ( int i = 0; i < 20; i++ ) junk.push_back( (unsigned char)i );

    const unsigned long hJunk = HashMember( 3, junk, kLangChunks, kLangCount, &bOk );
    CHECK( !bOk );

    // A DIFFERENT malformed image at the same index hashes the same: the walker
    // validates before it feeds, so no partial bytes leak into the number.
    Bytes junk2 = junk;
    junk2[10]   = 0xff;
    bool bOk2   = true;
    CHECK_EQ( hJunk, HashMember( 3, junk2, kLangChunks, kLangCount, &bOk2 ) );
    CHECK( !bOk2 );

    // And a miss is not the same as an empty-but-valid resource.
    bool        bOk3 = true;
    const Bytes empty;
    CHECK_EQ( hJunk, HashMember( 3, empty, kLangChunks, kLangCount, &bOk3 ) );
    CHECK( !bOk3 );
}

void TestTruncatedElementRejected( )
{
    std::vector<Element> a;
    a.push_back( List( "RSRH", "research text" ) );
    Bytes img = BuildRiff( "LANG", a );

    // Claim a payload longer than the image.
    img[16] = 0xff;
    img[17] = 0xff;

    unsigned long h = endatahash::kFnvOffset;
    CHECK( !endatahash::HashNamedChunks( h, &img[0], img.size( ), kLangChunks, kLangCount ) );
    CHECK_EQ( h, endatahash::kFnvOffset );  // nothing was fed
}

void TestTrailingBytesIgnored( )
{
    // The container is a concatenation, so the bytes of the NEXT entry sit
    // right behind this one. The RIFF size field has to bound the walk.
    std::vector<Element> a;
    a.push_back( List( "RSRH", "research text" ) );
    const Bytes img = BuildRiff( "LANG", a );

    Bytes withTail = img;
    for ( int i = 0; i < 64; i++ ) withTail.push_back( (unsigned char)( i * 7 ) );

    CHECK_EQ( HashChunks( img, kLangChunks, kLangCount ), HashChunks( withTail, kLangChunks, kLangCount ) );
}

void TestOddLengthPaddingNotHashed( )
{
    // An odd-length element is followed by a pad byte that is not payload.
    // Two builds of the same data must agree regardless of what the pad holds.
    std::vector<Element> a;
    a.push_back( List( "TERN", "odd" ) );  // 4 + 3 = 7 bytes -> one pad byte
    a.push_back( List( "TYPE", "after the pad" ) );

    Bytes img1 = BuildRiff( "LANG", a );
    Bytes img2 = img1;
    // Find the pad byte: it is the one just before the second element's "LIST".
    size_t padAt = 12 + 8 + 7;
    CHECK_EQ( img2[padAt], 0 );
    img2[padAt] = 0xaa;

    CHECK_EQ( HashChunks( img1, kLangChunks, kLangCount ), HashChunks( img2, kLangChunks, kLangCount ) );
}

}  // namespace

int main( )
{
    TestFnvIsTheStandardFunction( );
    TestReorderAmongUnnamed( );
    TestReorderAmongNamed( );
    TestNamedPayloadEditMoves( );
    TestArtEditDoesNotMove( );
    TestCreateRaceListOnly( );
    TestWholeFileMemberEdit( );
    TestMemberIndexIsFramed( );
    TestMalformedIsAMissAndFeedsNothingElse( );
    TestTruncatedElementRejected( );
    TestTrailingBytesIgnored( );
    TestOddLengthPaddingNotHashed( );
    return microtest::Summary( );
}
