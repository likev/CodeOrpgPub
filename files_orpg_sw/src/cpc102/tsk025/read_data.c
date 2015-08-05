/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 19:03:44 $
 * $Id: read_data.c,v 1.22 2014/03/18 19:03:44 jeffs Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */

#include <read_is.h>

#define READERROR	((size_t) -1)
#define URL_LENGTH	128
#define MINS_IN_HR	60
#define MIN_THRESH	6
#define MINS_IN_DAY	1440

/* Global Variables. */
int Cr_buffer_done;
Current_label_t Clabel;

/* Static Local Variables. */
static char Radar_url[URL_LENGTH];
static char Dir_url[URL_LENGTH];
static char Data_url[URL_LENGTH];
static char Local_url[URL_LENGTH];
static char Icao[8];
static int  Wait_interval;
static int  Read_local = 0;
static int  Get_current_time = 0;
static CURL *Curl_handle = NULL;
static char Error_buffer[CURL_ERROR_SIZE];
static MemoryStruct_t Dirmem;
static MemoryStruct_t Datamem;
static FileStruct_t Filename;
static FileStruct_t Lastread;
static char Last_finished[FILENAME_SIZE+1];
static int Icao_found = -1;


/* Function Prototypes. */
static int Read_options( int argc, char **argv );
static int Parse_directory( MemoryStruct_t *chunk, FileStruct_t *filename,
                            FileStruct_t *lastread );
static int Process_directory( MemoryStruct_t *dir, char *icao, FileStruct_t *filename,
                              FileStruct_t *lastread );
static size_t Read_levelII( FileStruct_t *lastread );
static void Init_curl_session();
static void Cleanup_curl_session();
static int Main_loop();
static size_t Process_IowaState();
static size_t Process_other( int local );
static int Get_local_url();

/*******************************************************************************
   Description:
      Libcurl callback function to read the directory of Level II data files.

   Inputs:
      ptr - pointer to data read.
      size - size of each 'nmemb', in bytes.
      nmemb - number of members.

   Outputs:
      data - local buffer where data will be stored.

*******************************************************************************/
static size_t ReadDirCallback( void *ptr, size_t size, size_t nmemb, 
                               void *data ){

   size_t realsize = size * nmemb;
   MemoryStruct_t *mem = (MemoryStruct_t *)data;
   
   /* Must store the directory. */
   mem->memory = realloc( mem->memory, mem->size + realsize + 1 );

   /* Exit on realloc failure.  */
   if( mem->memory == NULL ){

      /* Out of memory! */
      fprintf( stderr, "not enough memory (realloc returned NULL)\n" );
      sleep(1);
      exit(EXIT_FAILURE);

   }
  
   /* Copy directory data to local buffer and increase the size. */
   memcpy( &(mem->memory[mem->size]), ptr, realsize );
   mem->size += realsize;
   mem->memory[ mem->size ] = '\0';

   return realsize;

} /* End of ReadDirCallback() */

/*******************************************************************************
   Description:
      Libcurl callback function to read the Level II data files.

   Inputs:
      ptr - pointer to data read.
      size - size of each 'nmemb', in bytes.
      nmemb - number of members.

   Outputs:
      data - local buffer where data will be stored.

*******************************************************************************/
static size_t ReadDataCallback( void *ptr, size_t size, size_t nmemb,
                                void *data ){

   size_t realsize = size * nmemb;
   MemoryStruct_t *mem = (MemoryStruct_t *) data;

   /* Reallocate memory to store the data. */
   mem->memory = realloc( mem->memory, mem->size + realsize );

   /* Exit on failure. */
   if( mem->memory == NULL ){

      /* out of memory! */
      fprintf( stderr, "Not enough memory (realloc returned NULL)\n" );
      sleep(1);
      exit(EXIT_FAILURE);

   }

   /* Copy the data read to local buffer. */
   memcpy( &(mem->memory[mem->size]), ptr, realsize );
   mem->size += realsize;

   return realsize;

} /* End of ReadDataCallback() */


/***************************************************************************

   Description:
      Reads Level II data from the Iowa State web site and writes the data
      to LB.

   Note:  The size in the directory list can only be used to determine
          final size of a file.   The amount of data read can be larger
          than the size posted to the directory.  However the final size 
          of a file should agree with the final size posted to the 
          directory.

***************************************************************************/
int main( int argc, char *argv[] ){

   int ret;

   /* Process command line arguments. */
   if( Read_options( argc, argv ) != 0 ){

      sleep(1);
      exit( EXIT_FAILURE );

   }

   /* Initialize memory chunk. */
   Dirmem.memory = Datamem.memory = NULL;
   Dirmem.size = Datamem.size = 0;    

   /* Initialize the filename storage containers. */
   Filename.size = Lastread.size = 0;
   Filename.size_processed = Lastread.size_processed = 0;
   memset( Filename.filename, 0, (FILENAME_SIZE + 1) );
   memset( Lastread.filename, 0, (FILENAME_SIZE + 1) );
   memset( Last_finished, 0, (FILENAME_SIZE + 1) );

   /* Global initialization. */
   Cr_buffer_done = 0;

   /* Global initialization of the LIBCURL library. */
   curl_global_init( CURL_GLOBAL_ALL );

   /* Initialize the curl session. */
   Init_curl_session();

   /* Initialize MISC_systime. */
   MISC_systime(NULL);

   /* Main processing loop. */
   while(1){
 
      /* On abnormal error, cleanup and try again. */
      if( (ret = Main_loop()) < 0 ){

         fprintf( stderr, "Error ... Re-attempting Connection.\n" );

         /* Cleanup session. */
         Cleanup_curl_session();

         /* Recreate Local_url is needed. */
         if( Get_current_time ){

            /* We might have to rebuild the local URL 
               because the directory changes with day
               change. */
            if( Get_local_url() < 0 )
               break;

            Radar_url[0] = '\0';
            strcpy( Radar_url, Local_url );

            /* Create the Dir_url and initialize the Data_url. */
            Dir_url[0] = '\0';
            strcpy( Dir_url, Radar_url );

            Data_url[0] = '\0';
            strcpy( Data_url, Radar_url );

            fprintf( stderr, "URL: %s\n", Radar_url );

         }

         /* Initialize curl session. */
         Init_curl_session();

         /* Indicate we are done with the previous buffer. */
         Cr_buffer_done = 1;

      }

   }

   /* Cleanup curl stuff */
   Cleanup_curl_session();

   /* we're done with libcurl, so clean it up */
   curl_global_cleanup();

   /* Exit program. */
   return 0;

} /* End of main(). */

/***********************************************************************

   Description:  
      This function reads command line arguments.

   Input:        
      argc - Number of command line arguments.
      argv - Command line arguments.

   Output:       
      Usage message

   Returns:      
      0 on success or -1 on failure

*********************************************************************/
static int Read_options( int argc, char **argv ){

   extern char *optarg;    
   extern int optind;
   int c;                  
   int err;                
   int len = 0;

   /* Initialize the Radar_url to top level directory.  Initialize the Data_url
      and Dir_url strings. */
   memset( Icao, 0, 8 );
   memset( Data_url, 0, URL_LENGTH );
   memset( Dir_url, 0, URL_LENGTH );
   memset( Radar_url, 0, URL_LENGTH );
   memset( Local_url, 0, URL_LENGTH );
   Icao[0] = '\0';
   memset( LB_name, 0, 256 );

   Wait_interval = DEFAULT_WAIT;
   Verbose = 0;

   err = 0;

   while ((c = getopt (argc, argv, "vho:w:l:L")) != EOF) {

      switch (c) {

         /* Change the Wait Interval. */
         case 'w':
            Wait_interval = atoi( optarg );
            if( Wait_interval < MIN_WAIT )
               Wait_interval = MIN_WAIT;
            break;

         /* Get alternative output LB. */
         case 'o':
            strcat( LB_name, optarg );
            break;

         case 'l':
            Read_local = 1;
            strcat( Local_url, optarg );
            break;

         case 'L':
            Read_local = 1;
            Get_current_time = 1;
            break;

         /* Set verbose. */
         case 'v':
            Verbose = 1;
            break;

         /* Print out the usage information. */
         case 'h':
         case '?':
            err = 1;
            break;
      }

   }

   /* Get the ICAO. */
   if( optind == argc-1 )
      strncpy( (char *) &Icao[0], argv[optind], 5 );

   if( (err == 1) 
           || 
       ( (strlen( Icao ) == 0) || (strlen( Icao ) > ICAO_LENGTH)) ){ 

      fprintf( stderr, "Usage: %s [options] ICAO\n", argv[0]);
      fprintf( stderr, "Options:\n" );
      fprintf( stderr, "          -w <Wait Interval> (Default: %d secs)\n", DEFAULT_WAIT );
      fprintf( stderr, "          -o <Path to Output LB> (Default: $ORPGDIR/data/ingest/resp.0)\n" );
      fprintf( stderr, "          -v Verbose mode (Default: Not Verbose)\n" );
      fprintf( stderr, "\nThe following two (2) options are for ROC use only\n" );
      fprintf( stderr, "          -l <SWE Level 2 URL Containing \"raw\" files>\n" );
      fprintf( stderr, "          -L SWE Level 2 URL Containing \"raw\" files (Default: http://swewww/radars/)\n" );
      fprintf( stderr, "\n\nNote: Environmental Variables RADAR_URL, if defined, specifies the\n" );
      fprintf( stderr, "      standard location for Level II data if not using local URL.\n" );
      fprintf( stderr, "      Otherwise the tool defaults to Iowa State.\n" );
      fprintf( stderr, "\n      If using local URL, LOCAL_URL can be used to specify the\n" );
      fprintf( stderr, "      location of the Level II data when using the -L option.\n\n" );
      exit( EXIT_FAILURE );

   }

   /* If get_current_time is set, construct the local URL. */
   if( Get_current_time ){

      if( Get_local_url() < 0 )
         exit(0);

   }

   /* If local source not selected, set Radar_url. */
   if( !Read_local ){

      char location[URL_LENGTH];
      char *radar_url = getenv( "RADAR_URL" );

      if( radar_url != NULL ){

         if( Verbose )
            fprintf( stderr, "Environmental Variable RADAR_URL: %s\n",
                     radar_url );

         memset( &location[0], 0, URL_LENGTH );
         len = strlen( radar_url );

         if( len > 0 ){

            strcpy( location, radar_url );
            if( location[len-1] != '/' ){

               location[len] = '/';
               location[len+1] = '\0';

            }

            len = strlen( &location[0] );
            if( len < URL_LENGTH-1 )
               memcpy( &Radar_url[0], &location[0], len+1 );

         }
         else 
            strcat( Radar_url, "http://mesonet-nexrad.agron.iastate.edu/level2/raw/" );

      }

      /* If all else fails ..... */
      if( Radar_url[0] == '\0' )
         strcat( Radar_url, "http://mesonet-nexrad.agron.iastate.edu/level2/raw/" );

      strcat( Dir_url, "dir.list" );

      /* Copy ICAO to Data_url(). */
      strcat( Radar_url, Icao );
      Radar_url[ strlen( Radar_url ) ] = '/';

   }
   else{

      strcat( Radar_url, Local_url );
      len = strlen( Radar_url );
      if( (len < URL_LENGTH-1) && (Radar_url[len-1] != '/') ){

         Radar_url[len] = '/';
         Radar_url[len+1] = '\0';

      }

   }

   /* Create the Dir_url and initialize the Data_url. */
   strcpy( Dir_url, Radar_url );
   strcpy( Data_url, Radar_url );

   fprintf( stderr, "URL: %s\n", Radar_url );

   if( strlen( LB_name ) == 0 ){

      char *temp;
      temp = getenv( "ORPGDIR" );
      if( temp == NULL ){

         fprintf( stderr, "ORPGDIR Environmental Variable Not Found\n" );
         exit( EXIT_FAILURE );

      }
      strcat( LB_name, temp );

   }

   strcat( LB_name, "/ingest/resp.0" );
   fprintf( stderr, "Output LB Name: %s\n", LB_name );
   LB_fd = LB_open( LB_name, LB_WRITE, NULL );
   if( LB_fd < 0 ){

      fprintf( stderr, "Unable to Open Output LB: %d\n", LB_fd );
      sleep(1);
      exit( EXIT_FAILURE );

   }

   return (0);

/* End of Read_options() */
}

/**************************************************************************

   Description:
   Parse the dir.list file for the given ICAO.

**************************************************************************/
static int Parse_directory( MemoryStruct_t *chunk, FileStruct_t *locatefile,
                            FileStruct_t *lastread ){

   char *ptr = chunk->memory, *end_ptr = NULL, temp;
   int size;
   size_t inc;
   char filename[FILENAME_SIZE + 1];

   end_ptr = ptr;
   inc = 0;
   while(1){

      while( (*end_ptr != 0xA) && (inc <= chunk->size) ){

         /* Increment pointer and counter. */
         end_ptr++;
         inc++;

      }

      /* Set size and filename. */
      memset( filename, 0, FILENAME_SIZE+1 );
      if( (int) (end_ptr - ptr) > FILENAME_SIZE ){
  
         /* The value at the end_ptr should be the new line.  Save it off
            just in case so it can be restored after the sscanf() call. */
         temp = *end_ptr;
         *end_ptr = '\0';
         sscanf( ptr, "%d %s", &size, &filename[0] );

         /* Restore the value at the end_ptr. */
         *end_ptr = temp;

         /* Does file match the file to locate.  If so, check the size to 
            see if more information needs to read. */
         if( (locatefile != NULL) && (strlen( &locatefile->filename[0] ) > 0) ){

            if( strncmp( &filename[0], &locatefile->filename[0], 
                         FILENAME_SIZE ) == 0 ){

               lastread->size = size;
               strncpy( &lastread->filename[0], &filename[0], 
                        FILENAME_SIZE );
               return 0;

            }

         }

      }

      /* Prepare for next iteration. */
      end_ptr++;
      ptr = end_ptr;

      /* Break out of loop if no more data to process. */
      if( (inc >= chunk->size) || (*end_ptr == '\0') )
         break;
     
   }    
      
   /* Transfer information for caller. */
   lastread->size = size;
   strcpy( &lastread->filename[0], &filename[0] );

   return 0;

} /* End of Parse_directory(). */

/***********************************************************************************

   Description:
      This function handles the case where the dir.list file does not exist.

   Notes:
      Assumes file naming convention of the form:  ICAO_YYYYMMDD_HHMM. 

***********************************************************************************/
static int Process_directory( MemoryStruct_t *dir, char *icao, 
                              FileStruct_t *locatefile, FileStruct_t *lastread ){

   char *str = dir->memory;
   char *substr = NULL;
   char filename[FILENAME_SIZE];
   char match[8];
   int cnt = 0;

   /* Initialize the match string. */
   memset( match, 0, sizeof(match) );

   /* Initialize the filename.*/
   memset( &filename[0], 0, FILENAME_SIZE+1 );

   /* Define match string as "ICAO_. */
   match[0] = '"';
   strcat( match, icao );
   match[5] = '_';

   /* Parse the page .... locate all file links and save in Dir_entry. */
   while(1){

      int len;

      /* Ensure we do not go past the end of the directory. */
      len = (int) (str - &dir->memory[0]) + 1;
      if( len >= dir->size )
         break;

      /* Look for a substring match. */
      substr = strstr( str, match );
      if( substr != NULL ){

         /* Found a match .... adjust pointer to look for right quote. */
         char *ptr = substr + strlen( match );

         /* Each file link is quoted, i.e., "...".  Find the end of the quoted
            string .... Once found, remove the quotes.   We should be left with
            a string of the following form:  ICAO_YYYYMMDD_HHMM. */
         while(1){

            if( *ptr == '"' ){

               len = (int) (ptr - substr);
               if( len >= FILENAME_SIZE )
                  fprintf( stderr, "Filename Size: %d Too Big for File: %s\n", 
                           len, &substr[1] );
               memset( &filename[0], 0, FILENAME_SIZE+1 );
               memcpy( &filename[0], &substr[1], len-1 );
               filename[len] = '\0';

               cnt++;

               /* Does file match the file to locate?  */
               if( (locatefile != NULL) && (strlen( &locatefile->filename[0] ) > 0) ){

                  if( strncmp( &filename[0], &locatefile->filename[0], 
                               FILENAME_SIZE ) == 0 ){

                     /* Copy filename to lastread->filename. */
                     strncpy( &lastread->filename[0], &filename[0], 
                              FILENAME_SIZE );
                     return( cnt );

                  }

               }

            }
            else{

               /* Move the string pointer to the next character in the page. */
               ptr++;
               continue;

            }

            break;

         }

         /* Adjust the pointer to look for next file link. */
         str = ptr;

      }
      else
         break;

   }

   /* If there are directory entries, copy the last entry found into filename. */
   if( cnt > 0 )
      memcpy( &lastread->filename[0], &filename[0], strlen(&filename[0]) );

   /* Return the number of entries. */
   return cnt;

} /* End of Process_directory(). */

/***********************************************************************************

   Description:
      This function handles the case where the dir.list file does not exist.

   Notes:
      Assumes file naming convention of the form:  ICAOYYYYMMDDHHMMVxx.raw. 

      This function was written specifically to support Level II data recorded
      on the ROC SW Engineering file server.

***********************************************************************************/
static int Process_local_directory( MemoryStruct_t *dir, char *icao,
                                    FileStruct_t *locatefile, FileStruct_t *lastread ){

   char *str = dir->memory;
   char *substr = NULL;
   char filename[FILENAME_SIZE];
   char match[8];
   int cnt = 0;

   /* Initialize the match string. */
   memset( match, 0, sizeof(match) );

   /* Initialize the filename.*/
   memset( &filename[0], 0, FILENAME_SIZE+1 );

   /* Define match string as "ICAO. */
   match[0] = '"';
   strcat( match, icao );

   /* Parse the page .... locate all file links and save in Dir_entry. */
   while(1){

      int len;

      /* Ensure we do not go past the end of the directory. */
      len = (int) (str - &dir->memory[0]) + 1;
      if( len >= dir->size )
         break;

      /* Look for a substring match. */
      substr = strstr( str, match );
      if( substr != NULL ){

         /* Found a match .... adjust pointer to look for right quote. */
         char *ptr = substr + strlen( match );

         /* Each file link is quoted, i.e., "...".  Find the end of the quoted
            string .... Once found, remove the quotes.   We should be left with
            a string of the following form:  ICAOYYYYMMDDHHMMVxx.raw. */
         while(1){

            if( *ptr == '"' ){

               len = (int) (ptr - substr);
               memset( &filename[0], 0, FILENAME_SIZE+1 );
               memcpy( &filename[0], &substr[1], len-1 );
               filename[len] = '\0';

               cnt++;

               /* Does file match the file to locate?  */
               if( (locatefile != NULL) && (strlen( &locatefile->filename[0] ) > 0) ){

                  if( strncmp( &filename[0], &locatefile->filename[0],
                               FILENAME_SIZE ) == 0 ){

                     /* Copy filename to lastread->filename. */
                     strncpy( &lastread->filename[0], &filename[0],
                              FILENAME_SIZE );
                     return( cnt );

                  }

               }

            }
            else{

               /* Move the string pointer to the next character in the page. */
               ptr++;
               continue;

            }

            break;

         }

         /* Adjust the pointer to look for next file link. */
         str = ptr;

      }
      else
         break;

   }

   /* If there are directory entries, copy the last entry found into filename. */
   if( cnt > 0 )
      memcpy( &lastread->filename[0], &filename[0], strlen(&filename[0]) );

   /* Return the number of entries. */
   return cnt;

} /* End of Process_local_directory(). */


/****************************************************************************

   Description:
      Reads the Level II data specified by filename in lastread.  

   Inputs;
      lastread - struct holding filename and size of file.

   Returns:
      The number of bytes read from Level II file.

****************************************************************************/
static size_t Read_levelII( FileStruct_t *lastread ){ 

   int ret;
   char url[URL_LENGTH];

   static int error_cnt = 0;
   static int warning_cnt = 0;

   url[0] = '\0';
   strcat( url, Radar_url );

   if( (lastread != NULL) && (strlen( &lastread->filename[0] ) > 0) )
      strcat( url, &lastread->filename[0] );
      
   else{

      fprintf( stderr, "Error In Read_levelII\n" );
      sleep(1);
      exit( EXIT_FAILURE );

   }

   /* Specify URL to get.  This will be data file for the radar of interest. */
   if( (curl_easy_setopt( Curl_handle, CURLOPT_URL, url ) != 0)
                               || 

        /* Send all data to this function  */
       (curl_easy_setopt( Curl_handle, CURLOPT_WRITEFUNCTION, ReadDataCallback ) != 0) 
                               ||

        /* We pass our memory struct to the callback function */
       (curl_easy_setopt( Curl_handle, CURLOPT_WRITEDATA, (void *)&Datamem ) != 0)
                               ||
   
       /* Set where the read needs to start from using CURLOPT_RESUME_FROM. */
       (curl_easy_setopt( Curl_handle, CURLOPT_RESUME_FROM, (long) Datamem.size ) != 0) ){

      fprintf( stderr, "Unrecoverable Error. curl_easy_setopt Failed: %s\n", 
               &Error_buffer[0] );
      sleep(1);
      exit( EXIT_FAILURE );

   }

   /* Perform the read. */
   if( (ret = curl_easy_perform( Curl_handle )) != 0 ){

      /* These are CURL warnings .... add warning error codes as needed. 
         Note: If a certain error code is received alot, check the web.  Maybe
               the particular error code can be treated more as a warning than
               actual error (e.g., CURLE_PARTIAL_FILE) */
      if( ret == CURLE_PARTIAL_FILE )
         warning_cnt++;

      else
         error_cnt++;

      /* Write out informative message if Verbose mode and error/warning limit
         reached. */ 
      if( (Verbose) 
             && 
          ((error_cnt < MAX_ERROR_COUNT)
                     ||
          (error_cnt == MAX_ERROR_COUNT)
                     ||
           (warning_cnt == MAX_WARNING_COUNT)) )
         fprintf( stderr, "Read_levelII: curl_easy_perform Failed(%d): %s\n", 
                  ret, &Error_buffer[0] );

   }
   else{

      error_cnt = 0;
      warning_cnt = 0;

   }

   /* If error or warning limit reached, return READERROR. */
   if( (error_cnt >= MAX_ERROR_COUNT)
                  ||
       (warning_cnt >= MAX_WARNING_COUNT) ){
   
      fprintf( stderr, "Error/Warning Count Exceeded ... Attempting Recovery\n" );
      error_cnt = 0;
      warning_cnt = 0;

      /* Return error. */
      return(READERROR);

   }

   /* Reset url for next directory read. */
   if( (curl_easy_setopt( Curl_handle, CURLOPT_URL, Dir_url ) != 0) 
                               ||
       (curl_easy_setopt( Curl_handle, CURLOPT_WRITEFUNCTION, ReadDirCallback ) != 0)
                               || 
       (curl_easy_setopt( Curl_handle, CURLOPT_WRITEDATA, (void *)&Dirmem ) != 0) 
                               ||
       (curl_easy_setopt( Curl_handle, CURLOPT_RESUME_FROM, (long) 0 ) != 0) ){

      fprintf( stderr, "Unrecoverable Error. curl_easy_setopt Failed: %s\n", 
               &Error_buffer[0] ); 
      sleep(1);
      exit( EXIT_FAILURE );

   }

   return Datamem.size;

} /* End of Read_levelII(). */

/******************************************************************************

   Description:
      Initializes the curl session. 

   Return:
      No return value defined.

******************************************************************************/
static void Init_curl_session(){

   /* Init the curl session */
   Curl_handle = curl_easy_init();

   /* Check for error. */
   if( Curl_handle == NULL ){

      fprintf( stderr, "curl_easy_init Did Not Return Valid Handle.\n" );
      sleep(1);
      exit( EXIT_FAILURE );

   }

   /* Specity Error Buffer. */
   if( (curl_easy_setopt( Curl_handle, CURLOPT_ERRORBUFFER, &Error_buffer[0] ) != 0)
                                    ||

       /* Specify URL to get.  This will be dir.list file under the radar 
          of interest. */
       (curl_easy_setopt( Curl_handle, CURLOPT_URL, Dir_url ) != 0)
                                    ||

        /* Send all data to this function  */
       (curl_easy_setopt( Curl_handle, CURLOPT_WRITEFUNCTION, ReadDirCallback ) != 0)
                                    ||

       /* We pass our 'chunk' struct to the callback function */
       (curl_easy_setopt( Curl_handle, CURLOPT_WRITEDATA, (void *)&Dirmem ) != 0)
                                    ||

       /* Some servers don't like requests that are made without a user-agent
          field, so we provide one */
       (curl_easy_setopt( Curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0" ) != 0) ){

      fprintf( stderr, "Unrecoverable Error.  curl_easy_setopt Failed: %s\n",
               &Error_buffer[0] );
      sleep(1);
      exit( EXIT_FAILURE );

   }

} /* End of Init_curl_session(). */


/****************************************************************************

   Description:
      Cleanup curl session.

   Returns:
      No return value defined. 

****************************************************************************/
static void Cleanup_curl_session(){

   /* Cleanup curl stuff */
   curl_easy_cleanup( Curl_handle );

   /* Free all allocated memory. */

   /* Directory memory. */
   if( Dirmem.memory != NULL )
      free( Dirmem.memory );

   Dirmem.size = 0;
   Dirmem.memory = NULL;

   /* Data memory. */
   if( Datamem.memory != NULL )
      free( Datamem.memory );

   Datamem.size = 0;
   Datamem.memory = NULL;

   /* Cleanup up filenames. */
   Filename.size = 0;
   Filename.size_processed = 0;
   memset( &Filename.filename[0], 0, FILENAME_SIZE+1 );

   Lastread.size = 0;
   Lastread.size_processed = 0;
   memset( &Lastread.filename[0], 0, FILENAME_SIZE+1 );

} /* End of Cleanup_curl_session(). */

/*************************************************************************************

   Description:
      Main processing loop for read_is.

   Returns:
      0 on success, negative number on error. 

*************************************************************************************/
static int Main_loop(){

   int ret, elapsed_wait = 0;
   time_t start_time = 0;
   size_t size;

   static int error_cnt = 0;

   elapsed_wait = Wait_interval;
   while(1){

      if( elapsed_wait >= Wait_interval ){

         start_time = MISC_systime(NULL);
         elapsed_wait = 0;

         /* Get it!  Abort if the number of bytes returned is 0. */
         if( (ret = curl_easy_perform( Curl_handle )) != 0 ){

            if( Verbose )
               fprintf( stderr, "Main: curl_easy_perform Failed: %s\n", &Error_buffer[0] );

            error_cnt++;
            if( error_cnt >= MAX_ERROR_COUNT ){

               /* Report errors to caller. */
               error_cnt = 0;
               return(-1);

            }
 
            continue;

         }
         else
            error_cnt = 0;

         if( Dirmem.size == 0 ){

            if( Verbose )
               fprintf( stderr, "Error Reading Directory\n" );
            continue;

         }

         /* Pick which path .... */
         if( !Local_url )
            size = Process_IowaState();

         else 
            size = Process_other( 1 );

         /* Clear out Dirmem. */
         if( Dirmem.memory != NULL ){

            free( Dirmem.memory );
            Dirmem.memory = NULL;
            Dirmem.size = 0;

         }

         /* Check the return value.   If error, then return. */
         if( size == READERROR )
            return(-1);

      }

      sleep(1);
      elapsed_wait = (int) (MISC_systime(NULL) - start_time);

   }

} /* End of Main_loop(). */


/***********************************************************************************

   Description:
      Process Level II directory file from Iowa State.   This is the preferred 
      method.

   Returns:
      The size of the data processed.

***********************************************************************************/
static size_t Process_IowaState(){

   size_t size;

   /* Find the latest file. */
   Parse_directory( &Dirmem, NULL, &Lastread );

   /* Has the ICAO been found? */
   if( strlen( &Lastread.filename[0] ) == 0 ){

      fprintf( stderr, "Unable to Locate Directory Entry for %s\n",
               Icao );

      /* ICAO was found before, so it is a valid ICAO.   Just return
         to caller .... this will likely clear up after the next 
         directory read. */
      if( Icao_found )
         return 0;

      sleep(1);
      exit( EXIT_FAILURE );

   }

   /* Set the Icao found flag .... */
   Icao_found = 1;

   /* The following logic checks to see if all the data that needs to be 
      read, has been read. */
   if( strlen( &Filename.filename[0] ) == 0 ){

      /* Handles the case where a new file needs to be read. */
      Filename.size = Filename.size_processed = 0;
      strncpy( &Filename.filename[0], &Lastread.filename[0],
               FILENAME_SIZE+1 );

   }
   else if( strcmp( &Filename.filename[0], &Lastread.filename[0] ) != 0 ){

      /* Handles the case where a new files has been posted to the
         directory. */
      Parse_directory( &Dirmem, &Filename, &Lastread );
      if( (Filename.size_processed == Lastread.size)
                         ||
                  (Cr_buffer_done) ){

         /* We are done reading this data. */
         Filename.size = Filename.size_processed = 0;
         memset( &Filename.filename[0], 0, FILENAME_SIZE + 1 );
         Parse_directory( &Dirmem, NULL, &Lastread );

         /* Free Data memory. */
         if( Datamem.memory != NULL )
            free( Datamem.memory );

         Datamem.size = 0;
         Datamem.memory = NULL;

         /* Indicate we are done with this buffer. 
         if( Cr_buffer_done )
            Cr_buffer_done = 0;
         */

      }

   }

   /* Check to see if we have already processed this file.  If yes, skip processing
      the file again. */
   if( strcmp( &Last_finished[0], &Lastread.filename[0] ) == 0 ){

      if( Verbose )
         fprintf( stderr, "Already Read and Processed file: %s\n", &Last_finished[0] );

      /* Return Normal. */
      return 0;

   }

   /* Read the Level II data.  The return value is the total size read
      so far for this file.  (size_t) -1 is returned on error. */
   if( (size = Read_levelII( &Lastread )) == READERROR ){

      /* We don't want to process this data from the beginning so set Last_finished
         to Lastread. */
      memcpy( &Last_finished[0], &Lastread.filename[0], FILENAME_SIZE+1 );
      return( size );

   }

   /* This keeps track of what is available. */
   Filename.size = size;

   if( strlen( &Filename.filename[0] ) == 0 )
      strncpy( &Filename.filename[0], &Lastread.filename[0],
               FILENAME_SIZE + 1 );

   /* If data to process, write the data to LB. */
   if( (size != 0) && (size != READERROR) ){

      size = Process_current_buffer( &Datamem, &Filename );

      if( Cr_buffer_done ){

         /* Save the file that was last finished. */
         memcpy( &Last_finished[0], &Lastread.filename[0], FILENAME_SIZE+1 );

      }

      /* This keeps track of what has been processed. */
      if( size >= 0 )
         Filename.size_processed += size;

   }

   /* Return to caller. */
   return(size);

} /* End of Process_IowaState(). */

/******************************************************************************

   Description:
      A dir.list file does not exist.   Process a list of directory links.

   Returns:
      Size of the data processed.

******************************************************************************/
static size_t Process_other( int local ){

   size_t size;
   int cnt;
 
   /* Find the latest file in the directory. */
   if( local )
      cnt = Process_local_directory( &Dirmem, Icao, NULL, &Lastread );

   else
      cnt = Process_directory( &Dirmem, Icao, NULL, &Lastread );

   /* Has the ICAO been found? */
   if( strlen( &Lastread.filename[0] ) == 0 ){

      fprintf( stderr, "Unable to Locate Directory Entry for %s\n",
               Icao );
 
      /* ICAO was found before, so it is a valid ICAO.   Just return
         to caller .... this will likely clear up after the next 
         directory read. */
      if( Icao_found && !local )
         return 0;

      else
         return READERROR;

      exit( EXIT_FAILURE );

   }

   /* Set the Icao found flag .... This will be used later (see above code)
      to prevent exiting. */
   Icao_found = 1;

   /* The following logic checks to see if all the data that needs to be 
      read, has been read. */
   if( strlen( &Filename.filename[0] ) == 0 ){

      /* Handles the case where a new file needs to be read. */
      Filename.size = Filename.size_processed = 0;
      strncpy( &Filename.filename[0], &Lastread.filename[0],
               FILENAME_SIZE+1 );

   }
   else if( strcmp( &Filename.filename[0], &Lastread.filename[0] ) != 0 ){

      /* Handles the case where a new file has been posted to the
         directory. */
      if( local )
         Process_local_directory( &Dirmem, Icao, &Filename, &Lastread );

      else
         Process_directory( &Dirmem, Icao, &Filename, &Lastread );

      if( Cr_buffer_done ){

         /* We are done reading this data. */
         memset( &Filename.filename[0], 0, FILENAME_SIZE + 1 );
         if( local )
            Process_local_directory( &Dirmem, Icao, NULL, &Lastread );

         else
            Process_directory( &Dirmem, Icao, NULL, &Lastread );

         /* Free Data memory. */
         if( Datamem.memory != NULL )
            free( Datamem.memory );

         Datamem.size = 0;
         Datamem.memory = NULL;

         /* Indicate we are done with this buffer. 
         if( Cr_buffer_done )
            Cr_buffer_done = 0;

         */

      }

   }

   /* Check to see if we have already processed this file.  If yes, skip processing
      the file again. */
   if( strcmp( &Last_finished[0], &Lastread.filename[0] ) == 0 ){

      struct tm ctime;
      time_t now = time(NULL);

      if( Verbose )
         fprintf( stderr, "Already Read and Processed file: %s\n", &Last_finished[0] );

      /* Get the current time. */
      if( gmtime_r( &now, &ctime ) != NULL ){
      
         /* If the current date has changed or the last file read was within 
            threshold minutes of midnight, return READERROR.  This will force
            a new directory to be read. */
         if( (ctime.tm_mday != Clabel.day)  
                           ||
             (((Clabel.hour*MINS_IN_HR) + Clabel.minute + MIN_THRESH) >= MINS_IN_DAY) ){

            /* Tell the operator why we are returning an error. */
            if( Verbose ){

               if( ctime.tm_mday != Clabel.day )
                  fprintf( stderr, "Last File Date (%02d) Not Current (%02d)\n",
                           ctime.tm_mday, Clabel.day );

               else
                  fprintf( stderr, "Last File Time (%02d:%02d) Within %d Minutes of Midnight\n",
                           Clabel.hour, Clabel.minute, MIN_THRESH );

            }

            /* Force opening a new directory (if it exists). */
            return READERROR;

         }

      }

      /* Return normal. */
      return 0;

   }

   /* Read the Level II data.  The return value is the total size read
      so far for this file. */
   if( (size = Read_levelII( &Lastread )) == READERROR ){

      /* We don't want to process this data from the beginning so set Last_finished
         to Lastread. */
      memcpy( &Last_finished[0], &Lastread.filename[0], FILENAME_SIZE+1 );
      return( size );

   }

   if( strlen( &Filename.filename[0] ) == 0 )
      strncpy( &Filename.filename[0], &Lastread.filename[0],
               FILENAME_SIZE + 1 );

   /* If there is data to process, write the data to LB. */
   if( (size != 0) && (size != READERROR) ){

      size = Process_current_buffer( &Datamem, &Lastread );

      if( Cr_buffer_done ){

         /* Save the file that was last finished. */
         memcpy( &Last_finished[0], &Lastread.filename[0], FILENAME_SIZE+1 );

      }

   }

   /* Return to caller. */
   return( size );

} /* End of Process_other(). */


/******************************************************************************

   Description:
      Builds the local URL ... assumes data is on the ROC SWE file server.

   Returns:
      0 on success, or negative number on error. 

******************************************************************************/
static int Get_local_url(){

   struct tm ctime;

   char location[URL_LENGTH];
   char *local_url = NULL;
   time_t now = time(NULL);
   int len = 0;

#ifdef TEST_LOCAL_URL
   static int first_time = 1;
#endif

   /* Initialize the Local_url string to empty string. */
   memset( Local_url, 0, URL_LENGTH );

   /* First check if environmental variable LOCAL_URL is set. 
      If so, then use this instead of the hard coded value. */
   local_url = getenv( "LOCAL_URL" );
   if( local_url != NULL ){

      if( Verbose )
         fprintf( stderr, "Environmental Variable LOCAL_URL: %s\n",
                  local_url );

      memset( location, 0, URL_LENGTH );
      len = strlen( local_url );
      strcpy( location, local_url );
      if( location[len-1] != '/' ){

         location[len] = '/';
         location[len+1] = '\0';

      }

      strcat( Local_url, location );

   }
   else {

      /* Add the "common" part of the URL. */
      strcat( Local_url, "http://swewww/radars/" );

   }

   if( gmtime_r( &now, &ctime ) != NULL ){

#ifdef TEST_LOCAL_URL
      /* This is set the day of the month to the previous day.  
         Will not work on the first day of the month. */
      if( first_time && ctime.tm_mday != 1 ){

         first_time = 0;
         ctime.tm_mday -= 1;

      }
#endif

      /* Add the ICA0, year, month and day. */
      memset( location, 0, URL_LENGTH );
      sprintf( location, "%s/%s.%4d.%02d/%02d/",
               Icao, Icao, 1900+ctime.tm_year, ctime.tm_mon+1, ctime.tm_mday );
      strcat( Local_url, location );

      fprintf( stderr, "Local URL: %s\n", Local_url );
      if( Verbose )
         fprintf( stderr, "--->ctime.tm_hour: %d, ctime.tm_min: %d, ctime.tm_sec: %d\n",
                  ctime.tm_hour, ctime.tm_min, ctime.tm_sec );

   }
   else{

      fprintf( stderr, "gmtime error ... exiting\n" );
      return -1;

   }

   /* Return success. */
   return 0;

/* End of Get_local_url() */
}
