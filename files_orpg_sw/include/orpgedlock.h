/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 21:31:44 $
 * $Id: orpgedlock.h,v 1.3 2002/12/11 21:31:44 nolitam Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/**************************************************************************

      Module: orpgedlock.h

 Description: liborpg.a EDLOCK( locking mechanism for editing lb's) module 
 	      include file.

       Notes: This is included by orpg.h.

 **************************************************************************/

#ifndef ORPGEDLOCK_H
#define ORPGEDLOCK_H

#include <orpgda.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* DEFINES */


/* Global Variables */

enum {ORPGEDLOCK_EDIT_LOCKED, ORPGEDLOCK_NOT_LOCKED, ORPGEDLOCK_UPDATED, ORPGEDLOCK_NOT_UPDATED, ORPGEDLOCK_LOCK_SUCCESSFUL, ORPGEDLOCK_LOCK_UNSUCCESSFUL,
      ORPGEDLOCK_UNLOCK_SUCCESSFUL, ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL, ORPGEDLOCK_NOT_FOUND};

/* public functions */

/*  Read functions */

int ORPGEDLOCK_get_edit_status(int data_id, LB_id_t msg_id);

int ORPGEDLOCK_set_edit_lock(int data_id, LB_id_t msg_id);

int ORPGEDLOCK_clear_edit_lock(int data_id, LB_id_t msg_id);

int ORPGEDLOCK_check_update_status(int data_id, LB_id_t msg_id);

int ORPGEDLOCK_set_write_lock(int data_id, LB_id_t msg_id);

int ORPGEDLOCK_clear_write_lock(int data_id, LB_id_t msg_id);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ORPGEDLOCK_H DO NOT REMOVE! */
