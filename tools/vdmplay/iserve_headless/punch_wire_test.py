#!/usr/bin/env python3
# P1 NAT hole-punch rendezvous — wire-protocol test against a live iserve.
#
# Simulates the two peers a punch-enabled joiner needs iserve to bridge:
#   HOST  socket: registers a session (SenumREP) with dg source X -> iserve
#                 stamps X as the host's observed public candidate (P0.1)
#   JOINER socket: sends NatPunchREQ for that session
# Then asserts iserve does the STATELESS rendezvous:
#   -> NatPunchFWD arrives at the HOST socket (joiner's candidates)
#   -> NatPunchREP arrives at the JOINER socket (host's candidates)
# with the correct observed addresses on both. This exercises the novel P1
# server logic (CRegisterySession::OnNatPunchREQ + StampRegistration) on the
# wire; the peer PING/PONG legs are symmetric engine code covered by the build.
#
# All sockets are loopback, so "observed" addresses are 127.0.0.1:<port> — which
# is exactly what makes this a clean single-box test (the game join path can't
# exercise punch on one box because same-IP => same-NAT => private dial).

import socket, struct, sys, time

ISERVE = ('127.0.0.1', 1808)

# --- wire structs (pack(1), VP_TIMESTAMP=0) ---
# VPMSGHDR: WORD msgSize; BYTE msgKind; BYTE msgFlags; WORD from; WORD to; WORD id  = 10 bytes
def hdr(kind, size):
    return struct.pack('<HBBHHH', size, kind, 0, 0, 0xFFFF, 0)

MK = {c: ord(c) for c in 'sSHWhYy'}   # SenumREP='s' SenumREQ='S' REQ='H' FWD='W' REP='h' PING='Y' PONG='y'

# VPSESSIONINFO: VPGUID gameId[32]; VPSESSIONID sessionId[28]; DWORD version,
#   playerCount, sessionFlags, playerDataSize, dataSize; char sessionName[?]
# We only need a well-formed SenumREP whose sessionId prefix keys the session.
def make_sessionid(ip, stream, dg):
    b = bytearray(28)
    b[0:4] = socket.inet_aton(ip)
    b[4:6] = struct.pack('!H', stream)   # network order stream port
    b[6:8] = struct.pack('!H', dg)       # network order dg port
    return bytes(b)

PSEUDOSIZE = 32   # VP_PSEUDOSIZE guess for sessionName tail; iserve stores by size
def make_senumrep(sessionid, name=b'PunchTest'):
    guid = b'ENnat'.ljust(32, b'\0')
    version = 1; playerCount = 1; sessionFlags = 0; playerDataSize = 32; dataSize = 0
    body = guid + sessionid + struct.pack('<IIIII', version, playerCount, sessionFlags, playerDataSize, dataSize)
    body += name.ljust(PSEUDOSIZE, b'\0')
    return hdr(MK['s'], 10 + len(body)) + body

def make_natpunch(kind, sessionid, pubcand=None, privcand=None, nonce=0, flags=0):
    pubcand = pubcand or bytes(28)
    privcand = privcand or bytes(28)
    body = sessionid + pubcand + privcand + struct.pack('<II', nonce, flags)
    assert len(body) == 92, len(body)
    return hdr(MK[kind], 10 + len(body)) + body

def parse_natpunch(data):
    size, kind, flags, mf, mt, mid = struct.unpack('<HBBHHH', data[:10])
    body = data[10:]
    sessionid = body[0:28]; pub = body[28:56]; priv = body[56:84]
    nonce, pflags = struct.unpack('<II', body[84:92])
    return chr(kind), sessionid, pub, priv, nonce, pflags

def ipport(b28):
    ip = socket.inet_ntoa(b28[0:4]); stream = struct.unpack('!H', b28[4:6])[0]; dg = struct.unpack('!H', b28[6:8])[0]
    return f'{ip} stream={stream} dg={dg}'

def natcand_tail(b28):
    if b28[16:18] != b'NP': return '(no stamp)'
    ip = socket.inet_ntoa(b28[20:24]); port = struct.unpack('!H', b28[24:26])[0]
    return f'stamp pub={ip}:{port} flags=0x{b28[19]:02x}'

fails = 0
def check(cond, msg):
    global fails
    print(('PASS' if cond else 'FAIL') + ': ' + msg)
    if not cond: fails += 1

# host socket + joiner socket
host = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); host.bind(('127.0.0.1', 0)); host.settimeout(2)
join = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); join.bind(('127.0.0.1', 0)); join.settimeout(2)
host_port = host.getsockname()[1]; join_port = join.getsockname()[1]
print(f'host socket 127.0.0.1:{host_port}  joiner socket 127.0.0.1:{join_port}')

# The host's sessionId advertises its private (station) address. Use a distinct
# fake "private" IP so we can tell private vs observed(public) apart in the REP.
HOST_PRIV_IP = '10.9.9.9'; HOST_STREAM = 2347; HOST_DG = 2346
sid = make_sessionid(HOST_PRIV_IP, HOST_STREAM, HOST_DG)

# 1) HOST registers. iserve should stamp the OBSERVED source (127.0.0.1:host_port).
host.sendto(make_senumrep(sid), ISERVE)
time.sleep(0.3)

# 2) JOINER enumerates -> SenumREP back carries the stamped tail (P0.1).
join.sendto(hdr(MK['S'], 10), ISERVE)
try:
    data, _ = join.recvfrom(2048)
    kind = chr(data[10 if False else 2])
    # sessionId sits after 10-hdr + 32-guid in the SenumREP
    got_sid = data[10+32:10+32+28]
    print(f'  enum reply kind={kind!r} sessionId: {ipport(got_sid)} {natcand_tail(got_sid)}')
    check(got_sid[16:18] == b'NP', 'enum SenumREP carries observed-address stamp (P0.1)')
    stamp_ip = socket.inet_ntoa(got_sid[20:24])
    check(stamp_ip == '127.0.0.1', f'stamped public IP is the observed source (got {stamp_ip})')
    stamp_port = struct.unpack('!H', got_sid[24:26])[0]
    check(stamp_port == host_port, f'stamped public dg port == host observed port (got {stamp_port} want {host_port})')
except socket.timeout:
    check(False, 'enum reply received');

# 3) JOINER sends NatPunchREQ. Its self-reported private candidate carries a
#    made-up private IP + a claimed stream port so we can see it echoed.
JOIN_PRIV = make_sessionid('10.1.2.3', 4444, join_port)
nonce = 0xABCD1234
join.sendto(make_natpunch('H', sid, privcand=JOIN_PRIV, nonce=nonce), ISERVE)

# 4a) HOST should receive a NatPunchFWD with the joiner's candidates.
try:
    data, src = host.recvfrom(2048)
    k, s2, pub, priv, n, pf = parse_natpunch(data)
    print(f'  HOST <- {k!r} from {src}: joinerPub={ipport(pub)} joinerPriv={ipport(priv)} nonce=0x{n:08x} flags=0x{pf:x}')
    check(k == 'W', 'host received NatPunchFWD')
    check(n == nonce, 'FWD nonce matches REQ')
    check(socket.inet_ntoa(pub[0:4]) == '127.0.0.1', 'FWD joiner public IP = observed loopback')
    check(struct.unpack('!H', pub[6:8])[0] == join_port, 'FWD joiner public dg port = observed joiner port')
    check(struct.unpack('!H', pub[4:6])[0] == 4444, 'FWD joiner claimed stream port preserved')
except socket.timeout:
    check(False, 'host received NatPunchFWD (timed out)')

# 4b) JOINER should receive a NatPunchREP with the host's candidates.
try:
    data, src = join.recvfrom(2048)
    k, s2, pub, priv, n, pf = parse_natpunch(data)
    print(f'  JOINER <- {k!r} from {src}: hostPub={ipport(pub)} hostPriv={ipport(priv)} nonce=0x{n:08x} flags=0x{pf:x}')
    check(k == 'h', 'joiner received NatPunchREP')
    check(n == nonce, 'REP nonce matches REQ')
    check(socket.inet_ntoa(pub[0:4]) == '127.0.0.1', 'REP host public IP = observed loopback')
    check(struct.unpack('!H', pub[4:6])[0] == HOST_STREAM, 'REP host public candidate uses claimed stream port 2347')
    check(struct.unpack('!H', pub[6:8])[0] == host_port, 'REP host public dg port = host observed port')
    check(socket.inet_ntoa(priv[0:4]) == HOST_PRIV_IP, 'REP host private candidate = advertised station IP')
    # same public IP (both loopback) => SAMENAT hint set
    check((pf & 0x01) != 0, 'SAMENAT flag set (joiner & host share observed public IP)')
except socket.timeout:
    check(False, 'joiner received NatPunchREP (timed out)')

print()
print('RESULT:', 'ALL PASS' if fails == 0 else f'{fails} FAILURE(S)')
sys.exit(1 if fails else 0)
