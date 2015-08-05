/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2014/09/05 12:43:47 $
 * $Id: DP_Precip_4bit_STA_tab.c,v 1.15 2014/09/05 12:43:47 cmn Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */

/******************************************************************************
   Filename: DP_Precip_4bit_STA_tab.c

   Description
   ===========
      STA_tab() is the main generation module for the creation of the tabular
   alphanumeric block which is appended STA (product 171).

   Input:
     char*           outbuf        - output buffer
     int             sym_block_len - symbology block length
     int*            tab_len       - tab block length
     int*            tab_offset    - tab block offset
     int             vsnum         - volume number
     LT_Accum_Buf_t* inbuf         - input buffer
     Siteadp_adpt_t* Siteadp       - site adaptation info
     int*            offset;       - offset, in bytes, from start of msg

   Output: outbuf with TAB block filled in.

   Returns: FUNCTION_SUCCEEDED (0), FUNCTION_FAILED (1), NULL_POINTER (2)

   Change History
   ==============
   DATE     VERSION  PROGRAMMER   NOTES
   -------  -------  ----------   -------------------------------------
   10/2007   0000    Ward, Pham   Initial Implementation
   02/2008   0001    Ward         Deleted "BIAS APPLIED FLAG" as it
                                  duplicated "GAGE BIAS APPLIED"
   03/2010   0002    Ward         Replaced RhoHV_min_Kdp_rate with
                                  corr_thresh.
   10/2011   0003    Ward         For CCR NA11-00372:
                                  Replaced RhoHV_min_rate with art_corr
                                  Replaced Z_max_beam_blk with N/A
   01/2012   0004    Ward         CCR NA12-00058: Add exclusion zones
   20121028  0005    D.Berkowitz  For CCR NA12-00361 and 362
                                  Deleted useMLDAHeights,
                                  replaced with Melting_Layer_Source
******************************************************************************/

#include <string.h> /* strcat */
#include "DP_Precip_4bit_func_prototypes.h"

int STA_tab(char* outbuf, int sym_block_len, int* tab_len,
            int* tab_offset, int vsnum, LT_Accum_Buf_t* inbuf,
            Siteadp_adpt_t* Siteadp, int* offset)
{
   Scan_Summary*    summary;     /* scan summary, automatically filled      */
   TAB_header_t     tab_hdr;     /* TAB header structure                    */
   alpha_header_t   alpha_hdr;   /* alpha block header structure            */
   Graphic_product* gprod_ptr;   /* pointer to a graphic product structure  */
   int TAB_size      = 0;        /* holds size of the completed TAB block   */
   int est_TAB_size  = 0;        /* holds the est size of the TAB block     */
   int dd            = 0;        /* holds calendar day                      */
   int dm            = 0;        /* holds calendar month                    */
   int dy            = 0;        /* holds calendar year output              */
   int hr            = 0;        /* holds volume scan hour for isdp est.    */
   int min           = 0;        /* holds volume scan minute for isdp est.  */
   int zone          = 0;        /* exclusion zone counter                  */
   int num_tab_pages = 0;        /* number_of tab pages                     */
   char hhmm[6];                 /* container to hold time string           */
   char temp[LINE_WIDTH+1];      /* temporary line holder                   */
   char zone_hdr1[LINE_WIDTH+1]; /* exclusion zone header line 1            */
   char zone_hdr2[LINE_WIDTH+1]; /* exclusion zone header line 2            */
   char zone_hdr3[LINE_WIDTH+1]; /* exclusion zone header line 3            */
   char bool_str[6];             /* YES/NO or TRUE/FALSE as a string        */
   char ML_Source[19];           /* RPG_0C_Hgt, Radar_Based,
                                  * Model_Enhanced
                                  */
   char ISDP_app[4];             /* YES if ISDP estimate applied to base data */
   char ISDP_insuf[18];          /* Insufficient data string                */
   char msg_hdr_block[125];      /* container to holds TAB msg header block */
   char msg[200];                /* stderr message                          */

   static unsigned int graphic_size   = sizeof(Graphic_product);
   static unsigned int tab_hdr_size   = sizeof(TAB_header_t);
   static unsigned int alpha_hdr_size = sizeof(alpha_header_t);
   static unsigned int ad_size        = sizeof(alpha_data_t);

   /* Check for NULL pointers. */

   if(pointer_is_NULL(outbuf, "STA_tab", "outbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(tab_len, "STA_tab", "tab_len"))
      return(NULL_POINTER);

   if(pointer_is_NULL(tab_offset, "STA_tab", "tab_offset"))
      return(NULL_POINTER);

   if(pointer_is_NULL(inbuf, "STA_tab", "inbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(Siteadp, "STA_tab", "Siteadp"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "STA_tab", "offset"))
      return(NULL_POINTER);

   /* Step 1 - Make a copy of the main MHB (message header block which must  *
    * be duplicated from the main product) in local memory for TAB updating. *
    *                                                                        *
    * This could be for ease of AWIPS to tear off the TAB from the product   */

   memcpy(msg_hdr_block, outbuf, graphic_size);

   /* Acquire a pointer to the MHB */

   gprod_ptr = (Graphic_product*) msg_hdr_block;

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr,"graphic pointer set - prod_code = %hd vcp_num = %hd\n",
                      gprod_ptr->prod_code, gprod_ptr->vcp_num);

   /* Step 2 - Set current offset to beginning of the alpha block. *
    *          Copy the alpha header into the output buffer        *
    *                                                              *
    * CCR NA12-00058: Add exclusion zones.                         *
    * We have 3+ pages, each containing LINES_PER_PAGE (17) lines. *
    * Pages 4 and 5 list exclusion zones, with 3 header lines.     */

   num_tab_pages = 3;

   if(inbuf->qpe_adapt.dp_adapt.Num_zones > 0)
   {
      num_tab_pages++;

      if(inbuf->qpe_adapt.dp_adapt.Num_zones > (LINES_PER_PAGE-3))
         num_tab_pages++;
   }

   alpha_hdr.divider   = (short) -1;
   alpha_hdr.num_pages = num_tab_pages;

   /* We copy the message header after the TAB header, so add another 120 bytes. */

   *offset = graphic_size + sym_block_len + tab_hdr_size + graphic_size;

   memcpy (outbuf + *offset, &alpha_hdr, alpha_hdr_size);

   /* Quality control - check and be sure that we are not near the allocated
    * end size of the output buffer */

   if(*offset >= SIZE_4BIT)
   {
      sprintf(msg, "%s offset %d >= SIZE_4BIT %d\n",
              "STA_tab:",
              *offset,
              SIZE_4BIT);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_PRECIP_4BIT_DEBUG)
         fprintf(stderr, msg);

      return(FUNCTION_FAILED);
   }

   /* Step 3 - Format data strings and stuff into the TAB block. *
    *          Each line will be padded to LINE_WIDTH characters */

   *offset += alpha_hdr_size;

   if(DP_PRECIP_4BIT_DEBUG)
   {
      fprintf (stderr, "passed alpha header - start of data, offset = %d\n",
               *offset);
   }

   /* PAGE 1 LINE 1 ***********************************************************/

   if(DP_PRECIP_4BIT_DEBUG)
   {
      sprintf(temp, "%s%s%s%s%s%s%s%s",
              "123456789a",
              "123456789b",
              "123456789c",
              "123456789d",
              "123456789e",
              "123456789f",
              "123456789g",
              "123456789h");

      add_tab_str(0, temp, outbuf, offset);
   }

   add_tab_str(24, "STORM TOTAL ACCUMULATION", outbuf, offset);

   /* PAGE 1 LINE 2 */

   add_tab_str(LINE_WIDTH, NULL, outbuf, offset);

   /* PAGE 1 LINE 3 */

   /* 20070103 Cham: "The STA Tab uses volume scan date/time *
    * to match with legacy OHP/STP Tab products."            *
    * This is the "time the product was made".               */

   summary = RPGC_get_scan_summary(vsnum);

   RPGCS_julian_to_date(summary->volume_start_date, &dy, &dm, &dd);

   msecs_to_string(summary->volume_start_time * 1000, hhmm); /* millisecs */

   sprintf(temp, "RADAR ID: %4s     DATE: %02d/%02d/%02d     TIME: %5s",
           Siteadp->rpg_name, dm, dd, dy % 100, hhmm);

   add_tab_str(0, temp, outbuf, offset);

   /* PAGE 1 LINE 4 */

   sprintf(temp, "VOLUME COVERAGE PATTERN: %3d            MODE: ",
           summary->vcp_number);

   switch(summary->weather_mode)
   {
      case MAINTENANCE_MODE:
         strcat(temp, "Maintenance");
         break;

      case CLEAR_AIR_MODE:
         strcat(temp, "Clear-Air");
         break;

      case PRECIPITATION_MODE:
      default:
         strcat(temp, "Precip");
         break;
   }

   add_tab_str(0, temp, outbuf, offset);

   /* PAGE 1 LINE 5 */

   if(inbuf->qpe_adapt.adj_adapt.bias_flag == TRUE)
      strcpy(bool_str, "YES");
   else
      strcpy(bool_str, "NO");

   add_tab_page1_str(0, "GAGE BIAS APPLIED", "%5s",
                     bool_str, /* default NO */
                     outbuf, offset);

   /* PAGE 1 LINE 6 */

   add_tab_page1_float(5, "BIAS  ESTIMATE", "%8.2f",
                       inbuf->qpe_adapt.bias_info.bias, /* 1.0 */
                       outbuf, offset);

   /* PAGE 1 LINE 7 */

   add_tab_page1_float(5, "EFFECTIVE # G/R PAIRS", "%8.2f",
                       inbuf->qpe_adapt.bias_info.grpsiz,
                       outbuf, offset);

   /* PAGE 1 LINE 8 */

   add_tab_page1_float(5, "MEMORY SPAN (HOURS)", "%9.3f",
                       inbuf->qpe_adapt.bias_info.mspan,
                       outbuf, offset);

   /* PAGE 1 LINE 9 */

   RPGCS_julian_to_date(inbuf->qpe_adapt.bias_info.dbupdt, &dy, &dm, &dd);

   msecs_to_string(inbuf->qpe_adapt.bias_info.tbupdt * 1000, hhmm); /* millisec */

   sprintf(temp, "%02d/%02d/%02d %5s", dm, dd, dy % 100, hhmm);

   add_tab_page1_str(5, "DATE/TIME LAST BIAS UPDATE", "%s",
                     temp,
                     outbuf, offset);

   /* PAGE 1 LINE 10 */

   add_tab_page1_float(0, "HYBRID RATE PERCENT BINS FILLED", "%8.2f",
                       inbuf->supl.pct_hybrate_filled,
                       outbuf, offset);

   /* PAGE 1 LINE 11 */

   add_tab_page1_float(12, "HIGHEST ELEV. USED (DEG)", "%7.1f",
                       inbuf->supl.highest_elev_used,
                       outbuf, offset);

   /* PAGE 1 LINE 12 */

   add_tab_page1_float(12, "TOTAL PRECIP AREA (KM**2)", "%7.1f",
                       inbuf->supl.sum_area,
                       outbuf, offset);

   /* PAGE 1 LINE 13 */

   add_tab_page1_str(0, "AWIPS SITE ID OF MOST RECENT BIAS SOURCE", "%5s",
                     inbuf->qpe_adapt.bias_source, /* XXX */
                     outbuf, offset);

   /* END OF PAGE 1 */

   tab_page_end(1, outbuf, offset);

   /* PAGE 2 LINE 1 *********************************************************/

   /* 20080825 Dan Stein suggests not adding Refl_min to the TAB      *
    *          as it is a system parameter that will never be altered *
    *          by the operator. Mode_filter_len and Refl_min are the  *
    *          only adaptable parameters not in the TAB.              */

   add_tab_page2_floats("DEFAULT MELTING LAYER DEPTH",
                        "%6.1f",
                        inbuf->qpe_adapt.mlda.default_ml_depth, /* 0.5 */
                        "KM",
                        "MAX KDP BEAM BLOCKAGE",
                        "%10.0f",
                        (float) inbuf->qpe_adapt.dp_adapt.Kdp_max_beam_blk, /* 70 */
                        "%",
                        outbuf,
                        offset);

   /* PAGE 2 LINE 2 */

   /* MLDA heights are either -
    * 0 = MLDA_UseType_Default in melting_layer.c (RPG_0C_Hgt in HCI),
    * 1 = MLDA_UseType_Radar in melting_layer.c (Radar_Based in HCI),
    * 2 = MLDA_UseType_Model in melting_layer.c (Model_Enhanced in HCI),
    * where value of 2 is the normal default.
    */

   if(inbuf->qpe_adapt.mlda.Melting_Layer_Source == 0)
      {
      strcpy(ML_Source, "     RPG_0C_HGT    "); /* extra space for alignment with
					         * Model_Enhanced
                                                 */
      }
   else if(inbuf->qpe_adapt.mlda.Melting_Layer_Source == 1)
      {
      strcpy(ML_Source, "     RADAR_BASED   "); /* extra space for alignment with
					         * Model_Enhanced
                                                 */
      }
   else
      {
      strcpy(ML_Source, "     MODEL_ENHANCED"); 
      }
 
   /* 20111031 Ward Replaced Z_max_beam_blk with N/A */

   /* add_tab_page2_str_float("USE MLDA HEIGHTS", -* YES *-
    *                         "%17s",
    *                         bool_str,
    *                         "   MAX USABILITY BLOCKAGE",
    *                         "%9.0f",
    *                         (float) inbuf->qpe_adapt.dp_adapt.Z_max_beam_blk, -* 50 *-
    *                         "%",
    *                         outbuf,
    *                         offset);
    */

   sprintf(temp,"%s%19s%s%s",
                "MELTING LAYER SOURCE", 
                ML_Source,
                "  MAX USABILITY BLOCKAGE",
                "       N/A");

   add_tab_str(0, temp, outbuf, offset);

   /* PAGE 2 LINE 3 */

   add_tab_page2_floats("KDP MULTIPLIER COEFFICIENT",
                        "%5.0f",
                        inbuf->qpe_adapt.dp_adapt.Kdp_mult, /* 44 */
                        NULL,
                        "MIN KDP USAGE RATE",
                        "%15.1f",
                        inbuf->qpe_adapt.dp_adapt.Kdp_min_usage_rate, /* 10.0 */
                        "MM/HR",
                        outbuf,
                        offset);

   /* PAGE 2 LINE 4 */

   add_tab_page2_floats("KDP POWER COEFFICIENT",
                        "%14.3f",
                        inbuf->qpe_adapt.dp_adapt.Kdp_power, /* 0.822 */
                        NULL,
                        "WET SNOW MULT COEFF",
                        "%14.1f",
                        inbuf->qpe_adapt.dp_adapt.Ws_mult, /* 0.6 */
                        NULL,
                        outbuf,
                        offset);

   /* PAGE 2 LINE 5 */

   add_tab_page2_floats("Z-R MULTIPLIER COEFFICIENT",
                        "%5.0f",
                        inbuf->qpe_adapt.dp_adapt.Z_mult, /* 300 */
                        NULL,
                        "GRAUPEL MULT COEFF",
                        "%15.1f",
                        inbuf->qpe_adapt.dp_adapt.Gr_mult, /* 0.8 */
                        NULL,
                        outbuf,
                        offset);

   /* PAGE 2 LINE 6 */

   add_tab_page2_floats("Z-R POWER COEFFICIENT",
                        "%12.1f",
                        inbuf->qpe_adapt.dp_adapt.Z_power, /* 1.4 */
                        NULL,
                       "RAIN/HAIL MULT COEFF",
                        "%13.1f",
                        inbuf->qpe_adapt.dp_adapt.Rh_mult, /* 0.8 */
                        NULL,
                        outbuf,
                        offset);

   /* PAGE 2 LINE 7 */

   add_tab_page2_floats("ZDR/Z MULTIPLIER COEFF",
                        "%14.4f",
                        inbuf->qpe_adapt.dp_adapt.Zdr_z_mult, /* 0.0067 */
                        NULL,
                       "DRY SNOW MULT COEFF",
                        "%14.1f",
                        inbuf->qpe_adapt.dp_adapt.Ds_mult, /* 2.8 */
                        NULL,
                        outbuf,
                        offset);

   /* PAGE 2 LINE 8 */

   add_tab_page2_floats("ZDR/Z POWER COEFF FOR Z",
                        "%12.3f",
                        inbuf->qpe_adapt.dp_adapt.Zdr_z_power, /* 0.927 */
                        NULL,
                        "CRYSTALS MULT COEFF",
                        "%14.1f",
                        inbuf->qpe_adapt.dp_adapt.Ic_mult, /* 2.8 */
                        NULL,
                        outbuf,
                        offset);

   /* PAGE 2 LINE 9 */

   add_tab_page2_floats("ZDR/Z POWER COEFF FOR ZDR",
                        "%9.2f",
                        inbuf->qpe_adapt.dp_adapt.Zdr_zdr_power, /* -3.43 */
                        NULL,
                        "% RATE GRID FILLED THRESH",
                        "%8.1f",
                        inbuf->qpe_adapt.dp_adapt.Grid_is_full, /* 99.7 */
                        "%",
                        outbuf,
                        offset);

   /* PAGE 2 LINE 10 */

   /* 20081125 Two new adaptable parameters are not displayed in the
    *          layer 2/TAB. They are the Max_vols_per_hour, and
    *          the Min_early_term_ang. They may be added here,
    *          and to the ICD, at a later date.
    *
    * 20111031 Ward Replaced RhoHV_min_rate with art_corr */

   add_tab_page2_floats("MIN CORREL COEFF FOR PRECIP",
                        "%9.4f",
                        inbuf->qpe_adapt.dpprep_adapt.art_corr, /* 0.8000 */
                        NULL,
                        "PAIF PRECIP RATE THRESH",
                        "%10.1f",
                        inbuf->qpe_adapt.dp_adapt.Paif_rate, /* 0.5 */
                        "MM/HR",
                        outbuf,
                        offset);

   /* PAGE 2 LINE 11 */

   add_tab_page2_floats("MIN CORREL COEFF FOR KDP",
                        "%12.4f",
                        inbuf->qpe_adapt.dpprep_adapt.corr_thresh, /* 0.9000 */
                        NULL,
                        "PAIF PRECIP AREA THRESH",
                        "%8.0f",
                        (float) inbuf->qpe_adapt.dp_adapt.Paif_area, /* 80 */
                        "  KM**2",
                        outbuf,
                        offset);

   /* PAGE 2 LINE 12 */

   /* 20090824 At Mark Fresch request, changed DBZ to dBZ */

   add_tab_page2_floats("MAX REFLECTIVITY",
                        "%17.1f",
                        inbuf->qpe_adapt.dp_adapt.Refl_max, /* 53.0 */
                        "dBZ",
                        "PRECIP DETECTION TIME THRESH",
                        "%3.0f",
                        (float) inbuf->qpe_adapt.prep_adapt.rain_time_thresh, /* 60 */
                        "  MIN",
                        outbuf,
                        offset);

   /* PAGE 2 LINE 13 */

   /* 20101124 Ward Added max rate.
    * 20111031 Ward Replaced all ISOLATES with N/A */

   /* add_tab_page2_floats("MAX RATE",
    *                    "%26.2f",
    *                    inbuf->qpe_adapt.dp_adapt.Max_precip_rate, -* 200.0 *-
    *                    "MM/HR",
    *                    "ISOLATE ZONE SIZE",
    *                    "%16.1f",
    *                    0.0,
    *                    "KM",
    *                    outbuf,
    *                    offset);
    */

   sprintf(temp,"%s%26.2f%s%s%s",
                "MAX RATE",
                inbuf->qpe_adapt.dp_adapt.Max_precip_rate, /* 200.0 */
                " MM/HR",
                " ISOLATE ZONE SIZE",
                "            N/A");

   add_tab_str(0, temp, outbuf, offset);

   /* PAGE 2 LINE 14 */

   /* add_tab_page2_floats("ISOLATE NEIGHBOR THRESH",
    *                     "%8.0f",
    *                     (float) 0,
    *                     "BINS",
    *                     "ISOLATE RATE THRESH",
    *                     "%14.1f",
    *                     0.0,
    *                     "MM/HR",
    *                     outbuf,
    *                     offset);
    */

   sprintf(temp,"%s%s%s%s",
                "ISOLATE NEIGHBOR THRESH",
                "      N/A",
                "         ISOLATE RATE THRESH",
                "          N/A");

   add_tab_str(0, temp, outbuf, offset);

   /* END OF PAGE 2 */

   tab_page_end(2, outbuf, offset);

   /* PAGE 3 LINE 1 ***********************************************************/

   add_tab_page3_float("NUMBER OF EXCLUSION ZONES",
                       "%16.0f",
                       (float) inbuf->qpe_adapt.dp_adapt.Num_zones, /* 0 */
                       NULL,
                       outbuf,
                       offset);

   /* PAGE 3 LINE 2 */

   add_tab_page3_float("THRESHOLD ELAPSED TIME TO RESTART",
                       "%8.0f",
                       (float) inbuf->qpe_adapt.accum_adapt.restart_time, /* 60 */
                       "MINUTES",
                       outbuf,
                       offset);

   /* PAGE 3 LINE 3 */

   add_tab_page3_float("MAXIMUM TIME FOR INTERPOLATION",
                       "%11.0f",
                       (float) inbuf->qpe_adapt.accum_adapt.max_interp_time, /* 30 */
                       "MINUTES",
                       outbuf,
                       offset);

   /* PAGE 3 LINE 4 */

   add_tab_page3_float("MAXIMUM HOURLY ACCUMULATION VALUE",
                       "%8.0f",
                       (float) inbuf->qpe_adapt.accum_adapt.max_hourly_acc, /* 800 */
                       "MM",
                       outbuf,
                       offset);

   /* PAGE 3 LINE 5 */

   add_tab_page3_float("TIME BIAS ESTIMATION",
                       "%21.0f",
                       (float) inbuf->qpe_adapt.adj_adapt.time_bias, /* 50 */
                       "MINUTES",
                       outbuf,
                       offset);

   /* PAGE 3 LINE 6 */

   add_tab_page3_float("THRESHOLD NUMBER OF GAGE-RADAR PAIRS",
                       "%5.0f",
                       (float) inbuf->qpe_adapt.adj_adapt.num_grpairs, /* 10 */
                       NULL,
                       outbuf,
                       offset);

   /* PAGE 3 LINE 7 */

   add_tab_page3_float("RESET BIAS VALUE",
                       "%27.1f",
                       inbuf->qpe_adapt.adj_adapt.reset_bias, /* 1.0 */
                       NULL,
                       outbuf,
                       offset);

   /* PAGE 3 LINE 8 */

   add_tab_page3_float("LONGEST ALLOWABLE LAG",
                       "%20.0f",
                       (float) inbuf->qpe_adapt.adj_adapt.longst_lag, /* 168 */
                       "HOURS",
                       outbuf,
                       offset);

   /* PAGE 3 LINE 9 */

   if (inbuf->qpe_adapt.dpprep_adapt.isdp_est != -99)
      {
      add_tab_page3_float("RPG ESTIMATED ISDP",
                          "%23.0f",
                          (float) inbuf->qpe_adapt.dpprep_adapt.isdp_est, /* Computed by dpprep */
                          "DEG",
                          outbuf,
                          offset);
      }
   else
      {
      strcpy(ISDP_insuf, "INSUFFICIENT DATA");
      sprintf(temp,"%s%20s",
              "RPG ESTIMATED ISDP", 
              ISDP_insuf);

      add_tab_str(0, temp, outbuf, offset);
      }      

   /* PAGE 3 LINE 10 */

   if(inbuf->qpe_adapt.dpprep_adapt.isdp_apply == 0)
      {
      strcpy(ISDP_app, " NO");
      }
   else
      {
      strcpy(ISDP_app, "YES"); 
      }
   sprintf(temp,"%s%20s",
                "ISDP APPLIED TO DATA?", 
                ISDP_app);

   add_tab_str(0, temp, outbuf, offset);

   /* PAGE 3 LINE 11 */
   
   dy = inbuf->qpe_adapt.dpprep_adapt.isdp_yy;
   dm = inbuf->qpe_adapt.dpprep_adapt.isdp_mm;
   dd = inbuf->qpe_adapt.dpprep_adapt.isdp_dd;
   sprintf(temp, "DATE OF ISDP ESTIMATE:           %02d/%02d/%02d",
           dm, dd, dy);

   add_tab_str(0, temp, outbuf, offset);

   /* PAGE 3 LINE 12 */
   
   dy = inbuf->qpe_adapt.dpprep_adapt.isdp_yy;
   dm = inbuf->qpe_adapt.dpprep_adapt.isdp_mm;
   dd = inbuf->qpe_adapt.dpprep_adapt.isdp_dd;
   hr = inbuf->qpe_adapt.dpprep_adapt.isdp_hr;
   min= inbuf->qpe_adapt.dpprep_adapt.isdp_min;
   sprintf(temp, "TIME OF ISDP ESTIMATE:              %02d:%02d",
            hr, min);

   add_tab_str(0, temp, outbuf, offset);

   /* END OF PAGE 3 */

   tab_page_end(3, outbuf, offset);

   /* 20120123 Ward CCR NA12-00058: Add exclusion zones */

   if(inbuf->qpe_adapt.dp_adapt.Num_zones > 0)
   {
      sprintf(zone_hdr1, "%s %s",
              "                           ",
              "DUAL POL EXCLUSION ZONES");

      sprintf(zone_hdr2, " ");

      sprintf(zone_hdr3, "%s %s %s %s %s %s %s",
              "            ",
              "ZONE",
              "BEG AZM",
              "END AZM",
              "BEG RNG (NM)",
              "END RNG (NM)",
              "ELEV ANG");

      for(zone=1; zone<=inbuf->qpe_adapt.dp_adapt.Num_zones; zone++)
      {
          if((zone == 1) || (zone == (LINES_PER_PAGE-2)))
          {
             add_tab_str(0, zone_hdr1, outbuf, offset);
             add_tab_str(0, zone_hdr2, outbuf, offset);
             add_tab_str(0, zone_hdr3, outbuf, offset);
          }

          add_tab_zone(zone, outbuf, offset);

          if(zone == (LINES_PER_PAGE-3))
          {
             /* END OF PAGE 4 */

             tab_page_end(4, outbuf, offset);
          }
      }

      if(inbuf->qpe_adapt.dp_adapt.Num_zones < (LINES_PER_PAGE-3))
      {
         /* END OF PAGE 4 */

         tab_page_end(4, outbuf, offset);
      }
      else if(inbuf->qpe_adapt.dp_adapt.Num_zones > (LINES_PER_PAGE-3))
      {
         /* END OF PAGE 5 */

         tab_page_end(5, outbuf, offset);
      }
   }

   /* Step 4 - Fill in message header fields.                                 *
    *          Update the total block size of the TAB block in the TAB header */

   TAB_size = *offset - (graphic_size + sym_block_len);
   if(DP_PRECIP_4BIT_DEBUG)
      fprintf( stderr,"TAB_size = %d bytes\n",TAB_size);

   est_TAB_size = tab_hdr_size             +
                  graphic_size             +
                  alpha_hdr_size           +
                  LINES_PER_PAGE * ad_size +
                  num_tab_pages  * sizeof(short);

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr,"est_TAB_size = %d bytes\n", est_TAB_size);

   tab_hdr.divider  = (short) -1;
   tab_hdr.block_id = (short) 3;
   RPGC_set_product_int((void*) &tab_hdr.block_length, (unsigned int) TAB_size);

   memcpy(outbuf + graphic_size + sym_block_len, &tab_hdr, tab_hdr_size);

   /* Update the following fields in the TAB message header block *
    * insert the proper product codes for products 171            *
    *                                                             *
    * msg_len is the size of tab minus its header                 */

   RPGC_set_product_int( (void *) &gprod_ptr->msg_len,
                         (unsigned int) (TAB_size - tab_hdr_size));

   if(inbuf->supl.null_Storm_Total)
      gprod_ptr->n_blocks = (short) 3; /* 3 blocks msg_hdr_block     and tab */
   else
      gprod_ptr->n_blocks = (short) 4; /* 4 blocks msg_hdr_block/pdb and tab */

   /* These next 3 function calls are only done for the STA product because *
    * it's the only dual-pol product with a TAB.  For all the other         *
    * dual-pol products, this information is set with a single call to      *
    * RPGC_set_prod_block_offsets() in header_product (headerProd.c)        */

   /* Symb (alpha) block header */

   if(inbuf->supl.null_Storm_Total)
   {
      RPGC_set_product_int((void *) &gprod_ptr->sym_off, 0);
   }
   else
   {
      RPGC_set_product_int((void *) &gprod_ptr->sym_off,
                            graphic_size / sizeof (short));
   }

   /* No GAB block */

   RPGC_set_product_int( (void *) &gprod_ptr->gra_off, 0 );

   *tab_offset = (graphic_size + sym_block_len) / sizeof (short);

   /* TAB block */

   if(DP_PRECIP_4BIT_DEBUG)
   {
      int temp_val;
      RPGC_set_product_int( (void *) &gprod_ptr->tab_off, *tab_offset);

      RPGC_get_product_int( (void *) &gprod_ptr->sym_off, (void *) &temp_val);
      fprintf(stderr,"gprod_ptr->sym_off: %d shorts\n", temp_val);

      RPGC_get_product_int( (void *) &gprod_ptr->tab_off, (void *) &temp_val);
      fprintf(stderr,"gprod_ptr->tab_off: %d shorts\n", temp_val);

      fprintf(stderr,"Adding data %d bytes from beginning of outbuf\n",
                      sym_block_len + tab_hdr_size);
   }

   /* Store updated TAB message header block into the output buffer */

   memcpy (outbuf + graphic_size + sym_block_len + tab_hdr_size,
           &msg_hdr_block, graphic_size);

   /* Length of entire STA in bytes - used in headerProduct */

   *tab_len = *offset;

   if(DP_PRECIP_4BIT_DEBUG)
   {
      fprintf( stderr,"tab_offset: %d shorts (%d bytes) from start of ORPG "
                     "Message Header Block\n", *tab_offset, (*tab_offset * 2) );
      fprintf( stderr,"     sym_block_len: %d bytes from start of Product "
                     "Symbology Block\n", sym_block_len );
      fprintf( stderr,"            offset: %d bytes returned\n", *offset);
      fprintf(stderr,"            tab_len: %d bytes from start of ORPG "
                     "Message Header Block\n", *tab_len );

   }

   return(FUNCTION_SUCCEEDED);

} /* end STA_tab() ================================== */

/******************************************************************************
   Filename: DP_Precip_4bit_STA_tab.c

   Description
   ===========
      msecs_to_string() writes milliseconds since midnight to an HH:MM string
   It always outputs 5 characters.

   Input:
      int   time - milliseconds since midnight

   Output:
      char* str  - the time as a string

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----        -------    ----------         ------------------------
   10/07       0000       Cham Pham          Initial Implementation
   02/09       0001       James Ward         Added NULL pointer check
******************************************************************************/

int msecs_to_string(int time, char* str)
{
    int	h, m, s, frac;

    /* Check for NULL pointers. */

    if(pointer_is_NULL(str, "msecs_to_string", "str"))
       return(NULL_POINTER);

    memset(str, 0, 6);

    frac = time - 1000 * (time / 1000);
    time = time / 1000;
    h    = time / 3600;
    time = time - h * 3600;
    m    = time / 60;
    s    = time - m * 60;

    sprintf(str, "%02d:%02d", h, m);

    return(FUNCTION_SUCCEEDED);

} /* end msecs_to_string() ===================================================== */

/******************************************************************************
   Filename: DP_Precip_4bit_STA_tab.c

   Description
   ===========
      add_tab_str() adds a string to a TAB page without any special format.

   Input:
      int     num_leading_spaces - number of leading spaces
      char*   str                - string
      char*   outbuf             - output buffer
      int*    offset             - offset into output buffer

   Output:

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ----------         ----------------------
   20080813    0000       James Ward         Initial Implementation
******************************************************************************/

int add_tab_str(int   num_leading_spaces,
                char* str,
                char* outbuf,
                int*  offset)
{
   char line[LINE_WIDTH + 1];
   int  i, len;

   static unsigned int ad_size = sizeof(alpha_data_t);

   alpha_data_t alpha_data; /* output structure */

   /* Check for NULL pointers. str can be NULL */

   if(pointer_is_NULL(outbuf, "add_tab_str", "outbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "add_tab_str", "offset"))
      return(NULL_POINTER);

   /* Note: line is a convenience array for alpha_data.data.         *
    * alpha_data.data is not really a string, but an array           *
    * of characters, filled out to LINE_WIDTH, with no trailing NULL */

   memset(line, 0, LINE_WIDTH + 1);

   /* Add leading spaces */

   for(i=0; i<num_leading_spaces; i++)
      line[i] = ' ';

   len = strlen(line);

   /* Add the string */

   if(str != NULL)
   {
      sprintf(&(line[len]), "%s", str);

      len = strlen(line);
   }

   /* Pad with spaces out to LINE_WIDTH. */

   for(i = len; i < LINE_WIDTH; i++)
      line[i] = ' ';

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr, "%80.80s\n", line);

   /* Fill alpha_data structure */

   alpha_data.num_char = LINE_WIDTH;

   strncpy(alpha_data.data, line, LINE_WIDTH);

   /* Copy to output buffer */

   memcpy(outbuf + *offset, &alpha_data, ad_size);

   /* Readjust output buffer offset */

   *offset += ad_size;

   return(FUNCTION_SUCCEEDED);

} /* end add_tab_str() ===================================================== */

/******************************************************************************
   Filename: DP_Precip_4bit_STA_tab.c

   Description
   ===========
      add_tab_page1_float() adds a float to a TAB page in the page 1 format:

      LABEL1 - float1

   Input:
      int   num_leading_spaces - number of leading spaces
      char* label              - line label
      char* format             - format string for float value
      float value              - float value
      char* outbuf             - output buffer
      int*  offset             - offset into output buffer

   Output:

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ----------         ----------------------
   20080813    0000       James Ward         Initial Implementation
******************************************************************************/

int add_tab_page1_float(int   num_leading_spaces,
                        char* label,
                        char* format,
                        float value,
                        char* outbuf,
                        int*  offset)
{
   char line[LINE_WIDTH + 1];
   int  i, len;

   static unsigned int ad_size = sizeof(alpha_data_t);

   alpha_data_t alpha_data; /* output structure */

   /* Check for NULL pointers. */

   if(pointer_is_NULL(label, "add_tab_page1_float", "label"))
      return(NULL_POINTER);

   if(pointer_is_NULL(format, "add_tab_page1_float", "format"))
      return(NULL_POINTER);

   if(pointer_is_NULL(outbuf, "add_tab_page1_float", "outbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "add_tab_page1_float", "offset"))
      return(NULL_POINTER);

   /* Note: line is a convenience array for alpha_data.data.         *
    * alpha_data.data is not really a string, but an array           *
    * of characters, filled out to LINE_WIDTH, with no trailing NULL */

   memset(line, 0, LINE_WIDTH + 1);

   /* Add leading spaces */

   for(i=0; i<num_leading_spaces; i++)
      line[i] = ' ';

   /* Add label */

   strcat(line, label);

   len = strlen(line);

   /* Add spaces to center point */

   for(i=len; i<LINE_WIDTH/2; i++)
      line[i] = ' ';

   len = strlen(line);

   /* Add " - " */

   strcat(line, " - ");

   len = strlen(line);

   /* Add the value */

   sprintf(&(line[len]), format, value);

   len = strlen(line);

   /* Pad with spaces out to LINE_WIDTH. */

   for(i = len; i < LINE_WIDTH; i++)
      line[i] = ' ';

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr, "%80.80s\n", line);

   /* Fill alpha_data structure */

   alpha_data.num_char = LINE_WIDTH;

   strncpy(alpha_data.data, line, LINE_WIDTH);

   /* Copy to output buffer */

   memcpy(outbuf + *offset, &alpha_data, ad_size);

   /* Readjust output buffer offset */

   *offset += ad_size;

   return(FUNCTION_SUCCEEDED);

} /* end add_tab_page1_float() ===================================================== */

/******************************************************************************
   Filename: DP_Precip_4bit_STA_tab.c

   Description
   ===========
      add_tab_page1_str() adds a string to a TAB page in the page 1 format:

      LABEL - string

   Input:
      int   num_leading_spaces - number of leading spaces
      char* label              - line label
      char* format             - format string
      char* str                - string
      char* outbuf             - output buffer
      int*  offset             - offset into output buffer

   Output:

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ----------         ----------------------
   20080813    0000       James Ward         Initial Implementation
******************************************************************************/

int add_tab_page1_str(int   num_leading_spaces,
                      char* label,
                      char* format,
                      char* str,
                      char* outbuf,
                      int*  offset)
{
   char line[LINE_WIDTH + 1];
   int  i, len;

   static unsigned int ad_size = sizeof(alpha_data_t);

   alpha_data_t alpha_data; /* output structure */

   /* Check for NULL pointers. */

   if(pointer_is_NULL(label, "add_tab_page1_str", "label"))
      return(NULL_POINTER);

   if(pointer_is_NULL(format, "add_tab_page1_str", "format"))
      return(NULL_POINTER);

   if(pointer_is_NULL(str, "add_tab_page1_str", "str"))
      return(NULL_POINTER);

   if(pointer_is_NULL(outbuf, "add_tab_page1_str", "outbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "add_tab_page1_str", "offset"))
      return(NULL_POINTER);

   /* Note: line is a convenience array for alpha_data.data.         *
    * alpha_data.data is not really a string, but an array           *
    * of characters, filled out to LINE_WIDTH, with no trailing NULL */

   memset(line, 0, LINE_WIDTH + 1);

   /* Add leading spaces */

   for(i=0; i<num_leading_spaces; i++)
      line[i] = ' ';

   /* Add label */

   strcat(line, label);

   len = strlen(line);

   /* Add spaces to center point */

   for(i=len; i<LINE_WIDTH/2; i++)
      line[i] = ' ';

   len = strlen(line);

   /* Add " - " */

   strcat(line, " - ");

   len = strlen(line);

   /* Add the string */

   if(str != NULL)
   {
      sprintf(&(line[len]), format, str);

      len = strlen(line);
   }

   /* Pad with spaces out to LINE_WIDTH. */

   for(i = len; i < LINE_WIDTH; i++)
      line[i] = ' ';

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr, "%80.80s\n", line);

   /* Fill alpha_data structure */

   alpha_data.num_char = LINE_WIDTH;

   strncpy(alpha_data.data, line, LINE_WIDTH);

   /* Copy to output buffer */

   memcpy(outbuf + *offset, &alpha_data, ad_size);

   /* Readjust output buffer offset */

   *offset += ad_size;

   return(FUNCTION_SUCCEEDED);

} /* end add_tab_page1_str() ===================================================== */

/******************************************************************************
   Filename: DP_Precip_4bit_STA_tab.c

   Description
   ===========
      add_tab_page2_floats() adds two floats to a TAB page in the page 2 format:

            LABEL1    float1    units1    LABEL2    float2    units2

   Note: Format strings may be padded with beginning spaces to line
         up the float values. For ease of printing, we cast some ints
         into floats, and format them as "%X.0f", where the precision of 0
         suppresses the decimal point.

   Input:
      char* label1  - float1 label
      char* format1 - format string for float1 value
      float value1  - float1 value
      char* units1  - float1 units
      char* label2  - float2 label
      char* format2 - format string for float2 value
      float value2  - float2 value
      char* units2  - float2 units
      char* outbuf  - output buffer
      int*  offset  - offset into output buffer

   Output:

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ----------         ----------------------
   20080813    0000       James Ward         Initial Implementation
******************************************************************************/

int add_tab_page2_floats(char* label1,
                         char* format1,
                         float value1,
                         char* units1,
                         char* label2,
                         char* format2,
                         float value2,
                         char* units2,
                         char* outbuf,
                         int*  offset)
{
   char line[LINE_WIDTH + 1];
   int  i, len;

   static unsigned int ad_size = sizeof(alpha_data_t);

   alpha_data_t alpha_data; /* output structure */

   /* Check for NULL pointers. units can be NULL. */

   if(pointer_is_NULL(label1, "add_tab_page2_floats", "label1"))
      return(NULL_POINTER);

   if(pointer_is_NULL(format1, "add_tab_page2_floats", "format1"))
      return(NULL_POINTER);

   if(pointer_is_NULL(label2, "add_tab_page2_floats", "label2"))
      return(NULL_POINTER);

   if(pointer_is_NULL(format2, "add_tab_page2_floats", "format2"))
      return(NULL_POINTER);

   if(pointer_is_NULL(outbuf, "add_tab_page2_floats", "outbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "add_tab_page2_floats", "offset"))
      return(NULL_POINTER);

   /* Note: line is a convenience array for alpha_data.data.         *
    * alpha_data.data is not really a string, but an array           *
    * of characters, filled out to LINE_WIDTH, with no trailing NULL */

   memset(line, 0, LINE_WIDTH + 1);

   /* Add label1 */

   strcat(line, label1);

   len = strlen(line);

  /* Add value1 */

   sprintf(&(line[len]), format1, value1);

   len = strlen(line);

  /* Add units1 */

   if(units1 != NULL)
   {
      sprintf(&(line[len]), " %s", units1);

      len = strlen(line);
   }

   /* If we add spaces to the exact center point, *
    * then some of the units2 are off the page.   *
    * Move the 2nd column to the left.            */

   for(i=len; i<(LINE_WIDTH/2) + 1; i++)
      line[i] = ' ';

   len = strlen(line);

   /* Add label2 */

   strcat(line, label2);

   len = strlen(line);

  /* Add value2 */

   sprintf(&(line[len]), format2, value2);

   len = strlen(line);

  /* Add units2 */

   if(units2 != NULL)
   {
      sprintf(&(line[len]), " %s", units2);

      len = strlen(line);
   }

   /* Pad with spaces out to LINE_WIDTH. */

   for(i = len; i < LINE_WIDTH; i++)
      line[i] = ' ';

   if (DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr, "%80.80s\n", line);

   /* Fill alpha_data structure */

   alpha_data.num_char = LINE_WIDTH;

   strncpy(alpha_data.data, line, LINE_WIDTH);

   /* Copy to output buffer */

   memcpy(outbuf + *offset, &alpha_data, ad_size);

   /* Readjust output buffer offset */

   *offset += ad_size;

   return(FUNCTION_SUCCEEDED);

} /* end add_tab_page2_floats() ================================= */

/******************************************************************************
   Filename: DP_Precip_4bit_STA_tab.c

   Description
   ===========
      add_tab_page2_str_float() adds a string and a float to a TAB page
   in the page 2 format:

            LABEL1    str1    LABEL2    float2    units2

   Note: Format strings may be padded with beginning spaces to line
         up the float values. For ease of printing, we cast some ints
         into floats, and format them as "%X.0f", where the precision of 0
         suppresses the decimal point.

   Input:
      char* label1  - str1 label
      float value1  - str1 value
      char* label2  - float2 label
      char* format2 - format string for float2 value
      float value2  - float2 value
      char* units2  - float2 units
      char* outbuf  - output buffer
      int*  offset  - offset into output buffer

   Output:

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ----------         ----------------------
   20090218    0000       James Ward         Initial Implementation
******************************************************************************/

int add_tab_page2_str_float(char* label1,
                            char* format1,
                            char* str1,
                            char* label2,
                            char* format2,
                            float value2,
                            char* units2,
                            char* outbuf,
                            int*  offset)
{
   char line[LINE_WIDTH + 1];
   int  i, len;

   static unsigned int ad_size = sizeof(alpha_data_t);

   alpha_data_t alpha_data; /* output structure */

   /* Check for NULL pointers. units can be NULL. */

   if(pointer_is_NULL(label1, "add_tab_page2_str_float", "label1"))
      return(NULL_POINTER);

   if(pointer_is_NULL(format1, "add_tab_page2_str_float", "format1"))
      return(NULL_POINTER);

   if(pointer_is_NULL(str1, "add_tab_page2_str_float", "str1"))
      return(NULL_POINTER);

   if(pointer_is_NULL(label2, "add_tab_page2_str_float", "label2"))
      return(NULL_POINTER);

   if(pointer_is_NULL(format2, "add_tab_page2_str_float", "format2"))
      return(NULL_POINTER);

   if(pointer_is_NULL(outbuf, "add_tab_page2_str_float", "outbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "add_tab_page2_str_float", "offset"))
      return(NULL_POINTER);

   /* Note: line is a convenience array for alpha_data.data.         *
    * alpha_data.data is not really a string, but an array           *
    * of characters, filled out to LINE_WIDTH, with no trailing NULL */

   memset(line, 0, LINE_WIDTH + 1);

   /* Add label1 */

   strcat(line, label1);

   len = strlen(line);

   /* Add str1 */

   sprintf(&(line[len]), format1, str1);

   len = strlen(line);

   /* If we add spaces to the exact center point, *
    * then some of the units2 are off the page.   *
    * Move the 2nd column to the left.            */

   for(i=len; i<(LINE_WIDTH/2) - 2; i++)
      line[i] = ' ';

   len = strlen(line);

   /* Add label2 */

   strcat(line, label2);

   len = strlen(line);

  /* Add value2 */

   sprintf(&(line[len]), format2, value2);

   len = strlen(line);

  /* Add units2 */

   if(units2 != NULL)
   {
      sprintf(&(line[len]), " %s", units2);

      len = strlen(line);
   }

   /* Pad with spaces out to LINE_WIDTH. */

   for(i = len; i < LINE_WIDTH; i++)
      line[i] = ' ';

   if (DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr, "%80.80s\n", line);

   /* Fill alpha_data structure */

   alpha_data.num_char = LINE_WIDTH;

   strncpy(alpha_data.data, line, LINE_WIDTH);

   /* Copy to output buffer */

   memcpy(outbuf + *offset, &alpha_data, ad_size);

   /* Readjust output buffer offset */

   *offset += ad_size;

   return(FUNCTION_SUCCEEDED);

} /* end add_tab_page2_str_float() ================================= */

/******************************************************************************
   Filename: DP_Precip_4bit_STA_tab.c

   Description
   ===========
      add_tab_page3_float() adds a float to a TAB page in the page 3 format:

            LABEL    float    units

   Note: Format strings may be padded with beginning spaces to line
         up the float values. For ease of printing, we cast some ints
         into floats, and format them as "%X.0f", where the precision of 0
         suppresses the decimal point.

   Input:
      char* label  - float label
      char* format - format string for float value
      float value  - float value
      char* units  - float units
      char* outbuf - output buffer
      int*  offset - offset into output buffer

   Output:

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ----------         ----------------------
   20080813    0000       James Ward         Initial Implementation
******************************************************************************/

int add_tab_page3_float(char* label,
                        char* format,
                        float value,
                        char* units,
                        char* outbuf,
                        int*  offset)
{
   char line[LINE_WIDTH + 1];
   int  i, len;

   static unsigned int ad_size = sizeof(alpha_data_t);

   alpha_data_t alpha_data; /* output structure */

   /* Check for NULL pointers. units can be NULL. */

   if(pointer_is_NULL(label, "add_tab_page3_float", "label"))
      return(NULL_POINTER);

   if(pointer_is_NULL(format, "add_tab_page3_float", "format"))
      return(NULL_POINTER);

   if(pointer_is_NULL(outbuf, "add_tab_page3_float", "outbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "add_tab_page3_float", "offset"))
      return(NULL_POINTER);

   /* Note: line is a convenience array for alpha_data.data.         *
    * alpha_data.data is not really a string, but an array           *
    * of characters, filled out to LINE_WIDTH, with no trailing NULL */

   memset(line, 0, LINE_WIDTH + 1);

   /* Add label */

   strcat(line, label);

   len = strlen(line);

  /* Add value */

   sprintf(&(line[len]), format, value);

   len = strlen(line);

  /* Add units */

   if(units != NULL)
   {
      sprintf(&(line[len]), " %s", units);

      len = strlen(line);
   }

   /* Pad with spaces out to LINE_WIDTH. */

   for(i = len; i < LINE_WIDTH; i++)
      line[i] = ' ';

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr, "%80.80s\n", line);

   /* Fill alpha_data structure */

   alpha_data.num_char = LINE_WIDTH;

   strncpy(alpha_data.data, line, LINE_WIDTH);

   /* Copy to output buffer */

   memcpy(outbuf + *offset, &alpha_data, ad_size);

   /* Readjust output buffer offset */

   *offset += ad_size;

   return(FUNCTION_SUCCEEDED);

} /* end add_tab_page3_float() ================================= */

/******************************************************************************
   Filename: DP_Precip_4bit_STA_tab.c

   Description
   ===========
      add_tab_page3_str() adds a string to a TAB page in the page 3 format:

      LABEL string

   Input:
      char* label              - line label
      char* format             - format string
      char* str                - string
      char* outbuf             - output buffer
      int*  offset             - offset into output buffer

   Output:

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ----------         ----------------------
   20080813    0000       James Ward         Initial Implementation
******************************************************************************/

int add_tab_page3_str(char* label,
                      char* format,
                      char* str,
                      char* outbuf,
                      int*  offset)
{
   char line[LINE_WIDTH + 1];
   int  i, len;

   static unsigned int ad_size = sizeof(alpha_data_t);

   alpha_data_t alpha_data; /* output structure */

   /* Check for NULL pointers. str can be NULL */

   if(pointer_is_NULL(label, "add_tab_page3_str", "label"))
      return(NULL_POINTER);

   if(pointer_is_NULL(format, "add_tab_page3_str", "format"))
      return(NULL_POINTER);

   if(pointer_is_NULL(outbuf, "add_tab_page3_str", "outbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "add_tab_page3_str", "offset"))
      return(NULL_POINTER);

   /* Note: line is a convenience array for alpha_data.data.         *
    * alpha_data.data is not really a string, but an array           *
    * of characters, filled out to LINE_WIDTH, with no trailing NULL */

   memset(line, 0, LINE_WIDTH + 1);

   /* Add label */

   strcat(line, label);

   len = strlen(line);

   /* Add the string */

   if(str != NULL)
   {
      sprintf(&(line[len]), format, str);

      len = strlen(line);
   }

   /* Pad with spaces out to LINE_WIDTH. */

   for(i = len; i < LINE_WIDTH; i++)
      line[i] = ' ';

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr, "%80.80s\n", line);

   /* Fill alpha_data structure */

   alpha_data.num_char = LINE_WIDTH;

   strncpy(alpha_data.data, line, LINE_WIDTH);

   /* Copy to output buffer */

   memcpy(outbuf + *offset, &alpha_data, ad_size);

   /* Readjust output buffer offset */

   *offset += ad_size;

   return(FUNCTION_SUCCEEDED);

} /* end add_tab_page3_str() ================================= */

/******************************************************************************
   Filename: DP_Precip_4bit_STA_tab.c

   Description
   ===========
      tab_page_end() does end of TAB page processing

   Input:
      int   page_num - page number, for error messages
      char* outbuf   - output buffer
      int*  offset   - offset into output buffer

   Output:

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ----------         ----------------------
   20080813    0000       James Ward         Initial Implementation
******************************************************************************/

int tab_page_end(int page_num, char* outbuf, int* offset)
{
   char  msg[200];              /* stderr msg                    */
   short end_flag = (short) -1; /* 0xFFFF = end of TAB page flag */

   /* Check for NULL pointers. */

   if(pointer_is_NULL(outbuf, "tab_page_end", "outbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "tab_page_end", "offset"))
      return(NULL_POINTER);

    /* Copy to output buffer */

   memcpy(outbuf + *offset, &end_flag, sizeof(short));

   *offset += sizeof(short);

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr, "End tab page %d\n\n", page_num);

   /* Quality control - Check and be sure that we are not near
    * the allocated end size of the output buffer. */

   if(*offset >= SIZE_4BIT)
   {
     sprintf(msg, "tab page %d offset %d >= SIZE_4BIT %d %s\n",
                   page_num, *offset, SIZE_4BIT,
                   "(outbuf allocation size)");

      RPGC_log_msg(GL_INFO, msg);
      if(DP_PRECIP_4BIT_DEBUG)
         fprintf(stderr, msg);
   }

   return(FUNCTION_SUCCEEDED);

} /* end tab_page_end() ===================================================== */

/******************************************************************************
   Filename: DP_Precip_4bit_STA_tab.c

   Description
   ===========
      add_tab_zone() adds an exclusion zone to a TAB in the page 4 format:

      LABEL - string

   Input:
      int   zone   - exclusion zone number
      char* outbu  - output buffer
      int*  offse  - offset into output buffer

   Output:

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
   DATE        VERSION    PROGRAMMER   NOTES
   --------    -------    ----------   -----------------------------------
   20120123    0000       James Ward   CCR NA12-00058: Add exclusion zones
******************************************************************************/

int add_tab_zone(int zone, char* outbuf, int*  offset)
{
   char line[LINE_WIDTH + 1];
   int  i, len;

   double beg_azm  = 0.0;
   double end_azm  = 0.0;
   double beg_rng  = 0.0;
   double end_rng  = 0.0;
   double elev_ang = 0.0;

   char value_id[10];

   static unsigned int ad_size = sizeof(alpha_data_t);

   alpha_data_t alpha_data; /* output structure */

   /* Check for NULL pointers. */

   if(pointer_is_NULL(outbuf, "add_tab_zone", "outbuf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "add_tab_zone", "offset"))
      return(NULL_POINTER);

   /* Collect the data from the .alg file. */

   sprintf(value_id, "Beg_azm%d", zone);

   if(RPGC_ade_get_values("alg.dp_precip.", value_id, &beg_azm) != 0)
   {
      sprintf(line, "RPGC_ade_get_values() failed - %s", value_id);
      RPGC_log_msg(GL_INFO, line);
      fprintf(stderr, line);
   }

   sprintf(value_id, "End_azm%d", zone);

   if(RPGC_ade_get_values("alg.dp_precip.", value_id, &end_azm) != 0)
   {
      sprintf(line, "RPGC_ade_get_values() failed - %s", value_id);
      RPGC_log_msg(GL_INFO, line);
      fprintf(stderr, line);
   }

   sprintf(value_id, "Beg_rng%d", zone);

   if(RPGC_ade_get_values("alg.dp_precip.", value_id, &beg_rng) != 0)
   {
      sprintf(line, "RPGC_ade_get_values() failed - %s", value_id);
      RPGC_log_msg(GL_INFO, line);
      fprintf(stderr, line);
   }

   sprintf(value_id, "End_rng%d", zone);

   if(RPGC_ade_get_values("alg.dp_precip.", value_id, &end_rng) != 0)
   {
      sprintf(line, "RPGC_ade_get_values() failed - %s", value_id);
      RPGC_log_msg(GL_INFO, line);
      fprintf(stderr, line);
   }

   sprintf(value_id, "Elev_ang%d", zone);

   if(RPGC_ade_get_values("alg.dp_precip.", value_id, &elev_ang) != 0)
   {
      sprintf(line, "RPGC_ade_get_values() failed - %s", value_id);
      RPGC_log_msg(GL_INFO, line);
      fprintf(stderr, line);
   }

   /* Note: line is a convenience array for alpha_data.data.         *
    * alpha_data.data is not really a string, but an array           *
    * of characters, filled out to LINE_WIDTH, with no trailing NULL */

   memset(line, 0, LINE_WIDTH + 1);

   sprintf(line, "%s %2d  %5.1f   %5.1f      %3d          %3d         %4.1f",
           "             ",
           zone,
           beg_azm,
           end_azm,
           (int) beg_rng,
           (int) end_rng,
           elev_ang);

   len = strlen(line);

   /* Pad with spaces out to LINE_WIDTH. */

   for(i = len; i < LINE_WIDTH; i++)
      line[i] = ' ';

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr, "%80.80s\n", line);

   /* Fill alpha_data structure */

   alpha_data.num_char = LINE_WIDTH;

   strncpy(alpha_data.data, line, LINE_WIDTH);

   /* Copy to output buffer */

   memcpy(outbuf + *offset, &alpha_data, ad_size);

   /* Readjust output buffer offset */

   *offset += ad_size;

   return(FUNCTION_SUCCEEDED);

} /* end add_tab_zone() ===================================================== */
