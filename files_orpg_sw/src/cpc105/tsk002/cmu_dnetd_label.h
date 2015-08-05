/*   @(#) label.h 99/11/02 Version 1.4   */

/*
 *  STREAMS Network Daemon
 *
 *  Copyright (c) 1988-1997 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Written by Mark Valentine.
 *
 *  Made in Scotland.
 *
 *  @(#)$Id: cmu_dnetd_label.h,v 1.1 2000/10/20 15:20:18 jing Exp $
 */

/* 
Modification history:

Chg Date    	Init Description
1.   8-jun-98   rjp  Add support for uconx open.
*/

#ifndef _label_h
#define _label_h

struct label {
    struct label	*lower;    /* label of lower label after I_LINK */
    struct label 	*ass_label;
    int         	type;
    int         	value;
    int         	control;
    int         	refs;    /* Number of STREAMs which refer to label */
    char         	*name;
    UCONX_DEVICE	*pDevice;	/* #1				*/
};

/* label types */

#define NETD_FDESC_TYPE    1
#define NETD_MUXID_TYPE    2

extern char *netd_generate_label(void);
extern int netd_store_label(char *, int, UCONX_DEVICE *, char *, int, char *);
extern int netd_lookup_label(char *, int);
extern int netd_rename_label(char *, char *);
extern int netd_copy_label(char *, char *);
extern int netd_delete_label(char *);
extern void netd_delete_all_labels(void);
extern void netd_print_labels(void);
extern void netd_mark_control_label(char *);
extern UCONX_DEVICE *netd_control_label(char *);
extern char *get_lower_label(char *);
extern void delete_lower_label(char *);

#endif /* _label_h */
