// Minimal collection model used to compile the production GetSprite method.
#include <assert.h>
#include <stddef.h>

typedef int BOOL;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define ASSERT_VALID(object) ((void)0)
#define ASSERT(expression) ((void)0)

template<class T>
class Ptr {
public:
    Ptr() : m_value(NULL) {}
    explicit Ptr(T* value) : m_value(value) {}
    T* Value() const { return m_value; }
    T* operator->() const { return m_value; }
    void Set(T* value) { m_value = value; }
private:
    T* m_value;
};

class CSpriteHdr {
public:
    explicit CSpriteHdr(int views) : m_nViews(views) {}
    int m_nViews;
};

class CSprite {
public:
    explicit CSprite(int views)
        : m_header(views < 0 ? 0 : views),
          m_ptrspritehdr(views < 0 ? NULL : &m_header) {}
#include "test_sprite_getnumviews_generated.h"
private:
    CSpriteHdr m_header;
    Ptr<CSpriteHdr> m_ptrspritehdr;
};

class CSpriteCollectionInfo {
public:
    explicit CSpriteCollectionInfo(int index)
        : m_index(index), m_lastID(0), m_lastOffset(0), m_lastStrict(FALSE) {}
    int GetIndex(int id, int offset, BOOL strict) const {
        m_lastID = id;
        m_lastOffset = offset;
        m_lastStrict = strict;
        return m_index;
    }
    void SetIndex(int index) { m_index = index; }
    int LastID() const { return m_lastID; }
    int LastOffset() const { return m_lastOffset; }
    BOOL LastStrict() const { return m_lastStrict; }
private:
    int m_index;
    mutable int m_lastID;
    mutable int m_lastOffset;
    mutable BOOL m_lastStrict;
};

class CSpriteCollection {
public:
    CSpriteCollection(Ptr<CSprite>* sprites, int count, CSpriteCollectionInfo* info)
        : m_bOpen(1), m_nSprite(count), m_pptrsprite(sprites),
          m_ptrspritecollectioninfo(info) {}
    CSprite* GetSprite(int iID, int iIndex = 0, BOOL bStrict = FALSE);
private:
    BOOL m_bOpen;
    int m_nSprite;
    Ptr<CSprite>* m_pptrsprite;
    Ptr<CSpriteCollectionInfo> m_ptrspritecollectioninfo;
};
