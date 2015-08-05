
 /*
  * RCS info
  * $Author: steves $
  * $Locker:  $
  * $Date: 2014/11/13 20:14:16 $
  * $Id: orpgprq.c,v 1.16 2014/11/13 20:14:16 steves Exp $
  * $Revision: 1.16 $
  * $State: Exp *
  */

/************************************************************************
 *									*
 *	Module: orpgprq.c						*
 *									*
 *	Description:  This module contains a collection of functions	*
 *	dealing with product request processing.			*
 *									*
 ************************************************************************/



#include <infr.h>
#include <itc.h>
#include <orpg.h>
#include <orpgda.h>
#include <orpgvcp.h>
#include <orpgprq.h>
#include <rpg_vcp.h>
#include <math.h>
#include <stdarg.h>

/*	Global variables.						*/

static short Get_requested_elevation( short req_spec );
static int Find_closest_angle( float angle, float elev_angles[], 
                               int n_elevs );
static int Get_all_elevation_angles( int vcp, int num_cuts, 
                                     int vs_num, float *elev_angles,
                                     unsigned short *suppl );
static int Get_vcp_suppl_data( int vcp, int num_cuts, unsigned short *suppl );


/************************************************************************

    Expands the elevation specification in the product request into
    elevation angles.

    Input:
	vcp - The VCP number;
	ele_req - The request parameter for elevation;
	elevs_size - Size of the caller provided buffer "elevs".
        vs_num - Volume scan number ... valid range is [1,80].
                 Can also be volume sequence number.

    Output:
	elevs - The list of elevations. In .1 degrees. Negative elevation
	is negative.
        elev_inds - The list of corresponding RPG elevation index values.

    Return:
	The number of requested elevations on success or -1 on failure.

*************************************************************************/
int ORPGPRQ_get_requested_elevations( int vcp, short ele_req, int elevs_size, 
                                      int vs_num, short *elevs, short *elev_inds ) {

    short cntl_flags, req_spec;
    int n_elevs, cut_ctr, i;
    float elev_angles[MAX_ELEVATION_CUTS];
    unsigned short suppl[MAX_ELEVATION_CUTS];

    if (ele_req & ORPGPRQ_RESERVED_BIT)
        return (-1);

    cntl_flags = ele_req & ORPGPRQ_ELEV_FLAG_BITS;
    req_spec = ele_req & (~ORPGPRQ_ELEV_FLAG_BITS);

    if( vs_num < 0 )
       vs_num = 0;  

    if( vs_num > MAX_VSCAN ){
  
       vs_num %= MAX_VSCAN;
       if( vs_num == 0 )
          vs_num = MAX_VSCAN;

    }

    memset( &suppl[0], 0, MAX_ELEVATION_CUTS*sizeof(unsigned short) );
    n_elevs = Get_all_elevation_angles( vcp, MAX_ELEVATION_CUTS, 
                                        vs_num, elev_angles, suppl );
    if (n_elevs < 0) {
        LE_send_msg( GL_INFO, 
                     "ORPGPRQ: Getting elevation angles failed (vcp %d)\n", vcp);
        return (-1);
    }

    if (n_elevs > MAX_ELEVATION_CUTS) {
        LE_send_msg (GL_INFO, "ORPGPRQ: Unexpected too many elevations\n");
        return (-1);
    }

    /* Return the lowest "n_cuts" elevations.  The request specification
       is number of cuts. */
    if (cntl_flags == ORPGPRQ_LOWER_CUTS) {

        int n_cuts;

        n_cuts = req_spec;
        if (n_cuts > n_elevs)
            n_cuts = n_elevs;
        if (n_cuts > elevs_size) {
            LE_send_msg (GL_INFO, "ORPGPRQ: Elevation buffer too small\n");
            return (-1);
        }

        cut_ctr = 0;
        for (i = 0; i < n_elevs; i++){

            /* Supplemental cuts are not considered. */
            if ( suppl[i] & RDACNT_SUPPL_SCAN )
               continue;

            /* Increment the cut counter. */
            cut_ctr++;

            if( elev_angles[i] >= 0.0 )
               elevs[i] = (short)(elev_angles[i] * 10.f + .5f);
            else
               elevs[i] = (short)(elev_angles[i] * 10.f - .5f);
            if( elev_inds != NULL )
                elev_inds[i] = i + 1;

            /* If we have reached the limit, break. */
            if( cut_ctr == n_cuts )
               break;
        }
        return (n_cuts);
    }

    /* Return all elevation cuts in the VCP */
    else if (cntl_flags == ORPGPRQ_ALL_ELEVATIONS) {

        float angle;
        int ind, cnt;

        if (n_elevs > elevs_size) {
            LE_send_msg (GL_INFO, "ORPGPRQ: Elevation buffer too small\n");
            return (-1);
        }

        /* Case 1: No elevation angle specified.  Note: Supplemental cuts 
                   are not included. */
        if( req_spec == 0 ){

            ind = 0;
            for (i = 0; i < n_elevs; i++){

                if ( suppl[i] & RDACNT_SUPPL_SCAN )
                   continue;

                if( elev_angles[i] >= 0.0 )
                   elevs[ind] = (short)(elev_angles[i] * 10.f + .5f);
                else
                   elevs[ind] = (short)(elev_angles[i] * 10.f - .5f);
                if( elev_inds != NULL )
                    elev_inds[ind] = i + 1;

                ind++;

            }

            return (ind);

        }

        /* Case 2: An elevation angle is specified.  Note:  This mechanism
                   should be used to request supplemental cuts, like for 
                   example, when SAILS is active. */
        angle = (float)Get_requested_elevation (req_spec) * .1f;
        ind = Find_closest_angle( angle, elev_angles, n_elevs );

        if( elev_angles[ind] >= 0.0 )
           elevs[0] = (short)(elev_angles[ind] * 10.f + .5f);
        else
           elevs[0] = (short)(elev_angles[ind] * 10.f - .5f);
        if( elev_inds != NULL )
            elev_inds[0] = ind + 1;
        cnt = 1;

        /* Search the remaining angles for any match. */
        for( i = ind+1; i < n_elevs; i++ ){

           if( fabsf( elev_angles[ind] - elev_angles[i]) < 0.01 ){

               elevs[cnt] = elevs[0];
               if( elev_inds != NULL )
                   elev_inds[cnt] = i + 1;
               cnt++;

           }           

        }
        return(cnt);

    }

    /* Return all elevation cuts at or below "max_angle".  The request
       specification is elevation angle.  Note:  Supplemental cuts
       are not included, by design. */
    else if (cntl_flags == ORPGPRQ_LOWER_ELEVATIONS) {

        int max_angle, temp;
        int cnt;

        max_angle = Get_requested_elevation (req_spec);  /* In deg*10 */
        cnt = 0;
        for (i = 0; i < n_elevs; i++) {

            /* Convert VCP elevations to deg*10, rounding to the nearest 
               0.1 deg. */
            if( elev_angles[i] >= 0.0 )
               temp = (short)(elev_angles[i] * 10.f + .5f);
            else
               temp = (short)(elev_angles[i] * 10.f - .5f);

            /* If cut is above max_angle specified in request or the cut 
               is a supplemental cut, go to the next cut. */
            if ( (temp > max_angle) || (suppl[i] & RDACNT_SUPPL_SCAN) )
        	continue;

            if (cnt >= elevs_size) {
	        LE_send_msg (GL_INFO, "ORPGPRQ: Elevation buffer too small\n");
	        return (-1);
            }

            elevs[cnt] = temp;

            if( elev_inds != NULL )
                elev_inds[cnt] = i + 1;
            cnt++;

        }
        return (cnt);
    }

    /* Return the elevation angle closest to the angle requested. */
    else {

        float angle;
        int ind;

        if (n_elevs == 0)
            return (0);
        if (elevs_size <= 0) {
            LE_send_msg (GL_INFO, "ORPGPRQ: Elevation buffer too small\n");
            return (-1);
        }

        angle = (float)Get_requested_elevation (req_spec) * .1f;
        ind = Find_closest_angle( angle, elev_angles, n_elevs );

        if( elev_angles[ind] >= 0.0 )
           elevs[0] = (short)(elev_angles[ind] * 10.f + .5f);
        else
           elevs[0] = (short)(elev_angles[ind] * 10.f - .5f);
        if( elev_inds != NULL )
            elev_inds[0] = ind + 1;

        return (1);
    }
}

/************************************************************************

    Converts the elevation angle specification (always positive) in the 
    product request into numerical angles.

    Input:
	req_spec - The elevation specification in the product request;

    Return:
	The numerical elevation angle (positive or negative).

*************************************************************************/
static short Get_requested_elevation (short req_spec) {

    if (req_spec > 1800)
        return (req_spec - 3600);

    else
        return (req_spec);
}

/************************************************************************

    Finds the closest match of specified angle to angles in VCP. 

    Input:
	angle - The elevation angle to match.
        elev_angles - The elevation angles to search.
        n_elevs - The number of elevations to search.

    Return:
	The elevation index in the VCP of the angle with the closest
        match.

*************************************************************************/
static int Find_closest_angle( float angle, float elev_angles[], int n_elevs ){

    float min = 999.0, diff;
    int i, ind;

    ind = -1;

    for (i = 0; i < n_elevs; i++) {

        diff = elev_angles[i] - angle;
        if (diff < 0.f)
            diff = -diff;
        if (ind < 0 || diff < min) {
	    ind = i;
	    min = diff;
        }

    }

    return( ind );

}

/************************************************************************

   Gets all elevation angles for the specified VCP. 

   Inputs:
      vcp - VCP number.
      buffer_size - number of elements of array elev_angles.
      vs_num - volume scan number [1,80].
      elev_angle - buffer of size "buffer_size" to hold the angles,
                   in 0.1 deg.

   Outputs:
      elev_angle - elevation angles for VCP "vcp", in 0.1 degs.
      suppl - elevation cut supplemental flags

   Returns:
      Number of elevation angles in VCP.

************************************************************************/
static int Get_all_elevation_angles( int vcp, int buffer_size, 
                                     int vs_num, float *elev_angles,
                                     unsigned short *suppl ){

   VCP_ICD_msg_t *rdavcp = NULL;
   RDA_rdacnt_t *rdacnt = NULL;
   char *buf = NULL;
   int i, ind, ret, rda_num_elevs, num_elevs, elev_num, old_elev_num;

   static int initLB = 1;

   /* Check to make sure the volume scan number is valid, i.e., [1, 80]. */
   if( (vs_num < 0) || (vs_num > MAX_VSCAN) )
      ret = 0;

   else{ 

      /* This function can be called with negative vcp number.  In this
         case, get the vcp number from volume status. */
      if( vcp <= 0 )
         vcp = ORPGVST_get_vcp();

      /* Set the write permission in case this is the first access. */
      if( initLB ){

         ORPGDA_write_permission( ORPGDAT_ADAPTATION );
         initLB = 0;

      }

      /* Read the RDA_RDACNT data, if available.  If the vcp number matches
         use this data, otherwise, use the RDACNT data (via ORPGVCP 
         functions. ) */ 
      ret = ORPGDA_read( ORPGDAT_ADAPTATION, (void *) &buf, LB_ALLOC_BUF, 
                         RDA_RDACNT );

   }

   /* Some error occurred.  Use RDACNT data. */
   if( ret <= 0 ){

      if( (ret < 0) && (ret != LB_NOT_FOUND) )
         LE_send_msg( GL_ERROR, 
             "ORPGDA_read( ORPGDAT_ADAPTATION, RDA_RDACNT) Failed (%d)\n",
              ret );


      num_elevs = ORPGVCP_get_all_elevation_angles( vcp, buffer_size, 
                                                    elev_angles );
      Get_vcp_suppl_data( vcp, buffer_size, suppl );
      return( num_elevs );

   }

   /* Assign pointers. */
   rdacnt = (RDA_rdacnt_t *) buf;

   /* Determine which entry to use. */
   if( vs_num == 0 )
      ind = rdacnt->last_entry;

   else
      ind = vs_num % 2;

   rdavcp = (VCP_ICD_msg_t *) &rdacnt->data[ind].rdcvcpta[0];

   /* Extract the vcp number and check it against what was passed in. */
   if( rdavcp->vcp_msg_hdr.pattern_number != vcp ){

      /* Numbers don't match.  Use RDACNT data. */
      free(buf); 
      num_elevs = ORPGVCP_get_all_elevation_angles( vcp, buffer_size, 
                                                    elev_angles );
      Get_vcp_suppl_data( vcp, buffer_size, suppl );
      return( num_elevs );

   }

   /* VCP numbers match.   Return the elevation angles for the 
      current VCP. */
   rda_num_elevs = rdavcp->vcp_elev_data.number_cuts;
   num_elevs = 0;
   old_elev_num = elev_num = 0;
   for( i = 0; i < rda_num_elevs; i++ ){

      elev_num = rdacnt->data[ind].rdccon[i];

      /* Only interested in unique cuts.  Therefore only include
         once. */
      if( elev_num == old_elev_num )
         continue;

      /* Convert the angle, in BAMS, to degs. */
      elev_angles[num_elevs] = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE,
                                        rdavcp->vcp_elev_data.data[i].angle );
      suppl[num_elevs] = rdacnt->data[ind].suppl[i];

      num_elevs++;
      old_elev_num = elev_num;

      /* Have we reached the buffer size limit? */
      if( num_elevs >= buffer_size )
         break;

   }

   /* Free VCP buffer. */
   free(buf);

   return num_elevs;

}

/************************************************************************

   Gets VCP supplemental data for the specified VCP. 

   Inputs:
      vcp - VCP number.

   Outputs:
      suppl_flags - elevation cut supplemental flags

   Returns:
      The number of elevation cuts. 

************************************************************************/
static int Get_vcp_suppl_data( int vcp, int buffer_size, 
                               unsigned short *suppl_flags ){

   int ind, num_elevs = 0, max_ind = 0; 
   int new_ind = 0, old_ind = -1;
   VCP_ICD_msg_t *c_vcp = NULL;
   short *rdccon = NULL;
   float first_angle = -1.0;

   /* Get VCP from RPG. */
   if( (ind = ORPGVCP_index( vcp )) < 0 )
      return -1;

   if( (c_vcp = (VCP_ICD_msg_t *) ORPGVCP_ptr( ind )) == NULL )
      return -1;

   if( (rdccon = ORPGVCP_elev_indicies_ptr( ind )) == NULL )
      return -1;

   /* Do For All elevation cuts in the VCP ... */
   for( ind = 0; ind < c_vcp->vcp_elev_data.number_cuts; ind++ ){

      int waveform = (int) c_vcp->vcp_elev_data.data[ind].waveform;
      int phase  = (int) c_vcp->vcp_elev_data.data[ind].phase;
      int super_res = (int) c_vcp->vcp_elev_data.data[ind].super_res;
      float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE,
                                              c_vcp->vcp_elev_data.data[ind].angle );

      /* Need to stop when the buffer size is reached. */
      if( max_ind >= buffer_size )
         break;

      /* Only care about unique cuts. */
      new_ind= rdccon[ind];
      if( new_ind == old_ind )
         continue;

      /* Set waveform bit. */
      if( waveform == VCP_WAVEFORM_CS )
         suppl_flags[num_elevs] |= RDACNT_IS_CS;

      else if( waveform == VCP_WAVEFORM_CD )
         suppl_flags[num_elevs] |= RDACNT_IS_CD;

      else if( waveform == VCP_WAVEFORM_CDBATCH )
         suppl_flags[num_elevs] |= RDACNT_IS_CDBATCH;

      else if( waveform == VCP_WAVEFORM_BATCH )
         suppl_flags[num_elevs] |= RDACNT_IS_BATCH;

      else if( waveform == VCP_WAVEFORM_STP )
         suppl_flags[num_elevs] |= RDACNT_IS_SPRT;

      /* Set phase bit. */
      if( phase == VCP_PHASE_SZ2 )
         suppl_flags[num_elevs] |= RDACNT_IS_SZ2;

      /* Set super resolution  bit. */
      if( super_res & VCP_HALFDEG_RAD )
         suppl_flags[num_elevs] |= RDACNT_IS_SR;

      /* Until I figure out a better way to determine supplemental
         cuts, if the elevation angle matches the lowest cut, it
         is supplemental. */
      if( (rdccon[ind] > 1) && (elev_angle == first_angle) )
         suppl_flags[num_elevs] |= RDACNT_SUPPL_SCAN;

      /* Set old_ind to new_ind. */
      old_ind = new_ind;
 
      /* Increment the number of elevations. */
      num_elevs++;

   }

   return( num_elevs );

}
