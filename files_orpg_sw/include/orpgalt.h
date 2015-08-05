/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/13 19:49:49 $
 * $Id: orpgalt.h,v 1.17 2014/03/13 19:49:49 steves Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */
/****************************************************************
		
    Module: orpgalt.h
				
    Description: This is the header file to front end the 
		 alert threshold section of libORPG.

****************************************************************/


#ifndef ORPGALT_OPTIONS_H
#define ORPGALT_OPTIONS_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Default DEA alerting table filename */
#define ORPGALT_DEA_DFLT_TABLE_FNAME "alert_table.dea"

/* Define True/False for the ORPGALT library */
#define ORPGALT_FALSE 0
#define ORPGALT_TRUE  1

/* Various return status values for alert threshold functions	*/

#define	ORPGALT_SUCCESS			-9999
#define	ORPGALT_READ_FAILED		-9998
#define ORPGALT_WRITE_FAILED		-9997
#define	ORPGALT_INVALID_CATEGORY	-9996
#define	ORPGALT_INVALID_INDEX		-9995
#define	ORPGALT_INVALID_DATA		-9994
#define	ORPGALT_FAILURE                 -9993

/* The following two macros define the lower and upper limit for        *
 * product ID's which will be allowed in the alert threshold            *
 * table.								*/

#define	ORPGALT_PRODUCT_ID_MIN		15
#define	ORPGALT_PRODUCT_ID_MAX		ORPGPAT_MAX_PRODUCT_CODE

/* Specific message macros for alert threshold table data.		*/

#define	ORPGALT_CATEGORY_MSG_ID		  1
#define	ORPGALT_CATEGORY_TBL_SIZE	100
#define	ORPGALT_CATEGORY_LIST_SIZE	700
#define	ORPGALT_MAX_NUM_THRESH            6
#define	ORPGALT_NUM_GROUPS                3

#define	ORPGALT_UNLOCK			  0
#define	ORPGALT_LOCK			  1

#define ORPGALT_BASELINE_CATEGORY_MSG_ID  3

/* Alert Group parameters */
#define ORPGALT_GROUP1_ID   1
#define ORPGALT_GROUP1_NAME "Grid"
#define ORPGALT_GROUP2_ID   2
#define ORPGALT_GROUP2_NAME "Volume"
#define ORPGALT_GROUP3_ID   3
#define ORPGALT_GROUP3_NAME "Forecast"


/* Level of change authority (LOCA) values */
enum
{ ORPGALT_LOCA_NONE = 0, 
  ORPGALT_LOCA_URC,
  ORPGALT_LOCA_AGENCY,
  ORPGALT_LOCA_ROC
};


/* DEA database lock values */
enum
{ ORPGALT_EDIT_LOCKED = 0, 
  ORPGALT_EDIT_UNLOCKED = 1, 
  ORPGALT_LOCK_SUCCESSFUL = 4,
  ORPGALT_LOCK_NOT_SUCCESSFUL = 5,
  ORPGALT_UNLOCK_SUCCESSFUL = 6,
  ORPGALT_UNLOCK_NOT_SUCCESSFUL = 7
};


/*	Prototypes							*/

int 	ORPGALT_io_status       ();
int	ORPGALT_categories      ();
int	ORPGALT_groups          ();
int	ORPGALT_read            ();
int	ORPGALT_write           ();
int	ORPGALT_update_legacy   (char *data);
int     ORPGALT_restore_baseline();
int     ORPGALT_update_baseline ();
int     ORPGALT_init_groups     ();
int	ORPGALT_get_category    (int indx);
int	ORPGALT_get_group       (int cat_id);
int	ORPGALT_get_group_id    (int indx);
int	ORPGALT_get_loca        (int cat_id);
int	ORPGALT_get_min         (int cat_id);
int	ORPGALT_get_max         (int cat_id);
int	ORPGALT_get_type        (int cat_id);
int	ORPGALT_get_prod_code   (int cat_id);
int	ORPGALT_set_prod_code   (int cat_id, int prod_code);
int	ORPGALT_get_thresholds  (int cat_id);
int	ORPGALT_get_threshold   (int cat_id, int thresh_id);
int	ORPGALT_set_threshold   (int cat_id, int thresh_id, int value);
int     ORPGALT_clear_edit_lock ();
int     ORPGALT_set_edit_lock   ();
int     ORPGALT_get_edit_status ();
void	ORPGALT_error           (void (*user_exception_callback)());
char*   ORPGALT_get_name        (int cat_id);
char*   ORPGALT_get_group_name  (int cat_id);
char*   ORPGALT_get_unit        (int cat_id);
int     ORPGALT_get_update_flag ();

#ifdef __cplusplus
}
#endif

#endif

