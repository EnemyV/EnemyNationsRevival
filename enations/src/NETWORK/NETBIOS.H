#ifndef __NETBIOS_H__
#define __NETBIOS_H__


/* common defines */
#define MAX_NAME_SIZE             16
#define MAX_TABLE_NAMES           16
#define MAX_DATAGRAM_BUFFER       512
#define NO_WAIT                   0x80
#define MIN_NAME_NUM              2
#define MAX_NAME_NUM              254
#define ILLEGAL_NAME_NUM          0
#define MIN_LSN                   1
#define MAX_LSN                   254
#define ILLEGAL_LSN               0
#define MAX_ADAPTER_NUM           1
#define MAX_SESSION_COUNT         254
#define MAX_NAME_COUNT            254
#define MAX_COMMAND_COUNT         254

/*
 * the following calls are currently supported under windows 3.0/3.1
 * and have WinNET API routines defined below
 */
#define NETBIOS_ADD_NAME                0x30
#define NETBIOS_ADD_GROUP_NAME          0x36
#define NETBIOS_DELETE_NAME             0x31
#define NETBIOS_LISTEN                  0x11
#define NETBIOS_CALL                    0x10
#define NETBIOS_HANGUP                  0x12
#define NETBIOS_RECEIVE                 0x15
#define NETBIOS_SEND                    0x14
#define NETBIOS_CHAIN_SEND              0x17
#define NETBIOS_RESET                   0x32
#define NETBIOS_CANCEL                  0x35
#define NETBIOS_UNLINK                  0x70
#define NETBIOS_SEND_DATAGRAM           0x20
#define NETBIOS_RECEIVE_DATAGRAM        0x21
#define NETBIOS_SEND_BROADCAST          0x22
#define NETBIOS_RECEIVE_BROADCAST       0x23
#define NETBIOS_ADAPTER_STATUS          0x33
#define NETBIOS_SESSION_STATUS          0x34
#define NETBIOS_INVALID_COMMAND         0x7f

/* NetBIOS return codes */
#define NB_SUCCESSFUL                   0x00
#define NB_ILLEGAL_BUFFER_LENGTH        0x01
#define NB_INVALID_COMMAND              0x03
#define NB_COMMAND_TIMED_OUT            0x05
#define NB_MESSAGE_INCOMPLETE           0x06
#define NB_NO_ACK_FAILURE               0x07
#define NB_ILLEGAL_LSN                  0x08
#define NB_NO_RESOURCE_AVAILABLE        0x09
#define NB_SESSION_CLOSED               0x0a
#define NB_COMMAND_CANCELLED            0x0b
#define NB_DUPLICATE_LOCAL_NAME         0x0d
#define NB_NAME_TABLE_FULL              0x0e
#define NB_NAME_HAS_ACTIVE_SESSIONS     0x0f
#define NB_LOCAL_SESSION_TABLE_FULL     0x11
#define NB_SESSION_OPEN_REJECTED        0x12
#define NB_ILLEGAL_NAME_NUMBER          0x13
#define NB_CANNOT_FIND_CALLED_NAME      0x14
#define NB_NAME_NOT_FOUND_OR_ILLEGAL    0x15
#define NB_NAME_USED_ON_RMT_ADAPTER     0x16
#define NB_NAME_DELETED                 0x17
#define NB_SESSION_ENDED_ABNORMALLY     0x18
#define NB_NAME_CONFLICT_DETECTED       0x19
#define NB_INCOMPATABLE_RMT_DEVICE      0x1a
#define NB_INTERFACE_BUSY               0x21
#define NB_TOO_MANY_COMMANDS_PENDING    0x22
#define NB_INVALID_ADAPTER_NUMBER       0x23
#define NB_CMD_COMPLETED_DURING_CANCEL  0x24
#define NB_RESERVED_NAME_SPECIFIED      0x25
#define NB_CMD_NOT_VALID_TO_CANCEL      0x26
#define NB_LANA_SYSTEM_ERROR            0x40
#define NB_LANA_REMOTE_HOT_CARRIER      0x41
#define NB_LANA_LOCAL_HOT_CARRIER       0x42
#define NB_LANA_NO_CARRIER_DETECTED     0x43
#define NB_UNUSUAL_NETWORK_CONNECTION   0x44
#define NB_ADAPTER_MALFUNCTION          0x50
#define NB_COMMAND_PENDING              0xff

/* NAMETABLEENTRY cNameStatus flags ie: if ((cNameStatus & NS_MASK) == NS_ACTIVE_NAME) */
#define NS_MASK                         0x87
#define NS_ADD_IN_PROGRESS              0
#define NS_ACTIVE_NAME                  4
#define NS_DELETE_PENDING               5
#define NS_IMPROPER_DUP_NAME            6
#define NS_DUP_NAME_DELETE_PENDING      7

/* name table entry - referenced in LANASTAT */
typedef struct _NameTableEntry
{
    BYTE  cName[MAX_NAME_SIZE]; /* Local Name             */
    BYTE  cNameNum;             /* Name Number 2 - 254     */
    BYTE  cNameStatus;          /* & with 0x87 for status NS_* */
} NAMETABLEENTRY, FAR * LPNAMETABLEENTRY;

/* LAN Adapter status data structure return from AdapterStatus() */
typedef struct _AdapterStatusBase
{
     BYTE cPermNodeName[6];    /* Permanent Node Name          */
     BYTE cJumperSettings;     /* Jumper Settings              */
     BYTE cSelfTestResults;    /* POST Results                 */
     BYTE cSoftVersionMajor;   /* Software Version Major       */
     BYTE cSoftVersionMinor;   /* Software Version Minor       */
     WORD wReportPeriodMins;   /* Reporting Period in minutes  */
     WORD wCrcErrorCount;      /* Number of CRC Errors         */
     WORD wAlignmentErrors;    /* Number of Alignment Errors   */
     WORD wCollisionCount;     /* Number of Collisions         */
     WORD wXmitAbortCount;     /* Number of Aborted Transmissions */
     LONG lSuccessfulXmits;    /* Number of Successful Transmissions */
     LONG lSuccessfulRcvs;     /* Number of Successful Receives */
     WORD wRetransmitCount;    /* Number of Retransmissions     */
     WORD wResourceDepletionCount;  /* Number of Times Resources Exhausted */
     BYTE wReserved1[8];       /* Reserved 8 Bytes              */
     WORD wFreeCommandBlocks;  /* Free Command Blocks           */
     WORD wCurrentMaxNcbs;     /* Maximum Number of NCBs Configured */
     WORD wFreeNcbs;           /* Number of Free NCBs           */
     BYTE cReserved2[4];       /* Reserved 4 Bytes              */
     WORD wPendingSessionCount; /* Number of Pending Sessions   */
     WORD wCurrentMaxPendingSessions; /* Maximum Number of Sessions Configured */
     WORD wMaxSessionCount;    /* Maximum Number of Possible Sessions */
     WORD wMaxPacketSize;      /* Maximum Session Data Packet Size */
     WORD wNameTableEntries;   /* Number of Names               */
} LANASTATBASE;

typedef struct _AdapterStatus
{
     BYTE cPermNodeName[6];    /* Permanent Node Name          */
     BYTE cJumperSettings;     /* Jumper Settings              */
     BYTE cSelfTestResults;    /* POST Results                 */
     BYTE cSoftVersionMajor;   /* Software Version Major       */
     BYTE cSoftVersionMinor;   /* Software Version Minor       */
     WORD wReportPeriodMins;   /* Reporting Period in minutes  */
     WORD wCrcErrorCount;      /* Number of CRC Errors         */
     WORD wAlignmentErrors;    /* Number of Alignment Errors   */
     WORD wCollisionCount;     /* Number of Collisions         */
     WORD wXmitAbortCount;     /* Number of Aborted Transmissions */
     LONG lSuccessfulXmits;    /* Number of Successful Transmissions */
     LONG lSuccessfulRcvs;     /* Number of Successful Receives */
     WORD wRetransmitCount;    /* Number of Retransmissions     */
     WORD wResourceDepletionCount;  /* Number of Times Resources Exhausted */
     BYTE wReserved1[8];       /* Reserved 8 Bytes              */
     WORD wFreeCommandBlocks;  /* Free Command Blocks           */
     WORD wCurrentMaxNcbs;     /* Maximum Number of NCBs Configured */
     WORD wFreeNcbs;           /* Number of Free NCBs           */
     BYTE cReserved2[4];       /* Reserved 4 Bytes              */
     WORD wPendingSessionCount; /* Number of Pending Sessions   */
     WORD wCurrentMaxPendingSessions; /* Maximum Number of Sessions Configured */
     WORD wMaxSessionCount;    /* Maximum Number of Possible Sessions */
     WORD wMaxPacketSize;      /* Maximum Session Data Packet Size */
     WORD wNameTableEntries;   /* Number of Names               */
     NAMETABLEENTRY aTableName[MAX_TABLE_NAMES]; /* Name Table Entries */
} LANASTAT, FAR *LPLANASTAT;

/* session cState flags */
#define SS_LISTEN             1
#define SS_CALL               2
#define SS_ACTIVE             3
#define SS_HANGUP_PENDING     4
#define SS_HANGUP_COMPLETE    5
#define SS_SESSION_ABORTED    6

/* session status table entry - referenced in SESSIONSTAT */
typedef struct _SessionStatEntry
{
    BYTE  cLSN;                /* Local Session Number          */
    BYTE  cState;              /* Session State SS_*            */
    BYTE  cName[MAX_NAME_SIZE];     /* Local Name               */
    BYTE  cCallName[MAX_NAME_SIZE]; /* Remote Name              */
    BYTE  cReceivesPending;    /* Number of Receives Pending    */
    BYTE  cSendsPending;       /* Number of Sends/Chain Sends Pending */
} SESSIONSTATENTRY, FAR * LPSESSIONSTATENTRY;

/* LAN Session status data structure return from SessionStatus() */
typedef struct _SessionStatus
{
     BYTE cSessionNum;         /* Session's Name Number        */
     BYTE cSessions;           /* Number of Sessions under Name */
     BYTE cDatagramsPending;   /* Number of Receive Datagrams  */
                               /* or Receive Broadcasts Pending */
     BYTE cReceiveAnysPending; /* Number of Receive Any Pending */
     SESSIONSTATENTRY aSessionStatEntry[36]; /* Session Status List for Name */
} SESSIONSTAT, FAR *LPSESSIONSTAT;

/* NetBIOS Command Block data structure returned from all Post routines */
typedef struct _NCB         /* NetBIOS command block */
{
    BYTE    cCommand;       /* command code */
    BYTE    cRetcode;       /* return code */
    BYTE    cLSN;           /* local session number (1 - 254) */
    BYTE    cNum;           /* number of name in local name table */
    LPSTR   lpBuffer;       /* message buffer address */
    WORD    wLength;        /* message buffer length */
    char    cCallName[MAX_NAME_SIZE]; /* local or remote NetBIOS name */
    char    cName[MAX_NAME_SIZE]; /* local NetBIOS name */
    BYTE    cRTO;           /* receive timeout count */
    BYTE    cSTO;           /* send timeout count */
    FARPROC fnPost;         /* address of post routine */
    BYTE    cAdapterNum;    /* 1=1st adapter; 1=2nd adapter */
    BYTE    cCmdCplt;       /* command status */
    BYTE    cReserved[14];  /* reserved area */
		// the following is stuff I added to track these guys
		NETMSG	msg;
} NCB, FAR * LPNCB;

#define NULL_NAME_NUMBER    -1

// name messages
#define WM_ADDNAME          WM_USER + 1
#define WM_DELETENAME       WM_USER + 2

// adapter messages
#define WM_ADAPTERSTATUS    WM_USER + 3

// session messages
#define WM_LISTEN           WM_USER + 4
#define WM_CALL             WM_USER + 5
#define WM_SEND             WM_USER + 6
#define WM_RECEIVE          WM_USER + 7
#define WM_HANGUP           WM_USER + 8

// datagram messages
#define WM_SENDDATAGRAM     WM_USER + 9
#define WM_RECEIVEDATAGRAM  WM_USER + 10

// broadcast messages
#define WM_SENDBROADCAST    WM_USER + 11
#define WM_RECEIVEBROADCAST WM_USER + 12


#endif
