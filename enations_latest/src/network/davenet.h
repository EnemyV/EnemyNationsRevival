#ifndef __DAVENET_H__
#define __DAVENET_H__


#define DAVENETAPI __declspec (dllexport)

const int WM_NET_COMPLETE = 0x7000;

const int NET_PROTO_TCP = 1;
const int NET_PROTO_IPX = 2;
const int NET_PROTO_NETBIOS = 3;
const int NET_PROTO_MODEM = 4;
const int NET_PROTO_DIRECT = 5;

const int NET_MSG_ADD_NAME = 1;
const int NET_MSG_ADD_GROUP_NAME = 2;
const int NET_MSG_CALL = 3;
const int NET_MSG_CANCEL = 4;
const int NET_MSG_DELETE_NAME = 5;
const int NET_MSG_HANG_UP = 6;
const int NET_MSG_RECEIVE = 7;
const int NET_MSG_RECEIVE_DATAGRAM = 8;
const int NET_MSG_SEND = 9;
const int NET_MSG_SEND_DATAGRAM = 10;
const int NET_MSG_LISTEN = 11;
const int NET_MSG_MAX_NUM = 11;

// nb_lock.asm has these values hard-coded
const int NET_ERR_NONE = 0;
const int NET_ERR_UNKNOWN = 1;
const int NET_ERR_DUP_LOCAL_NAME = 2;
const int NET_ERR_DUP_REMOTE_NAME = 3;
const int NET_ERR_SESSION_CLOSED = 4;
const int NET_ERR_REQUEST_CANCELLED = 5;
const int NET_ERR_MAX_NUM = 5;


// mod 4 + 2 & <= shortest name amoung net protocols
// NOTE - nb_lock uses hard-coded numbers that depends on sizes and layout here
const int NAME_MAX = 14;

typedef struct tagNETMSG {
		BYTE				bInUse;				// true if in use
		BYTE				bCmd;					// the command completed
		BYTE				bErr;					// the return code
		BYTE				bNetNum;			// node number
		void * 			pUser;				// user defined data
		void *			pData;				// the data returned
		short int		iLen;					// length of the data
		char				sName[NAME_MAX];	// name of src node for receive datagram
} NETMSG;

extern "C" {
DAVENETAPI long naVersion ();
DAVENETAPI BOOL naHave (int ID);
DAVENETAPI void naErrMsgBox (void  *pHdl, NETMSG *pMsg);
DAVENETAPI void * naInit (int ID, HINSTANCE hInst, HWND hWnd);
DAVENETAPI void naClose (void  * pHdl);

DAVENETAPI BOOL naAddName (void  * pHdl, LPCSTR pName, LPCVOID pData = NULL);
DAVENETAPI BOOL naAddGroupName (void  * pHdl, LPCSTR pName, LPCVOID pData = NULL);
DAVENETAPI BOOL naCall (void  * pHdl, LPCSTR pLocalName, LPCSTR pRemoteName, LPCVOID pData = NULL);
DAVENETAPI void	naCancelReceive (void  *pHdl, int iNum);
DAVENETAPI void naCancelReceiveDatagram (void  *pHdl, int iNum);
DAVENETAPI BOOL naDeleteName (void  * pHdl, LPCSTR pName, LPCVOID pData = NULL);
DAVENETAPI BOOL naHangUp (void  *pHdl, int iNum, LPCVOID pUser);
DAVENETAPI BOOL naListen (void  * pHdl, LPCSTR pLocalName, LPCSTR pRemoteName, LPCVOID pData = NULL);
DAVENETAPI BOOL naReceive (void  *pHdl, int iNum, LPCVOID pUser);
DAVENETAPI BOOL naReceiveDatagram (void  *pHdl, int iNum, LPCVOID pUser);
DAVENETAPI BOOL naSend (void  *pHdl, int iNum, LPCVOID pData, int iLen, LPCVOID pUser = NULL);
DAVENETAPI BOOL naSendDatagram (void  *pHdl, int iNum, LPCSTR pName, LPCVOID pData, int iLen, LPCVOID pUser = NULL);
}

#endif
