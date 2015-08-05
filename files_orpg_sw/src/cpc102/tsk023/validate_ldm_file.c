/************************************************************************
 *                                                                      *
 *      Module:  validate_ldm_file.c                                    *
 *                                                                      *
 *      Description:  Validates integrity of level-II data file.        *
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/05/25 15:18:08 $
 * $Id: validate_ldm_file.c,v 1.1 2012/05/25 15:18:08 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/* Include files. */

#include <orpg.h>
#include <bzlib.h>
#include <netinet/in.h>

/* Defines/enums. */

enum { NO, YES };

#define	MAX_ICAO_LEN			5
#define	DATA_READ_TIMEOUT		60
#define	LDM_HEADER_SIZE			24
#define	MAX_PRINT_MSG_LEN		512
#define	METADATA_MSG_SIZE		2432
#define	SECONDS_PER_DAY			86400
#define	BITS_PER_BYTE			8
#define	BYTES_PER_KILOBYTE		1024
#define	BYTES_PER_MEGABYTE		BYTES_PER_KILOBYTE*BYTES_PER_KILOBYTE
#define	MILLISECONDS_PER_SECOND		1000
#define	RDA_STATUS_MSG			2
#define	RDA_RADIAL_MSG			31
#define	RVOL_TAG			"RVOL"
#define	RELV_TAG			"RELV"
#define	RRAD_TAG			"RRAD"
#define	DREF_TAG			"DREF"
#define	DVEL_TAG			"DVEL"
#define	DSW_TAG				"DSW "
#define	DZDR_TAG			"DZDR"
#define	DPHI_TAG			"DPHI"
#define	DRHO_TAG			"DRHO"
#define	LDM_CONTROL_WORD_SIZE		4
#define	MAX_LDM_BLOCK_SIZE		2000000
#define	READ_STDIN_FAILED		-4
#define	ALARM_INIT_FAILED		-5
#define	READ_ALARM_TIMEOUT		3
#define	EPOCH_INIT			0
#define	VCP_INIT			0
#define	EXIT_SUCCESS			0
#define	READ_STDIN_TIMEOUT		-10
#define	INVALID_CMDLINE_ARGUMENT	-11
#define	INVALID_LDM_HEADER		-12
#define	MISSING_LDM_HEADER		-13
#define	INSUFFICIENT_STDIN_BYTES_READ	-14
#define	DECOMPRESS_ERROR		-15
#define	SIGALARM_CALLBACK_FAILED	-16
#define	MISSING_START_OF_VOLUME_RADIAL	-17
#define	INVALID_DATA_BLOCK_TYPE		-18
#define	INVALID_DATA_BLOCK_NAME		-19
#define	INVALID_MOMENT_NAME		-20
#define	NO_END_OF_VOLUME_FOUND		-21

/* Static/global variables. */

static char         Read_buf[MAX_LDM_BLOCK_SIZE];
static char         ICAO_buf[MAX_ICAO_LEN] = "????\0";
static char         Uncompressed_buf[MAX_LDM_BLOCK_SIZE];
static unsigned int Uncompressed_length = 0;
static unsigned int Uncompressed_bytes = 0;
static unsigned int Compressed_bytes = 0;
static float        Compression_ratio = 0.0;
static int          Control_word = 0;
static int          End_of_volume_flag = NO;
static int          Space_output = NO;
static int          Debug_mode = NO;
static int          Read_stdin_alarm_timeout_flag = NO;
static int          Read_stdin_alarm_init_flag = NO;
static time_t       Volume_start_epoch = EPOCH_INIT;
static time_t       Volume_end_epoch = EPOCH_INIT;
static int          Volume_duration = 0;
static int          Number_of_elevations = 0;
static int          Number_of_radials = 0;
static int          VCP = 0;
static int          DP_flag = NO;
static int          Total_size = 0;

/* Function prototypes. */

static void Parse_command_line( int, char *[] );
static void Print_usage( char * );
static void Set_signals();
static void Signal_handler( int );
static void Check_for_start_of_volume();
static void Read_compressed_record();
static void Read_meta_data();
static void Read_radial_data();
static void Process_radial( char * );
static int  Read_stdin( char *, int );
static void Read_control_word();
static void P_out( const char *, ... );
static void P_err( const char *, ... );
static void P_debug( const char *, ... );
static void Read_stdin_alarm_handler();
static int  Init_read_stdin_alarm();
static void Byte_swap_msg_hdr( char * );
static void Print_stats( int );

/************************************************************************
 Description: This is the main function for the level-II stats decoder.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  char *buf = NULL;
  time_t initial_read_time = 0;

  /* Parse command line. */

  Parse_command_line( argc, argv );

  /* Set up signal handlers */

  Set_signals();

  /* Set input stream (stdin) to be unbuffered. */

  setvbuf( stdin, buf, _IONBF, sizeof( _IONBF ) );

  /* Loop/read data. The format of a level-II file is as follows:
     24 byte volume header -
           bytes 00-08 - file name (AR2V00xx. where xx is LDM version)
           bytes 09-11 - volume number sequence (001-999)
           bytes 12-15 - volume start date
           bytes 16-19 - volume start time (milliseconds after midnight)
           bytes 20-23 - ICAO
      4 byte flag indicating length compressed meta data record
      compressed meta data record
      4 byte flag indicating length of compressed radials record
      compressed radials record
      4 byte flag indicating length of compressed radials record
      compressed radials record
      .
      .
      .
  */

  /* Note initial start time. */

  initial_read_time = time( NULL );

  /* Read the initial control word. */

  Read_control_word();
  P_debug( "control word 1: %d", Control_word );

  /* Ensure this is the start of a new volume file. If not, bail. */

  Check_for_start_of_volume();

  /* Read meta data (first record of new volume file). */

  Read_meta_data();

  /* Loop and read rest of volume file. */

  while( End_of_volume_flag == NO )
  {
    /* Sanity check. If it takes too long, assume something
       bad has happened and bail. */
    if( time( NULL ) - initial_read_time > DATA_READ_TIMEOUT )
    {
      P_err( "Read timeout (>%d). Exiting.", DATA_READ_TIMEOUT );
      Print_stats( READ_STDIN_TIMEOUT );
    }

    /* Read control word that tells us what to do. */
    Read_control_word();
    P_debug( "control word 2: %d", Control_word );

    Read_radial_data();
  }

  Print_stats( EXIT_SUCCESS );

  return 0;
}

/************************************************************************
 Description: Print relevant stats.
 ************************************************************************/

static void Print_stats( int exit_code )
{
  float through_put = 0.0;

  if( Volume_duration > 0.0 )
  {
    through_put = ( ((float) Total_size / BYTES_PER_KILOBYTE ) * BITS_PER_BYTE ) / Volume_duration;
  }

  if( Volume_end_epoch == EPOCH_INIT && exit_code == EXIT_SUCCESS )
  {
    exit_code = NO_END_OF_VOLUME_FOUND;
  }

  if( Space_output == NO )
  {
    /* For non-spaced, csv format */
    P_out( "%s,%ld,%ld,%d,%d,%d,%d,%d,%d,%d,%f,%d,%f", ICAO_buf, Volume_start_epoch, Volume_end_epoch, VCP, DP_flag, Volume_duration, Number_of_elevations, Number_of_radials, Compressed_bytes, Uncompressed_bytes, Compression_ratio, Total_size, through_put );
  }
  else
  {
    /* For spaced, csv format */
    P_out( "%4s,%10ld,%10ld,%3d,%1d,%3d,%2d,%5d,%8d,%8d,%6.2f,%8d,%6.2f", ICAO_buf, Volume_start_epoch, Volume_end_epoch, VCP, DP_flag, Volume_duration, Number_of_elevations, Number_of_radials, Compressed_bytes, Uncompressed_bytes, Compression_ratio, Total_size, through_put );
  } 

  exit( exit_code );
}

/************************************************************************
 Description: Parse command line and take appropriate action.
 ************************************************************************/

static void Parse_command_line( int argc, char *argv[] )
{
  extern int optind;
  extern int opterr;
  extern char *optarg;
  int ch = -1;

  opterr = 1;
  while( ( ch = getopt( argc, argv, "hsx" ) ) != EOF )
  {
    switch( ch )
    {
      case 'h':
        Print_usage( argv[0] );
        exit( EXIT_SUCCESS );
      case 's':
        Space_output = YES;
        break;
      case 'x':
        Debug_mode = YES;
        break;
      case '?':
        Print_usage( argv[0] );
        exit( INVALID_CMDLINE_ARGUMENT );
        break;
    }
  }
}

/************************************************************************
 Description: Print usage of this task.
 ************************************************************************/

static void Print_usage( char *av0 )
{
  fprintf( stderr, "Usage: %s [options]\t\n", av0 );
  fprintf( stderr, "Note:\n" );
  fprintf( stderr, "  LDM file read from stdin (i.e. cat LDM file to pipe)\n" );
  fprintf( stderr, "Options:\n" );
  fprintf( stderr, "  -h      - print usage\n" );
  fprintf( stderr, "  -x      - debug mode, (Default: no)\n" );
  fprintf( stderr, "\n");
  fprintf( stderr, "The following signals are caught:\n");
  fprintf( stderr, "SIGINT,SIGTERM - terminate\n");
  fprintf( stderr, "SIGPIPE - ignore\n");
}

/************************************************************************
 Description: Set signal handlers.
 ************************************************************************/

static void Set_signals()
{
  signal( SIGPIPE, Signal_handler );
  signal( SIGINT, Signal_handler );
  signal( SIGTERM, Signal_handler );
}

/************************************************************************
 Description: Handle signals.
 ************************************************************************/

static void Signal_handler( int sig )
{
  switch( sig )
  {
    case SIGPIPE :
      P_debug( "signal SIGPIPE received...ignore" );
      return;
      break;
    case SIGINT :
      P_debug( "signal SIGINT received...terminate" );
      Print_stats( EXIT_SUCCESS );
      break;
    case SIGTERM :
      P_debug( "signal SIGTERM received...terminate" );
      Print_stats( EXIT_SUCCESS );
      break;
    default :
      P_debug( "Signal_handler: unhandled signal: %d", sig );
  }
}

/************************************************************************
 Description: Check if at start of level-II volume file.
 ************************************************************************/

static void Check_for_start_of_volume()
{
  int bytes_read = 0;
  int ignore_bytes = 0;
  char ignore_buf[LDM_HEADER_SIZE];

  /* If this is the beginning of a level-II file, then the 4-bytes
     just read is the beginning of the LDM volume header and not the
     record length. If this is the case, read and ignore the header. */

  if( ! strncmp( (char *)&Control_word, "AR", 2 ) )
  {
    ignore_bytes = LDM_HEADER_SIZE - LDM_CONTROL_WORD_SIZE;
    bytes_read = Read_stdin( (char *)ignore_buf, ignore_bytes );
    if( bytes_read != ignore_bytes )
    {
      P_err( "Missing volume file header (%d bytes read)", bytes_read );
      Print_stats( INVALID_LDM_HEADER );
    }
    memcpy( ICAO_buf, &ignore_buf[16], 4 );
    ICAO_buf[MAX_ICAO_LEN-1] = '\0';
  }
  else
  {
    P_err( "Missing volume file header (%d bytes read)", bytes_read );
    Print_stats( MISSING_LDM_HEADER );
  }
}

/************************************************************************
 Description: Read the compressed record of size size_of_record.
 ************************************************************************/

static void Read_compressed_record()
{
  int bytes_to_read = 0;
  int bytes_read = 0;
  int error = 0;
  int size_of_record = Control_word;

  /* Byte-swap size flag so we can use it. */

  bytes_to_read = ntohl( size_of_record );

  P_debug( "Read_compressed_record: read %d", bytes_to_read );

  /* Negative size indicates this is the last record of the volume. */

  if( bytes_to_read < 0 )
  {
    P_debug( "Read_compressed_record: EOV" );
    bytes_to_read = -bytes_to_read;
    End_of_volume_flag = YES;
  }

  /* Read in compressed record according to expected size. If fewer
     bytes are read than expected, that's a problem. */

  bytes_read = Read_stdin( (char *)Read_buf, bytes_to_read );

  if( bytes_read != bytes_to_read )
  {
    P_err( "Read_compressed_record: Bytes read (%d) less than (%d)",
               bytes_read, bytes_to_read );
    Print_stats( INSUFFICIENT_STDIN_BYTES_READ );
  }

  /* Decompress the compressed record and put in different buffer. */

  Uncompressed_length = sizeof( Uncompressed_buf );

  error = BZ2_bzBuffToBuffDecompress( Uncompressed_buf, &Uncompressed_length,
                                      Read_buf, bytes_read,
                                      0, 1 );
  if( error )
  {
    P_err( "Read_compressed_record: Decompress error - %d", error );
    Print_stats( DECOMPRESS_ERROR );
  }

  Uncompressed_bytes += Uncompressed_length;
  Compressed_bytes += bytes_read;
  Compression_ratio = (float) Uncompressed_bytes / (float) Compressed_bytes;
  P_debug( "Radial size - Comp: %d Uncomp: %d Ratio: %f", Compressed_bytes, Uncompressed_bytes, Compression_ratio );
}

/************************************************************************
 Description: Read and handle meta data.
 ************************************************************************/

static void Read_meta_data()
{
  int   buf_offset = 0;
  
  /* Read control word to get size of compressed record to read. */

  Read_control_word();
  P_debug( "control word for meta data: %d", Control_word );

  Read_compressed_record();

  /* Skip initial 12 bytes (comms manager header). */

  buf_offset = CTM_HDRSZE_BYTES;

  while( buf_offset < Uncompressed_length )
  {
    buf_offset += METADATA_MSG_SIZE;
  }
}

/************************************************************************
 Description: Read and handle radial data.
 ************************************************************************/

static void Read_radial_data()
{
  short msg_size = 0;
  short type = 0;
  int   offset = 0;
  int number_of_segments = 0;
  int segment_number = 0;
  char *ptr = NULL;
  RDA_RPG_message_header_t* msg_header = NULL;
  
  /* Read control word to get size of compressed record to read. */

  Read_compressed_record();

  while( offset < Uncompressed_length )
  {
    ptr = (char *)(Uncompressed_buf+offset+CTM_HDRSZE_BYTES);
    msg_header = (RDA_RPG_message_header_t*)ptr;
    Byte_swap_msg_hdr( (char *)msg_header );
    msg_size = msg_header->size*sizeof( unsigned short );
    type = msg_header->type;
    number_of_segments = msg_header->num_segs;
    segment_number = msg_header->seg_num;

    if( type == RDA_RADIAL_MSG )
    {
      P_debug( "MSG 31: SIZE: %d", msg_size );
      Process_radial( ptr );
      offset += ( msg_size+CTM_HDRSZE_BYTES );
    }
    else if( type == RDA_STATUS_MSG )
    {
      P_debug( "MSG 2: SIZE: %d", msg_size );
      offset += METADATA_MSG_SIZE;
    }
    else if( type != 0 )
    {
      P_debug( "Read_radial_data: Invalid type (%d)", type );
    }
  }
}

/**************************************************************************
 Description: Read 4-byte control word indicating size of record.
**************************************************************************/

static void Read_control_word()
{
  int bytes_read = 0;

  if( ( bytes_read = Read_stdin( (char *)&Control_word, LDM_CONTROL_WORD_SIZE ) ) != LDM_CONTROL_WORD_SIZE )
  {
    P_err( "read stdin failed: read %d bytes and not %d, cw = %d", bytes_read, LDM_CONTROL_WORD_SIZE, Control_word );
    Print_stats( READ_STDIN_FAILED );
  }
}


/**************************************************************************
 Description: Read stdin.
**************************************************************************/

static int Read_stdin( char *buf, int bytes_to_read )
{
  char *p = buf;
  int total_bytes_read = 0;
  int bytes_read = 0;
  
  if( !Read_stdin_alarm_init_flag )
  {
    Read_stdin_alarm_init_flag = YES;
    if( Init_read_stdin_alarm() < 0 )
    {
      P_err( "Failed to initialize read stdin alarm" );
      return ALARM_INIT_FAILED;
    }
  } 
  
  while( total_bytes_read < bytes_to_read && !Read_stdin_alarm_timeout_flag )
  {
    alarm( READ_ALARM_TIMEOUT );
    bytes_read = read( STDIN_FILENO, p, bytes_to_read-total_bytes_read );
    if( bytes_read > 0 )
    {
      total_bytes_read += bytes_read;
      p += bytes_read;
    }
    else if( bytes_read == 0 )
    {
      break;
    }
    else
    {
      P_err( "read stdin failed: bytes read < 0 (%d) errno = %d (%s)", bytes_read, errno, strerror( errno ) );
      Print_stats( READ_STDIN_FAILED );
    }
  }
  alarm( 0 );

  Total_size += total_bytes_read;

  return total_bytes_read;
}

/************************************************************************
 Description: Print messages to stdout.
 ************************************************************************/

static void P_out( const char *format, ... )
{
  char buf[MAX_PRINT_MSG_LEN];
  va_list arg_ptr;

  /* If no format, nothing to do. */
  if( format == NULL ){ return; }

  /* Extract print format. */
  va_start( arg_ptr, format );
  vsprintf( buf, format, arg_ptr );
  va_end( arg_ptr );

  /* Print message to stdout. */
  fprintf( stdout, "%s\n", buf );
  fflush( stdout );
}

/************************************************************************
 Description: Print messages to stderr.
 ************************************************************************/

static void P_err( const char *format, ... )
{
  char buf[MAX_PRINT_MSG_LEN];
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
 Description: Print messages to stdout if debug flag is set.
 ************************************************************************/

static void P_debug( const char *format, ... )
{
  char buf[MAX_PRINT_MSG_LEN];
  va_list arg_ptr;

  /* If not in debug mode, nothing to do. */
  if( !Debug_mode ){ return; }

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

/**************************************************************************
 Description: Initialize alarm for reading stdin.
**************************************************************************/

static int Init_read_stdin_alarm()
{
  struct sigaction newact;
  struct sigaction oldact;

  newact.sa_handler = Read_stdin_alarm_handler;
  newact.sa_flags = 0;
  sigemptyset( &newact.sa_mask );

  if( ( sigaction( SIGALRM, &newact, &oldact ) ) == -1 )
  {
    P_err( "sigaction for SIGALRM failed" );
    Print_stats( SIGALARM_CALLBACK_FAILED );
  }

  return 0;
}

/**************************************************************************
 Description: Handler function for alarm while reading stdin.
**************************************************************************/

static void Read_stdin_alarm_handler()
{
  Read_stdin_alarm_timeout_flag = YES;
  P_debug( "ALARM handler for reading stdin called" );
}

/**************************************************************************
 Description: Byte swap message header.
**************************************************************************/

static void Byte_swap_msg_hdr( char *buf )
{
   RDA_RPG_message_header_t* header = (RDA_RPG_message_header_t*) buf;

   MISC_swap_shorts( 1, (short *) &(header->size) );
   MISC_swap_shorts( 1, (short *) &(header->sequence_num) );
   MISC_swap_shorts( 1, (short *) &(header->julian_date) );
   MISC_swap_longs( 1, (long *) &(header->milliseconds) );
   MISC_swap_shorts( 1, (short *) &(header->num_segs) );
   MISC_swap_shorts( 1, (short *) &(header->seg_num) );
}

/**************************************************************************
 Description: Byte swap generic basedata header.
**************************************************************************/

static void Byte_swap_generic_basedata_hdr( Generic_basedata_header_t *hdr )
{
  MISC_swap_longs( 1, (long *) &(hdr->time) );
  MISC_swap_shorts( 2, (short *) &(hdr->date) );
  MISC_swap_floats( 1, &(hdr->azimuth) );
  MISC_swap_shorts( 1, (short *) &(hdr->radial_length) );
  MISC_swap_floats( 1, &(hdr->elevation) );
  MISC_swap_shorts( 1, (short *) &(hdr->no_of_datum) );
}

/**************************************************************************
 Description: Byte swap Generic Basedata msg.
**************************************************************************/

static void Process_radial( char *buf )
{
  Generic_basedata_t* rec = (Generic_basedata_t *) buf;
  Generic_any_t *data_block = NULL;
  int offset = -1;
  int no_of_datum = -1;
  int i = -1;
  char data_block_name[5];
  char moment_name[5];
  static int previous_az_number = 0;
  static float previous_azimuth = -1.0;
  static int previous_elev_number = 0;
  static float previous_elevation = -1.0;

  /* Increment radial counter */
  Number_of_radials++;

  /* Byte swap generic basedata header */
  Byte_swap_generic_basedata_hdr( &(rec->base) );

  /* Set values according to radial status */
  if( rec->base.status == BEG_VOL )
  {
    /* First radial date/time is start of volume date/time */
    Volume_start_epoch = rec->base.date*SECONDS_PER_DAY;
    Volume_start_epoch += rec->base.time/MILLISECONDS_PER_SECOND;
  }

  if( rec->base.status == END_VOL )
  {
    /* Last radial date/time is end of volume date/time */
    Volume_end_epoch = rec->base.date*SECONDS_PER_DAY;
    Volume_end_epoch += rec->base.time/MILLISECONDS_PER_SECOND;
    Volume_duration = Volume_end_epoch - Volume_start_epoch;
  }

  if( rec->base.status == BEG_ELEV || rec->base.status == BEG_VOL )
  {
    /* New elevation, so increment elevation counter */
    Number_of_elevations++;
    /* Check for elevation gaps */
    if( rec->base.elev_num - previous_elev_number != 1 )
    {
      P_err( "ELEV GAP: ELEVNUM1: %02d ELEV1: %6.2f ELEVNUM2: %02d ELEV2: %6.2f", previous_elev_number, previous_elevation, rec->base.elev_num, rec->base.elevation );
    }
    previous_elev_number = rec->base.elev_num;
    previous_elevation = rec->base.elevation;
  }

  /* Check for radial gaps */
  if( rec->base.azi_num - previous_az_number != 1 )
  {
    P_err( "  AZ GAP: ELEV: %6.2f AZNUM1: %03d AZ1: %7.2f AZNUM2: %03d AZ2: %7.2f", rec->base.elevation, previous_az_number, previous_azimuth, rec->base.azi_num, rec->base.azimuth );
  }
  previous_az_number = rec->base.azi_num;
  previous_azimuth = rec->base.azimuth;

  if( rec->base.status == END_ELEV )
  {
    /* If end of elevation, reset azimuth index counter */
    previous_az_number = 0;
    previous_azimuth = -1.0;
  }

  /* Make sure a start of volume flag was received */
  if( Volume_start_epoch == EPOCH_INIT )
  {
    P_err( "No beginning of volume radial found" );
    Print_stats( MISSING_START_OF_VOLUME_RADIAL );
  }

  /******************************************************************/
  /* From this point on, the radial data is parsed for information. */
  /******************************************************************/

  /* Number of data blocks in radial */
  no_of_datum = rec->base.no_of_datum;

  for( i = 0; i < no_of_datum; i++ )
  {
    MISC_swap_longs( 1, (long *) &(rec->base.data[i]) );
    offset = rec->base.data[i];

    data_block = (Generic_any_t *)
                 (buf + sizeof(RDA_RPG_message_header_t) + offset);

    /* Convert the block name to a string so we can do string compares. */
    memset( data_block_name, 0, 5 );
    memcpy( data_block_name, data_block->name, 4 );
    data_block_name[4] = '\0';

    if( data_block_name[0] == 'R' )
    {
      if( strcmp( data_block_name, RRAD_TAG ) == 0 )
      {
        continue;
      }
      else if( strcmp( data_block_name, RELV_TAG ) == 0 )
      {
        continue;
      }
      else if( strcmp( data_block_name, RVOL_TAG ) == 0 )
      {
         Generic_vol_t *vol = (Generic_vol_t *) data_block;
         MISC_swap_shorts( 1, (short *) &(vol->vcp_num) );
         VCP = vol->vcp_num;
      }
      else
      {
        P_err( "Invalid Data Block name %s", data_block_name );
        Print_stats( INVALID_DATA_BLOCK_NAME );
      }
    }
    else if( data_block_name[0] == 'D' )
    {
      Generic_moment_t *moment = (Generic_moment_t *) data_block;

      /* Convert the data name to a string so we can do string compares. */
      memset( moment_name, 0, 5 );
      memcpy( moment_name, moment->name, 4 );
      moment_name[4] = '\0';

      if( strcmp( moment_name, DREF_TAG ) == 0 )
      {
        continue;
      }
      else if( strcmp( moment_name, DVEL_TAG ) == 0 )
      {
        continue;
      }
      else if( strcmp( moment_name, DSW_TAG ) == 0 )
      {
        continue;
      }
      else if( strcmp( moment_name, DZDR_TAG ) == 0 )
      {
        DP_flag = YES;
      }
      else if( strcmp( moment_name, DPHI_TAG ) == 0 )
      {
        DP_flag = YES;
      }
      else if( strcmp( moment_name, DRHO_TAG ) == 0 )
      {
        DP_flag = YES;
      }
      else
      {
         P_err( "Invalid Moment_name: %s", moment_name );
         Print_stats( INVALID_MOMENT_NAME );
      }
    }
    else
    {
       P_err( "Invalid Data Block Type: %c", data_block_name[0] );
       Print_stats( INVALID_DATA_BLOCK_TYPE );
    }
  }
}

