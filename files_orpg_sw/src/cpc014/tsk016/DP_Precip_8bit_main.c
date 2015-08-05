/*
 */

/***********************************************************************
   Filename: DP_Precip_8bit_main.c

   Description
   ===========
      This module contains the main function for the Dual Pol
   Precipitation Products task.

   Input:   none
   Output:  none
   Returns: none
   Globals: none
   Notes:   All input and output for this function are provided through
            ORPG API services calls.

   Note: Site adaptation data was unused and removed. Used in STA tab
         to get the radar name. To put it back:

         Siteadp_adpt_t Siteadp; -* Site adaptation data *-

         RPGC_reg_site_info(&Siteadp);

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----        -------    ----------         -----
   10/07       0000       Cham Pham          Initial Implementation
************************************************************************/

#include "DP_Precip_8bit_func_prototypes.h"
#include "coldat.h"

int main( int argc, char *argv[] )
{
  char* inbuf_name = "DP_LT_ACCUM";

  int             vsnum = 0;
  int             opstat = RPGC_NORMAL;
  char            msg[200];
  char            prodname[20];
  LT_Accum_Buf_t* inbuf = NULL;

  /* 16 Aug 2010 Dan Stein - Added the Color_data structure so we can get the
   * max 4-bit storm-total value.  We'll use this max in building the DSA 
   * product.
   */
  Coldat_t        Color_data;   /* color data table           */
  int DSA_max = 0;  /* Max value for the DSA product, = 2 * 4-bit max */


  /* Register inputs and outputs. */

  RPGC_reg_io (argc, argv );

  /* Register adaptation blocks and scan summary array. */

  RPGC_reg_scan_summary();

  /* Register to get the color table. Stein 16 Aug 2010 */

  RPGC_reg_color_table((void *) &Color_data, BEGIN_VOLUME);

  /* Initialize this task. This task is volume-based. */

  RPGC_task_init( TASK_VOLUME_BASED, argc, argv );

  sprintf(msg, "BEGIN CPC014/TSK016 FOR DUAL-POL 8-BIT PRECIP PRODUCTS \n");
  RPGC_log_msg(GL_INFO, msg);
  if(DP_PRECIP_8BIT_DEBUG)
     fprintf(stderr, msg);

  /* The main loop, which will never end. */

  while ( 1 )
  {
    RPGC_wait_act( WAIT_DRIVING_INPUT );

    /* Get input buffer */

    inbuf = (LT_Accum_Buf_t*) RPGC_get_inbuf_by_name(inbuf_name, &opstat);

    if ((inbuf == NULL) || (opstat != RPGC_NORMAL))
    {
       sprintf(msg, "RPGC_get_inbuf_by_name %s failed %p, opstat %d\n",
                     inbuf_name,
                     (void*) inbuf,
                     opstat);

       RPGC_log_msg(GL_INFO, msg);
       if(DP_PRECIP_8BIT_DEBUG)
          fprintf(stderr, msg);

       if (inbuf != NULL)
       {
          RPGC_rel_inbuf ((void*) inbuf);
          inbuf = NULL;
       }

       RPGC_cleanup_and_abort(opstat);

       continue; /* go back to waiting for wake-up */

    } /* end didn't get input buffer */

    /* Get current volume scan number */

    vsnum = RPGC_get_buffer_vol_num( inbuf );

    sprintf(msg, "---------- volume: %d ---------- \n", vsnum);
    RPGC_log_msg(GL_INFO, msg);
    if ( DP_PRECIP_8BIT_DEBUG )
       fprintf(stderr, msg);

    /* ==================================================================== */
    /* Build graphical DIGITAL ONE HOUR ACCUMULATION (DAA) Prod code 170    */
    /* ==================================================================== */

    sprintf(prodname, "DAAPROD");
    opstat = RPGC_check_data_by_name(prodname);

    if (opstat == RPGC_NORMAL) /* 170 was requested this volume scan */
       build_DAA_product(inbuf, vsnum, prodname);
    else /* no GL_ERROR msg */
       RPGC_abort_dataname_because(prodname, opstat);

    /* ==================================================================== */
    /* Build graphical DIGITAL STORM TOTAL ACCUMULATION (DSA) Prod code 172 */
    /* ==================================================================== */

    sprintf(prodname, "DSAPROD");
    opstat = RPGC_check_data_by_name(prodname);

    if (opstat == RPGC_NORMAL) /* 172 was requested this volume scan */
    {

       /* This is the maximum value as defined by the user. This could change
        * as often as every volume. We'll use twice this value as a "cap" in
        * the DSA product. We agreed to use 2 times the STP/STA max, but that
        * value has no scientific basis (i.e. it was chosen somewhat
        * arbitrarily). The actual max we use in the product will be the
        * SMALLEST value of either product max or the DSA_max (i.e. use the
        * product max until it exceeds DSA_max). COLOR_INDEX_171 is the
        * color index table for the 4-bit storm-total products (STA & STP).
        * Note - the color table value - 4096 * 100 just seems to work. We
        * don't know why - the reason is probably lost in the obscurity of
        * the legacy Fortran PPS. We multiply by 100 to convert the value
        * from tenths of inches to thousandths of inches.
        */
       DSA_max = ( (Color_data.thresh[COLOR_INDEX_171][15] - 4096) * 100) * 2;

       build_DSA_product(inbuf, vsnum, prodname, DSA_max);
    }
    else /* no GL_ERROR msg */
       RPGC_abort_dataname_because(prodname, opstat);

    /* ==================================================================== */
    /* Build graphical DIGITAL ONE HOUR DIFFERENCE (DOD) Prod code 174      */
    /* ==================================================================== */

    sprintf(prodname, "DODPROD");
    opstat = RPGC_check_data_by_name(prodname);

    if (opstat == RPGC_NORMAL) /* 174 was requested this volume scan */
       build_DOD_product(inbuf, vsnum, prodname);
    else /* no GL_ERROR msg */
       RPGC_abort_dataname_because(prodname, opstat);

    /* ==================================================================== */
    /* Build graphical DIGITAL STORM TOTAL DIFFERENCE (DSD) Prod code 175   */
    /* ==================================================================== */

    sprintf(prodname, "DSDPROD");
    opstat = RPGC_check_data_by_name(prodname);

    if (opstat == RPGC_NORMAL) /* 175 was requested this volume scan */
       build_DSD_product(inbuf, vsnum, prodname);
    else /* no GL_ERROR msg */
       RPGC_abort_dataname_because(prodname, opstat);

    /* Release the input buffer */

    if(inbuf != NULL)
    {
       RPGC_rel_inbuf((void *) inbuf);
       inbuf = NULL;
    }

  } /* end while loop forever */

  return 0;

} /* end main() ============================================= */
