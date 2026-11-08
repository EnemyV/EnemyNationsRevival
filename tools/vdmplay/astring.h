#ifndef __ASTRING_H__
#define __ASTRING_H__

#include "rw\DEFS.H"

#include <string>
#include <cstdint>
#include <stdlib.h>

// Amount to shift hash values to avoid clustering
const size_t RW_HASH_SHIFT = 5;

inline static void mash(unsigned &hash, unsigned chars) {
    hash = (chars ^ ((hash << RW_HASH_SHIFT) | (hash >> (RWBITSPERBYTE * sizeof(uint32_t) - RW_HASH_SHIFT))));
}

class AString {
public:

    void cutLine() {
        int index = find("\n");

        if (index != -1)
            cutFrom(index);
    }

    void cutFrom(int index) {
        str = str.substr(index, str.length() - index);
    }

    int find(const char *pat) { return str.find(pat); }

    bool operator==(const AString &a) const {
        return str.length() ? str == a.str : a.str.length() == 0;
    }

    bool equal(const AString &a) const {
        return str.length() ? str == a.str : a.str.length() == 0;
    }

    AString &operator+=(const char *s) {
        str += s;
        return *this;
    }

// AString& operator +=(const RWCString& s) { str += s; return *this; }
    AString &operator+=(const AString &s) {
        str += s.str;
        return *this;
    }

    AString &append(const char *cs) {
        str.append(cs);
        return *this;
    }

    AString &append(const char *cs, size_t N) {
        str.append(cs, N);
        return *this;
    }

    AString &append(const void *cs, size_t N) {
        str.append((const char *) cs, N);
        return *this;
    }

    AString &append(const std::string &s) {
        str.append(s);
        return *this;
    }

    AString &append(const AString &s) {
        str.append(s.str);
        return *this;
    }


    int empty() const { return str.length() == 0; }

    /// A custom, case sensitive hash function. Taken from Rogue Wave's cstring implementation
    unsigned hash() const {
        // Taken
        unsigned int hv = (unsigned) length(); // Mix in the string length.
        unsigned i = length() * sizeof(char) / sizeof(unsigned);
        const unsigned *p = (const unsigned int *) str.data();
        {
            while (i--)
                mash(hv, *p++);   // XOR in the characters.
        }
        // XOR in any remaining characters:
        if ((i = length() * sizeof(char) % sizeof(unsigned)) != 0) {
            unsigned h = 0;
            const char *c = (const char *) p;
            while (i--)
                h = ((h << RWBITSPERBYTE * sizeof(char)) | *c++);
            mash(hv, h);
        }
        return hv;
    }

    unsigned length() const { return str.length(); }

    unsigned compare(const void *buf, unsigned len) const;

    unsigned copyTo(void *buf) const;

    void assignChunk(const void *buf, unsigned len);

    void clear() { str.resize(0); }


    AString(const void *buf, unsigned len);

    AString(const void *buf) : str((const char *) buf) {}

    AString() {}

    /// Constructs an AString of a given length, filling it with ' ' (space) characters.
    AString(unsigned int len) : str(' ', len) {}

    AString(const AString &s) : str(s.str) {}

    operator const char *() const { return str.data(); }

    void *data() { return (void *) str.data(); }

    void resize(size_t l) { str.resize(l); }

    unsigned char at(size_t i) const { return str[i]; }

    unsigned char operator[](size_t i) const {
        return (unsigned char)str[i];
    }

    size_t findChar(char ch, unsigned start = 0) const {
        return str.find(&ch, start);
    }


protected:
    std::string str;
};


//#ifdef __SC__
//inline char* operator=(char* b, const AString& s)
//{
// s.copyTo(b);
// return b;
//}
//
//inline unsigned char* operator=(unsigned char* b, const AString& s)
//{
// s.copyTo(b);
// return b;
//}
//
//#endif

#endif /* */
