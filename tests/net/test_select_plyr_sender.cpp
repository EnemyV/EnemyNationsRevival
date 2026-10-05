// A load-join seat claim (cmd_select_plyr) on the SHIPPED host handler.
//
// run-select-plyr-sender-test.py extracts VERBATIM: VPMsgHdr (vdmplay.h), class CNetCmd and
// class CNetSelectPlyr (netcmd.h) and CmdSelectPlyr (netapi.cpp) into sps_*.inc. Everything
// else here is scene: the host's player lists and recorders for what the handler sends.
//
// The joiner fills m_iNetNum with its own GetMyNetNum( ) and vpSendData stamps the same id
// into msgFrom. A body that names another joiner is a claim made for someone else.
//
// Exit: 0 all pass, 1 a check failed.

#include "../ai/microtest.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using BOOL  = int;
using BYTE  = unsigned char;
using WORD  = unsigned short;
using DWORD = unsigned long;
constexpr BOOL TRUE = 1, FALSE = 0;
#define VP_TIMESTAMP 0
typedef WORD VPPLAYERID;

class CPlayer;
#pragma pack( push, 1 )
#include "sps_hdr.inc"
#include "sps_cmd.inc"
#include "sps_msg.inc"
#pragma pack( pop )

// ---------------------------------------------------------------- scene
struct __POSITION;
typedef __POSITION* POSITION;

class CPlayer
{
  public:
    enum { ready = 3, replace = 4 };
    int m_iNetNum = 0, m_iPlyrNum = 0, m_iState = replace, m_iPerInit = -1;
    int  GetState( ) const { return m_iState; }
    void SetState( int i ) { m_iState = i; }
};

template <class T>
class CList
{
  public:
    std::vector<T> m_v;
    POSITION Find( T t, POSITION ) const
    {
        for ( size_t i = 0; i < m_v.size( ); i++ )
            if ( m_v[i] == t )
                return (POSITION)( i + 1 );
        return NULL;
    }
};

class CNetGetFile
{
  public:
    CNetGetFile( CPlayer* pTo, CPlayer*, int iBufLen ) : m_pTo( pTo ), m_iBufLen( iBufLen ) {}
    CPlayer* m_pTo;
    int      m_iBufLen;
};

class CFile   // only reached when the game file is not read yet; the scene preloads it
{
  public:
    enum { modeRead = 1, shareExclusive = 2, typeBinary = 4 };
    CFile( const char*, int ) {}
    int  GetLength( ) const { return 0; }
    void Read( void*, int ) {}
    void Close( ) {}
};

struct Sent
{
    int m_iTo;   // net id (theNet.Send) or player number (PostToClient)
    int m_iType;
};

class CGame
{
  public:
    std::vector<CPlayer*> m_all;
    CList<CPlayer*>       m_lstLoad;
    char*                 m_pGameFile   = NULL;
    int                   m_iGameBufLen = 0;
    std::string           m_sFileName;
    CPlayer               m_server;

    CPlayer* _GetPlayer( int iNetNum ) const
    {
        for ( CPlayer* p : m_all )
            if ( p->m_iNetNum == iNetNum )
                return p;
        return NULL;
    }
    CPlayer* _GetPlayerByPlyr( int iPlyrNum ) const
    {
        for ( CPlayer* p : m_all )
            if ( p->m_iPlyrNum == iPlyrNum )
                return p;
        return NULL;
    }
    CPlayer* GetServer( ) { return &m_server; }

    std::vector<std::pair<CPlayer*, CPlayer*>> m_loaded;
    std::vector<Sent>                          m_toClient, m_toAll;
    int                                        m_iGetFile = 0;
    void LoadToPlyr( CPlayer* pLoad, CPlayer* pAll ) { m_loaded.push_back( { pLoad, pAll } ); }
    void PostToClient( CPlayer* pPlr, CNetCmd const* pMsg, int ) { m_toClient.push_back( { pPlr->m_iPlyrNum, pMsg->GetType( ) } ); }
    void PostToClient( CPlayer* pPlr, CNetGetFile const*, int ) { m_iGetFile++; (void)pPlr; }
    void PostToAllClients( CNetCmd const* pMsg, int, BOOL ) { m_toAll.push_back( { 0, pMsg->GetType( ) } ); }
};
static CGame theGame;

struct MockCreate
{
    int  m_iBtns = 0;
    void UpdateBtns( ) { m_iBtns++; }
};
struct MockApp
{
    MockCreate  m_create;
    MockCreate* m_pCreateGame = &m_create;
};
static MockApp theApp;

struct MockNet
{
    std::vector<Sent> m_sent;
    BOOL Send( VPPLAYERID idTo, CNetCmd const* pMsg, int ) { m_sent.push_back( { idTo, pMsg->GetType( ) } ); return TRUE; }
};
static MockNet theNet;

// ---------------------------------------------------------------- shipped code
#include "sps_handler.inc"

// ---------------------------------------------------------------- the seats
static CPlayer s_joinerA, s_joinerB, s_saved3, s_saved4;
static char    s_aFile[32];

static void Reset( )
{
    s_joinerA = CPlayer( );  s_joinerA.m_iNetNum = 201;  s_joinerA.m_iPlyrNum = 21;
    s_joinerB = CPlayer( );  s_joinerB.m_iNetNum = 202;  s_joinerB.m_iPlyrNum = 22;
    s_saved3  = CPlayer( );  s_saved3.m_iPlyrNum = 3;
    s_saved4  = CPlayer( );  s_saved4.m_iPlyrNum = 4;
    theGame.m_all          = { &s_joinerA, &s_joinerB, &s_saved3, &s_saved4 };
    theGame.m_lstLoad.m_v  = { &s_joinerA, &s_joinerB };
    theGame.m_pGameFile    = s_aFile;
    theGame.m_iGameBufLen  = sizeof( s_aFile );
    theGame.m_loaded.clear( );
    theGame.m_toClient.clear( );
    theGame.m_toAll.clear( );
    theGame.m_iGetFile = 0;
    theNet.m_sent.clear( );
}

// What the joiner sends (SDL2PickPlayerDialog::OnOK), as the host receives it: m_iNetNum from
// the joiner's GetMyNetNum( ), msgFrom stamped by vpSendData.
static CNetSelectPlyr Claim( int iBodyNet, int iPlyrNum, int iFrom )
{
    CNetSelectPlyr msg( iBodyNet, iPlyrNum, "Joiner" );
    msg.msgFrom = (VPPLAYERID)iFrom;
    return msg;
}

static void TestOwnClaimIsGranted( )
{
    Reset( );
    CNetSelectPlyr msg = Claim( 201, 3, 201 );
    CmdSelectPlyr( &msg );
    CHECK( theGame.m_loaded.size( ) == 1 && theGame.m_loaded[0].first == &s_joinerA &&
           theGame.m_loaded[0].second == &s_saved3 );
    CHECK( s_saved3.m_iState == CPlayer::ready && s_saved3.m_iPerInit == 0 );
    CHECK( theGame.m_toClient.size( ) == 1 && theGame.m_toClient[0].m_iType == CNetCmd::cmd_select_ok );
    CHECK( theGame.m_toAll.size( ) == 1 && theGame.m_toAll[0].m_iType == CNetCmd::cmd_plyr_taken );
    CHECK_EQ( theGame.m_iGetFile, 1 );
    CHECK( theNet.m_sent.empty( ) );
}

static void TestClaimForSomeoneElseIsRefused( )
{
    // Joiner A (net 201) asks for saved player 3 in joiner B's name (net 202).
    Reset( );
    CNetSelectPlyr msg = Claim( 202, 3, 201 );
    CmdSelectPlyr( &msg );
    const bool bClaimed = !theGame.m_loaded.empty( );
    CHECK( !bClaimed );
    if ( bClaimed )
        std::printf( "  (cmd_select_plyr: net 201 claimed player 3 for net %d)\n",
                     theGame.m_loaded[0].first->m_iNetNum );
    CHECK( s_saved3.m_iState != CPlayer::ready );
    CHECK( theGame.m_toClient.empty( ) && theGame.m_toAll.empty( ) && theGame.m_iGetFile == 0 );
    // refused to the joiner that asked, not to the one it named
    CHECK( theNet.m_sent.size( ) == 1 && theNet.m_sent[0].m_iTo == 201 &&
           theNet.m_sent[0].m_iType == CNetCmd::cmd_select_not_ok );

    // B can still claim it for itself afterwards.
    msg = Claim( 202, 3, 202 );
    CmdSelectPlyr( &msg );
    CHECK( theGame.m_loaded.size( ) == 1 && theGame.m_loaded[0].first == &s_joinerB );
}

static void TestTakenSeatIsRefusedToTheAsker( )
{
    Reset( );
    s_saved4.m_iState = CPlayer::ready;
    CNetSelectPlyr msg = Claim( 201, 4, 201 );
    CmdSelectPlyr( &msg );
    CHECK( theGame.m_loaded.empty( ) );
    CHECK( theNet.m_sent.size( ) == 1 && theNet.m_sent[0].m_iTo == 201 &&
           theNet.m_sent[0].m_iType == CNetCmd::cmd_select_not_ok );
}

int main( )
{
    TestOwnClaimIsGranted( );
    TestClaimForSomeoneElseIsRefused( );
    TestTakenSeatIsRefusedToTheAsker( );
    return microtest::Summary( );
}
