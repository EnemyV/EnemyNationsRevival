/*
 * Definitions for Tools.h++ error messages
 *
 * $Id: toolerr.cpp,v 2.4 1993/09/10 03:59:57 keffer Exp $
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
 * $Log: /vdmplay/TOOLERR.CPP $
 * 
 * 1     4/02/96 13:46 Vadim
 * Revision 2.4  1993/09/10  03:59:57  keffer
 * Added RW_RCSID macro.
 *
 * Revision 2.3  1993/05/31  21:46:32  keffer
 * New messaging architecture for localization
 *
 * Revision 2.2  1993/05/18  19:08:14  keffer
 * Ported to cfront V2.1.
 *
 * Revision 2.1  1993/05/18  00:40:21  keffer
 * Rewrote to use new messaging and exception handling facility.
 *
 */

#include "rw/toolerr.h"
#include "rw/generic.h"

RW_RCSID("Copyright (C) Rogue Wave Software --- $RCSfile: toolerr.cpp,v $ $Revision: 1 $ $Date: 4/02/96 13:46 $");

#undef  DECLARE_MSG
#define DECLARE_MSG(NAME, VALUE, MSG) \
        const RWCatMsg name2(RWTOOLMSG_,NAME) = {RWTOOL, VALUE, MSG};    \
        const RWMsgId  name2(RWTOOL_,NAME)    = &name2(RWTOOLMSG_,NAME);

const char RWTOOL[] = "rwtools6.0";

DECLARE_MSG(ALLOCOUT,  0x2000, "[ALLOCOUT] %s destructor called with allocation outstanding");
DECLARE_MSG(BADRE,     0x2001, "[BADRE] Attempt to use invalid regular expression");
DECLARE_MSG(CRABS,     0x2002, "[CRABS] RWFactory: attempting to create abstract class with ID %hu (0x%hx)");
DECLARE_MSG(FLIST,     0x2003, "[FLIST] Free list size error: expected %ld, got %ld");
DECLARE_MSG(ID,        0x2004, "[ID] Unexpected class ID %hu; should be %hu");
DECLARE_MSG(INDEX,     0x2005, "[INDEX] Index (%u) out of range [0->%u]");
DECLARE_MSG(LOCK,      0x2006, "[LOCK] Locked object deleted");
DECLARE_MSG(LONGINDEX, 0x2007, "[LONGINDEX] Long index (%lu) out of range [0->%lu]");
DECLARE_MSG(MAGIC,     0x2008, "[MAGIC] Bad magic number: %ld (should be %ld)");
DECLARE_MSG(NEVECL,    0x2009, "[NEVECL] Unequal vector lengths: %u versus %u");
DECLARE_MSG(NOCREATE,  0x200a, "[NOCREATE] RWFactory: no create function for class with ID %hu (0x%hx)");
DECLARE_MSG(NOTALLOW,  0x200b, "[NOTALLOW] Function not allowed for derived class");
DECLARE_MSG(READERR,   0x200c, "[READERR] Read error");
DECLARE_MSG(REF,       0x200d, "[REF] Bad persistence reference");
DECLARE_MSG(SEEKERR,   0x200e, "[SEEKERR] Seek error");
DECLARE_MSG(STREAM,    0x200f, "[STREAM] Bad input stream");
DECLARE_MSG(SUBSTRING, 0x2010, "[SUBSTRING] Illegal substring (%d, %u) from %u element RWCString");
DECLARE_MSG(UNLOCK,    0x2011, "[UNLOCK] Improper use of locked object");
DECLARE_MSG(WRITEERR,  0x2012, "[WRITEERR] Write error");

