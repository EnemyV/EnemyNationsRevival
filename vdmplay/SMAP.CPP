#include "stdafx.h"
#include "vpparam.h"
#include "vpengine.h"
#include "vplist.h"
#include "smap.h"

void CSimplePlayerMap::Enum(EnumProc p, LPVOID userData)
{
	CMxLock lk(m_mutex);
	PLIterator  iter(m_list);
	
	while(!iter.AtEnd())
	{ 
		PLink* pl = iter();
		if (!p(pl->m_data, userData))
			return;
	}
}

void CSimplePlayerMap::Cleanup()
{
	CMxLock lk(m_mutex);
	PLink*  pl;
	
	while (NULL != (pl = m_list.First()))
	{
		m_list.Remove(pl);
		delete pl;
	}
}

	
CSimplePlayerMap::~CSimplePlayerMap()
{
	Cleanup();
}
	

PLink* CSimplePlayerMap::LookupPlayer(VPPLAYERID id)
{
	PLIterator iter(m_list);
	PLink* p;
	
	while(!iter.AtEnd())
	{
		p = iter();
		if (p->m_data->PlayerId() == id)
			return p;
	}
	return NULL;
}

BOOL CSimplePlayerMap::RemovePlayer(VPPLAYERID id)
{
	CMxLock lk(m_mutex);
	PLink*  pl = LookupPlayer(id);

	if (pl)
	{
		m_list.Remove(pl);
		delete pl;
		return TRUE;
	}
	return FALSE;
}

CPlayer* CSimplePlayerMap::PlayerAtId(VPPLAYERID playerId)
{
	CMxLock lk(m_mutex);
	PLink*  pl = LookupPlayer(playerId);

	if (pl)
	{
		return pl->m_data;
	}
	return NULL;
}

BOOL    CSimplePlayerMap::AddPlayer(CPlayer* p)
{
	CMxLock lk(m_mutex);
	PLink*  pl = LookupPlayer(p->PlayerId());

	VPASSERT(!pl);
	pl = new PLink(p);
	if (!pl)
		return FALSE;
	m_list.Insert(pl);
	return TRUE;
}
	
unsigned CSimpleWSMap::Count()
{
	return m_list.Count();
}

void CSimpleWSMap::Register(CWS* ws)
{
   CMxLock lk(m_mutex);
#ifndef FAST_WS   
   WSLink* l = Lookup(ws);
   if (!l)
   {
	   m_list.Append(new WSLink(ws));
	   ws->Ref();
   }
#else
   if (!Lookup(ws))
   {
	   m_list.Append(ws);
	   ws->Ref();
   }
#endif
}

void CSimpleWSMap::Deregister(CWS* ws)
{
   CMxLock lk(m_mutex);
#ifndef FAST_WS
   WSLink* l = Lookup(ws);
   
   VPASSERT(l != NULL);
   
   l->m_data->Unref();
   m_list.Remove(l);
   delete l;
#else
   if (ws == Lookup(ws))
   {
	   m_list.Remove(ws);
	   ws->Unref();
   }
#endif
}


void CSimpleWSMap::Cleanup()
{
	CMxLock lk(m_mutex);
#ifndef FAST_WS
	WSLink*  l;
	
	while (NULL != (l = m_list.RemoveFirst()))
	{
		l->m_data->Unref();
		delete l;
	}
#else
	CWS* ws;
	while(NULL != (ws = m_list.RemoveFirst()))
		ws->Unref();

#endif
}


CSimpleWSMap::~CSimpleWSMap()
{
	Cleanup();
}

CWS* CSimpleWSMap::Find(CWS* ws)
{
	CMxLock lk(m_mutex);

#ifndef FAST_WS
	WSLink*  l = Lookup(ws);
	
	if (!l)
		return NULL;
	
	return l->m_data;
#else
	return Lookup(ws);
#endif
}


CWS* CSimpleWSMap::FindBySafeLink(CNetLink* link)
{
	CMxLock lk(m_mutex);

	WSIterator  iter(m_list);
	
	while(!iter.AtEnd())
	{ 
#ifndef FAST_WS
		WSLink* l = iter();
		if (l->m_data->IsRemote() && 
			link == ((CRemoteWS*) (l->m_data))->m_safeLink)
			return l->m_data;
#else
		CWS* ws = iter();
		if (ws->IsRemote() &&
			(link == ((CRemoteWS*) ws)->m_safeLink))
			return ws;
#endif


	}

	return NULL;
}

CWS* CSimpleWSMap::FindByAddress(CNetAddress* a)
{
	CMxLock lk(m_mutex);

	WSIterator  iter(m_list);
	
	while(!iter.AtEnd())
	{ 
#ifndef FAST_WS
		WSLink* l = iter();
		if (l->m_data->IsRemote() && 
			a->IsEqual(((CRemoteWS*)(l->m_data))->m_address))
			return l->m_data;
#else
		CWS* ws = iter();
		if (ws->IsRemote() &&
			a->IsEqual(((CRemoteWS*)ws)->m_address))
			return ws;
#endif

	}

	return NULL;
}


#ifndef FAST_WS
WSLink* CSimpleWSMap::Lookup(CWS* ws)
#else
CWS* CSimpleWSMap::Lookup(CWS* ws)
#endif
{
	WSIterator  iter(m_list);
	
	while(!iter.AtEnd())
	{
#ifndef FAST_WS
		WSLink* l = iter();
		if (l->m_data->IsSameWs(ws))
			return l;
#else
		CWS* tWs = iter();
		if ((tWs == ws) || tWs->IsSameWs(ws))
			return tWs;

#endif
	}
	return NULL;
}
			

void CSimpleWSMap::Enum(EnumProc p, LPVOID userData)
{
	CMxLock lk(m_mutex);
	WSIterator  iter(m_list);
	
	while(!iter.AtEnd())
	{
#ifndef FAST_WS
		WSLink* l = iter();
		if (!p(l->m_data, userData))
			return;
#else
		CWS* ws = iter();
		if (!p(ws, userData))
			return;

#endif
	}
}
