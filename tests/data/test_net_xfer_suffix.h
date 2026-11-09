// Tests for the shipped load-join save transfer (CVPTransfer over CDataTransfer).
// The host sends its save with SendDataTo; the joiner receives it into the
// m_iBufLen-byte buffer CmdGetFile allocated. See test_net_xfer_prefix.h.
#include "../ai/microtest.h"

static const size_t kCanary  = 4096;
static char*        g_pLast  = NULL;
static size_t       g_cbLast = 0;

void* operator new[]( size_t cb )
{
    if ( cb > ( 1u << 26 ) )
        throw std::bad_alloc( );
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

static bool CanaryIntact( const char* p, size_t cb )
{
    for ( size_t i = 0; i < kCanary; i++ )
        if ( (unsigned char)p[cb + i] != 0xAB )
            return false;
    return true;
}

static const VPPLAYERID kHost   = 2;
static const VPPLAYERID kJoiner = 3;

static BOOL Deliver( CVPTransfer& xfer, const XferPacket& pkt )
{
    VPMESSAGE msg;
    memset( &msg, 0, sizeof( msg ) );
    msg.senderId = pkt.from;
    msg.toId     = pkt.to;
    msg.dataLen  = (DWORD)( pkt.bytes.size( ) - sizeof( VPMSGHDR ) );
    msg.u.data   = (LPVOID)( pkt.bytes.data( ) + sizeof( VPMSGHDR ) );
    return xfer.ProcessNotification( VP_READDATA, &msg );
}

// A packet the way vpSendData would carry it: header, then cbPayload bytes.
static XferPacket MakePacket( VPPLAYERID from, VPPLAYERID to, const void* pPayload, size_t cbPayload, size_t cbExtra = 0 )
{
    XferPacket pkt;
    pkt.from = from;
    pkt.to   = to;
    pkt.bytes.assign( sizeof( VPMSGHDR ) + cbPayload + cbExtra, 0 );
    memcpy( pkt.bytes.data( ) + sizeof( VPMSGHDR ), pPayload, cbPayload );
    // pkt.bytes.size( ) is what arrived; the test trims it back below when it
    // wants readable bytes past the end.
    return pkt;
}

static void TestLegitTransferArrivesWhole( )
{
    const DWORD cbFiles[] = { 1, 600, 50000, 8214278 / 64 };
    for ( DWORD cbFile : cbFiles )
    {
        std::vector<char> file( cbFile );
        for ( DWORD i = 0; i < cbFile; i++ )
            file[i] = (char)( i * 131 + 7 );

        g_xferWire.clear( );
        CVPTransfer host( NULL );
        CVPTransfer joiner( NULL );
        char*       pBuf = new char[cbFile];   // CmdGetFile: new char[ m_iBufLen ]
        CHECK( joiner.ReceiveDataFrom( kHost, kJoiner, pBuf, cbFile ) );
        CHECK( host.SendDataTo( kJoiner, kHost, file.data( ), cbFile ) );

        int iPackets = 0;
        while ( !g_xferWire.empty( ) && iPackets < 1000000 )
        {
            XferPacket pkt = g_xferWire.front( );
            g_xferWire.pop_front( );
            CHECK( Deliver( pkt.to == kJoiner ? joiner : host, pkt ) );
            iPackets++;
        }
        CHECK( joiner.Done( ) );
        CHECK( host.Done( ) );
        CHECK_EQ( joiner.GetError( ), 0 );
        CHECK_EQ( host.GetError( ), 0 );
        CHECK( memcmp( pBuf, file.data( ), cbFile ) == 0 );
        CHECK( CanaryIntact( pBuf, cbFile ) );
        delete[] pBuf;
    }
}

static void TestJoinerRefusesMoreThanAnnounced( )
{
    // One packet bigger than the whole announced file, and two that only
    // overrun together.
    const DWORD aSplit[][2] = { { 650, 0 }, { 400, 400 } };
    for ( const auto& split : aSplit )
    {
        g_xferWire.clear( );
        CVPTransfer joiner( NULL );
        const DWORD cbFile = 600;
        char*       pBuf   = new char[cbFile];
        joiner.ReceiveDataFrom( kHost, kJoiner, pBuf, cbFile );

        for ( DWORD cbChunk : split )
        {
            if ( cbChunk == 0 || joiner.GetError( ) )
                continue;
            std::vector<char> payload( sizeof( DWORD ) + cbChunk, 'X' );
            *(DWORD*)payload.data( ) = MSG_FILE_XFER;
            Deliver( joiner, MakePacket( kHost, kJoiner, payload.data( ), payload.size( ) ) );
        }
        const bool bIntact = CanaryIntact( pBuf, cbFile );
        CHECK( bIntact );
        CHECK( joiner.GetError( ) != 0 );
        if ( !bIntact )
            std::printf( "  (transfer wrote past the %u-byte buffer: %u + %u bytes)\n", (unsigned)cbFile,
                         (unsigned)split[0], (unsigned)split[1] );
        delete[] pBuf;
    }
}

static void TestHostRefusesAShortAck( )
{
    g_xferWire.clear( );
    std::vector<char> file( 100, 'f' );
    CVPTransfer       host( NULL );
    host.SendDataTo( kJoiner, kHost, file.data( ), (DWORD)file.size( ) );
    g_xferWire.clear( );

    // An ACK is the tag and a DWORD count; this one stops after the tag.
    DWORD      tag = MSG_FILE_XFER;
    XferPacket pkt = MakePacket( kJoiner, kHost, &tag, sizeof( tag ), 4 );
    memset( pkt.bytes.data( ) + sizeof( VPMSGHDR ) + sizeof( tag ), 0xFF, 4 );   // readable, but not sent
    VPMESSAGE msg;
    memset( &msg, 0, sizeof( msg ) );
    msg.senderId = pkt.from;
    msg.toId     = pkt.to;
    msg.dataLen  = sizeof( tag );
    msg.u.data   = pkt.bytes.data( ) + sizeof( VPMSGHDR );
    host.ProcessNotification( VP_READDATA, &msg );
    CHECK( host.GetError( ) != 0 );
}

static void TestGameMessagesStillPassThrough( )
{
    // While a transfer is open every VP_READDATA goes through it first. A game
    // message from the host (a 12-byte cmd_pause: 2 data bytes) is not the
    // transfer's and must come back FALSE without an error.
    g_xferWire.clear( );
    CVPTransfer joiner( NULL );
    char*       pBuf = new char[64];
    joiner.ReceiveDataFrom( kHost, kJoiner, pBuf, 64 );
    const BYTE abPause[2] = { 10, 0 };
    XferPacket pkt = MakePacket( kHost, kJoiner, abPause, sizeof( abPause ), 2 );
    VPMESSAGE  msg;
    memset( &msg, 0, sizeof( msg ) );
    msg.senderId = pkt.from;
    msg.toId     = pkt.to;
    msg.dataLen  = sizeof( abPause );
    msg.u.data   = pkt.bytes.data( ) + sizeof( VPMSGHDR );
    CHECK( !joiner.ProcessNotification( VP_READDATA, &msg ) );
    CHECK_EQ( joiner.GetError( ), 0 );
    delete[] pBuf;
}

// Runs last: on the unfixed code the 2-byte message below decodes as a transfer
// packet (the 2 bytes past it complete the tag) and dataLen - 4 wraps to ~4 GB.
static int ProcessGuarded( CVPTransfer* pXfer, VPMESSAGE* pMsg, BOOL* pbRet )
{
    __try
    {
        *pbRet = pXfer->ProcessNotification( VP_READDATA, pMsg );
        return 0;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        return 1;
    }
}

static void TestTwoByteMessageIsNotATransferPacket( )
{
    g_xferWire.clear( );
    CVPTransfer* pJoiner = new CVPTransfer( NULL );
    char*        pBuf    = new char[64];
    pJoiner->ReceiveDataFrom( kHost, kJoiner, pBuf, 64 );

    DWORD      tag = MSG_FILE_XFER;   // 03 05 00 00
    XferPacket pkt = MakePacket( kHost, kJoiner, &tag, sizeof( tag ) );
    VPMESSAGE  msg;
    memset( &msg, 0, sizeof( msg ) );
    msg.senderId = pkt.from;
    msg.toId     = pkt.to;
    msg.dataLen  = 2;   // only 03 05 arrived
    msg.u.data   = pkt.bytes.data( ) + sizeof( VPMSGHDR );
    BOOL bRet    = TRUE;
    std::fflush( stdout );
    const int iFault = ProcessGuarded( pJoiner, &msg, &bRet );
    CHECK_EQ( iFault, 0 );
    if ( iFault == 0 )
    {
        CHECK( !bRet );
        CHECK_EQ( pJoiner->GetError( ), 0 );
    }
    else
        std::printf( "  (2-byte message faulted inside ProcessNotification)\n" );
    // leaked on purpose: after a fault the object is not safe to destroy
}

int main( )
{
    TestLegitTransferArrivesWhole( );
    TestJoinerRefusesMoreThanAnnounced( );
    TestHostRefusesAShortAck( );
    TestGameMessagesStillPassThrough( );
    TestTwoByteMessageIsNotATransferPacket( );
    return microtest::Summary( );
}
