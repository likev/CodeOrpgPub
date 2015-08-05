/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:21:00 $
 * $Id: output_mpda_data.c,v 1.8 2009/10/27 22:21:00 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/**************************************************************************

   output_mpda_data.c

   PURPOSE:

   Converts mpda output velocity data back to Unisys biased integer format
   a radial at a time

   CALLED FROM:

   mpda_buf_cntrl.c

   INPUTS:

   Arrays for header data, reflectivity, velocity, and spectrum width
   short *radstat - radial status

   CALLS:

   vel_float_to_UIF
   
   OUTPUTS:

   None.

   RETURNS:

   None.

****************************************************************************/
#include <memory.h>
#include <generic_basedata.h>
#include <vcp.h>
#include "mpda_constants.h"
#include "mpda_structs.h"

unsigned short vel_short_to_UIF(short velocity);

struct mpda_data save;
struct dp_data_t save_dp;

int output_mpda_data( int i, void *out_buf, int *size ){

   Base_data_header *hdr = (Base_data_header *) out_buf;
   int radial_size, j, offset;
   unsigned short *refl, *vel, *spw;

   /* These statements copy the header, reflectivity and spectrum 
      width (from PRF1) into the output buffer */
   memcpy(hdr, &save.hskp_buff[i], BASEDATA_HD_SIZE * sizeof(short));

   refl = (unsigned short *) (((char *) out_buf) + hdr->ref_offset );
   spw = (unsigned short *)  (((char *) out_buf) + hdr->spw_offset );

   memcpy( refl, &save.ref_buff[i], hdr->n_surv_bins * sizeof(Moment_t) );
   memcpy( spw, &save.final_sw[i], hdr->n_dop_bins * sizeof(Moment_t) );

   /*  Add extra moments to ouput buffer */
   if(hdr->no_moments > 0)
     {
     offset = hdr->offsets[0];
     memcpy( out_buf + offset, &save_dp.dp_data[i][0], (save_dp.dp_end[i] - save_dp.dp_start[i]) );
     }

   /* Set the velocity dealiased bit in the header data */
   hdr->msg_type |= VEL_DEALIASED_BIT;

   /* This loop puts the final velocity field back into UIF format 
      for downstream compatibility */
   vel = (unsigned short *) (((char *) out_buf) + hdr->vel_offset );
   for( j = 0; j < hdr->n_dop_bins; ++j )
      vel[j] = vel_short_to_UIF(save.final_vel[i][j]);
 
   /* Determine the size of the output radial message and set the 
      corrrect size in the message header. */
   radial_size = hdr->ref_offset + hdr->n_surv_bins*sizeof(Moment_t);

   if( (hdr->vel_offset > hdr->ref_offset)
                       ||
       (hdr->spw_offset > hdr->ref_offset) ){

      if( hdr->vel_offset > hdr->spw_offset )
         radial_size = hdr->vel_offset + hdr->n_dop_bins*sizeof(Moment_t);

      else
         radial_size = hdr->spw_offset + hdr->n_dop_bins*sizeof(Moment_t);

   }
   
   if(hdr->no_moments > 0)
     {
     radial_size = save_dp.dp_end[i];
     }

   /* Make sure the size is a multiple of ALIGNED_LENGTH otherwise
      FORTRAN consumers may fail (if the data is written to shared
      memory LB). */
   radial_size = ALIGNED_T_SIZE(radial_size)*ALIGNED_LENGTH;
   hdr->msg_len = radial_size/sizeof(short);
   *size = hdr->msg_len*sizeof(short);

   return (int) hdr->status;

}
