/**************************************************************************

      Module: f2lb.c

 Description:

	Copies the contents of the regular file into an existing linear buffer
    file.

 Assumptions:
	An appropriate output LB file exists.

       Notes:

	The challenge of working with a legacy WAREHOSE.DAT file
	motivated me to add f2lb options to specify an offset at which to
	start reading from the file and to specify the number of bytes
	to read from the file (rather than reading the entire contents of
	the file).  At this point in time, this mod is working well enough
	to permit me to extract the Combined Attributes Table files from
	the WAREHOSE.DAT file.

**************************************************************************/

/*
 * RCS info
 * $Author: dodson $
 * $Locker:  $
 * $Date: 1998/08/28 18:29:25 $
 * $Id: f2lb.c,v 1.1 1998/08/28 18:29:25 dodson Exp $
 * $Revision: 1.1 $
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

#include <prod_gen_msg.h>
#include <infr.h>
 

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define NAME_LEN 128
#define RPG_PROD_CHAR 'f'           /* final product          */
#define INT_PROD_CHAR 'i'           /* intermediate product   */
#define MAX_IO_ATTEMPTS 5

/*
 * Static Globals
 */
static char Prog_name[NAME_LEN+1];

/*
 * Static Function Prototypes
 */
static int From_file_to_lb(int fd, int lbd,
                           LB_id_t lb_id, int prod_code,
                           int prod_type, int prod_hdr_flag,
                           off_t read_offset_bytes, int bytes_to_read) ;
static int Read_options(int argc, char **argv,
                        LB_id_t *lb_id, char *lb_name,
                        char *file_name, int *prod_code,
                        int *prod_type, int *prod_hdr_flag,
                        int *bytes_to_read, off_t *read_offset_bytes);



/**************************************************************************
 Description: Get the command line arguments; open the linear buffer file
              and the regular file; copy the data.
       Input: argc, argv
      Output:
     Returns: none
     Globals: Prog_name
       Notes:
 **************************************************************************/
int
main (int argc, char **argv)
{
   int f_flags ;
   mode_t f_mode ;
   int fd;
   char file_name [NAME_LEN];
   int lb_flags ;
   LB_id_t lb_id ;
   char lb_name[NAME_LEN] ;
   int lbd ;
   int num_bytes ;      /* specific # of bytes to read from file */
   off_t offset_bytes ;   /* offset at which to start reading from file */
   int prod_code ;
   int prod_hdr_flag ;
   int prod_type ;
   int retval ;


   (void) strncpy(Prog_name, MISC_string_basename(argv[0]), NAME_LEN);
   Prog_name[NAME_LEN - 1] = '\0';

   f_mode = 0 ;

   lb_flags = LB_WRITE ;
   f_flags = O_RDONLY ;

   retval = Read_options(argc,
                         argv,
                         &lb_id,
                         lb_name,
                         file_name,
                         &prod_code,
                         &prod_type,
                         &prod_hdr_flag,
                         &num_bytes,
                         &offset_bytes) ;
   if (retval != 0) {
      exit (-1);
   }


   /*
    * Try opening the Linear Buffer ...
    */
   lbd = LB_open(lb_name, lb_flags, NULL);
   if (lbd < 0) {
      (void) printf ("failed in opening LB %s (ret = %d) - %s\n", 
                     lb_name, lbd, Prog_name);
      exit(1) ;
   }


   /*
    * Try opening the Regular File ...
    */
   fd = open(file_name,f_flags,f_mode) ;
   if (fd < 0) {
      (void) printf ("failed in opening regular file %s (ret = %d) - %s\n", 
                     file_name, fd, Prog_name);
      exit(1) ;
   }

   retval = From_file_to_lb(fd,
                            lbd,
                            lb_id,
                            prod_code,
                            prod_type,
                            prod_hdr_flag,
                            offset_bytes,
                            num_bytes) ;
   if (retval < 0) {
      (void) printf("conversion failed: %d - %s\n",
                    retval,Prog_name);
   }

   exit (0);

/*END of main()*/
}



/**************************************************************************
 Description: Copy data from the regular file into a single message in the
              linear buffer file.
       Input: regular  file file descriptor
              linear buffer file descriptor
              linear buffer message id
              product code
              product type
              product header flag
              offset at which to start reading from the file (bytes)
              number of bytes to read from the file
      Output:
     Returns: 0 upon success; otherwise, -1
     Globals: Prog_name
       Notes:
 **************************************************************************/
static int
From_file_to_lb(int fd, int lbd,
                LB_id_t lb_id, int prod_code,
                int prod_type, int prod_hdr_flag,
                off_t read_offset_bytes, int bytes_to_read)
{
   size_t b_offset ;
   char *buf ;
   off_t f_size ;
   int io_attempts ;
   int lbret ;
   ssize_t nleft ;
   ssize_t nread ;
   Prod_header *phead ;
   int retval ;


   /*
    * Do we need to prepend an OSRPG Product Header to the data?
    */
   if (prod_hdr_flag) {

      b_offset = sizeof(Prod_header) ;

   }
   else {
      b_offset = 0 ;
   }


   f_size = lseek(fd, 0, SEEK_END) ;
   if (f_size == (off_t) -1) {
      (void) perror("lseek SEEK_END ") ;
      return(-1) ;
   }

   retval = (int) lseek(fd, read_offset_bytes, SEEK_SET) ; 
   if (retval != (int) read_offset_bytes) {
      (void) perror("lseek SEEK_SET ") ;
      return(-1) ;
   }


   /*
    * Allocate memory sufficient to hold contents of input file plus
    * the OSRPG Product Header, if required ...
    */

   if (bytes_to_read) {
      nleft = (ssize_t) bytes_to_read ;
   }
   else{
      nleft = (ssize_t) f_size ;
   }

   buf = calloc((size_t) 1, (size_t) nleft + b_offset) ;
   if (buf == NULL) {
      (void) perror("calloc ") ;
      return(-1) ;
   }

   if (prod_hdr_flag) {

      phead = (Prod_header *) buf ;

      phead->g.prod_id = prod_code ;

   }

   /*
    * Read the data from the regular file ...
    */

   nread = 0 ;
   io_attempts = 0 ;

   while ((nleft > 0) && (io_attempts < MAX_IO_ATTEMPTS)) {

      retval = read(fd, &buf[b_offset + nread], nleft) ;
      if (retval <= 0) {
         (void) perror("read ") ;
         free(buf) ;
         return(-1) ;
      }
      else {

         nread = nread + (ssize_t) retval ;
         nleft = nleft - (ssize_t) retval ;
         ++io_attempts ;

      }

   }


   lbret = LB_write(lbd, buf, retval + b_offset, lb_id) ;
   if (lbret <= 0) {
      (void) printf("lbret: %d - %s\n", lbret,Prog_name) ;
      retval = -1 ;
   }

   free(buf) ;

   return(0) ;

/*END of From_file_to_lb()*/
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
static int
Read_options(int argc, char **argv,
             LB_id_t *lb_id, char *lb_name,
             char *file_name, int *prod_code,
             int *prod_type, int *prod_hdr_flag,
             int *bytes_to_read, off_t *read_offset_bytes)
{
   extern char *optarg;    /* used by getopt */
   extern int optind;
   int c;                  /* used by getopt */
   int err;                /* error flag */
   int retval = 0 ;

   /*
    * Defaults ...
    */
   *lb_id = LB_ANY ;
   *bytes_to_read = 0 ;
   *read_offset_bytes = (off_t) 0 ;

   lb_name[0] = '\0';
   file_name[0] = '\0';

   *prod_hdr_flag = 0 ;
   *prod_code = 0 ;
   *prod_type = RPG_PROD_CHAR ;

   err = 0;
   while ((c = getopt (argc, argv, "f:hi:l:n:o:p:t:")) != EOF) {
   switch (c) {

      case 'f':
         (void) strncpy(file_name,optarg,NAME_LEN) ;
         break;

      case 'i':
         *lb_id = (LB_id_t) atoi(optarg) ;
         break;

      case 'l':
         (void) strncpy(lb_name,optarg,NAME_LEN) ;
         break;

      case 'n':
         *bytes_to_read = atoi(optarg) ;
         break;

      case 'o':
         *read_offset_bytes = (off_t) atoi(optarg) ;
         break;

      case 'p':
         *prod_hdr_flag = 1 ;
         *prod_code = atoi(optarg) ;
         break;

      case 't':
         (void) memcpy(prod_type,optarg,1) ;
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

     lb_name[NAME_LEN -1] = 0 ;
   file_name[NAME_LEN -1] = 0 ;

   if ((strlen(lb_name) == 0)
                   ||
       (strlen(file_name) == 0)) {
      err = 1 ;
   }

   if (err == 1) {

      (void) printf ("Usage: %s [options] -l LB_file -f regular file\n",
                     Prog_name);
      (void) printf ("      Options:\n");

      (void) printf ("      -i  message id [default: LB_ANY])\n");
      (void) printf ("      -n  number of bytes to read [default: entire file])\n");
      (void) printf ("      -o  offset bytes at which to start reading from file [default: 0])\n");
      (void) printf ("      -p  product code (prepend OSRPG Product Header [default: none])\n");
      (void) printf ("      -t  product type (final or intermediate [default: final])\n");
      (void) printf ("          enter \"-t f\" or \"-t i\" (without quotes)\n");

      (void) printf ("      -h  print usage information\n");
      (void) printf ("\n");

      retval = -1 ;
   }


   return(retval);

/*END of Read_options()*/
}
