
/******************************************************************

    Module: rmt_user_def.h	
				
    Description:  This file defines all user RPC functions for RSS
	as required by the RMT tool.

******************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2003/11/14 16:08:49 $
 * $Id: rmt_user_def.h,v 1.20 2003/11/14 16:08:49 jing Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 * $Log: rmt_user_def.h,v $
 * Revision 1.20  2003/11/14 16:08:49  jing
 * Updated
 *
 * Revision 1.19  2002/03/12 17:03:34  jing
 * Update
 *
 * Revision 1.18  2000/09/25 21:33:49  jing
 * @
 *
 * Revision 1.16  2000/08/22 14:04:00  jing
 * @
 *
 * Revision 1.14  1999/03/03 20:31:06  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.8  1998/06/19 17:06:16  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.1  1996/06/05 13:23:26  cm
 * Initial revision
 *
*/

#ifndef RMT_USER_DEF_H
#define RMT_USER_DEF_H


/* #define RMT_SECURITY_ON  */
#define RMT_USER_FUNCS_INIT_CALLED

/* The file access package */
#define rss_open(a, b, c) Rmt_user_func_1(a, b, c)
#define rss_read(a, b, c) Rmt_user_func_2(a, b, c)
#define rss_write(a, b, c) Rmt_user_func_3(a, b, c)
#define rss_close(a, b, c) Rmt_user_func_4(a, b, c)
#define rss_lseek(a, b, c) Rmt_user_func_5(a, b, c)
#define rss_unused6(a, b, c) Rmt_user_func_6(a, b, c)
#define rss_unused7(a, b, c) Rmt_user_func_7(a, b, c)


#define rss_unused8(a, b, c) Rmt_user_func_8(a, b, c)
#define rss_unused9(a, b, c) Rmt_user_func_9(a, b, c)
#define rss_kill(a, b, c) Rmt_user_func_10(a, b, c)
#define rss_time(a, b, c) Rmt_user_func_11(a, b, c)
#define rss_unused12(a, b, c) Rmt_user_func_12(a, b, c)


/* The LB package */
#define rss_LB_open(a, b, c) Rmt_user_func_13(a, b, c)
#define rss_LB_read(a, b, c) Rmt_user_func_14(a, b, c)
#define rss_LB_write(a, b, c) Rmt_user_func_15(a, b, c)
#define rss_LB_generic(a, b, c) Rmt_user_func_16(a, b, c)
#define rss_LB_remove(a, b, c)  Rmt_user_func_17(a, b, c)
#define rss_LB_seek(a, b, c) Rmt_user_func_18(a, b, c)
#define rss_LB_set_nr(a, b, c) Rmt_user_func_19(a, b, c)
#define rss_LB_stat(a, b, c) Rmt_user_func_20(a, b, c)
#define rss_LB_list(a, b, c) Rmt_user_func_21(a, b, c)
/* #define rss_rpc(a, b, c) Rmt_user_func_22(a, b, c) */


#define Rmt_def_user_func 22
/* #define Rmt_def_user_func 22 - for RSS_rpc */

#endif		/* #ifndef RMT_USER_DEF_H */

