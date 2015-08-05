/************************************************************************
 *									*
 *	Module:  orpgpat.c						*
 *		This module contains a collection of routines to	*
 *		access and modify the product attribute table.		*
 *									*
 ************************************************************************/



/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/03/02 22:00:54 $
 * $Id: orpgpat.c,v 1.39 2011/03/02 22:00:54 steves Exp $
 * $Revision: 1.39 $
 * $State: Exp $
 */

/*	System include files					*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <orpg.h>
#include <infr.h>
#include <mrpg.h>
#include <rss_replace.h>

static	int	Attr_tbl_size = 0;	/*  Size (bytes) of prod attr message	*/
static	int	Attr_tbl_num  = 0;	/*  Number of prod attr entries.	*/
static	int	Attr_tbl_ptr [MAX_PAT_TBL_SIZE];
				/*  Lookup table to each entry in the	*
				 *  product attributes table.		*/

static char	*Attr_tbl;
				/*  Pointer to product attributes table	*/

static	char	Mne [MAX_MNE_LENGTH];

static int Need_update = 1;		/* attribute table re-read needed */
static void (*User_exception_callback)() = NULL;	/* users exception call back function */

static short Id_to_index   [MAX_PAT_TBL_SIZE*2];
					/* prod_id to index table */
static short Code_to_index [MAX_PAT_TBL_SIZE*2];
					/* product code to index table */

static void Process_exception ();

static int io_status = 0;

/************************************************************************
 *									*
 *  Description:  Returns io status of last I/O operation

 ************************************************************************/
int ORPGPAT_io_status()
{
    return(io_status);
}


/************************************************************************
 *									*
 *  Description: This function registers a call back function, which 	*
 *		will be called when an exception condition is 		*
 *		encountered.						*
 *									*
 *  Input:	user_exception_callback - the user's callback function.	*
 *									*
 ************************************************************************/

void ORPGPAT_error (void (*user_exception_callback)())
{

    User_exception_callback = user_exception_callback;
    return;
}

/************************************************************************
 *									*
 *  Description: This function is called when an exception condition	*
 *		condition is detected. 					*
 *									*
 *  Input:	 None							*
 *									*
 ************************************************************************/

static void Process_exception ()
{

    if (User_exception_callback == NULL) {
	LE_send_msg (GL_INFO, "ORPGPAT process exception");
    }
    else
	User_exception_callback ();

    return;
}

/************************************************************************
 *  Description:  The following module clears the product attributes	*
 *  		  buffer and sets the number of products to 0. 		*
 *									*
 *  Return:	  If the table was previously initialized then 1 is	*
 *		  retuned.  Otherwise, 0 is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_clear_tbl (
)
{
	int	ret;
	int	i;

	Need_update = 0;

	for (i = 0; i < MAX_PAT_TBL_SIZE*2; i++)
	    Id_to_index [i] = ORPGPAT_DATA_NOT_FOUND;

	for (i = 0; i < MAX_PAT_TBL_SIZE*2; i++)
	    Code_to_index [i] = ORPGPAT_DATA_NOT_FOUND;

	if (Attr_tbl != (char *) NULL) {

	    free (Attr_tbl);
	    Attr_tbl = (char *) NULL;
	    Attr_tbl_size = 0;
	    Attr_tbl_num  = 0;

	    ret = 1;

	} else {

	    ret = 0;

	}

	return ret;
}

/**************************************************************************
   Description: 
      This function parses the ASCII PAT and initializes the binary PAT.

   Input:
      file_name - File name of the ASCII PAT.

   Output: 
      NONE

   Return:
      It returns the number of entries of the table on success
      or -1 on failure.

**************************************************************************/
int ORPGPAT_read_ASCII_PAT( char *file_name ){

    int p_cnt, total_size, err;
    prod_id_t	prod_id;
    prod_id_t   aliased_prod_id;
    prod_id_t   class_id;
    unsigned int class_mask;
    char	gen_task[ORPG_TASKNAME_SIZ];
    short	wx_modes;
    char	disabled;
    char	n_priority;
    char	n_dep_prods;
    char	n_opt_prods;
    short	prod_code;
    short	type;
    short	elev_index;
    short	alert;
    char        compression;
    char        format_type;
    int         warehoused;
    int         warehouse_id;
    int         warehouse_acct_id;
    int		max_size;
    char	name [PROD_NAME_LEN];
    char	desc [128];
    short	priority;
    short	dep_prod;
    short	opt_prod;
    short	param_index;
    short	param_min  ;
    short	param_max  ;
    short	param_default  ;
    int		param_scale;
    char	param_name [PARAMETER_NAME_LEN];
    char	param_units[PARAMETER_UNITS_LEN];
    int		indx;
    int		ret;

    p_cnt = total_size = err = 0;

    CS_cfg_name ( file_name );
    CS_control (CS_COMMENT | '#');
    CS_control (CS_RESET);

/*  Repeat for all the product definitions in the product_attributes	*
 *  configuration file.							*/

    do {
	int len;
	int cnt;

	if (CS_level (CS_DOWN_LEVEL) < 0)
	    continue;

	if (CS_entry ("prod_id", 1 | CS_SHORT, 0, 
					(char *)&prod_id) <= 0 ||
	    CS_entry ("prod_code", 1 | CS_SHORT, 0, 
					(char *)&prod_code) <= 0 ||
	    CS_entry ("wx_modes", 1 | CS_SHORT, 0, 
					(char *)&wx_modes) <= 0 ||
	    CS_entry ("disabled", 1 | CS_BYTE, 0, 
					(char *)&disabled) <= 0 ||
	    CS_entry ("n_priority", 1 | CS_BYTE, 0, 
					(char *)&n_priority) <= 0 ||
	    CS_entry ("n_dep_prods", 1 | CS_BYTE, 0, 
					(char *)&n_dep_prods) <= 0 ||
	    CS_entry ("alert", 1 | CS_SHORT, 0, 
					(char *)&alert) <= 0 ||
	    CS_entry ("warehoused", 1 | CS_INT, 0, 
					(char *)&warehoused) <= 0 ||
	    CS_entry ("type", 1 | CS_SHORT, 0, 
					(char *)&type) <= 0 ||
	    CS_entry ("max_size", 1 | CS_INT, 0, 
					(char *)&max_size) <= 0) {
	    err = 1;
	    break;
	}

/*	A new definition was read in so create a new table entry.  In	*
 *	the event the product is already defined, remove the definition	*
 *	to make room for the new definition.				*/

	indx = ORPGPAT_delete_prod (prod_id);
	if( indx >= 0 ){

	    LE_send_msg( GL_INFO, "Duplicate PAT Entry for Prod ID %d\n",
                         prod_id );
	    LE_send_msg( GL_INFO, "--->Replacing Original With New Entry.\n" );

	}

	indx = ORPGPAT_add_prod (prod_id);

	if (indx == ORPGPAT_ERROR) {

	    CS_report ("ERROR: duplicate product found");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_alert (prod_id, alert);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set alert field in PAT");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_warehoused (prod_id, warehoused);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set warehoused field in PAT");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_code (prod_id, prod_code);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set prod_code field in PAT");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_type (prod_id, type);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set type field in PAT");
	    err = 1;
	    break;

	}

	if( CS_entry ("gen_task", 1, ORPG_TASKNAME_SIZ, (char *)&gen_task) <= 0 )
	    ret = ORPGPAT_set_gen_task (prod_id, "");

        else
	    ret = ORPGPAT_set_gen_task (prod_id, gen_task);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set gen_task field in PAT");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_wx_modes (prod_id, wx_modes);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set wx_modes field in PAT");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_disabled (prod_id, disabled);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set disabled field in PAT");
	    err = 1;
	    break;

	}

	CS_control (CS_KEY_OPTIONAL);
	if (CS_entry ("elev_index", 1 | CS_SHORT, 0, 
					(char *)&elev_index) <= 0)
	    elev_index = -1;

	ret = ORPGPAT_set_elevation_index (prod_id, elev_index);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set elev_index field in PAT");
	    err = 1;
	    break;

	}

        if (CS_entry ("compression", 1 | CS_BYTE, 0,
                                         (char *)&compression) <  0)
           compression = 0;
  
        ret = ORPGPAT_set_compression_type (prod_id, compression);
  
        if (ret == ORPGPAT_ERROR) {
 
           CS_report ("ERROR: unable to set compression type field in PAT");
           err = 1;
           break;
 
        }

        if (CS_entry ("n_opt_prods", 1 | CS_BYTE, 0,
                                         (char *)&n_opt_prods) <  0)
           n_opt_prods = 0;
  
        if (CS_entry ("format_type", 1 | CS_BYTE, 0,
                                           (char *)&format_type) <  0)
           format_type = 0;

        ret = ORPGPAT_set_format_type (prod_id, format_type);
 
        if (ret == ORPGPAT_ERROR) {
  
           CS_report ("ERROR: unable to set format type field in PAT");
           err = 1;
           break;
  
        }

	CS_control (CS_KEY_REQUIRED);

	if (n_priority > 0) {	/* read the priority list */

	    cnt = 0;
	    while (cnt < n_priority &&
		   CS_entry ("priority_list", (cnt + 1) | CS_SHORT, 0,
					(char *) &priority) > 0) {

		ORPGPAT_add_priority (prod_id, priority);

		cnt++;
	    }

	    if (cnt != n_priority) {
		CS_report ("bad priority list");
		err = 1;
		break;
	    }
	}

	if (n_dep_prods > 0) {	/* read the dep prods list */

	    cnt = 0;
	    while (cnt < n_dep_prods &&
		   CS_entry ("dep_prods_list", (cnt + 1) | CS_SHORT, 0,
					(char *) &dep_prod) > 0) {

		ORPGPAT_add_dep_prod (prod_id, dep_prod);

		cnt++;
	    }

	    if (cnt != n_dep_prods) {
		CS_report ("bad dep prods list");
		err = 1;
		break;
	    }
	}

	if (n_opt_prods > 0) {	/* read the opt prods list */

	    cnt = 0;
	    while (cnt < n_opt_prods &&
		   CS_entry ("opt_prods_list", (cnt + 1) | CS_SHORT, 0,
					(char *) &opt_prod) > 0) {

		ORPGPAT_add_opt_prod (prod_id, opt_prod);

		cnt++;
	    }

	    if (cnt != n_opt_prods) {
		CS_report ("bad opt prods list");
		err = 1;
		break;
	    }
	}

	if ((len = 
	    CS_entry ("desc", 1, 256, (char *)desc)) < 0) {
	    err = 1;
	    break;
	}

	ORPGPAT_set_description (prod_id, desc);

	if ((len = 
	    CS_entry ("prod_id", 2, PROD_NAME_LEN, (char *)name)) < 0) {
	    err = 1;
	    break;
	}

	ORPGPAT_set_name (prod_id, name);

	/* read parameter list */

	CS_control (CS_KEY_OPTIONAL);

	if (CS_entry ("params", 1, 0, (char *)NULL) >= 0) {
	    while (CS_entry (CS_NEXT_LINE, 0, 0, NULL) >= 0) {
		if (CS_entry (CS_THIS_LINE, 0 | CS_SHORT, 0,
				(char *)&param_index) <= 0 ||
		    CS_entry (CS_THIS_LINE, 1 | CS_SHORT, 0,
				(char *)&param_min) <= 0 ||
		    CS_entry (CS_THIS_LINE, 2 | CS_SHORT, 0,
				(char *)&param_max) <= 0 ||
		    CS_entry (CS_THIS_LINE, 3 | CS_SHORT, 0,
				(char *)&param_default) <= 0 ||
		    CS_entry (CS_THIS_LINE, 4 | CS_INT, 0,
				(char *)&param_scale) <= 0 ||
		    CS_entry (CS_THIS_LINE, 5, PARAMETER_NAME_LEN,
				(char *)param_name) <= 0 ||
		    CS_entry (CS_THIS_LINE, 6, PARAMETER_UNITS_LEN,
				(char *)param_units) <= 0)
		    break;

		indx = ORPGPAT_add_parameter (prod_id);

		if (indx < 0) {

		    CS_report ("ERROR adding parameter to PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_index (prod_id, indx, param_index);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter index in PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_min   (prod_id, indx, param_min);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter min in PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_max   (prod_id, indx, param_max);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter max in PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_default (prod_id, indx, param_default);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter default in PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_scale (prod_id, indx, param_scale);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter scale in PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_name (prod_id, indx, param_name);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter name in PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_units (prod_id, indx, param_units);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter units in PAT");
		    err = 1;
		    break;

		}
	    }
	}

	if (CS_entry ("aliased_prod_id", 1 | CS_SHORT, 0, 
					(char *)&aliased_prod_id) <= 0 )
           aliased_prod_id = -1;

	ret = ORPGPAT_set_aliased_prod_id (prod_id, aliased_prod_id);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set aliased_prod_id field in PAT");
	    err = 1;
	    break;

	}

	if (CS_entry ("class_id", 1 | CS_SHORT, 0, 
					(char *)&class_id) <= 0 )
           class_id = prod_id;

	ret = ORPGPAT_set_class_id (prod_id, class_id);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set class_id field in PAT");
	    err = 1;
	    break;

	}

	if (CS_entry ("class_mask", 1 | CS_UINT, 0, 
					(char *)&class_mask) <= 0 )
           class_mask = 0;

	ret = ORPGPAT_set_class_mask (prod_id, class_mask);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set class_mask field in PAT");
	    err = 1;
	    break;

	}

	if (CS_entry ("warehouse_id", 1 | CS_INT, 0, 
					(char *)&warehouse_id) <= 0 )
           warehouse_id = 0;

	ret = ORPGPAT_set_warehouse_id (prod_id, warehouse_id);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set warehouse ID field in PAT");
	    err = 1;
	    break;

	}

        if( warehouse_id > 0 ){

 	   CS_control (CS_KEY_REQUIRED);

	   if (CS_entry ("warehouse_acct_id", 1 | CS_INT, 0, 
	  				(char *)&warehouse_acct_id) > 0 ){

		ret = ORPGPAT_set_warehouse_acct_id (prod_id, warehouse_acct_id);

		if (ret == ORPGPAT_ERROR) {

	    	    CS_report ("ERROR: unable to set warehouse acct ID field in PAT");
	    	    err = 1;
	    	    break;

		}

	    }

	}
           
 	CS_control (CS_KEY_REQUIRED);

	p_cnt++;

	CS_level (CS_UP_LEVEL);

    } while (CS_entry (CS_NEXT_LINE, 0, 0, NULL) >= 0);

    CS_cfg_name ("");
    
    /* If an error was detected, do nothing and return.*/
    if( err )
	return (-1);

    else
       return (p_cnt);

/* End of ORPGPAT_read_ASCII_PAT() */
}

/************************************************************************
 *  Description:  The following module reads the product attributes	*
 *  		  table from the product attributes lb.  The size of	*
 *		  the message containing the data is determined first 	*
 *  		  and memory is dynamically allocated to store the	*
 *  		  message in memory.  A pointer is kept to the start of	*
 *		  the data.						*
 *									*
 *  Return:	  On success 0 is returned, otherwise ORPGPAT_ERROR	*
 *		  is retuned.						*
 *									*
 ************************************************************************/

int
ORPGPAT_read_tbl (
)
{
	int	status;
	Pd_attr_entry	*pd_attr;
	int	offset;
	int i;

	Attr_tbl_num  = 0;
	Attr_tbl_size = 0;

/*	Make a pass through the product attributes message and create	*
 *	a lookup table to the start of each attribute definition.  We	*
 *	maintain lookup tables referenced by product code and id.	*/

/*	First initialize each lookup table entry to -1.			*/

	for (i = 0; i < MAX_PAT_TBL_SIZE*2; i++)
	    Id_to_index [i] = ORPGPAT_DATA_NOT_FOUND;

	for (i = 0; i < MAX_PAT_TBL_SIZE*2; i++)
	    Code_to_index [i] = ORPGPAT_DATA_NOT_FOUND;

/*	Check to see if the table has already been initialized.  If	*
 *	so, free up the memory associated with it.			*/

	if (Attr_tbl != (char *) NULL) {

	   free (Attr_tbl);

	}

/*	Now read the product attributes table into the buffer just	* 
 *	created.							*/

	status = ORPGDA_read( ORPGDAT_PAT, &Attr_tbl, LB_ALLOC_BUF,
		          PROD_ATTR_MSG_ID );
	io_status = status;
	if (status <= 0) {

	    LE_send_msg (GL_INPUT,
			"ORPGPAT: ORPGDA_read PD_PROD_ATTR_MSG_ID failed (ret %d)\n",
			status);
	    Process_exception ();
	    return (ORPGPAT_ERROR);

	}

	Attr_tbl_size = status;

	offset = 0;

	while (offset < Attr_tbl_size) {

	    short code, id;

	    if (Attr_tbl_num >= MAX_PAT_TBL_SIZE) {
		LE_send_msg (GL_INPUT,
			"ORPGPAT: too many products (%d)\n", Attr_tbl_num);
	        Process_exception ();
		return (ORPGPAT_ERROR);
	    }

/*	Attr_tbl_ptr contains the offset, in bytes, to the start of	*
 *	 each attributes table record.					*/

	    Attr_tbl_ptr [Attr_tbl_num] = offset;

/*	Cast the current buffer pointer to the main attributes table	*
 *	structure.  The size of the record can be determined from the	*
 *	entry_size element in the main structure.   The start of the	*
 *	next record is the curren offset plus the current record size.	*/


	    pd_attr = (Pd_attr_entry *) (Attr_tbl+offset);

	    offset = offset + pd_attr->entry_size;

/*	Update the reference record by product id lookup table		*/

	    id = pd_attr->prod_id;

	    if (abs(id) < MAX_PAT_TBL_SIZE) {

		if (Id_to_index[id+MAX_PAT_TBL_SIZE] < 0)
		    Id_to_index[id+MAX_PAT_TBL_SIZE] = Attr_tbl_num;
		else {
	            LE_send_msg (GL_INPUT,
			    "ORPGPAT: duplicated prod_id (%d) in prod attr table\n", 
			    id);
		}
	    }

/*	Update the reference record by product code lookup table	*
 *	Remember that negative product codes are allowed and have 	*
 *	special meaning (i.e., -91, -92). That is why thelookup table 	*
 *	is normalized.							*/

	    code = pd_attr->prod_code;

	    if ((code < MAX_PAT_TBL_SIZE) && (code >= -MAX_PAT_TBL_SIZE)) {

		if (Code_to_index[code+MAX_PAT_TBL_SIZE] < 0)
		    Code_to_index[code+MAX_PAT_TBL_SIZE] = Attr_tbl_num;
		else if (code != 0) {	/* non-zero duplicated pcode */
	            LE_send_msg (GL_INPUT,
		            "ORPGPAT: duplicated prod_code (%d) in prod attr table\n", 
			    code);
		}
	    }

	    Attr_tbl_num++;

	}

	Need_update = 0;
	return 0;
}

/************************************************************************
 *									*
 *  Description:  This function returns the state of the data update	*
 *		  flag (0 = no update; 1 = update)			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_update_flag ()
{
	return Need_update;
}

/************************************************************************
 *									*
 *  Description:  This sets the state of the data update flag		*
 *									*
 ************************************************************************/

void
ORPGPAT_set_update_flag ()
{
	Need_update = 1;
}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to an attributes	*
 *		  table record.						*
 *									*
 *  Inputs:	  ndx - index (record number) of the table entry.	*
 *									*
 *  Return:	  A NULL pointer on failure or a pointer to the		*
 *		  specified record on success.				*
 *									*
 ************************************************************************/

char
*ORPGPAT_get_tbl_ptr (
int	ndx
)
{
	char	*ret;

	if (Need_update)
	    ORPGPAT_read_tbl ();
	if (Need_update)
	    return (NULL);

	ret = (char *) NULL;

	if ((Attr_tbl != (char *) NULL) &&
	    (ndx >= 0) &&
	    (ndx < Attr_tbl_num)) {
	    ret = (char *) (Attr_tbl + Attr_tbl_ptr [ndx]);

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function looks for a specified product id	*
 *		  in the product atributes table.			*
 *									*
 *  Inputs:	  buf - the product id (buffer number) to search for.	*
 *									*
 *  Return:	  This function returns the index (record number)	*
 *		  of the record containing the specified product	*
 *		  id on success.  If buf is larger than table size,	*
 *		  ORPGPAT_ERROR is returned.  If buf is not in table,	*
 *		  ORPGPAT_DATA_NOT_FOUND is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_prod_in_tbl (
int	buf
)
{
	int	ret;

	if (Need_update)
	    ORPGPAT_read_tbl ();
	if (Need_update)
	    return (ORPGPAT_ERROR);

	ret = ORPGPAT_ERROR;

	if (abs(buf) < MAX_PAT_TBL_SIZE) {

	    ret = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to record in the	*
 *		  product attributes table conaining a specified 	*
 *		  product id.						*
 *									*
 *  Inputs:	  buf - The product id (buffer number) to find.		*
 *									*
 *  Return:	  A pointer to the record containing the buffer number	*
 *		  on success, a NULL pointer on failure.		*
 *									*
 ************************************************************************/

Pd_attr_entry
*ORPGPAT_get_tbl_entry (
int	buf
)
{
	Pd_attr_entry	*ptr;

	if (Need_update)
	    ORPGPAT_read_tbl ();
	if (Need_update)
	    return (NULL);

/*	If the input product id is outside the allowed range of ids,	*
 *	return a NULL pointer.						*/

	if (abs(buf) > MAX_PAT_TBL_SIZE) {

	    return (NULL);

	}

/*	If the input product id is not defined in the attributes table,	*
 *	return a NULL pointer.						*/

	if (Id_to_index [buf+MAX_PAT_TBL_SIZE] < 0) {

	    return (NULL);

	}

/*	The input product id is defined so return a pointer to the	*
 *	main structure for the record containing the input product id.	*/

	ptr = (Pd_attr_entry *) 
		&Attr_tbl [Attr_tbl_ptr [Id_to_index [buf+MAX_PAT_TBL_SIZE]]];

	return ptr;

}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to a Mrpg_data_t	*
 *		  data structure corresponding to a specified product	*
 *		  id.							*
 *									*
 *  Inputs:	  buf - The product id (buffer number) to find.		*
 *									*
 *  Outputs:      size - size of the Mrpg_data_t structure.		*
 *									*
 *  Return:	  A pointer to the record containing the buffer number	*
 *		  on success, a NULL pointer on failure.		*
 *									*
 ************************************************************************/

Mrpg_data_t* ORPGPAT_get_data_table_entry( int buf, int *size ){

	Mrpg_data_t *data = NULL;
	Pd_attr_entry *ptr = NULL;

/*	Initialize size to 0						*/
        *size = 0;

	if (Need_update)
	    ORPGPAT_read_tbl ();

	if (Need_update)
	    return (NULL);

/*	If the input product id is outside the allowed range of ids,	*
 *	return a NULL pointer.						*/
	if (abs(buf) > MAX_PAT_TBL_SIZE)
	    return (NULL);

/*	If the input product id is not defined in the attributes table,	*
 *	return a NULL pointer.						*/
	if (Id_to_index [buf+MAX_PAT_TBL_SIZE] < 0)
	    return (NULL);

/*	The input product id is defined so return data for this product
 *	id.  Currently, write permission is not implemented.		*/
	ptr = (Pd_attr_entry *) 
		&Attr_tbl [Attr_tbl_ptr [Id_to_index [buf+MAX_PAT_TBL_SIZE]]];

/*	If this is a product with product code, treat this differently. *
 *	Always return NULL in this case. 				*/
        if( ptr->prod_code != 0 )
           return(NULL);

/*	malloc block for Mrpg_data_t structure.				*/
	data = (Mrpg_data_t *) calloc( 1, sizeof(Mrpg_data_t) );
	if( data == NULL )
	    return(NULL);

        data->size = sizeof(Mrpg_data_t);
        *size = data->size;
        data->data_id = ptr->prod_id;

        /* Product compression is handled by the RPG/RPGC library. */
        data->compr_code = COMPRESSION_NONE;

        data->wp_size = 0;
        memset( data->wp, 0, sizeof(Mrpg_wp_item) );

	return( data );

}

/************************************************************************
 *									*
 *  Description:  This function checks if a specified product is	*
 *		  elevation based.					*
 *									*
 *  Input:	  buf - input product id (buffer number).		*
 *									*
 *  Return:	  If the product is elevation based, a positive value	*
 *		  is returned which represents the parameter index	*
 *		  of the elevation parameter.  Otherwise, -1 if not	*
 *		  elevation based or ORPGPAT_ERROR is returned.		*
 *									*
 ************************************************************************/

int
ORPGPAT_elevation_based (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    if (attr_tbl->type == TYPE_ELEVATION) {

		ret = attr_tbl->elev_index;

	    } else {

		ret = -1;

	    }
	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function returns the product id associated	*
 *		  with a specified product code				*
 *									*
 *  Input:	  code - product code.					*
 *									*
 *  Return:	  On success, the product_id (>= 0) is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_prod_id_from_code (
int	code
)
{
	Pd_attr_entry	*attr_tbl;
	int	indx;

	if (Need_update)
	    ORPGPAT_read_tbl ();
	if (Need_update)
	    return (0);

/*	If the input product code is not allowed, return with error.	*/

	if ((code < -MAX_PAT_TBL_SIZE) || (code >= MAX_PAT_TBL_SIZE)) {

	    return ORPGPAT_ERROR;

	}

/*	If the product code does not exist in the PAT, then the LUT	*
 *	value is <0 so return error.					*/

	indx = Code_to_index [code+MAX_PAT_TBL_SIZE];

	if (indx < 0) {

	    return ORPGPAT_ERROR;

	}

	attr_tbl = (Pd_attr_entry *) ORPGPAT_get_tbl_ptr (indx);

	return attr_tbl->prod_id;

}

/************************************************************************
 *									*
 *  Description:  This function returns the legacy product code		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  On success, the legacy product code is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_code (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->prod_code;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function modifies the legacy product code	*
 *		  element of a product attributes table entry.		*
 *									*
 *  Input:	  buf  - product id (buffer number) of table entry.	*
 *		  code - new code					*
 *									*
 *  Return:	  On success, the legacy product code is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_code (
int	buf,
int	code
)
{
	Pd_attr_entry	*attr_tbl;
	int	indx;

	if ((code < -MAX_PAT_TBL_SIZE) || (code > MAX_PAT_TBL_SIZE)) {

	    return (ORPGPAT_ERROR);

	}

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {

	    attr_tbl->prod_code = code;

	} else {

	    return (ORPGPAT_ERROR);

	}

	indx = ORPGPAT_prod_in_tbl (buf);
	Code_to_index [code+MAX_PAT_TBL_SIZE] = indx;

	return code;

}

/************************************************************************
 *									*
 *  Description:  This function returns the weather modes		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  On success, the weather mode bitmap is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_wx_modes (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->wx_modes;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function modifies the weather mode		*
 *		  element of a product attributes table entry.		*
 *									*
 *  Input:	  buf     - product id (buffer number) of table entry.	*
 *		  wx_mode - The new weather modes			*
 *									*
 *  Return:	  On success, the weather mode is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_wx_modes (
int	buf,
int	wx_mode
)
{
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {

	    attr_tbl->wx_modes = wx_mode;

	} else {

	    wx_mode = ORPGPAT_ERROR;

	}

	return wx_mode;

}

/************************************************************************
 *									*
 *  Description:  This function returns the product type		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  On success, the product type is returned (refer to	*
 *		  the description in "prod_distri_info.h for details	*
 *		  on the valid types).					*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_type (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->type;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function modifies the product type		*
 *		  element of a product attributes table entry.		*
 *									*
 *  Input:	  buf  - product id (buffer number) of table entry.	*
 *		  type - New product type				*
 *									*
 *  Return:	  On success, the product type is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_type (
int	buf,
int	type
)
{
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {

	    attr_tbl->type = type;

	} else {

	    type = ORPGPAT_ERROR;

	}

	return type;

}

/************************************************************************
 *									*
 *  Description:  This function returns the number of entries in	*
 *		  the product attributes table.				*
 *									*
 *  Input:	  NONE							*
 *									*
 *  Return:	  The number of items in the product attributes		*
 *		  table.						*
 *									*
 ************************************************************************/

int
ORPGPAT_num_tbl_items (
)
{
	if (Need_update)
	    ORPGPAT_read_tbl ();
	return Attr_tbl_num;
}

/************************************************************************
 *									*
 *  Description:  This function returns the aliased prod id		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  aliased prod id on success.				*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_aliased_prod_id (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->aliased_prod_id;

	}

	return ret;
}
/************************************************************************
 *                                                                      *
 *  Description:  This function returns the prod class id               *
 *                associated with a specified ORPG product.             *
 *                                                                      *
 *  Input:        buf - product id (buffer number).                     *
 *                                                                      *
 *  Return:       prod class id on success.                             *
 *                On failure ORPGPAT_ERROR is returned.                 *
 *                                                                      *
 ************************************************************************/

int
ORPGPAT_get_class_id (
int     buf
)
{
        Pd_attr_entry   *attr_tbl;
        int     ret;

        attr_tbl = ORPGPAT_get_tbl_entry (buf);
            
        ret = ORPGPAT_ERROR;
            
        if (attr_tbl != NULL) {
        
            ret = attr_tbl->class_id;
        
        }
            
        return ret;
}
       
/************************************************************************
 *                                                                      *
 *  Description:  This function returns the prod class mask             *
 *                associated with a specified ORPG product.             *
 *                                                                      *
 *  Input:        buf - product id (buffer number).                     *
 *                                                                      *
 *  Return:       prod class mask on success.                           *
 *                On failure 0 is returned.                             *
 *                                                                      *
 ************************************************************************/

unsigned int
ORPGPAT_get_class_mask (
int     buf
)
{
        Pd_attr_entry   *attr_tbl;
        unsigned int     ret;

        attr_tbl = ORPGPAT_get_tbl_entry (buf);
            
        ret = 0;
            
        if (attr_tbl != NULL) {
        
            ret = attr_tbl->class_mask;
        
        }
            
        return (0xffff & ret);
}       

/************************************************************************
 *									*
 *  Description:  This function returns the alert type			*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  0 - The product cannot be paired with an alert.	*
 *		  1 - The product can be paired with an alert.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_alert (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->alert;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function modifies the product aliased prod id	*
 *		  element of a product attributes table entry.		*
 *									*
 *  Input:	  buf   - product id (buffer number) of table entry.	*
 *		  aliased_prod_id - new aliased prod id value.		*
 *									*
 *  Return:	  On success, the aliased prod id is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_aliased_prod_id (
int	buf,
int	aliased_prod_id
)
{
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {

	    attr_tbl->aliased_prod_id = aliased_prod_id;

	} else {

	    aliased_prod_id = ORPGPAT_ERROR;

	}

	return aliased_prod_id;

}

/************************************************************************
 *                                                                      *
 *  Description:  This function modifies the product class id           *
 *                element of a product attributes table entry.          *
 *                                                                      *
 *  Input:        buf   - product id (buffer number) of table entry.    *
 *                class_id - new class id value.                        *
 *                                                                      *
 *  Return:       On success, the class id is returned.                 *
 *                On failure ORPGPAT_ERROR is returned.                 *
 *                                                                      *
 ************************************************************************/

int
ORPGPAT_set_class_id (
int     buf,
int     class_id
)
{
        Pd_attr_entry   *attr_tbl;

        attr_tbl = ORPGPAT_get_tbl_entry (buf);

        if (attr_tbl != NULL) {

            attr_tbl->class_id = class_id;

        } else {

            class_id = ORPGPAT_ERROR;

        }

        return class_id;

}
/************************************************************************
 *                                                                      *
 *  Description:  This function modifies the product class mask         *
 *                element of a product attributes table entry.          *
 *                                                                      *
 *  Input:        buf   - product id (buffer number) of table entry.    *
 *                class_mask - new class mask value.                    *
 *                                                                      *
 *  Return:       On success, the class mask is returned.               *
 *                On failure 0 is returned.                             *
 *                                                                      *
 ************************************************************************/

unsigned int
ORPGPAT_set_class_mask (
int     buf,
unsigned int class_mask
)
{
        Pd_attr_entry   *attr_tbl;

        attr_tbl = ORPGPAT_get_tbl_entry (buf);

        if (attr_tbl != NULL) {

            attr_tbl->class_mask = (unsigned short) ((0xffff) & class_mask);

        } else {

            class_mask = 0;

        }

        return class_mask;

}


/************************************************************************
 *									*
 *  Description:  This function modifies the product code alert		*
 *		  element of a product attributes table entry.		*
 *									*
 *  Input:	  buf   - product id (buffer number) of table entry.	*
 *		  alert - new alert value				*
 *									*
 *  Return:	  On success, the product alert code is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_alert (
int	buf,
int	alert
)
{
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {

	    attr_tbl->alert = alert;

	} else {

	    alert = ORPGPAT_ERROR;

	}

	return alert;

}

/************************************************************************
 *									*
 *  Description:  This function returns the number of seconds 		*
 *		  the specified ORPG product is warehoused.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  0 - The product is not warehoused.			*
 *		  >0 - The product is warehoused for this many seconds.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_warehoused (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->warehoused;

	}

	return ret;
}

/************************************************************************
 *                                                                      *
 *  Description:  This function returns the data ID where the data      *
 *                is warehoused.     	                                *
 *                                                                      *
 *  Input:        buf - product id (buffer number).                     *
 *                                                                      *
 *  Return:       0 - The product is not warehoused or no warehouse ID  *
 *                    is specified.                                     *
 *                >0 - The product warehouse data ID.                   *
 *                On failure ORPGPAT_ERROR is returned.                 *
 *                                                                      *
 ************************************************************************/

int
ORPGPAT_get_warehouse_id (
int     buf
)
{
        Pd_attr_entry   *attr_tbl;
        int     ret;

        attr_tbl = ORPGPAT_get_tbl_entry (buf);

        ret = ORPGPAT_ERROR;
        
        if (attr_tbl != NULL) {
        
            ret = attr_tbl->warehouse_id;

        }

        return ret;
}

/************************************************************************
 *                                                                      *
 *  Description:  This function returns the data ID where the data      *
 *                warehouse accounting data resides.                    *
 *                                                                      *
 *  Input:        buf - product id (buffer number).                     *
 *                                                                      *
 *  Return:       0 - The product is not warehoused or no warehouse ID  *
 *                    is specified.                                     *
 *                >0 - The product warehouse accounting data data ID.   *
 *                On failure ORPGPAT_ERROR is returned.                 *
 *                                                                      *
 ************************************************************************/

int
ORPGPAT_get_warehouse_acct_id (
int     buf
)
{
        Pd_attr_entry   *attr_tbl;
        int     ret;

        attr_tbl = ORPGPAT_get_tbl_entry (buf);

        ret = ORPGPAT_ERROR;

        if (attr_tbl != NULL) {

            ret = attr_tbl->warehouse_acct_id;

        }

        return ret;
}

/************************************************************************
 *									*
 *  Description:  This function modifies the product warehoused		*
 *		  element of a product attributes table entry.		*
 *									*
 *  Input:	  buf   - product id (buffer number) of table entry.	*
 *		  warehoused_time - new warehoused value		*
 *									*
 *  Return:	  On success, the product warehoused value is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_warehoused (
int	buf,
int	warehoused_time
)
{
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {

	    attr_tbl->warehoused = warehoused_time;

	} else {

	    warehoused_time = ORPGPAT_ERROR;

	}

	return warehoused_time;

}

/************************************************************************
 *                                                                      *
 *  Description:  This function modifies the product warehouse id       *
 *                element of a product attributes table entry.          *
 *                                                                      *
 *  Input:        buf   - product id (buffer number) of table entry.    *
 *                warehouse_id - new warehouse ID value                 *
 *                                                                      *
 *  Return:       On success, the product warehoused ID is returned.    *
 *                On failure ORPGPAT_ERROR is returned.                 *
 *                                                                      *
 ************************************************************************/

int
ORPGPAT_set_warehouse_id (
int     buf,
int     warehouse_id
)
{
        Pd_attr_entry   *attr_tbl;

        attr_tbl = ORPGPAT_get_tbl_entry (buf);

        if (attr_tbl != NULL) {

            attr_tbl->warehouse_id = warehouse_id;

        } else {

            warehouse_id = ORPGPAT_ERROR;

        }

        return warehouse_id;

}

/************************************************************************
 *                                                                      *
 *  Description:  This function modifies the product warehouse acct id  *
 *                element of a product attributes table entry.          *
 *                                                                      *
 *  Input:        buf   - product id (buffer number) of table entry.    *
 *                warehouse_acct_id - new warehouse acct ID value       *
 *                                                                      *
 *  Return:       On success, the product warehoused acct ID is         *
 *                returned.  On failure ORPGPAT_ERROR is returned.      *
 *                                                                      *
 ************************************************************************/

int
ORPGPAT_set_warehouse_acct_id (
int     buf,
int     warehouse_acct_id
)
{
        Pd_attr_entry   *attr_tbl;

        attr_tbl = ORPGPAT_get_tbl_entry (buf);

        if (attr_tbl != NULL) {

            attr_tbl->warehouse_acct_id = warehouse_acct_id;

        } else {

            warehouse_acct_id = ORPGPAT_ERROR;

        }

        return warehouse_acct_id;

}


/************************************************************************
 *									*
 *  Description:  This function returns the maximum size 		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  An integer >= 0 is returned on success.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_max_size (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->max_size;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function modifies the maximum size		*
 *		  element of a product attributes table entry.		*
 *									*
 *  Input:	  buf   - product id (buffer number) of table entry.	*
 *		  size  - new maximum size value			*
 *									*
 *  Return:	  On success, the maximum size value is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_max_size (
int	buf,
int	size
)
{
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {

	    attr_tbl->max_size = size;

	} else {

	    size = ORPGPAT_ERROR;

	}

	return size;

}

/************************************************************************
 *									*
 *  Description:  This function returns the number of parameters	*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  On success, the number of parameters are returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_num_parameters (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->n_params;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to the description	*
 *		  string associated with an ORPG product.		*
 *									*
 *  Input:	  buf    - product id (buffer number).			*
 *		  option - Option to return string pointer with leading	*
 *		  	   mnemonic (if one exists) or without.		*
 *			   STRIP_NOTHING  - with mnemonic		*
 *			   STRIP_MNEMONIC - without mnemonic		*
 *									*
 *  Return:	  On success, a pointer to the description string is	*
 *		  returned.						*
 *		  On failure NULL is returned.				*
 *									*
 ************************************************************************/

char
*ORPGPAT_get_description (
int	buf,
int	option
)
{
	char	*ret;
	int	offset;
	int	indx;
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = (char *) NULL;

	if (attr_tbl != NULL) {

	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    if (attr_tbl->desc > 0) {

		ret = (char *) &Attr_tbl [Attr_tbl_ptr [indx] + attr_tbl->desc];

		if (option == STRIP_MNEMONIC) {

/*	    Since the product mnemonic is at the beginning of the	*
 *	    description, move the pointer to the first character after	*
 *	    the first space character.					*/

		    offset = 0;

		    while (strncmp ((ret + offset)," ",1)) {

			offset++;

		    }

		    offset++;
		    ret = ret + offset;

		}
	    }
	}

	return ret;

}

/************************************************************************
 *  Description:  This function creates a new parameter element		*
 *		  in a product attributes record.  The following	*
 *		  order is assumed in a product attributes record:	*
 *									*
 *				<Main structure>			*
 *				<Parameter data>			*
 *				<Dep prods List>			*
 *				<Opt prods List>			*
 *				<Priority List>				*
 *				<Description string>			*
 *									*
 *		  Each piece is aligned on a word boundary.		*
 *									*
 *  Input:	  buf - product id (buffer number) of table entry.	*
 *									*
 *  Return:	  On success, the parameter index of the new		*
 *		  parameter element is returned.			*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_add_parameter (
int	buf
)
{
	int		i;
	int		size;
	int		psize;
	Pd_attr_entry	*attr_tbl;
	int		indx;
	char		*ptr;

/*	Get the pointer to the record.  Return an error if a problem	*
 *	exists getting the pointer.					*/

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl == NULL) {

	    return ORPGPAT_ERROR;

	}

	indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

/*	Determine the size of a parameter entry and align on word	*
 *	boundary.							*/

	size = sizeof (Pd_params_entry);

	if (size%4)
	    size = size + (4-size%4);

/*	Determine the size of the main structure.  We need it for 	*
 *	positioning.							*/

	psize = sizeof (Pd_attr_entry);

	if (psize%4)
	    psize = psize + (4-psize%4);

/*	Increase the size of the resident table by the new entry size.	*/

	ptr = (char *) realloc (Attr_tbl, (size_t) (Attr_tbl_size + size));

	if (ptr == (char *) NULL) {

	    LE_send_msg (GL_INFO,
		"ORPGPAT_add_parameter: Unable to allocate memory\n");

	}

	Attr_tbl = ptr;

/*	Refresh the pointer to the record since realloc may have moved	*
 *	it somewhere else.  Return an error if a problem exists getting	*
 *	the pointer.							*/

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl == NULL) {

	    return ORPGPAT_ERROR;

	}

/*	If this is the first parameter, set the offset field to start	*
 *	immediately after the main structure.				*/

	if (attr_tbl->params == 0) {

	    attr_tbl->params = psize;

	}

/*	Get the old record size and figure out a new size.		*/

	attr_tbl->entry_size = attr_tbl->entry_size + size;

/*	Shift all trailing data in the table by the size of the new	*
 *	parameter entry.						*/

/*	First move all trailing records.				*/

	for (i=Attr_tbl_size-1;
	     i>=Attr_tbl_ptr [indx] +
		attr_tbl->params +
		size*attr_tbl->n_params;
	     i--) {

	    Attr_tbl [i+size] = Attr_tbl [i];

	}

/*	Update the pointer to the trailing dep prods list.	*/

	if (attr_tbl->dep_prods_list) {

	    attr_tbl->dep_prods_list = attr_tbl->dep_prods_list + size;

	}

/*	Update the pointer to the trailing opt prods list.	*/

	if (attr_tbl->opt_prods_list) {

	    attr_tbl->opt_prods_list = attr_tbl->opt_prods_list + size;

	}

/*	Update the pointer to the trailing priority list.		*/

	if (attr_tbl->priority_list) {

	    attr_tbl->priority_list = attr_tbl->priority_list + size;

	}

/*	Update the pointer to the trailing description.		*/

	if (attr_tbl->desc) {

	    attr_tbl->desc = attr_tbl->desc + size;

	}

/*	Update pointer lookup table				*/

	for (i=indx+1;i<Attr_tbl_num;i++) {

	    Attr_tbl_ptr [i] = Attr_tbl_ptr [i] + size;

	}

	attr_tbl->n_params++;

	Attr_tbl_size = Attr_tbl_size + size;
	return ((int) attr_tbl->n_params-1);
}

/************************************************************************
 *  Description:  This function deletes a parameter element in		*
 *		  a product attributes record.  The following		*
 *		  order is assumed in a product attributes record:	*
 *									*
 *				<Main structure>			*
 *				<Parameter data>			*
 *				<Dep prods List>			*
 *				<Opt prods List>			*
 *				<Priority List>				*
 *				<Description string>			*
 *									*
 *  Input:	  buf - product id (buffer number) of table entry.	*
 *		  pindx    - parameter index				*
 *									*
 *  Return:	  On success, the parameter index of the old		*
 *		  parameter element is returned.			*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_delete_parameter (
int	buf,
int	pindx
)
{
	int		size;
	int		psize;
	int		i;
	int		indx;
	Pd_attr_entry	*attr_tbl;
	char		*ptr;

/*	Get the pointer to the record.  Return an error if a problem	*
 *	exists getting the pointer.					*/

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl == NULL) {

	    return ORPGPAT_ERROR;

	}

	indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

/*	If the record to be deleted does not exist, return an error.	*/

	if ((pindx < 0) ||
	    (pindx >= attr_tbl->n_params)) {

	    return ORPGPAT_ERROR;

	}

	attr_tbl->n_params--;

/*	Allign parameter list on word boundary				*/

	size = sizeof (Pd_params_entry);

	if (size%4)
	    size = size + (4-size%4);

	psize = sizeof (Pd_attr_entry);

	if (psize%4)
	    psize = psize + (4-psize%4);

/*	Move all trailing data.					*/

	Attr_tbl_size = Attr_tbl_size - size;

	for (i=Attr_tbl_ptr [indx] + psize + pindx*size;
	     i<Attr_tbl_size;
	     i++) {

	    Attr_tbl [i] = Attr_tbl [i+size];

	}

	ptr = (char *) realloc (Attr_tbl, (size_t) Attr_tbl_size);

	if (ptr == (char *) NULL) {

	    LE_send_msg (GL_INFO,
		"ORPGPAT_delete_parameter: Unable to allocate memory\n");

	}

	Attr_tbl = ptr;

/*	Refresh the pointer to the record since realloc may have moved	*
 *	it somewhere else.  Return an error if a problem exists getting	*
 *	the pointer.							*/

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl == NULL) {

	    return ORPGPAT_ERROR;

	}

/*	Update the pointer to the trailing dep prods list.	*/

	if (attr_tbl->dep_prods_list) {

	    attr_tbl->dep_prods_list = attr_tbl->dep_prods_list - size;

	}

/*	Update the pointer to the trailing opt prods list.	*/

	if (attr_tbl->opt_prods_list) {

	    attr_tbl->opt_prods_list = attr_tbl->opt_prods_list - size;

	}

/*	Update the pointer to the trailing priority list.		*/

	if (attr_tbl->priority_list) {

	    attr_tbl->priority_list = attr_tbl->priority_list - size;

	}

/*	Update the pointer to the trailing description.		*/

	if (attr_tbl->desc) {

	    attr_tbl->desc = attr_tbl->desc - size;

	}

/*	Update the record entry size element.			*/

	attr_tbl->entry_size = attr_tbl->entry_size - size;

/*	Update pointer lookup table					*/

	for (i=indx+1;i<Attr_tbl_num;i++) {

	    Attr_tbl_ptr [i] = Attr_tbl_ptr [i] - size;

	}

	if (attr_tbl->n_params == 0) {

	    attr_tbl->params = 0;

	}

	return attr_tbl->n_params;
}

/************************************************************************
 *  Description:  This function creates a prod list element in		*
 *		  a product attributes record.  The following		*
 *		  order is assumed in a product attributes record:	*
 *									*
 *				<Main structure>			*
 *				<Parameter data>			*
 *				<Dep prods List>			*
 *				<Opt prods List>			*
 *				<Priority List>				*
 *				<Description string>			*
 *									*
 *  Input:	  buf - product id (buffer number) of table entry.	*
 *		  prod_id - product id of new list item.		*
 *									*
 *  Return:	  On success, the index of the new dep prod list	*
 *		  element is returned.					*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_add_dep_prod (
int	buf,
int	prod_id
)
{
	int	params_size;
	int	psize;
	int	i;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	short	*list;
	char	*ptr;

/*	Get the pointer to the record.  Return an error if a problem	*
 *	exists getting the pointer.					*/

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl == NULL) {

	    return ORPGPAT_ERROR;

	}

	psize = sizeof (Pd_attr_entry);

	if (psize%4) 
		psize = psize + (4-psize%4);

	indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

/*	If the previous dependent product list was padded for alignment	*
 *	we don't have to increase the size of the table for the new	*
 *	list item.  Otherwise, we need to increase the table by a word	*
 *	and shift trailing data.					*/

	attr_tbl->n_dep_prods++;

	if (attr_tbl->n_dep_prods%2) {

/*	    We first need to grow the table by one word.		*/

	    Attr_tbl_size = Attr_tbl_size + 4;

	    ptr = (char *) realloc (Attr_tbl, (size_t) Attr_tbl_size);

	    if (ptr == (char *) NULL) {

		LE_send_msg (GL_INFO,
			"ORPGPAT_add_dep_prod: Unable to allocate memory\n");

	    }

	    Attr_tbl = ptr;

/*	Refresh the pointer to the record since realloc may have moved	*
 *	it somewhere else.  Return an error if a problem exists getting	*
 *	the pointer.							*/

	    attr_tbl = ORPGPAT_get_tbl_entry (buf);

	    if (attr_tbl == NULL) {

		return ORPGPAT_ERROR;

	    }

/*	    If this is the first item in the list we must define the	*
 *	    pointer to it in the main structure.			*/

	    if (attr_tbl->dep_prods_list == 0) {

		params_size = sizeof (Pd_params_entry);

		if (params_size%4) 
			params_size = params_size + (4-params_size%4);

		attr_tbl->dep_prods_list = psize +
					   attr_tbl->n_params*params_size;

	    }

/*	    Now shift all trailing data by 4 bytes.			*/

	    for (i=Attr_tbl_size-1;
		 i>Attr_tbl_ptr [indx] + attr_tbl->dep_prods_list +
					 attr_tbl->n_dep_prods*sizeof(short);
		 i--) {

		 Attr_tbl [i] = Attr_tbl [i-4];

	    }

/*	    Update the pointer to the trailing optional input list.		*/

	    if (attr_tbl->opt_prods_list) {

		attr_tbl->opt_prods_list = attr_tbl->opt_prods_list + 4;

	    }

/*	    Update the pointer to the trailing priority list.		*/

	    if (attr_tbl->priority_list) {

		attr_tbl->priority_list = attr_tbl->priority_list + 4;

	    }

/*	    Update the pointer to the trailing description.		*/

	    if (attr_tbl->desc) {

		attr_tbl->desc = attr_tbl->desc + 4;

	    }

/*	    Update the record entry size element.			*/

	    attr_tbl->entry_size = attr_tbl->entry_size+ 4;

/*	    Update pointer lookup table					*/

	    for (i=indx+1;i<Attr_tbl_num;i++) {

		Attr_tbl_ptr [i] = Attr_tbl_ptr [i] + 4;

	    }
	}

	list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
			  attr_tbl->dep_prods_list];
	list [attr_tbl->n_dep_prods-1] = (short) prod_id;

	return attr_tbl->n_dep_prods-1;
}

/************************************************************************
 *  Description:  This function deletes a prod list element in		*
 *		  a product attributes record.  The following		*
 *		  order is assumed in a product attributes record:	*
 *									*
 *				<Main structure>			*
 *				<Parameter data>			*
 *				<Dep prods List>			*
 *				<Opt prods List>			*
 *				<Priority List>				*
 *				<Description string>			*
 *									*
 *  Input:	  buf - product id (buffer number) of table entry.	*
 *		  pindx    - dependent product list element		*
 *									*
 *  Return:	  On success, the index of the old dep prod list	*
 *		  element is returned.					*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_delete_dep_prod (
int	buf,
int	pindx
)
{
	int	i;
	short	*list;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	char	*ptr;

/*	Get the pointer to the record.  Return an error if a problem	*
 *	exists getting the pointer.					*/

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl == NULL) {

	    return ORPGPAT_ERROR;

	}

	indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

/*	If the index is not valid then return an error.			*/

	if ((pindx < 0) ||
	    (pindx >= attr_tbl->n_dep_prods)) {

	    return ORPGPAT_ERROR;

	}

	attr_tbl->n_dep_prods--;

	list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
			  attr_tbl->dep_prods_list];

	for (i=pindx;i<attr_tbl->n_dep_prods;i++) {

	    list [i] = list [i+1];

	}

/*	If the old list was padded for aligment purposes, then		*
 *	we can shrink the table by one word.  In this case, move	*
 *	trailing list items up one short and then move trailing		*
 *	records up one word and resize.  Otherwise, move up list	*
 *	items only (old last list element will now be padding).		*/

	if (attr_tbl->n_dep_prods%2) {

	    Attr_tbl_size = Attr_tbl_size - 4;

	    for (i=Attr_tbl_ptr [indx]+attr_tbl->dep_prods_list + pindx*2;
		 i<Attr_tbl_size;
		 i++) {

		 Attr_tbl [i] = Attr_tbl [i+4];

	    }

/*	    Since we have moved everything now shrink the table.	*/

	    ptr = (char *) realloc (Attr_tbl, (size_t) Attr_tbl_size);

	    if (ptr == (char *) NULL) {

		LE_send_msg (GL_INFO,
			"ORPGPAT_delete_dep_prod: Unable to allocate memory\n");

	    }

	    Attr_tbl = ptr;

/*	Refresh the pointer to the record since realloc may have moved	*
 *	it somewhere else.  Return an error if a problem exists getting	*
 *	the pointer.							*/

	    attr_tbl = ORPGPAT_get_tbl_entry (buf);

	    if (attr_tbl == NULL) {

		return ORPGPAT_ERROR;

	    }

/*	    Update the pointer to the trailing optional products list.		*/

	    if (attr_tbl->opt_prods_list) {

		attr_tbl->opt_prods_list = attr_tbl->opt_prods_list - 4;

	    }

/*	    Update the pointer to the trailing priority list.		*/

	    if (attr_tbl->priority_list) {

		attr_tbl->priority_list = attr_tbl->priority_list - 4;

	    }

/*	    Update the pointer to the trailing description.		*/

	    if (attr_tbl->desc) {

		attr_tbl->desc = attr_tbl->desc - 4;

	    }

/*	    Update the record entry size element.			*/

	    attr_tbl->entry_size = attr_tbl->entry_size - 4;

/*	    Update pointer lookup table					*/

	    for (i=indx+1;i<Attr_tbl_num;i++) {

		Attr_tbl_ptr [i] = Attr_tbl_ptr [i] - 4;

	    }
	}

	if (attr_tbl->n_dep_prods == 0) {

	    attr_tbl->dep_prods_list = 0;

	}

	return pindx;
}

/************************************************************************
 *  Description:  This function creates an opt prod list element in	*
 *		  a product attributes record.  The following		*
 *		  order is assumed in a product attributes record:	*
 *									*
 *				<Main structure>			*
 *				<Parameter data>			*
 *				<Dep prods List>			*
 *				<Opt prods List>			*
 *				<Priority List>				*
 *				<Description string>			*
 *									*
 *  Input:	  buf - product id (buffer number) of table entry.	*
 *		  prod_id - product id of new list item.		*
 *									*
 *  Return:	  On success, the index of the new dep prod list	*
 *		  element is returned.					*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_add_opt_prod (
int	buf,
int	prod_id
)
{
	int	params_size;
	int	psize;
	int	i;
	int	indx;
        int     dep_prods_size;
	Pd_attr_entry	*attr_tbl;
	short	*list;
	char	*ptr;

/*	Get the pointer to the record.  Return an error if a problem	*
 *	exists getting the pointer.					*/

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl == NULL) {

	    return ORPGPAT_ERROR;

	}

	psize = sizeof (Pd_attr_entry);

	if (psize%4) 
		psize = psize + (4-psize%4);

	indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

/*	If the previous optional product list was padded for alignment	*
 *	we don't have to increase the size of the table for the new	*
 *	list item.  Otherwise, we need to increase the table by a word	*
 *	and shift trailing data.					*/

	attr_tbl->n_opt_prods++;

	if (attr_tbl->n_opt_prods%2) {

/*	    We first need to grow the table by one word.		*/

	    Attr_tbl_size = Attr_tbl_size + 4;

	    ptr = (char *) realloc (Attr_tbl, (size_t) Attr_tbl_size);

	    if (ptr == (char *) NULL) {

		LE_send_msg (GL_INFO,
			"ORPGPAT_add_opt_prod: Unable to allocate memory\n");

	    }

	    Attr_tbl = ptr;

/*	Refresh the pointer to the record since realloc may have moved	*
 *	it somewhere else.  Return an error if a problem exists getting	*
 *	the pointer.							*/

	    attr_tbl = ORPGPAT_get_tbl_entry (buf);

	    if (attr_tbl == NULL) {

		return ORPGPAT_ERROR;

	    }

/*	    If this is the first item in the list we must define the	*
 *	    pointer to it in the main structure.			*/

	    if (attr_tbl->opt_prods_list == 0) {

		params_size = sizeof (Pd_params_entry);

		if (params_size%4) 
			params_size = params_size + (4-params_size%4);

		dep_prods_size = 2*attr_tbl->n_dep_prods;

	        if (dep_prods_size%4) 
			dep_prods_size = dep_prods_size + (4-dep_prods_size%4);

		attr_tbl->opt_prods_list = psize +
					   attr_tbl->n_params*params_size +
                                           dep_prods_size;

	    }

/*	    Now shift all trailing data by 4 bytes.			*/

	    for (i=Attr_tbl_size-1;
		 i>Attr_tbl_ptr [indx] + attr_tbl->opt_prods_list +
					 attr_tbl->n_opt_prods*sizeof(short);
		 i--) {

		 Attr_tbl [i] = Attr_tbl [i-4];

	    }

/*	    Update the pointer to the trailing priority list.		*/

	    if (attr_tbl->priority_list) {

		attr_tbl->priority_list = attr_tbl->priority_list + 4;

	    }

/*	    Update the pointer to the trailing description.		*/

	    if (attr_tbl->desc) {

		attr_tbl->desc = attr_tbl->desc + 4;

	    }

/*	    Update the record entry size element.			*/

	    attr_tbl->entry_size = attr_tbl->entry_size+ 4;

/*	    Update pointer lookup table					*/

	    for (i=indx+1;i<Attr_tbl_num;i++) {

		Attr_tbl_ptr [i] = Attr_tbl_ptr [i] + 4;

	    }
	}

	list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
			  attr_tbl->opt_prods_list];
	list [attr_tbl->n_opt_prods-1] = (short) prod_id;

	return attr_tbl->n_opt_prods-1;
}

/************************************************************************
 *  Description:  This function deletes an opt prod list element in	*
 *		  a product attributes record.  The following		*
 *		  order is assumed in a product attributes record:	*
 *									*
 *				<Main structure>			*
 *				<Parameter data>			*
 *				<Dep prods List>			*
 *				<Opt prods List>			*
 *				<Priority List>				*
 *				<Description string>			*
 *									*
 *  Input:	  buf - product id (buffer number) of table entry.	*
 *		  pindx    - dependent product list element		*
 *									*
 *  Return:	  On success, the index of the old dep prod list	*
 *		  element is returned.					*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_delete_opt_prod (
int	buf,
int	pindx
)
{
	int	i;
	short	*list;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	char	*ptr;

/*	Get the pointer to the record.  Return an error if a problem	*
 *	exists getting the pointer.					*/

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl == NULL) {

	    return ORPGPAT_ERROR;

	}

	indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

/*	If the index is not valid then return an error.			*/

	if ((pindx < 0) ||
	    (pindx >= attr_tbl->n_opt_prods)) {

	    return ORPGPAT_ERROR;

	}

	attr_tbl->n_opt_prods--;

	list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
			  attr_tbl->opt_prods_list];

	for (i=pindx;i<attr_tbl->n_opt_prods;i++) {

	    list [i] = list [i+1];

	}

/*	If the old list was padded for aligment purposes, then		*
 *	we can shrink the table by one word.  In this case, move	*
 *	trailing list items up one short and then move trailing		*
 *	records up one word and resize.  Otherwise, move up list	*
 *	items only (old last list element will now be padding).		*/

	if (attr_tbl->n_opt_prods%2) {

	    Attr_tbl_size = Attr_tbl_size - 4;

	    for (i=Attr_tbl_ptr [indx]+attr_tbl->opt_prods_list + pindx*2;
		 i<Attr_tbl_size;
		 i++) {

		 Attr_tbl [i] = Attr_tbl [i+4];

	    }

/*	    Since we have moved everything now shrink the table.	*/

	    ptr = (char *) realloc (Attr_tbl, (size_t) Attr_tbl_size);

	    if (ptr == (char *) NULL) {

		LE_send_msg (GL_INFO,
			"ORPGPAT_delete_opt_prod: Unable to allocate memory\n");

	    }

	    Attr_tbl = ptr;

/*	Refresh the pointer to the record since realloc may have moved	*
 *	it somewhere else.  Return an error if a problem exists getting	*
 *	the pointer.							*/

	    attr_tbl = ORPGPAT_get_tbl_entry (buf);

	    if (attr_tbl == NULL) {

		return ORPGPAT_ERROR;

	    }

/*	    Update the pointer to the trailing priority list.		*/

	    if (attr_tbl->priority_list) {

		attr_tbl->priority_list = attr_tbl->priority_list - 4;

	    }

/*	    Update the pointer to the trailing description.		*/

	    if (attr_tbl->desc) {

		attr_tbl->desc = attr_tbl->desc - 4;

	    }

/*	    Update the record entry size element.			*/

	    attr_tbl->entry_size = attr_tbl->entry_size - 4;

/*	    Update pointer lookup table					*/

	    for (i=indx+1;i<Attr_tbl_num;i++) {

		Attr_tbl_ptr [i] = Attr_tbl_ptr [i] - 4;

	    }
	}

	if (attr_tbl->n_opt_prods == 0) {

	    attr_tbl->opt_prods_list = 0;

	}

	return pindx;
}
/************************************************************************
 *  Description:  This function creates a priority list element in	*
 *		  a product attributes record.  The following		*
 *		  order is assumed in a product attributes record:	*
 *									*
 *				<Main structure>			*
 *				<Parameter data>			*
 *				<Dep prods List>			*
 *				<Opt prods List>			*
 *				<Priority List>				*
 *				<Description string>			*
 *									*
 *  Input:	  buf - product id (buffer number) of table entry.	*
 *		  prod_id - product id of new list item.		*
 *									*
 *  Return:	  On success, the index of the new priority list	*
 *		  element is returned.					*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_add_priority (
int	buf,
int	priority
)
{
	int	params_size;
	int	psize;
	int	i;
	int	indx;
	int	dep_prods_size;
	int	opt_prods_size;
	Pd_attr_entry	*attr_tbl;
	short	*list;
	char	*ptr;

/*	Get the pointer to the record.  Return an error if a problem	*
 *	exists getting the pointer.					*/

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl == NULL) {

	    return ORPGPAT_ERROR;

	}

	psize = sizeof (Pd_attr_entry);

	if (psize%4) 
		psize = psize + (4-psize%4);

	indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

/*	If the previous product prioritylist was padded for alignment	*
 *	we don't have to increase the size of the table for the new	*
 *	list item.  Otherwise, we need to increase the table by a word	*
 *	and shift trailing data.					*/

	attr_tbl->n_priority++;

	if (attr_tbl->n_priority%2) {

/*	    We first need to grow the table by one word.		*/

	    Attr_tbl_size = Attr_tbl_size + 4;

	    ptr = (char *) realloc (Attr_tbl, (size_t) Attr_tbl_size);

	    if (ptr == (char *) NULL) {

		LE_send_msg (GL_INFO,
			"ORPGPAT_add_priority: Unable to allocate memory\n");

	    }

	    Attr_tbl = ptr;

/*	Refresh the pointer to the record since realloc may have moved	*
 *	it somewhere else.  Return an error if a problem exists getting	*
 *	the pointer.							*/

	    attr_tbl = ORPGPAT_get_tbl_entry (buf);

	    if (attr_tbl == NULL) {

		return ORPGPAT_ERROR;

	    }

/*	    If this is the first item in the list we must define the	*
 *	    pointer to it in the main structure.			*/

	    if (attr_tbl->priority_list == 0) {

		params_size = sizeof (Pd_params_entry);

		if (params_size%4) 
			params_size = params_size + (4-params_size%4);

		dep_prods_size = 2*attr_tbl->n_dep_prods;

		if (dep_prods_size%4) 
			dep_prods_size = dep_prods_size + (4-dep_prods_size%4);

		opt_prods_size = 2*attr_tbl->n_opt_prods;

		if (opt_prods_size%4) 
			opt_prods_size = opt_prods_size + (4-opt_prods_size%4);

		attr_tbl->priority_list = psize +
				 attr_tbl->n_params*params_size +
				 dep_prods_size + 
                                 opt_prods_size;

	    }

/*	    Now shift all trailing data by 4 bytes.			*/

	    for (i=Attr_tbl_size-1;
		 i>Attr_tbl_ptr [indx] + attr_tbl->priority_list +
					 attr_tbl->n_priority*sizeof(short);
		 i--) {

		 Attr_tbl [i] = Attr_tbl [i-4];

	    }

/*	    Update the pointer to the trailing description.		*/

	    if (attr_tbl->desc) {

		attr_tbl->desc = attr_tbl->desc + 4;

	    }

/*	    Update the record entry size element.			*/

	    attr_tbl->entry_size = attr_tbl->entry_size+ 4;

/*	    Update pointer lookup table					*/

	    for (i=indx+1;i<Attr_tbl_num;i++) {

		Attr_tbl_ptr [i] = Attr_tbl_ptr [i] + 4;

	    }
	}

	list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
			  attr_tbl->priority_list];
	list [attr_tbl->n_priority-1] = (short) priority;

	return attr_tbl->n_priority-1;
}

/************************************************************************
 *  Description:  This function deletes a priority list element in	*
 *		  a product attributes record.  The following		*
 *		  order is assumed in a product attributes record:	*
 *									*
 *				<Main structure>			*
 *				<Parameter data>			*
 *				<Dep prods List>			*
 *				<Opt prods List>			*
 *				<Priority List>				*
 *				<Description string>			*
 *									*
 *  Input:	  buf - product id (buffer number) of table entry.	*
 *		  pindx    - product priority list element		*
 *									*
 *  Return:	  On success, the index of the old priority list	*
 *		  element is returned.					*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_delete_priority (
int	buf,
int	pindx
)
{
	int	i;
	short	*list;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	char	*ptr;

/*	Get the pointer to the record.  Return an error if a problem	*
 *	exists getting the pointer.					*/

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl == NULL) {

	    return ORPGPAT_ERROR;

	}

	indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

/*	If the index is not valid then return an error.			*/

	if ((pindx < 0) ||
	    (pindx >= attr_tbl->n_priority)) {

	    return ORPGPAT_ERROR;

	}

	attr_tbl->n_priority--;

	list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
			  attr_tbl->priority_list];

	for (i=pindx;i<attr_tbl->n_priority;i++) {

	    list [i] = list [i+1];

	}

/*	If the old list was padded for aligment purposes, then		*
 *	we can shrink the table by one word.  In this case, move	*
 *	trailing list items up one short and then move trailing		*
 *	records up one word and resize.  Otherwise, move up list	*
 *	items only (old last list element will now be padding).		*/

	if (attr_tbl->n_priority%2) {

	    Attr_tbl_size = Attr_tbl_size - 4;

	    for (i=Attr_tbl_ptr [indx]+attr_tbl->priority_list + pindx*2;
		 i<Attr_tbl_size;
		 i++) {

		 Attr_tbl [i] = Attr_tbl [i+4];

	    }

/*	    Since we have moved everything now shrink the table.	*/

	    ptr = (char *) realloc (Attr_tbl, (size_t) Attr_tbl_size);

	    if (ptr == (char *) NULL) {

		LE_send_msg (GL_INFO,
			"ORPGPAT_delete_priority: Unable to allocate memory\n");

	    }

	    Attr_tbl = ptr;

/*	Refresh the pointer to the record since realloc may have moved	*
 *	it somewhere else.  Return an error if a problem exists getting	*
 *	the pointer.							*/

	    attr_tbl = ORPGPAT_get_tbl_entry (buf);

	    if (attr_tbl == NULL) {

		return ORPGPAT_ERROR;

	    }

/*	    Update the pointer to the trailing description.		*/

	    if (attr_tbl->desc) {

		attr_tbl->desc = attr_tbl->desc - 4;

	    }

/*	    Update the record entry size element.			*/

	    attr_tbl->entry_size = attr_tbl->entry_size - 4;

/*	    Update pointer lookup table					*/

	    for (i=indx+1;i<Attr_tbl_num;i++) {

		Attr_tbl_ptr [i] = Attr_tbl_ptr [i] - 4;

	    }
	}

	if (attr_tbl->n_priority == 0) {

	    attr_tbl->priority_list = 0;

	}

	return pindx;
}

/************************************************************************
 *									*
 *  Description:  This function modifies the product description	*
 *		  element of a product attributes table entry.		*
 *		  Since a poduct attributes entry has a variable	*
 *		  length, extra work has to be done to shift other	*
 *		  data around.  To minimize the work needed to do	*
 *		  this, an order is followed in the placement of	*
 *		  specific data elements.  The following order is	*
 *		  used:							*
 *				<Main structure>			*
 *				<Parameter data>			*
 *				<Dep Prods List>			*
 *				<Opt Prods List>			*
 *				<Priority List>				*
 *				<Description string>			*
 *									*
 *  Input:	  buf - product id (buffer) number of table entry.	*
 *									*
 *  Return:	  On success, the new description length is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_description (
int	buf,
char	*string
)
{
	int	i;
	int	indx;
	int	new_len;
	int	old_len;
	int	old_size;
	int	new_size;
	int	delta;
	Pd_attr_entry	*attr_tbl;
	char	*ptr;

/*	First check to see if product exists in attributes table.  If	*
 *	it doesn't, do nothing and return error.			*/

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl == NULL) {

	    return ORPGPAT_ERROR;

	}

	indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	new_len  = strlen (string);
	new_size = new_len+1;

	if (new_size%4)
		new_size = new_size + (4-new_size%4);

/*	Check to see if this record had a previously defined		*
 *	description.  If so, find out how big it was so we can		*
 *	determine how to reallocate memory.  Remember the string	*
 *	is terminated with a null character and is aligned on a word	*
 *	boundary.							*/

	if (attr_tbl->desc == 0) {

	    old_len   = 0;
	    old_size  = 0;

/*	    The product description string goes at the end.		*/

	    attr_tbl->desc = attr_tbl->entry_size;

	} else {

	    old_len  = strlen ((char *) &Attr_tbl [Attr_tbl_ptr [indx] +
					 attr_tbl->desc]);
	    old_size = old_len + 1;

	    if (old_size%4)
		old_size = old_size + (4-old_size%4);

	}

/*	If the old and new lengths are the same, then we do not have to	*
 *	worry about reallocating memory and shifting trailing table	*
 *	data.								*/

	if (old_len == new_len) {

/*	    Both lengths are zero so do nothing (no description string	*
 *	    to add.							*/

	    if (new_len == 0) {

		return new_len;

	    } 

/*	    Overwrite the old string with the new one.			*/

	    strncpy (&Attr_tbl [Attr_tbl_ptr [indx] + attr_tbl->desc],
			string, new_len);

/*	If the new string is larger than the old string, expand the	*
 *	table, in necessary, and write the new string.			*/

	} else if (new_len > old_len) {

/*	    If the aligned record size for the new string is the same	*
 *	    as for the old string, then we do not have to resize the	*
 *	    table since the padding is sufficient to hold the string.	*/

	    if (new_size == old_size) {

/*		Overwrite the old string with the new one.		*/

		strncpy (&Attr_tbl [Attr_tbl_ptr [indx] + attr_tbl->desc],
			string, new_len);

/*	    The new string does not fit into the old padded string so	*
 *	    we need to expand the table and shift trailing records.	*/

	    } else {

		delta = new_size - old_size;
		ptr = (char *) realloc (Attr_tbl, (size_t) (Attr_tbl_size+delta));

		if (ptr == (char *) NULL) {

		    LE_send_msg (GL_INFO,
			"ORPGPAT_set_description: Unable to allocate memory\n");

	 	}

		Attr_tbl = ptr;

/*	Refresh the pointer to the record since realloc may have moved	*
 *	it somewhere else.  Return an error if a problem exists getting	*
 *	the pointer.							*/

		attr_tbl = ORPGPAT_get_tbl_entry (buf);

		if (attr_tbl == NULL) {

		    return ORPGPAT_ERROR;

		}

/*		First move data from all trailing records.		*/

		for (i=Attr_tbl_size-1;i>=Attr_tbl_ptr [indx]+attr_tbl->entry_size;i--) {

		    Attr_tbl [i+delta] = Attr_tbl [i];

		}

/*		Update pointer lookup table				*/

		for (i=indx+1;i<Attr_tbl_num;i++) {

		    Attr_tbl_ptr [i] = Attr_tbl_ptr [i] + delta;

		}

/*		Overwrite the old string with the new one.		*/

		strncpy (&Attr_tbl [Attr_tbl_ptr [indx] + attr_tbl->desc],
			string, new_len);

		Attr_tbl_size = Attr_tbl_size + delta;

		attr_tbl = ORPGPAT_get_tbl_entry (buf);
		attr_tbl->entry_size = attr_tbl->entry_size + delta;

	    }

	    for (i=new_len;i<new_size;i++) {

		Attr_tbl [Attr_tbl_ptr [indx]+ attr_tbl->desc+i] = 0;

	    }

/*	else the new string is smaller than the old string, shrink the	*
 *	table, if necessary, and write the new string.			*/

	} else {

/*	    If the aligned record size for the new string is the same	*
 *	    as for the old string, then we do not have to resize the	*
 *	    table since the padding is sufficient to hold the string.	*/

	    if (new_size == old_size) {

/*		Overwrite the old string with the new one.		*/

		strncpy (&Attr_tbl [Attr_tbl_ptr [indx] + attr_tbl->desc],
			string, new_len);

/*	    The new string does not require as much memory as the old 	*
 *	    string.  Shrink the table and shift trailing records.	*/

	    } else {

		delta = old_size - new_size;

/*		First move data from all trailing records.		*/

		for (i=Attr_tbl_ptr [indx]+attr_tbl->entry_size;i<Attr_tbl_size;i++) {

		    Attr_tbl [i-delta] = Attr_tbl [i];

		}

/*		Update pointer lookup table				*/

		for (i=indx+1;i<Attr_tbl_num;i++) {

		    Attr_tbl_ptr [i] = Attr_tbl_ptr [i] - delta;

		}

/*		Overwrite the old string with the new one.		*/

		strncpy (&Attr_tbl [Attr_tbl_ptr [indx] + attr_tbl->desc],
			string, new_len);

		Attr_tbl_size = Attr_tbl_size - delta;

		ptr = (char *) realloc (Attr_tbl, (size_t) Attr_tbl_size);

		if (ptr == (char *) NULL) {

		    LE_send_msg (GL_INFO,
			"ORPGPAT_set_description: Unable to allocate memory\n");

	 	}

		Attr_tbl = ptr;

/*	Refresh the pointer to the record since realloc may have moved	*
 *	it somewhere else.  Return an error if a problem exists getting	*
 *	the pointer.							*/

		attr_tbl = ORPGPAT_get_tbl_entry (buf);

		if (attr_tbl == NULL) {

		    return ORPGPAT_ERROR;

		}

		attr_tbl->entry_size = attr_tbl->entry_size - delta;

	    }

	    for (i=new_len;i<new_size;i++) {

		Attr_tbl [Attr_tbl_ptr [indx]+ attr_tbl->desc+i] = 0;

	    }
	}

	return new_len;
}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to the mnemonic	*
 *		  string associated with an ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  On success, a pointer to the mnemonic string is	*
 *		  returned.						*
 *		  On failure NULL is returned.				*
 *									*
 ************************************************************************/

char
*ORPGPAT_get_mnemonic (
int	buf
)
{
	char	*found;
	int	offset;
	int	indx;
	Pd_attr_entry	*attr_tbl;

	memset (Mne, 0, 4);

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {

	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    if (attr_tbl->desc != 0) {

		found = (char *) &Attr_tbl [Attr_tbl_ptr [indx] +
				  attr_tbl->desc];

/*		Since the product mnemonic is at the beginning of the	*
 *		description, move the pointer to the first character	*
 *		after the first space character.			*/

		offset = 0;

		while (strncmp ((found + offset)," ",1)) {

		    offset++;

		}

		offset++;

		if (offset > 1) {

		    if (offset > MAX_MNE_LENGTH+1) {

			offset = MAX_MNE_LENGTH;

		    }

		    strncpy (Mne,found,offset-1);

		}
	    }
	}

	return Mne;

}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to the description	*
 *		  string associated with a parameter of an ORPG 	*
 *		  product.						*
 *									*
 *  Input:	  buf   - product id (buffer number).			*
 *		  pindx - the parameter index				*
 *									*
 *  Return:	  On success, a pointer to the description string is	*
 *		  returned.						*
 *		  On failure NULL is returned.				*
 *									*
 ************************************************************************/

char
*ORPGPAT_get_parameter_name (
int	buf,
int	pindx
)
{
	char	*found;
	int	size;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;
	
	found = (char *) NULL;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	    
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] + 
				attr_tbl->params +
				pindx*size];
		found  = params->name;

	    }
	}

	return found;

}

/************************************************************************
 *									*
 *  Description:  This function sets the description string for a	*
 *		  parameter associated with an ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *		  pindx  _ parameter index for parameter.		*
 *		  string - new description string for parameter		*
 *									*
 *  Return:	  On success, the length of the string is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_parameter_name (
int	buf,
int	pindx,
char	*string
)
{
	int	size;
	int	len;
	int	i;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;
	
	len = ORPGPAT_ERROR;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		len = strlen (string);

		if (len > PARAMETER_NAME_LEN)
			len = PARAMETER_NAME_LEN;
		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params + 
				pindx*size];

		for (i=0;i<PARAMETER_NAME_LEN;i++)
			params->name[i] = 0;

		memcpy (params->name, string, len);

	    }
	}

	return len;

}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to the units		*
 *		  string associated with a parameter of an ORPG 	*
 *		  product.						*
 *									*
 *  Input:	  buf   - product id (buffer number).			*
 *		  pindx - the parameter index				*
 *									*
 *  Return:	  On success, a pointer to the units string is		*
 *		  returned.						*
 *		  On failure NULL is returned.				*
 *									*
 ************************************************************************/

char
*ORPGPAT_get_parameter_units (
int	buf,
int	pindx
)
{
	char	*found;
	int	size;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;
	
	found = (char *) NULL;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	    
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params + 
				pindx*size];
		found  = params->units;

	    }
	}

	return found;

}

/************************************************************************
 *									*
 *  Description:  This function sets the units string for a		*
 *		  parameter associated with an ORPG product.		*
 *									*
 *  Input:	  buf    - product id (buffer number).			*
 *		  pindx  - parameter index for parameter.		*
 *		  string - new units string for parameter		*
 *									*
 *  Return:	  On success, the length of the string is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_parameter_units (
int	buf,
int	pindx,
char	*string
)
{
	int	size;
	int	len;
	int	i;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;
	
	len = ORPGPAT_ERROR;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	    
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		len = strlen (string);

		if (len > PARAMETER_UNITS_LEN)
			len = PARAMETER_UNITS_LEN;
		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params + 
				pindx*size];

		for (i=0;i<PARAMETER_UNITS_LEN;i++)
			params->units [i] = 0;

		memcpy (params->units, string, len);

	    }
	}

	return len;

}

/************************************************************************
 *									*
 *  Description:  This function returns the index of a parameter	*
 *		  associated with an ORPG product.			*
 *									*
 *  Input:	  buf   - product id (buffer number).			*
 *		  pindx - parameter position in the attributes		*
 *			  table parameter list.				*
 *									*
 *  Return:	  On success, the parameter index is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_parameter_index (
int	buf,
int	pindx
)
{
	int	ret;
	int	size;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params +
				pindx*size];
		ret    = params->index;

	    }
	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function sets the index of a parameter		*
 *		  associated with an ORPG product.			*
 *									*
 *  Input:	  buf   - product id (buffer number).			*
 *		  pindx - parameter position in the attributes		*
 *			  table parameter list.				*
 *		  val   - new parameter index.				*
 *									*
 *  Return:	  On success, the parameter index (pindx) is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_parameter_index (
int	buf,
int	pindx,
int	val
)
{
	int	ret;
	int	size;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params + 
				pindx*size];
		params->index = val;
		ret = pindx;

	    }
	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function returns the maximum allowed value of a	*
 *		  parameter associated with an ORPG product.		*
 *									*
 *  Input:	  buf   - product id (buffer number).			*
 *		  pindx - parameter position in the attributes		*
 *			  table parameter list.				*
 *									*
 *  Return:	  On success, the maximum allowed value is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_parameter_max (
int	buf,
int	pindx
)
{
	int	ret;
	int	size;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params + 
				pindx*size];
		ret    = params->max;

	    }
	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function sets the scaled maximum allowed value 	*
 *		  of a parameter associated with an ORPG product.	*
 *									*
 *  Input:	  buf   - product id (buffer number).			*
 *		  pindx - parameter position in the attributes		*
 *			  table parameter list.				*
 *		  val   - new maximum value.				*
 *									*
 *  Return:	  On success, the scaled maximumvalue is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_parameter_max (
int	buf,
int	pindx,
int	val
)
{
	int	ret;
	int	size;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params + 
				pindx*size];
		params->max = val;
		ret = pindx;

	    }
	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function returns the minimum allowed value of a	*
 *		  parameter associated with an ORPG product.		*
 *									*
 *  Input:	  buf   - product id (buffer number).			*
 *		  pindx - parameter position in the attributes		*
 *			  table parameter list.				*
 *									*
 *  Return:	  On success, the minimum allowed value is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_parameter_min (
int	buf,
int	pindx
)
{
	int	ret;
	int	size;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params + 
				pindx*size];
		ret    = params->min;

	    }
	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function sets the scaled minimum allowed value 	*
 *		  of a parameter associated with an ORPG product.	*
 *									*
 *  Input:	  buf   - product id (buffer number).			*
 *		  pindx - parameter position in the attributes		*
 *			  table parameter list.				*
 *		  val   - new minimum value.				*
 *									*
 *  Return:	  On success, the scaled minimum value is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_parameter_min (
int	buf,
int	pindx,
int	val
)
{
	int	ret;
	int	size;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params + 
				pindx*size];
		params->min = val;
		ret = pindx;

	    }
	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function returns the defined default value of a	*
 *		  parameter associated with an ORPG product.		*
 *									*
 *  Input:	  buf   - product id (buffer number).			*
 *		  pindx - parameter position in the attributes		*
 *			  table parameter list.				*
 *									*
 *  Return:	  On success, the defined default value is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_parameter_default (
int	buf,
int	pindx
)
{
	int	ret;
	int	size;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params + 
				pindx*size];
		ret    = params->def;

	    }
	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function sets the scaled defined default value 	*
 *		  of a parameter associated with an ORPG product.	*
 *									*
 *  Input:	  buf   - product id (buffer number).			*
 *		  pindx - parameter position in the attributes		*
 *			  table parameter list.				*
 *		  val   - new default value.				*
 *									*
 *  Return:	  On success, the scaled default value is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_parameter_default (
int	buf,
int	pindx,
int	val
)
{
	int	ret;
	int	size;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params + 
				pindx*size];
		params->def = (short) val;
		ret = pindx;

	    }
	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function returns the scale factor value of a	*
 *		  parameter associated with an ORPG product.		*
 *									*
 *  Input:	  buf   - product id (buffer number).			*
 *		  pindx - parameter position in the attributes		*
 *			  table parameter list.				*
 *									*
 *  Return:	  On success, the scale factor value is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_parameter_scale (
int	buf,
int	pindx
)
{
	int	ret;
	int	size;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params + 
				pindx*size];
		ret    = params->scale;

	    }
	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function sets the scale factor value of a 	*
 *		  parameter associated with an ORPG product.		*
 *									*
 *  Input:	  buf   - product id (buffer number).			*
 *		  pindx - parameter position in the attributes		*
 *			  table parameter list.				*
 *		  val   - new scale factor.				*
 *									*
 *  Return:	  On success, the scale factor is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_parameter_scale (
int	buf,
int	pindx,
int	val
)
{
	int	ret;
	int	size;
	int	indx;
	Pd_attr_entry	*attr_tbl;
	Pd_params_entry	*params;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {
	    
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];
	
	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_params)) {

		size = sizeof (Pd_params_entry);

		if (size%4)
			size = size + (4-size%4);

		params = (Pd_params_entry *)
				&Attr_tbl [Attr_tbl_ptr [indx] +
				attr_tbl->params + 
				pindx*size];
		params->scale = val;
		ret = val;

	    }
	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function returns the  ORPG product id for	*
 *		  an entry in the product attributes table.		*
 *									*
 *  Inputs:	  indx    - Attributes table index			*
 *									*
 *  Return:	  On success, the product id (buffer number) is		*
 *		  returned.						*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_prod_id (
int	indx
)
{
	int	prod_id;
	Pd_attr_entry	*attr_tbl;
	char		*buf;

	prod_id = ORPGPAT_ERROR;

	if (Attr_tbl_num > indx && indx >= 0) {

	    buf = ORPGPAT_get_tbl_ptr (indx);

	    if (buf != (char *) NULL) {

		attr_tbl = (Pd_attr_entry *) buf;

		prod_id = attr_tbl->prod_id;

	    }
	}

	return prod_id;
}

/************************************************************************
 *									*
 *  Description:  This function sets the ORPG product id for		*
 *		  an entry in the product attributes table.		*
 *									*
 *  Inputs:	  indx    - Attributes table index			*
 *		  prod_id - new product id				*
 *									*
 *  Return:	  On success, the product id (buffer number) is		*
 *		  returned.						*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_prod_id (
int	indx,
int	prod_id
)
{
	Pd_attr_entry	*attr_tbl;
	char	*buf;
	int	ret;

	ret = ORPGPAT_ERROR;

	if (Attr_tbl_num > indx && indx >= 0) {

	    buf = ORPGPAT_get_tbl_ptr (indx);

	    if (buf != (char *) NULL) {

		attr_tbl = (Pd_attr_entry *) buf;

		attr_tbl->prod_id = prod_id;
		ret = prod_id;

	    }
	}

	return prod_id;
}

/************************************************************************
 *									*
 *  Description:  This function returns the generation task name	*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  On success the generation task ID is returned.	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

char *
ORPGPAT_get_gen_task (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	char	*ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = NULL;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->gen_task;

	}

	return ret;
}


/************************************************************************
 *									*
 *  Description:  This function sets the generation task name		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *		  name  - new generation task name			*
 *									*
 *  Return:	  On success 0 is returned.				*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_gen_task (
int	buf,
char	*name
)
{
	int	ret;
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    strcpy( attr_tbl->gen_task, name );
            ret = 0;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function returns the disable flag		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  On success the disable flag is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_disabled (
int	buf
)
{
	int	ret;
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = (int) attr_tbl->disabled;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function sets the disable flag			*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *		  priority - new disable flag				*
 *									*
 *  Return:	  On success the disable flag is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_disabled (
int	buf,
int	disabled
)
{
	int	ret;
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    if ((disabled >= 0) &&
		(disabled <= 1)) {

		attr_tbl->disabled = disabled;
		ret = disabled;

	    }
	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function returns the elevation index		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  On success the elevation index is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_elevation_index (
int	buf
)
{
	int	ret;
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = (int) attr_tbl->elev_index;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function sets the elevation index		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *		  priority - new elevation index			*
 *									*
 *  Return:	  On success the elevation index is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_elevation_index (
int	buf,
int	elevation_index
)
{
	int	ret;
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    attr_tbl->elev_index = elevation_index;
	    ret = elevation_index;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function returns the compression type		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  On success the compression type is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_compression_type (
int	buf
)
{
	int	ret;
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = (int) attr_tbl->compression;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function sets the compression type		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *		  compression - new compression type			*
 *									*
 *  Return:	  On success the compression type is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_compression_type (
int	buf,
int	compression_type
)
{
	int	ret;
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    attr_tbl->compression = (char) compression_type;
	    ret = compression_type;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function gets the product resolution		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *		  flg - resolution component: ORPGPAT_X_AZI_RES		*
 *			or ORPGPAT_Y_RAN_RES				*
 *									*
 *  Return:	  On success the resolution is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_resolution (
int	buf,
int	flg
)
{
	int	ret;
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    switch (flg) {

		case ORPGPAT_X_AZI_RES :

		    ret = attr_tbl->x_azi_res;
		    break;

		case ORPGPAT_Y_RAN_RES :

		    ret = attr_tbl->y_ran_res;
		    break;

	    }
	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function sets the product resolution		*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *		  flg - resolution component: ORPGPAT_X_AZI_RES		*
 *			or ORPGPAT_Y_RAN_RES				*
 *		  res - new product resolution				*
 *									*
 *  Return:	  On success the resolution is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_resolution (
int	buf,
int	flg,
int	res
)
{
	int	ret;
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    switch (flg) {

		case ORPGPAT_X_AZI_RES :

		    attr_tbl->x_azi_res = (short) res;
		    ret = res;
		    break;

		case ORPGPAT_Y_RAN_RES :

		    attr_tbl->y_ran_res = (short) res;
		    ret = res;
		    break;

	    }
	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function returns the format type			*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *									*
 *  Return:	  On success the format type is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_format_type (
int	buf
)
{
	int	ret;
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = (int) attr_tbl->format_type;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function sets the format type			*
 *		  associated with a specified ORPG product.		*
 *									*
 *  Input:	  buf - product id (buffer number).			*
 *		  format_type - new product format type			*
 *									*
 *  Return:	  On success the format type is returned.		*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_format_type (
int	buf,
int	format_type
)
{
	int	ret;
	Pd_attr_entry	*attr_tbl;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    attr_tbl->format_type = (char) format_type;
	    ret = format_type;

	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function adds a new record to the attributes	*
 *		  table.						*
 *									*
 *  Inputs:	  buf - the product id (buffer number) associated	*
 *		  with the new record.					*
 *									*
 *  Return:	  On success, the index number of the new record is	*
 *		  returned.  If the attributes table already contains	*
 *		  a record with the same product id, or if an error	*
 *		  occurred creating the new record, ORPGPAT_ERROR is	*
 *		  returned.						*
 *									*
 ************************************************************************/

int
ORPGPAT_add_prod (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	size;
	int	indx;
	int	i;
	char	*ptr;

	if (Need_update)
	    ORPGPAT_read_tbl ();
	if (Need_update)
	    return (0);

	if (abs(buf) < MAX_PAT_TBL_SIZE) {

	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	} else {

	    return ORPGPAT_ERROR;

	}

/*	If the specified buffer number does NOT exist in the product	*
 *	attributes table, append the new buffer definition.		*/

	if (indx < 0) {

/*	    Get the size of the basic record and increase the size	*
 *	    of the table.  Make sure the record is aligned on a word	*
 *	    boundary.							*/

	    size = sizeof (Pd_attr_entry);
	    
	    if (size%4)
		size = size + (4-size%4);

	    ptr = (char *) realloc (Attr_tbl, (size_t) (Attr_tbl_size+size));

	    if (ptr == (char *) NULL) {

		LE_send_msg (GL_INFO,
			"ORPGPAT_add_record: Unable to allocate memory\n");

		return ORPGPAT_ERROR;

	    }

	    Attr_tbl = ptr;

/*	    Initialize the new record by setting all elements to 0.	*/

	    for (i=0;i<size;i++) {

		Attr_tbl [Attr_tbl_size+i] = 0;

	    }

/*	    Update the attributes table lookup table.			*/

	    Attr_tbl_ptr [Attr_tbl_num] = Attr_tbl_size;

/*	    Define a pointer to the new record data structure.		*/

	    attr_tbl = (Pd_attr_entry *) (Attr_tbl + Attr_tbl_size);

/*	    Set the size and product id elements in the record.		*/

	    attr_tbl->entry_size     = size;
	    attr_tbl->prod_id        = buf;
	    attr_tbl->type           = 0;
	    attr_tbl->elev_index     = -1;
	    attr_tbl->alert          = 0;
	    attr_tbl->warehoused     = 0;
	    attr_tbl->desc           = 0;
	    attr_tbl->disabled       = 0;
	    attr_tbl->wx_modes       = 0;
	    attr_tbl->compression    = 0;
	    attr_tbl->n_dep_prods    = 0;
	    attr_tbl->n_opt_prods    = 0;
	    attr_tbl->n_priority     = 0;
	    attr_tbl->n_params       = 0;
	    attr_tbl->priority_list  = 0;
	    attr_tbl->opt_prods_list = 0;
	    attr_tbl->params         = 0;

	    Id_to_index [buf+MAX_PAT_TBL_SIZE] = Attr_tbl_num;
	    indx = Attr_tbl_num;

	    if (Attr_tbl_num) {

		Pd_attr_entry	*tmp;

		tmp = (Pd_attr_entry *) &Attr_tbl [Attr_tbl_ptr [indx-1]];

	    }

	    Attr_tbl_num++;
	    Attr_tbl_size = Attr_tbl_size + size;

	} else {

	    indx = ORPGPAT_ERROR;

	}

	return indx;

}

/************************************************************************
 *									*
 *  Description:  This function removes a record in the attributes	*
 *		  table.						*
 *									*
 *  Inputs:	  buf_num - the product id (buffer number) associated	*
 *		  with the record.					*
 *									*
 *  Return:	  On success, the index number of the old record is	*
 *		  returned.  If the attributes table does not contain	*
 *		  a record of the input product id, or if an error	*
 *		  occurred deleting the old record, ORPGPAT_ERROR is	*
 *		  returned.						*
 *									*
 ************************************************************************/

int
ORPGPAT_delete_prod (
int	buf
)
{
	int	i, j;
	Pd_attr_entry	*attr_tbl;
	int	size;
	int	indx;
        int     offset;
        int     code = 0;
	char	*ptr;

/*	If the specified buffer number does exist in the product	*
 *	attributes table, move any following buffer definition up	*
 *	one position (overwrite deleted buffer number).			*/

	indx     = ORPGPAT_ERROR;
	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	if (attr_tbl != NULL) {

/*	    Temporarily save the size of the to be deleted table entry	*/

	    ptr  = (char *) attr_tbl;
	    size = attr_tbl->entry_size;;
            code = attr_tbl->prod_code;

/*	    Move all following data up in the data buffer.		*/

	    j    = 0;
	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    for (i=Attr_tbl_ptr [indx]+size;
		 i<Attr_tbl_size;i++) {

		ptr [j] = ptr [j+size];
		j++;

	    }

/*	    Resize the table size variables.				*/

	    Attr_tbl_size = Attr_tbl_size-size;

/*	    Create a new data buffer to hold the modified product	*
 *	    attributes data.						*/

	    ptr = (char *) realloc (Attr_tbl, (size_t) Attr_tbl_size);

	    if (ptr == (char *) NULL) {

		LE_send_msg (GL_INFO,
			"ORPGPAT_delete_record: Unable to allocate memory\n");

		return ORPGPAT_ERROR;

	    }

	    Attr_tbl = ptr;

/*	    Update the Id_to_index and Code_to Index LUT to remove 	*
 *	    buffer just deleted.					*/

	    Attr_tbl_num  = 0;

/*	    First initialize each lookup table entry to -1.		*/

	    for (i = 0; i < MAX_PAT_TBL_SIZE*2; i++)
	        Id_to_index [i] = ORPGPAT_DATA_NOT_FOUND;

	    for (i = 0; i < MAX_PAT_TBL_SIZE*2; i++)
	        Code_to_index [i] = ORPGPAT_DATA_NOT_FOUND;

	    offset = 0;

	    while (offset < Attr_tbl_size) {

	        short code, id;

	        if (Attr_tbl_num >= MAX_PAT_TBL_SIZE) {

		    LE_send_msg (GL_INPUT,
			"ORPGPAT: too many products (%d)\n", Attr_tbl_num);
	            Process_exception ();
		    return (ORPGPAT_ERROR);

	        }

/*	        Attr_tbl_ptr contains the offset, in bytes, to the	*
 *		start of each attributes table record.			*/

	        Attr_tbl_ptr [Attr_tbl_num] = offset;

/*		Cast the current buffer pointer to the main attributes	*
 *	        table structure.  The size of the record can be 	*
 * 		determined from the entry_size element in the main	*
 *		structure.  The start of the next record is the current	*
 * 		offset plus the current record size.			*/


	        attr_tbl = (Pd_attr_entry *) (Attr_tbl+offset);

	        offset = offset + attr_tbl->entry_size;

/*		Update the reference record by product id lookup table	*/

	        id = attr_tbl->prod_id;

	        if (abs(id) < MAX_PAT_TBL_SIZE) {

		    if (Id_to_index[id+MAX_PAT_TBL_SIZE] < 0)
		        Id_to_index[id+MAX_PAT_TBL_SIZE] = Attr_tbl_num;
		    else {
	                LE_send_msg (GL_INPUT,
			    "ORPGPAT: duplicated prod_id (%d) in prod attr table\n", 
			    id);
		    }

	        }

/*		Update the reference record by product code lookup table*
 * 		Remember that negative product codes are allowed and 	*
 *		have special meaning (i.e., -91, -92). That is why the	*
 *              lookup table is normalized.				*/

	        code = attr_tbl->prod_code;

	        if ((code < MAX_PAT_TBL_SIZE) && (code >= -MAX_PAT_TBL_SIZE)) {

		    if (Code_to_index[code+MAX_PAT_TBL_SIZE] < 0)
		        Code_to_index[code+MAX_PAT_TBL_SIZE] = Attr_tbl_num;
		    else if (code != 0) {	/* non-zero duplicated pcode */
	                LE_send_msg (GL_INPUT,
		            "ORPGPAT: duplicated prod_code (%d) in prod attr table\n", 
			    code);
		    }
	        }

	        Attr_tbl_num++;

	    }

	}

	return indx;

}

/************************************************************************
 *									*
 *  Description:  This function returns the number of product	 	*
 *		  priorities associated with an entry in the product	*
 *		  attributes table.					*
 *									*
 *  Inputs:	  buf - the product id (buffer number) associated	*
 *		  with the record.					*
 *									*
 *  Return:	  On success, the number of priority elements is	*
 *		  returned.  On failure, ORPGPAT_ERROR is returned.	*
 *									*
 ************************************************************************/

int
ORPGPAT_get_num_priorities (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->n_priority;

	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function returns the product priority of a 	*
 *		  product priority list element.			*
 *									*
 *  Inputs:	  buf  - the product id (buffer number) associated	*
 *		         with the record.				*
 *		  indx - the element position in the list		*
 *			 NOTE: 0 = default,   1 = Wx mode M,		*
 *			       2 = Wx mode A, 3 = Wx Mode B		*
 *									*
 *  Return:	  On success, the product priority of the list emement	*
 *		  is returned.						*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_priority (
int	buf,
int	pindx
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;
	int	indx;
	short	*list;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_priority)) {

		list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
				  attr_tbl->priority_list];
		ret  = list [pindx];

	    }
	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to the product	*
 *		  priority list for a product.				*
 *									*
 *  Inputs:	  buf  - the product id (buffer number) associated	*
 *		         with the record.				*
 *									*
 *  Return:	  On success, a pointer to the list is returned.	*
 *		  On failure NULL is returned.				*
 *									*
 ************************************************************************/

short
*ORPGPAT_get_priority_list (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	indx;
	short	*list;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	list = (short *) NULL;

	if (attr_tbl != NULL) {

	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    if (attr_tbl->priority_list > 0) {

		list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
				  attr_tbl->priority_list];

	    }
	}

	return list;
}

/************************************************************************
 *									*
 *  Description:  This function sets the product priority of a		*
 *		  specified product priority list element.		*
 *									*
 *  Inputs:	  buf  - the product id (buffer number) associated	*
 *		         with the record.				*
 *		  indx - the element position in the list		*
 *		  id   - the new product priority id.			*
 *									*
 *  Return:	  On success, the product priority of the list emement	*
 *		  is returned.						*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_priority (
int	buf,
int	lindx,
int	priority
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;
	int	indx;
	short	*list;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if ((attr_tbl != NULL) &&
	    (priority >= 0) &&
	    (priority <= 255)) {

	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    if ((lindx >= 0) &&
		(lindx < attr_tbl->n_priority)) {

		list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
				  attr_tbl->priority_list];
		list [lindx] = priority;
		ret  = priority;

	    }
	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function returns the number of dependent 	*
 *		  products associated with an entry in the product	*
 *		  attributes table.					*
 *									*
 *  Inputs:	  buf - the product id (buffer number) associated	*
 *		  with the record.					*
 *									*
 *  Return:	  On success, the number of tasks is returned.		*
 *		  On failure, ORPGPAT_ERROR is returned.		*
 *									*
 ************************************************************************/

int
ORPGPAT_get_num_dep_prods (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->n_dep_prods;

	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function returns the product id of a specified	*
 *		  dependent products list element.			*
 *									*
 *  Inputs:	  buf  - the product id (buffer number) associated	*
 *		         with the record.				*
 *		  indx - the element position in the list		*
 *									*
 *  Return:	  On success, the product id of the list emement is	*
 *		  returned.						*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_dep_prod (
int	buf,
int	pindx
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;
	int	indx;
	short	*list;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_dep_prods)) {

		list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
				  attr_tbl->dep_prods_list];
		ret  = list [pindx];

	    }
	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to the dependent	*
 *		  products list for a product.				*
 *									*
 *  Inputs:	  buf  - the product id (buffer number) associated	*
 *		         with the record.				*
 *									*
 *  Return:	  On success, a pointer to the list is returned.	*
 *		  On failure NULL is returned.			*
 *									*
 ************************************************************************/

short
*ORPGPAT_get_dep_prods_list (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	indx;
	short	*list;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	list = (short *) NULL;

	if (attr_tbl != NULL) {

	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    if (attr_tbl->dep_prods_list > 0) {

		list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
				  attr_tbl->dep_prods_list];

	    }
	}

	return list;
}

/************************************************************************
 *									*
 *  Description:  This function sets the product id of a specified	*
 *		  dependent products list element.			*
 *									*
 *  Inputs:	  buf  - the product id (buffer number) associated	*
 *		         with the record.				*
 *		  indx - the element position in the list		*
 *		  id   - the new dependent product id.			*
 *									*
 *  Return:	  On success, the product id of the list emement is	*
 *		  returned.						*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_dep_prod (
int	buf,
int	lindx,
int	id
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;
	int	indx;
	short	*list;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    if ((lindx >= 0) &&
		(lindx < attr_tbl->n_dep_prods)) {

		list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
				  attr_tbl->dep_prods_list];
		list [lindx] = id;
		ret  = id;

	    }
	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function returns the number of optional 		*
 *		  products associated with an entry in the product	*
 *		  attributes table.					*
 *									*
 *  Inputs:	  buf - the product id (buffer number) associated	*
 *		  with the record.					*
 *									*
 *  Return:	  On success, the number of tasks is returned.		*
 *		  On failure, ORPGPAT_ERROR is returned.		*
 *									*
 ************************************************************************/

int
ORPGPAT_get_num_opt_prods (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->n_opt_prods;

	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function returns the product id of a specified	*
 *		  optional products list element.			*
 *									*
 *  Inputs:	  buf  - the product id (buffer number) associated	*
 *		         with the record.				*
 *		  indx - the element position in the list		*
 *									*
 *  Return:	  On success, the product id of the list emement is	*
 *		  returned.						*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_opt_prod (
int	buf,
int	pindx
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;
	int	indx;
	short	*list;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    if ((pindx >= 0) &&
		(pindx < attr_tbl->n_opt_prods)) {

		list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
				  attr_tbl->opt_prods_list];
		ret  = list [pindx];

	    }
	}

	return ret;
}

/************************************************************************
 *									*
 *  Description:  This function returns a pointer to the optional	*
 *		  products list for a product.				*
 *									*
 *  Inputs:	  buf  - the product id (buffer number) associated	*
 *		         with the record.				*
 *									*
 *  Return:	  On success, a pointer to the list is returned.	*
 *		  On failure NULL is returned.				*
 *									*
 ************************************************************************/

short
*ORPGPAT_get_opt_prods_list (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	int	indx;
	short	*list;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	list = (short *) NULL;

	if (attr_tbl != NULL) {

	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    if (attr_tbl->opt_prods_list > 0) {

		list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
				  attr_tbl->opt_prods_list];

	    }
	}

	return list;
}

/************************************************************************
 *									*
 *  Description:  This function sets the product id of a specified	*
 *		  optional products list element.			*
 *									*
 *  Inputs:	  buf  - the product id (buffer number) associated	*
 *		         with the record.				*
 *		  indx - the element position in the list		*
 *		  id   - the new optional product id.			*
 *									*
 *  Return:	  On success, the product id of the list emement is	*
 *		  returned.						*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_opt_prod (
int	buf,
int	lindx,
int	id
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;
	int	indx;
	short	*list;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    indx = Id_to_index [buf+MAX_PAT_TBL_SIZE];

	    if ((lindx >= 0) &&
		(lindx < attr_tbl->n_opt_prods)) {

		list = (short *) &Attr_tbl [Attr_tbl_ptr [indx] +
				  attr_tbl->opt_prods_list];
		list [lindx] = id;
		ret  = id;

	    }
	}

	return ret;
}
/************************************************************************
 *									*
 *  Description:  This function returns a pointer to the product name	*
 *		  of an entry in the product attributes table.		*
 *									*
 *  Inputs:	  buf - the product id (buffer number) associated	*
 *		        with the record.				*
 *									*
 *  Return:	  On success, a pointer to the product name is		*
 *		  returned.						*
 *		  On failure, a null pointer is returned.		*
 *									*
 ************************************************************************/

char
*ORPGPAT_get_name (
int	buf
)
{
	Pd_attr_entry	*attr_tbl;
	char	*ret;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = (char *) NULL;

	if (attr_tbl != NULL) {

	    ret = attr_tbl->name;

	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function sets the product name associated	*
 *		  with an entry in the product attributes table.	*
 *									*
 *  Inputs:	  buf - the product id (buffer number) associated	*
 *		        with the record.				*
 *		  name - pointer to new name string.			*
 *									*
 *  Return:	  On success the length of the product name is		*
 *		  returned.						*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_set_name (
int	buf,
char	*name
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;
	int	len;
	int	i;

	attr_tbl = ORPGPAT_get_tbl_entry (buf);

	ret = ORPGPAT_ERROR;

	if (attr_tbl != NULL) {

	    for (i=0;i<PROD_NAME_LEN;i++)
		attr_tbl->name[i] = 0;

	    len = strlen (name);

	    if (len > PROD_NAME_LEN-1)
		len = PROD_NAME_LEN-1;

	    strncpy (attr_tbl->name, name, len);

	    ret = len;

	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function returns the product id associated	*
 *		  with an input product name.				*
 *									*
 *  Inputs:	  name - a pointer to a string contaning the poduct	*
 *			 name.						*
 *									*
 *  Return:	  On success the associated product id is returned	*
 *		  On failure ORPGPAT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPAT_get_prod_id_by_name (
char	*name
)
{
	Pd_attr_entry	*attr_tbl;
	int	ret;
	int	len;
	int	i;

	ret = ORPGPAT_ERROR;

	len = strlen (name);

	if (len > 0) {

	    for (i=0;i<Attr_tbl_num;i++) {

		attr_tbl = (Pd_attr_entry *) ORPGPAT_get_tbl_ptr (i);

		if (attr_tbl != NULL) {

		    if (!strncmp (attr_tbl->name, name, len)) {

			ret = attr_tbl->prod_id;
			break;

		    }
		}
	    }
	}

	return ret;

}

/************************************************************************
 *									*
 *  Description:  This function writes the product attributes table	*
 *		  to the lb.						*
 *									*
 *  Return:	  On success, a positive integer reflecting the number	*
 *		  of bytes written to the lb is returned.		*
 *		  Otherwise, any other value is considered an error.	*
 *									*
 ************************************************************************/

int
ORPGPAT_write_tbl (
)
{
	int	status;

/*	Check to see if the table has not been initialized.  If	so,	*
 *	do nothing and return an error.					*/

	if (Attr_tbl == (char *) NULL) {

	    return ORPGPAT_ERROR;

	}

	status = ORPGDA_write( ORPGDAT_PAT, Attr_tbl, Attr_tbl_size,
			       PROD_ATTR_MSG_ID );				      
	io_status = status;

	if (status <= 0) {
   
	    LE_send_msg (GL_LB (status),					      
		"ORPGPAT: ORPGDA_write PROD_ATTR_MSG_ID failed (ret %d, len %d)\n",
		status, Attr_tbl_size);						      
	} else {

	    Need_update = 1;

	}

	return status;
}
