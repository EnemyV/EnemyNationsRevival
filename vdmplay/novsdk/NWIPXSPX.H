/*    (c) COPYRIGHT 1990,1991 by Novell, Inc.  All Rights Reserved.   */
/*    This header file will allow you to include one header whether you
      program for DOS or Windows
*/

#if defined(WINDOWS) || defined(NWWIN)

   #include <nxtw.h>
   #include <nwsap.h>
   #include <nwdiag.h>

#else /* not windows */

   #include <nxtd.h>
   #include <sap.h>
   #include <diag.h>

#endif /* WINDOWS or NWWIN */
