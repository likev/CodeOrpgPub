/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/02/01 15:56:46 $
 * $Id: orpgdbm.h,v 1.16 2008/02/01 15:56:46 jing Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  orpgdbm.h						*
 *		This is the global include file for ORPGDBM (ORPG	*
 *		product Data Base Manager functions).			*
 *									*
 ************************************************************************/




#ifndef ORPGDBM_H
#define ORPGDBM_H

#define	ORPGDBM_MAX_LIST_SIZE	1000
#define	ORPGDBM_DONT_NEED_INDEX	   0
#define	ORPGDBM_MAX_WAIT_TIME	   5
#define ORPGDBM_SERVICE_ADDR_ENV_VAR "ORPGDBM_SERVICE_ADDR"

#include <sdq.h>
#include <rpgdbm.h>

/*	The following macro defines a new query fields so the user	*
 *	can use it to set the query mode or a range of volume time 	*/

#define	ORPGDBM_MODE	(RPGP_WAREHOUSED+1)
#define ORPGDBM_VOL_TIME_RANGE (ORPGDBM_MODE+1)
#define ORPGDBM_ELEV_RANGE (ORPGDBM_VOL_TIME_RANGE+1)

/*	The following macros define the bit settings for the query	*
 *	mode field.  For multiple settings "or" the desired macros	*
 *	together.							*/

#define	ORPGDBM_ALL_MATCH	SDQM_ALL_MATCH
#define	ORPGDBM_EXACT_MATCH	SDQM_EXACT_MATCH
#define	ORPGDBM_FULL_SEARCH	SDQM_FULL_SEARCH
#define	ORPGDBM_PARTIAL_SEARCH	SDQM_PARTIAL_SEARCH
#define	ORPGDBM_HIGHEND_SEARCH	SDQM_HIGHEND_SEARCH
#define	ORPGDBM_LOWEND_SEARCH	SDQM_LOWEND_SEARCH
#define	ORPGDBM_DISTINCT_FIELD_VALUES	SDQM_DISTINCT_FIELD_VALUES
#define	ORPGDBM_ALL_FIELD_VALUES	SDQM_ALL_FIELD_VALUES

/*	The following macros define field range limits.		*/

#define	ORPGDBM_MIN_VOLT	0
#define	ORPGDBM_MIN_RETENT	0
#define	ORPGDBM_MIN_PCODE	16
#define	ORPGDBM_MIN_ELEV	-10
#define	ORPGDBM_MIN_WAREHOUSED	0
#define	ORPGDBM_MAX_VOLT	2147483647
#define	ORPGDBM_MAX_RETENT	32767
#define	ORPGDBM_MAX_PCODE	ORPGPAT_MAX_PRODUCT_CODE
#define	ORPGDBM_MAX_ELEV	3600
#define	ORPGDBM_MAX_WAREHOUSED	ORPGPAT_MAX_PRODUCT_CODE

/* product query field enumeration for ORPGDBM_query */
enum {RPGP_RETENT, RPGP_VOLT, RPGP_GENT, RPGP_PCODE, RPGP_ELEV, RPGP_WAREHOUSED};

/*  The following defines a query data element.			*/

typedef struct {

	int	field;	/* name of query field:			*
			 *	RPGP_RETENT - retention		*
			 *	RPGP_VOLT   - volume time	*
			 *      RPGP_VOLT_RANGE - range of volume times *
			 *	RPGP_PCODE  - product code	*
			 *	RPGP_ELEV   - elevation*10	*
			 *	RPGP_WAREHOUSED - warehoused	*
			 *	ORPGDBM_MODE - set the query	*
			 *		       mode		*/
	int	value;	/* value for the paired field.		*/
	int     value2; /* second value to query ranges */

} ORPGDBM_query_data_t;

int	ORPGDBM_io_status();
int	ORPGDBM_read (int prod_id,
		      char **buf,
		      int vol_time,
		      short *params);

int	ORPGDBM_query (RPG_prod_rec_t *query_result,
		       ORPGDBM_query_data_t *query_data,
		       int query_data_len,
		       int max_list_size);

void	ORPGDBM_close_and_reconnect_to_rpgdbm ();
int ORPGDBM_set_server_address (char *s_name, int conn_n, 
					char *buf, int buf_size);

#endif
