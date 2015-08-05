
/***********************************************************************

    Description: Internal include file for rpgdbm.

***********************************************************************/

/*
* RCS info
* $Author: jing $
* $Locker:  $
* $Date: 2002/06/03 19:48:25 $
* $Id: rpgdbm_def.h,v 1.7 2002/06/03 19:48:25 jing Exp $
* $Revision: 1.7 $
* $State: Exp $
*/

#ifndef RPGDBM_DEF_H

#define RPGDBM_DEF_H

#define HOUSEKEEPING_TIME 4	/* housekeeping time period in seconds */
#define DELAYED_DEL_PERIOD 60	/* LB msg delayed deletion period - the 
				   returned msg_id in the query results 
				   will be available within this period. */
#define RANGE_BETWEEN_LOW_HIGH 64
				/* range between high and low water level 
				   reserved for avoiding frequent product
				   deletion processing */

int RPGDBM_PROD_init_db (int max_prod_gen_rate, int min_reten_time);
void RPGDBM_PROD_housekeeping ();
void RPGDBM_PROD_save_records ();

int RPGDBM_UP_init_db ();


#endif		/* #ifndef RPGDBM_DEF_H */

