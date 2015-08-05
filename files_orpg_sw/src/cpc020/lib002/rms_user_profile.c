/**************************************************************************
   
   Module:  rms_user_profile.c	
   
   Description:  This function returns a pointer to user profiles.
   
      
     

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/10/04 14:13:40 $
 * $Id: rms_user_profile.c,v 1.3 2005/10/04 14:13:40 jing Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define MAX_NAME_SIZE	256
#define PASSWORD 	8
#define USER_NAME	8
/*
* Static Globals
*/


/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  This function finds a user profile.

   Input: Line number and User type

   Output: System log message sent to FAA/RMMs or stored in record log LB

   Returns:  Pointer to a user profile.

   Notes:

   **************************************************************************/
Pd_user_entry *rms_find_user_profile (int line_num, int user_type){

	char name[MAX_NAME_SIZE];
	char *buf;

	Pd_user_entry *up;
	LB_info info;

	int Lb_fd, eol;
	int ret;

	/* Get the prod info LB number */
	ret = CS_entry ((char *)ORPGDAT_PROD_INFO, CS_INT_KEY | ORPGSC_LBNAME_COL,
							MAX_NAME_SIZE, name);
   	if (ret <= 0) {
		/*LE_send_msg (GL_CS(ret), "CS_entry failed (key %d, ret %d)\n",
						ORPGDAT_PROD_INFO, ret);*/
		LE_send_msg (0, "CS_entry failed (key %d, ret %d)\n",
						ORPGDAT_PROD_INFO, ret);
		return (NULL);
    		}

	/* Open the LB */
	Lb_fd = LB_open (name, LB_READ, NULL);
	if (Lb_fd < 0) {
		LE_send_msg (GL_LB(Lb_fd),"LB_open %s failed (ret %d)\n", name, Lb_fd);
		return (NULL);
		}


	/* get the message length */
	if ((ret = LB_seek (Lb_fd, 0, PD_USER_INFO_MSG_ID, &info)) < 0) {
	     LE_send_msg (GL_LB(ret),"LB_seek prod_gd_info failed (ret = %d)", ret);
	     LB_close (Lb_fd);
	     return (NULL);
	}

	/* allocate the buffer */
	buf = (char*) malloc (info.size);


	if (buf == NULL) {
	    LE_send_msg (GL_MEMORY, "malloc failed");
	    return (NULL);
	}

	/* read the message */
	ret = LB_read (Lb_fd, buf, info.size, PD_USER_INFO_MSG_ID);
	if (ret == LB_BUF_TOO_SMALL) {	/* the message is updated again */
	    free (buf);
	    LB_close (Lb_fd);
	    return (NULL);
	}

	if(ret < 0) {
	    LE_send_msg (GL_LB(ret),"LB_read prod_gd_info failed (ret = %d)", ret);
	    LB_close (Lb_fd);
	    return (NULL);
	}

	if(ret == 0)
	    return (NULL);

	/* Find end of user profiles */
	eol = ((int)buf + ret);

	 while (1) {

		/* Current user profile */
		up = (Pd_user_entry *)buf;

		/* Increment buffer to the next user porfile */
		buf += up->entry_size;

		ret = (int)buf + sizeof(Pd_user_entry);


		/* If pointer past end of user profile list exit */
		if (((int)buf + sizeof(Pd_user_entry)) > eol)
			break;

		/* If the user profile is the same as the input line number */
		if (up->line_ind != line_num)
	    		continue;

		/* if the type and line number are the same retunr the pointer */
		if ((up->up_type == user_type && up->line_ind == line_num)) {
			LB_close (Lb_fd);
		 	return (up);
	   	 	}/* user profile found */
   		} /* End loop */

    LB_close (Lb_fd);
    return (NULL);

} /* End rms find user profile*/

/**************************************************************************
   Description:  This function saves the user profile.

   Input: Pointer to a user profile, User type

   Output: User profile saved to adaptation data

   Returns:  0 = Successful save.

   Notes:

   **************************************************************************/

int rms_save_user_profile (Pd_user_entry *in_up, int user_type){

	char name[MAX_NAME_SIZE];
	char *buf;
	char *buf_ptr;

	Pd_user_entry *up;
	LB_info info;

	int Lb_fd, eol;
	int ret, i;

	/* Get the prod info LB name */
	ret = CS_entry ((char *)ORPGDAT_PROD_INFO, CS_INT_KEY | ORPGSC_LBNAME_COL,
							MAX_NAME_SIZE, name);
   	if (ret <= 0) {
		LE_send_msg (0, "CS_entry failed (key %d, ret %d)\n",
						ORPGDAT_PROD_INFO, ret);
		return (NULL);
    		}

	/* Get the LB id */
	Lb_fd = LB_open (name, LB_READ, NULL);

	if (Lb_fd < 0) {
		LE_send_msg (GL_LB(Lb_fd),"LB_open %s failed (ret %d)\n", name, Lb_fd);
		return (NULL);
		}

	/* Get the message length */
	if ((ret = LB_seek (Lb_fd, 0, PD_USER_INFO_MSG_ID, &info)) < 0) {
	     LE_send_msg (GL_LB(ret),"LB_seek prod_gd_info failed (ret = %d)", ret);
	     LB_close (Lb_fd);
	     return (NULL);
	}

	/* allocate the buffer */
	buf = (char*)malloc (info.size);


	if (buf == NULL) {
	    LE_send_msg (GL_MEMORY, "malloc failed");
	    return (NULL);
	}

	/* Read the user profile LB */
	ret = LB_read (Lb_fd, buf, info.size, PD_USER_INFO_MSG_ID);

	if (ret == LB_BUF_TOO_SMALL) {
	        free (buf);
	        LB_close (Lb_fd);
	        return (NULL);
	        } /* End if */
	
	if (ret == 0)
	    return (NULL);

	if(ret < 0) {
	        LE_send_msg (GL_LB(ret),"LB_read prod_gd_info failed (ret = %d)", ret);
	        LB_close (Lb_fd);
	        return (NULL);
	        } /* End if */

	/* Close the LB after the read */
	LB_close (Lb_fd);

	/* Set the end of the user profile message */
	eol = ((int)buf + ret);

	/* Set pointer to beginning of the buffer */
	buf_ptr = buf;

	while (1) {

		/* Set pointer to user profile in the message */
		up = (Pd_user_entry *)buf_ptr;

		/* Increment the buffer pointer for next user profile */
		buf_ptr += up->entry_size;

		/* If past the end of the user profile list exit */
		if (((int)buf_ptr + sizeof(Pd_user_entry)) > eol)
			break;

		/* If user profile line index is differnet from input line number get next user profile */
		if (up->line_ind != in_up->line_ind)
	    		continue;

		/* If user profile line index and input line number equal */
		if ((up->up_type == user_type && up->line_ind == in_up->line_ind)) {
			/* Store entry size */
			up->entry_size = in_up->entry_size;
			/* Store user id */
			up->user_id = in_up->user_id;
			/* Store pms len */
			up->pms_len = in_up->pms_len;
			/* Store dd len */
			up->dd_len = in_up->dd_len;
			/* Store map len */
			up->map_len = in_up->map_len;
			/* Store pms list */
			up->pms_list = in_up->pms_list;
			/* Store dd list */
			up->dd_list = in_up->dd_list;
			/* Store map list */
			up->map_list = in_up->map_list;
			/* Store max connect time */
			up->max_connect_time = in_up->max_connect_time;
			/* Store number request products */
			up->n_req_prods = in_up->n_req_prods;
			/* Store user type */
			up->up_type = in_up->up_type;
			/* Store line index */
			up->line_ind = in_up->line_ind;
			/* Store user class */
			up->class = in_up->class;

			/* Store user password */
			memcpy(up->user_password,in_up->user_password,PASSWORD);
			/* Store user name */
			memcpy(up->user_name, in_up->user_name,USER_NAME);

			/* Store user control */
			up->cntl = in_up->cntl;
			/* Store user defined */
			up->defined = in_up->defined;

			
			/* Get LB id */
			Lb_fd = LB_open (name, LB_WRITE, NULL);

			/* Write new user information */
			ret = LB_write (Lb_fd, buf, info.size, PD_USER_INFO_MSG_ID);

			if (ret == LB_BUF_TOO_SMALL) {
	    			LB_close (Lb_fd);
	    			return (-1);
				}

			free (buf);

		 	LB_close (Lb_fd);

	   	 	return (1);
	   	 	}/* User profile found */
   		}
   	free (buf);	
    	return (-1);
	
} /* end rms find user profile*/

