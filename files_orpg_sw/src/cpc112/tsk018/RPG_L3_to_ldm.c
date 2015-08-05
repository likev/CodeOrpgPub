/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/10/30 12:29:35 $
 * $Id: RPG_L3_to_ldm.c,v 1.2 2012/10/30 12:29:35 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
*/

/* Include files. */

#include <rpg_ldm.h>
#include <prod_gen_msg.h>

/* Defines/enums. */

#define LOOP_TIMER			5
#define	SUPPLEMENTAL_KEY_BUF_SIZE	32
#define MAX_FILTER_SIZE			300
#define FILTER_LIST_INTERVAL		5

/* Structures */

/* Static/global variables. */

static int   Read_data_flag = NO;
static int   Lb_id = ORPGDAT_PRODUCTS;
static int   Start_msg_id = 0;
static int   End_msg_id = 0;
static char *Data_buffer = NULL;
static int   Data_buffer_size = 0;
static char *Previous_data_buffer = NULL;
static int   Previous_data_buffer_size = 0;
static int   Product_code = 0;
static int   Product_id = 0;
static int   Volume_number = 0;
static int   Elevation_index = 0;
static int   RPG_prod_header_size = sizeof( Prod_header );
static int   Send_filter_flag = NO;
static int   Ignore_filter_flag = NO;
static int   Number_of_codes_to_send = 0;
static int  *Codes_to_send = NULL;
static int   Number_of_codes_to_ignore = 0;
static int  *Codes_to_ignore = NULL;
static int   Number_of_ids_to_send = 0;
static int  *Ids_to_send = NULL;
static int   Number_of_ids_to_ignore = 0;
static int  *Ids_to_ignore = NULL;
static int   Send_intermediate_products_flag = NO;
static int   Prepend_WMO_header = NO;

/* Function prototypes. */

static void Parse_cmd_line( int, char *[] );
static void Print_usage();
static void Data_callback_fx( int, LB_id_t );
static void Read_data();
static void Write_to_LB();
static int  Is_new_buffer_different();
static void Print_RPG_product_header( char * );
static void Customize_LDM_key( RPG_LDM_prod_hdr_t * );
static void Validate_filters( char *, int **, int * );
static void Add_filter_to_list( int, int **, int * );
static int  Apply_send_filters();
static int  Apply_ignore_filters();

/************************************************************************
 Description: Main function.
 ************************************************************************/

int main( int argc, char **argv )
{
  int ret = 0;
  int start_id = 0;
  int end_id = 0;
  int i = 0;

  /* Parse command line */
  Parse_cmd_line( argc, argv );

  /* Register UN events to be notified when data is updated */
  ORPGDA_write_permission( Lb_id );

  if( ( ret = ORPGDA_UN_register( Lb_id, LB_ANY, Data_callback_fx ) ) < 0 )
  {
    LE_send_msg( GL_INFO, "ORPGDA_UN_register failed (%d). Terminate.", ret );
    exit( RPG_LDM_UN_REGISTER_FAILED );
  }
  RPG_LDM_print_debug( "ORPGDA_UN_register successful" );

  /* Report ready-for-operation and wait until RPG is in operational mode. */
  ORPGMGR_report_ready_for_operation();
  LE_send_msg( GL_INFO, "Starting operation" );

  /* Process events */
  while( 1 )
  {
    /* Read new data */
    if( Read_data_flag == YES )
    {
      start_id = Start_msg_id;
      end_id = End_msg_id;
      Read_data_flag = NO;
      RPG_LDM_print_debug( "Read Start ID: %d End ID: %d", start_id, end_id );
      for( i = start_id; i <= end_id; i++ )
      {
        Read_data( i );
        Write_to_LB();
      }
    }

    sleep( LOOP_TIMER );
  }

  return 0;
}

/************************************************************************
 Description: Print usage of this task.
 ************************************************************************/

static void Print_usage( char *task_name )
{
  printf( "Usage: %s [options]\n", task_name );
  printf( "  Options:\n" );
  printf( "  -o args - list of RPG product codes to send (Default: ALL)\n" );
  printf( "  -O args - list of RPG product codes to ignore (Default: NONE)\n" );
  printf( "  -d args - list of RPG product ids to send (Default: ALL)\n" );
  printf( "  -D args - list of RPG product ids to ignore (Default: NONE)\n" );
  printf( "  -i - send intermediate products (Default: NO)\n" );
  printf( "  -p - prepend WMO header to level-III product (Default: NO)\n" );
  printf( "  -T task name - only used when invoked by mrpg (optional)\n" );
  printf( "  -h   - print usage info\n" );
  printf( "\n" );
  printf( "NOTE: lists for -oOdD options must be comma separated with\n" );
  printf( "      no white spaces\n" );
  printf( "\n" );
  printf( "NOTE: a product code of 0 (zero) is ignored as that is\n" );
  printf( "      reserved for intermediate products\n" );
  printf( "\n" );
  printf( "NOTE: Send and ignore filters can't be combined. In other\n" );
  printf( "      words, either send by code/id or ignore by code/id.\n" );
  printf( "      Using code and id combined is allowed. Code is always\n" );
  printf( "      processed first.\n" );
  printf( "\n" );
  RPG_LDM_print_usage();
  printf( "\n" );
}

/************************************************************************
 Description: Parse command line and take appropriate action.
 ************************************************************************/

static void Parse_cmd_line( int argc, char *argv[] )
{
  int c = 0;

  RPG_LDM_parse_cmd_line( argc, argv );

  opterr = 0;
  optind = 0;
  while( ( c = getopt( argc, argv, "o:O:d:D:ipT:h" ) ) != EOF )
  {
    switch( c )
    {
      case 'o':
        Validate_filters( optarg, &Codes_to_send, &Number_of_codes_to_send );
        Send_filter_flag = YES;
        break;
      case 'O':
        Validate_filters( optarg, &Codes_to_ignore, &Number_of_codes_to_ignore );
        Ignore_filter_flag = YES;
        break;
      case 'd':
        Validate_filters( optarg, &Ids_to_send, &Number_of_ids_to_send );
        Send_filter_flag = YES;
        break;
      case 'D':
        Validate_filters( optarg, &Ids_to_ignore, &Number_of_ids_to_ignore );
        Ignore_filter_flag = YES;
        break;
      case 'i':
        Send_intermediate_products_flag = YES;
        RPG_LDM_print_debug( "Send intermediate products" );
        break;
      case 'p':
        Prepend_WMO_header = YES;
        RPG_LDM_print_debug( "Prepend WMO header" );
        break;
      case 'h':
        Print_usage( argv[0] );
        exit( RPG_LDM_SUCCESS );
        break;
      case 'T':
        /* Ignore -T option */
        break;
      case '?':
        /* Ignore ? */
        LE_send_msg( GL_INFO, "Ignored option %c", (char) optopt );
        break;
      default:
        LE_send_msg( GL_ERROR, "Illegal option %c. Terminate.", (char) c );
        Print_usage( argv[0] );
        exit( RPG_LDM_INVALID_COMMAND_LINE );
        break;
    }
  }

  RPG_LDM_print_debug( "Num codes to send: %d", Number_of_codes_to_send );
  if( Number_of_codes_to_send > 0 )
  {
    for( c = 0; c < Number_of_codes_to_send; c++ )
    {
      RPG_LDM_print_debug( "  %d: %d", c, Codes_to_send[c] );
    }
  }

  RPG_LDM_print_debug( "Num codes to ignore: %d", Number_of_codes_to_ignore );
  if( Number_of_codes_to_ignore > 0 )
  {
    for( c = 0; c < Number_of_codes_to_ignore; c++ )
    {
      RPG_LDM_print_debug( "  %d: %d", c, Codes_to_ignore[c] );
    }
  }

  RPG_LDM_print_debug( "Num ids to send: %d", Number_of_ids_to_send );
  if( Number_of_ids_to_send > 0 )
  {
    for( c = 0; c < Number_of_ids_to_send; c++ )
    {
      RPG_LDM_print_debug( "  %d: %d", c, Ids_to_send[c] );
    }
  }

  RPG_LDM_print_debug( "Num ids to ignore: %d", Number_of_ids_to_ignore );
  if( Number_of_ids_to_ignore > 0 )
  {
    for( c = 0; c < Number_of_ids_to_ignore; c++ )
    {
      RPG_LDM_print_debug( "  %d: %d", c, Ids_to_ignore[c] );
    }
  }

  if( Send_filter_flag && Ignore_filter_flag )
  {
    LE_send_msg( GL_ERROR, "Can't filter by both send and ignore" );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
}

/************************************************************************
 Description: Callback if data received.
 ************************************************************************/

static void Data_callback_fx( int data_id, LB_id_t msg_id )
{
  RPG_LDM_print_debug( "Data callback function called" );
  if( ! Read_data_flag )
  {
    Read_data_flag = YES;
    Start_msg_id = msg_id;
  }
  End_msg_id = msg_id;
}

/************************************************************************
 Description: Read data and write to linear buffer.
 ************************************************************************/

static void Read_data( int msg_id )
{
  int ret = 0;

  RPG_LDM_print_debug( "Enter Read_data()" );

  if( Data_buffer != NULL )
  {
    free( Data_buffer );
    Data_buffer = NULL;
  }
  Data_buffer_size = 0;

  /* Read data from LB */
  ret = ORPGDA_read( Lb_id, &Data_buffer, LB_ALLOC_BUF, msg_id );
  if( ret == LB_TO_COME )
  {
    LE_send_msg( GL_INFO, "ORPGDA_read returned LB_TO_COME" );
  }
  else if( ret < 0 )
  {
    LE_send_msg( GL_INFO, "ORPGDA_read %d failed (%d)\n", msg_id, ret );
  }
  else
  {
    RPG_LDM_print_debug( "ORPGDA_read read %d bytes", ret );
    Data_buffer_size = ret;
  }

  RPG_LDM_print_debug( "Leave Read_data()" );
}
/************************************************************************
 Description: Write product to LB.
 ************************************************************************/

static void Write_to_LB()
{
  RPG_LDM_prod_hdr_t RPG_LDM_prod_hdr;
  RPG_LDM_msg_hdr_t RPG_LDM_msg_hdr;
  Graphic_product *gp = (Graphic_product *) &Data_buffer[RPG_prod_header_size];
  char *product_buffer = NULL;
  int   product_buffer_size = 0;
  int   prod_hdr_length = sizeof( RPG_LDM_prod_hdr_t );
  int   msg_hdr_length = sizeof( RPG_LDM_msg_hdr_t );
  int   data_length = Data_buffer_size;
  int   msg_length = msg_hdr_length + data_length;
  int   product_length = prod_hdr_length + msg_length;
  char *wmo_hdr = NULL;
  int   wmo_hdr_length = 0;
  int   ret = 0;
  int   offset = 0;

  RPG_LDM_print_debug( "Enter Write_to_LB()" );

  /* Sanity check */
  if( Data_buffer_size == 0 )
  {
    LE_send_msg( GL_INFO, "Data buffer size is zero. Skip writing to LB." );
    return;
  }

  /* Sanity check */
 if( Is_new_buffer_different() == NO )
  {
    LE_send_msg( GL_INFO, "New buffer isn't different. Skip writing to LB." );
    return;
  }

  /* Print product header (if debug mode) */
  if( RPG_LDM_get_debug_mode() )
  {
    Print_RPG_product_header( &Data_buffer[0] );
  }

  /* Set variables from product */
  Product_code = SHORT_BSWAP( gp->prod_code );
  Product_id = ORPGPAT_get_prod_id_from_code( Product_code );
  Volume_number = SHORT_BSWAP( gp->vol_num );
  Elevation_index = SHORT_BSWAP( gp->elev_ind );
  RPG_LDM_print_debug( "Product code:    %d", Product_code );
  RPG_LDM_print_debug( "Product id:      %d", Product_id );
  RPG_LDM_print_debug( "Volume:          %d", Volume_number );
  RPG_LDM_print_debug( "Elevation Index: %d", Elevation_index );

  /* Apply intermediate product filter */
  if( Product_code == 0 && !Send_intermediate_products_flag )
  {
    RPG_LDM_print_debug( "Ignore intermediate product id %d", Product_id );
    return;
  }

  /* Apply send/ignore filters, if applicable */
  if( Send_filter_flag && !Apply_send_filters() )
  {
    RPG_LDM_print_debug( "Product id %d not in send filter", Product_id );
    return;
  }
  else if( Ignore_filter_flag && !Apply_ignore_filters() )
  {
    RPG_LDM_print_debug( "Product id %d in ignore filter", Product_id );
    return;
  }

  /* Get WMO header, if applicable */
  if( Prepend_WMO_header )
  {
    if( ( wmo_hdr = RPG_LDM_get_WMO_header( gp ) ) != NULL )
    {
      wmo_hdr_length = strlen( wmo_hdr );
    }
    else
    {
      wmo_hdr_length = 0;
    }
  }
  else
  {
    wmo_hdr = NULL;
    wmo_hdr_length = 0;
  }

  /* Initialize RPG LDM product header. */
  RPG_LDM_prod_hdr_init( &RPG_LDM_prod_hdr );
  RPG_LDM_prod_hdr.timestamp = time( NULL );
  RPG_LDM_prod_hdr.flags = RPG_LDM_PROD_FLAGS_PRINT_CW;
  RPG_LDM_prod_hdr.feed_type = RPG_LDM_EXP_FEED_TYPE;
  RPG_LDM_prod_hdr.seq_num = 1;
  RPG_LDM_prod_hdr.data_len = msg_length;
  RPG_LDM_set_LDM_key( &RPG_LDM_prod_hdr, RPG_LDM_L3_PROD );
  /* For this product, customize the LDM key from the default */
  Customize_LDM_key( &RPG_LDM_prod_hdr );

  /* Initialize RPG LDM message header. */
  RPG_LDM_msg_hdr_init( &RPG_LDM_msg_hdr );
  RPG_LDM_msg_hdr.code = RPG_LDM_L3_PROD;
  RPG_LDM_msg_hdr.size = data_length;
  RPG_LDM_msg_hdr.timestamp = RPG_LDM_prod_hdr.timestamp;
  RPG_LDM_msg_hdr.segment_number = 1;
  RPG_LDM_msg_hdr.number_of_segments = 1;
  RPG_LDM_msg_hdr.wmo_header_size = wmo_hdr_length;

  /* Allocate memory for product. */
  product_buffer_size = product_length + wmo_hdr_length;
  RPG_LDM_print_debug( "Product buffer size: %d", product_buffer_size );
  if( ( product_buffer = malloc( product_buffer_size ) ) == NULL )
  {
    /* Memory allocation error */
    LE_send_msg( GL_ERROR, "Could not malloc product_buffer. Terminate." );
    exit( RPG_LDM_MALLOC_FAILED );
  }

  /* Copy header/data into output product buffer */
  offset = 0;
  RPG_LDM_print_debug( "Copy product header into product buffer" );
  memcpy( &product_buffer[offset], &RPG_LDM_prod_hdr, prod_hdr_length );
  offset += prod_hdr_length;
  RPG_LDM_print_debug( "Copy message header into product buffer" );
  memcpy( &product_buffer[offset], &RPG_LDM_msg_hdr, msg_hdr_length );
  offset += msg_hdr_length;
  /* Skip over RPG product header and copy WMO header, if applicable */
  if( wmo_hdr != NULL && wmo_hdr_length > 0 )
  {
    RPG_LDM_print_debug( "Copy WMO header into product buffer" );
    memcpy( &product_buffer[offset], &wmo_hdr[0], wmo_hdr_length );
    offset += wmo_hdr_length;
  }
  /* Skip over WMO header and copy data */
  RPG_LDM_print_debug( "Copy message into product buffer" );
  memcpy( &product_buffer[offset], &Data_buffer[RPG_prod_header_size], data_length - RPG_prod_header_size );

  /* Write to outgoing linear buffer */
  if( ( ret = RPG_LDM_write_to_LB( &product_buffer[0] ) ) < 0 )
  {
    LE_send_msg( GL_ERROR, "RPG_LDM_write_to_LB failed (%d)", ret );
  }
  else
  {
    LE_send_msg( GL_INFO, "%d bytes written to LB", ret );
  }

  /* Print contents of message (if debug mode) */
  if( RPG_LDM_get_debug_mode() )
  {
    RPG_LDM_print_L3_msg( &product_buffer[prod_hdr_length + msg_hdr_length], wmo_hdr_length );
  }

  if( product_buffer != NULL )
  {
    free( product_buffer );
    product_buffer = NULL;
  }
  if( Data_buffer != NULL )
  {
    free( Data_buffer );
    Data_buffer = NULL;
  }
  Data_buffer_size = 0;

  RPG_LDM_print_debug( "Leave Write_to_LB()" );
}

/************************************************************************
 Description: Compare new buffer to previous buffer.
 ************************************************************************/

static int Is_new_buffer_different()
{
  static int need_to_init_flag = YES;

  if( need_to_init_flag )
  {
    /* Initialize if first time through */
    need_to_init_flag = NO;
    RPG_LDM_print_debug( "Initialize previous buffer" );
    Previous_data_buffer_size = Data_buffer_size;
    Previous_data_buffer = malloc( Previous_data_buffer_size );
    memcpy( &Previous_data_buffer[0], &Data_buffer[0], Data_buffer_size );
    RPG_LDM_print_debug( "Previous buffere size: %d", Data_buffer_size );
    RPG_LDM_print_debug( "Return previous buffer is different" );
    return YES;
  }

  if( Data_buffer_size == Previous_data_buffer_size &&
      memcmp( &Data_buffer[0], &Previous_data_buffer[0], Data_buffer_size ) == 0 )
  {
    /* New and previous buffers are same */
    RPG_LDM_print_debug( "Return previous buffer is same" );
    return NO;
  }
  else
  {
    /* New and previous buffers differ */
    if( Data_buffer_size != Previous_data_buffer_size )
    {
      RPG_LDM_print_debug( "Sizes differ New: %d Previous: %d", Data_buffer_size, Previous_data_buffer_size );
    }
    else
    {
      RPG_LDM_print_debug( "New and previous buffers differ" );
    }

    /* Free previously allocated memory */
    if( Previous_data_buffer != NULL )
    {
      free( Previous_data_buffer );
      Previous_data_buffer = NULL;
    }

    /* Set previous variables to new values */
    Previous_data_buffer_size = Data_buffer_size;
    Previous_data_buffer = malloc( Previous_data_buffer_size );
    memcpy( &Previous_data_buffer[0], &Data_buffer[0], Data_buffer_size );
  }

  RPG_LDM_print_debug( "Return previous buffer is different" );
  return YES;
}

/************************************************************************
 Description: Apply send filters to see if product should be sent or not.
 ************************************************************************/

static int Apply_send_filters()
{
  int i = 0;

  /* Return YES if product is to be sent, NO if not */

  if( Number_of_codes_to_send > 0 )
  {
    for( i = 0; i < Number_of_codes_to_send; i++ )
    {
      if( Product_code == Codes_to_send[i] ){ return YES; }
    }
  }

  if( Number_of_ids_to_send > 0 )
  {
    for( i = 0; i < Number_of_ids_to_send; i++ )
    {
      if( Product_id == Ids_to_send[i] ){ return YES; }
    }
  }

  return NO;
}

/************************************************************************
 Description: Apply ignore filters to see if product should be sent or not.
 ************************************************************************/

static int Apply_ignore_filters()
{
  int i = 0;

  /* Return YES if product is to be sent, NO if not */

  if( Number_of_codes_to_ignore > 0 )
  {
    for( i = 0; i < Number_of_codes_to_ignore; i++ )
    {
      if( Product_code == Codes_to_ignore[i] ){ return NO; }
    }
  }

  if( Number_of_ids_to_ignore > 0 )
  {
    for( i = 0; i < Number_of_ids_to_ignore; i++ )
    {
      if( Product_id == Ids_to_ignore[i] ){ return NO; }
    }
  }

  return YES;
}

/************************************************************************
 Description: Print contents of RPG product header.
 ************************************************************************/

static void Print_RPG_product_header( char *buf )
{
  Prod_header *p = (Prod_header *) buf;
  int i = 0;

  RPG_LDM_print_debug( "Enter Print_RPG_product_header()" );

  LE_send_msg( GL_INFO, "PRODUCT ID:   %d", p->g.prod_id );
  LE_send_msg( GL_INFO, "INPUT STREAM: %d", p->g.input_stream );
  LE_send_msg( GL_INFO, "LB ID:        %d", p->g.id );
  LE_send_msg( GL_INFO, "GEN TIME:     %ld", p->g.gen_t );
  LE_send_msg( GL_INFO, "VOL TIME:     %d", p->g.vol_t );
  LE_send_msg( GL_INFO, "LEN:          %d", p->g.len );
  LE_send_msg( GL_INFO, "REQ NUM:      %d", p->g.req_num );
  LE_send_msg( GL_INFO, "ELEV INDEX:   %d", p->g.elev_ind );
  LE_send_msg( GL_INFO, "VOLUME NUM:   %d", p->g.vol_num );
  LE_send_msg( GL_INFO, "REQ: ");
  for( i = 0; i < PGM_NUM_PARAMS; i++ )
  {
    LE_send_msg( GL_INFO, "  P%d: %d ", i+1, p->g.req_params[i] );
  }
  LE_send_msg( GL_INFO, "RESP: ");
  for( i = 0; i < PGM_NUM_PARAMS; i++ )
  {
    LE_send_msg( GL_INFO, "  P%d: %d ", i+1, p->g.resp_params[i] );
  }
  LE_send_msg( GL_INFO, "ELEV TIME:    %d ", p->elev_t);
  LE_send_msg( GL_INFO, "ELEV COUNT:   %d ", p->elev_cnt);
  LE_send_msg( GL_INFO, "A3 FLAG:      %d ", p->archIII_flg);
  LE_send_msg( GL_INFO, "BD STAT:      %d ", p->bd_status);
  LE_send_msg( GL_INFO, "SPARE:        %d  ", p->spare);
  LE_send_msg( GL_INFO, "SB BITMAP:    %d\n", p->spot_blank_bitmap);
  LE_send_msg( GL_INFO, "WX MODE:      %d ", p->wx_mode);
  LE_send_msg( GL_INFO, "VCP:          %d ", p->vcp_num);
  LE_send_msg( GL_INFO, "COMPRESSION:  %d ", p->compr_method);
  LE_send_msg( GL_INFO, "ORIG SIZE:    %d\n", p->orig_size);

  RPG_LDM_print_debug( "Leave Print_RPG_product_header()" );
}

/**************************************************************************
 Description: Customize the LDM key.
**************************************************************************/

void Customize_LDM_key( RPG_LDM_prod_hdr_t *phdr )
{
  int key_len = strlen( phdr->key );
  char *key_ptr = &phdr->key[strlen(phdr->key)];
  char buf[SUPPLEMENTAL_KEY_BUF_SIZE];
  int buf_index = 0;
  char *p_mnemonic = NULL;

  RPG_LDM_print_debug( "Enter Customize_LDM_key()" );

  /* Append slash and RPG volume number */
  buf[buf_index] = '/';
  buf_index++;
  sprintf( &buf[buf_index], "VOL%03d", Volume_number );
  buf_index+= 6;

  /* Append slash and RPG elevation index */
  buf[buf_index] = '/';
  buf_index++;
  sprintf( &buf[buf_index], "%02d", Elevation_index );
  buf_index+= 2;

  /* Append slash and RPG product mnemonic and RPG product code */
  buf[buf_index] = '/';
  buf_index++;
  sprintf( &buf[buf_index], "%03d-", Product_code );
  buf_index+= 4;
  if( ( p_mnemonic = ORPGPAT_get_mnemonic( Product_id ) ) != NULL )
  {
    sprintf( &buf[buf_index], "%s", p_mnemonic );
    buf_index+= strlen( p_mnemonic );
  }
  else
  {
    sprintf( &buf[buf_index], "???" );
    buf_index+= 3;
  }
  RPG_LDM_print_debug( "Supplemental LDM Key: %s", buf );

  /* If customized key fits in the buffer, add it */
  if( buf_index < ( RPG_LDM_MAX_KEY_LEN - key_len ) )
  {
    RPG_LDM_print_debug( "Adding supplemental LDM Key" );
    sprintf( key_ptr, "%s", buf );
  }
  else
  {
    RPG_LDM_print_debug( "Supplemental LDM Key too long to add" );
  }

  RPG_LDM_print_debug( "Leave Customize_LDM_key()" );
}

/**************************************************************************
 Description: Validate filters used to send/ignore RPG products.
**************************************************************************/

static void Validate_filters( char *buf, int **filters, int *num_filters )
{
  char *cmd_args = NULL;
  char *tok = NULL;
  int   new_filter = 0;

  /* Error if list of filters is NULL */
  if( buf == NULL )
  {
    LE_send_msg( GL_ERROR, "List of buffers is NULL" );
    exit( RPG_LDM_FAILURE );
  }

  /* Duplicate string and ensure memory allocated */
  if( ( cmd_args = strdup( buf ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "strdup %s failed", buf );
    exit( RPG_LDM_FAILURE );
  }

  /* Check for first valid token */
  if( ( tok = strtok( cmd_args, "," ) ) == NULL )
  {
    LE_send_msg( GL_ERROR, "Empty list of filters to send" );
    exit( RPG_LDM_INVALID_COMMAND_LINE );
  }
  else
  {
    /* If token is a number, add to list. If not, ignore. */
    if( ( new_filter = atoi( tok ) ) > 0 )
    {
      Add_filter_to_list( new_filter, filters, num_filters );
    }
    else
    {
      LE_send_msg( GL_INFO, "Ignoring filter %s", tok );
    }
  }

  /* Check for any other tokens */
  while( ( tok = strtok( NULL, "," ) ) != NULL )
  {
    /* If token is a number, add to list. If not, ignore. */
    if( ( new_filter = atoi( tok ) ) > 0 )
    {
      Add_filter_to_list( new_filter, filters, num_filters );
    }
    else
    {
      LE_send_msg( GL_INFO, "Ignoring filter %s", tok );
    }
  }

  if( cmd_args != NULL )
  {
    free( cmd_args );
    cmd_args = NULL;
  }
}

/**************************************************************************
 Description: Add filter to list. Reallocate space if needed.
**************************************************************************/

static void Add_filter_to_list( int new_filter, int **filters, int *num )
{
  /* Check if more space needs to be allocated */
  if( *num % FILTER_LIST_INTERVAL == 0 )
  {
    if( ( *filters = (int *)realloc( *filters, sizeof(int) * ( *num + FILTER_LIST_INTERVAL ) ) ) == NULL )
    {
      LE_send_msg( GL_ERROR, "Realloc %d failed", *num + FILTER_LIST_INTERVAL );
      exit( RPG_LDM_MALLOC_FAILED );
    }
  }

  /* Add new filter to list and increase count */
  (*filters)[*num] = new_filter;
  *num = *num + 1;
}


