/************************************************************************
 *                                                                      *
 *      Module:  test_libl2.c                                           *
 *                                                                      *
 *      Description:  Example code for using libL2 library.             *
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2014/07/21 20:19:18 $
 * $Id: testlibl2.c,v 1.5 2014/07/21 20:19:18 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */


/* Include files. */

#include <libl2.h>
#include <ctype.h>

/* Defines/enums. */

/* Static/global variables. */

static int Print_status_msg_flag = LIBL2_NO;
static int Print_pmd_msg_flag = LIBL2_NO;
static int Print_vcp_msg_flag = LIBL2_NO;
static int Print_adapt_msg_flag = LIBL2_NO;
static int Print_bypass_map_msg_flag = LIBL2_NO;
static int Print_clutter_map_msg_flag = LIBL2_NO;
static int Print_rvol_flag = LIBL2_NO;
static int Print_relv_flag = LIBL2_NO;
static int Print_rrad_flag = LIBL2_NO;
static int Print_base_data_flag = LIBL2_NO;
static int Print_msg_hdr_flag = LIBL2_NO;
static int Print_base_hdr_flag = LIBL2_NO;
static int Print_moment_hdr_flag = LIBL2_NO;
static int Print_misc_flag = LIBL2_NO;
static int Segment_number = LIBL2_ALL_SEGMENTS;
static int Cut_index = LIBL2_ALL_CUTS;
static int Moment_index = LIBL2_ALL_MOMENTS;
static int Radial_number = LIBL2_ALL_RADIALS;

/* Function prototypes. */

static void Parse_cmd_line( int, char *[] );
static void Print_status_msg();
static void Print_pmd_msg();
static void Print_vcp_msg();
static void Print_adapt_msg();
static void Print_bypass_map_msg();
static void Print_clutter_map_msg();
static void Print_rvol();
static void Print_relv();
static void Print_rrad();
static void Print_base_data();
static void Print_msg_hdr();
static void Print_base_hdr();
static void Print_moment_hdr();
static void Print_misc();
static void Verify_input( char *, char * );
static void Print_usage();

/************************************************************************
 Description: This is the main function for the level-II stats decoder.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  int ret = 0;

  /* Parse command line options */
  Parse_cmd_line( argc, argv );

  /* Read level-II file */
  if( ( ret = libL2_read( argv[argc-1] ) ) < 0 )
  {
    fprintf( stderr, "libL2_read failed (%d)\n", ret );
  }

  /* Print specified level-II info */
  if( Print_status_msg_flag ){ Print_status_msg(); }
  if( Print_pmd_msg_flag ){ Print_pmd_msg(); }
  if( Print_vcp_msg_flag ){ Print_vcp_msg(); }
  if( Print_adapt_msg_flag ){ Print_adapt_msg(); }
  if( Print_bypass_map_msg_flag ){ Print_bypass_map_msg(); }
  if( Print_clutter_map_msg_flag ){ Print_clutter_map_msg(); }
  if( Print_rvol_flag ){ Print_rvol(); }
  if( Print_relv_flag ){ Print_relv(); }
  if( Print_rrad_flag ){ Print_rrad(); }
  if( Print_base_data_flag ){ Print_base_data(); }
  if( Print_msg_hdr_flag ){ Print_msg_hdr(); }
  if( Print_base_hdr_flag ){ Print_base_hdr(); }
  if( Print_moment_hdr_flag ){ Print_moment_hdr(); }
  if( Print_misc_flag ){ Print_misc(); }

  return 0;
}

/************************************************************************
 Description: Print usage information for this tool.
 ************************************************************************/

static void Print_usage()
{
  fprintf( stdout, "\n" );
  fprintf( stdout, "Usage: level-II_file [options]\n" );
  fprintf( stdout, "-A     - Print adapt msg\n" );
  fprintf( stdout, "-B     - Print bypass msg\n" );
  fprintf( stdout, "-C     - Print clutter msg\n" );
  fprintf( stdout, "-D     - Print base data\n" );
  fprintf( stdout, "-E     - Print relv block\n" );
  fprintf( stdout, "-G     - Print RDA/RPG message headers\n" );
  fprintf( stdout, "-J     - Print base data headers\n" );
  fprintf( stdout, "-K     - Print moment headers\n" );
  fprintf( stdout, "-M     - Print misc info\n" );
  fprintf( stdout, "-O     - Print rvol block\n" );
  fprintf( stdout, "-P     - Print pmd msg\n" );
  fprintf( stdout, "-R     - Print rrad block\n" );
  fprintf( stdout, "-S     - Print RDA status msg\n" );
  fprintf( stdout, "-V     - Print vcp msg\n" );
  fprintf( stdout, "-s int - Segment number (1-5) Default: ALL\n" );
  fprintf( stdout, "-c int - Cut index (0-25) Default: ALL\n" );
  fprintf( stdout, "-m int - Moment index (0-%d) Default: ALL\n", LIBL2_NUM_MOMENTS-1 );
  fprintf( stdout, "-r int - Radial number (1-720) Default: ALL\n" );
  fprintf( stdout, "-x     - Debug mode\n" );
  fprintf( stdout, "-h     - help\n" );
  fprintf( stdout, "\n" );
  fprintf( stdout, "MOMENT INDICES: " );
  fprintf( stdout, "%d-%s ", LIBL2_REF_INDEX, "DREF" );
  fprintf( stdout, "%d-%s ", LIBL2_SPW_INDEX, "DSW" );
  fprintf( stdout, "%d-%s ", LIBL2_VEL_INDEX, "DVEL" );
  fprintf( stdout, "%d-%s ", LIBL2_ZDR_INDEX, "DZDR" );
  fprintf( stdout, "%d-%s ", LIBL2_PHI_INDEX, "DPHI" );
  fprintf( stdout, "%d-%s ", LIBL2_RHO_INDEX, "DRHO" );
  fprintf( stdout, "\n" );
  fprintf( stdout, "\n" );
}

/************************************************************************
 Description: Parse command line options.
 ************************************************************************/

static void Parse_cmd_line( int argc, char *argv[] )
{
  int c = 0;

  while( ( c = getopt( argc, argv, "ABCDEGJKMOPRSVs:c:m:r:xh" ) ) != EOF )
  {
    switch( c )
    {
      case 'S':
        Print_status_msg_flag = LIBL2_YES;
        break;
      case 'P':
        Print_pmd_msg_flag = LIBL2_YES;
        break;
      case 'V':
        Print_vcp_msg_flag = LIBL2_YES;
        break;
      case 'A':
        Print_adapt_msg_flag = LIBL2_YES;
        break;
      case 'B':
        Print_bypass_map_msg_flag = LIBL2_YES;
        break;
      case 'C':
        Print_clutter_map_msg_flag = LIBL2_YES;
        break;
      case 'O':
        Print_rvol_flag = LIBL2_YES;
        break;
      case 'E':
        Print_relv_flag = LIBL2_YES;
        break;
      case 'G':
        Print_msg_hdr_flag = LIBL2_YES;
        break;
      case 'J':
        Print_base_hdr_flag = LIBL2_YES;
        break;
      case 'K':
        Print_moment_hdr_flag = LIBL2_YES;
        break;
      case 'R':
        Print_rrad_flag = LIBL2_YES;
        break;
      case 'D':
        Print_base_data_flag = LIBL2_YES;
        break;
      case 'M':
        Print_misc_flag = LIBL2_YES;
        break;
      case 's':
        Verify_input( "s", optarg );
        Segment_number = atoi( optarg );
        break;
      case 'c':
        Verify_input( "c", optarg );
        Cut_index = atoi( optarg );
        break;
      case 'm':
        Verify_input( "m", optarg );
        Moment_index = atoi( optarg );
        break;
      case 'r':
        Verify_input( "r", optarg );
        Radial_number = atoi( optarg );
        break;
      case 'x':
        libL2_debug_on();
        break;
      case 'h':
        Print_usage();
        exit(0);
        break;
      case '?':
      default:
        fprintf( stderr, "Bad command line argment\n" );
        Print_usage();
        exit( 1 );
    }
  }
}

/************************************************************************
 Description: Verify command line option is valid.
 ************************************************************************/

static void Verify_input( char *opt, char *arg )
{
  int i = 0;

  if( arg == NULL )
  {
    fprintf( stderr, "Option -%s is NULL\n", opt );
    exit( 1 );
  }

  for( i = 0; i < strlen( arg ); i++ )
  {
    if( !isdigit( arg[i] ) ) 
    {
      fprintf( stderr, "Option -%s is invalid format\n", opt );
      exit( 1 );
    }
  }
}

/************************************************************************
 Description: Print all RDA status messages in the level-II file.
 ************************************************************************/

static void Print_status_msg()
{
  int ret = 0;

  /* EXAMPLE */
  /* libL2_status_t **status_msgs = libL2_status_msgs(); */
  /* A 2-D array is returned, and can be used as follows: */
  /*
     libL2_status_t **a = libL2_status_msgs();
     for( i = 0; i < libL2_num_status_msgs(); i++ )
     {
       printf( "%ud\n", a[i]->rda_status );
     }
  */

  /* Call libL2 print function for output */

  if( ( ret = libL2_print_rda_status_msgs() ) != LIBL2_NO_ERROR )
  {
    printf("ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print RDA PMD message in the level-II file.
 ************************************************************************/

static void Print_pmd_msg()
{
  int ret = 0;

  /* EXAMPLE */
  /* libL2_pmd_t *pmd = libL2_pmd_msg(); */
  /* Use pmd as usual, such as pmd->t1_output_frames */

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_rda_pmd() ) != LIBL2_NO_ERROR )
  {
    printf("ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print RDA VCP message in the level-II file.
 ************************************************************************/

static void Print_vcp_msg()
{
  int ret = 0;
  
  /* EXAMPLE */
  /* libL2_vcp_t *vcp = libL2_vcp_msg(); */
  /* Use vcp as usual, such as vcp->vcp_elev_data */

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_rda_VCP() ) != LIBL2_NO_ERROR )
  {
    printf("ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print RDA adaptation message in the level-II file.
 ************************************************************************/

static void Print_adapt_msg()
{
  int ret = 0;

  /* EXAMPLE */
  /* libL2_adapt_t *adpt = libL2_adapt_msg(); */
  /* Use adpt as usual, such as adpt->adap_file_name */

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_rda_adapt() ) != LIBL2_NO_ERROR )
  {
    printf("ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print RDA bypass map message in the level-II file.
 ************************************************************************/

static void Print_bypass_map_msg()
{
  int ret = 0;

  /* EXAMPLE */
  /* libL2_bypass_map_t *byp = libL2_bypass_map_msg(); */
  /* Use byp as usual, such as byp->date */

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_rda_bypass_map( Segment_number, Radial_number ) ) != LIBL2_NO_ERROR )
  {
    printf("ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print RDA clutter map message in the level-II file.
 ************************************************************************/

static void Print_clutter_map_msg()
{
  int ret = 0;

  /* EXAMPLE */
  /* libL2_clutter_map_t *cl = libL2_clutter_map_msg(); */
  /* Use cl as usual, such as cl->date */

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_rda_clutter_map( Segment_number, Radial_number ) ) != LIBL2_NO_ERROR )
  {
    printf("ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print rvol data block.
 ************************************************************************/

static void Print_rvol()
{
  int ret = 0;

  /* EXAMPLE */
  /* libL2_RVOL_t *rvol = libL2_rvol( cut_index ); */
  /* Use rvol as usual, such as rvol->lat */

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_RVOL( Cut_index, Radial_number ) ) != LIBL2_NO_ERROR )
  {
    printf("ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print relv data block.
 ************************************************************************/

static void Print_relv()
{
  int ret = 0;

  /* EXAMPLE */
  /* libL2_RELV_t *relv = libL2_relv( cut_index ); */
  /* Use relv as usual, such as relv->atmos */

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_RELV( Cut_index, Radial_number ) ) != LIBL2_NO_ERROR )
  {
    printf("ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print rrad data block.
 ************************************************************************/

static void Print_rrad()
{
  int ret = 0;

  /* EXAMPLE */
  /* libL2_RRAD_t *rrad = libL2_rrad( cut_index ); */
  /* Use rrad as usual, such as rrad->horiz_noise */

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_RRAD( Cut_index, Radial_number ) ) != LIBL2_NO_ERROR )
  {
    printf("ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print miscellaneous info of the level-II file.
 ************************************************************************/

static void Print_misc()
{
  int ret = 0;

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_misc() ) != LIBL2_NO_ERROR )
  {
    printf( "ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print RDA/RPG message headers.
 ************************************************************************/

static void Print_msg_hdr()
{
  int ret = 0;

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_msg_header( Cut_index, Radial_number ) ) != LIBL2_NO_ERROR )
  {
    printf("ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print base data header for a radial.
 ************************************************************************/

static void Print_base_hdr()
{
  int ret = 0;

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_base_header( Cut_index, Radial_number ) ) != LIBL2_NO_ERROR )
  {
    printf("ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print header for moment of a radial.
 ************************************************************************/

static void Print_moment_hdr()
{
  int ret = 0;

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_moment_header( Cut_index, Radial_number, Moment_index ) ) != LIBL2_NO_ERROR )
  {
    printf("ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

/************************************************************************
 Description: Print base data from level-II file.
 ************************************************************************/

static void Print_base_data()
{
  int ret = 0;

  /* Call libL2 print function for output */
  if( ( ret = libL2_print_base_data( Cut_index, Radial_number, Moment_index ) ) != LIBL2_NO_ERROR )
  {
    printf( "ERROR: %d MSG: %s\n",libL2_error_code(), libL2_error_msg() );
  }
}

