#ifndef DATAHASH_READ_H
#define DATAHASH_READ_H

//---------------------------------------------------------------------------
//
//  Getting the RAW BYTES of a gameplay resource out of the loader
//  (015 area 3, phase 3).
//
//  Include AFTER the headers that define CDataFile, CMmio and CFile - it is an
//  .inl in all but name, the way terrain.inl and unit.inl are used. That is what
//  lets tests/data/test_data_loader.cpp compile these functions against the
//  SHIPPED windward/wind22/src/datafile.cpp and check the byte ranges for real,
//  instead of the arithmetic being static-read-only.
//
//  Everything here asks the LOADER where a resource is, never a fixed disk
//  path, so the bytes hashed are the bytes the game will actually load - loose
//  file, replaced loose file or container entry.
//
//---------------------------------------------------------------------------

#include <string.h>

#include <vector>

namespace endataread {

//  Sanity bound on one gameplay resource. The largest today is 9.rif at about
//  10 MB; this only stops a corrupt header from asking for a huge allocation.
const DWORD kMaxEntryBytes = 64UL * 1024UL * 1024UL;

inline unsigned long HdrLE32( const unsigned char* p )
{
    return ( (unsigned long)p[0] ) | ( (unsigned long)p[1] << 8 ) | ( (unsigned long)p[2] << 16 ) |
           ( (unsigned long)p[3] << 24 );
}

//  Reads the bytes of a RIFF entry exactly as the loader resolves it.
//
//  CMmio cannot hand back a raw span (its Read asserts against the current
//  chunk), so this takes the two facts a DescendRiff leaves behind - which file
//  the resolver picked, and roughly where inside it the form sits - and reads
//  that range with a plain CFile.
//
//  The "roughly" is deliberate. MMCKINFO::dwDataOffset for a RIFF chunk is
//  documented as the start of the data area, and the two implementations this
//  game runs on disagree about whether that is AT the form type or just PAST
//  it: the POSIX mmio shim sets chunkStart+8 on purpose (win32_compat.cpp:1040,
//  so that dwDataOffset + cksize lands on the end of the chunk), while the
//  Win32 documentation reads as chunkStart+12. Picking a side would silently
//  hash the wrong bytes on the other platform and split multiplayer down the
//  middle. So both candidates are tried and the one that actually lands on a
//  header - "RIFF", then this form type - wins, and the LENGTH then comes from
//  the header's own size field rather than from cksize, which has the same
//  ambiguity. A candidate that does not validate is never hashed.
inline bool ReadRiffEntry( CDataFile& datafile, const char* pEntry, const char* pForm,
                           std::vector<unsigned char>& buf )
{
    CMmio*  pMmio = NULL;
    CString sPath;
    DWORD   dwDataOffset = 0;

    try
    {
        pMmio = datafile.OpenAsMMIO( pEntry, pForm );
        if ( pMmio == NULL ) return false;

        pMmio->DescendRiff( pForm );
        dwDataOffset = pMmio->GetRiffChunkInfo( ).dwDataOffset;
        sPath        = pMmio->GetFileName( );
    }
    catch ( ... )
    {
        delete pMmio;
        return false;
    }
    delete pMmio;

    if ( sPath.IsEmpty( ) ) return false;

    CFile file;
    if ( file.Open( sPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) == FALSE ) return false;

    bool bOk = false;
    try
    {
        //  chunkStart+8 first: that is what the POSIX shim reports and what
        //  makes dwDataOffset + cksize land on the end of the chunk.
        static const DWORD adwBack[2] = { 8, 12 };

        for ( int iTry = 0; ( !bOk ) && ( iTry < 2 ); iTry++ )
        {
            if ( dwDataOffset < adwBack[iTry] ) continue;
            const DWORD dwStart = dwDataOffset - adwBack[iTry];

            unsigned char hdr[12];
            file.Seek( (LONG)dwStart, CFile::begin );
            if ( file.Read( hdr, sizeof( hdr ) ) != sizeof( hdr ) ) continue;
            if ( memcmp( hdr, "RIFF", 4 ) != 0 ) continue;
            if ( memcmp( hdr + 8, pForm, 4 ) != 0 ) continue;

            const unsigned long ulLen = HdrLE32( hdr + 4 ) + 8;
            if ( ( ulLen < 12 ) || ( ulLen > kMaxEntryBytes ) ) continue;

            buf.resize( (size_t)ulLen );
            file.Seek( (LONG)dwStart, CFile::begin );
            bOk = ( file.Read( &buf[0], (DWORD)ulLen ) == (DWORD)ulLen );
        }
    }
    catch ( ... )
    {
        bOk = false;
    }
    file.Close( );
    if ( !bOk ) buf.clear( );
    return bOk;
}

//  Reads a plain FILE entry (files\stdgta.dat) as the loader resolves it.
inline bool ReadFileEntry( CDataFile& datafile, const char* pEntry, std::vector<unsigned char>& buf )
{
    CFile* pFile = NULL;
    try
    {
        pFile = datafile.OpenAsFile( pEntry );
    }
    catch ( ... )
    {
        return false;
    }
    if ( pFile == NULL ) return false;

    bool bOk = false;
    try
    {
        //  A loose file comes back at position 0 and IS the entry. A container
        //  entry comes back seeked to its offset inside a 540 MB file, so its
        //  end has to come from the TOC - the loader is the only thing that
        //  knows where the next entry starts.
        const DWORD dwPos = pFile->GetPosition( );
        DWORD       dwLen = 0;

        if ( dwPos == 0 )
            dwLen = pFile->GetLength( );
        else
        {
            CString sKey = CString( "files\\" ) + pEntry;
            if ( datafile.GetContainerEntrySize( sKey, dwLen ) == FALSE ) dwLen = 0;
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
    if ( !bOk ) buf.clear( );
    return bOk;
}

}  // namespace endataread

#endif  // DATAHASH_READ_H
