/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/10/30 12:30:22 $
 * $Id: query_product_print.c,v 1.3 2012/10/30 12:30:22 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */  

/* Include files */

#include <query_product_print.h>

/* Defines/enums */

/* Structures */

/* Static/global variables */

static int Debug_mode = NO;

/* Function prototypes */


/************************************************************************
 Description: Set debug mode flag.
 ************************************************************************/

void Set_debug_mode( int flag )
{
  Debug_mode = flag;
}

/************************************************************************
 Description: Get debug mode flag.
 ************************************************************************/

int Get_debug_mode()
{
  return Debug_mode;
}

/************************************************************************
 Description: Print stdout message.
 ************************************************************************/

void Print_out( const char *format, ... )
{
  char buf[1024];
  va_list arg_ptr;

  /* If no format, nothing to do. */
  if( format == NULL ){ return; }

  /* Extract print format. */
  va_start( arg_ptr, format );
  vsprintf( buf, format, arg_ptr );
  va_end( arg_ptr );

  /* Print message to stderr. */
  fprintf( stdout, "%s\n", buf );
  fflush( stdout );
}

/************************************************************************
 Description: Print error message.
 ************************************************************************/

void Print_error( const char *format, ... )
{
  char buf[1024];
  va_list arg_ptr;

  /* If no format, nothing to do. */
  if( format == NULL ){ return; }

  /* Extract print format. */
  va_start( arg_ptr, format );
  vsprintf( buf, format, arg_ptr );
  va_end( arg_ptr );

  /* Print message to stderr. */
  fprintf( stderr, "ERROR: %s\n", buf );
  fflush( stderr );
}

/************************************************************************
 Description: Print if in debug mode.
 ************************************************************************/

void Print_debug( const char *format, ... )
{
  char buf[1024];
  va_list arg_ptr;

  /* If not in debug mode, do nothing. */
  if( ! Debug_mode ){ return; }

  /* If no format, nothing to do. */
  if( format == NULL ){ return; }

  /* Extract print format. */
  va_start( arg_ptr, format );
  vsprintf( buf, format, arg_ptr );
  va_end( arg_ptr );

  /* Print message to stdout. */
  fprintf( stdout, "DEBUG: %s\n", buf );
  fflush( stdout );
}

/************************************************************************
 Description: Print RPG_LDM product header.
 ************************************************************************/

void Print_RPG_LDM_prod_hdr( char *buf, int format_flag )
{
  RPG_LDM_prod_hdr_t *hdr = (RPG_LDM_prod_hdr_t *) &buf[0];

  if( format_flag == FORMAT_HDR_NONE ){ return; }

  if( format_flag & FORMAT_PROD_HDR_TIME )
  {
    Print_out( "Timestamp:         %ld (%s)", hdr->timestamp, RPG_LDM_get_timestring( hdr->timestamp ) );
  }
  if( format_flag & FORMAT_PROD_HDR_KEY )
  {
    Print_out( "Key:               %s", hdr->key );
  }
  if( format_flag & FORMAT_PROD_HDR_FLAGS )
  {
    Print_out( "Flags:             %s", RPG_LDM_get_prod_hdr_flag_string( hdr->flags ) );
  }
  if( format_flag & FORMAT_PROD_HDR_FEEDTYPE )
  {
      Print_out( "Feed Type:       %d (%s)", hdr->feed_type, RPG_LDM_get_feed_type_string_from_code( hdr->feed_type ) );
  }
  if( format_flag & FORMAT_PROD_HDR_SEQNUM )
  {
    Print_out( "Sequence Number:   %u", hdr->seq_num );
  }
  if( format_flag & FORMAT_PROD_HDR_LENGTH )
  {
    Print_out( "Message Length:    %u bytes", hdr->data_len );
  }
  if( format_flag & FORMAT_PROD_HDR_SPARE148 )
  {
    Print_out( "Spare148           %u", hdr->spare148 );
  }
  if( format_flag & FORMAT_PROD_HDR_SPARE150 )
  {
    Print_out( "Spare150           %u", hdr->spare150 );
  }
  if( format_flag & FORMAT_PROD_HDR_SPARE152 )
  {
    Print_out( "Spare152           %u", hdr->spare152 );
  }
  if( format_flag & FORMAT_PROD_HDR_SPARE154 )
  {
    Print_out( "Spare154           %u", hdr->spare154 );
  }
}

/************************************************************************
 Description: Print RPG_LDM message header.
 ************************************************************************/

void Print_RPG_LDM_msg_hdr( char *buf, int format_flag )
{
  RPG_LDM_msg_hdr_t *hdr = (RPG_LDM_msg_hdr_t *) &buf[0];

  if( format_flag == FORMAT_HDR_NONE ){ return; }

  if( format_flag & FORMAT_MSG_HDR_CODE )
  {
    Print_out( "Code:              %u (%s)", hdr->code, RPG_LDM_get_LDM_key_string_from_index( hdr->code ) );
  }
  if( format_flag & FORMAT_MSG_HDR_SIZE )
  {
    Print_out( "Size:              %u bytes", hdr->size );
  }
  if( format_flag & FORMAT_MSG_HDR_TIME )
  {
    Print_out( "Timestamp:         %ld (%s)", hdr->timestamp, RPG_LDM_get_timestring( hdr->timestamp ) );
  }
  if( format_flag & FORMAT_MSG_HDR_SERVERMASK )
  {
    Print_out( "Server Mask:       %d (%s)", hdr->server_mask, RPG_LDM_get_server_string_from_mask( hdr->server_mask ) );
  }
  if( format_flag & FORMAT_MSG_HDR_BUILD )
  {
    Print_out( "Build:             %5.2f", (float) hdr->build/RPG_LDM_BUILD_SCALE );
  }
  if( format_flag & FORMAT_MSG_HDR_ICAO )
  {
    Print_out( "ICAO:              %c%c%c%c", hdr->ICAO[0], hdr->ICAO[1], hdr->ICAO[2], hdr->ICAO[3] );
  }
  if( format_flag & FORMAT_MSG_HDR_FLAGS )
  {
    Print_out( "Flags:             %s", RPG_LDM_get_msg_hdr_flag_string( hdr->flags ) );
  }
  if( format_flag & FORMAT_MSG_HDR_SIZEU )
  {
    Print_out( "Size(Uncomp):      %d bytes", hdr->size_uncompressed );
  }
  if( format_flag & FORMAT_MSG_HDR_SEGNUM )
  {
    Print_out( "Segment num:       %d", hdr->segment_number );
  }
  if( format_flag & FORMAT_MSG_HDR_NUMSEGS )
  {
    Print_out( "Number of segs     %d", hdr->number_of_segments );
  }
  if( format_flag & FORMAT_MSG_HDR_WMO_SIZE )
  {
    Print_out( "WMO Header Size:   %d", hdr->wmo_header_size );
  }
  if( format_flag & FORMAT_MSG_HDR_SPARE38 )
  {
    Print_out( "Spare38:           %d", hdr->spare38 );
  }
  if( format_flag & FORMAT_MSG_HDR_SPARE40 )
  {
    Print_out( "Spare40:           %d", hdr->spare40 );
  }
  if( format_flag & FORMAT_MSG_HDR_SPARE42 )
  {
    Print_out( "Spare42:           %d", hdr->spare42 );
  }
  if( format_flag & FORMAT_MSG_HDR_SPARE44 )
  {
    Print_out( "Spare44:           %d", hdr->spare44 );
  }
  if( format_flag & FORMAT_MSG_HDR_SPARE46 )
  {
    Print_out( "Spare46:           %d", hdr->spare46 );
  }
}

