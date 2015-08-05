/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/03/08 17:21:27 $
 * $Id: qperate_read_Blockage.c,v 1.6 2011/03/08 17:21:27 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/*** Local Include Files  ***/

#include "qperate_func_prototypes.h"

/******************************************************************************
    Filename: qperate_read_Blockage.c

    Description:
    ============
       read_Blockage() reads in the blockage information for the tilt from the
    linear buffer, which has been computed by mnttsk_hydromet.c. The format
    in linear buffer is NORMAL resolution (3600 x 230 bytes) for each message.

    200902018 Ronald.G.Guenther@noaa.gov says that the terrain data files used
    for input to the blockage file generation process are 3600 ray by 230 bins
    per ray granularity = 360 tenth degrees out to 230 kms.

    BLOCK_RAD = 3600
    MX_RNG    =  230

    Input:
       int elev_angle_tenths - elevation angle (in tenths of a degree)

    Output:
       char Beam_Blockage[][MX_RNG] - beam blockage for a single elevation.

    AEL 3.1.2.2

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----        -------    ----------         --------------------------
    12/12/06    0000       Cham Pham          Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).
******************************************************************************/

#define BLCKSZ ( BLOCK_RAD * MX_RNG )

void read_Blockage(int elev_angle_tenths, char Beam_Blockage[BLOCK_RAD][MX_RNG])
{
   int  azm, rng, index;
   int  status, elevOffset;
   char *blkdata;
   LB_id_t  msg_id;

   #ifdef QPERATE_DEBUG
   {
      fprintf ( stderr, "Beginning read_Blockage() .........\n" );
      fprintf ( stderr, "elev_angle_tenths = %d\n", elev_angle_tenths );
   }
   #endif

/*
printf ("INSIDE READ BLOCKAGE. \n");
fprintf (stderr, "INSIDE READ BLOCKAGE. \n");
*/

   /* Initialize variables */

   index = 0;

   /* elevOffset is added to elev_angle_tenths to ensure that
    * msgid is never negative. */

   elevOffset = 10;

   /* Compute msg_id based on elevation angle (in tenths of a degree) */

   msg_id = elev_angle_tenths + elevOffset;

   /* Read beam blockage linear buffer based on msg_id */

   status = ORPGDA_read ( ORPGDAT_BLOCKAGE, &blkdata, LB_ALLOC_BUF, msg_id );

   #ifdef QPERATE_DEBUG
   {
      fprintf ( stderr, "ORPGDA_read API return the value(status) is %d\t%d\n",
                        status, (int)msg_id );
   }
   #endif

   /* When msg_id not found, initialize array to null value */

   if (status <= 0)
   {
/*
printf ("Status is less than I'd hoped for\n");
fprintf (stderr, "Status is less than I'd hoped for\n");
*/

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
/*
printf ("ABORT!  ABORT!  ABORT!\n");
fprintf (stderr, "ABORT!  ABORT!  ABORT!\n");
*/

         PS_task_abort ( "LB_read failed for %d. Status = %d. msg_id = %d\n",
                          ORPGDAT_BLOCKAGE, status, (int)msg_id );
      }
   }
   /* If there is beam data in this msg_id */
   else
   {
      for ( azm = 0; azm < BLOCK_RAD; ++azm )
      {
         for ( rng = 0; rng < MX_RNG; ++rng )
         {
            Beam_Blockage[azm][rng] = blkdata[index];
            index++;

         } /* End rng loop */

      } /* End azm loop */

      if(blkdata != NULL)
         free(blkdata);

   } /* End if (status > 0) */

/* 12 July 2010 djsiii Adding some debug output.  remove this later */

/*
   printf ( "The blockage data for radial 72.0:\n");
   for ( rng = 0; rng < MX_RNG; ++rng )
   {
         printf ( " %6d",  Beam_Blockage[720][rng]);
         printf ( "\n");
   }
*/

   #ifdef QPERATE_DEBUG
      for (azm = 0; azm < BLOCK_RAD; ++azm)
      {
         for (rng = 0; rng < MX_RNG; ++rng)
          {
              if(Beam_Blockage[azm][rng] == 0)
              {
                 fprintf (stderr, "%d) [%d][%d] %d\n",
                          elev_angle_tenths, azm, rng, Beam_Blockage[azm][rng]);
              }
          }
      }
      fprintf ( stderr, " \nEnd read_blockage() .........\n" );
   #endif

} /* end read_Blockage() ==================================== */
