// Tests for the shipped load-join wire code. See test_net_plyrjoin_prefix.h and
// run-data-tests.ps1 for how the production text gets here.

// The extracted class definitions must still be the wire layouts the game pins
// in wire_layout_assert.cpp (both configs); a mock or extraction drift fails
// the compile here instead of testing the wrong offsets.
static_assert( sizeof( CNetCmd ) == 12, "CNetCmd" );
static_assert( sizeof( CNetPlyrJoin ) == 93, "CNetPlyrJoin" );
static_assert( sizeof( CNetEnumPlyrs ) == 16, "CNetEnumPlyrs" );
static_assert( sizeof( CNetSelectPlyr ) == 100, "CNetSelectPlyr" );
static_assert( sizeof( CNetPlayer ) == 152, "CNetPlayer" );
static_assert( sizeof( CNetChat ) == 21, "CNetChat" );
static_assert( sizeof( CNetToHp ) == 21, "CNetToHp" );
static_assert( sizeof( CNetGetFile ) == 28, "CNetGetFile" );
static_assert( sizeof( CMsgAiMsg ) == 24, "CMsgAiMsg" );

//---------------------------------------------------------------------------
//  operator new[] with a canary tail: every array allocation gets kCanary
//  bytes of 0xAB after the requested size, so a write past the end of what
//  Alloc asked for is visible instead of silently corrupting the heap.
//---------------------------------------------------------------------------

static const size_t kCanary   = 2048;
static char*        g_pLast   = NULL;
static size_t       g_cbLast  = 0;

void* operator new[]( size_t cb )
{
    if ( cb > ( 1u << 20 ) )
        throw std::bad_alloc( );   // a negative length converted to size_t lands here
    char* p = (char*)malloc( cb + kCanary );
    if ( p == NULL )
        throw std::bad_alloc( );
    memset( p + cb, 0xAB, kCanary );
    g_pLast  = p;
    g_cbLast = cb;
    return p;
}
void operator delete[]( void* p ) noexcept { free( p ); }
void operator delete[]( void* p, size_t ) noexcept { free( p ); }

static bool LastCanaryIntact( )
{
    for ( size_t i = 0; i < kCanary; i++ )
        if ( (unsigned char)g_pLast[g_cbLast + i] != 0xAB )
            return false;
    return true;
}

static const int kNameOff = (int)( sizeof( CNetPlyrJoin ) - 1 );   // pack(1): m_sName is the last byte

// Exactly what CmdEnumPlyrs sends: the shipped Alloc( CPlayer* ), m_iLen bytes.
static CNetPlyrJoin* SenderRecord( const char* pName )
{
    CPlayer plr;
    plr.m_sName = pName;
    for ( int i = 0; i < CRsrchArray::num_types; i++ )
        plr.m_aRsrch[i].m_bDiscovered = ( i % 2 ) == 0;
    return CNetPlyrJoin::Alloc( &plr );
}

//---------------------------------------------------------------------------

static void TestLegitPlyrJoinRoundTrips( )
{
    std::string sMax( (size_t)VP_MAXSENDDATA - sizeof( CNetPlyrJoin ) - 2, 'm' );   // largest record vdmplay carries
    std::string s64( 64, 'n' );
    const char* apNames[] = { "", "A", "Joiner", s64.c_str( ), sMax.c_str( ) };

    for ( const char* pName : apNames )
    {
        CNetPlyrJoin* pSend = SenderRecord( pName );
        const int     cb    = pSend->m_iLen;
        CHECK_EQ( cb, (int)( sizeof( CNetPlyrJoin ) + strlen( pName ) + 2 ) );
        CHECK( cb <= (int)VP_MAXSENDDATA );

        // The datagram arrives as exactly m_iLen bytes (vdmplay msgSize) and
        // AddToQueue copies those bytes into a pool buffer.
        char aQueued[VP_MAXSENDDATA];
        memset( aQueued, 0xCD, sizeof( aQueued ) );
        memcpy( aQueued, pSend, cb );
        const CNetPlyrJoin* pRx = (const CNetPlyrJoin*)aQueued;
        CHECK( pRx->FitsBuffer( cb ) );

        // ...and CmdPlyrJoin keeps a copy made by the shipped Alloc( pData ).
        CNetPlyrJoin* pCopy = NULL;
        try { pCopy = CNetPlyrJoin::Alloc( pRx ); } catch ( ... ) { pCopy = NULL; }
        CHECK( pCopy != NULL );
        if ( pCopy != NULL )
        {
            CHECK( LastCanaryIntact( ) );
            CHECK( strcmp( pCopy->m_sName, pName ) == 0 );
            CHECK_EQ( pCopy->m_iPlyrNum, pSend->m_iPlyrNum );
            CHECK_EQ( pCopy->m_iRsrchLevel, pSend->m_iRsrchLevel );
            CHECK_EQ( pCopy->m_iMat[CMaterialTypes::gas], 777 );
            CHECK_EQ( pCopy->m_iNumVeh, 7 );
            CHECK_EQ( pCopy->m_bAvail, pSend->m_bAvail );
            delete[] (char*)pCopy;
        }
        delete[] (char*)pSend;
    }
}

// A record whose header came from a real sender, in a buffer with room to lie.
struct Wire
{
    union { char a[1024]; double align; };
    CNetPlyrJoin* p( ) { return (CNetPlyrJoin*)a; }
};

static void MakeLegit( Wire& w, const char* pName )
{
    memset( w.a, 0, sizeof( w.a ) );
    CNetPlyrJoin* pSend = SenderRecord( pName );
    memcpy( w.a, pSend, pSend->m_iLen );
    delete[] (char*)pSend;
}

static void TestHostilePlyrJoinIsRefused( )
{
    Wire w;
    MakeLegit( w, "Joiner" );
    const int cbLegit = w.p( )->m_iLen;
    CHECK( w.p( )->FitsBuffer( cbLegit ) );

    // Truncated datagrams: just the CNetCmd header, and one byte short of the
    // fixed fields.
    CHECK( !w.p( )->FitsBuffer( (int)sizeof( CNetCmd ) ) );
    CHECK( !w.p( )->FitsBuffer( (int)sizeof( CNetPlyrJoin ) - 1 ) );
    CHECK( !w.p( )->FitsBuffer( cbLegit - 4 ) );   // m_iLen claims more than arrived

    // m_iLen that lies: larger than the datagram, shorter than the header,
    // negative.
    const int aiBadLen[] = { 600, cbLegit + 1, INT_MAX, (int)sizeof( CNetPlyrJoin ) - 1, 12, 0, -1, INT_MIN };
    for ( int iLen : aiBadLen )
    {
        MakeLegit( w, "Joiner" );
        w.p( )->m_iLen = iLen;
        CHECK( !w.p( )->FitsBuffer( cbLegit ) );
    }

    // Name not NUL-terminated inside m_iLen.
    MakeLegit( w, "Joiner" );
    memset( w.a + kNameOff, 'A', cbLegit - kNameOff );
    CHECK( !w.p( )->FitsBuffer( cbLegit ) );
    CHECK( !w.p( )->FitsBuffer( (int)sizeof( w.a ) ) );   // bytes past m_iLen do not count

    // The smallest well-formed record (empty name, no slack) is fine.
    MakeLegit( w, "" );
    w.p( )->m_iLen = (int)sizeof( CNetPlyrJoin );
    CHECK( w.p( )->FitsBuffer( (int)sizeof( CNetPlyrJoin ) ) );
}

// Alloc( pData ) on its own, with no FitsBuffer in front: it must never write
// past what it allocated, and must not throw.
static void TestAllocCopyIsBounded( )
{
    const int aiLen[] = { 12, 0, -1, INT_MIN, (int)sizeof( CNetPlyrJoin ) - 1, 120, INT_MAX };
    for ( int iLen : aiLen )
    {
        Wire w;
        MakeLegit( w, "Joiner" );
        memset( w.a + kNameOff, 'A', sizeof( w.a ) - kNameOff - 1 );   // name runs to the end of the buffer
        w.a[sizeof( w.a ) - 1] = 0;
        w.p( )->m_iLen = iLen;

        CNetPlyrJoin* pCopy = NULL;
        bool          bThrew = false;
        try { pCopy = CNetPlyrJoin::Alloc( w.p( ) ); } catch ( ... ) { bThrew = true; }
        CHECK( !bThrew );
        if ( pCopy == NULL )
        {
            std::printf( "  (m_iLen %d: Alloc threw)\n", iLen );
            continue;
        }
        const bool bCanary = LastCanaryIntact( );
        CHECK( bCanary );
        CHECK( g_cbLast >= sizeof( CNetPlyrJoin ) );
        CHECK( g_cbLast <= VP_MAXSENDDATA );
        CHECK( memchr( pCopy->m_sName, 0, g_cbLast - kNameOff ) != NULL );
        CHECK_EQ( pCopy->m_iPlyrNum, 4 );
        if ( !bCanary )
            std::printf( "  (m_iLen %d: Alloc wrote past its %d-byte allocation)\n", iLen, (int)g_cbLast );
        delete[] (char*)pCopy;
    }
}

// cmd_enum_plyrs and the CNetSelectPlyr family are fixed size; every sender
// posts sizeof( the struct ).
static void TestSelectFamilyNeedsTheWholeRecord( )
{
    CNetEnumPlyrs enumMsg( 5 );   // OnMsgJoin: PostToServer( &msg, sizeof( msg ) )
    CHECK( enumMsg.FitsBuffer( (int)sizeof( enumMsg ) ) );
    CHECK( !enumMsg.FitsBuffer( (int)sizeof( CNetCmd ) ) );
    CHECK( !enumMsg.FitsBuffer( (int)sizeof( enumMsg ) - 1 ) );

    // SDL2PickPlayerDialog asks with this; CmdSelectPlyr turns the same record
    // into ok / taken / not_ok and sends sizeof( CNetSelectPlyr ) each time.
    CNetSelectPlyr msg( 3, 4, "Joiner" );
    for ( int iStep = 0; iStep < 4; iStep++ )
    {
        if ( iStep == 1 ) msg.ToOk( );
        if ( iStep == 2 ) msg.ToTaken( );
        if ( iStep == 3 ) msg.ToNotOk( );
        CHECK( msg.FitsBuffer( (int)sizeof( CNetSelectPlyr ) ) );
        CHECK( !msg.FitsBuffer( (int)sizeof( CNetCmd ) ) );
        CHECK( !msg.FitsBuffer( (int)sizeof( CNetCmd ) + 8 ) );   // m_iNetNum + m_iPlyrNum, no name
        CHECK( !msg.FitsBuffer( (int)sizeof( CNetSelectPlyr ) - 1 ) );
    }
    CHECK_EQ( msg.GetType( ), CNetCmd::cmd_select_not_ok );
}

//---------------------------------------------------------------------------
//  The sweep: every type a receive handler (ProcessMessage, or CAIMsg for the
//  record an ai_msg wraps) casts and reads past the 12-byte header. The class
//  is the one the handler casts to; its size is wire_layout_assert.cpp's
//  Release pin (the stand-ins are generated from those pins), which is also
//  what every sender posts.
//---------------------------------------------------------------------------

struct FixedCase
{
    int         iType;
    const char* pName;
    int         cbStruct;
};
#define FIXED( type, cls ) { CNetCmd::type, #type, (int)sizeof( cls ) }

static const FixedCase s_aFixed[] = {
    // listed before this sweep (regression guard)
    FIXED( cmd_ready, CNetReady ),           FIXED( cmd_you_are, CNetYouAre ),
    FIXED( cmd_start, CNetStart ),           FIXED( place_veh, CMsgPlaceVeh ),
    FIXED( veh_new, CMsgVehNew ),            FIXED( veh_goto, CMsgVehGoto ),
    FIXED( unit_attacked, CMsgUnitAttacked ), FIXED( edict_toggle, CNetEdictToggle ),
    FIXED( save_info, CNetSaveInfo ),        FIXED( cmd_enum_plyrs, CNetEnumPlyrs ),
    FIXED( cmd_select_plyr, CNetSelectPlyr ),
    // ProcessMessage handlers that were falling to the 12-byte default
    FIXED( cmd_plyr_status, CNetPlyrStatus ), FIXED( cmd_init_done, CNetInitDone ),
    FIXED( cmd_play, CNetPlay ),             FIXED( cmd_to_ai, CNetToAi ),
    FIXED( build_bridge, CMsgBuildBridge ),  FIXED( err_build_bridge, CMsgBuildBridge ),
    FIXED( bridge_new, CMsgBridgeNew ),      FIXED( unit_repair, CMsgUnitRepair ),
    FIXED( unit_set_repair, CMsgUnitRepair ), FIXED( deploy_it, CMsgDeployIt ),
    FIXED( plyr_dying, CMsgPlyrDying ),      FIXED( set_rsrch, CMsgRsrch ),
    FIXED( repair_veh, CMsgRepairVeh ),      FIXED( repair_bldg, CMsgRepairBldg ),
    FIXED( load_carrier, CMsgLoadCarrier ),  FIXED( unload_carrier, CMsgUnloadCarrier ),
    FIXED( unit_control, CMsgUnitControl ),  FIXED( set_relations, CMsgSetRelations ),
    FIXED( game_speed, CMsgGameSpeed ),      FIXED( bldg_materials, CMsgBldgMat ),
    FIXED( give_unit, CMsgGiveUnit ),        FIXED( set_time, CMsgSetTime ),
    FIXED( start_file, CMsgStartFile ),      FIXED( cancel_load, CMsgCancelLoad ),
    FIXED( pause_messages, CMsgPauseMsg ),   FIXED( ai_gpf_takeover, CNetAiGpf ),
    FIXED( research_disc, CNetRsrchDisc ),   FIXED( cmd_get_file, CNetGetFile ),
    // read only by the AI (CAIMsg), reached over the wire inside an ai_msg
    FIXED( bridge_done, CMsgBridgeDone ),    FIXED( unit_loaded, CMsgLoaded ),
    FIXED( unit_repaired, CMsgRepaired ),    FIXED( build_civ, CMsgBuildCiv ),
    FIXED( scenario, CMsgScenario ),         FIXED( scenario_atk, CMsgScenarioAtk ),
    FIXED( out_of_LOS, CMsgOutOfLos ),
};

static int PinOf( const char* pClass )
{
    for ( const WirePin& pin : s_aWirePin )
        if ( strcmp( pin.m_pName, pClass ) == 0 )
            return pin.m_iSize;
    return -1;
}

static void TestEveryFixedTypeNeedsItsStruct( )
{
    for ( const FixedCase& c : s_aFixed )
    {
        Wire w;
        memset( w.a, 0, sizeof( w.a ) );
        w.p( )->m_bMsg = (BYTE)c.iType;
        if ( c.iType == CNetCmd::cmd_get_file )
            ( (CNetGetFile*)w.a )->m_iBufLen = 8 * 1024 * 1024;

        const bool bWhole = w.p( )->FitsBuffer( c.cbStruct ) != FALSE;
        const bool bShort = w.p( )->FitsBuffer( c.cbStruct - 1 ) != FALSE;
        const bool bHdr   = w.p( )->FitsBuffer( (int)sizeof( CNetCmd ) ) != FALSE;
        CHECK( c.cbStruct > (int)sizeof( CNetCmd ) );
        CHECK( bWhole );
        CHECK( !bShort );
        CHECK( !bHdr );
        if ( !bWhole || bShort || bHdr )
            std::printf( "  (%s: %d bytes accepted=%d, %d accepted=%d, 12 accepted=%d)\n", c.pName, c.cbStruct,
                         bWhole, c.cbStruct - 1, bShort, bHdr );
    }

    // The sizes used above are the pinned wire sizes, not something the stand-ins made up.
    CHECK_EQ( PinOf( "CNetPlyrStatus" ), (int)sizeof( CNetPlyrStatus ) );
    CHECK_EQ( PinOf( "CMsgUnitControl" ), 17 );
    CHECK_EQ( PinOf( "CMsgGiveUnit" ), 60 );

    // Types whose handlers read nothing past the header stay at 12.
    const int aiBare[] = { CNetCmd::cmd_pause, CNetCmd::cmd_resume, CNetCmd::start_loaded_game };
    for ( int iType : aiBare )
    {
        CNetCmd cmd( iType );
        CHECK( cmd.FitsBuffer( (int)sizeof( CNetCmd ) ) );
    }
}

// Variable-length records: the shipped sender builds them, the receiver must take
// exactly that and refuse everything that lies about its length or its text.

static CPlayer NamedPlayer( const char* pName )
{
    CPlayer plr;
    plr.m_sName = pName;
    memset( &plr.m_InitData, 0x11, sizeof( plr.m_InitData ) );
    return plr;
}

// The four ways a self-sized record can lie; m_iLen sits at offset 12 in both.
static void CheckSelfSizedLies( const char* pWhat, const CNetCmd* pLegit, int cbLegit, int cbHeader, int iTextOff )
{
    Wire w;
    memcpy( w.a, pLegit, cbLegit );
    CHECK( w.p( )->FitsBuffer( cbLegit ) );
    CHECK( !w.p( )->FitsBuffer( (int)sizeof( CNetCmd ) ) );
    CHECK( !w.p( )->FitsBuffer( cbHeader - 1 ) );
    CHECK( !w.p( )->FitsBuffer( cbLegit - 1 ) );   // m_iLen claims more than arrived

    const int aiBadLen[] = { cbLegit + 1, INT_MAX, cbHeader - 1, 0, -1, INT_MIN };
    for ( int iLen : aiBadLen )
    {
        memcpy( w.a, pLegit, cbLegit );
        *(int*)( w.a + 12 ) = iLen;
        const bool bOk = w.p( )->FitsBuffer( cbLegit ) != FALSE;
        CHECK( !bOk );
        if ( bOk )
            std::printf( "  (%s: m_iLen %d accepted)\n", pWhat, iLen );
    }

    memcpy( w.a, pLegit, cbLegit );
    memset( w.a + iTextOff, 'A', cbLegit - iTextOff );   // text runs to the end of what arrived
    w.a[cbLegit] = 0;                                       // ...and ends just past it
    const bool bUnterminated = w.p( )->FitsBuffer( cbLegit ) != FALSE;
    CHECK( !bUnterminated );
    if ( bUnterminated )
        std::printf( "  (%s: unterminated text accepted)\n", pWhat );
}

static void TestPlayerAndChatRecords( )
{
    std::string sLong( (size_t)VP_MAXSENDDATA - sizeof( CNetPlayer ) - 1, 'p' );
    const char* apNames[] = { "", "Joiner", sLong.c_str( ) };
    for ( const char* pName : apNames )
    {
        CPlayer     plr  = NamedPlayer( pName );
        CNetPlayer* pMsg = CNetCmd::AllocPlayer( &plr );   // CmdReady / roster fan-out: Send( pMsg, GetLen( ) )
        CHECK_EQ( pMsg->GetLen( ), (int)( sizeof( CNetPlayer ) + strlen( pName ) + 1 ) );
        CHECK( pMsg->GetLen( ) <= (int)VP_MAXSENDDATA );
        CheckSelfSizedLies( "cmd_player", pMsg, pMsg->GetLen( ), (int)sizeof( CNetPlayer ),
                            (int)( pMsg->m_sName - (char*)pMsg ) );
        delete[] (char*)pMsg;
    }

    std::string sText( (size_t)VP_MAXSENDDATA - sizeof( CNetChat ) - 1, 't' );
    const char* apText[] = { "", "gg", sText.c_str( ) };
    for ( const char* pText : apText )
    {
        CPlayer   plr  = NamedPlayer( "Chatter" );
        CNetChat* pMsg = CNetChat::Alloc( &plr, pText );   // SDL2Chat_Send: Broadcast( pMsg, m_iLen )
        CHECK_EQ( pMsg->m_iLen, (int)( sizeof( CNetChat ) + strlen( pText ) + 1 ) );
        CheckSelfSizedLies( "cmd_chat", pMsg, pMsg->m_iLen, (int)sizeof( CNetChat ),
                            (int)( pMsg->m_sMsg - (char*)pMsg ) );
        delete[] (char*)pMsg;
    }
}

static void TestToHpRecord( )
{
    // No sender exists; the handler reads m_sName, so it must end inside what arrived.
    Wire w;
    memset( w.a, 0, sizeof( w.a ) );
    CNetToHp* pHp = (CNetToHp*)w.a;
    pHp->m_bMsg   = CNetCmd::cmd_to_hp;
    strcpy( pHp->m_sName, "Human" );
    const int cb = (int)sizeof( CNetToHp ) + 5;
    CHECK( pHp->FitsBuffer( cb ) );
    CHECK( !pHp->FitsBuffer( (int)sizeof( CNetCmd ) ) );
    CHECK( !pHp->FitsBuffer( (int)sizeof( CNetToHp ) - 1 ) );
    memset( pHp->m_sName, 'H', 16 );
    CHECK( !pHp->FitsBuffer( (int)sizeof( CNetToHp ) + 15 ) );   // unterminated inside what arrived
}

// Mail (ipc_msg) on the wire is a fixed 32-byte header -- CNetCmd, then m_iTo, m_iFrom,
// m_iCC, m_iType, m_iLen as 4-byte ints at 12..28 -- then message NUL subject NUL and 2 spare
// bytes. A literal, not sizeof( CMsgIPC ): that object holds two std::strings, 24 to 40 bytes
// each by library and config, so a peer built otherwise must still read the same bytes.
static const int kIpcHdr = 32;

static void PutInt( char* p, int iOff, int iVal )
{
    for ( int i = 0; i < 4; i++ )
        p[iOff + i] = (char)( ( (unsigned)iVal >> ( 8 * i ) ) & 0xFF );
}

static void TestIpcRecord( )
{
    CMsgIPC msg( CMsgIPC::email );   // SDL2ComposeDialog: m_iTo/m_iFrom/subject/body, then PostToClient
    msg.m_iFrom    = 4;
    msg.m_iTo      = 2;
    msg.m_sSubject = "Truce";
    msg.m_sMessage = "Leave my oil alone.";
    int   iLen = 0;
    char* pBuf = msg.ToBuf( &iLen );
    CHECK( pBuf != NULL );
    if ( pBuf == NULL )
        return;
    CHECK_EQ( iLen, (int)( kIpcHdr + strlen( "Truce" ) + strlen( "Leave my oil alone." ) + 4 ) );

    Wire w;
    memcpy( w.a, pBuf, iLen );
    CHECK( w.p( )->FitsBuffer( iLen ) );
    CHECK( !w.p( )->FitsBuffer( (int)sizeof( CNetCmd ) ) );
    CHECK( !w.p( )->FitsBuffer( kIpcHdr - 1 ) );
    CHECK( !w.p( )->FitsBuffer( kIpcHdr ) );   // header only: no message text at all

    // Cut right after the message's NUL: the subject is missing.
    const int cbNoSubject = kIpcHdr + (int)strlen( "Leave my oil alone." ) + 1;
    CHECK( !w.p( )->FitsBuffer( cbNoSubject ) );

    // Message text with no NUL inside what arrived.
    memset( w.a + kIpcHdr, 'm', iLen - kIpcHdr );
    w.a[iLen] = 0;
    CHECK( !w.p( )->FitsBuffer( iLen ) );
    delete[] pBuf;
}

// The shipped ToBuf writes exactly the documented bytes, whatever sizeof( CMsgIPC ) is here.
static void TestIpcWireBytes( )
{
    CHECK( (int)sizeof( CMsgIPC ) > kIpcHdr );   // the object really is bigger than its wire form
    CHECK_EQ( PinOf( "CMsgIPCWire" ), kIpcHdr );  // and wire_layout_assert.cpp pins the header

    CMsgIPC msg( CMsgIPC::email );
    msg.msgSize = 0; msg.msgKind = 0; msg.msgFlags = 0; msg.msgFrom = 0; msg.msgTo = 0; msg.msgId = 0;
    msg.m_iTo      = 4;
    msg.m_iFrom    = 2;
    msg.m_iCC      = 7;
    msg.m_iLen     = 0x01020304;
    msg.m_sSubject = "Re: oil";
    msg.m_sMessage = "Fine, take it.";
    int   iLen = 0;
    char* pBuf = msg.ToBuf( &iLen );
    CHECK( pBuf != NULL );
    if ( pBuf == NULL )
        return;

    char aWant[128];
    memset( aWant, 0, sizeof( aWant ) );
    aWant[10] = (char)CNetCmd::ipc_msg;   // m_bMsg; m_bMemPool (11) stays 0
    PutInt( aWant, 12, 4 );
    PutInt( aWant, 16, 2 );
    PutInt( aWant, 20, 7 );
    PutInt( aWant, 24, CMsgIPC::email );
    PutInt( aWant, 28, 0x01020304 );
    strcpy( aWant + kIpcHdr, "Fine, take it." );
    strcpy( aWant + kIpcHdr + strlen( "Fine, take it." ) + 1, "Re: oil" );
    const int cbWant = kIpcHdr + (int)strlen( "Fine, take it." ) + (int)strlen( "Re: oil" ) + 4;

    CHECK_EQ( iLen, cbWant );
    const bool bSame = iLen == cbWant && memcmp( pBuf, aWant, cbWant ) == 0;
    CHECK( bSame );
    if ( !bSame )
        std::printf( "  (ipc_msg: ToBuf wrote %d bytes, the wire format is %d; sizeof( CMsgIPC ) = %d here)\n",
                     iLen, cbWant, (int)sizeof( CMsgIPC ) );
    delete[] pBuf;
}

// The shipped receiver reads mail built byte by byte, as a peer of any compiler sends it,
// and mail from the shipped ToBuf.
static void TestIpcMailIsReceived( )
{
    CPlayer me     = NamedPlayer( "Me" );       // GetPlyrNum( ) == 4
    CPlayer sender = NamedPlayer( "Sender" );
    theGame.m_pMe     = &me;
    theGame.m_pSender = &sender;
    theGame.m_iSender = 2;

    // Built by hand: header ints, then the two strings.
    Wire w;
    memset( w.a, 0, sizeof( w.a ) );
    w.a[10] = (char)CNetCmd::ipc_msg;
    PutInt( w.a, 12, 4 );                 // to: us
    PutInt( w.a, 16, 2 );                 // from
    PutInt( w.a, 24, CMsgIPC::email );
    strcpy( w.a + kIpcHdr, "Body text" );
    strcpy( w.a + kIpcHdr + 10, "Subject line" );
    const int cb = kIpcHdr + 10 + 13 + 2;
    CHECK( w.p( )->FitsBuffer( cb ) );

    g_mailInbox.clear( );
    theGame.m_iMailEvt = 0;
    SDL2Mail_HandleIncoming( (CMsgIPC*)w.a );
    CHECK_EQ( (int)g_mailInbox.size( ), 1 );
    if ( g_mailInbox.size( ) == 1 )
    {
        const bool bOk = g_mailInbox[0].body == "Body text" && g_mailInbox[0].subject == "Subject line" &&
                         g_mailInbox[0].fromName == "Sender" && g_mailInbox[0].fromPlyr == 2;
        CHECK( bOk );
        if ( !bOk )
            std::printf( "  (ipc_msg: received body '%.20s' subject '%.20s' from '%s')\n", g_mailInbox[0].body.c_str( ),
                         g_mailInbox[0].subject.c_str( ), g_mailInbox[0].fromName.c_str( ) );
    }
    CHECK_EQ( theGame.m_iMailEvt, 1 );

    // Addressed to someone else: dropped.
    PutInt( w.a, 12, 3 );
    SDL2Mail_HandleIncoming( (CMsgIPC*)w.a );
    CHECK_EQ( (int)g_mailInbox.size( ), 1 );

    // Round trip through the shipped sender, FitsBuffer and receiver.
    CMsgIPC msg( CMsgIPC::email );
    msg.m_iFrom    = 2;
    msg.m_iTo      = 4;
    msg.m_sSubject = "Truce";
    msg.m_sMessage = "Leave my oil alone.";
    int   iLen = 0;
    char* pBuf = msg.ToBuf( &iLen );
    CHECK( pBuf != NULL );
    if ( pBuf != NULL )
    {
        Wire w2;
        memset( w2.a, 0, sizeof( w2.a ) );
        memcpy( w2.a, pBuf, iLen );
        CHECK( w2.p( )->FitsBuffer( iLen ) );
        g_mailInbox.clear( );
        SDL2Mail_HandleIncoming( (CMsgIPC*)w2.a );
        CHECK_EQ( (int)g_mailInbox.size( ), 1 );
        if ( g_mailInbox.size( ) == 1 )
            CHECK( g_mailInbox[0].body == "Leave my oil alone." && g_mailInbox[0].subject == "Truce" );
        delete[] pBuf;
    }

    // The legacy chat type folds into the chat log.
    memset( w.a, 0, sizeof( w.a ) );
    w.a[10] = (char)CNetCmd::ipc_msg;
    PutInt( w.a, 12, 4 );
    PutInt( w.a, 16, 2 );
    PutInt( w.a, 24, CMsgIPC::chat );
    strcpy( w.a + kIpcHdr, "hi" );
    g_chatLog.clear( );
    SDL2Mail_HandleIncoming( (CMsgIPC*)w.a );
    CHECK( g_chatLog.size( ) == 1 && g_chatLog[0] == "Sender: hi" );

    g_mailInbox.clear( );
    theGame.m_pMe     = NULL;
    theGame.m_pSender = NULL;
}

static void TestAiMsgWrapsAWholeRecord( )
{
    CPlayer plr = NamedPlayer( "AI" );

    // A client tells a host AI it lost line of sight: CGame::PostClientToClient
    // wraps sizeof( CMsgOutOfLos ) bytes with CMsgAiMsg::Alloc and sends m_iAllocLen.
    Wire inner;
    memset( inner.a, 0, sizeof( inner.a ) );
    inner.p( )->m_bMsg = CNetCmd::out_of_LOS;
    CMsgAiMsg* pAi = CMsgAiMsg::Alloc( &plr, inner.p( ), (int)sizeof( CMsgOutOfLos ) );
    CHECK_EQ( pAi->m_iAllocLen, (int)( sizeof( CMsgAiMsg ) + sizeof( CMsgOutOfLos ) ) );
    Wire w;
    memcpy( w.a, pAi, pAi->m_iAllocLen );
    const int cb = pAi->m_iAllocLen;
    delete[] (char*)pAi;
    CHECK( w.p( )->FitsBuffer( cb ) );
    CHECK( !w.p( )->FitsBuffer( (int)sizeof( CNetCmd ) ) );
    CHECK( !w.p( )->FitsBuffer( (int)sizeof( CMsgAiMsg ) - 1 ) );
    CHECK( !w.p( )->FitsBuffer( cb - 1 ) );   // inner record cut short in transit

    // m_iLen that lies about the inner record.
    const int aiBadLen[] = { (int)sizeof( CMsgOutOfLos ) + 1, INT_MAX, (int)sizeof( CNetCmd ) - 1, 0, -1, INT_MIN };
    for ( int iLen : aiBadLen )
    {
        ( (CMsgAiMsg*)w.a )->m_iLen = iLen;
        CHECK( !w.p( )->FitsBuffer( cb ) );
    }

    // A wrapped record too short for its own type is refused even when the
    // wrapper's lengths agree with each other.
    pAi = CMsgAiMsg::Alloc( &plr, inner.p( ), (int)sizeof( CMsgOutOfLos ) - 4 );
    memcpy( w.a, pAi, pAi->m_iAllocLen );
    const int cbShort = pAi->m_iAllocLen;
    delete[] (char*)pAi;
    CHECK( !w.p( )->FitsBuffer( cbShort ) );

    // A wrapped variable-length record is checked by its own rule too.
    CNetPlayer* pPlr = CNetCmd::AllocPlayer( &plr );
    pAi = CMsgAiMsg::Alloc( &plr, pPlr, pPlr->GetLen( ) );
    memcpy( w.a, pAi, pAi->m_iAllocLen );
    CHECK( w.p( )->FitsBuffer( pAi->m_iAllocLen ) );
    ( (CNetPlayer*)( (CMsgAiMsg*)w.a + 1 ) )->m_iLen = 600;
    CHECK( !w.p( )->FitsBuffer( pAi->m_iAllocLen ) );
    delete[] (char*)pAi;
    delete[] (char*)pPlr;
}

static void TestGetFileLength( )
{
    CPlayer   me  = NamedPlayer( "Joiner" );
    CPlayer   srv = NamedPlayer( "Host" );
    const int aiGood[] = { 1, 8214278, MAX_NET_GAME_FILE };   // 8214278: the largest save on the dev box
    for ( int iBuf : aiGood )
    {
        CNetGetFile msg( &me, &srv, iBuf );   // CmdSelectPlyr: PostToClient( &msg, sizeof( msg ) )
        CHECK( msg.FitsBuffer( (int)sizeof( msg ) ) );
        CHECK( !msg.FitsBuffer( (int)sizeof( msg ) - 1 ) );
    }
    const int aiBad[] = { 0, -1, INT_MIN, MAX_NET_GAME_FILE + 1, INT_MAX };
    for ( int iBuf : aiBad )
    {
        CNetGetFile msg( &me, &srv, iBuf );
        const bool bOk = msg.FitsBuffer( (int)sizeof( msg ) ) != FALSE;
        CHECK( !bOk );
        if ( bOk )
            std::printf( "  (cmd_get_file: m_iBufLen %d accepted)\n", iBuf );
    }
}

int main( )
{
    TestLegitPlyrJoinRoundTrips( );
    TestHostilePlyrJoinIsRefused( );
    TestAllocCopyIsBounded( );
    TestSelectFamilyNeedsTheWholeRecord( );
    TestEveryFixedTypeNeedsItsStruct( );
    TestPlayerAndChatRecords( );
    TestToHpRecord( );
    TestIpcRecord( );
    TestIpcWireBytes( );
    TestIpcMailIsReceived( );
    TestAiMsgWrapsAWholeRecord( );
    TestGetFileLength( );
    return microtest::Summary( );
}
