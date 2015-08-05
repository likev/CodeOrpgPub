/*$(
 *======================================================================= 
 * 
 *   (c) Copyright, 2012 Massachusetts Institute of Technology.
 *       This material may be reproduced by or for the 
 *       U.S. Government pursuant to the copyright license 
 *       under the clause at 252.227-7013 (Jun. 1995).
 * 
 *
 *=======================================================================
 *
 *
 *   FILE:    create_grid_lb.c
 *
 *   AUTHOR:  Michael F Donovan
 *
 *   CREATED: Oct 15, 2011; Initial Version
 *
 *   REVISION: 
 *
 * 
 *=======================================================================
 *
 *   DESCRIPTION:
 *
 *   This file contains functions that will serialize, compress, and write
 *   external model derived grid data of type RPGP_ext_data_t into a linear
 *   buffer file for other algorithms to use.
 *
 *   FUNCTIONS:
 *
 *   int send_grid_to_lb( int ext_grid_type, RPGP_ext_data_t *ext_data )
 *   int setup_packet( unsigned char **packet, int *packet_len )
 *   int compress_packet( unsigned char *msg, int msg_len, unsigned char *new_msg, int *new_msg_len )
 *   void construct_ext_msg( int msg_len, int pack_len, nexrad_header *msg_hdr, unsigned char *ext_msg )
 *   int write_to_file( int ext_grid_type, unsigned char *env_data_msg, nexrad_header msg_hdr )
 *   int get_msg_date( short *date_of_msg, int *seconds_of_day )
 *   void int_to_char2( unsigned char *msg, short var2, int *indx )
 *   void int_to_char4( unsigned char *msg, int var4, int *indx )
 *
 *   NOTES:    
 *
 *   Most of the code functionality was adopted from public domain AWIPS C++ software
 *   acquired from ESRL (author - G. Joanne Edwards).
 *
 *
 *=======================================================================
 *$)
 */

#include <create_grid_lb.h>

/* radar specific info */
char *full_site_name;
char *radar_name;
int radar_id;
int site_id;

char date_name[16];


/******************************************************************

   Description:
      This high level function makes calls to serialize, compress, and
      write an external grid data message.

   Inputs:
      ext_grid_type - index indicating type of grid data to pack
      ext_data - filled external grid structure of type RPGP_ext_data_t

   Output:
      none

   Returns:
      A value that is true unless problems were encountered during processing.

******************************************************************/
int send_grid_to_lb( int ext_grid_type, RPGP_ext_data_t *ext_data ) {

  int packet_len = 0;
  int msg_hdr_len, compressed_len, msg_len;
  int ret;
  unsigned char *packet;
  Siteadp_adpt_t site_info;

  /* copy the external data message */
  prod = ext_data;

  /* get the radar site name */
  ORPGSITE_get_site_data( &site_info );

  /* Set the radar name field. */
  radar_name = site_info.rpg_name;
  radar_id = (int)site_info.rpg_id;
  site_id = radar_id;
  LE_send_msg( GL_INFO, "Radar site info: radar_name= %s  radar_id= %d  site_id= %d\n", radar_name, radar_id, site_id );

  /* serialize the packet */
  ret = setup_packet( &packet, &packet_len );
  if ( ret == -1 ) {
    LE_send_msg( GL_ERROR, "Unable to set up the packet\n" );
    return( -1 );
  }

  /* compress the packet, only if packet size > 0 */
  if ( packet_len <= 0 ) {
    LE_send_msg( GL_ERROR, "Packet length is <= 0, not compressing\n" );
    return( -1 );
  }

  msg_hdr_len = sizeof(nexrad_header) + sizeof(Ext_msg_hdr);

  unsigned char *env_data_msg = malloc( msg_hdr_len + packet_len );

  compressed_len = packet_len;

  ret = compress_packet( packet, packet_len, (env_data_msg + msg_hdr_len), &compressed_len );
  free( packet );
  if ( ret == -1 ) {
    LE_send_msg( GL_ERROR, "Unable to successfully compress the external data packet\n" );
    free( env_data_msg );
    return( -1 );
  }

  msg_len = compressed_len + msg_hdr_len;

  /* complete contructing the external data message by adding header information */
  nexrad_header msg_hdr;
  construct_ext_msg( msg_len, packet_len, &msg_hdr, env_data_msg );

  /* write the serialized and compressed external data message to a file */
  ret = write_to_file( ext_grid_type, env_data_msg, msg_hdr );

  if ( ret != 1 ) {
    LE_send_msg( GL_ERROR, "Unable to write grid data\n" );
    return( -1 );
  }
  
  free( env_data_msg );

  return( 1 );

/* End of send_grid_to_lb() */
}

/******************************************************************

   Description:
      Serializes and packs the RPGP_ext_data_t data into a packet 29
      data structure.

   Inputs:
      none

   Output:
      packet - packet 29 data structure
      packet_len - length of packet 29 data structure

   Returns:
      A value that is true unless problems were encountered during processing.

******************************************************************/
int setup_packet( unsigned char **packet, int *packet_len ) {

    char *serialized_data = (char *)0;
 
    /* Serialized the data pointed to by prod */
    int serial_len = RPGP_product_serialize( (void*)prod, &serialized_data );
    /*LE_send_msg( GL_INFO, "  length of serialized data= %d\n", serial_len );*/
    if ( serial_len < 0 ) {
      LE_send_msg( GL_ERROR, "Unable to serialize the grid data for site %s\n", radar_name );
      return( -1 );
    }

    /* Set up packet 29 */
    unsigned char *buf = malloc( sizeof(Packet_29) + serial_len );

    Packet_29 packet29;
    packet29.packet_code = 29;
    packet29.spare2 = 0;
    packet29.num_bytes = serial_len;

    /* Now swap the bytes of packet */
    int indx = 0;
    int_to_char2( buf, packet29.packet_code, &indx );
    int_to_char2( buf, packet29.spare2, &indx );
    int_to_char4( buf, packet29.num_bytes, &indx );
    /*LE_send_msg( GL_INFO, "  Index in packet after swapping packet header= %d\n", indx );*/

    /* Now copy over the serialized data into the packet */
    memcpy( &buf[indx], serialized_data, serial_len );

    *packet_len = indx + serial_len;
    /*LE_send_msg( GL_INFO, "  Length of packet= %d\n", *packet_len );*/

    free( serialized_data );
    *packet = buf;
    return( 1 );

/* End of setup_packet() */
}

/******************************************************************

   Description:
      Compresses the data packet using BZIP2.

   Inputs:
      msg - packet 29 data structure
      msg_len - length of packet 29 data structure

   Output:
      new_msg - compressed packet 29 data structure
      new_msg_len - length of compressed packet 29 data structure

   Returns:
      A value that is true unless problems were encountered during processing.

******************************************************************/
int compress_packet( unsigned char *msg, int msg_len, unsigned char *new_msg, int *new_msg_len ) {

    int verbosity = 0;
    int work_factor = 30;

    /*LE_send_msg( GL_INFO, "In compressPacket..., msg len= %d\n", msg_len );*/

    /* Get the 100k block size */
    int block_size_100k = msg_len / 100000;
    int rem = msg_len % 100000;
    if (rem > 0)
      block_size_100k++; 
    if ( block_size_100k <= 0 ) {
      *new_msg_len = msg_len;
      memcpy( new_msg, msg, (size_t)msg_len );
      return( 1 );
    }

    /* If the size of the raw data is greater that 1 MByte then set the max
       block_size_100k to MAX_BLOCK_SIZE.  This will cause the compression 
       algorithm to do the compression in MAX_BLOCK_SIZE chunks. */
    if ( block_size_100k > MAX_BLOCK_SIZE )
      block_size_100k = MAX_BLOCK_SIZE; 

    /* Now, compress */
    int retval = BZ2_bzBuffToBuffCompress( (char *)new_msg, (unsigned int*)new_msg_len,
                                           (char *)msg, (unsigned int)msg_len,
                                           block_size_100k, verbosity, work_factor );
    if ( retval == BZ_OK ) {
      /*LE_send_msg( GL_INFO, "  Successfully compressed the data\n" );
      LE_send_msg( GL_INFO, "  Compressed length= %d\n", *new_msg_len );
      LE_send_msg( GL_INFO, "  Uncompressed length= %d\n", msg_len );*/
      return( 1 );
    }

    LE_send_msg( GL_ERROR, "Unable to compress the data, retval= %d\n", retval );
    return( -1 );

/* End of compress_packet() */
}

/******************************************************************

   Description:
      Constructs the external grid data message.

   Inputs:
      msg_len - length of packet 29 data structure plus external message
                header
      pack_len - length of packet 29 data structure

   Output:
      msg_hdr - external message header
      ext_msg - external data message

   Returns:
      none

******************************************************************/
void construct_ext_msg( int msg_len, int pack_len, nexrad_header *msg_hdr, unsigned char *ext_msg ) {

    /* Set up the nexrad header */
    msg_hdr->msgcode = 5;
    msg_hdr->src_id = site_id;
    msg_hdr->dst_id = radar_id;
    msg_hdr->no_of_blocks = 2;
    msg_hdr->length_of_msg_mshw = (msg_len & 0xffff0000) >> 16;
    msg_hdr->length_of_msg_lshw = msg_len & 0x0000ffff;

    /* Compute message date and time */
    short date_of_msg;
    int seconds_of_day;
    get_msg_date( &date_of_msg, &seconds_of_day );

    msg_hdr->date_of_msg = date_of_msg;
    msg_hdr->time_of_msg_mshw = (seconds_of_day & 0xffff0000) >> 16;
    msg_hdr->time_of_msg_lshw = seconds_of_day & 0x0000ffff;
    /*LE_send_msg( GL_INFO, "  Date of msg: %d  Seconds of day: %d\n", msg_hdr->date_of_msg, seconds_of_day );
    LE_send_msg( GL_INFO, "  time_of_msg_mshw: %d  time_of_msg_lshw: %d\n", 
                (unsigned short)msg_hdr->time_of_msg_mshw, (unsigned short)msg_hdr->time_of_msg_lshw );*/

    /* Put the message header into ext_msg */
    int indx = 0;
    int_to_char2( ext_msg, msg_hdr->msgcode, &indx );
    int_to_char2( ext_msg, msg_hdr->date_of_msg, &indx ); 
    int_to_char2( ext_msg, msg_hdr->time_of_msg_mshw, &indx ); 
    int_to_char2( ext_msg, msg_hdr->time_of_msg_lshw, &indx ); 
    int_to_char2( ext_msg, msg_hdr->length_of_msg_mshw, &indx ); 
    int_to_char2( ext_msg, msg_hdr->length_of_msg_lshw, &indx ); 
    int_to_char2( ext_msg, msg_hdr->src_id, &indx ); 
    int_to_char2( ext_msg, msg_hdr->dst_id, &indx ); 
    int_to_char2( ext_msg, msg_hdr->no_of_blocks, &indx ); 
    /*LE_send_msg( GL_INFO, "  Index in message, after header= %d\n", indx );*/

    /* Set up the External Data Msg and swap the bytes of symbology block */
    Ext_msg_hdr ext_msg_hdr;
    ext_msg_hdr.block_div = -1;
    ext_msg_hdr.block_id = 4;
    ext_msg_hdr.spare1 = 0;
    ext_msg_hdr.comp_type = 1;
    ext_msg_hdr.decomp_size = pack_len;

    int_to_char2( ext_msg, ext_msg_hdr.block_div, &indx ); 
    int_to_char2( ext_msg, ext_msg_hdr.block_id, &indx ); 
    int_to_char2( ext_msg, ext_msg_hdr.spare1, &indx ); 
    int_to_char2( ext_msg, ext_msg_hdr.comp_type, &indx ); 
    int_to_char4( ext_msg, ext_msg_hdr.decomp_size, &indx ); 
    /*LE_send_msg( GL_INFO, "  Index in msg after swapping ext msg hdr= %d\n", indx );*/

    return;

/* End of construct_ext_msg() */
}

/******************************************************************

   Description:
      Writes the compressed data message to a linear buffer data file.

   Inputs:
      ext_grid_type - index indicating type of grid data to pack
      env_data_msg - external data message
      msg_hdr - external message header

   Output:
      none

   Returns:
      A value that is true unless problems were encountered during processing.

******************************************************************/
int write_to_file( int ext_grid_type, unsigned char *env_data_msg, nexrad_header msg_hdr ) {

    unsigned short msw, lsw;
    unsigned int msg_length;
    int ret;

    msw = msg_hdr.length_of_msg_mshw;
    lsw = msg_hdr.length_of_msg_lshw;
    /*LE_send_msg( GL_INFO, "  MSW: %d  LSW: %d\n", msw, lsw );*/

    msg_length = (unsigned int) (0xffff0000 & ((msw) << 16)) | ((lsw) & 0xffff);

    if ( ext_grid_type == 1 ) {
      ret = RPGC_data_access_write( MODEL_FRZ_GRID, env_data_msg, msg_length, MODEL_FRZ_ID );
      LE_send_msg( GL_INFO, "FRZ Grid Message length= %d\n", msg_length );
    }
    else {
      ret = RPGC_data_access_write( MODEL_CIP_GRID, env_data_msg, msg_length, MODEL_CIP_ID );
      LE_send_msg( GL_INFO, "CIP Grid Message length= %d\n", msg_length );
    }

    if ( ret <= 0 ) {
      LE_send_msg( GL_ERROR, "Failed to write grid data for type %d; return val= %d\n", ext_grid_type, ret );
      return( -1 );
    }

    return( 1 );

/* End of write_to_file() */
}

/******************************************************************

   Description:
      Generates the current date in days since 1/1/70 and 
      current time in seconds since midnight.

   Inputs:
      none

   Output:
      date_of_msg - number of days since 1/1/70 using NEXRAD modified
                    julian date
      seconds_of_day - number of seconds since midnight

   Returns:
      A value that is true unless problems were encountered during processing.

******************************************************************/
int get_msg_date( short *date_of_msg, int *seconds_of_day ) {

    int current_time, julian_date;
    int yy, mm, dd, hh, mn, ss;
    int ret;
    time_t epoch_time = time( NULL );

    /* get current time, convert to unix time, and set to gen_time */
    ret = RPGCS_get_date_time( &current_time, &julian_date );
    hh = (int)( current_time / SECS_IN_HOUR );
    mn = (int)( (current_time - (hh * SECS_IN_HOUR)) / 60 );
    ss = current_time - (hh * SECS_IN_HOUR) - (mn * 60);

    ret = RPGCS_julian_to_date( julian_date, &yy, &mm, &dd );

    ret = RPGCS_ymdhms_to_unix_time( &epoch_time, yy, mm, dd, hh, mn, ss );

    /*LE_send_msg( GL_INFO, "  Current EPOCH Time: %d\n", (unsigned int)epoch_time );
    sprintf( date_name, "%04d%02d%02d_%02d%02d%02d", yy, mm, dd, hh, mn, ss );
    LE_send_msg( GL_INFO, "Current date_time: %s\n", date_name );*/

    *seconds_of_day = current_time;
    *date_of_msg = julian_date;

    /*LE_send_msg( GL_INFO, "  Days since 1/1/70: %d\n", (int)((unsigned int)epoch_time / 86400) );
    LE_send_msg( GL_INFO, "  Seconds since midnight: %d\n", *seconds_of_day );
    LE_send_msg( GL_INFO, "  Using NEXRAD modified julian date: %d\n", *date_of_msg );*/

    return( 1 );

/* End of get_msg_date() */
}

/******************************************************************

   Description:
      Converts var2 to a char[2] and puts it into msg with bytes swapped
      starting at indx.

   Inputs:
      msg - current message
      var2 - variable of type short
      indx - current index of msg

   Output:
      msg - updated message
      indx - updated index value

   Returns:
      none

******************************************************************/
void int_to_char2( unsigned char *msg, short var2, int *indx ) {

    unsigned char *cptr;

    cptr = (unsigned char *)&var2;
    msg[*indx] = cptr[1];
    msg[*indx+1]= cptr[0];
    *indx +=2;

    return;

/* End of int_to_char2() */
}

/******************************************************************

   Description:
      Converts var4 to a char[4] and puts it into msg with bytes swapped
      starting at indx.

   Inputs:
      msg - current message
      var4 - variable of type int
      indx - current index of msg

   Output:
      msg - updated message
      indx - updated index value

   Returns:
      none

******************************************************************/
void int_to_char4( unsigned char *msg, int var4, int *indx ) {

    unsigned char *cptr;

    cptr = (unsigned char *)&var4;
    msg[*indx] = cptr[3];
    msg[*indx+1] = cptr[2];
    msg[*indx+2] = cptr[1];
    msg[*indx+3] = cptr[0];
    *indx += 4;

    return;

/* End of int_to_char4() */
}
