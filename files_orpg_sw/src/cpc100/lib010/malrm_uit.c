/**************************************************************************

      Module:  malrm_uit.c

 Description:
        Unit Integration and Test driver for the Multiple Alarm Services
	(MALRM) library.

	This test driver accepts no command line arguments.  Instead, it
	reads from standard input until it is killed, either with the "-q"
	option or by some sort of interrupt like ^C.  Integer alarm IDs are
	expected.

	-r alarm_id callback_index       ... register an alarm
	-d alarm_id                      ... deregister an alarm
        -c alarm_id                      ... cancel an alarm
        -s alarm_id start_time ntvl_time ... set an alarm

	The number of alarms that may be registered or deregistered at a
	time is constrained by the value of _POSIX_MAX_INPUT.  

	The user may register/deregister a MALRM callback routine for a given
	alarm ID, one at a time:

	malrm_uit -r 1 0
		...
	malrm_uit -r 100 0

	malrm_uit -d 1
		...
	malrm_uit -d 100

	Currently, two callback routines are provided in this test driver.
	The second callback routine may be used to exercise exception-handling.

	This test driver uses a single MALRM callback routine for all
	registered alarm IDs.

	The user may choose to pause for a number of seconds with the
	"-p numsecs" option.

	The user may cleanly exit the test driver with the "-q" option.

	Following is an example test script:

        -----------------------------------------------------------
	#!/bin/sh

	malrm_uit <<TestCase
	-r 1 0
	-d 1
	-q
	TestCase
        -----------------------------------------------------------

	Another alternative is to create a test data file:

        -----------------------------------------------------------
	-r 1 0
	-d 1
	-q

	And read that data file (and others, perhaps) from the test
	script:

        -----------------------------------------------------------
	#!/bin/sh
        for file in test01.dat test02.dat test03.dat
        do
	   echo "Data File: $file"
	   echo "`date`"
   	   malrm_uit < $file
	   echo " "
        done
        -----------------------------------------------------------

 Assumptions:

 **************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/05/27 16:45:43 $
 * $Id: malrm_uit.c,v 1.4 2004/05/27 16:45:43 jing Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 */
#include <config.h>
#include <limits.h>            /* _POSIX_MAX_INPUT                        */
#include <unistd.h>            /* getopt,sleep                            */
#include <stdio.h>             /* stderr                                  */
#include <stdlib.h>            /* atoi                                    */
#include <string.h>            /* strtok                                  */
#include <strings.h>           /* bzero                                   */
#include <signal.h>
#include <time.h>

#include <malrm.h>


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define TRUE 1
#define FALSE 0
#define MAX_ENTRY_LEN 512
#define MAX_INPUT_LINE_LEN _POSIX_MAX_INPUT
#define NAME_LEN 128
#define WHOAMI "malrm_uit"

/*
 * Static Globals
 */
static int Autoflag ;
static char Prog_name[NAME_LEN];


/*
 * External Globals
 */
extern char *optarg ;
extern int optind ;
extern int opterr ;
extern int optopt ;

/*
 * Function Prototypes
 */
static void Callback0(malrm_id_t alarmid) ;
static void Callback1(malrm_id_t alarmid) ;
static int Read_options(int argc, char **argv) ;

/*
 *	Unit Test Cases
 *	UT Case 1.1
 */
static void Ut_case_1_1(void) ;



/**************************************************************************
 Description: MALRM Library Test Driver
       Input: standard input
      Output: standard output
     Returns: none
       Notes:
 **************************************************************************/
void main( int argc , char ** argv )
{
   malrm_id_t alarmid ;
   char buf[_POSIX_MAX_INPUT] ;
   int c ;
   void (*(callback[2]))() ;
   int callback_ndx ;
   time_t current_time ;
   char *fgets_p ;
   register i ;
   unsigned int interval_secs ;
   int myargc ;
   char *myargv[MAX_ENTRY_LEN] ;
   unsigned int numsecs ;
   unsigned int remaining_secs ;
   int retval ;
   time_t start_time ;

   callback[0] = Callback0 ;
   callback[1] = Callback1 ;

   retval = Read_options(argc,argv) ;
   if (retval != 0) {
      exit (EXIT_FAILURE);
   }


   if (Autoflag) {
      /*
       * Execute automated (canned) Unit Integration and Test cases ...
       */
      Uit_case_1_1() ;
      exit(EXIT_SUCCESS) ;
   }

   for (;;) {

      current_time = time((time_t *) NULL) ;

      (void) printf("USAGE: \t%d \t%s",current_time,ctime(&current_time)) ;
      (void) printf("\t-r alarm_id callback_index       (register)\n") ;
      (void) printf("\t-d alarm_id                      (deregister)\n") ;
      (void) printf("\t-c alarm_id                      (cancel)\n") ;
      (void) printf("\t-s alarm_id start_time ntvl_time (set)\n") ;

      /*
       * Get arguments from standard input ...
       */
      bzero(buf,_POSIX_MAX_INPUT) ;
      fgets_p = fgets(buf,_POSIX_MAX_INPUT,stdin) ;
      if (fgets_p == NULL) {
         continue ;
      }


      /*
       * getopt() does not care about myargv[0]
       */
      myargv[1] = strtok(buf," ") ;
      for (myargc=2; myargc < MAX_ENTRY_LEN; myargc++){
         myargv[myargc] = strtok(NULL," ") ;
         if (myargv[myargc] == NULL) {
            break ;
         }
      }


      /*
       * prime the getopt() pump (unnecessary the first time around, but
       * harmless)
       */
      optind = 1 ;
      opterr = 1 ;

      while ((c = getopt(myargc,myargv,"c:d:p:r:s:q#")) != -1) {
         switch(c) {

            case 'c':
               /*
                * Cancel an alarm ...
                */
               if (optind < 2) {
                  (void) printf("USAGE: -c alarmid\n") ;
                  break ;
               }
               alarmid = (malrm_id_t) atoi(optarg) ;

               (void) printf("\n") ;
               (void) printf("%s: Cancelling Alarm %d\n",
                              WHOAMI,alarmid) ;
               retval = MALRM_cancel(alarmid, &remaining_secs) ;
               (void) printf("MALRM_cancel: %d remaining_secs: %u\n",
                             retval,remaining_secs) ;
               (void) printf("\n") ;

               break ;

            case 'd':
               /*
                * Deregister an alarm ...
                */
               if (optind < 2) {
                  (void) printf("USAGE: -d alarmid\n") ;
                  break ;
               }
               alarmid = (malrm_id_t) atoi(optarg) ;

               (void) printf("\n") ;
               (void) printf("%s: Deregistering Alarm %d\n",
                              WHOAMI,alarmid) ;
               retval = MALRM_deregister(alarmid) ;
               (void) printf("MALRM_deregister: %d\n",retval) ;
               (void) printf("\n") ;

               break ;

            case 'p':
               /*
                * Pause ...
                */
               numsecs = (unsigned int) atoi(optarg) ;
               (void) printf("\n") ;
               (void) printf("%s: Sleeping %d seconds\n",WHOAMI,(int) numsecs) ;
               (void) printf("\n") ;
               (void) sleep(numsecs) ;
               break ;

            case 'q':
               exit(EXIT_SUCCESS) ;
               break ;

            case 'r':
               /*
                * Register an alarm ...
                */
               if (optind < 3) {
                  (void) printf("USAGE: -r alarmid callback_ndx\n") ;
                  break ;
               } ;
               alarmid = (malrm_id_t) atoi(optarg) ;
               callback_ndx = atoi(myargv[optind]) ;
               (void) printf("\n") ;
               (void) printf("%s: Registering Callback Index %d for Alarm %d\n",
                              WHOAMI,callback_ndx,alarmid) ;
               retval = MALRM_register(alarmid, callback[callback_ndx]) ;
               (void) printf("MALRM_register: %d\n",retval) ;
               (void) printf("\n") ;
               break ;

            case 's':
               /*
                * Set an alarm ...
                */
               if (optind < 2) {
                  (void) printf("USAGE: -s alarmid [1|2] interval_secs\n") ;
                  break ;
               } ;
               alarmid = (malrm_id_t) atoi(optarg) ;
               start_time = (time_t) atoi(myargv[optind++]) ;
               interval_secs = atoi(myargv[optind++]) ;
               (void) printf("\n") ;
               (void) printf("%s: Setting Alarm %d start %d ntvl %u\n",
                              WHOAMI,alarmid,start_time,interval_secs) ;
               retval = MALRM_set(alarmid,
                                  start_time,
                                  interval_secs) ;
               (void) printf("MALRM_set: %d\n",retval) ;
               (void) printf("\n") ;
               break ;

            case '#':
               c = -1 ;
               break ;

            default:
               (void) printf("\n") ;
               (void) printf("%s: Unrecognized option: 0x%x\n",WHOAMI,c) ;
               (void) printf("\n") ;
               break ;

         } ;

      }/*endwhile more stdin arguments to read*/


   }/*end forever*/


/*END of main()*/
}


/**************************************************************************
 Description: MALRM callback routine
       Input: event code, pointer to EN message, size of EN message
      Output: none
     Returns: none
       Notes:
 **************************************************************************/
static void Callback0(malrm_id_t alarmid)
{
   time_t current_time ;

   current_time = time((time_t *) NULL) ;
   (void) fprintf(stdout,"%s: Callback0 alarmid: %d %s",
		WHOAMI,(int) alarmid,
                ctime(&current_time)) ;
   return ;

/*END of Callback0()*/
}


/**************************************************************************
 Description: MALRM callback routine
       Input: event code, pointer to EN message, size of EN message
      Output: none
     Returns: none
       Notes:
 **************************************************************************/
static void Callback1(malrm_id_t alarmid)
{
   time_t current_time ;

   (void) fprintf(stdout,"%s: Callback1 alarmid: %d %s",
		WHOAMI,(int) alarmid,
                ctime(&current_time)) ;
   return ;

/*END of Callback1()*/
}


/**************************************************************************
 Description: Read the command-line options
       Input: argc, argv,
      Output: none
     Returns: 0 upon success; otherwise, -1
       Notes:
 **************************************************************************/
static int Read_options(int argc,
                        char **argv)
{
   extern char *optarg;    /* used by getopt */
   extern int optind;
   int c;                  /* used by getopt */
   int err;                /* error flag */
   int retval = 0 ;

   Autoflag = 0 ;
   (void) strncpy(Prog_name, argv[0], NAME_LEN);
   Prog_name[NAME_LEN - 1] = '\0';


   err = 0;
   while ((c = getopt (argc, argv, "ah")) != EOF) {
   switch (c) {

      case 'a':
         Autoflag = 1 ;
         break;

      case 'h':
         /*
          * INTENTIONAL FALL-THROUGH ...
          */
      case '?':
         err = 1;
         break;

      } /*endswitch*/

   } /*endwhile command-line characters to read*/

   if (err == 1) {

      (void) printf ("Usage: %s [-a][-h]\n",
                     Prog_name);
      (void) printf ("      Options:\n");
      (void) printf ("      -a  run automated (canned) uit cases \n");
      (void) printf ("      -h  print usage information\n");
      (void) printf ("\n");
      (void) printf ("DEFAULT: interactive mode (additional usage msg provided)\n");
      (void) printf ("\n");

      retval = -1 ;
   }


   return(retval);

/*END of Read_options()*/
}


/**************************************************************************
 Description: Unit Integration and Test Case 1.1 ??? ???
       Input: void
      Output: none
     Returns: void
       Notes:
 **************************************************************************/
static void Uit_case_1_1()
{
   malrm_id_t alarmid ;
   time_t curtime ;
   int expected ;
   register int i ;
   int retval ;

   static char *uut = "???" ;

   curtime = time((time_t *) NULL) ;

   (void) printf("BEGIN UIT Case 1.1 ??? ... %s",
                 ctime(&curtime)) ;

   /*
    * Subtest a:
    */


   /*
    * Subtest b:
    */

   (void) printf("\n   END UIT Case 1.1 ??? ... %s",
                 ctime(&curtime)) ;


   return ;

/*END of Uit_case_1_1()*/
}
