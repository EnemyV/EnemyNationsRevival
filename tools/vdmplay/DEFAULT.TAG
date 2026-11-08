	TAGS	UNSORTED
__VPFRMWK_H__	vpengine.h	30	#define __VPFRMWK_H__
CSession	vpengine.h	51	class CSession;
CPlayer	vpengine.h	67	class CPlayer;
CNetLink	vpengine.h	82	class CNetLink;
CNetAddress	vpengine.h	98	class CNetAddress;
CNetInterface	vpengine.h	117	class CNetInterface;
CWS	vpengine.h	138	class CWS;
CLocalWS	vpengine.h	149	class CLocalWS;
CRemotePlayer	vpengine.h	165	class CRemotePlayer;
CLocalPlayer	vpengine.h	186	class CLocalPlayer;
IMPOSSIBLE	vpengine.h	209	#define IMPOSSIBLE(x) ShouldNotBeHere(className, #x, NULL)
IMPLEMENT_CLASSNAME	vpengine.h	268	#define IMPLEMENT_CLASSNAME(C) const char C::className[] = #C
sendDataInfo	vpengine.h	2384	struct sendDataInfo 
__TCPNET_H__	tcpnet.h	29	#define __TCPNET_H__
tcpaddress_s	tcpnet.h	1199	struct  tcpaddress_s 
__VPNET_H__	vpnet.h	28	#define __VPNET_H__
CNetAddress	vpnet.h	47	class CNetAddress;
__VDMPLAY_H__	vdmplay.h	30	#define __VDMPLAY_H__
VPAPI	vdmplay.h	70	#define VPAPI __declspec(dllimport)
VPAPI	vdmplay.h	112	#define VPAPI __declspec(dllexport)
WM_VPNOTIFY	vdmplay.h	156	#define WM_VPNOTIFY (WM_USER+1000)
VP_PSEUDOSIZE	vdmplay.h	191	#define VP_PSEUDOSIZE 2
VPGETNOTIFICATION	vdmplay.h	707	#define VPGETNOTIFICATION(wParam) LOBYTE(wParam)
VPGETERRORCODE	vdmplay.h	756	#define VPGETERRORCODE(wParam) HIBYTE(wParam)
MAKEWPARAM	vdmplay.h	822	#define MAKEWPARAM(h, l) (((h)<<8) | ((l)))
VPHANDLE	vdmplay.h	884	typedef void FAR* VPHANDLE;
VPSESSIONHANDLE	vdmplay.h	911	typedef VPHANDLE VPSESSIONHANDLE;
VPPLAYERID	vdmplay.h	945	typedef WORD    VPPLAYERID;
LPVPPLAYERID	vdmplay.h	981	typedef VPPLAYERID FAR* LPVPPLAYERID;
VP_LOCALMACHINE	vdmplay.h	1040	const VPPLAYERID  VP_LOCALMACHINE = 0;  // Local VDMPLAY Manager 
VP_SESSIONSERVER	vdmplay.h	1106	const VPPLAYERID  VP_SESSIONSERVER = 1; 
VP_FIRSTPLAYER	vdmplay.h	1147	const VPPLAYERID  VP_FIRSTPLAYER = 2;
VP_ALLPLAYERS	vdmplay.h	1185	const VPPLAYERID  VP_ALLPLAYERS = (VPPLAYERID) ~0u;
VPMsgHdr	vdmplay.h	1235	typedef struct VPMsgHdr
VPMSGHDR	vdmplay.h	1383	} VPMSGHDR,  FAR* LPVPMSGHDR;
LPVPMSGHDR	vdmplay.h	1399	} VPMSGHDR,  FAR* LPVPMSGHDR;
LPCVPMSGHDR	vdmplay.h	1442	typedef CONST VPMSGHDR FAR *  LPCVPMSGHDR;
VPNetAddress	vdmplay.h	1472	typedef struct VPNetAddress
VPNETADDRESS	vdmplay.h	1517	} VPNETADDRESS, FAR* LPVPNETADDRESS;
LPVPNETADDRESS	vdmplay.h	1536	} VPNETADDRESS, FAR* LPVPNETADDRESS;
LPCVPNETADDRESS	vdmplay.h	1586	typedef CONST VPNETADDRESS FAR * LPCVPNETADDRESS;
VPGUID	vdmplay.h	1619	typedef struct VPGUID
VPGUID	vdmplay.h	1645	} VPGUID, FAR* LPVPGUID;
LPVPGUID	vdmplay.h	1658	} VPGUID, FAR* LPVPGUID;
LPCVPGUID	vdmplay.h	1703	typedef CONST VPGUID  FAR * CONST LPCVPGUID;
VPSESSIONID	vdmplay.h	1736	typedef VPNETADDRESS VPSESSIONID, FAR *LPVPSESSIONID;
LPVPSESSIONID	vdmplay.h	1754	typedef VPNETADDRESS VPSESSIONID, FAR *LPVPSESSIONID;
LPCVPSESSIONID	vdmplay.h	1800	typedef CONST VPSESSIONID FAR* LPCVPSESSIONID;
VPSessionInfo	vdmplay.h	1832	typedef struct VPSessionInfo
VPSESSIONINFO	vdmplay.h	2173	} VPSESSIONINFO, FAR* LPVPSESSIONINFO;
LPVPSESSIONINFO	vdmplay.h	2193	} VPSESSIONINFO, FAR* LPVPSESSIONINFO;
LPCVPSESSIONINFO	vdmplay.h	2245	typedef  CONST VPSESSIONINFO FAR* LPCVPSESSIONINFO; 
VPPlayerInfo	vdmplay.h	2280	typedef struct VPPlayerInfo
VPPLAYERINFO	vdmplay.h	2522	} VPPLAYERINFO, FAR* LPVPPLAYERINFO;
LPVPPLAYERINFO	vdmplay.h	2541	} VPPLAYERINFO, FAR* LPVPPLAYERINFO;
LPCVPPLAYERINFO	vdmplay.h	2590	typedef CONST VPPLAYERINFO FAR* LPCVPPLAYERINFO;
VPPlayerEnumInfo	vdmplay.h	2651	typedef struct VPPlayerEnumInfo
VPPENUMINFO	vdmplay.h	2811	} VPPENUMINFO, FAR* LPVPENUMINFO;
LPVPENUMINFO	vdmplay.h	2829	} VPPENUMINFO, FAR* LPVPENUMINFO;
LPCVPPENUMINFO	vdmplay.h	2876	typedef CONST VPPENUMINFO FAR* LPCVPPENUMINFO;
VPMessage	vdmplay.h	2909	typedef struct VPMessage
VPMESSAGE	vdmplay.h	3396	} VPMESSAGE, FAR* LPVPMESSAGE;
LPVPMESSAGE	vdmplay.h	3412	} VPMESSAGE, FAR* LPVPMESSAGE;
LPCVPMESSAGE	vdmplay.h	3455	typedef CONST VPMESSAGE FAR* LPCVPMESSAGE;
VP_MAXPLAYERDATA	vdmplay.h	3484	const DWORD VP_MAXPLAYERDATA = 256;
VP_MAXSESSIONDATA	vdmplay.h	3520	const DWORD VP_MAXSESSIONDATA = 256;
VP_MAXSENDDATA	vdmplay.h	3557	const DWORD VP_MAXSENDDATA  = 500;
vpPlayerInfoSize	vdmplay.h	6467	inline DWORD vpPlayerInfoSize(LPCVPPLAYERINFO pInfo) 
vpSessionInfoSize	vdmplay.h	6596	inline DWORD vpSessionInfoSize(LPCVPSESSIONINFO sInfo) 
__STDAFX_H__	stdafx.h	29	#define __STDAFX_H__
WIN32_LEAN_AND_MEAN	stdafx.h	85	#define WIN32_LEAN_AND_MEAN
CONST	stdafx.h	208	#define CONST const
IN	stdafx.h	236	#define IN
OUT	stdafx.h	247	#define OUT
INOUT	stdafx.h	259	#define INOUT
ASSERT	stdafx.h	407	#define ASSERT assert
_fmemcpy	stdafx.h	544	#define _fmemcpy memcpy
AFX_API_EXPORT	stdafx.h	611	#define AFX_API_EXPORT  __declspec (dllexport)
__VPLIST_H__	vplist.h	29	#define __VPLIST_H__
Append	vplist.h	1552	inline void CVPList::Append(CVPLink *newOne)
AppendAfter	vplist.h	1711	inline void CVPList::AppendAfter(CVPLink *newOne, CVPLink *oldOne)
Insert	vplist.h	1914	inline void CVPList::Insert(CVPLink *newOne)
InsertBefore	vplist.h	2077	inline void CVPList::InsertBefore(CVPLink *newOne, CVPLink *oldOne)
Remove	vplist.h	2238	inline void CVPList::Remove(CVPLink *elem)
count	vplist.h	2435	inline CVPList::CVPList() : first(0), last(0), count(0)
VPList	vplist.h	2458	#define VPList(type) name2(VPList,type)
VPListdeclare	vplist.h	2498	#define VPListdeclare(type) \
_g_ListIterator_	vplist.h	3133	#define _g_ListIterator_(etype, ltype, base) \
VPListIterator	vplist.h	3396	#define VPListIterator(ltype, etype) name3(VPIter, ltype, etype)
VPListIteratordeclare2	vplist.h	3461	#define VPListIteratordeclare2(ltype, etype) _g_ListIterator_(etype, ltype, CVPLIter)
__GENERIC_H	generic.h	95	#define __GENERIC_H	1
name2	generic.h	152	#define name2(n1,n2)		n1 ## n2
name3	generic.h	183	#define name3(n1,n2,n3)		n1 ## n2 ## n3
name4	generic.h	223	#define name4(n1,n2,n3,n4)	n1 ## n2 ## n3 ## n4
GPT	generic.h	278	typedef int (*GPT)	(int,char *);
set_handler	generic.h	344	#define set_handler(generic,type,x)	set_##type##generic##_handler(x)
errorhandler	generic.h	413	#define errorhandler(generic,type)	type##generic##handler
callerror	generic.h	471	#define callerror(generic,type,a,b)	(*errorhandler(generic,type))(a,b)
declare	generic.h	543	#define declare(a,type)			a##declare(type)
implement	generic.h	586	#define implement(a,type)		a##implement(type)
declare2	generic.h	632	#define declare2(a,type1,type2)		a##declare2(type1,type2)
implement2	generic.h	690	#define implement2(a,type1,type2)	a##implement2(type1,type2)
__VPUTIL_H__	vputil.h	29	#define __VPUTIL_H__
VPASSERT	vputil.h	751	#define VPASSERT(x) ASSERT(x)
ASSERT	vputil.h	796	#define ASSERT(x)
pInfo	gnsess.h	2855	CPlayer::CPlayer(LPVPPLAYERINFO pInfo, CWS* ws) : m_ws(ws), m_info(NULL)
m_info	gnsess.h	2883	CPlayer::CPlayer(LPVPPLAYERINFO pInfo, CWS* ws) : m_ws(ws), m_info(NULL)
CPlayer	gnsess.h	3088	CPlayer::~CPlayer()
ExceptEnumHelper	gnsess.h	7447	BOOL CSession::ExceptEnumHelper(CWS* ws, LPVOID data)
ForAllWorkstationsExcept	gnsess.h	7615	void CSession::ForAllWorkstationsExcept(CWS* ws, WSEnum p, LPVOID data)
SendOnLink	gnsess.h	7817	BOOL CSession::SendOnLink(CNetLink* link, LPVOID data)
FindPlayer	gnsess.h	8033	CPlayer* CSession::FindPlayer(VPPLAYERID pId, CWS* ws)
RemoveRemotePlayer	gnsess.h	8217	CRemotePlayer* CSession::RemoveRemotePlayer(LPPLAYERINFO pInfo, CRemoteWS* ws)
AddRemotePlayer	gnsess.h	8705	CRemotePlayer* CSession::AddRemotePlayer(LPPLAYERINFO pInfo, CRemoteWS* ws)
SendAllPlayers	gnsess.h	9123	BOOL CSession::SendAllPlayers(CWS* ws, LPVOID data)
SendToWs	gnsess.h	9404	BOOL CSession::SendToWs(CWS* ws, LPVOID data)
OnLeaveSession	gnsess.h	9902	BOOL CLocalSession::OnLeaveSession(plrInfoMsg* msg, CRemoteWS* ws)
OnJoinSession	gnsess.h	10364	BOOL CLocalSession::OnJoinSession(plrInfoMsg* msg, CRemoteWS* ws)
OnLeaveSession	gnsess.h	11420	BOOL CRemoteSession::OnLeaveSession(plrInfoMsg* msg, CRemoteWS* ws)
OnJoinSession	gnsess.h	11617	BOOL CRemoteSession::OnJoinSession(plrInfoMsg* msg, CRemoteWS* ws)
UpdateSessionData	gnsess.h	11785	BOOL CLocalSession::UpdateSessionData(LPVOID data)
BroadcastSessionData	gnsess.h	11942	void CLocalSession::BroadcastSessionData()
SendData	gnsess.h	12908	BOOL CRemotePlayer::SendData(sendDataInfo& sid)
UpdateSessionData	gnsess.h	13176	BOOL CRemoteSession::UpdateSessionData(LPVOID data)
SendData	gnsess.h	13283	BOOL CSession::SendData(VPPLAYERID toId,
__GAMENET_H__	gamenet.h	30	#define __GAMENET_H__
GN_NAMESIZE	gamenet.h	53	#define GN_NAMESIZE 32
WM_GNNOTIFY	gamenet.h	76	#define WM_GNNOTIFY (WM_USER+1000)
GN_PSEUDOSIZE	gamenet.h	111	#define GN_PSEUDOSIZE 0
GNGETNOTIFICATION	gamenet.h	476	#define GNGETNOTIFICATION(wParam) LOBYTE(wParam)
GNGETERRORCODE	gamenet.h	525	#define GNGETERRORCODE(wParam) HIBYTE(wParam)
GNHANDLE	gamenet.h	582	typedef void FAR* GNHANDLE;
GNSESSIONHANDLE	gamenet.h	609	typedef GNHANDLE GNSESSIONHANDLE;
GNPLAYERID	gamenet.h	644	typedef DWORD    GNPLAYERID;
GN_LOCALMACHINE	gamenet.h	701	const GNPLAYERID  GN_LOCALMACHINE = 0;  // Local GameNet Manager 
GN_SESSIONSERVER	gamenet.h	767	const GNPLAYERID  GN_SESSIONSERVER = 1; 
GN_ALLPLAYERS	gamenet.h	808	const GNPLAYERID  GN_ALLPLAYERS = ~0L,
gnMsgHdr	gamenet.h	845	typedef struct gnMsgHdr
GNMSGHDR	gamenet.h	926	} GNMSGHDR, FAR* LPGNMSGHDR, CONST FAR* LPCGNMSGHDR;
LPGNMSGHDR	gamenet.h	941	} GNMSGHDR, FAR* LPGNMSGHDR, CONST FAR* LPCGNMSGHDR;
LPCGNMSGHDR	gamenet.h	964	} GNMSGHDR, FAR* LPGNMSGHDR, CONST FAR* LPCGNMSGHDR;
gnNetAddress	gamenet.h	994	typedef struct gnNetAddress
GNNETADDRESS	gamenet.h	1059	} GNNETADDRESS, FAR* LPGNNETADDRESS, CONST FAR* LPCGNETADDRESS;
LPGNNETADDRESS	gamenet.h	1078	} GNNETADDRESS, FAR* LPGNNETADDRESS, CONST FAR* LPCGNETADDRESS;
LPCGNETADDRESS	gamenet.h	1105	} GNNETADDRESS, FAR* LPGNNETADDRESS, CONST FAR* LPCGNETADDRESS;
gnGUID	gamenet.h	1137	typedef struct gnGUID
GNGUID	gamenet.h	1163	} GNGUID, FAR* LPGNGUID;
LPGNGUID	gamenet.h	1176	} GNGUID, FAR* LPGNGUID;
GNSESSIONID	gamenet.h	1208	typedef GNNETADDRESS GNSESSIONID, FAR *LPGNSESSIONID;
LPGNSESSIONID	gamenet.h	1226	typedef GNNETADDRESS GNSESSIONID, FAR *LPGNSESSIONID;
gnSessionInfo	gamenet.h	1257	typedef struct gnSessionInfo
GNSESSIONINFO	gamenet.h	1506	} GNSESSIONINFO, FAR* LPGNSESSIONINFO , CONST  FAR* LPCGNSESSIONINFO; 
LPGNSESSIONINFO	gamenet.h	1526	} GNSESSIONINFO, FAR* LPGNSESSIONINFO , CONST  FAR* LPCGNSESSIONINFO; 
LPCGNSESSIONINFO	gamenet.h	1556	} GNSESSIONINFO, FAR* LPGNSESSIONINFO , CONST  FAR* LPCGNSESSIONINFO; 
gnPlayerInfo	gamenet.h	1591	typedef struct gnPlayerInfo
GNPLAYERINFO	gamenet.h	1833	} GNPLAYERINFO, FAR* LPGNPLAYERINFO, CONST FAR* LPCGNPLAYERINFO;
LPGNPLAYERINFO	gamenet.h	1852	} GNPLAYERINFO, FAR* LPGNPLAYERINFO, CONST FAR* LPCGNPLAYERINFO;
LPCGNPLAYERINFO	gamenet.h	1879	} GNPLAYERINFO, FAR* LPGNPLAYERINFO, CONST FAR* LPCGNPLAYERINFO;
gnMessage	gamenet.h	1912	typedef struct gnMessage
GNMESSGAGE	gamenet.h	2339	} GNMESSGAGE, FAR* LPGNMESSAGE;
LPGNMESSAGE	gamenet.h	2356	} GNMESSGAGE, FAR* LPGNMESSAGE;
GN_MAXPLAYERDATA	gamenet.h	2384	const DWORD GN_MAXPLAYERDATA = 256;
GN_MAXSESSIONDATA	gamenet.h	2420	const DWORD GN_MAXSESSIONDATA = 256;
GN_MAXSENDDATA	gamenet.h	2457	const DWORD GN_MAXSENDDATA  = 500;
gnPlayerInfoSize	gamenet.h	5312	inline DWORD gnPlayerInfoSize(LPCGNPLAYERINFO pInfo) 
gnSessionInfoSize	gamenet.h	5441	inline DWORD gnSessionInfoSize(LPCGNSESSIONINFO sInfo) 
__PSDNET_H__	psdnet.h	29	#define __PSDNET_H__
__SPLRMAP_H__	smap.h	30	#define __SPLRMAP_H__
PList	smap.h	272	typedef VPList(PLink) PList;
PLIterator	smap.h	357	typedef VPListIterator(PList, PLink) PLIterator;
WSList	smap.h	418	typedef VPList(WSLink) WSList;
WSIterator	smap.h	507	typedef VPListIterator(WSList, WSLink) WSIterator;
VDMPLAYCLASS	vdmplay.cpp	70	#define VDMPLAYCLASS "VdmPlayClass"
WM_WINSOCKTCP	vdmplay.cpp	106	#define WM_WINSOCKTCP (WM_USER+1000)
NTList	vdmplay.cpp	296	typedef VPList(NTLink) NTList;
OnMessage	vdmplay.cpp	4379	LRESULT CWinTcpNet::OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
Startup	vdmplay.cpp	7405	BOOL CVdmPlay::Startup(IN DWORD version, 
EnumSessions	vdmplay.cpp	7831	BOOL CVdmPlay::EnumSessions(IN HWND hWnd,
StopEnumSessions	vdmplay.cpp	8197	BOOL CVdmPlay::StopEnumSessions()
StopEnumPlayers	vdmplay.cpp	8337	BOOL CVdmPlay::StopEnumPlayers()
EnumPlayers	vdmplay.cpp	8485	BOOL CVdmPlay::EnumPlayers(IN HWND hWnd, 
Cleanup	vdmplay.cpp	8822	void CVdmPlay::Cleanup()
CreateSession	vdmplay.cpp	9054	VPSESSIONHANDLE CVdmPlay::CreateSession(IN HWND hWnd, 
JoinSession	vdmplay.cpp	9365	VPSESSIONHANDLE CVdmPlay::JoinSession(IN HWND hWnd,
WinProc	vdmplay.cpp	9742	LRESULT APIENTRY CVdmPlay::WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
InitWindowsStuff	vdmplay.cpp	10324	BOOL CVdmPlay::InitWindowsStuff()
InitNetworkProtocol	vdmplay.cpp	11795	BOOL CVdmPlay::InitNetworkProtocol(int protocol)
CleanupWindowsStuff	vdmplay.cpp	12157	void CVdmPlay::CleanupWindowsStuff()
MakeRemoteSession	vdmplay.cpp	12352	CWinRemoteSession* CVdmPlay::MakeRemoteSession(HWND wnd, LPVCSESSIONID id, BOOL forEnum)
vpStartup	vdmplay.cpp	12758	VPHANDLE VPAPI vpStartup(IN DWORD version, 
vpCleanup	vdmplay.cpp	13153	BOOL VPAPI vpCleanup(IN VPHANDLE pHdl)
vpEnumSessions	vdmplay.cpp	13296	BOOL VPAPI vpEnumSessions(IN VPHANDLE pHdl, 
vpStopEnumSessions	vdmplay.cpp	13668	BOOL VPAPI vpStopEnumSessions(IN VPHANDLE pHdl)
vpEnumPlayers	vdmplay.cpp	13825	BOOL VPAPI vpEnumPlayers(IN VPHANDLE pHdl,
vpCreateSession	vdmplay.cpp	14106	VPSESSIONHANDLE VPAPI vpCreateSession(IN VPHANDLE pHdl, 
vpGetSessionInfo	vdmplay.cpp	14509	BOOL VPAPI vpGetSessionInfo(IN VPSESSIONHANDLE pSesHdl, OUT LPVPSESSIONINFO sInfo)
vpGetSessionInfoById	vdmplay.cpp	14787	BOOL VPAPI vpGetSessionInfoById(IN VPHANDLE pHdl, 
vpUpdateSessionData	vdmplay.cpp	15163	BOOL VPAPI vpUpdateSessionData(IN VPSESSIONHANDLE pSesHdl, IN LPVOID sessionData)
vpJoinSession	vdmplay.cpp	15392	VPSESSIONHANDLE VPAPI vpJoinSession(IN VPHANDLE pHdl,
vpAddPlayer	vdmplay.cpp	15849	BOOL VPAPI vpAddPlayer(IN VPSESSIONHANDLE pSesHdl,
vpSendData	vdmplay.cpp	16210	BOOL VPAPI vpSendData(IN VPSESSIONHANDLE pSesHdl, 
vpCloseSesion	vdmplay.cpp	16652	BOOL VPAPI vpCloseSesion(IN VPSESSIONHANDLE pSesHdl, IN LPVOID userData)
vpAcknowledge	vdmplay.cpp	16935	BOOL VPAPI vpAcknowledge(IN VPHANDLE pHdl, LPCVPMESSAGE pMsg)
vpInvitePlayer	vdmplay.cpp	17266	BOOL VPAPI vpInvitePlayer(IN VPSESSIONHANDLE pSesHdl, VPPLAYERID playrId)
vpRejectPlayer	vdmplay.cpp	17360	BOOL VPAPI vpRejectPlayer(IN VPSESSIONHANDLE pSesHdl, VPPLAYERID playrId)
net	vpengine.cpp	161	CSession::CSession(CNetInterface* net, 
players	vpengine.cpp	197	                  CPlayerMap* players,
m_msgId	vpengine.cpp	394	    m_error(0), m_errInfo(0), m_msgId(0)
SafeDataHook	vpengine.cpp	428	void CSession::SafeDataHook(CNetLink* link, LPVOID context)
UnsafeDataHook	vpengine.cpp	535	void CSession::UnsafeDataHook(CNetLink* link, LPVOID context)
ConnectHook	vpengine.cpp	646	void CSession::ConnectHook(CNetLink* link, LPVOID context)
DisconnectHook	vpengine.cpp	752	void CSession::DisconnectHook(CNetLink* link, LPVOID context)
AcceptHook	vpengine.cpp	863	void CSession::AcceptHook(CNetLink* link, LPVOID context)
InitSessionInfo	vpengine.cpp	968	void CSession::InitSessionInfo(LPCVPGUID guid, 
ExceptEnumHelper	vpengine.cpp	1347	BOOL CSession::ExceptEnumHelper(CWS* ws, LPVOID data)
ForAllWorkstationsExcept	vpengine.cpp	1515	void CSession::ForAllWorkstationsExcept(CWS* ws, WSEnum p, LPVOID data)
SendOnLink	vpengine.cpp	1718	BOOL CSession::SendOnLink(CNetLink* link, LPVOID data)
PlayerCount	vpengine.cpp	1933	WORD CSession::PlayerCount() 
GetSessionInfo	vpengine.cpp	2004	BOOL CSession::GetSessionInfo(LPVPSESSIONINFO pInfo)
RemoveRemotePlayer	vpengine.cpp	2316	CRemotePlayer* CSession::RemoveRemotePlayer(VPPLAYERID pId, CRemoteWS* ws)
AddRemotePlayer	vpengine.cpp	2816	CRemotePlayer* CSession::AddRemotePlayer(plrInfoMsg* pInfoMsg, CRemoteWS* ws, BOOL doNotify)
MakeLocalPlayer	vpengine.cpp	3295	CLocalPlayer* CSession::MakeLocalPlayer(LPCSTR playerName, 
NextMessageId	vpengine.cpp	3740	WORD CSession::NextMessageId()
SendData	vpengine.cpp	3817	BOOL CSession::SendData(VPPLAYERID toId,
SendAllPlayers	vpengine.cpp	4629	BOOL CSession::SendAllPlayers(CWS* ws, LPVOID data)
SendToWs	vpengine.cpp	4878	BOOL CSession::SendToWs(CWS* ws, LPVOID data)
OnUDataREQ	vpengine.cpp	5045	BOOL CSession::OnUDataREQ(genericMsg* msg, CRemoteWS* ws)
OnUBDataREQ	vpengine.cpp	5257	BOOL CSession::OnUBDataREQ(genericMsg* msg, CRemoteWS* ws)
DeletePlayer	vpengine.cpp	5340	int DeletePlayer(CPlayer* p, LPVOID data)
DeleteWs	vpengine.cpp	5412	int DeleteWs(CWS* ws, LPVOID data)
CloseSession	vpengine.cpp	5490	BOOL CSession::CloseSession()
CSession	vpengine.cpp	5893	CSession::~CSession()
net	vpengine.cpp	5955	CLocalSession::CLocalSession(CNetInterface* net, 
players	vpengine.cpp	5980								CPlayerMap* players, 
m_nextPlayerId	vpengine.cpp	6052	   CSession(net, players, wsMap), m_nextPlayerId(VP_FIRSTPLAYER)
OnConnect	vpengine.cpp	6148	void CLocalSession::OnConnect(CNetLink* link)
OnAccept	vpengine.cpp	6348	void CLocalSession::OnAccept(CNetLink* link)
leaveInfo	vpengine.cpp	6837	struct leaveInfo
SimulateLeave	vpengine.cpp	6894	BOOL SimulateLeave(CPlayer* p, LPVOID ctx)
OnDisconnect	vpengine.cpp	7122	void CLocalSession::OnDisconnect(CNetLink* link)
OnSafeData	vpengine.cpp	7519	void CLocalSession::OnSafeData(CNetLink* link)
OnUnsafeData	vpengine.cpp	8432	void CLocalSession::OnUnsafeData(CNetLink* link)
NewPlayerId	vpengine.cpp	9372	VPPLAYERID CLocalSession::NewPlayerId()
AddRemotePlayer	vpengine.cpp	9448	CRemotePlayer* CLocalSession::AddRemotePlayer(plrInfoMsg* pInfo, 
AddLocalPlayer	vpengine.cpp	9616	BOOL CLocalSession::AddLocalPlayer(LPCSTR playerName, 
OnLeaveREQ	vpengine.cpp	10393	BOOL CLocalSession::OnLeaveREQ(plrInfoMsg* msg, CRemoteWS* ws)
OnJoinREQ	vpengine.cpp	11079	BOOL CLocalSession::OnJoinREQ(plrInfoMsg* msg, CRemoteWS* ws)
OnUBDataREQ	vpengine.cpp	11918	BOOL CLocalSession::OnUBDataREQ(genericMsg* msg, CRemoteWS* ws)
OnSenumREQ	vpengine.cpp	12201	BOOL CLocalSession::OnSenumREQ(genericMsg* msg, CRemoteWS* ws)
spiContext	vpengine.cpp	12431	struct spiContext
SendPlayerInfo	vpengine.cpp	12537	BOOL SendPlayerInfo(CPlayer* p, LPVOID data)
OnPenumREQ	vpengine.cpp	12895	BOOL CLocalSession::OnPenumREQ(genericMsg* msg, CRemoteWS* ws)
UpdateSessionInfo	vpengine.cpp	13172	BOOL CLocalSession::UpdateSessionInfo(LPVOID data)
BroadcastSessionData	vpengine.cpp	13351	BOOL CLocalSession::BroadcastSessionData()
net	vpengine.cpp	13909	CRemoteSession::CRemoteSession(CNetInterface* net, 
players	vpengine.cpp	13934								CPlayerMap* players, 
m_serverEnumData	vpengine.cpp	14070	   m_initialJoin(TRUE), m_serverEnumData(NULL)
LookForServer	vpengine.cpp	14120	BOOL CRemoteSession::LookForServer(LPVOID data)
SendData	vpengine.cpp	14595	BOOL CRemoteSession::SendData(VPPLAYERID toId,
OnLeaveREQ	vpengine.cpp	15262	BOOL CRemoteSession::OnLeaveREQ(plrInfoMsg* msg, CRemoteWS* ws)
OnJoinREP	vpengine.cpp	15732	BOOL CRemoteSession::OnJoinREP(plrInfoMsg* msg, CRemoteWS* ws)
RegisterPlayerWS	vpengine.cpp	16477	CRemoteWS* CRemoteSession::RegisterPlayerWS(plrInfoMsg* msg)
OnJoinADV	vpengine.cpp	16861	BOOL CRemoteSession::OnJoinADV(plrInfoMsg* msg, CRemoteWS* )
OnJoinNAK	vpengine.cpp	17112	BOOL CRemoteSession::OnJoinNAK(plrInfoMsg* msg, CRemoteWS* ws)
AddLocalPlayer	vpengine.cpp	17288	BOOL CRemoteSession::AddLocalPlayer(LPCSTR playerName, 
OnUDataREQ	vpengine.cpp	18001	BOOL CRemoteSession::OnUDataREQ(genericMsg* msg, CRemoteWS* ws)
OnUBDataREQ	vpengine.cpp	18110	BOOL CRemoteSession::OnUBDataREQ(genericMsg* msg, CRemoteWS* ws)
OnPenumREP	vpengine.cpp	18220	BOOL CRemoteSession::OnPenumREP(plrInfoMsg* msg, CRemoteWS* ws)
OnSenumREP	vpengine.cpp	18796	BOOL CRemoteSession::OnSenumREP(sesInfoMsg* msg, CRemoteWS* ws)
UpdateSessionInfo	vpengine.cpp	19248	BOOL CRemoteSession::UpdateSessionInfo(LPVOID data)
OnConnect	vpengine.cpp	19357	void CRemoteSession::OnConnect(CNetLink* link)
OnAccept	vpengine.cpp	19708	void CRemoteSession::OnAccept(CNetLink* link)
OnDisconnect	vpengine.cpp	20070	void CRemoteSession::OnDisconnect(CNetLink* link)
OnSafeData	vpengine.cpp	20413	void CRemoteSession::OnSafeData(CNetLink* link)
OnUnsafeData	vpengine.cpp	22622	void CRemoteSession::OnUnsafeData(CNetLink* link)
ConnectToServer	vpengine.cpp	23706	BOOL CRemoteSession::ConnectToServer(LPCVPNETADDRESS addr, LPVOID userData)
pInfo	vpengine.cpp	24030	CPlayer::CPlayer(LPVPPLAYERINFO pInfo, CWS* ws) : m_ws(ws), m_info(NULL)
m_info	vpengine.cpp	24058	CPlayer::CPlayer(LPVPPLAYERINFO pInfo, CWS* ws) : m_ws(ws), m_info(NULL)
pInfoMsg	vpengine.cpp	24266	CPlayer::CPlayer(plrInfoMsg* pInfoMsg, CWS* ws) : m_ws(ws), m_info(pInfoMsg)
m_info	vpengine.cpp	24297	CPlayer::CPlayer(plrInfoMsg* pInfoMsg, CWS* ws) : m_ws(ws), m_info(pInfoMsg)
CPlayer	vpengine.cpp	24360	CPlayer::~CPlayer()
SendData	vpengine.cpp	24505	BOOL CRemotePlayer::SendData(sendDataInfo& sdi)
SendData	vpengine.cpp	24800	BOOL CRemoteWS::SendData(sendDataInfo& sdi)
CRemoteWS	vpengine.cpp	25222	CRemoteWS::~CRemoteWS()
CNotification	vpengine.cpp	25383	CNotification::CNotification(WPARAM msgCode, 
CNotification	vpengine.cpp	25614	CNotification::~CNotification()
NTList	twsknet.cpp	263	typedef VPList(NTLink) NTList;
PrintableAddress	twsknet.cpp	2136	const char* PrintableAddress(LPCVPNETADDRESS a, CNetInterface* net)
PrintPlayer	twsknet.cpp	2369	void PrintPlayer(ostream& os, LPCVPPLAYERINFO info, CNetInterface* net)
msgText	twsknet.cpp	2646	const char* msgText(WORD msgCode)
ProcessNotifications	twsknet.cpp	3092	void ProcessNotifications(const char* label, CNotifyQueue* q, CNetInterface* net)
Poll	twsknet.cpp	4554	BOOL TTcpNet::Poll()
SimulationLoop	twsknet.cpp	5159	void SimulationLoop(TTcpNet* n1, TTcpNet* n2)
MYSesInfo	twsknet.cpp	5253	struct MYSesInfo
myInfo	twsknet.cpp	5323	MYSesInfo myInfo =
test2	twsknet.cpp	5563	void test2()
test1	twsknet.cpp	6582	void test1()
main	twsknet.cpp	7505	int main()
netCount	tcpnet.cpp	73	static int netCount = 0;
linkMap	tcpnet.cpp	135	CTcpNet::CTCPLink::LinkMap  CTcpNet::CTCPLink::linkMap;
streamPort	tcpnet.cpp	171	CTcpNet::CTcpNet(u_short streamPort, u_short dgPort, u_short wellKnownPort) :
dgPort	tcpnet.cpp	191	CTcpNet::CTcpNet(u_short streamPort, u_short dgPort, u_short wellKnownPort) :
m_dgLink	tcpnet.cpp	284	 m_wellKnownPort(htons(wellKnownPort)), m_listenLink(NULL), m_dgLink(NULL)
MakeSafeLink	tcpnet.cpp	927	CNetLink* CTcpNet::MakeSafeLink(CNetAddress* addr, LPVOID userData)
MakeUnsafeLink	tcpnet.cpp	1871	CNetLink* CTcpNet::MakeUnsafeLink(LPVOID userData)
AcceptLink	tcpnet.cpp	2362	CTcpNet::CTCPLink*  CTcpNet::AcceptLink()
MakeListenLink	tcpnet.cpp	2737	CTcpNet::CTCPLink* CTcpNet::MakeListenLink()
Listen	tcpnet.cpp	3035	BOOL CTcpNet::Listen(BOOL streamListen, BOOL serverMode)
BecomeDeef	tcpnet.cpp	4239	void CTcpNet::BecomeDeef()
CTcpNet	tcpnet.cpp	4416	CTcpNet::~CTcpNet()
EnqueueData	tcpnet.cpp	4586	BOOL CTcpNet::CTCPLink::EnqueueData(LPVOID data, DWORD dataSize)
GetRemoteAddress	tcpnet.cpp	4893	CNetAddress* CTcpNet::CTCPLink::GetRemoteAddress()
LookupLink	tcpnet.cpp	5330	CTcpNet::CTCPLink* CTcpNet::CTCPLink::LookupLink(SOCKET s)
SendWaitingData	tcpnet.cpp	5512	void CTcpNet::CTCPLink::SendWaitingData()
Send	tcpnet.cpp	6099	BOOL CTcpNet::CTCPLink::Send(LPVOID data, DWORD dataSize, DWORD flags)
SendTo	tcpnet.cpp	6655	BOOL CTcpNet::CTCPLink::SendTo(CNetAddress& na, LPVOID data, DWORD dataSize, DWORD flags)
Receive	tcpnet.cpp	7265	DWORD CTcpNet::CTCPLink::Receive(LPVOID data, DWORD dataSize)
ReceiveFrom	tcpnet.cpp	7717	DWORD CTcpNet::CTCPLink::ReceiveFrom(LPVOID data, DWORD dataSize, CNetAddress& na)
HasData	tcpnet.cpp	8290	DWORD CTcpNet::CTCPLink::HasData()
nets	psdnet.cpp	113	CPseudoNet* CPseudoNet::nets[MAXNET];
MakeAddress	psdnet.cpp	153	CNetAddress* CPseudoNet::MakeAddress(LPCVPNETADDRESS a)
MakeBroadcastAddress	psdnet.cpp	252	CNetAddress* CPseudoNet::MakeBroadcastAddress()
MakeUnsafeLink	psdnet.cpp	334	CNetLink* CPseudoNet::MakeUnsafeLink()
RegisterLink	psdnet.cpp	447	void CPseudoNet::RegisterLink(PNLink* l)
SimulationLoop	psdnet.cpp	616	void CPseudoNet::SimulationLoop()
UnregisterLink	psdnet.cpp	850	void CPseudoNet::UnregisterLink(PNLink* l)
GetRemoteAddress	psdnet.cpp	1042	CNetAddress* CPseudoNet::PNLink::GetRemoteAddress()
MakeSafeLink	psdnet.cpp	1159	CNetLink* CPseudoNet::MakeSafeLink(CNetAddress* a)
SendTo	psdnet.cpp	1792	BOOL CPseudoNet::PNLink::SendTo(CNetAddress& a, LPVOID data, DWORD dataSize,
Listen	psdnet.cpp	2252	BOOL CPseudoNet::Listen()
BecomeDeef	psdnet.cpp	2360	void  CPseudoNet::BecomeDeef()
Poll	psdnet.cpp	2438	BOOL CPseudoNet::Poll()
Send	psdnet.cpp	2917	BOOL CPseudoNet::PNLink::Send(LPVOID data, DWORD dataSize,  DWORD flags)
Receive	psdnet.cpp	3265	DWORD CPseudoNet::PNLink::Receive(LPVOID data, DWORD dataSize)
ReceiveFrom	psdnet.cpp	3571	DWORD CPseudoNet::PNLink::ReceiveFrom(LPVOID data, DWORD dataSize, 
AcceptData	psdnet.cpp	3916	void CPseudoNet::PNLink::AcceptData(LPVOID data, DWORD dataSize,  int from)
paddress	psdnet.cpp	4450	union paddress
SafeDelivery	psdnet.cpp	4509	void SafeDelivery(CNetLink* l, LPVOID )
UnsafeDelivery	psdnet.cpp	4892	void UnsafeDelivery(CNetLink* l, LPVOID data)
PSNetTest	psdnet.cpp	5349	void PSNetTest()
main	psdnet.cpp	6260	int main(int argc, char* argv[])
ASSERT	tfrmwk.cpp	28	#define ASSERT assert
NTList	tfrmwk.cpp	282	typedef VPList(NTLink) NTList;
paddress	tfrmwk.cpp	2148	union paddress
MYSesInfo	tfrmwk.cpp	2209	struct MYSesInfo
myInfo	tfrmwk.cpp	2279	MYSesInfo myInfo =
PrintPlayer	tfrmwk.cpp	2517	void PrintPlayer(ostream& os, LPVPPLAYERINFO info)
ProcessNotificatons	tfrmwk.cpp	2754	void ProcessNotificatons(const char* label, CNotifyQueue* q)
test1	tfrmwk.cpp	3501	void test1()
main	tfrmwk.cpp	4053	int main(int argc, char* argv[])
Enum	smap.cpp	82	void CSimplePlayerMap::Enum(EnumProc p, LPVOID userData)
CSimplePlayerMap	smap.cpp	287	CSimplePlayerMap::~CSimplePlayerMap()
LookupPlayer	smap.cpp	454	PLink* CSimplePlayerMap::LookupPlayer(VPPLAYERID id)
RemovePlayer	smap.cpp	653	BOOL CSimplePlayerMap::RemovePlayer(VPPLAYERID id)
PlayerAtId	smap.cpp	847	CPlayer* CSimplePlayerMap::PlayerAtId(VPPLAYERID playerId)
AddPlayer	smap.cpp	1021	BOOL    CSimplePlayerMap::AddPlayer(CPlayer* p)
Register	smap.cpp	1231	void CSimpleWSMap::Register(CWS* ws)
Deregister	smap.cpp	1403	void CSimpleWSMap::Deregister(CWS* ws)
CSimpleWSMap	smap.cpp	1571	CSimpleWSMap::~CSimpleWSMap()
Find	smap.cpp	1723	CWS* CSimpleWSMap::Find(CWS* ws)
FindBySafeLink	smap.cpp	1857	CWS* CSimpleWSMap::FindBySafeLink(CNetLink* link)
FindByAddress	smap.cpp	2134	CWS* CSimpleWSMap::FindByAddress(CNetAddress* a)
Lookup	smap.cpp	2417	WSLink* CSimpleWSMap::Lookup(CWS* ws)
Enum	smap.cpp	2599	void CSimpleWSMap::Enum(EnumProc p, LPVOID userData)
