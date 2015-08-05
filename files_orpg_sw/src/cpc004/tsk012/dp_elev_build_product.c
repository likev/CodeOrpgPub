/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/05/14 16:11:12 $
 * $Id: dp_elev_build_product.c,v 1.1 2008/05/14 16:11:12 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include "dp_elev_func_prototypes.h" 

/******************************************************************************
   Filename: dp_elev_build_product.c

   Description:
   ============
   DPE_build_product() builds the Base data elevation product.

   Inputs: void* radial - input radial data

   Outputs: void* outbuf - elevation product containing input radial
   
   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   5 Oct 2007    0000       Jihong Liu         Initial implementation 
*****************************************************************************/

void DPE_build_product( void* radial, void* outbuf )
{
   Generic_moment_t*         gm_data = NULL;
   Generic_moment_t*         gm1     = NULL;
   Base_data_header*         hdr     = (Base_data_header *) radial;
   Compact_dp_basedata_elev* out     = (Compact_dp_basedata_elev *) outbuf;
   unsigned short            size1   = 0;
   short                     radnum  = 0;
   float                     val[MAX_MODE_BINS];
   int                       i;

   /* 20071003 Jim Ward - The radials may not be presented in order. In
    * some data the last radial is presented first. We get the azimuth from 
    * the header, which should be in .5 degree and round it down. 
    * For example, azimuth 0.5 -> radnum 0, ... azimuth 359.5 -> radnum 359.
    *
    * 20071003 Jim Ward - even though MAX_RADIALS_ELEV goes out to 400, we are
    * going to ignore all above 360.
    *
    * 20071009 Jim Ward - Cham says that it's better to use hdr->azi_num than 
    * hdr->azimuth. azi_num seems to start at 1. */

   radnum = hdr->azi_num - 1;
   if ( (radnum < 0) || (radnum > 359) )
   {
      if ( DP_ELEV_PROD_DEBUG )
      {
         fprintf(stderr, "hdr->azi_num: %d, radnum %d < 0 or > 359, "
  	                 "can't save radial\n", hdr->azi_num, radnum);
      }
      return;
   }
   else
   {
      out->radial_is_good[radnum] = TRUE;
   }

   memcpy ( (void *) &(out->radial[radnum].bdh), (void *) radial, 
	    BASEDATA_HDR_SIZE );

   /* If the dual-pol data enabled, copy each dual-pol data */

   if (( hdr->msg_type & PREPROCESSED_DUALPOL_TYPE))
   {
      /***************************************************************/
      /************** only process RHV info **************************/
      /***************************************************************/

      gm_data = (Generic_moment_t*)((char*) radial + hdr->offsets[0]); 

      gm1=(Generic_moment_t*)(&out->radial[radnum].dualPol_data[0]);
      *gm1 = *gm_data;

      /* extract the data from Generic Moment */
      extract_momentData( gm1, gm_data );  
      
      /* update the offset for rhv */
      out->radial[radnum].bdh.offsets[0] = 0;

      /* specify the size for the next momment */
      size1 += ALIGNED_SIZE(sizeof(Generic_moment_t) + gm_data->no_of_gates); 
      out->radial[radnum].bdh.offsets[1] = size1;

      if ( DP_ELEV_PROD_DEBUG )
      {
         fprintf(stderr, "RHV - NumGates=%d\tResolution=%d\t"
	                 "BinSize=%d\tsize1=%d\n",
                         gm_data->no_of_gates, gm_data->data_word_size, 
			 gm_data->bin_size, size1); 
      } 

      /***************************************************************/
      /************** only process ZDR info **************************/
      /***************************************************************/

      gm_data = (Generic_moment_t*)((char*) radial + hdr->offsets[1]); 

      gm1 = (Generic_moment_t*)(&out->radial[radnum].dualPol_data[size1]);
      *gm1 = *gm_data;

      /* extract the data from Generic Moment */
      extract_momentData( gm1, gm_data );  

      /* specify the size for the next momment */
      size1 += ALIGNED_SIZE(sizeof(Generic_moment_t) + gm_data->no_of_gates); 
      out->radial[radnum].bdh.offsets[2] = size1;

      if ( DP_ELEV_PROD_DEBUG )
      {
         fprintf(stderr, "ZDR - NumGates=%d\tResolution=%d\tBinSize=%d\t"
	                 "size1=%d\n",
                         gm_data->no_of_gates, gm_data->data_word_size, 
			 gm_data->bin_size, size1); 
      } 

      /***************************************************************/
      /*************** only process SMZ info *************************/
      /***************************************************************/

      gm_data = (Generic_moment_t*)((char*) radial + hdr->offsets[2]); 

      gm1 = (Generic_moment_t*)(&out->radial[radnum].dualPol_data[size1]);
      *gm1 = *gm_data;

      /* extract the data from Generic Moment */
      extract_momentData( gm1, gm_data );  

      /* specify the size for the next momment */
      size1 += ALIGNED_SIZE(sizeof(Generic_moment_t) + gm_data->no_of_gates); 
      out->radial[radnum].bdh.offsets[3] =size1;

      if ( DP_ELEV_PROD_DEBUG )
      {
         fprintf(stderr, "SMZ - NumGates=%d\tResolution=%d\tBinSize=%d\t"
	                 "size1=%d\n",
                         gm_data->no_of_gates, gm_data->data_word_size, 
			 gm_data->bin_size, size1); 
      } 
     
     /***************************************************************/
     /***************** only process SNR info ***********************/
     /***************************************************************/

      gm_data = (Generic_moment_t*)((char*) radial + hdr->offsets[3]); 

      gm1 = (Generic_moment_t*)(&out->radial[radnum].dualPol_data[size1]);
      *gm1 = *gm_data;

      /* extract the data from Generic Moment */
      extract_momentData( gm1, gm_data );  

      /* specify the size for the next momment */
      size1 += ALIGNED_SIZE(sizeof(Generic_moment_t) + gm_data->no_of_gates); 
      out->radial[radnum].bdh.offsets[4] = size1;

      if ( DP_ELEV_PROD_DEBUG )
      {
         fprintf(stderr, "SNR - NumGates=%d\tResolution=%d\tBinSize=%d\t"
	                 "size1=%d\n",
                         gm_data->no_of_gates, gm_data->data_word_size, 
			 gm_data->bin_size, size1); 
      } 

     /***************************************************************/
     /**************** only process KDP info ************************/
     /***************************************************************/

      gm_data = (Generic_moment_t*)((char*) radial + hdr->offsets[4]); 

      gm1 = (Generic_moment_t*)(&out->radial[radnum].dualPol_data[size1]);
      *gm1 = *gm_data;

      /* extract the data from Generic Moment */
      extract_momentData( gm1, gm_data );  

      /* specify the size for the next momment */
      size1 += ALIGNED_SIZE(sizeof(Generic_moment_t) + gm_data->no_of_gates); 
      out->radial[radnum].bdh.offsets[5] = size1;

      if ( DP_ELEV_PROD_DEBUG )
      {
         fprintf(stderr, "KDP - NumGates=%d\tResolution=%d\tBinSize=%d\t"
	                 "size1=%d\n",
                         gm_data->no_of_gates, gm_data->data_word_size, 
			 gm_data->bin_size, size1); 
      } 

     /***************************************************************/
     /**************** only process HCA info ************************/
     /***************************************************************/

      gm_data = (Generic_moment_t*)((char*) radial + hdr->offsets[5]); 

      gm1 = (Generic_moment_t*)(&out->radial[radnum].dualPol_data[size1]);
      *gm1 = *gm_data;

      /* extract the data from Generic Moment */
      extract_momentData( gm1, gm_data );  

      /* 20071001 Jim Ward - Apply the mode filter after the data is copied.
       * We would like to apply it before the data is copied, but for some
       * reason the RPG won't let us alter the data in memory. A lock?
       *
       *              For testing:         For delivery:
       * 
       * mode_filter  OFF                  ON
       * save_radial  ON                   OFF
       * DEBUG_PRINT  PRINT_STDERR         PRINT_LELB_MON
       */
      /* 20071101 Brian Klein's suggested method to extract the radar data is: 
       *
       * To extract the data, just use RPGC_get_radar_data like this:
       *
       * Base_data_header *bd = (pointer to what was returned from 
       *                         RPGC_get_inbuf_by_name);
       * Generic_moment_t gm;
       * unsigned char * dp_data = NULL;
       *
       * strcpy(gm.name,"DHCA")
       * dp_data = RPGC_get_radar_data((void*)bd, RPGC_DANY, &gm);
       *
       * This will give you a pointer to the start of the HCA data in dp_data 
       * and a completed generic moment header (gm). You can then apply the 
       * gm.scale and gm.offset if you wish but since they should now be 1 and
       * 0 respectively, it won't make any difference.
       *
       * But Jihong has already extracted the data with extract_momentData(). 
       * Since the offset is 0, there is no stepping up or down. Use Brian's 
       * method as a backup. Jihong's method has the advantage that you don't 
       * have to malloc/free memory. */

      mode_filter((char*) gm1->gate.b, gm1->no_of_gates, out->mode_filter_length);

      /* Write the filtered array of chars to floats */

      for ( i=0; i<gm1->no_of_gates; i++ )
         val[i] = (float) gm1->gate.b[i];

      /* Write the filtered array of floats to a generic moment format. */

      Add_moment_dfhc( gm1, gm_data->first_gate_range, gm_data->bin_size, 
                       gm_data->no_of_gates, val );

      /* specify the size for the next momment */
      size1 += ALIGNED_SIZE(sizeof(Generic_moment_t) + gm_data->no_of_gates); 
      out->radial[radnum].bdh.offsets[6] = size1;

      if ( DP_ELEV_PROD_DEBUG )
      {
         fprintf(stderr, "HCA - NumGates=%d\tResolution=%d\tBinSize=%d\t"
	                 "size1=%d\n",
                         gm_data->no_of_gates, gm_data->data_word_size,
			 gm_data->bin_size, size1); 
      } 

      /* Compare the moment to the mode filter. */

      if (DP_ELEV_PROD_DEBUG)
         print_generic_moment( gm1 );

     /***************************************************************/
     /************** only process DML info **************************/
     /***************************************************************/
      gm_data = (Generic_moment_t*)((char*) radial + hdr->offsets[6]); 
      gm1 = (Generic_moment_t*)(&out->radial[radnum].dualPol_data[size1]);
      *gm1 = *gm_data;

      /* extract the data from Generic Moment */
      extract_momentData( gm1, gm_data );  

      if ( DP_ELEV_PROD_DEBUG )
      {
         fprintf(stderr, "DML - NumGates=%d\tResolution=%d\tBinSize=%d\t"
	                 "size1=%d\n",
                         gm_data->no_of_gates, gm_data->data_word_size,
			 gm_data->bin_size, size1); 
      } 
   }

   /* Increment the number of radials this elevation. */

   out->num_radials++;

} /* end DPE_build_product() =============================================== */

/******************************************************************************
   Filename: dp_elev_build_product.c

   Description:
   ============
   Add_moment_dfhc() writes a DFHC Generic Moment from an array of floats.
   It is based on Add_moment(), what Brian Klein used to create his moment,
   from ~/src/cpc017/tsk012/hca_process_radial.c but stripped down:
  
    Add_moment (buf + size1, "DHCA", 'h', params, ONE_WORD, 0,
                0, HCA_SCALE, HCA_OFFSET, data->hca);

   low = 0 makes the lowest value possible value 0.  HCA_SCALE, HCA_OFFSET, 
   HCA_NO_DATA (-1.e20f), C0 (0.f),and Cp5 (0.5f) are defined in ~/include/hca.h

   Inputs: void* radial - input radial data

   Outputs: void* outbuf - elevation product containing input radial
   
   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ---------------
   5 Oct 2007    0000       James Ward         Initial version
*****************************************************************************/

void Add_moment_dfhc ( Generic_moment_t* hd, short first_gate_range, 
                       short bin_size, unsigned short no_of_gates, float* data ) 
{
  int            i, up, low;
  unsigned char* cpt;
  float          dscale, doffset;
  int            t;
  float          f;

  /* 20071101 Brian Klein suggests using DFHC instead of DHCA as the name
   * because the data has been filtered. */

  strcpy (hd->name, "DFHC");

  hd->info             = 0;
  hd->first_gate_range = first_gate_range;
  hd->bin_size         = bin_size;
  low                  = 0;

  hd->no_of_gates      = no_of_gates;
  hd->tover            = 0;
  hd->SNR_threshold    = 0;
  hd->control_flag     = 0;
  hd->data_word_size   = 8;          /* ONE_WORD - see hca_process_radial.c */
  hd->scale            = HCA_SCALE;  /* 1.f                                 */
  hd->offset           = HCA_OFFSET; /* 0.f                                 */

  up      = 0xff;
  cpt     = hd->gate.b;
  dscale  = hd->scale;
  doffset = hd->offset;

  for ( i = 0; i < no_of_gates; i++ ) 
  {
     if ( data[i] == HCA_NO_DATA )
        t = 0;
     else 
     {
        f = data[i] * dscale + doffset;
        if ( f >= C0 )
          t = f + Cp5;
        else
          t = -(-f + Cp5);
	
        if ( t > up )
           t = up;

        if ( t < low )
           t = low;
     }

     cpt[i] = (unsigned char) t;
  }
} /* Add_moment_dfhc( ) ==================================================== */
