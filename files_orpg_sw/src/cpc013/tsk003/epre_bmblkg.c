/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/27 00:55:37 $
 * $Id: epre_bmblkg.c,v 1.2 2004/01/27 00:55:37 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include<stdio.h>
#include<rpg_globals.h>
#include"epreConstants.h"

extern char beamBlockage[BLOCK_RAD][MAX_RNG]; /* The bytes to be read into
                                                 beamBlockage array          */

/*****************************************************************************
    Routine c_read_blockage_file reads in the blockage information for
    the tilt from the linear buffer, which has been computed by
    mnttsk_hydromet.c. The format in the linear buffer is 3600x230 bytes
    for each message.
******************************************************************************/

void c_read_blockage_file(int elevangle)
{
 
 int azm, rng, index, debugit;
 int  status, elevOffset;
 char *blkdata;

 LB_id_t  msg_id;
 char blockage_default = '\0';    /* For the default value if msg_id not
                                     found in linear buffer                  */
 debugit=FALSE;
 index = 0;
 elevOffset = 10;
 msg_id = elevangle + elevOffset;

/* Read the Linear Buffer */

 status = ORPGDA_read( ORPGDAT_BLOCKAGE,
                       &blkdata,
                       LB_ALLOC_BUF,
                       msg_id );

 if(debugit)
    fprintf(stderr,"ORPGDA_read API return the value(status) is %d\t%d\n",
            status, (int)msg_id);

 if( status <= 0  ){
     /*when msg_id not found, use the default value */
     if ( status == LB_NOT_FOUND){
        for ( azm = 0; azm < BLOCK_RAD; azm++ )
            for( rng = 0; rng < MAX_RNG; rng++ )
               beamBlockage[azm][rng] = blockage_default;
      }

     else { /* in other cases, it will abort the whole task */
          PS_task_abort( "LB_read failed for %d. Status = %d. msg_id = %d\n",
                     ORPGDAT_BLOCKAGE, status, (int)msg_id );
      }
  }
 else {  /* if there is beam data in this mesg_id */

      for( azm = 0; azm < BLOCK_RAD; azm++ )
         for( rng = 0; rng < MAX_RNG; rng++ ){
             beamBlockage[azm][rng]=blkdata[index];
             index++;
          }
      free(blkdata);
  }

}
