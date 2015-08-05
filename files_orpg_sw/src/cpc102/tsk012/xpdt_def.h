
/****************************************************************
		
    This module contains the internal shared definitions for xpdt.

*****************************************************************/

/*
 * RCS info 
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 19:00:43 $
 * $Id: xpdt_def.h,v 1.4 2014/03/18 19:00:43 jeffs Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 * $Log: xpdt_def.h,v $
 * Revision 1.4  2014/03/18 19:00:43  jeffs
 * Fix return-type warnings
 *
 * Revision 1.3  2005-07-12 14:28:51-05  steves
 * issue 2-745
 *
 * Revision 1.3  2005-07-12 14:26:48-05  steves
 * issue 2-552
 *
 * Revision 1.2  2005/02/10 16:15:15  jing
 * Update
 *
 *
 */

#ifndef XPDT_DEF_H
#define XPDT_DEF_H

int Init_attr_tbl( char *file_name, int clear_table );

int XQ_init (char *dir, char *prod_table);
int XQ_routine ();
int XQ_read_product (unsigned int msg_id, char **data);
int XQ_get_all_vol_times (RPG_prod_rec_t *Db_info, int size);
int XQ_get_all_product_code (RPG_prod_rec_t *Db_info, int size);
int XQ_query_prod_by_code_and_vol_time (int prod_code, time_t vol_time,
					RPG_prod_rec_t *Db_info, int size);
int XQ_read_product_by_name (char *name, char **data);

void display_tabular_data ();
void display_hires_raster_data ();
void* Decompress_product( void *bufptr, int *size );

#endif		/* #ifndef XPDT_DEF_H */


