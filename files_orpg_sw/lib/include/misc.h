/*******************************************************************

    Module: misc.h

    Description: Public header file for the misc library.

*******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 19:33:33 $
 * $Id: misc.h,v 1.95 2012/07/27 19:33:33 jing Exp $
 * $Revision: 1.95 $
 * $State: Exp $
 */  

/**@#-*/ /*CcDoc Token Processing OFF*/
#ifndef MISC_H
#define MISC_H
/**@#+*/ /*CcDoc Token Processing ON*/

#include <stdio.h>             /* FILE *                                  */
#include <signal.h>            /* for MISC_sig_*                          */
#include <sys/types.h>
#include <sys/stat.h>


#include <misc_rsis.h>         /* for RSIS module                      */

#define ALIGNED_LENGTH 4	/* number of bytes required for 
					   machine dependent alignment */

typedef int ALIGNED_t;		/* the primary type that starts with aligned
				   address */
#define ALIGNED_T_SIZE(a)	\
	(((a) + ALIGNED_LENGTH - 1) / ALIGNED_LENGTH)
				/* number of ALIGNED_t converted from number
				   of bytes */

/**************************************************************************
 From the misc.3 manpage:

        ALIGNED_SIZE is a macro that returns the machine dependent aligned
        size of argument "size". If, for example, the aligned size is 4 and
        "size" = 2, ALIGNED_SIZE will return 4.

        We use this macro, for example, when we have to put several 
        structures in a given message and we want them all to be aligned
        so that we can cast the address of the entire message to a pointer.
 **************************************************************************/
#define ALIGNED_SIZE(a)	\
	((((a) + ALIGNED_LENGTH - 1) / ALIGNED_LENGTH) * ALIGNED_LENGTH)

#include <sys/time.h>

#define SHORT_BSWAP(a) ((((a) & 0xff) << 8) | (((a) >> 8) & 0xff)) 
#define INT_BSWAP(a) ((((a) & 0xff) << 24) | (((a) & 0xff00) << 8) | \
			(((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff)) 
#define INT_SSWAP(a) ((((a) & 0xffff) << 16) | (((a) >> 16) & 0xffff))

#define SHORT_SSWAP(a) {short z; z = a[0]; a[0] = a[1]; a[1] = z;}
#define FLOAT_BSWAP(a) {char *z, t; z = (char *)&a; t = z[0]; z[0] = z[3]; \
			z[3] = t; t = z[1]; z[1] = z[2]; z[2] = t;}

#ifdef LITTLE_ENDIAN_MACHINE

#define SHORT_BSWAP_L(a) SHORT_BSWAP(a)
#define INT_BSWAP_L(a) INT_BSWAP(a)
#define INT_SSWAP_L(a) INT_SSWAP(a)
#define FLOAT_BSWAP_L(a) FLOAT_BSWAP(a)

#else

#define SHORT_BSWAP_L(a) (a)
#define INT_BSWAP_L(a) (a)
#define INT_SSWAP_L(a) (a)
#define FLOAT_BSWAP_L(a) (a)

#endif

#ifdef LINUX

#define MISC_SYSTEM_SH "/bin/sh -l -c "

#else

#define MISC_SYSTEM_SH "/bin/sh -c "

#endif

/**
  * Macros for MISC_string_fit()
  */
                               /** Default replacement character. */
#define MISC_STRING_FIT_DFLT_FIT_CHAR   '*'
                               /** Fit string by replacing characters at
                                 * front of string. */
#define MISC_STRING_FIT_FRONT           1
                               /** Fit string by replacing characters in
                                 * middle of string. */
#define MISC_STRING_FIT_MIDDLE          2
                               /** Fit string by truncation. */
#define MISC_STRING_FIT_TRUNC           3

/* return values from MISC_cp_read_from_cp */
enum {MISC_CP_NODATA, MISC_CP_STDOUT, MISC_CP_STDERR};

#define MISC_CP_MATCH_STR 0x80000000

/* error returns from MISC_cp_* functions */
#define MISC_CP_MALLOC_FALIED  -680
#define MISC_CP_SIGSET_FALIED  -681
#define MISC_CP_PIPE_FALIED  -682
#define MISC_CP_FORK_FALIED  -683
#define MISC_CP_FCNTL_FALIED  -684
#define MISC_CP_DOWN  -685

/* compression method for MISC_compress/MISC_decompress */
enum {MISC_GZIP, MISC_BZIP2};

/* error returns from MISC_compress/MISC_decompress */
#define MISC_BUF_TOO_SMALL -33
#define MISC_BAD_METHOD -34
#define MISC_LIB_NOT_FOUND -35
#define MISC_CMP_FAILED -36
#define MISC_NOT_SUPPORTED -37

#ifdef TEST_OPTIONS	/* for simulating network behaviors */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define write(a,b,c) MISC_TO_write(a,b,c)
#define connect(a,b,c) MISC_TO_connect(a,b,c)
#define accept(a,b,c) MISC_TO_accept(a,b,c)
#define close(a) MISC_TO_close(a)

ssize_t MISC_TO_write (int fd, const void *buf, size_t count);
int MISC_TO_connect (int sockfd, const struct sockaddr *serv_addr,
					socklen_t addrlen);
int MISC_TO_accept (int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int MISC_TO_close (int fd);

#endif

/**@#-*/ /*CcDoc Token Processing OFF*/

#ifdef __cplusplus
extern "C"
{
#endif

int MISC_decompress (int method, char *src, int src_len,
					char *dest, int dest_len);
int MISC_compress (int method, char *src, int src_len,
					char *dest, int dest_len);

int msleep (int ms);
time_t MISC_systime (int *ms);
time_t MISC_cr_time ();
int unix_time (time_t *time, int *y, int *mon, int *d, int *h, int *m, int *s);
int MISC_bswap (int swap_size, void *in_buf, int n_items, void *out_buf);
int MISC_i_am_bigendian ();
void *MISC_open_table (int entry_size, int inc, 
			int keep_order, int *n_ent_pt, char **tbl_pt);
void *MISC_create_table (int entry_size, int inc);
void *MISC_get_table (void *tblpt, int *size);
void *MISC_table_new_entry (void *tblpt, int *ind);
void MISC_table_free_entry (void *tblpt, int ind);
void MISC_free_table (void *tblpt);
int MISC_table_search (void *tblpt, void *ent, 
			int (*cmp) (void *, void *), int *ind);
int MISC_table_insert (void *tblpt, void *ent, int (*cmp) (void *, void *));
int MISC_bsearch (void *ent, void *tbl, int tbl_size, int item_size, 
			int (*cmp) (void *, void *), int *ind);

void MISC_malloc_retry (int yes);
void *MISC_malloc (size_t size);
void MISC_free (void *p);
void *MISC_ind_malloc (size_t size);
int MISC_ind_free (void *p, size_t size);


/*  Same as MISC_swap_shorts */
void MISC_short_swap (void *buf, int size);

/*  Routines which convert to/and from BIG ENDIAN */
/*  							     */
void MISC_swap_shorts (int no_of_shorts, short* buf);
void MISC_swap_longs (int no_of_longs, long* longs);
void MISC_swap_floats (int no_of_floats, float* buf);

/*
 * Miscellaneous String-Manipulation routines
 */
char *MISC_string_basename(char *path) ;
void  MISC_string_fit(char *trgt_string, size_t trgt_string_size,
                      int fit_flag, char fit_char, const char *src_string);
char *MISC_basename (char *path);
char *MISC_dirname (char *path, char *buf, int buf_size);
char *MISC_full_path (char *dir, char *fname, char *buf, int buf_size);
char *MISC_tolower (char *str);
char *MISC_toupper (char *str);
int MISC_get_token (char *str, char *format, int ind, void *buf, int b_s);
int MISC_char_cnt (char *str, char *c_set);
void MISC_string_date_time (char *date_time,
                      int date_time_size, const time_t *intime);

/* MISC_log utility - in MISC_string.c */
void MISC_log (const char *format, ...);
void MISC_log_reg_callback (void (*log_cb)(char *));
void MISC_log_disable (int disable);

int MISC_equals (double d1, double d2);
int MISC_compare (double d1, double d2);
int MISC_equalsf (float d1, float d2);
int MISC_comparef (float d1, float d2);

char *MISC_test_options (char *opt);
void MISC_TO_add_fd (int fd);

/*
 * Miscellaneous Standard C signal-replacement routines
 */

/*
 * Miscellaneous simplified signal management-replacement routines
 */
#ifdef LINUX
#define SIG_HOLD ((__sighandler_t) 2) /* Hold signal. */
#endif

void (*MISC_sig_sigset(int sig, void (*disp)(int)))(int) ;
int MISC_sig_sighold(int signo) ;
int MISC_sig_sigrelse(int signo) ;

/*
 * Miscellaneous resource-management "wrapper" routines
 */
#define MISC_RSRC_GET_ERR (-2)
#define MISC_RSRC_SET_ERR (-3)
#define MISC_RSRC_DIR_MISSING		(-4)
#define MISC_RSRC_VAR_TOO_LARGE	(-5)
#define MISC_RSRC_ENVS_UNDEFINED	(-6)
#define MISC_RSRC_BUF_TOO_SMALL		(-7)
int MISC_rsrc_nofile(int nofile) ;
int MISC_get_work_dir (char *buf, int buf_size);
int MISC_get_cfg_dir (char *buf, int buf_size);
int MISC_mkdir (char *path);
int MISC_get_tmp_path (char *buf, int buf_size);

#define MISC_TOKEN_ERROR		-8
#define MISC_TOKEN_FORMAT_ERROR		-9

/*
 * Miscellaneous process-related routines
 */
void MISC_proc_printstack (int pid, int out_buf_size, char *out_buf);

#define MISC_SYSTEM_CMD_TOO_LONG	-440	
#define MISC_SYSTEM_SYNTAX_ERROR	-441
#define MISC_SYSTEM_GETRLIMIT		-442
#define MISC_SYSTEM_SIGACTION		-443
#define MISC_SYSTEM_PIPE		-444
#define MISC_SYSTEM_OPEN		-445
#define MISC_SYSTEM_WAITPID		-446
#define MISC_SYSTEM_WRITE		-447
#define MISC_SYSTEM_READ_PIPE		-448
#define MISC_SYSTEM_EXECVP		-449
#define MISC_SYSTEM_FORK		-450
#define MISC_SYSTEM_BG_DIED		-451
#define MISC_SYSTEM_BG_FAILED		-452
#define MISC_SYSTEM_SIGPROCMASK		-453

#define MISC_FILE_SYSTEM_FULL		-454
#define MISC_WRITE_FAILED		-455
#define MISC_READ_FAILED		-456
#define MISC_MALLOC_FAILED		-457
#define MISC_BAD_ARGUMENT		-458
#define MISC_MKDIR_FAILED		-459

#define MISC_CP_MANAGE 1

int MISC_system_to_buffer (char *cmd, char *obuf, int bsize, int *n_bytes);
void MISC_system_shell (char *shell_cmd);
int MISC_read_status (char *buf, int buf_size);
int MISC_system_status_signal (int sig);
void MISC_system_setsid (int yes);

void MISC_cp_close (void *cp);
int MISC_cp_read_from_cp (void *cp, char *buf, int b_size);
int MISC_cp_write_to_cp (void *cp, char *str);
int MISC_cp_open (char *cmd, int flag, void **rcp);
int MISC_cp_get_status (void *cp);
void *MISC_get_func (char *lib, char *func, int quiet);

int MISC_write (int fd, const char *buf, int wlen);
int MISC_read (int fd, char *buf, int rlen);
int MISC_close (int fd);
int MISC_open (const char *path, int oflag, mode_t mode);
int MISC_unlink (const char *path);
int MISC_stat (const char *path, struct stat *buf);
int MISC_fstat (int fd, struct stat *buf);
int MISC_system (const char *string);
FILE *MISC_fopen (const char *filename, const char *mode);
int MISC_fclose (FILE *stream);
char *MISC_expand_env (const char *str, char *buf, int buf_size);

#ifdef __cplusplus
}
#endif


#endif			/* #ifndef MISC_H */
/**@#+*/ /*CcDoc Token Processing ON*/
