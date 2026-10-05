// Runs the production EnComputeGameplayDataHash against the real loose data
// tree. The runner supplies EN_REAL_DATA_DIR and compiles the shipped loader,
// reader, walker, and aggregate against the same minimal I/O shim as the loader
// fixtures.

#include "../ai/microtest.h"
#include "stdafx.h"
#include "datafile_under_test.h"
#include "w22_settings.h"

#include <stdio.h>

#include "../../windward/wind22/src/datafile.cpp"
#include "datahash_under_test.cpp"

std::vector<std::string> g_openLog;
std::vector<std::string> g_mmioLog;
std::vector<std::string> g_msgBoxes;
int g_iRiffDataOffsetBias = 8;

int EnTestMessageBox( HWND, const char* pText, const char*, UINT )
{
    g_msgBoxes.push_back( pText ? pText : "" );
    return IDOK;
}

namespace w22 {
TestProfile& Profile( )
{
    static TestProfile p;
    return p;
}
}  // namespace w22

namespace {

bool LogContains( const std::vector<std::string>& log, const char* text )
{
    for ( size_t i = 0; i < log.size( ); ++i )
        if ( log[i].find( text ) != std::string::npos ) return true;
    return false;
}

DWORD ComputeRealSet( const char* dataDir )
{
    g_openLog.clear( );
    g_mmioLog.clear( );
    theDataFile._Init( NULL, dataDir, 10 );
    const DWORD hash = EnComputeGameplayDataHash( );

    CHECK( LogContains( g_mmioLog, "units\\units.rif" ) );
    CHECK( LogContains( g_mmioLog, "research\\research.rif" ) );
    CHECK( LogContains( g_mmioLog, "version\\version.rif" ) );
    CHECK( LogContains( g_mmioLog, "create\\create.rif" ) );
    CHECK( LogContains( g_mmioLog, "language\\9\\9.rif" ) );
    CHECK( LogContains( g_openLog, "files\\stdgta.dat" ) );
    theDataFile.Close( );
    return hash;
}

bool FlipLastByte( const std::string& path, unsigned char* original )
{
    FILE* fp = NULL;
    fopen_s( &fp, path.c_str( ), "rb+" );
    if ( fp == NULL ) return false;
    if ( fseek( fp, 0, SEEK_END ) != 0 ) { fclose( fp ); return false; }
    long size = ftell( fp );
    if ( size <= 0 || fseek( fp, size - 1, SEEK_SET ) != 0 ) { fclose( fp ); return false; }
    int byte = fgetc( fp );
    if ( byte == EOF ) { fclose( fp ); return false; }
    *original = (unsigned char)byte;
    if ( fseek( fp, size - 1, SEEK_SET ) != 0 || fputc( byte ^ 1, fp ) == EOF ) { fclose( fp ); return false; }
    fclose( fp );
    return true;
}

bool RestoreLastByte( const std::string& path, unsigned char original )
{
    FILE* fp = NULL;
    fopen_s( &fp, path.c_str( ), "rb+" );
    if ( fp == NULL ) return false;
    if ( fseek( fp, -1, SEEK_END ) != 0 || fputc( original, fp ) == EOF ) { fclose( fp ); return false; }
    fclose( fp );
    return true;
}

}  // namespace

int main( )
{
    const char* dataDir = getenv( "EN_REAL_DATA_DIR" );
    if ( dataDir == NULL || *dataDir == 0 )
    {
        fprintf( stderr, "EN_REAL_DATA_DIR must name a data/ directory\n" );
        return 2;
    }

    // The runner copies the extracted tree beneath its isolated output dir;
    // byte edits below therefore never touch the user's source data.
    const std::string root( dataDir );
    const DWORD base = ComputeRealSet( dataDir );
    CHECK( base != 0 );

    unsigned char original = 0;
    const std::string gameplay = root + "\\units\\units.rif";
    CHECK( FlipLastByte( gameplay, &original ) );
    const DWORD gameplayEdit = ComputeRealSet( dataDir );
    CHECK( gameplayEdit != base );
    CHECK( RestoreLastByte( gameplay, original ) );

    const std::string art = root + "\\effect\\effect.rif";
    CHECK( FlipLastByte( art, &original ) );
    const DWORD artEdit = ComputeRealSet( dataDir );
    CHECK_EQ( artEdit, base );
    CHECK( RestoreLastByte( art, original ) );

    printf( "[data_aggregate] base=%08lx gameplay-edit=%08lx art-edit=%08lx\n", (unsigned long)base,
            (unsigned long)gameplayEdit, (unsigned long)artEdit );
    return microtest::Summary( );
}
