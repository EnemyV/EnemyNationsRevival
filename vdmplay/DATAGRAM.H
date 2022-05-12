#ifndef __DATGARAM_H__
#define __DATAGRAM_H__

class Datagram: public CVPLink
{
public:
	WORD m_size; // actual data size
	WORD m_bufSize;	 // buffer size
	LPSTR m_data;
	WORD   m_offset;
	DWORD  m_time;
	DWORD  m_seq;
	WORD   m_chksum;



	Datagram() : m_size(0), m_data(NULL), m_offset(0) { }
	
	Datagram(WORD bSize) : m_bufSize(bSize), m_size(0), m_offset(0)
	{
#ifdef USE_VPMEM
		m_data = (LPSTR) vpAllocMem(bSize);
#else
		m_data = new char[bSize];
#endif
	}

	Datagram(LPVOID data, WORD size) :  m_size(size), m_offset(0)
	{
		VPASSERT(data);
		VPASSERT(size);
		m_time = GetCurrentTime();

#ifdef USE_VPMEM
		m_data = (LPSTR) vpAllocMem(m_size);
#else
		m_data = new char[m_size];
#endif
		if (m_data)
			_fmemcpy(m_data, data, m_size);

	}

	Datagram(LPVOID data, WORD size, BOOL pfx) :  m_size(size), m_offset(0)
	{
		VPASSERT(data);
		VPASSERT(size);

		m_time = GetCurrentTime();

		if (pfx) 
			m_size += sizeof(WORD);


#ifdef USE_VPMEM
		m_data = (LPSTR) vpAllocMem(m_size);
#else
		m_data = new char[m_size];
#endif
		if (m_data)
		{
			if (pfx)
			{
				*(WORD*)m_data = size;
				_fmemcpy(m_data+sizeof(WORD), data, size);
			}
			else
			{
			 	_fmemcpy(m_data, data, size);
			}

		}
	}

	void SetSum(WORD s) { m_chksum = s; }
	WORD GetSum() const { return m_chksum; }
	void SetSeq(DWORD s) { m_seq = s; }
	DWORD GetSeq() const { return m_seq; }

	BOOL Ok() const { return m_data != NULL; }
	void  AddOffset(WORD o) { m_offset += o; } 
	BOOL  Done() const { return m_offset >= m_size; }

	LPVOID  MoreData()
	{
			return (LPVOID) (m_offset + m_data);
	}

	WORD    DataSize() const
	{
		return m_size - m_offset;
	}
	
	
	virtual ~Datagram() 
	{
#ifdef USE_VPMEM
		vpFreeMem(m_data, m_size);
#else
		delete [] m_data;
#endif
	}
	
		
};

declare(VPList, Datagram);

class DataQueue
{
	
	public:
		VPList(Datagram) m_list;
		
		void Add(Datagram* d)  { m_list.Append(d); }
		unsigned Count() const { return m_list.Count(); }
		
		WORD FirstItemSize() 
		{
			Datagram* d = m_list.First();

			if (d)
				return d->DataSize();

			return 0;
		}

		Datagram* Get() 
		{
			if (!Count()) return NULL;
			Datagram* d = m_list.First();
			m_list.Remove(d);
			return d;
		}
	
		void Insert(Datagram* d)
		{
			m_list.Insert(d);
		}	
		
		~DataQueue()
		{
			Datagram* d;
			while(NULL != (d = Get()))
			{
				delete d;
			}
		}
		
		
		void Lock()  { }
		void Unlock() { }
		
};


#endif
