// test_staging_release.cpp -- the SHIPPED lifecycle-completion block of
// CAIGoalMgr::UpdateStagingTasks (caigmgr.cpp), extracted verbatim by
// run-staging-release-test.py into staging_release_actual.inc and compiled
// against the minimal fixture below.
//
// The block releases empty landing craft from a launched (INPROCESS, anchored)
// IDT_PREPAREWAR task, then resets the task when no unit holds it. Checks:
//   - only empty craft left (launched-wave leftover): craft released, task reset
//   - troops still waiting to embark: an empty craft sailing in keeps its task
//     (else AssignNavy re-admits it and StageUnit picks a new random hex)
//   - loaded craft / troops alone / pre-launch task: untouched
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>

using DWORD = uint32_t;
using WORD = uint16_t;
using BYTE = uint8_t;
using BOOL = int;
using POSITION = size_t;
constexpr BOOL TRUE = 1, FALSE = 0;
#define EN_AI_PROBES_WAR 0
constexpr int MAX_TASKPARAMS = 8;
constexpr int CAI_LOC_X = 0, CAI_LOC_Y = 1, CAI_PREV_X = 2, CAI_PREV_Y = 3, CAI_UNASSIGNED = 10;
constexpr int IDG_LANDWAR = 1018, IDG_ADVDEFENSE = 1022, IDG_SEAINVADE = 1033, IDT_PREPAREWAR = 2325;
constexpr BYTE UNASSIGNED_TASK = 0, INPROCESS_TASK = 1;

struct CTransportData { enum { landing_craft = 17, rangers = 19 }; };

struct AiVehSnap { int iCargoCount = 0; };
inline std::map<DWORD, AiVehSnap> theSnaps;
namespace AiSnap {
inline BOOL ReadVeh( DWORD id, AiVehSnap& out )
{
    auto p = theSnaps.find( id );
    if ( p == theSnaps.end( ) ) return FALSE;
    out = p->second;
    return TRUE;
}
}

struct CAIUnit {
    DWORD id = 0, data = 0; int owner = 1, typeUnit = 0, task = 0, goal = 0;
    WORD params[16]{};
    DWORD GetID( ) const { return id; }
    int GetOwner( ) const { return owner; }
    int GetTypeUnit( ) const { return typeUnit; }
    int GetTask( ) const { return task; } void SetTask( int t ) { task = t; }
    int GetGoal( ) const { return goal; } void SetGoal( int g ) { goal = g; }
    WORD GetParam( int i ) const { return params[i]; }
    void ClearParam( ) { std::memset( params, 0, sizeof( params ) ); }
    void SetDataDW( DWORD d ) { data = d; }
};
struct CAIUnitList {
    std::vector<CAIUnit*> items;
    POSITION GetHeadPosition( ) const { return items.empty( ) ? 0 : 1; }
    CAIUnit* GetNext( POSITION& p ) { auto q = items.at( p - 1 ); p = p < items.size( ) ? p + 1 : 0; return q; }
};
struct CAITask {
    int id = IDT_PREPAREWAR, goal = IDG_SEAINVADE; BYTE status = INPROCESS_TASK, priority = 5;
    WORD params[MAX_TASKPARAMS]{};
    int GetID( ) const { return id; } int GetGoalID( ) const { return goal; }
    BYTE GetStatus( ) const { return status; } void SetStatus( BYTE s ) { status = s; }
    void SetPriority( BYTE p ) { priority = p; }
    WORD GetTaskParam( int i ) const { return params[i]; } void SetTaskParam( int i, WORD v ) { params[i] = v; }
};
struct CAITaskList {
    CAITask* task = nullptr;
    CAITask* GetTask( int id, int goal ) { return task && task->id == id && task->goal == goal ? task : nullptr; }
};
struct MapUtil {
    int released = 0;
    void FlagStagingArea( BOOL bSet, int, int, int, int ) { if ( !bSet ) ++released; }
};
struct AIMap { MapUtil util; MapUtil* m_pMapUtil = &util; };

struct CAIGoalMgr {
    int m_iPlayer = 1;
    CAIUnitList* m_plUnits = nullptr;
    CAITaskList* m_plTasks = nullptr;
    AIMap* m_pMap = nullptr;
    void Lifecycle( );
};

void CAIGoalMgr::Lifecycle( )
{
#include "staging_release_actual.inc"
}

struct World {
    CAITask task; CAITaskList tasks; CAIUnitList units; AIMap map; CAIGoalMgr goal;
    std::vector<CAIUnit*> owned;
    World( BYTE status = INPROCESS_TASK )
    {
        theSnaps.clear( );
        task.status = status; task.params[CAI_LOC_X] = 40; task.params[CAI_LOC_Y] = 50;
        tasks.task = &task;
        goal.m_plUnits = &units; goal.m_plTasks = &tasks; goal.m_pMap = &map;
    }
    ~World( ) { for ( auto p : owned ) delete p; }
    CAIUnit* Add( DWORD id, int typeUnit, int cargo = -1 )
    {
        auto u = new CAIUnit; owned.push_back( u );
        u->id = id; u->typeUnit = typeUnit; u->task = IDT_PREPAREWAR; u->goal = IDG_SEAINVADE;
        u->params[CAI_UNASSIGNED] = INPROCESS_TASK;  // en route to its staging hex
        units.items.push_back( u );
        if ( cargo >= 0 ) theSnaps[id].iCargoCount = cargo;
        return u;
    }
    bool Holds( const CAIUnit* u ) const
    {
        return u->task == IDT_PREPAREWAR && u->goal == IDG_SEAINVADE && u->params[CAI_UNASSIGNED] == INPROCESS_TASK;
    }
};

int main( )
{
    int failed = 0, passed = 0;
    auto check = [&]( bool good, const char* name ) { std::cout << ( good ? "PASS " : "FAIL " ) << name << '\n'; good ? ++passed : ++failed; };
    {
        World w; auto lc = w.Add( 1, CTransportData::landing_craft, 0 );
        w.goal.Lifecycle( );
        check( lc->task == 0 && lc->goal == 0, "launched leftover: empty craft released" );
        check( w.task.status == UNASSIGNED_TASK && w.task.params[CAI_LOC_X] == 0 && w.map.util.released == 1,
               "launched leftover: spent task reset for next wave" );
    }
    {
        World w; auto ranger = w.Add( 1, CTransportData::rangers ); auto lc = w.Add( 2, CTransportData::landing_craft, 0 );
        w.goal.Lifecycle( );
        check( w.Holds( lc ), "troops waiting: empty craft en route keeps task and route" );
        check( w.Holds( ranger ) && w.task.status == INPROCESS_TASK && w.map.util.released == 0,
               "troops waiting: task not reset" );
    }
    {
        World w; auto lc = w.Add( 2, CTransportData::landing_craft, 0 ); auto ranger = w.Add( 1, CTransportData::rangers );
        w.goal.Lifecycle( );
        check( w.Holds( lc ) && w.Holds( ranger ), "troops waiting (craft listed first): empty craft keeps task" );
    }
    {
        World w; auto full = w.Add( 1, CTransportData::landing_craft, 3 ); auto lc = w.Add( 2, CTransportData::landing_craft, 0 );
        w.goal.Lifecycle( );
        check( w.Holds( full ) && w.task.status == INPROCESS_TASK, "loaded craft keeps task, task not reset" );
        check( lc->task == 0, "empty craft beside a loaded one (no troops ashore) released as before" );
    }
    {
        World w( UNASSIGNED_TASK ); auto lc = w.Add( 1, CTransportData::landing_craft, 0 );
        w.goal.Lifecycle( );
        check( w.Holds( lc ) && w.task.params[CAI_LOC_X] == 40, "pre-launch staging task untouched" );
    }
    std::cout << "RESULT passed=" << passed << " failed=" << failed << '\n';
    return failed ? 1 : 0;
}
