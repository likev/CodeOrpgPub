/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/10 20:23:44 $
 * $Id: global2.h,v 1.19 2012/01/10 20:23:44 ccalvert Exp $
 * $Revision: 1.19 $
 * $State: Exp $
 */

/* global2.h */
/* global definitions and includes for both the foreground */
/* task and the background task and those items that are   */
/* routinely changed each version                          */

#ifndef _GLOBAL2_H_
#define _GLOBAL2_H_


#define CVG_VERSION_STRING "CODEview Graphics 9.2"
#define VERSION_DATE_STRING "January, 2012"
#define CVG_SHORT_VER "CVG 9.2"
#define CVG_PREF_DIR_NAME "cvg9.2"


/* we define as a current build number even though the    */
/* cvg logic only significantly differs for build 6 and earlier */
/* The most recent option menu selection is currently Build 8+  */
#define CVG_DEFAULT_BUILD 8



/*  reduced from 152 in CVG 6.5, affected modules are: */
/*  process list_item() in cvg_read_db.c and */
/*  filter_prod_list() in prod_select.c */
/*  load_product_names() in product_names.c */
typedef char db_entry_string[112];
#define PROD_DESC_MAX 70


/*  from product_names.h */
/*  ORPG Build 8 and earlier: */
/*$ORPGDIR/pdist/prod_info.lb is used for product info*/
/* #define PROD_ATTR_MSG_ID 3 */
/* ORPG Build 9 and later: */
/*$ORPGDIR/mngrpg/pat.lbis used for product info*/
/*PROD_ATTR_MSG_ID  is defined in orpgpat.h*/


/* CODE 9.2: increased from 16000 */
#define DEFAULT_DB_SIZE 18000


#endif

