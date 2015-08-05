/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/02/19 21:19:22 $
 * $Id: process_data.c,v 1.10 2013/02/19 21:19:22 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

#include <read_is.h>
#include <netinet/in.h>
#include <bzlib.h>
#include <generic_basedata.h>


/* Local Defines. */
#define FILE_HEADER_SIZE	24
#define MESSAGE_SIZE		2432
#define BASEYR                  2440587

/* Global Variables. */
int Cr_buffer_done;
extern Current_label_t Clabel;

/* Static Local Variables. */
static unsigned int Bytes_read;
static char *Buffer_addr = NULL;
static int Seg_size = 0;
static int Wait_ms = 20;


/* Function Prototypes. */
static int Process_message( char *buffer, int n_bytes );
static void Print_fields( int type, int size, int n_segs, 
                          int seg_num );
static int Is_empty_message( int n_segs, int seg_num );
static void Output_data( int size, char *buffer );
static int Decompress_and_process_file( char *buffer, int offset, int size, 
                                        FileStruct_t *file );

/**************************************************************************

   Description:
      Opens and reads current file, checks file header and then reads each
      NCDC message and passes it to Process_message.

   Inputs:
      data - memory structure holding the radar data
      file - file structure holding information about the file being
             processed.

   Returns:
      The total number of compressed bytes of the file that have been
      processed.   Returns -1 on fatal error.

   Notes:
      This code was modified from read_ldm.c

**************************************************************************/
int Process_current_buffer( MemoryStruct_t *data, FileStruct_t *file ){

   char label[32];
   int date, time, year, month, day;
   int hours, minutes, seconds;
   int bytes_processed;
   time_t time_value;

   static int hd_processed = 0;
   static int bufptr = 0;
   static char radar_id[8] = "";

   if( (Buffer_addr == NULL) 
              ||
       (Cr_buffer_done) ){

      Cr_buffer_done = 0;
      bufptr = 0;

      /* New buffer to process. */
      Buffer_addr = (char *) data;
      if( Buffer_addr == NULL ){

         Cr_buffer_done = 1;

         if( Verbose )
            fprintf( stderr, "NULL Buffer in Process_current_buffer()\n" ); 

	 return bufptr;

      }

      Bytes_read = hd_processed = 0;

   }

   if( !hd_processed ){

      if( Bytes_read < FILE_HEADER_SIZE ){	/* read the file header */

         if( data->size < FILE_HEADER_SIZE ){

            if( Verbose )
               fprintf( stderr, "Read file header failed( file %s)\n", 
                        &file->filename[0] );

            return bufptr;

         }

         Bytes_read += FILE_HEADER_SIZE;

      }
    
      if( Bytes_read >= FILE_HEADER_SIZE ){	/* check file header */

         if( (strncmp( data->memory, "ARCHIVE2.", 9) != 0)
			 &&
	     (strncmp( data->memory, "AR2", 3) != 0) ){

            if( Verbose ){

               char header[10];
               char *hdr = NULL;

               memset( header, 0, 10 );
               memcpy( header, data->memory, 9 );
               fprintf( stderr, "File header check failed (file %s): %s\n", 
                        &file->filename[0], header );

               hdr = strstr( data->memory, "AR" );
               if( hdr != NULL )
                  fprintf( stderr, "--->File header: %s @ %p, memory @ %p\n",
                           hdr, hdr, data->memory ); 

            }

            return bufptr;

         }

         /* Write out information about this file. */
         date = ntohl( *((int *)(data->memory + 12)) );
         time = ntohl( *((int *)(data->memory + 16)) );
         strncpy( label, data->memory, 12 );
         label[12] = '\0';
         strncpy ( radar_id, data->memory + 20, 4 );
         radar_id[4] = '\0';
         time_value = ((date-1)*86400) + (time/1000);
         unix_time( &time_value, &year, &month, &day,
                    &hours, &minutes, &seconds );

         fprintf( stderr, 
             "File header: %s, %02d/%02d/%02d %02d:%02d:%02d, Radar ID: %s\n", 
             label, month, day, year, hours, minutes, seconds, radar_id );

         Clabel.year = year;
         Clabel.month = month;
         Clabel.day = day;
         Clabel.hour = hours;
         Clabel.minute = minutes;
         Clabel.second = seconds;
         memcpy( &Clabel.label[0], &label[0], 32 );
         memcpy( &Clabel.radar_id[0], &radar_id[0], 8 );

	 Bytes_read = 0;
	 hd_processed = 1;
	 Seg_size = 0;
	 bufptr = FILE_HEADER_SIZE;

      }
      else
         return bufptr;

   }

   /* Do some sanity checks ... I'll figure out how to handle later. */
   if( bufptr > data->size )
      fprintf( stderr, "data->size: %d is SMALLER than bufptr: %d ????\n",
               data->size, bufptr );

   /* Decompress and process the data. */
   bytes_processed = Decompress_and_process_file( data->memory, bufptr, 
	                                          data->size, file );

   if( Verbose ){

      if( bytes_processed == 0 )
         fprintf( stderr, "Decompress_and_process_file Returns 0\n" );

   }

   /* On success .... */
   if( bytes_processed >= 0 )
      bufptr += bytes_processed;

   /* On failure ... */
   else{

      if( Verbose )
         fprintf( stderr, "Error: Decompress_and_process_file() Returned: %d\n",
                  bytes_processed );
      return(-1);

   }

   return bufptr;

} /* End of Process_current_buffer(). */

/**************************************************************************

   Description:
      Reads buffer, decompress the data and processes the data.

   Inputs:
      buffer - buffer holding the compressed data.
      offset - current offset into the buffer where data should be read 
               from.
      size - total size of the buffer, i.e., how much data currently in
             the buffer.
      file - structure holding file name.

   Returns:
      The total number of compressed bytes processed this call.  Returns
      -1 on error. 

**************************************************************************/
static int Decompress_and_process_file( char *buffer, int offset, int size, 
                                        FileStruct_t *file ){

   char *data = buffer + offset;
   int bufptr = 0;
   int ret;

   while(1){

      /* After a segment has been processed, Seg_size is set to 0.  Seg_size
         is also set to 0 on nonrecoverable error. */
      if( Seg_size <= 0 ){ 

         /* Read the segment size.  The segment size is the first 4 bytes
            of the segment. */
         if( Bytes_read < 4 ){

            /* Check to make sure there is enough remaining space 
               in the buffer to read the segment size. */
            if( size < (offset + (4 - Bytes_read)) )
               return bufptr;

            /* Increment the buffer pointers by length of the size field. */
            Bytes_read += (4 - Bytes_read);
	    bufptr += (4 - Bytes_read);

         }

         if( Bytes_read >= 4 ){

            Seg_size = *((int *)((char *) data));
#ifdef LITTLE_ENDIAN_MACHINE
            Seg_size = INT_BSWAP (Seg_size);
#endif
            Bytes_read = 0;
            if( Seg_size < 0 )
               Seg_size = -Seg_size;

            if( (Seg_size <= 0)
                     || 
                (Seg_size >= 10000000) ){

               if( Verbose )
                  fprintf( stderr, "Invalid Seg_size: %d\n", Seg_size );

               /* Cleanup ..... */
               Cr_buffer_done = 1;
               Seg_size = 0;
               return -1;

	    }

         }
         else
            return bufptr;

      }

      if( Seg_size > 0 ){

         static char *dest = NULL;
         static int dest_bsize = 500000;
         int dest_len, bytes_processed;

         if( Bytes_read < Seg_size ){

            /* Is there enough data in the file to process this
               segment? */
            if( (offset + 4 + Seg_size) > size ){

                 if( Verbose )
		    fprintf( stderr, "offset (%d) + Seg_size (%d) + 4 > size: %d\n", 
                             offset, Seg_size, size );

               return bufptr;

	    }

            Bytes_read += Seg_size;

         }

         while(1){

            /* Is there enough data in the file to process this
               segment? */
            if( (offset + 4 + Seg_size) > size ){

               if( Verbose )
		  fprintf( stderr, "offset: %d, Seg_size: %d, size: %d\n",
	                   offset, Seg_size, size );

	       return bufptr;

	    }
	    
            if( dest == NULL )
               dest = MISC_malloc( dest_bsize );

            dest_len = dest_bsize;
            ret = BZ2_bzBuffToBuffDecompress( dest, (unsigned int *)&dest_len, 
                                              (char *) data + 4, Seg_size, 0, 0 );

            if( ret == BZ_OUTBUFF_FULL ){

               if( dest != NULL )
                  free( dest );

               dest_bsize *= 2;
               dest = MISC_malloc( dest_bsize );
               continue;

            }
            else  if( ret < 0 ){

               if( Verbose )
                  fprintf( stderr, "BZ2 decompress failed (file %s), returns %d\n", 
                           &file->filename[0], ret);
               
               /* Cleanup .... */
               Cr_buffer_done = 1;
               Seg_size = 0;
               return -1;

            }

            /* Move over this segment. */
	    data += (Seg_size + 4);
	    offset += (Seg_size + 4);
	    bufptr += (Seg_size + 4);

            break;

         }

         Bytes_read = dest_len;
         bytes_processed = 0;
         while( Bytes_read > bytes_processed ){

            bytes_processed += Process_message( (char *)dest + bytes_processed, 
                                                Bytes_read - bytes_processed );

         }

         Bytes_read = 0;
         Seg_size = 0;

      }

   }

   return bufptr;

} /* End of Decompress_and_process_file() */ 

/**************************************************************************

   Description:
      Process message in "buffer".  Calls Output_message to write message
      to LB.  Sets flag when end of volume encountered.

   Input:
      buffer - buffer holding the message.
      n_bytes - size of the message.

   Returns:
      Size of the message processed.

**************************************************************************/
static int Process_message( char *buffer, int n_bytes ){

   short size, type, *spt, n_segs, seg_num;
   unsigned char *cpt;

   spt = (short *)buffer;
   cpt = (unsigned char *)buffer;
   size = spt[6];
   n_segs = spt[12];
   seg_num = spt[13];
#ifdef LITTLE_ENDIAN_MACHINE
   size = SHORT_BSWAP( size );
   n_segs = SHORT_BSWAP( n_segs );
   seg_num = SHORT_BSWAP( seg_num );
#endif
   type = cpt[15];
   size *= 2;

   if( type != 31 ){

      if( size > MESSAGE_SIZE ){

         fprintf( stderr, "Message size larger than expected (%d)\n", MESSAGE_SIZE);
	 Print_fields( type, size, n_segs, seg_num );
	 return( MESSAGE_SIZE );

      }

      if( n_bytes < MESSAGE_SIZE ){

         
         fprintf( stderr, "Data bytes (%d) less than expected (%d)\n", 
                  n_bytes, MESSAGE_SIZE );
         Print_fields( type, size, n_segs, seg_num );
	 return( n_bytes );

      }

   }
   else{

      if( n_bytes < size ){

         fprintf( stderr, "Data bytes (%d) less than expected (%d)\n", 
                  n_bytes, size );
	 Print_fields (type, size, n_segs, seg_num);
	 return( n_bytes );

      }

   }

   if (type == 0)			/* unused message - discarded */
      return( MESSAGE_SIZE );

   if( (type <= 0) || (type > 50) ){	/* not a NCDC message */

      fprintf( stderr, "Unexpected data type (%d)\n", type);
      Print_fields (type, size, n_segs, seg_num);
      return( MESSAGE_SIZE );

   }

   if( type == RDA_STATUS_DATA ){

      /* RDA status data */
      if( size != 96 ){

         fprintf( stderr, "Unexpected RDA status size (%d)\n", size);
         return (MESSAGE_SIZE);

      }

      Output_data (size, buffer);
      return (MESSAGE_SIZE);

   }

   if( type == 31 ){

      Generic_basedata_t *hdr = 
                       (Generic_basedata_t *) &spt[6];
      if( hdr->base.status == GENDVOL ){

         fprintf( stderr, "End of volume detected\n" );
         Cr_buffer_done = 1;
	 Buffer_addr = NULL;

      }

      Output_data (size, buffer);

      return (size + 12);

   }

   if( !Is_empty_message (n_segs, seg_num) )
      Output_data( MESSAGE_SIZE, buffer );
 
   return( MESSAGE_SIZE );

} /* End of Process_message(). */

/******************************************************************

   Description:
      Called when something unexpected happens when processing 
      a message.   This function simply outputs a message to
      stderr providing information about the message.

   Inputs:
      type - message type.
      size - message size.
      n_segs - number of segments in the message.
      seg_num - segment number within the message.

   Returns:
      There is no return value defined.  

******************************************************************/
static void Print_fields( int type, int size, int n_segs, 
		          int seg_num ){

   fprintf( stderr, "    type = %d  size = %d  n_segs = %d  seg_num = %d\n",
            type, size, n_segs, seg_num );

} /* End of Print_fields(). */

/******************************************************************

   Description:
      Checks if message of "seg_num" is an empty message. The LDM 
      ICD assumes maximum sizes for certain messages. If a message 
      is smaller than its maximum size, returns 0.  Otherwise 
      returns 1.

******************************************************************/
static int Is_empty_message( int n_segs, int seg_num ){

   static int cr_num_segs = 0;

   if (seg_num == 1)
      cr_num_segs = n_segs;

   if (cr_num_segs > 0 && seg_num > cr_num_segs)
      return (1);

   return (0);

} /* End of Is_empty_message(). */

/**************************************************************************

   Description:
      Writes data of size "size" to the output LB. The 12 byte CTM header
      is removed.

   Inputs:
      size - size of the data to write, in bytes.
      buffer - buffer holding data to write.

   Returns:
      There is no return value defined for this function.

**************************************************************************/
static void Output_data( int size, char *buffer ){

   static int cnt = 0;
   int ret;

   ret = LB_write( LB_fd, (char *)buffer + 12, size, LB_NEXT );
   if( ret < 0 ){

      fprintf( stderr, "LB_write data failed (%d)\n", ret );
      return;

   }
   
   /* Increment the number of messages written to LB. */
   cnt++;

    if( (cnt % 300) == 1 )
	LE_send_msg (0, "%d messages written to LB\n", cnt);

    if (Wait_ms > 0)
	msleep (Wait_ms);

    return;
} /* End of Output_data(). */
