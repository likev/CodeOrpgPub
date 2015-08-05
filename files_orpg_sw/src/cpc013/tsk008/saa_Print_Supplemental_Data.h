/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:42 $
 * $Id: saa_Print_Supplemental_Data.h,v 1.2 2008/01/04 20:54:42 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/**************************************************************
File	: saa_Print_Supplemental_Data.h
Details	: prints the supplemental data obtained from the Hybrid Scan
	  input buffer for debugging 
***************************************************************/


#include "saaConstants.h"
#include "input_buffer_struct.h"
#include <stdlib.h>
#include <stdio.h>

/**************************************************************
Method	: saa_Print_Supplemental_Data
Details	: prints the supplemental data obtained from the Hybrid Scan
	  input buffer for debugging 
***************************************************************/

void saa_Print_Supplemental_Data(){

	/*int dd,dm,dy;*/
	
	fprintf(stderr,	"Supplemental Data \n");
	fprintf(stderr,	"avgdate : %d\n",		hybscan_suppl.avgdate);
	fprintf(stderr,	"avgtime : %d\n",		hybscan_suppl.avgtime);
	fprintf(stderr,	"zerohybrd : %d\n",		hybscan_suppl.zerohybrd);
	fprintf(stderr,	"rain_detec_flg : %d\n", 	hybscan_suppl.rain_detec_flg);
	fprintf(stderr,	"reset_stp_flg : %d\n",		hybscan_suppl.reset_stp_flg);
	fprintf(stderr,	"prcp_begin_flg : %d\n",	hybscan_suppl.prcp_begin_flg);
	fprintf(stderr,	"last_date_rain : %d\n",	hybscan_suppl.last_date_rain);
	fprintf(stderr,	"last_time_rain : %d\n",	hybscan_suppl.last_time_rain);
	fprintf(stderr,	"rej_blkg_cnt : %d\n",		hybscan_suppl.rej_blkg_cnt);
	fprintf(stderr,	"rej_cltr_cnt : %d\n",		hybscan_suppl.rej_cltr_cnt);
	fprintf(stderr,	"tot_bins_smooth : %d\n",	hybscan_suppl.tot_bins_smooth);
	fprintf(stderr,	"pct_hys_filled : %3.3f\n",	hybscan_suppl.pct_hys_filled);
	fprintf(stderr,	"highest_elang : %3.3f\n",	hybscan_suppl.highest_elang);
	fprintf(stderr,	"sum_area : %3.3f\n",		hybscan_suppl.sum_area);
	fprintf(stderr,	"vol_sb : %d\n",		hybscan_suppl.vol_sb);
	
	
}/*end saa_Print_Supplemental_Data */

/**************************************************************
Method	: saa_Print_Hydro_Message
Details	: prints hydro message obtained from EPRE
***************************************************************/
void saa_Print_Hydro_Message(){

	fprintf(stderr,	"Hydro Message  \n");
	fprintf(stderr,	"date_detect_ran : %d\n",	hybscan_buf.HydroMesg[0]);
	fprintf(stderr,	"time_detect_ran : %d\n",	hybscan_buf.HydroMesg[1]);
	fprintf(stderr,	"date_last_precip : %d\n",	hybscan_buf.HydroMesg[2]);
	fprintf(stderr,	"time_last_precip : %d\n", 	hybscan_buf.HydroMesg[3]);
	fprintf(stderr,	"cur_precip_cat : %d\n",	hybscan_buf.HydroMesg[4]);
	fprintf(stderr,	"last_precip_cat : %d\n",	hybscan_buf.HydroMesg[5]);
	
}/*end saa_Print_Hydro_Message */
