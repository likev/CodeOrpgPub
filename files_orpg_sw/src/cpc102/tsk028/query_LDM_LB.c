/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/10/30 12:30:21 $
 * $Id: query_LDM_LB.c,v 1.4 2012/10/30 12:30:21 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */  

/* Include files */

#include <query_product_print.h>

/* Defines/enums */

#define	LOOP_TIMER			1

/* Structures */

/* Static/global variables */

static char *Product_buf = NULL;
static int Monitor_mode_flag = NO;
static int Print_all_products_flag = NO;
static int Print_product_by_LB_index_flag = NO;
static int Print_format_flag = FORMAT_HDR_NONE;
static int Product_LB_index_number = -1;
static int LB_updated_flag = NO;

/* Function prototypes */

static void Parse_cmd_line( int, char *[] );
static void Print_usage();
static void Print_product();
static void Print_product_by_LB_index();
static void Print_all_products();
static void Process_realtime_msgs();
static void LB_updated_callback_fx( int, LB_id_t, int, void * );

/************************************************************************
 Description: Main function for task.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  int ret = 0;

  /* Parse command line. */
  Parse_cmd_line( argc, argv );

  if( Print_product_by_LB_index_flag )
  {
    Print_product_by_LB_index();
  }
  else if( Print_all_products_flag )
  {
    Print_all_products();
  }
  else
  {
    /* Monitor mode. */

    if( ( ret = ORPGDA_UN_register( ORPGDAT_LDM_WRITER_INPUT, LB_ANY, LB_updated_callback_fx ) ) != LB_SUCCESS )
    {
      Print_error( "MAIN: ORPGDA UN register failed (%d)", ret );
      exit( 1 );
    }

    while( 1 )
    {
      if( LB_updated_flag )
      {
        LB_updated_flag = NO;
        Process_realtime_msgs();
      }
      sleep( LOOP_TIMER );
    }
  }

  return 0;
}

/************************************************************************
 Description: Print product of given LB index.
 ************************************************************************/

static void Print_product_by_LB_index()
{
  int ret = 0;

  if( ( ret = ORPGDA_read( ORPGDAT_LDM_WRITER_INPUT, &Product_buf, LB_ALLOC_BUF, Product_LB_index_number ) ) < 0 )
  {
    Print_error( "PBI: READ failed: %d", ret );
    exit( 1 );
  }

  if( Product_buf == NULL )
  {
    Print_error( "PMBI: Buf is NULL" );
    exit( 1 );
  }

  Print_product();
  free( Product_buf );
}

/************************************************************************
 Description: Print all products of the LB.
 ************************************************************************/

static void Print_all_products()
{
  int ret = 0;
  int loop_flag = YES;
  LB_info *lbinfo = NULL;

  if( ( ret = ORPGDA_seek( ORPGDAT_LDM_WRITER_INPUT, 0, LB_FIRST, lbinfo ) ) != LB_SUCCESS  )
  {
    Print_error( "PAM: ORPGDA_seek failed (%d)", ret );
    exit( 1 );
  }
  if( lbinfo == NULL )
  {
    Print_error( "PAM: LB_info is NULL" );
  }
  else
  {
    Print_debug( "LB info: ID: %u SIZE: %d MARK: %d", lbinfo->id, lbinfo->size, lbinfo->mark );
  }

  while( loop_flag )
  {
    ret = ORPGDA_read( ORPGDAT_LDM_WRITER_INPUT, &Product_buf, LB_ALLOC_BUF, LB_NEXT );
    if( ret > 0 )
    {
      if( Product_buf == NULL )
      {
        Print_error( "PAM: Buf is NULL" );
      }
      else
      {
        Print_product();
        free( Product_buf );
      }
    }
    else if( ret == LB_TO_COME )
    {
      Print_error( "PAM: ORPGDA_read is LB_TO_COME" );
      loop_flag = NO;
    }
    else if( ret == LB_EXPIRED )
    {
      Print_error( "PAM: ORPGDA_read is LB_EXPIRED" );
      loop_flag = NO;
    }
    else
    {
      Print_error( "PAM: ORPGDA_read failed (%d)", ret );
      loop_flag = NO;
    }
  }
}

/************************************************************************
 Description: Process the next batch of realtime messages.
 ************************************************************************/

static void Process_realtime_msgs()
{
  int ret = 0;
  int loop_flag = YES;

  while( loop_flag )
  {
    ret = ORPGDA_read( ORPGDAT_LDM_WRITER_INPUT, &Product_buf, LB_ALLOC_BUF, LB_NEXT );
    if( ret > 0 )
    {
      if( Product_buf == NULL )
      {
        Print_error( "PRT: Buf is NULL" );
      }
      else
      {
        Print_product();
        free( Product_buf );
      }
    }
    else if( ret == LB_TO_COME )
    {
      Print_debug( "PRT: ORPGDA_read is LB_TO_COME" );
      loop_flag = NO;
    }
    else if( ret == LB_EXPIRED )
    {
      Print_error( "PRT: ORPGDA_read is LB_EXPIRED" );
      loop_flag = NO;
    }
    else
    {
      Print_error( "PRT: ORPGDA_read failed (%d)", ret );
      loop_flag = NO;
    }
  }
}

/************************************************************************
 Description: Print usage of this task.
 ************************************************************************/

static void Print_usage( char *task_name )
{
  Print_out( "Usage: %s [options]", task_name );
  Print_out( "  Options:" );
  Print_out( "  -h     - print usage info" );
  Print_out( "  -x     - debug mode" );
  Print_out( "  -l int - print LB message with LB index of int" );
  Print_out( "  -L     - print all LB messages" );
  Print_out( "  -m     - monitor mode (Default)" );
  Print_out( "" );
  Print_out( "  To print specific product header info use options:" );
  Print_out( "  -T - Timestamp" );
  Print_out( "  -K - LDM key (Default)" );
  Print_out( "  -G - Flags" );
  Print_out( "  -F - LDM feed type" );
  Print_out( "  -S - Sequence number" );
  Print_out( "  -D - Data length" );
  Print_out( "  -A - All product header options" );
  Print_out( "" );
  Print_out( "  To print specific message header info use options:" );
  Print_out( "  -c - Code" );
  Print_out( "  -z - Size" );
  Print_out( "  -t - Timestamp" );
  Print_out( "  -p - Server Mask" );
  Print_out( "  -d - Build" );
  Print_out( "  -i - ICAO" );
  Print_out( "  -g - Flags" );
  Print_out( "  -u - Size (uncompressed)" );
  Print_out( "  -s - Segment number" );
  Print_out( "  -n - Number of segments" );
  Print_out( "  -a - All message header options" );
  Print_out( "" );
  Print_out( "NOTE:" );
  Print_out( "  Options -l, -L, and -m are mutually exclusive" );
  Print_out( "" );
}

/************************************************************************
 Description: Parse command line and take appropriate action.
 ************************************************************************/

static void Parse_cmd_line( int argc, char *argv[] )
{
  int c = 0;
  int cnt = 0;
  char temp_buf[16];

  while( ( c = getopt( argc, argv, "hxl:LmTKGFSDAcztpdigusna" ) ) != EOF )
  {
    switch( c )
    {
      case 'h':
        Print_usage( argv[0] );
        exit( RPG_LDM_SUCCESS );
        break;
      case 'x':
        Set_debug_mode( YES );
        break;
      case 'l':
        if( optarg != NULL )
        {
          strncpy( temp_buf, optarg, 16 );
          if( strlen( optarg ) >= 16 )
          {
            temp_buf[strlen( optarg )-1] = '\0';
          }
          if( ( Product_LB_index_number = atoi( temp_buf ) ) < 1 )
          {
            Print_error( "Bad index: %s", temp_buf );
            exit( 1 );
          }
        }
        Print_product_by_LB_index_flag = YES;
        break;
      case 'L':
        Print_all_products_flag = YES;
        break;
      case 'm':
        Monitor_mode_flag = YES;
        break;
      case 'T':
        Print_format_flag |= FORMAT_PROD_HDR_TIME;
        break;
      case 'K':
        Print_format_flag |= FORMAT_PROD_HDR_KEY;
        break;
      case 'G':
        Print_format_flag |= FORMAT_PROD_HDR_FLAGS;
        break;
      case 'F':
        Print_format_flag |= FORMAT_PROD_HDR_FEEDTYPE;
        break;
      case 'S':
        Print_format_flag |= FORMAT_PROD_HDR_SEQNUM;
        break;
      case 'D':
        Print_format_flag |= FORMAT_PROD_HDR_LENGTH;
        break;
      case 'A':
        Print_format_flag |= FORMAT_PROD_HDR_ALL;
        break;
      case 'c':
        Print_format_flag |= FORMAT_MSG_HDR_CODE;
        break;
      case 'z':
        Print_format_flag |= FORMAT_MSG_HDR_SIZE;
        break;
      case 't':
        Print_format_flag |= FORMAT_MSG_HDR_TIME;
        break;
      case 'p':
        Print_format_flag |= FORMAT_MSG_HDR_SERVERMASK;
        break;
      case 'd':
        Print_format_flag |= FORMAT_MSG_HDR_BUILD;
        break;
      case 'i':
        Print_format_flag |= FORMAT_MSG_HDR_ICAO;
        break;
      case 'g':
        Print_format_flag |= FORMAT_MSG_HDR_FLAGS;
        break;
      case 'u':
        Print_format_flag |= FORMAT_MSG_HDR_SIZEU;
        break;
      case 's':
        Print_format_flag |= FORMAT_MSG_HDR_SEGNUM;
        break;
      case 'n':
        Print_format_flag |= FORMAT_MSG_HDR_NUMSEGS;
        break;
      case 'a':
        Print_format_flag |= FORMAT_MSG_HDR_ALL;
        break;
      case '?':
        Print_error( "Ignored option %c", (char) c );
        break;
      default:
        Print_error( "Illegal option %c. Terminate.", (char) c );
        Print_usage( argv[0] );
        exit( RPG_LDM_INVALID_COMMAND_LINE );
        break;
    }
  }

  Print_debug( "PRINT MSG BY INDX: %d", Print_product_by_LB_index_flag );
  Print_debug( "PRINT INDEX NUM:   %d", Product_LB_index_number );
  Print_debug( "PRINT ALL MSGS:    %d", Print_all_products_flag );
  Print_debug( "MONITOR MODE:      %d", Monitor_mode_flag );

  if( Print_format_flag == FORMAT_HDR_NONE )
  {
    /* Print product header key by default */
    Print_format_flag |= FORMAT_PROD_HDR_KEY;
  }

  Print_debug( "PRINT FORMAT FLAGS FOR PRODUCT HEADER" );
  if( Print_format_flag & FORMAT_PROD_HDR_TIME ){ Print_debug( "TIME" ); }
  if( Print_format_flag & FORMAT_PROD_HDR_KEY ){ Print_debug( "KEY" ); }
  if( Print_format_flag & FORMAT_PROD_HDR_FLAGS ){ Print_debug( "FLAGS" ); }
  if( Print_format_flag & FORMAT_PROD_HDR_FEEDTYPE ){ Print_debug( "FEEDTYPE" ); }
  if( Print_format_flag & FORMAT_PROD_HDR_SEQNUM ){ Print_debug( "SEQNUM" ); }
  if( Print_format_flag & FORMAT_PROD_HDR_LENGTH ){ Print_debug( "LENGTH" ); }
  if( Print_format_flag & FORMAT_PROD_HDR_SPARE148 ){ Print_debug( "SPARE148" ); }
  if( Print_format_flag & FORMAT_PROD_HDR_SPARE152 ){ Print_debug( "SPARE152" ); }

  Print_debug( "PRINT FORMAT FLAGS FOR MESSAGE HEADER" );
  if( Print_format_flag & FORMAT_MSG_HDR_CODE ){ Print_debug( "CODE" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_SIZE ){ Print_debug( "SIZE" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_TIME ){ Print_debug( "TIME" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_SERVERMASK ){ Print_debug( "SERVMASK" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_BUILD ){ Print_debug( "BUILD" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_ICAO ){ Print_debug( "ICAO" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_FLAGS ){ Print_debug( "FLAGS" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_SIZEU ){ Print_debug( "SIZE (U)" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_SEGNUM ){ Print_debug( "SEG NUM" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_NUMSEGS ){ Print_debug( "NUM SEGS" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_WMO_SIZE ){ Print_debug( "WMO SIZE" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_SPARE38 ){ Print_debug( "SPARE38" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_SPARE40 ){ Print_debug( "SPARE40" ); }
  if( Print_format_flag & FORMAT_MSG_HDR_SPARE44 ){ Print_debug( "SPARE44" ); }

  /* Validate flags */

  cnt = 0;
  if( Print_product_by_LB_index_flag ){ cnt++; }
  if( Print_all_products_flag ){ cnt++; }
  if( Monitor_mode_flag ){ cnt++; }
  if( cnt > 1 )
  {
    Print_error( "Options -l, -L, and -m are mutually exclusive" );
    exit( 1 );
  }
  else if( cnt == 0 )
  {
    Monitor_mode_flag = YES;
  }
}

/************************************************************************
 Description: Callback function for LDM LB.
 ************************************************************************/

static void LB_updated_callback_fx( int fd, LB_id_t mid, int minfo, void *arg )
{
  LB_updated_flag = YES;
}

/************************************************************************
 Description: Print according to format flag.
 ************************************************************************/

static void Print_product()
{
  RPG_LDM_prod_hdr_t *hdr = (RPG_LDM_prod_hdr_t *) &Product_buf[0];

  Print_RPG_LDM_prod_hdr( &Product_buf[0], Print_format_flag );
  if( hdr->feed_type != RPG_LDM_NEXRAD_FEED_TYPE )
  {
    Print_RPG_LDM_msg_hdr( &Product_buf[sizeof(RPG_LDM_prod_hdr_t)], Print_format_flag );
  }
}

