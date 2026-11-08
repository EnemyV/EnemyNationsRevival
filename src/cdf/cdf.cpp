#include <afx.h>

//  Magic number for data files.  WSDF = Windward Studio Data File
const char aDatafileMagic[4] = { 'W', 'S', 'D', 'F' };
#define DWORD_ALIGN( X ) ( ( ( X ) + 3 ) & ~3 )

char copyBuf[1024 * 32];

void print_usage()
{
    fprintf( stderr, "usage - cdf infile outfile\n" );
    fprintf( stderr, "\tinfile - A file listing all files to be put in the data file\n" );
    fprintf( stderr, "\toutfile - The name of the output datafile.\n" );
}

int determine_header_size( CStdioFile* inFile, CString* inputString)
{
    //  Count the number of lines in the input file and the
    //  size of each filename, so we know how large a header we need.
    int headerLen = 0;
    while ( inFile->ReadString( *inputString ) != FALSE )
    {
        inputString->TrimLeft( );
        inputString->TrimRight( );
        int length          = inputString->GetLength( );
        int whiteSpaceIndex = inputString->FindOneOf( " \t" );
        if ( whiteSpaceIndex != -1 )
            length = whiteSpaceIndex;

        if ( inputString->ReverseFind( '.' ) != -1 )
        {
            //  Actual filename is files\xxx.
            length = 6 + length;
        }
        else
        {
            //  Actual name is yyy\xxx\xxx.rif.  ReverseFind returns -1 if character
            //  is not found, so this reduces to length + 1 + length + 4 in that case,
            //  for a filename of xxx\xxx.rif
            int nameIndex = inputString->ReverseFind( '\\' ) + 1;
            length        = nameIndex + ( length - nameIndex ) + 1 + ( length - nameIndex ) + 4;
        }

        //  Add the length of this field to the header entry.
        //  Note that we pad the length of the string including
        //  NULL termination to a 4-byte boundary.
        //  The first 4 is for the string length, the second 4 is
        //  for the file offset, the length + 1 is for the NULL terminator.
        headerLen += 4 + DWORD_ALIGN( length + 1 ) + 4;
    }
    return headerLen;
}

void write_magic_number_and_header_size( CStdioFile* outFile, int headerLen )
{
    outFile->SeekToBegin( );
    outFile->Write( aDatafileMagic, sizeof( aDatafileMagic ) );
    outFile->Write( &headerLen, sizeof( headerLen ) );
}

int main( int argc, char* argv[] )
{
    printf( "cdf - Data File maker.  version 1.00 %s, %s\n", __DATE__, __TIME__ );
    printf( "Copyright 1996 Windward Studio  All Rights Reserved\n" );
    printf( "Modernization 2023 Blair \"Bobbias\" Stacey\n");

    if ( argc != 3 )
    {
        print_usage();
        return 1;
    }

    //  Open my input and output files.
    try
    {
        CStdioFile inFile( argv[1], CFile::modeRead | CFile::typeText | CFile::shareDenyWrite );
        CStdioFile outFile( argv[2], CFile::modeCreate | CFile::modeWrite | CFile::typeBinary | CFile::shareExclusive );

        CString inputString;

        int headerLen = determine_header_size( &inFile, &inputString );

        write_magic_number_and_header_size( &outFile, headerLen );

        //  Points to position in header for next file entry to be written.
        //  Starts right after the header size.
        long currentHeaderOffset = sizeof( headerLen ) + sizeof( aDatafileMagic );

        //  Points to position in file for current file data to be written.
        //  Starts right after the header.
        long currentFilePosition = headerLen + sizeof( headerLen ) + sizeof( aDatafileMagic );

        //  Re-read the input file, this time parsing each line and
        //  adding each listed file to the output data file.
        inFile.SeekToBegin( );
        while ( inFile.ReadString( inputString ) != FALSE )
        {
            //  Compute the length of the string.
            inputString.TrimLeft( );
            inputString.TrimRight( );
            CString filename, flags;
            int     whiteSpaceIndex = inputString.FindOneOf( " \t" );
            if ( whiteSpaceIndex != -1 )
            {
                //  Found whitespace in the middle of the string, must
                //  be flags present.
                filename = inputString.Left( whiteSpaceIndex );
                flags    = inputString.Right( inputString.GetLength( ) - whiteSpaceIndex );
                printf( "Processing file %s, flags %s\n", (const char*)filename, (const char*)flags );
            }
            else
            {
                filename = inputString;
                flags    = "";
                printf( "Processing file %s\n", (const char*)filename );
            }

            //  Write the file header.
            if ( filename.ReverseFind( '.' ) != -1 )
            {
                filename = "files\\" + filename;
            }
            else
            {
                int     nameIndex = filename.ReverseFind( '\\' ) + 1;
                CString dir       = filename.Left( nameIndex );
                CString name      = filename.Right( filename.GetLength( ) - nameIndex );
                filename          = dir + name + "\\" + name + ".rif";
            }

            outFile.Seek( currentHeaderOffset, CFile::begin );
            int alignedStringLength = DWORD_ALIGN( filename.GetLength( ) + 1 );
            outFile.Write( &alignedStringLength, sizeof( alignedStringLength ) );
            outFile.Write( (const char*)filename, filename.GetLength( ) + 1 );
            outFile.Seek( alignedStringLength - ( filename.GetLength( ) + 1 ), CFile::current );
            outFile.Write( &currentFilePosition, sizeof( currentFilePosition ) );
            currentHeaderOffset += 8 + alignedStringLength;

            //  Open the file to be added
            CFile addFile;
            if ( addFile.Open( filename, CFile::modeRead | CFile::typeBinary | CFile::shareDenyWrite ) == FALSE )
            {
                fprintf( stderr, "Error adding file %s to the data file.\n", (const char*)filename );
                return 1;
            }

            if ( flags != "" )
            {
                fprintf( stderr, "Warning : ignoring flags %s for file %s\n", (const char*)flags,
                         (const char*)filename );
            }

            //  Write the file.
            outFile.Seek( currentFilePosition, CFile::begin );
            int fileLength = addFile.GetLength( );
            int bytesRead  = 0;
            do
            {
                bytesRead = addFile.Read( copyBuf, sizeof( copyBuf ) );
                outFile.Write( copyBuf, bytesRead );
                fileLength -= bytesRead;
            } while ( bytesRead == sizeof( copyBuf ) );

            if ( fileLength != 0 )
            {
                fprintf( stderr, "Error reading file %s : got %llu out of %llu bytes\n", (const char*)filename,
                         addFile.GetLength( ) - fileLength, addFile.GetLength( ) );
                return 0;
            }

            outFile.Seek( DWORD_ALIGN( addFile.GetLength( ) ) - addFile.GetLength( ), CFile::current );
            currentFilePosition += DWORD_ALIGN( addFile.GetLength( ) );

            addFile.Close( );
        }

        outFile.Flush( );
        outFile.Close( );
        inFile.Close( );
    }
    catch (CFileException* pe)
    {
        fprintf( stderr, "Error opening file %s. Cause = %d\n", (LPCTSTR)pe->m_strFileName, pe->m_cause );

        pe->Delete();
        return 1;
    }







    //  At this point, my header pointer should point to the
    //  start of the data if I did everything correctly.
   // ASSERT( currentHeaderOffset == (int)( headerLen + sizeof( headerLen ) + sizeof( aDatafileMagic ) ) );

    printf( "%s created successfully.  Exiting.\n", argv[2] );
    return 0;
}
