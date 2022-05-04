/*
 * Definitions for Core.h++ error messages
 *
 * $Id: coreerr.cpp,v 2.5 1993/09/10 03:59:57 keffer Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 *
 * Copyright (C) 1993.
 * This software is subject to copyright protection under the laws of
 * the United States and other countries.
 *
 ***************************************************************************
 *
 * $Log: /vdmplay/COREERR.CPP $
 * 
 * 1     4/02/96 13:46 Vadim
 * Revision 2.5  1993/09/10  03:59:57  keffer
 * Added RW_RCSID macro.
 *
 * Revision 2.4  1993/06/21  22:59:29  randall
 * undefined EOF
 *
 * Revision 2.3  1993/05/31  21:46:32  keffer
 * New messaging architecture for localization
 *
 * Revision 2.2  1993/05/18  19:08:14  keffer
 * Ported to cfront V2.1.
 *
 */

#include "rw/coreerr.h"
#include "rw/generic.h"

RW_RCSID("Copyright (C) Rogue Wave Software --- $RCSfile: coreerr.cpp,v $ $Revision: 1 $ $Date: 4/02/96 13:46 $");

#undef  EOF 
#undef  DECLARE_MSG
#define DECLARE_MSG(NAME, VALUE, MSG) \
        const RWCatMsg name2(RWCOREMSG_,NAME) = {RWCORE, VALUE, MSG};    \
        const RWMsgId  name2(RWCORE_,NAME)    = &name2(RWCOREMSG_,NAME);

const char RWCORE[] = "rwcore6.0";

DECLARE_MSG(EOF,      0x1000, "[EOF] EOF on input");
DECLARE_MSG(GENERIC,  0x1001, "[GENERIC] Generic error number %d; %s");
DECLARE_MSG(INVADDR,  0x1002, "[INVADDR] Invalid address: %lx");
DECLARE_MSG(LOCK,     0x1003, "[LOCK] Unable to lock memory");
DECLARE_MSG(NOINIT,   0x1004, "[NOINIT] Memory allocated without being initialized");
DECLARE_MSG(NOMEM,    0x1005, "[NOMEM] No memory");
DECLARE_MSG(OPERR,    0x1006, "[OPERR] Could not open file %s");
DECLARE_MSG(OUTALLOC, 0x1007, "[OUTALLOC] Memory released with allocations still outstanding");
DECLARE_MSG(OVFLOW,   0x1008, "[OVFLOW] Overflow error -> \"%.*s\" <- (%u max characters)");
DECLARE_MSG(STREAM,   0x1009, "[STREAM] Bad input stream");
DECLARE_MSG(SYNSTREAM,0x100a, "[SYNSTREAM] Syntax error in input stream: expected %s, got %s");

