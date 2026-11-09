// Minimal observable world used to compile the production CExplosion::Operate
// body in isolation. This is intentionally only the APIs the method calls.
#include "../../enations_latest/src/explframe.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned long DWORD;
typedef int BOOL;
static const BOOL TRUE = 1;
static const BOOL FALSE = 0;
#define ASSERT_VALID(p) ((void)(p))
#define TRAP() ((void)0)

static int g_frame = 0;
static int g_vehicleReleaseFrame = -1;
static int g_visibleChangeFrame = -1;
static int g_buildingVisibleFrame = -1;
static int g_enumFrame = -1;
static int g_removeFrame = -1;
static int g_deleteFrame = -1;
static int g_frameAdvanceCalls = 0;
static int g_lastFrameAdvanceCall = -1;
static bool g_sideEffectBeforeKill = false;

struct CHex {};
typedef int CHexCoord;
int fnEnumHex2(CHex*, CHexCoord, void*);

class CUnit;
class CExplosion;
class COwner { public: BOOL IsMe() const { return FALSE; } };
static COwner g_owner;

class CSpriteView { public: enum { ANIM_FRONT_1 = 0 }; };
class CSprite {};

class CAmbient {
public:
    explicit CAmbient(CExplosion* owner = NULL) : m_owner(owner) {}
    BOOL IsOneShotFinished(CSpriteView*) const;
private:
    CExplosion* m_owner;
};

class CUnit {
public:
    enum { dying = 1, vehicle = 1, building = 2 };
    explicit CUnit(int type) : m_type(type), m_visible(1), m_flags(dying), m_target(NULL), m_owner(&g_owner) {}
    virtual ~CUnit() {}
    BOOL IsFlag(int flag) const { return (m_flags & flag) != 0; }
    int GetVisible() const { return m_visible; }
    int GetUnitType() const { return m_type; }
    void IncVisible(int delta) {
        m_visible += delta;
        if (g_visibleChangeFrame < 0) g_visibleChangeFrame = g_frame;
        if (g_frame < enexpl::EXPL_KILLFRAME) g_sideEffectBeforeKill = true;
    }
    void SetVisible(int value) { m_visible = value; }
    int GetID() const { return 1; }
    COwner* GetOwner() const { return m_owner; }
    CUnit* GetTarget() const { return m_target; }
    void SetTarget(CUnit* target) { m_target = target; }
    void SetOwner(COwner* owner) { m_owner = owner; }
private:
    int m_type, m_visible, m_flags;
    CUnit* m_target;
    COwner* m_owner;
};

class CVehicle : public CUnit {
public:
    CVehicle() : CUnit(vehicle) {}
    void ReleaseOwnership() {
        if (g_vehicleReleaseFrame < 0) g_vehicleReleaseFrame = g_frame;
        if (g_frame < enexpl::EXPL_KILLFRAME) g_sideEffectBeforeKill = true;
    }
};

class CBuilding : public CUnit {
public:
    CBuilding() : CUnit(building) {}
    void MakeBldgVisible() {
        SetVisible(1);
        if (g_buildingVisibleFrame < 0) g_buildingVisibleFrame = g_frame;
        if (g_frame < enexpl::EXPL_KILLFRAME) g_sideEffectBeforeKill = true;
    }
    CHexCoord GetHex() const { return 0; }
    int GetCX() const { return 1; }
    int GetCY() const { return 1; }
};

class CGameMap {
public:
    template<class Fn> void EnumHexes(CHexCoord, int, int, Fn, void*) {
        if (g_enumFrame < 0) g_enumFrame = g_frame;
        if (g_frame < enexpl::EXPL_KILLFRAME) g_sideEffectBeforeKill = true;
    }
};
class CGame {
public:
    void Event(int, int, CUnit*) {}
};
class CProjMap {
public:
    void Remove(CExplosion*) { g_removeFrame = g_frame; }
};
static CGameMap theMap;
static CGame theGame;
static CProjMap theProjMap;
static CUnit* g_target = NULL;
static CUnit* g_shooter = NULL;
CUnit* _GetUnit(DWORD) { return g_target; }
CUnit* GetUnit(DWORD) { return g_shooter; }
enum { EVENT_ATK_DESTROYED = 1, EVENT_NOTIFY = 2 };

class CExplosion {
public:
    CExplosion(int animationFrames, CUnit* target, int* deleted)
        : m_iFrames(0), m_iKillFrame(enexpl::EXPL_KILLFRAME), m_dwIDTarget(1), m_dwIDShooter(0),
          m_animationFrames(animationFrames), m_animationFrame(0), m_target(target), m_deleted(deleted), m_ambient(this) {}
    ~CExplosion() { *m_deleted = 1; g_deleteFrame = g_frame; }
    CSprite* GetSprite() { return m_animationFrames > 0 ? &m_sprite : NULL; }
    CSpriteView* GetView() { return m_animationFrames > 0 ? &m_view : NULL; }
    CAmbient* GetAmbient(int) { return &m_ambient; }
    int GetFrame(int layer) {
        assert(layer == CSpriteView::ANIM_FRONT_1);
        ++m_animationFrame;
        ++g_frameAdvanceCalls;
        g_lastFrameAdvanceCall = g_frame;
        return m_animationFrame;
    }
    BOOL AnimationFinished() const { return m_animationFrame >= m_animationFrames; }
    void Operate();

    int m_iFrames;
    int m_iKillFrame;
    DWORD m_dwIDTarget, m_dwIDShooter;
private:
    int m_animationFrames;
    int m_animationFrame;
    CUnit* m_target;
    int* m_deleted;
    CSprite m_sprite;
    CSpriteView m_view;
    CAmbient m_ambient;
};

BOOL CAmbient::IsOneShotFinished(CSpriteView*) const
{
    return m_owner->AnimationFinished();
}

int fnEnumHex2(CHex*, CHexCoord, void*) { g_enumFrame = g_frame; return 0; }
