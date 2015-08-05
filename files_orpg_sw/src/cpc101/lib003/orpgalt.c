/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/07/08 13:48:31 $
 * $Id: orpgalt.c,v 1.24 2009/07/08 13:48:31 steves Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */

/************************************************************************
 *                                                                      *
 *	Module:       orpgalt.c			                        *
 *                                                                      *
 *	Description:  This module contains a collection of functions	*
 *                to manipulate alert threshold data.                   *
 *                                                                      *
 ************************************************************************/



#include <stdio.h>
#include <stdlib.h>            /* free(), calloc()                        */
#include <string.h>

#include <infr.h>
#include <orpg.h>
#include <orpgalt.h>
#include <rpg_port.h>
#include <rss_replace.h>
#include <alert_threshold.h>
#include <orpgmisc.h>

/*	Static variables						*/

static	int	Init_flag            = 0;  /* 0 = Msg needs to be read
                                              1 = Msg not to be read */
static	int	Group_init_flag      = ORPGALT_FALSE; 
static	int	Group_tbl_size       = 0;  /* Size (bytes) of group table. */
static	int	Number_of_categories = 0;  /* Number of category defs */
static	int	Number_of_groups     = ORPGALT_NUM_GROUPS;  /* Num of groups */
static	char	*Category_names      = NULL; /* Category names */
static	char	*Category            = NULL; /* Pointer to category table */
static	char	*Group               = NULL; /* Pointer to group table */

static	int	En_registered = 0;
static	void	(*User_exception_callback)() = NULL;	/* User exception
					callback function.	*/
static  int	io_status = 0;
						
/*	Local functions.	*/
static	void	En_callback (int lbfd, LB_id_t msgid, int msglen, void *arg);
static	void	Process_exception ();
static  int     Get_categories ();
static  int     Fill_category_buffer ();
static  int     Write_category_data ();




/************************************************************************

    Description: Return the status of the last I/O operation

    Return:      none
 
 ************************************************************************/
int 	ORPGALT_io_status()
{
	return(io_status);
}

/************************************************************************

    Description: The following routine returns a pointer to the group
                 name specified by the function argument.

    Input:       group_id - group identifier

    Return:      group name on success, NULL pointer on failure.

 ************************************************************************/

char
*ORPGALT_get_group_name (
int	group_id
)
{
	int	i;
	int	status;
	orpg_alert_threshold_group_t	*group;

	if (!Init_flag) {

	    status = ORPGALT_read ();
	    if (status == ORPGALT_READ_FAILED) {

		return (char *) NULL;

	    }
	}

	for (i=0;i<ORPGALT_groups ();i++) {

	    group = (orpg_alert_threshold_group_t *) 
			(Group + i*sizeof (orpg_alert_threshold_group_t));

	    if (group->id == group_id) {

		return (char *) group->name;

	    }
	}

	return (char *) NULL;

} /* End ORPGALT_get_group_name() */


/************************************************************************

    Description: The following routine returns a pointer to the name 
                 descriptor specified by the function argument.
 
    Input:       category_id - category identifier

    Return:      category name on success, NULL pointer on failure.

 ************************************************************************/

char
*ORPGALT_get_name (
int	category_id
)
{
	int	i;
	int	status;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();
	    if (status == ORPGALT_READ_FAILED) {

		return (char *) NULL;

	    }
	}

	for (i=0;i<Number_of_categories;i++) {

	    category = (orpg_alert_threshold_data_t *)
				(Category + i*sizeof (orpg_alert_threshold_data_t));

	    if (category_id == category->category) {

		return (char *) category->name;

	    }
	}

	return (char *) NULL;
}


/************************************************************************

    Description: The following routine returns a pointer to the units
                 descriptor specified by the function argument. 
 
    Input:       category_id - category identifier

    Return:      units descriptor on success, NULL pointer on failure.

 ************************************************************************/

char
*ORPGALT_get_unit (
int	category_id
)
{
	int	i;
	int	status;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();
	    if (status == ORPGALT_READ_FAILED) {

		return (char *) NULL;

	    }
	}

	for (i=0;i<Number_of_categories;i++) {

	    category = (orpg_alert_threshold_data_t *)
				(Category + i*sizeof (orpg_alert_threshold_data_t));

	    if (category_id == category->category) {

		return (char *) category->unit;

	    }

	}

	return (char *) NULL;
}


/************************************************************************

    Description: The following routine returns the group number for an 
                 alert theshold index.
  
    Input:       category - category identifier

    Return:      alert group number on success, OROGALT_INVALID_CATEGORY
		 or ORPGALT_READ_FAILED on failure.

 ************************************************************************/

int
ORPGALT_get_group (
int	category_id
)
{
	int	i;
	int	status;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();
	
	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	for (i=0;i<Number_of_categories;i++) {

	    category = (orpg_alert_threshold_data_t *)
				(Category + i*sizeof (orpg_alert_threshold_data_t));

	    if (category_id == category->category) {

		return category->group;

	    }
	}

	return ORPGALT_INVALID_CATEGORY;
}


/************************************************************************

    Description: The following routine returns the category number for 
                 an alert theshold index.
  
    Input:       indx - threshold table index

    Return:      alert category number on success, ORPGALT_INVALID_INDEX
		 or ORPGALT_READ_FAILED on failure.

 ************************************************************************/

int
ORPGALT_get_category (
int	indx
)
{
	int	status;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();

	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	if ((indx < 0) || (indx > Number_of_categories)) {

	    return ORPGALT_INVALID_INDEX;

	} else {

	    category = (orpg_alert_threshold_data_t *)
				(Category + indx*sizeof (orpg_alert_threshold_data_t));
	    return category->category;

	}
}


/************************************************************************

    Description: The following routine returns the group number for 
                 an group index.
  
    Input:       indx - group index

    Return:      group identifier on success, ORPGALT_INVALID_INDEX
		 or ORPGALT_READ_FAILED on failure.

 ************************************************************************/

int
ORPGALT_get_group_id (
int	indx
)
{
	int	status;
	orpg_alert_threshold_group_t	*group;

	if (!Init_flag) {

	    status = ORPGALT_read ();

	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	if ((indx < 0) || (indx > Number_of_groups)) {

	    return ORPGALT_INVALID_INDEX;

	} else {

	    group = (orpg_alert_threshold_group_t *)
				(Group + indx*sizeof (orpg_alert_threshold_group_t));
	    return group->id;

	}

} /* End ORPGALT_get_group_id() */


/************************************************************************

    Description: The following routine returns the security level for an 
                 alert theshold category.
  
    Input:       category_id - category identifier

    Return:      LOCA (bitmap) value on success, ORPGALT_INVALID_CATEGORY
		 or ORPGALT_READ_FAILED on failure.

 ************************************************************************/

int
ORPGALT_get_loca (
int	category_id
)
{
	int	i;
	int	status;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();

	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	for (i=0;i<Number_of_categories;i++) {

	    category = (orpg_alert_threshold_data_t *)
				(Category + i*sizeof (orpg_alert_threshold_data_t));

	    if (category_id == category->category) {

		return category->loca;

	    }
	}

	return ORPGALT_INVALID_CATEGORY;
}

/************************************************************************

    Description: The following routine returns the minimum value for an 
                 alert theshold index.
  
    Input:       category_id - category identifier

    Return:      minimum alert value on success, ORPGALT_INVALID_CATEGORY
		 or ORPGALT_READ_FAILED on failure.

 ************************************************************************/

int
ORPGALT_get_min (
int	category_id
)
{
	int	i;
	int	status;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();

	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	for (i=0;i<Number_of_categories;i++) {

	    category = (orpg_alert_threshold_data_t *)
				(Category + i*sizeof (orpg_alert_threshold_data_t));

	    if (category_id == category->category) {

		return category->min;

	    }
	}

	return ORPGALT_INVALID_CATEGORY;
}


/************************************************************************

    Description: The following routine returns the maximum value for an 
                 alert theshold index.
  
    Input:       category_id - category identifier

    Return:      maximum alert value on success, ORPGALT_INVALID_CATEGORY
		 or ORPGALT_READ_FAILED on failure.

 ************************************************************************/

int
ORPGALT_get_max (
int	category_id
)
{
	int	i;
	int	status;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();
	    
	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	for (i=0;i<Number_of_categories;i++) {

	    category = (orpg_alert_threshold_data_t *)
				(Category + i*sizeof (orpg_alert_threshold_data_t));

	    if (category_id == category->category) {

		return category->max;

	    }
	}

	return ORPGALT_INVALID_CATEGORY;
}

/************************************************************************

    Description: The following routine returns the number of thresholds 
                 for an alert threshold category.
  
    Input:       category_id - category identifier

    Return:      number of thresholds defined on success,
		 ORPGALT_INVALID_CATEGORY or ORPGALT_READ_FAILED
		 on failure.

 ************************************************************************/

int
ORPGALT_get_thresholds (
int	category_id
)
{
	int	i;
	int	status;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();

	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	for (i=0;i<Number_of_categories;i++) {

	    category = (orpg_alert_threshold_data_t *)
				(Category + i*sizeof (orpg_alert_threshold_data_t));

	    if (category_id == category->category) {

		return category->num_thresh;

	    }
	}

	return ORPGALT_INVALID_CATEGORY;
}

/************************************************************************

    Description: The following routine returns the product identifier 
                 for an alert threshold category.
  
    Input:       category_id - categoy identifier

    Return:      product id on success, ORPGALT_INVALID_CATEGORY
		 or ORPGALT_READ_FAILED on failure.

 ************************************************************************/

int
ORPGALT_get_prod_code (
int	category_id
)
{
	int	i;
	int	status;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();

	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	for (i=0;i<Number_of_categories;i++) {

	    category = (orpg_alert_threshold_data_t *)
				(Category + i*sizeof (orpg_alert_threshold_data_t));

	    if (category_id == category->category) {

		return category->prod_code;

	    }
	}

	return ORPGALT_INVALID_CATEGORY;
}


/************************************************************************

    Description: The following routine sets the product identifier for an
                 alert threshold category.
  
    Inputs:      category_id - category identifier
                 id   - Product identifier 

    Output:      ORPGALT_SUCCESS on success, ORPGALT_INVALID_CATEGORY
		 or ORPGALT_READ_FAILED on failure.

 ************************************************************************/

int
ORPGALT_set_prod_code (
int	category_id,
int	id
)
{
	int	i;
	int	status;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();

	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	if ((id < ORPGALT_PRODUCT_ID_MIN) ||
	    (id > ORPGALT_PRODUCT_ID_MAX)) {

	    return ORPGALT_INVALID_DATA;

	}

	for (i=0;i<Number_of_categories;i++) {

	    category = (orpg_alert_threshold_data_t *)
				(Category + i*sizeof (orpg_alert_threshold_data_t));

	    if (category_id == category->category) {

		category->prod_code = id;
		return ORPGALT_SUCCESS;

	    }
	}

	return ORPGALT_INVALID_CATEGORY;
}

/************************************************************************

    Description: The following routine returns the product type(s) 
                 allowed for an alert threshold category.
  
    Input:       category_id - categoy identifier

    Return:      product type(s), or ORPGALT_INVALID_CATEGORY if
		 index is out of range or ORPGALT_READ_FAILED .
		 	bit 1 set = VOLUME products
			bit 2 set = Elevation products
			bit 3 set = Hydromet Products

 ************************************************************************/

int
ORPGALT_get_type (
int	category_id
)
{
	int	i;
	int	status;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();

	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	for (i=0;i<Number_of_categories;i++) {

	    category = (orpg_alert_threshold_data_t *)
				(Category + i*sizeof (orpg_alert_threshold_data_t));

	    if (category_id == category->category) {

		return category->type;

	    }
	}

	return ORPGALT_INVALID_CATEGORY;
}

/************************************************************************

	Description: This function registers a callback function which
		     will be called when an exception condition is
		     encountered.

	Input:	     user_exception_callback - the user's callback
		     function.

************************************************************************/

void
ORPGALT_error (
void	(*user_exception_callback) ()
)
{

	User_exception_callback = user_exception_callback;
	return;

}

/************************************************************************

    Description: This function is called when an exception condition
		 is detected.

************************************************************************/

static void
Process_exception ()
{

	if (User_exception_callback == NULL) {

	    LE_send_msg (GL_INFO | GL_TERM,"ORPGALT: process exception");

	} else {

	    User_exception_callback ();

	}
}

/************************************************************************

    Description: The following routine handles alert threshold table
		 update events.

    Input: fd       - LB file descriptor (unused)
	   msg_id   - ID of message which was updated (unused)
	   msg_info - length of new message in bytes (unused)
	   arg      - user registered argument (unused)

    NOTE: The only thing this LB notification callback does is set
	  a flag to indicate the message has been updated so other
	  functions know to read it when they are called.
************************************************************************/

static void
En_callback (
int	fd,
LB_id_t	msgid,
int	msg_info,
void	*arg
)
{
	Init_flag = 0;
}

/************************************************************************
    Description: The following routine reads the alert threshold 
                 adaptation data table.

    Return:      return non-negative integer on success.
                 return negative integer on failure
 ************************************************************************/
int ORPGALT_read ()
{
   int status;


   /* Check to see if alert threshold table update events have	*
    * been registered.  If not, register for them.		*/

   if (!En_registered) 
   {
      status = DEAU_UN_register ("Alert", En_callback);

      if ( status < 0 )
      {
         LE_send_msg (GL_INFO,
            "ORPGALT: DEAU_UN_register failed (ret %d)\n", status);
         Process_exception ();

         En_registered = 1;
         io_status = status;
         return (-1);
      }

      En_registered = 1;
   }

   /* If Group data not initialized, do that now */
   if ( Group_init_flag == ORPGALT_FALSE )
   {
      status = ORPGALT_init_groups();
      if ( status < 0 )
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT_read: error initializing group data (err=%d)\n", status);
         return (-1);
      }
   }

   /* Compute the size of the alert/threshold category table so we *
    * can allocate memory for it.                                  */

   if (Category != (char *) NULL) 
   {
      free (Category);
      Category = NULL;
   }

   if (Category_names != (char *) NULL) 
   {
      free (Category_names);
      Category_names = NULL;
   }

   Category = (char *) calloc ( ALERT_THRESHOLD_CATEGORIES *
      sizeof(orpg_alert_threshold_data_t), 1);

   Category_names =
     (char *) calloc ( (size_t) ORPGALT_CATEGORY_LIST_SIZE, (size_t) 1);

   /* Determine number of categories */
   status = Get_categories();
   if ( status < 0 )
   {
      LE_send_msg(GL_ERROR,
         "ORPGALT: error getting categories (err=%d)\n", status);
      return (-1);
   }

   Init_flag = 1;
   status = Fill_category_buffer();

   io_status = status;
   if (status < 0) 
   {
      LE_send_msg (GL_INFO,
         "ORPGALT: filling category buffer failed (err=%d)\n", status);
      free (Category);
      Category = (char *) NULL;
      free (Category_names);
      Category_names = (char *) NULL;
      Init_flag = 0;
      return ORPGALT_READ_FAILED;
   }
   return status;

} /* End ORPGALT_read() */


/************************************************************************

    Description: The following routine updates the alert threshold 
                 adaptation data table.
      
    Return:      status - return size in bytes of structure on success, 
                          or value < 0 on failure (see lb.h)

 ************************************************************************/
int ORPGALT_write ()
{
   int	status;

   if (!Init_flag) 
   {
      return ORPGALT_WRITE_FAILED;
   }
	
   status = Write_category_data();

   io_status = status;
	
   if (status < 0)
   {
      LE_send_msg (GL_INFO, "ORPGDA_write (ORPGALT_CATEGORY_MSG_ID): %d\n", status);
      return ORPGALT_WRITE_FAILED;
   }
   else
   {
      /* Post an event so that everyone else will know that this *
       * data store has been updated.                            */
      EN_post (ORPGEVT_WX_ALERT_ADAPT_UPDATE, NULL, 0, 0);
   }

   return status;
}


/************************************************************************

    Description: This routine gets the threshold value for a specific
                 threshold identifier and index.

    Inputs:      category_id - category identifier
                 item - threshold identifier

    Return:      value - return threshold value for this category,
                         or -1 on failure

 ************************************************************************/

int
ORPGALT_get_threshold (
int	category_id,
int	item
)
{
	int	status;  /* status returned from the threshold read */
	int	value;
	int	i;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();
	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	for (i=0;i<Number_of_categories;i++) {

	    category = (orpg_alert_threshold_data_t *)
				(Category + i*sizeof (orpg_alert_threshold_data_t));

	    if (category_id == category->category) {

		switch (item) {

		    case 1 :

			value = category->thresh_1;
			break;

		    case 2 :

			value = category->thresh_2;
			break;

		    case 3 :

			value = category->thresh_3;
			break;

		    case 4 :

			value = category->thresh_4;
			break;

		    case 5 :

			value = category->thresh_5;
			break;

		    case 6 :

			value = category->thresh_6;
			break;

		    default :

			return ORPGALT_INVALID_INDEX;

		}

		return value;

	    }
	}

	return ORPGALT_INVALID_CATEGORY;

}

/************************************************************************

    Description: This routine sets the threshold value for a specific
                 threshold identifier and index.

    Inputs:      category_id - category identifier
                 item  - threshold identifier
                 value - the value to set the threshold to

 ************************************************************************/

int
ORPGALT_set_threshold (
int	category_id,
int	item,
int	value
)
{
	int	i;
	int	status;
	orpg_alert_threshold_data_t	*category;

	if (!Init_flag) {

	    status = ORPGALT_read ();
	    
	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	for (i=0;i<Number_of_categories;i++) {

	    category = (orpg_alert_threshold_data_t *)
				(Category + i*sizeof (orpg_alert_threshold_data_t));

	    if (category_id == category->category) {

		if ((value >= category->min) &&
		    (value <= category->max)) {

		    switch (item) {

			case 1 :

			    category->thresh_1 = value;
			    break;

			case 2 :

			    category->thresh_2 = value;
		    	    break;

			case 3 :

			    category->thresh_3 = value;
			    break;

			case 4 :

			    category->thresh_4 = value;
			    break;

			case 5 :

			    category->thresh_5 = value;
			    break;

			case 6 :

			    category->thresh_6 = value;
			    break;

			default :

			    return ORPGALT_INVALID_INDEX;

		    }

		    return ORPGALT_SUCCESS;

		} else {

		    return ORPGALT_INVALID_DATA;

		}
	    }
	}

	return ORPGALT_INVALID_CATEGORY;

}

/************************************************************************

    Description: This routine returns the number of alert threshold
		 categories in the table.

    Inputs:      None
    Return:	 Number of alert threshold categories (>=0)

 ************************************************************************/

int
ORPGALT_categories (
)
{
	int	status;

	if (!Init_flag) {

	    status = ORPGALT_read ();

	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	return Number_of_categories;
}

/************************************************************************

    Description: This routine returns the number of alert threshold
		 groups in the table.

    Inputs:      None
    Return:	 Number of alert threshold groups (>=0)

 ************************************************************************/

int
ORPGALT_groups (
)
{
	int	status;

	if (!Init_flag) {

	    status = ORPGALT_read ();

	    if (status == ORPGALT_READ_FAILED) {

		return status;

	    }
	}

	return Number_of_groups;
}


/********************************************************************

    Description: 
        This function gathers the legacy alerting adaptation data
        from the DEA database and fills the buffer supplied by the 
        caller.

    Return:	
        Non-negative integer on success.
        Negative integer on failure.

********************************************************************/
int ORPGALT_update_legacy (char* data)
{
   int                         status;
   int                         i;
   int                         id;
   int                         ind;
   alert_threshold_t           *tbl;
   orpg_alert_threshold_data_t *category;


   /* Read new data if necessary */
   if (!Init_flag) 
   {
      status = ORPGALT_read ();
	    
      if (status == ORPGALT_READ_FAILED) 
      {
         return(status);
      }
   }

   /* Copy ORPG alert threshold data to legacy structure. */

   tbl = (alert_threshold_t *) (data);

   for (i=0;i<Number_of_categories;i++) 
   {
      category = (orpg_alert_threshold_data_t *)
         (Category + i*sizeof (orpg_alert_threshold_data_t));
      id = category->category;
      ind = id - 1;

      tbl->data [ind].group = ORPGALT_get_group (id);
      tbl->data [ind].category = id;
      tbl->data [ind].num_thresh = ORPGALT_get_thresholds (id);
      tbl->data [ind].thresh_1 = ORPGALT_get_threshold (id, 1);
      tbl->data [ind].thresh_2 = ORPGALT_get_threshold (id, 2);
      tbl->data [ind].thresh_3 = ORPGALT_get_threshold (id, 3);
      tbl->data [ind].thresh_4 = ORPGALT_get_threshold (id, 4);
      tbl->data [ind].thresh_5 = ORPGALT_get_threshold (id, 5);
      tbl->data [ind].thresh_6 = ORPGALT_get_threshold (id, 6);
      tbl->data [ind].prod_code = ORPGALT_get_prod_code (id);
   }

   return (0);

} /* End of ORPGALT_update_legacy() */


/********************************************************************

    Description: 
        This function copies the alerting table "default" values in
        the DEA database to the "current" values.

    Return:	
        Non-negative integer on success.
        Negative integer on failure.

********************************************************************/
int ORPGALT_restore_baseline ()
{
   int i              = 0;
   int status         = 0;
   int ret            = 0;
   int n_branches     = 0;
   char* branch_list  = NULL;
   char field_name[50];


   n_branches = DEAU_get_branch_names (ALERTING_DEA_NAME, &branch_list);
   if ( n_branches < 0 )
   {
      LE_send_msg( GL_ERROR,
         "ORPGALT_restore_baseline: error getting branch names (err=%d)\n",
         n_branches);
      return (-1);
   }

   for ( i = 0; i < n_branches; i++ )
   {
      /* restore threshold values */
      strcpy( field_name, ALERTING_DEA_NAME );
      strcat( field_name, ".");
      strcat( field_name, branch_list );
      strcat( field_name, ".thresh" );
      status = DEAU_move_baseline( field_name, 0 );
      if (status < 0 )
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error restoring baseline threshold values (err=%d)\n",
            status);
         ret = -1;
      }

      /* restore paired product values */
      strcpy( field_name, ALERTING_DEA_NAME );
      strcat( field_name, ".");
      strcat( field_name, branch_list );
      strcat( field_name, ".prod_code" );
      status = DEAU_move_baseline( field_name, 0 );
      if (status < 0 )
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error restoring baseline prod_code values (err=%d)\n",
            status);
         ret = -1;
      }

      /* increment branch list pointer */
      branch_list = branch_list + strlen(branch_list) + 1;
   }

   return ret;

} /* End ORPGALT_restore_baseline() */


/********************************************************************

    Description: 
        This function initializes the Group data.  This data used to
        be part of the configuration file.  When we moved to using
        the DEA database, we decided the Group data didn't need to
        be put into the DB because it will rarely change, if ever.

    Return:	
        Non-negative integer on success.
        Negative integer on failure.

********************************************************************/
int ORPGALT_init_groups ()
{
   int                          len           = 0;
   int                          ret           = 0;
   char                         temp_str[100];
   orpg_alert_threshold_group_t groups[3];

   /* Clean Group buffer if not already clean */
   if (Group != (char *) NULL)
   {
      free (Group);
   }

   /* Determine the size of the alert/threshold group table so we *
    * can allocate memory for it.                                 */

   Group_tbl_size = Number_of_groups * sizeof(orpg_alert_threshold_group_t);
   Group = (char *) calloc ( Group_tbl_size , 1 );

   /* Assign structure elements */
   groups[0].id = (short) ORPGALT_GROUP1_ID;
   strcpy(temp_str, ORPGALT_GROUP1_NAME);
   strcpy(temp_str, "\0");
   len = strlen(temp_str);
   if ( len < ALERT_THRESHOLD_NAME_LEN )
   {
      strcpy(groups[0].name, ORPGALT_GROUP1_NAME);
   }
   else
   {
      LE_send_msg(GL_ERROR, "ORPGALT_init_groups: group name too long\n");
      return (-1);
   }
   groups[1].id = (short) ORPGALT_GROUP2_ID;
   strcpy(groups[1].name, ORPGALT_GROUP2_NAME);
   groups[2].id = (short) ORPGALT_GROUP3_ID;
   strcpy(groups[2].name, ORPGALT_GROUP3_NAME);

   /* Copy structure data into Group buffer */
   memcpy((void *) Group, &groups, Group_tbl_size );

   return ret;

} /* End ORPGALT_init_groups() */


/********************************************************************

    Description: 
        This private function locates the unique categories in the 
        DEA database and fills the Category_names global buffer with
        the list of unique names separated by null terminators.  It
        also determines the actual number of defined Alert Threshold
        Categories and sets the global Number_of_categories variable.

    Return:	
        On success, returns non-negative integer equal to number of
           categories
        On failure, returns negative integer 

********************************************************************/
static int Get_categories()
{
   int   i            = 0;
   int   len          = 0;
   int   n_branches   = 0;
   char* ptr          = NULL;
   char* branch_list  = NULL;


   n_branches = DEAU_get_branch_names (ALERTING_DEA_NAME, &branch_list);
   if ( n_branches < 0 )
   {
      LE_send_msg( GL_ERROR,
         "ORPGALT (Get_categories): error getting branch names (err=%d)\n",
         n_branches);
      return (-1);
   }

   /* Initialize global num categories variable */
   Number_of_categories = 0;

   ptr = Category_names;

   for ( i = 0; i < n_branches; i++ )
   {
      /* record string length */
      len = strlen( branch_list );

      /* check to see if branch is a new branch name */
      if ( strstr( Category_names, branch_list) == NULL )
      {
         /* add new name */
         strcpy( ptr, branch_list );

         /* increment character pointer */
         ptr = ptr + len + 1;
         
         /* increment category counter */
         Number_of_categories++;
      }

      /* increment branch_list ptr */
      branch_list = branch_list + len + 1;
   }

   return Number_of_categories;

} /* End Get_categories() */


/********************************************************************

    Description: 
        This private function reads the DEA database and fills the
        buffer used to hold the category data.

    Return:	
        Non-negative integer on success.
        Negative integer on failure.

********************************************************************/
static int Fill_category_buffer ()
{
   int                          i,ii,j         = 0;
   int                          ret            = 0;
   int                          name_len       = 0;
   int                          min_max_len    = 0;
   char                         field_name[50];
   char*                        cat_name       = NULL;
   char*                        min_max_ptr    = NULL;
   char*                        cat_name_ptr   = NULL;
   orpg_alert_threshold_data_t* cat_struct     = NULL;
   DEAU_attr_t* attr;

   static double*               val_buf        = NULL;

   /* allocate space for values buffer - 20 is sufficiently large */
   if( val_buf == NULL )
      val_buf = (double *) malloc( (size_t) (20 * sizeof(double)) );

   if( val_buf == NULL )
   {

      LE_send_msg( GL_ERROR, "malloc Failed for %d bytes\n", 20*sizeof(double) );
      return (-1);

   }

   cat_name_ptr = Category_names;

   /* loop through category DB names */
   for (i = 0; i < Number_of_categories; i++ ) 
   {
      name_len = strlen( cat_name_ptr );

      /* cast pointer to category data structure */
      cat_struct = (orpg_alert_threshold_data_t*)
         (Category + (i * sizeof( orpg_alert_threshold_data_t )));

      /*** get database values and assign structure fields ***/

      strcpy(field_name, ALERTING_DEA_NAME);
      strcat(field_name, ".");
      strcat(field_name, cat_name_ptr);
      strcat(field_name, ".group");
      if ( (ret = DEAU_get_values(field_name, val_buf, 1)) > 0 )
      {
         cat_struct->group = (short)(*val_buf);
      }
      else
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error filling category buffer group field (err=%d)\n",
            ret);
         return (-1);
      }

      strcpy(field_name, ALERTING_DEA_NAME);
      strcat(field_name, ".");
      strcat(field_name, cat_name_ptr);
      strcat(field_name, ".category");
      if ( (ret = DEAU_get_values(field_name, val_buf, 1)) > 0 )
      {
         cat_struct->category = (short)(*val_buf);
      }
      else
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error filling category buffer category field (err=%d)\n",
            ret);
         return (-1);
      }

      strcpy(field_name, ALERTING_DEA_NAME);
      strcat(field_name, ".");
      strcat(field_name, cat_name_ptr);
      strcat(field_name, ".thresh");
      if ( (ret = DEAU_get_number_of_values(field_name)) > 0 )
      {
         cat_struct->num_thresh = ret;
      }
      else
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error filling category buffer num_thresh field (err=%d)\n",
            ret);
         return (-1);
      }

      /* for the threshold values, since there are a variable number of them
         depending on which category it is, we'll use ptr arithmetic to assign
         the structure values */
      if ((ret=DEAU_get_values(field_name, val_buf, cat_struct->num_thresh)) > 0)
      {
         short* thresh_ptr = &(cat_struct->thresh_1); 
         for ( ii = 0; ii < ret; ii++ )
         {
            *(thresh_ptr + ii) = (short)(*(val_buf + ii));
         }
      }
      else
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error filling category buffer threshold fields (err=%d)\n",
            ret);
         return (-1);
      }

      strcpy(field_name, ALERTING_DEA_NAME);
      strcat(field_name, ".");
      strcat(field_name, cat_name_ptr);
      strcat(field_name, ".prod_code");
      if ( (ret=DEAU_get_values(field_name, val_buf, 1)) > 0 )
      {
         cat_struct->prod_code = (short)(*val_buf);
      }
      else
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error filling category buffer prod_code field (err=%d)\n",
            ret);
         return (-1);
      }

      strcpy(field_name, ALERTING_DEA_NAME);
      strcat(field_name, ".");
      strcat(field_name, cat_name_ptr);
      strcat(field_name, ".prod_type");
      if ( (ret=DEAU_get_values(field_name, val_buf, 1)) > 0 )
      {
         cat_struct->type = (short)(*val_buf);
      }
      else
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error filling category buffer type field (err=%d)\n",
            ret);
         return (-1);
      }

      /* first get the threshold attributes */
      strcpy(field_name, ALERTING_DEA_NAME);
      strcat(field_name, ".");
      strcat(field_name, cat_name_ptr);
      strcat(field_name, ".thresh");
      if ( (ret = DEAU_get_attr_by_id( field_name, &attr )) < 0 )
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error retrieving threshold attributes (err=%d)\n", ret);
         return (-1);
      }

      /* now get the min and max */
      if ( (ret = DEAU_get_min_max_values( attr, &min_max_ptr )) < 0 )
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error retrieving threshold attributes (err=%d)\n", ret);
         return (-1);
      }
      
      /* if the return value is not 0 (i.e., is an interval), then ... */
      if( ret > 0 )
      { 
         /* convert min and max to numeric values */
         min_max_len = strlen (min_max_ptr);
         cat_struct->min = (short) atoi(min_max_ptr);
         cat_struct->max = (short) atoi(min_max_ptr + min_max_len + 1);

      }
      else
      {
         char *tmp_ptr = NULL;

         /* must get allowable values ... assume first is min, last is max. */
         if ( (ret = DEAU_get_allowable_values( attr, &min_max_ptr )) < 0 )
         {
            LE_send_msg( GL_ERROR,
               "ORPGALT: error retrieving threshold attributes (err=%d)\n", ret);
            return (-1);
         }

         /* convert min and max to numeric values */
         tmp_ptr = min_max_ptr;
         for( j = 0; j < cat_struct->num_thresh; j++ ){

            if( j == 0 )
               cat_struct->min = (short) atoi(tmp_ptr);

            if( j == (cat_struct->num_thresh-1) )
               cat_struct->max = (short) atoi(tmp_ptr);

            min_max_len = strlen (tmp_ptr);
            tmp_ptr += (min_max_len + 1);

         }

      }


      /* get LOCA value */
      strcpy(field_name, ALERTING_DEA_NAME);
      strcat(field_name, ".");
      strcat(field_name, cat_name_ptr);
      strcat(field_name, ".loca");
      if ( (ret=DEAU_get_values(field_name, val_buf, 1)) > 0 )
      {
         cat_struct->loca = (short)(*val_buf);
      }
      else
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error filling category buffer loca field (err=%d)\n",
            ret);
         return (-1);
      }

      /* get category name */
      strcpy(field_name, ALERTING_DEA_NAME);
      strcat(field_name, ".");
      strcat(field_name, cat_name_ptr);
      strcat(field_name, ".name");
      if ( (ret=DEAU_get_string_values(field_name, &cat_name )) > 0 )
      {
         strcpy(cat_struct->name, cat_name);
      }
      else
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error filling category buffer name field (err=%d)\n",
            ret);
         return (-1);
      }

      /* get thresh attributes and copy unit string to buffer */
      strcpy(field_name, ALERTING_DEA_NAME);
      strcat(field_name, ".");
      strcat(field_name, cat_name_ptr);
      strcat(field_name, ".thresh");
      if ( (ret = DEAU_get_attr_by_id( field_name, &attr )) < 0 )
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error retrieving threshold attributes (err=%d)\n", ret);
         return (-1);
      }
      strcpy(cat_struct->unit, attr->ats[DEAU_AT_UNIT]);

      /* go to next name in the category names list */
      cat_name_ptr = cat_name_ptr + name_len + 1;
   }

   return ret;

} /* End Fill_category_buffer() */


/********************************************************************

    Description: 
        This private function writes the data in the Category buffer
        to the DEA database 

    Return:	
        Non-negative integer on success.
        Negative integer on failure.

********************************************************************/
static int Write_category_data ()
{
   int                          i            = 0;
   int                          ret          = 0;
   int                          not_str      = 0; /* DEAU function arg */
   int                          not_bline    = 0; /* DEAU function arg */
   int                          cat_name_len = 0; 
   double*                      numeric_data = NULL;
   char                         field_name[100];
   char*                        cat_name_ptr = NULL;
   orpg_alert_threshold_data_t* data;

   cat_name_ptr = Category_names;

   for ( i = 0; i < Number_of_categories; i++)
   {
      /* store category name length */
      cat_name_len = strlen(cat_name_ptr);
      if ( cat_name_len == 0 )
      {
         LE_send_msg(GL_INFO,
            "Write_category_data(): category %d name len = 0.\n", i);
         continue;
      }

      /* cast struct array pointer to the Category buffer */
      data = (orpg_alert_threshold_data_t*)
         (Category + (i * sizeof(orpg_alert_threshold_data_t)));

      /* set category thresholds */
      numeric_data = calloc( ORPGALT_MAX_NUM_THRESH , sizeof(double) );
      numeric_data[0] = (double) data->thresh_1;
      numeric_data[1] = (double) data->thresh_2;
      numeric_data[2] = (double) data->thresh_3;
      numeric_data[3] = (double) data->thresh_4;
      numeric_data[4] = (double) data->thresh_5;
      numeric_data[5] = (double) data->thresh_6;
      strcpy(field_name, ALERTING_DEA_NAME);
      strcat(field_name, ".");
      strcat(field_name, cat_name_ptr);
      strcat(field_name, ".thresh");
      if ( (ret = DEAU_set_values(field_name, not_str, (void *)numeric_data,
         data->num_thresh, not_bline )) < 0 )
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error writing threshold values (err=%d)\n", ret);
         free(numeric_data);
         return (-1);
      } 
      free(numeric_data);

      /* set category product code */
      strcpy(field_name, ALERTING_DEA_NAME);
      strcat(field_name, ".");
      strcat(field_name, cat_name_ptr);
      strcat(field_name, ".prod_code");
      numeric_data = (double *) calloc( 1, sizeof(double) );
      *numeric_data = (double)( data->prod_code );
      if ((ret = DEAU_set_values(field_name, not_str, (void *)numeric_data,
         1, not_bline )) < 0 )
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error writing product code (err=%d)\n", ret);
         free(numeric_data);
         return (-1);
      } 
      free(numeric_data);

      /* increment category name pointer */
      cat_name_ptr = cat_name_ptr + cat_name_len + 1;

   }

   return ret;

} /* End Write_category_data() */


/********************************************************************

    Description: 
        This function copies the alerting table "current" values in
        the DEA database to the "default" values.

    Return:	
        Non-negative integer on success.
        Negative integer on failure.

********************************************************************/
int ORPGALT_update_baseline ()
{
   int i              = 0;
   int status         = 0;
   int ret            = 0;
   int n_branches     = 0;
   char* branch_list  = NULL;
   char field_name[50];


   n_branches = DEAU_get_branch_names (ALERTING_DEA_NAME, &branch_list);
   if ( n_branches < 0 )
   {
      LE_send_msg( GL_ERROR,
         "ORPGALT_restore_baseline: error getting branch names (err=%d)\n",
         n_branches);
      return (-1);
   }

   for ( i = 0; i < n_branches; i++ )
   {
      /* restore threshold values */
      strcpy( field_name, ALERTING_DEA_NAME );
      strcat( field_name, ".");
      strcat( field_name, branch_list );
      strcat( field_name, ".thresh" );
      status = DEAU_move_baseline( field_name, 1 );
      if (status < 0 )
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error restoring baseline threshold values (err=%d)\n",
            status);
         ret = -1;
      }

      /* restore paired product values */
      strcpy( field_name, ALERTING_DEA_NAME );
      strcat( field_name, ".");
      strcat( field_name, branch_list );
      strcat( field_name, ".prod_code" );
      status = DEAU_move_baseline( field_name, 1 );
      if (status < 0 )
      {
         LE_send_msg( GL_ERROR,
            "ORPGALT: error restoring baseline prod_code values (err=%d)\n",
            status);
         ret = -1;
      }

      /* increment branch list pointer */
      branch_list = branch_list + strlen(branch_list) + 1;
   }

   return ret;
} /* End ORPGALT_update_baseline() */


/********************************************************************

    Description: 
        This function releases the lock on the Alerting data in the 
        DEA DB.  Note that the DEA locking mechanism is ADVISORY in
        nature.

    Return:	
        ORPGALT_UNLOCK_SUCCESSFUL on success.
        ORPGALT_UNLOCK_NOT_SUCCESSFUL on failure.

********************************************************************/
int ORPGALT_clear_edit_lock ()
{
   int ret = 0;
   int status = 0;

   status = DEAU_lock_de(ALERTING_DEA_NAME, LB_UNLOCK);
   if ( status < 0 )
   {
      LE_send_msg( GL_ERROR,
         "ORPGALT_clear_edit_lock: error unlocking DB (err=%d)\n", status);
      ret = ORPGALT_UNLOCK_NOT_SUCCESSFUL;
   }
   else
   {
      ret = ORPGALT_UNLOCK_SUCCESSFUL;
   }

   return ret;

} /* End ORPGALT_clear_edit_lock () */


/********************************************************************

    Description: 
        This function locks the Alerting data in the 
        DEA DB so that other applications know it's being edited.
        NOTE: the DEA locking mechanism is ADVISORY in nature.

    Return:	
        ORPGALT_LOCK_SUCCESSFUL on success.
        ORPGALT_LOCK_NOT_SUCCESSFUL on failure.

********************************************************************/
int ORPGALT_set_edit_lock ()
{
   int ret = 0;
   int status = 0;

   status = DEAU_lock_de(ALERTING_DEA_NAME, LB_EXCLUSIVE_LOCK);
   if ( status < 0 )
   {
      LE_send_msg( GL_ERROR,
         "ORPGALT_set_edit_lock: error locking DB (err=%d)\n", status);
      ret = ORPGALT_LOCK_NOT_SUCCESSFUL;
   }
   else
   {
      ret = ORPGALT_LOCK_SUCCESSFUL;
   }

   return ret;

} /* End ORPGALT_set_edit_lock () */


/********************************************************************

    Description: 
        This function tests the lock on the Alerting data in the 
        DEA DB to see whether other applications are editing it.

    Return:	
        ORPGALT_EDIT_UNLOCKED on success.
        ORPGALT_EDIT_LOCKED on failure.

********************************************************************/
int ORPGALT_get_edit_status ()
{
   int ret = 0;
   int status = 0;

   status = DEAU_lock_de(ALERTING_DEA_NAME, LB_TEST_EXCLUSIVE_LOCK);
   if ( status == LB_SUCCESS )
   {
      ret = ORPGALT_EDIT_UNLOCKED;
   }
   else if ( status == LB_HAS_BEEN_LOCKED )
   {
      ret = ORPGALT_EDIT_LOCKED;
   }
   else 
   {
      LE_send_msg( GL_ERROR,
         "ORPGALT_get_edit_status: error getting lock status (err=%d)\n",
         status);
      ret = ORPGALT_FAILURE;
   }

   return ret;

} /* End ORPGALT_get_edit_status () */

/********************************************************************

    Description:
        This function returns a flag to determine if new info is available.

    Return:
        1 if new info is available, 0 otherwise.

********************************************************************/
int ORPGALT_get_update_flag ()
{
   /* Init_flag is 0 if new info is available. The common practice */
   /* is to return 1 if a new read is warranted, so return the     */
   /* complement (!) of Init_flag.                                 */

   return !Init_flag;

} /* End ORPGALT_get_update_flag () */

