#ifndef __ACMUTIL_H__
#define __ACMUTIL_H__

template<class S> class CACMStruct
{
public:
	S FAR * operator->() { return &s; }
	const S FAR * operator->() const { return &s; }
	operator S FAR *() { return &s; }
	CACMStruct()
	{
		memset(&s, 0, sizeof(s));
		s.cbStruct = sizeof(s);
	}
public:
	S s;
};


class CACMException
{
public:
	const char* m_errText;
	MMRESULT    m_mmr;
	CACMException(MMRESULT mmr, const char* err) 
		: m_mmr(mmr), m_errText(err) { }

};


class CACMWaveFormat
{
public:
	CACMWaveFormat(WORD fmt = WAVE_FORMAT_UNKNOWN, DWORD sz=0);
	~CACMWaveFormat();

	LPWAVEFORMATEX 	operator->() { return m_fmt; }
	operator LPWAVEFORMATEX() { return m_fmt; }

	BOOL Prepare(DWORD flags);
	DWORD Size() const
	{
		ASSERT(m_fmt);
		return sizeof(*m_fmt) + m_fmt->cbSize;
	}

protected:

	static BOOL PASCAL fmtEnumCallback(
		HACMDRIVERID hadid,
		LPACMFORMATDETAILS pafd, 
		DWORD dwInstance, 
		DWORD fdwSupport);


public:
	LPWAVEFORMATEX m_fmt;
	DWORD		   m_size;
};

class CACMStream
{

	LPWAVEFORMATEX m_srcFmt;
	LPWAVEFORMATEX m_dstFmt;
	HACMSTREAM	   m_strm;
	CACMStruct<ACMSTREAMHEADER> m_hdr;
	MMRESULT       m_mmr;

public:
	CACMStream(LPWAVEFORMATEX src, LPWAVEFORMATEX dst);
	~CACMStream();


	void	 Open();
	void 	 Close();
	void	 Convert(LPVOID src, DWORD size); 
	
	// return the actual size of the destination buffer
	DWORD	 ResultSize() const
	{
		ASSERT(m_strm);
		ASSERT(m_hdr->fdwStatus & 
			ACMSTREAMHEADER_STATUSF_DONE);

		return m_hdr->cbDstLengthUsed;
	}

	// return a pointer to the actual converted data
	LPVOID	 ResultData()
	{
		ASSERT(m_strm);
		ASSERT(m_hdr->fdwStatus & 
			ACMSTREAMHEADER_STATUSF_DONE);
		return m_hdr->pbDst;
	}

	MMRESULT LastError() const { return m_mmr; }
	void Prepare(LPVOID src, DWORD size);
	void Unprepare();
};


// convert raw ADPCM data to raw PCM data
// Usage:
//  Create the object
//   Call Convert
//   Call ResultData and ResultSize to get the converted data
// 	Destroy the object
//  Notes:
//   Convert may be called multiple times
//   however the converted data should be read IMMIDIATELY after
//   the invocation of Convert and the invocation of Convert 
//   should be done only after you've completely done with the
//   result data buffer
//   Also next invocations of Convert will be more efficient
//   if the input buffer pointer is always the same one
class CADPCMtoPCMConvert
{
public:

	CADPCMtoPCMConvert();
	~CADPCMtoPCMConvert();


	LPVOID ResultData()
	{
		ASSERT(m_stream);
		return m_stream->ResultData();
	}

	DWORD ResultSize()
	{
		ASSERT(m_stream);
		return m_stream->ResultSize();
	}

	void Convert(LPVOID buf, DWORD size)
	{
		ASSERT(m_stream);
		m_stream->Convert(buf, size);
	}

private:
	CACMWaveFormat m_pcmFmt;
	CACMWaveFormat m_adpcmFmt;
	CACMStream*      m_stream;
};



#endif