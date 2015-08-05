/********************************************************************

    This file implements the EN functions.

********************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/09/14 15:40:44 $
 * $Id: en.c,v 1.12 2005/09/14 15:40:44 jing Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <config.h>
#include <stdarg.h>
#include <signal.h>

#include <en.h>
#include <lb.h>

int EN_post_event (EN_id_t event) {
    return (EN_post_msgevent ((EN_id_t)event, NULL, 0));
}

int EN_deregister (EN_id_t event, void (*encallback)(EN_id_t, char *, int, void*))
{

    EN_control (EN_DEREGISTER);
    return (EN_register ((EN_id_t)event, encallback));
}

int EN_post (EN_id_t event, const void *msg, int msglen, int msg_flags)
{

    if (msg_flags & EN_POST_FLAG_DONT_NTFY_SENDER)
	EN_control (EN_NOT_TO_SELF);
    if (msg_flags & EN_POST_FLAG_NTFY_SENDER_ONLY)
	EN_control (EN_TO_SELF_ONLY);
    return (EN_post_msgevent (event, (char *)msg, msglen));
}

int EN_cntl(int cmd, ...)
{
    va_list args;
    int sw, ret;

    if (cmd != EN_CNTL_STATE) {
	fprintf (stderr, "unkown EN_cntl cmd\n");
	return (0);
    }

    va_start (args, cmd);
    sw = va_arg (args, int);
    ret = 0;
    switch (sw) {
	int s;

	case EN_STATE_BLOCKED:
	    EN_control (EN_BLOCK);
	    ret = EN_STATE_BLOCKED;
	    break;

	case EN_STATE_UNBLOCKED:
	    EN_control (EN_UNBLOCK);
	    ret = EN_STATE_UNBLOCKED;
	    break;

	case EN_STATE_WAIT:
	    s = va_arg (args, int);
	    if (s == 0)
		s = 0x100000;
	    EN_control (EN_WAIT, s * 1000);
	    break;

	case EN_CANCEL_WAIT:
	case EN_QUERY_STATE:
	    fprintf (stderr, "EN_cntl cmd %d %d not supported\n", cmd, sw);
	    ret = -1;
	    break;

	default:
	    ret = -1;
	    fprintf (stderr, "EN_cntl cmd %d %d not expected\n", cmd, sw);
	    break;
    }

    va_end (args);
    return (ret);
}

/**************************************************************************

    Description: This public function supports object-oriented clients by
              allowing them to provide a pointer to an object that they
              wish to access upon delivery of a given event.

    Returns: The value returned by EN_register.

**************************************************************************/

int EN_register_obj (EN_id_t event, void (*encallback)(EN_id_t, char *, int, void*), void *obj_ptr)
{

   EN_control (EN_PUSH_ARG, obj_ptr);
   return (EN_register (event, encallback));
}

int EN_cntl_block_with_resend(void)
{
    int ret;

    ret = EN_control (EN_REDELIVER);
    if (ret < 0)
	return (ret);
    else
	return (EN_STATE_BLOCKED);
}

int EN_cntl_block(void)
{
    return (EN_cntl (EN_CNTL_STATE, EN_STATE_BLOCKED));
}

int EN_cntl_unblock(void)
{
    return (EN_cntl (EN_CNTL_STATE, EN_STATE_UNBLOCKED));
}

int EN_cntl_wait(unsigned int wait_sec)
{
    return (EN_cntl (EN_CNTL_STATE, EN_STATE_WAIT, wait_sec));
}

int EN_cntl_get_state (void)
{
    int ret;

    ret = EN_control (EN_GET_BLOCK_STATE);
    if (ret)
	return (EN_STATE_BLOCKED);
    else
	return (EN_STATE_UNBLOCKED);
}





