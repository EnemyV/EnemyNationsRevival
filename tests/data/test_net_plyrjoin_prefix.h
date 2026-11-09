// Minimal world used to compile the SHIPPED receive-side size checks from
// enations_latest/src: CNetCmd::FitsBuffer and the senders of its variable-length
// records (netcmd.cpp, ipcmsg.cpp), the record classes themselves (netcmd.h,
// ipcmsg.hpp) and the CMaterialTypes enum (base.h). run-data-tests.ps1 extracts
// all of those verbatim on every run; this file only supplies what they
// reference. Every other type FitsBuffer names gets a stand-in sized from
// wire_layout_assert.cpp's Release pins, and the suffix pins the extracted
// classes to those numbers too, so a mock that drifts is caught.
#include <limits.h>
#include <new>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "../ai/microtest.h"

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef int            BOOL;
typedef DWORD          COLORREF;
#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#define ASSERT_STRICT( x ) ( (void)0 )
#define ASSERT_VALID( x )  ( (void)0 )
#define ASSERT_CMD( x )    ( (void)0 )

// vdmplay.h (VP_TIMESTAMP 0): the 10-byte header every CNetCmd starts with.
typedef WORD VPPLAYERID;
typedef struct VPMsgHdr
{
    WORD       msgSize;
    BYTE       msgKind;
    BYTE       msgFlags;
    VPPLAYERID msgFrom;
    VPPLAYERID msgTo;
    WORD       msgId;
} VPMSGHDR;
const DWORD VP_MAXSENDDATA = 700;

class CNetUnpublish;
class CNetJoinGame;
class CNetJoinName;
class CNetPlayer;

//---------------------------------------------------------------------------
//  What CNetPlyrJoin::Alloc( const CPlayer* ) reads off the player. Values are
//  arbitrary but distinct so the end-to-end copy check can see each field.
//---------------------------------------------------------------------------

struct CRsrchStatus
{
    BOOL m_bDiscovered;
};

struct CRsrchItem
{
    int m_iPtsRequired;
};

class CRsrchArray
{
  public:
    enum { num_types = 8 };
    CRsrchItem m_a[num_types];
    CRsrchItem& operator[]( int i ) { return m_a[i]; }
};
static CRsrchArray theRsrch = { { { 0 }, { 10 }, { 10 }, { 10 }, { 10 }, { 10 }, { 10 }, { 10 } } };

class CCreateBase;
struct MockApp
{
    CCreateBase* m_pCreateGame;
};
static MockApp theApp = { NULL };

// racedata.h's CInitData: only its size matters here (CNetPlayer embeds it).
class CInitData
{
  public:
    char m_ab[124];
};

// mfc_compat.h's CString is a std::string wrapper; CMsgIPC embeds two.
class CString
{
  public:
    CString( ) {}
    CString& operator=( const char* psz ) { m_str = psz; return *this; }
    operator const char*( ) const { return m_str.c_str( ); }
    int  GetLength( ) const { return (int)m_str.size( ); }
    void Empty( ) { m_str.clear( ); }
    std::string m_str;
};

class CPlayer
{
  public:
    enum { ready = 3 };
    std::string  m_sName;
    CRsrchStatus m_aRsrch[CRsrchArray::num_types];
    CInitData    m_InitData;

    char const*   GetName( ) const { return m_sName.c_str( ); }
    int           GetNetNum( ) const { return 9; }
    int           GetPlyrNum( ) const { return 4; }
    int           GetPwrHave( ) const { return 100; }
    int           GetPwrNeed( ) const { return 90; }
    int           GetPplBldg( ) const { return 30; }
    int           GetPplNeedBldg( ) const { return 25; }
    int           GetMaterialHave( int i ) const { return 1000 + i; }
    int           GetFood( ) const { return 555; }
    int           GetGasHave( ) const { return 777; }
    CRsrchStatus& GetRsrch( int i ) { return m_aRsrch[i]; }
    int           GetBldgsHave( ) const { return 12; }
    int           GetVehsHave( ) const { return 7; }
    int           GetState( ) const { return 0; }
    BOOL          IsAI( ) const { return TRUE; }
};

// event.h values the mail receiver raises; only "was it raised" matters here.
enum { EVENT_NOTIFY = 1, EVENT_HAVE_MAIL = 55 };

struct MockGame
{
    int GetMyNetNum( ) const { return 9; }

    // SDL2Mail_HandleIncoming: who we are (GetPlyrNum( ) == 4) and who sent it.
    CPlayer* m_pMe      = NULL;
    CPlayer* m_pSender  = NULL;
    int      m_iSender  = 2;
    int      m_iMailEvt = 0;
    CPlayer* GetMe( ) const { return m_pMe; }
    CPlayer* GetPlayerByPlyr( int iPlyrNum ) const { return iPlyrNum == m_iSender ? m_pSender : NULL; }
    void     Event( int iEvent, int ) { if ( iEvent == EVENT_HAVE_MAIL ) m_iMailEvt++; }
};
static MockGame theGame;

static std::vector<std::string> g_chatLog;
static void SDL2Chat_AddMessage( const std::string& line ) { g_chatLog.push_back( line ); }
