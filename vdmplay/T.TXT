#include "astring.h"
#include <string.h>
AString::AString(const void *data, unsigned len) : str(' ', len)
{
	if (len)
		memcpy((void *)str.data(), data, len);
}
unsigned AString::copyTo(void* buf) const
{
	if (length())
	{
		memcpy(buf, str.data(), length());
	}
	return length();
}
void AString::assignChunk(const void* buf, unsigned len)
{
	str.resize(len);
	if (len)
		memcpy((void*) str.data(), buf, len);
}
unsigned AString::compare(const void* buf, unsigned len) const
{
	return( len ? (str.length() == 0) : memcmp(buf, (const void *)str.data(), len));
}
