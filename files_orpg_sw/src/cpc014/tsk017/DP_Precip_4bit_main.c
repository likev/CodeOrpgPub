/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/02/03 18:51:15 $
 * $Id: DP_Precip_4bit_main.c,v 1.6 2010/02/03 18:51:15 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/***********************************************************************
   Filename: DP_Precip_4bit_main.c

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

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----        -------    ----------         -----
   10/07       0000       Cham Pham          Initial Implementation
************************************************************************/

#include "DP_Precip_4bit_func_prototypes.h" /* LT_Accum_Buf_t */

int main( int argc, char *argv[] )
{
  char* inbuf_name = "DP_LT_ACCUM";

  int   vsnum = 0;            /* volume number    */
  int   opstat = RPGC_NORMAL; /* operation status */
  char  msg[200];             /* stderr message   */
  char  prodname[20];         /* product name     */

  LT_Accum_Buf_t* inbuf = NULL; /* input buffer pointer       */
  Coldat_t        Color_data;   /* color data table           */
  Siteadp_adpt_t  Siteadp;      /* site adaptation parameters */

  /* Register inputs and outputs. */

  RPGC_reg_io (argc, argv);

  /* Register to get the scan summary array.
   * The TAB uses this to get a volume time and the weather mode. */

  RPGC_reg_scan_summary();

  /* Register to get the color table. */

  RPGC_reg_color_table((void *) &Color_data, BEGIN_VOLUME);

  /* Register for site info adaptation data */

  RPGC_reg_site_info(&Siteadp);

  /* Initialize this task. This task is volume-based. */

  RPGC_task_init(TASK_VOLUME_BASED, argc, argv);

  sprintf(msg, "BEGIN CPC014/TSK017 FOR DUAL-POL 4-BIT PRECIP PRODUCTS \n");
  RPGC_log_msg(GL_INFO, msg);
  if(DP_PRECIP_4BIT_DEBUG)
    fprintf(stderr, msg);

  /* The main loop, which will never end. */

  while ( 1 )
  {
    RPGC_wait_act( WAIT_DRIVING_INPUT );

    /* Get the input buffer */

    inbuf = (LT_Accum_Buf_t*) RPGC_get_inbuf_by_name(inbuf_name, &opstat);

    if(inbuf == NULL || opstat != RPGC_NORMAL)
    {
       sprintf(msg, "RPGC_get_inbuf_by_name %s failed %p, opstat %d\n",
                     inbuf_name,
                     (void*) inbuf,
                     opstat);

       RPGC_log_msg(GL_INFO, msg);
       if(DP_PRECIP_4BIT_DEBUG)
          fprintf(stderr, msg);

       if(inbuf != NULL)
       {
          RPGC_rel_inbuf((void*) inbuf);
          inbuf = NULL;
       }

       RPGC_cleanup_and_abort(opstat);

       continue;

    } /* end didn't get input buffer */

    /* Get current volume scan number */

    vsnum = RPGC_get_buffer_vol_num(inbuf);

    sprintf(msg, "---------- volume: %d ---------- \n", vsnum);
    RPGC_log_msg(GL_INFO, msg);
    if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr, msg);

    /* ============================================================== */
    /* Build graphical OHE HOUR ACCUMULATION (OHA) Prod code 169      */
    /* ============================================================== */
    /* Determine whether product has been requested, this volume scan */

    sprintf(prodname, "OHAPROD");
    opstat = RPGC_check_data_by_name(prodname);

    if (opstat == RPGC_NORMAL)
       Build_OHA_product(inbuf, vsnum, prodname, &Color_data);
    else /* no GL_ERROR msg */
       RPGC_abort_dataname_because(prodname, opstat);

    /* ============================================================== */
    /* Build graphical STORM TOTAL ACCUMULATION (STA) Prod code 171   */
    /* ============================================================== */
    /* Determine whether product has been requested, this volume scan */

    sprintf(prodname, "STAPROD");
    opstat = RPGC_check_data_by_name(prodname);

    if (opstat == RPGC_NORMAL)
       Build_STA_product(inbuf, vsnum, prodname, &Color_data, &Siteadp);
    else /* no GL_ERROR msg */
       RPGC_abort_dataname_because(prodname, opstat);

    /* Release the input buffer */

    if(inbuf != NULL)
    {
       RPGC_rel_inbuf((void *) inbuf);
       inbuf = NULL;
    }

  } /* end while(1) */

  return 0;

} /* end main() ========================================== */
