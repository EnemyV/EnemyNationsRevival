#ifndef __VPLIST_H__
#define __VPLIST_H__

#ifdef USE_VPMEM
#include "vputil.h"  // ensures vpAllocMem/vpFreeMem are declared
#endif

class CVPLink {
    friend class CVPList;

protected:
    CVPLink *next;
    CVPLink *prev;

public:
    CVPLink *Next() const { return next; }

    CVPLink *Prev() const { return prev; }

    void LinkAfter(CVPLink *l) {
        next = l->next;
        prev = l;
        if (next) {
            next->prev = this;
        }
        l->next = this;

    }

    void LinkBefore(CVPLink *l) {
        next = l;
        prev = l->prev;
        if (prev) {
            prev->next = this;
        }
        l->prev = this;
    }

    void Unlink() {
        if (prev) {
            prev->next = next;
        }
        if (next) {
            next->prev = prev;
        }
        next = prev = 0;
    }

    CVPLink() { next = prev = 0; }

#ifdef USE_VPMEM
    public:
     void* operator new(size_t s) { return vpAllocMem(s); }
     void operator delete(void* p, size_t s) { vpFreeMem(p, s); }
#endif
};

// Doubly linked list
class CVPList {
public:
    unsigned Count() const { return count; }

    CVPLink* First() const;

    CVPLink* Last() const;

    //+ Append the new element at the end of the list
    void Append(CVPLink* newOne);

    //+ Append the new element after an old element
    void AppendAfter(CVPLink* newOne, CVPLink* oldOne);

    //+ Insert a new element at the head of the list
    void Insert(CVPLink* newOne);

    //+ Insert e new element before an old element
    void InsertBefore(CVPLink* newOne, CVPLink* oldOne);

    //+ Remove an element form the list
    void Remove(CVPLink* elem);

    virtual //+ Remove the first element from the list
    CVPLink* RemoveFirst();


    //+ Construct the empty list
    CVPList();

protected:
    CVPLink* first;
    CVPLink* last;
    unsigned count;
};


inline CVPLink *CVPList::First() const {
    return (CVPLink *) first;
}

inline CVPLink *CVPList::Last() const {
    return (CVPLink *) last;
}


//+ Remove the first element from the list
inline CVPLink *CVPList::RemoveFirst() {
    CVPLink *e = First();

    if (e)
        Remove(e);
    return e;
}

// Insert the newu element at the end of the list
inline void CVPList::Append(CVPLink *newOne) {
    if (last) {
        newOne->LinkAfter(last);
        last = newOne;
    } else {
        first = last = newOne;
    }
    count++;
}

inline void CVPList::AppendAfter(CVPLink *newOne, CVPLink *oldOne) {
    newOne->LinkAfter(oldOne);
    if (last == oldOne) {
        last = newOne;
    }
    count++;
}

// Insert a new element at the head of the list
inline void CVPList::Insert(CVPLink *newOne) {

    if (first) {
        newOne->LinkBefore(first);
        first = newOne;
    } else {
        first = last = newOne;
    }
    count++;
}

inline void CVPList::InsertBefore(CVPLink *newOne, CVPLink *oldOne) {

    newOne->LinkBefore(oldOne);
    if (first == oldOne) {
        first = newOne;
    }
    count++;
}


inline void CVPList::Remove(CVPLink *elem) {
    if (first == elem) {
        first = elem->next;
    }
    if (last == elem) {
        last = elem->prev;
    }
    elem->Unlink();
    count--;
}


inline CVPList::CVPList() : first(0), last(0), count(0) {
}

#define VPList(type) name2(VPList,type)
#define VPListdeclare(type) \
class VPList(type) : public CVPList\
{\
public:\
 type* First() { return (type *) first; }\
 type* Last()  { return (type *) last; }\
 type* RemoveFirst() { return (type*) CVPList::RemoveFirst(); }\
}\


// Iteator for CVPLists
class CVPLIter {
protected:
    CVPLink *curElement;
    const CVPList &theList;

public:
    void Reset() { curElement = theList.First(); }

    CVPLIter(const CVPList &q) : theList(q) { curElement = q.First(); }

    CVPLink *Next() {
        register CVPLink *retValue = curElement;
        if (retValue) {
            curElement = retValue->Next();
        }
        return retValue;
    }

    CVPLink *operator()() { return Next(); }

    int AtEnd() const { return curElement == 0; }
};


#define _g_ListIterator_(etype, ltype, base) \
class VPListIterator(ltype, etype):public base \
{\
public:\
 VPListIterator(ltype,etype)(const ltype &q) : base(q) { }\
 etype *Next() { return (etype *)base::Next(); }\
 etype *operator()() { return Next(); }\
};

#define VPListIterator(ltype, etype) name3(VPIter, ltype, etype)
#define VPListIteratordeclare2(ltype, etype) _g_ListIterator_(etype, ltype, CVPLIter)


#endif

