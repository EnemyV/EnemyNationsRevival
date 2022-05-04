#include "stdafx.h"

#include "rifffile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void banner()
{
	fprintf( stdout, "makeriff v2.0 %s %s\nCopyright 1996 Windward Studios All Rights Reserved.\n", __DATE__, __TIME__ );
}

void usage()
{
	fprintf( stderr, "usage - makeriff [ -Ddefine ... ] mif_file [ rif_file ]\n" );
	exit( 1 );
}

int main( int argc, char *argv[] )
{
	CRiffFile	rifFile;

	banner();

	if ( argc < 2 )
		usage();

	int argvIndex;
	for( argvIndex = 1; argvIndex < argc && argv[ argvIndex ][ 0 ] == '-'; argvIndex++ )
	{
		//  Needs strupr because Win95 can't make up it's mind whether to be
		//  case sensitive or not.
		strupr( argv[ argvIndex ] );
		
		if ( argv[ argvIndex ][ 1 ] == 'D' )
			rifFile.AddDefinition( &argv[ argvIndex ][ 2 ] );
		else
			usage();
	}

	if ( argc - argvIndex < 1 || argc - argvIndex > 2 )
		usage();

	CString mifFilename, rifFilename;
	
	mifFilename = argv[ argvIndex ];
	if ( argc - argvIndex == 1 )
	{
		rifFilename = argv[ argvIndex ];
		int index = rifFilename.ReverseFind( '.' );
		if ( index != -1 )
			rifFilename.ReleaseBuffer( index );
		rifFilename += ".rif";
	}
	else
	{
		argvIndex++;
		rifFilename = argv[ argvIndex ];
	}
		
	//  Make sure we processed all of the arguments.
	//  argvIndex should be indexing the last argument,
	//  which is argc - 1.
	ASSERT( argvIndex == argc - 1 );

	rifFile.Parse( mifFilename, rifFilename );
	
	return 0;
}
