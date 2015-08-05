/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2011/05/22 16:48:20 $
 * $Id: signals.c,v 1.1 2011/05/22 16:48:20 cmn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <infr.h>

/* Constant Definitions/Macro Definitions/Type Definitions */
#define SIGNAL_NAME_SZ 		16

typedef struct signal_attr {

   int signal;

   unsigned char coregen;    /* flag; if set, this is sig generates 
                                core file. */

   unsigned char catch;      /* flag; if set, we are to catch this 
                                signal. */

   char name[SIGNAL_NAME_SZ];

} Signal_attr_t;

static Signal_attr_t Signals[] = {{  SIGHUP, 0, 1,  "SIGHUP"},
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

static size_t Num_signals = sizeof(Signals) / sizeof(Signal_attr_t) ;

/* Function Prototypes. */
static int (*User_term_handler) ( int signal, int sig_type ) = NULL;
static void Term_handler( int signal );
static Signal_attr_t * Get_sigattr( int signal );

/*\///////////////////////////////////////////////////////////////////////////

   Description: 
      Register a termination handler.

   Input: 
      Pointer to handler function (may be NULL)

   Returns: 
      0 upon success; otherwise -1
             
/////////////////////////////////////////////////////////////////////////\*/
int SIG_reg_term_handler( int (*term_handler)(int, int) ){

   int i ;
   void (*retval)(int) ;

   /* Deregister termination handler. */
   if( term_handler == NULL ){

      User_term_handler = NULL;
      return 0;

   }

   for( i = 0; i < Num_signals; ++i ){

      /* Automatic variables ... */
      Signal_attr_t *sigattr_p = &(Signals[i]);

      if( sigattr_p->catch ){

         retval = MISC_sig_sigset( sigattr_p->signal, Term_handler );
         if( retval == SIG_ERR ){

            fprintf( stderr, "Unable to register sig %d (%s)\n",
                     sigattr_p->signal, sigattr_p->name );
            return(-1) ;

         }

      } /* endif this signal is catchable and we are to catch it */

   }/*endfor each signal in the attribute table*/

   User_term_handler = term_handler;

   return(0) ;

/* End of Reg_term_handler(). */
}

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Handle catchable termination signals
  
      The registered cleanup function --- if any --- will be executed prior
      to exiting.

////////////////////////////////////////////////////////////////////////////\*/
static void Term_handler( int signal ){

   Signal_attr_t *sigattr_p ;
   int exit_code = -1;
   int to_exit;

   if( signal == SIGPIPE )
      return;

   /* Determine if the process is being terminated "normally" ... */
   sigattr_p = Get_sigattr( signal );
   if( sigattr_p == NULL )
      fprintf( stderr, "TERM HANDLER signal %d: no signal attribute found!\n",
               signal) ;
    
   else{

      if( !sigattr_p->coregen )
         exit_code = -1;
       
   }

   to_exit = 1;
   if( User_term_handler != NULL ){

      /* If user termination handler returns value other than 0,
         do not exit. */
      if( User_term_handler( signal, exit_code ) != 0 )
            to_exit = 0;

      fprintf( stderr, "Term handler called (sig %d, sigtype %d)\n",
               signal, exit_code );
      if( !to_exit )
         fprintf( stderr, "--->To Exit Immediately, Use CTRL-C\n" );

   }

   /* User termination handler returned non-zero value. */
   if( to_exit )
      exit(exit_code);

   else
      return;

/* End of Term_handler(). */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      Get attributes for specified signal

   Input: 
      signal

   Returns: 
      Pointer to signal attributes upon success; otherwise, NULL
      pointer is returned

***************************************************************************/
static Signal_attr_t *Get_sigattr( int signal ){

   register int i ;
   static Signal_attr_t *sigattr_p = NULL ;

   for( i = 0; i < Num_signals; ++i ){

      sigattr_p = &(Signals[i]);
      if( signal == sigattr_p->signal )
         return(sigattr_p) ;
        
   }

   return((Signal_attr_t *) NULL) ;

/* End of Get_sigattr() */
}

