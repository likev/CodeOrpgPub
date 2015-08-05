/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/13 15:01:27 $
 * $Id: cn_write_records.c,v 1.1 2014/05/13 15:01:27 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 *
 *
 */  

#include "change_nyquist.h"
#include <bzlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define LDM_VOL_HDR_SIZE         24
#define BASEYR                   2440587
#define SECONDS_IN_A_DAY         86400
#define MILLISECONDS_PER_SECOND  1000

static char *Site_icao;
static uint Julian_date;
static uint Julian_time;
static int  Volume_id = 0;
static struct tm Vol_tm;
static ArchII_perfmon_t LDM_perfmon;

/* function declarations */

static void Construct_volume_hdr (char *buffer);
static int  Create_and_open_file (char *file_name, int *err);
static int  Populate_uncompressed_data (char *data_buffer, int data_len, 
                                        int rad_status);
static void Write_local_data (char *key, char *data, int size, int radial_stat);
static void Write_record (char *buffer, int compressed_len, int uncompressed_len, 
                          int record_num, short rad_status, char *ldm_key, 
                          char *local_key);

/********************************************************************************

   Description: Add the message to the radial buffer

   Input: *msg_buf        - pointer to the message
          *msg_properties - properties of the message being processed
          ldm_Ver         - LDM version number

   Output: None

   Return: None
   
*********************************************************************************/
void WR_add_msg_to_radial_buffer (char *msg_buf, msg_data_t *msg_properties, 
                                  int previous_rda_status) {

   static int  radial_cnt = 0;
   static int  byte_cnt = 0;
   static char *block_ptr = NULL;
   int         block_offset = 0;
   int         packet_size;

      /* clear the buffer of residual radials if the buffer is not empty
         and this is a beginning-of-volume message - this can happen on
         volume restarts */

   if ((msg_properties->radial_status == BEG_VOL) && 
       (block_ptr != NULL) && (byte_cnt != 0)) {
        radial_cnt = 0;
        byte_cnt = 0;
        free (block_ptr);
        block_ptr = NULL;
   }

      /* determine packet size for this msg */

   if (msg_properties->msg_hdr.type == MESSAGE_TYPE_GENERIC_DIGITAL_RADAR_DATA)
      packet_size = msg_properties->msg_size + CTM_HDRSZE_BYTES;
   else {  /* legacy radial or RDA status msg assumed */
      packet_size = NEX_PACKET_SIZE;

          /* we must clip the msg if it is larger than the ICD defined packet 
             size (hopefully, this will not happen) */

      if (packet_size < (msg_properties->msg_size + CTM_HDRSZE_BYTES)) {
         LE_send_msg (GL_ERROR, 
            "msg type %d size (%d) is > max size allowed (%d)...clipping msg",
            msg_properties->msg_hdr.type, 
            msg_properties->msg_size + CTM_HDRSZE_BYTES,
            NEX_PACKET_SIZE);  
         msg_properties->msg_size = packet_size;
      }
   }

      /* allocate memory for this msg */

   if ((block_ptr = realloc (block_ptr, (size_t) (byte_cnt + packet_size))) == NULL)
      MA_Abort_Task("realloc failed...task terminating");

   memset (block_ptr + byte_cnt, 0, packet_size);

      /* copy the msg to the data block */

   block_offset = byte_cnt + CTM_HDRSZE_BYTES;

   memcpy (block_ptr + block_offset, msg_buf, msg_properties->msg_size);

      /* update the data counters (RDA status msgs are not counted) */

   byte_cnt += packet_size;

   if (msg_properties->msg_hdr.type != MESSAGE_TYPE_RDA_STATUS)
      ++radial_cnt;

      /* see if it's time to write the data block to the LDM queue */

   if ((radial_cnt == 120) || 
       (msg_properties->radial_status == END_VOL)    ||
       (msg_properties->radial_status == END_ELEV)   ||
      ((msg_properties->rda_status != RS_OPERATE)    &&
       (previous_rda_status == RS_OPERATE))) {
         WR_process_record (block_ptr, byte_cnt, msg_properties);
         radial_cnt = 0;
         byte_cnt = 0;
         free (block_ptr);
         block_ptr = NULL;
   }

   return;
}


/********************************************************************************

   Description: Initialize record information locally in this file for
                 convenience

   Input: *site_id - pointer to the site ICAO char string

   Output: Site_icao - pointer to the site id string

   Return: 

   Note: Site_icao is globally defined at the beginning of this file.
   
*********************************************************************************/
void WR_init_record_info (char *site_id) {

    memset (&LDM_perfmon, 0, sizeof(ArchII_perfmon_t));

    Site_icao = site_id;
    return;
}


/********************************************************************************

   Description: Pacakge the data block into a LDM record and write the
                 LDM record to the LDM queue/local file.

   Input: *buf        - pointer to the data block containing the LDM data
          data_len    - data length in bytes of the data block
          msg_data    - structure of message data of interest used to process
                               the message

   Output: Vol_tm      - the volume time structure

   Return: 

   Note: Vol_tm is globally defined at the beginning of this file.
   
*********************************************************************************/
void WR_process_record (char *buf, int data_len, msg_data_t *msg_data) {

   char key_status;
   char local_key [128];
   char ldm_key [128];
   char *compressed_buf;
   uint compressed_data_len = 0;
   int  block_size;
   int  ret;
   int  offset = 0;          /* offset to the data */
   int  header_len;          /* the actual length of the record header */
   static int record = 0;

      /* When "BEG_VOL" is processed, the record # is set to 1, the 
         Volume Header is generated and the Metadata record is written
         to the LDM LB */

   if (msg_data->radial_status == BEG_VOL) {
      key_status = 'S';
      record = 0;
      Vol_tm = *gmtime (&(msg_data->radial_time));
      offset = LDM_VOL_HDR_SIZE + sizeof (int); /* LDM header + control word */
      /* increment the LDM volume number */
      if (Volume_id < 999)
          Volume_id++;
      else
          Volume_id = 1;
   } else if (msg_data->radial_status == END_VOL) {
      key_status = 'E';
      offset = sizeof (int);  /* control word */
   } else {
      key_status = 'I';
      offset = sizeof (int);  /* control word */
   }

   ++record;  /* increment the file record number  */

      /* construct the local and LDM keys for this record */
   
   sprintf (local_key, "%s%04d%02d%02d%02d%02d%02dV%02d.raw",
            Site_icao, Vol_tm.tm_year+1900,Vol_tm.tm_mon+1, Vol_tm.tm_mday, 
            Vol_tm.tm_hour,Vol_tm.tm_min, Vol_tm.tm_sec, 6);

   sprintf (ldm_key, "L2-%s/%s/%04d%02d%02d%02d%02d%02d/%d/%d/%c/V%02d/0",
            "BZIP2", Site_icao, Vol_tm.tm_year+1900,Vol_tm.tm_mon+1,
            Vol_tm.tm_mday, Vol_tm.tm_hour,Vol_tm.tm_min,
            Vol_tm.tm_sec, Volume_id, record, key_status, 6);

     /* compress the record */

     /* The BZ2 documentation on www.bzip.org states that the buffer size for 
        the compressed data should be 1% larger than the uncompressed data, 
        plus six hundred extra bytes. The buffer size has been cushioned with
        20% + 600 bytes of the uncompressed data size */

   compressed_buf = calloc (((size_t)((data_len + offset) * 1.2) + 600), 1);

   if (compressed_buf == NULL)
      MA_Abort_Task ("calloc failure");

      /* compute the BZ2 block size (block size increment = 100KB IAW BZ2 
         documentation) */

   block_size = (data_len + offset)/100000 + 1;
   if (block_size > 9)
      block_size = 9;

      /* compress the record. leave enough space for the LDM header
         (if start of volume) and 4-byte control word that incdicates
         the size of the compressed data. */

   compressed_data_len = data_len;

   ret = BZ2_bzBuffToBuffCompress (compressed_buf + offset, &compressed_data_len, 
                                   buf, data_len, block_size, 0, 30); 

   if (ret) {
       LE_send_msg (GL_INFO, 
          "BZ2 Compress error (err: %d; data record size: %d)\n", 
          ret, compressed_data_len); 
       MA_Abort_Task(NULL);
   }

      /* Populate the uncompressed data section. This fills in the LDM
         header and also the 4-byte control word that indicates the
         size of the compressed data. */

   header_len = Populate_uncompressed_data (compressed_buf, compressed_data_len, 
                                            msg_data->radial_status);

   /* Write data */

   Write_record (compressed_buf, compressed_data_len + header_len, 
                 data_len + header_len, record, msg_data->radial_status, 
                 ldm_key, local_key);

   free( compressed_buf );

   return;
}


/********************************************************************************

    Description: Construct the LDM volume header. This is called when the 
                 beginning of volume scan radial is detected and is the first
                 part of the first record for a new LDM volume scan file.

         Output: *vol_hdr    - pointer to the volume header buffer space
                 Julian_date - the computed Julian date

         Return: 

           Note: Julian_date is globally defined at the beginning of this file.
                 Volume_id is globally defined at the beginning of this file.
   
*********************************************************************************/
static void Construct_volume_hdr (char *vol_hdr) {

   time_t cur_time;      /* current UNIX time */

      /* Compute the Modified Julian Date. This equation was inherited from the 
         original convert_ldm task. It doesn't look right but has been left intact 
         since no one has complained about it */

   Julian_date = (1461 * ((Vol_tm.tm_year + 1900) + 4800 +
                  (Vol_tm.tm_mon - 13) / 12)) / 4 + (367 * (Vol_tm.tm_mon - 1 - 12
                 * ((Vol_tm.tm_mon - 13) / 12))) / 12 - (3 * (((Vol_tm.tm_year + 1900)
                 + 4900 + (Vol_tm.tm_mon - 13) / 12) / 100)) / 4 + Vol_tm.tm_mday 
                 - 32075;

      /* Subtract base year to convert from Julian date to Modified Julian date. */

   Julian_date -= BASEYR;

      /* Set the header time to msec since midnight */

   cur_time = time (NULL);
   Julian_time = (int) ((cur_time % (time_t) SECONDS_IN_A_DAY) *
                        MILLISECONDS_PER_SECOND);

      /* Construct the 12-byte LDM volume header */

   sprintf((char *)vol_hdr, "AR2V00%02d.%3.3d", 6, Volume_id);

   LE_send_msg (GL_INFO, "*********************************************");
   LE_send_msg (GL_INFO, "Archive II Label: %s", vol_hdr);
   LE_send_msg (GL_INFO, "Date: %d, Time: %d, ICAO: %s\n",  
                Julian_date, Julian_time, Site_icao);
   return;
}


#define FILE_PERMISSION    0660
#define MAX_PATH_LEN       255

/********************************************************************************

   Description: Construct and open the file for writing in "record" mode

   Input: *file_name - pointer to the filename

   Output: *err - library error code if an error is encountered

   Return: fd on success; -1 on error

*********************************************************************************/
static int Create_and_open_file (char *file_name, int *err) {

   int  i;
   mode_t mode = (mode_t) FILE_PERMISSION;
   static char full_path[MAX_PATH_LEN];
   static char working_dir[MAX_PATH_LEN];
   static char *home_dir = NULL;
   static int  fd = -1;

   /* Set error code to 0 */

   *err = 0;

   /* If file_name is empty it's an error */

   if( file_name == NULL )
   {
     LE_send_msg (GL_INFO, "File name is NULL" );
     *err = -1;
     return -1;
   }

   /* Define working directory only once */

   if( home_dir == NULL )
   {
     LE_send_msg (GL_INFO, "Home directory NULL. Define working directory." );

     /* Get HOME directory */
     if( ( home_dir = getenv( "HOME" ) ) == NULL )
     {
       LE_send_msg (GL_INFO, "Cound not get HOME directory" );
       *err = -2;
       return -1;
     }

     /* Define working directory */
     sprintf( working_dir, "%s/CHANGE_NYQUIST/", home_dir );
     LE_send_msg (GL_INFO, "Working directory: %s", working_dir );

     /* Make sure working directory exists */

     if( opendir( working_dir ) == NULL )
     {
       if( errno != ENOENT )
       {
         /* Any error besides "directory doesn't exist" ends here */
         LE_send_msg (GL_INFO, "Opendir error: %s", strerror( errno ) );
         *err = errno;
         return -1;
       }
       else if( mkdir( working_dir, 0755 ) < 0 )
       {
         /* Error creating directory */
         LE_send_msg (GL_INFO, "Mkdir error: %s", strerror( errno ) );
         *err = errno;
         return -1;
       }
       else if( opendir( working_dir ) == NULL )
       {
         /* Successfully created directory, but still can't open it */
         LE_send_msg (GL_INFO, "Opendir error: %s", strerror( errno ) );
         home_dir = NULL;
         *err = errno;
         return -1;
       }
     }
   }

   /* Replace all "/" in file name with "_". */

   for (i = 0; i < strlen(file_name); i++) {
        if (file_name[i] == '/')
           file_name[i] = '_';
   }

   /* Clear the full_path variable and redefine it */

   memset (full_path, 0, MAX_PATH_LEN);
   sprintf( full_path, "%s/%s", working_dir, file_name);

      /* Open the file. */ 

   if ((fd = open (full_path, O_CREAT | O_RDWR, mode)) < 0) {
        *err = errno;
        LE_send_msg (GL_ERROR, "Open(%s) Failed: %d\n", full_path, fd);
   } else
        LE_send_msg (GL_INFO, "Opened File %s With Write Permission\n", 
                   full_path);

   return fd;
}


/********************************************************************************

    Description: Populate the LDM record with the information that consists of the
                 record header which is excluded from data compression

    Input: *data_buffer - pointer to the compressed record buffer
           data_len     - the compressed record length in bytes
           rad_status   - the status of the current radial
           Julian_date  - the Julian date (global)
           Julian_time  - the Julian time (global)

    Output: *data_buffer - the record header

    Return: header_len   - the length of the record header

    Note: Julian_date is globally defined at the beginning of this file.
          Julian_time is globally defined at the beginning of this file.
   
*********************************************************************************/
static int Populate_uncompressed_data (char *data_buffer, int data_len, 
                                       int rad_status) {

   uint temp_u;  /* temp unsigned int */
   int  temp_s;  /* temp signed int */
   int  header_len = 0;


      /* if the data length is 0, then generate a special header for this
         record */

   if (data_len == 0) {  /* NOTE: in the original convert_ldm, this was only checked 
                                  if it was BEG_VOL which I think is wrong, so I am 
                                  now checking every record */
      temp_s = -8; /* set data length to -8 bytes */
      temp_u = INT_BSWAP_L (temp_s);
      memcpy (data_buffer, &temp_u, 4);

      temp_u = INT_BSWAP_L (Julian_date);
      memcpy (data_buffer + 4, &temp_u, 4);

      temp_u = INT_BSWAP_L (Julian_time);
      memcpy (data_buffer + 8, &temp_u, 4);

      header_len = 12;

      LE_send_msg (GL_ERROR, "Error...0 byte length record processed");

   } else if (rad_status == BEG_VOL) { /* Generate the new Volume Header string */
      /* First 12 bytes of new level-II file has format
         AR2V00XX.YYY where XX is version of level-II data
         being distributed and YYY is volume number. */
      Construct_volume_hdr (data_buffer);

      /* Next 4 bytes is modified Julian date */
      temp_u = INT_BSWAP_L (Julian_date);
      memcpy (data_buffer + 12, &temp_u, 4);  /* skip past the Volume Hdr */

      /* Next 4 bytes is milliseconds past midnight */
      temp_u= INT_BSWAP_L (Julian_time);
      memcpy (data_buffer + 16, &temp_u, 4);

      /* Next 4 bytes is 4-letter ICAO */
      strncpy (data_buffer + 20, Site_icao, 4);

      /* Next 4 bytes is control word indicating record length */
      temp_u = INT_BSWAP_L (data_len);
      memcpy (data_buffer + 24, &temp_u, 4);

      header_len = 28;

   } else if (rad_status == END_VOL) {
      /* End of volume. Set control word to negative length of data block. */
      /* The negative indicates to the user this is end of volume. */
      temp_s = -data_len;
      temp_u = INT_BSWAP_L (temp_s);
      memcpy (data_buffer, &temp_u, 4);

      header_len = 4;

   } else  {
      /* Intermediate radial. Set control word to length of data block */
      temp_u = INT_BSWAP_L (data_len);
      memcpy (data_buffer, &temp_u, 4);

      header_len = 4;
   }

   return (header_len);
}

/********************************************************************************

   Description: Write data to the local data file.

   Input: key         - the local record key
          data        - pointer to the data to be written.
          size        - the size of the data, in bytes, to be written.
          radial_stat - the current radial status

   Output:

   Return: 

*********************************************************************************/
static void Write_local_data (char *key, char *data, int size, int radial_stat) {

   int status;
   int error = 0;
   static int fd = -1;

      /* Create fully-qualified path of file and open the file. */

   if (radial_stat == BEG_VOL) {
      if (fd > 0) {
         LE_send_msg (GL_ERROR, "Unexpected Start of Volume - closing local file");
         close (fd);
      }

      if ((fd = Create_and_open_file (key, &error)) < 0) {
         LE_send_msg (GL_ERROR, "Error opening local file \"%s\" (fd: %d; err: fd)",
                      key, fd, error);
         return;
      }
   }

      /* Write data to file. */

   if (fd >= 0) {
      if ((status = write( fd, data, size )) <= 0)
         LE_send_msg (GL_ERROR, "Write data of size %d to fd %d failed\n", 
                      size, fd);
   } else
      LE_send_msg (GL_INFO, "Attempting Write With No Open File\n");

      /* Close the file. */

   if(radial_stat == END_VOL && (fd >= 0)) {
      LE_send_msg (GL_INFO, "Closing local file %s\n", key);
      close (fd);
      fd = -1;
   }

   return;
}


/********************************************************************************

    Description: Determine the running mode and call the appropriate 
                 routine to perform the actual data write

    Input: *buffer          - pointer to the record to write
           compressed_len   - the compressed record length
           uncompressed_len - the uncompressed record length
           record_num       - the record number
           rad_status       - the current radial status
           ldm_key          - the LDM key for this record
           local_key        - the local/record mode key for this record

   Return: 

   Note: LDM_perfmon is globally defined at the beginning of this file.
   
* ********************************************************************************/
static void Write_record (char *buffer, int compressed_len, int uncompressed_len, 
                          int record_num, short rad_status, char *ldm_key, 
                          char *local_key) {

   /* Retreive the different types of Level 2 processing modes */

   LE_send_msg (GL_INFO,
         "Writing local data: %s, Vol ID: %d, Rec: %d, Size: (C: %d;  U: %d), Rad Stat: %d\n",
         local_key, Volume_id, record_num, compressed_len, uncompressed_len, rad_status);

   Write_local_data (local_key, buffer, compressed_len, rad_status);

   return;
}
