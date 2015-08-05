/*************************************************************************

   Module:  mnttsk_hydromet.c

   Description:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/08/03 21:23:20 $
 * $Id: terrain_blockage_functions.c,v 1.1 2005/08/03 21:23:20 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <errno.h>
#include <sys/types.h>         /* open(),lseek()                          */
#include <sys/stat.h>          /* open()                                  */
#include <fcntl.h>             /* open()                                  */
#include <math.h>

#include <orpg.h>
#include <hydro_files.h>

/*****************************************************************************

   Description:
	Initializes the Beam Blockage files.

   Inputs: 

   Outputs:
   
   Returns:
	Negative value on error, or 0 on success.

*****************************************************************************/
int Initialize_beam_blockage(void){

    /* This is a stub function which does absolutely nothing. */
    return(0) ;

/*END of Initialize_beam_blockage()*/
}
