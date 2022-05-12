/*
 * Messaging facility
 *
 * $Id: message.cpp,v 1.3 1993/09/10 03:59:57 keffer Exp $
 *
 ****************************************************************************
 *
 * Rogue Wave Software, Inc.
 * P.O. Box 2328
 * Corvallis, OR 97339
 * Voice: (503) 754-3010	FAX: (503) 757-6650
 *
 * Copyright (C) 1989 - 1993.
 * This software is subject to copyright protection under the laws of
 * the United States and other countries.
 *
 ***************************************************************************
 *
 * These routines rationalize the different messaging routines available.
 *
 ***************************************************************************
 *
 * $Log: /vdmplay/MESSAGE.CPP $
 * 
 * 1     4/02/96 13:46 Vadim
 * Revision 1.3  1993/09/10  03:59:57  keffer
 * Added RW_RCSID macro.
 *
 * Revision 1.2  1993/05/31  21:46:32  keffer
 * New messaging architecture for localization
 *
 * Revision 1.1  1993/05/18  00:45:50  keffer
 * Initial revision
 *
 *
 */

#include "rw/message.h"

RW_RCSID("Copyright (C) Rogue Wave Software --- $RCSfile: message.cpp,v $ $Revision: 1 $ $Date: 4/02/96 13:46 $");

/* For sun, stdio.h must be included BEFORE stdarg.h;
   For Glock, it must be included AFTER stdarg.h. */

STARTWRAP
#ifndef __GLOCK__
# include <stdio.h>
#endif
#include <stdarg.h>
#ifdef __GLOCK__
# include <stdio.h>
#endif
#include <string.h>
ENDWRAP

#define RWXMSGBUFSIZE 504

#if !defined(RW_MESSAGE) || (RW_MESSAGE==RW_NOMSG)
   const char* rwexport rwMsgLookup(RWMsgId msgId)
   {
     return msgId->msg_;
   }
#endif


#if defined(RW_MESSAGE) && (RW_MESSAGE==RW_CATGETS)
#  include <nl_types.h>
   const char* rwexport rwMsgLookup(RWMsgId msgId)
   {
     nl_catd descriptor = ::catopen((char*)msgId->domainName_, 0);
     const char* ret = ::catgets(descriptor, 0, msgId->msgNumber_, (char*)msgId->msg_);
     ::catclose(descriptor);
     return ret;
   }
#endif

#if defined(RW_MESSAGE) && (RW_MESSAGE==RW_GETTEXT)
#  include <libintl.h>
   const char* rwexport rwMsgLookup(RWMsgId msgId)
   {
     ::textdomain(msgId->domainName_);
     return ::gettext(msgId->msg_);
   }
#endif


#if defined(RW_MESSAGE) && (RW_MESSAGE==RW_DGETTEXT)
#  include <libintl.h>
   const char* rwexport rwMsgLookup(RWMsgId msgId)
   {
     return ::dgettext(msgId->domainName_, msgId->msg_);
   }
#endif

RWMessage::RWMessage(const char* str)
{
  RWPRECONDITION(str!=0);
  msg_ = new char[strlen(str)+1];
  strcpy(msg_, str);
}

RWMessage::RWMessage(RWMsgId msgId ...)
{
  va_list arglist;
  va_start(arglist, msgId);

  char msgbuf[RWXMSGBUFSIZE];
  vsprintf(msgbuf, rwMsgLookup(msgId), arglist);

  // Sanity check:
  RWASSERT(strlen(msgbuf) < RWXMSGBUFSIZE);

  msg_ = new char[strlen(msgbuf)+1];
  strcpy(msg_, msgbuf);

#ifdef RW_MSC_BACKEND
  arglist = 0;
#else
  va_end(arglist);
#endif
}

RWMessage::RWMessage(const RWMessage& msg)
{
  RWPRECONDITION(msg.msg_!=0);
  msg_ = new char[strlen(msg.msg_)+1];
  strcpy(msg_, msg.msg_);
}

RWMessage::~RWMessage()
{
  delete msg_;
}

RWMessage&
RWMessage::operator=(const RWMessage& msg)
{
  if(this != &msg)
  {
    RWPRECONDITION(msg.msg_!=0);
    delete msg_;
    msg_ = new char[strlen(msg.msg_)+1];
    strcpy(msg_, msg.msg_);
  }
  return *this;
}
