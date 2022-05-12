#include "stdafx.h"
#include "rifffile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CRiffFile::CRiffFile() :
	symbolTable()
{
	cCurrentChar = 0;
	cNextChar = 0;
	cSecondChar = 0;
	bEOF = FALSE;
	pOutFile = NULL;
	currentLine = 0;
	pInFile = NULL;
	pFileStack = NULL;
}

CRiffFile::~CRiffFile()
{
	//  These calls will also close their respective files if the files
	//  were successfully opened.
	delete pInFile;
	delete pOutFile;
}

//  Peeks at the next character in the file.
//  Returns 0 if at end of file.
char CRiffFile::PeekNextChar()
{
	return cNextChar;
}

char CRiffFile::PeekSecondChar()
{
	return cSecondChar;
}

//  Gets the next character in the file.  Returns
//  0 if at end of file.
char CRiffFile::GetNextChar()
{
	cCurrentChar = cNextChar;
	cNextChar = cSecondChar;

	if ( !bEOF )
	{
		if ( pInFile->Read( &cSecondChar, 1 ) == 0 )
		{
			cSecondChar = 0;
			bEOF = TRUE;
		}
	}

	if ( cCurrentChar == '\n' )
		currentLine++;

	return cCurrentChar;
}

//  Skip over white space.and comments.
void CRiffFile::EatWhiteSpace()
{
	char ch = PeekNextChar();
	while( ch == ' ' || ch == '\t' || ch == '\n' || ch == '/' || ch == 0 || 
		( ( cCurrentChar == '\n' || cCurrentChar == 0 ) && ch == '#' ) )
	{
		if ( ch == 0 )
		{
			//  If there are no files on the stack, this is end of the 
			//  base file and we're done.
			if ( pFileStack == NULL )
 				return;
			PopPreviousFile();
		}
		else if ( ch == '/' )
		{
			if ( PeekSecondChar() == '/' )
			{
				//  Found comment, seek to EOL and then
				//  continue scanning white space.
				while( PeekNextChar() != '\n' && PeekNextChar() != 0 )
					( void )GetNextChar();
			}
			else
				//  The slash was not a comment, so we
				//  found a non-whitespace character.
				return;
		}
		else if ( ch == '#' )
		{
			ProcessDirective();
			
			//  Need to _not_ do the GetNextChar(), since the next
			//  char could well be the first char of an included file.
			ch = PeekNextChar();
			continue;
		}

		( void )GetNextChar();
		ch = PeekNextChar();
	}
}

BOOL CRiffFile::IsWhiteSpace( char ch )
{
	return ( ch == ' ' || ch == '\t' || ch == '\n' );
}

void CRiffFile::ReportError( const char *pErrorStr )
{
	//  Report the processing error, then bail.  We don't try to recover.
	fprintf( stderr, "Error parsing file %s at line %d : %s.\n", 
		( const char * )name, currentLine, ( pErrorStr ? pErrorStr : "unknown error" ) );
	fprintf( stderr, "Stopping compilation.\n" );
	exit( -1 );
}

//  Reads a RIFF command from the file.  Returns TRUE
//  or FALSE, based on whether or not the next command
//  in the file is a RIFF command.
void CRiffFile::GetRiffCommand()
{
	//  Remove any leading white space.
	EatWhiteSpace();

	//  Read the four byte command.
	char aCommand[ 4 ];
	aCommand[ 0 ] = GetNextChar();
	aCommand[ 1 ] = GetNextChar();
	aCommand[ 2 ] = GetNextChar();
	aCommand[ 3 ] = GetNextChar();

	if ( strncmp( aCommand, "RIFF", 4 ) != 0 )
	{
		CString	str;
		str.Format( "expected RIFF, got %c%c%c%c", aCommand[ 0 ], aCommand[ 1 ],
			aCommand[ 2 ], aCommand[ 3 ] );
		ReportError( str );
	}
}

void CRiffFile::GetChunkName( char *pChunkName )
{
	ASSERT( pChunkName != NULL );

	//  Remove any leading white space.
	EatWhiteSpace();

	//  Read the four byte name.
	pChunkName[ 0 ] = GetNextChar();
	pChunkName[ 1 ] = GetNextChar();
	pChunkName[ 2 ] = GetNextChar();
	pChunkName[ 3 ] = GetNextChar();

	//  Make sure we didn't hit end of file in the middle
	//  of the name.
	if ( pChunkName[ 0 ] == 0 || pChunkName[ 1 ] == 0 ||
		pChunkName[ 2 ] == 0 || pChunkName[ 3 ] == 0 )
		ReportError( "incomplete chunk name" );
}

void CRiffFile::GetChunkStart()
{
	//  Remove any leading white space or comments.
	EatWhiteSpace();

	//  Read the next character.
	char ch = GetNextChar();
	
	//  Make sure it's the open chunk character we expect.
	if ( ch != '{' )
	{
		CString str;
		str.Format( "expected {, got %c", ch );
		ReportError( str );
	}
}

//  Called to parse the RIFF file.
void CRiffFile::Parse( CString inFile, CString outFile )
{
	if ( OpenInputFile( inFile ) == FALSE )
	{
		CString errString;
		errString.Format( "opening file %s for reading", inFile );
		ReportError( errString );
	}

	pOutFile = new CStdioFile;
	if ( pOutFile == NULL )
		ReportError( "allocating memory" );

	if ( pOutFile->Open( outFile, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeBinary ) == FALSE )
	{
		CString errString;
		errString.Format( "opening file %s for writing", outFile );
		ReportError( errString );
	}

	LIST riff = 
	{
		{ 'R','I','F','F' }, 0, { 0, 0, 0, 0 }
	};

	GetRiffCommand();

	GetChunkName( riff.aName );

	//  Save the position and write the header.  We'll
	//  re-write the header with the actual size later.
	long headerPos = pOutFile->GetPosition();
	pOutFile->Write( &riff, sizeof( riff ) );

	GetChunkStart();

	ProcessList();

	//  Determine how large a chunk was written, seek back to
	//  the beginning of the chunk and write the size, and then
	//  seek back to the end of the chunk.  The size of the chunk
	//  needs to include the last four characters of the list header,
	//  so the actual size is the difference between the current position
	//  and the header position, minus the size of the header, plus four.
	long currentPos = pOutFile->GetPosition();
	riff.size = currentPos - ( headerPos + sizeof( riff ) ) + 4;
	pOutFile->Seek( headerPos, CFile::begin );
	pOutFile->Write( &riff, sizeof( riff ) );
	pOutFile->Seek( currentPos, CFile::begin );

	EatWhiteSpace();
	if ( !bEOF )
		ReportError( "unexpected characters at end of file" );
}

void CRiffFile::ProcessList()
{
	char	aCmdBuf[ 6 ];
	LIST	list = 
	{
		{ 'L','I','S','T' }, 0, { 0, 0, 0, 0 }
	};

	for( ;; )
	{
		EatWhiteSpace();

		//  Read the first character.
		aCmdBuf[ 0 ] = GetNextChar();

		//  If the first character is a }, we have reached
		//  the end of the list.
		if ( aCmdBuf[ 0 ] == '}' )
			break;

		//  Otherwise, read until we hit white space.
		int index = 1;
		while( IsWhiteSpace( PeekNextChar() ) == FALSE && index < sizeof( aCmdBuf ) - 1 )
			aCmdBuf[ index++ ] = GetNextChar();
		aCmdBuf[ index ] = 0;

		if ( strcmp( aCmdBuf, "LIST" ) == 0 )
		{
			GetChunkName( list.aName );

			//  Save the position and write the header.  We'll
			//  re-write the header with the actual size later.
			long headerPos = pOutFile->GetPosition();
			pOutFile->Write( &list, sizeof( list ) );

			GetChunkStart();

			ProcessList();

			//  Determine how large a chunk was written, seek back to
			//  the beginning of the chunk and write the size, and then
			//  seek back to the end of the chunk.
			long currentPos = pOutFile->GetPosition();
			list.size = currentPos - ( headerPos + sizeof( list ) ) + 4;
			pOutFile->Seek( headerPos, CFile::begin );
			pOutFile->Write( &list, sizeof( list ) );
			pOutFile->Seek( currentPos, CFile::begin );
		}
		else if ( strcmp( aCmdBuf, "CHUNK" ) == 0 )
		{
			CHUNK	chunk;
			
			GetChunkName( chunk.aName );

			//  Save the position and write the header.  We'll
			//  re-write the header with the actual size later.
			long headerPos = pOutFile->GetPosition();
			pOutFile->Write( &chunk, sizeof( chunk ) );

			GetChunkStart();

			ProcessData();

			//  Determine how large a chunk was written, seek back to
			//  the beginning of the chunk and write the size, and then
			//  seek back to the end of the chunk.  The length of a chunk
			//  must always be even, so we pad the chunk with an extra
			//  0 byte if necessary.
			long currentPos = pOutFile->GetPosition();
			chunk.size = currentPos - ( headerPos + sizeof( chunk ) );
			if ( chunk.size & 1 )
			{
				char cZero = 0;
				pOutFile->Write( &cZero, 1 );
				chunk.size++;
			}
			pOutFile->Seek( headerPos, CFile::begin );
			pOutFile->Write( &chunk, sizeof( chunk ) );
			pOutFile->Seek( chunk.size, CFile::current );
		}
		else
		{
			CString str;
			str.Format( "unrecognized command %s, expecting CHUNK or LIST", aCmdBuf );
			ReportError( str );
		}
	}
}

void CRiffFile::ProcessData()
{
	char	aCmdBuf[ 7 ];

	for( ;; )
	{
		EatWhiteSpace();

		//  Read the first character.
		aCmdBuf[ 0 ] = GetNextChar();

		//  If the first character is a }, we have reached
		//  the end of the data chunk.
		if ( aCmdBuf[ 0 ] == '}' )
			break;

		//  Otherwise, read until we hit white space.
		int index = 1;
		while( IsWhiteSpace( PeekNextChar() ) == FALSE && index < sizeof( aCmdBuf ) - 1 )
			aCmdBuf[ index++ ] = GetNextChar();
		aCmdBuf[ index ] = 0;

		if ( strcmp( aCmdBuf, "int" ) == 0 )
			ProcessInt();
		else if ( strcmp( aCmdBuf, "short" ) == 0 )
			ProcessInt();
		else if ( strcmp( aCmdBuf, "long" ) == 0 )
			ProcessLong();
		else if ( strcmp( aCmdBuf, "float" ) == 0 )
			ProcessFloat();
		else if ( strcmp( aCmdBuf, "fixed" ) == 0 )
			ProcessFixed();
		else if ( strcmp( aCmdBuf, "file" ) == 0 )
			ProcessFile();
		else if ( strcmp( aCmdBuf, "string" ) == 0 )
			ProcessString();
		else
		{
			CString str;
			str.Format( "unexpected command %s", aCmdBuf );
			ReportError( str );
		}
	}
}

void CRiffFile::ProcessInt()
{
	for( ;; )
	{
		EatWhiteSpace();

		char	aIntBuf[ 7 ];	//  -32768 is longest possible #
		int index = 0;

		if ( PeekNextChar() == '-' )
			aIntBuf[ index++ ] = GetNextChar();

		if ( !isdigit( PeekNextChar() ) )
			ReportError( "found invalid char in int" );

		while( isdigit( PeekNextChar() ) && index < sizeof( aIntBuf ) - 1 )
			aIntBuf[ index++ ] = GetNextChar();
		aIntBuf[ index ] = 0;

		if ( isdigit( PeekNextChar() ) )
			ReportError( "integer overflow" );

		short actualVal = ( short )atoi( aIntBuf );

		//  Make sure no overflow occurred
		char	aResultBuf[ 7 ];
		_itoa( actualVal, aResultBuf, 10 );
		if ( strcmp( aResultBuf, aIntBuf ) != 0 )
			ReportError( "integer overflow" );

		//  Write the short to the file.
		pOutFile->Write( &actualVal, 2 );

		EatWhiteSpace();

		if ( PeekNextChar() != ',' )
			break;

		( void )GetNextChar();
	}
}

void CRiffFile::ProcessLong()
{
	for( ;; )
	{
		EatWhiteSpace();

		char	aIntBuf[ 12 ];	//  -2147483647 is longest possible #
		int index = 0;

		if ( PeekNextChar() == '-' )
			aIntBuf[ index++ ] = GetNextChar();

		if ( !isdigit( PeekNextChar() ) )
			ReportError( "found invalid char in int" );

		while( isdigit( PeekNextChar() ) && index < sizeof( aIntBuf ) - 1 )
			aIntBuf[ index++ ] = GetNextChar();
		aIntBuf[ index ] = 0;

		if ( isdigit( PeekNextChar() ) )
			ReportError( "integer overflow" );

		long actualVal = atoi( aIntBuf );

		//  Make sure no overflow occurred
		char	aResultBuf[ 12 ];
		_itoa( actualVal, aResultBuf, 10 );
		if ( strcmp( aResultBuf, aIntBuf ) != 0 )
			ReportError( "integer overflow" );

		//  Write the long to the file.
		pOutFile->Write( &actualVal, 4 );

		EatWhiteSpace();

		if ( PeekNextChar() != ',' )
			break;

		( void )GetNextChar();
	}
}

void CRiffFile::ProcessFloat()
{
	for( ;; )
	{
		EatWhiteSpace();

		char	aFloatBuf[ 256 ];	//  Arbitrary limit, but should be good enough
									//  Note that I'm not doing range checking on the
									//  index _or_ the float here.  I'm just assuming that
									//  the file being compiled is well-behaved.
		int index = 0;

		if ( PeekNextChar() == '-' )
			aFloatBuf[ index++ ] = GetNextChar();

		if ( !isdigit( PeekNextChar() ) && PeekNextChar() != '.' )
			ReportError( "found invalid char in float" );

		while( isdigit( PeekNextChar() ) )
			aFloatBuf[ index++ ] = GetNextChar();

		if ( PeekNextChar() == '.' )
		{
			aFloatBuf[ index++ ] = GetNextChar();
			while( isdigit( PeekNextChar() ) )
				aFloatBuf[ index++ ] = GetNextChar();
		}

		if ( PeekNextChar() == '+' || PeekNextChar() == '-' )
		{
			aFloatBuf[ index++ ] = GetNextChar();
			if ( PeekNextChar() != 'e' && PeekNextChar() != 'E' )
				ReportError( "badly formed float" );

			aFloatBuf[ index++ ] = GetNextChar();
			if ( !isdigit( PeekNextChar() ) )
				ReportError( "badly formed float" );

			while( isdigit( PeekNextChar() ) )
				aFloatBuf[ index++ ] = GetNextChar();
		}

		aFloatBuf[ index ] = 0;

		float actualVal = ( float )atof( aFloatBuf );

		//  Write the float to the file.
		pOutFile->Write( &actualVal, 4 );

		EatWhiteSpace();

		if ( PeekNextChar() != ',' )
			break;

		( void )GetNextChar();
	}
}

void CRiffFile::ProcessFixed()
{
	for( ;; )
	{
		EatWhiteSpace();

		char	aFloatBuf[ 256 ];	//  Arbitrary limit, but should be good enough
									//  Note that I'm not doing range checking on the
									//  index _or_ the float here.  I'm just assuming that
									//  the file being compiled is well-behaved.
		int index = 0;

		if ( PeekNextChar() == '-' )
			aFloatBuf[ index++ ] = GetNextChar();

		if ( !isdigit( PeekNextChar() ) && PeekNextChar() != '.' )
			ReportError( "found invalid char in float" );

		while( isdigit( PeekNextChar() ) )
			aFloatBuf[ index++ ] = GetNextChar();

		if ( PeekNextChar() == '.' )
		{
			aFloatBuf[ index++ ] = GetNextChar();
			while( isdigit( PeekNextChar() ) )
				aFloatBuf[ index++ ] = GetNextChar();
		}
		aFloatBuf[ index ] = 0;

		//  NOTE :  Don't handle exponents for fixed point numbers.
		//  Seems kind of silly.

		long actualVal = ( long )( atof( aFloatBuf ) * 65536.0 );

		//  Write the fixed point number to the file.
		pOutFile->Write( &actualVal, 4 );

		EatWhiteSpace();

		if ( PeekNextChar() != ',' )
			break;

		( void )GetNextChar();
	}
}

char	copyBuf[ 1024 * 32 ];

void CRiffFile::ProcessFile()
{
	char aFilename[ MAX_PATH ];

	for( ;; )
	{
		EatWhiteSpace();

		int index = 0;
		while( IsWhiteSpace( PeekNextChar() ) == FALSE && PeekNextChar() != ',' && index < sizeof( aFilename ) - 1 )
			aFilename[ index++ ] = GetNextChar();
		aFilename[ index ] = 0;

		if ( IsWhiteSpace( PeekNextChar() ) == FALSE && PeekNextChar() != ',' )
			ReportError( "filename too long" );

		CStdioFile	readFile;
		if ( readFile.Open( aFilename, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary ) == FALSE )
		{
			CString str;
			str.Format( "could not open file %s", aFilename );
			ReportError( str );
		}

		//  Write the length of the file as a long.
		long fileLength = readFile.GetLength();
		pOutFile->Write( &fileLength, sizeof( fileLength ) );

		//  Copy the file.
		long bytesRead;
		do
		{
			bytesRead = readFile.Read( copyBuf, sizeof( copyBuf ) );
			pOutFile->Write( copyBuf, bytesRead );
			fileLength -= bytesRead;
		}
		while( bytesRead == sizeof( copyBuf ) );

		//  Close the file.
		readFile.Close();

		if ( fileLength != 0 )
		{
			CString str;
			str.Format( "could not copy all of file %s", aFilename );
			ReportError( str );
		}
		
		EatWhiteSpace();

		if ( PeekNextChar() != ',' )
			break;

		( void )GetNextChar();
	}
}

void CRiffFile::ProcessString()
{
	for( ;; )
	{
		EatWhiteSpace();

		if ( GetNextChar() != '\"' )
			ReportError( "strings must begin with \"" );

		int allocLen = 80;
		char *pStr = ( char * )malloc( allocLen );
		if ( pStr == NULL )
			ReportError( "Error allocating string buffer" );
		char *pCurrent = pStr;
		int len = 0;

		while( 1 )
		{
			if ( PeekNextChar() == '\n' )
				ReportError( "embedded newline in string" );

			if ( PeekNextChar() == '\"' )
			{
				*pCurrent = 0;
				( void )GetNextChar();
				EatWhiteSpace();
				if ( PeekNextChar() != '\"' )
					break;
				( void )GetNextChar();
			}

			if ( len == allocLen )
			{
				pStr = ( char * )realloc( pStr, allocLen * 2 );
				if ( pStr == NULL )
					ReportError( "Error allocating string buffer" );
				pCurrent = pStr + allocLen;
				allocLen = allocLen * 2;
			}

			*pCurrent++ = GetNextChar();
			len++;
		}

		short strLen = ( short )( len + 1 );
		pOutFile->Write( &strLen, sizeof( strLen ) );
		pOutFile->Write( pStr, strLen );

		free( pStr );

		EatWhiteSpace();
		if ( PeekNextChar() != ',' )
			break;

		( void )GetNextChar();
		EatWhiteSpace();
	}
}

void CRiffFile::ProcessDirective()
{
	DirectiveToken	token = GetDirectiveToken();

	switch( token )
	{
		default :
			ReportError( "internal error - unknown token encountered" );
			break;

		case DEFINE_TOKEN :
			ProcessDefine();
			break;

		case INCLUDE_TOKEN :
			ProcessInclude();
			break;

		case IFDEF_TOKEN :
			ProcessIfdef();
			break;

		case ELSE_TOKEN :
			ProcessElse();
			break;

		case ENDIF_TOKEN :
			ProcessEndif();
			break;
	}
}

void CRiffFile::ProcessDefine()
{
	//  Note that I am only using the first 80 characters.
	char	aNewDefine[ 80 ];
	int		index = 0;

	//  Can't use EatWhiteSpace here, since the definition _has_ to be
	//  on this line and EatWhiteSpace eats across lines and comments.
	while( PeekNextChar() == ' ' || PeekNextChar() == '\t' )
		( void )GetNextChar();

	if ( PeekNextChar() == '\n' )
		ReportError( "#define with nothing defined" );

	while( IsWhiteSpace( PeekNextChar() ) == FALSE && index < sizeof( aNewDefine ) - 1 )
		aNewDefine[ index++ ] = GetNextChar();
	aNewDefine[ index ] = 0;

	//  Needs strupr because command-line defines come in
	//  uppercase from some shells and lowercase from
	//  others, so I just make the whole thing uppercase
	//  so I don't have to think about it.
	strupr( aNewDefine );
	AddDefinition( aNewDefine );

	//  Just eat anything else up to the end of the line.
	SkipToEndOfLine();
}

void CRiffFile::ProcessInclude()
{
	char	aIncludeFilename[ MAX_PATH ];
	int		index = 0;

	while( PeekNextChar() == ' ' || PeekNextChar() == '\t' )
		( void )GetNextChar();

	if ( PeekNextChar() == '\n' )
		ReportError( "#include with no filename" );

	if ( PeekNextChar() == '\"' )
		( void )GetNextChar();

	while( PeekNextChar() != ' ' && PeekNextChar() != '\n' 
		&& PeekNextChar() != '\"' && index < sizeof( aIncludeFilename ) - 1 )
		aIncludeFilename[ index++ ] = GetNextChar();
	aIncludeFilename[ index ] = 0;

	SkipToEndOfLine();

	PushCurrentFile();
	if ( OpenInputFile( aIncludeFilename ) == FALSE )
	{
		CString errString;
		errString.Format( "couldn't open include file %s", aIncludeFilename );
		ReportError( errString );
	}
}

void CRiffFile::ProcessIfdef()
{
	//  Note that I am only using the first 80 characters.
	char	aDefinition[ 80 ];
	int		index = 0;

	//  Can't use EatWhiteSpace here, since the symbol _has_ to be
	//  on this line and EatWhiteSpace eats across lines and comments.
	while( PeekNextChar() == ' ' || PeekNextChar() == '\t' )
		( void )GetNextChar();

	if ( PeekNextChar() == '\n' )
		ReportError( "#ifdef with no definition" );

	while( IsWhiteSpace( PeekNextChar() ) == FALSE && index < sizeof( aDefinition ) - 1 )
		aDefinition[ index++ ] = GetNextChar();
	aDefinition[ index ] = 0;

	void *dummy;
	if ( symbolTable.Lookup( aDefinition, dummy ) == FALSE )
		SkipToElseOrEndif();
}

void CRiffFile::ProcessElse()
{
	SkipToEndif();
}

void CRiffFile::ProcessEndif()
{
	SkipToEndOfLine();
}

void CRiffFile::SkipToElseOrEndif()
{
	for( ;; )
	{
		SkipToEndOfLine();
		( void )GetNextChar();

		if ( PeekNextChar() == '#' )
		{
			DirectiveToken	token = GetDirectiveToken();
			if ( token == IFDEF_TOKEN )
				SkipToEndif();
			else if ( token == ELSE_TOKEN || token == ENDIF_TOKEN )
				break;
		}
		else if ( PeekNextChar() == 0 )
			break;
	}

	SkipToEndOfLine();
}

void CRiffFile::SkipToEndif()
{
	for( ;; )
	{
		SkipToEndOfLine();
		( void )GetNextChar();

		if ( PeekNextChar() == '#' )
		{
			DirectiveToken token = GetDirectiveToken();
			if ( token == IFDEF_TOKEN )
				SkipToEndif();
			else if ( token == ENDIF_TOKEN )
				break;
		}
		else if ( PeekNextChar() == 0 )
			break;
	}

	SkipToEndOfLine();
}

void CRiffFile::SkipToEndOfLine()
{
	while( PeekNextChar() != '\n' && PeekNextChar() != 0 )
		( void )GetNextChar();
}

CRiffFile::DirectiveToken CRiffFile::GetDirectiveToken()
{
	char	aDirective[ 9 ];
	int		index = 0;

	ASSERT( PeekNextChar() == '#' );
	while( IsWhiteSpace( PeekNextChar() ) == FALSE && index < sizeof( aDirective ) - 1 )
		aDirective[ index++ ] = GetNextChar();
	aDirective[ index ] = 0;

	if ( strcmp( aDirective, "#define" ) == 0 )
		return DEFINE_TOKEN;
	else if ( strcmp( aDirective, "#include" ) == 0 )
		return INCLUDE_TOKEN;
	else if ( strcmp( aDirective, "#ifdef" ) == 0 )
		return IFDEF_TOKEN;
	else if ( strcmp( aDirective, "#else" ) == 0 )
		return ELSE_TOKEN;
	else if ( strcmp( aDirective, "#endif" ) == 0 )
		return ENDIF_TOKEN;
	else
	{
		CString str;
		str.Format( "unknown directive %s", aDirective );
		ReportError( str );
	}

	return INVALID_TOKEN;
}

void CRiffFile::PopPreviousFile()
{
	//  If the stack is empty, do nothing.
	if ( pFileStack == NULL )
		return;

	//  Remove the top of the stack.
	InputFileState *pFileState = pFileStack;
	pFileStack = pFileStack->pNextFileState;

	//  Update all of the internal variables.
	cCurrentChar = pFileState->cCurrentChar;
	cNextChar = pFileState->cNextChar;
	cSecondChar = pFileState->cSecondChar;
	currentLine = pFileState->nCurrentLine;
	name = pFileState->name;
	bEOF = pFileState->bEOF;
	delete pInFile;	//  MUST DELETE OLD FILE BEFORE RESTORING POINTER!
	pInFile = pFileState->pInFile;

	delete pFileState;
}

void CRiffFile::PushCurrentFile()
{
	//  Allocate a new file state structure.
	InputFileState *pFileState = new InputFileState;
	if ( pFileState == NULL )
		ReportError( "not enough memory" );

	//  Set the new file state structure.
	pFileState->cCurrentChar = cCurrentChar;
	pFileState->cNextChar = cNextChar;
	pFileState->cSecondChar = cSecondChar;
	pFileState->bEOF = bEOF;
	pFileState->pInFile = pInFile;
	pFileState->nCurrentLine = currentLine;
	pFileState->name = name;

	//  Add the new structure to the top of the stack.
	pFileState->pNextFileState = pFileStack;
	pFileStack = pFileState;
}

BOOL CRiffFile::OpenInputFile( const char *pInFilename )
{
	ASSERT( pInFilename != NULL );

	pInFile = new CStdioFile;
	if ( pInFile == NULL )
		return FALSE;

	if ( pInFile->Open( pInFilename, CFile::modeRead | CFile::typeText | CFile::shareDenyWrite ) == FALSE )
		return FALSE;

	bEOF = FALSE;
	currentLine = 0;

	//  Need to do this twice to initialize the character lookaheads.
	//  After these two calls the first character of the file is available
	//  to PeekNextChar and GetNextChar.
	( void )GetNextChar();
	( void )GetNextChar();

	//  Clear the current character, so we don't confuse the parser with
	//  old data ( happens with includes including includes - the current
	//  char and next char can both be '#'.
	cCurrentChar = 0;

	name = pInFilename;

	return TRUE;
}

void CRiffFile::AddDefinition( const char *pNewDefinition )
{
	//  Check for duplicate define.
	void *dummy;
	if ( symbolTable.Lookup( pNewDefinition, dummy ) != FALSE )
		printf( "Warning - duplicate define %s, line %d\n", pNewDefinition, currentLine );
	else
		//  Add this definition to the table of definitions we have.
		//  It doesn't matter what value we put in there, we'll be checking for
		//  existence, not result.
		//  This could throw a memory exception.
		symbolTable[ pNewDefinition ] = ( void * )1;
}
