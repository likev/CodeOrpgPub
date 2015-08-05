/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/17 20:21:18 $
 * $Id: orpgedlock.c,v 1.9 2014/03/17 20:21:18 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/********************************************************************************
   
   Module:  orpgedlock.c
   
   Description:  This is the module for checking and setting edit locks.
   
   

   Assumptions:
      
   ******************************************************************************/

/*
* System Include Files/Local Include Files
*/
#include <lb.h>
#include <le.h>
#include <rss_replace.h>
#include <orpgedlock.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/


/*
* Static Globals
*/


/*
* Static Function Prototypes
*/


/**************************************************************************
   Description:  This routine checks to see if an lb is being edited.
      
   Input: 
      
   Output:
      
   Returns: 0 - if lb is not being edited; 1 - if being edited; lb error code.
   
   Notes: 

   **************************************************************************/
   
int ORPGEDLOCK_get_edit_status(int data_id, LB_id_t msg_id)
{
    
    int lock_status;
    int Lb_fd;
    
    
    /* Get lb id. */
    	
    Lb_fd = ORPGDA_lbfd( data_id);

    /* Try to set an exclusive lock on the lb message. */
    if(msg_id != 0){  
	lock_status = LB_lock (Lb_fd, LB_EXCLUSIVE_LOCK, msg_id);
    }
    else{
	lock_status = LB_lock (Lb_fd, LB_EXCLUSIVE_LOCK, LB_LB_LOCK);
    }
		
    /* If the lb has a shared lock it will not allow the exclusive lock and
       indicates that someone is editing the file already. */
	   
    if(lock_status == LB_HAS_BEEN_LOCKED)
	return(ORPGEDLOCK_EDIT_LOCKED);

    /* If the lock is successful the lb message is not being edited by 
       someone else so remove the exclusive lock and return. */
	   	
    if(lock_status == LB_SUCCESS){
	if(msg_id != 0){
    	    lock_status = LB_lock (Lb_fd, LB_UNLOCK, msg_id);
	    return (ORPGEDLOCK_NOT_LOCKED);
	}
	else{
  	    lock_status = LB_lock (Lb_fd, LB_UNLOCK, LB_LB_LOCK);
	    return (ORPGEDLOCK_NOT_LOCKED);
	}
    }
		
    /* An error occured while trying to lock return error code. */	
	
    if(lock_status < 0){
	LE_send_msg(0,"Unable to get edit status %d", lock_status);
	return(lock_status);
    }
	
   /* Eliminate compiler warning. */
   return(ORPGEDLOCK_NOT_LOCKED);
	
} /* End edit status */


/**************************************************************************
   Description:  This routine sets an edit lock.
      
   Input: Pointer to product info 
      
   Output:
      
   Returns: 
   
   Notes: 

   **************************************************************************/
   
int ORPGEDLOCK_set_edit_lock(int data_id, LB_id_t msg_id)
{
   
    int lock_status;
    int Lb_fd;
    
    /* Get lb id. */
   	
    Lb_fd = ORPGDA_lbfd(data_id);
    
    /* Place a shared lock on the lb message to indicate that the
       lb message is being edited. */
    if(msg_id != 0){   
	lock_status = LB_lock (Lb_fd, LB_SHARED_LOCK, msg_id);
    }
    else{
	lock_status = LB_lock (Lb_fd, LB_SHARED_LOCK, LB_LB_LOCK);
    }
	
    /* Lock was successful. */
	
    if(lock_status == LB_SUCCESS){
        return (ORPGEDLOCK_LOCK_SUCCESSFUL);
    }

    /* An error occured while trying to lock return error code. */	

    if(lock_status < 0){
	LE_send_msg(0,"Unable to set edit lock %d", lock_status);
	return(ORPGEDLOCK_LOCK_UNSUCCESSFUL);
    }
	
    /* Remove compiler warning. */
    return(ORPGEDLOCK_LOCK_UNSUCCESSFUL);

} /* End set edit lock */




/**************************************************************************
   Description:  This routine clears an edit lock.
      
   Input: Pointer to product info 
      
   Output:
      
   Returns: 
   
   Notes: 

   **************************************************************************/
   
int ORPGEDLOCK_clear_edit_lock(int data_id, LB_id_t msg_id)
{
   
    int lock_status;
    int Lb_fd;
    
    /* Get lb id. */
   	
    Lb_fd = ORPGDA_lbfd(data_id);
    
    /* Remove a shared lock on the lb message to indicate that the
       lb message is not being edited. */
    if(msg_id != 0){
       	lock_status = LB_lock (Lb_fd, LB_UNLOCK, msg_id);
    }
    else {
	lock_status = LB_lock (Lb_fd, LB_UNLOCK, LB_LB_LOCK);
    }
		
    /* Unlock was successful. */
	
    if(lock_status == LB_SUCCESS){
	return (ORPGEDLOCK_UNLOCK_SUCCESSFUL);
    }
		
    /* An error occured while trying to unlock return error code. */
			
    if(lock_status < 0){
	LE_send_msg(0,"Unable to clear edit lock %d", lock_status);
	return(ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL);
    }
	
    /* Remove compiler warning. */
    return(ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL);

} /* End clear edit lock */


/**************************************************************************
   Description:  This routine checks for an lb update.
      
   Input:  
      
   Output:
      
   Returns: -1 - Updated; -2 - LB not updated; -3 - LB not found
   
   Notes: 

   **************************************************************************/
   
int ORPGEDLOCK_check_update_status(int data_id, LB_id_t msg_id)
{
   
    	int ret;
    	int Lb_fd;
    	LB_status lb_status;
    	LB_check_list check_list[1];
	LB_attr attr;
	
		
   	/* Get lb id. */
   	
    	Lb_fd = ORPGDA_lbfd(data_id);
	
    	lb_status.attr = &attr;
    	lb_status.n_check = 0;
    	
    	ret = LB_stat (Lb_fd, &lb_status);
    	
    	if(ret == LB_SUCCESS){
    	
    		if (attr.types & LB_DB){
    		
    			/* Set n_check */
			lb_status.attr = NULL;
			check_list[0].id = msg_id;
			lb_status.check_list = check_list;
			lb_status.check_list[0].status= 0;
			lb_status.n_check = 1;
		   	
    			/* Get lb status. */
    	
			ret = LB_stat (Lb_fd, &lb_status);
		
		
			/* Get lb status was successful. */
			if(ret == LB_SUCCESS){
		
				if(msg_id == 0){
		
					if(lb_status.updated == LB_TRUE){
						return(ORPGEDLOCK_UPDATED);
						}
					else if (lb_status.updated == LB_FALSE){
						return(ORPGEDLOCK_NOT_UPDATED);
						}
					else{
						LE_send_msg(0,"Unable to retrieve lb update status (%d)", lb_status.updated);
						return(lb_status.updated);
						}
					}
				else{
		
					if(lb_status.check_list[0].status == LB_MSG_UPDATED){
						return(ORPGEDLOCK_UPDATED);
						}
					else if (lb_status.check_list[0].status == LB_MSG_NOCHANGE){
						return(ORPGEDLOCK_NOT_UPDATED);
						}
					else if (lb_status.check_list[0].status == LB_MSG_NOT_FOUND){
						return(ORPGEDLOCK_NOT_FOUND);
						}
					else{
						LE_send_msg(0,"Unable to retrieve lb update status (%d)", lb_status.check_list[0].status);
						return(lb_status.check_list[0].status);
						}
				
					} /* End else */
			
				}/* End if */
			
			else {
				/* Was not successful in retrieving lb status. */

				LE_send_msg(0,"Unable to retrieve lb update status (%d)", ret);	
				return (ret);

				}/* End else */
				
   			}/* End if */	    	
    		else{
    	   	
			if(lb_status.updated == LB_TRUE){
				return(ORPGEDLOCK_UPDATED);
				}
			else if (lb_status.updated == LB_FALSE){
				return(ORPGEDLOCK_NOT_UPDATED);
				}
			else{
				LE_send_msg(0,"Unable to retrieve lb update status (%d)", lb_status.updated);
				return(lb_status.updated);
				}
			
			} /* End else */
			
		} /* End If */
			
	else {
		/* Was not successful in retrieving lb status. */

		LE_send_msg(0,"Unable to retrieve lb update status (%d)", ret);	
		return (ret);

		}/* End else */
} /* End check update status */


/**************************************************************************
   Description:  This routine sets a write lock.
      
   Input: 
      
   Output:
      
   Returns: 
   
   Notes: 

   **************************************************************************/
   
int ORPGEDLOCK_set_write_lock(int data_id, LB_id_t msg_id)
{
   
    int lock_status;
    int Lb_fd;
    
    /* Get lb id. */
   	
    Lb_fd = ORPGDA_lbfd(data_id);
    	
    /* Place an exclusive lock on the entire lb to preclude someone else
       from writing while you do. */
    	
    lock_status = LB_lock (Lb_fd, LB_EXCLUSIVE_LOCK, LB_LB_LOCK);
	
    /* Lock was successful. */
	
    if(lock_status == LB_SUCCESS){
	return (ORPGEDLOCK_LOCK_SUCCESSFUL);
    }
		
    /* Was not successful in locking lb. */
		
    if(lock_status < 0){
	LE_send_msg(0,"Unable to set write lock %d", lock_status);
	return(ORPGEDLOCK_LOCK_UNSUCCESSFUL);
    }
	
    /* Eliminate compiler warning. */
    return(ORPGEDLOCK_LOCK_UNSUCCESSFUL);

} /* End set write lock */



/**************************************************************************
   Description:  This routine clears a write lock.
      
   Input: 
      
   Output:
      
   Returns: 
   
   Notes: 

   **************************************************************************/
   
int ORPGEDLOCK_clear_write_lock(int data_id, LB_id_t msg_id)
{
   
    int lock_status;
    int Lb_fd;
      
    /* Get lb id. */
   	
    Lb_fd = ORPGDA_lbfd(data_id);
    	
    /* Unlock the exclusive block on the entire lb to allow someone else
       to write. */
    	  
    lock_status = LB_lock (Lb_fd, LB_UNLOCK, LB_LB_LOCK);

    /* Lock was successful. */
	
    if(lock_status == LB_SUCCESS){
	return (ORPGEDLOCK_UNLOCK_SUCCESSFUL);
    }

    /* Unlock was not successful. */
	
    if (lock_status < 0){
	LE_send_msg(0,"Unable to clear write lock %d", lock_status);
	return(ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL);
    }
		
    /* Remove compiler warning. */

    return(ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL);

} /* End clear write lock */


