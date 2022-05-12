#ifndef __ASTRLIST_H__
#define __ASTRLIST_H__


#define _USE_GENERIC

#include "astring.h"
#if !defined(_USE_GENERIC)
#include <rw/tvslist.h>

typedef RWTValSlist<AString> AStrList;


#else
#include <rw/gslist.h>

class AStrList;

#if 0
class AStrLink
{
	AString str;

friend AStrList;	
};
#endif

declare(RWGSlist, AString)

class AStrList: private RWGSlist(AString) 
{



public:
	RWGSlist(AString)::entries;
	void clearAndDestroy()
	{
		AString* p;
		while(NULL != (p = get()))
			delete p;
	}
	
	AStrList& operator=(const AStrList& l)	
	{
		clear();
		append(l);
		return *this;
	}
	
	AStrList() { }
	
	AStrList(const AStrList& l) 
	{
		append(l);
	}   
	
	void append(const AStrList& l)
	{
		RWGSlistIterator(AString) i((AStrList&) l);
		const AString* p;
		
		while(0 != (p = i()))
		{  
			append(*p);
		}
	}
		
	void append(const AString& s)		
	{
		 AString* l = new AString(s);
		 RWGSlist(AString)::append(l);
	}	 

	void clear() { 	clearAndDestroy(); }
	~AStrList() { clear(); }
	
	const AString& at(unsigned i) const
	{
		const AString* l = RWGSlist(AString)::at(i);
		return *l;
	}	

	const AString& operator[](unsigned i) const
	{
		return at(i);
	}
	
};

#endif


#endif

