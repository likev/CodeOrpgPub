/**************************************************************************

      Module: orpgtask.h

  Created by: Arlis Dodson

 Description: ORPG Library Task routines ORPGTASK public header
	file.

        Note: The LB Task Attribute Table (TAT) resides as message ID
              ORPGINFO_TAT_MSGID in the ORPGDAT_RPG_INFO LB file. 

              The LB Task Status messages reside in the ORPGDAT_TASK_STATUS
              LB file.

 Assumptions:

 **************************************************************************/

/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2007/11/16 13:15:11 $
 * $Id: orpgtask.h,v 1.51 2007/11/16 13:15:11 cmn Exp $
 * $Revision: 1.51 $
 * $State: Exp $
 */

/**@#-*/ /*CcDoc Token Processing OFF*/
#ifndef ORPGTASK_H
#define ORPGTASK_H
/**@#+*/ /*CcDoc Token Processing ON*/

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */

/**
  * Task Exit Codes
  */
enum {ORPGTASK_EXIT_SUCCESS=0,
      ORPGTASK_EXIT_FAILURE,
      ORPGTASK_EXIT_ABORTED,
      ORPGTASK_EXIT_NORMAL_SIG,
      ORPGTASK_EXIT_ABNORMAL_SIG,
      ORPGTASK_EXIT_NONE=0xffffffff} ;

/**
  * Global System Log Messages
  *
  * Message numbers range from 1 to 4095.
  */
#define ORPGTASK_SYSLOG_TERM_HDLR 1
#define ORPGTASK_SYSLOG_TERM_HDLR_TXT "Task Termination Handler: "
#define ORPGTASK_SYSLOG_TASK_EXIT 2
#define ORPGTASK_SYSLOG_TASK_EXIT_TXT "Task Exit: "


/**
  * System Log Message tokens.
  *
  * These are used to communicate information, including:
  *
  * Task Exit Code (hex)
  * Task Termination Signal Number (decimal)
  */
#define ORPGTASK_SYSLOG_TOKEN_SIGNUM "signal: "
#define ORPGTASK_SYSLOG_TOKEN_EXITCODE "exitcode: 0x"
#define ORPGTASK_SYSLOG_TOKEN_PID "pid: "


#define ORPGTASK_MAX_ATTRTBL_ENTRY_LEN	80
#define ORPGTASK_MAX_ATTRTBL_ENTRY_SIZ	((ORPGTASK_MAX_ATTRTBL_ENTRY_LEN) + 1)

#define ORPGTASK_MAX_TASKARG_LEN	64
#define ORPGTASK_MAX_TASKARG_SIZ	((ORPGTASK_MAX_TASKARG_LEN) + 1)

/*
 * Macros for the Task Attribute Table entry I/O Redirection field
 */
#define ORPGTASK_IO_STDOUT_NULL		1
#define ORPGTASK_IO_STDERR_NULL		2
#define ORPGTASK_IO_STDOUT_FILE		4
#define ORPGTASK_IO_STDERR_FILE		8


/*
 * NOTE: this macro is an artifact from when we used the DELIVERABLE_SW
 * compiler macro to control the catching of corefile-generating signals.
 * We now use the ORPG_DELIVERABLE_ENVVAR environment variable to control
 * this behavior.
 */
#define ORPGTASK_reg_term_hdlr(ARG) (ORPGTASK_reghdlr((ARG)))

/**@#-*/ /*CcDoc Token Processing OFF*/
/*
 * Public Function Prototypes
 */
enum {ORPGTASK_MODIFY_TERMSIGS_ADD,
      ORPGTASK_MODIFY_TERMSIGS_REMOVE} ;
#define ORPGTASK_MODIFY_TERMSIGS_UNKNOWN (-2)
#define ORPGTASK_MODIFY_TERMSIGS_TOOLATE (-3)
#define ORPGTASK_MODIFY_TERMSIGS_BADFLAG (-4)
 int ORPGTASK_modify_termsigs(int flag, int termsig) ;
void ORPGTASK_exit(int exit_code) ;
int ORPGTASK_reghdlr(void (*cleanup_fxn_p)(int exit_code)) ;
int ORPGTASK_reg_term_handler (int (*term_handler)(int, int));
void ORPGTASK_print_stack (char *host, int pid);
char *ORPGTASK_get_sig_name (int signal);


#define ORPGTASK_GET_BUF_TOO_SMALL (ORPGSTAT_TASK_GET_STATUS_BUF_TOO_SMALL)

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ORPGTASK_H */
/**@#+*/ /*CcDoc Token Processing ON*/
