/*
* RCS info
* $Author: ccalvert $
* $Locker:  $
* $Date: 2008/05/14 16:32:42 $
* $Id: rpgdbm.h,v 1.25 2008/05/14 16:32:42 ccalvert Exp $
* $Revision: 1.25 $
* $State: Exp $
*/

/***********************************************************************

    Description: public include file for the rpgdbm task.

***********************************************************************/


#ifndef RPGDBM_H

#define RPGDBM_H

# include <precip_grid_rec.h>

/***   product data base   ***/
#define RPG_PROD_DB_NAME 	"rpg_prod_db"

typedef struct {           /* record struct for RPG product data base */
    int        msg_id;     /* message ID as stored in the product LB */
    int        reten_t;    /* product retention time, in seconds */
    int        vol_t;      /* product volume time, UNIX time */
    time_t     gen_t;      /* product generation time, Unix time */
    short      prod_code;  /* product code */
    short      elev;       /* elevation angle */
    short      warehoused; /* if non-zero, indicates product is warehoused
                              and warehouse value is the product ID. */
    short      wx_mode;    /* what weather mode the product was generated */
    short      elev_ind;   /* elevation index */
    short      params[6];  /* product parameters */
    short      req_params[6];  /* product parameters as requested */
} RPG_prod_rec_t;	   /* If you add a new field in this struct, be sure 
			      to adjust the offsets in struct qfs in function
			      RPGDBM_PROD_init_db */


/***   user profile data base   ***/
#define RPG_UP_DB_NAME 		"rpg_user_profile_db"

/* query field enumeration */
enum {RPGU_MSG_ID, RPGU_USER_NAME, RPGU_USER_ID, RPGU_UP_TYPE, 
			RPGU_CLASS_NUM, RPGU_LINE_IND, RPGU_DISTRI_METHOD};

typedef struct {           /* record struct for RPG product data base */
    int        msg_id;     /* message ID as stored in the product LB */
    char       *user_name; /* user or class name, Pd_user_entry.user_name */
    short      user_id;    /* Pd_user_entry.user_id. See prod_distri_info.h */
    short      up_type;    /* user profile type. Pd_user_entry.up_type. 
			      See prod_distri_info.h */
    short      class_num;  /* Pd_user_entry.class. See prod_distri_info.h */
    short      line_ind;   /* Pd_user_entry.line_ind. See prod_distri_info.h */
    int        distri_method;	/* Pd_user_entry.distri_method. See 
				   prod_distri_info.h */
} RPG_up_rec_t;	   	   /* If you add a new field in this struct, be sure 
			      to adjust the offsets in struct qfs in function
			      RPGDBM_UP_init_db */


#endif		/* #ifndef RPGDBM_H */
