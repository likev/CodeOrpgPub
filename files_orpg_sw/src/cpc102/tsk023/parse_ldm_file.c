/************************************************************************
 *                                                                      *
 *      Module:  parse_ldm_file.c                                       *
 *                                                                      *
 *      Description:  Decodes level-II data and prints values.          *
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/06/05 20:37:42 $
 * $Id: parse_ldm_file.c,v 1.6 2013/06/05 20:37:42 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */


/* Include files. */

#include <orpg.h>
#include <bzlib.h>
#include <netinet/in.h>

/* Defines/enums. */

enum { NO = 0, YES };

#define	MAX_ICAO_LEN		5
#define	DATA_READ_TIMEOUT	3
#define	VOLUME_HEADER_SIZE	24
#define	MAX_PRINT_MSG_LEN	512
#define	METADATA_MSG_SIZE	2432
#define	RDA_ADAPT_MSG		18
#define	RDA_ADAPT_NUM_SEGS	5
#define	RDA_ADAPT_MAX_LEN	RDA_ADAPT_NUM_SEGS*METADATA_MSG_SIZE
#define	RDA_CLUTTER_MSG		15
#define	RDA_CLUTTER_NUM_SEGS	77
#define	RDA_CLUTTER_MAX_LEN	RDA_CLUTTER_NUM_SEGS*METADATA_MSG_SIZE
#define	RDA_BYPASS_MSG		13
#define	RDA_BYPASS_NUM_SEGS	49
#define	RDA_BYPASS_MAX_LEN	RDA_BYPASS_NUM_SEGS*METADATA_MSG_SIZE
#define	RDA_VCP_MSG		5
#define	RDA_VCP_NUM_SEGS	1
#define	RDA_VCP_MAX_LEN		RDA_VCP_NUM_SEGS*METADATA_MSG_SIZE
#define	RDA_PMD_MSG		3
#define	RDA_PMD_NUM_SEGS	1
#define	RDA_PMD_MAX_LEN		RDA_PMD_NUM_SEGS*METADATA_MSG_SIZE
#define	RDA_STATUS_MSG		2
#define	RDA_STATUS_NUM_SEGS	1
#define	RDA_STATUS_MAX_LEN	RDA_STATUS_NUM_SEGS*METADATA_MSG_SIZE
#define	RDA_RADIAL_MSG		31
#define	BAMS_ELEV		(180.0/32768.0)
#define	BAMS_AZ			(45.0/32768.0)
#define	BIT_0_MASK		0x00000001
#define	BIT_1_MASK		0x00000002
#define	BIT_2_MASK		0x00000004
#define	BIT_3_MASK		0x00000008
#define	BIT_4_MASK		0x00000010
#define	BIT_5_MASK		0x00000020
#define	BIT_6_MASK		0x00000040
#define	BIT_7_MASK		0x00000080
#define	BIT_8_MASK		0x00000100
#define	BIT_9_MASK		0x00000200
#define	BIT_10_MASK		0x00000400
#define	BIT_11_MASK		0x00000800
#define	BIT_12_MASK		0x00001000
#define	BIT_13_MASK		0x00002000
#define	BIT_14_MASK		0x00004000
#define	BIT_15_MASK		0x00008000
#define	BIT_16_MASK		0x00010000
#define	BIT_17_MASK		0x00020000
#define	BIT_18_MASK		0x00040000
#define	BIT_19_MASK		0x00080000
#define	BIT_20_MASK		0x00100000
#define	BIT_21_MASK		0x00200000
#define	BIT_22_MASK		0x00400000
#define	BIT_23_MASK		0x00800000
#define	BIT_24_MASK		0x01000000
#define	BIT_25_MASK		0x02000000
#define	BIT_26_MASK		0x04000000
#define	BIT_27_MASK		0x08000000
#define	BIT_28_MASK		0x10000000
#define	BIT_29_MASK		0x20000000
#define	BIT_30_MASK		0x40000000
#define	BIT_31_MASK		0x80000000
#define	RDA_ADAPT_TAG		"rda_adapt"
#define	RDA_CLUTTER_TAG		"rda_clutter"
#define	RDA_BYPASS_TAG		"rda_bypass"
#define	RDA_VCP_TAG		"rda_vcp"
#define	RDA_PMD_TAG		"rda_pmd"
#define	RDA_STATUS_TAG		"rda_status"
#define	BASE_HDR_TAG		"base_hdr"
#define	RVOL_TAG		"RVOL"
#define	RELV_TAG		"RELV"
#define	RRAD_TAG		"RRAD"
#define	RADIAL_HDR_TAG		"radial_hdr"
#define	LDM_CONTROL_WORD_SIZE	4
#define	MAX_LDM_BLOCK_SIZE	2000000
#define	LEVELII_STATS_PQ_SUCCESS	0
#define	LEVELII_STATS_PQ_READ_STDIN_FAILED	-4
#define	LEVELII_STATS_PQ_ALARM_INIT_FAILED	-5
#define	MAX_TIMESTRING_LEN	64
#define	READ_ALARM_TIMEOUT	3

/* Static/global variables. */

static char  Read_buf[MAX_LDM_BLOCK_SIZE];
static char  ICAO_buf[MAX_ICAO_LEN];
static char  Uncompressed_buf[MAX_LDM_BLOCK_SIZE];
static unsigned int Uncompressed_length = 0;
static unsigned int Uncompressed_bytes = 0;
static unsigned int Compressed_bytes = 0;
static float Compression_ratio = 0.0;
static int   End_of_volume_flag = NO;
static int   Print_rda_adapt_flag = NO;
static int   Print_rda_clutter_flag = NO;
static int   Print_rda_bypass_flag = NO;
static int   Print_rda_vcp_flag = NO;
static int   Print_rda_pmd_flag = NO;
static int   Print_rda_status_flag = NO;
static int   Print_base_hdr_flag = NO;
static int   Print_RVOL_flag = NO;
static int   Print_RELV_flag = NO;
static int   Print_RRAD_flag = NO;
static int   Print_radial_hdr_flag = NO;
static int   Debug_mode = NO;
static int   Read_stdin_alarm_timeout_flag = NO;
static int   Read_stdin_alarm_init_flag = NO;

/* Function prototypes. */

static void  Parse_command_line( int, char *[] );
static void  Print_usage( char * );
static void  Set_signals();
static void  Signal_handler( int );
static int   Is_start_of_volume_file( char * );
static int   Read_compressed_record( int );
static int   Read_meta_data();
static int   Read_radial_data( int );
static void  Process_radial( char * );
static void  Print_rda_adapt( char * );
static void  Print_rda_clutter( char * );
static void  Print_rda_bypass( char * );
static void  Print_rda_vcp( char * );
static void  Print_rda_pmd( char * );
static void  Print_rda_status( char * );
static void  Print_bdh( Generic_basedata_header_t *, char * );
static void  Print_RVOL( Generic_vol_t * );
static void  Print_RELV( Generic_elev_t * );
static void  Print_RRAD( Generic_rad_t * );
static void  Print_radial_header( char *, char * );
static int   Parse_print_args( char * );
static int   Validate_print_arg( char * );
static int LevelII_stats_pq_read_stdin( char *, int );
static int LevelII_stats_pq_read_control_word();
static void LevelII_stats_out( const char *, ... );
static void LevelII_stats_err( const char *, ... );
static void LevelII_stats_debug( const char *, ... );
static void Read_stdin_alarm_handler();
static int  Init_read_stdin_alarm();

/************************************************************************
 Description: This is the main function for the level-II stats decoder.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  char *buf = NULL;
  int control_word = 0;
  int start_flag = 0;
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

  if( ( control_word = LevelII_stats_pq_read_control_word() ) == LEVELII_STATS_PQ_READ_STDIN_FAILED )
  {
    LevelII_stats_err( "Could not read initial control word. Exiting." );
    exit( 2 );
  }

  /* Ensure this is the start of a new volume file. If not, exit. */

  if( ( start_flag = Is_start_of_volume_file( (char *)control_word ) ) < 0 )
  {
    LevelII_stats_err( "Error checking for start of volume" );
    exit( 3 );
  }
  else if( start_flag == NO )
  {
    LevelII_stats_err( "Not start of volume file. Exiting." );
    exit( 4 );
  }

  /* Read meta data (first record of new volume file). */

  if( Read_meta_data() < 0 )
  {
    LevelII_stats_err( "Read meta data failed. Exiting." );
    exit( 5 );
  }

  /* Loop and read rest of volume file. */

  while( End_of_volume_flag == NO )
  {
    /* Sanity check. If it takes too long, assume something
       bad has happened and exit. */
    if( time( NULL ) - initial_read_time > DATA_READ_TIMEOUT )
    {
      LevelII_stats_err( "Read timeout (>%d). Exiting.", DATA_READ_TIMEOUT );
      exit( 6 );
    }

    /* Read control word that tells us what to do. */
    if( ( control_word = LevelII_stats_pq_read_control_word() ) == LEVELII_STATS_PQ_READ_STDIN_FAILED )
    {
      LevelII_stats_err( "Read control word failed. Exiting" );
      exit( 7 );
    }

    if( Read_radial_data( control_word ) < 0 )
    {
      LevelII_stats_err( "Read radial data failed. Exiting." );
      exit( 8 );
    }
  }

  return 0;
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
  while( ( ch = getopt( argc, argv, "hxp:" ) ) != EOF )
  {
    switch( ch )
    {
      case 'h':
        Print_usage( argv[0] );
        exit( 0 );
      case 'x':
        Debug_mode = YES;
        break;
      case 'p':
        if( optarg != NULL )
        {
          if( Parse_print_args( optarg ) < 0 )
          {
            Print_usage( argv[0] );
            exit( 1 );
          }
        }
        break;
      case '?':
        Print_usage( argv[0] );
        exit( 1 );
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
  fprintf( stderr, "  Recommend separating stdout/stderr\n" );
  fprintf( stderr, "  Example: cat LDM.file | %s > out.txt 2> err.txt\n", av0 );
  fprintf( stderr, "Options:\n" );
  fprintf( stderr, "  -h      - print usage\n" );
  fprintf( stderr, "  -x      - debug mode, (Default: no)\n" );
  fprintf( stderr, "  -p p1,p2,... - print specified info\n" );
  fprintf( stderr, "     where p1,p2,... is a comma delimited list\n" );
  fprintf( stderr, "     options:\n" );
  fprintf( stderr, "        %s - RDA adaptation data\n", RDA_ADAPT_TAG );
  fprintf( stderr, "        %s - RDA Clutter Map\n", RDA_CLUTTER_TAG );
  fprintf( stderr, "        %s - RDA Bypass Map\n", RDA_BYPASS_TAG );
  fprintf( stderr, "        %s - RDA VCP\n", RDA_VCP_TAG );
  fprintf( stderr, "        %s - RDA Performance Maintenance\n", RDA_PMD_TAG );
  fprintf( stderr, "        %s - RDA Status\n", RDA_STATUS_TAG );
  fprintf( stderr, "        %s - Generic Basedata header\n", BASE_HDR_TAG );
  fprintf( stderr, "        %s - Generic Volume data block\n", RVOL_TAG );
  fprintf( stderr, "        %s - Generic Elevation data block\n", RELV_TAG );
  fprintf( stderr, "        %s - Generic Radial data block\n", RRAD_TAG );
  fprintf( stderr, "        %s - Radial header for data moment\n", RADIAL_HDR_TAG );
  fprintf( stderr, "\n");
  fprintf( stderr, "The following signals are caught:\n");
  fprintf( stderr, "SIGINT,SIGTERM - terminate\n");
  fprintf( stderr, "SIGPIPE - ignore\n");
}

/************************************************************************
 Description: Parse print argument from command line.
 ************************************************************************/

static int Parse_print_args( char *arg )
{
  char *tok = NULL;

  if( ( tok = strtok( arg, "," ) ) == NULL ){ return -1; }
  if( Validate_print_arg( tok ) < 0 ){ return -1; }

  while( ( tok = strtok( NULL, "," ) ) != NULL )
  {
    if( Validate_print_arg( tok ) < 0 ){ return -1; }
  }

  return 0;
}

/************************************************************************
 Description: Validate token from print argument.
 ************************************************************************/

static int Validate_print_arg( char *arg )
{
  if( strcmp( arg, RDA_ADAPT_TAG ) == 0 ){ Print_rda_adapt_flag = YES; }
  else if( strcmp( arg, RDA_CLUTTER_TAG ) == 0 ){ Print_rda_clutter_flag = YES; }
  else if( strcmp( arg, RDA_BYPASS_TAG ) == 0 ){ Print_rda_bypass_flag = YES; }
  else if( strcmp( arg, RDA_VCP_TAG ) == 0 ){ Print_rda_vcp_flag = YES; }
  else if( strcmp( arg, RDA_PMD_TAG ) == 0 ){ Print_rda_pmd_flag = YES; }
  else if( strcmp( arg, RDA_STATUS_TAG ) == 0 ){ Print_rda_status_flag = YES; }
  else if( strcmp( arg, BASE_HDR_TAG ) == 0 ){ Print_base_hdr_flag = YES; }
  else if( strcmp( arg, RVOL_TAG ) == 0 ){ Print_RVOL_flag = YES; }
  else if( strcmp( arg, RELV_TAG ) == 0 ){ Print_RELV_flag = YES; }
  else if( strcmp( arg, RRAD_TAG ) == 0 ){ Print_RRAD_flag = YES; }
  else if( strcmp( arg, RADIAL_HDR_TAG ) == 0 ){ Print_radial_hdr_flag = YES; }
  else{ return -1; }

  return 0;
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
      LevelII_stats_debug( "signal SIGPIPE received...ignore" );
      return;
      break;
    case SIGINT :
      LevelII_stats_debug( "signal SIGINT received...terminate" );
      exit( 0 );
      break;
    case SIGTERM :
      LevelII_stats_debug( "signal SIGTERM received...terminate" );
      exit( 0 );
      break;
    default :
      LevelII_stats_debug( "Signal_handler: unhandled signal: %d", sig );
  }
}

/************************************************************************
 Description: Check if buf is start of level-II volume file.
 ************************************************************************/

static int Is_start_of_volume_file( char *buf )
{
  int bytes_read = 0;
  int ignore_bytes = 0;
  char ignore_buf[VOLUME_HEADER_SIZE];

  /* If this is the beginning of a level-II file, then the 4-bytes
     just read is the beginning of the volume header and not the
     record length. If this is the case, read and ignore the header. */

  if( ! strncmp( (char *)&buf, "AR", 2 ) )
  {
    ignore_bytes = VOLUME_HEADER_SIZE - LDM_CONTROL_WORD_SIZE;
    bytes_read = LevelII_stats_pq_read_stdin( (char *)ignore_buf, ignore_bytes );
    if( bytes_read != ignore_bytes )
    {
      LevelII_stats_out( "Missing volume file header (%d bytes read)", bytes_read );
      return -1;
    }
    memcpy( ICAO_buf, &ignore_buf[16], 4 );
    ICAO_buf[MAX_ICAO_LEN-1] = '\0';
    return YES;
  }

  return NO;
}

/************************************************************************
 Description: Read the compressed record of size size_of_record.
 ************************************************************************/

static int Read_compressed_record( int size_of_record )
{
  int bytes_to_read = 0;
  int bytes_read = 0;
  int error = 0;

  /* Byte-swap size flag so we can use it. */

  bytes_to_read = ntohl( size_of_record );
  Compressed_bytes += bytes_to_read;

  LevelII_stats_debug( "Read_compressed_record: read %d", bytes_to_read );

  /* Negative size indicates this is the last record of the volume. */

  if( bytes_to_read < 0 )
  {
    LevelII_stats_debug( "Read_compressed_record: EOV" );
    bytes_to_read = -bytes_to_read;
    End_of_volume_flag = YES;
  }

  /* Read in compressed record according to expected size. If fewer
     bytes are read than expected, that's a problem. */

  bytes_read = LevelII_stats_pq_read_stdin( (char *)Read_buf, bytes_to_read );

  if( bytes_read != bytes_to_read )
  {
    LevelII_stats_debug( "Read_compressed_record: Bytes read (%d) less than (%d)",
               bytes_read, bytes_to_read );
    return -1;
  }

  /* Decompress the compressed record and put in different buffer. */

  Uncompressed_length = sizeof( Uncompressed_buf );

  error = BZ2_bzBuffToBuffDecompress( Uncompressed_buf, &Uncompressed_length,
                                      Read_buf, bytes_read,
                                      0, 1 );
  if( error )
  {
    LevelII_stats_debug( "Read_compressed_record: Decompress error - %d", error );
    return -1;
  }

  Uncompressed_bytes += Uncompressed_length;
  Compressed_bytes += bytes_read;
  Compression_ratio = (float) Uncompressed_bytes / (float) Compressed_bytes;

  return 0;
}

/************************************************************************
 Description: Read and handle meta data.
 ************************************************************************/

static int Read_meta_data()
{
  int   control_word = 0;
  short msg_size = 0;
  short type = 0;
  int   buf_offset = 0;
  int   copy_offset = 0;
  int   copy_size = 0;
  int   number_of_segments = 0;
  int   segment_number = 0;
  char  msg18buf[RDA_ADAPT_MAX_LEN];
  int   msg18_index = 0;
  char  msg15buf[RDA_CLUTTER_MAX_LEN];
  int   msg15_index = 0;
  char  msg13buf[RDA_BYPASS_MAX_LEN];
  int   msg13_index = 0;
  char  msg5buf[RDA_VCP_MAX_LEN];
  int   msg5_index = 0;
  char  msg3buf[RDA_PMD_MAX_LEN];
  int   msg3_index = 0;
  char  msg2buf[RDA_STATUS_MAX_LEN];
  int   msg2_index = 0;
  RDA_RPG_message_header_t* msg_header = NULL;
  RDA_RPG_message_header_t* msg5_header = NULL;
  
  /* Read control word to get size of compressed record to read. */

  if( ( control_word = LevelII_stats_pq_read_control_word() ) == LEVELII_STATS_PQ_READ_STDIN_FAILED )
  {
    LevelII_stats_debug( "Read_meta_data: Read control word failed" );
    return -1;
  }

  if( Read_compressed_record( control_word ) < 0 )
  {
    LevelII_stats_debug( "Read_meta_data: Read compressed_record failed" );
    return -1;
  }

  /* Skip initial 12 bytes (comms manager header). */

  buf_offset = CTM_HDRSZE_BYTES;

  while( buf_offset < Uncompressed_length )
  {
    msg_header = (RDA_RPG_message_header_t*)(Uncompressed_buf+buf_offset);
    UMC_RDAtoRPG_message_header_convert( (char *)msg_header );
    msg_size = msg_header->size*sizeof( unsigned short );
    type = msg_header->type;
    number_of_segments = msg_header->num_segs;
    segment_number = msg_header->seg_num;
    buf_offset += METADATA_MSG_SIZE;

    if( segment_number != 1 )
    {
      copy_offset = sizeof( RDA_RPG_message_header_t );
      copy_size = msg_size - sizeof( RDA_RPG_message_header_t );
    }
    else
    {
      copy_offset = 0;
      copy_size = msg_size;
    }

    if( type == RDA_ADAPT_MSG )
    {
      LevelII_stats_debug( "MSG 18: SIZE: %d INDEX: %d SEGMENT: %d OF %d",
                   msg_size, msg18_index, segment_number, number_of_segments );
      memcpy( (char *)(msg18buf+msg18_index),
              (char *)msg_header+copy_offset, copy_size );
      msg18_index += copy_size;
      msg_header = (RDA_RPG_message_header_t *)msg18buf;
      msg_header->size = msg18_index;
    }
    else if( type == RDA_CLUTTER_MSG )
    {
      LevelII_stats_debug( "MSG 15: SIZE: %d INDEX: %d SEGMENT: %d OF %d",
                   msg_size, msg15_index, segment_number, number_of_segments );
      LevelII_stats_debug( " MSG15_INDEX = %d, COPY_SIZE = %d, COPY_OFFSET = %d\n", msg15_index, msg_size, copy_offset);
      memcpy( (char *)(msg15buf+msg15_index),
              (char *)msg_header+copy_offset, copy_size );
      msg15_index += copy_size;
      msg_header = (RDA_RPG_message_header_t *)msg15buf;
      msg_header->size = msg15_index;
    }
    else if( type == RDA_BYPASS_MSG )
    {
      LevelII_stats_debug( "MSG 13: SIZE: %d INDEX: %d SEGMENT: %d OF %d",
                   msg_size, msg13_index, segment_number, number_of_segments );
      memcpy( (char *)(msg13buf+msg13_index),
              (char *)msg_header+copy_offset, copy_size );
      msg13_index += copy_size;
      msg_header = (RDA_RPG_message_header_t *)msg13buf;
      msg_header->size = msg13_index;
    }
    else if( type == RDA_VCP_MSG )
    {
      LevelII_stats_debug( "MSG 5: SIZE: %d INDEX: %d SEGMENT: %d OF %d",
                   msg_size, msg5_index, segment_number, number_of_segments );

      memcpy( (char *)(msg5buf+msg5_index),
              (char *)msg_header+copy_offset, copy_size );
      msg5_index += copy_size;
      msg_header = (RDA_RPG_message_header_t *)msg5buf;
      msg5_header = (RDA_RPG_message_header_t *)msg5buf;
      msg_header->size = msg5_index;
    }
    else if( type == RDA_PMD_MSG )
    {
      LevelII_stats_debug( "MSG 3: SIZE: %d INDEX: %d SEGMENT: %d OF %d",
                   msg_size, msg3_index, segment_number, number_of_segments );
      memcpy( (char *)(msg3buf+msg3_index),
              (char *)msg_header+copy_offset, copy_size );
      msg3_index += copy_size;
      msg_header = (RDA_RPG_message_header_t *)msg3buf;
      msg_header->size = msg3_index;
    }
    else if( type == RDA_STATUS_MSG )
    {
      LevelII_stats_debug( "MSG 2: SIZE: %d INDEX: %d SEGMENT: %d OF %d",
                   msg_size, msg2_index, segment_number, number_of_segments );
      memcpy( (char *)(msg2buf+msg2_index),
              (char *)msg_header+copy_offset, copy_size );
      msg2_index += copy_size;
      msg_header = (RDA_RPG_message_header_t *)msg2buf;
      msg_header->size = msg2_index;
    }
    else if( type != 0 )
    {
      LevelII_stats_debug( "Read_meta_data: Invalid type (%d)", type );
    }
  }

  LevelII_stats_debug( "MSG SIZES 18: %d 15: %d 13: %d 5: %d 3: %d 2: %d",
              msg18_index, msg15_index, msg13_index,
              msg5_index, msg3_index, msg2_index );

  UMC_RDAtoRPG_message_convert_to_internal( RDA_ADAPT_MSG, (char *)msg18buf );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)&msg18buf[1328] );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)&msg18buf[2500] );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)&msg18buf[3672] );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)&msg18buf[4844] );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)&msg18buf[6016] );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)&msg18buf[7188] );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_CLUTTER_MSG, (char *)msg15buf );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_BYPASS_MSG, (char *)msg13buf );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)msg5buf );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_PMD_MSG, (char *)msg3buf );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_STATUS_MSG, (char *)msg2buf );

  if( Debug_mode || Print_rda_adapt_flag )
  {
    Print_rda_adapt( (char *)msg18buf );
  }
  if( Debug_mode || Print_rda_clutter_flag )
  {
    Print_rda_clutter( (char *)msg15buf );
  }
  if( Debug_mode || Print_rda_bypass_flag )
  {
    Print_rda_bypass( (char *)msg13buf );
  }
  if( Debug_mode || Print_rda_vcp_flag )
  {
    Print_rda_vcp( (char *)msg5buf );
  }
  if( Debug_mode || Print_rda_pmd_flag )
  {
    Print_rda_pmd( (char *)msg3buf );
  }
  if( Debug_mode || Print_rda_status_flag )
  {
    Print_rda_status( (char *)msg2buf );
  }

  return 0;
}

/************************************************************************
 Description: Read and handle radial data.
 ************************************************************************/

static int Read_radial_data( int control_word )
{
  short msg_size = 0;
  short type = 0;
  int   offset = 0;
  int number_of_segments = 0;
  int segment_number = 0;
  char *ptr = NULL;
  RDA_RPG_message_header_t* msg_header = NULL;
  
  /* Read control word to get size of compressed record to read. */
 
  if( Read_compressed_record( control_word ) < 0 )
  {
    LevelII_stats_debug( "Read_radial_data: Read_compressed_record failed" );
    return -1;
  }

  while( offset < Uncompressed_length )
  {
    ptr = (char *)(Uncompressed_buf+offset+CTM_HDRSZE_BYTES);
    msg_header = (RDA_RPG_message_header_t*)ptr;
    UMC_RDAtoRPG_message_header_convert( (char *)msg_header );
    msg_size = msg_header->size*sizeof( unsigned short );
    type = msg_header->type;
    number_of_segments = msg_header->num_segs;
    segment_number = msg_header->seg_num;

    if( type == RDA_RADIAL_MSG )
    {
      LevelII_stats_debug( "MSG 31: SIZE: %d", msg_size );
      Process_radial( ptr );
      offset += ( msg_size+CTM_HDRSZE_BYTES );
    }
    else if( type == RDA_STATUS_MSG )
    {
      LevelII_stats_debug( "MSG 2: SIZE: %d", msg_size );
      UMC_RDAtoRPG_message_convert_to_internal( RDA_STATUS_MSG, ptr );
      if( Debug_mode || Print_rda_status_flag )
      {
        Print_rda_status( ptr );
      }
      offset += METADATA_MSG_SIZE;
    }
    else if( type != 0 )
    {
      LevelII_stats_debug( "Read_radial_data: Invalid type (%d)", type );
    }
  }

  return 0;
}

/************************************************************************
 Description: Process radial of data.
 ************************************************************************/

static void Process_radial( char *buf )
{
  int i;
  char *ptr = NULL;

  Generic_basedata_t *bd = (Generic_basedata_t *) buf;
  Generic_basedata_header_t *bdh = &bd->base;
  Generic_any_t *dblk = NULL;
  char temp_buf[5];

  if( bdh->elev_num != 1 ){ return; }

  UMC_RDAtoRPG_message_convert_to_internal( RDA_RADIAL_MSG, (char *)buf );

  memset( temp_buf, 0, 5 );
  temp_buf[4] = '\0';

  if( Debug_mode || Print_base_hdr_flag )
  {
    memcpy( temp_buf, bdh->radar_id, sizeof( bdh->radar_id ) );
    Print_bdh( bdh, temp_buf );
  }

  for( i = 0; i < bdh->no_of_datum; i++ )
  {
    ptr = (char *)(buf+sizeof( RDA_RPG_message_header_t )+bdh->data[i] );
    dblk = (Generic_any_t *)ptr;

    memcpy( temp_buf, dblk->name, sizeof( dblk->name ) );

    if( strstr( temp_buf, "RVOL" ) != NULL )
    {
      if( Debug_mode || Print_RVOL_flag )
      {
        Print_RVOL( ( Generic_vol_t * ) dblk );
      }
    }
    else if( strstr( temp_buf, "RELV" ) != NULL )
    {
      if( Debug_mode || Print_RELV_flag )
      {
        Print_RELV( ( Generic_elev_t * ) dblk );
      }
    }
    else if( strstr( temp_buf, "RRAD" ) != NULL )
    {
      if( Debug_mode || Print_RRAD_flag )
      {
        Print_RRAD( ( Generic_rad_t * ) dblk );
      }
    }
    else if( strstr( temp_buf, "DREF" ) != NULL )
    {
      if( Debug_mode || Print_radial_hdr_flag )
      {
        Print_radial_header( temp_buf, ptr );
      }
    }
    else if( strstr( temp_buf, "DVEL" ) != NULL )
    {
      if( Debug_mode || Print_radial_hdr_flag )
      {
        Print_radial_header( temp_buf, ptr );
      }
    }
    else if( strstr( temp_buf, "DSW" ) != NULL )
    {
      if( Debug_mode || Print_radial_hdr_flag )
      {
        Print_radial_header( temp_buf, ptr );
      }
    }
    else if( strstr( temp_buf, "DZDR" ) != NULL )
    {
      if( Debug_mode || Print_radial_hdr_flag )
      {
        Print_radial_header( temp_buf, ptr );
      }
    }
    else if( strstr( temp_buf, "DPHI" ) != NULL )
    {
      if( Debug_mode || Print_radial_hdr_flag )
      {
        Print_radial_header( temp_buf, ptr );
      }
    }
    else if( strstr( temp_buf, "DRHO" ) != NULL )
    {
      if( Debug_mode || Print_radial_hdr_flag )
      {
        Print_radial_header( temp_buf, ptr );
      }
    }
  }
}

/************************************************************************
 Description: Print RDA Adaptation message.
 ************************************************************************/

static void Print_rda_adapt( char *buf )
{
  int i;

  ORDA_adpt_data_msg_t *adapt_msg = (ORDA_adpt_data_msg_t *)buf;
  ORDA_adpt_data_t rda_adapt = adapt_msg->rda_adapt;

  LevelII_stats_out( "RDA ADAPT MSG (MSG TYPE 18)" );
  LevelII_stats_out( "  ADPT FILE NAME:  %s", rda_adapt.adap_file_name );
  LevelII_stats_out( "  ADPT FORMAT:  %s", rda_adapt.adap_format );
  LevelII_stats_out( "  ADPT REVISION:  %s", rda_adapt.adap_revision );
  LevelII_stats_out( "  ADPT DATE:  %s", rda_adapt.adap_date );
  LevelII_stats_out( "  ADPT TIME:  %s", rda_adapt.adap_time );
  LevelII_stats_out( "  AZ POS GAIN FACTOR:  %f", rda_adapt.k1 );
  LevelII_stats_out( "  LATENCY OF DCU AZ MEASURE:  %f", rda_adapt.az_lat );
  LevelII_stats_out( "  ELEV POS GAIN FACTOR:  %f", rda_adapt.k3 );
  LevelII_stats_out( "  LATENCY OF DCU ELEV MEASURE:  %f", rda_adapt.el_lat );
  LevelII_stats_out( "  PED PARK POS AZ:  %f", rda_adapt.parkaz );
  LevelII_stats_out( "  PED PARK POS ELEV:  %f", rda_adapt.parkel );
  for( i = 0; i < 11; i++ )
  {
    LevelII_stats_out( "  FUEL LEVEL %d:  %f", i, rda_adapt.a_fuel_conv[i] );
  }
  LevelII_stats_out( "  EQUIP ALARM MIN TEMP:  %f", rda_adapt.a_min_shelter_temp );
  LevelII_stats_out( "  EQUIP ALARM MAX TEMP:  %f", rda_adapt.a_max_shelter_temp );
  LevelII_stats_out( "  MIN A/C DISCHARGE AIR TEMP DIFF:  %f", rda_adapt.a_min_shelter_ac_temp_diff );
  LevelII_stats_out( "  XMTR LEAVING AIR ALARM MAX TEMP:  %f", rda_adapt.a_max_xmtr_air_temp );
  LevelII_stats_out( "  RADOME ALARM MAX TEMP:  %f", rda_adapt.a_max_rad_temp );
  LevelII_stats_out( "  MAX RADOME MINUS EXT TEMP DIFF:  %f", rda_adapt.a_max_rad_temp_rise );
  LevelII_stats_out( "  PED 28V PS TOLERANCE:  %f", rda_adapt.ped_28V_reg_lim );
  LevelII_stats_out( "  PED 5V PS TOLERANCE:  %f", rda_adapt.ped_5V_reg_lim );
  LevelII_stats_out( "  PED 15V PS TOLERANCE:  %f", rda_adapt.ped_15V_reg_lim );
  LevelII_stats_out( "  GEN SHELTER ALARM MIN TEMP:  %f", rda_adapt.a_min_gen_room_temp );
  LevelII_stats_out( "  GEN SHELTER ALARM MAX TEMP:  %f", rda_adapt.a_max_gen_room_temp );
  LevelII_stats_out( "  DAU 5V PS TOLERANCE:  %f", rda_adapt.dau_5V_reg_lim );
  LevelII_stats_out( "  DAU 15V PS TOLERANCE:  %f", rda_adapt.dau_15V_reg_lim );
  LevelII_stats_out( "  DAU 28V PS TOLERANCE:  %f", rda_adapt.dau_28V_reg_lim );
  LevelII_stats_out( "  ENCODER 5V PS TOLERANCE:  %f", rda_adapt.en_5V_reg_lim );
  LevelII_stats_out( "  ENCODER 5V PS NOMINAL VOLTAGE:  %f", rda_adapt.en_5V_nom_volts );
  LevelII_stats_out( "  RPG CO-LOCATED:  %s", rda_adapt.rpg_co_located );
  LevelII_stats_out( "  XMTR SPEC FILTER INSTALLED:  %s", rda_adapt.spec_filter_installed );
  LevelII_stats_out( "  TPS INSTALLED:  %s", rda_adapt.tps_installed );
  LevelII_stats_out( "  RMS INSTALLED:  %s", rda_adapt.rms_installed );
  LevelII_stats_out( "  PERF TEST INTERVAL:  %d", rda_adapt.a_hvdl_tst_int );
  LevelII_stats_out( "  RPG LOOP TEST INTERVAL:  %d", rda_adapt.a_rpg_lt_int );
  LevelII_stats_out( "  REQD INTERVAL STABLE UTIL PWR:  %d", rda_adapt.a_min_stab_util_pwr_time );
  LevelII_stats_out( "  MAX GENERATOR AUTO EXER INTERVAL:  %d", rda_adapt.a_gen_auto_exer_interval );
  LevelII_stats_out( "  REC SWITCH TO UTIL PWR TIME INTERVAL:  %d", rda_adapt.a_util_pwr_sw_req_interval );
  LevelII_stats_out( "  LOW FUEL LEVEL WARNING:  %f", rda_adapt.a_low_fuel_level );
  LevelII_stats_out( "  CONFIG CHANNEL NUMBER:  %d", rda_adapt.config_chan_number );
  LevelII_stats_out( "  SPARE220:  %d", rda_adapt.spare_220 );
  LevelII_stats_out( "  RED CHANNEL CONFIG:  %d", rda_adapt.redundant_chan_config );
  for( i = 0; i < 104; i++ )
  {
    LevelII_stats_out( "  ATTEN TABLE-%d:  %f", i, rda_adapt.atten_table[i] );
  }
  for( i = 0; i < 69; i++ )
  {
    LevelII_stats_out( "  PATH LOSS-%d:  %f", i, rda_adapt.path_losses[i] );
  }
  LevelII_stats_out( "  NONCON CHANNEL CALIB DIFF:  %f", rda_adapt.chan_cal_diff );
  LevelII_stats_out( "  SPARE PATH LOSSES:  %f", rda_adapt.path_losses_70_71 );
  LevelII_stats_out( "  LOG AMP FACTOR1:  %f", rda_adapt.log_amp_factor[0] );
  LevelII_stats_out( "  LOG AMP FACTOR2:  %f", rda_adapt.log_amp_factor[1] );
  LevelII_stats_out( "  AME VERT TEST SIG PWR:  %f", rda_adapt.v_ts_cw );
  for( i = 0; i < 13; i++ )
  {
    LevelII_stats_out( "  HORIZ RECV NOISE NORMAL-%d:  %f", i, rda_adapt.rnscale[i] );
  }
  for( i = 0; i < 13; i++ )
  {
    LevelII_stats_out( "  TWO-WAY ATMOS LOSS-%d:  %f", i, rda_adapt.atmos[i] );
  }
  for( i = 0; i < 12; i++ )
  {
    LevelII_stats_out( "  BYP MAP ELEV ANGLE-%d:  %f", i, rda_adapt.el_index[i] );
  }
  LevelII_stats_out( "  XMTR FREQUENCY:  %d", rda_adapt.tfreq_mhz );
  LevelII_stats_out( "  PT CLUTTER SUPP THRESHOLD:  %f", rda_adapt.base_data_tcn );
  LevelII_stats_out( "  RANGE UNFOLD OVERLAY THRESHOLD:  %f", rda_adapt.refl_data_tover );
  LevelII_stats_out( "  HORIZ TARGET SYS CALIB LONG PULSE:  %f", rda_adapt.tar_h_dbz0_inc_lp );
  LevelII_stats_out( "  VERT TARGET SYS CALIB LONG PULSE:  %f", rda_adapt.tar_v_dbz0_inc_lp );
  LevelII_stats_out( "  PHI CLUTTER TARGE AZ:  %f", rda_adapt.phi_clutter_az );
  LevelII_stats_out( "  PHI CLUTTER TARGE ELEV:  %f", rda_adapt.phi_clutter_el );
  LevelII_stats_out( "  MATCHED FILTER LOSS LONG PULSE:  %f", rda_adapt.lx_lp );
  LevelII_stats_out( "  MATCHED FILTER LOSS SHORT PULSE:  %f", rda_adapt.lx_sp );
  LevelII_stats_out( "  HYDRO REFRACT FACTOR:  %f", rda_adapt.meteor_param );
  LevelII_stats_out( "  ANTENNA BEAMWIDTH:  %f", rda_adapt.beamwidth );
  LevelII_stats_out( "  ANTENNA GAIN (INCL RADOME):  %f", rda_adapt.antenna_gain );
  LevelII_stats_out( "  USE COHO THRESHOLDS:  %d", rda_adapt.use_coho_thresholding );
  LevelII_stats_out( "  VEL CHECK DELTA MAINT LIMIT:  %f", rda_adapt.vel_maint_limit );
  LevelII_stats_out( "  SPW CHECK DELTA MAINT LIMIT:  %f", rda_adapt.wth_maint_limit );
  LevelII_stats_out( "  VEL CHECK DELTA DEGRADE LIMIT:  %f", rda_adapt.vel_degrad_limit );
  LevelII_stats_out( "  SPW CHECK DELTA DEGRADE LIMIT:  %f", rda_adapt.wth_degrad_limit );
  LevelII_stats_out( "  HORIZ SYS NOISE TEMP DEGRADE LIMIT:  %f", rda_adapt.h_noisetemp_dgrad_limit );
  LevelII_stats_out( "  HORIZ SYS NOISE TEMP MAINT LIMIT:  %f", rda_adapt.h_noisetemp_maint_limit );
  LevelII_stats_out( "  VERT SYS NOISE TEMP DEGRADE LIMIT:  %f", rda_adapt.v_noisetemp_dgrad_limit );
  LevelII_stats_out( "  VERT SYS NOISE TEMP MAINT LIMIT:  %f", rda_adapt.v_noisetemp_maint_limit );
  LevelII_stats_out( "  KLYSTRON OUTPUT TARGET CONSIS DEGRADE LIMIT:  %f", rda_adapt.kly_degrade_limit );
  LevelII_stats_out( "  COHO PWR AT A1J4:  %f", rda_adapt.ts_coho );
  LevelII_stats_out( "  AME HORIZ TEST SIG PWR:  %f", rda_adapt.ts_cw );
  LevelII_stats_out( "  RF DRIVE TEST SIG SHORT PULSE:  %f", rda_adapt.ts_rf_sp );
  LevelII_stats_out( "  RF DRIVE TEST SIG LONG PULSE:  %f", rda_adapt.ts_rf_lp );
  LevelII_stats_out( "  STALO PWR AT A1J2:  %f", rda_adapt.ts_stalo );
  LevelII_stats_out( "  RF NOISE TEST SIG EXCESS NOISE RATIO:  %f", rda_adapt.ts_noise );
  LevelII_stats_out( "  XMTR PEAK PWR ALARM MAX LEVEL:  %f", rda_adapt.xmtr_peak_power_high_limit );
  LevelII_stats_out( "  XMTR PEAK PWR ALARM MIN LEVEL:  %f", rda_adapt.xmtr_peak_power_low_limit );
  LevelII_stats_out( "  COMPUTE/TARGET HORIZ ddBZ0 DIFF LIMIT:  %f", rda_adapt.h_dbz0_delta_limit );
  LevelII_stats_out( "  BYP MAP NOISE THRESHOLD:  %f", rda_adapt.threshold1 );
  LevelII_stats_out( "  BYP MAP REJECT RATIO THRESHOLD:  %f", rda_adapt.threshold2 );
  LevelII_stats_out( "  HORIZ CLUTT SUPP DEGRADE LIMIT:  %f", rda_adapt.h_clut_supp_dgrad_lim );
  LevelII_stats_out( "  HORIZ CLUTT SUPP MAINT LIMIT:  %f", rda_adapt.h_clut_supp_maint_lim );
  LevelII_stats_out( "  TRUE RANGE AT FIRST RANGE BIN:  %f", rda_adapt.range0_value );
  LevelII_stats_out( "  XMTR PWR BYTE DATA TO WATTS SCLAE FACTOR:  %f", rda_adapt.xmtr_pwr_mtr_scale );
  LevelII_stats_out( "  COMPUTE/TARGET VERT ddBZ0 DIFF LIMIT:  %f", rda_adapt.v_dbz0_delta_limit );
  LevelII_stats_out( "  HORIZ TARGET SYS CALIB SHORT PULSE:  %f", rda_adapt.tar_h_dbz0_sp );
  LevelII_stats_out( "  VERT TARGET SYS CALIB SHORT PULSE:  %f", rda_adapt.tar_v_dbz0_sp );
  LevelII_stats_out( "  SITE PRF SET:  %d", rda_adapt.deltaprf );
  LevelII_stats_out( "  SPARE1256:  %d", rda_adapt.spare1256 );
  LevelII_stats_out( "  SPARE1260:  %d", rda_adapt.spare1260 );
  LevelII_stats_out( "  XMTR OUTPUT PULSE WIDTH SHORT PULSE:  %d", rda_adapt.tau_sp );
  LevelII_stats_out( "  XMTR OUTPUT PULSE WIDTH LONG PULSE:  %d", rda_adapt.tau_lp );
  LevelII_stats_out( "  NUM 1/4km CORRUPT BINS AT SWEEP END:  %d", rda_adapt.nc_dead_value );
  LevelII_stats_out( "  RF DRIVE PULSE WIDTH SHORT PULSE:  %d", rda_adapt.tau_rf_sp );
  LevelII_stats_out( "  RF DRIVE PULSE WIDTH LONG PULSE:  %d", rda_adapt.tau_rf_lp );
  LevelII_stats_out( "  CLUTTER MAP ELEV BNDRY SEG 1/2:  %f", rda_adapt.seg1lim );
  LevelII_stats_out( "  SITE LAT SECS:  %f", rda_adapt.slatsec );
  LevelII_stats_out( "  SITE LONG SECS:  %f", rda_adapt.slonsec );
  LevelII_stats_out( "  PHI CLUTTER RANGE:  %d", rda_adapt.phi_clutter_range );
  LevelII_stats_out( "  SITE LAT DEGS:  %d", rda_adapt.slatdeg );
  LevelII_stats_out( "  SITE LAT MINS:  %d", rda_adapt.slatmin );
  LevelII_stats_out( "  SITE LONG DEGS:  %d", rda_adapt.slongdeg );
  LevelII_stats_out( "  SITE LONG MINS:  %d", rda_adapt.slongmin );
  LevelII_stats_out( "  SITE LAT DIR:  %s", rda_adapt.slatdir );
  LevelII_stats_out( "  SITE LONG DIR:  %s", rda_adapt.slondir );
  LevelII_stats_out( "  INDEX TO CURRENT VCP:  %d", rda_adapt.vc_ndx );
/*
  LevelII_stats_out( "  :  %s", rda_adapt.vcpat11 );
  LevelII_stats_out( "  :  %s", rda_adapt.vcpat21 );
  LevelII_stats_out( "  :  %s", rda_adapt.vcpat31 );
  LevelII_stats_out( "  :  %s", rda_adapt.vcpat32 );
  LevelII_stats_out( "  :  %s", rda_adapt.vcpat300 );
  LevelII_stats_out( "  :  %s", rda_adapt.vcpat301 );
*/
  LevelII_stats_out( "  AZ BORESIGHT CORR FACTOR:  %f", rda_adapt.az_correction_factor );
  LevelII_stats_out( "  ELEV BORESIGHT CORR FACTOR:  %f", rda_adapt.el_correction_factor );
  LevelII_stats_out( "  ICAO:  %s", rda_adapt.site_name );
  LevelII_stats_out( "  ELEV ANGLE MIN:  %d", rda_adapt.ant_manual_setup_ielmin );
  LevelII_stats_out( "  ELEV ANGLE MAX:  %d", rda_adapt.ant_manual_setup_ielmax );
  LevelII_stats_out( "  AZ VELOCITY MAX:  %d", rda_adapt.ant_manual_setup_fazvelmax );
  LevelII_stats_out( "  ELEV VELOCITY MAX:  %d", rda_adapt.ant_manual_setup_felvelmax );
  LevelII_stats_out( "  SITE GROUND HEIGHT:  %d", rda_adapt.ant_manual_setup_ignd_hgt );
  LevelII_stats_out( "  SITE RADAR HEIGHT:  %d", rda_adapt.ant_manual_setup_irad_hgt );
  for( i = 0; i < 75; i++ )
  {
    LevelII_stats_out( "  SPARE75-%d:  %d", i, rda_adapt.spare8496[i] );
  }
  LevelII_stats_out( "  WAVEGUIDE LENGTH:  %d", rda_adapt.rvp8NV_iwaveguide_length );
  for( i = 0; i < 11; i++ )
  {
    LevelII_stats_out( "  VERT REC NOISE SCALE-%d:  %f", i, rda_adapt.v_rnscale[i] );
  }
  LevelII_stats_out( "  VEL UNFOLDING OVERLAY THRESHOLD:  %f", rda_adapt.vel_data_tover );
  LevelII_stats_out( "  SPW UNFOLDING OVERLAY THRESHOLD:  %f", rda_adapt.width_data_tover );
  LevelII_stats_out( "  SPARE8752-0:  %d", rda_adapt.spare_8752[0] );
  LevelII_stats_out( "  SPARE8752-1:  %d", rda_adapt.spare_8752[1] );
  LevelII_stats_out( "  SPARE8752-2:  %d", rda_adapt.spare_8752[2] );
  LevelII_stats_out( "  START RANGE FOR FIRST DOPPLER RADIAL:  %f", rda_adapt.doppler_range_start );
  LevelII_stats_out( "  MAX INDEX FOR ELEV INDEX PARAMS:  %d", rda_adapt.max_el_index );
  LevelII_stats_out( "  CLUTTER MAP ELEV BNDRY SEG 2/3:  %f", rda_adapt.seg2lim );
  LevelII_stats_out( "  CLUTTER MAP ELEV BNDRY SEG 3/4:  %f", rda_adapt.seg3lim );
  LevelII_stats_out( "  CLUTTER MAP ELEV BNDRY SEG 4/5:  %f", rda_adapt.seg4lim );
  LevelII_stats_out( "  NUM ELEV SEGS IN CLUTTER MAP:  %d", rda_adapt.nbr_el_segments );
  LevelII_stats_out( "  HORIZ RECV NOISE LONG PULSE:  %f", rda_adapt.h_noise_long );
  LevelII_stats_out( "  ANTENNA NOISE TEMP:  %f", rda_adapt.ant_noise_temp );
  LevelII_stats_out( "  HORIZ RECV NOISE SHORT PULSE:  %f", rda_adapt.h_noise_short );
  LevelII_stats_out( "  HORIZ RECV NOISE TOLERANCE:  %f", rda_adapt.h_noise_tolerance );
  LevelII_stats_out( "  HORIZ DYNAMIC RANGE MIN:  %f", rda_adapt.h_min_dyn_range );
  LevelII_stats_out( "  AUX GENERATOR INSTALLED:  %s", rda_adapt.gen_installed );
  LevelII_stats_out( "  AUX GENERATOR AUTO EXER ENAB:  %s", rda_adapt.gen_exercies );
  LevelII_stats_out( "  VERT RECV NOISE TOLERANCE:  %f", rda_adapt.v_noise_tolerance );
  LevelII_stats_out( "  VERT DYNAMIC RANGE MIN:  %f", rda_adapt.v_min_dyn_range );
  LevelII_stats_out( "  ZDR BIAS DEGRADE LIMIT:  %f", rda_adapt.zdr_bias_degraded_lim );
  LevelII_stats_out( "  SPARE8836-0:  %d", rda_adapt.spare8836[0] );
  LevelII_stats_out( "  SPARE8836-1:  %d", rda_adapt.spare8836[1] );
  LevelII_stats_out( "  SPARE8836-2:  %d", rda_adapt.spare8836[2] );
  LevelII_stats_out( "  SPARE8836-3:  %d", rda_adapt.spare8836[3] );
  LevelII_stats_out( "  VERT RECV NOISE LONG PULSE:  %f", rda_adapt.v_noise_long );
  LevelII_stats_out( "  VERT RECV NOISE SHORT PULSE:  %f", rda_adapt.v_noise_short );
  LevelII_stats_out( "  ZDR UNFOLDING OVERLAY THRESHOLD:  %f", rda_adapt.zdr_data_tover );
  LevelII_stats_out( "  PHI UNFOLDING OVERLAY THRESHOLD:  %f", rda_adapt.phi_data_tover );
  LevelII_stats_out( "  RHO UNFOLDING OVERLAY THRESHOLD:  %f", rda_adapt.rho_data_tover );
  LevelII_stats_out( "  STALO PWR DEGRADE LIMIT:  %f", rda_adapt.stalo_pwr_dgrad_lim );
  LevelII_stats_out( "  STALO PWR MAINT LIMIT:  %f", rda_adapt.stalo_pwr_maint_lim );
  LevelII_stats_out( "  HORIZ PWR SENSE MIN:  %f", rda_adapt.min_h_pwr_sense );
  LevelII_stats_out( "  VERT PWR SENSE MIN:  %f", rda_adapt.min_v_pwr_sense );
  LevelII_stats_out( "  HORIZ PWR SENSE CALIB OFFSET:  %f", rda_adapt.min_h_pwr_sense_off );
  LevelII_stats_out( "  VERT PWR SENSE CALIB OFFSET:  %f", rda_adapt.min_v_pwr_sense_off );
  LevelII_stats_out( "  SPARE8888:  %d", rda_adapt.spare_8888 );
  LevelII_stats_out( "  RF PALLET BROADBAND LOSS:  %f", rda_adapt.rf_pallet_bb_loss );
  LevelII_stats_out( "  ZDR CHECK FAILURE THRESHOLD:  %f", rda_adapt.zdr_check_thr );
  LevelII_stats_out( "  PHI CHECK FAILURE THRESHOLD:  %f", rda_adapt.phi_check_thr );
  LevelII_stats_out( "  RHO CHECK FAILURE THRESHOLD:  %f", rda_adapt.rho_check_thr );
  for( i = 0; i < 13; i++ )
  {
    LevelII_stats_out( "  SPARE8808-%d:  %d", i, rda_adapt.spare_8808[i] );
  }
  LevelII_stats_out( "  AME PS TOLERANCE:  %f", rda_adapt.ame_ps_tolerance );
  LevelII_stats_out( "  MAX AME INT ALARM TEMP:  %f", rda_adapt.ame_max_temp );
  LevelII_stats_out( "  MIN AME INT ALARM TEMP:  %f", rda_adapt.ame_min_temp );
  LevelII_stats_out( "  MAX AME RECV MOD ALARM TEMP:  %f", rda_adapt.rcvr_mod_max_temp );
  LevelII_stats_out( "  MIN AME RECV MOD ALARM TEMP:  %f", rda_adapt.rcvr_mod_min_temp );
  LevelII_stats_out( "  MAX AME BITE MOD ALARM TEMP:  %f", rda_adapt.bite_mod_max_temp );
  LevelII_stats_out( "  MIN AME BITE MOD ALARM TEMP:  %f", rda_adapt.bite_mod_min_temp );
  LevelII_stats_out( "  DEFAULT POLARIZATION:  %d", rda_adapt.default_polarization );
  LevelII_stats_out( "  HORIZ TR LIMITER DEGRADE LIMIT:  %f", rda_adapt.h_tr_limit_degraded_lim );
  LevelII_stats_out( "  HORIZ TR LIMITER MAINT LIMIT:  %f", rda_adapt.h_tr_limit_maint_lim );
  LevelII_stats_out( "  VERT TR LIMITER DEGRADE LIMIT:  %f", rda_adapt.v_tr_limit_degraded_lim );
  LevelII_stats_out( "  VERT TR LIMITER MAINT LIMIT:  %f", rda_adapt.v_tr_limit_maint_lim );
  LevelII_stats_out( "  AME PELT CURRENT TOLERANCE:  %f", rda_adapt.ame_current_tol );
  LevelII_stats_out( "  HORIZ POLARIZATION:  %d", rda_adapt.h_only_polarization );
  LevelII_stats_out( "  VERT POLARIZATION:  %d", rda_adapt.v_only_polarization );
  LevelII_stats_out( "  SPARE8809-0:  %d", rda_adapt.spare_8809[0] );
  LevelII_stats_out( "  SPARE8809-1:  %d", rda_adapt.spare_8809[1] );
  LevelII_stats_out( "  ANTENNA REFLECTOR BIAS:  %f", rda_adapt.reflector_bias );
/* No use printing a bunch of spares at the end.
  for( i = 0; i < 109; i++ )
  {
    LevelII_stats_out( "  :  %d", rda_adapt.spare_8810[i] );
  }
*/
}

/************************************************************************
 Description: Print RDA Clutter Map message.
 ************************************************************************/

static void Print_rda_clutter( char *buf )
{
  int i,j,k;
  int clm_ptr = 0;
  short *clm_map = NULL;

  ORDA_clutter_map_msg_t *clutter_msg = (ORDA_clutter_map_msg_t *)buf;
  ORDA_clutter_map_t *clutter_map = &clutter_msg->map;
  ORDA_clutter_map_segment_t *clutter_segment;
  ORDA_clutter_map_filter_t *clutter_filter;

  LevelII_stats_out( "RDA CLUTTER MSG (MSG TYPE 15)" );
  LevelII_stats_out( "  DATE:       %d", clutter_map->date );
  LevelII_stats_out( "  TIME:       %d", clutter_map->time );
  LevelII_stats_out( "  NUM SEG:    %d", clutter_map->num_elevation_segs );

  /*
    Data is stored by segment (up to 5).
    Each segment has a 1-D array (num radials).
    There are 360 radials, each with up to 25 zones
    Zones are processed starting with bin 0.
    Each zone has a filter code and a stop range.
    Each zone is either:
      0 (Bypass clutter filters -- no clutter filtering)
      1 (Bypass map in control -- use Bypass Map to determine if clutter)
      2 (Force filter -- force clutter filtering regardless of Bypass Map)
  */

  clm_map = (short *) &clutter_map->data[0];
  clm_ptr = 0;

  for( i = 0; i < clutter_map->num_elevation_segs; i++ )
  {
    LevelII_stats_out( "ELEVATION SEGMENT: %d", i+1 );
    for( j = 0; j < NUM_AZIMUTH_SEGS_ORDA; j++ )
    {
      clutter_segment = ( ORDA_clutter_map_segment_t * ) &clm_map[clm_ptr];
      for( k = 0; k < clutter_segment->num_zones; k++ )
      {
        clutter_filter = ( ORDA_clutter_map_filter_t *) &clutter_segment->filter[k];
        LevelII_stats_out( "  AZ %d ZONE: %d OP: %d RNG: %d", j+1, k+1, clutter_filter->op_code, clutter_filter->range );
      }
      clm_ptr += ( ( clutter_segment->num_zones*sizeof( ORDA_clutter_map_filter_t )/sizeof( short ) )+1 );
    }
  }
}

/************************************************************************
 Description: Print RDA Bypass Map message.
 ************************************************************************/

static void Print_rda_bypass( char *buf )
{
  int i,j,k;
  int dataval;
  char printbuf[33];

  ORDA_bypass_map_msg_t *bypass_msg = (ORDA_bypass_map_msg_t *)buf;
  ORDA_bypass_map_t bypass_map = bypass_msg->bypass_map;
  ORDA_bypass_map_segment_t bypass_seg;

  LevelII_stats_out( "RDA BYPASS MSG (MSG TYPE 13)" );
  LevelII_stats_out( "  DATE:       %d", bypass_map.date );
  LevelII_stats_out( "  TIME:       %d", bypass_map.time );
  LevelII_stats_out( "  NUM SEGS:   %d", bypass_map.num_segs );

  /*
    Data is stored by segment (up to 5).
    Each segment has a 2-D array (num radials x halfwords per radial).
    There are 360 radials, each with 32 halfwords.
    The 32 halfwords equate to 512 bits, one for each range bin.
    Each bin is either:
      0 (perform clutter filtering.
      1 (bypass clutter filtering)
  */

  for( i = 0; i < bypass_map.num_segs; i++ )
  {
    bypass_seg = bypass_map.segment[i];
    LevelII_stats_out( "  SEGMENT: %d", bypass_seg.seg_num );
    for( j = 0; j < ORDA_BYPASS_MAP_RADIALS; j++ )
    {
      for( k = 0; k < HW_PER_RADIAL; k++ )
      {
        dataval = bypass_seg.data[j][k];
        if( dataval & BIT_0_MASK ){ printbuf[0] = '1'; }
        else{ printbuf[0] = '0'; }
        if( dataval & BIT_1_MASK ){ printbuf[1] = '1'; }
        else{ printbuf[1] = '0'; }
        if( dataval & BIT_2_MASK ){ printbuf[2] = '1'; }
        else{ printbuf[2] = '0'; }
        if( dataval & BIT_3_MASK ){ printbuf[3] = '1'; }
        else{ printbuf[3] = '0'; }
        if( dataval & BIT_4_MASK ){ printbuf[4] = '1'; }
        else{ printbuf[4] = '0'; }
        if( dataval & BIT_5_MASK ){ printbuf[5] = '1'; }
        else{ printbuf[5] = '0'; }
        if( dataval & BIT_6_MASK ){ printbuf[6] = '1'; }
        else{ printbuf[6] = '0'; }
        if( dataval & BIT_7_MASK ){ printbuf[7] = '1'; }
        else{ printbuf[7] = '0'; }
        if( dataval & BIT_8_MASK ){ printbuf[8] = '1'; }
        else{ printbuf[8] = '0'; }
        if( dataval & BIT_9_MASK ){ printbuf[9] = '1'; }
        else{ printbuf[9] = '0'; }
        if( dataval & BIT_10_MASK ){ printbuf[10] = '1'; }
        else{ printbuf[10] = '0'; }
        if( dataval & BIT_11_MASK ){ printbuf[11] = '1'; }
        else{ printbuf[11] = '0'; }
        if( dataval & BIT_12_MASK ){ printbuf[12] = '1'; }
        else{ printbuf[12] = '0'; }
        if( dataval & BIT_13_MASK ){ printbuf[13] = '1'; }
        else{ printbuf[13] = '0'; }
        if( dataval & BIT_14_MASK ){ printbuf[14] = '1'; }
        else{ printbuf[14] = '0'; }
        if( dataval & BIT_15_MASK ){ printbuf[15] = '1'; }
        else{ printbuf[15] = '0'; }
        if( dataval & BIT_16_MASK ){ printbuf[16] = '1'; }
        else{ printbuf[16] = '0'; }
        if( dataval & BIT_17_MASK ){ printbuf[17] = '1'; }
        else{ printbuf[17] = '0'; }
        if( dataval & BIT_18_MASK ){ printbuf[18] = '1'; }
        else{ printbuf[18] = '0'; }
        if( dataval & BIT_19_MASK ){ printbuf[19] = '1'; }
        else{ printbuf[19] = '0'; }
        if( dataval & BIT_20_MASK ){ printbuf[20] = '1'; }
        else{ printbuf[20] = '0'; }
        if( dataval & BIT_21_MASK ){ printbuf[21] = '1'; }
        else{ printbuf[21] = '0'; }
        if( dataval & BIT_22_MASK ){ printbuf[22] = '1'; }
        else{ printbuf[22] = '0'; }
        if( dataval & BIT_23_MASK ){ printbuf[23] = '1'; }
        else{ printbuf[23] = '0'; }
        if( dataval & BIT_24_MASK ){ printbuf[24] = '1'; }
        else{ printbuf[24] = '0'; }
        if( dataval & BIT_25_MASK ){ printbuf[25] = '1'; }
        else{ printbuf[25] = '0'; }
        if( dataval & BIT_26_MASK ){ printbuf[26] = '1'; }
        else{ printbuf[26] = '0'; }
        if( dataval & BIT_27_MASK ){ printbuf[27] = '1'; }
        else{ printbuf[27] = '0'; }
        if( dataval & BIT_28_MASK ){ printbuf[28] = '1'; }
        else{ printbuf[28] = '0'; }
        if( dataval & BIT_29_MASK ){ printbuf[29] = '1'; }
        else{ printbuf[29] = '0'; }
        if( dataval & BIT_30_MASK ){ printbuf[30] = '1'; }
        else{ printbuf[30] = '0'; }
        if( dataval & BIT_31_MASK ){ printbuf[31] = '1'; }
        else{ printbuf[31] = '0'; }
        printbuf[32] = '\0';
        LevelII_stats_out( "  RAD: %03d HW: %02d DATA: %s", j+1, k+1, printbuf );
      }
    }
  }
}

/************************************************************************
 Description: Print RDA VCP message.
 ************************************************************************/

static void Print_rda_vcp( char *buf )
{
  int i;

  VCP_ICD_msg_t *rda_vcp_msg = (VCP_ICD_msg_t *)( buf+sizeof( RDA_RPG_message_header_t ) );
  VCP_message_header_t elev_hdr = rda_vcp_msg->vcp_msg_hdr;
  VCP_elevation_cut_header_t elev_data = rda_vcp_msg->vcp_elev_data;

  LevelII_stats_out( "RDA VCP MSG (MSG TYPE 5)" );
  LevelII_stats_out( "  MSG SIZE:       %d", elev_hdr.msg_size );
  LevelII_stats_out( "  PATTERN TYPE:   %d", elev_hdr.pattern_type );
  LevelII_stats_out( "  PATTERN NUM:    %d", elev_hdr.pattern_number );
  LevelII_stats_out( "  NUM CUTS:       %d", elev_data.number_cuts );
  LevelII_stats_out( "  CLUTTER GROUP:  %d", elev_data.group );
  LevelII_stats_out( "  DOPPLER RES:    %d", elev_data.doppler_res );
  LevelII_stats_out( "  PULSE WIDTH:    %d", elev_data.pulse_width );
  LevelII_stats_out( "  SPARE7:         %d", elev_data.spare7 );
  LevelII_stats_out( "  SPARE8:         %d", elev_data.spare8 );
  LevelII_stats_out( "  SPARE9:         %d", elev_data.spare9 );
  LevelII_stats_out( "  SPARE10:        %d", elev_data.spare10 );
  LevelII_stats_out( "  SPARE11         %d\n", elev_data.spare11 );
  for( i = 0; i < elev_data.number_cuts; i++ )
  {
    LevelII_stats_out( "  CUT[%02d]", i+1 );
    LevelII_stats_out( "  ANGLE:             %f", elev_data.data[i].angle*BAMS_ELEV );
    LevelII_stats_out( "  PHASE:             %d", elev_data.data[i].phase );
    LevelII_stats_out( "  WAVEFORM:          %d", elev_data.data[i].waveform );
    LevelII_stats_out( "  SR:                %d", elev_data.data[i].super_res );
    LevelII_stats_out( "  SURV PRF#:         %d", elev_data.data[i].surv_prf_num );
    LevelII_stats_out( "  SURV PRF PULSE:    %d", elev_data.data[i].surv_prf_pulse );
    LevelII_stats_out( "  AZ RATE:           %f", elev_data.data[i].azimuth_rate*BAMS_AZ );
    LevelII_stats_out( "  REF THRESH:        %f", elev_data.data[i].refl_thresh/8.0 );
    LevelII_stats_out( "  VEL THRESH:        %f", elev_data.data[i].vel_thresh/8.0 );
    LevelII_stats_out( "  SPW THRESH:        %f", elev_data.data[i].sw_thresh/8.0 );
    LevelII_stats_out( "  DIFF REFL THRESH:  %f", elev_data.data[i].diff_refl_thresh/8.0 );
    LevelII_stats_out( "  DIFF PHASE THRESH: %f", elev_data.data[i].diff_phase_thresh/8.0 );
    LevelII_stats_out( "  CORR COEFF THRESH: %f", elev_data.data[i].corr_coeff_thresh/8.0 );
    LevelII_stats_out( "  EDGE ANGLE 1:      %f", elev_data.data[i].edge_angle1*BAMS_ELEV );
    LevelII_stats_out( "  DOP PRF #1:        %d", elev_data.data[i].dopp_prf_num1 );
    LevelII_stats_out( "  DOP PRF PULSE #1:  %d", elev_data.data[i].dopp_prf_pulse1 );
    LevelII_stats_out( "  SPARE 15:          %d", elev_data.data[i].spare15 );
    LevelII_stats_out( "  EDGE ANGLE 2:      %f", elev_data.data[i].edge_angle2*BAMS_ELEV );
    LevelII_stats_out( "  DOP PRF #2:        %d", elev_data.data[i].dopp_prf_num2 );
    LevelII_stats_out( "  DOP PRF PULSE #2:  %d", elev_data.data[i].dopp_prf_pulse2 );
    LevelII_stats_out( "  SPARE 19:          %d", elev_data.data[i].spare19 );
    LevelII_stats_out( "  EDGE ANGLE 3:      %f", elev_data.data[i].edge_angle3*BAMS_ELEV );
    LevelII_stats_out( "  DOP PRF #3:        %d", elev_data.data[i].dopp_prf_num3 );
    LevelII_stats_out( "  DOP PRF PULSE #3:  %d", elev_data.data[i].dopp_prf_pulse3 );
    LevelII_stats_out( "  SPARE 23:          %d\n", elev_data.data[i].spare23 );
  }
}

/************************************************************************
 Description: Print RDA PMD message.
 ************************************************************************/

static void Print_rda_pmd( char *buf )
{
  int i;

  orda_pmd_t *orda_pmd = (orda_pmd_t *)buf;
  Pmd_t pmd = orda_pmd->pmd;

  LevelII_stats_out( "RDA PMD MSG (MSG TYPE 3)" );
  LevelII_stats_out( "COMMS" );
  LevelII_stats_out( "  SPARE1:  %d", pmd.spare1 );
  LevelII_stats_out( "  LOOPBACK TEST STATUS:               %d", pmd.loop_back_test_status );
  LevelII_stats_out( "  T1 OUTPUT FRAMES:                   %d", pmd.t1_output_frames );
  LevelII_stats_out( "  T1 INTPUT FRAMES:                   %d", pmd.t1_input_frames );
  LevelII_stats_out( "  ROUTER MEM BYTES USED:              %d", pmd.router_mem_used );
  LevelII_stats_out( "  ROUTER MEM BYTES FREE:              %d", pmd.router_mem_free );
  LevelII_stats_out( "  ROUTER MEM UTIL PCT:                %d", pmd.router_mem_util );
  LevelII_stats_out( "  SPARE12:                            %d", pmd.spare12 );
  LevelII_stats_out( "  CSU LOSS OF SIGNAL:                 %d", pmd.csu_loss_of_signal );
  LevelII_stats_out( "  CSU LOSS OF FRAMES:                 %d", pmd.csu_loss_of_frames );
  LevelII_stats_out( "  CSU YELLOW ALARMS:                  %d", pmd.csu_yellow_alarms );
  LevelII_stats_out( "  CSU BLUE ALARMS:                    %d", pmd.csu_blue_alarms );
  LevelII_stats_out( "  CSU 24 HR ERR SECS:                 %d", pmd.csu_24hr_err_scnds );
  LevelII_stats_out( "  CSU 24 HR SEVERE ERR SECS:          %d", pmd.csu_24hr_sev_err_scnds );
  LevelII_stats_out( "  CSU 24 HR SEVERE ERR FRAMING SECS:  %d", pmd.csu_24hr_sev_err_frm_scnds );
  LevelII_stats_out( "  CSU 24 HR UNAVAIL SECS:             %d", pmd.csu_24hr_unavail_scnds );
  LevelII_stats_out( "  CSU 24 HR CONTROLLED SLIP SECS:     %d", pmd.csu_24hr_cntrld_slip_scnds );
  LevelII_stats_out( "  CSU 24 HR PATH CODING VIOLATIONS:   %d", pmd.csu_24hr_path_cding_vlns );
  LevelII_stats_out( "  CSU 24 HR LINE ERR SECS:            %d", pmd.csu_24hr_line_err_scnds );
  LevelII_stats_out( "  CSU 24 HR BURSTY ERR SECS:          %d", pmd.csu_24hr_brsty_err_scnds );
  LevelII_stats_out( "  CSU 24 HR DEGRADED MINS:            %d", pmd.csu_24hr_degraded_mins );
  LevelII_stats_out( "  LAN SWITCH MEM BYTES USED:          %d", pmd.lan_switch_mem_used );
  LevelII_stats_out( "  LAN SWITCH MEM BYTES FREE:          %d", pmd.lan_switch_mem_free );
  LevelII_stats_out( "  LAN SWITCH MEM UTIL PCT:            %d", pmd.lan_switch_mem_util );
  LevelII_stats_out( "  SPARE44:                            %d", pmd.spare44 );
  LevelII_stats_out( "  NTP REJECT PKTS:                    %d", pmd.ntp_rejected_packets );
  LevelII_stats_out( "  NTP EST TIME ERR:                   %d", pmd.ntp_est_time_error );
  LevelII_stats_out( "  GPS SATELLITES:                     %d", pmd.gps_satellites );
  LevelII_stats_out( "  GPS MAX SIGNAL STRENGTH:            %d", pmd.gps_max_sig_strength );
  LevelII_stats_out( "  IPC STATUS:                         %d", pmd.ipc_status );
  LevelII_stats_out( "  CMD CHANNEL CONTROL:                %d", pmd.cmd_chnl_ctrl );
  LevelII_stats_out( "  DAU TEST 0:                         %d", pmd.dau_tst_0 );
  LevelII_stats_out( "  DAU TEST 1:                         %d", pmd.dau_tst_1 );
  LevelII_stats_out( "  DAU TEST 2:                         %d", pmd.dau_tst_2 );
  LevelII_stats_out( "AME" );
  LevelII_stats_out( "  POLARIZATION:               %d", pmd.polarization );
  LevelII_stats_out( "  AME INTERNAL TEMP:          %f", pmd.internal_temp );
  LevelII_stats_out( "  AME RECV MOD TEMP:          %f", pmd.rec_module_temp );
  LevelII_stats_out( "  AME BITE/CAL MOD TEMP:      %f", pmd.bite_cal_module_temp );
  LevelII_stats_out( "  AME PELT PULSE WID MOD:     %d", pmd.peltier_pulse_width_modulation );
  LevelII_stats_out( "  AME PELT STATUS:            %d", pmd.peltier_status );
  LevelII_stats_out( "  AME A/D CONV STATUS:        %d", pmd.a2d_converter_status );
  LevelII_stats_out( "  AME STATE:                  %d", pmd.ame_state );
  LevelII_stats_out( "  AME +3.3V PS:               %f", pmd.p_33vdc_ps );
  LevelII_stats_out( "  AME +5V PS:                 %f", pmd.p_50vdc_ps );
  LevelII_stats_out( "  AME +6.5V PS:               %f", pmd.p_65vdc_ps );
  LevelII_stats_out( "  AME +15V PS:                %f", pmd.p_150vdc_ps );
  LevelII_stats_out( "  AME +48V PS:                %f", pmd.p_480vdc_ps );
  LevelII_stats_out( "  AME STALO PWR:              %f", pmd.stalo_power );
  LevelII_stats_out( "  PELT CURRENT:               %f", pmd.peltier_current );
  LevelII_stats_out( "  ADC CALIB REF VOLTAGE:      %f", pmd.adc_calib_ref_voltage );
  LevelII_stats_out( "  AME MODE:                   %d", pmd.mode );
  LevelII_stats_out( "  AME PELT MODE:              %d", pmd.peltier_mode );
  LevelII_stats_out( "  AME PELT INT FAN CURRENT:   %f", pmd.peltier_inside_fan_current );
  LevelII_stats_out( "  AME PELT EXT FAN CURRENT:   %f", pmd.peltier_outside_fan_current );
  LevelII_stats_out( "  HORIZ TR LIMITER VOLTAGE:   %f", pmd.h_tr_limiter_voltage );
  LevelII_stats_out( "  VERT TR LIMITER VOLTAGE:    %f", pmd.v_tr_limiter_voltage );
  LevelII_stats_out( "  ADC CALIB OFFSET VOLTAGE:   %f", pmd.adc_calib_offset_voltage );
  LevelII_stats_out( "  ADC CALIB GAIN CORRECTION:  %f", pmd.adc_calib_gain_correction );
  LevelII_stats_out( "POWER" );
  LevelII_stats_out( "  UPS BATTERY STATUS:    %d", pmd.ups_batt_status );
  LevelII_stats_out( "  UPS TIME ON BATTERY:   %d", pmd.ups_time_on_batt );
  LevelII_stats_out( "  UPS BATTERY TEMP:      %f", pmd.ups_batt_temp );
  LevelII_stats_out( "  UPS OUTPUT VOLTAGE:    %f", pmd.ups_output_volt );
  LevelII_stats_out( "  UPS OUTPUT FREQUENCY:  %f", pmd.ups_output_freq );
  LevelII_stats_out( "  UPS OUTPUT CURRENT:    %f", pmd.ups_output_current );
  LevelII_stats_out( "  POWER ADMIN LOAD:      %f", pmd.pwr_admin_load );
  for( i = 0; i < 24; i++ )
  {
    LevelII_stats_out( "  SPARE113-%d:           %d", i, pmd.spare113[i] );
  }
  LevelII_stats_out( "TRANSMITTER" );
  LevelII_stats_out( "  +5 VDC PS:  %d", pmd.p_5vdc_ps );
  LevelII_stats_out( "  +15 VDC PS:  %d", pmd.p_15vdc_ps );
  LevelII_stats_out( "  +28 VDC PS:  %d", pmd.p_28vdc_ps );
  LevelII_stats_out( "  -15 VDC PS:  %d", pmd.n_15vdc_ps );
  LevelII_stats_out( "  +45 VDC PS:  %d", pmd.p_45vdc_ps );
  LevelII_stats_out( "  FILAMENT PS VOLTAGE:  %d", pmd.flmnt_ps_vlt );
  LevelII_stats_out( "  VACUUM PUMP PS VOLTAGE:  %d", pmd.vcum_pmp_ps_vlt );
  LevelII_stats_out( "  FOCUS COIL PS VOLTAGE:  %d", pmd.fcs_coil_ps_vlt );
  LevelII_stats_out( "  FILAMENT PS:  %d", pmd.flmnt_ps );
  LevelII_stats_out( "  KLYSTRON WARMUP:  %d", pmd.klystron_warmup );
  LevelII_stats_out( "  XMTR AVAILABLE:  %d", pmd.trsmttr_avlble );
  LevelII_stats_out( "  WG SWITCH POSITION:  %d", pmd.wg_swtch_position );
  LevelII_stats_out( "  WG/PFN TRANS INTERLOCK:  %d", pmd.wg_pfn_trsfr_intrlck );
  LevelII_stats_out( "  MAINT MODE:  %d", pmd.mntnce_mode );
  LevelII_stats_out( "  MAINT REQD:  %d", pmd.mntnce_reqd );
  LevelII_stats_out( "  PFN SWITCH POSITION:  %d", pmd.pfn_swtch_position );
  LevelII_stats_out( "  MODULATOR OVERLOAD:  %d", pmd.modular_ovrld );
  LevelII_stats_out( "  MODULATOR INV CURRENT:  %d", pmd.modulator_inv_crnt );
  LevelII_stats_out( "  MODULATOR SWITCH FAIL:  %d", pmd.modulator_swtch_fail );
  LevelII_stats_out( "  MAIN POWER VOLTAGE:  %d", pmd.main_pwr_vlt );
  LevelII_stats_out( "  CHARGING SYS FAIL:  %d", pmd.chrg_sys_fail );
  LevelII_stats_out( "  INV DIODE CURRENT:  %d", pmd.invrs_diode_crnt );
  LevelII_stats_out( "  TRIGGER AMP:  %d", pmd.trggr_amp );
  LevelII_stats_out( "  CIRCULATOR TEMP:  %d", pmd.circulator_temp );
  LevelII_stats_out( "  SPECTRUM FILTER PRESSURE:  %d", pmd.spctrm_fltr_pressure );
  LevelII_stats_out( "  WG ARC/VSWR:  %d", pmd.wg_arc_vswr );
  LevelII_stats_out( "  CABINET INTERLOCK:  %d", pmd.cbnt_interlock );
  LevelII_stats_out( "  CABINET AIR TEMP:  %d", pmd.cbnt_air_temp );
  LevelII_stats_out( "  CABINET AIRFLOW:  %d", pmd.cbnt_air_flow );
  LevelII_stats_out( "  KLYSTRON CURRENT:  %d", pmd.klystron_crnt );
  LevelII_stats_out( "  KLYSTRON FILAMENT CURRENT:  %d", pmd.klystron_flmnt_crnt );
  LevelII_stats_out( "  KLYSTRON VACION CURRENT:  %d", pmd.klystron_vacion_crnt );
  LevelII_stats_out( "  KLYSTRON AIR TEMP:  %d", pmd.klystron_air_temp );
  LevelII_stats_out( "  KLYSTRON AIR FLOW:  %d", pmd.klystron_air_flow );
  LevelII_stats_out( "  MODULATOR SWITCH MAINT:  %d", pmd.modulator_swtch_mntnce );
  LevelII_stats_out( "  POST CHARGE REG MAINT:  %d", pmd.post_chrg_regulator );
  LevelII_stats_out( "  WG PRESSURE/HUMIDITY:  %d", pmd.wg_pressure_humidity );
  LevelII_stats_out( "  XMTR OVERVOLTAGE:  %d", pmd.trsmttr_ovr_vlt );
  LevelII_stats_out( "  XMTR OVERCURRENT:  %d", pmd.trsmttr_ovr_crnt );
  LevelII_stats_out( "  FOCUS COIL CURRENT:  %d", pmd.fcs_coil_crnt );
  LevelII_stats_out( "  FOCUS COIL AIRFLOW:  %d", pmd.fcs_coil_air_flow );
  LevelII_stats_out( "  OIL TEMP:  %d", pmd.oil_temp );
  LevelII_stats_out( "  PRF LIMIT:  %d", pmd.prf_limit );
  LevelII_stats_out( "  XMTR OIL LEVEL:  %d", pmd.trsmttr_oil_lvl );
  LevelII_stats_out( "  XMTR BATTERY CHARGING:  %d", pmd.trsmttr_batt_chrgng );
  LevelII_stats_out( "  HIGH VOLTAGE STATUS:  %d", pmd.hv_status );
  LevelII_stats_out( "  XMTR RECYCLING SUMMARY:  %d", pmd.trsmttr_recycling_smmry );
  LevelII_stats_out( "  XMTR INOP:  %d", pmd.trsmttr_inoperable );
  LevelII_stats_out( "  XMTR AIR FILTER:  %d", pmd.trsmttr_air_fltr );
  LevelII_stats_out( "  ZERO TEST BIT 0:  %d", pmd.zero_tst_bit_0 );
  LevelII_stats_out( "  ZERO TEST BIT 1:  %d", pmd.zero_tst_bit_1 );
  LevelII_stats_out( "  ZERO TEST BIT 2:  %d", pmd.zero_tst_bit_2 );
  LevelII_stats_out( "  ZERO TEST BIT 3:  %d", pmd.zero_tst_bit_3 );
  LevelII_stats_out( "  ZERO TEST BIT 4:  %d", pmd.zero_tst_bit_4 );
  LevelII_stats_out( "  ZERO TEST BIT 5:  %d", pmd.zero_tst_bit_5 );
  LevelII_stats_out( "  ZERO TEST BIT 6:  %d", pmd.zero_tst_bit_6 );
  LevelII_stats_out( "  ZERO TEST BIT 7:  %d", pmd.zero_tst_bit_7 );
  LevelII_stats_out( "  ONE TEST BIT 0:  %d", pmd.one_tst_bit_0 );
  LevelII_stats_out( "  ONE TEST BIT 1:  %d", pmd.one_tst_bit_1 );
  LevelII_stats_out( "  ONE TEST BIT 2:  %d", pmd.one_tst_bit_2 );
  LevelII_stats_out( "  ONE TEST BIT 3:  %d", pmd.one_tst_bit_3 );
  LevelII_stats_out( "  ONE TEST BIT 4:  %d", pmd.one_tst_bit_4 );
  LevelII_stats_out( "  ONE TEST BIT 5:  %d", pmd.one_tst_bit_5 );
  LevelII_stats_out( "  ONE TEST BIT 6:  %d", pmd.one_tst_bit_6 );
  LevelII_stats_out( "  ONE TEST BIT 7:  %d", pmd.one_tst_bit_7 );
  LevelII_stats_out( "  XMTR/DAU INTFCE:  %d", pmd.xmtr_dau_interface );
  LevelII_stats_out( "  XMTR SUMMARY STATUS:  %d", pmd.trsmttr_smmry_status );
  LevelII_stats_out( "  SPARE204:  %d", pmd.spare204 );
  LevelII_stats_out( "  XMTR RF PWR:  %f", pmd.trsmttr_rf_pwr );
  LevelII_stats_out( "  HORIZ XMTR PEAK PWR:  %f", pmd.h_trsmttr_peak_pwr );
  LevelII_stats_out( "  XMTR PEAK PWR:  %f", pmd.xmtr_peak_pwr );
  LevelII_stats_out( "  VERT XMTR PEAK PWR:  %f", pmd.v_trsmttr_peak_pwr );
  LevelII_stats_out( "  XMTR RF AVG PWR:  %f", pmd.xmtr_rf_avg_pwr );
  LevelII_stats_out( "  XMTR POWER METER ZERO:  %d", pmd.xmtr_pwr_mtr_zero );
  LevelII_stats_out( "  SPARE216:  %d", pmd.spare216 );
  LevelII_stats_out( "  XMTR RECYCLE COUNT:  %d", pmd.xmtr_recycle_cnt );
  LevelII_stats_out( "  RECIEVER BIAS:  %f", pmd.receiver_bias );
  LevelII_stats_out( "  TRANSMIT_IMBALANCE:  %f", pmd.transmit_imbalance );
  for( i = 0; i < 6; i++ )
  {
    LevelII_stats_out( "  SPARE223-%d:  %d", i, pmd.spare223[i] );
  }
  LevelII_stats_out( "UTILITIES" );
  LevelII_stats_out( "  AC #1 COMPRESSOR SHUTOFF:       %d", pmd.ac_1_cmprsr_shut_off );
  LevelII_stats_out( "  AC #1 COMPRESSOR SHUTOFF:       %d", pmd.ac_2_cmprsr_shut_off );
  LevelII_stats_out( "  GENERATOR MAINT REQD:           %d", pmd.gnrtr_mntnce_reqd );
  LevelII_stats_out( "  GENERATOR BATTERY VOLTAGE:      %d", pmd.gnrtr_batt_vlt );
  LevelII_stats_out( "  GENERATOR ENGINE:               %d", pmd.gnrtr_engn );
  LevelII_stats_out( "  GENERATOR VOLT/FREQUENCY:       %d", pmd.gnrtr_vlt_freq );
  LevelII_stats_out( "  POWER SOURCE:                   %d", pmd.pwr_src );
  LevelII_stats_out( "  TRANS PWR SOURCE (TPS):         %d", pmd.trans_pwr_src );
  LevelII_stats_out( "  GENERATOR AUTO/RUN/OFF SWITCH:  %d", pmd.gen_auto_run_off_switch );
  LevelII_stats_out( "  AIRCRAFT HAZARD LIGHT:          %d", pmd.aircraft_hzrd_lighting );
  LevelII_stats_out( "  DAU UART:                       %d", pmd.dau_uart );
  for( i = 0; i < 10; i++ )
  {
    LevelII_stats_out( "  SPARE240-%d:                    %d", i, pmd.spare240[i] );
  }
  LevelII_stats_out( "SHELTER" );
  LevelII_stats_out( "  EQUIP FIRE DETECT SYS:   %d", pmd.equip_shltr_fire_sys );
  LevelII_stats_out( "  EQUIP FIRE/SMOKE:        %d", pmd.equip_shltr_fire_smk );
  LevelII_stats_out( "  GENERATOR FIRE/SMOKE:    %d", pmd.gnrtr_shltr_fire_smk );
  LevelII_stats_out( "  UTIL VOLTAGE/FREQUENCY:  %d", pmd.utlty_vlt_freq );
  LevelII_stats_out( "  SITE SECURITY ALARM:     %d", pmd.site_scrty_alarm );
  LevelII_stats_out( "  SECURITY EQUIP:          %d", pmd.scrty_equip );
  LevelII_stats_out( "  SECURITY SYS:            %d", pmd.scrty_sys );
  LevelII_stats_out( "  RECV CONN TO ANTENNA:    %d", pmd.rcvr_cnctd_to_antna );
  LevelII_stats_out( "  RADOME HATCH:            %d", pmd.radome_hatch );
  LevelII_stats_out( "  AC #1 FILTER DIRTY:      %d", pmd.ac_1_fltr_drty );
  LevelII_stats_out( "  AC #2 FILTER DIRTY:      %d", pmd.ac_2_fltr_drty );
  LevelII_stats_out( "  EQUIP TEMP:              %f", pmd.equip_shltr_temp );
  LevelII_stats_out( "  OUTSIDE TEMP:            %f", pmd.outside_amb_temp );
  LevelII_stats_out( "  XMTR LEAVING TEMP:       %f", pmd.trsmttr_leaving_air_temp );
  LevelII_stats_out( "  AC #1 DISCHARGE TEMP:    %f", pmd.ac_1_dschrg_air_temp );
  LevelII_stats_out( "  GENERATOR SHELTER TEMP:  %f", pmd.gnrtr_shltr_temp );
  LevelII_stats_out( "  RADOME AIR TEMP:         %f", pmd.radome_air_temp );
  LevelII_stats_out( "  AC #2 DISCHARGE TEMP:    %f", pmd.ac_2_dschrg_air_temp );
  LevelII_stats_out( "  DAU +15v PS:             %f", pmd.dau_p_15v_ps );
  LevelII_stats_out( "  DAU -15v PS:             %f", pmd.dau_n_15v_ps );
  LevelII_stats_out( "  DAU +28v PS:             %f", pmd.dau_p_28v_ps );
  LevelII_stats_out( "  DAU +5v PS:              %f", pmd.dau_p_5v_ps );
  LevelII_stats_out( "  GENERATOR FUEL LEVEL:    %d", pmd.cnvrtd_gnrtr_fuel_lvl );
  for( i = 0; i < 7; i++ )
  {
    LevelII_stats_out( "  SPARE284-%d:             %d", i, pmd.spare284[i] );
  }
  LevelII_stats_out( "PEDESTAL" );
  LevelII_stats_out( "  PED +28v PS:               %f", pmd.pdstl_p_28v_ps );
  LevelII_stats_out( "  PED +15v PS:               %f", pmd.pdstl_p_15v_ps );
  LevelII_stats_out( "  ENC +5v PS:                %f", pmd.encdr_p_5v_ps );
  LevelII_stats_out( "  PED +5v PS:                %f", pmd.pdstl_p_5v_ps );
  LevelII_stats_out( "  PED -15v PS:               %f", pmd.pdstl_n_15v_ps );
  LevelII_stats_out( "  +150V OVERVOLTAGE:         %d", pmd.p_150v_ovrvlt );
  LevelII_stats_out( "  +150V UNDERVOLTAGE:        %d", pmd.p_150v_undrvlt );
  LevelII_stats_out( "  ELEV SERVO AMP INHIBIT:    %d", pmd.elev_srvo_amp_inhbt );
  LevelII_stats_out( "  ELEV SERVO AMP SHORT CIR:  %d", pmd.elev_srvo_amp_shrt_crct );
  LevelII_stats_out( "  ELEV SERVO AMP OVERTEMP:   %d", pmd.elev_srvo_amp_ovr_temp );
  LevelII_stats_out( "  ELEV MOTOR OVERTEMP:       %d", pmd.elev_motor_ovr_temp );
  LevelII_stats_out( "  ELEV STOW PIN:             %d", pmd.elev_stow_pin );
  LevelII_stats_out( "  ELEV PCU PARITY:           %d", pmd.elev_pcu_parity );
  LevelII_stats_out( "  ELEV DEAD LIMIT:           %d", pmd.elev_dead_lmt );
  LevelII_stats_out( "  ELEV +NORMAL LIMIT:        %d", pmd.elev_p_nrml_lmt );
  LevelII_stats_out( "  ELEV -NORMAL LIMIT:        %d", pmd.elev_n_nrml_lmt );
  LevelII_stats_out( "  ELEV ENCODER LIGHT:        %d", pmd.elev_encdr_light );
  LevelII_stats_out( "  ELEV GEARBOX OIL:          %d", pmd.elev_grbx_oil );
  LevelII_stats_out( "  ELEV HANDWHEEL:            %d", pmd.elev_handwheel );
  LevelII_stats_out( "  ELEV AMP PS:               %d", pmd.elev_amp_ps );
  LevelII_stats_out( "  AZ SERVO AMP INHIBIT:      %d", pmd.azmth_srvo_amp_inhbt );
  LevelII_stats_out( "  AZ SERVO AMP SHORT CIR:    %d", pmd.azmth_srvo_amp_shrt_crct );
  LevelII_stats_out( "  AZ SERVO OVERTEMP:         %d", pmd.azmth_srvo_amp_ovr_temp );
  LevelII_stats_out( "  AZ MOTOR OVERTEMP:         %d", pmd.azmth_motor_ovr_temp );
  LevelII_stats_out( "  AZ STOW PIN:               %d", pmd.azmth_stow_pin );
  LevelII_stats_out( "  AZ PCU PARITY:             %d", pmd.azmth_pcu_parity );
  LevelII_stats_out( "  AZ ENCODER LIGHT:          %d", pmd.azmth_encdr_light );
  LevelII_stats_out( "  AZ GEARBOX OIL:            %d", pmd.azmth_grbx_oil );
  LevelII_stats_out( "  AZ BULL GEAR OIL:          %d", pmd.azmth_bull_gr_oil );
  LevelII_stats_out( "  AZ HANDWHEEL:              %d", pmd.azmth_handwheel );
  LevelII_stats_out( "  AZ SERVO AMP PS:           %d", pmd.azmth_srvo_amp_ps );
  LevelII_stats_out( "  SERVO:                     %d", pmd.srvo );
  LevelII_stats_out( "  PED INTERLOCK SWITCH:      %d", pmd.pdstl_intrlock_swtch );
  LevelII_stats_out( "  AZ POS CORRECTION:         %d", pmd.azmth_pos_correction );
  LevelII_stats_out( "  ELEV POS CORRECTION:       %d", pmd.elev_pos_correction );
  LevelII_stats_out( "  SELF TEST 1 STATUS:        %d", pmd.slf_tst_1_status );
  LevelII_stats_out( "  SELF TEST 2 STATUS:        %d", pmd.slf_tst_2_status );
  LevelII_stats_out( "  SELF TEST 2 DATA2 DATA:    %d", pmd.slf_tst_2_data );
  for( i = 0; i < 7; i++ )
  {
    LevelII_stats_out( "  SPARE334-%d:               %d", i, pmd.spare334[i] );
  }
  LevelII_stats_out( "RECEIVER" );
  LevelII_stats_out( "  COHO/CLOCK:               %d", pmd.coho_clock );
  LevelII_stats_out( "  RF GEN FREQ SEL OSCILL:   %d", pmd.rf_gnrtr_freq_slct_osc );
  LevelII_stats_out( "  RF GEN RF/STALO:          %d", pmd.rf_gnrtr_rf_stalo );
  LevelII_stats_out( "  RF GEN PHASE SHIFT COHO:  %d", pmd.rf_gnrtr_phase_shft_coho );
  LevelII_stats_out( "  +9v RECV PS:              %d", pmd.p_9v_rcvr_ps );
  LevelII_stats_out( "  +5v RECV PS:              %d", pmd.p_5v_rcvr_ps );
  LevelII_stats_out( "  +/-18v RECV PS:           %d", pmd.pn_18v_rcvr_ps );
  LevelII_stats_out( "  -9v RECV PS:              %d", pmd.n_9v_rcvr_ps );
  LevelII_stats_out( "  +5v RECV PROTECT PS:      %d", pmd.p_5v_rcvr_prtctr_ps );
  LevelII_stats_out( "  SPARE350:                 %d", pmd.spare350 );
  LevelII_stats_out( "  HORIZ SHORT PULSE NOISE:  %f", pmd.h_shrt_pulse_noise );
  LevelII_stats_out( "  HORIZ LONG PULSE NOISE:   %f", pmd.h_long_pulse_noise );
  LevelII_stats_out( "  HORIZ NOISE TEMP:         %f", pmd.h_noise_temp );
  LevelII_stats_out( "  VERT SHORT PULSE NOISE:   %f", pmd.v_shrt_pulse_noise );
  LevelII_stats_out( "  VERT LONG PULSE NOISE:    %f", pmd.v_long_pulse_noise );
  LevelII_stats_out( "  VERT NOISE TEMP:          %f", pmd.v_noise_temp );
  LevelII_stats_out( "CALIBRATION" );
  LevelII_stats_out( "  HORIZ LINEARITY:              %f", pmd.h_linearity );
  LevelII_stats_out( "  HORIZ DYN RANGE:              %f", pmd.h_dynamic_range );
  LevelII_stats_out( "  HORIZ ddBZ0:                  %f", pmd.h_delta_dbz0 );
  LevelII_stats_out( "  VERT ddBZ0:                   %f", pmd.v_delta_dbz0 );
  LevelII_stats_out( "  KD PEAK MEASURED:             %f", pmd.kd_peak_measured );
  LevelII_stats_out( "  SPARE373-0:                   %d", pmd.spare373[0] );
  LevelII_stats_out( "  SPARE373-1:                   %d", pmd.spare373[1] );
  LevelII_stats_out( "  HORIZ SHORT PULSE ddBZ0:      %f", pmd.shrt_pls_h_dbz0 );
  LevelII_stats_out( "  HORIZ LONG PULSE ddBZ0:       %f", pmd.long_pls_h_dbz0 );
  LevelII_stats_out( "  VEL PROCESSED:                %d", pmd.velocity_prcssd );
  LevelII_stats_out( "  SPW PROCESSED:                %d", pmd.width_prcssd );
  LevelII_stats_out( "  VEL RF GEN:                   %d", pmd.velocity_rf_gen );
  LevelII_stats_out( "  SPW RF GEN:                   %d", pmd.width_rf_gen );
  LevelII_stats_out( "  HORIZ I0:                     %f", pmd.h_i_naught );
  LevelII_stats_out( "  VERT I0:                      %f", pmd.v_i_naught );
  LevelII_stats_out( "  VERT DYN RANGE:               %f", pmd.v_dynamic_range );
  LevelII_stats_out( "  VERT SHORT PULSE ddBZ0:       %f", pmd.shrt_pls_v_dbz0 );
  LevelII_stats_out( "  VERT LONG PULSE ddBZ0:        %f", pmd.long_pls_v_dbz0 );
  for( i = 0; i < 4; i++ )
  {
    LevelII_stats_out( "  SPARE-%d:                     %d", i, pmd.spare393[i] );
  }
  LevelII_stats_out( "  HORIZ PWR SENSE:              %f", pmd.h_pwr_sense );
  LevelII_stats_out( "  VERT PWR SENSE:               %f", pmd.v_pwr_sense );
  LevelII_stats_out( "  ZDR BIAS:                     %f", pmd.zdr_bias );
  for( i = 0; i < 6; i++ )
  {
    LevelII_stats_out( "  SPARE385-%d:                  %d", 3+i, pmd.spare385[i] );
  }
  LevelII_stats_out( "  HORIZ CLUTT SUPP DELTA:       %f", pmd.h_cltr_supp_delta );
  LevelII_stats_out( "  HORIZ CLUTT SUPP UNFILT PWR:  %f", pmd.h_cltr_supp_ufilt_pwr );
  LevelII_stats_out( "  HORIZ CLUTT SUPP FILT PWR:    %f", pmd.h_cltr_supp_filt_pwr );
  LevelII_stats_out( "  TRANSMIT BURST PWR:           %f", pmd.trsmit_brst_pwr );
  LevelII_stats_out( "  TRANSMIT BURST PHASE:         %f", pmd.trsmit_brst_phase );
  for( i = 0; i < 6; i++ )
  {
    LevelII_stats_out( "  SPARE419-%d:                  %d", i, pmd.spare419[i] );
  }
  LevelII_stats_out( "  VERT LINEARITY:               %f", pmd.v_linearity );
  for( i = 0; i < 4; i++ )
  {
    LevelII_stats_out( "  SPARE427-%d:                  %d", i, pmd.spare427[i] );
  }
  LevelII_stats_out( "FILE STATUS" );
  LevelII_stats_out( "  STATE FILE READ STATUS:            %d", pmd.state_file_rd_stat );
  LevelII_stats_out( "  STATE FILE WRITE STATUS:           %d", pmd.state_file_wrt_stat );
  LevelII_stats_out( "  BYP MAP READ STATUS:               %d", pmd.bypass_map_file_rd_stat );
  LevelII_stats_out( "  BYP MAP WRITE STATUS:              %d", pmd.bypass_map_file_wrt_stat );
  LevelII_stats_out( "  SPARE435:                          %d", pmd.spare435 );
  LevelII_stats_out( "  SPARE436:                          %d", pmd.spare436 );
  LevelII_stats_out( "  CURR ADAPT FILT READ STATUS:       %d", pmd.crnt_adpt_file_rd_stat );
  LevelII_stats_out( "  CURR ADAPT FILT WRITE STATUS:      %d", pmd.crnt_adpt_file_wrt_stat );
  LevelII_stats_out( "  CCZ FILE READ STATUS:              %d", pmd.cnsr_zn_file_rd_stat );
  LevelII_stats_out( "  CCZ FILE WRITE STATUS:             %d", pmd.cnsr_zn_file_wrt_stat );
  LevelII_stats_out( "  REMOTE VCP FILE READ STATUS:       %d", pmd.rmt_vcp_file_rd_stat );
  LevelII_stats_out( "  REMOTE VCP FILE WRITE STATUS:      %d", pmd.rmt_vcp_file_wrt_stat );
  LevelII_stats_out( "  BASELINE ADAPT FILE READ STATUS:   %d", pmd.bl_adpt_file_rd_stat );
  LevelII_stats_out( "  SPARE444:  %d", pmd.spare444 );
  LevelII_stats_out( "  CLUTT FILT MAP FILE READ STATUS:   %d", pmd.cf_map_file_rd_stat );
  LevelII_stats_out( "  CLUTT FILT MAP FILE WRITE STATUS:  %d", pmd.cf_map_file_wrt_stat );
  LevelII_stats_out( "  GENERAL DISK I/O ERROR:            %d", pmd.gnrl_disk_io_err );
  for( i = 0; i < 13; i++ )
  {
    LevelII_stats_out( "  SPARE448-%d:                       %d", i, pmd.spare448[i] );
  }
  LevelII_stats_out( "DEVICE STATUS" );
  LevelII_stats_out( "  DAU COMM STATUS:       %d", pmd.dau_comm_stat );
  LevelII_stats_out( "  HCI COMM STATUS:       %d", pmd.hci_comm_stat );
  LevelII_stats_out( "  PED COMM STATUS:       %d", pmd.pdstl_comm_stat );
  LevelII_stats_out( "  SIG PROC COMM STATUS:  %d", pmd.sgnl_prcsr_comm_stat );
  LevelII_stats_out( "  AME COMM STATUS:       %d", pmd.ame_comm_stat );
  LevelII_stats_out( "  RMS LINK STATUS:       %d", pmd.rms_lnk_stat );
  LevelII_stats_out( "  RPG LINK STATUS:       %d", pmd.rpg_lnk_stat );
  for( i = 0; i < 13; i++ )
  {
    LevelII_stats_out( "  SPARE468-%d:           %d", i, pmd.spare468[i] );
  }
}

/************************************************************************
 Description: Print RDA Status message.
 ************************************************************************/

static void Print_rda_status( char *buf )
{
  int i;

  ORDA_status_msg_t *rda_status_msg = (ORDA_status_msg_t *)buf;

  LevelII_stats_out( "RDA STATUS MSG (MSG TYPE 2)" );
  LevelII_stats_out( "  RDA STATUS:     %d", rda_status_msg->rda_status );
  LevelII_stats_out( "  OP STATUS:      %d", rda_status_msg->op_status );
  LevelII_stats_out( "  CONTROL STATUS: %d", rda_status_msg->control_status );
  LevelII_stats_out( "  AUX PWR STATE:  %d", rda_status_msg->aux_pwr_state );
  LevelII_stats_out( "  AVG TRANS PWR:  %d", rda_status_msg->ave_trans_pwr );
  LevelII_stats_out( "  HOR CALIB COR:  %d", rda_status_msg->ref_calib_corr );
  LevelII_stats_out( "  DATA ENABLED:   %d", rda_status_msg->data_trans_enbld );
  LevelII_stats_out( "  VCP:            %d", rda_status_msg->vcp_num );
  LevelII_stats_out( "  RDA CONT AUTH:  %d", rda_status_msg->rda_control_auth );
  LevelII_stats_out( "  RDA BUILD NUM:  %d", rda_status_msg->rda_build_num );
  LevelII_stats_out( "  OP MODE:        %d", rda_status_msg->op_mode );
  LevelII_stats_out( "  SUPER RES:      %d", rda_status_msg->super_res );
  LevelII_stats_out( "  CMD:            %d", rda_status_msg->cmd );
  LevelII_stats_out( "  AVSET:          %d", rda_status_msg->avset );
  LevelII_stats_out( "  RDA ALARM SUMM: %d", rda_status_msg->rda_alarm );
  LevelII_stats_out( "  CMD STATUS:     %d", rda_status_msg->command_status );
  LevelII_stats_out( "  CHANNEL STATUS: %d", rda_status_msg->channel_status );
  LevelII_stats_out( "  SPOTBLK STATUS: %d", rda_status_msg->spot_blanking_status );
  LevelII_stats_out( "  BYPASS DATE:    %d", rda_status_msg->bypass_map_date );
  LevelII_stats_out( "  BYPASS TIME:    %d", rda_status_msg->bypass_map_time );
  LevelII_stats_out( "  CLUTTER DATE:   %d", rda_status_msg->clutter_map_date );
  LevelII_stats_out( "  CLUTTER TIME:   %d", rda_status_msg->clutter_map_time );
  LevelII_stats_out( "  VERT CALIB COR: %d", rda_status_msg->vc_ref_calib_corr );
  LevelII_stats_out( "  TPS STATUS:     %d", rda_status_msg->tps_status );
  LevelII_stats_out( "  RMS CONTROL:    %d", rda_status_msg->rms_control_status );
  LevelII_stats_out( "  PERF CHK STAT   %d", rda_status_msg->perf_check_status );
  for( i = 0; i < MAX_RDA_ALARMS_PER_MESSAGE; i++ )
  {
    LevelII_stats_out( "  ALARM%02d:        %d", i+1, rda_status_msg->alarm_code[i] );
  }
}

/************************************************************************
 Description: Print Generic basedata header.
 ************************************************************************/

static void Print_bdh( Generic_basedata_header_t *bdh, char *icao_buf )
{
  LevelII_stats_out( "GENERIC BASEDATA HEADER" );
  LevelII_stats_out( "ICAO: %s TIME: %ld DATE: %d RAD LEN: %d",
             icao_buf, bdh->time, bdh->date, bdh->radial_length );
  LevelII_stats_out( "AZ#: %d AZ: %6.2f AZ IND: %d ELEV#: %d ELEV: %5.2f",
             bdh->azi_num, bdh->azimuth, bdh->azimuth_index,
             bdh->elev_num, bdh->elevation );
  LevelII_stats_out( "CMP: %d AZ RES: %d STATUS: %d SECTOR#: %d",
             bdh->compress_type, bdh->azimuth_res,
             bdh->status, bdh->sector_num );
  LevelII_stats_out( "SPOT: %d #DATUM: %d SPARE: %d",
             bdh->spot_blank_flag, bdh->no_of_datum, bdh->spare_17 );
}

/************************************************************************
 Description: Print RVOL data block.
 ************************************************************************/

static void Print_RVOL( Generic_vol_t *rvol )
{
  LevelII_stats_out( "RVOL" );
  LevelII_stats_out( "LEN:         %d", rvol->len );
  LevelII_stats_out( "MAJOR:       %d", rvol->major_version );
  LevelII_stats_out( "MINOR:       %d", rvol->minor_version );
  LevelII_stats_out( "LATITUDE:    %f", rvol->lat );
  LevelII_stats_out( "LONGITUDE:   %f", rvol->lon );
  LevelII_stats_out( "HEIGHT:      %d", rvol->height );
  LevelII_stats_out( "FEEDHORN:    %d", rvol->feedhorn_height );
  LevelII_stats_out( "CALIB CONST: %f", rvol->calib_const );
  LevelII_stats_out( "HORZ TX PWR: %f", rvol->horiz_shv_tx_power );
  LevelII_stats_out( "VERT TX PWR: %f", rvol->vert_shv_tx_power );
  LevelII_stats_out( "DIFF REFL:   %f", rvol->sys_diff_refl );
  LevelII_stats_out( "DIFF PHASE:  %f", rvol->sys_diff_phase );
  LevelII_stats_out( "VCP:         %d", rvol->vcp_num );
  LevelII_stats_out( "SIG PROC:    %d", rvol->sig_proc_states );
}

/************************************************************************
 Description: Print RELV data block.
 ************************************************************************/

static void Print_RELV( Generic_elev_t *relv )
{
  LevelII_stats_out( "LEN:         %d", relv->len );
  LevelII_stats_out( "ATMOS:       %d", relv->atmos );
  LevelII_stats_out( "CALIB CONST: %f", relv->calib_const );
}

/************************************************************************
 Description: Print RRAD data block.
 ************************************************************************/

static void Print_RRAD( Generic_rad_t *rrad )
{
  LevelII_stats_out( "LEN: %d\n",rrad->len);
  LevelII_stats_out( "UNAMB RANGE: %d", rrad->unamb_range );
  LevelII_stats_out( "HORZ NOISE:  %f", rrad->horiz_noise );
  LevelII_stats_out( "VERT NOISE:  %f", rrad->vert_noise );
  LevelII_stats_out( "NYQUIST VEL: %d", rrad->nyquist_vel );
  LevelII_stats_out( "SPARE:       %d", rrad->spare );
}

/************************************************************************
 Description: Print radial header.
 ************************************************************************/

static void Print_radial_header( char *moment, char *buf )
{
  Generic_moment_t *rmom = (Generic_moment_t *) buf;

  LevelII_stats_out( "MOMENT: %s", moment );
  LevelII_stats_out( "INFO:         %d", rmom->info );
  LevelII_stats_out( "# GATES:      %d", rmom->no_of_gates );
  LevelII_stats_out( "1ST GATE:     %d", rmom->first_gate_range );
  LevelII_stats_out( "BIN SIZE:     %d", rmom->bin_size );
  LevelII_stats_out( "TOVER:        %d", rmom->tover );
  LevelII_stats_out( "SNR THRESH:   %d", rmom->SNR_threshold );
  LevelII_stats_out( "CONTROL FLAG: %d", rmom->control_flag );
  LevelII_stats_out( "WORD SIZE   : %d", rmom->data_word_size );
  LevelII_stats_out( "SCALE:        %f", rmom->scale );
  LevelII_stats_out( "OFFSET:       %f", rmom->offset );
}

/**************************************************************************
 Description: Read 4-byte control word indicating size of record.
**************************************************************************/

int LevelII_stats_pq_read_control_word()
{
  int cw = 0;

  if( LevelII_stats_pq_read_stdin( (char *)&cw, LDM_CONTROL_WORD_SIZE ) != LDM_CONTROL_WORD_SIZE )
  {
    LevelII_stats_out( "read stdin failed: cw = %d and not %d", cw, LDM_CONTROL_WORD_SIZE );
    return LEVELII_STATS_PQ_READ_STDIN_FAILED;
  }

  return cw;
}

/**************************************************************************
 Description: Read stdin.
**************************************************************************/

int LevelII_stats_pq_read_stdin( char *buf, int bytes_to_read )
{
  char *p = buf;
  int total_bytes_read = 0;
  int bytes_read = 0;
  
  if( !Read_stdin_alarm_init_flag )
  {
    Read_stdin_alarm_init_flag = YES;
    if( Init_read_stdin_alarm() < 0 )
    {
      LevelII_stats_out( "Failed to initialize read stdin alarm" );
      return LEVELII_STATS_PQ_ALARM_INIT_FAILED;
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
      LevelII_stats_out( "read stdin failed: br < 0 (%d)", bytes_read );
      return LEVELII_STATS_PQ_READ_STDIN_FAILED;
      break;
    }
  }
  alarm( 0 );

  return total_bytes_read;
}

/************************************************************************
 Description: Print messages to stdout.
 ************************************************************************/

void LevelII_stats_out( const char *format, ... )
{
  char buf[MAX_PRINT_MSG_LEN];
  char timebuf[MAX_TIMESTRING_LEN];
  long timestamp = 0;
  va_list arg_ptr;

  /* If no format, nothing to do. */
  if( format == NULL ){ return; }

  /* Extract print format. */
  va_start( arg_ptr, format );
  vsprintf( buf, format, arg_ptr );
  va_end( arg_ptr );

  /* Create timestamp. */
  timestamp = time( NULL );
  if( ! strftime( timebuf, MAX_TIMESTRING_LEN, "%m/%d/%Y %H:%M:%S", gmtime( &timestamp ) ) )
  {
    sprintf( timebuf, "\?\?/\?\?/???? ??:??:??" );
  }

  /* Print message to stdout. */
  fprintf( stdout, "%s >> %s\n", timebuf, buf );
  fflush( stdout );
}

/************************************************************************
 Description: Print messages to stderr.
 ************************************************************************/

void LevelII_stats_err( const char *format, ... )
{
  char buf[MAX_PRINT_MSG_LEN];
  char timebuf[MAX_TIMESTRING_LEN];
  long timestamp = 0;
  va_list arg_ptr;

  /* If no format, nothing to do. */
  if( format == NULL ){ return; }

  /* Extract print format. */
  va_start( arg_ptr, format );
  vsprintf( buf, format, arg_ptr );
  va_end( arg_ptr );

  /* Create timestamp. */
  timestamp = time( NULL );
  if( ! strftime( timebuf, MAX_TIMESTRING_LEN, "%m/%d/%Y %H:%M:%S", gmtime( &timestamp ) ) )
  {
    sprintf( timebuf, "\?\?/\?\?/???? ??:??:??" );
  }

  /* Print message to stderr. */
  fprintf( stderr, "%s >> %s\n", timebuf, buf );
  fflush( stderr );
}

/************************************************************************
 Description: Print messages to stdout if debug flag is set.
 ************************************************************************/

void LevelII_stats_debug( const char *format, ... )
{
  char buf[MAX_PRINT_MSG_LEN];
  va_list arg_ptr;

  if( Debug_mode )
  {
    /* Extract print format. */
    va_start( arg_ptr, format );
    vsprintf( buf, format, arg_ptr );
    va_end( arg_ptr );
    LevelII_stats_out( buf );
  }
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
    LevelII_stats_debug( "sigaction for SIGALRM failed" );
    return -1;
  }

  return 0;
}

/**************************************************************************
 Description: Handler function for alarm while reading stdin.
**************************************************************************/

static void Read_stdin_alarm_handler()
{
  Read_stdin_alarm_timeout_flag = YES;
  LevelII_stats_debug( "ALARM handler for reading stdin called" );
}

