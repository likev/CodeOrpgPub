
/****************************************************************
		
    Module: rss_def.h	
				
    Description: This file defines all private objects that are 
	required to build the server as well as the RSS library. 
	This file is not needed for application programs that use
	RSS library routines.

****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/05/19 19:35:12 $
 * $Id: rss_def.h,v 1.29 2011/05/19 19:35:12 jing Exp $
 * $Revision: 1.29 $
 * $State: Exp $
 * $Log: rss_def.h,v $
 * Revision 1.29  2011/05/19 19:35:12  jing
 * Update
 *
 * Revision 1.27  2002/03/18 22:40:14  jing
 * Update
 *
 * Revision 1.26  2002/03/12 17:03:39  jing
 * Update
 *
 * Revision 1.25  1999/06/25 15:11:44  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.22  1999/04/09 16:09:45  eforren
 * Change RSS_THREADED to THREADED
 *
 * Revision 1.21  1999/03/31 22:56:14  eforren
 * Reverted from version 1.19
 *
 * Revision 1.19  1999/03/31 18:32:22  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.12  1998/07/06 18:24:33  eforren
 * Allow compressed data buffer to be larger than non-compressed data buffer
 *
 * Revision 1.11  1998/06/19 17:06:22  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.1  1996/06/04 16:44:18  cm
 * Initial revision
 *
 * 
*/

#ifndef RSS_DEF_H
#define RSS_DEF_H


#define RSS_MAX_DATA_LENGTH RMT_PREFERRED_SIZE 
				/* maximum data length read/write; We use the
				   RMT preferred size defined in rmt.h for max 
				   efficiency */

/* size of a static buffer shared by all client side modules */
#define STATIC_BUFFER_SIZE	RSS_MAX_DATA_LENGTH + 256
				/* assuming that LB_WRITE_HD <= 256 and 
				   WRITE_HD <= 256; Note that this must
				   also be sufficient for the server */

#define PATH_LIST_SIZE 64	/* maximum number of config. pathes */
#define STRING_SIZE 256		/* size of the string processed */
#define TMP_SIZE 256			/* local temp buffer size */

/* we use the type rmt_t defined in rmt.h for the header */
#define OPEN_MSG 300			/* message size for rss_open */
#define READ_HD 2 * sizeof (rmt_t)	/* header size for rss_read */
#define WRITE_HD 2 * sizeof (rmt_t)	/* header size for rss_write */

#define LB_OPEN_MSG 512			/* message size for rss_LB_open */
#define LB_LIST_HD 8 * sizeof (rmt_t)	/* header size for rss_LB_list */

/* header size and fields for rss_LB_read */
#define LB_READ_HD 7 * sizeof (rmt_t)
enum {LRRQ_FD, LRRQ_BSIZE, LRRQ_ID, LRRQ_NB, LRRQ_CMPRS};
enum {LRRT_LBRET, LRRT_ERRNO, LRRT_ID, LRRT_N_LEFT, LRRT_T_LEN, LRRT_TAG, LRRT_COMPRESS};

/* header size and fields for rss_LB_write */
#define LB_WRITE_HD 6 * sizeof (rmt_t)
enum {LWRQ_FD, LWRQ_STEP, LWRQ_LEN, LWRQ_ID, LWRQ_TAG, LWRQ_PARAMS};
enum {LWRT_LBRET, LWRT_ERRNO, LWRT_ID, LWRT_FHOST};

/* fields for rss_LB_set_req */
#define LB_WUR_HD (5 * sizeof (rmt_t))
#define LB_WUR_RT_LEN (3 * sizeof (rmt_t))
enum {WUR_NOTIFY, WUR_WAIT};	/* function type, WUR_WAIT no long used*/
/* for LB_set_an_req */
enum {LNTFQ_TYPE, LNTFQ_FD, LNTFQ_HOST, LNTFQ_PID, LNTFQ_MSGID};
enum {LNTFT_RET, LNTFT_APID, LNTFT_AFD};

#define LB_STAT_N_IDS 256		/* max number of id processed in 
					   LB_stat */

/* function commands used for rss_LB_generic function */
enum {CALL_LB_CLOSE, CALL_LB_MISC, CALL_LB_PREVIOUS_MSGID, CALL_LB_SET_POLL,
CALL_LB_REGISTER, CALL_LB_READ_WINDOW, CALL_LB_SET_TAG, CALL_LB_LOCK, CALL_LB_CLEAR, CALL_LB_WRITE_FAILED_HOST, CALL_LB_DELETE, CALL_LB_SDQS_ADDRESS, CALL_ORPGDA_LB_OPEN};
enum {CALL_LB_SEEK, CALL_LB_MSG_INFO};

/* Macro processing combined fd (file descriptor) */
#define COMBINE_FD(cfd,fd) (((cfd + 1) << 16) + fd)
					/* form combined fd from the comm fd
					   and file fd */
#define GET_CFD(cmfd) ((cmfd >> 16) - 1)	/* retrieve comm fd */
#define GET_FFD(cmfd) (cmfd & 0xffff)	/* retrieve file fd */
#define IS_LOCAL(fd) (fd == -1)    	/* test if a comm fd is local */

/* return value of RSS_find_host_name */
enum {LOCAL_HOST_IMPLICIT, REMOTE_HOST, LOCAL_HOST_EXPLICIT};

/* private functions */
int RSS_find_host_name (const char *name, char *host_name, char *path, int size);

int RSS_check_file_permission (char *path);
int RSS_add_home_path (char *path, int path_size);

char *RSS_shared_buffer ();		/* used by both client and server */
void RSS_set_send_log (void (*send_log)());

char *RSS_compress (int length, char *data, int hd_size, int *ret);
char *RSS_decompress (int length, char *data, int hd_size, int *ret);
int RSS_compress_available ();
void RSS_compress_free_buffer (char *buf);


#endif		/* #ifndef RSS_DEF_H */
