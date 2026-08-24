#ifndef __VPFRMWK_H__
#define __VPFRMWK_H__

#include "stdafx.h"
#include "vpparam.h"

class CSession;

class CPlayer;

class CNetLink;

class CNetAddress;

class CNetInterface;

class CWS;

class CLocalWS;

class CRemotePlayer;

class CLocalPlayer;

class CTDLogger;

#define IMPOSSIBLE(x) ShouldNotBeHere(className, #x, NULL)
#define IMPLEMENT_CLASSNAME(C) const char FAR C::className[] = #C
/*
  We have Players, Sessions, Workstations.....
  The Players can be Local (to workstation) or Remote
  The Sessions can be too Local (which means this workstation is
  the Session Server) or Remote...
  The Workstations can be too Local (only one such is allowed) and
  Remote.  With each Remote workstation could be associated up to 2 NetLinks
  one is for non garanteeed data delivery, the second (optional) for
  garanteed data delivery.
  We're capable to doing GARANTEED DELIVERY BROADCASTs (with the help
  of the server of course)
  Each player is identified by a PlayerID which is assigned by the 
  server... Each workstation maintains a copy of player map for the
  joined session. 
  Sessions uses NetInterfaces to manage NetLinks and NetAddresses
   
  Join Transaction:
   The WS send to server JoinREQ and waits for server responce
   The Server 
    1) assignes the playerID, 
    2) respond with JoinREP
    3) Advertises the player to other WSs with JoinADV
      The original WS get the JoinREP and store the player
   info in it's player map.
   The workstation then sends PenumREQ to the server to obtain the list of players
   the server responds with a series of PenumREP....
      Other Workstations receives the JoinADV and updates their
   player map.
   The REQ/REP pairs are matched uisng msgId field in the message header
         

  Player enumeration Transaction:
   The client connects (if not laready connected) to the server  and send
 PEnumREQ...
 The server responds with a series of PenumREPs each containing info on one player
 Each the player info contains the sequence information as in faxes - i-th of N...
 When the client get i == N  it know there is no more info
   
  

 PlayerMap and WSMap are abstract classes used to manage the player and workstation collection

 Notification hierarchy serves to encapsuluate various notifications to the user app

 genericMsg  hierarchy encapsulates messages exchanged over the network links


   
*/

// structure used to pack parameters for various SendData calls
struct sendDataInfo {
    sendDataInfo(LPVOID data,   // data to send
                 DWORD dataSize,
                 DWORD sendFlags,
                 LPVOID userData,  // user data for completion notification
                 LPVOID ctxData = NULL // context data for splitted ops
    ) :
            m_data(data), m_dataSize(dataSize),
            m_sendFlags(sendFlags), m_userData(userData),
            m_ctxData(ctxData) {}


    LPVOID m_data;
    DWORD m_dataSize;
    DWORD m_sendFlags;
    LPVOID m_userData;
    LPVOID m_ctxData;
};


class CNotification : public CActivity {
public:
    VPMESSAGE m_vpmsg;

    // !!!!!!  Warning,  NON PORTABLE construct !!!!!!!!!!!!!!
    static CNotification *ContainingObject(LPCVPMESSAGE msg) {
        size_t off = offsetof(CNotification, m_vpmsg);

        return (CNotification *) (((LPSTR) msg) - off);
    }

    CNotification(WORD msgCode, LPVOID data, DWORD dataLen,
                  LPVOID userData);

    virtual ~CNotification();

    void SetResult(WORD code, WORD err) {
        m_vpmsg.notificationCode = (code | (err << 8));
    }

    virtual void Complete() {}

    void SetError(WORD errCode) {
        SetResult((WORD) (m_vpmsg.notificationCode & 0x00ff), errCode);
    }


};


// Workstation
class CWS : public CVPLink, public CRef {
public:
    virtual BOOL SendData(sendDataInfo &sdi) = 0;

    virtual BOOL IsSameWs(CWS *ws) const = 0;

    virtual CNetAddress *GetAddress() const { return NULL; }

    void AddPlayer() {
        m_plrCount++;
    }

    void RemovePlayer() {
        m_plrCount--;
    }

    WORD PlayerCount() const {
        return m_plrCount;
    }

    DWORD GetLastError() {
        DWORD ret = m_error;

        m_error = VP_ERR_OK;
        return ret;
    }

    BOOL IsLocal() const { return !IsRemote(); }

    virtual BOOL IsRemote() const = 0;

    void Touch() { m_lastSeen = GetCurrentTime(); }

    DWORD LastSeen() const { return m_lastSeen; }

    CWS() : m_error(0), m_plrCount(0), m_ftInfo(NULL) {}

    int FtState() const {
        return m_ftInfo ? m_ftInfo->ftState : 0;
    }

    void SetFtState(int state) {
        if (m_ftInfo)
            m_ftInfo->ftState = state;
    }


public:
    DWORD m_error;
    WORD m_plrCount;
    DWORD m_lastSeen;
    LPVPFTINFO m_ftInfo;


};

struct sesInfoMsg;

class CRemoteWS : public CWS {
public:
    CNetLink *m_safeLink;   // a link for garanteed delivery
    CNetLink *m_unsafeLink;  // a link for non garanteed delivery
    CNetAddress *m_address;
    sesInfoMsg *m_info;
    DWORD m_safeTrafficTime; // when we had the last traffic over
    // the safe link

    // Host relay (docs/plans/host-relay-spec.md 4b): this peer is unreachable
    // directly, so its traffic rides the join link and the host forwards it.
    // Set on a CLIENT only, and never on the session's server WS; sticky for
    // the session (no retries in v1). Meaningless while EN_HOST_RELAY is off:
    // nothing sets it and every reader checks the gate first.
    BOOL m_relayMode;

    // One-shot lobby probe marker (spec 8): the async direct dial has already
    // been kicked for this peer, don't kick it again.
    BOOL m_relayProbed;

    // When THIS peer's direct dial started (0 = no dial pending), for the same
    // bounded-dial watchdog CRemoteSession::OnTimer runs over the server dial.
    // The session's m_dialStart covers only m_serverWS; a client-to-client dial
    // into a NAT that DROPS the SYN never completes and never errors inside the
    // app's horizon, so OnDisconnect never ran and m_relayMode never armed - the
    // host-relay fallback was unreachable in exactly the blackhole topology it
    // exists for. Armed by the peer dial sites, disarmed on connect and on any
    // teardown of the safe link (a stale stamp must not outlive its dial).
    DWORD m_dialStart;


    sesInfoMsg *Info() const { return m_info; }

    virtual CNetAddress *GetAddress() const { return m_address; }

    virtual BOOL SendData(sendDataInfo &sdi);

    virtual BOOL IsSameWs(CWS *ws) const {
        return m_address->IsEqual(ws->GetAddress());
    }

    ~CRemoteWS();

    CRemoteWS(CNetAddress *address,
              CNetLink *safeLink, CNetLink *unsafeLink) :
            m_address(address), m_safeLink(safeLink), m_unsafeLink(unsafeLink),
            m_safeTrafficTime(0), m_info(NULL),
            m_relayMode(FALSE), m_relayProbed(FALSE), m_dialStart(0) {
        if (m_safeLink)
            m_safeLink->Ref();
        if (m_unsafeLink)
            m_unsafeLink->Ref();
        m_address->Ref();

    }

    void KeepInfo(sesInfoMsg *m);

    void StopUsingSafeLink() {
        if (m_safeLink)
            m_safeLink->Unref();
        m_safeLink = 0;
        m_dialStart = 0;   // no link, no pending dial - never leave a stale stamp
                           // behind for a later SetSafeLink (inbound accept) to
                           // be expired against.
    }

    void SetSafeLink(CNetLink *link) {
        VPASSERT(!m_safeLink);
        m_safeLink = link;
        m_safeLink->Ref();
    }

    BOOL IsRemote() const { return TRUE; }

    void SetAddress(CNetAddress *a) {
        a->Ref();
        if (m_address)
            m_address->Unref();
        m_address = a;
    }


    void *operator new(size_t s);

    void operator delete(void *);

    unsigned Unref() {
        unsigned ret = --m_refCnt;
        if (!ret)
            delete this;
        return ret;
    }


    static void InitPool();

    static void CleanPool();

};

class CLocalWS : public CWS {
public:
    virtual BOOL SendData(sendDataInfo &sdi) { return TRUE; }

    virtual BOOL IsSameWs(CWS *ws) { return ws == this; }

    BOOL IsRemote() const { return FALSE; }

    CSession *m_session;
};

enum MessageCodes {
    JoinREQ = 'J', // Join request 
    JoinREP = 'j', // Join Reply (Acceptance of a the join request)
    JoinNAK = 'N', // Join Negative acknowledge
    JoinADV = 'A', // Join advertisement (sent by the server to advertise
    // that somebody joined the session)
    LeaveREQ = 'L',  // Leave Request
    LeaveREP = 'l', // Leave Reply

    PenumREQ = 'P',   // Player Enum Request
    PenumREP = 'p',   // Player Enum Reply

    SenumREQ = 'S',   // Session Enum Request
    SenumREP = 's',   // Session Enum Reply
    UDataREQ = 'U',   // User Data Request
    UBDataREQ = 'B',   // User Broadcast Data Request
    FtREQ = 'F',  // File transfer Request
    FtACK = 'f',   // File transfer ACK
    DummyREQ = 'D',  // dummy packet to signal link alive

    // NAT hole-punch rendezvous (docs/plans/nat-holepunch-iserve-feasibility.md P1).
    // All ride the existing dg/:1707 channel; iserve stays a registry+signaler
    // (stateless forward), game traffic never touches it. Peer probes are gated
    // by EN_NAT_PUNCH; old builds route unknown kinds to OnUnexpectedMsg (ignored).
    NatPunchREQ = 'H',  // client -> iserve: rendezvous me with session X
    NatPunchFWD = 'W',  // iserve -> host: a joiner wants in; its candidates
    NatPunchREP = 'h',  // iserve -> client: the host's candidates
    NatPunchPING = 'Y',  // peer -> peer: punch probe (opens/keeps the mapping)
    NatPunchPONG = 'y'   // peer -> peer: probe reply (nonce echo = pair works)
};


//
//!!!! IMPORTANT: these thing are transmitted over tghen wire so keep pragma in place 
//

#pragma pack(1)

struct genericMsg : CRef {
    VPMSGHDR hdr;

    DWORD Size() const { return hdr.msgSize; }

    LPVOID Data() { return &hdr; }

    LPVOID Contents() { return 1 + &hdr; }

    DWORD ContentSize() const { return hdr.msgSize ? (hdr.msgSize - sizeof(hdr)) : 0u; }

#ifdef USE_VPMEM
    void* operator new(size_t s, size_t moreData)
    {
     size_t* p = (size_t*) vpAllocMem(s+ moreData+sizeof(size_t));
     if (p)
     {
      *p++ = s+moreData+sizeof(size_t);
     }
     return p;
    }
    void operator delete(void* p, size_t s)
    {
     size_t* pp = (size_t*) p;

     pp--;
     vpFreeMem(pp, *pp);
    }
#else


    void *operator new(size_t s, size_t moreData) {
        return new char[s + moreData];
    }


#if _MSC_VER >= 1930    // VS2022
    void operator delete( void* obj ) noexcept
    {
        reinterpret_cast<genericMsg*>( obj )->~genericMsg( );
        free( obj );
    }
#elif _MSC_VER >= 1920  // VS2019
    void operator delete( void* obj ) noexcept
    {
        reinterpret_cast<genericMsg*>( obj )->~genericMsg( );
        std::free( obj );
    }
#else
    // is this all I needed?
    void operator delete( void* obj ) noexcept
    {
        reinterpret_cast<genericMsg*>( obj )->~genericMsg( );
        std::free( obj );
    }
#endif



#endif

    // Zero the whole header before filling it in. The old ctors assigned only
    // msgKind/msgSize, leaving msgFlags/msgFrom/msgTo/msgId as whatever the
    // stack held — and those bytes go out on the wire. Measured against the
    // live AWS iserve: its empty-registry DummyREQ carried msgFrom=0x1810
    // msgTo=0x261b msgId=0x7fff, identical across probes (deterministic stack
    // residue), i.e. the process was leaking stack to unauthenticated clients.
    // Dispatch switches on msgKind alone and the engine reads msgTo in exactly
    // one place (CDataNotification, below) where it is surfaced as toId, so no
    // routing changes; senders that set these fields still overwrite the zeros.
    genericMsg() { memset( &hdr, 0, sizeof( hdr ) ); }

    genericMsg(MessageCodes code, size_t dataSize) {
        memset( &hdr, 0, sizeof( hdr ) );
        hdr.msgKind = code;
        hdr.msgSize = sizeof(hdr) + dataSize;
    }

};

struct plrInfoMsg : genericMsg {
    struct {
        WORD index, total;
    } seq; // Sequencing info for multi-part responecs;

    VPPLAYERINFO data;

    // Zero the payload. The ctors used to initialise only the header, leaving
    // seq/data/playerData as allocator residue — and operator new is
    // `new char[s + moreData]`, which does not zero. Those bytes go on the wire
    // (measured on sesInfoMsg: a UTF-16 registry path reached the registration
    // server). Sized from the members, NOT from hdr.msgSize: this ctor's size
    // expression adds sizeof(playerName) where sesInfoMsg subtracts it, so
    // msgSize is 2 bytes LARGER than the allocation and memsetting by it would
    // turn a read overrun into a write overrun. See board 2026-08-23.
    plrInfoMsg() { memset( &seq, 0, sizeof( seq ) + sizeof( data ) ); }

    plrInfoMsg(size_t dataSize) :
            genericMsg(PenumREP,
                       sizeof(seq) + dataSize + sizeof(data) + sizeof(data.playerName)) {
        memset( &seq, 0, sizeof( seq ) + sizeof( data ) + dataSize );
    }
};


struct sesInfoMsg : genericMsg {
    VPSESSIONINFO data;

    // Zero the payload — see plrInfoMsg above. This is the struct the leak was
    // measured on: playerCount/sessionFlags arrived as garbage and the
    // sessionData tail carried a UTF-16 registry path from a freed audio
    // allocation. sizeof(data) + dataSize is exactly the space operator new
    // reserved past the header, so this stays in bounds.
    sesInfoMsg() { memset( &data, 0, sizeof( data ) ); }

    sesInfoMsg(size_t dataSize) :
            genericMsg(SenumREP, dataSize + sizeof(data) - sizeof(data.sessionName)) {
        memset( &data, 0, sizeof( data ) + dataSize );
    }

    ~sesInfoMsg() override {}

};

struct ftMsg : genericMsg {
    VPFTINFO data;

    // Zero the payload — see plrInfoMsg above. Fixed-size body, no tail.
    ftMsg(MessageCodes code) :
            genericMsg(code, sizeof(data)) {
        memset( &data, 0, sizeof( data ) );
    }
};

// NAT hole-punch rendezvous payload (all 5 NatPunch* kinds share it; unused
// fields stay zero). Addresses are VPNETADDRESS overlays in the transport's
// normal form (tcpaddress_s for TCP), candidate tails per vpnatcand.h.
// Wire size: 3*28 + 4 + 4 = 92 (+10 hdr) — well under VP_MAXSENDDATA.
struct natPunchInfo {
    VPSESSIONID sessionId;   // target session as enumerated (incl. stamped tail)
    VPNETADDRESS pubCand;    // requesting/target peer's OBSERVED public candidate
    VPNETADDRESS privCand;   // that peer's self-reported private candidate
    DWORD nonce;             // round-trip id: PING/PONG echo proves the pair
    DWORD flags;             // EN_NATCAND_F_* hints (bit0: peers share a public IP)
};

struct natPunchMsg : genericMsg {
    natPunchInfo data;

    natPunchMsg(MessageCodes code) :
            genericMsg(code, sizeof(data)) {
        memset(&data, 0, sizeof(data));
    }
};

static_assert(sizeof(natPunchInfo) == 92, "natPunchInfo wire size (3x28 + 2x4, pack(1))");

struct ftReqMsg : ftMsg {
    ftReqMsg() : ftMsg(FtREQ) {}
};

struct ftAckMsg : ftMsg {
    ftAckMsg() : ftMsg(FtACK) {}
};


inline void CRemoteWS::KeepInfo(sesInfoMsg *m) {
    m_info = m;
    m_info->Ref();
}

#pragma pack()


class CPlayer {
public:

    CPlayer(LPVPPLAYERINFO pInfo, CWS *ws);

    CPlayer(plrInfoMsg *pInfoMsg, CWS *ws);

    virtual ~CPlayer();

    virtual BOOL SendData(sendDataInfo &sdi) = 0;


    VPPLAYERID PlayerId() const {
        VPASSERT(m_info);
        return m_info->data.playerId;
    }

    BOOL IsLocal() const { return !IsRemote(); }

    virtual BOOL IsRemote() const = 0;

    void AssignWs(CWS *ws) {
        if (ws)
            ws->Ref();
        if (m_ws)
            m_ws->Unref();

        m_ws = ws;
    }

public:
    plrInfoMsg *m_info;
    CWS *m_ws;   // player home workstation;
};


#if 0
class CLocalLink: public CNetLink
{
public:

};
#endif


class CWSMap {
public:
    typedef BOOL (*EnumProc)(CWS *ws, LPVOID data);

    virtual unsigned Count() = 0;

    virtual void Enum(EnumProc p, LPVOID data) = 0;

    virtual void Register(CWS *ws) = 0;

    virtual void Deregister(CWS *ws) = 0;

    virtual CWS *Find(CWS *ws) = 0;

    virtual CWS *FindByAddress(CNetAddress *a) = 0;

    virtual CWS *FindBySafeLink(CNetLink *a) = 0;

    virtual void Cleanup() = 0;

    virtual ~CWSMap() {}

};

class CNetLinkMap {
    typedef BOOL (*EnumProc)(CNetLink *link, LPVOID data);

    void Add(CNetLink *link);

    void Remove(CNetLink *link);

    void Enum(EnumProc p, LPVOID data);

};


//+ Player collection management
class CPlayerMap {
public:
    typedef BOOL (*EnumProc)(CPlayer *, LPVOID);

    virtual BOOL AddPlayer(CPlayer *p) = 0;

    virtual CPlayer *PlayerAtId(VPPLAYERID playerId) = 0;

    virtual BOOL RemovePlayer(VPPLAYERID id) = 0;

    virtual DWORD Count() const = 0;

    virtual void Enum(EnumProc p, LPVOID userData) = 0;

    virtual void Cleanup() = 0;

    CPlayerMap() {}

    virtual ~CPlayerMap() {}
};


//+  Network message dispatching protocol
class CMsgDispatcher {
public:

    virtual BOOL OnJoinREQ(plrInfoMsg *msg, CRemoteWS *ws) = 0;

    virtual BOOL OnJoinREP(plrInfoMsg *msg, CRemoteWS *ws) = 0;

    virtual BOOL OnJoinNAK(plrInfoMsg *msg, CRemoteWS *ws) = 0;

    virtual BOOL OnJoinADV(plrInfoMsg *msg, CRemoteWS *ws) = 0;


    virtual BOOL OnLeaveREQ(plrInfoMsg *msg, CRemoteWS *ws) = 0;

    virtual BOOL OnUDataREQ(genericMsg *msg, CRemoteWS *ws) = 0;

    virtual BOOL OnUBDataREQ(genericMsg *msg, CRemoteWS *ws) = 0;

    virtual BOOL OnPenumREQ(genericMsg *msg, CRemoteWS *ws) = 0;

    virtual BOOL OnPenumREP(plrInfoMsg *msg, CRemoteWS *ws) = 0;

    virtual BOOL OnSenumREQ(genericMsg *msg, CRemoteWS *ws) = 0;

    virtual BOOL OnSenumREP(sesInfoMsg *msg, CRemoteWS *ws) = 0;

    virtual BOOL OnFtREQ(ftReqMsg *msg, CRemoteWS *ws) = 0;

    virtual BOOL OnFtACK(ftAckMsg *msg, CRemoteWS *ws) = 0;


    virtual void OnUnexpectedMsg(genericMsg *, CNetLink *, BOOL isSafe) = 0;

    virtual ~CMsgDispatcher() {}
};

class CSession {

public:
    CSession() : m_error(0), m_errInfo(0), m_invalid(FALSE) {}

    virtual ~CSession() {}

    BOOL IsLocal() const { return !IsRemote(); }

    BOOL IsValid() const { return !m_invalid; }

    virtual BOOL IsRemote() const = 0;

    virtual BOOL IsServerMode() const { return IsLocal(); }

    virtual void PostNotification(CNotification *) = 0;

    virtual BOOL AddLocalPlayer(LPCSTR playerName,
                                DWORD playerFlags,
                                LPVOID userData, LPVPPLAYERID pId = NULL) = 0;


    virtual BOOL SendData(VPPLAYERID fromId, VPPLAYERID toId,
                          LPVOID data, DWORD dataSize, DWORD flags, LPVOID userData) = 0;


    virtual BOOL UpdateSessionInfo(LPVOID data) = 0;

    virtual BOOL GetSessionInfo(LPVPSESSIONINFO sesInfo) = 0;

    virtual BOOL CloseSession() = 0;

    virtual BOOL KillPlayer(VPPLAYERID id) = 0;


    virtual BOOL SetVisibility(BOOL visibility) = 0;


    // Start File Transfer
    virtual BOOL StartFT(LPVPFTINFO ftInfo) = 0;

    // Accept incoming file transfer request
    virtual BOOL AcceptFT(LPVPFTINFO ftInfo) = 0;


    // Send a file  data block
    virtual BOOL SendBlock(LPVPFTINFO ftInfo,  // file transfer info
                           LPCVOID buf,
                           DWORD bufSize
    ) = 0;

    // Receive a file data block
    virtual BOOL GetBlock(LPVPFTINFO ftInfo,    // file transfer info
                          LPVOID buf,
                          DWORD bufSize
    ) = 0;

    // Terminate a File Transfer
    virtual BOOL StopFT(LPVPFTINFO ftInfo) = 0;


    virtual DWORD LastError(DWORD &errInfo) {
        DWORD e = m_error;
        m_error = VP_ERR_OK;
        errInfo = m_errInfo;
        return e;
    }

    virtual void SetError(DWORD error, DWORD errInfo = 0) = 0;

    virtual void FatalError(DWORD error, DWORD errInfo = 0) = 0;

    virtual void InitSessionInfo(LPCVPGUID guid,
                                 DWORD version,
                                 DWORD sesInfoSize,
                                 DWORD playerInfoSize) = 0;

    virtual void SetSessionId(LPCVPSESSIONID id) = 0;


    virtual void OnTimer() {}


public:
    DWORD m_error;
    DWORD m_errInfo;
    BOOL m_invalid;
};


// One in-flight/confirmed hole-punch pairing (client: the host; host: a joiner).
// Not a wire struct — local state only, driven from OnTimer.
struct PunchPeer {
    enum { IDLE = 0, WAIT_REP, PROBING, CONFIRMED };

    DWORD m_nonce;
    VPNETADDRESS m_pub;        // peer public candidate (dialable overlay)
    VPNETADDRESS m_priv;       // peer private candidate
    VPNETADDRESS m_confirmed;  // the address whose probe round-tripped
    VPSESSIONID m_sessionId;   // client role: target session (for REQ resends)
    int m_state;
    int m_tries;
    DWORD m_lastSend;
    DWORD m_lastAlive;

    void Reset() { memset(this, 0, sizeof(*this)); }
};

class CVpSession : public CSession, public CMsgDispatcher {
public:
    typedef CWSMap::EnumProc WSEnum;


    CVpSession(CTDLogger *log, CNetInterface *net, CPlayerMap *players, CWSMap *wsMap);

    virtual ~CVpSession();


    // Incoming message handling
    virtual BOOL OnUDataREQ(genericMsg *msg, CRemoteWS *ws);

    virtual BOOL OnUBDataREQ(genericMsg *msg, CRemoteWS *ws);

    virtual BOOL OnFtREQ(ftReqMsg *msg, CRemoteWS *ws);

    virtual BOOL OnFtACK(ftAckMsg *msg, CRemoteWS *ws);


    virtual CRemoteWS *MakeRemoteWS(CNetAddress *addr, CNetLink *safeLink, CNetLink *unsafeLink = NULL);

    virtual CPlayer *FindPlayer(VPPLAYERID pId, CWS *ws = NULL) const;

    // Host relay (spec 8): is our link to this player's workstation relayed
    // through the host rather than direct? Read-only; FALSE on the host and
    // whenever the gate is off.
    BOOL PeerIsRelayed(VPPLAYERID pId) const;

    virtual CRemotePlayer *AddRemotePlayer(plrInfoMsg *infoMsg, CRemoteWS *ws, BOOL doNotify);

    virtual CRemotePlayer *RemoveRemotePlayer(VPPLAYERID id, CRemoteWS *ws);

    virtual CLocalPlayer *MakeLocalPlayer(LPCSTR playerName,
                                          DWORD playerFlags);


    CNetLink *MakeSafeLink(CNetAddress *addr) {
        return m_net->MakeSafeLink(addr);
    }

    // --- NAT hole-punch plumbing shared by host/client/iserve roles ---
    // (docs/plans/nat-holepunch-iserve-feasibility.md P1; EN_NAT_PUNCH-gated at
    // the call sites in the role classes.)

    // Fire a datagram at a raw VPNETADDRESS from the session's dg socket
    // (m_broadcastLink) — the SAME socket that registers/enums, so every send
    // reuses the one NAT mapping the rendezvous exchanged (mapping is
    // per-socket; a throwaway socket would learn a different one).
    BOOL SendDgTo(LPCVPNETADDRESS to, LPVOID data, DWORD size);

    // Send NatPunchPING at both of a peer's candidates (private skipped when
    // absent or equal to public). Bumps tries/lastSend.
    void PunchFireProbes(PunchPeer &p);

    // Unconditional PONG reply to an incoming PING's observed source; also
    // reports whether the ping's nonce matched `p` (host reached us => the
    // pair works even before our own probe round-trips).
    BOOL PunchHandlePing(natPunchMsg *msg, CNetAddress *from, PunchPeer &p);

    // PONG receive: nonce match => record the observed source as the working
    // pair. Returns TRUE when this PONG confirmed (or refreshed) the peer.
    BOOL PunchHandlePong(natPunchMsg *msg, CNetAddress *from, PunchPeer &p);
    // Refresh a CONFIRMED pair's liveness from ANY datagram off that peer.
    void PunchNoteTraffic(PunchPeer &p, CNetAddress *from);


    virtual BOOL SendData(VPPLAYERID fromId, VPPLAYERID toId,
                          LPVOID data, DWORD dataSize, DWORD flags, LPVOID userData);

    virtual BOOL UpdateSessionInfo(LPVOID data) = 0;

    virtual BOOL GetSessionInfo(LPVPSESSIONINFO sesInfo);

    virtual BOOL CloseSession();

    virtual WORD PlayerCount();


    virtual void SetError(DWORD error, DWORD errInfo = 0);

    virtual void FatalError(DWORD error, DWORD errInfo = 0);

    virtual void Log(LPCSTR msg);

    virtual void InitSessionInfo(LPCVPGUID guid,
                                 DWORD version,
                                 DWORD sesInfoSize,
                                 DWORD playerInfoSize);

    virtual void SetSessionId(LPCVPSESSIONID id) {
        VPASSERT(m_info);
        m_info->data.sessionId = *id;
    }


    virtual BOOL KillPlayer(VPPLAYERID id);


    virtual BOOL KnockOutPlayer(VPPLAYERID id, plrInfoMsg *msg, CRemoteWS *ws);

    virtual BOOL SetVisibility(BOOL visibility) = 0;


    // Start File Transfer
    virtual BOOL StartFT(LPVPFTINFO ftInfo);

    // Accept incoming file transfer request
    virtual BOOL AcceptFT(LPVPFTINFO ftInfo);


    // Send a file  data block
    virtual BOOL SendBlock(LPVPFTINFO ftInfo,  // file transfer info
                           LPCVOID buf,
                           DWORD bufSize
    );

    // Receive a file data block
    virtual BOOL GetBlock(LPVPFTINFO ftInfo,    // file transfer info
                          LPVOID buf,
                          DWORD bufSize
    );

    // Terminate a File Transfer
    virtual BOOL StopFT(LPVPFTINFO ftInfo);


protected:


    virtual WORD NextMessageId();

    virtual BOOL GoodBroadcastOptions(DWORD flags) = 0;

    virtual void ShouldNotBeHere(LPCSTR aClass, LPCSTR aMethod, LPCSTR txt) = 0;

    static BOOL SendOnLink(CNetLink *link, LPVOID data);

    static BOOL SendAllPlayers(CWS *ws, LPVOID data);

    static BOOL SendToWs(CWS *ws, LPVOID data);

    // callback routines for various network events
    static void SafeDataHook(CNetLink *link, LPVOID ctx);

    static void UnsafeDataHook(CNetLink *link, LPVOID ctx);

    static void ConnectHook(CNetLink *link, LPVOID ctx);

    static void DisconnectHook(CNetLink *link, LPVOID ctx);

    static void AcceptHook(CNetLink *link, LPVOID ctx);


    virtual void OnSafeData(CNetLink *link);

    virtual void OnUnsafeData(CNetLink *link) = 0;

    virtual void OnConnect(CNetLink *link) = 0;

    virtual void OnDisconnect(CNetLink *link) = 0;

    virtual void OnAccept(CNetLink *link) = 0;

    virtual void HandleNetDown();

    virtual void ProcessSafeData(CNetLink *link, genericMsg *msg) = 0;

    struct exceptEnumInfo {
        CWS *exceptWs;
        WSEnum p;
        LPVOID data;
    };

    static BOOL ExceptEnumHelper(CWS *ws, LPVOID data);

    void ForAllWorkstations(WSEnum p, LPVOID data) {
        m_wsMap->Enum(p, data);
    }

    void ForAllWorkstationsExcept(CWS *ws, WSEnum p, LPVOID data);

    static const char FAR className[];
public:
    virtual BOOL InitNetwork(BOOL streamListen);

public:
    CTDLogger *m_log;
    CNetInterface *m_net;
    sesInfoMsg *m_info;     // session info data
    CPlayerMap *m_players;
    CWSMap *m_wsMap;    // Wokstation map
    CLocalWS *m_localWS;
    CNetLink *m_broadcastLink;
    CNetAddress *m_broadcastAddress;
    CNetAddress *m_registrationAddress;

    CMutex m_mutex;    // Mutual exclusion semaphore
    DWORD m_error;    // Last error code
    DWORD m_errInfo;  // additional error info
    WORD m_msgId;    // a message id for next generated message
    BOOL m_invalid;
    BOOL m_keepLog;

    // Lock/Unlock the session object
    class CSessLock {
    public:
        CSessLock(const CVpSession *s) : m_lock((CMutex &) s->m_mutex) {}

        CSessLock(const CVpSession &s) : m_lock((CMutex &) s.m_mutex) {}

    private:
        CMxLock m_lock;

    };


};


class CLocalPlayer : public CPlayer {
public:
    CLocalPlayer(plrInfoMsg *pInfo, CLocalWS *ws) : CPlayer(pInfo, ws) {
    }

    virtual BOOL IsRemote() const { return FALSE; }

    void FixId(VPPLAYERID id) {
        m_info->data.playerId = id;
    }

    virtual BOOL SendData(sendDataInfo &sdi) {
        return TRUE;
    }


};

class CRemotePlayer : public CPlayer {
public:
    CRemotePlayer(plrInfoMsg *pInfo, CRemoteWS *ws) : CPlayer(pInfo, ws) {
    }

    virtual BOOL IsRemote() const { return TRUE; }

    BOOL SendData(sendDataInfo &);


};

class CMsgNotification : public CNotification {
public:
    CMsgNotification(genericMsg *msg,
                     WPARAM notifyCode,
                     LPVOID data,
                     DWORD dataSize,
                     LPVOID userData = NULL)
            :
            CNotification(notifyCode, data, dataSize, userData), m_msg(msg) {
        m_msg->Ref();
    }

    void Complete() {
        m_msg->Unref();
        m_msg = NULL;
    }

    ~CMsgNotification() {
        if (m_msg)
            Complete();
    }

protected:
    genericMsg *m_msg;

};

class CPlrInfoNotification : public CMsgNotification {
public:
    CPlrInfoNotification(plrInfoMsg *info, WPARAM nCode, LPVOID userData)
            : CMsgNotification(info,
                               nCode,
                               &info->data,
                               vpPlayerInfoSize(&info->data),
                               userData) {
    }
};


class CPenumNotification : public CPlrInfoNotification {
public:
    CPenumNotification(plrInfoMsg *info,
                       DWORD errCode = VP_ERR_OK,
                       LPVOID userData = NULL)
            : CPlrInfoNotification(info,
                                   (WPARAM) ((errCode << 8) | VP_PLAYERENUM),
                                   userData) {
    }


};


class CJoinNotification : public CPlrInfoNotification {
public:
    CJoinNotification(CPlayer *p,
                      DWORD errCode = VP_ERR_OK,
                      LPVOID userData = NULL)
            : CPlrInfoNotification(p->m_info,
                                   (WPARAM) ((errCode << 8) | VP_JOIN),
                                   userData) {
    }


};


class CLocalJoin : public CJoinNotification {
public:
    CLocalJoin(CLocalPlayer *p, WORD msgId, LPVOID userData = NULL)
            : CJoinNotification(p, VP_ERR_OK, userData),
              m_player(p), m_msgId(msgId) {}


    CLocalPlayer *Player() const { return m_player; }

    WORD MsgId() const { return m_msgId; }


public:
    CLocalPlayer *m_player;
    WORD m_msgId;
};


class CLeaveNotification : public CJoinNotification {
public:
    CLeaveNotification(CPlayer *p, WORD errCode = 0, LPVOID userData = NULL) :
            CJoinNotification(p, errCode, userData), m_player(p) {
        SetResult(VP_LEAVE, errCode);
    }

    virtual void Complete() {
        CJoinNotification::Complete();
        delete m_player;
        m_player = NULL;
    }

    CPlayer *m_player;
};

class CDataNotification : public CMsgNotification {
public:
    CDataNotification(genericMsg *msg)
            : CMsgNotification(msg, VP_READDATA, msg->Contents(), msg->ContentSize()) {
        m_vpmsg.senderId = msg->hdr.msgFrom;
        m_vpmsg.toId = msg->hdr.msgTo;
#if VP_TIMESTAMP
        m_vpmsg.creationTime = msg->hdr.msgTime;
#endif
    }
};


class CSenumNotification : public CMsgNotification {
public:
    CSenumNotification(genericMsg *msg, LPVOID userData)
            : CMsgNotification(msg, VP_SESSIONENUM, msg->Contents(), msg->ContentSize(), userData) {
    }

};

class CServerDownNotification : public CMsgNotification {
public:
    CServerDownNotification(CRemoteWS *ws, LPVOID userData)
            : CMsgNotification(ws->Info(), VP_SERVERDOWN, ws->Info()->Contents(), ws->Info()->ContentSize(), userData) {
    }

};

class CFtNotification : public CMsgNotification {
public:

    CFtNotification(ftMsg *msg, LPVOID userData)
            : CMsgNotification(msg, VP_FTREQ, msg->Contents(), msg->ContentSize(), userData) {
    }

};


class CLocalSession : public CVpSession {
public:

    virtual BOOL IsRemote() const { return FALSE; }

    virtual BOOL UpdateSessionInfo(LPVOID data);

    virtual VPPLAYERID NewPlayerId();

    virtual CRemotePlayer *AddRemotePlayer(plrInfoMsg *infoMsg, CRemoteWS *ws);

    virtual BOOL OnJoinREQ(plrInfoMsg *msg, CRemoteWS *ws);

    virtual BOOL OnJoinREP(plrInfoMsg *msg, CRemoteWS *ws) {
        IMPOSSIBLE(OnJoinREP);
        return TRUE;
    }

    virtual BOOL OnJoinNAK(plrInfoMsg *msg, CRemoteWS *ws) {
        IMPOSSIBLE(OnJoinNAK);
        return TRUE;
    }

    virtual BOOL OnJoinADV(plrInfoMsg *msg, CRemoteWS *ws) {
        IMPOSSIBLE(OnJoinADV);
        return TRUE;
    }


    virtual BOOL OnLeaveREQ(plrInfoMsg *msg, CRemoteWS *ws);

    // Host relay (spec 4a): forwards a unicast whose destination lives on
    // another workstation; delegates to the base (local delivery) otherwise.
    virtual BOOL OnUDataREQ(genericMsg *msg, CRemoteWS *ws);

    virtual BOOL OnUBDataREQ(genericMsg *msg, CRemoteWS *ws);

    virtual BOOL OnPenumREQ(genericMsg *msg, CRemoteWS *ws);

    virtual BOOL OnPenumREP(plrInfoMsg *msg, CRemoteWS *ws) {
        IMPOSSIBLE(OnPenumREP);
        return TRUE;
    }


    virtual BOOL OnSenumREQ(genericMsg *msg, CRemoteWS *ws);

    virtual BOOL OnSenumREP(sesInfoMsg *msg, CRemoteWS *ws) {
        return TRUE;
    }

    virtual BOOL AddLocalPlayer(LPCSTR playerName,
                                DWORD playerFlags,
                                LPVOID userData, LPVPPLAYERID pId = NULL);


    CLocalSession(CTDLogger *log, CNetInterface *net, CPlayerMap *players, CWSMap *wsMap);

    virtual BOOL SetVisibility(BOOL visibility);


protected:
    virtual BOOL BroadcastSessionData();

    virtual void OnUnsafeData(CNetLink *link);

    virtual void OnConnect(CNetLink *link);

    virtual void OnDisconnect(CNetLink *link);

    virtual void OnAccept(CNetLink *link);

    virtual void ProcessSafeData(CNetLink *link, genericMsg *msg);


public:

    BOOL IsVisible() const { return m_visible; }

    virtual void OnTimer();

    // NAT hole-punch, host role: a NatPunchFWD pushed by iserve through our
    // warm registration mapping announces a joiner; we probe its candidates.
    void OnNatPunchFWD(natPunchMsg *msg, CNetAddress *from);

protected:
    void DrivePunch();     // retries/keepalive-expiry, from OnTimer

    enum { MAX_PUNCH_PEERS = 4 };
    PunchPeer m_punch[MAX_PUNCH_PEERS];

    static const char FAR className[];
    VPPLAYERID m_nextPlayerId;
    BOOL m_visible;
    BOOL m_gotsEnumREQ;
    DWORD m_lastRegTime;

};


class CRemoteSession : public CVpSession {
public:

    virtual BOOL IsRemote() const { return TRUE; }


    virtual BOOL SendData(VPPLAYERID fromId, VPPLAYERID toId,
                          LPVOID data, DWORD dataSize, DWORD flags, LPVOID userData);

    BOOL UpdateSessionInfo(LPVOID data);

    virtual BOOL OnJoinREQ(plrInfoMsg *msg, CRemoteWS *ws) {
        IMPOSSIBLE(OnJoinREQ);
        return TRUE;
    }

    virtual BOOL OnJoinREP(plrInfoMsg *msg, CRemoteWS *ws);

    virtual BOOL OnJoinNAK(plrInfoMsg *msg, CRemoteWS *ws);

    virtual BOOL OnJoinADV(plrInfoMsg *msg, CRemoteWS *ws);

    virtual BOOL OnLeaveREQ(plrInfoMsg *msg, CRemoteWS *ws);

#if 0
    virtual BOOL OnUDataREQ(genericMsg* msg, CRemoteWS* ws);
    virtual BOOL OnUBDataREQ(genericMsg* msg, CRemoteWS* ws);
#endif

    virtual BOOL OnPenumREQ(genericMsg *msg, CRemoteWS *ws) {
        IMPOSSIBLE(OnPenumREQ);
        return TRUE;
    }

    virtual BOOL OnPenumREP(plrInfoMsg *msg, CRemoteWS *ws);

    virtual BOOL OnSenumREQ(genericMsg *msg, CRemoteWS *ws) {
        IMPOSSIBLE(OnSenumREQ);
        return TRUE;
    }

    virtual BOOL OnSenumREP(sesInfoMsg *msg, CRemoteWS *ws);


    virtual BOOL AddLocalPlayer(LPCSTR playerName,
                                DWORD playerFlags,
                                LPVOID userData, LPVPPLAYERID pId = NULL);


    virtual CRemoteWS *RegisterPlayerWS(plrInfoMsg *msg);

    // Host relay (spec 8): kick the lazy direct dial to a peer ONCE, so the
    // lobby learns direct-vs-relayed before the first real send. No payload is
    // sent - this is exactly the MakeSafeLink the first unicast would do, just
    // earlier. No-op unless the gate is on. Returns TRUE if a dial was started.
    BOOL ProbePeerLink(VPPLAYERID pId);

    CRemoteSession(CTDLogger *log,
                   CNetInterface *net,
                   CPlayerMap *players,
                   CWSMap *wsMap,
                   DWORD maxAge = VP_SERVER_AGE_PERIOD);

    virtual BOOL ConnectToServer(LPCVPNETADDRESS addr, LPVOID userData);

    virtual BOOL LookForServer(LPVOID data);

    virtual BOOL SetVisibility(BOOL visibility);

    virtual void OnTimer();    // punch retries/keepalive (no-op when punch idle)

    // NAT hole-punch, client role: ask iserve to rendezvous us with the (NAT'd)
    // host of `sessionId`, then probe the candidates from NatPunchREP. Runs in
    // parallel with the TCP dial; EN_NAT_PUNCH-gated.
    void StartNatPunch(LPCVPNETADDRESS sessionId);

    void OnNatPunchREP(natPunchMsg *msg, CNetAddress *from);

    // iserve role only (CRegisterySession overrides); a game client ignores it.
    virtual void OnNatPunchREQ(natPunchMsg *msg, CNetAddress *from) {}

    // Registration observed-address stamp (P0.1). Called from the shared
    // OnUnsafeData SenumREP case with the datagram's OBSERVED source; only the
    // registration server (CRegisterySession) does anything with it.
    virtual void StampRegistration(sesInfoMsg *msg, CNetAddress *observed) {}

protected:
    void DrivePunch();     // REQ resends / probe retries / keepalive, from OnTimer

    PunchPeer m_punch;

    // Re-punch state. PunchPeer::Reset() memsets the whole struct, so the
    // session id a re-arm needs cannot live inside it. Kept here so DrivePunch
    // can re-arm a dead pair for the life of the session (board ruling
    // 2026-08-23): a pair lost to a transient outage was previously permanent,
    // because StartNatPunch was reachable only from ConnectToServer.
    VPSESSIONID m_punchSessionId;
    BOOL        m_punchWanted;    // a pair has been asked for at least once
    DWORD       m_punchLastArm;   // last StartNatPunch, for the 30s re-arm ladder

    virtual void OnUnsafeData(CNetLink *link);

    virtual void OnConnect(CNetLink *link);

    virtual void OnDisconnect(CNetLink *link);

    virtual void OnAccept(CNetLink *link);

    virtual void ProcessSafeData(CNetLink *link, genericMsg *msg);

    static BOOL CheckForAge(CWS *ws, LPVOID userData);

    // OnTimer bounded-dial watchdog, PEER side: collects every remote WS whose
    // direct dial has been pending past the deadline (see CRemoteWS::m_dialStart).
    static BOOL CheckDialDeadline(CWS *ws, LPVOID userData);

    virtual void AgeServerList();

public:
    CRemoteWS *m_serverWS;   // game server WS

    // TCP-enum (phase-3): a one-shot TCP enum probe per enum session (m_tcpEnumTried),
    // fired in parallel with the UDP query for a directed reg server (happy-eyeballs) so
    // discovery survives a UDP-blocked path. TryTcpEnumFallback() does the connect+send.
    BOOL m_tcpEnumTried;
    void TryTcpEnumFallback();
    CLocalJoin *m_pendingJoin;
    BOOL m_connected;

    // P0.1 candidate fallback (dial-both): a stamped session carries two dialable
    // candidates (observed PUBLIC + advertised PRIVATE). ConnectToServer dials the primary
    // and stashes the OTHER here; if the primary's connect drops BEFORE the join completes
    // (m_connected still FALSE — the async EINPROGRESS failure that lands in OnDisconnect),
    // OnDisconnect re-dials this alternate ONCE. Cleared on a successful connect so a real
    // mid-game link drop never re-dials. Inert unless a stamp offered two distinct candidates
    // -> only ever engages on a path that otherwise dead-ends at the menu (cannot regress a
    // working single-candidate join).
    BOOL         m_hasAltServer;
    VPNETADDRESS m_altServerAddr;
    LPVOID       m_altServerUserData;
    // Re-dial is DEFERRED to the next OnTimer pass, never called inline from
    // OnDisconnect (the pump may still hold the dropped link — re-entrant
    // ConnectToServer there risks UAF / iterator invalidation; newwin review).
    BOOL         m_redialPending;
    // When the pending pre-join server dial started (0 = none). A candidate
    // behind a DROP firewall/NAT never errors the socket inside the app's join
    // window (OS SYN budget 21-127s vs the UI's 8s guard), so without an
    // app-level deadline the OnDisconnect dial-both fallback is unreachable.
    // OnTimer expires the dial and routes it through OnDisconnect.
    DWORD        m_dialStart;

    BOOL m_initialJoin;
    LPVOID m_serverEnumData;
    DWORD m_maxServerAge;

protected:
    static const char FAR className[];

};


class CRegisterySession : public CRemoteSession {

public:

    CRegisterySession(CTDLogger *log,
                      CNetInterface *net,
                      CPlayerMap *players,
                      CWSMap *wsMap,
                      DWORD maxAge = VP_REGISTERY_AGE_PERIOD);

    virtual BOOL OnSenumREQ(genericMsg *msg, CRemoteWS *ws);

    virtual BOOL LookForServer(LPVOID data);

    // P0.1: stamp the host's observed public UDP endpoint into the stored
    // sessionId tail at registration time (vpnatcand.h layout).
    virtual void StampRegistration(sesInfoMsg *msg, CNetAddress *observed);

    // P1: stateless rendezvous — forward the joiner's candidates to the host
    // through its warm registration mapping, reply the host's to the joiner.
    virtual void OnNatPunchREQ(natPunchMsg *msg, CNetAddress *from);

    // TCP-enum (phase-1): dispatch a client's SenumREQ that arrives over a TCP
    // connection (vs the UDP datagram path). The base CRemoteSession::ProcessSafeData
    // has no SenumREQ case (a game client never RECEIVES a query — its OnSenumREQ is
    // IMPOSSIBLE()), so the reg server overrides it here; all other kinds delegate to
    // the base. Additive — inert until the reg server gets a TCP-enum listener.
    virtual void ProcessSafeData(CNetLink *link, genericMsg *msg);

    BOOL IsServerMode() const { return TRUE; }

protected:
    static int ReplyServerInfo(CWS *ws, LPVOID data);

public:
    DWORD m_queryAge;

protected:
    static const char FAR className[];

};


#endif
