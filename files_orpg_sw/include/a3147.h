/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:09:57 $
 * $Id: a3147.h,v 1.4 2002/12/11 22:09:57 nolitam Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/***********************************************************************

	This file defines the macros that are needed by the ported RPG
	tasks.

	The contents in this file are derived from a3147.inc. The macros 
	must be consistent with those defined there. Thus if a3147.inc is 
	modified, this file has to be updated accordingly.

***********************************************************************/


#ifndef A3147_H
#define A3147_H

#include <a3146.h>             /* need KRADS and KBINS                    */

/*
 * Comment from a3147.inc ...
 *
 *    User Selectable Database (USERSEL.DAT) File Structure            Rec No.
 *    ---------------------------------------------------------        -------
 *    Header_Record_Number                      USDB_HDR_RECNO         ( 1   )
 *    Hourly_Record_Number  (MAX_USDB_HRS)      USDB_HRLY_RECNO        ( 2-31)
 *    Adjusted_Record_Number(MAX_ADJU_HRS)      USDB_ADJU_RECNO        (32-61)
 *    Dflt_24H_Record_Number                    DFLT_24H_RECNO         (   62)
 *
 * File Record Numbers
 */
#define USDB_HDR_RECNO	1
#define MAX_USDB_HRS	30
#define MAX_ADJU_HRS 	(MAX_USDB_HRS)
#define USDB_NEWEST	1
#define USDB_OLDEST	(MAX_USDB_HRS)
#define DFLT_24H_RECNO	((USDB_HDR_RECNO) + (MAX_USDB_HRS) + (MAX_ADJU_HRS) + 1)
#define MAX_USDB_RECS	(DFLT_24H_RECNO)

#define NUM_HDR_BYTES	1536
#define NUM_POLAR_BYTES	((KBINS) * (KRADS) * 2)

#define NUM_HDR_SCTRS	((((NUM_HDR_BYTES) - 1) / 256) + 1)
#define NUM_POLAR_SCTRS	((((NUM_POLAR_BYTES) - 1) / 256) + 1)
#define NUM_ADJU_SCTRS	(NUM_POLAR_SCTRS)
#define NUM_24H_SCTRS	(NUM_POLAR_SCTRS)

#define USDB_HDR_SCTR	0
#define DFLT_24H_SCTR	((USDB_HDR_SCTR) + (NUM_HDR_SCTRS) + ((NUM_POLAR_SCTRS) * (MAX_USDB_HRS)) + ((NUM_ADJU_SCTRS) * (MAX_ADJU_HRS)))
#define USDB_SIZE	((NUM_HDR_SCTRS) + ((NUM_POLAR_SCTRS) * (MAX_USDB_HRS)) + ((NUM_ADJU_SCTRS) * (MAX_ADJU_HRS)) + (NUM_24H_SCTRS))

#define USDB_NO_DATA	-2

/*
 * This is the C equivalent to the legacy A3147C9 common block ...
 */
typedef struct {
   fint usdb_start_dir ;
   fint4 usdb_sctr_offs[MAX_USDB_RECS] ;
   fint2 usdb_hrs_old[MAX_USDB_HRS] ;
   fint2 usdb_hrly_recno[MAX_USDB_HRS] ;
   fint2 usdb_adju_recno[MAX_ADJU_HRS] ;
   fint2 usdb_hrly_edate[MAX_USDB_HRS] ;
   fint2 usdb_hrly_etime[MAX_USDB_HRS] ;
   fint4 usdb_hrly_scan_type[MAX_USDB_HRS] ;
   fint4 usdb_sb_status_hrly[MAX_USDB_HRS] ;
   fint4 usdb_flg_zero_hrly[MAX_USDB_HRS] ;
   fint4 usdb_flg_no_hrly[MAX_USDB_HRS] ;
   freal usdb_cur_bias[MAX_USDB_HRS] ;
   freal usdb_cur_grpsiz[MAX_USDB_HRS] ;
   freal usdb_cur_mspan[MAX_USDB_HRS] ;
   fint4 usdb_flg_adjust[MAX_USDB_HRS] ;
   fint2 usdb_last_data_recno ;
   fint2 usdb_last_date ;
   fint2 usdb_last_time ;
   fint2 date_gcprod ;
   fint2 time_gcprod ;
   fint2 time_span_gcprod ;
   flogical flag_no_gcprod ;
   fint usdb_end_dir ;
   fint padding[31] ;          /* padding ensures sizeof(a3147c9_common_s)*/
                               /* matches NUM_HDR_BYTES ... 1536 bytes or */
                               /* six legacy disk 256-byte sectors        */
} a3147c9_common_s ;

#endif 		/* #ifndef A3147_H */
