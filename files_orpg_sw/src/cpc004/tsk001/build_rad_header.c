/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:21:00 $
 * $Id: build_rad_header.c,v 1.5 2009/10/27 22:21:00 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/**************************************************************************

   build_rad_header.c

   PURPOSE:

   This saves radial header data to a houskeeping buffer and saves the
   azimuth sine and cosine values to a lookup table.  It also sets variables
   needed by the fortran buffer control module. 

   CALLED FROM:

   mpda_buf_cntrl.c

   INPUTS:

   Base_data_header *hdr  - Pointer to radial header data 
   length	  	  - length of input buffer

   CALLS:

   None.

   OUTPUTS:

   None.

   RETURNS:

   *el_num         - cut number of the current radial
   *rpg_ind        - unique elevation number of the current radial
   *rad_stat       - status of the current radial

   HISTORY:
   W. Zittel  10/09	 Modified to saving dual polarization fields for 1st
			 	Doppler cut
   R. May      6/02      Changed to use simpler approach and memcpy
   W. Zittel   1/02      Original Development
               
               
*********************************************************************/

#include <memory.h>
#include <generic_basedata.h>
#include <rdacnt.h>
#include <vcp.h>
#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_vcp_info.h"
#include "mpda_trig_arrays.h"

#include <a309.h>

struct mpda_data save;
struct trig_arrays radscan;
Base_data_header rpg_hdr;
Generic_moment_t mom1;

void build_rad_header( Base_data_header *hdr, int *el_num, int *rpg_ind,
                       int *rad_stat, int length )
{

/*
    If current elevation cut does not require MPDA processing then return
    immediately after setting elevation number, unique elevation number, and
    radial status.
*/

   *el_num = hdr->elev_num;
   *rpg_ind = hdr->rpg_elev_ind;
   *rad_stat = hdr->status;

   if(vcp.cuts_per_elv[*rpg_ind -1] == 1)
     return;

   memcpy(&rpg_hdr, hdr , sizeof(Base_data_header));
/*
   If this radial is the from the first velocity cut, then save the header
   information for loading into the output buffer and save the cosine and sine
   of the azimuth angle  
*/

   if(vcp.vel_num[*el_num - 1] == 1)
      {
      radscan.sinazim[rpg_hdr.azi_num - 1] = hdr->sin_azi;
      radscan.cosazim[rpg_hdr.azi_num - 1] = hdr->cos_azi;
      memcpy(save.hskp_buff[rpg_hdr.azi_num-1], &rpg_hdr, sizeof(Base_data_header));

      /* If first radial, initialize mpda dual pol structure elements */
      if(rpg_hdr.azi_num ==1 && hdr->no_moments > 0)
         {
         memset(save_dp.dp_data,0,sizeof(save_dp.dp_data)); 
         }

         save_dp.dp_start[rpg_hdr.azi_num-1] = hdr->offsets[0];
         save_dp.dp_end[rpg_hdr.azi_num-1] = length;

      unsigned short *mom1 = (unsigned short *) (((char *) hdr) + hdr->offsets[0]);
      if(hdr->no_moments > 0)
         {
         memcpy(save_dp.dp_data[rpg_hdr.azi_num-1], mom1, 
                save_dp.dp_end[rpg_hdr.azi_num-1] - save_dp.dp_start[rpg_hdr.azi_num-1] );
         }

      }
}
