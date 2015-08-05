/****************************************************************
		
    Module: lb_def.h
				
    Description: This module contains the internal shared definitions
	for the LB (linear buffer) library module.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:54 $
 * $Id: lb_def.h,v 1.71 2012/06/14 18:57:54 jing Exp $
 * $Revision: 1.71 $
 * $State: Exp $
 * $Log: lb_def.h,v $
 * Revision 1.71  2012/06/14 18:57:54  jing
 * Update
 *
 * Revision 1.62  2002/03/18 22:37:14  jing
 * Update
 *
 * Revision 1.61  2002/03/12 16:51:24  jing
 * Update
 *
 * Revision 1.59  2000/08/21 20:50:12  jing
 * @
 *
 * Revision 1.58  2000/03/13 15:26:10  jing
 * @
 *
 * Revision 1.43  1999/06/29 21:21:14  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.40  1999/05/27 02:27:40  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.36  1999/05/03 20:58:52  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.22  1998/05/29 22:16:50  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.3  1996/08/22 13:06:42  cm
 * SunOS 5.5 modifications
 *
*/

#ifndef LB_DEF_H
#define LB_DEF_H

#ifdef LB_THREADED
#include <pthread.h>
#endif

#include <lb.h>

/* #define MMAP_UNMAP_NEEDED  --  This flag removes any mmapped area
		when file size is extended with write, truncated, or file
		size has been changed (HPUX write does not write the data 
		(except write 0) if file is mmapped). */

#ifdef HPUX
#define MMAP_UNMAP_NEEDED 
#endif

/* Definitions / macros / types */

#define ALL_TYPE_FLAGS (LB_FILE | LB_MEMORY | LB_REPLACE | LB_NORMAL | \
	LB_MUST_READ | LB_SINGLE_WRITER | LB_NOEXPIRE | LB_UNPROTECTED | \
	LB_DIRECT | LB_UN_TAG | LB_MSG_POOL | LB_DB | LB_SHARE_STREAM)

/* LB_IDNUMBER is a number for identifying the LB and its version */
/* The first LB version started with 52398402. Each new version uses a 
   new number calculated by subtracting 1000. 
   Version 1: The original version.
   Version 2: added variable replaceable msg sizes.
   Version 3: added variable tag sizes and double buffered message length.
	      LB_mark is removed. LB_set_tag and LB_tag are added.
   Version 4: Added storage space management support for replaceable LB.
   Version 5: First version that is operational.
   Version 6: A bug fix in RSIS_size.
*/
#define LB_IDNUMBER    52393402	/* version 6 */
#define LB_N_VERSIONS	6	/* version numbers supported for compatibility
				   which means the existing LBs of the latest
				   LB_N_VERSIONS are supported. */

/* we use the MSB of LB_structure.flags for storing LB version number */
#define GET_VERSION(x) ((x) >> 24)
#define SET_VERSION(x,v) (((x) & 0xffffff) | ((v) << 24))

#define N_INCREMENT    8	/* increment for total number of open LB's */
#define MIN_INDEX     32	/* the minimum LB index */

typedef int lb_t;		/* integer type used for LB - we use 4
				   bytes assuming memory access is atomic;
				   This is machine independent - must be
				   a 4-byte integer */

typedef struct {		/* the LB header */

    char remark[LB_REMARK_LENGTH];

    lb_t lb_id;			/* LB_IDNUMBER for identifying an LB medium */
    lb_t msg_size;		/* average message size. 0 means arbitrary. */
    lb_t n_msgs;		/* max number of messages */
    lb_t acc_mode; 		/* access mode */
    lb_t lb_types; 		/* LB type flags */
    lb_t shm_key;		/* shm key */
    lb_t lb_time;		/* creation time */
    lb_t ptr_page; 		/* current pointer page number */
    lb_t num_pt;		/* current msg pointer_number combination */
    lb_t ptr_read;		/* first unread msg. Used by types of
				   LB_MUST_READ and LB_SHARE_STREAM */
    lb_t unused_bytes;		/* number of bytes that are not used at the end
				   of the message area (This field is needed by
				   LB_DIRECT type LB) */

    lb_t unique_msgid;		/* used for LB_misc (LB_GET_UNIQUE_MSGID) */
    lb_t sdqs_port;		/* sdqs port number */
    lb_t sdqs_ip;		/* sdqs IP address */
    unsigned char non_dec_id;	/* the LB is of non-decreasing ID. Upper 7 bit
				   used for MUST_READ read count setting. */
    unsigned char upd_flag;	/* update flag used for guaranteeing that one
				   always accesses updated LB after receiving 
				   UN */
    unsigned char nra_size;	/* max number of notification request records 
				   is nra_size * LB_NR_FACTOR */
    unsigned char tag_size;	/* tag size. 0 - 32 */
    unsigned char sms_ok;	/* The SMS control tables are OK */
    unsigned char is_bigendian;/* boolean: whether LB control area is in big
				   endian byte order */
    unsigned char miscflags;	/* misc control flags */
    unsigned char read_cnt;	/* read count of the current read msg. 
				   MUST_READ and read count set only. */
} Lb_header_t;

/* for Lb_header_t.miscflags */
#define LB_ID_BY_USER 1		/* LB_DB: message ID specified by caller */
#define LB_MSG_DELETED 2	/* LB_DB: a message has been deleted */

#define MAX_LB_SIZE 0x7f000000	/* max size of the LB file */
#define MAXN_MSGS     0x7ffb	/* maximum number of messages (15 bits) */
#define POINTER_RANGE 0x20000	/* range for pointers (17 bits) */
/* #define POINTER_RANGE 80 */	/* range for pointers for testing */
#define LB_MSG_DB_ID_MASK 0x7fff
				/* msg ID mask used in msg search for 
				   LB_DB type LB */

#define NMSG_SHIFT  17		/* shift for message numbers */
#define POINTER_MASK  0x1ffff	/* mask for pointers */
#define NMSG_MASK   0x7fff	/* mask for message numbers */
#define N_PAGES	4		/* number of pages in the pointer range */

#define CHAR_SIZE	8	/* number of bits in a char */
#define GET_POINTER(a) (a & POINTER_MASK)	
				/* extract message pointer */
#define GET_NMSG(a) ((a >> NMSG_SHIFT) & NMSG_MASK)	
				/* extract number of messages */
#define COMBINE_NUMPT(num,pt) ((pt & POINTER_MASK) | (num<< NMSG_SHIFT))	
				/* combine a pointer and a number of msgs */

#define LB_EXCLUSIVE_LOCK_OFF 0	/* lock file offset for exclusive LB access
				   lock */
#define LB_TEST_LOCK_OFF 1	/* lock file offset for lock test */
#define LB_SINGLE_WRITE_LOCK_OFF 2
				/* lock file offset for single writer lock */
#define LB_TAG_LOCK_OFF 3	/* offset for tag lock */
#define LB_ACTIVE_SV_LOCK_OFF 4	/* offset for server lock */
#define LB_LB_LOCK_OFF 5	/* offset for locking LB itself */
#define LB_DIRECT_ACCESS_LOCK_OFF 6	/* direct access lock offset  */
#define LB_NR_LOCK_OFF 7	/* offset for NR records */
#define LB_MSG_LOCK_OFF (LB_NR_LOCK_OFF + LB_MAXN_NRS)
				/* lock file offset for msg lock (LB_lock) */

#define LB_LOCK_COMMAND 0xff	/* bit mask for argument "command" of LB_lock */

#define DB_WORD_OFFSET(woff,ucont) ((woff) * 2 + ((ucont) % 2))
				/* This returns the word offset in a double
				   buffered area such as the tag area and the 
				   message length area for replaceable LBs, 
				   where woff is the single buffer word offset 
				   and ucont is the double buffer update count.
				    */

/* define conversion to the machine independent lb_t storing format */
#ifdef LITTLE_ENDIAN_MACHINE
    #define LB_T_BSWAP(a) LB_byte_swap(a)
    #define LB_SHORT_BSWAP(a) ((((a) >> 8) & 0xff) | (((a) & 0xff) << 8))
#else
    #define LB_T_BSWAP(a) (a)
    #define LB_SHORT_BSWAP(a) ((a) & 0xffff)
#endif

typedef struct {		/* structure for message dir */
    LB_id_t id;			/* message id */
    lb_t loc;			/* message location (offset) of the next message 
				   in the message area for sequential LB,
				   update cnt for replaceable LB, msg location 
				   for arbitrary msg size sequential LB */
} LB_dir;

typedef struct {		/* structure for message info, replaceable LB 
				   only */
    lb_t len;			/* message length */
    lb_t loc;			/* message location (offset) */
} LB_msg_info_t;

typedef struct {		/* structure for message info, arbitrary msg 
				   size sequential LB only */
    lb_t len;			/* message length */
} LB_msg_info_seq_t;

enum {LRT_HOLD_SHARED, LRT_HOLD_EXCLUSIVE, LRT_LOCK_WAIT};
				/* Lock_record_t.lock_type */

typedef struct {		/* lock record struct */
    struct lb_struct *lb;	/* the LB fd that owns the lock */
    short lock_id;		/* lock ID */
    short lock_type;		/* lock type */
} Lock_record_t;

enum {LB_UNLOCKED, LB_LB_LOCKED};
				/* values used for LB_struct.locked */

typedef struct {		/* per-lb data shared by open fds */

    /* variable fields */
    char *lb_pt;		/* pointer to the beginning of the medium */

    char *map_pt;		/* pointer to the mmapped LB msg area */
    int map_off;		/* file offset of the mmapped LB msg area */
    int map_len;		/* length of the mmapped LB msg area */
    int cntr_mlen;		/* mapping length of the control area */

    /* non-parity-checked fields */
    char cw_perm;		/* control write permission is enabled 
				   (LB_TRUE/LB_FALSE) */
    char dw_perm;		/* data write permission is enabled 
				   (LB_TRUE/LB_FALSE) */
    short use_cnt;		/* number of fds using this file */

    int shmid;			/* LB shared memory id */
    int fd;			/* LB file fd */
    int lockfd;			/* lock file fd */
    char *lb_name;		/* LB name */
    unsigned int parity;

    Lock_record_t *lock_recs;	/* lock record table. This is needed even
				   in single threaded case bacause LB NTF will
				   need to test lock held by local process.*/
    int n_lock_recs;		/* size of lock record table */
    void *lock_recs_tid;	/* lock record table ID */

    char *rsid;			/* RSIS record set ID used by LB_sms */
    char *rsid_buf;		/* pre-allocated buffer for rsid */

#ifdef LB_THREADED
    pthread_mutex_t access_mutex;
				/* LB_access mutex */
    pthread_mutex_t lock_recs_mutex;
				/* lock records access mutex */
    pthread_cond_t recs_ok;	/* lock signal */
#endif

} Per_lb_data_t;

typedef struct lb_struct {	/* the LB data structure */

    /* variable fields */
    char locked;		/* the exclusive access lock status:
				   LB_UNLOCKED - not locked by this process;
				   LB_LB_LOCKED - locked by this process */
    char unused[3];

    /* fixed fields */
    int flags;			/* LB flags (open flags and type flags) */
    int ma_size;		/* message area size. */

    int n_slots;		/* number of slots for the msg dir */
    int maxn_msgs;		/* maximum number of messages */
    int off_a;			/* offset of the message area */
    int off_tag;		/* offset of the tag area */
    int off_msginfo;		/* offset of the message info area */
    int off_nra;		/* offset of the NTF request area; 0 
				   indicates the NTF request area are not 
				   available */
    int off_sms;		/* offset of the space management area */

    int page_size;		/* read/write pointer page size */
    int ptr_range;		/* message pointer range used; This must be 
				   a multiple of maxn_msgs */
    int mpg_size;		/* page size of the mmap */

    /* non-parity-checked fields */
    Per_lb_data_t *pld;

    Lb_header_t *hd;		/* pointer to LB header */
    LB_dir *dir;		/* pointer to LB dir area */
    void *msginfo;		/* pointer to msg info area (LB_msg_info_t or
				   LB_msg_info_seq_t) */
    lb_t *tag;			/* pointer to the message tag area */

    int key;			/* shared memory key */
    int rpt;			/* read pointer */
    int rpage;			/* read page number */
    int rpt_incd;		/* common read pointer being counted */

    void *stat_info;		/* data structure used by LB_stat () */
    int *utag;			/* user's tag address */
    LB_id_t *umid;		/* user's msg ID address */

    unsigned int partial;	/* partial parity value */
    unsigned int parity;	/* parity of the LB_struct fields; This must be 
				   the last field in the structure */
    LB_id_t prev_id;		/* id of the previously read/written msg */
    unsigned int urhost;	/* unreachable host IP for NTF messaging  */
    time_t nr_lock_check_time;	/* lost notification request lock check time */
    int wait_time;		/* waiting time (in ms) between poll */
    int max_poll;		/* maximum number of polls performed when 
				   reading a message that is to come */

    int read_offset;		/* the LB_read window offset */
    int read_size;		/* the LB_read window size */

    char inuse;			/* this LB descriptor is currently been used.
				   This is used for protecting LB functions
				   from been called before a previous LB call
				   with the same descriptor returns. */
    char upd_flag_check;	/* update flag check is needed */
    char upd_flag_set;		/* update flag has been set */
    char active_test;		/* boolean: active sv lock test is needed */

    char prev_perm;		/* previous permission */
    char maint_param;		/* maintenance parameter */
    char lb_misc_locked;	/* exclusively locked by LB_misc and can only 
				   be released by LB_misc */
    char direct_access;		/* In direct access state */

#ifdef LB_THREADED
    pthread_t thread_id;	/* ID of the thread that opens this LB fd */
#else
    int thread_id;
#endif
} LB_struct;

#define N_PLD_VARIABLE_FIELDS	5
				/* number of variable int fields in 
				   Per_lb_data_t */
#define N_LBS_VARIABLE_FIELDS	1
				/* number of variable int fields in 
				   LB_struct */
#define N_LBS_CHECKED_FIELDS	13
				/* number of parity checked int fields in 
				   LB_struct */


/* values used by LB_process_lock and some other functions */
enum {SET_LOCK, SET_LOCK_WAIT, UNSET_LOCK, TEST_LOCK};
enum {NO_LOCK, EXC_LOCK, SHARED_LOCK};
#define LB_LOCKED	1
#define LB_SELF_LOCKED	2	/* these two must be different from LB_SUCCESS 
				   and non-negative */

/* return values from LB_Check_pointer () */
enum {RP_OK, RP_EXPIRED, RP_TO_COME};

/* "sw" argument values of LB_data_transfer () */
enum {READ_TRANS, WRITE_TRANS}; 

/* "perm" argument values of LB_lock_mmap () */
enum {READ_PERM, WRITE_PERM, PREV_PERM};

/* definitions used by LB_notify */

/* message types - no byte swap needed */
#define TO_CLIENT_UN 0x02020202
#define FROM_CLIENT_REG 0x03030303
#define FROM_LB_WRITE_UN 0x04040404		/* for update notification */
#define TO_CLIENT_ACK 0x05050505
#define TO_CLIENT_AN 0x07070707
#define FROM_CLIENT_AN 0x08080808
#define FROM_SV_REG 0x09090909
#define TO_SV_ACK 0x0a0a0a0a
#define FROM_SV_AN 0x0b0b0b0b
#define FROM_SV_CONN 0x0c0c0c0c			/* connection init/resp msgs */

/* values for msg_len field of To_client_msg_t */
enum {TO_CLIENT_ACK_DONE, TO_CLIENT_ACK_FAILED};

/* values for field "fd" for FROM_SV_CONN type messages */
enum {SV_CONN_INIT, SV_CONN_RESP, SV_CONN_REGS};

typedef struct {			/* struct for various NTF messages;
					   AN msg follows this for *_AN types;
					   in big endian for FROM_SV_AN and
					   TO_SV_ACK types */
    lb_t msg_type;			/* message type: TO_CLIENT_UN, 
					   TO_CLIENT_AN or 
					   FROM_CLIENT_AN or 
					   FROM_SV_AN or
					   TO_CLIENT_ACK or
					   TO_SV_ACK or
					   FROM_SV_CONN */
    lb_t fd;				/* LB fd (or LB_AN_FD) 
					   FROM_SV_CONN:
						SV_CONN_INIT: conn init msg; 
						SV_CONN_RESP: response msg;
						SV_CONN_REGS: AN reg info */
    lb_t msgid;				/* LB message id (or AN event id) */
    lb_t lbmsgid;			/* The LB message id in case msgid is
					   LB_ANY or LB_MSG_EXPIRED */
    lb_t msg_len;			/* message length;
					   TO_CLIENT_ACK: LB_NOTIFY_DONE or 
					   LB_NOTIFY_FAILED */
    lb_t lost_cnt;			/* number of NTF lost;
					   TO_SV_ACK: echo FROM_SV_REG.a_pid */
    unsigned short sender_id;		/* the NTF sender's ID */
    short reserved;
} Ntf_msg_t;				/* This struct must be no larger than 
					   any of the following two structs */

typedef struct {			/* struct for NTF registration msgs */
    lb_t msg_type;			/* message type: FROM_CLIENT_REG or
					   FROM_SV_REG */
    lb_t signal;			/* the signal to use; < 0 indicates
					   no signal needed; == -1 indicates
					   deregisteration */
    lb_t pid;				/* process id */
    lb_t fd;				/* LB fd */
    lb_t msgid;				/* LB message id */
    lb_t a_pid;				/* aliased pid - shared with other
					   pid-fd pair for identifying the LB;
					   FROM_SV_REG: the host index */
    lb_t a_fd;				/* aliased fd; -1 indicates no alias */
} Ntf_regist_msg_t;

typedef struct {			/* message struct from LB_write to 
					   the server; All fields must in 
					   big endian byte order */
    lb_t msg_type;			/* message type: FROM_LB_WRITE_UN or
					   FROM_LB_WRITE_WUR */
    lb_t pid;				/* process id to notify */
    lb_t fd;				/* LB fd to notify */
    lb_t msgid;				/* the new message id */
    lb_t lbmsgid;			/* The LB message id in case msgid is
					   LB_ANY or LB_MSG_EXPIRED */
    lb_t msg_len;			/* the new message length */
    unsigned short sender_id;		/* the NTF sender's ID */
    short reserved;
} From_lb_write_t;

#define N_MSG_RM_COMPU -1		/* special value for argument n_rms of
					   LB_process_nr () */

enum {				/* values for the flag field of LB_nr_t */
    LB_NR_NON,			/* The request is not used */
    LB_NR_NOTIFY		/* The request is for LB_notify */
};

typedef struct {		/* the notification request structure */
    unsigned int host;		/* host IP address (4 bytes) */
    unsigned int fd;		/* LB file descriptor (4 bytes) */
    unsigned int msgid;		/* message id (4 bytes) */
    unsigned short pid;		/* process id (2 bytes) */
    char flag;			/* record flag (1 bytes) */
    char lock;			/* non-zero: The record may be locked */
} LB_nr_t;

/* This section is used for passing large pid, which is currently used by IRIX.
   It is not need otherwise. We may remove this in the future when IRIX support
   no longer needed. */
#define Get_big_pid(pid,lock,flag) \
    ((pid) | ((((lock) >> 1) & 0x7f) << 16) | ((((flag) >> 2) & 0x3f) << 23))
#define Get_nr_lock(lock) ((lock) & 0x1)
#define Get_nr_flag(flag) ((flag) & 0x3)
#define Pid_in_lock(pid) ((((pid) >> 16) & 0x7f) << 1)
#define Pid_in_flag(pid) ((((pid) >> 23) & 0x3f) << 2)


/* bit masks for overloading on Ntf_msg_t.msg_len of Ntf_msg_t.msg_type */
#define AN_NOT_TO_SELF 0x80000000
				/* not sending this AN to this process */
#define AN_SELF_ONLY 0x40000000
				/* sending this AN to this process only */
#define AN_MSG_LEN_MASK 0x3fffffff
				/* mask for getting the msg length */

typedef struct {		/* per-thread variables */
    int ntf_block_intern;	/* cntl flag: NTF delievery blocked 
				   internally. */
    int in_callback;		/* cntl flag: the thread is in NTF callback */
} LB_per_thread_data_t;


/* shared internal functions */
LB_struct *LB_Get_lb_structure (int lbd, int *err);
int LB_Check_pointer (LB_struct *lb, int ptr, int page);
int LB_Get_corrected_page_number (LB_struct *lb, int pointer, int ref_page);
int LB_Compute_pointer (LB_struct *lb, int pt, int page, int inc, int *n_page);
void LB_Update_pointer (LB_struct *lb, int n_add, int n_rm);
int LB_Search_msg (LB_struct *lb, LB_id_t id, int *pt, int *page);
int LB_Get_message_info (LB_struct *lb, int rpt, int *loc, LB_id_t *id);
int LB_data_transfer (int sw, LB_struct *lb, char *buf, int len, int loc);

int LB_Unlock_return (LB_struct *lb, int ret);
void LB_reset_inuse (LB_struct *lb);
void LB_Set_parity (LB_struct *lb);
int LB_lock_mmap (LB_struct *lb, int perm, int lock);
int LB_test_file_lock (int fd, int flags);
void LB_unmap (LB_struct *lb);
int LB_mmap (LB_struct *lb, int perm);
int LB_sig_wait (int signo, int ms);
char *LB_alloc_tmp_space (int alloc, int fd, int size);

int LB_init_lock_table (Per_lb_data_t *pld);
void LB_cleanup_lock_table (Per_lb_data_t *pld);
int LB_process_lock (int sw, LB_struct *lb, int type, int lock_id);
void LB_cleanup_locks (LB_struct *lb);

int LB_Ptr_compare (LB_struct *lb, int pt1, int pt2);
void LB_write_tag (LB_struct *lb, int ind, int tag, int add_db_off);
lb_t LB_read_tag (LB_struct *lb, int ind);
lb_t LB_byte_swap (lb_t x);
int LB_read_nrs (LB_struct *lb, LB_nr_t **nr_recs);
int LB_direct_access_lock (LB_struct *lb, int lock_sw, int perm);

int LB_stat_init (LB_struct *lb);
int LB_stat_free (LB_struct *lb);
void LB_stat_update (LB_struct *lb, int num_pt, int ucnt, int ind);
int LB_stat_check (LB_struct *lb);
int LB_stat (int lbd, LB_status *status);

void LB_close_notify (int fd);
int LB_send_to_server (unsigned int host, char *msg, int msg_len);
int LB_process_nr (LB_struct *lb, LB_id_t new_msgid, 
		int msg_len, int tag, int n_rms, unsigned int onp);
int LB_get_nra_size (LB_struct *lb);
int LB_un_send_state (int new_state);
void LB_pass_sender_id (unsigned short sender_id);
int LB_get_extern ();
unsigned int LB_get_local_ip ();

int LB_sms_space_used (LB_struct *lb);
int LB_sms_cntl_size (int n_msgs, int version);
int LB_sms_free_space (LB_struct *lb, int loc, int len);
int LB_sms_get_free_space (LB_struct *lb, int length, int *is_new);
int LB_sms_validate_header (LB_struct *lb);

LB_per_thread_data_t *LB_get_per_thread_data (void);
int LB_search_for_message (LB_struct *lb, LB_id_t id, int offset,
						int *pt, int *page);
int LB_read_a_message (LB_struct *lb, void *buf, int buflen, 
			int rpt, int rpage);

#endif		/* #ifndef LB_DEF_H */

