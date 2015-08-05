/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/03/12 13:12:51 $
 * $Id: qia_readBlockage.c,v 1.1 2012/03/12 13:12:51 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/*** Local Include Files  ***/
#include <rpgc.h>
#include "qia.h"
/******************************************************************************
    Filename: qia_readBlockage.c

    Description:
    ============
       read_Blockage() reads in the blockage information for the tilt from the
    linear buffer, which has been computed by mnttsk_hydromet.c. The format
    in linear buffer is NORMAL resolution (3600 x 230 bytes) for each message.

    200902018 Ronald.G.Guenther@noaa.gov says that the terrain data files used
    for input to the blockage file generation process are 3600 ray by 230 bins
    per ray granularity = 360 tenth degrees out to 230 kms.

    Input:
       int elev_angle_tenths - elevation angle (in tenths of a degree)

    Output:
       char Beam_Blockage[][MX_RNG] - beam blockage for a single elevation.

    AEL 3.1.2

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----        -------    ----------         --------------------------
    12/12/06    0000       Cham Pham          Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).
    01/10/2012  0001       Brian Klein        NA11-00386.
******************************************************************************/

#define BLCKSZ ( MAXRADIALS * 10 * BLOCK_RNG )

void read_Blockage(int elev_angle_tenths, char Beam_Blockage[MAXRADIALS*10][BLOCK_RNG])
{
   int  azm, rng, index;
   int  status, elevOffset;
   char *blkdata;
   LB_id_t  msg_id;

   /* Initialize variables */

   index = 0;

   /* elevOffset is added to elev_angle_tenths to ensure that
    * msgid is never negative. */

   elevOffset = 10;

   /* Compute msg_id based on elevation angle (in tenths of a degree) */

   msg_id = elev_angle_tenths + elevOffset;

   /* Read beam blockage linear buffer based on msg_id */

   status = ORPGDA_read ( ORPGDAT_BLOCKAGE, &blkdata, LB_ALLOC_BUF, msg_id );

   /* When msg_id not found, initialize array to null value */

   if (status <= 0)
   {
      /* Instead of setting Beam_Blockage to 0, we could set
       * a flag to FALSE and pass the flag around. Check ORPGDAT_BLOCKAGE
       * at the start of every volume and malloc Beam_Blockage
       * on demand. Maybe revisit this if PBB is redone. */

      if(status == LB_NOT_FOUND)
      {
         memset ( Beam_Blockage, '\0', BLCKSZ * sizeof(char) );

      }
      else /* abort the whole task */
      {
         PS_task_abort ( "LB_read failed for %d. Status = %d. msg_id = %d\n",
                          ORPGDAT_BLOCKAGE, status, (int)msg_id );
      }
   }
   /* If there is beam data in this msg_id */
   else
   {
      for ( azm = 0; azm < (MAXRADIALS*10); ++azm )
      {
         for ( rng = 0; rng < BLOCK_RNG; ++rng )
         {
            Beam_Blockage[azm][rng] = blkdata[index];
            index++;

         } /* End rng loop */

      } /* End azm loop */

      if(blkdata != NULL)
         free(blkdata);

   } /* End if (status > 0) */



} /* end read_Blockage() ==================================== */
