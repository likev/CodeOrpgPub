/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:45 $
 * $Id: saa_comp_accum.h,v 1.2 2008/01/04 20:54:45 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/************************************************************
File 	: saa_comp_accum.h
Details	:Computes the hourly, 3 hourly and storm total accumulations from 
    	 the snow rate.  
*************************************************************/

#ifndef SAA_COMP_ACCUM_H
#define SAA_COMP_ACCUM_H

#include <stdlib.h>

int   current_hybscan_date;  /*current date for the scan - Julian date */
int   current_hybscan_time;  /*current time for the scan - seconds after midnight */

int hybscan_begin_date;
int hybscan_begin_time;
int stp_begin_date;/*mod 3/12 */
int stp_begin_time;
int stp_data_valid_flag;

int saa_compute_accumulations ();

/*returns true if compute_accumulations has not been called before */
int saa_acc_calledFirstTime();

/*returns true if there is valid data in the accumulation outputs */
int saa_Is_Accum_Valid();
#endif
