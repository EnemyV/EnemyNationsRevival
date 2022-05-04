#ifndef TGAFILE_H
#define TGAFILE_H

#include "image.h"

class TGAFile
{
	public :
	Image *Load( const char *pFilename );
};

#endif
