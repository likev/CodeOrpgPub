/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 23:08:34 $
 * $Id: dp_elev_add_radial.c,v 1.3 2009/10/27 23:08:34 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include "dp_elev_func_prototypes.h"

/******************************************************************************
   Filename: dp_elev_build_product.c

   Description:
   ============
   Add_radial() adds 1 radial to the output buffer.

   Inputs: char* inbuf            - input data = Base_data_header + data
           int*  num_out_of_range - number of radials out of range
           int*  num_duplicates   - number of duplicated radials

   Outputs: Compact_dp_basedata_elev* cdbe - output buffer

   Note: HCA puts out the input buffer not as a structure, but as a
         Base_data_header followed by a bunch of characters (the moments).

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   -----------   -------    -----------------  ------------------------------
   05 Oct 2007    0000       Jihong Liu        Initial implementation
   19 May 2008    0001       James Ward        The old way was to read the
                                               moments in one after another.
                                               The problem is that if the ROC
                                               changes the order, you have to
                                               modify the code. The new way
                                               bases it on the moment name.
******************************************************************************/

void Add_radial(char* inbuf, Compact_dp_basedata_elev* cdbe,
                int* num_out_of_range, int* num_duplicates)
{
   short radnum = 0;             /* radial number, to be determined */
   int   i;                      /* counter                         */
   int   num_output_moments = 0; /* number of moments collected     */
   char  msg[200];               /* stderr message                  */

   Generic_moment_t* input_moment  = NULL;
   Generic_moment_t* output_moment = NULL;

   Base_data_header* hdr = (Base_data_header *) inbuf;

   /* 20071003 Jim Ward - The radials may not be presented in order. In
    * some data, the last radial is presented first. We get the azimuth from
    * the header, which should be in .5 degree and round it down.
    * For example, azimuth 0.5 -> radnum 0, ... azimuth 359.5 -> radnum 359.
    *
    * 20071003 Jim Ward - even though basedata_elev.h MAX_RADIALS_ELEV
    * goes out to 400, we are ignoring all above 360 (MAX_AZM). We have
    * never seen more than 361 (see below)
    *
    * 20071009 Jim Ward - Cham says that it's better to use hdr->azi_num
    * than hdr->azimuth. azi_num starts at 1 instead of 0. hca may set this.
    *
    * 20080612 Jim Ward - Take the first radial we get, and don't take any
    * duplicated radials. This is to detect errors from SuperRes
    * recombination.
    *
    * 20081105 Jim Ward - Chris Calvert said that Super Res may send 361
    * radials for some elevations. Replacing the 0th radial with the 361st
    * didn't make any difference in the output products via a prod_cmpr, but
    * it did screw up the qperate volume time computations as our start time
    * is based on the time of the first radial in the array. So leave the
    * code as it is now, discarding the 361st radial
    *
    * 20090209 Jim Ward - Brian Klein says that it's better to use
    * hdr->azimuth (the angle) than hdr->azi_num (the number of the radial
    * indexed from where the radar started). In the Super Res dual pol data
    * azimuth != azi_num */

   /* Old: radnum = hdr->azi_num - 1;
    *
    * We round down instead of nearest int, so 10.5 -> 10 not 11. */

   radnum = (short) hdr->azimuth;

   if((radnum > -1) && (radnum < MAX_AZM)) /* in range */
   {
      /* When we initialize an elevation, we set radial_is_good[radnum]
       * to FALSE for all radials. When a new radial comes in,
       * we set radial_is_good[radnum] to TRUE, and if it comes in again
       * and radial_is_good is TRUE, then we know it is a duplicate */

      if(cdbe->radial_is_good[radnum] == TRUE) /* duplicated */
      {
         (*num_duplicates)++;

         if(DP_ELEV_PROD_DEBUG)
         {
            sprintf(msg, "hdr->azi_num: %d, radnum %d already processed\n",
                    hdr->azi_num, radnum);

            RPGC_log_msg(GL_INFO, msg);
            fprintf(stderr, msg);
         }

         return;

      } /* end if radial has already been seen */
      else /* a new radial */
      {
         cdbe->radial_is_good[radnum] = TRUE;
      }
   } /* end if radial is in range */
   else /* out of range */
   {
      (*num_out_of_range)++;

      if(DP_ELEV_PROD_DEBUG)
      {
         sprintf(msg, "hdr->azi_num: %d, radnum %d < 0 or > %d, "
                      "can't save radial\n",
                       hdr->azi_num, radnum, MAX_AZM - 1);

         RPGC_log_msg(GL_INFO, msg);
         fprintf(stderr, msg);
      }

      return;

   } /* end if radial is out of range */

   /* Copy the Base_data_header */

   memcpy(&(cdbe->radial[radnum].bdh), inbuf, BASEDATA_HDR_SIZE);

   for(i = 0; i < hdr->no_moments; i++)
   {
      /* Point to the next input moment */

      input_moment = (Generic_moment_t*) (inbuf + hdr->offsets[i]);

      /* We collect all 11 input moments. We moved HCA to the
       * end of the if-list because it has a filter applied,
       * with lots of comments. */

      if(strncmp(input_moment->name, "DKDP", 4) == 0)
      {
        output_moment = (Generic_moment_t*)
                         &(cdbe->radial[radnum].kdp_moment);
        memcpy(output_moment, input_moment, KDP_SIZE);
      }
      else if(strncmp(input_moment->name, "DML" , 3) == 0)
      {
        output_moment = (Generic_moment_t*)
                         &(cdbe->radial[radnum].ml_moment);
        memcpy(output_moment, input_moment, ML_SIZE);
      }
      else if(strncmp(input_moment->name, "DPHI", 4) == 0)
      {
        output_moment = (Generic_moment_t*)
                         &(cdbe->radial[radnum].phi_moment);
        memcpy(output_moment, input_moment, PHI_SIZE);
      }
      else if(strncmp(input_moment->name, "DRHO", 4) == 0)
      {
        output_moment = (Generic_moment_t*)
                         &(cdbe->radial[radnum].rho_moment);
        memcpy(output_moment, input_moment, RHO_SIZE);
      }
      else if(strncmp(input_moment->name, "DSDP", 4) == 0)
      {
        output_moment = (Generic_moment_t*)
                         &(cdbe->radial[radnum].sdp_moment);
        memcpy(output_moment, input_moment, SDP_SIZE);
      }
      else if(strncmp(input_moment->name, "DSDZ", 4) == 0)
      {
        output_moment = (Generic_moment_t*)
                         &(cdbe->radial[radnum].sdz_moment);
        memcpy(output_moment, input_moment, SDZ_SIZE);
      }
      else if(strncmp(input_moment->name, "DSMV", 4) == 0)
      {
        output_moment = (Generic_moment_t*)
                         &(cdbe->radial[radnum].smv_moment);
        memcpy(output_moment, input_moment, SMV_SIZE);
      }
      else if(strncmp(input_moment->name, "DSMZ", 4) == 0)
      {
        output_moment = (Generic_moment_t*)
                         &(cdbe->radial[radnum].smz_moment);
        memcpy(output_moment, input_moment, SMZ_SIZE);
      }
      else if(strncmp(input_moment->name, "DSNR", 4) == 0)
      {
        output_moment = (Generic_moment_t*)
                         &(cdbe->radial[radnum].snr_moment);
        memcpy(output_moment, input_moment, SNR_SIZE);
      }
      else if(strncmp(input_moment->name, "DZDR", 4) == 0)
      {
        output_moment = (Generic_moment_t*)
                         &(cdbe->radial[radnum].zdr_moment);
        memcpy(output_moment, input_moment, ZDR_SIZE);
      }
      else if(strncmp(input_moment->name, "DHCA", 4) == 0)
      {
        /* Note: Name change from HCA to EHC - Brian Klein suggests
         * using EHC (Elevation HydroClass) instead of HCA to indicate
         * that the data has been filtered. */

        output_moment = (Generic_moment_t*)
                         &(cdbe->radial[radnum].ehc_moment);
        memcpy(output_moment, input_moment, EHC_SIZE);

        /* 20071001 Jim Ward - Apply the mode filter after the data is
         * copied There are several ways to do this. We would like to
         * the filter before the data is copied,  but for some reason
         * the RPG won't let us alter the input data in memory.
         * Some sort of lock?
         *
         * 20071101 Brian Klein's suggested method to extract the radar
         * data is:
         *
         * Base_data_header *bd = (pointer to what was returned from
         *
         * Generic_moment_t gm;
         * unsigned char*   dp_data = NULL;
         *
         * strcpy(gm.name, "DHCA")
         * dp_data = RPGC_get_radar_data((void*) bd, RPGC_DANY, &gm);
         *
         * This will give you a pointer to the start of the HCA data
         * in dp_data and a completed generic moment header (gm). You
         * can then apply the gm.scale and gm.offset if you wish but
         * since they should now be 1 and 0 respectively, it won't make
         * any difference.
         *
         * 20080715 But ... we've has already copied the data. Since the
         * scale/offset is 1/0, we can directly filter on the output
         * buffer without the additional function call. It the
         * scale/offset were something other than 1/0, we'd
         * have to use an intermediate array of floats. This has one
         * advantage that you don't have to malloc/free memory:
         *
         * int   j;
         * float val[MAX_MODE_BINS];
         *
         * mode_filter((char*) output_moment->gate.b,
         *               output_moment->no_of_gates,
         *               cdbe->mode_filter_length);
         *
         * - Convert the filtered array from chars to floats -
         *
         * for(j = 0; j < output_moment->no_of_gates; j++)
         *     val[j] = (float) output_moment->gate.b[j];
         *
         * - Write the floats to the output moment using scale/offset. -
         *
         * Add_moment_dehc(output_moment, val);                          */

        mode_filter((char*) output_moment->gate.b,
                     output_moment->no_of_gates,
                     cdbe->mode_filter_length);     /* AEL 3.1.1 */

        /* Change the name to indicate the moment has been filtered */

        strcpy(output_moment->name, "DEHC");

      } /* end if HCA moment */
      else /* an unknown moment */
      {
        if(DP_ELEV_PROD_DEBUG)
        {
           sprintf(msg, "Unknown moment: %4.4s\n", output_moment->name);

           RPGC_log_msg(GL_INFO, msg);
           fprintf(stderr, msg);
        }

        continue; /* with next moment */

      } /* end else unknown moment */

      if(DP_ELEV_PROD_DEBUG)
      {
         fprintf(stderr, "%d) %s, %s %d, %s %d, %s %d\n",
                 num_output_moments, output_moment->name,
                 "no_of_gates",      output_moment->no_of_gates,
                 "data_word_size",   output_moment->data_word_size,
                 "bin_size",         output_moment->bin_size);

         print_generic_moment(output_moment);
      }

      /* Increment the number of moments we have collected */

      num_output_moments++;
      if(num_output_moments == NUM_MOMENTS) /* got all 11, quit */
         break;

   } /* end loop over all input moments */

   /* Check that we collected all the moments. */

   if(num_output_moments != NUM_MOMENTS)
   {
      /* We didn't collect all the moments we expected to get */

      sprintf(msg, "num_output_moments %d != NUM_MOMENTS %d\n",
                    num_output_moments, NUM_MOMENTS);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_ELEV_PROD_DEBUG)
         fprintf(stderr, msg);

   } /* end if num_output_moments != NUM_MOMENTS */

   /* Increment the number of radials in this elevation. */

   cdbe->num_radials++;

} /* end Add_radial() =========================== */

/******************************************************************************
   Filename: dp_elev_build_product.c

   Description:
   ============
   Add_moment_dehc() writes a DEHC (Elevation Hydro Class) Generic Moment
   from an array of floats. It is based on Add_moment(), what Brian Klein
   used to create his moment, from ~/src/cpc023/tsk001/hca_process_radial.c,
   but stripped down:

    Add_moment (buf + output_offset, "DHCA", 'h', params, ONE_WORD, 0,
                0, HCA_SCALE, HCA_OFFSET, data->hca);

   low = 0 makes the lowest value possible value 0. HCA_SCALE, HCA_OFFSET,
   HCA_NO_DATA, C0, and Cp5 are defined in ~/include/hca.h

   Inputs: Generic_moment_t* hd   - the generic moment to fill
           float*            data - the data to fill it with

   Outputs: Generic_moment_t* hd  - the generic moment with the new data

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ---------------
   10071005      0000       James Ward         Initial version
   20080715      0001       James Ward         This is no longer used, but kept
                                               here in case HCA scale/offset
                                               changes from 1/0.
*****************************************************************************/

/*
 * void Add_moment_dehc (Generic_moment_t* moment, float* data)
 * {
 *   int            i, up, low;
 *   unsigned char* cpt;
 *   float          dscale, doffset;
 *   int            t;
 *   float          f;
 *
 *   -* 20071101 Brian Klein suggests using DEHC (Elevation HydroClass)
 *    * instead of DHCA as the name because the data has been filtered.
 *    * All the other moment parameters stay the same.                  *-
 *
 *   strcpy(moment->name, "DEHC");
 *
 *   up      = 0xff;
 *   low     = 0;
 *   cpt     = moment->gate.b;
 *   dscale  = moment->scale;
 *   doffset = moment->offset;
 *
 *   for ( i = 0; i < moment->no_of_gates; i++ )
 *   {
 *      if ( data[i] == HCA_NO_DATA )
 *         t = 0;
 *      else
 *      {
 *         f = data[i] * dscale + doffset;
 *         if ( f >= C0 )
 *           t = f + Cp5;
 *         else
 *           t = -(-f + Cp5);
 * 	
 *         if ( t > up )
 *            t = up;
 *
 *         if ( t < low )
 *            t = low;
 *      }
 *
 *      cpt[i] = (unsigned char) t;
 *
 *   } -* end loop over all the gates *-
 *
 * } -* end Add_moment_dehc( ) ================================ */
