/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 18:43:19 $
 * $Id: apply_mpda.c,v 1.7 2009/02/27 18:43:19 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/***************************************************************************

   apply_mpda.c

   PURPOSE:

   This routine is the main driver for MPDA processing. It is called from the   
   mpda_buf_cntrl.ftn driver program. This routine make calls to save the data
   from the individual PRFs, and calls the routine to do the processing. 

   CALLED FROM:

   mpda_buf_cntrl.c

   INPUTS:

   short int arrays containing reflectivity and velocity data
   pointers to radial status and azimuth number

   CALLS:

   save_mpda_data
   initialize_lookup_tables
   get_derived_params
   fill_radials
   process_mpda_scans

   OUTPUTS:

   None. 

   RETURNS:

   None.

   NOTES:

   HISTORY:

   B. Conway, 5/96	- original development
   B. Conway, 5/00	- cleanup
   D. Zittel, 10/01	- modified to interface to orpg
   
****************************************************************************/

#define GLOBAL_DEFINED

#include <a309.h>
#include <basedata.h>
#include <rdacnt.h>
#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_vcp_info.h"
#include <veldeal.h>

/* Flag to tell when to update unfold table; currently only true to start
   program */

Base_data_header rpg_hdr;

int initialize_lookup_tables(int rng, int rng_nyq_prod);
void save_mpda_data();
void process_mpda_scans();
void fill_radials(int *PCT_OBS_VOL_TIME);
void get_derived_params();

int
apply_mpda( void *input_ptr )
{

  static int make_table = TRUE;
  int radstat, cut_cnt;

  Base_data_header *bhd = (Base_data_header *) input_ptr;
  unsigned short *refl = (unsigned short *) (((char *) bhd) + bhd->ref_offset);
  unsigned short *vel = (unsigned short *) (((char *) bhd) + bhd->vel_offset);
  unsigned short *spw = (unsigned short *) (((char *) bhd) + bhd->spw_offset);

/* 
   Process as an mpda scan; set the count for the current cut, the radial
   status, azimuth number, and the current number of PRFs 
*/

  cut_cnt = rpg_hdr.elev_num - 1;
  radstat = rpg_hdr.status;
  num_prfs = vcp.vel_num[cut_cnt];

/*
  Save the volume scan start time (seconds past midnight) and volume scan
  start date (modified Julian)
*/
  if( (radstat == GOODBVOL) && (rpg_hdr.elev_num == 1) ){

     Pct.vol_time = rpg_hdr.time/1000;
     Pct.vol_date = rpg_hdr.date;

  }

/*
   Save the current radial of data
*/

  save_mpda_data(refl,vel,spw);  

/*
   If we have valid velocity data and need to make the table, and this is the
   first velocity cut to start the volume, initialize the unfold lookup table.
*/
  if(make_table)
    if( radstat == GOODBVOL || radstat == GOODBEL )
      if(num_prfs >= 1)
        {
        make_table = initialize_lookup_tables((rpg_hdr.unamb_range + 5)/INT_10,
                     (rpg_hdr.unamb_range * rpg_hdr.nyquist_vel + 50)/INT_100);
        old_resolution = MISSING;
        }

/*
   If the radial status shows this to be the end of the scan and we have more
   than one PRF, get parameters derived from the data, fill in holes left by
   radial alignment, and start processing the MPDA data
*/ 

  if(radstat == END_ELEV || radstat == END_VOL)
    {
    init_prf_arry[num_prfs] = FALSE;
      
    if(vcp.last_scan_flag[cut_cnt])
      {
      get_derived_params();
      fill_radials( &Pct.vol_time );
      process_mpda_scans();
      }
    }

    return radstat;
}
