/*************************************************************************

      Module: lelb_mon.c

**************************************************************************/

/*
 * RCS info
 * $Author: eddief $
 * $Locker:  $
 * $Date: 2002/05/14 18:52:37 $
 * $Id: lelb_mon.c,v 1.34 2002/05/14 18:52:37 eddief Exp $
 * $Revision: 1.34 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 */
#include <assert.h>
#include <ctype.h>             /* isprint()                               */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>         /* MAXPATHLEN                              */
#include <sys/types.h>

#include <orpgerr.h>
#include <infr.h>
#include <rss_lb.h>            /* replace LB_ fxns with RSS_LB_ fxns */


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define CRIT_MSGCODE_CHAR	'C'
#define NONCRIT_MSGCODE_CHAR	' '

#define DFLT_DYN_POLL_MSEC	1000
#define DSPLY_PROCNAME_LEN	16
#define DSPLY_PROCNAME_SIZE	((DSPLY_PROCNAME_LEN)+1)
#define MON_MODE_DYNAMIC 0
#define MON_MODE_STATIC 1
#define NAME_LEN 128
#define PROCNAME_LEN (size_t) 64
#define PROCNAME_SIZE (size_t) ((PROCNAME_LEN) + 1)
#define SLEEP_SEC	30

/*
 * Is there a more sophisticated way of determing the where the text[0]
 * byte appears within the LE_critical_message and LE_message structures?
 */
typedef struct {
    char text[1] ;
} pad_struct_t ;

#define LEMSG_TXT_PAD_BYTES (ALIGNED_SIZE(sizeof(pad_struct_t)))

/*
 * The Process Array is comprised of one or more Process List Entries.
 */
typedef struct {
    char procname[PROCNAME_SIZE] ;
    char lbname[MAXPATHLEN+PROCNAME_LEN+5+1] ;
                               /* 5 add'l chars for "/" and ".log"        */
    int lbd ;                  /* LB descriptor                           */
    unsigned char msg_to_print ;
    ALIGNED_t curmsg[ALIGNED_T_SIZE (LE_MAX_MSG_LENGTH+1)] ;
                               /* temp storage for msg before printing    */
} Procary_entry ;

/*
 * External Globals
 */
extern int optind ;


/*
 * Static Globals
 */
static char Codemap_fname[NAME_LEN+1] ;
static char Proc_fname[NAME_LEN+1] ;

static int History_length = 0 ;
static ALIGNED_t Le_buf[ALIGNED_T_SIZE (LE_MAX_MSG_LENGTH)] ;
static char Le_dir[MAXPATHLEN+1] ;
static en_t Le_critical_evtcd ;
static int Monitor_mode = MON_MODE_DYNAMIC ;
static int Numprocs = 0 ;
static int Poll_rate_msec ;
static Procary_entry *Procary_p = NULL ;
                               /* Points to head of the Process List      */
                               /*                                         */
static char Prog_name[NAME_LEN];
static char *Le_file_name = NULL;

static int Pid, New_vl;

/*
 * Static Function Prototypes
 */
static  int Print_interleaved_le(int num_procs,
                                 Procary_entry *procary_p,
                                 int num_msgs) ;
static void Print_lemsg(LE_message *lemsg_p, char *procname) ;
static  int Print_static_le(Procary_entry *proc_p, int num_msgs) ;
static  int Read_next_msg(Procary_entry *proc_p,
                          unsigned char seek_flag,
                          int offset) ;
static  int Read_options(int argc, char **argv) ;
static  int Read_processes_file(void) ;
static void Change_process_verbose_level ();
static void Remove_trailing_line_return (char *in_string);


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
    register int i ;
    int retval ;
    unsigned int sleep_sec ;
    unsigned int unslept_sec ;
    Procary_entry *proc_p ;


    retval = Read_options (argc, argv);
    if (retval != 0) {
       exit(EXIT_FAILURE);
    }

    if (Pid > 0)
	Change_process_verbose_level ();

    if (Le_critical_evtcd <= 0) {

        /*
         * Either dynamic or static "monitoring" of LE LB files being
         * written to by one or more processes ...
         */

        if (Le_file_name != NULL) {
            Numprocs = 1; 
            Procary_p = calloc((size_t) Numprocs, sizeof(Procary_entry)) ;
            if (Procary_p == NULL) {
                fprintf (stderr, "calloc of Process Array failed: %d", errno) ;
                exit(EXIT_FAILURE) ;
            }
            proc_p = Procary_p;
            strcpy (proc_p->procname, (char *)Le_file_name) ;
	}
        else if (strlen(Proc_fname)) {

            retval = Read_processes_file() ;
            if (retval < 0) {
                (void) fprintf(stderr,
                               "Read_processes_file() returned %d\n",
                               retval) ;
                exit(EXIT_FAILURE) ;
            }

        }
        else {
            /*
             * Read the process name(s) from the command-line ...
             * We've already performed some error-checking in Read_options()
             * to ensure that at least one process name has been specified
             * (we did that check so that we could print a usage message).
             */

            Numprocs = argc - optind ; 

            errno = 0 ;
            Procary_p = calloc((size_t) Numprocs, sizeof(Procary_entry)) ;
            if (Procary_p == NULL) {
                (void) fprintf(stderr,"calloc of Process Array failed: %d", errno) ;
                exit(EXIT_FAILURE) ;
            }

            for (i=optind; i < argc; ++i) {

                proc_p = (Procary_p + ((i-optind))) ;

                (void) strncpy(proc_p->procname,
                               (char *) argv[i],
                               PROCNAME_LEN) ;
            }

        }

        if (Numprocs <= 0) {
            (void) fprintf(stderr,
                           "SORRY! No processes specified!\n") ;
            exit(EXIT_FAILURE) ;
        }

        /*
         * Use the process names and the well-known LE LB filename
         * extension (".log") to build the LB filenames ...
         * we have already gone to great lengths to ensure that
         * the directory and process names are not too long ...
         */
	if (Le_file_name == NULL) {
	    for (i=0; i < Numprocs; ++i) {
		proc_p = (Procary_p + i) ;
		(void) sprintf(proc_p->lbname,
			       "%s/%s.log",
			       Le_dir, proc_p->procname) ;
	    }
	}
	else {
	    for (i=0; i < Numprocs; ++i) {
		proc_p = (Procary_p + i) ;
		strcpy (proc_p->lbname, proc_p->procname) ;
	    }
	}

        if (Monitor_mode == MON_MODE_STATIC) {
            /*
             * Display the specified number of messages and then exit ...
             * Ignore any processes listed after the first process ...
             */

            Procary_p->lbd = LB_open(Procary_p->lbname,
                                     LB_READ,
                                     NULL) ;
            if (Procary_p->lbd < 0) {
                (void) fprintf(stderr,
                               "LB_open(%s) returned %d\n",
                               Procary_p->lbname, Procary_p->lbd) ;
                exit(EXIT_FAILURE) ;
            }

            retval = Print_static_le(Procary_p,
                                     History_length) ;
            if (retval < 0) {
                (void) fprintf(stderr,
                               "Print_static_le() returned %d\n",
                               retval) ;
                exit(EXIT_FAILURE) ;
            }

            exit(EXIT_SUCCESS) ;
        }
        else {
            /*
             * Dynamic monitoring of LE LB messages ...
             * First, open each of the LB files ...
             */
            for (i=0; i < Numprocs; ++i) {
                proc_p = (Procary_p + i) ;
                proc_p->lbd = LB_open(proc_p->lbname,
                                      LB_READ,
                                      NULL) ;
                if (proc_p->lbd < 0) {
                    (void) fprintf(stderr,
                                   "LB_open(%s) returned %d\n",
                                   proc_p->lbname, proc_p->lbd) ;
                    proc_p->lbd = -1 ;
		    exit (1);
                }
            } /*endfor each of the processes*/

            /*
             * Display extant messages ...
             */
            retval = Print_interleaved_le(Numprocs, Procary_p, 0) ;
            if (retval < 0) {
                (void) fprintf(stderr,
                               "Print_interleaved_le() returned %d\n",
                               retval) ;
                exit(EXIT_FAILURE) ;
            }

            /*
             * Information message ...
             */
            (void) fprintf(stderr,
                           "Monitoring LE LB messages (every %d msecs) for ",
                           Poll_rate_msec) ;
            for (i=0; i < Numprocs; ++i) {
                proc_p = (Procary_p + i) ;
                if (proc_p->lbd > 0) {
                    (void) fprintf(stderr, "%s ", proc_p->procname) ;
                }
            }
            (void) fprintf(stderr,"\n") ;

	    fflush (stdout);
            for (;;) {

                /*
                 * msleep() return value is of no use (re: misc(3))
                 */
                (void) msleep(Poll_rate_msec) ;

                retval = Print_interleaved_le(Numprocs,
                                              Procary_p,
                                              -1) ;
                if (retval < 0) {
                    (void) fprintf(stderr,
                                   "Print_interleaved_le() returned %d\n",
                                   retval) ;
                }

		fflush (stdout);
            } /*endfor ever ...*/

        }

    }
    else {

        sleep_sec = SLEEP_SEC ;
        for (;;) {

            unslept_sec = sleep(sleep_sec) ;
            if (unslept_sec > SLEEP_SEC) {
               unslept_sec = SLEEP_SEC ;
            }
            if (unslept_sec) {
               sleep_sec = unslept_sec ;
            }
            else {
               sleep_sec = SLEEP_SEC ;
            }

        } /*endfor ever ...*/

    }

/*END of main()*/
}

/********************************************************************

    Changes process's ("Pid") verbose level to "New_vl". We don't call
    LE_set_vl because we don't want call LE_init, which requires the
    LE LB to exist.

*********************************************************************/

#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>

static void Change_process_verbose_level () {
    unsigned int *add;
    int nadd;
    char *dir_ev, *pt;
    LE_VL_change_ev_t msg;
    int event_num, ret;

    nadd = NET_find_local_ip_address (&add);
    if (nadd <= 0) {
	fprintf (stderr, "NET_find_local_ip_address failed (ret %d)\n", nadd);
	exit (1);
    }

    if ((dir_ev = getenv ("LE_DIR_EVENT")) == NULL) {
	fprintf (stderr, "Environment LE_DIR_EVENT not defined\n");
	exit (1);
    }
    pt = dir_ev;
    while (*pt != '\0' && *pt != ':')
	pt++;
    if (*pt != ':' ||
	sscanf (pt + 1, "%d", &event_num) != 1) {
	fprintf (stderr, "LE event number not foundd\n");
	exit (1);
    }

    msg.host_ip = htonl (add[0]);
    msg.pid = htonl (Pid);
    msg.new_vl = htonl (New_vl);

    ret = EN_post (event_num, (void *)&msg, 
					sizeof (LE_VL_change_ev_t), 0);
    if (ret < 0) {
	fprintf (stderr, "EN_post failed (ret %d)\n", ret);
	exit (1);
    }

    exit (0);
}



/**************************************************************************
 Description: Print messages in LE LB file
       Input: Pointer to Process Array entry
      Output: messages are displayed
     Returns: number of messages displayed upon success; otherwise, -1
       Notes:
 **************************************************************************/
static int
Print_interleaved_le(int num_procs,
                     Procary_entry *procary_p,
                     int num_msgs)
{
    unsigned char seek_flag ;
    register int i ;
    unsigned int msgs_to_print = 0 ;
    int offset ;
    short oldest_index ;
    time_t oldest_time = (time_t) 0 ;
    Procary_entry *proc_p ;
    int retval = 0 ;
    int retval2 ;


    /*
     * num_msgs:
     *     < 0 - read only those messages that have been written
     *           to the LB since we last read from it
     *       0 - read all available messages
     *     > 0 - read that many of the "latest" messages
     *
     * Determine the seek offset and where to seek from ...
     */
    if (num_msgs == 0) {
        (void) fprintf(stderr, "All LE LB messages for ") ;

        for (i=0; i < Numprocs; ++i) {
            proc_p = (procary_p + i) ;
            if (proc_p->lbd > 0) {
                (void) fprintf(stderr, "%s ", proc_p->procname) ;
            }
        }
        (void) fprintf(stderr, "\n") ;

        offset = 0 ;
        seek_flag = 1 ;
    }
    else if (num_msgs > 0) {
        (void) fprintf(stderr, "Last %d LE LB messages for ", num_msgs) ;
        for (i=0; i < Numprocs; ++i) {
            proc_p = (procary_p + i) ;
            if (proc_p->lbd > 0) {
                (void) fprintf(stderr, "%s ", proc_p->procname) ;
            }
        }
        (void) fprintf(stderr, "\n") ;

        offset = (-1) * (num_msgs - 1) ;
        seek_flag = 1 ;
    }
    else {
#ifdef NEVERDEFINED
        (void) fprintf(stderr, "latest LE LB messages for ", num_msgs) ;
        for (i=0; i < Numprocs; ++i) {
            proc_p = (procary_p + i) ;
            if (proc_p->lbd > 0) {
                (void) fprintf(stderr, "%s ", proc_p->procname) ;
            }
        }
        (void) fprintf(stderr, "\n") ;
#endif
        offset = 0 ;
        seek_flag = 0 ;
    }


    /*
     * Read initial batch of messages ... (one message
     * per process) ...
     */
    for (i = 0; i < num_procs; ++i) {

        proc_p = (procary_p + i) ;
   
        retval2 = Read_next_msg(proc_p,
                                seek_flag,
                                offset) ;
        if (retval2 < 0) {
            if (retval2 != LB_TO_COME) {
                (void) fprintf(stderr,
                               "Read_next_msg(%s) returned %d\n",
                               proc_p->lbname, retval2) ;
            }
        }
        else {
            ++msgs_to_print ;
        }

    } /*endfor each of the processes*/

    while (msgs_to_print) {

        oldest_index = 0 ;
        oldest_time = (time_t) 0 ;

        for (i = 0; i < num_procs; ++i) {


            /*
             * Automatic variables ...
             */
            LE_message lemsg ;
            LE_message *lemsg_p ;

            proc_p = (procary_p + i) ;

            /*
             * Ignore this process if it has no message to be printed ...
             */
            if (proc_p->msg_to_print) {

                (void) memcpy(&lemsg,
                              proc_p->curmsg,
                              ALIGNED_SIZE(sizeof(LE_message))) ;
                lemsg_p = (LE_message *) &lemsg ;

                if (oldest_time == (time_t) 0) {
                    oldest_time = lemsg_p->time ;
                    oldest_index = i ;
                }
                else {
                    if (lemsg_p->time < oldest_time) {
                        oldest_time = lemsg_p->time ;
                        oldest_index = i ;
                    }
                }

            } /*endif this process has a message to be printed*/

        } /*endfor each of the processes*/

        /*
         * Print only the oldest LE LB message ...
         */
        proc_p = (procary_p + oldest_index) ;
        Print_lemsg((LE_message *) proc_p->curmsg,
                    (char *) proc_p->procname) ;
        (void) memset(proc_p->curmsg, 0, sizeof(proc_p->curmsg)) ;
        proc_p->msg_to_print = 0 ;
        --msgs_to_print ;

        /*
         * TBD: read next message for the process for which
         * we just displayed an LE message ...
         */
        retval2 = Read_next_msg(proc_p, 0, -1) ;
        if (retval2 < 0) {
            if (retval2 != LB_TO_COME) {
                (void) fprintf(stderr,
                               "Read_next_msg(%s) returned %d\n",
                               proc_p->lbname, retval2) ;
            }
        }
        else {
            ++msgs_to_print ;
        }

    } /*endwhile we have one or more messages to print*/

    return(retval) ;

/*END of Print_interleaved_le()*/
}



/**************************************************************************
 Description: Print LE message
       Input: Pointer to message
      Output: message is displayed
     Returns: void
       Notes: We have not implemented support for optional LE message-code
              mapping.
 **************************************************************************/
static void
Print_lemsg(LE_message *msg_p, char *procname)
{
    int code ;
    LE_critical_message *critmsg_p ;
    char fname[LE_SOURCE_NAME_SIZE] ;
    LE_message* lemsg_p ;
    char msgcode_char ;        /* use unless code-mapping specified       */

                               /* local copies of LE message elements ... */
                               /* helpful because we have two types of    */
                               /* messages to deal with                   */
    int n_reps ;
    pid_t pid = 0 ;
    int line_num = 0 ;
    ALIGNED_t text[ALIGNED_T_SIZE (LE_MAX_MSG_LENGTH)] ;
    time_t time ;

    int day ;
    int hr ;
    int min ;
    int month ;
    int sec ;
    int year ;
    int yr ;
    static char disp_date[9] ; /* static so will be nulled-out            */
    static char disp_time[9] ; /* static so will be nulled-out            */

    static int cur_day ;       /* static to survive from call to call     */
    static int cur_month ;
    static int cur_yr ;
    char msg_types[16];

    lemsg_p = (LE_message *) msg_p ;

    code = lemsg_p->code ;

    if (code & LE_CRITICAL_BIT) {

        msgcode_char = CRIT_MSGCODE_CHAR ;

        /*
         * Display Critical LE Message ...
         */
        critmsg_p = (LE_critical_message *) msg_p ;

        pid = (pid_t) critmsg_p->pid ;
        time = critmsg_p->time ;
        n_reps = critmsg_p->n_reps ;

        line_num = critmsg_p->line_num ;
        (void) strncpy(fname, critmsg_p->fname, LE_SOURCE_NAME_SIZE-1) ;
        fname[LE_SOURCE_NAME_SIZE-1] = '\0' ;

        /*
         * Retrieve the message text ... note that we must "back up"
         * the pointer because of the padding bytes ...
         */
        (void) strcpy((char *)text, critmsg_p->text) ;

        /*
         * Remove trailing line-returns from the message text ...
         */
        Remove_trailing_line_return((char *)text) ;
    }
    else {

        msgcode_char = NONCRIT_MSGCODE_CHAR ;

        time = lemsg_p->time ;
        n_reps = lemsg_p->n_reps ;

        /*
         * Retrieve the message text ... note that we must "back up"
         * the pointer because of the padding bytes ...
         */
        (void) strcpy((char *)text, lemsg_p->text) ;

        /*
         * Remove trailing line-returns from the message text ...
         */
        Remove_trailing_line_return((char *)text) ;
   }

    (void) unix_time((time_t *) &time,&year,&month,&day,&hr,&min,&sec) ;
    yr = year % 100 ;

    if ((cur_day != day) || (cur_month != month) || (cur_yr != yr)) {
        /*
         * Display the day/month/year information only if it has changed ...
         */
        cur_day = day ; cur_month = month ; cur_yr = yr ;
        (void) sprintf(disp_date,
                       "%02d/%02d/%02d",
                       cur_month,cur_day,cur_yr) ;
        (void) printf("%s\n", disp_date) ;
    }

    (void) sprintf(disp_time,
                   "%02d:%02d:%02d",
                   hr,min,sec) ;

    msg_types[0] = '\0';
    if (code & GL_ERROR_BIT)
	strcat (msg_types, "E");
    if (code & GL_STATUS_BIT)
	strcat (msg_types, "S");
    if (code & GL_GLOBAL_BIT)
	strcat (msg_types, "G");
    if ((code & LE_VL3) == LE_VL3)
	strcat (msg_types, "3");
    else {
	if ((code & LE_VL1) == LE_VL1)
	    strcat (msg_types, "1");
	if ((code & LE_VL2) == LE_VL2)
	    strcat (msg_types, "2");
    }

    /*  */
    printf ("%s %2s %s", disp_time, msg_types, (char *)text) ;

    if (n_reps != 1) {
        printf (" (n_reps: %d)", n_reps) ;
    }

    if (code & LE_CRITICAL_BIT)
        printf (" -%s:%d\n", fname, line_num);
    else
	printf ("\n");

/*END of Print_lemsg()*/
}

/**************************************************************************

    Removes trailing line return character in string "in_string".

 **************************************************************************/

static void Remove_trailing_line_return (char *in_string) {
    char *ptr;

    if (in_string == NULL)
        return;

    ptr = in_string + strlen (in_string) - 1;
    while (ptr >= in_string) {
        if (*ptr == '\n')
	    *ptr = '\0';
	else
	    break;
        ptr--;
    }

    return;
}



/**************************************************************************
 Description: Print messages in LE LB file
       Input: Pointer to Process Array entry
              number of messages to be displayed (zero means all messages)
      Output: messages are displayed
     Returns: 0 upon success; otherwise, -1
       Notes:
 **************************************************************************/
static int
Print_static_le(Procary_entry *proc_p, int num_msgs)
{
    LB_info info ;
    LB_id_t lb_id ;
    int offset ;
    int retval = 0 ;
    int retval2 ;


    /*
     * num_msgs:
     *       0 - read all available messages
     *     > 0 - read that many of the "latest" messages
     *
     * Determine the seek offset and where to seek from ...
     */
    if (num_msgs == 0) {
        (void) fprintf(stderr,
                       "All LE LB messages for %s (%s) ...\n",
                       proc_p->procname, proc_p->lbname) ;
        offset = 0 ;
        lb_id = LB_FIRST ;
    }
    else if (num_msgs > 0) {
        (void) fprintf(stderr,
                       "Last %d LE LB messages for %s (%s) ...\n",
                       num_msgs, proc_p->procname, proc_p->lbname) ;
        offset = (-1) * (num_msgs - 1) ;
        lb_id = LB_LATEST ;
    }
    else {
        (void) fprintf(stderr, "num_msgs %d is negative!\n", num_msgs) ;
        return(-1) ;
    }

    /*
     * We seek to the earliest message requested (may have
     * expired ... this is okay) ... then read from there ...
     */
    retval = LB_seek(proc_p->lbd,
                     offset,
                     lb_id,
                     &info) ;
    if (retval != LB_SUCCESS) {
        (void) fprintf(stderr,
                       "LB_seek(%s) returned %d\n",
                       proc_p->lbname, retval) ;
        return(-1) ;
    }

    /*
     * Loop to read from the LE LB file ...
     */
    do {

        retval2 = LB_read(proc_p->lbd,
                         Le_buf,
                         LE_MAX_MSG_LENGTH,
                         LB_NEXT) ;
        if (retval2 <= 0) {

            if (retval2 == LB_TO_COME) {
                /*
                 * We're through ...
                 */
                retval = 0 ;
                break ;
            }
            else if (retval2 != LB_EXPIRED) {
                (void) fprintf(stderr,
                               "LB_read(%s) returned %d\n",
                               proc_p->lbname, retval) ;
                retval = retval2 ;
                break ;
            }
        }
        else {
            Print_lemsg((LE_message *) Le_buf,
                        (char *) proc_p->procname) ;
        }

    } while ((retval2 == LB_EXPIRED) || (retval2 >= 0)) ;

    return(retval) ;

/*END of Print_static_le()*/
}



/**************************************************************************
 Description: Read the next LE LB message for the specified process
       Input: pointer to process array entry
              seek flag (set if we must LB_seek() before reading)
              offset value
      Output: none
     Returns: 0 upon success; otherwise, -1
       Notes:
 **************************************************************************/
static int
Read_next_msg(Procary_entry *proc_p,
              unsigned char seek_flag,
              int offset)
{
    LB_info info ;
    LB_id_t lb_id ;
    int retval ;

    /*
     * Assume we have no message to print ...
     */
    proc_p->msg_to_print = 0 ;

    if (seek_flag) {

        if (offset == 0) {
            lb_id = LB_FIRST ;
        }
        else {
            lb_id = LB_LATEST ;
        }

        /*
         * We seek to the earliest message requested (may have
         * expired ... this is okay) ... then read from there ...
         */
        retval = LB_seek(proc_p->lbd,
                         offset,
                         lb_id,
                         &info) ;
        if (retval != LB_SUCCESS) {
            (void) fprintf(stderr,
                           "LB_seek(%s) returned %d\n",
                           proc_p->lbname, retval) ;
            return(-1) ;
        }

    } /*endif we need to LB_seek()*/

    
    /*
     * Loop to read from the LE LB file ...
     * we're through either when we hit the end of the LB
     * (LB_TO_COME) or when we read a message ...
     */
    do {

        retval = LB_read(proc_p->lbd,
                         proc_p->curmsg,
                         LE_MAX_MSG_LENGTH,
                         LB_NEXT) ;
        if (retval <= 0) {
            if (retval == LB_TO_COME) {
                /*
                 * We're through ...
                 */
                break ;
            }
            else if (retval != LB_EXPIRED) {
                (void) fprintf(stderr,
                               "LB_read(%s) returned %d\n",
                               proc_p->lbname, retval) ;
            }
        }
        else {
            proc_p->msg_to_print = 1 ;
            break ;
        }

    } while ((retval == LB_EXPIRED) || (retval >= 0)) ;

    return(retval) ;

/*END of Read_next_msg()*/
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
Read_options(int argc,
             char **argv)
{
    char *le_direv ;
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */
    int retval = 0 ;
    int retval2 ;

    Le_critical_evtcd = 0 ;
    Poll_rate_msec = DFLT_DYN_POLL_MSEC ;

    (void) strncpy(Prog_name, argv[0], NAME_LEN);
    Prog_name[NAME_LEN - 1] = '\0';
    Le_file_name = NULL;
    Pid = 0;
    New_vl = 0;

    err = 0;
    while ((c = getopt (argc, argv, "v:c:d:f:hm:n:p:s:")) != EOF) {
    switch (c) {

	case 'v':
	    if (sscanf (optarg, "%d%*c%d", &Pid, &New_vl) != 2) {
                fprintf (stderr, "Incorrect -v option\n");
		err = 1;
	    }
 	    return (0);

       case 'c':
            if (Monitor_mode == MON_MODE_STATIC) {
                err = 1 ;
                break ;
            }

            if (strspn(optarg,"0123456789") != strlen(optarg)) {
                (void) fprintf(stderr,
                               "strspn() indicates event code NAN!\n") ;
                err = 1 ;
                break ;
            }

            retval2 = sscanf(optarg, "%d", &Le_critical_evtcd) ;
            if (retval2 == EOF) {
                (void) fprintf(stderr,
                               "sscanf() failed to read Le_critical_evtcd\n") ;
                err = 1;
                break;
            }
 
            if (Le_critical_evtcd <= EN_POST_MAX_RESERVED_EVTCD) {
                (void) fprintf(stderr,
                               "specified event code reserved: %d\n",
                               Le_critical_evtcd);
                err = 1;
            }
 
            break;

        case 'd':
            if (strlen(optarg) <= MAXPATHLEN) {
                retval2 = sscanf(optarg, "%s", Le_dir) ;
                if (retval2 == EOF) {
                    (void) fprintf (stderr,
                                    "sscanf() failed to read Le_dir\n") ;
                    err = 1;
                }
            }
            else {
                (void) fprintf(stderr,
                               "specified LE directory too long (%d)!\n",
                               (int)strlen(optarg)) ;    
                err = 1;
            }

            break;

        case 'f':
            if (strlen(optarg) <= NAME_LEN) {
                retval2 = sscanf(optarg, "%s", Proc_fname) ;
                if (retval2 == EOF) {
                    (void) fprintf(stderr,
                                   "sscanf() failed to read Proc_fname\n") ;
                    err = 1;
                }
                Proc_fname[NAME_LEN] = '\0' ;
            }
            else {
                (void) fprintf(stderr,
                               "Processes filename longer than %d chars: %s\n",
                               NAME_LEN, optarg) ;
                err = 1;
            }
            break;

        case 'm':
            if (strlen(optarg) <= NAME_LEN) {
                retval2 = sscanf(optarg, "%s", Codemap_fname) ;
                if (retval2 == EOF) {
                    (void) fprintf(stderr,
                                   "sscanf() failed to read Codemap_fname\n") ;
                    err = 1;
                }
                Codemap_fname[NAME_LEN] = '\0' ;
            }
            else {
                (void) fprintf(stderr,
                               "Msg code map filename longer than %d chars: %s\n",
                               NAME_LEN, optarg) ;
                err = 1;
            }
/***
 *** DEBUG
 ***/
(void) fprintf(stderr,"SORRY ... -m option not implemented yet!\n") ;
/***
 *** DEBUG
 ***/
            break;

        case 'n':
            if (strlen(optarg) <= NAME_LEN) {
		Le_file_name = optarg;
            }
            else {
                (void) fprintf(stderr,
                               "Msg code map filename longer than %d chars: %s\n",
                               NAME_LEN, optarg) ;
                err = 1;
            }
            break;

        case 'p':
            if (strspn(optarg,"0123456789") != strlen(optarg)) {
                (void) fprintf(stderr,
                               "strspn() indicates dynamic poll rate NAN!\n") ;
                err = 1 ;
                break ;
            }
            retval2 = sscanf(optarg, "%d", &Poll_rate_msec) ;
            if (retval2 == EOF) {
                (void) fprintf(stderr,
                               "sscanf() failed to read Poll_rate_msec\n") ;
                err = 1;
                break;
            }
            break;

        case 's':
            if (Le_critical_evtcd > 0) {
                err = 1 ;
                break ;
            }

            Monitor_mode = MON_MODE_STATIC ;

            if (strspn(optarg,"0123456789") != strlen(optarg)) {
                (void) fprintf (stderr,
                                "strspn() indicates static history length NAN!\n") ;
                err = 1 ;
                break ;
            }

            retval2 = sscanf(optarg, "%d", &History_length) ;
            if (retval2 == EOF) {
                (void) fprintf (stderr,
                                "sscanf() failed to read History_length\n") ;
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


    if (strlen(Le_dir) == 0) {
        /*
         * Use LE_DIR_EVENT environment variable, if set ...
         * otherwise, use current directory ...
         */
        le_direv = getenv("LE_DIR_EVENT") ;
        if (le_direv != NULL) {
            /*
             * Automatic variables ...
             * LE_DIR_EVENT: path + ':' + ten-digit event code + null
             */
            char env_string[MAXPATHLEN+1+10+1] ;
            char *retval_p ;

            (void) memset(env_string, 0, MAXPATHLEN+1+10+1) ;
            (void) strncpy(env_string, le_direv, MAXPATHLEN+1+10) ;

            /*
             * strtok() alters the string being tokenized ...
             */
            retval_p = strtok(env_string, ":") ;
            if (retval_p != NULL) {
                (void) strncpy(Le_dir,retval_p,MAXPATHLEN) ;
            }
            else {
                /*
                 * Use current directory ...
                 */
                (void) fprintf(stderr,
                               "BADLY-FORMED LE_DIR_EVENT %s ... using current directory!\n",
                               le_direv) ;
                (void) strcpy(Le_dir,".") ;
            }

        }
        else {
            /*
             * Use current directory ...
             */
            (void) strcpy(Le_dir,".") ;
        }

    } /* endif LE directory not specified at command-line */


    if (Le_critical_evtcd > 0) {
        if ((argc - optind) >= 1) {
            /*
             * Do not accept a command-line process name if LE Critical
             * Message event being registered for ...
             */
            err = 1 ;
        }
    }
    else {
        /*
         * If no processes file was specified for input, and if we're
         * not registering for the LE Critical Message event ... ensure that
         * one or more process names were specified at the command-line ...
         */
        if (strlen(Proc_fname) == 0 && Le_file_name == NULL) {
            if ((argc - optind) < 1) {
                err = 1 ;
            }
        }
    }


    if (err == 1) {

        (void) printf ("Usage: %s [options] [proc1 proc2 ... procN]\n",
                       Prog_name);
        (void) printf ("\tOptions (numbers are base-10):\n");
        (void) printf ("\t\t-v  pid,vl (change process pid's verbose level to vl)\n");
        (void) printf ("\t\t-d  le_dir [default: LE_DIR_EVENT or cwd]\n");
        (void) printf ("\t\t-n  le_file_name [default: none]\n");
        (void) printf ("\t\t-p  poll_msec (dynamic poll rate [default: %d])\n", Poll_rate_msec) ;
        (void) printf ("\t\t-s  length (static display of length messages\n") ;
        (void) printf ("\t\t\t [length of 0 implies all available messages])\n") ;
        (void) printf ("\n");
        (void) printf ("\t\tNOTE: -s, and -p options are exclusive!\n");
        (void) printf ("\t\tNOTE: for -s option, only first process name will be considered\n");
        (void) printf ("\n");

        retval = -1 ;
    }

    return(retval);

/*END of Read_options()*/
}



/**************************************************************************
 Description: Read list of processes from the Processes File
       Input: void
      Output: none
     Returns: 0 upon success; otherwise, -1
       Notes:
 **************************************************************************/
static int
Read_processes_file(void)
{
    register int i ;
    Procary_entry *proc_p ;
    int retval ;


    (void) CS_cfg_name(Proc_fname) ;            
    CS_control(CS_COMMENT | '#') ;
    CS_error ((void (*)()) printf) ;

    /*
     * We need to first count the number of processes listed
     * in the file ...
     */
    if (( CS_entry("Appsw_proc_list", 0, 0, NULL) < 0 )
                           ||
        ( CS_level(CS_DOWN_LEVEL) < 0 )) {
        return(-1) ;
    }

    Numprocs = 0 ;
    do {
        retval = CS_entry(CS_THIS_LINE, 0, 0, NULL) ;
        if (retval < 0) {
            return(-1) ;
        }
        ++Numprocs ;
    } while (CS_entry (CS_NEXT_LINE, 0, 0, NULL) >= 0) ;

    /*
     * Now that we know how many processes we are being
     * asked to monitor, allocate memory as required and
     * then reread the Processes File ...
     */
    errno = 0 ;
    Procary_p = calloc((size_t) Numprocs, sizeof(Procary_entry)) ;
    if (Procary_p == NULL) {
        (void) fprintf(stderr,"calloc of Process Array failed: %d", errno) ;
        exit(EXIT_FAILURE) ;
    }

    if ( (CS_level(CS_TOP_LEVEL) < 0)) {
        return(-1) ;
    }

    if (( CS_entry("Appsw_proc_list", 0, 0, NULL) < 0 )
                           ||
        ( CS_level(CS_DOWN_LEVEL) < 0 )) {
        return(-1) ;
    }

    i = 0 ;
    do {

        if (i >= Numprocs) {
            (void) fprintf(stderr,
                           "PROBLEM: i (%d) exceeds Numprocs (%d) while reading file\n",
                           i, Numprocs) ;
            return(-1) ;
        }

        proc_p = (Procary_p + i) ;

        retval = CS_entry(CS_THIS_LINE, 0, PROCNAME_SIZE-1, proc_p->procname) ;
        if (retval < 0) {
            return(-1) ;
        }

        proc_p->procname[PROCNAME_SIZE-1] = '\0' ;

        ++i ;

    } while (CS_entry (CS_NEXT_LINE, 0, 0, NULL) >= 0) ;

    CS_control(CS_CLOSE) ;
    CS_control(CS_DELETE) ;

    return(0) ;

/*END of Read_processes_file()*/
}
