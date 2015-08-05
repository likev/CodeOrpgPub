
/****************************************************************
		
    Module: rss_replace.h	
				
    Description: This header file contains function name replacement
	directives for certain RSS functions. When this file is 
	included instead of rss.h, The user can use RSS services 
	with their original function names (open, read and so on).

****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2003/11/14 16:08:54 $
 * $Id: rss_replace.h,v 1.16 2003/11/14 16:08:54 jing Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 * $Log: rss_replace.h,v $
 * Revision 1.16  2003/11/14 16:08:54  jing
 * Updated
 *
 * Revision 1.16  2002/03/18 22:31:07  jing
 * Update
 *
 * Revision 1.15  2002/03/12 16:45:05  jing
 * Update
 *
 * Revision 1.14  1999/04/27 20:42:41  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.13  1999/03/03 20:27:40  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.11  1998/12/22 20:28:48  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.10  1998/12/02 22:07:16  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.9  1998/12/01 20:42:57  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.6  1998/06/19 16:55:42  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.5  1998/03/13 16:59:24  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.4  1997/01/21 22:25:11  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.3  96/10/15  15:43:48  15:43:48  jing (Zhongqi Jing)
 * NO COMMENT SUPPLIED
 * 
 * Revision 1.1  1996/06/04 16:49:55  cm
 * Initial revision
 *
*/

#ifndef RSS_REPLACE_H
#define RSS_REPLACE_H

#include <rss.h>

#define LB_open(a,b,c) RSS_LB_open(a,b,c)
#define LB_read(a,b,c,d) RSS_LB_read(a,b,c,d)
#define LB_write(a,b,c,d) RSS_LB_write(a,b,c,d)
#define LB_close(a) RSS_LB_close(a)
#define LB_remove(a) RSS_LB_remove(a)

#define LB_seek(a,b,c,d) RSS_LB_seek(a,b,c,d)
#define LB_msg_info(a,b,c) RSS_LB_msg_info(a,b,c)
#define LB_clear(a,b) RSS_LB_clear(a,b)
#define LB_delete(a,b) RSS_LB_delete(a,b)
#define LB_stat(a,b) RSS_LB_stat(a,b)
#define LB_list(a,b,c) RSS_LB_list(a,b,c)
#define LB_misc(a,b) RSS_LB_misc(a,b)
#define LB_previous_msgid(a) RSS_LB_previous_msgid(a)
/* #define LB_write_failed_host(a) RSS_LB_write_failed_host(a) */

#define LB_direct(a,b,c) RSS_LB_direct(a,b,c)
#define LB_set_poll(a,b,c) RSS_LB_set_poll(a,b,c)
#define LB_lock(a,b,c) RSS_LB_lock(a,b,c)
#define LB_set_tag(a,b,c) RSS_LB_set_tag(a,b,c)
#define LB_register(a,b,c) RSS_LB_register(a,b,c)
#define LB_read_window(a,b,c) RSS_LB_read_window(a,b,c)

#define LB_sdqs_address(a,b,c,d) RSS_LB_sdqs_address(a,b,c,d)

#endif 		/* #ifndef RSS_REPLACE_H */
