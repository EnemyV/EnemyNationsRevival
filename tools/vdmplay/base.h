#ifndef __BASE_H__
#define __BASE_H__

#include "stdafx.h"

static LPCSTR protoName[] = { "TCP", "IPX", "NETBIOS", "COMM", "MODEM", "TAPI", "DP" };

// Default well-known TCP port. Must match the port the game's network dialogs
// use (2346) so that if vdmplay can't read the written WellKnownPort from the
// ini (e.g. gLocalIni resolves to the Windows-dir ini), host and client still
// fall back to the SAME port and can find each other. Was 1707 (the original
// vdmplay default), which silently mismatched the game's 2346 -> no discovery.
const int DEF_TCP_PORT = 2346;
const int DEF_IPX_PORT = 1707;
const int DEF_NETBIOS_LANA = 255;
const char DEF_COMM_PORT [] = "COM1";
const char DEF_MODEM_INIT [] = "ATZ";
const char DEF_MODEM_SUFFIX [] = "^M";
const char DEF_MODEM_DIAL [] = "ATDT";
const char DEF_MODEM_ANSWER [] = "ATA";
const char DEF_MODEM_HANGUP [] = "ATH";
const char DEF_MODEM_CONNECT [] = "CONNECT";
const char DEF_MODEM_FAILURE [] = "ERROR";
const char DEF_IP_REG_SERVER [] = "iserve.windward.net";
#endif
