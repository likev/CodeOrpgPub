/* 
 * RCS info 
 * $Author: nolitam $ 
 * $Locker:  $ 
 * $Date: 2002/12/11 21:31:45 $ 
 * $Id: orpggst.h,v 1.6 2002/12/11 21:31:45 nolitam Exp $ 
 * $Revision: 1.6 $ 
 * $State: Exp $ 
 */ 
/******************************************************************

	file: orpggst.h

	This is the header for Get Status library file.
	
******************************************************************/
#ifndef ORPGGST_H
#define ORPGGST_H

#include <infr.h>
#include <orpg.h>

#define RPG_OPERATIONAL_STATUS	1
#define RPG_STATE		2
#define	RPG_ALARMS		3

/*orpggst_rpg_info*/
unsigned int ORPGGST_get_rpg_status(int request);

/*orpggst_rda_info*/
RDA_status_t  *ORPGGST_get_rda_stats();
int ORPGGST_get_wb_alarm(int wb_num);

/* orpggst_product_info.c */
Pd_distri_info *ORPGGST_get_prod_distribution_info();
int ORPGGST_save_prod_distribution_info(Pd_distri_info *p_tbl, int len);

#endif
