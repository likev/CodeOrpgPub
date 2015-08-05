/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/02/19 21:34:36 $
 * $Id: orpgdbm.c,v 1.24 2004/02/19 21:34:36 jing Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  orpgdbm.c						*
 *		This module contains a collection of routines to	*
 *		manipulate info in the ORPG products data base.		*
 *									*
 ************************************************************************/





/*	System include file definitions.				*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*	Local include file definitions.					*/

#include <infr.h>
#include <orpg.h>
#include <orpgdbm.h>
#include <rss_replace.h>
#include <rpgdbm.h>
#include <orpgerr.h>

#define	SV_ADDR_SIZE	256
#define ORPGDBM_QUERY_RETRIES_MAX	2

static  int     io_status = 0;  /* Last I/O status */

static void Add_section (char *str, char **vb);


int ORPGDBM_io_status()
{
   return(io_status);
}

/************************************************************************
 *	Description: This function reads a specified product from the	*
 *	ORPG products data base.  This function first queries the	*
 *	ORPG products data base and then tries to match all of the	*
 *	parameters (defined for the product in the Product Attributes	*
 *	Table) in the parameters list (params 1-6).  If a match		*
 *	is found, memory is allocated to hold the product and it	*
 *	is read.  On success, a positive value indicating the product	*
 *	length is returned.  If the product was not found, then 0	*
 *	is returned.  All other values (<0) correspond to normal	*
 *	LB errors (see the header file lb.h for specific error		*
 *	information for a particular negative return value).		*
 *									*
 *	Input:  prod_id	- product ID of product to read			*
 *		vol_time - time of product to read (julian seconds)	*
 *		params - product parameters to match			*
 *	Output: buf - product data					*
 *	Return: <0 - Error; 0 - product not found; >0 size of product	*
 ************************************************************************/

int
ORPGDBM_read (
int	prod_id,
char	**buf,
int	vol_time,
short	*params
)
{
	int	i, j, k;
	int	ret;
	int	match = 0;
	int	prod_code;
	int	msg_id;
	int	old_vol_t;
	int	n;
	ORPGDBM_query_data_t	query_data [16];
	RPG_prod_rec_t	db_info [1000];

/*	First check to see if a valid product id was input.  If not,	*
 *	then return 0.							*/

	if ((prod_id < 0) || (prod_id >= MAX_PAT_TBL_SIZE)) {

	    LE_send_msg (GL_INFO,
		"ORPGDBM_read: invalid product id: %d\n", prod_id);

	    return (0);

	}

/*	Get the product code associated with the product id.  The	*
 *	Products data base holds only products which can be distributed	*
 *	to external users (have an ICD defined code).			*/

	prod_code = ORPGPAT_get_code (prod_id);

	io_status = ORPGPAT_io_status();	
	if (prod_code <= 0) {

	    LE_send_msg (GL_INFO,
		"ORPGDBM_read: product %d has no valid product code: %d\n",
		prod_id, prod_code);

	    return (0);

	}

/*	If the product is elevation based, then we want to match	*
 *	the elevation angle n the parameter data.  Elevation is a	*
 *	query field.							*/

	k = ORPGPAT_elevation_based (prod_id);

	io_status = ORPGPAT_io_status();

/*	Next, query the ORPG products data base so we can get info	*
 *	on what products are available for the specified product id	*
 *	and volume time.						*/

	n = 0;

	query_data [n].field   = ORPGDBM_MODE;
	query_data [n++].value = ORPGDBM_HIGHEND_SEARCH;
	query_data [n].field   = RPGP_PCODE;
	query_data [n++].value = prod_code;

	if (vol_time > 0) {

	    query_data [n].field   = RPGP_VOLT;
	    query_data [n++].value = vol_time;

	}

	if (k >= 0) {

	    if (params [k] != PARAM_ANY_VALUE) {

	    query_data [n].field   = RPGP_ELEV;
	    query_data [n++].value = params [k];

	    }
	}

	ret = ORPGDBM_query (db_info,
			     query_data,
			     n,
			     100);

	io_status = ret;
/*	If any products were found for the specified volume time,	*
 *	check to see if all parameters match.				*/

	if (ret <= 0) {

	    LE_send_msg (GL_INFO,
		"ORPGDBM_read: requested product not found\n");
	    return (0);

	}

/*	Try to match all of the parameters passed in as input with	*
 *	all of the parameters in each product in the data base.  If	*
 *	a match is found, then read the product and return the number	*
 *	of bytes in the product.  If a match is not found, return 0.	*/

	old_vol_t = -1;
	msg_id    = -1;

	for (i=0;i<ret;i++) {

	    match = 1;

/*	    Only check those parameters which are defined for the	*
 *	    product in the Product Attributes Table.			*/

	    for (j=0;j<ORPGPAT_get_num_parameters (prod_id);j++) {

		k = ORPGPAT_get_parameter_index (prod_id, j);

		if (db_info[i].params[k] != params [k]) {

		    if (params[k] != PARAM_ANY_VALUE) {

			match = 0;
			break;

		    }
		}
	    }

/*	    A product was found matching the specified inputs.  Get	*
 *	    the message id from the query record.  If the message id	*
 *	    is valid, read the product and set the return value to	*
 *	    the message length.  Otherwise, set the return value to	*
 *	    0.								*/

	    if (match) {

		if (vol_time == -1) {

		    if (db_info[i].vol_t > old_vol_t) {

			msg_id    = db_info[i].msg_id;
			old_vol_t = db_info[i].vol_t;

		    }

		} else {

		    msg_id = db_info[i].msg_id;
		    break;

		}
	    }
	}
	io_status = ORPGPAT_io_status();

	if (msg_id < 0) {

	    LE_send_msg (GL_INFO,
		"ORPGDBM_read: invalid message id detected: %d\n",
		msg_id);
	    ret = 0;

	} else {

	    ret = ORPGDA_read (ORPGDAT_PRODUCTS,
		 buf,
		 LB_ALLOC_BUF,
		 msg_id);

	    io_status = ret; 
	    if (ret <= 0) {

		LE_send_msg (GL_INFO,
		    "ORPGDBM_read: ORPGDA_read () returned %d\n", ret);
		
	    }
	}

	return (ret);
}

/************************************************************************
 *	Description: This function is used by ORPG tasks to query the	*
 *	ORPG Products Data Base.  The user supplies a variable list	*
 *	of search parameters and in return gets an array of structures	*
 *	containing information about the products which matched the	*
 *	search criteria.						*
 *									*
 *	Input:  data - structure containing search criteria.		*
 *		data_len - number of search parameters in data.		*
 *		max_list_size - max number of matches to return		*
 *	Output: db_info	 - structure containing results of data base	*
 *		query.							*
 *	Return: On success, a non-negative number indicating the number	*
 *		of products in the ORPG Products Data Base which match	*
 *		the search criteria.  If no products are found, then 0	*
 *		is returned.  A negative number is returned when	*
 *		internal ORPG functions fail and the error value for	*
 *		that functional area is returned.			*
 ************************************************************************/

int
ORPGDBM_query (
RPG_prod_rec_t	*db_info,
ORPGDBM_query_data_t	*data,
int	data_len,
int	max_list_size
)
{
	int	ret;
	void	*query_result;
	int	i;
	int	records_found;
	int	records_returned;
	int	mode_flag;
	int	retries;

/*	Lets begin our query.						*/

	retries = 0;

	while (retries < ORPGDBM_QUERY_RETRIES_MAX) {
	    char buf[256], *lb_name;
	    static char *vb = NULL;

	    vb = STR_copy (vb, "");
	    mode_flag = 0;

	    if (data_len == 0) {

	        SDQ_set_query_mode (SDQM_ALL_MATCH);

	    } else {

	        for (i=0;i<data_len;i++) {

		    switch (data[i].field) {
	    
		        case RPGP_VOLT :

			    if (mode_flag) {
				sprintf (buf, "vol_t >= %d and vol_t <= %d", 
					ORPGDBM_MIN_VOLT, ORPGDBM_MAX_VOLT);
			    } else {
				sprintf (buf, "vol_t = %d", data[i].value);
			    }
			    Add_section (buf, &vb);
			    break;

		        case RPGP_RETENT :

			    if (mode_flag) {
				sprintf (buf, 
				    "reten_t >= %d and reten_t <= %d", 
				    ORPGDBM_MIN_RETENT, ORPGDBM_MAX_RETENT);
			    } else {
				sprintf (buf, "reten_t = %d", data[i].value);
			    }
			    Add_section (buf, &vb);
			    break;
	    
		        case RPGP_PCODE :

			    if (mode_flag) {
				sprintf (buf, 
					"prod_code >= %d and prod_code <= %d", 
					ORPGDBM_MIN_PCODE, ORPGDBM_MAX_PCODE);
			    } else {
				sprintf (buf, "prod_code = %d", data[i].value);
			    }
			    Add_section (buf, &vb);
			    break;
	    
		        case RPGP_ELEV :

			    if (mode_flag) {
				sprintf (buf, "elev >= %d and elev <= %d", 
					ORPGDBM_MIN_ELEV, ORPGDBM_MAX_ELEV);
			    } else {
				sprintf (buf, "elev = %d", data[i].value);
			    }
			    Add_section (buf, &vb);
			    break;
	    
		        case RPGP_WAREHOUSED :

			    if (mode_flag) {
				sprintf (buf, 
					"warehoused >= %d && warehoused <= %d",
			ORPGDBM_MIN_WAREHOUSED, ORPGDBM_MAX_WAREHOUSED);
			    } else {
				sprintf (buf, "warehoused = %d", data[i].value);
			    }
			    Add_section (buf, &vb);
			    break;

		        case ORPGDBM_MODE:

			    SDQ_set_query_mode (data[i].value);

			    if (data[i].value & SDQM_DISTINCT_FIELD_VALUES){

			        mode_flag = 1;

			    } else {

			        mode_flag = 0;

			    }

			    break;

		        case ORPGDBM_VOL_TIME_RANGE :
			    sprintf (buf, "vol_t >= %d and vol_t <= %d", 
					data[i].value, data[i].value2);
			    Add_section (buf, &vb);
			    break;

		        case ORPGDBM_ELEV_RANGE :
			    sprintf (buf, "elev >= %d and elev <= %d", 
					data[i].value, data[i].value2);
			    Add_section (buf, &vb);
			    break;

		         default :

			    LE_send_msg (GL_INFO,
				"ORPGDBM_query: invalid query field: %d (ignored)\n",
				data[i].field);
			    break;
		    }
	        }
	    }

/*	    Now that we have defined the query fields, lets execute the	*
 *	    query.							*/

	    lb_name = ORPGDA_lbname (ORPGDAT_PRODUCTS);
	    if (lb_name == NULL) {
		LE_send_msg (GL_INFO,
			"ORPGDBM: Product DB LB name not found\n");
		return (-1);
	    }
	    SDQ_set_maximum_records (max_list_size);
	    ret = SDQ_select (lb_name, vb, (void **)&query_result);

/*	    If the return value is negative, then an error occurred.	*
 *	    Log an error message and return with error.			*/

	    if (ret < 0) {

	        LE_send_msg (GL_INFO,
		    "ORPGDBM: SDQ_select (%s) failed: %d\n", vb, ret);

	        if (query_result != NULL)
		    free (query_result);

		retries++;

	    } else {

		break;

	    }
	}

	if (retries >= ORPGDBM_QUERY_RETRIES_MAX) {

	    return (-1);

	}

/*	Check to see if there were any query errors.  If so, log the	*
 *	error code and return with error.				*/

	ret = SDQ_get_query_error (query_result);

	if (ret < 0) {

	    LE_send_msg (GL_INFO,
		"ORPGDBM: query error: %d\n", ret);

	    if (query_result != NULL)
		free (query_result);

	    return (ret);

	}

/*	At this point it is assumed that the query was a success	*
 *	so now we need to determine the number of records which		*
 *	matched the query criteria.					*/

	records_found    = SDQ_get_n_records_found    (query_result);
	records_returned = SDQ_get_n_records_returned (query_result);

/*	Lets check to see if the number of records returned is not	*
 *	equal to the number of records matching the query criteria.	*
 *	If they are different, log a warning message and continue.	*
 *	NOTE: In this case the user may not find a product which does	*
 *	exist in the data base.  If this happens, then the macro	*
 *	MAX_LIST_SIZE should probably be increased.  Normally, this	*
 *	condition won't happen if enough query filters are specified.	*/

	if (records_found != records_returned) {

	    LE_send_msg (GL_INFO,
		"ORPGDBM: Warning - query matched more records (%d) than returned (%d)\n",
		records_found, records_returned);

	}

/*	Now we need to allocate memory for the query data which are	*
 *	to be returned to the caller.  It is the responsibility of	*
 *	the caller to free this memory after use.			*/

	if (records_returned <= 0) {

	    if (query_result != NULL)
		free (query_result);
	    return (0);

	}

/*	Now we need to copy the query data from the SDQ data to the	*
 *	user query data variable.					*/

	for (i=0;i<records_returned;i++) {

	    RPG_prod_rec_t	*rec;
	    int	j;

	    if (SDQ_get_query_record (query_result, i, (void **) &rec)) {

		db_info [i].msg_id     = rec->msg_id;
		db_info [i].reten_t    = rec->reten_t;
		db_info [i].vol_t      = rec->vol_t;
		db_info [i].prod_code  = rec->prod_code;
		db_info [i].elev       = rec->elev;
		db_info [i].warehoused = rec->warehoused;

		for (j=0;j<6;j++) {

		    db_info [i].params [j] = rec->params [j];
		    db_info [i].req_params [j] = rec->req_params [j];

		}

/*	    If an error occurs in the extraction, free data and return	*
 *	    no data.							*/

	    } else {

		LE_send_msg (GL_INFO,
			"ORPGDBM: SDQ_get_query_record (%d) failed\n", i);

		if (query_result != NULL)
		    free (query_result);
		return (0);

	    }
	}


	if (query_result != NULL)
	    free (query_result);
	return (records_returned);
}

/**************************************************************************

    Appends SQL section of "str" to "vb".

**************************************************************************/

static void Add_section (char *str, char **vb) {

    if (*vb == NULL || **vb == '\0')
	*vb = STR_copy (*vb, str);
    else {
	*vb = STR_cat (*vb, " and ");
	*vb = STR_cat (*vb, str);
    }
}
