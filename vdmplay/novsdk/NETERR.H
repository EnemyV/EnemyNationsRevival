/* Copyright (C) 1988 by Novell, Inc.  All Rights Reserved.  Mod:  6/13/90 */

#define NERR_Success			0

/* other errors (2, 5, 6, 87, 124) defined in bseerr.h (std. OS/2 defs) */

#define	NERR_BASE			2100

#define NERR_NetNotStarted		(NERR_BASE+2)
#define NERR_UnknownServer		(NERR_BASE+3)
#define NERR_UnknownDevDir		(NERR_BASE+16)
#define NERR_BufTooSmall		(NERR_BASE+23)
#define NERR_LanmanIniError		(NERR_BASE+31)
#define NERR_InternalError		(NERR_BASE+40)
#define NERR_ServiceInstalled		(NERR_BASE+82)
#define NERR_ServiceNotInstalled	(NERR_BASE+84)
#define NERR_NetNameNotFound		(NERR_BASE+210)
