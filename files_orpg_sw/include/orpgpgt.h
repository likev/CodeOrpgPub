/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2005/07/11 15:04:25 $
 * $Id: orpgpgt.h,v 1.9 2005/07/11 15:04:25 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  orpgpgt.h						*
 *		This is the global include file for ORPGPGT.		*
 *		(ORPG Product Generation Table)				*
 *									*
 ************************************************************************/


#ifndef ORPGPGT_H

#define ORPGPGT_H

#include <cs.h>

#include <prod_distri_info.h>
#include <gen_stat_msg.h>
#include "a309.h"

#define	MAX_PRODUCT_GENERATION_TBL_SIZE		8192

#define	ORPGPGT_MAX_PARAMETERS			   6

#define	ORPGPGT_ERROR			      -32768	
#define	ORPGPGT_INVALID_DATA		      -32767

#define	ORPGPGT_MIN_INTERVAL			   0
#define	ORPGPGT_MAX_INTERVAL			  20
#define	ORPGPGT_MIN_GENERATION_INTERVAL		   0
#define	ORPGPGT_MAX_GENERATION_INTERVAL		   1
#define	ORPGPGT_MIN_ARCHIVE_INTERVAL		 -20
#define	ORPGPGT_MAX_ARCHIVE_INTERVAL		  20
#define	ORPGPGT_MIN_STORAGE_INTERVAL		   0
#define	ORPGPGT_MAX_STORAGE_INTERVAL		   1
#define	ORPGPGT_MIN_PERIOD			  30
#define	ORPGPGT_MAX_PERIOD			 360

/*	The following macros define the message IDs of the different	*
 *	product generation tables in ORPGDAT_PROD_INFO.			*/

#define	ORPGPGT_DEFAULT_M_TABLE	    MAINTENANCE_MODE
#define	ORPGPGT_DEFAULT_B_TABLE	      CLEAR_AIR_MODE
#define	ORPGPGT_DEFAULT_A_TABLE	  PRECIPITATION_MODE
#define	ORPGPGT_CURRENT_TABLE	 		   3


/*	Functions dealing with product generation table		*/

void	ORPGPGT_error (void (*user_exception_callback)());

/*	Product Generation Table I/O functions			*/
int	ORPGPGT_clear_tbl    (int table);
int	ORPGPGT_read_tbl     (int table);
int	ORPGPGT_write_tbl    (int table);
int	ORPGPGT_copy_tbl     (int source, int destination);
int	ORPGPGT_replace_tbl  (int source, int destination);

int	ORPGPGT_add_entry      (int id);
int	ORPGPGT_delete_entry   (int id, int indx);

char	*ORPGPGT_get_tbl_ptr (int table, int indx);
int	ORPGPGT_get_tbl_num  (int table);
int	ORPGPGT_get_tbl_size (int table);
int	ORPGPGT_buf_in_tbl   (int table, int buf_num);

int	ORPGPGT_get_prod_id             (int table, int indx);
int	ORPGPGT_get_archive_interval    (int table, int indx);
int	ORPGPGT_get_generation_interval (int table, int indx);
int	ORPGPGT_get_storage_interval    (int table, int indx);
int	ORPGPGT_get_retention_period    (int table, int indx);
int	ORPGPGT_get_parameter           (int table, int indx, int param);
int	ORPGPGT_get_update_flag         ();

int	ORPGPGT_set_prod_id             (int table, int indx, int val);
int	ORPGPGT_set_archive_interval    (int table, int indx, int val);
int	ORPGPGT_set_generation_interval (int table, int indx, int val);
int	ORPGPGT_set_storage_interval    (int table, int indx, int val);
int	ORPGPGT_set_retention_period    (int table, int indx, int val);
int	ORPGPGT_set_parameter           (int table, int indx, int param, int val);

#endif
