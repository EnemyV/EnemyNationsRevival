// Peer messages that name a player nobody has, on the SHIPPED code.
//
// run-null-plyr-lookup-test.py extracts VERBATIM: CGame::_GetPlayer / GetPlayer /
// _GetPlayerByPlyr / GetPlayerByPlyr (player.cpp) and the ProcessMessage receive cases listed
// in the runner (netapi.cpp) into npl_*.inc. Everything else here is scene: a fake CPlayer and
// CGame with the player lists the lookups walk, and recorders for what the handlers call.
//
// A message's player number comes from the datagram. FitsBuffer checks its size, not its
// contents, so any number can arrive; the lookups return NULL for one nobody has. Each handler
// is run on such a number inside SEH: an access violation is a FAIL. It is then run on real
// players to show the guard changes nothing for them.
//
// Exit: 0 all pass, 1 a check failed.

#include "../ai/microtest.h"

#include <excpt.h>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using BOOL      = int;
using BYTE      = unsigned char;
using WORD      = unsigned short;
using DWORD     = unsigned long;
using DWORD_PTR = unsigned long long;
constexpr BOOL TRUE = 1, FALSE = 0;

#define ASSERT( x )       ( (void)0 )
#define ASSERT_VALID( x ) ( (void)0 )
#define ASSERT_CMD( x )   ( (void)0 )
#define TRAP( ... )       ( (void)0 )
#define NOINLINE          __declspec( noinline )

enum { EVENT_NOTIFY = 1, EVENT_WARN = 2, EVENT_PLAYER_DEAD = 30, EVENT_CONST_CANT = 31 };
enum { IDS_EVENT_DEAD = 900 };

// ---------------------------------------------------------------- MFC list, the part used
struct __POSITION;
typedef __POSITION* POSITION;
template <class T>
class CList
{
  public:
    std::vector<T> m_v;
    POSITION       GetTailPosition( ) const { return m_v.empty( ) ? NULL : (POSITION)(size_t)m_v.size( ); }
    T              GetPrev( POSITION& pos ) const
    {
        size_t i = (size_t)pos;
        pos      = ( i > 1 ) ? (POSITION)( i - 1 ) : NULL;
        return m_v[i - 1];
    }
    int GetCount( ) const { return (int)m_v.size( ); }
};

// ---------------------------------------------------------------- scene: player + game
class CPlayer
{
  public:
    enum { ready, load_file, dead };
    int         m_iPlyrNum = 0;
    int         m_iNetNum  = 0;
    DWORD_PTR   m_dwAiHdl  = 0;
    BOOL        m_bMsgDead = FALSE;
    BOOL        m_bAI      = FALSE;
    int         m_iState   = ready;
    BOOL        m_bMe      = FALSE;
    std::string m_sName;
    class CVPTransfer* m_pXferToClient = NULL;

    NOINLINE int         GetPlyrNum( ) const { return m_iPlyrNum; }
    NOINLINE int         GetNetNum( ) const { return m_iNetNum; }
    NOINLINE DWORD_PTR   GetAiHdl( ) const { return m_dwAiHdl; }
    NOINLINE const char* GetName( ) const { return m_sName.c_str( ); }
    NOINLINE void        SetState( int i ) { m_iState = i; }
    NOINLINE BOOL        IsMe( ) const { return m_bMe; }
    NOINLINE void        SetAI( BYTE b ) { m_bAI = b; }
    NOINLINE void        SetNetNum( WORD i ) { m_iNetNum = i; }
    NOINLINE void        SetName( char const* p ) { m_sName = p; }
};

// vpxfer.h: the transfer StartFile opens to a load-join joiner.
class CVPTransfer
{
  public:
    explicit CVPTransfer( int ) {}
    int  m_iSends = 0;
    BOOL SendDataTo( WORD, WORD, void*, DWORD ) { m_iSends++; return TRUE; }
};
struct MockNet
{
    int  m_iHidden = 0;
    int  _GetSessionHandle( ) const { return 7; }
    void SetSessionVisibility( BOOL b ) { if ( !b ) m_iHidden++; }
};
static MockNet theNet;

class CWndArea
{
  public:
    int  m_iSetups = 0;
    void SetupStart( ) { m_iSetups++; }
};
struct MockAreaList
{
    CWndArea* m_pTop = NULL;
    CWndArea* GetTop( ) const { return m_pTop; }
};
static MockAreaList theAreaList;

struct CCreateBase
{
    enum { load_multi = 5, load_join = 6 };
    int m_iTyp;
};
struct MockApp
{
    CCreateBase* m_pCreateGame = NULL;
};
static MockApp theApp;

class CGame
{
  public:
    enum { create = 0, approve = 1, any = 2 };
    CPlayer*          m_pMe = NULL;
    CList<CPlayer*>   m_lstAll, m_lstDead, m_lstLoad, m_lstAi;
    int               m_iNetJoin    = create;
    int               m_iNumSends   = 0;
    char*             m_pGameFile   = NULL;
    int               m_iGameBufLen = 0;
    int               GetNetJoin( ) const { return m_iNetJoin; }
    CList<CPlayer*>&  GetAi( ) { return m_lstAi; }

    CPlayer* _GetPlayer( int iNetNum ) const;
    CPlayer* GetPlayer( int iNetNum ) const;
    CPlayer* _GetPlayerByPlyr( int iPlyrNum ) const;
    CPlayer* GetPlayerByPlyr( int iPlyrNum ) const;

    CPlayer* _GetMe( ) const { return m_pMe; }

    // recorders; each touches the player the way the real one does first
    std::vector<CPlayer*> m_removed;
    std::vector<int>      m_events;
    NOINLINE void RemovePlayer( CPlayer* pPlr )   // CGame::RemovePlayer: pPlr->SetState( dead ) first
    {
        pPlr->SetState( CPlayer::dead );
        m_removed.push_back( pPlr );
    }
    void Event( int iEvent, int, CPlayer* = NULL ) { m_events.push_back( iEvent ); }
    std::vector<CPlayer*> m_takenOver;
    NOINLINE void AiTakeOverPlayer( CPlayer* pPlr, BOOL, BOOL )   // logs pPlr->GetPlyrNum( ), then SetAI( TRUE )
    {
        pPlr->SetAI( TRUE );
        m_takenOver.push_back( pPlr );
    }
};
static CGame theGame;

struct AiCall
{
    DWORD_PTR m_dwID;
    const void* m_pMsg;
    int       m_iLen;
};
static std::vector<AiCall> g_aiCalls;
class CNetCmd;
static void AiMessage( DWORD_PTR dwID, CNetCmd const* pMsg, int iLen ) { g_aiCalls.push_back( { dwID, pMsg, iLen } ); }

static std::string EnLoadStdString( unsigned int ) { return "%s is dead"; }
static std::string strPrintf( const char* fmt, ... )
{
    char    s[256];
    va_list va;
    va_start( va, fmt );
    vsnprintf( s, sizeof( s ), fmt, va );
    va_end( va );
    return s;
}
static std::vector<std::string> g_popups;
class CDlgModelessMsg
{
  public:
    void Create( const char* p ) { g_popups.push_back( p ); delete this; }
};

// ---------------------------------------------------------------- messages: the fields read
class CNetCmd
{
  public:
    enum { cmd_to_hp = 9, plyr_dying = 40, ai_msg = 41, start_file = 42, err_place_bldg = 43, ai_gpf_takeover = 44 };
    BYTE m_bMsg = 0;
    explicit CNetCmd( int i ) : m_bMsg( (BYTE)i ) {}
    int GetType( ) const { return m_bMsg; }
};
class CMsgPlyrDying : public CNetCmd
{
  public:
    CMsgPlyrDying( ) : CNetCmd( plyr_dying ) {}
    int m_iPlyrNum = 0;
};
class CMsgAiMsg : public CNetCmd
{
  public:
    CMsgAiMsg( ) : CNetCmd( ai_msg ) {}
    int m_iAllocLen = 0, m_iPlyrNum = 0, m_iLen = 0;
};

class CNetToHp : public CNetCmd
{
  public:
    CNetToHp( ) : CNetCmd( cmd_to_hp ) {}
    int  m_iPlyrNum = 0, m_iNetNum = 0;
    char m_sName[16] = "Human";
};
class CNetAiGpf : public CNetCmd
{
  public:
    CNetAiGpf( ) : CNetCmd( ai_gpf_takeover ) {}
    int m_iPlyrNum = 0;
};
class CMsgStartFile : public CNetCmd
{
  public:
    CMsgStartFile( ) : CNetCmd( start_file ) {}
    WORD m_idFrom = 0, m_idTo = 0;
};
class CMsgPlaceBldg : public CNetCmd
{
  public:
    CMsgPlaceBldg( ) : CNetCmd( err_place_bldg ) {}
    int m_iPlyrNum = 0;
};

// ---------------------------------------------------------------- shipped code
#include "npl_lookup.inc"
#include "npl_funcs.inc"

static void Dispatch( CNetCmd* pCmd )
{
    switch ( pCmd->GetType( ) )
    {
#include "npl_cases.inc"
    default:
        break;
    }
}

// FALSE when the handler faulted.
static bool Survives( CNetCmd* pCmd )
{
    __try
    {
        Dispatch( pCmd );
        return true;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        return false;
    }
}

// ---------------------------------------------------------------- the players
static CPlayer s_me, s_ai, s_remote, s_dead;

static void Reset( )
{
    s_me     = CPlayer( );
    s_ai     = CPlayer( );
    s_remote = CPlayer( );
    s_dead   = CPlayer( );
    s_me.m_iPlyrNum = 1;     s_me.m_iNetNum = 101;    s_me.m_sName = "Me";
    s_ai.m_iPlyrNum = 2;     s_ai.m_iNetNum = 0;      s_ai.m_sName = "Robot";  s_ai.m_bAI = TRUE; s_ai.m_dwAiHdl = 0x2222;
    s_remote.m_iPlyrNum = 3; s_remote.m_iNetNum = 103; s_remote.m_sName = "Remote";
    s_dead.m_iPlyrNum = 4;   s_dead.m_iNetNum = 104;  s_dead.m_sName = "Gone";  s_dead.m_iState = CPlayer::dead;
    theGame.m_pMe = &s_me;
    theGame.m_lstAll.m_v  = { &s_me, &s_ai, &s_remote };
    theGame.m_lstDead.m_v = { &s_dead };
    theGame.m_lstLoad.m_v.clear( );
    theGame.m_lstAi.m_v = { &s_ai };
    theGame.m_iNetJoin  = CGame::create;
    theGame.m_iNumSends = 0;
    theGame.m_takenOver.clear( );
    s_me.m_bMe = TRUE;
    theNet.m_iHidden = 0;
    theAreaList.m_pTop = NULL;
    theGame.m_removed.clear( );
    theGame.m_events.clear( );
    g_aiCalls.clear( );
    g_popups.clear( );
}

static const int s_aiBadPlyr[] = { 5, 9, -1, 255, INT_MAX, INT_MIN };

static void TestLookupsReturnNullForUnknownNumbers( )
{
    Reset( );
    for ( int i : s_aiBadPlyr )
    {
        CHECK( theGame.GetPlayerByPlyr( i ) == NULL );
        CHECK( theGame.GetPlayer( i ) == NULL );
    }
    CHECK( theGame.GetPlayerByPlyr( 0 ) == &s_me );   // 0 is "us"
    CHECK( theGame.GetPlayerByPlyr( 3 ) == &s_remote );
    CHECK( theGame.GetPlayerByPlyr( 4 ) == &s_dead );  // the dead are still found
    CHECK( theGame.GetPlayer( 103 ) == &s_remote );
}

static void TestPlyrDying( )
{
    for ( int i : s_aiBadPlyr )
    {
        Reset( );
        CMsgPlyrDying msg;
        msg.m_iPlyrNum = i;
        const bool bOk = Survives( &msg );
        CHECK( bOk );
        if ( !bOk )
            std::printf( "  (plyr_dying for player %d faulted)\n", i );
        CHECK( theGame.m_removed.empty( ) && theGame.m_events.empty( ) && g_popups.empty( ) );
    }

    // A real player: removed, told once.
    Reset( );
    CMsgPlyrDying msg;
    msg.m_iPlyrNum = 3;
    CHECK( Survives( &msg ) );
    CHECK( theGame.m_removed.size( ) == 1 && theGame.m_removed[0] == &s_remote );
    CHECK( s_remote.m_bMsgDead == TRUE && s_remote.m_iState == CPlayer::dead );
    CHECK( theGame.m_events.size( ) == 1 && theGame.m_events[0] == EVENT_PLAYER_DEAD );
    CHECK( g_popups.size( ) == 1 && g_popups[0] == "Remote is dead" );
    CHECK( Survives( &msg ) );   // again: removed again, not told again
    CHECK_EQ( (int)theGame.m_removed.size( ), 2 );
    CHECK_EQ( (int)g_popups.size( ), 1 );
}

static void TestAiMsg( )
{
    struct
    {
        CMsgAiMsg hdr;
        char      inner[16];
    } wire;
    for ( int i : s_aiBadPlyr )
    {
        Reset( );
        wire.hdr.m_iPlyrNum = i;
        wire.hdr.m_iLen     = 16;
        const bool bOk      = Survives( &wire.hdr );
        CHECK( bOk );
        if ( !bOk )
            std::printf( "  (ai_msg for player %d faulted)\n", i );
        CHECK( g_aiCalls.empty( ) );
    }

    // A real AI: handed the inner record.
    Reset( );
    wire.hdr.m_iPlyrNum = 2;
    wire.hdr.m_iLen     = 16;
    CHECK( Survives( &wire.hdr ) );
    CHECK( g_aiCalls.size( ) == 1 && g_aiCalls[0].m_dwID == 0x2222 && g_aiCalls[0].m_pMsg == &wire.hdr + 1 &&
           g_aiCalls[0].m_iLen == 16 );
}

// cmd_to_hp and start_file name a NET number, so the unknown ones are net numbers here.
static const int s_aiBadNet[] = { 5, 9, -1, 255, 65535, INT_MAX };

static void TestToHp( )
{
    for ( int i : s_aiBadNet )
    {
        Reset( );
        CNetToHp msg;
        msg.m_iPlyrNum = i;
        msg.m_iNetNum  = 120;
        const bool bOk = Survives( &msg );
        CHECK( bOk );
        if ( !bOk )
            std::printf( "  (cmd_to_hp for net number %d faulted)\n", i );
    }

    // A real player: becomes the human on net 120.
    Reset( );
    s_remote.m_bAI = TRUE;
    CNetToHp msg;
    msg.m_iPlyrNum = 103;
    msg.m_iNetNum  = 120;
    CHECK( Survives( &msg ) );
    CHECK( s_remote.m_bAI == FALSE && s_remote.m_iNetNum == 120 && s_remote.m_sName == "Human" );
}

static void TestAiGpfTakeover( )
{
    for ( int i : s_aiBadPlyr )
    {
        Reset( );
        CNetAiGpf msg;
        msg.m_iPlyrNum = i;
        const bool bOk = Survives( &msg );
        CHECK( bOk );
        if ( !bOk )
            std::printf( "  (ai_gpf_takeover for player %d faulted)\n", i );
        CHECK( theGame.m_takenOver.empty( ) );
    }
    Reset( );
    CNetAiGpf msg;
    msg.m_iPlyrNum = 2;
    CHECK( Survives( &msg ) );
    CHECK( theGame.m_takenOver.size( ) == 1 && theGame.m_takenOver[0] == &s_ai );
}

static void TestStartFile( )
{
    char aFile[64] = { 0 };
    for ( int i : s_aiBadNet )
    {
        Reset( );
        theGame.m_pGameFile   = aFile;
        theGame.m_iGameBufLen = sizeof( aFile );
        CMsgStartFile msg;
        msg.m_idFrom = 101;
        msg.m_idTo   = (WORD)i;
        const bool bOk = Survives( &msg );
        CHECK( bOk );
        if ( !bOk )
            std::printf( "  (start_file to net number %d faulted)\n", (int)msg.m_idTo );
        CHECK_EQ( theGame.m_iNumSends, 0 );   // no send was started, so none is counted
    }
    Reset( );
    theGame.m_pGameFile   = aFile;
    theGame.m_iGameBufLen = sizeof( aFile );
    CMsgStartFile msg;
    msg.m_idFrom = 101;
    msg.m_idTo   = 103;
    CHECK( Survives( &msg ) );
    CHECK_EQ( theGame.m_iNumSends, 1 );
    CHECK( s_remote.m_iState == CPlayer::load_file && s_remote.m_pXferToClient != NULL &&
           s_remote.m_pXferToClient->m_iSends == 1 );
    delete s_remote.m_pXferToClient;
}

static void TestErrPlaceBldg( )
{
    CWndArea area;
    for ( int i : s_aiBadPlyr )
    {
        Reset( );
        theAreaList.m_pTop = &area;
        CMsgPlaceBldg msg;
        msg.m_iPlyrNum = i;
        const bool bOk = Survives( &msg );
        CHECK( bOk );
        if ( !bOk )
            std::printf( "  (err_place_bldg for number %d faulted)\n", i );
        CHECK( theGame.m_events.empty( ) );
    }
    // Whoever GetPlayer finds: told only if it is us. (The handler looks a PLAYER number
    // up with the NET-number lookup; that is reported, not changed, by this fix.)
    Reset( );
    area.m_iSetups     = 0;
    theAreaList.m_pTop = &area;
    CMsgPlaceBldg msg;
    msg.m_iPlyrNum = 101;   // s_me's net number
    CHECK( Survives( &msg ) );
    CHECK( theGame.m_events.size( ) == 1 && theGame.m_events[0] == EVENT_CONST_CANT && area.m_iSetups == 1 );
    Reset( );
    msg.m_iPlyrNum = 103;   // someone else
    CHECK( Survives( &msg ) );
    CHECK( theGame.m_events.empty( ) );
}

int main( )
{
    TestLookupsReturnNullForUnknownNumbers( );
    TestPlyrDying( );
    TestAiMsg( );
    TestToHp( );
    TestAiGpfTakeover( );
    TestStartFile( );
    TestErrPlaceBldg( );
    return microtest::Summary( );
}
