
/*******************************************************************

    Module: lb.h

    Description: Public header file for the linear buffer (LB) module.

*******************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:51 $
 * $Id: lb.h,v 1.96 2012/06/14 18:57:51 jing Exp $
 * $Revision: 1.96 $
 * $State: Exp $
 * $Log: lb.h,v $
 * Revision 1.96  2012/06/14 18:57:51  jing
 * Update
 *
 * Revision 1.93  2007/05/08 21:59:24  jing
 * Update
 *
 * Revision 1.81  2002/05/20 20:13:57  jing
 * Update
 *
 * Revision 1.81  2002/03/18 22:31:04  jing
 * Update
 *
 * Revision 1.80  2002/03/12 16:44:57  jing
 * Update
 *
 * Revision 1.79  2000/08/31 21:06:37  jing
 * @
 *
 * Revision 1.78  2000/08/21 20:42:38  jing
 * @
 *
 * Revision 1.77  2000/04/19 20:21:41  jing
 * @
 *
 * Revision 1.76  2000/04/18 18:19:44  jing
 * @
 *
 * Revision 1.75  2000/03/08 17:18:09  jing
 * @
 *
 * Revision 1.6  96/10/22  15:26:56  15:26:56  jing (Zhongqi Jing)
 * NO COMMENT SUPPLIED
 * 
 * Revision 1.1  96/06/04  16:49:36  16:49:36  cm (OS RPG Configuration Management Account)
 * Initial revision
 * 
*/

#ifndef LB_H
#define LB_H

#include <sys/types.h>
#include <signal.h>

/*  include <sys/varargs.h> for Solaris CC compiler because */
/*  stdarg.h caused error below
	used preprocessor to get rid of the problem for now
"/usr/include/stdarg.h", line 115: Error: No direct declarator preceding "(".
1 Error(s) detected.
*/

#if (defined(__cplusplus) && !defined(__GNUC__))
#include <sys/varargs.h>
#else	
#include <stdarg.h> 
#endif  


typedef unsigned int LB_id_t;		/* type for LB message id; This must be
					   of the same size as lb_t because they
					   share the same byte swap function */

#define LB_REMARK_LENGTH	64	/* remark string length including the 
					   NULL terminate */
#define LB_MAXN_TAG_BITS	32	/* maximum tag size */

#define LB_VARIABLE_MAXN_MSGS	0x80000000
					/* bit flag for LB_attr.maxn_msgs */

typedef struct {	/* LB attribute structure for input and output */
    char remark[LB_REMARK_LENGTH];
			/* remark string */
    mode_t mode;	/* access permission */
    int msg_size;	/* average message size */
    int maxn_msgs;	/* maximum number of messages. If bit 
			   LB_VARIABLE_MAXN_MSGS is set, it specifies:
			   LB_VARIABLE_MAXN_MSGS | (range << 24) | init_maxn */
    int types;		/* flags defining the LB type */
    int tag_size;	/* tag size in number of bits 
			   0 <= and <= LB_MAXN_TAG_BITS;
			   The notification request area size is specified
			   on the three most significant bytes. */
    int version;	/* the LB version number */
} LB_attr;

/* bit flags for LB_attr.types */
#define LB_FILE		0
#define LB_MEMORY	0x10
#define LB_REPLACE	0x20		/* for compatibily */
#define LB_QUEUE	0
#define LB_NORMAL	0		/* for compatibily */
#define LB_MSG_QUEUE	0
#define LB_MUST_READ	0x40
#define LB_SINGLE_WRITER 0x100
#define LB_NOEXPIRE	0x200
#define LB_UNPROTECTED	0x400
#define LB_DIRECT	0x800
#define LB_UN_TAG	0x1000
#define LB_MSG_POOL	0x2000		/* for compatibily */
#define LB_DB		0x4000
#define LB_SHARE_STREAM	0x8000		/* data stream shared by readers */

#define LB_POOL		LB_DB		/* for compatibily */

#define LB_INDEX_ID	0x10000		/* not checked */

/* bit flags for the flags argument of LB_open */
#define LB_READ		0
#define LB_WRITE	0x1
#define LB_CREATE	0x2

/* special value for LB_read buflen argument */
#define LB_ALLOC_BUF	0

/* maximum allowed message id */
#define LB_MAX_ID	(LB_id_t)0xfffbffff

/* special values for the arguments of LB functions */
#define LB_NEXT		(LB_id_t)0xffffffff
#define LB_ANY		(LB_id_t)0xffffffff
#define LB_FIRST	(LB_id_t)0xfffffffd
#define LB_LATEST	(LB_id_t)0xfffffffe
#define LB_CURRENT	(LB_id_t)0xffffffff
#define LB_N_UNREAD	(LB_id_t)0xfffffffa
#define LB_SHARED_READ	(LB_id_t)0xfffffffc
#define LB_READ_ENTIRE_LB	(LB_id_t)0xfffffff9
#define LB_MSG_UPDATED	(LB_id_t)0xffffffff
#define LB_MSG_NOCHANGE	(LB_id_t)0xfffffffe
#define LB_MSG_NOT_FOUND	(LB_id_t)0xfffffffd
#define LB_ALL		0x7fffffff 
#define LB_TAG_LOCK	(LB_id_t)0xffffffff
#define LB_LB_LOCK	(LB_id_t)0xfffffffe
#define LB_MSG_EXPIRED	(LB_id_t)0xfffffffc
#define LB_GET_NRS	(LB_id_t)0xfffffffb

/* used for LB_read LB_MULTI_READ specification */
#define LB_MULTI_READ	(LB_id_t)0xfffd0000
#define LB_MULTI_READ_FULL  (LB_id_t)0xfffc0000
#define LB_MULTI_READ_NUM	0x3fff
#define LB_MR_COMP(x)	(((x) + 1) << 14)

/* used for UN message ID group specification */
#define LB_UN_MSGID_TEST  0xfffe0000
#define LB_UN_MSGID_MASK  0xffff
#define LB_UN_MSGID_GROUP(x)  (((x) & LB_UN_MSGID_MASK) | LB_UN_MSGID_TEST)

/* values for the arguments "type" of LB_register */
enum {LB_ID_ADDRESS, LB_TAG_ADDRESS, LB_UP_ADDRESS, LB_UP_VALUE};


/* return value in argument "info->size" of LB_seek */
#define LB_SEEK_TO_COME	-1
#define LB_SEEK_EXPIRED	-2


typedef struct {	/* data structure for LB_list */
    LB_id_t id;		/* the message id */
    int size;		/* size of the message */
    int mark;		/* the mark value of the message */
} LB_info;

typedef struct {	/* data structure used for message based update check */
    LB_id_t id;		/* msg id */
    int status;		/* msg status */
} LB_check_list;

typedef struct {	/* data structure for LB_stat */
    LB_attr *attr;	/* The LB attributes */
    time_t time;	/* LB creation time */
    int n_msgs;		/* number of messages in the LB */
    int updated;	/* LB_TRUE or LB_FALSE - update status (the LB has 
			   been updated since last LB_stat call) */
    int n_check;	/* The size of check_list */
    LB_check_list *check_list;	/* message based update check list */ 
} LB_status;

enum {LB_FALSE = 0, LB_TRUE};

#define LB_SUCCESS 0	/* function return value on success */
#define LB_FAILURE -1	/* function return value on failure */

#define LB_TO_COME  -38	/* return value indicating that the message is 
			   yet to come */
#define LB_FULL	   -39	/* return value indicating that the LB is full 
			   due to unread messages that can not be expired
			   (LB_MUST_READ is set) */

enum {LB_UNLOCK, LB_SHARED_LOCK, LB_EXCLUSIVE_LOCK, 
			LB_TEST_SHARED_LOCK, LB_TEST_EXCLUSIVE_LOCK};
			/* values for argument "command" of LB_lock */
#define LB_BLOCK 0x100	/* flag for argument "command" of LB_lock */


/* The tag_size field of LB_attr is a combination of the tag size and the 
   wake-up request area size */
#define TAG_SIZE_MASK	0xff	/* bit mask for tag size */
#define NRA_SIZE_SHIFT	8	/* bit shift for notific. request area size */
#define LB_NR_FACTOR	8	/* multiplication factor to nra_size for total 
				   available number of NR records */

#define LB_MAXN_NRS	(254 * LB_NR_FACTOR)
				/* maximum notification request area (one 
				   byte representation internally) */
#define LB_DEFAULT_NRS	(255 * LB_NR_FACTOR)

enum {				/* values for LB_misc "cmd" argument */
    LB_SET_ACTIVE_TEST,		/* set LB_write server active test */
    LB_UNSET_ACTIVE_TEST,	/* unset LB_write server active test */
    LB_SET_ACTIVE_LOCK,		/* set server active lock */
    LB_UNSET_ACTIVE_LOCK,	/* unset server active lock */
    LB_GET_LB_SIZE,		/* get the space the LB used */
    LB_GET_UNIQUE_MSGID,	/* get a unique msgid for client/server apps */
    LB_GAIN_EXCLUSIVE_LOCK,	/* gain and hold LB exclusive lock */
    LB_RELEASE_EXCLUSIVE_LOCK,	/* release LB exclusive lock */
    LB_VALIDATE_HEADERS,	/* validate the LB header structures */
    LB_IS_BIGENDIAN,		/* returns the is_bigendian LB header field */
    LB_UNSET_DERECT_ACCESS,	/* stop current direct file LB access */
    LB_CHECK_AND_WRITE,		/* set the next LB_write to check and write */
    LB_DISCARD_UNREAD_IN_SHARED_STREAM,
				/* sets the shared read pointer to the incoming
				   new message for LB_SHARE_STREAM lb */
    LB_N_MUST_READERS		/* sets the number of readers for LB_MUST_READ
				   type */
};
#define LB_SET_N_MUST_READERS	(LB_N_MUST_READERS << 16)


/* error returns; Refer to lb.doc for detailed descriptions. */
#define LB_TOO_MANY_OPENED	-40	/* not used */
#define LB_BAD_ARGUMENT		-41
#define LB_MALLOC_FAILED	-42
#define LB_OPEN_FAILED		-43

#define LB_MMAP_FAILED		-44
#define LB_PERMISSION		-45	/* not used */
#define LB_BAD_DESCRIPTOR	-46
#define LB_BAD_ACCESS		-47

#define LB_LB_ERROR		-48
#define LB_BUF_TOO_SMALL	-49
#define LB_DIFFER		-50
#define LB_TOO_MANY_WRITERS	-51

#define LB_NOT_EXIST		-52
#define LB_EXPIRED		-53
#define LB_MSG_TOO_LARGE	-54
#define LB_UPDATE_FAILED	-55

#define LB_NOT_FOUND		-56
#define LB_LOCK_FAILED          -57
#define LB_LENGTH_ERROR		-58

#define LB_FTOK_FAILED		-59
#define LB_SHMGET_FAILED	-60
#define LB_REMOVE_FAILED	-61

#define LB_FTRUNCATE_FAILED	-62
#define LB_PARITY_ERROR		-63
#define LB_MPROTECT_FAILED	-64
#define LB_SHMDT_FAILED		-65

#define LB_NON_LB_FILE		-66
#define LB_HAS_BEEN_LOCKED	-67
#define LB_MISC_BAD_CMD		-68
#define LB_N_CHECK_ERROR	-69

#define LB_FILE_SYSTEM_FULL	-70
#define LB_OPEN_LOCK_FILE_FAILED	-71
#define LB_INUSE		-72
#define LB_NOT_SUPPORTED	-73

#define LB_SEEK_FAILED		-74
#define LB_WRITE_FAILED		-75
#define LB_TOO_MANY_WAKEUP_REQ	-76
#define LB_SIGHOLD_FAILED	-77	/* not used */

#define LB_SIGPAUSE_FAILED	-79	/* not used */
#define LB_ZERO_HOST_ADDRESS	-80

#define LB_VERSION_NOT_SUPPORTED  -90

#define LB_RSIS_FAILED		-91
#define LB_RSIS_CHECK_SUM_ERROR	-92
#define LB_NOT_ACTIVE		-93
#define LB_BAD_BYTE_ORDER	-94
#define LB_CHMOD_FAILED		-95

#define LB_FCNTL_LOCK_FAILED	-96
#define LB_FCNTL_LOCK_NOT_SUPPORTED	-97
#define LB_PTHREAD_KEY_CREATE_FAILED	-98

#define LB_INTERNAL_ERROR LB_LB_ERROR

#define LB_PREV_MSGID_FAILED	0xffffffff

/* public functions; Refer to lb.doc for detailed descriptions. */

#ifdef __cplusplus
extern "C"
{
#endif

int LB_open (const char *lb_name, int flags, LB_attr *attr);
int LB_read (int lbd, void *buf, int buflen, LB_id_t id);
int LB_write (int lbd, const char *message, int length, LB_id_t id);
int LB_close (int lbd);
int LB_seek (int lbd, int offset, LB_id_t from, LB_info *info);
int LB_clear (int lbd, int nrm);
int LB_delete (int lbd, LB_id_t id);
int LB_stat (int lbd, LB_status *status);
int LB_list (int lbd, LB_info *list, int nlist);
int LB_remove (const char *lb_name);
int LB_direct (int lbd, char **ptr, LB_id_t id);
LB_id_t LB_previous_msgid (int lbd);
int LB_set_poll (int lbd, int max_poll, int wait_time);
int LB_lock (int lbd, int command, LB_id_t id);
int LB_set_tag (int lbd, LB_id_t id, int tag);
int LB_register (int lbd, int type, void *value);
int LB_read_window (int lbd, int offset, int size);
int LB_msg_info (int lbd, LB_id_t id, LB_info *info);
int LB_misc (int lbd, int cmd);

int LB_UN_register (int fd, LB_id_t id, 
		void (*notify_func)(int, LB_id_t, int, void *));

/*
int LB_AN_register (LB_id_t event, 
		void (*notify_func)(LB_id_t, char *, int, void *));
int LB_NTF_control (int cntl_function, ...);
int LB_AN_post (LB_id_t event, const char *msg, int msg_len);
int LB_NTF_lost ();
int LB_NTF_sender_id ();
*/
int LB_maintain (int lbd, int param);
int LB_sdqs_address (int lbd, int func, int *port, unsigned int *ip);

int LB_fix_byte_order (char *lb_name);
void LB_test_compiler_flag (void);

enum {LB_NOT_UPDATED, LB_UPDATED, LB_NOT_CHECKED};
                      /* LB update status values returned by LB_stat () */

/* for libinfr internal use */

#define LB_UN_PARAMS_GET 0xffffffff
	/* special value for argument "un_params" of LB_UN_parameters. */

unsigned int LB_UN_parameters (unsigned int un_params);
unsigned int LB_write_failed_host (int lbd);
int LB_set_nr (int fd, LB_id_t msgid, unsigned int host, 
					int pid, int *a_pid, int *a_fd);
int RSS_LB_set_nr (int fd, LB_id_t msgid, unsigned int host, 
					int pid, int *a_pid, int *a_fd);

#ifdef __cplusplus
}
#endif


#endif		/* #ifndef LB_H */
