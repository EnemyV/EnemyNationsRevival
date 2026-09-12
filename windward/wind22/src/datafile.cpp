//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "en_logpath.h"   // EnLogPath - logs to the launch dir, not the exe dir
#include "_windwrd.h"
#include "io.h"
#include "w22_settings.h"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CDataFile theDataFile;

char CDataFile::aDatafileMagic[4] = {'W', 'S', 'D', 'F'};

// GH #8: a bad ENations.dat killed the game silently (uncaught
// ERR_DATAFILE_NO_ENTRY, no window, no log). Two cases share that code but need
// opposite user fixes: no datafile open = .dat missing; open but entry absent =
// wrong/truncated .dat. Old diagnostics were OutputDebugString, invisible to
// players. Log to the same file as CPU:/FONT:.
static bool DataFileExists( const char* pszPath ) {
    if ( !pszPath || !*pszPath ) return false;
    FILE* f = fopen( pszPath, "rb" );
    if ( !f ) return false;
    fclose( f );
    return true;
}

// Absolute path of <exe dir>/<pszLeaf>, or empty on failure. GetModuleFileNameA is
// shimmed on POSIX (/proc/self/exe, _NSGetExecutablePath).
static CString DataFileBesideExe( const char* pszLeaf ) {
    char exePath[ 1024 ] = { 0 };
    if ( GetModuleFileNameA( NULL, exePath, sizeof( exePath ) - 1 ) <= 0 )
        return CString( "" );
    char* p1 = strrchr( exePath, '\\' );
    char* p2 = strrchr( exePath, '/' );
    char* slash = ( p1 > p2 ) ? p1 : p2;
    if ( !slash ) return CString( "" );
    *( slash + 1 ) = '\0';
    return CString( exePath ) + pszLeaf;
}

// The loose data set's marker file: <patch dir>\manifest.txt, written next to the
// extracted entries by tools/data/dat_extract.py in the discussion repo. Returns the
// data ROOT that holds the marker - the DIRECTORY, not the marker path - or "" when
// there is none. Probed cwd-relative first (that is how the entry lookups themselves
// resolve the patch dir) and then beside the exe, because the run directory and the exe
// directory are not always the same folder. Opened with CFile, not the raw fopen of
// DataFileExists: on POSIX CFile::Open normalises the backslash, fopen does not.
//
// It returns the ROOT because the caller has to be able to USE it. This helper used to
// return the marker PATH, which _Init only logged - m_pPatchDir stayed set from the
// original relative pPatchDir, so a marker accepted beside the exe was followed by entry
// opens under the CWD and every entry missed. The beside-the-exe fallback the helper
// advertised could therefore never work (WinAstra review 16). One root is selected here
// and _Init uses that same one for the entry probes.
static CString LooseDataManifest( const char* pPatchDir ) {
    if ( !pPatchDir || !*pPatchDir ) return CString( "" );

    CString aRoot[ 2 ];
    aRoot[ 0 ] = CString( pPatchDir );             // as configured: resolved against the cwd
    aRoot[ 1 ] = DataFileBesideExe( pPatchDir );   // <exe dir>\<patch dir>, absolute

    for ( int iOn = 0; iOn < 2; iOn++ ) {
        if ( aRoot[ iOn ].IsEmpty() ) continue;
        CFile test;
        if ( test.Open( aRoot[ iOn ] + "\\manifest.txt",
                        CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) != FALSE ) {
            test.Close();
            return aRoot[ iOn ];
        }
    }
    return CString( "" );
}

static void LogDataFileProblem( const char* pszMsg ) {
    OutputDebugString( pszMsg );
    OutputDebugString( "\n" );
    FILE* fp = fopen( EnLogPath( "GameWindow_Debug.log" ).c_str(), "a" );
    if ( fp ) {
        fprintf( fp, "DATAFILE: %s\n", pszMsg );
        fclose( fp );
    }
}

CDataFile::CDataFile() {
    m_countryCode = DEF_COUNTRY_CODE;
    m_pPatchDir = NULL;
    m_pDataFile = NULL;
    m_pFileMap = NULL;
#ifdef _DEBUG
    m_bNegativeSeekCheck = FALSE;
    m_lastPos = 0;
#endif
}

CDataFile::~CDataFile() {
    //  Call close, just in case the file
    //  wasn't closed.  Close is safe to call
    //  multiple times.
    Close();
}

static EnLocateDataFileFn s_pfnLocate = NULL;

void SetLocateDataFileHandler( EnLocateDataFileFn pfn ) { s_pfnLocate = pfn; }
EnLocateDataFileFn GetLocateDataFileHandler( ) { return s_pfnLocate; }

static BOOL GetFileName(CString &strFileName) {

    // Registered picker wins. The legacy path below is dead on every platform:
    // CFileDialog is an IDCANCEL stub since the MFC removal, GetDriveType always
    // answers DRIVE_FIXED so the CD branch is unreachable, and on POSIX
    // MessageBoxA returns IDOK which fails the != IDYES test immediately.
    if (s_pfnLocate) {
        char picked[1024] = { 0 };
        if (!s_pfnLocate(strFileName, picked, (int)sizeof(picked)))
            return (FALSE);
        if (!picked[0] || !DataFileExists(picked))
            return (FALSE);
        CString sMsg;
        sMsg.Format("user picked '%s'", picked);
        LogDataFileProblem(sMsg);
        strFileName = picked;
        return (TRUE);
    }


    CString sTmp(strFileName);
    _fullpath(strFileName.GetBuffer(258), sTmp, 256);
    strFileName.ReleaseBuffer(-1);

    // get the drive name
    CString sDrive(strFileName);
    int iInd = sDrive.Find('\\');
    if (iInd >= 0)
        sDrive.ReleaseBuffer(iInd + 1);

    // if it's a CD we prompt them to insert the CD
    if (GetDriveType(sDrive) == DRIVE_CDROM) {
        CDlgSelCD dlg;
        dlg.m_sFileName = strFileName;
        dlg.m_strMsg.LoadString(IDS_INSERT_CD);
        char sBuf[2];
        sBuf[0] = toupper(strFileName[0]);
        sBuf[1] = 0;
        csPrintf(&dlg.m_strMsg, (char const *) sBuf);
        if (dlg.DoModal() != IDOK) {
            TRAP();
            return (FALSE);
        }

        strFileName = dlg.m_sFileName;
    } else

        // not a CD - prompt for the location
    {
        CString sMsg;
        sMsg.LoadString(IDS_BAD_DATA_FILE);
        csPrintf(&sMsg, (char const *) strFileName);
        if (::MessageBoxA(NULL, sMsg, "Enemy Nations", MB_YESNO | MB_ICONSTOP) != IDYES)
            return (FALSE);

        CString sFilters;
        sFilters.LoadString(IDS_DATA_FILTERS);
        CFileDialog dlg(TRUE, "dat", strFileName, OFN_FILEMUSTEXIST, sFilters, NULL);
        if (dlg.DoModal() != IDOK)
            return (FALSE);
        strFileName = dlg.GetPathName();
    }

    return (TRUE);
}

BOOL CDataFile::Init(const char *pFilename, int iRifVer, BOOL bErr) {

    m_iRifVer = iRifVer;

    CString strFileName;
    // Use Win32 command line — skip exe name, grab first argument if present
    const char* pCmdArgs = ::PathGetArgsA(::GetCommandLineA());
    if ((!bErr) && pCmdArgs && pCmdArgs[0]) {
        TRAP();
        strFileName = pCmdArgs;
    } else {
        CString sDefault = CString(".\\") + pFilename;
        strFileName = w22::GetProfileString("Game", "DataFile", sDefault);
    }
    // The path above comes from HKCU Game/DataFile and is written back on every
    // success, so an old install pins an absolute path that outlives it and the
    // ENations.dat shipped beside the exe is never tried. Reported as "cannot find
    // ENations.dat even when it is in the same folder as the exe" (GH #8).
    // If the pinned path is gone, fall back to the copy beside the exe.
    if (!DataFileExists(strFileName)) {
        CString sBeside = DataFileBesideExe(pFilename);
        if (!sBeside.IsEmpty() && DataFileExists(sBeside)) {
            CString sMsg;
            sMsg.Format("pinned '%s' not found, falling back to '%s'",
                        (const char*)strFileName, (const char*)sBeside);
            LogDataFileProblem(sMsg);
            strFileName = sBeside;
        }
    }

    CString sPatch = w22::GetProfileString("Game", "Patch", "data");
    if (!sPatch.IsEmpty())
        if (sPatch[sPatch.GetLength() - 1] == '\\')
            sPatch.ReleaseBuffer(sPatch.GetLength() - 1);

    // if it's an error we force looking for a new file
    if (bErr) {
        if (!GetFileName(strFileName))
            return (FALSE);
    }

    // open the data file
    for (; TRUE;) {
        try {
            theDataFile._Init(strFileName, sPatch, iRifVer);
            // Pin only a path that really opened. Since 015 phase 2 _Init can
            // return without a container (loose-only), and writing back the name
            // of a file we could not open would pin a dead path for the next run.
            // The pin itself is unchanged - QA is keeping it for now.
            if (m_pDataFile)
                w22::WriteProfileString("Game", "DataFile", strFileName);
            // Report opened vs NOT opened. This used to log "using <path>" on the
            // success path for a file that was never opened, which is worse than
            // silence. Say which actually happened. GH #8.
            {
                CString sMsg;
                if (m_pDataFile)
                    sMsg.Format("opened '%s'", (const char*)strFileName);
                else
                    sMsg.Format("NOT OPENED '%s' (loose-only; entries from the patch dir)",
                                (const char*)strFileName);
                LogDataFileProblem(sMsg);
            }
            return (TRUE);
        }

        catch (...) {
            if (!GetFileName(strFileName))
                return (FALSE);
        }
    }

    ASSERT (FALSE);
    return (FALSE);
}

// Read the data file.
void CDataFile::_Init(const char *pFilename, const char *pPatchDir, int iRifVer) {

    m_iRifVer = iRifVer;

    // The root every entry probe below resolves against. It is the caller's pPatchDir
    // unless loose-only mode selects a different one (see LooseDataManifest).
    CString sPatchRoot( pPatchDir != NULL ? pPatchDir : "" );

    if (pFilename) {
        m_pDataFile = new CStdioFile;

        if (m_pDataFile == NULL) {
            ThrowError(ERR_OUT_OF_MEMORY);
        }
        if (m_pDataFile->Open(pFilename, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) == FALSE) {
            // Historically this swallowed the failure into "patch-dir-only" mode
            // (@d3724edb, macOS port) on the premise that cross-platform builds ship
            // only a loose data/ tree; that was false at the time - the shipped data/
            // held terrain PNGs and ZERO .rif files - so it only pushed the failure
            // into an uncaught ERR_DATAFILE_NO_ENTRY with no window and no message,
            // and GH #8 made it throw again so Init's catch runs the picker.
            // 015 phase 2 makes the original premise true, but only when it IS true:
            // a complete extracted data set (tools/data/dat_extract.py) leaves a
            // manifest.txt beside the entries, and that file is the marker that says
            // "loose-only is intended here". With it, run loose-only; without it,
            // still throw so the user gets the picker and the GH #8 message.
            delete m_pDataFile;
            m_pDataFile = NULL;
            {
                CString sMsg;
                sMsg.Format("could not open '%s'", pFilename);
                LogDataFileProblem(sMsg);
            }
            CString sLooseRoot = LooseDataManifest(pPatchDir);
            if (sLooseRoot.IsEmpty())
                ThrowError(ERR_DATAFILE_OPEN);
            // Probe entries under the root the marker was ACTUALLY found in. Found
            // cwd-relative, this is the caller's pPatchDir unchanged and nothing moves;
            // found beside the exe, it is that absolute directory - and without this
            // line OpenAsMMIO/OpenAsFile would keep probing <cwd>\<patch dir> and miss
            // every entry we just accepted the marker for.
            sPatchRoot = sLooseRoot;
            {
                CString sMsg;
                sMsg.Format("loose-only mode: entries come from the patch dir ('%s')",
                            (const char*)sPatchRoot);
                LogDataFileProblem(sMsg);
            }
        } else {
        m_sFileName = pFilename;

        m_pFileMap = new CMapStringToPtr;
        if (m_pFileMap == NULL) {
            ThrowError(ERR_OUT_OF_MEMORY);
        }

        //  The two entries in the file should be the magic number of the file and the 
        //  size of the header in bytes.
        // On-disk fields are 32-bit. Use LONG (==int32_t on Linux, ==long on
        // Win32) NOT `long` — a bare `long` is 64-bit on Linux (LP64) and would
        // make this header 16 bytes instead of the 8 the file actually has.
        struct {
            char aMagicNum[4];
            LONG tableSize;
        } dfHdr {};
        if (m_pDataFile->Read(&dfHdr, sizeof(dfHdr)) != sizeof(dfHdr)) {
            ThrowError(ERR_DATAFILE_READ);
        }

        // Check for magic number
        if (strncmp(dfHdr.aMagicNum, aDatafileMagic, 4) != 0) {
            ThrowError(ERR_DATAFILE_BAD_MAGIC);
        }

        //  Read in the entire file table before constructing the map.
        //  The input file is buffered, but this will still be faster.
        char *pFileTableBuff = new char[dfHdr.tableSize];
        if (pFileTableBuff == NULL)
            ThrowError(ERR_OUT_OF_MEMORY);
        if (m_pDataFile->Read(pFileTableBuff, dfHdr.tableSize) != (UINT) dfHdr.tableSize)
            ThrowError(ERR_DATAFILE_READ);

        char *pBuff = pFileTableBuff;
        while (pBuff < pFileTableBuff + dfHdr.tableSize) {
            //  Get the string length (on-disk 32-bit: LONG, not long — LP64).
            LONG stringLength = *(LONG *) pBuff;
            pBuff += sizeof(LONG);

            //  Save a pointer to the string
            char *pStr = pBuff;
            pBuff += stringLength;

            //  Get the offset.
            LONG fileOffset = *(LONG *) pBuff;
            pBuff += sizeof(LONG);

            //  Add the string/offset pair to the map.
            //  Can ThrowError CMemoryException.
            _strlwr(pStr);
            (*m_pFileMap)[pStr] = (void *)(intptr_t) fileOffset;   // offset smuggled through void* (x64: value-preserving)
        }

        //  Delete the file table buffer, which is no longer
        //  needed.
        delete[] pFileTableBuff;
        } // end else (master archive opened successfully)
    }

    if (pPatchDir) {
        // sPatchRoot == pPatchDir unless loose-only mode resolved a different root.
        m_pPatchDir = new CString(sPatchRoot);
        // The ROOT keeps the case it was given. It used to be force-lower-cased
        // here, a no-op on Windows but wrong on a case-sensitive volume: an exe-dir
        // root such as "/opt/EN/Data" became "/opt/en/data" and every entry probe
        // below missed. Case-folding belongs to the relative entry path we
        // synthesise, not to a directory the user chose (015 plan phase 2c).
        if (m_pPatchDir == NULL)
            ThrowError(ERR_OUT_OF_MEMORY);
    }
}

void CDataFile::Close() {
    delete m_pDataFile;
    m_pDataFile = NULL;

    delete m_pFileMap;
    m_pFileMap = NULL;

    delete m_pPatchDir;
    m_pPatchDir = NULL;
}

void CDataFile::SetCountryCode(int countryCode) {
    m_countryCode = countryCode;
}

BOOL CDataFile::GetContainerEntrySize(const char *pEntryPath, DWORD &dwSize) {
    dwSize = 0;
    if ((pEntryPath == NULL) || (m_pDataFile == NULL) || (m_pFileMap == NULL))
        return FALSE;

    // The map is keyed on the lower-cased path, the same way _Init built it.
    CString sKey(pEntryPath);
    sKey.MakeLower();

    void *pvStart;
    if (m_pFileMap->Lookup(sKey, pvStart) == FALSE)
        return FALSE;
    const DWORD dwStart = (DWORD)(intptr_t) pvStart;

    // Entries are concatenated and the TOC carries offsets only, so this entry
    // ends at the next-higher offset, or at EOF for the last one. That is the
    // same rule tools/data/dat_extract.py splits the container with, so a loose
    // entry and its container original are the same bytes.
    DWORD dwEnd = m_pDataFile->GetLength();
    POSITION pos = m_pFileMap->GetStartPosition();
    while (pos != NULL) {
        CString sName;
        void   *pvOff;
        m_pFileMap->GetNextAssoc(pos, sName, pvOff);
        const DWORD dwOff = (DWORD)(intptr_t) pvOff;
        if ((dwOff > dwStart) && (dwOff < dwEnd))
            dwEnd = dwOff;
    }

    if (dwEnd <= dwStart)
        return FALSE;
    dwSize = dwEnd - dwStart;
    return TRUE;
}

CMmio *CDataFile::OpenAsMMIO(const char *pFilename, const char *pRif) {
    //  Get the relative path of the file for which to search.
    CString file(pFilename);
    CString path;
    if (pFilename == NULL)
        path.Format("language\\%d\\%d.rif", m_countryCode, m_countryCode);
    else
        path = file + "\\" + file + ".rif";
    path.MakeLower();

    //  Check to see if the file exists in the patch directory.
    if (m_pPatchDir) {
        CString patchPath = *m_pPatchDir + "\\" + path;

        CFile test;
        if (test.Open(patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) != FALSE) {
            //  Close the file so we can re-open it as an mmio file.
            //  Otherwise we'd need to keep the CFile around so it's
            //  destructor wouldn't close the file handle.
            test.Close();
            CMmio *pNewFile = new CMmio(patchPath);
            if (pNewFile == NULL)
                ThrowError(ERR_OUT_OF_MEMORY);

            // does the version match?
            try {
                pNewFile->DescendRiff(pRif);
                pNewFile->DescendList('F', 'V', 'E', 'R');
                pNewFile->DescendChunk('D', 'A', 'T', 'A');
                int iRifVer = pNewFile->ReadShort();
                pNewFile->AscendChunk();
                pNewFile->AscendList();

                if (iRifVer == m_iRifVer) {
                    // go back to begining (hack!!)
                    delete pNewFile;
                    pNewFile = new CMmio(patchPath);
                    if (pNewFile == NULL)
                        ThrowError(ERR_OUT_OF_MEMORY);
                    return pNewFile;
                }
            }

            catch (...) {
                OutputDebugString("CDataFile::OpenAsMMIO: check version fail\n");
            }

            delete pNewFile;
        }

        // we now look in the patch dir (users version)
        // Case-fold this probe as well, but the LEAF only. The nested probe above
        // lower-cases its whole relative path (path.MakeLower() at the top of this
        // function) and joins it onto the root as given; this one built its leaf
        // from the original-case argument, so on a case-sensitive volume a
        // lower-case loose set was found nested and missed flat (015 plan 3c,
        // measured on mac). Folding the JOINED path would fold the patch ROOT too,
        // which is the phase-2c defect: the root is the user's install path and its
        // case is not ours to change. No-op on Windows either way.
        CString leaf;
        if (pFilename == NULL)
            leaf.Format("%d.rif", m_countryCode);
        else
            leaf = file + ".rif";
        leaf.MakeLower();
        patchPath = *m_pPatchDir + CString("\\") + leaf;

        if (test.Open(patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) != FALSE) {
            //  Close the file so we can re-open it as an mmio file.
            //  Otherwise we'd need to keep the CFile around so it's
            //  destructor wouldn't close the file handle.
            test.Close();
            CMmio *pNewFile = new CMmio(patchPath);
            if (pNewFile == NULL)
                ThrowError(ERR_OUT_OF_MEMORY);

            // does the version match?
            try {
                pNewFile->DescendRiff(pRif);
                pNewFile->DescendList('F', 'V', 'E', 'R');
                pNewFile->DescendChunk('D', 'A', 'T', 'A');
                int iRifVer = pNewFile->ReadShort();
                pNewFile->AscendChunk();
                pNewFile->AscendList();

                if (iRifVer == m_iRifVer) {
                    // go back to begining (hack!!)
                    delete pNewFile;
                    pNewFile = new CMmio(patchPath);
                    if (pNewFile == NULL)
                        ThrowError(ERR_OUT_OF_MEMORY);
                    return pNewFile;
                }
            }

            catch (...) {
                OutputDebugString("CDataFile::OpenAsMMIO: check version fail\n");
            }

            delete pNewFile;
        }
    }

    //  If here, the file does not exist in the 
    //  patch directory or there is no patch directory.
    //  If a datafile was opened, look for it in the
    //  datafile.
    if (m_pDataFile) {
        //  Look for the file path in the map.  If 
        //  it is not found, if searching for language 
        //  file try searching for US version;  otherwise,
        //  return NULL.
        void *dummy;
        if (m_pFileMap->Lookup(path, dummy) == FALSE) {
            if (pFilename == NULL && m_countryCode != DEF_COUNTRY_CODE) {
                m_countryCode = DEF_COUNTRY_CODE;
                return OpenAsMMIO(NULL, pRif);
            }
            // .dat opened but the entry is missing: wrong/old/truncated file.
            // Fatal (nothing catches the throw), so name the file and say so.
            {
                CString sMsg;
                sMsg.Format("entry '%s' missing from '%s'", (const char*)path,
                            (const char*)m_sFileName);
                LogDataFileProblem(sMsg);

                static bool bTold = false;
                if (!bTold) {
                    bTold = true;
                    CString sBox;
                    sBox.Format("This ENations.dat cannot be used.\n\n"
                                "File:\n%s\n\nMissing entry: %s\n\n"
                                "It is from an older or different version of the game. "
                                "Use the ENations.dat shipped with this release.",
                                (const char*)m_sFileName, (const char*)path);
                    ::MessageBoxA(NULL, sBox, "Enemy Nations", MB_OK | MB_ICONERROR);
                }
            }
            ThrowError(ERR_DATAFILE_NO_ENTRY);
        }

        //  If here, file was found in the map.  Seek to
        //  that position in the file.
        //  If negative seek checks between CMmio's are 
        //  desired, they should be done here.
        long fileOffset = (long)(intptr_t) dummy;   // void*-smuggled offset (x64: value-preserving)
        long offsetFromHere = fileOffset - (long) m_pDataFile->GetPosition();

#ifdef _DEBUG
        if ( m_bNegativeSeekCheck == TRUE && fileOffset < m_lastPos )
            WarnNegativeSeek( pFilename, m_lastPos, fileOffset );
#endif

        m_pDataFile->Seek(offsetFromHere, CFile::current);

//#ifdef _DEBUG
        auto lastPos = m_pDataFile->GetPosition();
//#endif

        //  Create a new CMmioEmbeddedFile object.  It will be positioned 
        //  at the current file offset.
        // BUGBUG - problem
        CMmio *pNewFile = new CMmioEmbeddedFile(m_pDataFile->m_hFile, m_sFileName, lastPos);
        if (pNewFile == NULL) {
            OutputDebugString(
                    "CDataFile::OpenAsMMIO: failed to allocate CMmioEmbeddedFile, returning ERR_OUT_OF_MEMORY\n");
            ThrowError(ERR_OUT_OF_MEMORY);
        }

        return pNewFile;
    }

    //  If here, file was not found in patch dir ( or
    //  no patch dir was given ), and no datafile was
    //  opened, so return NULL ( no file found ).
    // No container open. Since 015 phase 2 that is the loose-only install: _Init
    // only returns without a container when the patch dir carries a manifest.txt,
    // so getting here means a LOOSE ENTRY is missing, of the wrong FVER, or
    // malformed - not that ENations.dat is missing. Name the entry; the old text
    // sent players hunting for a file the install does not use. Fatal and
    // unrecoverable (nothing catches this), so say it once.
    {
        char cwd[MAX_PATH] = { 0 };
        ::GetCurrentDirectoryA(sizeof(cwd), cwd);
        CString sPatch = m_pPatchDir ? *m_pPatchDir : CString("(none)");
        CString sMsg;
        sMsg.Format("loose entry '%s' not usable under '%s'. cwd='%s'", (const char*)path,
                    (const char*)sPatch, cwd);
        LogDataFileProblem(sMsg);

        static bool bTold = false;
        if (!bTold) {
            bTold = true;
            CString sBox;
            sBox.Format("A game data file is missing or unusable.\n\n"
                        "Entry: %s\n\nLooked under: %s\n\nIn: %s\n\n"
                        "Re-extract the game data set, or put ENations.dat next to "
                        "enations.exe.", (const char*)path, (const char*)sPatch, cwd);
            ::MessageBoxA(NULL, sBox, "Enemy Nations", MB_OK | MB_ICONERROR);
        }
    }
    ThrowError(ERR_DATAFILE_NO_ENTRY);
    return NULL;
}

CFile *CDataFile::OpenAsFile(const char *pFilename) {

    // Try the name exactly as given first.
    // POSIX: an absolute path starts with '/'. GetFileName() of an embedded MMIO is the
    // .dat's OWN full path (OpenAsMMIO:487) - music.cpp re-opens the container through
    // here so the payload offsets line up. The old qualified-path test (X:, UNC, leading
    // '/') never matched on mac/linux, so the open fell through to the files\ map and
    // threw ERR_DATAFILE_NO_ENTRY = no music/voice handles on POSIX from a byte-identical
    // .dat. 015 phase 2 broke it the other way too: with no container, GetFileName() is
    // the loose entry's patch-dir path ("data\music\music.rif"), which is RELATIVE, so
    // the same test missed and the name was mangled into "files\data\music\music.rif".
    // Trying the name as given covers both, and still only succeeds for a name that opens
    // as a real file on disk.
    if (pFilename != NULL) {
        CFile test;
        if (test.Open(pFilename, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) != FALSE) {
            //  Close the file so we can allocate a new CFile object to
            //  return to the caller.
            test.Close();

            //  Allocate a new CFile object.  By creating and opening the CFile
            //  this way, the CFile dtor will automagically close the file 
            //  handle for us.
            CFile *pNewFile = new CFile;
            if (pNewFile == NULL)
                ThrowError(ERR_OUT_OF_MEMORY);

            if (pNewFile->Open(pFilename, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) == FALSE)
                ThrowError(ERR_PATCHFILE_OPEN);

            return pNewFile;
        }
    }

    // open a FILE
    CString filename(pFilename);
    CString path;
    path = "files\\" + filename;
    path.MakeLower();

    CFile *pRtn = _OpenAsFile(path);
    if (pRtn != NULL)
        return (pRtn);

    // open a RIF
    CString file(pFilename);
    if (pFilename == NULL)
        path.Format("language\\%d\\%d.rif", m_countryCode, m_countryCode);
    else
        path = file + "\\" + file + ".rif";
    path.MakeLower();

    if ((pRtn = _OpenAsFile(path)) != NULL)
        return (pRtn);

    // 015 phase 2: name the entry. In a loose-only install this is a missing
    // loose file, not a missing container, and the caller (music.cpp) swallows
    // the throw - without this line the failure left no trace at all.
    {
        CString sMiss;
        sMiss.Format("no file entry for '%s'", pFilename ? pFilename : "(language)");
        LogDataFileProblem(sMiss);
    }
    ThrowError(ERR_DATAFILE_NO_ENTRY);
    return NULL;
}

CFile *CDataFile::_OpenAsFile(const char *pFilename) {
    ASSERT(pFilename);

    //  Check to see if the file exists in the patch directory.
    if (m_pPatchDir) {
        CString patchPath = *m_pPatchDir + "\\" + pFilename;

        CFile test;
        if (test.Open(patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) != FALSE) {
            //  Close the file so we can allocate a new CFile object to
            //  return to the caller.
            test.Close();

            //  Allocate a new CFile object.  By creating and opening the CFile
            //  this way, the CFile dtor will automagically close the file 
            //  handle for us.
            CFile *pNewFile = new CFile;
            if (pNewFile == NULL)
                ThrowError(ERR_OUT_OF_MEMORY);

            if (pNewFile->Open(patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) == FALSE)
                ThrowError(ERR_PATCHFILE_OPEN);

            return pNewFile;
        }

        // we now look in the patch dir (users version)
        const char *pName = strrchr(pFilename, '\\');
        if (pName == NULL)
            pName = pFilename;
        else
            pName++;
        patchPath = *m_pPatchDir + CString("\\") + pName;

        if (test.Open(patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) != FALSE) {
            //  Close the file so we can allocate a new CFile object to
            //  return to the caller.
            test.Close();

            //  Allocate a new CFile object.  By creating and opening the CFile
            //  this way, the CFile dtor will automagically close the file 
            //  handle for us.
            CFile *pNewFile = new CFile;
            if (pNewFile == NULL)
                ThrowError(ERR_OUT_OF_MEMORY);

            if (pNewFile->Open(patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) == FALSE)
                ThrowError(ERR_PATCHFILE_OPEN);

            return pNewFile;
        }
    }

    //  If here, the file does not exist in the 
    //  patch directory or there is no patch directory.
    //  If a datafile was opened, look for it in the
    //  datafile.
    if (m_pDataFile) {
        //  Look for the file path in the map.  If 
        //  it is not found, if searching for language 
        //  file try searching for US version;  otherwise,
        //  return NULL.
        void *dummy;
        if (m_pFileMap->Lookup(pFilename, dummy) == FALSE)
            return (NULL);

        //  If here, file was found in the map.  Seek to
        //  that position in the file.
        //  If negative seek checks between CMmio's are 
        //  desired, they should be done here.
        long fileOffset = (long)(intptr_t) dummy;   // void*-smuggled offset (x64: value-preserving)
        long offsetFromHere = fileOffset - (long) m_pDataFile->GetPosition();

#ifdef _DEBUG
        if ( m_bNegativeSeekCheck == TRUE && fileOffset < m_lastPos )
            WarnNegativeSeek( pFilename, m_lastPos, fileOffset );
#endif

        m_pDataFile->Seek(offsetFromHere, CFile::current);

#ifdef _DEBUG
        m_lastPos = m_pDataFile->GetPosition();
#endif

        //  Create a new CFile object.  It will be positioned 
        //  at the current file offset.  Note that this dtor will
        //  not automagically close the file handle, which is what we
        //  want.
        // DNT - fixed to provide duplicate handle - need to set offset
        CFile *pNewFile = new CFile();
        if (pNewFile == NULL)
            ThrowError(ERR_OUT_OF_MEMORY);
        if (pNewFile->Open(m_sFileName, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) == FALSE)
            ThrowError(ERR_OUT_OF_MEMORY);
        pNewFile->Seek(m_pDataFile->GetPosition(), CFile::begin);

        return pNewFile;
    }

    //  If here, file was not found in patch dir ( or
    //  no patch dir was given ), and no datafile was
    //  opened, so return NULL ( no file found ).
    return NULL;
}

CArchive *CDataFile::OpenAsCArchive(const char *pFilename) {
    ASSERT(pFilename);

    //  Get the relative path of the file for which to search.
    CString filename(pFilename);
    CString path;
    path = "files\\" + filename;
    path.MakeLower();

    //  Check to see if the file exists in the patch directory.
    if (m_pPatchDir) {
        CString patchPath = *m_pPatchDir + "\\" + path;

        CFile test;
        if (test.Open(patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) != FALSE) {
            //  Close the file so we can allocate a new CFile object to
            //  return to the caller.
            test.Close();

            //  Allocate a new CFile object.  By creating and opening the CFile
            //  this way, the CFile dtor will automagically close the file 
            //  handle for us.
            CFile *pNewFile = new CFile;
            if (pNewFile->Open(patchPath, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) == FALSE)
                ThrowError(ERR_PATCHFILE_OPEN);
            CArchive *pNewCArchive = new CArchive(pNewFile, CArchive::load);
            if (pNewCArchive == NULL)
                ThrowError(ERR_OUT_OF_MEMORY);

            return pNewCArchive;
        }
    }

    //  If here, the file does not exist in the 
    //  patch directory or there is no patch directory.
    //  If a datafile was opened, look for it in the
    //  datafile.
    if (m_pDataFile) {
        //  Look for the file path in the map.  If 
        //  it is not found, if searching for language 
        //  file try searching for US version;  otherwise,
        //  return NULL.
        void *dummy;
        if (m_pFileMap->Lookup(path, dummy) == FALSE)
            ThrowError(ERR_DATAFILE_NO_ENTRY);

        //  If here, file was found in the map.  Seek to
        //  that position in the file.
        //  If negative seek checks between CMmio's are 
        //  desired, they should be done here.
        long fileOffset = (long)(intptr_t) dummy;   // void*-smuggled offset (x64: value-preserving)
        long offsetFromHere = fileOffset - (long) m_pDataFile->GetPosition();

#ifdef _DEBUG
        if ( m_bNegativeSeekCheck == TRUE && fileOffset < m_lastPos )
            WarnNegativeSeek( pFilename, m_lastPos, fileOffset );
#endif

        m_pDataFile->Seek(offsetFromHere, CFile::current);

#ifdef _DEBUG
        m_lastPos = m_pDataFile->GetPosition();
#endif

        //  Create a new CFile object.  It will be positioned 
        //  at the current file offset.  Note that this dtor will
        //  not automagically close the file handle, which is what we
        //  want.
        CFile *pNewFile = new CFile();
        if (pNewFile == NULL)
            ThrowError(ERR_OUT_OF_MEMORY);
        if (pNewFile->Open(m_sFileName, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary) == FALSE)
            ThrowError(ERR_OUT_OF_MEMORY);
        pNewFile->Seek(m_pDataFile->GetPosition(), CFile::begin);

        CArchive *pNewCArchive = new CArchive(pNewFile, CArchive::load);
        if (pNewCArchive == NULL)
            ThrowError(ERR_OUT_OF_MEMORY);

        return pNewCArchive;
    }

    //  If here, file was not found in patch dir ( or
    //  no patch dir was given ), and no datafile was
    //  opened, so return NULL ( no file found ).
    ThrowError(ERR_DATAFILE_NO_ENTRY);
    return NULL;
}

void CDataFile::CloseCArchive(CArchive *pArchive) {
    ASSERT(pArchive);

    //  Flush the archive and get the file.
    pArchive->Flush();
    CFile *pFile = pArchive->GetFile();

    //  Close the archive.
    pArchive->Close();
    delete pArchive;

    //  Close the file.
    pFile->Close();
    delete pFile;
}

#ifdef TEST_DATAFILE
CDataFile dataFile;

int main( int /*argc*/, char * /*argv*/[] )
{
    //  Create a CDataFile object.
    CString str;
    CMmio *pFile;

    dataFile.Init( "test.dat", "." );

    pFile = dataFile.OpenAsMMIO( "t1" );
    pFile->DescendRiff( 'T','S','T','1' );
    pFile->DescendList( 'T','S','T','L' );
    pFile->DescendChunk( 'D','A','T','A' );
    pFile->ReadString( str );
    pFile->AscendChunk();
    pFile->DescendChunk( 'D','A','T','A' );
    pFile->ReadString( str );
    delete pFile;

    pFile = dataFile.OpenAsMMIO( "t2" );
    pFile->DescendRiff( 'T','S','T','2' );
    pFile->DescendChunk( 'D','A','T','A' );
    pFile->ReadString( str );
    delete pFile;

    pFile = dataFile.OpenAsMMIO( "units" );
    pFile->DescendRiff( 'U','N','I','T' );
    pFile->DescendList( 'T','E','R','N' );
    pFile->DescendChunk( 'N','M','B','R' );
    short x = ( short )pFile->ReadInt();
    delete pFile;

    CFile *pNormFile;
    pNormFile = dataFile.OpenAsFile( "bk.bat" );
    char array[ 64 ];
    pNormFile->Read( array, 64 );
    delete pNormFile;

    CArchive *pArchive;
    pArchive = dataFile.OpenAsCArchive( "bk.bat" );
    dataFile.CloseCArchive( pArchive );

    dataFile.SetCountryCode( 1 );
    pFile = dataFile.OpenAsMMIO( NULL );
    delete pFile;

    dataFile.SetCountryCode( 2 );
    pFile = dataFile.OpenAsMMIO( NULL );
    delete pFile;

    dataFile.SetCountryCode( 12 );
    pFile = dataFile.OpenAsMMIO( NULL );
    delete pFile;

    dataFile.Close();

    dataFile.Init( NULL, "." );

    pFile = dataFile.OpenAsMMIO( "t1" );
    pFile->DescendRiff( 'T','S','T','1' );
    pFile->DescendList( 'T','S','T','L' );
    pFile->DescendChunk( 'D','A','T','A' );
    pFile->ReadString( str );
    pFile->AscendChunk();
    pFile->DescendChunk( 'D','A','T','A' );
    pFile->ReadString( str );
    delete pFile;
    
    pNormFile = dataFile.OpenAsFile( "bk2.bat" );
    pNormFile->Read( array, 64 );
    delete pNormFile;

    pArchive = dataFile.OpenAsCArchive( "bk2.bat" );
    dataFile.CloseCArchive( pArchive );

    dataFile.Close();

    return 0;
}

#endif


/////////////////////////////////////////////////////////////////////////////
// CDlgSelCD dialog


CDlgSelCD::CDlgSelCD(CWnd *pParent /*=NULL*/)
        : CDialog(CDlgSelCD::IDD, pParent) {
    //{{AFX_DATA_INIT(CDlgSelCD)
    m_strMsg = _T("");
    //}}AFX_DATA_INIT
}


void CDlgSelCD::DoDataExchange(CDataExchange *pDX) {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDlgSelCD)
    DDX_Text(pDX, IDC_SEL_MSG, m_strMsg);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgSelCD, CDialog)
                    //{{AFX_MSG_MAP(CDlgSelCD)
                    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
                    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgSelCD message handlers

// we let them pick a location
void CDlgSelCD::OnBrowse() {

    CString sFilters;
    sFilters.LoadString(IDS_DATA_FILTERS);
    CFileDialog dlg(TRUE, "dat", m_sFileName, OFN_FILEMUSTEXIST, sFilters, NULL);
    if (dlg.DoModal() != IDOK) {
        TRAP();
        EndDialog(IDCANCEL);
    }

    m_sFileName = dlg.GetPathName();
    EndDialog(IDOK);
}

void CDlgSelCD::OnOK() {

    UpdateData(TRUE);
    m_strMsg.LoadString(IDS_CD_CHECKING);
    SetDlgItemText(IDC_SEL_MSG, m_strMsg);
    UpdateWindow();

    // this is my attempt to get the system to read the CD.
    CString sCdFile = CString(m_sFileName[0]) + ":\\";
    GetDriveType(sCdFile);
    DWORD dw1, dw2, dw3, dw4;
    GetDiskFreeSpace(sCdFile, &dw1, &dw2, &dw3, &dw4);
    ::Sleep(100);

    // is it readable yet?
    if (!GetVolumeInformation(sCdFile, NULL, 0, &dw1, &dw2, &dw3, NULL, 0)) {
        m_strMsg.LoadString(IDS_CD_NOT_READY);
        UpdateData(FALSE);
        UpdateWindow();
        return;
    }

    CFileStatus fs;
    if ((CFile::GetStatus(m_sFileName, fs) == 0) || (fs.m_size < 1000)) {
        m_strMsg.LoadString(IDS_CD_NO_FILE);
        UpdateData(FALSE);
        UpdateWindow();
        return;
    }

    m_strMsg.LoadString(IDS_CD_OK);
    UpdateData(FALSE);
    SetDlgItemText(IDC_SEL_MSG, m_strMsg);
    UpdateWindow();

    CDialog::OnOK();
}

#ifdef _DEBUG

void CDataFile::WarnNegativeSeek( const char *pFilename, long oldPos, long newPos )
{

    if (pFilename == NULL)
        pFilename = "NULL";

    char str[ 256 ];
    wsprintf( str, "WARNING! - Negative seek from %ld to %ld while loading %s\n", oldPos, newPos, pFilename );
    OutputDebugString( str );
}

void CDataFile::EnableNegativeSeekChecking()
{
    m_bNegativeSeekCheck = TRUE;
    m_lastPos = 0;
}

void CDataFile::DisableNegativeSeekChecking()
{
    m_bNegativeSeekCheck = FALSE;
}

#endif


