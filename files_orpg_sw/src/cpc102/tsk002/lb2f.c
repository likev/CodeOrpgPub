/**************************************************************************

      Module: lb2f.c

 Description:

	Copies one or more messages from the linear buffer file into one or
    more regular files.

 Assumptions:
	An appropriate input LB file exists.


**************************************************************************/

/*
 * RCS info
 * $Author: dodson $
 * $Locker:  $
 * $Date: 1998/08/29 14:56:17 $
 * $Id: lb2f.c,v 1.18 1998/08/29 14:56:17 dodson Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <orpg.h>
#include <prod_gen_msg.h>
#include <infr.h>
 

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define MAX_IO_ATTEMPTS 5

/*
 * Static Globals
 */
static char Prog_name[ORPG_PROGNAME_SIZ];

/*
 * Static Function Prototypes
 */
static int Build_outfile_name(const char *basename, LB_id_t lb_id,
                   char *fname, size_t fname_size) ;
static int Write_to_file(char *buf, size_t buf_size,
                         int fd, unsigned char prod_hdr_flag) ;
static int Read_options(int argc, char **argv,
                        LB_id_t *lb_id, char *lb_name,
                        char *file_name, unsigned char *prod_hdr_flag,
                        unsigned char *auto_flag, unsigned char *verbose_flag) ;



/**************************************************************************
 Description: Get the command line arguments; open the linear buffer file
              and the regular file; copy the data.
       Input: argc, argv
      Output:
     Returns: none
     Globals: Prog_name
       Notes:
 **************************************************************************/
int main (int argc, char **argv)
{
    unsigned char auto_flag ;  /* if set, we continuously (automatically) */
                               /* convert LB messages to disk files as    */
                               /* are written to the LB file              */
    char *buf_p = NULL ;
    int buf_size ;
    int f_flags ;
    mode_t f_mode ;
    int fd;
    char file_name [ORPG_PATHNAME_SIZ];
    int lb_flags ;
    LB_id_t lb_id ;
    LB_info lb_info ;
    char lb_name[ORPG_PATHNAME_SIZ] ;
    int lbd ;
    unsigned char prod_hdr_flag ;
                               /* if set, we include the ORPG product hdr */
                               /* when converting ... default: strip it   */
    int retval ;
    unsigned char verbose_flag ;


    f_mode = 0 ;

    lb_flags = LB_READ ;
    f_flags = O_RDWR | O_CREAT | O_TRUNC ;
    f_mode = S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP ;

    retval = Read_options(argc, argv, &lb_id, lb_name,
                          file_name, &prod_hdr_flag,
                          &auto_flag, &verbose_flag) ;
    if (retval != 0) {
        exit (EXIT_FAILURE);
    }


    /*
     * Try opening the Linear Buffer ...
     */
    lbd = LB_open(lb_name, lb_flags, NULL);
    if (lbd < 0) {
        (void) printf ("failed in opening LB %s (ret = %d) - %s\n", 
                       lb_name, lbd, Prog_name);
        exit(EXIT_FAILURE) ;
    }


    if (! auto_flag) {
        /*
         * Try opening the Regular File ...
         */
        fd = open(file_name,f_flags,f_mode) ;
        if (fd < 0) {
            (void) printf ("failed in opening regular file %s (ret = %d) - %s\n", 
                           file_name, fd, Prog_name);
            exit(EXIT_FAILURE) ;
        }

        if (lb_id == LB_LATEST) {

            /*
             * We need to seek to the latest message so that we can read it ...
             */
            (void) LB_seek(lbd, 0, LB_LATEST, &lb_info) ;
            lb_id = lb_info.id ;
        }

        buf_size = LB_read(lbd, &buf_p, LB_ALLOC_BUF, lb_id) ;
        if (buf_size < 0) {
            (void) fprintf(stderr, "LB_read returned %d\n", buf_size) ;
            exit(EXIT_FAILURE) ;
        }

        if (verbose_flag) {
            (void) fprintf(stdout, "Read %d bytes from msg %d ... ",
                           buf_size, (int) LB_previous_msgid(lbd)) ;
        }

        retval = Write_to_file(buf_p, (size_t) buf_size, fd, prod_hdr_flag) ;
        if (retval < 0) {
            (void) fprintf(stderr,"Write_to_file failed: %d\n", retval);
            exit(EXIT_FAILURE) ;
        }

        (void) close(fd) ;

        if (verbose_flag) {
            (void) fprintf(stdout, "wrote %d bytes to %s\n",
                           retval, file_name) ;
        }
        else {
            (void) fprintf(stdout, "%s\n", file_name) ;
        }

        if (buf_p != NULL) {
            free(buf_p) ;
            buf_p = NULL ;
        }

    }
    else {
        /*
         * Automatic ("continuous") conversion of LB messages ...
         */

        /*
         * Seek to the beginning of the file (unless a particular ID was
         * specified) ...
         */
        if (lb_id != LB_LATEST) {

            /*
             * We need to seek to the latest message so that we can read it ...
             */
            (void) LB_seek(lbd, 0, lb_id, &lb_info) ;
            if (verbose_flag) {
                (void) fprintf(stderr, "seeking to msg %d ...\n",
                               (int) lb_id) ;
            }
        }
        else {
            (void) LB_seek(lbd, 0, LB_FIRST, &lb_info) ;
            if (verbose_flag) {
                (void) fprintf(stderr, "seeking to first msg (%d)  ...\n",
                               (int) lb_info.id) ;
            }
        }

        lb_id = LB_NEXT ;

        retval = LB_set_poll(lbd, 300, 10000) ;
        if (retval != LB_SUCCESS) {
            (void) fprintf(stderr, "LB_set_poll returned %d\n", retval) ;
            exit(EXIT_FAILURE) ;
        }

        for (;;) {
            buf_size = LB_read(lbd, &buf_p, LB_ALLOC_BUF, lb_id) ;
            if ((buf_size < 0)
                          &&
                (buf_size != LB_TO_COME)
                          &&
                (buf_size != LB_EXPIRED)) {
                (void) fprintf(stderr, "LB_read returned %d\n", buf_size) ;
                exit(EXIT_FAILURE) ;
            }
            else if ((buf_size == LB_EXPIRED) || (buf_size == LB_TO_COME)) {
                if (buf_p != NULL) {
                    free(buf_p) ;
                    buf_p = NULL ;
                }
                sleep(10) ;
                continue ;
            }

            retval = Build_outfile_name((const char *) lb_name,
                                        LB_previous_msgid(lbd),
                                        file_name, sizeof(file_name)) ;
            if (retval < 0) {
                (void) fprintf(stderr, "Build_outfile_name %s.%d failed: %d\n",
                               lb_name, (int) LB_previous_msgid(lbd), retval) ;
                exit(EXIT_FAILURE) ;
            }

            if (verbose_flag) {
                (void) fprintf(stdout, "Read %d bytes from msg %d ... ",
                               buf_size, (int) LB_previous_msgid(lbd)) ;
            }

            fd = open(file_name,f_flags,f_mode) ;
            if (fd < 0) {
                (void) printf ("\nfailed in opening regular file %s (ret = %d)\n", 
                               file_name, fd);
                exit(EXIT_FAILURE) ;
            }

            retval = Write_to_file(buf_p, (size_t) buf_size, fd, prod_hdr_flag) ;
            if (retval < 0) {
                (void) fprintf(stderr,"\nWrite_to_file failed: %d\n", retval);
                exit(EXIT_FAILURE) ;

            }

            if (verbose_flag) {
                (void) fprintf(stdout, "wrote %d bytes to %s\n",
                               retval, file_name) ;
            }
            else {
                (void) fprintf(stdout, "%s\n", file_name) ;
            }

            (void) close(fd) ;

            if (buf_p != NULL) {
                free(buf_p) ;
                buf_p = NULL ;
            }


        } /*end infinite loop ...*/

    } /*endelse we're running in automatic/continuous mode */


   exit (EXIT_SUCCESS);

/*END of main()*/
}



/**************************************************************************
 Description: Build an output file name
       Input:
      Output:
     Returns: 0 upon success; otherwise, -1
     Globals: Prog_name
       Notes:
 **************************************************************************/
#define ID_STRING_LEN 24
#define ID_STRING_SIZ ((ID_STRING_LEN) + 1)
static int
Build_outfile_name(const char *basename, LB_id_t lb_id,
                   char *fname, size_t fname_size)
{
    char id_string[ID_STRING_SIZ] ;

    (void) memset(id_string, 0, sizeof(id_string)) ;
    (void) sprintf(id_string, "%d", (int) lb_id) ;
    id_string[ID_STRING_LEN] = '\0' ;

    (void) memset(fname, 0, fname_size) ;

    if ((strlen(basename) + 1 + strlen(id_string)) < fname_size-1) {
        (void) sprintf(fname, "%s.%s", basename, id_string) ;
    }
    else {
        (void) fprintf(stderr,"output filename too long: %s.%s (more than %d chars)\n",
                       basename, id_string, fname_size-1) ;
        return(-1) ;
    }

    return(0) ;

/*END of Build_outfile_name()*/
}



/**************************************************************************
 Description: Copy a message from the linear buffer file to the regular
              file
       Input: linear buffer file descriptor
              regular  file file descriptor
      Output:
     Returns: number of bytes written upon success; otherwise, -1
     Globals: Prog_name
       Notes:
 **************************************************************************/
static int
Write_to_file(char *buf_p, size_t buf_size, int fd, unsigned char prod_hdr_flag)
{
    int io_attempts ;
    ssize_t nleft ;
    ssize_t nwrite ;
    size_t phd_offset = 0 ;    /* ORPG Product Header offset (bytes)      */
    int retval = 0 ;


    /*
     * Write the linear buffer file data to the regular file ...
     * Default behavior is to NOT strip the ORPG Product Header before
     * writing the message to the file ...
     */
    if (prod_hdr_flag) {
        phd_offset = sizeof(Prod_header) ;
    }

    nwrite = 0 ;
    nleft = (ssize_t) (buf_size - phd_offset) ;
    io_attempts = 0 ;

    while ((nleft > 0) && (io_attempts < MAX_IO_ATTEMPTS)) {

        retval = (int) write(fd, &buf_p[phd_offset+nwrite], nleft) ;
        if (retval == -1) {
            (void) perror(" write ") ;
            free(buf_p) ;
            buf_p = NULL ;
            return(-1) ;
        }
        else {
            nwrite = nwrite + (ssize_t) retval ;
            nleft = nleft - (ssize_t) retval ;
            ++io_attempts ;
        }

    } /*endwhile we still have bytes to write*/

    if (buf_p != NULL) {
        free(buf_p) ;
        buf_p = NULL ;
    }

   return(nwrite) ;

/*END of Write_to_file()*/
}



/**************************************************************************
 Description: Read the command-line options and initize several global
              variables.
       Input: argc, argv,
              pointer to storage for linear buffer message id
              pointer to storage for linear buffer file name
              pointer to storage for  regular file file name
              various pointers
      Output: none
     Returns: 0 upon success; otherwise, -1
     Globals: Prog_name
       Notes:
 **************************************************************************/
static int Read_options(int argc, char **argv,
                        LB_id_t *lb_id, char *lb_name,
                        char *file_name, unsigned char *prod_hdr_flag,
                        unsigned char *auto_flag, unsigned char *verbose_flag)
{
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */
    int retval = 0 ;

    (void) strncpy(Prog_name, MISC_string_basename(argv[0]), ORPG_PROGNAME_LEN);
    Prog_name[ORPG_PROGNAME_LEN] = '\0';


    /*
     * Defaults ...
     */
    *lb_id = LB_LATEST ;

    lb_name[0] = '\0';
    file_name[0] = '\0';

    *prod_hdr_flag = 0 ;
    *verbose_flag = 0 ;
    *auto_flag = 0 ;

    err = 0;
    while ((c = getopt (argc, argv, "f:hi:l:pv")) != EOF) {
    switch (c) {

        case 'f':
            retval = sscanf(optarg, "%s", file_name) ;
            if (retval == EOF) {
                (void) fprintf(stderr,
                               "sscanf() failed to read file name\n") ;
                 err = 1 ;
             }
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

        case 'p':
            *prod_hdr_flag = 1 ;
             break;

        case 'v':
            *verbose_flag = 1 ;
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

    lb_name[ORPG_PATHNAME_LEN] = 0 ;
    file_name[ORPG_PATHNAME_LEN] = 0 ;

    if ((strlen(lb_name) == 0)
                           ||
        (strlen(file_name) == 0)) {
        err = 1 ;
    }

   if (! strncmp(file_name,"auto",4)) {
      *auto_flag = 1 ;
   }

   if (err == 1) {

      (void) printf ("\nUsage: %s [options] -l LB_file -f regular_file\n",
                     Prog_name);
      (void) printf ("\n\tOptions:\n\n");
      (void) printf ("\t\t-h  print usage information\n");
      (void) printf ("\t\t-i  message id [default: latest message]\n");
      (void) printf ("\t\t-p  strip ORPG prod hdr [default: include]\n");
      (void) printf ("\t\t-v  run in verbose mode [default: quiet]\n");
      (void) printf ("\n");
      (void) printf ("\tNOTE: Specifying \"-f auto\" results in\n") ;
      (void) printf ("\tautomatic/continuous reading of LB messages.\n");
      (void) printf ("\tIn this mode, the output files will be named with\n");
      (void) printf ("\tbasename of LB file with extension that matches\n");
      (void) printf ("\tcorresponding LB message ID.  Also, if no message\n");
      (void) printf ("\tID is specified in this mode, the tool will begin\n");
      (void) printf ("\treading messages starting with the first (oldest)\n");
      (void) printf ("\tmessage in the LB file.\n");
      (void) printf ("\n");
      (void) printf ("\n");

      return(-1) ;
   }


   return(0);

/*END of Read_options()*/
}
