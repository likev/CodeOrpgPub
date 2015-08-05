/* RCS info */
/* $Author: ryans $ */
/* $Locker:  $ */
/* $Date: 2007/06/26 16:10:52 $ */
/* $Id: radcdmsg.c,v 1.1 2007/06/26 16:10:52 ryans Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#include "rcm.h"

#define ETFLAG 1
#define CAFLAGA 2
#define CAFLAGC 3
#define CONST 135.0      /* Constant as used in B5, Sec 30.6 */
#define PRIME 105.0      /* Prime meridian longitude (negative) */
#define R2KO 249.6348607 /* Constant 2RKo as used in B5 Sec 30.6 */
#define NOMEM -1         /* FLAG FOR NO MEMORY AVAILABLE */
#define RE_PROJ_SQ 40704400.0 /* Square of the earth radius constant RE_PROJ */
#define RE_PROJ 6380.0   /* Earth radius constant used to convert X/Y to
                            Lat/Long (different from 6371 to account for
                            refraction)*/

/* Global variables (file scope) */
static void* Cref_dat_ptr; /* CRPG input data pointer (360x460) */
static void* Combatt_dat_ptr; /* COMBATTR input data pointer */
static void* Et_dat_ptr; /* Echo tops input data pointer */
static void* Vad_dat_ptr; /* VAD input data pointer */
static void* Hybr_dat_ptr; /* HYBRSCN input data pointer (360x230) */

/*******************************************************************************
* Description:
*    This subroutine is the buffer control routine for the radar coded message
*    task. This task processes input from the composite reflectivity polar grid,
*    combined attributes, echo top, and vad winds.
*
* Inputs:
*    None
*
* Outputs:
*
* Globals:
*
* Notes:
*    1/11/07 R. Solomon: Ported from Fortran file a30821.f.
*******************************************************************************/
int a30821_buffer_control()
{
   void* optr = NULL;  /* Output buffer pointer */
   void* optr_sc = NULL; /* Output scratch buffer pointer */
   int ret;     /* Generic function return variable */
   int idx;     /* Generic loop index */
   int ibuf_idx;  /* Loop var for input buf indexes */
   int voln;    /* Volume number */
   int cdate;   /* Current date */
   int ctime;   /* Current time */
   int abort;   /* Flag used to determine if the current vol was aborted */
   int opstat;  /* Status flag for inbuf and outbuf */
   int rmsg_siz = 15000; /* radar coded msg buf size, bytes */
   int finished_flag = RCM_FALSE;
   int low_volnum = 1; /* Lowest volume number */
   int hi_volnum = 80; /* Highest volume number */
   int optr_sc_size = 21632; /* size of RCM scratch buffer, bytes */
   int status = 0; /* function return status */


   /* INITIALIZE INPUT BUFFER POINTERS */
   for (idx = 0; idx < RCM_NIBUFS; idx++)
   {
      Rcm_params.tibf[idx] = NULL;
   }

   /* SET FLAG TO 0 FOR VALID BUFFER RETRIEVAL */
   for (idx = 0; idx < RCM_NIOBUFS; idx++)
   {
      Rcm_params.vb[idx] = 0;
   }

   /* Initialize volume abort flag */
   abort = RCM_FALSE;

   /* GET OUTPUT BUFFER FOR RADAR CODED MESSAGE */
   optr = RPGC_get_outbuf_by_name("RADARMSG", rmsg_siz, &opstat);
   if (opstat == NORMAL)
   {
      Rcm_params.vb[RCM_RADBUF] = RCM_TRUE;

      /* GET OUTPUT SCRATCH BUFFER FOR RADAR CODED MESSAGE */
      optr_sc = RPGC_get_outbuf(SCRATCH, optr_sc_size, &opstat);
      if (opstat == RPGC_NORMAL)
      {
         Rcm_params.vb[RCM_SCRBUF] = RCM_TRUE;
      }
      else if (opstat == RPGC_NO_MEM)
      {
         Rcm_params.vb[RCM_SCRBUF] = NOMEM;
      }

      /* CALL A3082U TO GET INPUT BUFFERS NECESSARY TO
         PROCESS RADAR CODED MESSAGE */
      a3082u_get_inbufs();

      /* IF SCRATCH AND POLAR GRID NOT AVAILABLE DON'T PROCESS RCM */
      if ( (Rcm_params.vb[RCM_SCRBUF] == RCM_TRUE) &&
           (Rcm_params.vb[RCM_PGBUF] == RCM_TRUE)  &&
           (Rcm_params.vb[RCM_HYBUF] == RCM_TRUE))
      {
         /* Only process when vol num is in range */
         finished_flag = RCM_FALSE;
         idx = 0;
         while ((finished_flag == RCM_FALSE) &&
                (idx < RCM_NIBUFS))
         {
            voln = RPGC_get_buffer_vol_seq_num( Rcm_params.tibf[idx] );
            if ( voln >= low_volnum && voln <= hi_volnum)
            {
               abort = RCM_FALSE;

               /* set flag so we don't repeat the loop */
               finished_flag = RCM_TRUE;

               /* GET CURRENT DATE AND TIME, CALL MAIN RCM DRIVER */
               ret = RPGCS_get_date_time( &ctime, &cdate );
               if ( ret!= 0 )
               {
                  RPGC_log_msg( GL_INFO,
                     "Error returned from RPGCS_get_date_time()\n" );
               }

               a3082l_rcm_driver(optr, optr_sc, voln, cdate, ctime);

               /* RELEASE INPUT BUFFERS */
               for (idx = RCM_PGBUF; idx < RCM_NIBUFS; idx++)
               {
                  if (Rcm_params.vb[idx] == RCM_TRUE)
                  {
                     ret = RPGC_rel_inbuf( Rcm_params.tibf[idx] );
                  }
               }

               if (Rcm_params.vb[RCM_SCRBUF] == RCM_TRUE)
               {
                  ret = RPGC_rel_outbuf( optr_sc, DESTROY );
                  if ( ret != 0 )
                  {
                     RPGC_log_msg( GL_INFO,
                        "Error returned from RPGC_rel_outbuf()\n" );
                  }
               }

               /* RELEASE OUTPUT BUFFER: DESTROY IF ALL INPUT BUFFERS ARE NOT
                  AVAILABLE, OTHERWISE FORWARD */
               if ((Rcm_params.vb[RCM_PGBUF] == RCM_FALSE) &&
                   (Rcm_params.vb[RCM_CABUF] == RCM_FALSE) &&
                   (Rcm_params.vb[RCM_ETBUF] == RCM_FALSE) &&
                   (Rcm_params.vb[RCM_VWBUF] == RCM_FALSE) &&
                   (Rcm_params.vb[RCM_HYBUF] == RCM_FALSE))
               {
                  abort = RCM_TRUE;
               }
               else
               {
                  /* Strip off the graphical portion of the product (i.e.,
                     automatically turn it into the post-edit uneditted
                     version. */
                  RCM_strip_off_graphic( (char *) optr, &status);
                  if ( status != 0 )
                  {
                     LE_send_msg(GL_INFO,
                        "a30821: Error (%d) returned from RCM_strip_off_graphic\n",
                        status);
                     abort = RCM_TRUE;
                  }
                  else
                  {
                     RPGC_rel_outbuf(optr, FORWARD);
                  }
               }
            }
            else
            {
               abort = RCM_TRUE;
               idx = idx + 1;
            }
         } /* end while loop */
      }
      else
      {
         /* ADDED FOR REHOST.  RCM SHOULD RELEASE ANY INPUT BUFFERS
            ALREADY ACQUIRED!! */
         for (ibuf_idx = RCM_PGBUF; ibuf_idx < RCM_NIBUFS; ibuf_idx++)
         {
            if (Rcm_params.vb[ibuf_idx] == RCM_TRUE)
            {
               RPGC_log_msg( GL_INFO,
                  "a30821_buffer_control: error (ibuf_idx=%d)\n",ibuf_idx);
               RPGC_abort();
            }
            RPGC_rel_inbuf(&Rcm_params.tibf[ibuf_idx]);
            Rcm_params.vb[ibuf_idx] = RCM_FALSE;
         }
	 abort = RCM_TRUE;
      }
   }
   else  /* else for "if opstat == normal" line */
   {
      /* IF STATUS INDICATES NO MEMORY TAKE ABORT EXIT DUE TO LOAD
         SHEDDING OTHERWISE REGULAR ABORT */
      if (opstat == RPGC_NO_MEM)
      {
         RPGC_abort_because(PROD_MEM_SHED);
      }
      else
      {
         RPGC_abort();
      }
   }
      
   /* IF ABORT PROCESSING AND IF SCRATCH BUFFERS ACQUIRED RELEASE THEM FOR
      DESTRUCTION. IF SCRATCH BUFFER COULD NOT BE ACQUIRED BECAUSE OF NO MEMORY
      ABORT DUE TO LOAD SHEDDING; OTHERWISE TAKE NORMAL ABORT. */
   if (abort == RCM_TRUE )
   {
      if ((Rcm_params.vb[RCM_SCRBUF] == RCM_TRUE) && 
          (optr_sc != NULL))
      {
         RPGC_rel_outbuf(optr_sc, DESTROY);
      }

      RPGC_rel_outbuf(optr, DESTROY);

      if (Rcm_params.vb[RCM_SCRBUF] == RPGC_NO_MEM)
      {
         RPGC_abort_because(PROD_MEM_SHED);
      }
      else
      {
         RPGC_abort();
      }
   }

   return 0;

} /* end a30821_buffer_control */


/*******************************************************************************
* Description:
*    This subroutine gets the input buffers for composite reflectivity polar
*    grid, combined attributes, echo tops, and VAD winds.
*
* Inputs:
*
* Outputs:
*    Rcm_params.tibf   Array of input buffer pointers.
*    Rcm_params.vb     Buffer valid status (0=no, 1=yes).
*
* Returns:
*    int - RCM_SUCCESS or RCM_FAILURE
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082u_get_inbufs()
{
   int opstat;

   /* GET INPUT BUFFER FOR COMPOSITE REFLECTIVITY POLAR GRID */
   Cref_dat_ptr = RPGC_get_inbuf_by_name("CRPG", &opstat);
   if (opstat == RPGC_NORMAL)
   {
      Rcm_params.vb[RCM_PGBUF] = RCM_TRUE;
      Rcm_params.tibf[RCM_PGBUF] = Cref_dat_ptr;
   }

   /* GET INPUT BUFFER FOR COMBINED ATTRIBUTES */
   Combatt_dat_ptr = RPGC_get_inbuf_by_name("COMBATTR", &opstat);
   if (opstat == RPGC_NORMAL) 
   {
      Rcm_params.vb[RCM_CABUF] = RCM_TRUE;
      Rcm_params.tibf[RCM_CABUF] = Combatt_dat_ptr;
      Combattr = (combattr_t*) Combatt_dat_ptr;
   }

   /* GET INPUT BUFFER FOR ECHO TOPS */
   Et_dat_ptr = RPGC_get_inbuf_by_name("ETTAB", &opstat);
   if (opstat == RPGC_NORMAL) 
   {
      Rcm_params.vb[RCM_ETBUF] = RCM_TRUE;
      Rcm_params.tibf[RCM_ETBUF] = Et_dat_ptr;
      Etop_parms = (Echo_top_params_t *) Et_dat_ptr;
   }

   /* GET INPUT BUFFER FOR VAD WINDS */
   Vad_dat_ptr = RPGC_get_inbuf_by_name("VADTMHGT", &opstat);
   if (opstat == RPGC_NORMAL)
   {
      Rcm_params.vb[RCM_VWBUF] = RCM_TRUE;
      Rcm_params.tibf[RCM_VWBUF] = Vad_dat_ptr;
      Vad_parms = (Vad_params_t *) Vad_dat_ptr;
   }

   /* GET INPUT BUFFER FOR HYBRID SCAN BUFFER */
   Hybr_dat_ptr = RPGC_get_inbuf_by_name("HYBRSCAN", &opstat);
   if (opstat == RPGC_NORMAL)
   {
      Rcm_params.vb[RCM_HYBUF] = RCM_TRUE;
      Rcm_params.tibf[RCM_HYBUF] = Hybr_dat_ptr;
   }

   return RPGC_NORMAL;

} /* end a3082u_get_inbufs */


/*******************************************************************************
* Description:
*    Main processing routine of the radar coded message task.  It is called by
*    the buffer control routine and manages the remainder of the generation of
*    the output buffer.
*
* Inputs:
*    void*     optr        Output buffer pointer
*    void*     optr_sc     Output scratch buffer pointer (100x100 1/16 LFM grid)
*    int       voln        Volume number
*    int       cdate       Current date
*    int       ctime       Current time
*
* Outputs:
*    void*     optr        Output buffer pointer
*    void*     optr_sc     Output scratch buffer pointer (100x100 1/16 LFM grid)
*
* Returns:
*    Currently not used for information, always returns RCM_SUCCESS
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082l_rcm_driver(void* optr, void* optr_sc, int voln, int cdate, int ctime)
{
   short split_date[3]; 
   int yr, mo, day;
   int day_idx = 0, mo_idx = 1, yr_idx = 2;
   int prod_bytes = 0;
   int icol;
   int time;
   int irow;
   int voldate;    /* Volume start date */
   short i2date;   /* Short int for representing vol start date */
   int hmtime;     /* Time in HHMM past GMT */


   /* GET DATE AND TIME FROM VOLUME SCAN */
   Summary = RPGC_get_scan_summary( voln );
   voldate = Summary->volume_start_date;
   i2date = (short) voldate;
   time = Summary->volume_start_time;
   a30829_cnvtime(&time, &hmtime);
   RPGCS_julian_to_date(i2date, &yr, &mo, &day);
   split_date[day_idx] = day;
   split_date[mo_idx] = mo;
   split_date[yr_idx] = yr;

   /* BUILD RCM GRID */
   a30822_rcm_grid( (short*) Cref_dat_ptr, optr_sc, (short*) Hybr_dat_ptr );
   
   /* FILL INTERMEDIATE GRAPHIC PRODUCT HEADER */
   a3082n_header_inter( Combattr->cat_num_storms, &Combattr->cat_feat[0][0],
      &Combattr->comb_att[0][0], Combattr->num_fposits,
      &Combattr->forcst_posits[0][0][0], &Combattr->cat_tvst[0][0], optr, cdate,
      ctime, voln, optr_sc);

   /* SET UP COMMUNICATIONS HEADER LINE FOR RADAR CODED MESSAGE */
   a3082m_comm_line(&irow, &icol, &prod_bytes, optr);
   a30823_rcm_control(split_date, &hmtime, &irow, &icol, voln, &prod_bytes,
      optr_sc, optr);
   /* IF ECHO TOPS BUFFER IS AVAILABLE, BUILD ECHO TOPS */
   if (Rcm_params.vb[RCM_ETBUF] == RCM_TRUE)
   {
      a3082c_max_echotop(Etop_parms, &irow, &icol, &prod_bytes, optr);
   }
   else
   {
      a3082v_buffer_notavail(ETFLAG, prod_bytes, irow, icol, optr);
   }

   /* IF COMBINED ATTRIBUTES BUFFER IS AVAILABLE, BUILD CENTROIDS */
   if (Rcm_params.vb[RCM_CABUF] == RCM_TRUE)
   {
      a3082d_centroids_parta(Combattr->cat_num_storms, &Combattr->cat_feat[0][0],
         &Combattr->comb_att[0][0], Combattr->num_fposits,
         &Combattr->forcst_posits[0][0][0], &Combattr->cat_tvst[0][0], &irow,
         &icol, &prod_bytes, optr);
   }
   else
   {
      a3082v_buffer_notavail(CAFLAGA, prod_bytes, irow, icol, optr);
   }

   /* CALL A3082E TO ENCODE PART B INDICATOR LINE */
   a3082e_partbc_header(ETFLAG, split_date, hmtime, irow, icol, prod_bytes,
      optr);

   /* IF VAD WINDS BUFFER IS AVAILABLE, BUILD VAD WINDS */
   if (Rcm_params.vb[RCM_VWBUF] == RCM_TRUE)
   {
      a3082f_vad_winds(Vad_parms->vad_data_hts, irow, icol, prod_bytes, optr);
   }

   /* CALL A3082E TO ENCODE PART C INDICATOR LINE */
   a3082e_partbc_header(CAFLAGA, split_date, hmtime, irow, icol, prod_bytes,
      optr);

   /* IF COMBINED ATTRIBUTES BUFFER IS AVAILABLE */
   if (Rcm_params.vb[RCM_CABUF] == RCM_TRUE)
   {
      /* BUILD TVS */
      a3082h_tvs(Combattr->cat_num_storms, &Combattr->cat_feat[0][0],
         &Combattr->comb_att[0][0], Combattr->num_fposits,
         &Combattr->forcst_posits[0][0][0], Combattr->cat_num_rcm,
         &Combattr->cat_tvst[0][0], irow, icol, prod_bytes, optr);

      /* BUILD MESOCYCLONES */
      a3082i_mesocyclones(Combattr->cat_num_storms, &Combattr->cat_feat[0][0],
         &Combattr->comb_att[0][0], Combattr->num_fposits,
         &Combattr->forcst_posits[0][0][0], Combattr->cat_num_rcm,
         Combattr->cat_mdat, irow, icol, prod_bytes, optr);

      /* BUILD CENTROIDS FOR PART C */
      a3082j_centroids_partc(Combattr->cat_num_storms, &Combattr->cat_feat[0][0],
         &Combattr->comb_att[0][0], Combattr->num_fposits,
         &Combattr->forcst_posits[0][0][0], &Combattr->cat_tvst[0][0],
         irow, icol, prod_bytes, optr);
   }
   else
   {
      a3082v_buffer_notavail(CAFLAGC, prod_bytes, irow, icol, optr);
   }

   /* FILL PRODUCT HEADER */
   a30824_header(optr, cdate, ctime, voln, prod_bytes);

   return RCM_SUCCESS;

} /* end a3082l_rcm_driver */


/*******************************************************************************
* Description:
*    This module converts time expressed in milliseconds past 0000 GMT to time
*    in hours and minutes past 00 GMT.
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a30829_cnvtime(int* seconds, int* hmtime)
{
   int hr, sec, min;
   int hrthrt100 = 100;  /* Indicates 'hours-to-hours' times 100. */

   /* compute the number of hours */
   hr = *seconds / SEC_P_HR;

   /* compute the remaining number of minutes */
   min = (*seconds - hr * SEC_P_HR) / MIN_P_HR;

   /* compute the remaining number of seconds */
   sec = *seconds - hr * SEC_P_HR - min * MIN_P_HR;

   /* round off to the nearest minute */
   if (sec >= (MIN_P_HR / 2))
   {
      ++min;
   }

   /* if the rounding caused minutes to exceed 60, subtract 60 from the minutes
      and add 1 to the hours */
   if (min >= MIN_P_HR)
   {
      min += -MIN_P_HR;
      ++hr;
   }

   /* if adding 1 to the hours caused it to become 24, set it back to 00 */
   if (hr == HR_P_DAY)
   {
      hr = 0;
   }

   /* compute the time in hours and minutes */
   *hmtime = hr * hrthrt100 + min;

    return 0;

} /* end a30829_cnvtime */


/*******************************************************************************
* Description:
*    Set up radar-coded-message grid:
*       1) Initialize 1/16 LFM grid to 0.
*       2) For each azimuth
*       2a) For bins 0 to 230
*       2a.1) If Color Table value corresponding to Hybrid Scan Reflectivity
*             data value is greater than current LFM grid value, set grid value
*             equal to the color table value.
*       2b) For bins 231 to 460.
*       2b.1) If Color Table value corresponding to Comp Refl Polar Grid data 
*             value is greater than current LFM grid value, set grid value equal
*             to the color table value.
*       
* Inputs:
*    short*   polgrid   Polar grid portion of polar grid buffer.
*    short*   rcmgrid   Radar-coded-message grid (100x100 1/16 LFM grid).
*    short*   hybrscan  Hybrid Scan array of digital reflectivity values on
*                       polar grid.
*
* Outputs:
*    short*   rcmgrid   Radar-coded-message grid (100x100 1/16 LFM grid).
*
* Returns:
*    RCM_SUCCESS or RCM_FAILURE
*
* Globals:
*
* Notes:
*******************************************************************************/
int a30822_rcm_grid(short* polgrid, short* rcmgrid, short* hybrscan)
{
   static int bias_gt230; /* THRESHOLD VALUE BIASED FOR GT 230 KM */
   static int i; /* Grid I-coordinate point. */
   static int az; /* AZIMUTH ANGLE */
   static int bin; /* BIN NUMBER */
   static int bin_cross = 230; /* BIN CROSS POINT 230 KM */
   static int bias = 32.0; /* Bias for threshold conversion at GT230 km. */

   /* Initialize 1/16 LFM grid to zeros */
   for (i = 0; i < (RCM_NROWS * RCM_NCOLS); i++) 
   {
      *(rcmgrid + i) = 0;
   }

   /* DETERMINE THRESHOLD VALUE BIASED FOR BEYOND 230 KM */
   bias_gt230 = RPGC_NINT(2.0 * (Rcm_adapt.range_thresh + bias)) + 2;

   /* DETERMINE IF THE GRID TABLE LATITUDE/LONGITUDE EQUAL THE SITE LAT/LON */
   if ( (Lfm_parms.grid_lat == Siteadp.rda_lat) &&
        (Lfm_parms.grid_lon == Siteadp.rda_lon))
   {
      /* DO FOR ALL THE AZIMUTH VALUES */
      for (az = 0; az < RCM_NRADS; az++) 
      {
         /* DO FOR ALL THE BIN NUMBERS UP TO THE BIN CROSS VALUE OF 230 */
         for (bin = 0; bin < bin_cross; bin++) 
         {
            /* DETERMINE IF THE COLOR THRESHOLD VALUE EXCEEDS ZERO */
            if (Colrtbl.coldat[RCM_NC][*(hybrscan+(az*MAX_HYBINS)+bin)] > 0) 
            {
               /* IF THE COLOR THRESHOLD VALUE EXCEEDS THE RCMGRID VALUE, THEN
                  THE RCMGRID VALUE IS SET TO THE VALUE FROM THE COLOR TABLE */

               if (Colrtbl.coldat[RCM_NC][*(hybrscan+(az*MAX_HYBINS)+bin)] >
                  *(rcmgrid + Lfm_parms.lfm16grid[az][bin]))
               {
                  *(rcmgrid + Lfm_parms.lfm16grid[az][bin]) = 
                     Colrtbl.coldat[RCM_NC][*(hybrscan+(az*MAX_HYBINS)+bin)];
               }
            }
         }

         /* DO FOR ALL BINS FROM THE BIN CROSS TO THE MAXIMUM NUMBER OF BINS */
         /* PROCESS IF THE LFM16GRID VALUE IS NOT EQUAL TO THE BEYOND GRID FLAG
            VALUE */
         bin = bin_cross;
         while ( ( bin < RCM_NBINS ) && (Lfm_parms.lfm16grid[az][bin] != BEYOND_GRID) )
         {
            /* DETERMINE IF THE COLOR THRESHOLD VALUE EXCEEDS ZERO */
            if (Colrtbl.coldat[RCM_NC][*(polgrid+(az*RCM_NBINS)+bin)] > 0) 
            {
               /* WHEN THE POLGRID VALUE IS GREATER THAN THE BIAS VALUE, SET
                  THE RCMGRID VALUE TO LEVEL 7 COLOR */
               if (*(polgrid+(az*RCM_NBINS)+bin) > bias_gt230) 
               {
                  *(rcmgrid + Lfm_parms.lfm16grid[az][bin]) = 7;

                  /* WHEN THE POLGRID VALUE IS WITHIN THE BIAS VALUE, SET THE
                     RCMGRID VALUE TO LEVEL 8 COLOR */

               } 
               else if (*(polgrid+(az*RCM_NBINS)+bin) <= bias_gt230) 
               {
                  *(rcmgrid + Lfm_parms.lfm16grid[az][bin]) = 8;
               }
            }
            bin++;
         }
         /* 1/16 LFM GRID BOX VALUE IS BEYOND GRID FLAG VALUE, GET NEXT
            AZIMUTH TO PROCESS */
      }

      /* DO FOR THE 1/16 LFM GRID */
      for (i = 0; i < (RCM_NROWS * RCM_NCOLS); i++) 
      {
         /* CHECK IF THE 1/16 LFM GRID BOX FLAG RANGE IS A HOLE RANGE SET THE
            BIN AND AZIMUTH VALUE FROM THE LFM 16 FLAG TABLE */

         if (Lfm_parms.lfm16flag[FLAG_RNG][i] > 0) 
         {
            bin = Lfm_parms.lfm16flag[FLAG_RNG][i];
            az = Lfm_parms.lfm16flag[FLAG_AZ][i];

            /* CHECK IF THE 1/16 LFM GRID BOX FLAG IS WITHIN 230 KM */

            if (Lfm_parms.lfm16flag[FLAG_RNG][i] <= bin_cross) 
            {
               /* CHECK IF THE COLOR THRESHOLD VALUE IS POSITIVE */

               if (Colrtbl.coldat[RCM_NC][*(hybrscan+(az*MAX_HYBINS)+bin)] > 0)
               {
                  *(rcmgrid + Lfm_parms.lfm16grid[az][bin]) = 
                     Colrtbl.coldat[RCM_NC][*(hybrscan+(az*MAX_HYBINS)+bin)];
               }
            }
            else
            {
               /* CHECK IF THE COLOR THRESHOLD VALUE IS POSITIVE */
               if (Colrtbl.coldat[RCM_NC][*(polgrid+(az*RCM_NBINS)+bin)] > 0) 
               {
                  /* WHEN THE POLGRID VALUE IS GREATER THAN THE BIAS VALUE, SET 
                     THE RCMGRID VALUE TO LEVEL 7 COLOR */
                  if (*(polgrid+(az*RCM_NBINS)+bin) > bias_gt230) 
                  {
                     *(rcmgrid + Lfm_parms.lfm16grid[az][bin]) = 7;

                     /* WHEN THE POLGRID VALUE IS WITHIN THE BIAS VALUE, SET THE
                        RCMGRID VALUE TO LEVEL 8 COLOR */
                  } 
                  else if (*(polgrid+(az*RCM_NBINS)+bin) <= bias_gt230) 
                  {
                     *(rcmgrid + Lfm_parms.lfm16grid[az][bin]) = 8;
                  }
               }
            }
         }
      }
   }

/* TBD - temp print */
for (i = 0; i < (RCM_NROWS * RCM_NCOLS); i++) 
{
   fprintf(stderr, "check10: rcmgrid[%d]=%d\n", i, rcmgrid[i]);
//   LE_send_msg(GL_INFO, "check10: rcmgrid[%d]=%d\n", i, rcmgrid[i]);
}

   return RCM_SUCCESS;

} /* end a30822_rcm_grid */


/*******************************************************************************
* Description:
*    Stores the header data into the output buffer for the intermediate graphic
*    product for radar coded message.
*
* Inputs:
*    int     cat_num_storms       Number of storms processed for the combined
*                                 attributes table.
*    int     *cat_feat            Table of associated severe features.  2-D 
*                                 array of storm ids and severe weather category
*                                 absent/present flags. [CAT_MXSTMS][CAT_NF]
*    float   *comb_att            Table of combined attributes. 2D array of 
*                                 storm attributes combined from the Storm
*                                 Series, TVS, and MDA algorithms.
*                                 [CAT_MXSTMS][CAT_DAT]
*    int     num_fposits          Number of positions for which forecasts were
*                                 made for the storms.
*    float   *forcst_posits       Forecast position data for each storm cell.
*                                 [CAT_MXSTMS][MAX_FPOSITS][FOR_DAT]
*    float   *cat_tvst            2D array of TVS attributes.
*                                 [CAT_MXSTMS][CAT_NTVS]
*    short   *bufout              Output buffer pointer.
*    int     gendate              Generation date.
*    int     gentime              Generation time.
*    int     volnum               Volume scan number.
*    short   *rcmgrid             Scratch buf ptr (100x100 1/16 LFM grid).
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082n_header_inter(int cat_num_storms, int* cat_feat, float* comb_att,
   int num_fposits, float* forcst_posits, float* cat_tvst, short* bufout,
   int gendate, int gentime, int volnum, short* rcmgrid)
{
   int idx;  /* generic loop index */
   int len_lay1;
   int tempi4, clrval;
   int msgid = RPGC_get_code_from_name("RADARMSG");
   int nblks = 3;
   int rcmcl = RCM_NC; /* Index into the color level table COLDAT for all the color
                       tables in the RPG: Radar Coded msg 7 level clear air.*/
   int vol_spot_blank = 0; /* Spot Blank Field in Scan Summary Table def */
   int maxlev = PCTTHSIZ;  /* Maximum number of thresh levels */
   int level7 = 7; /* LEVEL 7 OFFSET */
   int level8 = 8; /* LEVEL 8 OFFSET */
   int blockid = 1; /* BLOCK ID NUMBER */
   int nlyr = 3; /* Product block offsets: Number of layers */
   int lfm_loc = 10; /* NUMBER OF DATA LOCATIONS FOR LFM */
   int lay1_over = 2; /* LAYER ONE OVERHEAD */
   int init_idx = 68; /* INITIAL RCM INDEX VALUE */


   /* INITIALIZE HEADER TO ZERO */
   for (idx = 0; idx < PHEADLNG; idx++) 
   {
      bufout[idx] = 0;
   }

   /* STORE MESSAGE CODE, SOURCE ID #, # BLOCKS, AND DIVIDER */
   bufout[MESCDOFF] = msgid;
   bufout[SRCIDOFF] = Siteadp.rpg_id;
   bufout[NBLKSOFF] = nblks;
   bufout[DIV1OFF] = RCM_DIVIDER;

   /* STORE RADAR LATITUDE/LONGITUDE */
   RPGC_set_product_int((void*)&bufout[LTMSWOFF], Siteadp.rda_lat);
   RPGC_set_product_int((void*)&bufout[LNMSWOFF], Siteadp.rda_lon);

   /* STORE RADAR HEIGHT, PRODUCT CODE, OPERATIONAL MODE */
   bufout[RADHGTOFF] = Siteadp.rda_elev;
   bufout[PRDCODOFF] = msgid;
   bufout[WTMODOFF] = Summary->weather_mode;

   /* SET COLOR THRESHOLD VALUES DEPENDING ON OPERATIONAL MODE */
   if (bufout[WTMODOFF] == CLEAR_AIR_MODE) 
   {
      clrval = rcmcl;
   }
   else
   {
      clrval = RCM_NC;
   }

   /* STORE VOL COVERAGE PATTERN, VOL SCAN #, VOL SCAN DATE & TIME */
   bufout[VCPOFF] = Summary->vcp_number;
   bufout[VSNUMOFF] = volnum;
   bufout[VSDATOFF] = Summary->volume_start_date;
   RPGC_set_product_int((void*)&bufout[VSTMSWOFF], Summary->volume_start_time);

   /* STORE GENERATION DATE/TIME */
   bufout[GDPRDOFF] = gendate;
   RPGC_set_product_int((void*)&bufout[GTMSWOFF], gentime);

   /* SET SPOT BLANK STATUS */
   if (RPGCS_bit_test((unsigned char*)&Summary->spot_blank_status,
      vol_spot_blank)) 
   {
      bufout[NMAPSOFF] = RCM_SBON;
   }

   /* STORE DATA LEVELS FOR COLOR THRESHOLD */
   for (idx = 0; idx < maxlev; idx++) 
   {
      bufout[DL1OFF + idx] = Colrtbl.thresh[clrval][idx];
   }

   /* SET > THRESHOLD AND < THRESHOLD VALUES FOR DATA LEVELS */
   tempi4 = (int) Rcm_adapt.range_thresh;
   RPGCS_bit_set((unsigned char*)&tempi4, 20);
   bufout[DL1OFF + level7] = (short) tempi4;
   tempi4 = (int) Rcm_adapt.range_thresh;
   RPGCS_bit_set((unsigned char*)&tempi4, 21);
   bufout[DL1OFF + level8] = (short) tempi4;

   /* STORE RADAR CODED MESSAGE TIMERS FOR PUP */
   bufout[MDL3OFF] = 0;
   bufout[MDL4OFF] = 0;

   /* STORE BLOCK DIVIDER, BLOCK ID # */
   bufout[DIV2OFF] = RCM_DIVIDER;
   bufout[BLOCKIDOFF] = blockid;

   /* STORE # LAYERS */
   bufout[NLYROFF] = nlyr;

   /* STORE LAYER DIVIDER AND LENGTH OF LAYER ONE */
   bufout[LYRDIVOFF] = RCM_DIVIDER;

   /* Needed for byte swapping on LittLe Endian Machine */
   len_lay1 = (lfm_loc * RCM_BYTES_PER_HW) + lay1_over;
   RPGC_set_product_int((void*)&bufout[LYRLMSWOFF], len_lay1);

   /* SET RADAR CODED MESSAGE INDEX FOR BUFOUT INDEXING */
   Rcm_params.rcmidx = init_idx;

   /* CALL A3082O TO BUILD LAYER SECTIONS FOR PRODUCT HEADER */
   a3082o_header_layers(cat_num_storms, cat_feat, comb_att, num_fposits,
      forcst_posits, cat_tvst, len_lay1, bufout, rcmgrid);

   /* STORE OFFSET TO PRODUCT INFO, OFFSETS TO TAB/ADAPTATION */
   /* RCMIDX POINTS TO THE DIVIDER BEFORE MSG #74 FOR RCM */
   RPGC_set_product_int((void*)&bufout[OPRMSWOFF],PHEADLNG);
   RPGC_set_product_int((void*)&bufout[OTADMSWOFF], Rcm_params.rcmidx);

   return 0;

} /* a3082n_header_inter */

/*******************************************************************************
* Description:
*    Store the encoded communication header line, beginning with "1234" and
*    ending with the RPG Site ID, into the Radar Coded Message output buffer.
*
* Inputs:
*    int*       irow           Used to hold row number
*    int*       icol           Input column number
*    int*       nbytes         Number of bytes
*    short*     rcmbuf         Radar coded message buffer space
*
* Outputs:
*    int*       irow           Used to hold row number
*    int*       icol           Input column number
*    int*       nbytes         Number of bytes
*    short*     rcmbuf         Radar coded message buffer space
*
* Returns:
*    RCM_SUCCESS or RCM_FAILURE
*
* Globals:
*    Rcm_params
*    Siteadp
*
* Notes:
*******************************************************************************/
int a3082m_comm_line(int* irow, int* icol, int* nbytes, short* rcmbuf)
{
   static char *robuu = " ROBUU ";
   static char blank = ' ';
   static int idx;
   static char *pup_node = "1234";
   static char rpg_id_str[5];
   char* rcm_line; 
   char* str_p;        /* generic string pointer */
   int bytes_over = 6; /* BYTES OF OVERHEAD */

   /* SET NBYTES TO NUMBER OF BYTES IN PRODUCT HEADER */
   *nbytes = RCM_PHBYTES + (Rcm_params.rcmidx * RCM_BYTES_PER_HW) + bytes_over;
   Rcm_params.save_byte = *nbytes;

   /* Allocate space for rcm_line */
   rcm_line = (char *)malloc(RCM_NACOLS + 1);

   /* CLEAR RCM_LINE TO BLANKS */
   for (idx = 0; idx < RCM_NACOLS; idx++) 
   {
      rcm_line[idx] = blank;
   }

   /* WRITE PUP COMMUNICATION NODE, ROBUU, AND SIDD */
   str_p = rcm_line;
   strncpy(str_p, pup_node, strlen(pup_node));
   str_p += strlen(pup_node);
   strncpy(str_p, robuu, strlen(robuu));
   str_p += strlen(robuu);
   sprintf(rpg_id_str, "%04i", Siteadp.rpg_id);
   strncpy(str_p, rpg_id_str, strlen(rpg_id_str));
   rcm_line[RCM_NACOLS] = '\0';

   /* STORE LINE INTO RADAR CODED MESSAGE PRODUCT BUFFER */
   *irow = 0;
   *icol = 0;
   a3082k_store_line(irow, icol, nbytes, rcm_line, rcmbuf);

   //free(rcm_line);

   return RCM_SUCCESS;

} /* end a3082m_comm_line */



/*******************************************************************************
* Description:
*    General control routine for radar coded message product generation.
*
* Inputs:
*    short*     split_date     Date in three I*2 words (day, month, year)
*    int*       hmtime         Hours and minutes of time data
*    int*       irow           Used to hold row number
*    int*       icol           Input column number
*    int        voln           Volume scan number
*    int*       nbytes         Number of bytes
*    short      rcmgrid[][]    Radar coded message grid space
*    short*     rcmbuf         Radar coded message buffer space
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a30823_rcm_control(short* split_date, int* hmtime, int* irow, int* icol,
   int voln, int* nbytes, short rcmgrid[][RCM_NCOLS], short* rcmbuf)
{
   char *num_ints = "/NI"; /* Header for Number of Intensities */
   char colon = ':'; 
   int i; /* generic loop index */
   int row_idx, col_idx; /* Row/Column loop indices */
   int lbl; /* LABEL LFM BOX FLAG */
   int ist; /* Internal variable - used to hold I index value. */
   int jst; /* J index variable saved. */
   int reps; /* NUMBER OF REPETITIONS */
   int pos; /* position marker in buffer */
   int run_vip; /* RUN VALUE INDEX PARAMETERS */
   union radne_union_t
   {
      char ppine[6];   /* CHARACTERS RADNE */
      short ippine[3]; /* INDEX PARAMETER */
   } radne_union;

   union intensities_union_t
   {
      char cnint[8];   /* ENCODE COUNTER FOR TOTAL INTENSITIES */
      short icnint[4]; /* INDEX FOR TOTAL NUMBER OF INTENSITIES */
   } intensities_union;

   radne_union.ppine[0] = ' ';
   radne_union.ppine[1] = 'R';
   radne_union.ppine[2] = 'A';
   radne_union.ppine[3] = 'D';
   radne_union.ppine[4] = 'N';
   radne_union.ppine[5] = 'E';

   /* INITIALIZE COUNTER FOR TOTAL NUMBER OF INTENSITIES TO BE CALCULATED IN
      A30825 ROUTINE */
   Rcm_params.count_int = 0;

   /* ENCODE PART A INDICATOR LINES */
   a3082b_parta_header(split_date, hmtime, irow, icol, voln, nbytes, rcmbuf);

   /* DO FOR ALL ROWS IN THE RCMGRID */
   for (row_idx = 0; row_idx < RCM_NROWS; row_idx++) 
   {
      pos = RCM_END;
      run_vip = rcmgrid[row_idx][0];
      reps = 1; /* init reps to 1 (we know there's at least 1 of these vip's) */
      ist = row_idx;
      jst = 1;
      lbl = RCM_TRUE;

      /* CHECK NEW RCMGRID VALUE AGAINST RUN_VIP FOR EACH COLUMN. Start in col
         1 (0 based) because we've already set run_vip to col 0 value. */
      for (col_idx = 1; col_idx < RCM_NCOLS; col_idx++) 
      {
         /* IF VALUES ARE EQUAL, INCREMENT NUMBER OF REPETITIONS */
         if (rcmgrid[row_idx][col_idx] == run_vip) 
         {
            reps++;
         }
         else
         {
            /* IF VALUES ARE DIFFERENT, CALL A30825 TO ENCODE RUN_VIP VALUE */
            a30825_rcm_encode(&ist, &jst, &lbl, &pos, &run_vip, &reps, irow,
               icol, nbytes, (char*)rcmbuf);
            run_vip = rcmgrid[row_idx][col_idx];
            reps = 1;
            ist = row_idx;
            jst = col_idx;
         }
      }
      pos = RCM_END;
      a30825_rcm_encode(&ist, &jst, &lbl, &pos, &run_vip, &reps, irow, icol,
         nbytes, (char*)rcmbuf);
   }

   /* IF NO DATA WAS FOUND ON THE CRPG, SET RADNE FLAG */
   if (Rcm_params.count_int == 0) 
   {
      for (i = 0; i < 3; i++) 
      {
         rcmbuf[(Rcm_params.rcmoff/RCM_BYTES_PER_HW) + i] = radne_union.ippine[i];
      }
   }

   /* WRITE TOTAL NUMBER OF INTENSITIES FOUND INTO /NI FIELD */
   sprintf(intensities_union.cnint, "%3s%04d%c", num_ints, Rcm_params.count_int,
      colon);

   for (i = 0; i < 4; i++) 
   {
      rcmbuf[(Rcm_params.opmodeoff/RCM_BYTES_PER_HW) + i] =
         intensities_union.icnint[i];
   }

   /* BLANK PAD REMAINDER OF LAST INTENSITY ROW */
   a30826_blank_pad(irow, icol, nbytes, (char*)rcmbuf);
   ++(*irow);
   *icol = 1;

/* TBD - debug print */
for (i=0;i<15000;i++)
{
   fprintf(stderr, "check20: rcmbuf[%d]=%d\n", i, rcmbuf[i]);
//   LE_send_msg(GL_INFO, "check20: rcmbuf[%d]=%d\n", i, rcmbuf[i]);
}


   return 0;

} /* end a30823_rcm_control */

/*******************************************************************************
* Description:
*    Encodes maximum echo top for Part A
*
* Inputs:
*    Echo_top_params_t  *etpar  Array of echo tops auxiliary parameters
*    int                *irow   Pointer to row number
*    int                *icol   Pointer to column number 
*    int                *nbytes Pointer to number of bytes
*    short              *rcmbuf Pointer to radar coded message buffer
*
* Outputs:
*    irow 
*    icol
*    nbytes 
*    rcmbuf
*
* Returns:
*    RCM_SUCCESS or RCM_FAILURE
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082c_max_echotop(Echo_top_params_t* etpar, int *irow, int *icol,
   int *nbytes, short *rcmbuf)
{
   static char *maxtop = "/MT";
   static char *colon = ":";
   static char blank = ' ';

   /* Local variables */
   char* rcm_line; 
   char* str_p;               /* generic string pointer */
   static int idx;            /* generic loop index */
   int ival, jval;
   static float azim, elev;
   static int ipos, jpos;
   static float xval, yval, range;
   static int height;
   static char height_str[4];
   int ht_scale_factor = 10;  /* height scale factor */
   char grid[RCM_LEN_LETTERS];

   /* Allocate space for rcm_line */
   rcm_line = (char *)malloc(RCM_NACOLS + 1);

   /* CLEAR RCM_LINE TO BLANKS */
   for (idx = 0; idx < RCM_NACOLS; idx++) 
   {
      rcm_line[idx] = blank;
   }

   /* STORE HEIGHT, I+J CARTESIAN COORDINATES */
   height = etpar->max_ht * ht_scale_factor;
   ipos = etpar->max_col_posn;
   jpos = etpar->max_row_posn;
   yval = BOXHGHT * (((NETROW * HALF) + HALF) - jpos);
   xval = BOXWDTH * (ipos - ((NETCOL * HALF) - HALF));

   /* CALCULATE AZIMUTH ANGLE AND RANGE */
   azim = atan2(xval, yval) * RCM_RADTODEG;
   if (azim < 0 ) 
   {
      azim += RCM_ADDAZ;
   }
   range = sqrt((xval * xval) + (yval * yval));
   elev = 0;

   /* CALL A3082G TO GET THE LFM GRID BOX COORDINATES */
   a3082g_get_ij(&azim, &range, &elev, &ival, &jval);

   /* GET LFM GRID LOCATION */
   a30828_grid_letters(ival, jval, grid);

   /* WRITE MAXIMUM ECHO TOP HEIGHT AND LFM LOCATION */
   str_p = rcm_line;
   strncpy(str_p, maxtop, strlen(maxtop));
   str_p += strlen(maxtop);
   sprintf(height_str, "%03i", height);
   strncpy(str_p, height_str, strlen(height_str));
   str_p += strlen(height_str);
   strncpy(str_p, colon, strlen(colon));
   str_p += strlen(colon);
   strncpy(str_p, &grid[0], strlen(&grid[0]));
   str_p += strlen(&grid[0]);
   strncpy(str_p, &grid[1], strlen(&grid[1]));
   str_p += strlen(&grid[1]);
   strncpy(str_p, &grid[2], strlen(&grid[2]));
   str_p += strlen(&grid[2]);
   rcm_line[RCM_NACOLS] = '\0';

   /* STORE LINE INTO RADAR CODED MESSAGE PRODUCT BUFFER */
   a3082k_store_line(irow, icol, nbytes, rcm_line, rcmbuf);

   //free(rcm_line);

   return (RCM_SUCCESS);

} /* end a3082c_max_echotop */


/*******************************************************************************
* Description:
*    This module will enter into the alpha-numeric radar coded message line for
*    /MT, /NCEN, /NTVS, and /NMES a field of NA (not available) if the input
*    buffer data is not available 
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082v_buffer_notavail(int func, int nbytes, int irow, int icol,
   short* rcmbuf)
{
   char blank = ' ';
   char *maxtop_na = "/MTNA:";
   char *centroid_na = "/NCENNA:";
   char *tvs_na = "/NTVSNA:";
   char *meso_na = "/NMESNA:";
   char* rcm_line = NULL; 
   int idx;

   /* CLEAR RCM_LINE TO BLANKS */
   for (idx = 0; idx < RCM_NACOLS; idx++)
   {
      rcm_line[idx] = blank;
   }

   /* IF FUNCTION FLAG = 1, ECHO TOPS BUFFER NOT AVAILABLE, WRITE /MTNA: INTO
      PRODUCT BUFFER */
   if (func == 1)
   {
      strncpy(rcm_line, maxtop_na, strlen(maxtop_na));
      rcm_line[RCM_NACOLS] = '\0';
      a3082k_store_line(&irow, &icol, &nbytes, rcm_line, rcmbuf);

      /* IF FUNCTION FLAG = 2, COMBINED ATTRIBUTES BUFFER NOT AVAILABLE FOR
         PART A, WRITE /NCENNA: INTO PRODUCT BUFFER */
   }
   else if (func == 2) 
   {
      strncpy(rcm_line, centroid_na, strlen(centroid_na));
      rcm_line[RCM_NACOLS] = '\0';
      a3082k_store_line(&irow, &icol, &nbytes, rcm_line, rcmbuf);

      /* IF FUNCTION FLAG = 3, COMBINED ATTRIBUTES BUFFER NOT AVAILABLE FOR
         PART C, WRITE /NTVSNA:, /NMESNA: AND /NCENNA: INTO PRODUCT BUFFER */
   }
   else if (func == 3)
   {
      strncpy(rcm_line, tvs_na, strlen(tvs_na));
      rcm_line[RCM_NACOLS] = '\0';
      a3082k_store_line(&irow, &icol, &nbytes, rcm_line, rcmbuf);

      /* Reset rcm_line string */
      for (idx = 0; idx < RCM_NACOLS; idx++)
      {
         rcm_line[idx] = blank;
      }

      strncpy(rcm_line, meso_na, strlen(meso_na));
      rcm_line[RCM_NACOLS] = '\0';
      a3082k_store_line(&irow, &icol, &nbytes, rcm_line, rcmbuf);

      /* Reset rcm_line string */
      for (idx = 0; idx < RCM_NACOLS; idx++)
      {
         rcm_line[idx] = blank;
      }

      strncpy(rcm_line, centroid_na, strlen(centroid_na));
      rcm_line[RCM_NACOLS] = '\0';
      a3082k_store_line(&irow, &icol, &nbytes, rcm_line, rcmbuf);
   }

   return 0;

} /* end a3082v_buffer_notavail */


/*******************************************************************************
* Description:
*    Encodes centroid data in Part A of the RCM.
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*    int   max_storm      Maximum number of storms.
*    int   cat_elcn       Index for elevation of storm centroid.
*    int   cat_az         Combined attributes index for Azimuth of storm
*                         centroid.
*    int   cat_fdir       Combined attributes index for storm fcst direction.
*    int   cat_fspd       Combined attributes index for storm forecast speed. 
*    int   cat_rng        Combined attributes index for projected range of storm
*                         centroid.
*    int   cat_sid        Combined attributes features index for storm ID.
*    float mps_to_kts     Scaling factor for meters per second to knots.
*
* Notes:
*******************************************************************************/
int a3082d_centroids_parta(int cat_num_storms, int *cat_feat, float *combatt,
   int num_fposits, float *forcst_posits, float *cat_tvst, int *irow, int *icol,
   int *nbytes, short *rcmbuf)
{
   static char *colon = ":";
   static char *comma = ",";
   static char *cchar = "C";
   static char blank = ' ';
   static char *enda = "/ENDAA";
   static char *num_cens = "/NCEN";

   /* Local variables */
   int idx = 0; /* generic loop index */
   static char *rcm_line;   /* Radar coded message line */
   static char *str_p;      /* Generic string pointer */
   static int storm_idx;
   static int ic;
   static int is;
   static int storm_count;
   static int cent_total;
   static int ism;
   static float cdir[RCM_MAX_STORM];
   static int lend;
   char grid[RCM_LEN_LETTERS];
   static int idir;
   static float cspd[RCM_MAX_STORM];
   static int ival, jval, ispd;
   static float azim[RCM_MAX_STORM], elev[RCM_MAX_STORM], range[RCM_MAX_STORM];
   static int total, cennum[RCM_MAX_STORM];
   static int eol_flag = 100;  /* flag for end of line */
   static int increm = 14;     /* incremental value for comma position */
   static char cenchar[2];
   char total_str[4];
   char idir_str[4];
   char ispd_str[4];

   /* Allocate and initialize rcm_line */
   rcm_line = (char *) malloc(RCM_NACOLS + 1);
   for (idx = 0; idx < RCM_NACOLS; idx++)
   {
	*(rcm_line + idx) = blank;
   }

   /* COLLECT DATA FOR CENTROIDS WITHIN 230 KM RANGE */
   storm_count = 0;
   for (storm_idx = 0; storm_idx < cat_num_storms; storm_idx++)
   {
      while (( *(combatt + (storm_idx * CAT_DAT) + CAT_RNG) <= RCM_RNG_MAX) &&
             ( storm_count <= RCM_MAX_STORM ))
      {
	 storm_count++;

         /* SAVE CENTROID NUMBER, AZIMUTH, RANGE, ELEVATION ANGLE, DIRECTION
            AND SPEED */
         cennum[storm_count] = *(cat_feat + (storm_idx * CAT_NF) + CAT_SID);
         azim[storm_count] = *(combatt + (storm_idx * CAT_DAT) + CAT_AZ);
         range[storm_count] = *(combatt + (storm_idx * CAT_DAT) + CAT_RNG);
         elev[storm_count] = *(combatt + (storm_idx * CAT_DAT) + CAT_ELCN);
         cdir[storm_count] = *(combatt + (storm_idx * CAT_DAT) + CAT_FDIR);
         cspd[storm_count] = *(combatt + (storm_idx * CAT_DAT) + CAT_FSPD) *
            MPS_TO_KTS;
      }
   }

   cent_total= storm_count;
   total = MIN(Rcm_adapt.num_storms, cent_total);

   /* WRITE TOTAL NUMBER OF CENTROIDS */
   str_p = rcm_line;
   strncpy(str_p, num_cens, strlen(num_cens));
   str_p += strlen(num_cens);
   sprintf(total_str, "%02d", total);
   strncpy(str_p, total_str, strlen(total_str));
   str_p += strlen(total_str);
   strncpy(str_p, colon, strlen(colon));
   str_p += strlen(colon);
   if (total == 0)
   {
      *(rcm_line + RCM_NACOLS) = '\0';
      a3082k_store_line(irow, icol, nbytes, rcm_line, rcmbuf);
   }

   /* INITIALIZE INDEX PARAMETERS FOR BUFFER WRITE */
   is = 10;   /* "is" is the start pos'n for writing storm data */
   ic = 23;   /* "ic" is the start pos'n for writing the comma */
   lend = 0;  /* "lend" is the end of line indicator */

   /* COLLECT DATA FOR EACH CENTROID */
   cat_num_storms = total;
   for (storm_idx = 0; storm_idx < total; storm_idx++) 
   {
      /* GET LFM GRID LOCATION */
      a3082g_get_ij(&azim[storm_idx], &range[storm_idx], &elev[storm_idx],
         &ival, &jval);
      a30828_grid_letters(ival, jval, grid);

      /* ROUND TO NEAREST INTEGER CENTROID DIRECTION AND SPEED */
      ispd = RPGC_NINT(cspd[storm_idx]);
      idir = RPGC_NINT(cdir[storm_idx]);
      ism = is + 12;
      sprintf(cenchar, "%2s", (char*)&cennum[storm_idx]);

      /* WRITE CENTROID IDENTIFIER #, LFM LOCATION, DIRECTION AND SPEED */
      strncpy(str_p, cchar, strlen(cchar));      
      str_p += strlen(cchar);
      strncpy(str_p, cenchar, strlen(cenchar));      
      str_p += strlen(cenchar);
      strncpy(str_p, grid, strlen(grid));      
      str_p += strlen(grid);
      strncpy(str_p, grid + 1, strlen(grid + 1));      
      str_p += strlen(grid + 1);
      strncpy(str_p, grid + 2, strlen(grid + 2));      
      str_p += strlen(grid + 2);
      sprintf(idir_str, "%03d", idir);
      strncpy(str_p, idir_str, strlen(idir_str));      
      str_p += strlen(idir_str);
      sprintf(ispd_str, "%03d", ispd);
      strncpy(str_p, ispd_str, strlen(ispd_str));      
      str_p += strlen(ispd_str);

      /* CHECK IF MORE CENTROID DATA FOLLOWS: ADD COMMA SEPARATOR */
      if (total > storm_idx) 
      {
         strncpy(str_p, comma, strlen(comma));
      }
      is += 14;
      ic += 14;

      /* CHECK FOR END OF LINE CONDITION */
      if (storm_idx == 4 || storm_idx == 9 || storm_idx == 14 || storm_idx == 19)
      {
         lend = eol_flag;
      }

      /* CHECK IF NO MORE CENTROID DATA OR END OF LINE CONDITION EXISTS */
      if (total == storm_idx || lend == eol_flag)
      {
         a3082q_process_line(irow, icol, nbytes, &is, &ic, &increm, &lend,
            rcm_line, rcmbuf);
      }
   }
 
   /* ENCODE END OF PART A IDENTIFIER */
   strncpy(rcm_line, enda, strlen(enda));

   /* Make sure last character is a NULL terminator */
   *(rcm_line + RCM_NACOLS) = '\0';

   /* STORE LINE INTO RADAR CODED MESSAGE PRODUCT BUFFER */
   a3082k_store_line(irow, icol, nbytes, rcm_line, rcmbuf);

   //free(rcm_line);

   return 0;

} /* end a3082d_centroids_parta */

/*******************************************************************************
* Description:
*    Encodes Part B or C header line in the radar coded message based on
*    availability of VAD winds. If these are available normal Part A or B is
*    encoded; otherwise, "VADNA" is appended to the message.
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082e_partbc_header(int ipart, short split_date[], int hmtime, int irow,
   int icol, int nbytes, short* rcmbuf)
{
   static char *partb = "/NEXRBB ";
   static char *partc = "/NEXRCC ";
   static char *vadna = " VADNA";
   static char blank = ' ';
   static char *rcm_line = NULL;
   static char *str_p = NULL;
   static int i;
   static char partx[8];
   static int b_header = 1; /* B HEADER LINE FLAG */
   static char day_str[3], mo_str[3], yr_str[3];
   static char rpg_id_str[5];
   static char hmtime_str[5];


   /* SET PARTX FOR EITHER /NEXRBB OR /NEXRCC */
   if (ipart == 1)
   {
      strcpy(partx, partb);
   }
   if (ipart == 2)
   {
      strcpy(partx, partc);
   }
   
   /* Allocate and initialize rcm_line */
   rcm_line = (char *) malloc(RCM_NACOLS + 1);
   for (i = 0; i < RCM_NACOLS; i++)
   {
	rcm_line[i] = blank;
   }

   /* Set generic string pointer to rcm_line */
   str_p = rcm_line;

   /* IF VAD WINDS BUFFER IS NOT AVAILABLE AND IPART = 1, THEN ADD VADNA TO LINE
      INDICATING NO VAD WINDS AVAILABLE */

   /* WRITE /NEXRBB OR /NEXRCC, SIDD, DATE/TIME, AND VADNA */
   sprintf(rpg_id_str, "%04d", Siteadp.rpg_id);
   sprintf(day_str, "%02d", split_date[0]);
   sprintf(mo_str, "%02d", split_date[1]);
   sprintf(yr_str, "%02d", split_date[2]);
   sprintf(hmtime_str, "%04d", hmtime);
   if ((Rcm_params.vb[RCM_VWBUF] == RCM_FALSE) &&
       (ipart == b_header))
   {
      strncpy(str_p, partb, strlen(partb));
      str_p += strlen(partb);
      strncpy(str_p, rpg_id_str, strlen(rpg_id_str));
      str_p += strlen(rpg_id_str);
      strncpy(str_p, day_str, strlen(day_str));
      str_p += strlen(day_str);
      strncpy(str_p, mo_str, strlen(mo_str));
      str_p += strlen(mo_str);
      strncpy(str_p, yr_str, strlen(yr_str));
      str_p += strlen(yr_str);
      strncpy(str_p, hmtime_str, strlen(hmtime_str));
      str_p += strlen(hmtime_str);
      strncpy(str_p, vadna, strlen(vadna));
      str_p += strlen(vadna);
      rcm_line[RCM_NACOLS] = '\0';
   }
   else
   {
      /* TYPE *,'ENCODE B HEADER' */
      strncpy(str_p, partx, strlen(partx));
      str_p += strlen(partx);
      strncpy(str_p, rpg_id_str, strlen(rpg_id_str));
      str_p += strlen(rpg_id_str);
      strncpy(str_p, day_str, strlen(day_str));
      str_p += strlen(day_str);
      strncpy(str_p, mo_str, strlen(mo_str));
      str_p += strlen(mo_str);
      strncpy(str_p, yr_str, strlen(yr_str));
      str_p += strlen(yr_str);
      strncpy(str_p, hmtime_str, strlen(hmtime_str));
      str_p += strlen(hmtime_str);
      rcm_line[RCM_NACOLS] = '\0';
   }

   /* STORE LINE INTO RADAR CODED MESSAGE PRODUCT BUFFER */
   a3082k_store_line(&irow, &icol, &nbytes, rcm_line, rcmbuf);

   //free (rcm_line);

   return 0;

} /* end a3082e_partbc_header */

/*******************************************************************************
* Description:
*    Encodes VAD winds for Part B
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082f_vad_winds(float vad_data_hts[][VAD_HT_PARAMS], int irow, int icol,
   int nbytes, short* rcmbuf)
{
   /* Initialized data */
   static char *err_mess = "HEIGHT MISMATCH AT LOCATION: ";
   static char *comma = ",";
   static char blank = ' ';
   static char *endb = "/ENDBB";

   /* Local variables */
   static char *rcm_line = NULL;
   static char *str_p = NULL;
   static int idx;  /* Generic loop index variable */
   static int k, good_data;
   static int ic; /* Start position for writing comma */
   static int is; /* Start position for writing storm data */
   static int ism; /* End position for writing storm data */
   static int lend, idir, ispd;
   static float vdir[MAXVHGTS], vspd[MAXVHGTS], rconf[MAXVHGTS];
   static int found;
   static float height[MAXVHGTS];
   static int iheight;
   static char confval;
   static int indlist[MAXVHGTS], saveptr;
   static int l1_end = 6;  /* max # of vad winds on line 1 */
   static int l2_end = 12; /* max # of vad winds on line 2 */
   static int l3_end = 18; /* max # of vad winds on line 3 */
   static int eol_flag = 100; /* flag for end of line */
   static int increm = 11; /* INCREMENTAL VALUE FOR COMMA POSITION */
   char iheight_str[4];
   char idir_str[4];
   char ispd_str[4];

   /* Allocate and initialize rcm_line */
   rcm_line =  (char *)malloc(RCM_NACOLS + 1);
   for (idx = 0; idx < RCM_NACOLS; idx++)
   {
      rcm_line[idx] = blank;
   }

   /* INITIALIZE INDEX PARAMETERS FOR BUFFER WRITE */
   is = 0;
   ic = increm;
   lend = 0;
   saveptr = 0;

   /* CHECK IF RCM HEIGHTS ARE A SUBSET OF USER SELECTED HEIGHTS */
   for (idx = 0; idx < MAXVHGTS; ++idx)
   {
      found = RCM_FALSE;

      /* IF HEIGHT VALUE IS ZERO THEN EMPTY SO SKIP */
      if (Prod_sel.vad_rcm_heights.rcm[idx] != 0) 
      {
         for (k = 0; k < MAX_VAD_HTS; k++) 
         {
            /* IF MATCHING HEIGHTS ARE FOUND SAVE INDEX OF VAD_AHTS IN INDEX
               ARRAY TO USE LATER */
            if (Prod_sel.vad_rcm_heights.rcm[idx] == Prod_sel.vad_rcm_heights.vad[k])
            {
               ++saveptr;
               indlist[saveptr] = k;
               found = RCM_TRUE;
            }
         }

         /* IF MATCHING HEIGHTS ARE NOT FOUND WRITE ERROR MESSAGE */
         if (found == RCM_FALSE) 
         {
            /* CONSTRUCT AND SEND ERROR MESSAGE */
            LE_send_msg(GL_ERROR, "%29s%2d", err_mess, idx);
         }
      }
   }

   /* COLLECT GOOD DATA FOR VAD WINDS */
   idx = 0;
   for (k = 0; k < saveptr; k++) 
   {
      if ((vad_data_hts[indlist[k]][VAD_HTG] != NODATA) && 
          (vad_data_hts[indlist[k]][VAD_RMS] != NODATA) && 
          (vad_data_hts[indlist[k]][VAD_HWD] != NODATA) && 
          (vad_data_hts[indlist[k]][VAD_SHW] != NODATA)) 
      {
         /* SAVE HEIGHT */
         height[idx] = vad_data_hts[indlist[k]][VAD_HTG];

         /* SAVE CONFIDENCE LEVEL */
         rconf[idx] = vad_data_hts[indlist[k]][VAD_RMS];

         /* SAVE DIRECTION */
         vdir[idx] = vad_data_hts[indlist[k]][VAD_HWD];

         /* SAVE SPEED */
         vspd[idx] = vad_data_hts[indlist[k]][VAD_SHW];

         /* INCREMENT POINTER */
         idx++;
      }
   }

   /* SET UPPER INDEX FOR CONVERT LOOP EQUAL TO LAST INDEX THAT WAS COLLECTED ABOVE */
   good_data = idx;

   /* COLLECT DATA FOR EACH VAD WIND */
   for (k = 0; k < good_data; k++)
   {
      a3082t_vad_convert(&height[k], &rconf[k], &vdir[k], &vspd[k], &iheight,
         &confval, &idir, &ispd);
      ism = is + 9;

      /* WRITE VAD HEIGHT, CONFIDENCE LEVEL, DIRECTION AND SPEED */
      str_p = rcm_line;
      str_p += is;
      sprintf(iheight_str, "%03d", iheight);
      strncpy(str_p, iheight_str, strlen(iheight_str));
      str_p += strlen(iheight_str);
      strncpy(str_p, &confval, strlen(&confval));
      str_p += strlen(&confval);
      sprintf(idir_str, "%03d", idir);
      strncpy(str_p, idir_str, strlen(idir_str));
      str_p += strlen(idir_str);
      sprintf(ispd_str, "%03d", ispd);
      strncpy(str_p, ispd_str, strlen(ispd_str));
      str_p += strlen(ispd_str);

      /* CHECK IF MORE VAD DATA FOLLOWS: ADD COMMA SEPARATOR */
      if (good_data > k)
      {
         str_p = rcm_line;
         str_p += ic;
         strncpy(str_p, comma, strlen(comma));
      }

      is += 11;
      ic += 11;

      /* CHECK FOR END OF LINE CONDITION */
      if (k == l1_end || k == l2_end || k == l3_end)
      {
         lend = eol_flag;
      }

      /* CHECK IF NO MORE VAD DATA OR END OF LINE CONDITION EXISTS */
      if (good_data == k || lend == eol_flag)
      {
         a3082q_process_line(&irow, &icol, &nbytes, &is, &ic, &increm, &lend,
            rcm_line, rcmbuf);
      }
   }

   /* ENCODE END OF PART B IDENTIFIER */
   strncpy(rcm_line, endb, strlen(endb));
   rcm_line[RCM_NACOLS] = '\0';

   /* STORE LINE INTO RADAR CODED MESSAGE PRODUCT BUFFER */
   a3082k_store_line(&irow, &icol, &nbytes, rcm_line, rcmbuf);

   return 0;

} /* end a3082f_vad_winds */

/*******************************************************************************
* Description:
*    Encodes tornadic vortex signatures for Part C
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082h_tvs(int cat_num_storms, int* cat_feat, float* comb_att,
   int num_fposits, float* forcst_posits, int* cat_num_rcm, float* cat_tvst,
   int irow, int icol, int nbytes, short* rcmbuf)
{
   static char *colon = ":";
   static char *comma = ",";
   static char blank = ' ';
   static char *num_tvs = "/NTVS";
   static char *tvs = "TVS";
   int idx, idx2; /* generic loop indices */
   static char *rcm_line = NULL;
   static char *str_p = NULL;
   static int tvs_total, ic, is;
   static int ism, tot_num_tvs, lend;
   char grid[RCM_LEN_LETTERS];
   static int ival, jval;
   static float elev[20], azim[20];
   static float range[20];
   static int tvs_id;
   static int increm = 9; /* incremental value for comma placement */
   static int l1_end = 6;  /* max # of tvs on line 1 */
   static int l2_end = 13; /* max # of tvs on line 2 */
   static int l3_end = 20; /* max # of tvs on line 3 */
   static int eol_flag = 100; /* flag for end of line */
   char tvs_total_str[3];
   char tvs_id_str[3];

   /* Allocate and initialize rcm_line */
   rcm_line = (char *) malloc(RCM_NACOLS + 1);
   for (idx = 0; idx < RCM_NACOLS; idx++) 
   {
      rcm_line[idx] = blank;
   }

   tot_num_tvs = cat_num_rcm[2];

   /* COLLECT DATA FOR TVS WITHIN 230 KM RANGE */
   idx = 0;
   for (idx2 = 0; idx2 < tot_num_tvs; idx2++)
   {
      if ((cat_tvst[idx2 * 3 + 2] <= RCM_RNG_MAX) &&
          (cat_tvst[idx2 * 3 + 2] >= 0.0)) 
      {
         ++idx;

         /* SAVE AZIMUTH, RANGE AND ELEVATION ANGLE */
         azim[idx] = cat_tvst[idx2 * 3 + 1];
         range[idx] = cat_tvst[idx2 * 3 + 2];
         elev[idx] = cat_tvst[idx2 * 3 + 3];
      }
   }
   tvs_total = idx;

   /* WRITE TOTAL NUMBER OF TORNADIC VORTEX SIGNATURES */
   str_p = rcm_line;
   strncpy(str_p, num_tvs, strlen(num_tvs));
   str_p += strlen(num_tvs);
   sprintf(tvs_total_str, "%02d", tvs_total);
   strncpy(str_p, num_tvs, strlen(num_tvs));
   str_p += strlen(tvs_total_str);
   strncpy(str_p, colon, strlen(colon));
   str_p += strlen(colon);

   if (tvs_total == 0)
   {
      rcm_line[RCM_NACOLS] = '\0';
      a3082k_store_line(&irow, &icol, &nbytes, rcm_line, rcmbuf);
   }
   
   /* INITIALIZE INDEX PARAMETERS FOR BUFFER WRITE */
   is = 10;
   ic = 18;
   lend = 0;

   /* COLLECT DATA FOR EACH TVS */
   for (idx2 = 0; idx2 < tvs_total; idx2++)
   {
      tvs_id = idx2;

      /* GET LFM GRID LOCATION */
      a3082g_get_ij(&azim[idx2], &range[idx2], &elev[idx2], &ival, &jval);
      a30828_grid_letters(ival, jval, grid);
      ism = is + 7;

      /* WRITE TVS IDENTIFIER NUMBER AND LFM LOCATION */
      strncpy(str_p, tvs, strlen(tvs));
      str_p += strlen(tvs);
      sprintf(tvs_id_str, "%02d", tvs_id);
      strncpy(str_p, tvs_id_str, strlen(tvs_id_str));
      str_p += strlen(tvs_id_str);
      strncpy(str_p, &grid[0], strlen(&grid[0]));
      str_p += strlen(&grid[0]);
      strncpy(str_p, &grid[1], strlen(&grid[1]));
      str_p += strlen(&grid[1]);
      strncpy(str_p, &grid[2], strlen(&grid[2]));
      str_p += strlen(&grid[2]);

      /* CHECK IF MORE TVS DATA FOLLOWS: ADD COMMA SEPARATOR */
      if (tvs_total > idx2)
      {
         str_p = rcm_line;
         str_p += ic;
         strncpy(str_p, comma, strlen(comma));
      }
      is += 9;
      ic += 9;

      /* CHECK FOR END OF LINE CONDITION */
      if (idx2 == l1_end || idx2 == l2_end || idx2 == l3_end)
      {
         lend = eol_flag;
      }

      /* CHECK IF NO MORE TVS DATA OR END OF LINE CONDITION EXISTS */
      if (tvs_total == idx2 || lend == eol_flag)
      {
         rcm_line[RCM_NACOLS] = '\0';
         a3082q_process_line(&irow, &icol, &nbytes, &is, &ic, &increm, &lend,
            rcm_line, rcmbuf);
      }
   }

   //free (rcm_line);

   return 0;

} /* end a3082d_centroids_parta */


/*******************************************************************************
* Description:
*    Encodes mesocyclones (MDA features) for Part C
*
* Inputs:
*
* Outputs:
*
* Returns:
*    RCM_SUCCESS or RCM_FAILURE
*
* Globals:
*
* Notes:
*    As of CCR NA05-23001, all references to legacy mesocyclone information (i.e.
*    mesos and shears) are unused or replaced by references to MDA features.
*******************************************************************************/
int a3082i_mesocyclones(int cat_num_storms, int* cat_feat, float* comb_att,
   int num_fposits, float* forcst_posits, int* cat_num_rcm,
   float cat_mdat[][CAT_NMDA], int irow, int icol, int nbytes, short* rcmbuf)
{
   char *colon = ":";
   char *comma = ",";
   char blank = ' ';
   char *mchar = "M";
   char *num_meso = "/NMES";
   char *rcm_line = NULL;
   char *str_p = NULL;
   char meso_total_str[3];
   char meso_sr_str[3];
   int idx;  /* generic loop index */
   int ic, is;
   int meso_total;
   int ism, lend;
   char grid[RCM_LEN_LETTERS];
   int ival, jval;
   float elev[RCM_MAXSZ], azim[RCM_MAXSZ];
   int tot_num_meso;
   float range[RCM_MAXSZ];
   int meso_sr[RCM_MAXSZ];
   mda_adapt_t mda_adapt;
   int increm = 7;       /* INCREMENTAL VALUE FOR COMMA START */
   static int eol_flag = 100; /* flag for end of line */
   int l1_end = 8;  /* MAX NUMBER OF MESO ON LINE 1 */
   int l2_end = 18; /* MAX NUMBER OF MESO ON LINE 2 */
   int l3_end = 28; /* MAX NUMBER OF MESO ON LINE 3 */
   int l4_end = 38; /* MAX NUMBER OF MESO ON LINE 4 */
   int l5_end = 40; /* MAX NUMBER OF MESO ON LINE 5 */

   /* Allocate and initialize rcm_line */
   rcm_line = (char *) malloc(RCM_NACOLS + 1);
   for (idx = 0; idx < RCM_NACOLS; idx++)
   {
      rcm_line[idx] = blank;
   }

   tot_num_meso = cat_num_rcm[RCM_MDA];

   /* COLLECT DATA FOR MESO WITHIN 230 KM RANGE */
   meso_total = 0;
   for (idx = 0; idx < tot_num_meso; idx++)
   {
      if ((cat_mdat[idx][CAT_MDARN] <= RCM_RNG_MAX) &&
          (cat_mdat[idx][CAT_MDARN] >= 0) &&
          (cat_mdat[idx][CAT_MDASR] >= mda_adapt.min_filter_rank))
      {
         /* If meso_total within limits, store info */
         if ( meso_total < RCM_MAXSZ )
         {
            /* SAVE MDA STRENGTH RANK, AZIMUTH, RANGE AND ELEVATION INDEX */
            meso_sr[meso_total] = cat_mdat[idx][CAT_MDASR];
            azim[meso_total] = cat_mdat[idx][CAT_MDAAZ];
            range[meso_total] = cat_mdat[idx][CAT_MDARN];
            elev[meso_total] = cat_mdat[idx][CAT_MDAEL];

            /* Increment meso count */
            meso_total++;
         }
      }
   }

   /* WRITE TOTAL NUMBER OF MESOCYCLONES */
   str_p = rcm_line;
   strncpy(str_p, num_meso, strlen(num_meso));
   str_p += strlen(num_meso);
   sprintf(meso_total_str, "%02d", meso_total);
   strncpy(str_p, meso_total_str, strlen(meso_total_str));
   str_p += strlen(meso_total_str);
   strncpy(str_p, colon, strlen(colon));
   str_p += strlen(colon);

   if (meso_total == 0)
   {
      rcm_line[RCM_NACOLS] = '\0';
      a3082k_store_line(&irow, &icol, &nbytes, rcm_line, rcmbuf);
   }

   /* INITIALIZE INDEX PARAMETERS FOR BUFFER WRITE */
   is = 10;
   ic = 16;
   lend = 0;

   /* COLLECT DATA FOR ALL MESO'S AND 2D UNC. SHEARS */
   for (idx = 0; idx < meso_total; idx++)
   {
      /* GET LFM GRID LOCATION */
      a3082g_get_ij(&azim[idx], &range[idx], &elev[idx], &ival, &jval);
      a30828_grid_letters(ival, jval, grid);
      ism = is + 5;

      /* WRITE MESO OR 2D UNC. IDENTIFIER NUMBER AND LFM LOCATION */
      strncpy(str_p, mchar, strlen(mchar));
      str_p += strlen(mchar);
      sprintf(meso_sr_str, "%02d", meso_sr[idx]);
      strncpy(str_p, meso_sr_str, strlen(meso_sr_str));
      str_p += strlen(meso_sr_str);
      strncpy(str_p, &grid[0], strlen(&grid[0]));
      str_p += strlen(&grid[0]);
      strncpy(str_p, &grid[1], strlen(&grid[1]));
      str_p += strlen(&grid[1]);
      strncpy(str_p, &grid[2], strlen(&grid[2]));
      str_p += strlen(&grid[2]);

      /* CHECK IF MORE DATA FOLLOWS: ADD COMMA SEPARATOR */
      if (meso_total > idx)
      {  
         str_p = rcm_line;
         str_p += ic;
         strncpy(str_p, comma, strlen(comma));  
      }
      is += increm;
      ic += increm;

      /* CHECK FOR END OF LINE CONDITION */
      if (idx == l1_end || idx == l2_end || idx == l3_end || idx == l4_end
         || idx == l5_end) 
      {
         lend = eol_flag;
      }

      /* CHECK IF NO MORE DATA OR END OF LINE CONDITION EXISTS */
      if (meso_total == idx || lend == eol_flag)
      {
         a3082q_process_line(&irow, &icol, &nbytes, &is, &ic, &increm, &lend,
            rcm_line, rcmbuf);
      }
   }

   return RCM_SUCCESS;

} /* end a3082i_mesocyclones */


/*******************************************************************************
* Description:
*    For all storms less than 230 km specific centroid data is collected and
*    saved. This centroid data includes: storm id, azimuth, range, elevation,
*    storm top height and hail index. The total number of centroids is computed
*    and written to the rcm buffer. Finally, for all centroids the I,J
*    coordinates are calculated, the LFM grid location is computed, and all
*    centroid related data written into the RCM buffer.
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082j_centroids_partc(int cat_num_storms, int *cat_feat, float *comb_att,
   int num_fposits, float *forcst_posits, float *cat_tvst, int irow, int icol,
   int nbytes, short* rcmbuf)
{
   char *pchar = "P";
   char *nchar = "N";
   char *uchar = "U";
   char *colon = ":";
   char *comma = ",";
   char *cchar = "C";
   char blank = ' ';
   char *schar = "S";
   char *hchar = "H";
   char *hail = " ";
   char *num_cens = "/NCEN";
   static char *rcm_line = NULL;
   static char *str_p = NULL;
   int idx; /* Generic loop index */
   int counter;
   int ic, is;
   int cent_total;
   int ism;
   int lend;
   char grid[RCM_LEN_LETTERS];
   int ival, jval;
   float elev[20], azim[20], range[20];
   int total;
   float height[20];
   int cennum[20];
   int hailind[20], iheight;
   int increm = 14;
   float km2hft = M_TO_FT*10.0; /* Convert kilometers to 100s of feet */
   char cenchar[2];  /* STORM ID CHARACTER VALUE */
   int l1_end = 4; /* MAX NUMBER OF CENTROIDS ON LINE 1 */
   int l2_end = 9; /* MAX NUMBER OF CENTROIDS ON LINE 2 */
   int l3_end = 14; /* MAX NUMBER OF CENTROIDS ON LINE 3 */
   int l4_end = 19; /* MAX NUMBER OF CENTROIDS ON LINE 4 */
   int l5_end = 24; /* MAX NUMBER OF CENTROIDS ON LINE 5 */
   int l6_end = 29; /* MAX NUMBER OF CENTROIDS ON LINE 6 */
   static int eol_flag = 100; /* flag for end of line */
   char total_str[3];
   char iheight_str[4];

   /* Allocate and initialize rcm_line */
   rcm_line = (char *) malloc(RCM_NACOLS + 1);
   for (idx = 0; idx < RCM_NACOLS; idx++)
   {
      *(rcm_line + idx) = blank;
   }
   *(rcm_line + RCM_NACOLS) = '\0';

   /* COLLECT DATA FOR CENTROIDS WITHIN 230 KM RANGE */
   counter = 0;
   for (idx = 0; idx < cat_num_storms; idx++)
   {
      while ((*(comb_att + (idx * CAT_DAT) + CAT_RNG) <= RCM_RNG_MAX) &&
             (counter < RCM_MAX_STORM))
      {
         /* SAVE CENTROID NUMBER, AZIMUTH, RANGE, ELEVATION ANGLE, HEIGHT AND
            HAIL INDEX */
         ++counter;

         cennum[counter] = *(cat_feat + (idx * CAT_NF) + CAT_SID);
         azim[counter] = *(comb_att + (idx * CAT_DAT) + CAT_AZ);
         range[counter] = *(comb_att + (idx * CAT_DAT) + CAT_RNG);
         elev[counter] = *(comb_att + (idx * CAT_DAT) + CAT_ELCN);
         height[counter] = abs(*(comb_att + (idx * CAT_DAT) + CAT_STP) * km2hft);
         hailind[counter] = *(cat_feat + (idx * CAT_NF) + CAT_HAIL);
      }
   }

   cent_total = counter;
   total = MIN(Rcm_adapt.num_storms,cent_total);

   /* WRITE TOTAL NUMBER OF CENTROIDS */
   str_p = rcm_line;
   strncpy(str_p, num_cens, strlen(num_cens));
   str_p += strlen(num_cens);
   sprintf(total_str, "%02d", total);
   strncpy(str_p, total_str, strlen(total_str));
   str_p += strlen(total_str);
   strncpy(str_p, colon, strlen(colon));
   str_p += strlen(colon);
   *(rcm_line + RCM_NACOLS) = '\0';

   if (total == 0)
   {
      a3082k_store_line(&irow, &icol, &nbytes, rcm_line, rcmbuf);
   }

   /* INITIALIZE INDEX PARAMETERS FOR BUFFER WRITE */
   is = 10;  
   ic = 23;
   lend = 0;

   /* COLLECT DATA FOR EACH CENTROID */
   for (idx = 0; idx < total; idx++) 
   {
      /* GET LFM GRID LOCATION */
      a3082g_get_ij(&azim[idx], &range[idx], &elev[idx], &ival, &jval);
      a30828_grid_letters(ival, jval, grid);

      /* ADJUST HEIGHT TO MSL */
      height[idx] = height[idx] + (Siteadp.rda_elev / RCM_HUNDFT);

      /* ROUND TO NEAREST INTEGER FOR HEIGHT */
      iheight = RPGC_NINT(height[idx]);

      /* HAIL INDEX = 1 (HAIL), = 2 (POSSIBLE OR PROBABLE HAIL), = 3 (NO HAIL),
         = 4 (UNKNOWN) */
      if (hailind[idx] == LAB_POS)
      {
         hail = hchar;
      }
      if (hailind[idx] == LAB_PRB)
      {
         hail = pchar;
      }
      if (hailind[idx] == LAB_NEG) 
      {
         hail = nchar;
      }
      if (hailind[idx] == LAB_UNK)
      {
         hail = uchar;
      }
      ism = is + 12;  

      sprintf(cenchar, "%2s", (char*)&cennum[idx]);

      /* WRITE CENTROID IDENTIFIER #, LFM LOCATION, HEIGHT AND HAIL INDEX */
      str_p = rcm_line;
      str_p += is;
      strncpy(str_p, cchar, strlen(cchar));
      str_p += strlen(cchar);
      strncpy(str_p, cenchar, strlen(cenchar));
      str_p += strlen(cenchar);
      strncpy(str_p, &grid[0], strlen(&grid[0]));
      str_p += strlen(&grid[0]);
      strncpy(str_p, (char*)grid + 1, strlen((char*)grid + 1));
      str_p += strlen((char*)grid + 1);
      strncpy(str_p, (char*)grid + 2, strlen((char*)grid + 2));
      str_p += strlen((char*)grid + 2);
      strncpy(str_p, schar, strlen(schar));
      str_p += strlen(schar);
      sprintf(iheight_str, "%03d", iheight);
      strncpy(str_p, iheight_str, strlen(iheight_str));
      str_p += strlen(iheight_str);
      strncpy(str_p, hchar, strlen(hchar));
      str_p += strlen(hchar);
      strncpy(str_p, hail, strlen(hail));
      str_p += strlen(hail);

      /* CHECK IF MORE CENTROID DATA FOLLOWS: ADD COMMA SEPARATOR */
      if (total > idx)
      {
         str_p = rcm_line;
         str_p += ic;
         strncpy(str_p, comma, strlen(comma));
         str_p += strlen(comma);
      }
      is += increm;
      ic += increm;

      /* CHECK FOR END OF LINE CONDITION */
      if (idx == l1_end || idx == l2_end || idx == l3_end || idx == l4_end ||
          idx == l5_end || idx == l6_end)
      {
         lend = eol_flag;
      }

      /* CHECK IF NO MORE CENTROID DATA OR END OF LINE CONDITION EXISTS */
      if (total == idx || lend == eol_flag)
      {
         a3082q_process_line(&irow, &icol, &nbytes, &is, &ic, &increm, &lend,
            rcm_line, rcmbuf);
      }
   }

   return 0;

} /* end a3082j_centroids_partc */


/*******************************************************************************
* Description:
*    This module stores the header data into the output buffer for the radar
*    coded message product.
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a30824_header(short* bufout, int gendate, int gentime, int volnum,
   int prod_bytes)
{
   int idx; /* generic loop index */
   int totleng;
   int save_block;
   int blockid = 3; /* block id */
   int nblks = 3; /* number of blocks */
   int vol_spot_blank = 0; /* Spot Blank Field in Scan Summary Table def */
   int block_over = 8; /* NUMBER OF BYTES OF BLOCK OVERHEAD */
   int msgid = RPGC_get_code_from_name("RADARMSG");


   /*  STORE BLOCK DIVIDER, AND BLOCK ID */
   bufout[Rcm_params.rcmidx] = RCM_DIVIDER;
   bufout[Rcm_params.rcmidx + 1] = blockid;

   /*  CALCULATE LENGTH OF BLOCK FOR RCM PRODUCT */
   totleng = prod_bytes - (Rcm_params.rcmidx * RCM_BYTES_PER_HW) + 2;
   save_block = totleng;
   RPGC_set_product_int((void*)(bufout + (Rcm_params.rcmidx + 2)), totleng);
   Rcm_params.rcmidx += 4;

   /*  INIT HEADER TO ZERO */
   for (idx = Rcm_params.rcmidx; idx < Rcm_params.rcmidx + PHEADLNG; idx++) 
   {
      bufout[idx] = 0;
   }

   /*  MESSAGE ID, SOURCE ID, NUMBER OF BLOCKS AND DIVIDER */
   bufout[Rcm_params.rcmidx + MESCDOFF] = msgid;
   bufout[Rcm_params.rcmidx + SRCIDOFF] = (short) Siteadp.rpg_id;
   bufout[Rcm_params.rcmidx + NBLKSOFF] = nblks;
   bufout[Rcm_params.rcmidx + DIV1OFF] = RCM_DIVIDER;

   /*  RADAR LAT/LNG */
   RPGC_set_product_int((void*)&bufout[Rcm_params.rcmidx + LTMSWOFF],
      Siteadp.rda_lat);
   RPGC_set_product_int((void*)&bufout[Rcm_params.rcmidx + LNMSWOFF],
      Siteadp.rda_lon);

   /*  RADAR HEIGHT ABOVE SEA LEVEL IN FEET */
   bufout[Rcm_params.rcmidx + RADHGTOFF] = Siteadp.rda_elev;

   /*  WEATHER MODE */
   bufout[Rcm_params.rcmidx + WTMODOFF] = Summary->weather_mode;

   /*  VOLUME COVERAGE PATTERN */
   bufout[Rcm_params.rcmidx + VCPOFF] = Summary->vcp_number;

   /*  PRODUCT CODE */
   bufout[Rcm_params.rcmidx + PRDCODOFF] = msgid;

   /*  VOL SCAN NUMBER */
   bufout[Rcm_params.rcmidx + VSNUMOFF] = volnum;

   /*  VOLUME SCAN DATE AND TIME */
   bufout[Rcm_params.rcmidx + VSDATOFF] = Summary->volume_start_date;
   RPGC_set_product_int((void*)&bufout[Rcm_params.rcmidx + VSTMSWOFF],
      Summary->volume_start_time);

   /*  GENERATION DATE/TIME */
   bufout[Rcm_params.rcmidx + GDPRDOFF] = gendate;
   RPGC_set_product_int((void*)&bufout[Rcm_params.rcmidx + GTMSWOFF], gentime);

   /*  OFFSET TO PRODUCT INFO */
   RPGC_set_product_int((void*)&bufout[Rcm_params.rcmidx + OPRMSWOFF], PHEADLNG);

   /* SET SPOT BLANK STATUS */
   if (RPGCS_bit_test((unsigned char*)&Summary->spot_blank_status,
      vol_spot_blank))
   {
      bufout[Rcm_params.rcmidx + NMAPSOFF] = RCM_SBON;
   }

   /* LENGTH OF MESSAGE FOR ENTIRE RCM BUFFER 83 AND 74 */
   RPGC_set_product_int((void*)&bufout[LGMSWOFF], prod_bytes);

   /*  LENGTH OF MESSAGE FOR RCM BUFFER 74 */
   totleng = save_block - block_over;
   RPGC_set_product_int((void*)&bufout[Rcm_params.rcmidx + LGMSWOFF], totleng);

   return 0;

} /* end a30824_header */


/*******************************************************************************
* Description:
*    Stores the header data (LFM parameters, storm centroids, and raster
*    run length) into the output buffer for the intermediate graphic product for
*    radar coded message.
*
* Inputs:
*    int     cat_num_storms       Number of storms processed for the combined
*                                 attributes table.
*    int     *cat_feat            Table of associated severe features.  2-D 
*                                 array of storm ids and severe weather category
*                                 absent/present flags. [CAT_MXSTMS][CAT_NF]
*    float   *comb_att            Table of combined attributes. 2D array of 
*                                 storm attributes combined from the Storm
*                                 Series, TVS, and MDA algorithms.
*                                 [CAT_MXSTMS][CAT_DAT]
*    int     num_fposits          Number of positions for which forecasts were
*                                 made for the storms.
*    float   *forcst_posits       Forecast position data for each storm cell.
*                                 [CAT_MXSTMS][MAX_FPOSITS][FOR_DAT]
*    float   *cat_tvst            2D array of TVS attributes.
*                                 [CAT_MXSTMS][CAT_NTVS]
*    int     len_lay1             Length of Layer 1
*    short   *bufout              Product header array.
* Outputs:
*
* Returns:
*    RCM_SUCCESS or RCM_FAILURE
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082o_header_layers(int cat_num_storms, int *cat_feat, float *comb_att,
   int num_fposits, float *forcst_posits, float *cat_tvst, int len_lay1,
   short *bufout, short *rcmgrid)
{
   int ret = 0; /* generic function return value */
   int idx;
   int cent_total, save;
   int i4wrd; /* I*4 WORD EQUIVALENCED TO I2WRD */
   int num_shorts; /* number of I*2 words returned from RLE function */
   int len_lay2, len_lay3;
   int lay3_over = 4; /* LAYER THREE OVERHEAD */
   int block_over = 28; /* BLOCK OVERHEAD LENGTH */
   int lfm_lyrcd = 30; /* LFM LAYER CODE */
   int len_centroid = 20; /* LENGTH OF CENTROID LAYER DATA */
   int lay2_over = 4; /* LAYER 2 OVERHEAD */
   int sid_lyrcd = 31; /* STORM LAYER CODE */
   int run_lyrcd = 32; /* RUN LENGTH LAYER CODE */

   /* STORE LFM GRID PARAMETERS, STORE LAYER CODE AND ADAPTATION PARAMETERS FOR
      USE BY PUP, GROUP IN DISPLAYING THE LFM GRID */

   bufout[Rcm_params.rcmidx] = lfm_lyrcd;

   /* STORE DELTATHE- ANGLE ROTATION FROM NORTH TO LFM GRID COLUMN AXIS (- FOR
      EAST OF 105 DEGREES LONGITUDE, + FOR WEST OF 105) */
   RPGC_set_product_float((void*)&bufout[Rcm_params.rcmidx + 1],
      Rcm_adapt.angle_of_rotation);

   /* STORE XOFF- DISTANCE FROM RADAR TO UPPER RIGHT CORNER OF UNROTATED MM BOX
      (X AXIS DISTANCE) */
   RPGC_set_product_float((void*)&bufout[Rcm_params.rcmidx + 3],
      Rcm_adapt.x_axis_distance);

   /* STORE YOFF- DISTANCE FROM RADAR TO UPPER RIGHT CORNER OF UNROTATED MM BOX
      (Y AXIS DISTANCE) */
   RPGC_set_product_float((void*)&bufout[Rcm_params.rcmidx + 5],
      Rcm_adapt.y_axis_distance);

   /* STORE BOXSIZ- 1/16TH LFM GRID BOX SIZE */
   RPGC_set_product_float((void*)&bufout[Rcm_params.rcmidx + 7],
      Rcm_adapt.box_size);
   bufout[Rcm_params.rcmidx + 9] = 0;
   bufout[Rcm_params.rcmidx + 10] = 0;
   Rcm_params.rcmidx += 11;

   /* CHECK IF COMBINED ATTRIBUTE INPUT BUFFER WAS OBTAINED ADJUST CENTROID
      TOTAL TO REFLECT THE TOTAL NUMBER OF CENTROIDS WITHIN THE 230 KM RANGE */
   if (Rcm_params.vb[RCM_CABUF] == RCM_TRUE) 
   {
      cent_total = cat_num_storms;

      for (idx = 0; idx < cat_num_storms; idx++) 
      {
         if (*(comb_att + (idx * CAT_DAT) + CAT_RNG) > RCM_RNG_MAX) 
         {
            --cent_total;
         }
      }
   }
   else
   {
      /* COMBINED ATTRIBUTES INPUT BUFFER WAS NOT OBTAINED, SO SET CENTROID
         TOTAL TO ZERO */
      cent_total = 0;
   }

   /* STORE STORM INFORMATION FOR CENTROIDS STORE LAYER DIVIDER, LENGTH OF DATA
      LAYER, LAYER CODE, AND TOTAL NUMBER OF CENTROIDS */
   bufout[Rcm_params.rcmidx] = RCM_DIVIDER;
   len_lay2 = (len_centroid * RCM_BYTES_PER_HW *
      MIN(Rcm_adapt.num_storms,cent_total)) + lay2_over;
   RPGC_set_product_int((void *)&bufout[Rcm_params.rcmidx + 1], len_lay2);
   bufout[Rcm_params.rcmidx + 3] = sid_lyrcd;
   bufout[Rcm_params.rcmidx + 4] = (short) MIN(Rcm_adapt.num_storms, cent_total);
   Rcm_params.rcmidx += 5;

   /* BUILD PACKETS FOR STORM CENTROID DATA ONLY IF COMBINED ATTRIBUTES INPUT
      BUFFER WAS OBTAINED */
   if ((Rcm_params.vb[RCM_CABUF] == RCM_TRUE) &&
       (cat_num_storms > 0)) 
   {
      a3082s_packet_cent(cat_num_storms, cat_feat, comb_att,
         num_fposits, forcst_posits, cat_tvst, bufout);
   }

   /* STORE RASTER RUN LENGTH DATA */
   /* STORE LAYER DIVIDER, RUN LENGTH LAYER CODE, NUMBER OF ROWS */
   save = Rcm_params.rcmidx;
   bufout[Rcm_params.rcmidx] = RCM_DIVIDER;
   bufout[Rcm_params.rcmidx + 3] = run_lyrcd;
   bufout[Rcm_params.rcmidx + 4] = RCM_NROWS;
   Rcm_params.rcmidx += 5;

   /* CALL A3CM22 TO ENCODE THE RASTER RUN-LENGTH VALUES */
   ret = RPGC_raster_run_length(RCM_NROWS, RCM_NCOLS, rcmgrid,
      &(Colrtbl.coldat[0][0]) + (RCMIGP * PCTCDMAX), RCM_MAXIND, bufout,
      Rcm_params.rcmidx, &num_shorts);
   if ( ret != 0 )
   {
      LE_send_msg(GL_INFO,
         "a3082o: RPGC_raster_run_length returned error (%d)\n", ret);
   }
   
   /* STORE LENGTH OF DATA LAYER */
   len_lay3 = (num_shorts * RCM_BYTES_PER_HW) + lay3_over;

   RPGC_set_product_int((void*)&bufout[save + 1], len_lay3);

   /* STORE LENGTH OF BLOCK */
   i4wrd = len_lay1 + len_lay2 + len_lay3 + block_over;
   Rcm_params.msg_siz1 = i4wrd + RCM_PHBYTES;
   RPGC_set_product_int((void *)&bufout[LRMSWOFF], i4wrd);

   /* NOTE: RCMIDX IS THE NEXT AVAILABLE LOCATION FOR BUFFER STORAGE!! */
   Rcm_params.rcmidx += num_shorts;

   return (RCM_SUCCESS);

} /* end a3082o_header_layers */


/*******************************************************************************
* Description:
*    Store encoded line into RCM output buffer
*
* Inputs:
*    int*       irow        Row number
*    int*       icol        Column number
*    int*       nbytes      Byte offset into output buffer
*    char*      rcm_line    Radar coded message line to be stored in outbuf
*    short*     rcmbuf      Radar coded message output buffer
*
* Outputs:
*    short*     rcmbuf      Radar coded message output buffer
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
void a3082k_store_line(int* irow, int* icol, int* nbytes, char* rcm_line,
   short* rcmbuf)
{
   static int idx;  /* generic loop index */
   union data_union_t
   {
      char rcm_out[RCM_NACOLS];
      short ircm_line[RCM_NI2WDS_LINE];
   } data_union;

   /* TRANSFER INTEGER HALFWORD VALUE INTO OUTPUT BUFFER */
   for (idx = 0; idx < RCM_NACOLS; idx++)
   {
      *(unsigned char *)&data_union.rcm_out[idx] =
         *(unsigned char *)&rcm_line[idx];
   }
   for (idx = 0; idx < RCM_NI2WDS_LINE; idx++)
   {
      rcmbuf[*nbytes/RCM_BYTES_PER_HW] = data_union.ircm_line[idx];
      *nbytes += RCM_BYTES_PER_HW;
   }

   /* INCREMENT ROW NUMBER AND SET COLUMN = 0 */
   ++(*irow);
   *icol = 0;

} /* end a3082k_store_line */

/*******************************************************************************
* Description:
*    Encodes Part A header line
*
* Inputs:
*    short* split_date    Date in three halfwords
*    int*   hmtime        Hours and minutes of time data
     int*   irow          Row number 
*    int*   icol          Column number 
*    int*   nvol          Volume scan number 
*    int*   nbytes        Number of bytes 
*    short* rcmbuf        Radar-coded-message buffer
*
* Outputs:
*    int    opmodeoff     Operational mode offset
*    int    rcmoff        Offset into Part-A for RADNE (no reportable
*                         reflectivity intensity values) and RADOM (radar down 
*                         for maintenance) (from top of product buf header).
*    int*   icol          Column number 
*    int*   irow          Row number
*    int*   nbytes        Number of bytes 
*    short* rcmbuf        Radar-coded-message buffer 
*
* Returns:
*
* Globals:
*
* Notes: 
*    Converted from Fortran module a3082b.f
*******************************************************************************/
int a3082b_parta_header(short* split_date, int* hmtime, int* irow, int* icol,
   int nvol, int* nbytes, short *rcmbuf)
{
   static char *parta = "/NEXRAA ";
   static char *blank = " ";
   static char *colon = ":";
   static char *blanks = "    ";
   static char *unedit = "UNEDITED";
   static char *radom = " RADOM";
   static char *mode_precip = "PCPN";
   static char *mode_clear = "CLAR";
   static char *scans_11 = "1405";
   static char *scans_21 = "0906";
   static char *scans_31_32 = "0510";
   static char *op_mode = "/MD";
   static char *scan_min = "/SC";
   static char *num_ints = "/NI";
   int idx;
   char mode_out[5], scan_out[5];
   char *rcm_line = NULL;
   char *str_p = NULL;
   short rda_status[40];
   int read_status, down_status;
   int scanst, opstat;
   static int radar_down = 32; /* RADAR DOWN FLAG */
   static int vcp11 = 11; /* VOLUME COVERAGE PATTERN 11 */
   static int vcp21 = 21; /* VOLUME COVERAGE PATTERN 21 */
   static int vcp31 = 31; /* VOLUME COVERAGE PATTERN 31 */
   static int vcp32 = 32; /* VOLUME COVERAGE PATTERN 32 */
   static int num_init  = 0; /* INITIALIZED NUMBER OF INTENSITIES */
   static int radne_off  = 102; /* RADNE OFFSET */
   static int opmod_off  = 156; /* OPERATIONAL MODE OFFSET */
   char rpg_id_str[5];
   char day_str[4];
   char mo_str[3];
   char yr_str[3];
   char hmtime_str[5];
   char num_init_str[6];

   /* Allocate and initialize rcm_line */
   rcm_line = (char *) malloc(RCM_NACOLS + 1);
   for (idx = 0; idx < RCM_NACOLS; idx++)
   {
      rcm_line[idx] = *blank;
   }
   rcm_line[RCM_NACOLS] = '\0';

   /* CHECK IF STATUS OF RADAR IS DOWN, APPEND RADOM TO OUTPUT */
   /* WRITE /NEXRAA, SIDD, DATE/TIME, UNEDITED */

   /* Read the lastest RDA status data. */
   rcm_read_rdastatus_lb(&rda_status[0], &read_status);
   down_status = rda_status[1];
   str_p = rcm_line;
   sprintf(rpg_id_str, "%04d", Siteadp.rpg_id);
   sprintf(day_str, " %02d", split_date[0]);
   sprintf(mo_str, "%02d", split_date[1]);
   sprintf(yr_str, "%02d", split_date[2]);
   if (down_status == radar_down) 
   {
      strncpy(str_p, parta, strlen(parta));
      str_p += strlen(parta);
      strncpy(str_p, rpg_id_str, strlen(rpg_id_str));
      str_p += strlen(rpg_id_str);
      strncpy(str_p, day_str, strlen(day_str));
      str_p += strlen(day_str);
      strncpy(str_p, mo_str, strlen(mo_str));
      str_p += strlen(mo_str);
      strncpy(str_p, yr_str, strlen(yr_str));
      str_p += strlen(yr_str);
      sprintf(hmtime_str, "%04d", *hmtime);
      strncpy(str_p, hmtime_str, strlen(hmtime_str));
      str_p += strlen(hmtime_str);
      strncpy(str_p, blank, strlen(blank));
      str_p += strlen(blank);
      strncpy(str_p, unedit, strlen(unedit));
      str_p += strlen(unedit);
      strncpy(str_p, radom, strlen(radom));
      str_p += strlen(radom);
   }
   else
   {
      strncpy(str_p, parta, strlen(parta));
      str_p += strlen(parta);
      strncpy(str_p, rpg_id_str, strlen(rpg_id_str));
      str_p += strlen(rpg_id_str);
      strncpy(str_p, day_str, strlen(day_str));
      str_p += strlen(day_str);
      strncpy(str_p, mo_str, strlen(mo_str));
      str_p += strlen(mo_str);
      strncpy(str_p, yr_str, strlen(yr_str));
      str_p += strlen(yr_str);
      sprintf(hmtime_str, "%04d", *hmtime);
      strncpy(str_p, hmtime_str, strlen(hmtime_str));
      str_p += strlen(hmtime_str);
      strncpy(str_p, blank, strlen(blank));
      str_p += strlen(blank);
      strncpy(str_p, unedit, strlen(unedit));
      str_p += strlen(unedit);
   }

   /* STORE LINE INTO RADAR CODED MESSAGE PRODUCT BUFFER */
   a3082k_store_line(irow, icol, nbytes, rcm_line, rcmbuf);

   /* FIND OPERATIONAL MODE */
   opstat = Summary->weather_mode;
   if (opstat == PRECIPITATION_MODE)
   {
      strcpy(mode_out, mode_precip);
   }
   else
   {
      strcpy(mode_out, mode_clear);
   }

   /* FIND SCAN STRATEGY */
   scanst = Summary->vcp_number;
   if (scanst == vcp11) 
   {
      strcpy(scan_out, scans_11);
   }
   else if (scanst == vcp21)
   {
      strcpy(scan_out, scans_21);
   }
   else if (scanst == vcp31 || scanst == vcp32)
   {
      strcpy(scan_out, scans_31_32);
   }
   else
   {
      strcpy(scan_out, blanks);
   }

   /* CLEAR RCM_LINE TO BLANKS */
   for (idx = 0; idx < RCM_NACOLS; idx++)
   {
      rcm_line[idx] = *blank;
   }
   rcm_line[RCM_NACOLS] = '\0';

   /* WRITE OPERATIONAL MODE, SCAN STRATEGY, AND INITIALIZED NUMBER OF
      INTENSITIES */
   str_p = rcm_line;
   strncpy(str_p, op_mode, strlen(op_mode));
   str_p += strlen(op_mode);
   strncpy(str_p, mode_out, strlen(mode_out));
   str_p += strlen(mode_out);
   strncpy(str_p, blank, strlen(blank));
   str_p += strlen(blank);
   strncpy(str_p, scan_min, strlen(scan_min));
   str_p += strlen(scan_min);
   strncpy(str_p, scan_out, strlen(scan_out));
   str_p += strlen(scan_out);
   strncpy(str_p, blank, strlen(blank));
   str_p += strlen(blank);
   strncpy(str_p, num_ints, strlen(num_ints));
   str_p += strlen(num_ints);
   sprintf(num_init_str, "%05d", num_init);
   strncpy(str_p, num_init_str, strlen(num_init_str));
   str_p += strlen(num_init_str);
   strncpy(str_p, colon, strlen(colon));
   str_p += strlen(colon);

   /* SAVE INDEX FOR /NEXRAA LINE FOR RADNE */
   Rcm_params.rcmoff = Rcm_params.save_byte + radne_off;

   /* SAVE INDEX FOR OPERATIONAL MODE LINE FOR TOTAL COUNT */
   Rcm_params.opmodeoff = Rcm_params.save_byte + opmod_off;

   /* STORE LINE INTO RADAR CODED MESSAGE PRODUCT BUFFER */
   a3082k_store_line(irow, icol, nbytes, rcm_line, rcmbuf);

   return 0;

} /* end a3082b_parta_header */


/*******************************************************************************
* Description:
*    Encode the radar coded message buffer.
*
* Inputs:
*      int*     ist       I starting coordinate
*      int*     jst       J starting coordinate
*      int*     lbl       Label LFM box flag
*      int*     pos       Positional parameter
*      int*     run_vip   Run value index
*      int*     reps      Number of repetitions
*      int*     irow      Row number
*      int*     icol      Col number
*      int*     nbytes    Number of bytes
*      char*    rcmbuf    Radar coded message output buffer
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a30825_rcm_encode(int* ist, int* jst, int* lbl, int* pos, int* run_vip,
   int* reps, int* irow, int* icol, int* nbytes, char* rcmbuf)
{
   char vip[9] = "0" "1" "2" "3" "4" "5" "6" "7" "8";
   char zero_char = '0';
   char comma_char = ',';
   int k, l;
   char grid[RCM_LEN_LETTERS];
   int max_zero = 4; /* Maximum number of embedded zeros */
   int n_compact = 2; /* Compaction number */
  
   /* CHECK IF BEGINNING OR END OF ROW AND VIP = ZERO */
   if (*pos == RCM_END && *run_vip == 0)
   {
   }
   else if (*run_vip == 0 && *reps > max_zero) 
   { /* MORE THAN 4 ZERO VIPS IN RUN_VIP */
      *lbl = RCM_TRUE;
   }
   else
   { /* VIP IS > ZERO OR VIP = ZERO AND REPS < 4 */
      if (*lbl == RCM_TRUE)
      {
         *pos = RCM_MID;
         /* CHECK IF ENOUGH ROOM ON ROW TO PLACE COMMA SEPARATOR */
         if (*irow == 4 && *icol == 0) 
         {
         }
         else
         {
            if (*icol > RCM_NACOLS) 
            {
               ++(*irow);
               *icol = 0;
            }
            ++(*nbytes);
            *(rcmbuf + *nbytes) = comma_char;
            ++(*icol);
         }

         /* CHECK IF ENOUGH ROOM ON ROW TO PUT ID */
         if (*icol + 2 > RCM_NACOLS)
         {
            a30826_blank_pad(irow, icol, nbytes, rcmbuf);
            ++(*irow);
            *icol = 0;
         }

         /* GET 3 LETTER LFM GRID LOCATION AND STORE INTO BUFFER */
         a30828_grid_letters(*ist, *jst, grid);
         for (l = 0; l < RCM_LEN_LETTERS; ++l)
         {
            ++(*nbytes);
            *(rcmbuf + *nbytes) = grid[l];
         }

         *icol += RCM_LEN_LETTERS;
         *lbl = RCM_FALSE;
      }

      /* IF REPETITION IS > n_compact THEN START COMPACTION PROCESS */
      if (*reps > n_compact)
      {
         a3082a_compact_rcm(reps, run_vip, irow, icol, nbytes, rcmbuf);
      }
      else
      {
         /* REPETITIONS IS <= n_compact, WRITE RUN_VIP NUMBER INTO BUFFER */
         for (k = 0; k < *reps; k++)
         {
            if (*icol > RCM_NACOLS) 
            {
               *icol = 0;
               ++(*irow);
            }
            ++(*nbytes);
            *(rcmbuf + *nbytes) = *(unsigned char *)&vip[*run_vip];
            ++(*icol);
         }

         /* INCREMENT COUNTER FOR NUMBER OF INTENSITIES BY NUMBER OF REPETITIONS
            (DON'T INCLUDE EMBEDDED ZERO INTENSITIES IN COUNT) */
         if (*(unsigned char *)&vip[*run_vip] != zero_char)
         {
            Rcm_params.count_int += *reps;
         }
      }
   }

   return 0;

} /* end a30825_rcm_encode */


/*******************************************************************************
* Description:
*    Pad blanks in radar coded message.
*
* Inputs:
*
* Outputs:

* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a30826_blank_pad(int* irow, int* icol, int* nbytes, char* rcmbuf)
{
   int idx;  /* loop index variable */

   /* DO FOR ALL COLUMNS IN RCM PRODUCT- PLACE BLANK IN COLUMN */
   for (idx = *icol; idx <= RCM_NACOLS; idx++) 
   {
      ++(*nbytes);
      *(unsigned char *)&rcmbuf[*nbytes] = ' ';
   }

   *icol = RCM_NACOLS;

    return 0;

} /* end a30826_blank_pad */


/*******************************************************************************
* Description:
*    Determines the LFM grid coordinates of a given slant range, azimuth angle,
*    and elevation angle.  Equations implemented are specified in CPCI-03 B5
*    Section 10.6 - 3.0. 
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082g_get_ij(float* azim, float* range, float* elev, int* lfm_i,
   int* lfm_j)
{
   int ika[3] = { 1,4,10 };     /* Integer version of constant ka */
   double ka[3] = { 1.,4.,10. };/* Constant KA(m) as defined in B5, Sec 30.6 */
   int offset[3] = { 7,49,66 };
   double cos_ls = 0.;
   double sin_ls = 0.;
   double sin_lamdas_prime = 0.;
   double cos_lamdas_prime = 0.;

   /* Local variables */
   double b, r, gi, gj;
   int is[3], js[3];
   double rl, ls, cos_dlamda, sin_dlamda;
   int first_time;
   double gis, gjs, cos_b, sin_b, cos_l, sin_l, cos_s, sin_s;
   static float elevt;
   static double lamdas, pre_gxs;


   /* CHECK IF THIS IS FIRST TIME MODULE HAS BEEN CALLED SINCE TASK LOAD */

   first_time = RCM_TRUE;
   if (first_time) 
   {
      first_time = RCM_FALSE;

      /* COMPUTE SITE LATITUDE (LS) AND LONGITUDE (LAMDAS) BASED VALUES */
      ls = Siteadp.rda_lat / 1e3f;
      lamdas = Siteadp.rda_lon / 1e3f;
      cos_lamdas_prime = cos((lamdas + PRIME) * DEG_TO_RAD);
      sin_lamdas_prime = sin((lamdas + PRIME) * DEG_TO_RAD);
      sin_ls = sin(ls * DEG_TO_RAD);
      cos_ls = cos(ls * DEG_TO_RAD);

      /* COMPUTE COMMON PART OF THE GIS AND GJS EQUATIONS */
      pre_gxs = cos_ls * R2KO / (sin_ls + 1.);

      /* COMPUTE REGERENCE GRID BOX COORDINATES */
      gis = pre_gxs * sin_lamdas_prime + ip;
      gjs = pre_gxs * cos_lamdas_prime + ip;

      /* COMPUTE GRID BOX NUMBERS FOR BOX 0,0 OF LOCAL GRIDS */
      is[lfm16_idx] = (int) (ika[lfm16_idx] * (int)(gis) - offset[lfm16_idx]);
      js[lfm16_idx] = (int) (ika[lfm16_idx] * (int)(gjs) - offset[lfm16_idx]);
   }

   /* SAVE LOCAL COPY OF ELEVATION (ELEVT), RANGE (R) AND AZIMUTH (B) */
   elevt = *elev * DEG_TO_RAD;
   r = *range * cos(elevt);
   b = *azim;

   /* CALCULATE SINE AND COSINE OF A GIVEN BEARING ANGLE */
   sin_b = sin(b * DEG_TO_RAD);
   cos_b = cos(b * DEG_TO_RAD);

   /* CALCULATE SINE AND COSINE OF ANGLE S */
   sin_s = r / RE_PROJ * (1. - r * CONST / RE_PROJ_SQ);
   cos_s = sqrt(1. - sin_s * sin_s);

   /* CALCULATE SINE AND COSINE OF LAT (L) FOR A GIVEN RANGE BEARING CELL */
   sin_l = sin_ls * cos_s + cos_ls * sin_s * cos_b;
   cos_l = sqrt(1. - sin_l * sin_l);

   /* CALCULATE SINE AND COSINE OF DELTA LAMDA (I.E. DELTA LONGITUDE) */
   sin_dlamda = sin_s * sin_b / cos_l;
   cos_dlamda = sqrt(1. - sin_dlamda * sin_dlamda);
   rl = cos_l * R2KO / (sin_l + 1.);

   /* CALCULATE GI,GJ COORDINATE FOR A GIVEN RNG AND AZI IN 1/4 LFM BOX UNITS */
   gi = rl * (sin_dlamda * cos_lamdas_prime + cos_dlamda * sin_lamdas_prime) + ip;
   gj = rl * (cos_dlamda * cos_lamdas_prime - sin_dlamda * sin_lamdas_prime) + ip;

   /* CALCULATE LFM_I,LFM_J COORDINATES */
   *lfm_i = (int) ((int)(gi * ka[lfm16_idx]) - is[lfm16_idx]);
   *lfm_j = (int) ((int)(gj * ka[lfm16_idx]) - js[lfm16_idx]);

   return 0;

} /* end a3082g_get_ij */


/*******************************************************************************
* Description:
*    Insert letters into the grid.
*
* Inputs:
*    int      i          Grid I-coordinate of point
*    int      j          Grid J-coordinate of point
*    char     *ret_str   Return string for storing grid letters
*
* Outputs:
*    Stores 3-letter ID in ret_str
*
* Returns:
*    void
*
* Globals:
*
* Notes:
*******************************************************************************/
void a30828_grid_letters(int i, int j, char ret_str[RCM_LEN_LETTERS])
{
   int prei;        /* Conversion position for I-coord */
   int prej;        /* Conversion position for J-coord */
   int prek;        /* Conversion position */
   char letter[25] = "A" "B" "C" "D" "E" "F" "G" "H" "I" "J" "K" 
      "L" "M" "N" "O" "P" "Q" "R" "S" "T" "U" "V" "W" "X" "X";

   /* CALCULATE LFM GRID BOX AND RETURN IN GRIDID */
   prei = ((i - 1) / RCM_NUM4) + 1;
   prej = ((j - 1) / RCM_NUM4) + 1;
   ret_str[0] = letter[prej];
   ret_str[1] = letter[prei];
   prek = (((i - 1) - ((prei -1) * RCM_NUM4)) * RCM_NUM4) + j - 
          ((prej - 1) * RCM_NUM4);
   ret_str[2] = letter[prek];
   
} /* end a30828_grid_letters */


/*******************************************************************************
* Description:
*    Store encoded line into rcm output buffer
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082q_process_line(int* irow, int* icol, int* nbytes, int* is, int* ic,
   int* increm, int* lend, char* rcm_line, short* rcmbuf)
{
   char blank = ' ';
   int idx; /* generic loop index */

   /* STORE LINE INTO PRODUCT BUFFER */
    a3082k_store_line(irow, icol, nbytes, rcm_line, rcmbuf);

   /* CLEAR RCM_LINE, SET START WRITE POSITION FOR DATA AND COMMA */
   for (idx = 0; idx < RCM_NACOLS; idx++) 
   {
      rcm_line[idx] = blank;
   }
   rcm_line[RCM_NACOLS] = '\0';

   *is = 1;
   *ic = *increm;
   *lend = 0;

   return 0;

} /* end a3082q_process_line */


/*******************************************************************************
* Description:
*    This module converts input algorithm values for VAD winds for Part B to
*    radar coded message product output values.  Height is converted from feet
*    to hundreds of feet and speed is converted from meters/sec to knots.  RMS
*    confidence value is converted to the letters A-G as per NTR specifications.
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082t_vad_convert(float* height, float* rconf, float* vdir, float* vspd,
   int* iheight, char* confval, int* idir, int* ispd)
{
   char conf[7] = "A" "B" "C" "D" "E" "F" "G";
   int iconf;  /* RMS integer value */

   /* CONVERT INPUT HEIGHT(FEET) TO OUTPUT HEIGHT(HUNDREDS OF FEET) */
   *height = *height/ RCM_HUNDFT;
   *iheight = RPGC_NINT(*height);

   /* CONVERT INPUT RMS (REAL) TO OUTPUT RMS (CHARACTER) */
   iconf = (int)(*rconf);
   if (iconf == 0)
   {
      iconf = 1;
   }
   if (iconf > 7)
   {
      iconf = 7;
   }

   *(unsigned char *)confval = *(unsigned char *)&conf[iconf - 1];

   /* ROUND TO NEAREST INTEGER VAD DIRECTION (DEGREES) */
   *idir = RPGC_NINT(*vdir);

   /* CONVERT INPUT SPEED (METER/SEC) TO OUTPUT SPEED (KNOTS) */
   *vspd *= 1.9438f;
   *ispd = RPGC_NINT(*vspd);

   return 0;

} /* end a3082t_vad_convert */


/*******************************************************************************
* Description:
*    Stores the layer data for radar coded message centroids into packets.
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082s_packet_cent(int cat_num_storms, int *cat_feat, float *comb_att,
   int num_fposits, float *forcst_posits, float *cat_tvst, short *bufout)
{
   static char blank = ' ';
   static int k;
   static int cent_total; /* Total number of centroids within 230 KM range. */
   static float elev[RCM_MAX_STORM]; /* ELEVATION ANGLE OF CENTROID */
   static float azim[RCM_MAX_STORM]; /* AZIMUTH OF CENTROID */
   static float range[RCM_MAX_STORM]; /* RANGE OF CENTROID */
   static int cennum[RCM_MAX_STORM]; /* STORM ID NUMBER */
   static int counter;
   static int sid_packcd = 15; /* STORM PACKET CODE */
   static int len_blck = 6; /* LENGTH OF BLOCK */

   union data_union_1_t
   {
      char cenchar[4]; /* STORM ID CHARACTER */
      int tempnum;
   } data_union_1;

   union data_union_2_t
   {
      char rgt_just[4]; /* RIGHT JUSTIFIED VALUE OF CENCHAR */
      int temp_just;
   } data_union_2;

   /* COLLECT DATA FOR CENTROIDS WITHIN 230 KM RANGE */
   counter = 0;
   for (k = 0; k < cat_num_storms; k++)
   {
      if ( counter < RCM_MAX_STORM )
      {
         if (*(comb_att + (k * CAT_DAT) + CAT_RNG) <= (float)RCM_RNG_MAX)
         {
            /* SAVE CENTROID NUMBER, AZIMUTH, RANGE AND ELEVATION ANGLE */
            cennum[counter] = *(cat_feat + (k * CAT_NF) + CAT_SID);
            azim[counter] = *(comb_att + (k * CAT_DAT) + CAT_AZ) * DEG_TO_RAD;
            range[counter] = *(comb_att + (k * CAT_DAT) + CAT_RNG);
            elev[counter] = *(comb_att + (k * CAT_DAT) + CAT_ELCN) * DEG_TO_RAD;
            counter++;
         }
      }
      else
      {
         break; /* reached storm max, break out of for loop */
      }
   }

   cent_total = counter;

   /* STORE CENTROID DATA */
   for (k = 0; k < MIN(Rcm_adapt.num_storms, cent_total); k++) 
   {
      data_union_1.tempnum = cennum[k];

      /* STORE PACKET CODE FOR STORM ID, BLOCK LENGTH, I&J COORDINATES */
      bufout[Rcm_params.rcmidx] = sid_packcd;
      bufout[Rcm_params.rcmidx + RCM_OFF1] = len_blck;
      bufout[Rcm_params.rcmidx + RCM_OFF2] =
         (short) (RPGC_NINT(range[k] * sin(azim[k]) * cos(elev[k]))
         * RCM_NUM4);
      bufout[Rcm_params.rcmidx + RCM_OFF3] =
         (short)(RPGC_NINT(range[k] * cos(azim[k]) * cos(elev[k])) * RCM_NUM4);

      /* STORE CENTROID ID NUMBER */
      data_union_2.rgt_just[0] = blank;
      data_union_2.rgt_just[1] = blank;
      data_union_2.rgt_just[2] = data_union_1.cenchar[0];
      data_union_2.rgt_just[3] = data_union_1.cenchar[1];
      bufout[Rcm_params.rcmidx + RCM_OFF4] = (short) (data_union_2.temp_just);

      /* STORE PACKET CODE FOR SPECIAL SYMBOL, LENGTH OF BLOCK, I&J COORDINATES
         AND CODE FOR SPECIAL SYMBOL */
      bufout[Rcm_params.rcmidx + 5] = 2;
      bufout[Rcm_params.rcmidx + 6] = 6;
      bufout[Rcm_params.rcmidx + 7] = bufout[Rcm_params.rcmidx + 2];
      bufout[Rcm_params.rcmidx + 8] = bufout[Rcm_params.rcmidx + 3];
      bufout[Rcm_params.rcmidx + 9] = 8736;
      Rcm_params.rcmidx += 10;
   }

   return 0;

} /* end a3082s_packet_cent */


/*******************************************************************************
* Description:
*    Compacts consecutive VIP numbers
*
* Inputs:
*    int   reps       - number of repetitions 
*    int   run_vip    - run value index 
*    int   irow       - row number
*    int   icol       - column number
*    int   nbytes     - number of bytes
*    char* rcmbuf     - radar-coded-message buffer
*
* Outputs:
*    int   count_int  - total number of reflectivity intensities reported in
*                       Part A, with a range of 0 to 16.
*    int   irow       - row number
*    char* rcmbuf     - radar-coded-message buffer 
*
* Returns:
*    int
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082a_compact_rcm(int *reps, int *run_vip, int *irow, int *icol,
   int *nbytes, char *rcmbuf)
{
   static char letter[26] = "A" "B" "C" "D" "E" "F" "G" "H" "I" "J" "K" 
	    "L" "M" "N" "O" "P" "Q" "R" "S" "T" "U" "V" "W" "X" "X" "Z";
   static char vip[9] = "0" "1" "2" "3" "4" "5" "6" "7" "8";
   static char zero_char = '0';

   static int rem, num;
   static int repsa, colchk;

    /* Function Body */

   /* INITIALIZE COLUMN CHECK TO ONE, INCREMENT VALUE BASED ON NUMBER OF REPS */
   colchk = 1;
   if (*reps == 28)
   {
      ++colchk;
   }
   if (*reps >= 29) 
   {
      colchk += 2;
   }
   if (*reps == 55)
   {
      ++colchk;
   }
   if (*reps >= 56)
   {
      colchk += 2;
   }
   if (*reps == 82)
   {
      ++colchk;
   }
   if (*reps >= 83) 
   {
      colchk += 2;
   }

   /* NOT ENOUGH ROOM ON THIS ROW TO PUT VIP AND COMPACTION LETTER */
   if (*icol + colchk > 70) 
   {
      a30826_blank_pad(irow, icol, nbytes, rcmbuf + 1);
      ++(*irow);
      *icol = 1;
   }

   /* CALCULATE NUMBER OF COMPACTION GROUPS AND REMAINDER FROM MOD */
   repsa = *reps - 1;
   num = repsa / 27;
   rem = repsa % 27;

   /* IF ONE COMPACTION GROUP, STORE VIP AND COMPACTION LETTER */
   if (num == 0) 
   {
      *(unsigned char *)&rcmbuf[*nbytes + 1] = *(unsigned char *)&vip[*run_vip];
      *(unsigned char *)&rcmbuf[*nbytes + 2] = *(unsigned char *)&letter[repsa - 1];
      *nbytes += 2;
   }

   /* CHECK IF MORE COMPACTION IS NECESSARY */
   if (num >= 1) 
   {
      a3082p_compact_again(run_vip, nbytes, &num, &rem, rcmbuf + 1);
   }

   /* INCREMENT TOTAL NUMBER OF INTENSITIES (DON'T INCLUDE EMBEDDED ZEROS IN
      INTENSITY COUNT) */
   if (*(unsigned char *)&vip[*run_vip] != zero_char)
   {
      Rcm_params.count_int += *reps;
   }

   /* ADJUST COLUMN POSITIONS */
   if (*reps <= 27)
   {
      *icol += 2;
   }
   if (*reps == 28) 
   {
      *icol += 3;
   }
   if (*reps >= 29 && *reps <= 54) 
   {
      *icol += 4;
   }
   if (*reps == 55) 
   {
      *icol += 5;
   }
   if (*reps >= 56 && *reps <= 81) 
   {
      *icol += 6;
   }
   if (*reps == 82)  
   {
      *icol += 7;
   }
   if (*reps >= 83) 
   {
      *icol += 8;
   }

   return 0;

} /* end a3082a_compact_rcm */


/*******************************************************************************
* Description:
*    Compacts additional consecutive VIP numbers 
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
int a3082p_compact_again(int* run_vip, int* nbytes, int* num, int* rem,
   char *rcmbuf)
{
   /* Initialized data */

   static char letter[1*26] = "A" "B" "C" "D" "E" "F" "G" "H" "I" "J" "K" 
      "L" "M" "N" "O" "P" "Q" "R" "S" "T" "U" "V" "W" "X" "X" "Z";
   static char vip[1*9] = "0" "1" "2" "3" "4" "5" "6" "7" "8";

   /* STORE VIP NUMBER, LETTER(MAX_LETTER) AND VIP NUMBER */
   if (*num >= 1)
   {
      *(unsigned char *)&rcmbuf[*nbytes + 1] =
         *(unsigned char *)&vip[*run_vip];
      *(unsigned char *)&rcmbuf[*nbytes + 2] = *(unsigned char *)&letter[25];
      *(unsigned char *)&rcmbuf[*nbytes + 3] = *(unsigned char *)&vip[*run_vip];
      *nbytes += 3;
   }

   /* CHECK FOR NUM = 2, STORE LETTER(MAX_LETTER) AND VIP NUMBER */
   if (*num >= 2) 
   {
      *(unsigned char *)&rcmbuf[*nbytes + 1] = *(unsigned char *)&letter[25];
      *(unsigned char *)&rcmbuf[*nbytes + 2] = *(unsigned char *)&vip[*run_vip];
      *nbytes += 2;
   }

   /* CHECK FOR NUM = 3, STORE LETTER(MAX_LETTER), VIP NUMBER */
   if (*num == 3)
   {
      *(unsigned char *)&rcmbuf[*nbytes + 1] = *(unsigned char *)&letter[25];
      *(unsigned char *)&rcmbuf[*nbytes + 2] = *(unsigned char *)&vip[*run_vip];
      *nbytes += 2;
   }

   /* IF REMAINDER = 1, STORE VIP NUMBER */
   if (*rem == 1)
   {
      ++(*nbytes);
      *(unsigned char *)&rcmbuf[*nbytes] = *(unsigned char *)&vip[*run_vip];
   }

   /* IF REMAINDER >=2, STORE LETTER(REMAINDER) */
   if (*rem >= 2)
   {
      ++(*nbytes);
      *(unsigned char *)&rcmbuf[*nbytes] = *(unsigned char *)&letter[*rem - 1];
   }

   return 0;

} /* end a3082p_compact_again */


/*******************************************************************************
* Description:
*
* Inputs:
*
* Outputs:
*
* Returns:
*
* Globals:
*
* Notes:
*******************************************************************************/
void rcm_read_rdastatus_lb( short *rda_status, int *status )
{
   int size;
   RDA_status_t buff;

   /*
     Set the rda status data size, in bytes.
   */
   size = sizeof( RDA_status_msg_t ) - 
          sizeof( RDA_RPG_message_header_t );

   /*
     Read the RDA status data.  If read fails, set the RDA
     operability status to INOPERABLE and return.
   */
   if( (*status = ORPGDA_read( ORPGDAT_RDA_STATUS, (char*) &buff, 
                               sizeof(RDA_status_t),
                               RDA_STATUS_ID )) < 0 )
   {
      rda_status[RS_OPERABILITY_STATUS] = OS_INOPERABLE; 
      return;

   }

   /*
     Copy RDA status data to rda_status buffer.
   */
   memcpy( rda_status, &buff.status_msg.rda_status, size );

} /* end rcm_read_rdastatus_lb */
