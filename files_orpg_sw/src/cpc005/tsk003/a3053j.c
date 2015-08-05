/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/10/18 15:31:18 $
 * $Id: a3053j.c,v 1.4 2005/10/18 15:31:18 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
#include <prfselect.h>

#define STOP    1
#define ABORT   2

/*\////////////////////////////////////////////////////////////////////////

   Description:
      Acquires radials until start of elevation detected.  After detected,
      performs abort for remaining volume scan.

////////////////////////////////////////////////////////////////////////\*/
int A3053J_dummy_processor(){

 
   /* Variable initializations. */
   int radial_status = INT_ELEV;
   int status = STOP;
   int istat;
   char *inbuf;
 
   /* DO UNTIL Radial Status = `End_of_Volume', or Abort condition occurs
      Acquire input buffer of radial data. */
   while(1){
 
      inbuf = (char *) RPGC_get_inbuf( BASEDATA, &istat );
      if( istat == NORMAL ){
 
         /* If input buffer operation successful, extract Radial Status. */
         Base_data_header *bhd = (Base_data_header *) inbuf;

         radial_status = bhd->status;
 
         /* Release the input buffer. */
         RPGC_rel_inbuf( inbuf );
 
         /* If beginning of next elevation cut, abort radial processing for the
            remainder of the volume scan. */
         if( radial_status == BEG_ELEV ){ 

            RPGC_abort_remaining_volscan();
            status = ABORT;

         }
 
         /* End DO UNTIL (Radial Status =`End_Vol' or Program Status =`Abort'). */
         if( (radial_status != END_VOL) 
                         &&
             ( status != ABORT ) )

            continue;

         break;

      }
      else{ 

         /* Else if input buffer not successfully acquired, Abort processing. */
         RPGC_abort();
         status = ABORT;
         break;

      }

   } /* End of while(1) loop. */
   
   return 0;

} /* End of A3053J_dummy_processor() */
