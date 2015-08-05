
/***********************************************************************

    Description: Internal include file for hci_up_nb functions.

***********************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2006/03/22 15:27:13 $
 * $Id: hci_up_nb.h,v 1.5 2006/03/22 15:27:13 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef HCI_UP_NB_H

#define HCI_UP_NB_H

#include <infr.h>
#include <orpg.h>
#include <prod_status.h>
#include <prod_distri_info.h>

#define MAX_DIAL_USERS	500	/* Maximum dial users allowed. Used	*
				 * for SDQM query.			*/
#define	NB_HAS_NO_CONNECTIONS		1
#define	NB_HAS_CONNECTIONS		2
#define	NB_HAS_FAILED_CONNECTIONS	3

typedef struct {
    short line_num;
    short type;		/* Pd_line_entry.line_type (DEDICATED, DIAL_IN) */	
    short protocol;	/* Pd_line_entry.protocol (PROTO_X25, PROTO_TCP) */	
    short user_id;	/* Prod_user_status.uid (short); -1 if not available */
    short status;	/* STATUS_*. src: Prod_user_status.link 
			   (US_DISCONNECTED, US_CONNECT_PENDING, 
			   US_CONNECTED) */
    short state;	/* Prod_user_status.line_stat
			   (US_LINE_NORMAL, US_LINE_NOISY, US_LINE_FAILED,
			    US_UNABLE_CONN_DEDIC) */
    short util;		/* Prod_user_status.util (in percentage) */
    short rate;		/* Prod_user_status.rate.
			   achieved baud rate (in percentage) */
    short uclass;	/* user class, Prod_user_status.uclass; -1 if not 
			   defined */
    short enable;	/* boolean: non-zero for enabled and zero for disabled 
			   (src: Pd_line_entry.link_state) */
    short selected;	/* the line is currently selected by the user */
    short spare;

    char  str[128];	/* for line status string */
} Line_status;

/*	SMI_struct Line_status;	*/

typedef struct {
    int line_num;
    int type;
    char port_pswd[PASSWORD_LEN];
			/* Pd_line_entry.port_password */
    int baud_rate;	/* Pd_line_entry.baud_rate */
    int pserver_num;	/* Pd_line_entry.p_server_ind; -1 if not defined */
    int comms_mgr_num;	/* Pd_line_entry.cm_ind; -1 if not defined */
    int max_conn_time;	/* Pd_line_entry.conn_time_limit */
    int packet_size;	/* Pd_line_entry.packet_size */
    int protocol;	/* Pd_line_entry.protocol */
    int	defined;	/* Pd_line_entry.defined */
    int user_id;
    char user_name[USER_NAME_LEN];
			/* UP */
    int	distri_method;	/* UP */
    int uclass;		/* user class */

    int retries;	/* Pd_distri_info.nb_retries */
    int timeout;	/* Pd_distri_info.nb_timeout */
    int alarm;		/* Pd_distri_info.ol_alarm */
    int warning;	/* Pd_distri_info.ol_warning */
} Line_details;

/*	Line table info	*/

typedef struct {
    int sizeof_data;			/* Number of elements in table */
    Line_details *data;			/* table data */
} Line_details_tbl_t;

/*	SMI_struct Line_details;	*/
/*	SMI_struct Line_details_tbl_t;	*/

/*	Dialup user info from User Profile DB.			*/

typedef struct {
    int msg_id;				/* Message ID for user record in LB */
    int user_id;			/* User ID */
    char user_name[USER_NAME_LEN];	/* User name */
    char user_password[PASSWORD_LEN];	/* User Password */
    int max_connect_time;		/* Maximum connect time */
    int	disconnect_override;		/* disconnect override flag
					   0 = not set, 1 = set. */
    int	class;				/* Class associated with this user */
    int	distri_method;			/* Distribution method */
    int defined;			/* Properties defined bits */
    int	delete_flag;			/* Record deleted = 1;*/
} Dial_details;

/*	Dialup table info	*/

typedef struct {
    int sizeof_data;			/* Number of elements in table */
    Dial_details *data;			/* table data */
} Dial_details_tbl_t;

/*	SMI_struct Dial_details;	*/
/*	SMI_struct Dial_details_tbl_t;	*/

/*	Class info from User Profile DB.			*/

typedef struct {
    int	class;				/* Class identifier */
    int	line_ind;			/* User type allowed */
    char name[USER_NAME_LEN];		/* Class name (distribution method )*/
    int	distri_method;			/* distribution method associated
					 * with this class. */
    int defined;			/* Properties defined bits */
    int	max_connect_time;		/* Maximum connect time */
} Class_details;

/*	Class table info	*/

typedef struct {
    int sizeof_data;			/* Number of elements in table */
    Class_details *data;		/* table data */
} Class_details_tbl_t;

/*	SMI_struct Class_details;	*/
/*	SMI_struct Class_details_tbl_t;	*/

/*	Function prototypes	*/

Line_details *hci_up_nb_get_line_details (Line_status *in);

int	hci_up_nb_update_dial_user_table      (void *out);
int	hci_up_nb_update_dedicated_user_table (void *out);
int	hci_up_nb_update_class_table          (void *out);

void	hci_read_nb_connection_status();
int	hci_get_nb_connection_status();

#endif		/* #ifndef HCI_UP_NB_H */

