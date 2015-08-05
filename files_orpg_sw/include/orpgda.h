/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/06/01 20:32:49 $
 * $Id: orpgda.h,v 1.35 2011/06/01 20:32:49 jing Exp $
 * $Revision: 1.35 $
 * $State: Exp $
 */

/**************************************************************************

      Module: orpgda.h

 Description: liborpg.a DA (data access) module include file.

       Notes: This is included by orpg.h.

 **************************************************************************/

#ifndef ORPGDA_H
#define ORPGDA_H

#include <lb.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* error code */
#define ORPGDA_ERR_BASE		-600
#define	ORPGDA_TOO_MANY_ITEMS	-600
#define	ORPGDA_STRING_TOO_BIG	-601
#define	ORPGDA_SC_ERROR		-602
#define	ORPGDA_MALLOC_FAILED	-603
#define	ORPGDA_EN_FAILED	-604

#define	ORPGDA_INVALID_BUF_SIZE		-605
#define ORPGDA_COPY_INVALID_MSG_ID	-606
#define ORPGDA_COPY_ONTO_SELF_ERROR	-607
#define ORPGDA_WRITE_NOT_PERMITTED	-608
#define ORPGDA_COMPRESSION_FAILURE	-609
#define ORPGDA_BUFFER_TOO_SMALL		-610
#define ORPGDA_WRONG_PERMISSION		-611

#define ORPGDA_COPY_NEW_MSGS_ONLY 	  0x0000
#define ORPGDA_COPY_ALL_MSGS		  0x0001
#define ORPGDA_CLEAR_DESTINATION	  0x0002
#define ORPGDA_CREATE_DESTINATION	  0x0004
#define ORPGDA_TRUE			  1
#define ORPGDA_FALSE			  0

#define ORPGDA_ARG_PUSHED_UNDEFINED       ((void *) NULL)

#define ORPGDA_DATA_ID_MASK 0xffffff
#define ORPGDA_STREAM_SHIFT 24
#define ORPGDA_STREAM_DATA_ID(stream,id) ((stream << ORPGDA_STREAM_SHIFT) | (id))

/* public functions */

/* public functions */
int ORPGDA_read (int data_id, void *buf, int buflen, LB_id_t id);
int ORPGDA_read_window (int data_id, int offset, int size);
int ORPGDA_write (int data_id, char *msg, int length, LB_id_t id);
int ORPGDA_direct (int data_id, char **msg, LB_id_t id);
LB_id_t ORPGDA_get_msg_id ();
LB_id_t ORPGDA_previous_msgid (int data_id);
int ORPGDA_msg_info (int data_id, LB_id_t id, LB_info *info);
int ORPGDA_info (int data_id, LB_id_t id, LB_info *info);
int ORPGDA_clear (int data_id, int msgs);
int ORPGDA_delete (int data_id, LB_id_t id);
int ORPGDA_seek (int data_id, int offset, LB_id_t id, LB_info *info);
int ORPGDA_changed (int data_id);
int ORPGDA_stat (int data_id, LB_status *status);
int ORPGDA_list (int data_id, LB_info *list, int nlist);
int ORPGDA_UN_register (int data_id, LB_id_t msg_id, void (*notify_func)());
int ORPGDA_group (int group, int data_id, LB_id_t id, 
					char *buf, int buf_size);
int ORPGDA_set_stream (int stream);
int ORPGDA_update (int group);
int ORPGDA_close (int data_id);
int ORPGDA_lbfd (int data_id);
int ORPGDA_open (int data_id, int flags);
int ORPGDA_write_permission (int data_id);
char* ORPGDA_lbname (int data_id);
char *ORPGDA_get_data_hostname (int data_id);
void ORPGDA_verbose (int on);
int ORPGDA_get_verbose ();
void ORPGDA_set_event_msg_byteswap_function ();
int ORPGDA_lbfd2 (int data_id, int flags);
void ORPGDA_push_arg( void *arg );
int ORPGDA_get_id_from_fd( int fd );
void ORPGDA_bypass_write_permission_check (int on_off);
int ORPGDA_reset_sys_cfg_local_host (char *host);


/* For ORPGDA_NTF_control, do token replacement. */
#define ORPGDA_NTF_control LB_NTF_control

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ORPGDA_H DO NOT REMOVE! */
