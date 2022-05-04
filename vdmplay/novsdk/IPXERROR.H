/*****************************************************************************
 *
 * Filename:	  ipxerror.h
 *
 * Comments:	  Error return codes defined for IPX.
 *
 * COPYRIGHT (c) 1988 by Novell, Inc.  All Rights Reserved.
 *
 ****************************************************************************/

#define IPX_TIMEOUT					0x9001
#define IPX_NO_ROUTE					0x9002
#define IPX_SOCKET_IN_USE			0x9003
#define IPX_SOCKET_NOT_OPEN		0x9004

#define LAN_ERR_OUT_OF_RESOURCE	0x8001
#define LAN_ERR_BAD_PARAM			0x8002
#define LAN_ERR_NO_MORE_ITEMS		0x8003
#define LAN_ERR_NOT_FOUND			0x8004
#define LAN_ERR_FAILED				0x8005
#define LAN_ERR_RECV_OVERFLOW		0x8006
#define LAN_ERR_CANCELLED			0x8007
#define LAN_ERR_INVALID_FUNC		0x8008
#define LAN_ERR_DUP_ENTRY			0x8009
