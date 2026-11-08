#ifndef __ASTRLIST_H__
#define __ASTRLIST_H__


#define _USE_GENERIC

#include "astring.h"

#if !defined(_USE_GENERIC)
#include <rw/tvslist.h>

typedef RWTValSlist<AString> AStrList;


#else

#include "rw/GSLIST.H"

class AStrList;

#if 0
class AStrLink
{
 AString str;

friend AStrList; 
};
#endif

class AStringRWGSlist : public RWSlist {
public:
    AStringRWGSlist() {}

    AStringRWGSlist(AString *a) : RWSlist(a) {}

    AString *append(AString *a) { return (AString *) RWSlist::append(a); }

    AString *&at(size_t i) { return (AString *&) RWSlist::at(i); }

    AString *at(size_t i) const { return (AString *) RWSlist::at(i); }

    RWBoolean contains(RWBoolean(*t)(const AString *, const void *), const void *a) const {
        return RWSlist::contains((RWtestGeneric) t, a);
    }

    RWBoolean containsReference(const AString *a) const { return RWSlist::containsReference(a); }

    void apply(void(*ap)(AString *, void *), void *x) { RWSlist::apply((RWapplyGeneric) ap, x); }

    AString *find(RWBoolean(*t)(const AString *, const void *), const void *a) const {
        return (AString *) RWSlist::find((RWtestGeneric) t, a);
    }

    AString *findReference(const AString *a) const { return (AString *) RWSlist::findReference(a); }

    AString *first() const { return (AString *) RWSlist::first(); }

    AString *get() { return (AString *) RWSlist::get(); }

    AString *insert(AString *a) { return (AString *) RWSlist::append(a); }

    AString *insertAfter(int i, AString *a) { return (AString *) RWSlist::insertAfter(i, a); }

    AString *insertAt(size_t i, AString *a) { return (AString *) RWSlist::insertAt(i, a); }

    AString *last() const { return (AString *) RWSlist::last(); }

    size_t occurrencesOf(RWBoolean(*t)(const AString *, const void *), const void *a) const {
        return RWSlist::occurrencesOf((RWtestGeneric) t, a);
    }

    size_t occurrencesOfReference(const AString *a) const { return RWSlist::occurrencesOfReference(a); }

    AString *prepend(AString *a) { return (AString *) RWSlist::prepend(a); }

    AString *remove(RWBoolean(*t)(const AString *, const void *), const void *a) {
        return (AString *) RWSlist::remove((RWtestGeneric) t, a);
    }

    AString *removeReference(const AString *a) { return (AString *) RWSlist::removeReference(a); }
};

class AStringRWGSlistIterator : public RWSlistIterator {
public:
    AStringRWGSlistIterator(AStringRWGSlist &s) : RWSlistIterator(s) {}

    AString *findNext(RWBoolean(*t)(const AString *, const void *),
                      const void *a) { return (AString *) RWSlistIterator::findNext((RWtestGeneric) t, a); }

    AString *findNextReference(const AString *a) { return (AString *) RWSlistIterator::findNextReference(a); }

    AString *insertAfterPoint(AString *a) { return (AString *) RWSlistIterator::insertAfterPoint(a); }

    AString *key() { return (AString *) RWSlistIterator::key(); }

    AString *operator()() { return (AString *) RWSlistIterator::operator()(); }

    AString *remove() { return (AString *) RWSlistIterator::remove(); }

    AString *removeNext(RWBoolean(*t)(const AString *, const void *),
                        const void *a) { return (AString *) RWSlistIterator::removeNext((RWtestGeneric) t, a); }

    AString *removeNextReference(const AString *a) { return (AString *) RWSlistIterator::removeNextReference(a); }
};

class AStrList : private AStringRWGSlist {


public:
    using AStringRWGSlist::entries;

    void clearAndDestroy() {
        AString *p;
        while (NULL != (p = get()))
            delete p;
    }

    AStrList &operator=(const AStrList &l) {
        clear();
        append(l);
        return *this;
    }

    AStrList() {}

    AStrList(const AStrList &l) {
        append(l);
    }

    void append(const AStrList &l) {
        AStringRWGSlistIterator i((AStrList &) l);
        const AString *p;

        while (0 != (p = i())) {
            append(*p);
        }
    }

    void append(const AString &s) {
        AString *l = new AString(s);
        AStringRWGSlist::append(l);
    }

    void clear() { clearAndDestroy(); }

    ~AStrList() { clear(); }

    const AString &at(unsigned i) const {
        const AString *l = AStringRWGSlist::at(i);
        return *l;
    }

    const AString &operator[](unsigned i) const {
        return at(i);
    }

};

#endif


#endif

