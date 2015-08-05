/*
 * RCS info.
 * $Author: ccalvert $
 * $Date: 2009/10/27 22:48:08 $
 * $Locker:  $
 * $Id: dp_dua_accum_Main.c,v 1.4 2009/10/27 22:48:08 ccalvert Exp $
 * $State: Exp $
 * $Revision: 1.4 $
 */

#include "dp_dua_accum_consts.h" 
#include "dp_dua_accum_types.h"
#include "dp_dua_accum_func_prototypes.h"
#include "dp_dua_accum_common.h"

/******************************************************************************
   Filename: dp_dua_accum_Main.c

   Description:
   ============
      It does all the RPG registrations and initializations, then enters into
      a loop control of request awaiting. If there are user requests, it will
      retrieve the requests and generate products accordingly. 		

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2008    0000       Zhan Zhang         Initial implementation
******************************************************************************/

int main(int argc, char * argv[])
{
    /* Register a volume based accum prod as an input */
    RPGC_reg_inputs(argc, argv); 

    /* Register output */
    RPGC_reg_outputs(argc, argv);

    /* Register for Scan Summary */
    RPGC_reg_scan_summary();

    /* Task timing */
    RPGC_task_init( VOLUME_BASED, argc, argv );  

    /* ACL  */
    while (1)
    {
        RPGC_wait_act(WAIT_DRIVING_INPUT);
        task_handler();

    }

    return 0;
}  /* end of main() */

