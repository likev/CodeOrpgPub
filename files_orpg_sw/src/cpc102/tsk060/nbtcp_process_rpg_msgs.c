/********************************************************************************

       File: nbtcp_process_rpg_msgs.c
             This file contains all the routines used to process the RPG
             messages and to write the product messages as files to disk.

 ********************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/02/12 21:49:26 $
 * $Id: nbtcp_process_rpg_msgs.c,v 1.14 2014/02/12 21:49:26 steves Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */

#include <errno.h>

#include <time.h>        /* time (...) */
#include <sys/types.h>   /* creat(...)  */
#include <sys/stat.h>    /*      "      */ 
#include <fcntl.h>       /*      "      */
#include <stdio.h>       /* fflush(...) */
#include <netinet/in.h>  /* ntohl(...)
                            ntohs(...)  */
#include <dirent.h>      /* directory system calls */

#include <product.h>
#include <misc.h>
#include "nbtcp.h"

   /* process level globlas */

extern int    PIDS[MAXRPS];
extern char   MNE[MAXRPS][4];
extern int    Otrs_pending;
extern int    Otr_only;
extern int    Ignore_special_codes;
extern int    Remove_products;
extern int    WMO_AWIPS_header_added;


static char   Err_msg [1500];            /* error msg buffer */
static short  Gsmmode = -1;              /* GSM mode */
static char   Product_dir[FILENAME_MAX]; /* product directory where product
                                            files are written */
static char  *Icao = NULL;			 /* Site ID. */
static char  *Prod_dir_ptr = NULL;       /* product directory ptr */
static int   Product_save_flag = FALSE;
static int   New_connection_made = FALSE; /* new connection made flag */
static int   Product_filename_format;     /* product filename format */


typedef struct product_file {

   char name[FILENAME_MAX];		 /* Product file name. */

   time_t time_written;			 /* UTC time file was written. */
	
   struct product_file *next;		 /* Link to next file. */

} Product_file_t;

static Product_file_t *Head = NULL;
static Product_file_t *Tail = NULL;

static int Construct_filename (int msg_code, u_short elevation_number,
                               ushort vol_scan_number, short vol_scan_date,
                               int vol_scan_time, char *prodbuf, char *filepath);
static void Process_req_resp_message( char *msg_data );
static void Add_to_list( char *filename );
static void Remove_from_list();

/********************************************************************************

     Description: This routine processes the messages read from the RPG. The
                  the product msgs are witten to disk as files if the user
                  specified the products to be saved.

           Input: buf - The buffer containing the msg

          Output:

          Return: 0 on success; -1 on error


 ********************************************************************************/

int PROC_process_msg (char *buf, int msg_len)
{
   int         ret;
   msg_header  *msghead;
   static char filepathname[FILENAME_MAX];
   static int  prodfd = -1;
   static int  msg_code = -1;
   static int  prod_length = 0;
   static int  bytes_processed = 0;
   static int  msg_segmented = FALSE;
   static Graphic_product *product_msg=NULL;  /* see orpg/include/product.h */
   static char *temp = NULL;

   unsigned short prod_length_m, prod_length_l;
   void *free_buf_addr = NULL;
   int header_size = 0;

      /* remove any segmented product if the connection has been lost */

   if ((buf == NULL) && (msg_len == -1)) {
      if (prodfd != -1) {
         close (prodfd);
         unlink (filepathname);
         prodfd = -1;
         msg_segmented = FALSE;
         printf ("Removing segmented file \"%s\" due to lost connection\n", filepathname);
      }
      return (0);
   }

      /* Initialize header_size .... this will be set if a WMO/AWIPS header
         is added. */
   header_size = 0;

      /* initialize the variables if this is the first msg segment */

   if (msg_segmented == FALSE) {
      bytes_processed = 0;
      msg_segmented = TRUE;
      msghead = (msg_header *)buf;
      msg_code = ntohs(msghead->msgcode);
      prod_length_m = ntohs(msghead->length_of_msg_msw);
      prod_length_l = ntohs(msghead->length_of_msg_lsw);
      prod_length = (prod_length_m << 16) | prod_length_l;
      product_msg = (Graphic_product *)buf;

      if (Otr_only) {
         if (msg_code == GSM_CODE)
            printf("Receiving GSM");
         else if (msg_code == 8)
            printf("Receiving Product List");
         else if (msg_code == 3)
            Process_req_resp_message( buf );
         else 
            printf("Receiving Product #%d",msg_code);
      
         fflush(stdout);
      }

         /* check for a GSM */

      if ( msg_code == GSM_CODE ) {	/* GSM message? */
         gsm_header *gsmhead;
         short      mode;

            /* mode = 0 - maintenance */
            /*      = 1   clear air   */
            /*      = 2 - precip      */ /* could change RPS based on this */

         gsmhead = (gsm_header *)buf;
         mode = ntohs(gsmhead->gsm_mode);    	
 
         if (((Gsmmode != mode)    || 
               New_connection_made == TRUE) && mode != 0) {
                  New_connection_made = FALSE;
                  Gsmmode = mode;
                  MA_rps_required (TRUE);
         }
      }

         /* construct the product file name and obtain the file descriptor 
            if the product/msg should be saved to disk  */

      if ((Product_save_flag == TRUE)) {
            typedef union {
               int vol_time;
               short time_shorts[2];
            } v_time_t;

            v_time_t v_time;

            /* If the Ignore_special_codes flag is set, determine whether
               to write this file.  We need to call here so that prod_fd = -1.
               Otherwise, because of the logic in this module the file still
               gets written because prod_fd is declared static. */
         if ((!Ignore_special_codes)
                      ||
             (msg_code >= MIN_FINAL_PROD_CODE)){

            v_time.time_shorts[0] = product_msg->vol_time_ms;
            v_time.time_shorts[1] = product_msg->vol_time_ls;

            /* Add a WMO/AWIPS header, if needed. */
            if( (msg_code == GSM_CODE) 
                       ||
                (msg_code >= MIN_FINAL_PROD_CODE ) ){

               temp = WAH_add_header( buf, msg_len, msg_code );
               if( temp != NULL ){

                  buf = temp;
                  header_size = WMO_HEADER_SIZE;
                  free_buf_addr = (void *) buf;
                  temp = NULL;

               }

            }

            prodfd = Construct_filename (msg_code, ntohs(product_msg->elev_ind),
                                         ntohs (product_msg->vol_num), 
                                         ntohs (product_msg->vol_date), 
                                         ntohl (v_time.vol_time),
                                         buf, filepathname);

           Add_to_list( filepathname );

         }

      }

   }

      /* write the data to disk if a file is open */
   if (prodfd != -1) {

      /* If the Ignore_special_codes flag is set, determine whether
         to write this file. */
      if ((!Ignore_special_codes) 
                   ||
            (msg_code >= MIN_FINAL_PROD_CODE)) {
         if((ret = write(prodfd, buf, (msg_len+header_size))) < 0) {
             strcat (Err_msg, "File write error - file: \0");
             strcat (Err_msg, filepathname);
             MA_abort(Err_msg);
         }
      }
   }

      /* update the number of bytes processed */

   bytes_processed += msg_len;

      /* Display status dots  */

   if (Otr_only) {
      printf("...");
      fflush(stdout);
   }

      /* reset everything if all the msg segments for this
         product have been processed */

   if(bytes_processed >= prod_length) {
      if (Otr_only)
         printf ("Done.\n");
      if (prodfd != -1) {
         if((close(prodfd)) < 0){
             strcat (Err_msg, "File close error - file: \0");
             strcat (Err_msg, filepathname);
             MA_abort(Err_msg);
         }
         prodfd = -1;
         Remove_from_list();
      }
      msg_segmented = FALSE;
      if (Otr_only &&
          (msg_code == 3 || msg_code >= 16)) { /*PRR or a product    */
         Otrs_pending--;         /* Decrement the outstanding OTR counter      */
         if (Otrs_pending == 0){ /* Disconnect if no more outstanding requests */
            SOC_close_sockets ();
            printf("No more outstanding requests...disconnecting...\n");
            fflush(stdout);
            sleep(2);
         }
      }
   }

   /* Free any calloc'd memory. */
   if( (WMO_AWIPS_header_added)
              &&
       (free_buf_addr == (void *) buf) ){

      free( buf );
      free_buf_addr = NULL;

   }

   return (0);
}


/********************************************************************************

     Description: Set the product filename format

           Input: User selection from the menu

          Output:

          Return: 


 ********************************************************************************/

void PROC_publish_naming_convention (int selection)
{
   Product_filename_format = selection;
   return;
}


/********************************************************************************

     Description: This routine initializes the product directory

           Input: buf - The buffer containing the directory name

          Output:

          Return: 0 on success; -1 on error


 ********************************************************************************/

int PROC_init_product_dir (char *dir, int delete_files)
{
   DIR *dir_p = NULL;
   char filename[FILENAME_MAX];
   int ret = 0;

   strncpy (Product_dir, dir, FILENAME_MAX);

   if (strlen (Product_dir) < (FILENAME_MAX - 1))
       strcat (Product_dir, "/\0");
   else
       strcat (&Product_dir [FILENAME_MAX - 2], "/\0");

      /* If directory exists and delete_files is set, remove
         directory. */
   if (((dir_p = opendir (Product_dir)) != NULL)
                       &&
                (delete_files != 0)) {

      struct dirent *this_entry = NULL;

      while ((this_entry = readdir(dir_p)) != NULL ) {

         /* Ignore "." and ".." */
         if ((strcmp(this_entry->d_name, ".") != 0) 
                          &&
            (strcmp(this_entry->d_name, "..") != 0) )

            /* Prepend the directory name. */
            sprintf( &filename[0], "%s%s", Product_dir, this_entry->d_name ); 
            ret = unlink(&filename[0]);

      }

   }

      /* attempt to create the directory if it does not exist */

   if (opendir (Product_dir) == NULL) {
       printf ("Creating Product directory \"%s\"\n", 
               Product_dir);

      if ((mkdir (Product_dir, S_ISUID | 
                  S_ISGID | S_ISVTX | S_IRWXU | 
                  S_IRWXG | S_IROTH | S_IXOTH)) == -1) {
           perror ("Error creating Product directory -- ");
           printf ("Program is aborting\n");
           return (-1);
      }
   }

      /* update the product directory pointer and set the 
         "Save products" flag */

   Prod_dir_ptr = Product_dir;
   Product_save_flag = TRUE;

   return (0);
}

/********************************************************************************

     Description: This routine gets the product directory pointer

           Input:

          Output:

          Return: pointer to the product directory


 ********************************************************************************/

void *PROC_get_product_dir ()
{
   return (Prod_dir_ptr);
}


/********************************************************************************

     Description: This routine returns the product save flag.

           Input: Product_save_flag (file scope global flag)

          Output:

          Return: The Product_save flag


 ********************************************************************************/

int PROC_get_prod_save_flag ()
{
   return (Product_save_flag);
}


/********************************************************************************

     Description: This routine toggles the product save flag.

           Input:

          Output:

          Return:

 ********************************************************************************/

void PROC_toggle_prod_save_flag ()
{
   Product_save_flag ^= 0x00000001;
   return;
}


/********************************************************************************

     Description: This routine sets the new_connection_made flag which will 
                  force the "RPS List Required" flag to be set after
                  a GSM msg is received. 

          Input:

          Output:

          Return:

 ********************************************************************************/

void PROC_new_conn (void)
{
   New_connection_made = TRUE;
   return;
}

/********************************************************************************

     Description: This routine constructs the product's file name and opens the
                  file.

           Input: msg_code         - the ICD message code (for products, is the
                                     product code)
                  elevation_number - the elevation number retrieved from the
                                     message header
                  vol_scan_number  - volume scan number (1-80)
                  vol_scan_date    - start of volume scan date
                  vol_scan_time    - start of volume scan time
                  prodbuf          - product buffer (used to retrieve WMO/AWIPS
                                     header information)
                   
          Output: filepath - The full path name of the file

          Return: The file's file descriptor on success, or -1 if an error
                  occurs

 ********************************************************************************/

static int Construct_filename (int msg_code, u_short elevation_number,
                               ushort vol_scan_number, short vol_scan_date,
                               int vol_scan_time, char *prodbuf, char *filepath)
{
   int    i;
   int    ret;
   int    fd = -1;
   char   prod_mne[5];     
   char   filename[FILENAME_MAX]; 
   char   tmp_name[FILENAME_MAX]; 
   char   part_filename[FILENAME_MAX];
   char   elev_cut [3];
   long   newtime;
   struct stat stat_buf;
   struct tm *tp, *gmtime(), *localtime();

   static int index = 0;

   if (msg_code < MIN_FINAL_PROD_CODE) {
      strcpy (elev_cut, "0\0"); 
      vol_scan_number = 0;
      elevation_number = 0;
      vol_scan_date = 0;
      vol_scan_time = 0;
   }
   else
      snprintf (elev_cut, 3, "%d", elevation_number);

   for (i=0; ; i++) {
      if(PIDS[i] == -1) {   /* -1 = end of the populated PID array */
         sprintf(prod_mne, "P%d", msg_code);
         break;
      }

      if(PIDS[i] != msg_code) 
         continue;

      strcpy(prod_mne, MNE[i]);
      break; 
   }

      /* construct the file name */

   strcpy(filepath, Prod_dir_ptr);
   time(&newtime);
   tp = gmtime(&newtime);

      /* format the filename according to the user's instructions */

   switch (Product_filename_format) {

      case 1:
         sprintf(filename,"%s_%03d_E%s_%4s_%02d-%02d-%04d_%02d:%02d:%02dZ",
                 prod_mne, msg_code, elev_cut, MA_get_site_id (), (tp->tm_mon+1),
                 tp->tm_mday, (tp->tm_year+1900), tp->tm_hour, tp->tm_min,
                 tp->tm_sec);
      break;

      case 2:
         sprintf(filename,"%s_%03d_E%s_%d_%d_V%03d_%4s",
                 prod_mne, msg_code, elev_cut, vol_scan_date, vol_scan_time,
                 vol_scan_number, MA_get_site_id ());
      break;

      case 3:
         sprintf(filename, "%03d_%s_V%03d_E%s_%4s_%02d_%04d",
                 msg_code, prod_mne, vol_scan_number, elev_cut, 
                 MA_get_site_id (), vol_scan_date, vol_scan_time);
      break;

      case 4:
         sprintf(filename,"V%03d_E%s_%03d_%s_%4s_%02d-%02d-%04d_%02d:%02d:%02dZ",
                 vol_scan_number, elev_cut, msg_code, prod_mne, 
                 MA_get_site_id (), (tp->tm_mon+1), tp->tm_mday, 
                 (tp->tm_year+1900), tp->tm_hour, tp->tm_min, 
                 tp->tm_sec);
      break;

      case 5:
         sprintf(filename, "V%03d_%03d_%s_E%s_%4s_%02d_%04d",
                 vol_scan_number, msg_code, prod_mne, elev_cut, 
                 MA_get_site_id (), vol_scan_date, vol_scan_time);
      break;

      case 6:
         sprintf(filename, "bV%03d_%s_%03d_E%s_%4s_%02d_%04d",
                 vol_scan_number, prod_mne, msg_code, elev_cut, 
                 MA_get_site_id (), vol_scan_date, vol_scan_time);
      break;

      case 7:
      {
         short id = 0, elevation_angle = 0;
         Graphic_product *phd = (Graphic_product *) prodbuf;
         Icao = MA_get_site_id();

         if( elevation_number > 0 ) 
            elevation_angle = ntohs(phd->param_3);

         id = ntohs(phd->dest_id);
         if( id == 0 )
            id = MA_get_user_id();

         index++;
         if( index > 99999999 )
            index = 1;

         sprintf(part_filename, "PS.%3d_SC.U_DI.C_DC.RADAR_CP_OPUP_AR.VICINITY_PA.RADAR-%4s-%d-%05d-%08d",
                 id, &Icao[0], msg_code, elevation_angle, index );
         sprintf(filename, "%s_DD.%04d%02d%02d_DT.%02d%02d_DF.nxd",
                 part_filename,  (tp->tm_year+1900), (tp->tm_mon+1), tp->tm_mday, 
                 tp->tm_hour, tp->tm_min);
      break;
      }

      case 8:
      {
         WMO_AWIPS_hdr_t *hdr = (WMO_AWIPS_hdr_t *) prodbuf;
         char icao[5];
         char form_data_dist[7];
         char cat_prod[7];
         time_t ctime;
         int yr, mon, day, hr, min, sec;
 
         if( (msg_code == GSM_CODE) || (msg_code >= MIN_FINAL_PROD_CODE) ) {
            memcpy( icao, &hdr->wmo.originator[0], 4 );
            icao[4] = '\0';
            memcpy( form_data_dist, &hdr->wmo.form_type[0], 6 );
            form_data_dist[6] = '\0';
            memcpy( cat_prod, &hdr->awips.category[0], 6 );
            cat_prod[6] = '\0';
          
            ctime = (time_t) (vol_scan_date-1)*86400 + vol_scan_time;
            unix_time( &ctime, &yr, &mon, &day, &hr, &min, &sec );
            sprintf(filename, "%s_%s_%s_%04d%02d%02d%02d%02d",
                    icao, form_data_dist, cat_prod, 
                    yr, mon, day, hr, min);
         }
         else
            sprintf(filename,"%s_%03d_E%s_%4s_%02d-%02d-%04d_%02d:%02d:%02dZ",
                    prod_mne, msg_code, elev_cut, MA_get_site_id (), (tp->tm_mon+1),
                    tp->tm_mday, (tp->tm_year+1900), tp->tm_hour, tp->tm_min,
                    tp->tm_sec);
         break;

      }

      default:

         sprintf(filename,"%s_%03d_E%s_%4s_%02d-%02d-%04d_%02d:%02d:%02dZ",
                 prod_mne, msg_code, elev_cut, MA_get_site_id (), (tp->tm_mon+1),
                 tp->tm_mday, (tp->tm_year+1900), tp->tm_hour, tp->tm_min,
                 tp->tm_sec);
      break;
   }

   for (ret = 0; ret < strlen(filename); ret++) 
      if (filename[ret] == 0 || filename[ret] == ' ') filename[ret] = '_';

   strcat(filepath, filename);

      /* ensure the filename is unique */

   strcpy (tmp_name, filepath);
   i = 1;
   while (stat(tmp_name, &stat_buf) != -1) {
      sprintf (tmp_name, "%s.%01d", filepath, i);
      i++;
   }

      /* update the filename then create the file */

   strcpy (filepath, tmp_name);

   if ((fd = creat(filepath, 0664)) == -1) {
       strcat (Err_msg, "File creation error - file: \0");
       strcat (Err_msg, filepath);
       MA_abort(Err_msg);
   }
   return (fd);
}

/**************************************************************************

   Description: Processes Request/Response Message

   Inputs:      msg_data - Request/Response Message.

**************************************************************************/
static void Process_req_resp_message( char *msg_data ){

   short prod_code;
   unsigned int error_code;
   Prod_request_response_msg_icd *rr_msg;

   rr_msg = (Prod_request_response_msg_icd *) msg_data;
   prod_code = ntohs((short) rr_msg->msg_code);
   error_code = ntohl((((unsigned int) rr_msg->error_codem << 16) & 0xffff0000) + 
                ((unsigned int) rr_msg->error_codel & 0x0000ffff));

   printf( "Product Request Response for product# %d ",prod_code);
   switch( error_code ){

      case RR_NO_SUCH_MESSAGE:

         printf( "  NO SUCH MSG CODE, 0x%08x", error_code );
         break;

      case RR_NO_SUCH_PRODUCT:

         printf( " NO SUCH PROD CODE, 0x%08x", error_code );
         break;

      case RR_PRODUCT_NOT_GEN:

         printf( "PROD NOT GENERATED, 0x%08x", error_code );
         break;

      case RR_ONE_TIME_FAULT: 

         printf( "     OT GEN FAILED, 0x%08x", error_code );
         break;

      case RR_NARROWBAND_LS:  

         printf( "       NB LOADSHED, 0x%08x", error_code );
         break;

      case RR_ILLEGAL_REQUEST: 

         printf( "   ILLEGAL REQUEST, 0x%08x", error_code );
         break;

      case RR_MEMORY_LOADSHED:

         printf( "   MEMORY LOADSHED, 0x%08x", error_code );
         break;

      case RR_RPG_CPU_LOADSHED: 

         printf( "      CPU LOADSHED, 0x%08x", error_code );
         break;

      case RR_SLOT_UNAVAIL:   

         printf( "  UNAVAILABLE SLOT, 0x%08x", error_code );
         break;
  
      case RR_TASK_FAILURE: 

         printf( "      TASK FAILURE, 0x%08x", error_code );
         break;

      case RR_TASK_UNAVAIL: 

         printf( "  TASK UNAVAILABLE, 0x%08x", error_code );
         break;

      case RR_AVAIL_NEXT_SCAN: 

         printf( "   AVAIL NEXT SCAN, 0x%08x", error_code );
         break;

      case RR_DISABLED_MOMENT:

         printf( "   MOMENT DISABLED, 0x%08x", error_code );
         break;

      case RR_INVALID_PASSWORD: 

         printf( "  INVALID PASSWORD, 0x%08x", error_code );
         break;

      case RR_VOLUME_SCAN_ABORT: 

         printf( "    VOLUME ABORTED, 0x%08x", error_code );
         break;

      case RR_INVLD_PROD_PARAMS: 

         printf( "  INVLD PROD PARMS, 0x%08x", error_code );
         break;

      case RR_DATA_SEQ_ERROR: 

         printf( "    DATA SEQ ERROR, 0x%08x", error_code );
         break;

      case RR_TASK_TERM: 

         printf( "    TASK SELF TERM, 0x%08x", error_code );
         break;

      default:

         printf( "????????????? 0x%08x", error_code );
         break;

   /* End of "switch" statement. */
   }

/* End of Process_req_resp_message() */
}


/**************************************************************************

   Description: Adds product file name to tail of linked list.

   Inputs:      filename - Product file name.

**************************************************************************/
static void Add_to_list( char *filename ){

   Product_file_t *node = NULL;

   /* If the Remove_products value is 0, don't track. */
   if( Remove_products == 0 )
      return;

   /* Allocate space for product file in list. */
   node = (Product_file_t *) malloc( sizeof(Product_file_t) );

   if( node == NULL ){

      char text[128];
      sprintf( &text[0], "malloc of %d bytes Failed\n", sizeof(Product_file_t) );
      MA_abort(text);

   }

   strcpy( node->name, filename );
   node->time_written = time( NULL );
   node->next = NULL;

   /* If Tail is undefined, define it. */
   if( Tail == NULL ) 
      Tail = node;

   else{

      /* Add to Tail, then define new Tail */
      Tail->next = node;
      Tail = node;

   }

   if( Head == NULL )
     Head = Tail;
   
/* End of Add_to_list() */
}

/**************************************************************************

   Description: Removes old product files from head of linked list.

**************************************************************************/
static void Remove_from_list(){

   int ret;
   time_t ctime = time(NULL);
   Product_file_t *node;

   /* If the Remove_products value is 0, don't delete. */
   if( Remove_products == 0 )
      return;

   /* Go through list of products, removing products older than
      the time specified by Remove_products. */
   node = Head;
   while( node != NULL ){

      /* Is time difference greater than threshold? */
      if( (ctime - node->time_written) >= (time_t) Remove_products ){

         /* Remove product .... */
         ret = unlink( node->name );

         if( ret != 0 )
            fprintf( stderr, "Remove Product File: %s Failed\n", 
                     node->name );

         /* Set the next product to Head of list. */
         Head = node->next;
         free(node);

         /* Prepare for next product in list. */         
         node = Head;
          

      }
      else
         break;

   }

   /* If the whole list of products removed, set Tail to Head. */
   if( Head == NULL )
      Tail = NULL;

/* End of Remove_from_list() */
}
