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

	size_t n = fread(buf, 1, *size, f);
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
		adPcm->nChannels = 2;
		adPcm->nSamplesPerSec = 44100;

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
		adPcm->nChannels = 2;
		adPcm->nSamplesPerSec = 44100;

		if (!adPcm.Prepare(flags))
		{
			printf("ADPCM format prepartion failed\n");
			return FALSE;
		}

		CACMWaveFormat pcm(WAVE_FORMAT_PCM);

		pcm->wBitsPerSample = 16;
		pcm->nChannels = 2;
		pcm->nSamplesPerSec = 44100;
		if (!pcm.Prepare(flags))
		{
			printf("PCM format prepartion failed\n");
			return FALSE;
		}



		if (!ReadRawFile(srcfName, &inBuf, &bufSize))
		{
			return FALSE;
		}

		strm = new CACMStream(pcm, adPcm);

		strm->Open();
		strm->Convert(inBuf, bufSize);
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
	if (inBuf)
		GlobalFree(inBuf);
	return ret;
}



void usage()
{
	printf("usage [-r] toadpcm source destination\n"
			"  -r means that input file is a RAW pcm file\n"
			"  otherwise it is supposed to be a WAV file\n");
};

int main(int argc, char* argv[])
{

	if (argc < 3)
	{
		usage();

		return 1;
	}


	if (!strcmpi(argv[1], "-r"))
	{
		if (argc < 4)
		{
			usage(); 
			return 1;
		}
		return !ConvertPCMRawFile(argv[2], argv[3]);
	}

	return ConvertPCMWaveFile(argv[1], argv[2]);
}
