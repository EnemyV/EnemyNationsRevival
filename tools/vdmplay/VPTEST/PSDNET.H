#ifndef __PSDNET_H__
#define __PSDNET_H__

class Datagram: public CVPLink
{
public:
	WORD m_size;
	LPVOID m_data;
	
	Datagram(LPVOID data, WORD size, int from) : m_from(from), m_size(size)
	{
		m_data = new char[m_size];
		if (m_data)
			_fmemcpy(m_data, data, m_size);
	}
	~Datagram() 
	{
		delete m_data;
	}
	
		
};

declare(VPList, Datagram);

class DataQueue
{
	
	public:
		VPList(Datagram) m_list;
		void Add(Datagram* d)  { m_list.Append(d); }
		unsigned Count() const { return m_list.Count(); }
		Datagram* Get() 
		{
			if (!Count()) return NULL;
			Datagram* d = m_list.First();
			m_list.Remove(d);
			return d;
		}
		
		~DataQueue()
		{
			Datagram* d;
			while(NULL != (d = Get()))
			{
				delete d;
			}
		}
		
};
	


class CPseudoNet: public CNetInterface
{
	
public:
      
      
      class PNAddress: public CNetAddress
	  {
		 public:
		 friend class CPseudoNet;
		 int  index;
		
		 void GetNormalForm(LPVPNETADDRESS ap)
		 {
			 *(int *)ap = index;
		 }
		 
		 BOOL IsEqual(CNetAddress* a)

			 {
			 return index == ((PNAddress*) a)->index;
		 }
	     PNAddress(int i) : index(i) { }
	  };
	 
	  class PNLink: public CNetLink
	  {
		 friend CPseudoNet; 
	     public:
            PNLink*  target;
			CPseudoNet* net;
			int myAddress;
			DataQueue dataQ;
		
			BOOL HasData() const { return dataQ.Count() > 0; }

			CNetAddress* GetRemoteAddress();
			
			~PNLink() 
			{
				if (target)
				{
					ASSERT(target->target == this);
					target->target = NULL;
				}
			}
			
				
			PNLink(int a, CPseudoNet* n) : myAddress(a), target(NULL),
				net(n) { }
			
			BOOL Send(LPVOID data, DWORD dataSize, DWORD flags);
			DWORD Receive(LPVOID buf, DWORD bufSize);
			DWORD ReceiveFrom(LPVOID buf, DWORD bufSize, CNetAddress& from);
			BOOL SendTo(CNetAddress& to, 
			              LPVOID data, 
						  DWORD dataSize, 
						  DWORD flags);

				protected:
				void AcceptData(LPVOID data, DWORD dataSize, int from=-1);

				
		};
	
		virtual void GetAddress(LPVPNETADDRESS addr)
		{
			*(int *)addr = m_address;
		}
		
		CNetLink* MakeSafeLink(CNetAddress* a);
		CNetLink* MakeUnsafeLink();
		CNetAddress* MakeAddress(LPCVPNETADDRESS a);
		virtual CNetAddress* MakeBroadcastAddress();

		CNetLink* MakeListenLink();
		BOOL  Listen();
		void  BecomeDeef();
		
		PNAddress* MakeAddressObject(int a)
		{
			return new PNAddress(a);
		}
		
		void AcceptData(LPVOID data, DWORD dataSize, int from)
		{
			if (m_listenLink) 
				m_listenLink->AcceptData(data, dataSize, from);
		}
		
		
		CPseudoNet(int a) : m_address(a), m_listenLink(NULL)
		{
			ASSERT(!nets[a]);
			_fmemset(m_links, 0, sizeof(m_links));
			nets[a] = this;
		}
		
		static CPseudoNet* CheckNet(int address)
		{
			return nets[address];
		}

		BOOL Poll();
		

		static void SimulationLoop();
		
	 protected:
		 
		 void RegisterLink(PNLink* l);
		 void UnregisterLink(PNLink* l);
	 private:
		 enum { MAXNET = 16, MAXLINK=16 };
		 int  m_address;
		 PNLink*      m_listenLink;
		 PNLink*      m_links[MAXLINK];
		
		 friend PNLink;
		 
		 static CPseudoNet* nets[MAXNET];
};

		 
	

#endif 
