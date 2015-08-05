/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:43:37 $
 * $Id: prcpacum_AcumAlg_buffctrl.c,v 1.2 2008/01/04 20:43:37 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_AcumAlg_buffctrl.c

   Description
   ===========
     The buffer control function is entered whenever there is an input buffer
   to process and it is this task's time to execute. This function performs
   two general functions: First, it represents the highest level of control   
   for the precipitation accumulation algorithm, passing control to subordinate
   functions to complete the required functions.  Second, it controls the 
   appropriate data interfaces for the predecessor rate and successor adjustment
   algorithm. The buffer control function determines whether or not the data in
   the input buffer is to be processed for the current volume scan.  If data
   is not to be processed, the input buffer is copied to the output buffer
   and the current rate scan  is written to disk so that it becomes the 
   previous rate scan the next time this task is executed.  If the input 
   buffer is to be processed, then period and hourly accumulations are computed
   and corrections made to the hourly totals should any of these values exceed
   a pre defined threshold.  If there are any errors during the aquisition of
   input and output buffers; or disk i/o errors, the task is aborted by calls
   RPGC_abort and RPGC_hari_kiri. 
   
   Change History
   ===============
   02/21/89      0000      P. Pisani            spr# 90067
   04/23/90      0001      David M. Lynch       spr# 90697
   08/06/90      0002      Edward Wendowski     spr# 90842
   01/08/91      0003      Paul Jendrowski      spr# 90888
   02/13/91      0003      Bayard Johnston      spr # 91254
   12/03/91      0004      Steve Anderson       spr # 92740
   12/10/91      0005      Ed Nichlas           spr 92637 pdl removal
   04/24/92      0006      Toolset              spr 91895
   03/25/93      0007      Toolset              spr na93-06801
   01/28/94      0008      Toolset              spr na94-01101
   03/03/94      0009      Toolset              spr na94-05501
   04/11/96      0010      Toolset              ccr na95-11802
   12/23/96      0011      Toolset              ccr na95-11807
   03/16/99      0012      Toolset              ccr na98-23803
   12/31/02      0013      D. Miller            ccr na00-28601
   06/30/03      0014      D. Miller            ccr na02-06508
   01/05/05      0015      C. Pham              ccr NA05-01303
****************************************************************************/
/* Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/* Local include files */
#include "prcpacum_AcumAlg_buffctrl.h"
#include "prcprtac_Constants.h"

void Accum_Buffer_Control( )
{
int   ostat, iostat, flg_dead, 
      offset, scansz, 
      bufsz; 
short cur_ratscn[MAX_AZMTHS][MAX_RABINS], /*Current rate scan of 360x115      */
      prev_ratscn[MAX_AZMTHS][MAX_RABINS],/*Previous rate scan read from disk */
      accumscan[MAX_AZMTHS][MAX_ACUBINS], /*Accumulation arrays scan-to-scan  */
      accumhrly[MAX_AZMTHS][MAX_ACUBINS], /*Accumulation arrays hourly scan   */
      nperdscn[MAX_AZMTHS][MAX_RABINS];   /*Period accumulation scan data     */
int   readrec = 0,           /*Constant read operation flag for LB_read call  */
      writerec = 1;          /*Constant write operation flag for LB_write call*/
char  *acumbuf = NULL;       /*Pointer to output buffer                       */
PRCPRTAC_smlbuf_t smlbuf;    /*Pointer to a PRCPRTAC_smlbuf_t structure       */
PRCPRTAC_medbuf_t medbuf;    /*Pointer to a PRCPRTAC_medbuf_t structure       */
PRCPRTAC_lrgbuf_t lrgbuf;    /*Pointer to a PRCPRTAC_lrgbuf_t structure       */

 if ( DEBUG ) 
 {
   fprintf(stderr,"BEGINNING MODULE A31351__BUFFER_CONTROLLER\n");
   fprintf(stderr," .. SMLSIZ_ACUM = %d  MEDSIZ_ACUM = %d  LRGSIZ_ACUM = %d\n",
                       SMLSIZ_ACUM,MEDSIZ_ACUM,LRGSIZ_ACUM);
 }

/* Adaptation data to convenient format for local use*/
  init_acum_adapt( );

/* Copy current RateScan array to cur_ratscn array local use */
  memcpy( cur_ratscn, RateScan, MAX_AZMTHS*MAX_RABINS*sizeof(short) );

  if ( DEBUG ) 
  {
    fprintf(stderr,"RateSupl.flg_badscn: %d   RateSupl.tim_scndif: %d\n",
                  RateSupl.flg_badscn,RateSupl.tim_scndif);
    fprintf(stderr,"THRSH_RESTART = %d\n",blka.thrsh_restart);
  }

/* If this is a bad scan, do not process it*/
  if ((RateSupl.flg_badscn==FLAG_SET)||(RateSupl.tim_scndif>blka.thrsh_restart))
  {
  /* Get small size output buffer to copy prcp stat message, adaptation
     data, supplemental data, and lfm 4 x 4.
   */
    acumbuf = (char *)RPGC_get_outbuf( HYACCSCN, SMLSIZ_ACUM, &ostat );

    /* If no error on get output buffer then proceed, else abort. */
    if ( ostat == NORMAL )
    {  

     /* Status from get output buffer good so copy data from input to output */
      copy_in2out_buffer( &smlbuf );
                                                
     /* Fill supplemental data array indicating a missing time period.*/
      fill_supl_missing( );

     /* Copy accumulation supplemental info to local struct */
      offset = SSIZ_PRE + SSIZ_RATE;
      memcpy(&smlbuf.HydrSupl[offset], &AcumSupl, sizeof(PRCPRTAC_acc_supl_t));

     /* Write rate scan header to disk.*/
      write_rate_hdr( &iostat );

     /* If i/o status ok proceed, else abort. */
      if ( iostat == IO_OK ) 
      {
       /* if this is not a bad scan or the current zero rate flag is clear
          then write the current rate scan to disk.
        */
        if (RateSupl.flg_zerate==FLAG_CLEAR && RateSupl.flg_badscn==FLAG_CLEAR)
        {
          if ( DEBUG ) 
            {fprintf(stderr,"Current rate scan written to disk w/o proc.\n");}

          iostat = Scan_IO( writerec, ratscn, cur_ratscn );
          if ( DEBUG ) {fprintf(stderr,"Scan_IO (iostat %d)\n",iostat);}
        }
      }/*End (iostat == IO_OK) */

     /* If i/o status good then release output buffer to be carried
         forward to adjustment. if status is bad, destroy output buffer.
      */
      if ( iostat == IO_OK ) 
      {
        memcpy( acumbuf, &smlbuf, sizeof(PRCPRTAC_smlbuf_t) );
        RPGC_rel_outbuf( (void *)acumbuf, FORWARD );
      }
      else 
      {
        RPGC_rel_outbuf( (void *)acumbuf, DESTROY );
        fprintf(stderr,"calling module RPGC_hari_kiri\n");
        RPGC_hari_kiri( );
      }

    }/*End ostat==NORMAL      */
    else 
    {

    /* Process aborted because output buffers could not be obtained.*/
      if ( DEBUG ) {fprintf(stderr," ***** Could not get output\n");}

      if ( ostat == NO_MEM ) {
        RPGC_abort_datatype_because( HYACCSCN, PROD_MEM_SHED );
      }
      else {
        RPGC_abort_datatype_because( HYACCSCN, ostat );
      }

    }  

  }/*End bad scan */
/* This is not a bad scan nor restart condition, so proceed to 
   analyse input data.
 */
  else 
  {

  /* Call function to get headers from disk, define hour, fill new headers,
     normalize times, determine non missing time in hour, and determine case
     type: extrapolate or interpolate. 
   */
    set_up_hdrs( &iostat ); 

  /* If i/o status ok proceed else abort. */
    if ( iostat == IO_OK ) 
    {
    /* Determine buffer size needed: small, medium, or large.*/
      buffer_needs( &bufsz );
  
    /* Get output buffer according to size determined.*/
      acumbuf = (char *)RPGC_get_outbuf( HYACCSCN, bufsz, &ostat );

    /* If status from get outbuf good, proceed else abort */
      if ( ostat == NORMAL )  
      {
     /* Copy precip status message, adaptation data, supplemental data and lfm
        4 x 4 from input to output buffer. 
      */
       copy_in2out_buffer( &smlbuf );

     /* Fill supplemental data array with appropriate flags and values
        now that there is an output buffer available. 
      */
       fill_supl_array( );

       if (RateSupl.flg_zerate==FLAG_CLEAR && RateSupl.flg_zerref==FLAG_CLEAR)
       {
       /* Read the previous rate scan from disk.*/
         if ( DEBUG ) {fprintf(stderr,"Previous rate scan read from disk\n");}
         iostat = Scan_IO( readrec, ratscn, prev_ratscn );
       }              

     /* If i/o status ok proceed, else abort. */
       if ( iostat == IO_OK ) 
       {
       /* Status from read scan good so determine period accumulations
          if current zero rate flag is clear. 
        */
         if ( RateSupl.flg_zerate == FLAG_CLEAR ) 
         {
           determine_period_acums( cur_ratscn, prev_ratscn, 
                                      nperdscn, accumscan );
         }
                
       /* Determine hourly accumulations if both zero hourly and no hourly
          flags are clear. 
        */
         if (( HourHdr[curr_hour].h_flag_zero == FLAG_CLEAR ) &&
             ( HourHdr[curr_hour].h_flag_nhrly == FLAG_CLEAR )) 
         {
             determine_hourly_acum( prev_ratscn, nperdscn,
                                    accumscan, accumhrly, &iostat );
         }

       /* If i/o status is good then prepare outputs which include:
          restore times, rotate indices, write period and hourly headers,
          write rate scan header, and write current rate scan to disk. 
        */
         if ( iostat == IO_OK ) 
         {
           prepare_outputs( cur_ratscn, nperdscn, accumscan, 
                                        accumhrly, &iostat ); 
         }

       }/*iostat == IO_OK */

     /* If i/o status indicates an error call subroutine to disable
        task and set flag indicating that the process is dead. 
      */
       if ( iostat != IO_OK ) 
       {
         fprintf(stderr,"***** a31351: i/o error = %d\n",iostat);
         fprintf(stderr,"calling RPGC_hari_kiri\n");
         RPGC_hari_kiri( );
         flg_dead = FLAG_SET;
       }
       else 
       {
         flg_dead = FLAG_CLEAR;
       }
     /* Copy Accumulation (AcumSupl) supplemental data to the output buffer */
       offset = SSIZ_PRE + SSIZ_RATE;
       memcpy(&smlbuf.HydrSupl[offset], &AcumSupl, sizeof(PRCPRTAC_acc_supl_t));

     /* if output buffers were succesfully acquired and there were no
        i/o errors, then the output buffer is released with disposition
        forward; otherwise, with disposition destroy. 
      */
       if ( flg_dead == FLAG_CLEAR )
       {

         scansz = MAX_AZMTH * MAX_HBINS * sizeof(short);
         if ( bufsz == SMLSIZ_ACUM ) 
         { 
           if (DEBUG) 
             {fprintf(stderr,"***** Need Small Buffsize (%d)\n",bufsz);}

           memcpy( (char *)acumbuf, &smlbuf, sizeof(PRCPRTAC_smlbuf_t) );
         }
         else if ( bufsz == MEDSIZ_ACUM ) 
         {
           if (DEBUG) 
             {fprintf(stderr,"***** Need Medium Buffsize (%d)\n",bufsz);}

           memcpy( &(medbuf.smlbuf_acum), &smlbuf, sizeof(PRCPRTAC_smlbuf_t) );
           memcpy( medbuf.AcumScan, accumscan, scansz );
           memcpy( acumbuf, &medbuf, sizeof(PRCPRTAC_medbuf_t) );
         }
         else 
         {
           if (DEBUG) 
             {fprintf(stderr," ***** Need Large Buffsize (%d)\n",bufsz);}

           memcpy( &(medbuf.smlbuf_acum), &smlbuf, sizeof(PRCPRTAC_smlbuf_t) );
           memcpy( medbuf.AcumScan, accumscan, scansz );
           memcpy( &(lrgbuf.medbuf_acum), &medbuf, sizeof(PRCPRTAC_medbuf_t) );
           memcpy( lrgbuf.AcumHrly, accumhrly, scansz );
           memcpy( acumbuf, &lrgbuf, sizeof(PRCPRTAC_lrgbuf_t) );
         }

         RPGC_rel_outbuf( acumbuf, FORWARD );

       }/*End flg_dead==FLAG_CLEAR */
       else
       {
         RPGC_rel_outbuf( acumbuf, DESTROY );
       }

      }/* opstat==NORMAL */
    /* Output buffer could not be obtained so abort task.*/
      else 
      {
        if (DEBUG)
          {fprintf(stderr,"***** a31351: could not get output %d\n",ostat);}

        if ( ostat == NO_MEM ) {
          RPGC_abort_datatype_because( HYACCSCN, PROD_MEM_SHED );
        }
        else {
          RPGC_abort_datatype_because( HYACCSCN, ostat );
        }
      }/*opstat!=NORMAL */
    }
  /* Disk i/o error so abort the task.*/
    else 
    {
      fprintf(stderr,"calling RPGC_hari_kiri\n");
      RPGC_hari_kiri( );
    }

  }/*End good scan */
 
  if ( DEBUG ) 
    {fprintf(stderr,"ENDING MODULE A31351__BUFFER_CONTROLLER\n\n");}
}
