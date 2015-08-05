/**************************************************************************

      Module:  orpgtask.c

 Description:
        This file provides ORPG library routines for performing task-related
        duties.

        Functions that are public are defined in alphabetical order at the
        top of this file and are identified by a prefix of "ORPGTASK_".

        The scope of all other routines defined within this file is
        limited to this file.  The private functions are defined in
        alphabetical order, following the definitions of the API functions.

 Convenience Functions:
    We provide several convenience functions that are simple front-ends to
    ORPGSTAT_task*() routines.  These convenience functions have been placed 
    at the bottom of this file --- but before the built-in Unit Test driver
    code.

 **************************************************************************/

/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2007/11/16 13:15:08 $
 * $Id: orpgtask.c,v 1.11 2007/11/16 13:15:08 cmn Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

#include <debugassert.h>
#include <stdio.h>
#include <stdlib.h>            /* EXIT_FAILURE                            */
#include <string.h>            /* strstr()                                */

#include <infr.h>
#include <orpg.h>

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define SIGNAL_NAME_SZ 16
typedef struct {
    int signal;
    unsigned char coregen ;    /* flag; if set, this is sig generates core*/
                               /* file                                    */
    unsigned char catch ;      /* flag; if set, we are to catch this sig  */
    char name[SIGNAL_NAME_SZ] ;
} Signal_attr_t ;

static Signal_attr_t Signals[] = {{  SIGHUP, 0, 1,   "SIGHUP"},
                           {  SIGINT, 0, 1,   "SIGINT"},
                           { SIGQUIT, 1, 1,  "SIGQUIT"},
                           {  SIGILL, 1, 1,   "SIGILL"},
                           { SIGABRT, 0, 1,  "SIGABRT"},
                           {  SIGFPE, 1, 1,   "SIGFPE"},
                           {  SIGBUS, 1, 1,   "SIGBUS"},
                           { SIGSEGV, 1, 1,  "SIGSEGV"},
                           {  SIGSYS, 1, 1,   "SIGSYS"},
                           { SIGPIPE, 0, 1,  "SIGPIPE"},
                           { SIGTERM, 0, 1,  "SIGTERM"},
                           { SIGCHLD, 0, 0,  "SIGCHLD"},
                           {  SIGPWR, 0, 0,   "SIGPWR"},
                           {  SIGURG, 0, 0,   "SIGURG"},
                           { SIGPOLL, 0, 0,  "SIGPOLL"},
                           { SIGSTOP, 0, 0,  "SIGSTOP"},
                           { SIGXCPU, 1, 1,  "SIGXCPU"},
                           { SIGXFSZ, 1, 1,  "SIGXFSZ"},
#ifndef LINUX
                           { SIGLOST, 1, 1,  "SIGLOST"},
#endif
                           { SIGCONT, 0, 0,  "SIGCONT"}} ;


/*
 * Static Variables
 */

static void (*Cleanup_fxn)(int term_code) ;
                               /* Pointer to (optional) process-specific  */
                               /*   cleanup function                      */

static int (*User_term_handler) (int signal, int sig_type) = NULL;

static unsigned char Termsig_too_late = 0 ;
                               /* flag; if set, indicates that it is too  */
                               /* late for the caller to add to the list  */
                               /* of caught signals                       */

static size_t Num_signals = sizeof(Signals) / sizeof(Signal_attr_t) ;

static void Term_handler(int signal) ;
static Signal_attr_t * Get_sigattr(int signal) ;

/*************************************************************************

   Description:
      Exit an ORPG process
  
      The registered cleanup function --- if any --- will be executed prior
      to exiting.
  
   Input:
      Reason for exiting.

   Output:

   Return:

*************************************************************************/
void ORPGTASK_exit(int exit_code){


    /*
     * If provided, invoke the cleanup function ...
     */
    if (Cleanup_fxn != NULL) {
        Cleanup_fxn(exit_code) ;
        LE_send_msg(GL_INFO,
                    "ORPGTASK_exit cleanup fxn 0x%x exit code 0x%08x\n",
                    Cleanup_fxn, exit_code) ;
    }
    if (User_term_handler != NULL) {
        User_term_handler (-1, ORPGTASK_EXIT_NORMAL_SIG) ;
        LE_send_msg(GL_INFO,
                    "ORPGTASK_exit term handler called exit code %d\n",
                    exit_code) ;
    }

    LE_send_msg (GL_INFO, "ORPGTASK_exit - %d", exit_code) ;
    exit(exit_code) ;

/*END of ORPGTASK_exit()*/
}
 
/**************************************************************************
 Description: Register the termination sigaction handler.
       Input: pointer to cleanup function (may be NULL)
      Output: none
     Returns: 0 upon success; otherwise -1
       Notes: We keep the messy details in the two private functions.

              This function should not be appear in the application
              software.  Instead, the ORPGASK_register_term_handler macro
              should be used (the replacement string of this macro is
              determined by a preprocessor directive ... refer to
              orpgtask.h).

              This macro is an artifact: we used to pass a flag indicating
              whether or not to catch corefile-generating signals.  The
              definition of the macro was controlled by a compiler macro.
             
 **************************************************************************/
int ORPGTASK_reghdlr(void (*cleanup_fxn_p)(int)){

    register int i ;
    void (*retval)(int) ;

    /*
     * Too late for the caller to add to the list of caught signals ...
     */
    Termsig_too_late = 1 ;


    for (i=0; i < Num_signals; ++i) {
        /*
         * Automatic variables ...
         */
        Signal_attr_t *sigattr_p = &(Signals[i]) ;

        if (sigattr_p->catch) {
            /*
             * We do not catch corefile-generating signals in the development
             * environment ...
             */
            if (1 /*sigattr_p->signal != SIGINT */) {
                retval = MISC_sig_sigset (sigattr_p->signal, Term_handler) ;
                if (retval == SIG_ERR) {
                    LE_send_msg(GL_OS,
                                "Unable to register sig %d (%s)",
                                sigattr_p->signal, sigattr_p->name) ;
                    return(-1) ;
                }
            }
        
        } /*endif this signal is catchable and we are to catch it*/
 
    } /*endfor each signal in the attribute table*/


    if (cleanup_fxn_p != NULL) {
        Cleanup_fxn = cleanup_fxn_p ;
    }

    return(0) ;

/*END of ORPGTASK_reghdlr()*/
}
 
/**************************************************************************

 Description: Register a termination handler.
       Input: pointer to handler function (may be NULL)
      Output: none
     Returns: 0 upon success; otherwise -1
             
 **************************************************************************/

int ORPGTASK_reg_term_handler (int (*term_handler)(int, int)) {
    int i ;
    void (*retval)(int) ;

    /*
     * Deregister termination handler.
     */
    if( term_handler == NULL ){
       User_term_handler = NULL;
       return 0;
    }

    /*
     * Too late for the caller to add to the list of caught signals ...
     */
    Termsig_too_late = 1 ;


    for (i=0; i < Num_signals; ++i) {
        /*
         * Automatic variables ...
         */
        Signal_attr_t *sigattr_p = &(Signals[i]) ;

        if (sigattr_p->catch) {
            /*
             * We do not catch corefile-generating signals in the development
             * environment ...
             */
            if (1 /* sigattr_p->signal != SIGINT */) {
                retval = MISC_sig_sigset(sigattr_p->signal, Term_handler) ;
                if (retval == SIG_ERR) {
                    LE_send_msg(GL_OS,
                                "Unable to register sig %d (%s)",
                                sigattr_p->signal, sigattr_p->name) ;
                    return(-1) ;
                }
            }
        
        } /*endif this signal is catchable and we are to catch it*/
 
    } /*endfor each signal in the attribute table*/

    User_term_handler = term_handler;

    return(0) ;

/*END of ORPGTASK_reghdlr()*/
}
 
/**
  * Handle catchable termination signals
  *
  * The registered cleanup function --- if any --- will be executed prior
  * to exiting.
  *
  */
static void Term_handler(int signal) {
    Signal_attr_t *sigattr_p ;
    int exit_code = ORPGTASK_EXIT_ABNORMAL_SIG ;
                               /* Assume the worst */
    int to_exit;

    if (signal == SIGPIPE) {
        LE_send_msg(GL_INFO, "SIGPIPE received");
	return;
    }

    /*
     * Determine if the process is being terminated "normally" ...
     */
    sigattr_p = Get_sigattr(signal) ;
    if (sigattr_p == NULL) {
        LE_send_msg(GL_INFO,
                    "TERM HANDLER signal %d: no signal attribute found!",
                    signal) ;
    }
    else {
        if (! sigattr_p->coregen) {
            exit_code = ORPGTASK_EXIT_NORMAL_SIG ;
        }
    }


    /*
     * If provided, invoke the cleanup function ...
     */
    if (Cleanup_fxn != NULL) {
        Cleanup_fxn(exit_code) ;
        LE_send_msg(GL_INFO,
                    "TERM HANDLER exit code: 0x%08x cleanup fxn 0x%x called",
                    exit_code, Cleanup_fxn) ;
    }
    to_exit = 1;
    if (User_term_handler != NULL) {
        if (User_term_handler (signal, exit_code) != 0)
	    to_exit = 0;
        LE_send_msg(GL_INFO,
                    "Term handler called (sig %d, sigtype %d)",
						signal, exit_code) ;
    }

    if (exit_code == ORPGTASK_EXIT_ABNORMAL_SIG) {
        /*
         * Print the process stack contents ...
         */
        LE_send_msg(GL_INFO,
                    "Printing process stack contents to STDERR ...") ;
        ORPGTASK_print_stack (NULL, (int)getpid ());
	to_exit = 1;
    }

    if (to_exit)
	exit(exit_code) ;
    else
	return;

/*END of Term_handler()*/
}

/**************************************************************************

    Prints stack of process "pid" on host "host" into the caller's LE
    log file.

**************************************************************************/

#define STACK_BUF_SIZE 2048

void ORPGTASK_print_stack (char *host, int pid) {
    char stack_buf[STACK_BUF_SIZE], rpc_buf[128], *buf;
    int len, bsize;

    strcpy (stack_buf, "Process Stack Dump:\n");
    len = strlen (stack_buf);
    bsize = STACK_BUF_SIZE - len;
    buf = stack_buf + len;
    if (host != NULL) {
	int ret;
	sprintf (rpc_buf, "%s:MISC_proc_printstack", host);
	if ((ret = RSS_rpc (rpc_buf, "i i ba-%d-o", bsize, pid, 
		bsize, buf)) < 0)
	    sprintf (stack_buf, 
		"RSS_rpc MISC_proc_printstack failed (%d)", ret);
    }
    else
	MISC_proc_printstack (pid, bsize, buf);
    ORPGMISC_send_multi_line_le (GL_INFO, stack_buf);
}

/**************************************************************************
 Description: Get attributes for specified signal
       Input: signal
      Output: none
     Returns: pointer to signal attributes upon success; otherwise, NULL
              pointer is returned
       Notes:
***************************************************************************/
static Signal_attr_t *Get_sigattr(int signal) {
    register int i ;
    static Signal_attr_t *sigattr_p = NULL ;

    for (i=0; i < Num_signals; ++i) {
        sigattr_p = &(Signals[i]) ;
        if (signal == sigattr_p->signal) {
            return(sigattr_p) ; 
        }
    }

    return((Signal_attr_t *) NULL) ;
}

/**************************************************************************
 Description: Get name for specified signal
       Input: signal
      Output: none
     Returns: name of signal
***************************************************************************/
char *ORPGTASK_get_sig_name(int signal) {
    int i ;
    Signal_attr_t *temp_sigattr_p = NULL ;
    static char buf[ SIGNAL_NAME_SZ ];

    strcpy( buf, "UNKNOWN" );

    for (i=0; i < Num_signals; ++i) {
        temp_sigattr_p = &(Signals[i]) ;
        if (signal == temp_sigattr_p->signal) {
            strcpy( buf, temp_sigattr_p->name ); 
        }
    }

    return(buf) ;
}

