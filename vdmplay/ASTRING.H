#ifndef __ASTRING_H__
#define __ASTRING_H__

#ifndef USE_MFC
#include <rw/cstring.h>
#endif

#include <string.h>

#ifndef bool
#define bool int
#endif

class AString
{
public:

	void cutLine()
	{
		int index = find("\n");
		
		if (index != -1)
			cutFrom(index);
	}


#ifndef USE_MFC
	void cutFrom(int index)
	{
		str = str(index, str.length()-index);
	}
	
	int find(const char* pat) { return str.index(pat); }

    bool operator==(const AString& a)  const 
    {  
    	return str.length() ? str == a.str : a.str.length() == 0;
    }

	bool equal(const AString& a) const 
	{  
		return str.length() ? str == a.str : a.str.length() == 0;
	}

	AString& operator +=(const char* s) { str += s; return *this; }
//	AString& operator +=(const RWCString& s) { str += s; return *this; }
	AString& operator +=(const AString& s) { str += s.str; return *this; }

	AString&	append(const char* cs) { str.append(cs); return *this; }
	AString&	append(const char* cs, size_t N) 
	{ 
		str.append(cs, N); return *this; 
	}
	
	AString&	append(const void* cs, size_t N) 
	{ 
		str.append((const char*)cs, N); return *this;
	}
	
	AString&	append(const RWCString& s) { str.append(s); return *this; }
	AString&	append(const AString& s) { str.append(s.str); return *this; } 
	
	
	int  empty() const { return str.length() == 0; }
	unsigned hash() const { return str.hash(); }
	unsigned length() const { return str.length(); }

	unsigned compare(const void* buf, unsigned len) const;
	unsigned copyTo(void* buf) const;
	void assignChunk(const void* buf, unsigned len);
	void clear() { str.resize(0); }


	AString(const void* buf, unsigned len);
	AString(const void* buf) : str((const char*)buf) { }
	AString() { }
	AString(unsigned len) : str(' ', len) { }
	AString(const AString& s): str(s.str) { }

	operator const char* () const { return (const char*) str; }
	void* data() { return (void*) str.data(); }
	void resize(unsigned l) { str.resize(l); }
	unsigned char at(unsigned i) const { return str[i]; }
    unsigned char operator[](int i) const
    {
    	return (unsigned char) str[(unsigned) i];
    }
    
	unsigned char operator[](unsigned i) const 
	{ 
		return (unsigned char) str[i];
	}
	
	unsigned findChar(char ch, unsigned start=0) const 
	{
		return str.index(&ch, 1, start, str.exact);
	}
	

protected:
	RWCString str;
#endif
};


#ifdef __SC__
inline char* operator=(char* b, const AString& s)	
{
	s.copyTo(b);
	return b;
}

inline unsigned char* operator=(unsigned char* b, const AString& s)	
{
	s.copyTo(b);
	return b;
}
     
#endif

#endif /* */
