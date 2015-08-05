/*******************
  RCS info
  $Author: chrisg $
  $Locker:  $
  $Date: 2002/07/30 18:40:17 $
  $Id: mngdskerr_lnux_x86.c,v 1.1 2002/07/30 18:40:17 chrisg Exp $
  $Revision: 1.1 $
  $State: Exp $

  Do nothing for x86 hardware. Kernel reporting is dependant on the O.S.
  This will need to be researched for x86 hardware if it is even needed.

********************/

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

int poll_time = 15;

int main (int argc, char **argv) {
   
   do {

      sleep ((u_int)poll_time);

   } while (1);

   /* NOTREACHED */

}
