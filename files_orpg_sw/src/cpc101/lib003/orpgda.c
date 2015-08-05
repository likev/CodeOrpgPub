/****************************************************************
		
    Module: orpgda.c	
		
    Description: This file contains the DA (data access) module
		of liborpg.a.

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/08/09 18:39:14 $
 * $Id: orpgda.c,v 1.76 2012/08/09 18:39:14 jing Exp $
 * $Revision: 1.76 $
 * $State: Exp $
 */  

/* System include files */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

/* Local include files */

#include <orpg.h>
#include <orpgda.h>
#include <prod_gen_msg.h>
#include <orpgdat_api.h>
#include <orpgsmi.h>
#include <infr.h>

#define MAX_NAME_SIZE	256	/* maximum name string size */
#define HOST_NAME_SIZE	64	/* maximum host name size */

#define MAX_N_DT	200	/* max number of data types */
#define MAX_N_MG	32	/* max number of msgs in groups */

enum {DA_INPUT, DA_OUTPUT};	/* for argument "io" of Byte_swap_data */

typedef struct {		/* structure for an open data store */
    int data_id;		/* data store ID */
    int fd;			/* LB file descriptor */
    char *name;			/* name of the LB */
    int flag;			/* LB_READ or LB_WRITE */
    short byteswap;		/* byte swap needed flag */
    short wperm;		/* write permission; -1 is not checked */
    int compr_code;		/* compression code - from data table */
    int n_wps;			/* size of wps */
    Mrpg_wp_item *wps;		/* write permission table - from data table */
} Da_type;

typedef struct {		/* structure for message groups */
    int group;			/* group number */
    int data_id;		/* data store ID */
    int msg_id;			/* message ID */
    char *buf;			/* user buffer for the message */
    int buf_size;		/* buffer size */
    int updated;		/* the message is updated */
} Da_group;

static Da_type *Dt_list[MAX_N_DT];
				/* list of open data stores */
static int N_dt_list = 0;	/* size of Dt_list */

static Da_group *Mg_list[MAX_N_MG];
				/* list of messages in groups */
static int N_mg_list = 0;	/* size of Mg_list */

static int Initialized = 0;	/* This module is initialized */

static int Sc_changed = 1;	/* flag indicating that the system 
				   configuration is changed */

static void *Arg_pushed = ORPGDA_ARG_PUSHED_UNDEFINED;
				/* initialize the pushed argument to invalid */

static LB_id_t Msg_id;		/* latest message ID */

typedef struct {		/* for data store type and IDs */
    int major;
    int minor;
    char *type;
} Type_id_t;

static int N_type_ids = 0;	/* total number of typed data IDs */
static Type_id_t **Type_ids = NULL;	/* typed data IDs */

#define LB_OPEN_AS_SPECIFIED	0x10000
				/* open the LB using specified flag */

static int Bypass_write_permission_check = 0;

static char Sys_cfg_local_host[HOST_NAME_SIZE] = "";
				/* IP to replace local host in system config */

static char Task_name[ORPG_TASKNAME_SIZ] = ";";	
				/* name of this task. ";" - not-initialized */

static int Data_stream = 0;	/* data stream number */

/* local functions */
static int Open_lb (int data_id, int flag);
static int Open_all_lb ();
static int Initialize ();
static int Get_index (int data_id, int flag);
static int Read_product (int data_id, void *buf, int buflen, 
                         LB_id_t id);
static int Read_data_store (int data_id, void *buf, int buflen, 
                            LB_id_t id);
static char *Byte_swap_data (int io, int len, 
				char *buf, Da_type *dt, LB_id_t id);
static int Write_permission_OK (Da_type *dt, LB_id_t id);
static void Sort_major (int n, Type_id_t **ra);
static int Get_index_by_major (int major);
static char *Get_type_by_id (int major, int minor);
static int Get_data_store_path (int data_id, char *path, int buf_size);
static int Get_lb_open_flag (int n_wps, Mrpg_wp_item *wps, int flag);
static void Sort_wps (int wp_size, Mrpg_wp_item *wps);
static int Match_task_name (char *task_name, Mrpg_wp_item *wp);
static int Get_stream_data_path (int data_id, char *name, int n_s);
static int Get_stream_data_id (int id);
static int Search_open_id (int st_ind, int data_id);
static char *Data_id_text (int data_id);


/********************************************************************
			
    Sets the data stream number for this process if "stream" is not
    negative. It returns the data stream number after setting.

********************************************************************/

int ORPGDA_set_stream (int stream) {
    if (stream >= 0)
	Data_stream = stream;
    return (Data_stream);
}

/********************************************************************
			
    Receives host name "host" for being used later to replace the local 
    data stores in the system configuration by those on "host". An
    empty "host" turns off this feature. Return 0 on success or a
    negative error code.

********************************************************************/

int ORPGDA_reset_sys_cfg_local_host (char *host) {
    if (strlen (host) >= HOST_NAME_SIZE) {
	LE_send_msg (GL_ERROR, "ORPGDA: host name too long");
	return (ORPGDA_STRING_TOO_BIG);
    }
    strcpy (Sys_cfg_local_host, host);
    return (0);
}

/********************************************************************
			
    Description: Turns on/off the verbose mode. 

    Input:	on - non-zero is on and zero is off;

********************************************************************/

void ORPGDA_verbose (int on) {
    if (on)
	LE_set_option ("LE disable", 0);
    else
	LE_set_option ("LE disable", 1);
}

/********************************************************************
			
    No longer used.

********************************************************************/

int ORPGDA_get_verbose () {
    return (1);
}

/********************************************************************
			
    Description: This function registers a message in a group. 

    Input:	group - group number;
		data_id - data store ID;
		id - message ID;
		buf - pointer to the message buffer;
		buf_size - size of the message buffer;

    Returns:	This function returns 0 on success or a negative number 
		for indicating an error condition. 

********************************************************************/

int ORPGDA_group (int group, int data_id, LB_id_t id, 
		char *buf, int buf_size)
{
    int i;

    for (i = 0; i < N_mg_list; i++)
	if (Mg_list[i]->group == group &&
	    Mg_list[i]->data_id == data_id &&
	    Mg_list[i]->msg_id == id)
	    return (0);

    if (N_mg_list >= MAX_N_MG)
	return (ORPGDA_TOO_MANY_ITEMS);

    /* allocate the data struct */
    Mg_list[N_mg_list] = (Da_group *)malloc (sizeof (Da_group));
    if (Mg_list[N_mg_list] == NULL)
	return (ORPGDA_MALLOC_FAILED);

    Mg_list[N_mg_list]->group = group;
    Mg_list[N_mg_list]->data_id = data_id;
    Mg_list[N_mg_list]->msg_id = id;
    Mg_list[N_mg_list]->buf = buf;
    Mg_list[N_mg_list]->buf_size = buf_size;
    N_mg_list++;

    return (0);
}

/********************************************************************
			
    Description: This function updates all messages in group "group". 

    Input:	group - message group number;

    Returns:	This function returns 0 on success or a negative number 
		for indicating an error condition. 

********************************************************************/

int ORPGDA_update (int group)
{
    LB_status status;
    int ret;
    int i;

    /* initialize this module */
    if (!Initialized &&	(ret = Initialize ()) < 0)
	return (ret);

    /* system configuration updated */
    if (Sc_changed && (ret = Open_all_lb ()) < 0)
	return (ret);

    for (i = 0; i < N_mg_list; i++)
	Mg_list[i]->updated = -1;

    /* set up the updated field in Mg_list */
    for (i = 0; i < N_mg_list; i++) {
	int data_id;
	int k, m;
	LB_check_list check_list[MAX_N_MG];
	int n_check;

	if (Mg_list[i]->group != group || Mg_list[i]->updated >= 0)
	    continue;
	data_id = Mg_list[i]->data_id;

	for (k = 0; k < N_dt_list; k++)	/* check whether it is open */
	    if (Dt_list[k] != NULL && Dt_list[k]->data_id == data_id)
		break;

	/* open the LB */
 	if (k >= N_dt_list && (k = Open_lb (data_id, LB_READ)) < 0)	
	    return (k);

	status.attr = NULL;
	n_check = 0;
	status.check_list = check_list;
	for (m = i; m < N_mg_list; m++) {
	    if (Mg_list[m]->group == group &&
		Mg_list[m]->data_id == data_id) {
	        check_list [n_check].id = Mg_list[m]->msg_id;
		n_check++;
	    }
	}
	status.n_check = n_check;

	while (1) {
	    ret = LB_stat (Dt_list[k]->fd, &(status));
	    if (ret == RMT_TIMED_OUT) {
		if ((k = Open_lb (data_id, LB_READ)) < 0)
		    return (k);
	    }
	    else
		break;
	}

	if (ret != LB_SUCCESS)
	    return (ret);
	else {
	    int cnt;

	    cnt = 0;
	    for (m = i; m < N_mg_list; m++) {
	        if (Mg_list[m]->group == group &&
		    Mg_list[m]->data_id == data_id) {
		    if (check_list[cnt].status == LB_MSG_NOCHANGE)
			Mg_list[m]->updated = 0;
		    else
			Mg_list[m]->updated = 1;
		    cnt++;
	        }
	    }
	}
    }

    /* read the messages */
    for (i = 0; i < N_mg_list; i++) 
    {

	if (Mg_list[i]->updated == 1)
	{
	   if ((ret = ORPGDA_read (Mg_list[i]->data_id, Mg_list[i]->buf, 
		Mg_list[i]->buf_size, Mg_list[i]->msg_id)) <= 0)
		return(ret);
	}
	
    }

    return (0);
}

/********************************************************************
			
    Description: This function returns the LB descriptor for data_id.

    Input:	data_id - data store ID;

    Returns:	This function returns the LB descriptor on success 
		or a negative number for indicating an error 
		condition. 

********************************************************************/

int ORPGDA_lbfd (int data_id)
{
    int ind;

    if ((ind = Get_index (data_id, LB_READ)) < 0)
	return (ind);

    return (Dt_list[ind]->fd);
}

/********************************************************************
			
    Opens data store "data_id" with flags "flags" (LB_READ, LB_WRITE
    or their OR). This function provides the caller with full control
    of the LB_open flag.

    Returns the LB descriptor on success or a negative number for 
    indicating an error condition. 

********************************************************************/

int ORPGDA_open (int data_id, int flags) {
    int ind;

    if ((ind = Get_index (data_id, flags | LB_OPEN_AS_SPECIFIED)) < 0)
	return (ind);

    return (Dt_list[ind]->fd);
}

/********************************************************************
			
    Sets a flag to bypass the LB write permission check for this 
    process. This is designed for tools that need to do so.

********************************************************************/

void ORPGDA_bypass_write_permission_check (int on_off) {

    Bypass_write_permission_check = on_off;
}

/********************************************************************
			
    This function is identical to ORPGDA_open.

********************************************************************/

int ORPGDA_lbfd2 (int data_id, int flags) {
    return (ORPGDA_open (data_id, flags));
}

/********************************************************************
			
    Description: This function returns the name of an LB for data_id.

    Input:	data_id - data store ID;

    Returns:	This function returns the the name of the LB success 
		or NULL on failure

********************************************************************/

#define MAX_LBNAME_SIZE 200

char* ORPGDA_lbname (int data_id) {
    static char buffer[MAX_LBNAME_SIZE];
    char name[MAX_LBNAME_SIZE];
    int i;

    i = Search_open_id (0, data_id);
    if (i < N_dt_list)
	strcpy (name, Dt_list[i]->name);
    else if (Get_stream_data_path (data_id, name, MAX_LBNAME_SIZE) < 0)
	return (NULL);
    MISC_expand_env (name, buffer, MAX_LBNAME_SIZE);
    if (buffer[0] == '\0')
	return (NULL);
    return (buffer);
}

char *ORPGDA_get_data_hostname (int data_id) {
    static char *buf = NULL;
    char *path;
    int len;

    path = ORPGDA_lbname (data_id);
    if (path == NULL)
	return ("");
    len = MISC_char_cnt (path, "\0:");
    if (len <= 0 || path[len] != ':')
	return ("");
    if (buf != NULL)
	free (buf);
    buf = MISC_malloc (len + 1);
    memcpy (buf, path, len);
    buf[len] = '\0';
    return (buf);
}

/********************************************************************
			
    Description: This function reads a message "id" in data store 
		"data_id". 

    Input:	data_id - data store ID;
		buf - pointer to the message buffer;
		buflen - size of the message buffer;
		id - the message ID;

    Returns:	This function returns the message length on success 
		or a negative number for indicating an error 
		condition. 

********************************************************************/

int ORPGDA_read (int data_id, void *buf, int buflen, LB_id_t id)
{

   /* Check if data_id is valid product.  If yes, call Read_product.
      Otherwise, call Read_data_store. */
   if( data_id < ORPGDAT_BASE )
      return( Read_product( data_id, buf, buflen, id ) );

   return( Read_data_store( data_id, buf, buflen, id ) );

}

/********************************************************************
			
    Description: This function sets a window for reading part of a 
                 message.

    Input:	data_id - Data ID.
                offset - window offset, in bytes
                size - size of window, in bytes

    Returns:	The message ID. 

********************************************************************/
int ORPGDA_read_window ( int data_id, int offset, int size )
{

    while (1) {
	int ret, ind;

	if ((ind = Get_index (data_id, LB_READ)) < 0)
	    return (ind);

	ret = LB_read_window(Dt_list[ind]->fd, offset, size);
	if (ret != RMT_TIMED_OUT)
	    return (ret);
    }
    return (-1); /* should never happen -- here for cireport */
}

/********************************************************************
			
    Description: This function returns the message ID involved in the
		latest successful read/write of any LB. 

    Returns:	The message ID. 

********************************************************************/

LB_id_t ORPGDA_get_msg_id ()
{

    return (Msg_id);
}

/********************************************************************
			
    Description: This function returns the message ID involved in the
		latest successful LB_read/LB_write of data_id. 

    Input:	data_id - Data ID.

    Returns:	The message ID. 

********************************************************************/
LB_id_t ORPGDA_previous_msgid ( int data_id )
{

    while (1) {
	int ret, ind;

	if ((ind = Get_index (data_id, LB_READ)) < 0)
	    return (ind);

	ret = LB_previous_msgid (Dt_list[ind]->fd);
	if (ret != RMT_TIMED_OUT)
	    return (ret);
    }
    return (-1); /* should never happen -- here for cireport */
}

/********************************************************************
			
    Description: This function writes a message "msg" of ID "id" in 
		data store "data_id". 

    Input:	data_id - data store ID;
		msg - pointer to the message;
		length - length of the message;
		id - the message ID;

    Returns:	This function returns the message length on success 
		or a negative number for indicating an error 
		condition. 

********************************************************************/

int ORPGDA_write (int data_id, char *msg, int length, LB_id_t id)
{

    while (1) {
	int ret, ind;
	char *b, *out;

	if ((ind = Get_index (data_id, LB_WRITE)) < 0)
	    return (ind);

	if (Dt_list[ind]->n_wps > 0 &&
	    !Write_permission_OK (Dt_list[ind], id))
	    return (ORPGDA_WRITE_NOT_PERMITTED);

	b = msg;
	if (Dt_list[ind]->byteswap && length > 0)
	    b = Byte_swap_data (DA_OUTPUT, length, msg, Dt_list[ind], id);
	out = NULL;
	if (Dt_list[ind]->compr_code && length > 0) {
	    ret = ORPGCMP_compress (Dt_list[ind]->compr_code, b, length, &out);
	    if (ret < 0) {
		LE_send_msg (0, "ORPGDA: ORPGCMP_compress failed (%d)\n", ret);
		return (ORPGDA_COMPRESSION_FAILURE);
	    }
	    else if (ret > 0) {
		length = ret;
		b = out;
	    }
	}

	ret = LB_write (Dt_list[ind]->fd, b, length, id);
	if (out != NULL)
	    free (out);
	if (ret != RMT_TIMED_OUT)
	    return (ret);
    }
    return (-1); /* should never happen -- here for cireport */
}

/********************************************************************

   Description:
      This function returns the pointer in the LB to a message.  This
      is the ORPGDA version of the LB_direct function.

    Input:      data_id - the data ID;
                id - the message id to be returned;
   
    Output:     ptr - the pointer to the message;
  
    Returns:    This function returns the length of the message
                on success or a non-positive error number.
 
    Notes:      Refer to lb.doc for a detailed description of LB_direct
                function.

********************************************************************/
int ORPGDA_direct( int data_id, char **ptr, LB_id_t id ){

   int ind;

   if ((ind = Get_index (data_id, LB_READ)) < 0)
      return (ind);
   return( LB_direct (Dt_list[ind]->fd, (void *) ptr, id) );
}

/********************************************************************

   Description:
      This function returns information about a message.  This is
      the ORPGDA interface for LB_msg_info.

   Input:	data_id - the data ID.
		id - the message ID for which information is requested.

   Output:	info - LB_info structure to receive information.

   Returns:	See lb man page (LB_msg_info) for possible return
                values.

   Notes:	Refer to lb.doc page for a detailed description
                of LB_msg_info. 

********************************************************************/
int ORPGDA_msg_info( int data_id, LB_id_t id, LB_info *info ){

   while (1) {

      int ret, ind;

      if ((ind = Get_index (data_id, LB_READ)) < 0)
         return (ind);
      ret = LB_msg_info (Dt_list[ind]->fd, id, info);
      if (ret != RMT_TIMED_OUT)
         return (ret);
    }
    return (-1); /* should never happen -- here for cireport */
}

/********************************************************************

   Description:
      This function removes messages from an LB.  This is
      the ORPGDA interface for LB_clear.

   Input:	data_id - the data ID.
		nmsgs - the number of messages to remove.

   Output:	

   Returns:	See lb man page (LB_clear) for possible return
                values.

   Notes:	Refer to lb.doc page for a detailed description
                of LB_clear. 

********************************************************************/
int ORPGDA_clear( int data_id, int nmsgs ){

   while (1) {

      int ret, ind;

      if ((ind = Get_index (data_id, LB_WRITE)) < 0)
         return (ind);
      ret = LB_clear (Dt_list[ind]->fd, nmsgs);
      if (ret != RMT_TIMED_OUT)
         return (ret);
    }
    return (-1); /* should never happen -- here for cireport */
}

/********************************************************************

   Description:
      This function deletes messages from an LB.  This is
      the ORPGDA interface for LB_delete.  This function
      should only be used for LB_DB type.

   Input:	data_id - the data ID.
		LB_id_t - the message ID.

   Output:	

   Returns:	See lb man page (LB_delete) for possible return
                values.

   Notes:	Refer to lb.doc page for a detailed description
                of LB_delete. 

********************************************************************/
int ORPGDA_delete( int data_id, LB_id_t id ){

   while (1) {

      int ret, ind;

      if ((ind = Get_index (data_id, LB_WRITE)) < 0)
         return (ind);
      ret = LB_delete (Dt_list[ind]->fd, id);
      if (ret != RMT_TIMED_OUT)
         return (ret);
    }
    return (-1); /* should never happen -- here for cireport */
}

/********************************************************************
			
    Description: This function is ORPGDA version of the LB_seek 
		function. 

    Input:	data_id - data store ID;
		offset - seek offset;
		id - the message ID;

    Output:	info - for returning the message info.

    Returns:	This function returns LB_SUCCESS on success 
		or a negative number for indicating an error 
		condition. 

********************************************************************/

int ORPGDA_seek (int data_id, int offset, LB_id_t id, LB_info *info)
{

    while (1) {
	int ret, ind;

	if ((ind = Get_index (data_id, LB_READ)) < 0)
	    return (ind);

	ret = LB_seek (Dt_list[ind]->fd, offset, id, info);
	if (ret != RMT_TIMED_OUT)
	    return (ret);
    }
    return (-1); /* should never happen -- here for cireport */
}


/********************************************************************
			
    Description: This function returns the attributes of a list of 
		the latest "nlist" messages in data store "data_id". 

    Input:	data_id - data store ID;
		nlist - number of items in the list;

    Output:	list - the attributes of a list of the latest "nlist" 
		message;

    Returns:	This function returns the number of items in the list on 
		success or a negative number for indicating an error 
		condition. 

********************************************************************/

int ORPGDA_list (int data_id, LB_info *list, int nlist)
{

    while (1) {
	int ret, ind;

	if ((ind = Get_index (data_id, LB_READ)) < 0)
	    return (ind);
	ret = LB_list (Dt_list[ind]->fd, list, nlist);
	if (ret != RMT_TIMED_OUT)
	    return (ret);
    }
    return (-1); /* should never happen -- here for cireport */
}

/********************************************************************
			
    Description: This function returns the attributes of message of ID 
		"id" in data store "data_id". 

    Input:	data_id - data store ID;
		id - the message ID;

    Output:	info - the attributes of the message;

    Returns:	This function returns LB_SUCCESS (0) on success or a 
		negative number for indicating an error condition. 

********************************************************************/

int ORPGDA_info (int data_id, LB_id_t id, LB_info *info)
{

    while (1) {
	int ret, ind;

	if ((ind = Get_index (data_id, LB_READ)) < 0)
	    return (ind);
	ret = LB_seek (Dt_list[ind]->fd, 0, id, info);
	if (ret != RMT_TIMED_OUT)
	    return (ret);
    }
    return (-1); /* should never happen -- here for cireport */
}

/********************************************************************
			
    Description: This function returns update status in data store 
		"data_id". 

    Input:	data_id - data store ID;

    Returns:	return 1 if any message in data store "data_id" 
		has been updated since previous call to ORPGDA_changed. 
		It returns 0 if there is no change. On failure, it 
		returns a negative number for indicating an error 
		condition. 

********************************************************************/

int ORPGDA_changed (int data_id)
{

    while (1) {
	int ret, ind;
	LB_status status;

	if ((ind = Get_index (data_id, LB_READ)) < 0)
	    return (ind);
	status.attr = NULL;
	status.n_check = 0;
	ret = LB_stat (Dt_list[ind]->fd, &(status));
	if (ret != RMT_TIMED_OUT) {
	    if (ret != LB_SUCCESS)
		return (ret);
	    else if (status.updated)
		return (1);
	    else
		return (0);
	}
    }
    return(-1); /* should never happen -- here for cireport */
}

/********************************************************************
			
    Description: This function returns the status of data store 
                 "data_id" in user supplied buffer "status".  This
                 is the ORPGDA interface for function LB_stat.

    Input:	data_id - data store ID;
		status - pointer to LB_status structure;

    Output:	status - the status of the data store;

    Returns:	This function returns LB_SUCCESS (0) on success or a 
		negative number for indicating an error condition. 


********************************************************************/
int ORPGDA_stat (int data_id, LB_status *status)
{

   while(1){

      int ret, ind;

      if ((ind = Get_index (data_id, LB_READ)) < 0)
         return (ind);

      ret = LB_stat (Dt_list[ind]->fd, status);
      if(ret != RMT_TIMED_OUT)
         return(ret);

   }
   return(-1);	/* should never happen -- here for cireport */
}  

/********************************************************************
			
    Description: This function opens the LB with WRITE permission.

    Input:	data_id - data store ID;

    Returns:	This function returns non-negative value on success or a 
		negative number for indicating an error condition. 


********************************************************************/
int ORPGDA_write_permission (int data_id)
{

   return( Get_index (data_id, LB_WRITE) );

}
  
/********************************************************************
			
    Description: This function registers an UN callback function.
                 This is the ORPGDA interface for function LB_UN_register.

    Input:	data_id - data store ID;
                msg_id - message ID;
		notify_func- the notifcation callback function;

    Output:	

    Returns:	This function returns LB_SUCCESS (0) on success or a 
		negative number for indicating an error condition. 


********************************************************************/
int ORPGDA_UN_register (int data_id, LB_id_t msg_id, void (*notify_func)())
{

   void *temp_arg = Arg_pushed;

   /* Reset the global argument. */
   Arg_pushed = ORPGDA_ARG_PUSHED_UNDEFINED;

   while(1){

      int ret, ind;

      if ((ind = Get_index (data_id, LB_READ)) < 0)
         return (ind);

      if( temp_arg != ORPGDA_ARG_PUSHED_UNDEFINED )
         EN_control( EN_PUSH_ARG, temp_arg );


      ret = LB_UN_register (Dt_list[ind]->fd, msg_id, notify_func);
      if(ret != RMT_TIMED_OUT)
         return(ret);

   }
   return(-1);	/* should never happen -- here for cireport */
}

/********************************************************************
			
    Description: This function returns the LB ID (data store ID) 
                 given the file descriptor. 

    Input:	fd - file descriptor;

    Returns:	The LB ID if opened, or LB_NOT_FOUND.

********************************************************************/
int ORPGDA_get_id_from_fd( int fd ){

   int i;

   for (i = 0; i < N_dt_list; i++){

      if( (Dt_list[i] != NULL) && (Dt_list[i]->fd == fd) )
	 return( Dt_list[i]->data_id );

   }

   return(LB_NOT_FOUND);

}

/********************************************************************
			
    Description: This function passes the user defined callback 
                 function argument to ORPGDA library.  Intended
                 to be used in conjuction with ORPGDA_UN_register.

    Input:	arg - argument to push 

    Returns:

********************************************************************/
void ORPGDA_push_arg( void *arg ){

   Arg_pushed = arg; 

}

  
/********************************************************************
			
    Description: This function initializes this module if it is not
		initialized. It then checks whether the system 
		configuration has been changed. If it is changed, 
		Get_index reopens all LBs that are relocated. 
		Get_index then searches for the index in Dt_list that 
		corresponds to data_id if it is open. If the LB is not 
		open, this function opens the LB. 

    Input:	data_id - data store ID;
		flag - the LB open flag (LB_READ or LB_WRITE);

    Returns:	This function returns the index in Dt_list for the 
		open data store on success or a negative number for 
		indicating an error condition. 

********************************************************************/

static int Get_index (int data_id, int flag)
{
    int ret;
    int i;

    /* initialize this module */
    if (!Initialized &&	(ret = Initialize ()) < 0)
	return (ret);

    /* system configuration updated */
    if (Sc_changed && (ret = Open_all_lb ()) < 0)
	return (ret);

#ifdef NEW_CODE


    The following replaces all lines after it in the function.



    /* check whether it is open and of the right access mode */
    i = Search_open_id (0, data_id);
    if (i < N_dt_list) {	/* already open */
	if ((flag & LB_WRITE) && !(Dt_list[i]->flag & LB_WRITE)) {
	    LE_send_msg (GL_ERROR, 
		"Data store %s not open with write permission", 
					Data_id_text (data_id));
	    return (ORPGDA_WRONG_PERMISSION);
	}
	return (i);
    }

    /* open the LB */
    return (Open_lb (data_id, flag));	
#endif


    /* check whether it is open and of the right access mode */
    if (flag & LB_WRITE) {
	i = 0;
	while ((i = Search_open_id (i, data_id)) < N_dt_list) {
	    if (Dt_list[i]->flag == LB_WRITE)
		break;
	    else {
		LE_send_msg (GL_ERROR, "$$$$ Need LB_WRITE open - data_id %s",
				Data_id_text (data_id));
		exit (1);
	    }
	    i++;
	}
    }
    else
	i = Search_open_id (0, data_id);

    /* open the LB */
    if (i >= N_dt_list && (i = Open_lb (data_id, flag)) < 0)		
	return (i);

    return (i);
}

/********************************************************************
			
    Searches for the entry in Dt_list that matches "data_id". The 
    search starts with index "st_ind". Returns the index found or 
    N_dt_list if not found.

********************************************************************/

static int Search_open_id (int st_ind, int data_id) {
    int stream_d_id, i;

    stream_d_id = Get_stream_data_id (data_id);
    if (stream_d_id == data_id) {
	for (i = st_ind; i < N_dt_list; i++)
	    if (Dt_list[i] != NULL && Dt_list[i]->data_id == data_id)
		break;
    }
    else {
	for (i = st_ind; i < N_dt_list; i++)
	    if (Dt_list[i] != NULL && (Dt_list[i]->data_id == data_id || 
				    Dt_list[i]->data_id == stream_d_id))
		break;
    }
    return (i);
}

/********************************************************************
			
    Description: This function opens the data store "data_id". 

    Inputs:	data_id - the data store ID;
		flag - LB_READ or LB_WRITE;

    Return:	returns the index in Dt_list for the open LB on success 
		or an ORPGDA negative error number on failure.

********************************************************************/

static int Open_lb (int data_id, int flag)
{
    char name [MAX_NAME_SIZE];
    int fd, lb_is_bigendian;
    int i, s, size;
    char *cpt;
    Mrpg_data_t *data_attr;

    /* get data store path */
    data_id = Get_stream_data_path (data_id, name , MAX_NAME_SIZE);
    if (data_id < 0)
	return (data_id);

    /* get compr_code, n_wps and wps from data table API.  If not
       defined in data table, check product table. */
    data_attr = NULL;
    if (!Bypass_write_permission_check) {
	data_attr = ORPGDAT_get_entry (data_id, &size);
	if( (data_attr == NULL) && (data_id < ORPGDAT_BASE) )
           data_attr = ORPGPAT_get_data_table_entry(data_id, &size); 
    }

    if (data_attr != NULL) {
	flag = Get_lb_open_flag (data_attr->wp_size, data_attr->wp, flag);
	if (flag < 0) {
	    free (data_attr);
	    return (flag);
	}
    }

    /* We close this if it is open */
    ORPGDA_close (data_id);

    /* open LB */
    flag &= ~LB_OPEN_AS_SPECIFIED;
    fd = RSS_orpgda_lb_open (name, flag, (void *)&Msg_id, &lb_is_bigendian);
    if (MISC_test_options ("PROFILE")) {
	char b[128];
	MISC_string_date_time (b, 128, (const time_t *)NULL);
	fprintf (stderr, "%s PROF: ORPGDA LB open %s (%d), fd %d\n", 
						b, name, data_id, fd);
    }
    if (fd < 0) {
	LE_send_msg (0, "ORPGDA: RSS_orpgda_lb_open %s failed (ret = %d)\n",
							name, fd);
	if (data_attr != NULL)
	    free (data_attr);
	return (fd);
    }

    if (Bypass_write_permission_check) {
	data_attr = ORPGDAT_get_entry (data_id, &size);
	if( (data_attr == NULL) && (data_id < ORPGDAT_BASE) )
           data_attr = ORPGPAT_get_data_table_entry(data_id, &size); 
    }

    /* find a slot in the list */
    for (i = 0; i < N_dt_list; i++)
	if (Dt_list[i] == NULL)
	    break;

    if (i >= MAX_N_DT)
	return (ORPGDA_TOO_MANY_ITEMS);

    /* allocate the data struct */
    s = sizeof (Da_type) + strlen (name) + 1;
    if (data_attr != NULL)
	s += data_attr->wp_size * sizeof (Mrpg_wp_item);
    cpt = (char *)malloc (s);
    if (cpt == NULL) {
	LE_send_msg (0, "ORPGDA: malloc failed\n");
	LB_close (fd);
	if (data_attr != NULL)
	    free (data_attr);
	return (ORPGDA_MALLOC_FAILED);
    }
    Dt_list[i] = (Da_type *)cpt;
    cpt += sizeof (Da_type);
    if (data_attr != NULL) {
	Dt_list[i]->wps = (Mrpg_wp_item *)cpt;
	cpt += data_attr->wp_size * sizeof (Mrpg_wp_item);
    }

    Dt_list[i]->name = cpt;
    strcpy (Dt_list[i]->name, name);
    Dt_list[i]->fd = fd;
    Dt_list[i]->flag = flag;
    Dt_list[i]->data_id = data_id;
    if (lb_is_bigendian == MISC_i_am_bigendian ())
	Dt_list[i]->byteswap = 0;
    else
	Dt_list[i]->byteswap = 1;
    Dt_list[i]->wperm = -1;

    if (data_attr == NULL) {
	Dt_list[i]->compr_code = 0;
	Dt_list[i]->n_wps = 0;
    }
    else {
	Dt_list[i]->compr_code = data_attr->compr_code;
	if (flag & LB_WRITE) {
	    Dt_list[i]->n_wps = data_attr->wp_size;
	    memcpy (Dt_list[i]->wps, data_attr->wp, 
				data_attr->wp_size * sizeof (Mrpg_wp_item));
	    Sort_wps (data_attr->wp_size, Dt_list[i]->wps);
	}
	else
	   Dt_list[i]->n_wps = 0;
    }

    if (i >= N_dt_list)
	N_dt_list++;

    if (data_attr != NULL)
	free (data_attr);
    return (i);
}

/********************************************************************
			
    Sorts the write permission table "wps" of size "wp_size" so it 
    is easier to use latter. Items with the same message ID are put 
    together.

********************************************************************/

static void Sort_wps (int wp_size, Mrpg_wp_item *wps) {
    int i;

    for (i = 1; i < wp_size; i++) {
	int k;
	if (wps[i].msg_id == wps[i - 1].msg_id)
	    continue;
	for (k = i + 1; k < wp_size; k++) {
	    if (wps[k].msg_id == wps[i - 1].msg_id) {
		Mrpg_wp_item t;
		t = wps[k];
		wps[k] = wps[i];
		wps[i] = t;
		break;
	    }
	}
    }
}

/********************************************************************
			
    Description: This function closes the data store "data_id". 

    Inputs:	data_id - the data store ID;

    Returns:	This function returns 0 on success.

********************************************************************/

int ORPGDA_close (int data_id)
{
    int i;

    /* We close this if it is open */
    i = 0;
    while ((i = Search_open_id (i, data_id)) < N_dt_list) {
	LB_close (Dt_list[i]->fd);
	free ((char *)Dt_list[i]);
	Dt_list[i] = NULL;
	i++;
    }
    return (0);
}

/********************************************************************
			
    Description: This function reopens all LBs. 

    Return:	0 on success or a CS error number on failure.

********************************************************************/

static int Open_all_lb ()
{
    char name[MAX_NAME_SIZE];
    int i;

    Sc_changed = 0;

    for (i = 0; i < N_dt_list; i++) {
	int ret;

	if (Dt_list[i] == NULL)
	    continue;

	ret = Get_data_store_path (Dt_list[i]->data_id, name, MAX_NAME_SIZE);
	if (ret < 0) {
	    LE_send_msg (0, 
		"ORPGDA: Data store (%s) not found in syscfg (%d)\n", 
				Data_id_text (Dt_list[i]->data_id), ret);
	    return (ret);
	}

	if (strcmp (Dt_list[i]->name, name) == 0)
	    continue;

	/* reopen the LB; we ignore possible failure here */
	Open_lb (Dt_list[i]->data_id, Dt_list[i]->flag);
    }

    return (0);
}

/********************************************************************
			
    Description: This function initialize this module. It registers
		the system configuration change status variable. 

    Returns:	0 on success or ORPGDA_EN_FAILED on failure. 

********************************************************************/

static int Initialize ()
{
    int ret;

    ret = CS_event (ORPGEVT_CFG_CHANGE, &Sc_changed);
    if (ret != 0)
	return (ORPGDA_EN_FAILED);

    Initialized = 1;

    return (0);
}

/********************************************************************
			
    Description: This function reads a message "id" in data store 
		"data_id".  "data_id" is an orpg product.  If message
                is a link to product database, message is read from
                there.

    Input:	data_id - data store ID;
		buf - pointer to the message buffer;
		buflen - size of the message buffer;
		id - the message ID;

    Returns:	This function returns the message length on success 
		or a negative number for indicating an error 
		condition. 

********************************************************************/

int Read_product( int data_id, void *buf, int buflen, LB_id_t id )
{

   int ret;
   char **cpt;
   LB_id_t save_id;

   /* Validate the buffer length.  Must be at least the size of a 
      product header if the library is not allocating the buffer
      for the application. */
   if( buflen < (int)sizeof( Prod_header ) && buflen != LB_ALLOC_BUF )
      return( ORPGDA_INVALID_BUF_SIZE );

   /* Read product from product LB. */
   if( (ret = Read_data_store( data_id, buf, buflen, id ) ) < 0 )
      return( ret );

   /* Save the message ID of the message just read. */
   save_id = Msg_id;

   /* Check returned size.  If equal to product header, check if ABORT_MSG or
      link to product database. */
   if( ret == sizeof( Prod_header ) ){

      Prod_header *phd;

      /* Check orpg header length and msg_id fields. */
      if( buflen == LB_ALLOC_BUF ){

         cpt = (char **) buf;
         phd = (Prod_header *) *cpt;

      }
      else
         phd = (Prod_header *) buf;

      if( phd->g.len < 0 ){

         /* This is an ABORT_MSG so just return. */
         return( ret );

      }

      if( phd->g.id > 0 ){

         int pid;
         time_t gen_time;
         LB_id_t msg_id;

         /* This is a link to the product database.  

            Save the product ID and generation time from product header
            just read for validation purposes.  Save message ID so product
            can be read. */
         pid = phd->g.prod_id;
         gen_time = phd->g.gen_t; 
         msg_id = phd->g.id;
      
         /* Free allocated buffer. */
         if( buflen == LB_ALLOC_BUF ){

            free( phd );
            phd = NULL;

         }

         /* Read product from product database. */
         ret = Read_data_store( ORPGDAT_PRODUCTS, buf, buflen, msg_id ); 

         /* Restore the message ID */
         Msg_id = save_id;

         /* Check the returned size.  If buffer too small is ok since
            we can still validate the product header. */
         if( ret == LB_BUF_TOO_SMALL || ret >= (int)sizeof( Prod_header ) ){

            /* Validate product header data. */
            if( buflen == LB_ALLOC_BUF ){

               cpt = (char **) buf;
               phd = (Prod_header *) *cpt;

            }
            else
               phd = (Prod_header *) buf;

            if( pid != phd->g.prod_id || gen_time != phd->g.gen_t ){

               /* Headers do not match.  Product must have expired from 
                  database. */
               return( LB_EXPIRED );

            }

            phd->g.id = msg_id;

         }
         else
            return( ret );

      }

   }

   return( ret );

}

/********************************************************************
			
    Description: This function reads a message "id" in data store 
		"data_id". 

    Input:	data_id - data store ID;
		buf - pointer to the message buffer;
		buflen - size of the message buffer;
		id - the message ID;

    Returns:	This function returns the message length on success 
		or a negative number for indicating an error 
		condition. 

********************************************************************/
static int Read_data_store (int data_id, void *buf, int buflen, 
                            LB_id_t id){

    while (1) {
	int ret, ind, msg_len;
	char *msg, *tmp;
	Da_type *da;

	if ((ind = Get_index (data_id, LB_READ)) < 0)
	    return (ind);
	da = Dt_list[ind];
	msg_len = LB_read (da->fd, buf, buflen, id);
	if (buf == NULL && buflen == 0)
	    return (msg_len);
	msg = tmp = NULL;
	if (buflen == LB_ALLOC_BUF) {
	    if (msg_len > 0)
		msg = *((char **)buf);
	}
	else {			/* user provided biffer */
	    if (da->compr_code && msg_len == LB_BUF_TOO_SMALL) {
		if (id == LB_NEXT)
		    LB_seek (da->fd, -1, LB_CURRENT, NULL);
		msg_len = LB_read (da->fd, (char *)&tmp, LB_ALLOC_BUF, id);
		msg = tmp;
	    }
	    else
		msg = (char *)buf;
	}
	if (msg_len == RMT_TIMED_OUT)
	    continue;
	if (msg_len <= 0)
	    return (msg_len);
	if (da->compr_code) {
	    char *out;
	    ret = ORPGCMP_decompress (da->compr_code, msg, msg_len, &out);
	    if (ret < 0) {
		if (msg != (char *)buf && msg != NULL)
		    free (msg);
		LE_send_msg (0, 
			"ORPGDA: ORPGCMP_decompress failed (%d)\n", ret);
		return (ORPGDA_COMPRESSION_FAILURE);
	    }
	    if (buflen == LB_ALLOC_BUF) {
		if (ret > 0) {
		    if (msg != NULL)
			free (msg);
		    msg_len = ret;
		    msg = out;
		    *((char **)buf) = out;
		}
	    }
	    else {
		if (ret > buflen || (ret == 0 && msg_len > buflen)) {
		    if (ret > 0)
			free (out);
		    if (tmp != NULL)
			free (tmp);
		    return (ORPGDA_BUFFER_TOO_SMALL);
		}
		if (ret > 0) {
		    memcpy (buf, out, ret);
		    free (out);
		    msg_len = ret;
		}
		else if (buf != msg)
		    memcpy (buf, msg, msg_len);
		if (tmp != NULL)
		    free (tmp);
		msg = buf;
	    }
	}

	if (da->byteswap && msg_len > 0)
	    Byte_swap_data (DA_INPUT, msg_len, msg, da, id);
	return (msg_len);
    }
    return(-1); /* should never happen -- here for cireport */
}

/********************************************************************
			
    Gets, from the system configuration file, the full path of data 
    store "data_id" and returns it in "path" of size "buf_size". 

    Returns 0 on success or a negative error code on failure. 

********************************************************************/
static int Get_data_store_path (int data_id, char *path, int buf_size) {
    int ret;
    char cr_cfg_name[MAX_NAME_SIZE], *cfg_name;

    cfg_name = CS_cfg_name (NULL);
    if (cfg_name != NULL) {
	strncpy (cr_cfg_name, cfg_name, MAX_NAME_SIZE);
	cr_cfg_name[MAX_NAME_SIZE - 1] = '\0';
    }
    CS_cfg_name ("");		/* make sure we read from sys_cfg */
    CS_parse_control (CS_NO_ENV_EXP);
    ret = CS_entry_int_key (data_id, ORPGSC_LBNAME_COL, buf_size, path);
    CS_parse_control (CS_YES_ENV_EXP);
    if (cfg_name != NULL)
	CS_cfg_name (cr_cfg_name);
    if (ret < 0)
	return (ret);

    if (Sys_cfg_local_host[0] != '\0' &&
	strstr (path, ":") == NULL) {
	int path_len, host_len;
	path_len = strlen (path);
	host_len = strlen (Sys_cfg_local_host);
	if (host_len + path_len + 1 >= buf_size) {
	    LE_send_msg (GL_ERROR, "ORPGDA: path name buffer too short");
	    return (ORPGDA_STRING_TOO_BIG);
	}
	memmove (path + host_len + 1, path, path_len + 1);
	strcpy (path, Sys_cfg_local_host);
	path[host_len] = ':';
    }
    return (0);
}

/********************************************************************
			
    Gets, from the system configuration file, the full path of data 
    store "data_id" for this stream and returns it in "path" of size 
    "buf_size". Returns the data ID found on success or a negative 
    error code on failure. 

********************************************************************/

static int Get_stream_data_path (int data_id, char *name, int n_s) {
    int stream_d_id, ret;

    stream_d_id = Get_stream_data_id (data_id);
    ret = Get_data_store_path (stream_d_id, name, n_s);
    if (ret >= 0)
	return (stream_d_id);
    else if (stream_d_id != data_id) {
	ret = Get_data_store_path (data_id, name, n_s);
	if (ret >= 0)
	    return (data_id);
	LE_send_msg (0, 
		"ORPGDA: Data stores (%s and %s) not found in syscfg (%d)\n", 
		Data_id_text (stream_d_id), Data_id_text (data_id), ret);
    }
    else
	LE_send_msg (0, "ORPGDA: Data store (%s) not found in syscfg (%d)\n", 
					Data_id_text (stream_d_id), ret);
    return (ret);
}

/********************************************************************
			
    Returns the data id for "id" and the current data stream. 

********************************************************************/

static char *Data_id_text (int data_id) {
    static char buf[64];
    if ((data_id >> ORPGDA_STREAM_SHIFT) == 0)
	sprintf (buf, "%d", data_id);
    else
	sprintf (buf, "%d-%d", data_id >> ORPGDA_STREAM_SHIFT, 
				data_id & ORPGDA_DATA_ID_MASK);
    return (buf);
}

/********************************************************************
			
    Returns the data id for "id" and the current data stream. 

********************************************************************/

static int Get_stream_data_id (int id) {

    if (Data_stream > 0 &&
	(id >> ORPGDA_STREAM_SHIFT) == 0 &&
	id >= ORPGDAT_BASE)
	id = ORPGDA_STREAM_DATA_ID (Data_stream, id);
    return (id);
}

/***********************************************************************

    Byte swaps the data "buf" of length "len" bytes. "dt" is the ORPGDA
    data type struct. "id" is the message ID. "len" is the return value 
    from LB_read. We don't process if len == LB_BUF_TOO_SMALL. We may 
    want to add it later.

***********************************************************************/

static char *Byte_swap_data (int io, int len, 
				char *buf, Da_type *dt, LB_id_t id) {
    static char *wbuf = NULL;
    static int wbuf_size = 0;
    char *type, *ret_buf;
    extern SMI_info_t *ORPG_smi_info (char *type_name, void *data);
    extern char *ORPG_smi_info_type_by_id (int major, int minor);
    int ret;

    if (len <= 0 || !dt->byteswap || buf == NULL)
	return (buf);
    if (dt->data_id < ORPGDAT_BASE || dt->data_id == ORPGDAT_PRODUCTS) {
						/* product */
	type = "Prod_header";
    }
    else {
	if (io == DA_INPUT && (id == LB_NEXT || id == LB_ANY))
	    id = LB_previous_msgid (dt->fd);
	type = Get_type_by_id (dt->data_id, id);
	if (type == NULL) {		/* data SMI not found */
	    dt->byteswap = 0;		/* disable it */
	    if (dt->data_id == ORPGSMI_EVENT_MESSAGE)
		LE_send_msg (GL_ERROR, 
		"ORPGDA: Byte swap type (event %d) not found", id);
	    else
		LE_send_msg (GL_ERROR, 
		"ORPGDA: Byte swap type (data %s, msgid %d) not found", 
					Data_id_text (dt->data_id), id);
	    return (buf);
	}
    }

    SMIA_set_smi_func (ORPG_smi_info);
    if (io == DA_INPUT) {
	ret = SMIA_bswap_input (type, buf, len);
	ret_buf = buf;
    }
    else {
	if (len > wbuf_size) {
	    if (wbuf != NULL)
		free (wbuf);
	    while ((wbuf = malloc (len + 1024)) == NULL) {
		LE_send_msg (GL_ERROR, "ORPGDA: malloc failed - retrying");
		sleep (1);
	    }
	    wbuf_size = len + 1024;
	}
	memcpy (wbuf, buf, len);
	ret = SMIA_bswap_output (type, wbuf, len);
	ret_buf = wbuf;
    }
    if (ret < 0 && ret != SMIA_DATA_TOO_SHORT)
	LE_send_msg (GL_ERROR, 
	    "ORPGDA: Byte swap data (%s, msg %d, type %s) failed (ret %d)", 
			Data_id_text (dt->data_id), id, type, ret);
    return (ret_buf);
}

/***********************************************************************

    Byte swaps an RPG event message "msg" of size "msg_len" for event
    "event". It calls Byte_swap_data to do the job.

***********************************************************************/

static char *Process_event_message (int where, EN_id_t event, 
						char *msg, int msg_len) {
#ifdef LITTLE_ENDIAN_MACHINE
    Da_type dt;
    int io;

    dt.byteswap = 1;
    dt.data_id = ORPGSMI_EVENT_MESSAGE;
    dt.fd = 0;
    if (where == EN_MSG_OUT)
	io = DA_OUTPUT;
    else
	io = DA_INPUT;

    return (Byte_swap_data (io, msg_len, msg, &dt, event));
#else
    return (msg);
#endif
}

/***********************************************************************

    Register the RPG event message processing function in EN.

***********************************************************************/

void ORPGDA_set_event_msg_byteswap_function () {
    EN_control (EN_SET_PROCESS_MSG_FUNC, Process_event_message);
}

/********************************************************************

    Checks write permission for data store "dt" and message ID "id".
    Returns non-zero if OK or zero if the permission is not OK. There
    is a segment of code in wp_test.c for testing this function.

********************************************************************/

static int Write_permission_OK (Da_type *dt, LB_id_t id) {
    int i, msg_id_explicit_match, wildcard_wp, cr_id_wp, a_id_not_wp;

    if (Bypass_write_permission_check)
	return (1);

    if (dt->wperm >= 0)
	return (dt->wperm);

    dt->wperm = 1;
    if (Task_name[0] == ';' && Task_name[1] == '\0') {
	LE_set_option ("LE disable", 1);
	ORPGTAT_get_my_task_name (Task_name, ORPG_TASKNAME_SIZ);
	LE_set_option ("LE disable", 0);
	if (Task_name[0] == '\0')
	    LE_send_msg (GL_ERROR, "Empty task name - need to call ORPGDA_open\n");
    }
    if (Task_name[0] == '\0')
	return (1);
    msg_id_explicit_match = 0;
    wildcard_wp = 0;
    cr_id_wp = 0;
    a_id_not_wp = 0;
    for (i = 0; i < dt->n_wps; i++) {
	Mrpg_wp_item *wp;
	int cr_msg_match;

	wp = dt->wps + i;
	if (id == LB_ANY && i > 0 && wp->msg_id != wp[-1].msg_id) {
	    if (!cr_id_wp)
		a_id_not_wp = 1;
	    cr_id_wp = 0;
	}

	cr_msg_match = 0;
	if (wp->msg_id == LB_ANY)
	    cr_msg_match = 2;		/* wild card match */
	else if (id == wp->msg_id) {
	    cr_msg_match = 1;		/* explicit match */
	    msg_id_explicit_match = 1;
	}

	if (cr_msg_match > 0 || id == LB_ANY) {
	    if (Match_task_name (Task_name, wp)) {
		if (id == LB_ANY)
		    cr_id_wp = 1;
		if (cr_msg_match == 1)
		    return (1);
		else if (cr_msg_match == 2)
		    wildcard_wp = 1;
	    }
	}
    }
    if (id == LB_ANY) {
	if (wildcard_wp && !a_id_not_wp && cr_id_wp)
	    return (1);
    }
    else if (msg_id_explicit_match == 0 && wildcard_wp)
	return (1);

    {	/* tmp code: we warn, instead of disallow, permission violation */
	LE_send_msg (GL_ERROR, 
		"Write permission needed for datastore %d\n", dt->data_id);
	return (1);
    }

    dt->wperm = 0;
    return (0);
}

/********************************************************************

    Checks if "task_name" matches Mrpg_wp_item "wp". Returns
    1 if true or 0 otherwise. Sufix defined by '.' in task_name is
    ignored in matching (e.g. p_server.3 matchs p_server).

********************************************************************/

static int Match_task_name (char *task_name, Mrpg_wp_item *wp) {
    char *p, *pw;

    if (wp->name[0] == '*')
	return (1);
    p = task_name;
    pw = wp->name;
    while (1) {
	if (*pw != *p)
	    break;
	if (*p == '\0')
	    return (1);		/* identical strings */
	p++;
	pw++;
    }
    if (*p != '.' || *pw != '\0')
	return (0);
    p++;
    while (*p != '\0') {
	if (*p == '.')		/* not the last '.' */
	    return (0);
	p++;
    }
    return (1);
}

/********************************************************************

    Returns the LB_open flag in terms of the specified LB_open "flag" 
    for data store with permission table "wps" of size "n_wps". Returns
    a negative error code on failure.

********************************************************************/

static int Get_lb_open_flag (int n_wps, Mrpg_wp_item *wps, int flag) {
    int i, has_wp;

    if (n_wps == 0 || (flag & LB_OPEN_AS_SPECIFIED) || 
	Bypass_write_permission_check)
	return (flag);

    if (Task_name[0] == ';' && Task_name[1] == '\0') {
	LE_set_option ("LE disable", 1);
	ORPGTAT_get_my_task_name (Task_name, ORPG_TASKNAME_SIZ);
	LE_set_option ("LE disable", 0);
	if (Task_name[0] == '\0')
	    LE_send_msg (GL_ERROR, "Empty task name - need to call ORPGDA_open\n");
    }
    if (Task_name[0] == '\0')
	return (flag);
    has_wp = 0;
    for (i = 0; i < n_wps; i++) {
	Mrpg_wp_item *wp;

	wp = wps + i;
	if (Match_task_name (Task_name, wp)) {
	    has_wp = 1;		/* has some write permission */
	    break;;
	}
    }
    if (has_wp)
	return (flag | LB_WRITE);
    return (flag);
}

extern int ORPG_smi_info_get_all_ids (int **majors, int **minors, char ***types);

/********************************************************************
			
    Returns the type name for IDS "major" and "minor". Returns NULL 
    if not found.

********************************************************************/

static char *Get_type_by_id (int major, int minor) {
    int stind, ind, i;

    stind = Get_index_by_major (major);
    if (stind < 0)
	return (NULL);

    ind = -1;
    for (i = stind; i < N_type_ids; i++) {
	if (Type_ids[i]->major != major)
	    break;
	if (minor >= 0 && Type_ids[i]->minor == minor)
	    return (Type_ids[i]->type);
	if (Type_ids[i]->minor < 0 && ind < 0) {
	    ind = i;
	    if (minor < 0)
		break;
	}
    }
    if (ind >= 0)
	return (Type_ids[ind]->type);
    return (NULL);
}

/********************************************************************
			
    Sorts the data store IDs and searches for the major ID "major".
    It returns the index in Type_ids on success or -1 on failure.

********************************************************************/

static int Get_index_by_major (int major) {
    int st, end, ind, i;

    if (Type_ids == NULL) {	/* initialize Type_ids */
	int *majors, *minors;
 	char **types;
	Type_id_t *tmp;

	N_type_ids = ORPG_smi_info_get_all_ids (&majors, &minors, &types); 
	while ((Type_ids = (Type_id_t **)malloc 
		(N_type_ids * (sizeof (Type_id_t) + 
				sizeof (Type_id_t *)))) == NULL)
	    msleep (1000);
	tmp = (Type_id_t *)((char *)Type_ids + 
				N_type_ids * sizeof (Type_id_t *));
	for (i = 0; i < N_type_ids; i++) {
	    Type_ids[i] = tmp;
	    tmp->major = majors[i];
	    tmp->minor = minors[i];
	    tmp->type = types[i];
	    tmp++;
	}
	Sort_major (N_type_ids, Type_ids);
    }

    if (N_type_ids == 0)
	return (-1);

    st = 0;
    end = N_type_ids - 1;
    ind = -1;
    while (1) {
	i = (st + end) >> 1;
	if (st == i) {
	    if (Type_ids[st]->major == major)
		ind = st;
	    else if (Type_ids[end]->major == major)
		ind = end;
	    break;
	}
	if (major <= Type_ids[i]->major)
	    end = i;
	else
	    st = i;
    }
    return (ind);
}

/********************************************************************
			
    Sort array "ra" of size "n". The sort is in-place.

********************************************************************/

static void Sort_major (int n, Type_id_t **ra) {
    int l, j, ir, i;
    Type_id_t *rra;				/* type dependent */

    if (n <= 1)
	return;
    ra--;
    l = (n >> 1) + 1;
    ir = n;
    for (;;) {
	if (l > 1)
	    rra = ra[--l];
	else {
	    rra = ra[ir];
	    ra[ir] = ra[1];
	    if (--ir == 1) {
		ra[1] = rra;
		return;
	    }
	}
	i = l;
	j = l << 1;
	while (j <= ir) {
	    if (j < ir && ra[j]->major < ra[j + 1]->major)
		++j;				/* type dependent */
	    if (rra->major < ra[j]->major) {	/* type dependent */
		ra[i] = ra[j];
		j += (i = j);
	    }
	    else
		j = ir + 1;
	}
	ra[i] = rra;
    }
}


