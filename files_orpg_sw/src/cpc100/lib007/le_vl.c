/*************************************************************************

      Module: le_vl.c

 Description:

    LE Verbosity Level (VL) Tool

    Use this tool to change the LE VL of a given process.  The process
    is identified by its PID; the process may be executing on the local
    host or on a remote host.
    
 Important Data Structures:
    TBD

 Assumptions:
    Event Notification Daemon is up and running on the pertinent hosts.
	The LE_DIR_EVENT environment variable, if set, is correct.

 Notes:

**************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2006/02/10 21:50:04 $
 * $Id: le_vl.c,v 1.4 2006/02/10 21:50:04 jing Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 */
#include <assert.h>
#include <errno.h>
#include <netdb.h>             /* MAXHOSTNAMELEN                          */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>         /* MAXPATHLEN                              */
#include <sys/types.h>

#include <infr.h>


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define NAME_LEN 128

/*
 * External Globals
 */
extern int errno ;
extern int optind ;


/*
 * Static Globals
 */
static char Prog_name[NAME_LEN];


/*
 * Static Function Prototypes
 */
static int
Read_options(int argc, char **argv, int *target_pid, int *new_vl,
             char *pidhost, unsigned int *pidhost_ip) ;


/**************************************************************************
 Description: Get the command line arguments; open the linear buffer file
       Input: argc, argv
      Output:
     Returns: none
       Notes:
 **************************************************************************/
int
main (int argc, char **argv)
{
    int new_level ;
    char pidhost[MAXHOSTNAMELEN+1], buf[64];
    int retval ;
    int target_pid ;
    unsigned int host_ip ;


    retval = Read_options(argc, argv, &target_pid, &new_level,
                          pidhost, &host_ip) ;
    if (retval != 0) {
       exit(EXIT_FAILURE);
    }

    (void) fprintf(stdout,
                   "Changing VL host %s (%s) PID %d New Level %d\n",
                   pidhost, NET_string_IP (host_ip, 1, buf),
                   target_pid, new_level) ;

    LE_init(argc, argv) ;

    LE_set_vl(host_ip, target_pid, new_level) ;

    exit(EXIT_SUCCESS) ;

/*END of main()*/
}

/**************************************************************************
 Description: Read the command-line options and initialize several global
              variables.
       Input: argc, argv,
      Output: none
     Returns: 0 upon success; otherwise, -1
       Notes:
 **************************************************************************/
static int
Read_options(int argc, char **argv, int *target_pid, int *new_vl,
             char *pidhost, unsigned int *pidhost_ip)
{
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */
    int retval = 0 ;
    int retval2 ;

    *target_pid = 0 ;
    *new_vl = -1 ;

    (void) memset(Prog_name, 0, sizeof(Prog_name)) ;
    (void) strncpy(Prog_name, MISC_string_basename(argv[0]),
                   sizeof(Prog_name) - 1);

    (void) memset(pidhost, 0, MAXHOSTNAMELEN+1) ;
    retval2 = gethostname(pidhost, MAXHOSTNAMELEN) ;
    if (retval2 < 0) {
        (void) fprintf(stderr,"gethostname() failed: %d (errno %d)\n",
                       retval2, errno) ;
        return(-1) ;
    }


    err = 0;
    while ((c = getopt (argc, argv, "e:H:hl:p:")) != EOF) {
    switch (c) {
 
        case 'H':
            (void) strncpy(pidhost, optarg, MAXHOSTNAMELEN) ;
            break;

        case 'l':
            if (strspn(optarg,"0123456789") != strlen(optarg)) {
                (void) fprintf(stderr,
                               "strspn() indicates level NAN!\n") ;
                err = 1 ;
                break ;
            }
            retval2 = sscanf(optarg, "%d", new_vl) ;
            if (retval2 == EOF) {
                (void) fprintf(stderr,
                               "sscanf() failed to read new_vl\n") ;
                err = 1;
                break;
            }
            break;

       case 'p':
            if (strspn(optarg,"0123456789") != strlen(optarg)) {
                (void) fprintf(stderr,
                               "strspn() indicates PID NAN!\n") ;
                err = 1 ;
                break ;
            }
            retval2 = sscanf(optarg, "%d", target_pid) ;
            if (retval2 == EOF) {
                (void) fprintf(stderr,
                               "sscanf() failed to read target_pid\n") ;
                err = 1;
                break;
            }
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

    /*
     * The PID and New Level arguments are required ...
     */
    if ((*target_pid == 0) || (*new_vl == -1)) {
        err = 1 ;
    }

    if ( ! err ) {
        /*
         * Determine the IP address of the PID host ...
         */
        *pidhost_ip = NET_get_ip_by_name(pidhost) ;
        if (*pidhost_ip == INADDR_NONE) {
            (void) fprintf(stderr,"NET_get_ip_by_name(%s) failed!\n",
                           pidhost) ; 
            err = 1 ;
        }
    }
    


    if (err == 1) {

        (void) printf ("\nUsage: %s [options] -p pid -l new_level\n",
                       Prog_name);
        (void) printf ("\n\tOptions:\n\n");
        (void) printf ("\t\t-H  hostname [default: %s]\n", pidhost);
        (void) printf ("\t\t-h  print usage message and exit\n") ;
        (void) printf ("\n");

        retval = -1 ;
    }

    return(retval);

/*END of Read_options()*/
}
