/**************************************************************************

      Module: lb_locktst.c

 Description:

    Tool for exercising LB_lock() mechanism.

    The lock is broken when the process is terminated (by, for example,
    entering Control-C at the command-line.

 Assumptions:
	The LB file to be locked exists.


**************************************************************************/

/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 17:20:11 $
 * $Id: lb_locktst.c,v 1.2 2014/03/18 17:20:11 jeffs Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 */
#include <stdio.h>
#include <string.h>            /* strncpy(), strspn()                     */
#include <unistd.h>            /* sleep                                   */
#include <stdlib.h>

#include <infr.h>
 

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define PROGNAME_LEN 64
#define PROGNAME_SIZ ((PROGNAME_LEN) + 1)
#define PATHNAME_LEN 1024
#define PATHNAME_SIZ ((PATHNAME_LEN) + 1)

/*
 * Static Globals
 */
static char Prog_name[PROGNAME_SIZ];

/*
 * Static Function Prototypes
 */
static int Read_options(int argc, char **argv,
                        LB_id_t *lb_id, char *lb_name, int *cmd) ;



/**************************************************************************
 Description: Get the command line arguments
       Input: argc, argv
      Output:
     Returns: none
     Globals: Prog_name
       Notes:
 **************************************************************************/
int main (int argc, char **argv)
{
    int cmd ;
    int lb_flags ;
    LB_id_t lb_id ;
    LB_info lb_info ;
    char lb_name[PATHNAME_SIZ] ;
    int lbd ;
    int retval ;
    unsigned sleep_sec ;


    lb_flags = LB_WRITE ;

    retval = Read_options(argc, argv, &lb_id, lb_name, &cmd) ;
    if (retval != 0) {
        exit (EXIT_FAILURE);
    }


    /*
     * Try opening the Linear Buffer ...
     */
    lbd = LB_open(lb_name, lb_flags, NULL);
    if (lbd < 0) {
        (void) printf ("failed in opening LB %s: %d\n", lb_name, lbd) ;
        exit(EXIT_FAILURE) ;
    }

    (void) printf ("About to lock %s ... \n", lb_name) ;

    if (cmd & LB_BLOCK) {
        (void) printf ("LB_BLOCK specified ... \n") ;
    }

    retval = LB_lock(lbd, cmd, lb_id) ;
    if (retval == LB_HAS_BEEN_LOCKED) {
        (void) printf ("LB %s ALREADY LOCKED!\n", lb_name) ;
        exit(EXIT_SUCCESS) ;
    }
    else if (retval != LB_SUCCESS) {
        (void) printf ("failed in locking LB %s: %d\n", lb_name, retval) ;
        exit(EXIT_FAILURE) ;
    }

    (void) printf ("LB_lock returned: %d ...\n", retval) ;
    (void) printf ("Control-C to unlock ... sleeping ...\n") ;


    sleep_sec = 60 ;

    for (;;) {

        (void) sleep(sleep_sec) ;

    } /*endforever*/

    exit (EXIT_SUCCESS);

/*END of main()*/
}



/**************************************************************************
 Description: Read the command-line options and initize several global
              variables.
       Input: argc, argv,
              pointer to storage for linear buffer message id
              pointer to storage for linear buffer file name
              various pointers
      Output: none
     Returns: 0 upon success; otherwise, -1
     Globals: Prog_name
       Notes:
 **************************************************************************/
static int Read_options(int argc, char **argv,
                        LB_id_t *lb_id, char *lb_name, int *cmd)
{
    int block ;
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */
    int retval = 0 ;

    (void) strncpy(Prog_name, MISC_string_basename(argv[0]), PROGNAME_LEN);
    Prog_name[PROGNAME_LEN] = '\0';


    /*
     * Defaults ...
     */
    lb_name[0] = '\0';
    *cmd = LB_EXCLUSIVE_LOCK ;
    block = 0 ;

    err = 0;
    while ((c = getopt (argc, argv, "bhi:l:st")) != EOF) {
    switch (c) {

        case 'b':
             block = LB_BLOCK ;
             break;


        case 'i':
            if (strspn(optarg,"0123456789") != strlen(optarg)) {
                (void) fprintf(stderr,
                               "strspn() indicates LB ID NAN!\n") ;
                err = 1 ;
            }
            retval = sscanf(optarg, "%d", (int *) lb_id) ;
            if (retval == EOF) {
                (void) fprintf(stderr,
                               "sscanf() failed to read lb_id\n") ;
                err = 1 ;
            }
            break;

        case 'l':
            retval = sscanf(optarg, "%s", lb_name) ;
            if (retval == EOF) {
                (void) fprintf(stderr,
                               "sscanf() failed to read LB name\n") ;
                 err = 1 ;
             }
             break;

        case 's':
            *cmd = LB_SHARED_LOCK ;
             break;

        case 't':
            *lb_id = LB_TAG_LOCK ;
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

    if (block == LB_BLOCK) {
        *cmd |= LB_BLOCK ;
    }

    lb_name[PATHNAME_LEN] = 0 ;

    if (strlen(lb_name) == 0) {
        err = 1 ;
    }


    if (err == 1) {

      (void) printf ("\nUsage: %s [options] [-i msgid | -t] -l LB_file\n",
                     Prog_name);
      (void) printf ("\n\tOptions:\n\n");
      (void) printf ("\t\t-b  LB_BLOCK [default: non-blocking]\n");
      (void) printf ("\t\t-h  print usage information and exit\n");
      (void) printf ("\t\t-i  message id [default: latest message]\n");
      (void) printf ("\t\t-s  shared lock [default: exclusive lock]\n");
      (void) printf ("\n");
      (void) printf ("\n\t\t -i msgid can only be used with LB_REPLACE LB");
      (void) printf ("\n\t\t -t locks attribute header");
      (void) printf ("\n\n");

      return(-1) ;
   }


   return(0);

/*END of Read_options()*/
}
