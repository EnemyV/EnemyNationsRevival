#ifndef BMPFILE_H
#define BMPFILE_H

#include "image.h"

class BMPFile
{
	private :

	static word BMP_MAGIC;
	BITMAPFILEHEADER	bmpFileHeader;
	BITMAPINFOHEADER	bmpInfoHeader;
	RGBColor			aPalette[ 256 ];

	int	BytesPerRow();
	int SizeInBytes();
	bool ReadPixels( Image *pImage, bool bTopDown, FILE *pFile );

	public :

	BMPFile() {};
	Image *Load( const char *pFilename );
	bool Save( const char *pFilename, const Image *pImage );
	RGBColor *LoadPalette( const char *pFilename );
	bool SavePalette( const RGBColor *pPalette, const char *pFilename );
};

#endif
