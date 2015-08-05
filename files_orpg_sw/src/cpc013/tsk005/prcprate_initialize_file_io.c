/*
 * RCS info
 * $Author: cm $
 * $Locker:  $
 * $Date: 2010/09/14 21:39:27 $
 * $Id: prcprate_initialize_file_io.c,v 1.3 2010/09/14 21:39:27 cm Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/*****************************************************************************
  File name: prcprate_initialize_file_io.c

  Description
  ===========
     Write initial values of zeroes to header records and bad
     time-stamp record to the reference scan file, and clear
     bad scan time-stamp array in common area.

  Change history:
  ==============
  09/16/88      0000      Greg Umstead         SPR # 80390
  04/13/90      0001      Dave Hozlock         SPR # 90697
  02/22/91      0002      Bayard Johnston      SPR # 91254
  02/15/91      0002      John Dephilip        SPR # 91762
  12/03/91      0003      Steve Anderson       SPR # 92740
  12/10/91      0004      Ed Nichlas           SPR 92637 PDL Removal
  04/24/92      0005      Toolset              SPR 91895
  03/25/93      0006      Toolset              SPR NA93-06801
  01/28/94      0007      Toolset              SPR NA94-01101
  03/03/94      0008      Toolset              SPR NA94-05501
  04/11/96      0009      Toolset              CCR NA95-11802
  12/23/96      0010      Toolset              CCR NA95-11807
  03/16/99      0011      Toolset              CCR NA98-23803
  11/25/04      0012      Cham Pham            CCR NA05-01303
*****************************************************************************/
/* Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/* Local include files */
#include "prcprtac_file_io.h"
#include "prcprtac_Constants.h"

void initialize_file_io( int *iostatus )
{
  int i;
  int writerec = 1;
  int lb_flags = LB_CREATE|LB_READ;

  *iostatus = 0;
  strcpy ( attr.remark, "HYACCUMS.DAT" );
  attr.mode = 0664;
  attr.msg_size = 82800;
  attr.maxn_msgs = 30;
  attr.types = LB_FILE|LB_DB;
  attr.tag_size = 32;

/* Debug writes...*/
  if (DEBUG) 
  {
   fprintf(stderr," ***** BEGIN MODULE A3134U__INITIALIZE_FILE_IO\n");
  }

/* File open for HYACCUMS.DAT */
  fdlb = open_disk_file();

  if ( DEBUG ) {fprintf(stderr,"After call open_disk_file %d\n",fdlb);}

  /* Check open status code...If file does not exist, initialize.*/
  if ( fdlb < 0 )  
  {
    if ( DEBUG ) {fprintf(stderr,"Create file: %s\n",LB_name);}

    fdlb = LB_open( LB_name, lb_flags, &attr );  

    if ( fdlb < 0 ) 
    {
       RPGC_log_msg(GL_ERROR,"LB_open %s (create) failed (fd %d)",LB_name,fdlb);
    }

    /* Initialize Rate header array to zeroes*/
    (void *) memset( &RateHdr, INIT_VALUE, sizeof(Rate_Header_t) );

    /* Initialize reference flags to appropriate values...*/
    RateHdr.hdr_rflagz = FLAG_SET;
    RateHdr.hdr_rfscgd = FLAG_CLEAR;

    /* Write header record ... */
    *iostatus = Header_IO ( writerec, rathdr );

    if ( DEBUG ) 
      {fprintf(stderr,"return from rate Header_IO (%d)\n",*iostatus);}

    if ( *iostatus == IO_OK ) 
    {
     /* Initialize values in period header array record...
        do for each period header...
      */
      memset( &PerdHdr, INIT_VALUE, ACZ_TOT_PRDS*sizeof(Period_Header_t) );

      for ( i=0; i<ACZ_TOT_PRDS; i++ ) 
      {
        /* Set flag(zero) and flag(missing)...*/
        PerdHdr[i].p_flag_zero = FLAG_SET;
        PerdHdr[i].p_flag_miss = FLAG_SET;
      }

     /* Write period header record ... */
      *iostatus = Header_IO( writerec, prdhdr );

      if (DEBUG) 
        {fprintf(stderr,"return period Header_IO (%d)\n",*iostatus);}

      if ( *iostatus == IO_OK ) 
      {
       /* Initialize the hourly header record...*/
       memset( &HourHdr, INIT_VALUE, ACZ_TOT_HOURS*sizeof(Hour_Header_t) );
        
       /* Set flag (no hourly) and flag (zero)...*/
       HourHdr[curr_hour].h_flag_nhrly = FLAG_SET;
       HourHdr[curr_hour].h_flag_zero  = FLAG_SET;

       /* Initialize current period and max hourly fields...*/
       HourHdr[curr_hour].h_curr_prd = NUM_PREV_PRD;

       /* Initialize scan type and case...*/
       HourHdr[curr_hour].h_scan_type = END_CURR;
       HourHdr[curr_hour].h_flag_case = INTERP;
         
       /* Write hourly header record ... */
       memcpy( &HourlyHdr, &HourHdr[curr_hour], sizeof(Hour_Header_t) );
       *iostatus = Header_IO( writerec, hlyhdr );

       if ( DEBUG ) 
       {
         fprintf(stderr,"return hourly Header_IO (%d)\n",*iostatus);
         fprintf(stderr,"======= A3134U HOURLY HEADER ========\n");
         fprintf(stderr,"h_beg_date: %d  %d\n",HourHdr[prev_hour].h_beg_date,
                                          HourHdr[curr_hour].h_beg_date);
         fprintf(stderr,"h_beg_time: %d  %d\n",HourHdr[prev_hour].h_beg_time,
                                          HourHdr[curr_hour].h_beg_date);
         fprintf(stderr,"h_flag_nhrly %d  %d\n",HourHdr[prev_hour].h_flag_nhrly,
                                           HourHdr[curr_hour].h_flag_nhrly);
         fprintf(stderr,"h_flag_zero %d  %d\n",HourHdr[prev_hour].h_flag_zero,
                                          HourHdr[curr_hour].h_flag_zero);
         fprintf(stderr,"h_end_date %d  %d\n",HourHdr[prev_hour].h_end_date,
                                         HourHdr[curr_hour].h_end_date);
         fprintf(stderr,"h_end_time %d  %d\n",HourHdr[prev_hour].h_end_time,
                                         HourHdr[curr_hour].h_end_time);
         fprintf(stderr,"h_scan_type %d  %d\n",HourHdr[prev_hour].h_scan_type,
                                          HourHdr[curr_hour].h_scan_type);
         fprintf(stderr,"h_curr_prd %d  %d\n",HourHdr[prev_hour].h_curr_prd,
                                         HourHdr[curr_hour].h_curr_prd);
         fprintf(stderr,"h_max_hrly %d  %d\n",HourHdr[prev_hour].h_max_hrly,
                                         HourHdr[curr_hour].h_max_hrly);
         fprintf(stderr,"h_flag_case %d  %d\n",HourHdr[prev_hour].h_flag_case,
                                          HourHdr[curr_hour].h_flag_case);
         fprintf(stderr,"h_spot_blank %d  %d\n",HourHdr[prev_hour].h_spot_blank,
                                           HourHdr[curr_hour].h_spot_blank);
         fprintf(stderr,"============================================\n");
       }

       if ( *iostatus == IO_OK ) 
       {
        /* Initialize values in bad scan array record buffer...*/
        memset( &a313hbsc, INIT_VALUE, MAX_TSTAMPS*sizeof(a313hbsc_t) );

        /* Write bad scan record ... */ 
        *iostatus = Badscan_IO( writerec );

        if ( DEBUG ) 
         {fprintf(stderr,"return Badscan_IO (%d)\n",*iostatus);}

       }/* End if block write hlyhdr */

      }/* End if block write prdhdr */

    }/* End if block write rathdr */

  }/* End(fdlb < 0) */

  if (DEBUG) {fprintf(stderr," ***** END MODULE A3134U_INITIALIZE_FILE_IO\n");}
}
