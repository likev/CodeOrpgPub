/**************************************************************************

      Module:  inet_nultoa.c

 Description:
        This file defines a simple-minded replacement for the inet_ntoa()
	utility, which does not conform to any standard.  "nultoa" ==
	"Network Unsigned Long TO Ascii".

 Interruptible System Calls:

	None

 Memory Allocation:
	The pointer returned to the calling process points to static
	memory within this routine.

 Assumptions:
	Network byte-order is Big Endian.

 **************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/05/27 16:45:54 $
 * $Id: misc_inet_nultoa.c,v 1.2 2004/05/27 16:45:54 jing Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 * $Log: misc_inet_nultoa.c,v $
 * Revision 1.2  2004/05/27 16:45:54  jing
 * Update
 *
 * Revision 1.1  2000/09/12 17:46:28  jing
 * Initial revision
 *
 * Revision 1.6  1997/09/22 20:13:45  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.3  1997/08/22 09:43:08  eforren
 * Solaris x86 port
 *
 * Revision 1.2  96/08/27  16:38:15  16:38:15  jing (Zhongqi Jing)
 * modefy
 * 
 * Revision 1.1  1996/06/04 16:41:43  cm
 * Initial revision
 *
 * 
 */

/*
 * System Include Files/Local Include Files
 */
#include <config.h>
#include <stdio.h>             /* sprintf                                 */

#include <misc.h>              /* MISC_bswap                              */

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define INADDR_STRING_LEN 16

/**************************************************************************
 Description: Provided a network byte-ordered unsigned long internet
              address, return a pointer to the string representation of the
              "dotted" IP address.
       Input: source Internet protocol address
      Output: none
     Returns: pointer to static string representation of "dotted" IP address
     Globals: none
       Notes: We assume that network byte-order is Big Endian.
              This routine replaces inet_ntoa(), which does not conform to
              any known standard.
 **************************************************************************/
char *MISC_inet_nultoa(unsigned long inaddr)
{
   static char addr_s[INADDR_STRING_LEN] ;
   unsigned char *byte_p ;

   /*
    * Network-order is Big Endian ... bytes are actually swapped only
    * if we are compiled for a Little Endian machine ...
    */
      byte_p = (unsigned char *) &inaddr ;
      (void) sprintf(addr_s,"%d.%d.%d.%d",
                     *(byte_p+0),
                     *(byte_p+1),
                     *(byte_p+2),
                     *(byte_p+3)) ;

   addr_s[INADDR_STRING_LEN-1] = 0x00 ;

   return(addr_s) ;

/*END of MISC_inet_nultoa()*/
}
