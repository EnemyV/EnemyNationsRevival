#ifndef RIFFFILE_H
#define RIFFFILE_H

#include "stdafx.h"

class CRiffFile
{
	struct LIST
	{
		char	aId[ 4 ];
		DWORD	size;
		char	aName[ 4 ];
	};

	struct CHUNK
	{
		char	aName[ 4 ];
		DWORD	size;
	};

	enum DirectiveToken
	{
		INVALID_TOKEN,
		DEFINE_TOKEN,
		INCLUDE_TOKEN,
		IFDEF_TOKEN,
		ELSE_TOKEN,
		ENDIF_TOKEN
	};

	struct InputFileState
	{
		//  Current, next and next-next characters which 
		//  will be read from the file.  0 at beginning and if 
		//  end of file has been reached.
		char cCurrentChar, cNextChar, cSecondChar;

		int nCurrentLine;

		//  TRUE if the end of file has been reached.  FALSE otherwise.
		BOOL bEOF;

		//  Pointer to the actual input file.
		CStdioFile *pInFile;

		CString name;

		InputFileState *pNextFileState;
	};

	//  Current, next and next-next characters which 
	//  will be read from the file.  0 at beginning and if 
	//  end of file has been reached.
	char cCurrentChar, cNextChar, cSecondChar;

	//  TRUE if the end of file has been reached.  FALSE otherwise.
	BOOL bEOF;

	//  The file currently reading from
	CStdioFile *pInFile;

	//  Pointer to list of file states.  When
	//  an include file is read, the current file
	//  will be added to the list.  When an
	//  EOF is encountered, the first file in the list
	//  will be removed and made into the current file.
	InputFileState *pFileStack;

	//  The file to write the RIFF to.
	CStdioFile *pOutFile;

	//  A map of currently defined symbols.
	CMapStringToPtr	symbolTable;

	CString name;
	int currentLine;

	void ReportError( const char *pErrorStr = NULL );
	char PeekNextChar();
	char PeekSecondChar();
	char GetNextChar();
	void EatWhiteSpace();
	BOOL IsWhiteSpace( char ch );
	void GetRiffCommand();
	void GetChunkName( char *pChunkName );
	void GetChunkStart();
	void ProcessList();
	void ProcessData();
	void ProcessInt();
	void ProcessLong();
	void ProcessFloat();
	void ProcessFixed();
	void ProcessFile();
	void ProcessString();
	void ProcessDirective();
	void ProcessDefine();
	void ProcessInclude();
	void ProcessIfdef();
	void ProcessElse();
	void ProcessEndif();
	void SkipToElseOrEndif();
	void SkipToEndif();
	void SkipToEndOfLine();
	DirectiveToken GetDirectiveToken();
	void PopPreviousFile();
	void PushCurrentFile();
	BOOL OpenInputFile( const char *pInFilename );

	public :

	CRiffFile();
	~CRiffFile();

	void Parse( CString inFile, CString outFile );
};

#endif
