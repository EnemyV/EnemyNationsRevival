#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include "assert.h"
#include "debug.h"
#include "wavefile.h"
#include "acmutil.h"

#include <stdio.h>
#include <io.h>


int iChannels = 2;
int iBits = 16;
int iSamples = 22050;


BOOL WriteWaveFile(LPCSTR fileName, LPWAVEFORMATEX fmt, 
				   LPVOID buf, DWORD bufSize)
{

	WaveFile file;

	if (!file.Create(fileName, fmt))
	{
		printf("Error %u creating WAV file %s\n", file.m_mmr, fileName);
		return FALSE;
	}

	UINT s = file.Write((CONST BYTE*)buf, bufSize);
	if (s != bufSize)
	{
		printf("Error %u writing  WAV file %s\n", file.m_mmr, fileName);
		return FALSE;
	}

	return TRUE;
}

BOOL WriteRawFile(LPCSTR fileName, LPVOID buf, DWORD bufSize)
{
	FILE* f = fopen(fileName, "wb");

	if (NULL == f)
	{
		printf("Error creating RAW file %s\n", fileName);
		return FALSE;
	}

	size_t n = fwrite(buf, 1, bufSize, f);
	if (n != bufSize)
	{
		printf("Error writing RAW file %s\n", fileName);
		fclose(f);
		return FALSE;
	}

	fclose(f);
	return TRUE;

}


BOOL ReadRawFile(LPCSTR fileName, LPVOID* buf, LPDWORD size)
{
	FILE* f = fopen(fileName, "rb");

	if (NULL == f)
	{
		printf("Error opening RAW file %s\n", fileName);
		return FALSE;
	}

	*size = filelength(f->_file);
	*buf = GlobalAlloc(GMEM_FIXED, *size);

	if (!*buf)
	{
		printf("Error allocating %u bytes for RAW file %s\n", *size, fileName);
		fclose(f);
		return FALSE;
	}

	size_t n = fread(*buf, 1, *size, f);
	if (n != *size)
	{
		printf("Error reading RAW  file %s\n", fileName);
		GlobalFree(*buf);
		fclose(f);
		*buf = NULL;
		return FALSE;
	}

	fclose(f);
	return TRUE;
}


BOOL ConvertPCMWaveFile(LPCSTR srcfName, LPCSTR dstfName)
{
	WaveFile srcFile;
	LPVOID inBuf = NULL;
	CACMStream* strm = NULL;
	BOOL ret = FALSE;
	const DWORD flags = ACM_FORMATENUMF_WFORMATTAG | 
			ACM_FORMATENUMF_WBITSPERSAMPLE |
			ACM_FORMATENUMF_NCHANNELS |
			ACM_FORMATENUMF_NSAMPLESPERSEC; 

	printf("Converting %s to %s ADPCM file\n", srcfName, 
		dstfName);
	
	if (!srcFile.Open(srcfName))
	{
		printf("Error opening input file %s\n", srcfName);
		return FALSE;
	}

	DWORD s = srcFile.GetNumBytesRemaining();
	inBuf = GlobalAlloc(GMEM_FIXED, s);

	if (!inBuf)
	{
		printf("Not enough memory for input buffer of %u bytes", s);
		return FALSE;
	}

	UINT rs = srcFile.Read((LPBYTE) inBuf, s);

	if (rs != s)
	{
		printf("Got only %u bytes of the %u requested\n", rs, s);
		GlobalFree(inBuf);
		return FALSE;
	}

	try
	{
	
		CACMWaveFormat adPcm(WAVE_FORMAT_ADPCM);

		adPcm->wBitsPerSample = 4;
		adPcm->nChannels = iChannels;
		adPcm->nSamplesPerSec = iSamples;

		if (!adPcm.Prepare(flags))
		{
			printf("ADPCM format prepartion failed\n");
			goto fail;
		}

		strm = new CACMStream(srcFile.m_pwfmt, adPcm);

		strm->Open();
		strm->Convert(inBuf, s);
		printf("Converted %u bytes of PCM to %u bytes of ADPCM\n",
			s, strm->ResultSize());

		if (!WriteWaveFile(dstfName, 
					  adPcm, 
					  strm->ResultData(), 
					  strm->ResultSize()))
			goto fail;
	
		ret = TRUE;

	} 
	catch(const CACMException& ex)
	{
		printf("Problem during conversion:\n\tCode=%u, text=%s\n",
			  ex.m_mmr, ex.m_errText);
		goto fail;
	}

fail:
	delete strm;
	GlobalFree(inBuf);
	return ret;
}

//
// this converts a raw file to a cmp file?
//
BOOL ConvertPCMRawFile(LPCSTR srcfName, LPCSTR dstfName)
{
	LPVOID inBuf = 0;
	DWORD bufSize;
	BOOL ret = FALSE;
	CACMStream* strm;
	const DWORD flags = ACM_FORMATENUMF_WFORMATTAG | 
			ACM_FORMATENUMF_WBITSPERSAMPLE |
			ACM_FORMATENUMF_NCHANNELS |
			ACM_FORMATENUMF_NSAMPLESPERSEC; 

	try
	{
		CACMWaveFormat adPcm(WAVE_FORMAT_ADPCM);

		adPcm->wBitsPerSample = 4;
		adPcm->nChannels = iChannels;
		adPcm->nSamplesPerSec = iSamples;

		if (!adPcm.Prepare(flags))
		{
			printf("ADPCM format prepartion failed\n");
			return FALSE;
		}

		CACMWaveFormat pcm(WAVE_FORMAT_PCM);

		pcm->wBitsPerSample = iBits;
		pcm->nChannels = iChannels;
		pcm->nSamplesPerSec = iSamples;

		// BUGBUG
		// crashing in this prepare()
		if (!pcm.Prepare(flags))
		{
			printf("PCM format prepartion failed\n");
			return FALSE;
		}


		// read in the raw data from the srcfName and store it
		if (!ReadRawFile(srcfName, &inBuf, &bufSize))
		{
			return FALSE;
		}

		// create an object that will do the conversion
		strm = new CACMStream(pcm, adPcm);
//		strm = new CACMStream (adPcm, pcm);	// test reverse

		// perform the conversion
		strm->Open();
		strm->Convert(inBuf, bufSize);

		// output the converted cmp file to disk
		WriteRawFile (dstfName, strm->ResultData (), strm->ResultSize () );

#ifdef BUGBUG
		if (!WriteWaveFile(dstfName, 
				  adPcm, 
//				  pcm, 		// test
				  strm->ResultData(), 
				  strm->ResultSize()))
			goto fail;
#endif

	
		ret = TRUE;
	}
	catch(const CACMException& ex)
	{
		printf("Problem during conversion:\n\tCode=%u, text=%s\n",
			  ex.m_mmr, ex.m_errText);
		goto fail;
	}
	
fail:
	delete strm;
	if (inBuf)
		GlobalFree(inBuf);
	return ret;
}



void usage()
{
	printf("usage codec16 [-r] [-8] [-m] [-11] source destination\n"
			"  -r means that input file is a RAW pcm file\n"
			"  -8 means that it is 8-bit (instead of 16)\n"
			"  -m means that it is mono (instead of stereo)\n"
			"  -11 means that it is 11K (instead of 22K)\n"
			"  otherwise it is supposed to be a WAV file\n");
};

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		usage();

		return 1;
	}

	int iOn = 1;
	BOOL bRaw = FALSE;
	while ( (iOn < argc) && (*(argv[iOn]) == '-') )
		{
		if (!strcmpi(argv[iOn], "-r"))
			bRaw = TRUE;
		else
			if (!strcmpi(argv[iOn], "-8"))
				iBits = 8;
			else
				if (!strcmpi(argv[iOn], "-m"))
					iChannels = 1;
				else
					if (!strcmpi(argv[iOn], "-11"))
						iSamples = 11025;
					else
						{
						usage(); 
						return 1;
						}
		iOn ++;
		}

	if (argc < iOn + 2)
		{
			usage(); 
			return 1;
		}

	// this is a raw file for input
	if ( bRaw )
		{
		printf ("Converting %s -r (%d,%d,%d) %s\n", argv[iOn], iBits, iChannels, iSamples, argv[iOn+1] );
		return !ConvertPCMRawFile(argv[iOn], argv[iOn+1]);
		}

	// this is a wav file for input
	printf ("Converting %s (%d,%d,%d) %s\n", argv[iOn], iBits, iChannels, iSamples, argv[iOn+1] );
	return ConvertPCMWaveFile(argv[iOn], argv[iOn+1]);

}
