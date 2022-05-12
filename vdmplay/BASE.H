#ifndef __BASE_H__
#define __BASE_H__

static LPCSTR protoName[] = { "TCP", "IPX", "NETBIOS", "COMM", "MODEM", "TAPI", "DP" };

const int DEF_TCP_PORT = 1707;
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
