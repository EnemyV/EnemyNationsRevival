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
	fprintf( stderr, "usage - makeriff mif_file [ rif_file ]\n" );
	exit( 1 );
}

int main( int argc, char *argv[] )
{
	banner();

	if ( argc < 2 || argc > 3 )
		usage();

	CString mifFilename, rifFilename;
	
	mifFilename = argv[ 1 ];
	if ( argc == 2 )
	{
		rifFilename = argv[ 1 ];
		int index = rifFilename.ReverseFind( '.' );
		if ( index != -1 )
			rifFilename.ReleaseBuffer( index );
		rifFilename += ".rif";
	}
	else
		rifFilename = argv[ 2 ];
		
	CRiffFile	rifFile;
	rifFile.Parse( mifFilename, rifFilename );
	
	return 0;
}
