#ifndef __SPLRMAP_H__
#define __SPLRMAP_H__

#define FAST_WS

struct PLink: public CVPLink
{
     CPlayer*  m_data;
	 PLink(CPlayer* p) : m_data(p) { }
	 
};

struct WSLink: public CVPLink
{
	CWS* m_data;
	WSLink(CWS* ws) : m_data(ws) { }
};

declare(VPList, PLink);
typedef VPList(PLink) PList;
declare2(VPListIterator, PList, PLink);

typedef VPListIterator(PList, PLink) PLIterator;

#ifndef FAST_WS
declare(VPList, WSLink);
typedef VPList(WSLink) WSList;
declare2(VPListIterator, WSList, WSLink);
typedef VPListIterator(WSList, WSLink) WSIterator;
typedef WSList WSXList;
typedef WSIterator WSXIterator;
#else
declare(VPList, CWS);
typedef VPList(CWS) WSList;
declare2(VPListIterator, WSList, CWS);
typedef VPListIterator(WSList, CWS) WSIterator;
declare(VPList, WSLink);
typedef VPList(WSLink) WSXList;
declare2(VPListIterator, WSXList, WSLink);
typedef VPListIterator(WSXList, WSLink) WSXIterator;

#endif


class CSimplePlayerMap: public CPlayerMap
{
public:
	virtual BOOL     AddPlayer(CPlayer* p);
	virtual CPlayer* PlayerAtId(VPPLAYERID playerId);
	virtual BOOL     RemovePlayer(VPPLAYERID id);
	virtual DWORD    Count() const { return m_list.Count(); }
    virtual void	 Enum(EnumProc p, LPVOID userData);
	virtual void  	 Cleanup();

	CSimplePlayerMap() { }
	virtual ~CSimplePlayerMap();

protected:
    PLink*  LookupPlayer(VPPLAYERID id);
	
	PList  m_list;
	CMutex  m_mutex;


};

class CSimpleWSMap: public CWSMap
{
public:
	  virtual unsigned Count();
	  virtual void Enum(EnumProc p, LPVOID data);
      virtual void Register(CWS* ws);
	  virtual void Deregister(CWS* ws);
	  virtual CWS* Find(CWS* ws);
	  virtual CWS* FindByAddress(CNetAddress* a);
	  virtual CWS* FindBySafeLink(CNetLink* a);
	  virtual void Cleanup();
	  
	  ~CSimpleWSMap();
	  
protected:
#ifndef FAST_WS
      WSLink* Lookup(CWS* ws);
#else
      CWS* Lookup(CWS* ws);
#endif
	 
protected:
     WSList m_list;
	 CMutex m_mutex;

};
 

#endif 


	