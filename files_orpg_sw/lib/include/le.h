
/*******************************************************************

    Module: le.h

    Description: header file for the LE module.

*******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/13 19:51:53 $
 * $Id: le.h,v 1.44 2014/03/13 19:51:53 steves Exp $
 * $Revision: 1.44 $
 * $State: Exp $
 */  

#ifndef LE_H
#define LE_H

#include <time.h>

#if (defined(__cplusplus) && !defined(__GNUC__))
#include <sys/varargs.h>
#else	
#include <stdarg.h> 
#endif  


#define LE_MAX_MSG_LENGTH	256
				/* The buffer size for a log/error message */

#define LE_NAME_LEN	128	/* max name length for the LE LB */
#define LE_SOURCE_NAME_SIZE 16	/* max size of source file names */

#define LE_CRITICAL	(LE_file_line ((char *) __FILE__, __LINE__))
				/* macro specifying critical messages */
#define LE_CRITICAL_BIT	0x80000000
				/* bit mask for the critical messages used in
				   LE_message.code */

#define LE_FILENAME_EXT "log"
#define LE_LOG_LABEL "$@t=3671Fa\n"

#define LE_VL_SHIFT		26	/* verbose level field shifts */
#define LE_VL_MASK		(0x3 << LE_VL_SHIFT)
					/* verbose level field mask */
#define LE_GET_MVL(code)	(((code) >> LE_VL_SHIFT) & 0x3)
					/* get msg VL from LE_send_msg code */
#define LE_log(format, ...) LE_send_msg (0, format, __VA_ARGS__)

/* LE message verbose level Codes. The higher VL, the more msgs posted. */
#define LE_VL0	0
#define LE_VL1	(1 << LE_VL_SHIFT)
#define LE_VL2	(2 << LE_VL_SHIFT)
#define LE_VL3	(3 << LE_VL_SHIFT)

/* LE message data structure */
typedef struct {
    unsigned int code;		/* msg code */
    time_t time;		/* msg generation time */
    int n_reps;			/* number of msg repetitions */
    char pad [3];		/* pad to align structure */
    char text[1];		/* user text msg */
} LE_message;

typedef struct {
    unsigned int code;		/* msg code */
    time_t time;		/* msg generation time */
    int pid;			/* process id */
    int n_reps;			/* number of msg repetitions */
    int line_num;		/* the source line number */
    char fname[LE_SOURCE_NAME_SIZE];
				/* the source file name */
    char pad [3];		/* pad to align structure */
    char text[1];		/* user text msg */
} LE_critical_message;

/* LE VL change event message - all fields in network data format */
typedef struct {
    unsigned int host_ip;	/* host IP address */
    int pid;			/* process id to receive the event */
    int new_vl;			/* new verbose level */
} LE_VL_change_ev_t;


/**
  * Function return error numbers
  */
#define LE_ENV_DEF_ERROR -990		/* environ variable LE_DIR_EVENT is
					   incorrectly defined */
#define LE_OPT_ALREADY_DEFINED -991	/* the option has already defined */
#define LE_ENV_NOT_DEF -992		/* LE_DIR_EVENT not defined */
#define LE_BUF_TOO_SMALL -993		/* caller provided buffer too small */
#define LE_LB_OPEN_CREATE_FAILED -994	/* failed in opening/creating the LE 
					   LB */
#define LE_LB_NAME_UNDEFINED -995	/* the LE LB name is not defined */
#define LE_BAD_ARGUMENT -996		/* one calling argument is incorrect */
#define LE_DUPLI_INSTANCE -997		/* The LE LB can not be open since it 
					   is opened by another instance of the
					   same task. */

/* obsolete macros */
#define LE_OPEN_LB_FAILED -1
#define LE_HOST_NAME_ERROR -1
#define LE_MALLOC_ERROR -1


/* public functions */

#ifdef __cplusplus
extern "C"
{
#endif
int LE_init (int argc, char **argv);
void LE_send_msg (int code, const char *format, ...);
int LE_set_option (const char *option_name, ...);
void LE_set_callback (void (*callback) (char *, int));
char *LE_gen_text (const char *format, ...);
void LE_instance (int instance_number);
void LE_set_foreground ();
int LE_local_vl (int new_vl);
void LE_set_vl (unsigned int host_ip, int pid, int new_vl);
int LE_fd ();
int LE_close ();

unsigned int LE_file_line (char *file, int line_num);
void LE_also_print_stderr (int yes);
void LE_terminate (void (*term_func)(), int status);

int LE_filename (const char *name, int instance, char *buf, size_t bufsz) ;
int LE_dirpath (char *buf, size_t bufsz) ;
int LE_filepath (const char *name, int instance, char *buf, size_t bufsz) ;
int LE_create_lb (char *argv0, int n_msgs, int lb_type, int instance);
void LE_get_process_name (char *buf, int buf_size);
int LE_get_argv (char ***argv_p);

#ifdef __cplusplus
}
#endif

#endif			/* #ifndef LE_H */
