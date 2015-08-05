/*
 * rcs info
 * $author$
 * $locker$
 * $date$
 * $id$
 * $revision$     
 * $state$
 */ 
/*.****************************************************************************
*.  File Name:  prcpacum_read_header_flds.c
*.  
*.  Description   
*.  ===========   
*.         This function performs the following functions:
*.    1) Reads previous periods from disk into the period header array.
*.    2) Reads the previous hourly data from disk. 
*.    3) Sets the current_index to the value contained within the hour
*.       header for the previous hour. This function is called during
*.       precipitation accumulation initalization as setup prior to
*.       computation of period and hourly totals.
*.
*.  Change History
*.  ==============
*.  02/21/89      0000      P. Pisani            spr # 90067
*.  02/22/91      0001      Bayard Johnston      spr # 91254
*.  02/15/91      0001      John Dephilip        spr # 91762
*.  12/03/91      0002      Steve Anderson       spr # 92740
*.  12/10/91      0003      Ed Nichlas           spr 92637 pdl removal
*.  04/24/92      0004      Toolset              spr 91895
*.  03/25/93      0005      Toolset              spr na93-06801
*.  01/28/94      0006      Toolset              spr na94-01101
*.  03/03/94      0007      Toolset              spr na94-05501
*.  04/11/96      0008      Toolset              ccr na95-11802
*.  12/23/96      0009      Toolset              ccr na95-11807
*.  03/16/99      0010      Toolset              ccr na98-23803
*.  12/31/02      0011      D. Miller            ccr na00-28601
*.  01/05/05      0012      Cham Pham            ccr NA05-01303
*****************************************************************************/
/* Global include file */
#include <a313h.h>

/* Local include files */
#include "prcprtac_file_io.h"
#include "prcprtac_Constants.h"

void read_header_flds( int *iostat )
{
 int readrec = 0;

  if ( DEBUG ) {fprintf(stderr,"A3135V__READ_HDR_FLDS\n");}

/* Set i/o status to i/o ok */
  *iostat = IO_OK;

/* Read in previous period hour header. */
  *iostat = Header_IO( readrec, prdhdr );
  
  if ( DEBUG ) 
  {
    fprintf(stderr,"iostat1 = %d\n",*iostat);
    fprintf(stderr,"=== BEFORE A3135V HOURLY HEADER ========\n");
    fprintf(stderr,"h_beg_date: %d  %d\n",HourHdr[prev_hour].h_beg_date,
                                          HourHdr[curr_hour].h_beg_date);
    fprintf(stderr,"h_beg_time: %d  %d\n",HourHdr[prev_hour].h_beg_time,
                                          HourHdr[curr_hour].h_beg_time);
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

/* If i/o status is good then read in 13 period headers. */
  if (*iostat == IO_OK) 
  {
    *iostat = Header_IO( readrec, hlyhdr );
    memcpy( &HourHdr[prev_hour], &HourlyHdr, sizeof(Hour_Header_t) );
    
    if ( DEBUG ) 
    {
      fprintf(stderr,"=== AFTER A3135V HOURLY HEADER ========\n");
      fprintf(stderr,"h_beg_date: %d  %d\n",HourHdr[prev_hour].h_beg_date,
                                          HourHdr[curr_hour].h_beg_date);
      fprintf(stderr,"h_beg_time: %d  %d\n",HourHdr[prev_hour].h_beg_time,
                                          HourHdr[curr_hour].h_beg_time);
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
      fprintf(stderr,"iostat2 = %d\n",*iostat);
    }

/* If i/o status still good then set current index from slot in
   previous hour header. 
 */
    if ( *iostat == IO_OK ) 
    {
      blka.current_index = HourHdr[prev_hour].h_curr_prd;
      if (DEBUG) 
      { 
        fprintf(stderr,"CURRENT_INDEX: %d  HourHdr[%d].h_curr_prd: %d\n",
                  blka.current_index,prev_hour,HourHdr[prev_hour].h_curr_prd);
      }
    }  

  }/* End if block (*iostat == IO_OK) */
}
