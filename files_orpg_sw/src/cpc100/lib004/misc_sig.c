
/*************************************************************************

      Module: MISC_sig.c

 Description:

	Miscellaneous Services replacements for the Standard C signal()
	and raise() functions.

	The POSIX signal mechanism is regarded to be superior to the
	Standard C signal mechanism (see, for example, Chapter 10 of
	W. Richard Stevens' "Advanced Programming in the UNIX Environment"
	or Chapter 6 of Donald Lewine's "POSIX Programmer's Guide").

	However, in our software design, we are making a conscious
	effort to relegate system calls (including POSIX calls that have
	no Standard C equivalent) to the infrastructure level.

	So, we are using these replacement functions to keep the application
	software Standard C while providing the advantages of the POSIX
	signal mechanism.

	Functions that are public are defined in alphabetical order at
	the top of this file and are identified with a prefix of
	"MISC_sig_".

	Functions that are private to this file are defined in alphabetical
	order, following the definition of the public functions.


 Assumptions:

**************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/05/27 16:46:25 $
 * $Id: misc_sig.c,v 1.3 2004/05/27 16:46:25 jing Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 */
#include <config.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>            /* getpid()                                */
#include <sys/types.h>         /* kill()                                  */

#include <misc.h>              /* MISC_SIG_ defines                       */


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */

/*
 * External Globals
 */

/*
 * Static Globals
 */

/*
 * Static Function Prototypes
 */


/**************************************************************************
 Description: POSIX signal replacement for the Solaris sigset(3C)
              simplified signal management function
       Input: signal number whose disposition is being modified
              pointer to a function or SIG_DFL or SIG_IGN
      Output: POSIX sigaction() is used to change the signal action
     Returns: (Solaris sigset(3C))

      "sigset() is used to modify signal dispositions.  'signo' specifies
      the signal, which may be any signal except SIGKILL and SIGSTOP.
      'disp' specifies the signal disposition, which may be SIG_DFL,
      SIG_IGN, or the address of a signal handler.  If 'disp' is the
      address of a signal handler, the system adds 'signo' to the calling
      process's signal mask before executing the signal handler; when
      the signal handler returns, the system restores the calling
      process's signal mask to its state prior to the delivery of the
      signal.  In addition, if 'disp' is equal to SIG_HOLD, 'signo' is
      added to the calling process's signal mask and the signal's
      disposition remains unchanged."

      "On success, sigset() returns SIG_HOLD if the signal had been
      blocked or the signal's previous disposition if it had not been
      blocked.  On failure, it returns SIG_ERR and sets errno to
      indicate the error."

       Notes:

 **************************************************************************/
void (*
MISC_sig_sigset(int signo, void (*disp)(int)))(int)
{
#ifdef SUNOS
    return(sigset(signo, disp)) ;
#else

    struct sigaction act ;     /* "new" sigaction structure               */
    struct sigaction oact ;    /* "old" sigaction structure               */
    unsigned char return_sig_hold = 0 ;
    int retval ;

    /*
     * Retrieve the current mask of signals to be blocked during
     * execution of signal handler ...
     */
    if (sigaction(signo, (const struct sigaction *) NULL, &oact) < 0) {
        return(SIG_ERR) ;
    }

    /*
     * Determine if this signal had been blocked ...
     */
    retval = sigismember((const sigset_t *) &oact.sa_mask, signo) ;
    if (retval == 1) {
        return_sig_hold = 1 ;
    }
    else if (retval < 0) {
        return(SIG_ERR) ;
    }
    else {
        /*
         * Add the specified signal to the set of signals to be blocked
         * during execution of signal handler ...
         */
        if (sigaddset(&oact.sa_mask, signo) < 0) {
            return(SIG_ERR) ;
        }
    }


    /*
     * Initialize the "new" sigaction structure ...
     */
    if (disp == SIG_HOLD) {
        act.sa_handler = oact.sa_handler ;
    }
    else {
        act.sa_handler = disp ;
    }
    act.sa_flags = oact.sa_flags ;
    act.sa_mask = oact.sa_mask ;

    if (sigaction(signo, (const struct sigaction *) &act,
                  (struct sigaction *) NULL) < 0) {
        return(SIG_ERR) ;
    }

    /*
     * Return SIG_HOLD if signal had been blocked ... otherwise
     * return the signal's previous disposition ...
     */
    if (return_sig_hold) {
        return(SIG_HOLD) ;
    }
    else {
        return(oact.sa_handler) ;
    }

#endif
/***
 *** endif no SUNOS
 ***/

/*END of MISC_sig_sigset()*/
}



/**************************************************************************
 Description: POSIX signal replacement for the Solaris sighold(3C)
              convenience function
       Input: signal number to be added to calling process's signal mask
      Output: POSIX sigprocmask() is used to add the signal to the signal
              mask
     Returns: (Solaris sighold(3C))

              Returns 0 on success.  On failure, -1 is returned and errno
              is set to indicate the error.

       Notes:

 **************************************************************************/
int
MISC_sig_sighold(int signo)
{
#ifdef SUNOS
    return(sighold(signo)) ;
#else

    sigset_t maskset ;           /* new signal set                          */

    /*
     * Build the sigset_t argument for sigprocmask() ...
     */
    if (sigemptyset(&maskset) < 0) {
        return(-1) ;
    }
    if (sigaddset(&maskset, signo) < 0) {
        return(-1) ;
    }

    /*
     * We use sigprocmask() to change the signal mask ...
     */
    return(sigprocmask(SIG_BLOCK, (const sigset_t *) &maskset,
                         (sigset_t *) NULL)) ;
#endif
/***
 *** endif no SUNOS
 ***/

/*END of MISC_sig_sighold()*/
}



/**************************************************************************
 Description: POSIX signal replacement for the Solaris sigrelse(3C)
              convenience function
       Input: signal number to be removed from the calling process's signal
              mask
      Output: POSIX sigprocmask() is used to remove the signal to the
              signal mask
     Returns: (Solaris sigrelse(3C))

              Returns 0 on success.  On failure, -1 is returned and errno
              is set to indicate the error.

       Notes:

 **************************************************************************/
int
MISC_sig_sigrelse(int signo)
{

#ifdef SUNOS
    return(sigrelse(signo)) ;
#else

    sigset_t maskset ;

    /*
     * Build the sigset_t argument for sigprocmask() ...
     */
    if (sigemptyset(&maskset) < 0) {
        return(-1) ;
    }
    if (sigaddset(&maskset, signo) < 0) {
        return(-1) ;
    }

    /*
     * We use sigprocmask() to change the signal mask ...
     */
    return (sigprocmask(SIG_UNBLOCK, (const sigset_t *) &maskset,
                         (sigset_t *) NULL)) ;
#endif
/***
 *** endif no SUNOS
 ***/

/*END of MISC_sig_sigrelse()*/
}

