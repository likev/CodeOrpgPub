/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/27 14:41:40 $
 * $Id: orpgpgt.c,v 1.31 2012/09/27 14:41:40 steves Exp $
 * $Revision: 1.31 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  orpgpgt.c						*
 *		This module contains a collection of routines to	*
 *		manipulate the product generation table.		*
 *									*
 ************************************************************************/


/*	System include files					*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <orpg.h>
#include <orpgpgt.h>
#include <orpgmisc.h>

#include <rss_replace.h>

static	int	Gen_default_A_tbl_size = 0;
			/*  Size (bytes) of default prod gen msg for	*
			 *  weather mode A.				*/
static	int	Gen_default_A_tbl_num  = 0;
			/*  Number of Wx mode A prod gen entries.	*/
static	char	*Gen_default_A_tbl = NULL;
				/* Pointer to the Wx mode A product	*
				 * generation table			*/
static	int	Gen_default_B_tbl_size = 0;
			/*  Size (bytes) of default prod gen msg for	*
			 *  weather mode B.				*/
static	int	Gen_default_B_tbl_num  = 0;
			/*  Number of Wx mode B prod gen entries.	*/
static	char	*Gen_default_B_tbl = NULL;
				/* Pointer to the Wx mode B product	*
				 * generation table			*/
static	int	Gen_current_tbl_size = 0;
			/*  Size (bytes) of current prod gen msg	*/
static	int	Gen_current_tbl_num  = 0;
			/*  Number of current prod gen entries.	*/
static	char	*Gen_current_tbl = NULL;
				/* Pointer to the current product	*
				 * generation table			*/

static	int	Gen_default_A_update  = 1;/* gen table re-read needed	*/
static	int	Gen_default_B_update  = 1;/* gen table re-read needed	*/
static	int	Gen_current_update    = 1;/* gen table re-read needed	*/
static	int	Gen_LB_update_flag    = 1;/* gen table LB re-read needed*/
static	int	Gen_en_registered   = 0;/* EN registered		*/
static	int	Gen_LB_registered   = 0;/* LB registered		*/
static	void	(*Gen_exception_callback)() = NULL;
					/* User's exception callback	*
					 * function.			*/

static	void	Gen_en_callback (EN_id_t evtcd, char *msg, int msglen, void *arg);
static	void	Gen_LB_callback (int fd, LB_id_t msg_id,
                                 int msg_info, void *arg);
static	void	Gen_process_exception ();

/************************************************************************
 *									*
 *	Description:  This function registers a callback funtion which	*
 *		      will be called when an exception condition is	*
 *		      encountered.					*
 *									*
 *	Input:	      Gen_exception_callback - the user's callback	*
 *		      function.						*
 *									*
 ***********************************************************************/

void
ORPGPGT_error (void (*user_exception_callback)())
{
	Gen_exception_callback = user_exception_callback;
	return;
}

/************************************************************************
 *									*
 *	Description:  This function is called when an exception		*
 *		      condition is detected.				*
 *									*
 *	Input:	      None						*
 *									*
 ************************************************************************/

static void
Gen_process_exception ()
{
	if (Gen_exception_callback == NULL) {

	    LE_send_msg (GL_ERROR,
		"ORPGPGT: process exception");

	} else {

	    Gen_exception_callback ();

	}

	return;
}

/************************************************************************
 *									*
 *	Description:  This is the event notification callback function.	*
 *									*
 ************************************************************************/

static void
Gen_en_callback (
EN_id_t	evtcd,
char	*msg,
int	msglen,
void    *arg
)
{

	Gen_current_update    = 1;
	Gen_default_A_update  = 1;
	Gen_default_B_update  = 1;
	return;

}

/************************************************************************
 *                                                                      *
 *      Description:  This is the LB callback function. It is called	*
 *		      when the LB is updated.				*
 *                                                                      *
 ************************************************************************/

static void
Gen_LB_callback (
int	fd,
LB_id_t	msg_id,
int	msg_info,
void	*arg
)
{
  Gen_LB_update_flag    = 1;
  return;
}

/************************************************************************
 *                                                                      *
 *      Description:  This function returns a flag indicating whether	*
 *		      new info is available in the current generation	*
 *		      lists' LB.					*
 *                                                                      *
 ************************************************************************/

int
ORPGPGT_get_update_flag ()
{
  return Gen_LB_update_flag;
}

/************************************************************************
 *									*
 *	Description:  The following module clears the specified product	*
 *		      generation table.					*
 *									*
 *	Return:	      If the table was previously initialized, 1	*
 *		      is returned, otherwise, 0 is returned.		*
 *									*
 ************************************************************************/

int
ORPGPGT_clear_tbl (
int	table
)
{
	int	ret = 0;

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

		if (Gen_default_A_tbl != (char *) NULL) {

		    free (Gen_default_A_tbl);
		    Gen_default_A_tbl_num   = 0;
		    Gen_default_A_tbl_size  = 0;
		    Gen_default_A_update    = 0;
                    Gen_default_A_tbl       = (char *) NULL;
		    ret                     = 1;

		} else {

		    ret = 0;

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

		if (Gen_default_B_tbl != (char *) NULL) {

		    free (Gen_default_B_tbl);
		    Gen_default_B_tbl_num   = 0;
		    Gen_default_B_tbl_size  = 0;
		    Gen_default_B_update    = 0;
                    Gen_default_B_tbl       = (char *) NULL;
		    ret                     = 1;

		} else {

		    ret = 0;

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

		if (Gen_current_tbl != (char *) NULL) {

		    free (Gen_current_tbl);
		    Gen_current_tbl_num   = 0;
		    Gen_current_tbl_size  = 0;
		    Gen_current_update    = 0;
		    Gen_LB_update_flag    = 0;
                    Gen_current_tbl       = (char *) NULL;
		    ret                   = 1;

		} else {

		    ret = 0;

		}

		break;

	}

	return ret;
}


/***********************************************************************
 *									*
 *	Description:  The following module reads the product generation	*
 *		      tables from the product generation lb.  The size	*
 *		      of the message containing the data is determined	*
 *		      first and memory is dynamically allocated to	*
 *		      store the message in memory.  A pointer is kept	*
 *		      to the start of the data.				*
 *									*
 *	Return:	      On success, 0 is returned, otherwise		*
 *		      ORPGPGT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPGT_read_tbl (
int	table
)
{
	int	status;
	
	if (!Gen_en_registered) {

	    int	ret;

	    ret = EN_register (ORPGEVT_PROD_LIST, Gen_en_callback);

	    if (ret < 0) {

		LE_send_msg (GL_CONFIG,
			"ORPGPGT: EN_register failed (ret %d)\n",
			ret);
		Gen_process_exception ();
		return (ORPGPGT_ERROR);

	    }

	    Gen_en_registered = 1;

	}
	
	if (!Gen_LB_registered) {

	    status = ORPGDA_UN_register( ORPGDAT_PROD_INFO,
	                             PD_CURRENT_PROD_MSG_ID,
	                             Gen_LB_callback );

	    if( status != LB_SUCCESS )
	    {
	      LE_send_msg( GL_CONFIG,
	                   "ORPGPGT: ORPGDA_UN_register failed (ret %d)\n",
	                   status);
	      Gen_process_exception ();
		return (ORPGPGT_ERROR);
	    }

	    Gen_LB_registered = 1;

	}
	
	/*  Set compression for the product info data buffer */
	 ORPGMISC_set_compression(ORPGDAT_PROD_INFO);	 

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

		Gen_default_A_tbl_num   = 0;
		Gen_default_A_tbl_size  = 0;

		while (1) {

		    if (Gen_default_A_tbl != (char *) NULL) {

			free (Gen_default_A_tbl);

		    }
		
		    Gen_default_A_tbl_num = 0;

		    status = ORPGDA_read (ORPGDAT_PROD_INFO,
				  &Gen_default_A_tbl,
				  LB_ALLOC_BUF,
				  PD_DEFAULT_A_PROD_MSG_ID);

		    if (status < 0) {

			LE_send_msg (GL_INPUT,
			    "ORPGPGT: ORPGDA_read PD_DEFAULT_A_PROD_MSG_ID failed (ret %d)\n",
			    status);

			Gen_process_exception ();
			return (ORPGPGT_ERROR);

		    }

		    Gen_default_A_tbl_size = status;
		    Gen_default_A_tbl_num  = status/sizeof (Pd_prod_entry);
		    break;

		}

		Gen_default_A_update = 0;
		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

		Gen_default_B_tbl_num   = 0;
		Gen_default_B_tbl_size  = 0;

		while (1) {

		    if (Gen_default_B_tbl != (char *) NULL) {

			free (Gen_default_B_tbl);

		    }
		
		    Gen_default_B_tbl_num = 0;

		    status = ORPGDA_read (ORPGDAT_PROD_INFO,
				  &Gen_default_B_tbl,
				  LB_ALLOC_BUF,
				  PD_DEFAULT_B_PROD_MSG_ID);

		    if (status < 0) {

			LE_send_msg (GL_INPUT,
			    "ORPGPGT: ORPGDA_read PD_DEFAULT_B_PROD_MSG_ID failed (ret %d)\n",
			    status);

			Gen_process_exception ();
			return (ORPGPGT_ERROR);

		    }

		    Gen_default_B_tbl_size = status;
		    Gen_default_B_tbl_num  = status/sizeof (Pd_prod_entry);
		    break;

		}

		Gen_default_B_update = 0;
		break;

	    case ORPGPGT_CURRENT_TABLE :

		Gen_current_tbl_num     = 0;
		Gen_current_tbl_size    = 0;

		while (1) {

		    if (Gen_current_tbl != (char *) NULL) {

			free (Gen_current_tbl);

		    }
		
		    Gen_current_tbl_num = 0;

		    Gen_LB_update_flag = 0;

		    status = ORPGDA_read (ORPGDAT_PROD_INFO,
				  &Gen_current_tbl,
				  LB_ALLOC_BUF,
				  PD_CURRENT_PROD_MSG_ID);

		    if (status < 0) {

		        Gen_LB_update_flag = 1;

			LE_send_msg (GL_INPUT,
			    "ORPGPGT: ORPGDA_read PD_CURRENT_PROD_MSG_ID failed (ret %d)\n",
			    status);

			Gen_process_exception ();
			return (ORPGPGT_ERROR);

		    }

		    Gen_current_tbl_size = status;
		    Gen_current_tbl_num  = status/sizeof (Pd_prod_entry);
		    break;

		}

		Gen_current_update = 0;
		break;

	    default :

		return ORPGPGT_ERROR;

	}

	return (0);
}

/************************************************************************
 *									*
 *	Description:  This function returns a pointer to the generation	*
 *		      table record in the specified table (current or	*
 *		      current).					*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE 			*
 *		      ndx   - index (record number) of the table entry	*
 *									*
 *	Return:	      A NULL pointer on failure or a pointer to the	*
 *		      specified record on success.			*
 ************************************************************************/

char
*ORPGPGT_get_tbl_ptr (
int	table,
int	ndx
)
{
	switch (table) {

	    case ORPGPGT_CURRENT_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_current_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_current_update) {

			return ((char *) NULL);

		    }
		}

		if (Gen_current_tbl == (char *) NULL) {

		    return (char *) NULL;

		} else {

		    return (char *) (Gen_current_tbl +
				     sizeof (Pd_prod_entry)*ndx);

		}

	    case ORPGPGT_DEFAULT_A_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_A_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_A_update) {

			return ((char *) NULL);

		    }
		}

		if (Gen_default_A_tbl == (char *) NULL) {

		    return (char *) NULL;

		} else {

		    return (char *) (Gen_default_A_tbl +
				     sizeof (Pd_prod_entry)*ndx);

		}

	    case ORPGPGT_DEFAULT_B_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_B_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_B_update) {

			return ((char *) NULL);

		    }
		}

		if (Gen_default_B_tbl == (char *) NULL) {

		    return (char *) NULL;

		} else {

		    return (char *) (Gen_default_B_tbl +
				     sizeof (Pd_prod_entry)*ndx);

		}


	}

	return (char *) NULL;
}

/************************************************************************
 *									*
 *	Description:  This function looks for the first occurrance of	*
 *		      of the input butter number is the specified	*
 *		      product generation table.				*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE   	 	*
 *		      buf   - the ORPG buffer number to look for	*
 *									*
 *	Return:	      This function returns the index of the entry in	*
 *		      in the specified table on success.  On failure	*
 *		      ORPGPGT_ERROR or -1 is returned.			*
 *									*
 ************************************************************************/

int
ORPGPGT_buf_in_tbl (
int	table,
int	buf
)
{
	int	i;
	int	size;
	int	found;
	char		*ptr;
	Pd_prod_entry	*gen_tbl;

	found = -1;

/*	Loop through the desired product generation table until the 	*
 *	buffer number matches a table entry.  Return the table index	*
 *	associated with the product.					*/

	switch (table) {

	    case ORPGPGT_CURRENT_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_current_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_current_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		size = Gen_current_tbl_num;
		break;

	    case ORPGPGT_DEFAULT_A_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_A_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_A_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		size = Gen_default_A_tbl_num;
		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_B_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_B_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		size = Gen_default_B_tbl_num;
		break;

	    default:

		return (ORPGPGT_ERROR);

	}

	for (i=0;i<size;i++) {

	    ptr     = (char *) ORPGPGT_get_tbl_ptr (table, i);
	    gen_tbl = (Pd_prod_entry *) ptr;
	
	    if (buf == gen_tbl->prod_id) {

		found = i;
		break;

	    }
	}

	return found;

}

/************************************************************************
 *									*
 *	Description:  This routine modifies the archive field in the	*
 *		      specified product generation table.		*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE 			*
 *		      ndx   - the record number in the table.		*
 *		      val   - The new archive period.			*
 *									*
 *	Return:	      The value "val" is returned on success.  		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_set_archive_interval (
int	table,
int	ndx,
int	val
)
{
	char		*ptr;
	Pd_prod_entry	*gen;

/*	Get the pointer to the specified record and table.  If not	*
 *	a valid record or table, return ORPGPGT_ERROR.			*/

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

		if ((ndx >= 0) && (ndx < Gen_default_A_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

		if ((ndx >= 0) && (ndx < Gen_default_B_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

		if ((ndx >= 0) && (ndx < Gen_current_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

            default :
               return (ORPGPGT_ERROR);

	}

/*	Check to see if the change value is within allowed range	*/
/*	NOTE:  The archive interval is special since it needs to	*
 *	allow negative values.  Negative values indicate archive	*
 *	the lowest N cuts.  Positive values indicate the archive	*
 *	interval.							*/

	if ((val < ORPGPGT_MIN_ARCHIVE_INTERVAL) ||
	    (val > ORPGPGT_MAX_ARCHIVE_INTERVAL)) {

	    return (ORPGPGT_INVALID_DATA);

/*	    The archive interval must be an even multiple of the	*
 *	    generation interval.  If not, return an error.		*/

	} else {

	    if (val > 0) {

		if (val%gen->gen_pr) {

		    return (ORPGPGT_INVALID_DATA);

		}
	    }
	}

	gen->arch_pr = val;

	return val;
}

/************************************************************************
 *									*
 *	Description:  This routine modifies the generation field in the	*
 *		      specified product generation table.		*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE			*
 *		      ndx   - the record number in the table.		*
 *		      val   - The new generation period.		*
 *									*
 *	Return:	      The value "val" is returned on success. 		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_set_generation_interval (
int	table,
int	ndx,
int	val
)
{
	char		*ptr;
	Pd_prod_entry	*gen;

/*	Get the pointer to the specified record and table.  If not	*
 *	a valid record or table, return ORPGPGT_ERROR.			*/

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

		if ((ndx >= 0) && (ndx < Gen_default_A_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

		if ((ndx >= 0) && (ndx < Gen_default_B_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

		if ((ndx >= 0) && (ndx < Gen_current_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

             default :
                return (ORPGPGT_ERROR);

	}

/*	Check to see if the change value is within allowed range	*/

	if ((val < ORPGPGT_MIN_GENERATION_INTERVAL) ||
	    (val > ORPGPGT_MAX_GENERATION_INTERVAL)) {

	    return (ORPGPGT_INVALID_DATA);

	}

	gen->gen_pr = val;

	return val;
}

/************************************************************************
 *									*
 *	Description:  This routine modifies the storage field in the	*
 *		      specified product generation table.		*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE			*
 *		      ndx   - the record number in the table.		*
 *		      val   - The new storage period.			*
 *									*
 *	Return:	      The value "val" is returned on success. 		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_set_storage_interval (
int	table,
int	ndx,
int	val
)
{
	char		*ptr;
	Pd_prod_entry	*gen;

/*	Get the pointer to the specified record and table.  If not	*
 *	a valid record or table, return ORPGPGT_ERROR.			*/

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

		if ((ndx >= 0) && (ndx < Gen_default_A_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

		if ((ndx >= 0) && (ndx < Gen_default_B_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

		if ((ndx >= 0) && (ndx < Gen_current_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

            default :
               return (ORPGPGT_ERROR);

	}

/*	Check to see if the change value is within allowed range	*/

	if ((val < ORPGPGT_MIN_STORAGE_INTERVAL) ||
	    (val > ORPGPGT_MAX_STORAGE_INTERVAL) ||
	    (gen->gen_pr <= 0)) {

	    return (ORPGPGT_INVALID_DATA);

/*	    The storage interval must be an even multiple of the	*
 *	    generation interval.  If not, return an error.		*/

	} else {

	    if (val%gen->gen_pr) {

		return (ORPGPGT_INVALID_DATA);

	    }
	}

	gen->stor_pr = val;

	return val;
}

/************************************************************************
 *									*
 *	Description:  This routine modifies the retention field in the	*
 *		      specified product generation table.		*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE			*
 *		      ndx   - the record number in the table.		*
 *		      val   - The new retention period.			*
 *									*
 *	Return:	      The value "val" is returned on success.		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_set_retention_period (
int	table,
int	ndx,
int	val
)
{
	char		*ptr;
	Pd_prod_entry	*gen;

/*	Check to see if the change value is within allowed range	*/

	if (val < 0) {

	    return (ORPGPGT_ERROR);

	}

/*	Get the pointer to the specified record and table.  If not	*
 *	a valid record or table, return ORPGPGT_ERROR.			*/

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

		if ((ndx >= 0) && (ndx < Gen_default_A_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

		if ((ndx >= 0) && (ndx < Gen_default_B_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

		if ((ndx >= 0) && (ndx < Gen_current_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

            default :
               return (ORPGPGT_ERROR);

	}

/*	Check to see if the change value is within allowed range	*/

	if (((gen->gen_pr > 0) &&
		((val < ORPGPGT_MIN_PERIOD) ||
		 (val > ORPGPGT_MAX_PERIOD))) ||
	    ((gen->gen_pr <= 0) &&
		(val != 0))) {

	    return (ORPGPGT_INVALID_DATA);

	}

	gen->stor_retention = val;

	return val;
}

/************************************************************************
 *									*
 *	Description:  This routine modifies the buffer num field in the	*
 *		      specified product generation table.		*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE			*
 *		      ndx   - the record number in the table.		*
 *		      val   - The new buffer number.			*
 *									*
 *	Return:	      The value "val" is returned on success.		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_set_prod_id (
int	table,
int	ndx,
int	val
)
{
	char		*ptr;
	Pd_prod_entry	*gen;

/*	Check to see if the change value is within allowed range	*/

	if (val < 0) {

	    return (ORPGPGT_ERROR);

	}

/*	Get the pointer to the specified record and table.  If not	*
 *	a valid record or table, return ORPGPGT_ERROR.			*/

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

		if ((ndx >= 0) && (ndx < Gen_default_A_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

		if ((ndx >= 0) && (ndx < Gen_default_B_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

		if ((ndx >= 0) && (ndx < Gen_current_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

            default :
               return (ORPGPGT_ERROR);

	}

	gen->prod_id = val;

	return val;
}

/************************************************************************
 *									*
 *	Description:  This routine returns the scaled parameter field	*
 *		      int the specified product generation table.	*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE			*
 *		      ndx   - the record number in the table.		*
 *		      param - The parameter number to update		*
 *			      	range: 0 to 5				*
 *									*
 *	Return:	      ORPGPGT_ERROR is returned on failure.		*
 *		      On success, the value or one of the special	*
 *		      parameter values is returned.			*
 ************************************************************************/

int
ORPGPGT_get_parameter (
int	table,
int	ndx,
int	param
)
{
	int		buf_num;
	int		i;
	int		found;
	int		num_params;
	char		*ptr;
	Pd_prod_entry	*gen;

/*	Check to see if the parameter is a valid one			*/

	buf_num = ORPGPGT_get_prod_id (table, ndx);

/*	If no parameters are associated with the product, then return	*
 *	with PARAM_UNUSED.						*/

	num_params = ORPGPAT_get_num_parameters (buf_num);

	found = -1;

	if (num_params <= 0) {

	    return ((float) PARAM_UNUSED);

/*	Else, check to see if the parameter number is valid for the	*
 *	product.  If so, proceed.  Otherwise, return with PARAM_UNUSED.	*/

	} else {

	    for (i=0;i<num_params;i++) {

		if (param == ORPGPAT_get_parameter_index (buf_num, i)) {

		    found = i;
		    break;

		}
	    }
	}

	if (found < 0) {

	    return ((float) PARAM_UNUSED);

	}

/*	Get the pointer to the specified record and table.  If not	*
 *	a valid record or table, return ORPGPGT_ERROR.			*/

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_A_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_A_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_default_A_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_B_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_B_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_default_B_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_current_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_current_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_current_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    default:

		return (ORPGPGT_ERROR);

	}

	return ((int) gen->params [param]);

}

/************************************************************************
 *									*
 *	Description:  This routine modifies a parameter field in the	*
 *		      specified product generation table.		*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE			*
 *		      ndx   - the record number in the table.		*
 *		      param - The parameter number to update		*
 *			      	range: 0 to 5				*
 *		      val   - The new parameter value.			*
 *									*
 *	Return:	      The value 0 is returned on success.		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_set_parameter (
int	table,
int	ndx,
int	param,
int	val
)
{
	int		buf_num;
	char		*mne;
	int		i;
	int		found;
	int		num_params;
	char		*ptr;
	Pd_prod_entry	*gen;
	int		min = -1;
	int		max = -1;

/*	Check to see if the parameter is a valid one			*/

	buf_num = ORPGPGT_get_prod_id (table, ndx);

/*	If no parameters are associated with the product, then return	*
 *	with error.							*/

	num_params = ORPGPAT_get_num_parameters (buf_num);

	found = -1;

	if (num_params <= 0) {

	    return (ORPGPGT_ERROR);

/*	Else, check to see if the parameter number is valid for the	*
 *	product.  If so, proceed.  Otherwise, return with error.	*/

	} else {

	    for (i=0;i<num_params;i++) {

		if (param == ORPGPAT_get_parameter_index (buf_num, i)) {

		    found = i;
		    min   = ORPGPAT_get_parameter_min   (buf_num, i);
		    max   = ORPGPAT_get_parameter_max   (buf_num, i);
		    break;

		}
	    }
	}

	if (found < 0) {

	    return (ORPGPGT_ERROR);

	}

/*	Check for the special conditions (i.e., SRR and SRM product	*
 *	types).							*/

	mne = ORPGPAT_get_mnemonic (buf_num);

	if (((!strncmp (mne,"SRR",3) && (val < 0))  ||
	     (!strncmp (mne,"SRM",3) && (val < 0))) &&
	    ((param == 3) || (param == 4))) {

	    if (val == -10) {

		val = PARAM_ALG_SET;

	    } else {

		return (ORPGPGT_ERROR);

	    }

	} else {

/*	Check to see if the change value is within allowed range	*/

	    if ((val < PARAM_UNUSED) || (val > PARAM_MAX_SPECIAL))  {

		if ((val < min) || (val > max)) {
         
                    int elev_param = ORPGPAT_elevation_based( buf_num );

                    if( elev_param >= 0 ){

			if( (val & ORPGPRQ_ELEV_FLAG_BITS) == 0 )
			    return (ORPGPGT_ERROR);

                    }
                    else 
			return (ORPGPGT_ERROR);
		}
	    }
	}

/*	Get the pointer to the specified record and table.  If not	*
 *	a valid record or table, return ORPGPGT_ERROR.			*/

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

		if ((ndx >= 0) && (ndx < Gen_default_A_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

		if ((ndx >= 0) && (ndx < Gen_default_B_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

		if ((ndx >= 0) && (ndx < Gen_current_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    default:
		return (ORPGPGT_ERROR);

	}

	gen->params [param] = (short) val;

	return 0;
}

/************************************************************************
 *									*
 *	Description:  This routine returns the buffer number associated	*
 *		      with an entry in the specified product generation	*
 *		      table.						*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE			*
 *		      ndx   - the record number in the table.		*
 *									*
 *	Return:	      A positive value is returned on success.		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_get_prod_id (
int	table,
int	ndx
)
{
	char		*ptr;
	Pd_prod_entry	*gen;

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_A_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_A_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_default_A_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_B_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_B_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_default_B_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_current_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_current_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_current_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

            default :
               return (ORPGPGT_ERROR);

	}

	return gen->prod_id;

}

/************************************************************************
 *									*
 *	Description:  This routine returns the archive interval assoc.	*
 *		      with an entry in the specified product generation	*
 *		      table.						*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE			*
 *		      ndx   - the record number in the table.		*
 *									*
 *	Return:	      A positive value is returned on success.		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_get_archive_interval (
int	table,
int	ndx
)
{
	char		*ptr;
	Pd_prod_entry	*gen;

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_A_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_A_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_default_A_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_B_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_B_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_default_B_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_current_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_current_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_current_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

            default :
               return (ORPGPGT_ERROR);

	}

	return gen->arch_pr;

}

/************************************************************************
 *									*
 *	Description:  This routine returns the generation interval	*
 *		      associated with an entry in the specified product	*
 *		      generation table.					*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE			*
 *		      ndx   - the record number in the table.		*
 *									*
 *	Return:	      A positive value is returned on success.		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_get_generation_interval (
int	table,
int	ndx
)
{
	char		*ptr;
	Pd_prod_entry	*gen;

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_A_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_A_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_default_A_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_B_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_B_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_default_B_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_current_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_current_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_current_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

            default :
               return (ORPGPGT_ERROR);

	}

	return gen->gen_pr;

}

/************************************************************************
 *									*
 *	Description:  This routine returns the storage interval assoc.	*
 *		      with an entry in the specified product generation	*
 *		      table.						*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE			*
 *		      ndx   - the record number in the table.		*
 *									*
 *	Return:	      A positive value is returned on success.		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_get_storage_interval (
int	table,
int	ndx
)
{
	char		*ptr;
	Pd_prod_entry	*gen;

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_A_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_A_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_default_A_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_B_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_B_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_default_B_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_current_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_current_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_current_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

            default:
               return (ORPGPGT_ERROR);

	}

	return gen->stor_pr;

}

/************************************************************************
 *									*
 *	Description:  This routine returns the retention period assoc.	*
 *		      with an entry in the specified product generation	*
 *		      table.						*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE			*
 *		      ndx   - the record number in the table.		*
 *									*
 *	Return:	      A positive value is returned on success.		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_get_retention_period (
int	table,
int	ndx
)
{
	char		*ptr;
	Pd_prod_entry	*gen;

	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_A_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_A_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_default_A_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_B_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_B_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_default_B_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

	    case ORPGPGT_CURRENT_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_current_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_current_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((ndx >= 0) && (ndx < Gen_current_tbl_num))  {

		    ptr = (char *) ORPGPGT_get_tbl_ptr (table, ndx);
		    gen = (Pd_prod_entry *) ptr;

		} else {

		    return (ORPGPGT_ERROR);

		}

		break;

            default :
               return (ORPGPGT_ERROR);

	}

	return gen->stor_retention;

}

/************************************************************************
 *									*
 *	Description:  The following functions returns the number of	*
 *		      elements in the specified product generaion	*
 *		      table.						*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE 			*
 *									*
 *	Return:	      A positive value is returned on success.		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_get_tbl_num (
int	table
)
{
	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_A_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_A_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		return Gen_default_A_tbl_num;

	    case ORPGPGT_DEFAULT_B_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_B_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_B_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		return Gen_default_B_tbl_num;

	    case ORPGPGT_CURRENT_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_current_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_current_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		return Gen_current_tbl_num;

	    default :

		return (ORPGPGT_ERROR);

	}
}

/************************************************************************
 *									*
 *	Description:  The following functions returns the size (in	*
 *		      bytes) of the specified product generation table	*
 *		      message.						*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE 			*
 *									*
 *	Return:	      A positive value is returned on success.		*
 *		      ORPGPGT_ERROR is returned on failure.		*
 *									*
 ************************************************************************/

int
ORPGPGT_get_tbl_size (
int	table
)
{
	switch (table) {

	    case ORPGPGT_DEFAULT_A_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_A_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_A_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		return Gen_default_A_tbl_size;

	    case ORPGPGT_DEFAULT_B_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_B_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_B_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		return Gen_default_B_tbl_size;

	    case ORPGPGT_CURRENT_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_current_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_current_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		return Gen_current_tbl_size;

	    default :

		return (ORPGPGT_ERROR);

	}
}

/************************************************************************
 *									*
 *	Description:  The following functions adds an entry to the	*
 *		      specified product generation table.		*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE 			*
 *									*
 *	Return:	      The index in the table for the new entry is	*
 *		      returned on success.  ORPGPGT_ERROR is returned	*
 *		      on failure.					*
 *									*
 ************************************************************************/


int
ORPGPGT_add_entry (
int	table
)
{
	int	i;
	int	indx;
	int	size;

	size = sizeof (Pd_prod_entry);

	switch (table) {

	    case ORPGPGT_CURRENT_TABLE :

		indx = Gen_current_tbl_size;

	        Gen_current_tbl_size = Gen_current_tbl_size + size;
		Gen_current_tbl = (char *) realloc (Gen_current_tbl,
						    Gen_current_tbl_size);

/*		Clear the memory for the new entry.			*/

		for (i=0;i<size;i++) {

		    Gen_current_tbl [indx+i] = 0;

		}

		Gen_current_tbl_num++;
		Gen_current_update = 0;
		Gen_LB_update_flag = 0;

		return (Gen_current_tbl_num - 1);

	    case ORPGPGT_DEFAULT_A_TABLE :

		indx = Gen_default_A_tbl_size;

	        Gen_default_A_tbl_size = Gen_default_A_tbl_size + size;
		Gen_default_A_tbl = (char *) realloc (Gen_default_A_tbl,
						     Gen_default_A_tbl_size);

/*		Clear the memory for the new entry.			*/

		for (i=0;i<size;i++) {

		    Gen_default_A_tbl [indx+i] = 0;

		}

		Gen_default_A_tbl_num++;
		Gen_default_A_update = 0;
		return (Gen_default_A_tbl_num - 1);

	    case ORPGPGT_DEFAULT_B_TABLE :

		indx = Gen_default_B_tbl_size;

	        Gen_default_B_tbl_size = Gen_default_B_tbl_size + size;
		Gen_default_B_tbl = (char *) realloc (Gen_default_B_tbl,
						     Gen_default_B_tbl_size);

/*		Clear the memory for the new entry.			*/

		for (i=0;i<size;i++) {

		    Gen_default_B_tbl [indx+i] = 0;

		}

		Gen_default_B_tbl_num++;
		Gen_default_B_update = 0;
		return (Gen_default_B_tbl_num - 1);

	    default :

		return (ORPGPGT_ERROR);

	}
}

/************************************************************************
 *									*
 *	Description:  The following functions deletes an entry to the	*
 *		      specified product generation table.		*
 *									*
 *	Inputs:	      table - the ID of the requested generation table  *
 *			      ORPGPGT_CURRENT_TABLE or			*
 *			      ORPGPGT_DEFAULT_A_TABLE or		*
 *			      ORPGPGT_DEFAULT_B_TABLE 			*
 *		      indx  - index of entry in the table to be deleted	*
 *									*
 *	Return:	      0 is returned on success, ORPGPGT_ERROR on failure*
 *									*
 ************************************************************************/

int
ORPGPGT_delete_entry (
int	table,
int	indx
)
{
	int	i;
	int	size;

	switch (table) {

	    case ORPGPGT_CURRENT_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_current_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_current_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((indx >= Gen_current_tbl_num)  ||
		    (indx <  0)) { 

		    return (ORPGPGT_ERROR);

		}

/*		If there is any data after the record to be deleted	*
 *		move everything past it up and reallocate memory.	*/

		size = sizeof (Pd_prod_entry);

		if (indx < (Gen_current_tbl_num - 1)) {

		    for (i=(indx+1)*sizeof (Pd_prod_entry);
			 i<Gen_current_tbl_size;
			 i++) {

			Gen_current_tbl [i-size] = Gen_current_tbl [i];

		    }
		}

		Gen_current_tbl_num--;
		Gen_current_tbl_size = Gen_current_tbl_size - size;
		Gen_current_tbl = (char *) realloc (Gen_current_tbl,
				Gen_current_tbl_size);

		if (Gen_current_tbl == (char *) NULL) {

		    return (ORPGPGT_ERROR);

		}

		return (0);

	    case ORPGPGT_DEFAULT_A_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_A_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_A_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((indx >= Gen_default_A_tbl_num)  ||
		    (indx <  0)) { 

		    return (ORPGPGT_ERROR);

		}

/*		If there is any data after the record to be deleted	*
 *		move everything past it up and reallocate memory.	*/

		size = sizeof (Pd_prod_entry);

		if (indx < (Gen_default_A_tbl_num - 1)) {

		    for (i=(indx+1)*sizeof (Pd_prod_entry);
			 i<Gen_default_A_tbl_size;
			 i++) {

			Gen_default_A_tbl [i-size] = Gen_default_A_tbl [i];

		    }
		}

		Gen_default_A_tbl_num--;
		Gen_default_A_tbl_size = Gen_default_A_tbl_size - size;
		Gen_default_A_tbl = (char *) realloc (Gen_default_A_tbl,
				Gen_default_A_tbl_size);

		if (Gen_default_A_tbl == (char *) NULL) {

		    return (ORPGPGT_ERROR);

		}

		return (0);

	    case ORPGPGT_DEFAULT_B_TABLE :

/*		First check to see if table need to be updated.  If so,	*
 *		read them from the LB.					*/

		if (Gen_default_B_update) {

		    ORPGPGT_read_tbl (table);

/*		    If the table wasn't updated, then return an error.	*/

		    if (Gen_default_B_update) {

			return (ORPGPGT_ERROR);

		    }
		}

		if ((indx >= Gen_default_B_tbl_num)  ||
		    (indx <  0)) { 

		    return (ORPGPGT_ERROR);

		}

/*		If there is any data after the record to be deleted	*
 *		move everything past it up and reallocate memory.	*/

		size = sizeof (Pd_prod_entry);

		if (indx < (Gen_default_B_tbl_num - 1)) {

		    for (i=(indx+1)*sizeof (Pd_prod_entry);
			 i<Gen_default_B_tbl_size;
			 i++) {

			Gen_default_B_tbl [i-size] = Gen_default_B_tbl [i];

		    }
		}

		Gen_default_B_tbl_num--;
		Gen_default_B_tbl_size = Gen_default_B_tbl_size - size;
		Gen_default_B_tbl = (char *) realloc (Gen_default_B_tbl,
				Gen_default_B_tbl_size);

		if (Gen_default_B_tbl == (char *) NULL) {

		    return (ORPGPGT_ERROR);

		}

		return (0);

	    default :

		return (ORPGPGT_ERROR);

	}
}

/************************************************************************
 *									*
 *	Description:  The following module writes the specified product	*
 *		      generation table to the product generation lb.	*
 *									*
 *	Return:	      On success, 0 is returned, otherwise		*
 *		      ORPGPGT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPGT_write_tbl (
int	table
)
{
	int	status;
	int	table_id;

	table_id = table;
	
	/*  Set compression for the product info data buffer */
	 ORPGMISC_set_compression(ORPGDAT_PROD_INFO);	 
	
	switch (table) {

	    case ORPGPGT_CURRENT_TABLE :

		status = ORPGDA_write (ORPGDAT_PROD_INFO,
				      Gen_current_tbl,
				      Gen_current_tbl_size,
				      PD_CURRENT_PROD_MSG_ID);
		if (status <= 0) {

		    LE_send_msg (GL_LB (status),
		    	"ORPGPGT: ORPGDA_write PD_CURRENT_PROD_MSG_ID (ret %d)\n",
			status);
		    return (-1);
		
		} else {

/*		    Post an event to indicate that the current product	*
 *		    generation table has been successfully updated.	*/

		    EN_post (ORPGEVT_PROD_LIST,
			     (char *) &table_id, 4,
			     EN_POST_FLAG_DONT_NTFY_SENDER);

		}

		break;

	    case ORPGPGT_DEFAULT_A_TABLE :

		status = ORPGDA_write (ORPGDAT_PROD_INFO,
				      Gen_default_A_tbl,
				      Gen_default_A_tbl_size,
				      PD_DEFAULT_A_PROD_MSG_ID);

		if (status <= 0) {

		    LE_send_msg (GL_LB (status),
		    	"ORPGPGT: ORPGDA_write PD_DEFAULT_A_PROD_MSG_ID (ret %d)\n",
			status);
		    return (-1);

		} else {

/*		    Post an event to indicate that the default_A product*
 *		    generation table has been successfully updated.	*/

		    EN_post (ORPGEVT_PROD_LIST,
			     (char *) &table_id, 4,
			     EN_POST_FLAG_DONT_NTFY_SENDER);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

		status = ORPGDA_write (ORPGDAT_PROD_INFO,
				      Gen_default_B_tbl, 
				      Gen_default_B_tbl_size,
				      PD_DEFAULT_B_PROD_MSG_ID);
		if (status <= 0) {

		    LE_send_msg (GL_LB (status),
		    	"ORPGPGT: ORPGDA_write PD_DEFAULT_B_PROD_MSG_ID (ret %d)\n",
			status);
		    return (-1);

		} else {

/*		    Post an event to indicate that the default_B product*
 *		    generation table has been successfully updated.	*/

		    EN_post (ORPGEVT_PROD_LIST,
			     (char *) &table_id, 4,
			     EN_POST_FLAG_DONT_NTFY_SENDER);

		}

		break;

	    default :

		return (-1);

	}

	return 0;
}

/************************************************************************
 *									*
 *	Description:  The following module copies the input table msg	*
 *		      to the output table msg buffers (does not write	*
 *		      to lb).						*
 *									*
 *	Input:        table1 - The id of the source table (read)	*
 *		      table2 - The id of the destination table (write)	*
 *									*
 *	Return:	      On success, 0 is returned, otherwise		*
 *		      ORPGPGT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPGT_copy_tbl (
int	table1,
int	table2
)
{
	int	status;
	int	table_id;

	table_id = table2;
	status   = ORPGPGT_ERROR;
	
	switch (table2) {

	    case ORPGPGT_CURRENT_TABLE :

		switch (table1) {

		    case ORPGPGT_DEFAULT_A_TABLE :

			if (Gen_default_A_tbl == (char *) NULL) {

			    status = ORPGPGT_read_tbl (table1);

			    if (status < 0) {

				return ORPGPGT_ERROR;

			    }
			}

			if (Gen_current_tbl != (char *) NULL) {

			    free (Gen_current_tbl);

			}

			Gen_current_tbl_size = Gen_default_A_tbl_size;
			Gen_current_tbl_num  = Gen_default_A_tbl_num;

			if (Gen_current_tbl_size > 0) {

			    Gen_current_tbl = (char *) calloc (Gen_current_tbl_size, 1);

			    if (Gen_current_tbl == (char *) NULL) {

			        LE_send_msg (GL_ERROR,
				"ORPGPGT_replace_tbl (ORPGPGT_DEFAULT_A_TABLE,ORPGPGT_CURRENT_TABLE) calloc failed");
			        return (ORPGPGT_ERROR);

			    }

			    memcpy (Gen_current_tbl, Gen_default_A_tbl, Gen_current_tbl_size);

			}

			status = 0;
			break;

		    case ORPGPGT_DEFAULT_B_TABLE :

			if (Gen_default_B_tbl == (char *) NULL) {

			    status = ORPGPGT_read_tbl (table1);

			    if (status < 0) {

				return ORPGPGT_ERROR;

			    }
			}

			if (Gen_current_tbl != (char *) NULL) {

			    free (Gen_current_tbl);

			}

			Gen_current_tbl_size = Gen_default_B_tbl_size;
			Gen_current_tbl_num  = Gen_default_B_tbl_num;

			if (Gen_current_tbl_size > 0) {

			    Gen_current_tbl = (char *) calloc (Gen_current_tbl_size, 1);

			    if (Gen_current_tbl == (char *) NULL) {

			        LE_send_msg (GL_ERROR,
				"ORPGPGT_replace_tbl (ORPGPGT_DEFAULT_B_TABLE,ORPGPGT_CURRENT_TABLE) calloc failed");
			        return (ORPGPGT_ERROR);

			    }

			    memcpy (Gen_current_tbl, Gen_default_B_tbl, Gen_current_tbl_size);

			}

			status = 0;
			break;

		}
		break;

	    case ORPGPGT_DEFAULT_A_TABLE :

		switch (table1) {

		    case ORPGPGT_CURRENT_TABLE :

			if (Gen_current_tbl == (char *) NULL) {

			    status = ORPGPGT_read_tbl (table1);

			    if (status < 0) {

				return ORPGPGT_ERROR;

			    }
			}

			if (Gen_default_A_tbl != (char *) NULL) {

			    free (Gen_default_A_tbl);

			}

			Gen_default_A_tbl_size = Gen_current_tbl_size;
			Gen_default_A_tbl_num  = Gen_current_tbl_num;

			if (Gen_default_A_tbl_size > 0) {

			    Gen_default_A_tbl = (char *) calloc (Gen_default_A_tbl_size, 1);

			    if (Gen_default_A_tbl == (char *) NULL) {

			        LE_send_msg (GL_ERROR,
				"ORPGPGT_replace_tbl (ORPGPGT_CURRENT_TABLE,ORPGPGT_DEFAULT_A_TABLE) calloc failed");
			        return (ORPGPGT_ERROR);

			    }

			    memcpy (Gen_default_A_tbl, Gen_current_tbl, Gen_default_A_tbl_size);

			}

			status = 0;
			break;

		    case ORPGPGT_DEFAULT_B_TABLE :

			if (Gen_default_B_tbl == (char *) NULL) {

			    status = ORPGPGT_read_tbl (table1);

			    if (status < 0) {

				return ORPGPGT_ERROR;

			    }
			}

			if (Gen_default_A_tbl != (char *) NULL) {

			    free (Gen_default_A_tbl);

			}

			Gen_default_A_tbl_size = Gen_default_B_tbl_size;
			Gen_default_A_tbl_num  = Gen_default_B_tbl_num;

			if (Gen_default_A_tbl_size > 0) {

			    Gen_default_A_tbl = (char *) calloc (Gen_default_A_tbl_size, 1);

			    if (Gen_default_A_tbl == (char *) NULL) {

			        LE_send_msg (GL_ERROR,
				"ORPGPGT_replace_tbl (ORPGPGT_DEFAULT_B_TABLE,ORPGPGT_DEFAULT_A_TABLE) calloc failed");
			        return (ORPGPGT_ERROR);

			    }

			    memcpy (Gen_default_A_tbl, Gen_default_B_tbl, Gen_default_A_tbl_size);

			}

			status = 0;
			break;

		}
		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

		switch (table1) {

		    case ORPGPGT_CURRENT_TABLE :

			if (Gen_current_tbl == (char *) NULL) {

			    status = ORPGPGT_read_tbl (table1);

			    if (status < 0) {

				return ORPGPGT_ERROR;

			    }
			}

			if (Gen_default_B_tbl != (char *) NULL) {

			    free (Gen_default_B_tbl);

			}

			Gen_default_B_tbl_size = Gen_current_tbl_size;
			Gen_default_B_tbl_num  = Gen_current_tbl_num;

			if (Gen_default_B_tbl_size > 0) {

			    Gen_default_B_tbl = (char *) calloc (Gen_default_B_tbl_size, 1);

			    if (Gen_default_B_tbl == (char *) NULL) {

			        LE_send_msg (GL_ERROR,
				"ORPGPGT_replace_tbl (ORPGPGT_CURRENT_TABLE,ORPGPGT_DEFAULT_B_TABLE) calloc failed");
			        return (ORPGPGT_ERROR);

			    }

			    memcpy (Gen_default_B_tbl, Gen_current_tbl, Gen_default_B_tbl_size);

			}

			status = 0;
			break;

		    case ORPGPGT_DEFAULT_A_TABLE :

			if (Gen_default_A_tbl == (char *) NULL) {

			    status = ORPGPGT_read_tbl (table1);

			    if (status < 0) {

				return ORPGPGT_ERROR;

			    }
			}

			if (Gen_default_B_tbl != (char *) NULL) {

			    free (Gen_default_B_tbl);

			}

			Gen_default_B_tbl_size = Gen_default_A_tbl_size;
			Gen_default_B_tbl_num  = Gen_default_A_tbl_num;

			if (Gen_default_B_tbl_size > 0) {

			    Gen_default_B_tbl = (char *) calloc (Gen_default_B_tbl_size, 1);

			    if (Gen_default_B_tbl == (char *) NULL) {

			        LE_send_msg (GL_ERROR,
				"ORPGPGT_replace_tbl (ORPGPGT_DEFAULT_A_TABLE,ORPGPGT_DEFAULT_B_TABLE) calloc failed");
			        return (ORPGPGT_ERROR);

			    }

			    memcpy (Gen_default_B_tbl, Gen_default_A_tbl, Gen_default_B_tbl_size);

			}

			status = 0;
			break;

		}
		break;

	}

	return (status);
}

/************************************************************************
 *									*
 *	Description:  The following module writes the input table msg	*
 *		      to the output table msg in the product generation	*
 *		      lb.						*
 *									*
 *	Input:        table1 - The id of the source table (read)	*
 *		      table2 - The id of the destination table (write)	*
 *									*
 *	Return:	      On success, 0 is returned, otherwise		*
 *		      ORPGPGT_ERROR is returned.			*
 *									*
 ************************************************************************/

int
ORPGPGT_replace_tbl (
int	table1,
int	table2
)
{
	int	status;
	int	table_id;

	table_id = table2;
	
	/*  Set compression for the product info data buffer */
	 ORPGMISC_set_compression(ORPGDAT_PROD_INFO);	 
	
	switch (table2) {

	    case ORPGPGT_CURRENT_TABLE :

		switch (table1) {

		    case ORPGPGT_DEFAULT_A_TABLE :

			if ((Gen_default_A_tbl == (char *) NULL) ||
			    (Gen_default_A_update)) {

			    status = ORPGPGT_read_tbl (table1);

			    if (status < 0) {

				return ORPGPGT_ERROR;

			    }
			}

			status = ORPGDA_write (ORPGDAT_PROD_INFO,
				      Gen_default_A_tbl,
				      Gen_default_A_tbl_size,
				      PD_CURRENT_PROD_MSG_ID);

			if (status < 0) {

			    LE_send_msg (GL_LB (status),
			    	"ORPGPGT: ORPGDA_write PD_CURRENT_PROD_MSG_ID (ret %d)\n",
				status);
			    return (-1);

			}

			Gen_current_update = 1;

			break;

		    case ORPGPGT_DEFAULT_B_TABLE :

			if ((Gen_default_B_tbl == (char *) NULL) ||
			    (Gen_default_B_update)) {

			    status = ORPGPGT_read_tbl (table1);

			    if (status < 0) {

				return ORPGPGT_ERROR;

			    }
			}

			status = ORPGDA_write (ORPGDAT_PROD_INFO,
				      Gen_default_B_tbl,
				      Gen_default_B_tbl_size,
				      PD_CURRENT_PROD_MSG_ID);

			if (status < 0) {

			    LE_send_msg (GL_LB (status),
			    	"ORPGPGT: ORPGDA_write PD_CURRENT_PROD_MSG_ID (ret %d)\n",
				status);
			    return (-1);

			}

			Gen_current_update = 1;

			break;

		    default:

			return (-1);

		}

		break;

	    case ORPGPGT_DEFAULT_A_TABLE :

		switch (table1) {

		    case ORPGPGT_CURRENT_TABLE :

			if ((Gen_current_tbl == (char *) NULL) ||
			    (Gen_current_update)) {

			    status = ORPGPGT_read_tbl (table1);

			    if (status < 0) {

				return ORPGPGT_ERROR;

			    }
			}

			status = ORPGDA_write (ORPGDAT_PROD_INFO,
				      Gen_current_tbl,
				      Gen_current_tbl_size,
				      PD_DEFAULT_A_PROD_MSG_ID);

			if (status < 0) {

			    LE_send_msg (GL_LB (status),
			    	"ORPGPGT: ORPGDA_write PD_DEFAULT_A_PROD_MSG_ID (ret %d)\n",
				status);
			    return (-1);

			}

			break;

		    case ORPGPGT_DEFAULT_B_TABLE :

			if ((Gen_default_B_tbl == (char *) NULL) ||
			    (Gen_default_B_update)) {

			    status = ORPGPGT_read_tbl (table1);

			    if (status < 0) {

				return ORPGPGT_ERROR;

			    }
			}

			status = ORPGDA_write (ORPGDAT_PROD_INFO,
				      Gen_default_B_tbl,
				      Gen_default_B_tbl_size,
				      PD_DEFAULT_A_PROD_MSG_ID);

			if (status < 0) {

			    LE_send_msg (GL_LB (status),
			    	"ORPGPGT: ORPGDA_write PD_DEFAULT_A_PROD_MSG_ID (ret %d)\n",
				status);
			    return (-1);

			}

			break;

		    default:

			return (-1);

		}

		break;

	    case ORPGPGT_DEFAULT_B_TABLE :

		switch (table1) {

		    case ORPGPGT_CURRENT_TABLE :

			if ((Gen_current_tbl == (char *) NULL) ||
			    (Gen_current_update)) {

			    status = ORPGPGT_read_tbl (table1);

			    if (status < 0) {

				return ORPGPGT_ERROR;

			    }
			}

			status = ORPGDA_write (ORPGDAT_PROD_INFO,
				      Gen_current_tbl,
				      Gen_current_tbl_size,
				      PD_DEFAULT_B_PROD_MSG_ID);

			if (status < 0) {

			    LE_send_msg (GL_LB (status),
			    	"ORPGPGT: ORPGDA_write PD_DEFAULT_B_PROD_MSG_ID (ret %d)\n",
				status);
			    return (-1);

			}

			break;

		    case ORPGPGT_DEFAULT_A_TABLE :

			if ((Gen_default_A_tbl == (char *) NULL) ||
			    (Gen_default_A_update)) {

			    status = ORPGPGT_read_tbl (table1);

			    if (status < 0) {

				return ORPGPGT_ERROR;

			    }
			}

			status = ORPGDA_write (ORPGDAT_PROD_INFO,
				      Gen_default_A_tbl,
				      Gen_default_A_tbl_size,
				      PD_DEFAULT_B_PROD_MSG_ID);

			if (status < 0) {

			    LE_send_msg (GL_LB (status),
			    	"ORPGPGT: ORPGDA_write PD_DEFAULT_B_PROD_MSG_ID (ret %d)\n",
				status);
			    return (-1);

			}

			break;

		    default:

			return (-1);

		}

		break;

	    default :

		return (-1);

	}

/*	Post an event to indicate that the destination product		*
 *	generation table has been successfully updated.			*/

	EN_post (ORPGEVT_PROD_LIST,
	     (char *) &table_id, 4, 0);

	return 0;
}

