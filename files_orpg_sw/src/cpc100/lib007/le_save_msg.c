/****************************************************************
		
    Module: le_save_msg.c	
		
    Description: This module contains the LE functions that save
		and retieve messages.

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/05/24 21:15:25 $
 * $Id: le_save_msg.c,v 1.14 2007/05/24 21:15:25 jing Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */  


#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <infr.h>
#include <le_def.h>


#define LE_MSG_BUFFER_SIZE	1024

static int St_off = 0;		/* offset in buffer to the first msg */
static int Free_off = 0;	/* offset in buffer to the free space */
static char Buffer[LE_MSG_BUFFER_SIZE];
				/* static buffer for storing messages */

static int Get_msg_length (char *msg);

/********************************************************************
			
    Description: This function saves a message of "msg". If there 
		is no room for saving the message, the message is 
		simply not saved. The messages are stored in buffer
		consecutively for efficient memory utilization. Note
		that this is based on the fact that all messages will 
		be read when the retrieval starts.

    Input:	msg - Pointer to the message structure.

********************************************************************/

void LE_save_message (char *msg)
{
    int len, alen;

    len = Get_msg_length (msg);
    alen = ALIGNED_SIZE (len);

    if (Free_off + alen <= LE_MSG_BUFFER_SIZE) {
	memcpy (Buffer + Free_off, msg, len);
	Free_off += alen;
    }

    return;
}

/********************************************************************
			
    Description: This function returns the oldest message in the saved
		message poll. If there is no message, it creates a
		special message and returns it.

    Return:	The pointer to the retrieved message.

********************************************************************/

char *LE_get_saved_msg ()
{

    if (St_off < Free_off) {
	char *pt;

	pt = Buffer + St_off;
	St_off += ALIGNED_SIZE (Get_msg_length (pt));
	if (St_off >= Free_off) {
	    St_off = Free_off = 0;	/* the buffer is reused */
	}
	return (pt);
    }
    else {			/* no message available */
	static char lost_text[] = "LE: a msg lost";
	static ALIGNED_t buf[ALIGNED_T_SIZE (sizeof (LE_critical_message) + 
						sizeof (lost_text))];
	LE_critical_message *msg;
	char tmp[LE_SOURCE_NAME_SIZE];

	/* process the source name */
	MISC_string_fit (tmp, LE_SOURCE_NAME_SIZE, 
		MISC_STRING_FIT_MIDDLE, '*', MISC_string_basename (__FILE__));

	msg = (LE_critical_message *)buf;
	msg->code = LE_CRITICAL_BIT;
	msg->pid = getpid ();
	msg->line_num = __LINE__;
	msg->n_reps = 0;
	strcpy (msg->fname, tmp);
	strcpy (msg->text, lost_text);

	return ((char *)buf);
    }
}

/********************************************************************
			
    Description: This function returns the length of an LE msg.

    Input	msg - pointer to the LE message.

    Return:	length of the message.

********************************************************************/

static int Get_msg_length (char *msg)
{
    unsigned int code;
    int len;

    code = *((unsigned int *)msg);
    if (code & LE_CRITICAL_BIT) {
	LE_critical_message *crmsg;

	crmsg = (LE_critical_message *)msg;
	len = sizeof (LE_critical_message) + strlen (crmsg->text);
    }
    else {
	LE_message *lemsg;

	lemsg = (LE_message *)msg;
	len = sizeof (LE_message) + strlen (lemsg->text);
    }
    return (len);
}

