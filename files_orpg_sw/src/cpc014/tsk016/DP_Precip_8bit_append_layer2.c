/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2014/09/05 12:40:51 $
 * $Id: DP_Precip_8bit_append_layer2.c,v 1.11 2014/09/05 12:40:51 cmn Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/******************************************************************************
   Filename: DP_Precip_8bit_append_layer2.c

   Description
   ===========
       append_ascii_layer2() formats the ASCII data in Layer 2 of the Digital
   Storm-Total Accumulation product buffer. The layer contains 3 sub-layers
   of information:
                               # fields
                               --------
   	1. Adaptation data        32    QPE parameters
        2. Supplemental data      11    QPE rate and accumulation
   	3. Bias-related fields    13    Bias data

   Each sub-layer is preceded by a 8-character descriptive header
   indicating the title and the number of fields of the sub-layer to follow.

   The layer 2 is not visible in AWIPS, but you can see it via cvg in the
   upper left hand corner. The info presented in the 4-bit STA tab should
   match.

   Inputs:
      LT_Accum_Buf_t* inbuf      - adaptable parameters/supplemental data
      unsigned int    layer1_len - layer 1 length
      int             vsnum      - current volume to get the scan summary

   Outputs:
      char* outbuf - product buffer containing with layer 2 appended

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Called by: Build_DSA_product()

   Change History
   ==============
   DATE      VERSION  PROGRAMMER   NOTES
   --------  -------  ----------   --------------------------------
   20071001  0000     Pham, Ward   Initial Implementation
   20080429  0001     Ward         Converted to use packet function
   20100310  0002     James Ward   Replaced RhoHV_min_Kdp_rate with
                                   corr_thresh.
   20101124  0003     James Ward   Added Max Rate.
   20111031  0004     James Ward   For CCR NA11-00372:
                                   Replaced RhoHV_min_rate with art_corr
                                   Replaced Z_max_beam_blk with N/A
   20121028  0005     D.Berkowitz  For CCR NA12-00361 and 362
                                   Deleted useMLDAHeights,
                                   replaced with Melting_Layer_Source
******************************************************************************/

#include "DP_Precip_8bit_func_prototypes.h"

int append_ascii_layer2 (LT_Accum_Buf_t* inbuf, char* outbuf,
                         unsigned int layer1_len, int vsnum)
{
   char   yes_or_no[4];                       /*  YES or    NO helper */
   char   ML_Source[8];                       /* RPG_0C_Hgt, Radar_Based,
                                               * Model_Enhanced had to be
                                               * shortened to fit 8 characters
                                               */
   char   t_or_f1[2], t_or_f2[2], t_or_f3[2]; /* TRUE or FALSE helper */
   char*  layer2_ptr      = NULL;
   short* layer2_ptr2     = NULL;
   int    data_layer_len  = 0;
   int    num_bytes_added = 0;
   short  row_increment   = 9; /* pixels to move down per row */
   int    last_prcp_date, last_prcp_mins_since_midnight;

   /* i_start and j_start are screen coordinates in pixels.
    * On cvg, there are about 5 pixels/character. */

   short  i_start, j_start;
   char   text[MAX_PACKET1 + 1];

   Scan_Summary*    summary = NULL;
   Symbology_block* sym_ptr = NULL;

   static unsigned int graphic_size = sizeof(Graphic_product);

   if(DP_PRECIP_8BIT_DEBUG)
      fprintf(stderr,"\nBeginning append_ascii_layer2() ...\n");

   /* Check for NULL pointers */

   if(pointer_is_NULL(inbuf, "append_ascii_layer2", "inbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(outbuf, "append_ascii_layer2", "outbuf"))
      return(NULL_POINTER);

   /* Skip to just after layer 1 in the output buffer */

   layer2_ptr = (char *) (outbuf + graphic_size + layer1_len);

   /* We are now at the start of layer 2.   *
    * Add the layer divider (-1) as a short */

   layer2_ptr2  = (short *) layer2_ptr;
   *layer2_ptr2 = -1;
   layer2_ptr  += 2;  /* 1 short = 2 bytes */

   /* Skip over the data layer length (4 bytes),  *
    * which we will fill this in at the end.      */

   layer2_ptr += 4;

   /* The data we're filling in should match what we're filling in the
    * STA tab block. Previously we encoded all the lines as one big 1000
    * character block and let cvg handle the display by automatically
    * wrapping every 80 characters, but Tom Ganger said this is the wrong
    * way to do it - each line should be encoded as a separate packet 1. */

   /* Line 1: 10 fields * 8 chars per field = 80 chars */

   i_start = 7; /* about one char (7 pixels) from left hand side */

   /* Note: null_Storm_Total is not TRUE/FALSE, but can be set to
    * several non-negative values. */

   if(inbuf->supl.null_Storm_Total)
      j_start = 36; /* leave room for 4 line (36 pixel) null product message */
   else
      j_start = 9;  /* about one char (9 pixels) from top */

   if (inbuf->qpe_adapt.mlda.Melting_Layer_Source == 0)
      {
      strcpy(ML_Source, "RPG_0C_H");
      }
   else if (inbuf->qpe_adapt.mlda.Melting_Layer_Source == 1)
      {
      strcpy(ML_Source, "R_Based");
      } 
   else
      {
      strcpy(ML_Source, "M_Enhanc");
      }

   memset(text, 0, MAX_PACKET1 + 1);

   sprintf(text, "ADAP(%2d)%8.1f%8s%8.0f%8.3f%8.0f%8.1f%8.4f%8.3f%8.2f",
           NUM_ADAP,                                 /*  36      */
           inbuf->qpe_adapt.mlda.default_ml_depth,   /*   0.5    */
           ML_Source,                                /*  YES     */
           inbuf->qpe_adapt.dp_adapt.Kdp_mult,       /*  44      */
           inbuf->qpe_adapt.dp_adapt.Kdp_power,      /*   0.822  */
           inbuf->qpe_adapt.dp_adapt.Z_mult,         /* 300      */
           inbuf->qpe_adapt.dp_adapt.Z_power,        /*   1.4    */
           inbuf->qpe_adapt.dp_adapt.Zdr_z_mult,     /*   0.0067 */
           inbuf->qpe_adapt.dp_adapt.Zdr_z_power,    /*   0.927  */
           inbuf->qpe_adapt.dp_adapt.Zdr_zdr_power); /*  -3.43   */

   num_bytes_added = add_packet_1(layer2_ptr, i_start, j_start, text);
   if(num_bytes_added == NULL_POINTER)
      return(NULL_POINTER);

   data_layer_len += num_bytes_added;
   layer2_ptr     += num_bytes_added;

   /* Line 2: 10 fields * 8 chars per field = 80 chars
    *
    * 20080825 Dan Stein suggests not adding Refl_min (-32.0) to the
    *          layer 2 as it is a config parameter that will never be
    *          altered by the operator. Mode_filter_len (9) and Refl_min
    *          are the only adaptable parameters not in the layer 2.
    *
    * 20081125 Two new adaptable parameters will not be displayed
    *          in the layer 2/TAB. They are the Max_vols_per_hour (30),
    *          and Min_early_term_ang (5). They may be added here,
    *          and to the ICD, at a later date. */

   j_start += row_increment;

   memset(text, 0, MAX_PACKET1 + 1);

   /* 20111031 Ward Replaced RhoHV_min_rate with art_corr
    *               Replaced Z_max_beam_blk with N/A */

   sprintf(text, "%8.4f%8.4f%8.1f%8d     N/A%8.1f%8.1f%8.1f%8.1f%8.1f",
           inbuf->qpe_adapt.dpprep_adapt.art_corr,       /*  0.8000 */
           inbuf->qpe_adapt.dpprep_adapt.corr_thresh,    /*  0.9000 */
           inbuf->qpe_adapt.dp_adapt.Refl_max,           /* 53.0    */
           inbuf->qpe_adapt.dp_adapt.Kdp_max_beam_blk,   /* 70      */
           /* inbuf->qpe_adapt.dp_adapt.Z_max_beam_blk,  -* 50      */
           inbuf->qpe_adapt.dp_adapt.Kdp_min_usage_rate, /* 10.0    */
           inbuf->qpe_adapt.dp_adapt.Ws_mult,            /*  0.6    */
           inbuf->qpe_adapt.dp_adapt.Gr_mult,            /*  0.8    */
           inbuf->qpe_adapt.dp_adapt.Rh_mult,            /*  0.8    */
           inbuf->qpe_adapt.dp_adapt.Ds_mult);           /*  2.8    */

   num_bytes_added = add_packet_1(layer2_ptr, i_start, j_start, text);
   if(num_bytes_added == NULL_POINTER)
      return(NULL_POINTER);

   data_layer_len += num_bytes_added;
   layer2_ptr     += num_bytes_added;

   /* Line 3: 10 fields * 8 chars per field = 80 chars */

   j_start += row_increment;

   memset(text, 0, MAX_PACKET1 + 1);

   sprintf(text, "%8.1f%8.1f%8.1f%8d%8d%8d%8.2f%8.1f%8d%8.1f",
           inbuf->qpe_adapt.dp_adapt.Ic_mult,                 /*  2.8  */
           inbuf->qpe_adapt.dp_adapt.Grid_is_full,            /*  99.7 */
           inbuf->qpe_adapt.dp_adapt.Paif_rate,               /*   0.5 */
           inbuf->qpe_adapt.dp_adapt.Paif_area,               /*  80   */
           inbuf->qpe_adapt.prep_adapt.rain_time_thresh,      /*  60   */
           inbuf->qpe_adapt.dp_adapt.Num_zones,               /*   0   */
           inbuf->qpe_adapt.dp_adapt.Max_precip_rate,         /* 200.00 */
           0.0,
           0,
           0.0);

   num_bytes_added = add_packet_1(layer2_ptr, i_start, j_start, text);
   if(num_bytes_added == NULL_POINTER)
      return(NULL_POINTER);

   data_layer_len += num_bytes_added;
   layer2_ptr     += num_bytes_added;

   /* Line 4: 10 fields * 8 chars per field = 80 chars */

   j_start += row_increment;

   /* 20070103 The (legacy) DSP uses the 'time of scan', which is
    * computed from the average of the times of the 2 rate grids.
    * Mark Fresch says that in order for layer2 to match with the
    * STA Tab (thus providing the same information in a text format
    * in one place for the 4 and 8 bit products), we will instead
    * do it as the STA tab does = 'time the product was made'.
    * The old way:
    *
    * UNIX_time_to_julian_mins (inbuf->supl.avgtime,
    *                    &scan_date, &scan_mins_since_midnight);
    */

   summary = RPGC_get_scan_summary(vsnum);
   if(pointer_is_NULL(summary, "append_ascii_layer2", "summary"))
      return(NULL_POINTER);

   /* Convert the last_time_prcp. */

   UNIX_time_to_julian_mins (inbuf->supl.last_time_prcp,
                             &last_prcp_date,
                             &last_prcp_mins_since_midnight);

   last_prcp_date                = check_date((short) last_prcp_date);
   last_prcp_mins_since_midnight = check_time((short) last_prcp_mins_since_midnight);

   memset(text, 0, MAX_PACKET1 + 1);

   sprintf(text, "%8d%8d%8d%8d%8d%8.1f%8dSUPL(%2d)%8d%8d",
           inbuf->qpe_adapt.accum_adapt.restart_time,    /*  60   */
           inbuf->qpe_adapt.accum_adapt.max_interp_time, /*  30   */
           inbuf->qpe_adapt.accum_adapt.max_hourly_acc,  /* 800   */
           inbuf->qpe_adapt.adj_adapt.time_bias,         /*  50   */
           inbuf->qpe_adapt.adj_adapt.num_grpairs,       /*  10    */
           inbuf->qpe_adapt.adj_adapt.reset_bias,        /*   1.0  */
           inbuf->qpe_adapt.adj_adapt.longst_lag,        /* 168    */
           NUM_SUPL,                                     /*  11    */
           summary->volume_start_date,                   /*  --    */
           summary->volume_start_time);                  /*  --    */

   num_bytes_added = add_packet_1(layer2_ptr, i_start, j_start, text);
   if(num_bytes_added == NULL_POINTER)
      return(NULL_POINTER);

   data_layer_len += num_bytes_added;
   layer2_ptr     += num_bytes_added;

   /* Line 5: 10 fields * 8 chars per field = 80 chars */

   j_start += row_increment;

   if (inbuf->supl.prcp_detected_flg == TRUE)
      strcpy(t_or_f1, "T");
   else
      strcpy(t_or_f1, "F");

   if (inbuf->supl.ST_active_flg == TRUE)
      strcpy(t_or_f2, "T");
   else
      strcpy(t_or_f2, "F");

   if (inbuf->supl.prcp_begin_flg == TRUE)
      strcpy(t_or_f3, "T");
   else
      strcpy(t_or_f3, "F");

   memset(text, 0, MAX_PACKET1 + 1);

   sprintf(text, "%8s%8s%8s%8d%8d%8.2f%8.1f%8.1f%8dBIAS(%2d)",
           t_or_f1,                               /* -- */
           t_or_f2,                               /* -- */
           t_or_f3,                               /* -- */
           last_prcp_date,                        /* -- */
           last_prcp_mins_since_midnight,         /* -- */
           inbuf->supl.pct_hybrate_filled,        /* -- */
           inbuf->supl.highest_elev_used,         /* -- */
           inbuf->supl.sum_area,                  /* -- */
           inbuf->supl.vol_sb,                    /* -- */
           NUM_BIAS);                             /* 13 */

   num_bytes_added = add_packet_1(layer2_ptr, i_start, j_start, text);
   if(num_bytes_added == NULL_POINTER)
      return(NULL_POINTER);

   data_layer_len += num_bytes_added;
   layer2_ptr     += num_bytes_added;

   /* Line 6: 10 fields * 8 chars per field = 80 chars */

   j_start += row_increment;

   if (inbuf->qpe_adapt.adj_adapt.bias_flag == TRUE)
      strcpy(yes_or_no, "YES");
   else
      strcpy(yes_or_no, "NO");

   memset(text, 0, MAX_PACKET1 + 1);

   sprintf(text, "%8d%8d%8d%8d%8d%8d%8d%8d%8s%8.2f",
           inbuf->qpe_adapt.bias_info.tbupdt,    /* --- */
           inbuf->qpe_adapt.bias_info.dbupdt,    /* --- */
           inbuf->qpe_adapt.bias_info.tbtbl_upd, /* --- */
           inbuf->qpe_adapt.bias_info.dbtbl_upd, /* --- */
           inbuf->qpe_adapt.bias_info.tbtbl_obs, /* --- */
           inbuf->qpe_adapt.bias_info.dbtbl_obs, /* --- */
           inbuf->qpe_adapt.bias_info.tbtbl_gen, /* --- */
           inbuf->qpe_adapt.bias_info.dbtbl_gen, /* --- */
           yes_or_no,                            /* --- */
           inbuf->qpe_adapt.bias_info.bias);     /* --- */

   num_bytes_added = add_packet_1(layer2_ptr, i_start, j_start, text);
   if(num_bytes_added == NULL_POINTER)
      return(NULL_POINTER);

   data_layer_len += num_bytes_added;
   layer2_ptr     += num_bytes_added;

   /* Line 7: 3 fields * 8 chars per field = 24 chars
    *
    * NOTE: In build 11, we still used the legacy BIAS information so some
    * fields are filled with a default value (i.e AWIPS Site ID) */

   j_start += row_increment;

   memset(text, 0, MAX_PACKET1 + 1);

   sprintf(text, "%8.2f%8.3f%8s",
           inbuf->qpe_adapt.bias_info.grpsiz, /* --- */
           inbuf->qpe_adapt.bias_info.mspan,  /* --- */
           inbuf->qpe_adapt.bias_source);     /* XXX */

   num_bytes_added = add_packet_1(layer2_ptr, i_start, j_start, text);
   if(num_bytes_added == NULL_POINTER)
      return(NULL_POINTER);

   data_layer_len += num_bytes_added;
   layer2_ptr     += num_bytes_added;

   /* Skip to right after layer 1 in the output buffer, *
    * then skip over the layer divider                  */

   layer2_ptr  = (char *) (outbuf + graphic_size + layer1_len);
   layer2_ptr += 2;  /* 1 short = 2 bytes */

   /* Write the length of the data layer to the product (4 bytes). *
    * According to the Code Manual, Vol. 2, p. 74, the length of   *
    * the data layer does not include the layer divider nor the    *
    * length of the data layer.                                    */

   RPGC_set_product_int ((void *) layer2_ptr, data_layer_len);

   /* Augment the length of the symbology block to account for all *
    * the layer2 we've just written. The extra 6 bytes is for:     *
    *                                                              *
    * the layer2 divider    (2 bytes) +                            *
    * length of data layer2 (4 bytes)                              */

   sym_ptr = (Symbology_block*) (outbuf + graphic_size);

   RPGC_set_product_int ((void *) &sym_ptr->block_len, layer1_len +
                         6 + data_layer_len);

   sym_ptr->n_layers = 2; /* there are now 2 layers */

   if(DP_PRECIP_8BIT_DEBUG)
      fprintf(stderr,"Ending append_ascii_layer2() ...\n");

   return(FUNCTION_SUCCEEDED);

} /* end append_ascii_layer2() ============================== */
