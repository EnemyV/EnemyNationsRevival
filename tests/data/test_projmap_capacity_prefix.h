// Minimal map/list model used to compile the production CProjMap capacity query.
#include <assert.h>
#include <stddef.h>
#include <utility>
#include <vector>

typedef unsigned long DWORD;
typedef int POSITION;

class CProjBase {
public:
    enum TILE_TYPE { projectile, explosion };
    explicit CProjBase(TILE_TYPE type) : m_type(type), m_pNext(NULL) {}
    TILE_TYPE GetType() const { return m_type; }
    void Link(CProjBase* next) { m_pNext = next; }
private:
    friend class CProjMap;
    TILE_TYPE m_type;
    CProjBase* m_pNext;
};

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class CMap {
public:
    POSITION GetStartPosition() const { return m_entries.empty() ? 0 : 1; }
    void GetNextAssoc(POSITION& position, KEY& key, VALUE& value) const {
        assert(position > 0);
        const size_t index = static_cast<size_t>(position - 1);
        assert(index < m_entries.size());
        key = m_entries[index].first;
        value = m_entries[index].second;
        position = (index + 1 < m_entries.size()) ? static_cast<int>(index + 2) : 0;
    }
    void SetAt(KEY key, VALUE value) { m_entries.push_back(std::make_pair(key, value)); }
    int GetCount() const { return static_cast<int>(m_entries.size()); }
private:
    std::vector<std::pair<KEY, VALUE> > m_entries;
};

class CProjMap : public CMap<DWORD, DWORD, CProjBase*, CProjBase*> {
public:
    int GetProjectileHexCount() const;
};
