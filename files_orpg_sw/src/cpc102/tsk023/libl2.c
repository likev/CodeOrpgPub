/************************************************************************
 *                                                                      *
 *      Module:  libL2.c                                                *
 *                                                                      *
 *      Description:  Library for decoding level-II files               *
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2014/07/21 20:19:18 $
 * $Id: libl2.c,v 1.8 2014/07/21 20:19:18 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/* Include files. */

#include <libl2.h>
#include <math.h>
#include <zlib.h>
#include <bzlib.h>
#include <netinet/in.h>

/* Defines/enums. */

enum { DATA_BLOCK_RVOL, DATA_BLOCK_RELV, DATA_BLOCK_RRAD };
enum { PRINT_MSG_HDR, PRINT_BDH, PRINT_MOMENT_HDR, PRINT_BASE_DATA };

#define	DEFAULT_COMPRESSION_RATIO	-1.0
#define	DEFAULT_VCP			-1
#define	DEFAULT_ELEVATION		-1.0
#define	DEFAULT_ELEVATION_OFFSET	-1
#define	DEFAULT_AZIMUTH			-1.0
#define	DEFAULT_GATE			-1
#define	DEFAULT_RESOLUTION		-1
#define	DEFAULT_NUM_AZIMUTHS		-1
#define	DEFAULT_NUM_MOMENTS		-1
#define	DEFAULT_NUM_GATES		-1
#define	FEET_TO_METERS			0.3048
#define	TIME_BUF_LEN			64
#define	NUM_SECS_IN_DAY			86400
#define	KILOBYTE			1024
#define	MEGABYTE			( KILOBYTE * KILOBYTE )
#define	MAX_TEXT_MSG_LEN		1024
#define	MAX_ICAO_LEN			4
#define	MAX_L2_DATE_LEN			4
#define	MAX_L2_TIME_LEN			4
#define	VOLUME_HEADER_SIZE		24
#define	UNCOMPRESS_FILE_SIZE_MAX	( 20 * MEGABYTE )
#define	MAX_LDM_BLOCK_SIZE		( 2 * MEGABYTE )
#define	METADATA_MSG_SIZE		2432
#define	MAX_NUM_RADIALS			750
#define	MAX_NUM_GATES			1900
#define	MAX_NUM_STATUS_MSGS		100
#define	RDA_ADAPT_MSG			18
#define	RDA_CLUTTER_MSG			15
#define	RDA_BYPASS_MSG			13
#define	RDA_VCP_MSG			5
#define	RDA_PMD_MSG			3
#define	RDA_STATUS_MSG			2
#define	RDA_RADIAL_MSG			31
/* Radial status */
#define RS_BEG_ELEV			BEG_ELEV
#define RS_INT_ELEV			INT_ELEV
#define RS_END_ELEV			END_ELEV
#define RS_BEG_VOL			BEG_VOL
#define RS_END_VOL			END_VOL
#define RS_BEG_ELEV_LAST_CUT		BEG_ELEV_LC
#define RS_PSEUDO_END_ELEV		PSEND_ELEV
#define RS_PSEUDO_END_VOL		PSEND_VOL
/* Misc */
#define COMMS_HEADER_SIZE		12
#define MAX_NUM_CUTS			VCP_MAXN_CUTS
#define BYPASS_MAP_NUM_RADIALS		ORDA_BYPASS_MAP_RADIALS
#define BYPASS_MAP_NUM_GATES		BYPASS_MAP_BINS
#define BYPASS_MAP_HW_PER_RADIAL	HW_PER_RADIAL
#define CLUTTER_MAP_NUM_RADIALS		NUM_AZIMUTH_SEGS_ORDA
#define CLUTTER_MAP_NUM_GATES		512
#define	BAMS_ELEV			(180.0/32768.0)
#define	BAMS_AZ				(45.0/32768.0)
/* Moment bitmasks */
#define	EMPTY_BITMASK			0x00
#define	REF_BITMASK			0x01
#define	VEL_BITMASK			0x02
#define	SPW_BITMASK			0x04
#define	ZDR_BITMASK			0x08
#define	PHI_BITMASK			0x10
#define	RHO_BITMASK			0x20
#define	MAX_BITMASK		( REF_BITMASK | VEL_BITMASK | SPW_BITMASK | ZDR_BITMASK | PHI_BITMASK | RHO_BITMASK )
/* Bitmasks for Bypass Map Data */
#define	BIT_1_MASK			0x0001
#define	BIT_2_MASK			0x0002
#define	BIT_3_MASK			0x0004
#define	BIT_4_MASK			0x0008
#define	BIT_5_MASK			0x0010
#define	BIT_6_MASK			0x0020
#define	BIT_7_MASK			0x0040
#define	BIT_8_MASK			0x0080
#define	BIT_9_MASK			0x0100
#define	BIT_10_MASK			0x0200
#define	BIT_11_MASK			0x0400
#define	BIT_12_MASK			0x0800
#define	BIT_13_MASK			0x1000
#define	BIT_14_MASK			0x2000
#define	BIT_15_MASK			0x4000
#define	BIT_16_MASK			0x8000

/* Static/global variables. */

static int                  Debug_flag = LIBL2_NO;
static int                  Error_code = LIBL2_NO_ERROR;
static char                 Error_msg[MAX_TEXT_MSG_LEN];
static char                 L2_file_name_buf[LIBL2_MAX_FILE_NAME_LEN+1];
static int                  L2_file_type_flag = LIBL2_UNKNOWN_FORMAT;
static int                  L2_uncompress_size_max = UNCOMPRESS_FILE_SIZE_MAX;
static int                  L2_uncompress_size = 0;
static char                *L2_buf = NULL;
static char                 L2_ICAO[MAX_ICAO_LEN+1];
static int                  L2_date = 0;
static int                  L2_time = 0;
static float                L2_compression_ratio = DEFAULT_COMPRESSION_RATIO;
static libL2_pmd_t         *L2_pmd = NULL;
static libL2_vcp_t         *L2_vcp = NULL;
static libL2_adapt_t       *L2_adapt = NULL;
static libL2_clutter_map_t *L2_clutter = NULL;
static libL2_bypass_map_t  *L2_bypass = NULL;
static libL2_status_t      **L2_status_msgs = NULL;
static int                  L2_num_status_msgs = 0;
/* Per file/volume */
static int                  L2_VCP = DEFAULT_VCP;
static int                  L2_num_cuts = 0;
static float                L2_elevations[MAX_NUM_CUTS];
static int                  L2_num_azimuths[MAX_NUM_CUTS];
static int                  L2_num_moments[MAX_NUM_CUTS];
static int                  L2_moments[MAX_NUM_CUTS];
static int                  L2_elevation_offsets[MAX_NUM_CUTS];
static float                L2_azimuths[MAX_NUM_CUTS][MAX_NUM_RADIALS];
static libL2_RVOL_t         L2_vol[MAX_NUM_CUTS][MAX_NUM_RADIALS];
static libL2_RELV_t         L2_elv[MAX_NUM_CUTS][MAX_NUM_RADIALS];
static libL2_RRAD_t         L2_rad[MAX_NUM_CUTS][MAX_NUM_RADIALS];
/* Per cut/sweep */
static int L2_azimuth_resolutions[MAX_NUM_CUTS][LIBL2_NUM_MOMENTS];
static int L2_gate_resolutions[MAX_NUM_CUTS][LIBL2_NUM_MOMENTS];
static int L2_num_gates[MAX_NUM_CUTS][LIBL2_NUM_MOMENTS];
static int L2_gates[MAX_NUM_CUTS][LIBL2_NUM_MOMENTS][MAX_NUM_GATES];

/* Function prototypes. */

static int   Init_variables( char * );
static int   Read_L2_file();
static int   Read_L2_file_NEXRAD();
static int   Read_L2_file_gzip();
static int   Read_L2_file_bzip2();
static int   Read_L2_file_compress();
static int   Parse_L2_buf();
static int   Parse_meta_data();
static int   Parse_base_data();
static int   Reparse_base_data( int, int, int, float * );
static int   Add_status_msg( char * );
static int   Is_start_of_volume_file();
static int   Validate_cut_index( int );
static int   Validate_moment_index( int );
static int   Validate_moment_bitmask( int );
static int   Validate_elevation_segment_number( int );
static int   Validate_bypass_map_segment_number( int );
static int   Validate_bypass_map_radial_number( int, int );
static int   Validate_clutter_map_segment_number( int );
static int   Validate_clutter_map_radial_number( int, int );
static int   Validate_radial_number( int, int );
static int   Get_moment_index_from_string( char * );
static void  Process_radial( int, char * );
static void  Get_radial_info( char * );
static int   Get_moments( int );
static int   Print_msg( int, int, int, int );
static int   Print_msg_header( libL2_msg_hdr_t * );
static int   Print_base_data_header( libL2_base_hdr_t * );
static int   Print_moment_header( libL2_moment_t * );
static int   Print_base_data( libL2_moment_t * );
static char *Get_moment_bitmask_string( int );
static char *Get_moment_index_string( int );
static char *Convert_date( int );
static char *Convert_milliseconds( long );
static float Decode_angle( short );
static void  libL2_debug( const char *, ... );

/*
  The format of a NEXRAD level-II file is as follows:
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

/*
  The format of a NCDC level-II file is as follows:
    24 byte volume header -
           bytes 00-08 - file name (AR2V00xx. where xx is LDM version)
           bytes 09-11 - volume number sequence (001-999)
           bytes 12-15 - volume start date
           bytes 16-19 - volume start time (milliseconds after midnight)
           bytes 20-23 - ICAO
    meta data record
    radials record
    radials record
    .
    .
    .
*/

/*
  For this library:
    (variable name)_index is implied to be zero-indexed
    (variable name)_number is implied to be one-indexed
*/

/************************************************************************
 Description: Set level-II file name, initialize variables, read the file
              and parse the data to determine intial parameters.
 ************************************************************************/

int libL2_read( char *l2_filename )
{
  static char *fx = "libL2_read";
  int ret = LIBL2_NO_ERROR;

  libL2_debug( "%s: Level-II file name %s", fx, l2_filename );

  if( l2_filename == NULL )
  {
    Error_code = LIBL2_L2_FILENAME_NULL;
    sprintf( Error_msg, "%s: L2 filename is NULL (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }
  else if( strlen( l2_filename ) > LIBL2_MAX_FILE_NAME_LEN )
  {
    Error_code = LIBL2_L2_FILENAME_TOO_LONG;
    sprintf( Error_msg, "%s: L2 filename too long (%d). Max is %d.", fx, Error_code, LIBL2_MAX_FILE_NAME_LEN );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  if( ( ret = Init_variables( l2_filename ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Init_variables returns %d", fx, ret );
    return ret;
  }

  if( ( ret = Read_L2_file() ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Read_L2_file returns %d", fx, ret );
    return ret;
  }

  if( ( ret = Parse_L2_buf() ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Parse_L2_buf returns %d", fx, ret );
    return ret;
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Turn debug messages off (default).
 ************************************************************************/

void libL2_debug_on()
{
  libL2_debug( "Setting debug flag YES" );
  Debug_flag = LIBL2_YES;
}

/************************************************************************
 Description: Turn debug messages on.
 ************************************************************************/

void libL2_debug_off()
{
  libL2_debug( "Setting debug flag NO" );
  Debug_flag = LIBL2_NO;
}

/************************************************************************
 Description: Return error message.
 ************************************************************************/

char *libL2_error_msg()
{
  return &Error_msg[0];
}

/************************************************************************
 Description: Return error code.
 ************************************************************************/

int libL2_error_code()
{
  return Error_code;
}

/************************************************************************
 Description: Return file type macro associated with the level-II file.
 ************************************************************************/

int libL2_filetype()
{
  return L2_file_type_flag;
}

/************************************************************************
 Description: Return file type string associated with the level-II file.
 ************************************************************************/

char *libL2_filetype_string()
{
  static char file_type[20];

  if( L2_file_type_flag == LIBL2_NEXRAD )
  {
    strcpy( file_type, "NEXRAD" );
  }
  else if( L2_file_type_flag == LIBL2_GZIP )
  {
    strcpy( file_type, "NCDC GZIP" );
  }
  else if( L2_file_type_flag == LIBL2_BZIP2 )
  {
    strcpy( file_type, "NCDC BZIP2" );
  }
  else if( L2_file_type_flag == LIBL2_COMPRESS )
  {
    strcpy( file_type, "NCDC COMPRESS" );
  }
  else if( L2_file_type_flag == LIBL2_UNCOMPRESS )
  {
    strcpy( file_type, "NCDC NON-COMPRESS" );
  }
  else
  {
    strcpy( file_type, "UNKNOWN FILE TYPE" );
  }

  return &file_type[0];
}

/************************************************************************
 Description: Return ICAO string associated with the level-II file.
 ************************************************************************/

char *libL2_ICAO()
{
  static char *fx = "libL2_ICAO";

  if( &L2_ICAO[0] == NULL || strlen( L2_ICAO ) == 0 )
  {
    Error_code = LIBL2_ICAO_NOT_DEFINED;
    sprintf( Error_msg, "%s: ICAO undefined in %s (%d)", fx, L2_file_name_buf, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  return &L2_ICAO[0];
}

/************************************************************************
 Description: Return epoch seconds associated with the level-II file.
 ************************************************************************/

time_t libL2_epoch()
{
  return (time_t) ( ((L2_date-1)*NUM_SECS_IN_DAY) + (L2_time/1000) );
}

/************************************************************************
 Description: Return date string associated with the level-II file.
 ************************************************************************/

char *libL2_date()
{
  return Convert_date( L2_date );
}

/************************************************************************
 Description: Return time string associated with the level-II file.
 ************************************************************************/

char *libL2_time()
{
  return Convert_milliseconds( (long) L2_time );
}

/************************************************************************
 Description: Return latitude of radar.
 ************************************************************************/

float libL2_latitude()
{
  return L2_vol[0][0].lat;
}

/************************************************************************
 Description: Return longitude of radar.
 ************************************************************************/

float libL2_longitude()
{
  return L2_vol[0][0].lon;
}

/************************************************************************
 Description: Return elevation/height of radar (convert feet to meters).
 ************************************************************************/

int libL2_height()
{
  return L2_vol[0][0].height * FEET_TO_METERS;
}

/************************************************************************
 Description: Return VCP associated with the level-II file.
 ************************************************************************/

int libL2_VCP()
{
  return L2_VCP;
}

/************************************************************************
 Description: Return number of cuts for level-II file.
 ************************************************************************/

int libL2_num_cuts()
{
  return L2_num_cuts;
}

/************************************************************************
 Description: Return array of elevations (in degrees) of cuts in level-II
     file.
 ************************************************************************/

float *libL2_elevations()
{
  static char *fx = "libL2_elevations";

  if( L2_num_cuts < 1 )
  {
    Error_code = LIBL2_NO_DATA_CUTS;
    sprintf( Error_msg, "%s: No data cuts defined in %s (%d)", fx, L2_file_name_buf, Error_code );
    libL2_debug( "%s", Error_msg );
    libL2_debug( "%s: L2_num_cuts = %d", fx, L2_num_cuts );
    return NULL;
  }

  return &L2_elevations[0];
}

/************************************************************************
 Description: Return compression ratio. Ratio only takes into account
     compressed data. Data already uncompressed in the file is not used
     in the calculation.
 ************************************************************************/

float libL2_compression_ratio()
{
  return L2_compression_ratio;
}

/************************************************************************
 Description: Return number of RDA status messages for level-II file.
 ************************************************************************/

int libL2_num_status_msgs()
{
  return L2_num_status_msgs;
}

/************************************************************************
 Description: Return array of RDA status messages for level-II file.
 ************************************************************************/

libL2_status_t **libL2_status_msgs()
{
  static char *fx = "libL2_status_msgs";

  if( L2_num_status_msgs < 1 )
  {
    Error_code = LIBL2_NO_STATUS_MSGS;
    sprintf( Error_msg, "%s: No RDA Status Msgs in %s (%d)", fx, L2_file_name_buf, Error_code );
    libL2_debug( "%s", Error_msg );
    libL2_debug( "%s: L2_num_status_msgs = %d", fx, L2_num_status_msgs );
    return NULL;
  }

  return &L2_status_msgs[0];
}

/************************************************************************
 Description: Return pointer to performance data in metadata.
 ************************************************************************/

libL2_pmd_t *libL2_pmd_msg()
{
  static char *fx = "libL2_pmd_msgs";

  if( L2_pmd == NULL )
  {
    Error_code = LIBL2_NO_PMD_MSG;
    sprintf( Error_msg, "%s: No RDA PMD Msgs in %s (%d)", fx, L2_file_name_buf, Error_code );
    libL2_debug( "%s", Error_msg );
    libL2_debug( "%s: L2_pmd is NULL", fx );
    return NULL;
  }

  return L2_pmd;
}

/************************************************************************
 Description: Return pointer to RDA VCP data in metadata.
 ************************************************************************/

libL2_vcp_t *libL2_vcp_msg()
{
  static char *fx = "libL2_vcp_msgs";

  if( L2_vcp == NULL )
  {
    Error_code = LIBL2_NO_VCP_MSG;
    sprintf( Error_msg, "%s: No RDA VCP Msgs in %s (%d)", fx, L2_file_name_buf, Error_code );
    libL2_debug( "%s", Error_msg );
    libL2_debug( "%s: L2_vcp is NULL", fx );
    return NULL;
  }

  return L2_vcp;
}

/************************************************************************
 Description: Return pointer to RDA adaptation data in metadata.
 ************************************************************************/

libL2_adapt_t *libL2_adapt_msg()
{
  static char *fx = "libL2_adapt_msgs";

  if( L2_adapt == NULL )
  {
    Error_code = LIBL2_NO_ADAPT_MSG;
    sprintf( Error_msg, "%s: No RDA Adapt Msgs in %s (%d)", fx, L2_file_name_buf, Error_code );
    libL2_debug( "%s", Error_msg );
    libL2_debug( "%s: L2_adapt is NULL", fx );
    return NULL;
  }

  return L2_adapt;
}

/************************************************************************
 Description: Return pointer to RDA bypass data in metadata.
 ************************************************************************/

libL2_bypass_map_t *libL2_bypass_map_msg()
{
  static char *fx = "libL2_bypass_map_msg";

  if( L2_bypass == NULL )
  {
    Error_code = LIBL2_NO_BYPASS_MAP_MSG;
    sprintf( Error_msg, "%s: No RDA Bypass Msgs in %s (%d)", fx, L2_file_name_buf, Error_code );
    libL2_debug( "%s", Error_msg );
    libL2_debug( "%s: L2_bypass is NULL", fx );
    return NULL;
  }

  return L2_bypass;
}

/************************************************************************
 Description: Return pointer to RDA clutter data in metadata.
 ************************************************************************/

libL2_clutter_map_t *libL2_clutter_map_msg()
{
  static char *fx = "libL2_clutter_map_msg";

  if( L2_clutter == NULL )
  {
    Error_code = LIBL2_NO_CLUTTER_MAP_MSG;
    sprintf( Error_msg, "%s: No RDA Clutter Msgs in %s (%d)", fx, L2_file_name_buf, Error_code );
    libL2_debug( "%s", Error_msg );
    libL2_debug( "%s: L2_clutter is NULL", fx );
    return NULL;
  }

  return L2_clutter;
}

/************************************************************************
 Description: Return number of radials for specified cut index.
 ************************************************************************/

int libL2_num_azimuths( int cut_index )
{
  static char *fx = "libL2_clutter_map_msg";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return L2_num_azimuths[cut_index];
}

/************************************************************************
 Description: Return number of gates for specified cut index and moment
     index.
 ************************************************************************/

int libL2_num_gates( int cut_index, int moment_index )
{
  static char *fx = "libL2_clutter_map_msg";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( ( ret = Validate_moment_index( moment_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_moment_index returns %d", fx, ret );
    return ret;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }
  else if( moment_index == LIBL2_ALL_MOMENTS )
  {
    Error_code = LIBL2_BAD_MOMENT_INDEX;
    sprintf( Error_msg, "%s: All moments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return L2_num_gates[cut_index][moment_index];
}

/************************************************************************
 Description: Return array of azimuths (in degrees) for specified cut index.
 ************************************************************************/

float *libL2_azimuths( int cut_index )
{
  static char *fx = "libL2_azimuths";
  static float *arr = NULL;
  int i = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", ret );
    return NULL;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  if( arr != NULL )
  {
    libL2_debug( "%s: arr previously defined", fx );
    free( arr );
    arr = NULL;
  }
  if( ( arr = malloc( libL2_num_azimuths(cut_index) * sizeof( float ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for arr (%d)", fx, Error_code );
    return NULL;
  }
  for( i = 0; i < libL2_num_azimuths(cut_index); i++ )
  {
    arr[i] = L2_azimuths[cut_index][i];
  }

  return &arr[0];
}

/************************************************************************
 Description: Return array of gates (in meters) for specified cut index and
     moment index.
 ************************************************************************/

float *libL2_gates( int cut_index, int moment_index )
{
  static char *fx = "libL2_gates";
  static float *arr = NULL;
  int i = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return NULL;
  }
  else if( ( ret = Validate_moment_index( moment_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_moment_index returns %d", fx, ret );
    return NULL;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }
  else if( moment_index == LIBL2_ALL_MOMENTS )
  {
    Error_code = LIBL2_BAD_MOMENT_INDEX;
    sprintf( Error_msg, "%s: All moments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  if( arr != NULL )
  {
    libL2_debug( "%s: arr previously defined", fx );
    free( arr );
    arr = NULL;
  }
  if( ( arr = malloc( libL2_num_gates(cut_index,moment_index) * sizeof( float ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for arr (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }
  for( i = 0; i < libL2_num_gates(cut_index,moment_index); i++ )
  {
    arr[i] = L2_gates[cut_index][moment_index][i];
  }

  return &arr[0];
}

/************************************************************************
 Description: Return array of data block structs for each radial of a
     specified cut index and data block.
 ************************************************************************/

char *Get_data_block( int cut_index, int radial_number, int data_block )
{
  static char *fx = "Get_data_block";
  static char *arr = NULL;
  int i = 0;
  int j = 0;
  int num_elements = 0;
  int pos = 0;
  int ret = LIBL2_NO_ERROR;

  libL2_debug( "%s: cut_index: %d radial_number: %d data_block: %d", cut_index, radial_number, data_block );

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return NULL;
  }
  else if( ( ret = Validate_radial_number( cut_index, radial_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_radial_number returns %d", fx, ret );
    return NULL;
  }

  if( arr != NULL )
  {
    libL2_debug( "%s: arr previously defined", fx );
    free( arr );
    arr = NULL;
  }

  num_elements = 0; 
  for( i = 0; i < libL2_num_cuts(i); i++ )
  {
    if( cut_index == LIBL2_ALL_CUTS || cut_index == i )
    {
      if( radial_number == LIBL2_ALL_RADIALS )
      {
        num_elements += libL2_num_azimuths(i);
      }
      else
      {
        num_elements++;
      }
    }
  }

  if( data_block == DATA_BLOCK_RVOL )
  {
    if( ( arr = malloc( num_elements * sizeof( libL2_RVOL_t ) ) ) == NULL )
    {
      Error_code = LIBL2_MALLOC_FAILED;
      sprintf( Error_msg, "%s: failed malloc for arr (%d)", fx, Error_code );
      libL2_debug( "%s", Error_msg );
      return NULL;
    }
  }
  else if( data_block == DATA_BLOCK_RELV )
  {
    if( ( arr = malloc( num_elements * sizeof( libL2_RELV_t ) ) ) == NULL )
    {
      Error_code = LIBL2_MALLOC_FAILED;
      sprintf( Error_msg, "%s: failed malloc for arr (%d)", fx, Error_code );
      libL2_debug( "%s", Error_msg );
      return NULL;
    }
  }
  else if( data_block == DATA_BLOCK_RRAD )
  {
    if( ( arr = malloc( num_elements * sizeof( libL2_RRAD_t ) ) ) == NULL )
    {
      Error_code = LIBL2_MALLOC_FAILED;
      sprintf( Error_msg, "%s: failed malloc for arr (%d)", fx, Error_code );
      libL2_debug( "%s", Error_msg );
      return NULL;
    }
  }

  pos = 0;
  for( i = 0; i < libL2_num_cuts(i); i++ )
  {
    if( cut_index == LIBL2_ALL_CUTS || cut_index == i )
    {
      for( j = 0; j < libL2_num_azimuths(i); j++ )
      {
        if( radial_number == LIBL2_ALL_RADIALS || radial_number == j+1 )
        {
          if( data_block == DATA_BLOCK_RVOL )
          {
            memcpy( &arr[pos], (char *) &L2_vol[i][j], sizeof( libL2_RVOL_t ) );
            pos += sizeof( libL2_RVOL_t );
          }
          else if( data_block == DATA_BLOCK_RELV )
          {
            memcpy( &arr[pos], (char *) &L2_elv[i][j], sizeof( libL2_RELV_t ) );
            pos += sizeof( libL2_RELV_t );
          }
          else if( data_block == DATA_BLOCK_RRAD )
          {
            memcpy( &arr[pos], (char *) &L2_rad[i][j], sizeof( libL2_RRAD_t ) );
            pos += sizeof( libL2_RRAD_t );
          }
        }
      }
    }
  }

  return &arr[0];
}

/************************************************************************
 Description: Return array of RVOL structs for each radial of a specified
     cut index.
 ************************************************************************/

libL2_RVOL_t *libL2_rvol( int cut_index, int radial_number )
{
  return (libL2_RVOL_t *) Get_data_block( cut_index, radial_number, DATA_BLOCK_RVOL );
}

/************************************************************************
 Description: Return array of ELEV structs for each radial of a specified
     cut index.
 ************************************************************************/

libL2_RELV_t *libL2_relv( int cut_index, int radial_number )
{
  return (libL2_RELV_t *) Get_data_block( cut_index, radial_number, DATA_BLOCK_RELV );
}

/************************************************************************
 Description: Return array of RRAD structs for each radial of a specified
     cut index.
 ************************************************************************/

libL2_RRAD_t *libL2_rrad( int cut_index, int radial_number )
{
  return (libL2_RRAD_t *) Get_data_block( cut_index, radial_number, DATA_BLOCK_RRAD );
}

/************************************************************************
 Description: Get number of moments contained in the specified cut index.
 ************************************************************************/

int libL2_num_moments( int cut_index )
{
  static char *fx = "libL2_num_moments";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return L2_num_moments[cut_index];
}

/************************************************************************
 Description: Get moment bitmask indicating the moments contained
     in the specified cut index.
 ************************************************************************/

static int Get_moments( int cut_index )
{
  static char *fx = "Get_moments";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return L2_moments[cut_index];
}

/************************************************************************
 Description: Convert moment string into its equivalent index.
 ************************************************************************/

static int Get_moment_index_from_string( char *moment_string )
{
  static char *fx = "Get_moment_index_from_string";

  if( strcmp( moment_string, "DREF" ) == 0 )
  {
    return LIBL2_REF_INDEX;
  }
  else if( strcmp( moment_string, "DVEL" ) == 0 )
  {
    return LIBL2_VEL_INDEX;
  }
  if( strcmp( moment_string, "DSW " ) == 0 )
  {
    return LIBL2_SPW_INDEX;
  }
  if( strcmp( moment_string, "DZDR" ) == 0 )
  {
    return LIBL2_ZDR_INDEX;
  }
  if( strcmp( moment_string, "DPHI" ) == 0 )
  {
    return LIBL2_PHI_INDEX;
  }
  if( strcmp( moment_string, "DRHO" ) == 0 )
  {
    return LIBL2_RHO_INDEX;
  }

  libL2_debug( "%s: returning -1 for %s", fx, moment_string );

  return -1;
}

/************************************************************************
 Description: Convert moment bitmask into a string that contains
     a comma separated list of moments.
 ************************************************************************/

#define	MOMENT_STRING_LEN	32

static char *Get_moment_bitmask_string( int moment_bitmask )
{
  static char *fx = "Get_moment_bitmask_string";
  static char *buf = NULL;
  int pos = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_moment_bitmask( moment_bitmask ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_moment_bitmask returns %d", fx, ret );
    return NULL;
  }

  libL2_debug( "%s: moment_bitmask = %d", fx, moment_bitmask );

  if( buf != NULL )
  {
    libL2_debug( "%s: buf previously defined", fx );
    free( buf );
    buf = NULL;
  }
  if( ( buf = malloc( MOMENT_STRING_LEN * sizeof( char ) ) ) != NULL )
  {
    pos = 0;
    if( moment_bitmask & REF_BITMASK )
    {
      sprintf( &buf[pos], "%s,", Get_moment_index_string( LIBL2_REF_INDEX ) );
      pos = strlen( buf );
    }
    if( moment_bitmask & VEL_BITMASK )
    {
      sprintf( &buf[pos], "%s,", Get_moment_index_string( LIBL2_VEL_INDEX ) );
      pos = strlen( buf );
    }
    if( moment_bitmask & SPW_BITMASK )
    {
      sprintf( &buf[pos], "%s,", Get_moment_index_string( LIBL2_SPW_INDEX ) );
      pos = strlen( buf );
    }
    if( moment_bitmask & ZDR_BITMASK )
    {
      sprintf( &buf[pos], "%s,", Get_moment_index_string( LIBL2_ZDR_INDEX ) );
      pos = strlen( buf );
    }
    if( moment_bitmask & PHI_BITMASK )
    {
      sprintf( &buf[pos], "%s,", Get_moment_index_string( LIBL2_PHI_INDEX ) );
      pos = strlen( buf );
    }
    if( moment_bitmask & RHO_BITMASK )
    {
      sprintf( &buf[pos], "%s,", Get_moment_index_string( LIBL2_RHO_INDEX ) );
      pos = strlen( buf );
    }
    if( buf[strlen(buf)-1] == ',' ){ buf[strlen(buf)-1] = '\0'; }
  }
  else
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for buf (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  if( buf == NULL || strlen( buf ) == 0 )
  {
    Error_code = LIBL2_NO_MOMENTS_IN_FLAG;
    sprintf( Error_msg, "%s: No moments for bitmask %d (%d)", fx, moment_bitmask, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  libL2_debug( "%s: returning %s", fx, &buf[0] );

  return &buf[0];
}

/************************************************************************
 Description: Boolean function indicating if base reflectivity moment
     is defined in the specified cut index.
 ************************************************************************/

int libL2_cut_has_REF( int cut_index )
{
  static char *fx = "libL2_cut_has_REF";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return ( Get_moments(cut_index) & REF_BITMASK );
}

/************************************************************************
 Description: Boolean function indicating if base velocity moment
     is defined in the specified cut index.
 ************************************************************************/

int libL2_cut_has_VEL( int cut_index )
{
  static char *fx = "libL2_cut_has_VEL";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return ( Get_moments(cut_index) & VEL_BITMASK );
}

/************************************************************************
 Description: Boolean function indicating if base spectrum width moment
     is defined in the specified cut index.
 ************************************************************************/

int libL2_cut_has_SPW( int cut_index )
{
  static char *fx = "libL2_cut_has_SPW";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return ( Get_moments(cut_index) & SPW_BITMASK );
}

/************************************************************************
 Description: Boolean function indicating if differential reflectivity moment
     is defined in the specified cut index.
 ************************************************************************/

int libL2_cut_has_ZDR( int cut_index )
{
  static char *fx = "libL2_cut_has_ZDR";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return ( Get_moments(cut_index) & ZDR_BITMASK );
}

/************************************************************************
 Description: Boolean function indicating if differential phase moment
     is defined in the specified cut index.
 ************************************************************************/

int libL2_cut_has_PHI( int cut_index )
{
  static char *fx = "libL2_cut_has_PHI";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return ( Get_moments(cut_index) & PHI_BITMASK );
}

/************************************************************************
 Description: Boolean function indicating if correlation coefficient moment
     is defined in the specified cut index.
 ************************************************************************/

int libL2_cut_has_RHO( int cut_index )
{
  static char *fx = "libL2_cut_has_RHO";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return ( Get_moments(cut_index) & RHO_BITMASK );
}

/************************************************************************
 Description: Returns resolution of radial azimuths.
 ************************************************************************/

#define	AZ_RES_DECODE	10.0

float libL2_azimuth_resolutions( int cut_index, int moment_index )
{
  static char *fx = "libL2_azimuth_resolutions";
  float res = -1;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return res;
  }
  else if( ( ret = Validate_moment_index( moment_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_moment_index returns %d", fx, ret );
    return res;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return res;
  }
  else if( moment_index == LIBL2_ALL_MOMENTS )
  {
    Error_code = LIBL2_BAD_MOMENT_INDEX;
    sprintf( Error_msg, "%s: All moments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return res;
  }

  res = (float) L2_azimuth_resolutions[cut_index][moment_index];
  return ( res / AZ_RES_DECODE );
}

/************************************************************************
 Description: Returns resolution of gate range.
 ************************************************************************/

float libL2_gate_resolutions( int cut_index, int moment_index )
{
  static char *fx = "libL2_gate_resolutions";
  float res = -1;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return res;
  }
  else if( ( ret = Validate_moment_index( moment_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_moment_index returns %d", fx, ret );
    return res;
  }
  else if( cut_index == LIBL2_ALL_CUTS )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: All cuts option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return res;
  }
  else if( moment_index == LIBL2_ALL_MOMENTS )
  {
    Error_code = LIBL2_BAD_MOMENT_INDEX;
    sprintf( Error_msg, "%s: All moments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return res;
  }

  res = (float) L2_gate_resolutions[cut_index][moment_index];
  return ( res / AZ_RES_DECODE );
}

/************************************************************************
 Description: Return a float array of data for a given cut index, moment
     index and radial number.
 ************************************************************************/

float *libL2_base_data( int cut_index, int moment_index, int radial_number )
{
  static char *fx = "libL2_base_data";
  static float *arr = NULL;
  int i = 0;
  int j = 0;
  int k = 0;
  int num_elements = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return NULL;
  }
  else if( ( ret = Validate_moment_index( moment_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_moment_index returns %d", fx, ret );
    return NULL;
  }
  else if( ( ret = Validate_radial_number( cut_index, radial_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_radial_number returns %d", fx, ret );
    return NULL;
  }

  if( arr != NULL )
  {
    libL2_debug( "%s: previously defined arr", fx );
    free( arr );
    arr = NULL;
  }

  num_elements = 0; 
  for( i = 0; i < libL2_num_cuts(i); i++ )
  {
    if( cut_index == LIBL2_ALL_CUTS || cut_index == i )
    {
      for( j = 0; j < libL2_num_azimuths(i); j++ )
      {
        if( radial_number == LIBL2_ALL_RADIALS || radial_number == j+1 )
        {
          for( k = 0; k < LIBL2_NUM_MOMENTS; k++ )
          {
            if( moment_index == LIBL2_ALL_MOMENTS || moment_index == k )
            {
              num_elements += libL2_num_gates( i, k );
            }
          }
        }
      }
    }
  }

  if( ( arr = malloc( num_elements * sizeof( float ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for arr (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  if( ( ret = Reparse_base_data( cut_index, moment_index, radial_number, arr ) ) != 0 )
  {
    Error_code = ret;
    sprintf( Error_msg, "%s: failed reparse base_data (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  return &arr[0];
}

/************************************************************************
 Description: Ensure cut index is a valid value.
 ************************************************************************/

static int Validate_cut_index( int cut_index )
{
  static char *fx = "Validate_cut_index";

  libL2_debug( "%s: cut_index = %d", fx, cut_index );

  if( cut_index != LIBL2_ALL_CUTS &&
      ( cut_index < 0 || cut_index >= L2_num_cuts ) )
  {
    Error_code = LIBL2_BAD_CUT_INDEX;
    sprintf( Error_msg, "%s: Argument (%d) is < 0 or >= %d (%d)", fx, cut_index, L2_num_cuts, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Ensure moment index is a valid value.
 ************************************************************************/

static int Validate_moment_index( int moment_index )
{
  static char *fx = "Validate_moment_index";

  libL2_debug( "%s: moment_index = %d", fx, moment_index );

  if( moment_index != LIBL2_ALL_MOMENTS &&
      ( moment_index < 0 || moment_index > LIBL2_NUM_MOMENTS ) )
  {
    Error_code = LIBL2_BAD_MOMENT_INDEX;
    sprintf( Error_msg, "%s: Argument (%d) is < 0 or > %d (%d)", fx, moment_index, LIBL2_NUM_MOMENTS, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Ensure moment bitmask is a valid value.
 ************************************************************************/

static int Validate_moment_bitmask( int moment_bitmask )
{
  static char *fx = "Validate_moment_bitmask";

  libL2_debug( "%s: moment_bitmask = %d", fx, moment_bitmask );

  if( moment_bitmask <= EMPTY_BITMASK || moment_bitmask > MAX_BITMASK )
  {
    Error_code = LIBL2_BAD_MOMENT_BITMASK;
    sprintf( Error_msg, "%s: Argument (%d) is <= %d or > %d (%d)", fx, moment_bitmask, EMPTY_BITMASK, MAX_BITMASK, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Ensure elevation segment number is a valid value.
 ************************************************************************/

static int Validate_elevation_segment_number( int segment_number )
{
  static char *fx = "Validate_elevation_segment_number";

  libL2_debug( "%s: segment_number = %d", fx, segment_number );

  if( segment_number != LIBL2_ALL_SEGMENTS &&
      ( segment_number < LIBL2_ELEV_SEGMENT_NUMBER_ONE || segment_number > libL2_num_segments() ) )
  {
    Error_code = LIBL2_BAD_SEGMENT_NUMBER;
    sprintf( Error_msg, "%s: Argument (%d) is < %d or >= %d (%d)", fx, segment_number, LIBL2_ELEV_SEGMENT_NUMBER_ONE, libL2_num_segments(), Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Ensure bypass map segment number is a valid value.
 ************************************************************************/

static int Validate_bypass_map_segment_number( int segment_number )
{
  static char *fx = "Validate_bypass_map_segment_number";

  libL2_debug( "%s: segment_number = %d", fx, segment_number );

  if( segment_number != LIBL2_ALL_SEGMENTS &&
      ( segment_number < LIBL2_ELEV_SEGMENT_NUMBER_ONE || segment_number > libL2_bypass_map_num_segments() ) )
  {
    Error_code = LIBL2_BAD_SEGMENT_NUMBER;
    sprintf( Error_msg, "%s: Argument (%d) is < %d or >= %d (%d)", fx, segment_number, LIBL2_ELEV_SEGMENT_NUMBER_ONE, libL2_bypass_map_num_segments(), Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Ensure clutter map segment number is a valid value.
 ************************************************************************/

static int Validate_clutter_map_segment_number( int segment_number )
{
  static char *fx = "Validate_clutter_map_segment_number";

  libL2_debug( "%s: segment_number = %d", fx, segment_number );

  if( segment_number != LIBL2_ALL_SEGMENTS &&
      ( segment_number < LIBL2_ELEV_SEGMENT_NUMBER_ONE || segment_number > libL2_clutter_map_num_segments() ) )
  {
    Error_code = LIBL2_BAD_SEGMENT_NUMBER;
    sprintf( Error_msg, "%s: Argument (%d) is < %d or >= %d (%d)", fx, segment_number, LIBL2_ELEV_SEGMENT_NUMBER_ONE, libL2_clutter_map_num_segments(), Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Ensure radial number is a valid value for specified cut.
 ************************************************************************/

static int Validate_radial_number( int cut_index, int radial_number )
{
  static char *fx = "Validate_radial_number";
  int ret = LIBL2_NO_ERROR;
  int i = 0;

  libL2_debug( "%s: cut_index = %d radial_number = %d", fx, cut_index, radial_number );

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }

  if( cut_index == LIBL2_ALL_CUTS )
  {
    for( i = 0; i < libL2_num_cuts(); i++ )
    {
      if( ( ret = Validate_radial_number( i, radial_number ) ) != LIBL2_NO_ERROR )
      {
        break;
      }
    }
    return ret;
  }
  else if( radial_number != LIBL2_ALL_RADIALS &&
      ( radial_number <= 0 || radial_number > libL2_num_azimuths(cut_index) ) )
  {
    Error_code = LIBL2_BAD_RADIAL_NUMBER;
    sprintf( Error_msg, "%s: Argument (%d) is <= 0 or >= %d (%d)", fx, radial_number, libL2_num_azimuths(cut_index), Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Ensure radial number is a valid value for specified segment.
 ************************************************************************/

static int Validate_clutter_map_radial_number( int segment_number, int radial_number )
{
  static char *fx = "Validate_clutter_map_radial_number";
  int ret = LIBL2_NO_ERROR;
  int i = 0;

  libL2_debug( "%s: segment_number = %d radial_number = %d", fx, segment_number, radial_number );

  if( ( ret = Validate_clutter_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_clutter_map_segment_number returns %d", fx, ret );
    return ret;
  }

  if( segment_number == LIBL2_ALL_SEGMENTS )
  {
    for( i = 0; i < libL2_clutter_map_num_segments(); i++ )
    {
      if( ( ret = Validate_clutter_map_radial_number( i+1, radial_number ) ) != LIBL2_NO_ERROR )
      {
        break;
      }
    }
    return ret;
  }
  else if( radial_number != LIBL2_ALL_RADIALS &&
      ( radial_number <= 0 || radial_number > libL2_clutter_map_num_azimuths(segment_number) ) )
  {
    Error_code = LIBL2_BAD_RADIAL_NUMBER;
    sprintf( Error_msg, "%s: Argument (%d) is <= 0 or >= %d (%d)", fx, radial_number, libL2_clutter_map_num_azimuths(segment_number), Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Ensure radial number is a valid value for specified segment.
 ************************************************************************/

static int Validate_bypass_map_radial_number( int segment_number, int radial_number )
{
  static char *fx = "Validate_bypass_map_radial_number";
  int ret = LIBL2_NO_ERROR;
  int i = 0;

  libL2_debug( "%s: segment_number = %d radial_number = %d", fx, segment_number, radial_number );
  
  if( ( ret = Validate_bypass_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_bypass_map_segment_number returns %d", fx, ret );
    return ret;
  }

  if( segment_number == LIBL2_ALL_SEGMENTS )
  {
    for( i = 0; i < libL2_bypass_map_num_segments(); i++ )
    { 
      if( ( ret = Validate_bypass_map_radial_number( i+1, radial_number ) ) != LIBL2_NO_ERROR )
      {
        break;
      }
    }
    return ret;
  }
  else if( radial_number != LIBL2_ALL_RADIALS &&
      ( radial_number <= 0 || radial_number > libL2_bypass_map_num_azimuths(segment_number) ) )
  {
    Error_code = LIBL2_BAD_RADIAL_NUMBER;
    sprintf( Error_msg, "%s: Argument (%d) is <= 0 or >= %d (%d)", fx, radial_number, libL2_bypass_map_num_azimuths(segment_number), Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Get number of segments defined in RDA adaptation data.
 ************************************************************************/

int libL2_num_segments()
{
  return L2_adapt->nbr_el_segments;
}

/************************************************************************
 Description: Get elevation (in degrees) that separates a segment as
     defined in RDA adaptation data. For segment 1, the elevation is
     what separates segements 1 and 2. For segment 2, the elevation is
     what separates segements 2 and 3. 
 ************************************************************************/

float libL2_segment_limits( int segment_number )
{
  static char *fx = "libL2_segment_limits";
  float slimit = -1.0;
  int ret = LIBL2_NO_ERROR; 

  if( ( ret = Validate_elevation_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_elevation_segment_number returns %d", fx, ret );
    return slimit;
  }
  else if( segment_number == LIBL2_ALL_SEGMENTS )
  {
    Error_code = LIBL2_BAD_SEGMENT_NUMBER;
    sprintf( Error_msg, "%s: All segments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return slimit;
  }

  libL2_debug( "%s: segment_number = %d", fx, segment_number );

  switch( segment_number )
  {
    case LIBL2_ELEV_SEGMENT_NUMBER_ONE:
      slimit = L2_adapt->seg1lim;
      break;
    case LIBL2_ELEV_SEGMENT_NUMBER_TWO:
      slimit = L2_adapt->seg2lim;
      break;
    case LIBL2_ELEV_SEGMENT_NUMBER_THREE:
      slimit = L2_adapt->seg3lim;
      break;
    case LIBL2_ELEV_SEGMENT_NUMBER_FOUR:
    default:
      slimit = L2_adapt->seg4lim;
  }

  libL2_debug( "%s: returning %f", slimit );

  return slimit;
}

/************************************************************************
 Description: Get number of segments of bypass map associated with this
     level-II file.
 ************************************************************************/

int libL2_bypass_map_num_segments()
{
  return L2_bypass->num_segs;
}

/************************************************************************
 Description: Get 2-D array of data  for given segment of bypass map.
 ************************************************************************/

int *libL2_bypass_map( int segment_number, int radial_number )
{
  static char *fx = "libL2_bypass_map";
  static int *arr = NULL;
  libL2_bypass_map_segment_t bypass_seg;
  short dataval = 0;
  int i = 0;
  int j = 0;
  int k = 0;
  int sn = 0;
  int pos = 0;
  int num_elements = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_bypass_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_bypass_map_segment_number returns %d", fx, ret );
    return NULL;
  }
  else if( ( ret = Validate_bypass_map_radial_number( segment_number, radial_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_bypass_map_radial_number returns %d", fx, ret );
    return NULL;
  }

  if( arr != NULL )
  {
    libL2_debug( "%s: arr previously defined", fx );
    free( arr );
    arr = NULL;
  }

  num_elements = 0; 
  for( i = 0; i < libL2_bypass_map_num_segments(i); i++ )
  {
    if( segment_number == LIBL2_ALL_SEGMENTS || segment_number == i+1 )
    {
      for( j = 0; j < libL2_bypass_map_num_azimuths(segment_number); j++ )
      {
        if( radial_number == LIBL2_ALL_RADIALS || radial_number == j+1 )
        {
          num_elements += libL2_bypass_map_num_gates(segment_number);
        }
      }
    }
  }

  if( ( arr = malloc( num_elements * sizeof( int ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for arr (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  pos = 0;
  for( i = 0; i < libL2_bypass_map_num_segments(i); i++ )
  {
    if( segment_number == LIBL2_ALL_SEGMENTS || segment_number == i+1 )
    {
      sn = i+1;
      bypass_seg = L2_bypass->segment[sn-1];
      for( j = 0; j < libL2_bypass_map_num_azimuths(sn); j++ )
      {
        if( radial_number == LIBL2_ALL_RADIALS || radial_number == j+1 )
        {
          for( k = 0; k < BYPASS_MAP_HW_PER_RADIAL; k++ )
          {
            dataval = bypass_seg.data[j][k];
            arr[pos++] = ( dataval & BIT_1_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_2_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_3_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_4_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_5_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_6_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_7_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_8_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_9_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_10_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_11_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_12_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_13_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_14_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_15_MASK ) ? 1 : 0;
            arr[pos++] = ( dataval & BIT_16_MASK ) ? 1 : 0;
          }
        }
      }
    }
  }

  return &arr[0];
}

/************************************************************************
 Description: Get number of radials for given segment of bypass map.
 ************************************************************************/

int libL2_bypass_map_num_azimuths( int segment_number )
{
  static char *fx = "libL2_bypass_map_num_azimuths";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_bypass_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_bypass_map_segment_number returns %d", fx, ret );
    return ret;
  }
  else if( segment_number == LIBL2_ALL_SEGMENTS )
  {
    Error_code = LIBL2_BAD_SEGMENT_NUMBER;
    sprintf( Error_msg, "%s: All segments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return BYPASS_MAP_NUM_RADIALS;
}

/************************************************************************
 Description: Get array of azimuths (in degrees) for given segment of
     bypass map.
 ************************************************************************/

float *libL2_bypass_map_azimuths( int segment_number )
{
  static char *fx = "libL2_bypass_map_azimuths";
  static float *arr = NULL;
  int i = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_bypass_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_bypass_map_segment_number returns %d", fx, ret );
    return NULL;
  }
  else if( segment_number == LIBL2_ALL_SEGMENTS )
  {
    Error_code = LIBL2_BAD_SEGMENT_NUMBER;
    sprintf( Error_msg, "%s: All segments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  if( arr != NULL )
  {
    libL2_debug( "%s: arr previously defined", fx );
    free( arr );
    arr = NULL;
  }
  if( ( arr = malloc( libL2_bypass_map_num_azimuths( segment_number ) * sizeof( float ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for arr (%d)", fx, Error_code );
    return NULL;
  }

  for( i = 0; i < libL2_bypass_map_num_azimuths( segment_number ); i++ )
  {
    arr[i] = (float)i; /* 1.4 degree beam width */
  }

  return &arr[0];
}

/************************************************************************
 Description: Get number of gates for given segment of bypass map.
 ************************************************************************/

int libL2_bypass_map_num_gates( int segment_number )
{
  static char *fx = "libL2_bypass_map_num_gates";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_bypass_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_bypass_map_segment_number returns %d", fx, ret );
    return ret;
  }
  else if( segment_number == LIBL2_ALL_SEGMENTS )
  {
    Error_code = LIBL2_BAD_SEGMENT_NUMBER;
    sprintf( Error_msg, "%s: All segments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return BYPASS_MAP_NUM_GATES;
}

/************************************************************************
 Description: Get array of gates (in meters) for given segment of
     bypass map.
 ************************************************************************/

float *libL2_bypass_map_gates( int segment_number )
{
  static char *fx = "libL2_bypass_map_gates";
  static float *arr = NULL;
  int i = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_bypass_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_bypass_map_segment_number returns %d", fx, ret );
    return NULL;
  }
  else if( segment_number == LIBL2_ALL_SEGMENTS )
  {
    Error_code = LIBL2_BAD_SEGMENT_NUMBER;
    sprintf( Error_msg, "%s: All segments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  if( arr != NULL )
  {
    libL2_debug( "%s: arr previously defined", arr );
    free( arr );
    arr = NULL;
  }
  if( ( arr = malloc( libL2_bypass_map_num_gates( segment_number ) * sizeof( float ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for arr (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  for( i = 0; i < libL2_bypass_map_num_gates( segment_number ); i++ )
  {
    arr[i] = i * 1000.0; /* 1km resolution */
  }

  return &arr[0];
}

/************************************************************************
 Description: Get number of segments of clutter map associated with this
     level-II file.
 ************************************************************************/

int libL2_clutter_map_num_segments()
{
  return L2_clutter->num_elevation_segs;
}

/************************************************************************
 Description: Get 2-D array of data  for given segment of clutter map.
 ************************************************************************/

int *libL2_clutter_map( int segment_number, int radial_number )
{
  static char *fx = "libL2_clutter_map";
  static int *arr = NULL;
  libL2_clutter_map_segment_t *clutter_radial = NULL;
  libL2_clutter_map_filter_t *clutter_filter = NULL;
  int clm_ptr = 0;
  int gate = 0;
  short *clm_map = NULL;
  int i = 0;
  int j = 0;
  int k = 0;
  int l = 0;
  int pos = 0;
  int num_elements = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_clutter_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_clutter_map_segment_number returns %d", fx, ret );
    return NULL;
  }
  else if( ( ret = Validate_clutter_map_radial_number( segment_number, radial_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_clutter_map_radial_number returns %d", fx, ret );
    return NULL;
  }

  if( arr != NULL )
  {
    libL2_debug( "%s: arr previously defined", fx );
    free( arr );
    arr = NULL;
  }

  num_elements = 0; 
  for( i = 0; i < libL2_clutter_map_num_segments(i); i++ )
  {
    if( segment_number == LIBL2_ALL_SEGMENTS || segment_number == i+1 )
    {
      for( j = 0; j < libL2_clutter_map_num_azimuths(segment_number); j++ )
      {
        if( radial_number == LIBL2_ALL_RADIALS || radial_number == j+1 )
        {
          num_elements += libL2_clutter_map_num_gates(segment_number);
        }
      }
    }
  }

  if( ( arr = malloc( num_elements * sizeof( int ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for arr (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  clm_map = (short *) &L2_clutter->data[0];
  clm_ptr = 0;

  pos = 0;
  for( i = 0; i < libL2_clutter_map_num_segments(); i++ )
  {
    for( j = 0; j < libL2_clutter_map_num_azimuths( segment_number ); j++ )
    {
      clutter_radial = ( libL2_clutter_map_segment_t * ) &clm_map[clm_ptr];
      gate = 0;
      if( ( segment_number == LIBL2_ALL_SEGMENTS || segment_number == i+1 ) &&
          ( radial_number == LIBL2_ALL_RADIALS || radial_number == j+1 ) )
      {
        for( k = 0; k < clutter_radial->num_zones; k++ )
        {
          clutter_filter = ( libL2_clutter_map_filter_t *) &clutter_radial->filter[k];
          for( l = gate; l < clutter_filter->range; l++ )
          {
            arr[pos++] = clutter_filter->op_code;
            gate++;
          }
        }
      }
      clm_ptr += ( ( clutter_radial->num_zones*sizeof( libL2_clutter_map_filter_t )/sizeof( short ) ) + 1 );
    }
  }

  return &arr[0];
}

/************************************************************************
 Description: Get number of radials for given segment of clutter map.
 ************************************************************************/

int libL2_clutter_map_num_azimuths( int segment_number )
{
  static char *fx = "libL2_clutter_map_num_azimuths";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_clutter_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_clutter_map_segment_number returns %d", fx, ret );
    return ret;
  }
  else if( segment_number == LIBL2_ALL_SEGMENTS )
  {
    Error_code = LIBL2_BAD_SEGMENT_NUMBER;
    sprintf( Error_msg, "%s: All segments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return CLUTTER_MAP_NUM_RADIALS;
}

/************************************************************************
 Description: Get array of azimuths (in degrees) for given segment of
     clutter map.
 ************************************************************************/

float *libL2_clutter_map_azimuths( int segment_number )
{
  static char *fx = "libL2_clutter_map_azimuths";
  static float *arr = NULL;
  int i = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_clutter_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_clutter_map_segment_number returns %d", fx, ret );
    return NULL;
  }
  else if( segment_number == LIBL2_ALL_SEGMENTS )
  {
    Error_code = LIBL2_BAD_SEGMENT_NUMBER;
    sprintf( Error_msg, "%s: All segments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  if( arr != NULL )
  {
    libL2_debug( "%s: arr previously defined" );
    free( arr );
    arr = NULL;
  }
  if( ( arr = malloc( libL2_clutter_map_num_azimuths( segment_number ) * sizeof( float ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for arr (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  for( i = 0; i < libL2_clutter_map_num_azimuths( segment_number ); i++ )
  {
    arr[i] = (float)i; /* 1.0 degree beam width */
  }

  return &arr[0];
}

/************************************************************************
 Description: Get number of gates for given segment of clutter map.
 ************************************************************************/

int libL2_clutter_map_num_gates( int segment_number )
{
  static char *fx = "libL2_clutter_map_num_gates";
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_clutter_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_clutter_map_segment_number returns %d", fx, ret );
    return ret;
  }
  else if( segment_number == LIBL2_ALL_SEGMENTS )
  {
    Error_code = LIBL2_BAD_SEGMENT_NUMBER;
    sprintf( Error_msg, "%s: All segments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  return CLUTTER_MAP_NUM_GATES;
}

/************************************************************************
 Description: Get array of gates (in meters) for given segment of
     clutter map.
 ************************************************************************/

float *libL2_clutter_map_gates( int segment_number )
{
  static char *fx = "libL2_clutter_map_gates";
  static float *arr = NULL;
  int i = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_clutter_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_clutter_map_segment_number returns %d", fx, ret );
    return NULL;
  }
  else if( segment_number == LIBL2_ALL_SEGMENTS )
  {
    Error_code = LIBL2_BAD_SEGMENT_NUMBER;
    sprintf( Error_msg, "%s: All segments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  if( arr != NULL )
  {
    libL2_debug( "%s: arr previously defined", fx );
    free( arr );
    arr = NULL;
  }
  if( ( arr = malloc( libL2_clutter_map_num_gates( segment_number ) * sizeof( float ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for arr (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  for( i = 0; i < libL2_clutter_map_num_gates( segment_number ); i++ )
  {
    arr[i] = i * 1000.0; /* 1km resolution */
  }

  return &arr[0];
}

/************************************************************************
 Description: Initialize variables for the new level-II file name. This
     allows a program to open more than one file.
 ************************************************************************/

static int Init_variables( char *l2_filename )
{
  static char *fx = "Init_variables";
  int i = 0;
  int j = 0;
  int k = 0;

  Error_code = LIBL2_NO_ERROR;
  for( i = 0; i < MAX_TEXT_MSG_LEN + 1; i++ )
  {
    Error_msg[i] = '\0';
  }

  for( i = 0; i < LIBL2_MAX_FILE_NAME_LEN + 1; i++ )
  {
    L2_file_name_buf[i] = '\0';
  }

  if( L2_buf != NULL )
  {
    libL2_debug( "%s: L2_buf previously defined", fx );
    free( L2_buf );
    L2_buf = NULL;
  }
  if( ( L2_buf = malloc( L2_uncompress_size_max ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for L2_buf (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  for( i = 0; i < MAX_ICAO_LEN + 1; i++ )
  {
    L2_ICAO[i] = '\0';
  }
  L2_date = 0;
  L2_time = 0;

  L2_uncompress_size_max = UNCOMPRESS_FILE_SIZE_MAX;
  L2_uncompress_size = 0;
  L2_compression_ratio = DEFAULT_COMPRESSION_RATIO;

  if( L2_pmd != NULL )
  {
    libL2_debug( "%s: L2_pmd previously defined", fx );
    free( L2_pmd );
    L2_pmd = NULL;
  }
  if( ( L2_pmd = malloc( sizeof( libL2_pmd_t ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for L2_pmd (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  if( L2_vcp != NULL )
  {
    libL2_debug( "%s: L2_vcp previously defined", fx );
    free( L2_vcp );
    L2_vcp = NULL;
  }
  if( ( L2_vcp = malloc( sizeof( libL2_vcp_t ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for L2_vcp (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  if( L2_adapt != NULL )
  {
    libL2_debug( "%s: L2_adapt previously defined", fx );
    free( L2_adapt );
    L2_adapt = NULL;
  }
  if( ( L2_adapt = malloc( sizeof( libL2_adapt_t ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for L2_adapt (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  if( L2_clutter != NULL )
  {
    libL2_debug( "%s: L2_clutter previously defined", fx );
    free( L2_clutter );
    L2_clutter = NULL;
  }
  if( ( L2_clutter = malloc( sizeof( libL2_clutter_map_t ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for L2_clutter (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  if( L2_bypass != NULL )
  {
    libL2_debug( "%s: L2_bypass previously defined", fx );
    free( L2_bypass );
    L2_bypass = NULL;
  }
  if( ( L2_bypass = malloc( sizeof( libL2_bypass_map_t ) ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for L2_bypass (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }

  if( L2_status_msgs != NULL )
  {
    libL2_debug( "%s: L2_status_msgs previously defined", fx );
    for( i = 0; i < MAX_NUM_STATUS_MSGS; i++ )
    {
      free( L2_status_msgs[i] );
    } 
    free( L2_status_msgs );
    L2_status_msgs = NULL;
  }
  if( ( L2_status_msgs = malloc( sizeof( libL2_status_t * ) * MAX_NUM_STATUS_MSGS ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc for L2_status_msgs (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }
  for( i = 0; i < MAX_NUM_STATUS_MSGS; i++ )
  {
    if( ( L2_status_msgs[i] = malloc( sizeof( libL2_status_t ) ) ) == NULL )
    {
      Error_code = LIBL2_MALLOC_FAILED;
      sprintf( Error_msg, "%s: failed malloc for L2_status_msgs[%d] (%d)", fx, i, Error_code );
      libL2_debug( "%s", Error_msg );
      return Error_code;
    }
  }
  L2_num_status_msgs = 0;

  L2_VCP = DEFAULT_VCP;
  L2_num_cuts = 0;

  for( i = 0; i < MAX_NUM_CUTS; i++ )
  {
    L2_elevations[i] = DEFAULT_ELEVATION;
    L2_num_azimuths[i] = DEFAULT_NUM_AZIMUTHS;
    L2_num_moments[i] = DEFAULT_NUM_MOMENTS;
    L2_moments[i] = EMPTY_BITMASK;
    L2_elevation_offsets[i] = DEFAULT_ELEVATION_OFFSET;
    for( j = 0; j < MAX_NUM_RADIALS; j++ )
    {
      L2_azimuths[i][j] = DEFAULT_AZIMUTH;
    }
    for( j = 0; j < LIBL2_NUM_MOMENTS; j++ )
    {
      L2_num_gates[i][j] = DEFAULT_NUM_GATES;
      L2_azimuth_resolutions[i][j] = DEFAULT_RESOLUTION;
      L2_gate_resolutions[i][j] = DEFAULT_RESOLUTION;
      for( k = 0; k < MAX_NUM_GATES; k++ )
      {
        L2_gates[i][j][k] = DEFAULT_GATE;
      }
    }
  }

  L2_file_type_flag = LIBL2_UNKNOWN_FORMAT;
  if( strstr( l2_filename, ".raw" ) )
  {
    L2_file_type_flag = LIBL2_NEXRAD;
    libL2_debug( "%s: file type = NEXRAD", fx );
  }
  else if( strstr( l2_filename, ".bz2" ) )
  {
    L2_file_type_flag = LIBL2_BZIP2;
    libL2_debug( "%s: file type = NCDC BZIP2", fx );
  }
  else if( strstr( l2_filename, ".Z" ) )
  {
    L2_file_type_flag = LIBL2_COMPRESS;
    libL2_debug( "%s: file type = NCDC COMPRESS", fx );
  }
  else if( strstr( l2_filename, ".gz" ) )
  {
    L2_file_type_flag = LIBL2_GZIP;
    libL2_debug( "%s: file type = NCDC GZIP", fx );
  }
  else
  {
    /* For now this is a catch-all. If non-compressed NCDC files
       have a specified format, then it goes here and the macro
       for LIBL2_UNKNOWN_FORMAT has meaning. */
    L2_file_type_flag = LIBL2_UNCOMPRESS;
    libL2_debug( "%s: file type = NCDC NON-COMPRESS", fx );
  }

  strcpy( L2_file_name_buf, l2_filename );

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Open the level-II file name passed into the library, and
     if successful, parse the data.
 ************************************************************************/

static int Read_L2_file()
{
  static char *fx = "Read_L2_file";

  if( L2_file_type_flag == LIBL2_NEXRAD )
  {
    libL2_debug( "%s: calling Read_L2_file_NEXRAD()", fx );
    return Read_L2_file_NEXRAD();
  }
  else if( L2_file_type_flag == LIBL2_GZIP )
  {
    libL2_debug( "%s: calling Read_L2_file_gzip()", fx );
    return Read_L2_file_gzip();
  }
  else if( L2_file_type_flag == LIBL2_BZIP2 )
  {
    libL2_debug( "%s: calling Read_L2_file_bzip2()", fx );
    return Read_L2_file_bzip2();
  }
  else if( L2_file_type_flag == LIBL2_COMPRESS )
  {
    libL2_debug( "%s: calling Read_L2_file_compress()", fx );
    return Read_L2_file_compress();
  }
  else if( L2_file_type_flag == LIBL2_UNCOMPRESS )
  {
    libL2_debug( "%s: calling Read_L2_file_gzip()", fx );
    /* If not compressed, gzip library treats it as such */
    return Read_L2_file_gzip();
  }

  Error_code = LIBL2_L2_UNKNOWN_FORMAT;
  sprintf( Error_msg, "%s: Level-II has unknown format (%d)", fx, Error_code );
  libL2_debug( "%s", Error_msg );
  return Error_code;
}

/************************************************************************
 Description: Read NEXRAD (*.raw) Level-II file.
 ************************************************************************/

static int Read_L2_file_NEXRAD()
{
  static char *fx = "Read_L2_file_NEXRAD";
  FILE *infile = NULL;
  char *file_buf = NULL;
  char *temp_buf = NULL;
  struct stat fs;
  int pos = 0;
  int control_word = 0;
  int error = LIBL2_NO_ERROR;
  unsigned int d_size = 0;
  int file_size = 0;
  int eov_flag = LIBL2_NO;
  int readblock_size = 2*MEGABYTE;
  int ret = LIBL2_NO_ERROR;

  /* Get size of compressed file, open file for reading, allocate space
     for buffer to hold contents of file and read file into buffer.
     Allocate space in a temporary buffer to be used as an intermediary
     between the file buffer and the final buffer. This buffer is
     needed because bzip2 decompression doesn't know the size of the
     decompressed data. The buffer allows the L2_buf to be resized
     if needed. */

  if( stat( L2_file_name_buf, &fs ) < 0 )
  {
    Error_code = LIBL2_STAT_L2_FILE_FAILED;
    sprintf( Error_msg, "%s: failed stat for level-II file (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }
  else if( ( file_size = (int) fs.st_size ) < 1 )
  {
    Error_code = LIBL2_STAT_L2_FILE_SIZE_ERROR;
    sprintf( Error_msg, "%s: Level-II file stat size < 1 (%d:%d)", fx, Error_code, file_size );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }
  else if( ( infile = fopen( L2_file_name_buf, "r" ) ) == NULL )
  {
    Error_code = LIBL2_OPEN_L2_FILE_FAILED;
    sprintf( Error_msg, "%s: failed fopen (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    ret = Error_code;
  }
  else if( ( file_buf = malloc( file_size ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed file_buf malloc (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    ret = Error_code;
  }
  else if( fread( &file_buf[0], sizeof( char ), file_size, infile ) != file_size )
  {
    Error_code = LIBL2_L2_FILE_READ_ERROR;
    sprintf( Error_msg, "%s: failed fread (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    ret = Error_code;
  }
  else if( ( temp_buf = malloc( readblock_size ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed temp_buf malloc (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    ret = Error_code;
  }

  if( ret == LIBL2_NO_ERROR )
  {
    /* Read volume header */
    memcpy( &L2_buf[L2_uncompress_size], &file_buf[pos], VOLUME_HEADER_SIZE );
    L2_uncompress_size += VOLUME_HEADER_SIZE;
    pos += VOLUME_HEADER_SIZE;

    /* Loop through compressed records. Each compressed record
       is preceded by a 4-byte control word indicating the size 
       of the compressed record. */
    while( pos < file_size && eov_flag == LIBL2_NO )
    {
      memcpy( &control_word, &file_buf[pos], sizeof( int ) );
      pos += sizeof( int );
  
      control_word = ntohl( control_word );
      if( control_word < 0 )
      {
        /* Negative size indicates last record of the volume. */
        control_word = -control_word;
        eov_flag = LIBL2_YES;
      }

      /* Decompress record into intermediate buffer. */
      d_size = readblock_size;
      error = BZ2_bzBuffToBuffDecompress( &temp_buf[0], &d_size,
                                      &file_buf[pos], control_word,
                                      0, 1 );
      if( error )
      {
        Error_code = LIBL2_BUNZIP_FAILED;
        sprintf( Error_msg, "%s: failed decompress (%d:%d)", fx, Error_code, error );
        libL2_debug( "%s", Error_msg );
        ret = Error_code;
        break;
      }

      pos += control_word;

      if( L2_uncompress_size + d_size > L2_uncompress_size_max )
      {
        /* If new data will fill L2_buf, resize */
        L2_uncompress_size_max = (int) ( L2_uncompress_size_max * 1.5 );
        if( ( L2_buf = realloc( L2_buf, L2_uncompress_size_max ) ) == NULL )
        {
          Error_code = LIBL2_REALLOC_FAILED;
          sprintf( Error_msg, "%s: failed realloc (%d)", fx, Error_code );
          libL2_debug( "%s", Error_msg );
          ret = Error_code;
          break;
        }
      }
      /* Copy decompressed data from intermediate buffer to L2_buf */
      memcpy( &L2_buf[L2_uncompress_size], &temp_buf[0], d_size );
      L2_uncompress_size += d_size;
      /* Calculate compression ratio */
      L2_compression_ratio = L2_uncompress_size / (float) file_size;
    }
  }

  /* Close file handle */
  if( infile != NULL )
  {
    libL2_debug( "%s: fclose infile", fx );
    fclose( infile );
  }

  /* Free file buffer */
  if( file_buf != NULL )
  {
    libL2_debug( "%s: free file_buf", fx );
    free( file_buf );
    file_buf = NULL;
  }

  /* Free intermediate buffer */
  if( temp_buf != NULL )
  {
    libL2_debug( "%s: free temp_buf", fx );
    free( temp_buf );
    temp_buf = NULL;
  }

  libL2_debug( "%s: returning %d", fx, ret );

  return ret;
}

/************************************************************************
 Description: Read gzip (*.gz) or uncompressed NCDC file.
 ************************************************************************/

static int Read_L2_file_gzip()
{
  static char *fx = "Read_L2_file_gzip";
  gzFile infile = NULL;
  int ret = LIBL2_NO_ERROR;
  int num_bytes = 0;
  int file_size = 0;
  char *temp_buf = NULL;
  int readblock_size = 2*MEGABYTE;
  struct stat fs;

  /* Get size of compressed file, open file for reading and allocate
     space in a temporary buffer to be used as an intermediary
     between the gzip stream and the final buffer. This buffer is
     needed to allow the L2_buf to be resized if needed. */

  if( stat( L2_file_name_buf, &fs ) < 0 )
  {
    Error_code = LIBL2_STAT_L2_FILE_FAILED;
    sprintf( Error_msg, "%s: failed stat for level-II file (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }
  else if( ( file_size = (int) fs.st_size ) < 1 )
  {
    Error_code = LIBL2_STAT_L2_FILE_SIZE_ERROR;
    sprintf( Error_msg, "%s: Level-II file stat size < 1 (%d:%d)", fx, Error_code, file_size );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }
  else if( ( infile = gzopen( L2_file_name_buf, "rb" ) ) == NULL )
  {
    Error_code = LIBL2_GUNZIP_FAILED;
    sprintf( Error_msg, "%s: failed gzopen (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    ret = Error_code;
  }
  else if( ( temp_buf = malloc( readblock_size ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    ret = Error_code;
  }

/* For zlib 1.2.4+, currently at zlib 1.2.3
  if( gzbuffer( infile, 2*MEGABYTE ) < 0 )
  {
    Error_code = LIBL2_GUNZIP_FAILED;
    sprintf( Error_msg, "%s: failed gzbuffer (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    ret = Error_code;
  }
*/

  if( ret == LIBL2_NO_ERROR )
  {
    /* Read in a pre-determined block of data (readblock_size) to the
       intermediate buffer. */
    while( !gzeof( infile ) )
    {
      if( ( num_bytes = gzread( infile, &temp_buf[0], readblock_size ) ) < 0 )
      {
        Error_code = LIBL2_GUNZIP_FAILED;
        sprintf( Error_msg, "%s: failed gzread (%d:%d)", fx, Error_code, ret );
        libL2_debug( "%s", Error_msg );
        ret = Error_code;
        break;
      }

      /* If new data will fill L2_buf, resize */
      if( L2_uncompress_size + num_bytes > L2_uncompress_size_max )
      {
        L2_uncompress_size_max = (int) ( L2_uncompress_size_max * 1.5 );
        if( ( L2_buf = realloc( L2_buf, L2_uncompress_size_max ) ) == NULL )
        {
          Error_code = LIBL2_REALLOC_FAILED;
          sprintf( Error_msg, "%s: failed realloc (%d)", fx, Error_code );
          libL2_debug( "%s", Error_msg );
          ret = Error_code;
          break;
        }
      }
      /* Copy decompressed data from intermediate buffer to L2_buf */
      memcpy( &L2_buf[L2_uncompress_size], &temp_buf[0], num_bytes );
      L2_uncompress_size += num_bytes;
      /* Calculate compression ratio */
      L2_compression_ratio = L2_uncompress_size / (float) file_size;
    } 
  } 

  /* Free intermediate buffer */
  if( temp_buf != NULL )
  {
    libL2_debug( "%s: free temp_buf", fx );
    free( temp_buf );
    temp_buf = NULL;
  }

  /* Close file handler */
  if( ( ret = gzclose( infile ) ) != Z_OK )
  {
    Error_code = LIBL2_GUNZIP_FAILED;
    sprintf( Error_msg, "%s: failed gzclose (%d:%d)", fx, Error_code, ret );
    libL2_debug( "%s", Error_msg );
    ret = Error_code;
  }

  libL2_debug( "%s: returning %d", fx, ret );

  return ret;
}

/************************************************************************
 Description: Read bzip2 (*.bz2) NCDC file.
 ************************************************************************/

static int Read_L2_file_bzip2()
{
  static char *fx = "Read_L2_file_bzip2";
  FILE *infile = NULL;
  BZFILE *bzinfile = NULL;
  int ret = LIBL2_NO_ERROR;
  int num_bytes = 0;
  int file_size = 0;
  struct stat fs;
  int bzerror_flag = BZ_OK;
  char *temp_buf = NULL;
  int readblock_size = 2*MEGABYTE;

  /* Get size of compressed file, open file for reading and allocate
     space in a temporary buffer to be used as an intermediary
     between the gzip stream and the final buffer. This buffer is
     needed to allow the L2_buf to be resized if needed. */

  if( stat( L2_file_name_buf, &fs ) < 0 )
  {
    Error_code = LIBL2_STAT_L2_FILE_FAILED;
    sprintf( Error_msg, "%s: failed stat for level-II file (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }
  else if( ( file_size = (int) fs.st_size ) < 1 )
  {
    Error_code = LIBL2_STAT_L2_FILE_SIZE_ERROR;
    sprintf( Error_msg, "%s: Level-II file stat size < 1 (%d:%d)", fx, Error_code, file_size );
    libL2_debug( "%s", Error_msg );
    return Error_code;
  }
  else if( ( infile = fopen( L2_file_name_buf, "rb" ) ) == NULL )
  {
    Error_code = LIBL2_BUNZIP_FAILED;
    sprintf( Error_msg, "%s: failed fopen (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    ret = Error_code;
  }
  else if( ( bzinfile = BZ2_bzReadOpen( &bzerror_flag, infile, 1, 0, NULL, 0 ) ) == NULL )
  {
    Error_code = LIBL2_BUNZIP_FAILED;
    sprintf( Error_msg, "%s: failed BZ2_bzReadOpen (%d:%d)", fx, Error_code, bzerror_flag );
    libL2_debug( "%s", Error_msg );
    ret = Error_code;
  }
  else if( ( temp_buf = malloc( readblock_size ) ) == NULL )
  {
    Error_code = LIBL2_MALLOC_FAILED;
    sprintf( Error_msg, "%s: failed malloc (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    ret = Error_code;
  }

  if( ret == LIBL2_NO_ERROR )
  {
    /* Read in a pre-determined block of data (readblock_size) to the
       intermediate buffer. */
    while( bzerror_flag == BZ_OK )
    {
      num_bytes = BZ2_bzRead( &bzerror_flag, bzinfile, &temp_buf[0], readblock_size );
      if( bzerror_flag != BZ_OK && bzerror_flag != BZ_STREAM_END )
      {
        Error_code = LIBL2_BUNZIP_FAILED;
        sprintf( Error_msg, "%s: failed BZ2_bzRead (%d:%d)", fx, Error_code, bzerror_flag );
        libL2_debug( "%s", Error_msg );
        ret = Error_code;
        break;
      }

      /* If new data will fill L2_buf, resize */
      if( L2_uncompress_size + num_bytes > L2_uncompress_size_max )
      {
        L2_uncompress_size_max = (int) ( L2_uncompress_size_max * 1.5 );
        if( ( L2_buf = realloc( L2_buf, L2_uncompress_size_max ) ) == NULL )
        {
          Error_code = LIBL2_REALLOC_FAILED;
          sprintf( Error_msg, "%s: failed realloc (%d)", fx, Error_code );
          libL2_debug( "%s", Error_msg );
          ret = Error_code;
          break;
        }
      }
      /* Copy decompressed data from intermediate buffer to L2_buf */
      memcpy( &L2_buf[L2_uncompress_size], &temp_buf[0], num_bytes );
      L2_uncompress_size += num_bytes;
      /* Calculate compression ratio */
      L2_compression_ratio = L2_uncompress_size / (float) file_size;
    } 
  } 

  /* Free intermediate buffer */
  if( temp_buf != NULL )
  {
    libL2_debug( "%s: free temp_buf", fx );
    free( temp_buf );
    temp_buf = NULL;
  }

  /* Close bzip2 file handler */
  if( bzinfile != NULL )
  {
    libL2_debug( "%s: BZ2_bzReadClose bzinfile", fx );
    BZ2_bzReadClose( &bzerror_flag, bzinfile );
    if( bzerror_flag != BZ_OK )
    {
      Error_code = LIBL2_BUNZIP_FAILED;
      sprintf( Error_msg, "%s: failed BZ2_bzReadClose (%d:%d)", fx, Error_code, bzerror_flag );
      libL2_debug( "%s", Error_msg );
      ret = Error_code;
    }
  }

  /* Close file handler */
  if( infile != NULL )
  {
    libL2_debug( "%s: fclose infile", fx );
    fclose( infile );
  }

  return ret;
}

/************************************************************************
 Description: Read compressed (*.Z) NCDC file.
 ************************************************************************/

static int Read_L2_file_compress()
{
  static char *fx = "Read_L2_file_compress";

  /* *.Z decompression is not yet supported */
  Error_code = LIBL2_UNCOMPRESS_FAILED;
  sprintf( Error_msg, "%s: compress format (*.Z) not supported (%d)", fx, Error_code );
  libL2_debug( "%s", Error_msg );
  return Error_code;
}

/************************************************************************
 Description: Parse the buffer of data read in from the level-II file.
     This is an initial parse to extract basic information.
 ************************************************************************/

static int Parse_L2_buf()
{
  static char *fx = "Parse_L2_buf";
  int ret = LIBL2_NO_ERROR;

  /* Make initial pass through L2_buf and make sure format
     is as expected */
  if( ( ret = Is_start_of_volume_file() ) != LIBL2_NO_ERROR )
  {
    /* Initial volume header format is invalid */
    libL2_debug( "%s: Is_start_of_volume_file returns %d", fx, ret );
    return ret;
  }
  else if( ( ret = Parse_meta_data() ) != LIBL2_NO_ERROR )
  {
    /* Meta data format is invalid */
    libL2_debug( "%s: Parse_meta_data returns %d", fx, ret );
    return ret;
  }
  else if( ( ret = Parse_base_data() ) != LIBL2_NO_ERROR )
  {
    /* Base data format is invalid */
    libL2_debug( "%s: Parse_base_data returns %d", fx, ret );
    return ret;
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Check for expected AR2 header to ensure the level-II file
     is in the expected format.
 ************************************************************************/

static int Is_start_of_volume_file()
{
  static char *fx = "Is_start_of_volume_file";
  int offset = 0;

  /* If this is the beginning of a level-II file, the first 4-bytes
     will be the beginning of the volume header. */

  if( L2_uncompress_size > VOLUME_HEADER_SIZE )
  {
    if( ! strncmp( (char *) &L2_buf[0], "AR2V", MAX_ICAO_LEN ) )
    {
      offset = VOLUME_HEADER_SIZE - MAX_ICAO_LEN;
      memcpy( L2_ICAO, &L2_buf[offset], MAX_ICAO_LEN );
      L2_ICAO[MAX_ICAO_LEN] = '\0';
      offset -= MAX_L2_TIME_LEN;
      memcpy( &L2_time, &L2_buf[offset], MAX_L2_TIME_LEN  );
      L2_time = INT_BSWAP_L( L2_time );
      offset -= MAX_L2_DATE_LEN;
      memcpy( &L2_date, &L2_buf[offset], MAX_L2_DATE_LEN );
      L2_date = INT_BSWAP_L( L2_date );
      libL2_debug( "%s: ICAO = %s", fx, &L2_ICAO[0] );
      libL2_debug( "%s: date = %d", fx, L2_date );
      libL2_debug( "%s: time = %d", fx, L2_time );
      return LIBL2_NO_ERROR;
    }
  }

  Error_code = LIBL2_NO_L2_HEADER;
  sprintf( Error_msg, "%s: Volume header not detected in level-II file (%d)", fx, Error_code );
  libL2_debug( "%s", Error_msg );
  return Error_code;
}

#define	RDA_ADAPT_NUM_SEGS	5
#define	RDA_ADAPT_MAX_LEN	RDA_ADAPT_NUM_SEGS*METADATA_MSG_SIZE
#define	RDA_CLUTTER_NUM_SEGS	77
#define	RDA_CLUTTER_MAX_LEN	RDA_CLUTTER_NUM_SEGS*METADATA_MSG_SIZE
#define	RDA_BYPASS_NUM_SEGS	49
#define	RDA_BYPASS_MAX_LEN	RDA_BYPASS_NUM_SEGS*METADATA_MSG_SIZE
#define	RDA_VCP_NUM_SEGS	1
#define	RDA_VCP_MAX_LEN		RDA_VCP_NUM_SEGS*METADATA_MSG_SIZE
#define	RDA_PMD_NUM_SEGS	1
#define	RDA_PMD_MAX_LEN		RDA_PMD_NUM_SEGS*METADATA_MSG_SIZE
#define	RDA_STATUS_NUM_SEGS	1
#define	RDA_STATUS_MAX_LEN	RDA_STATUS_NUM_SEGS*METADATA_MSG_SIZE
#define	NUM_METADATA_MSGS	( RDA_ADAPT_NUM_SEGS + RDA_CLUTTER_NUM_SEGS + RDA_BYPASS_NUM_SEGS + RDA_VCP_NUM_SEGS + RDA_PMD_NUM_SEGS + RDA_STATUS_NUM_SEGS )
#define	MAX_METADATA_SIZE	( NUM_METADATA_MSGS * METADATA_MSG_SIZE )

/************************************************************************
 Description: Parse the metadata section of the level-II file.
 ************************************************************************/

static int Parse_meta_data()
{
  static char *fx = "Parse_meta_data";
  int i = 0;
  short msg_size = 0;
  short type = 0;
  int   buf_offset = VOLUME_HEADER_SIZE;
  int   copy_offset = 0;
  int   copy_size = 0;
  int   number_of_msg_segments = 0;
  int   msg_segment_number = 0;
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
  libL2_msg_hdr_t* msg_header = NULL;

  /* Skip initial 12 bytes (comms manager header). */
  buf_offset += COMMS_HEADER_SIZE;

  /* Loop over metadata portion of L2_buf as it has a fixed size. */
  while( buf_offset < MAX_METADATA_SIZE )
  {
    msg_header = (libL2_msg_hdr_t*) &L2_buf[buf_offset];
    /* Byte-swap message header */
    UMC_RDAtoRPG_message_header_convert( (char *)msg_header );
    msg_size = msg_header->size * sizeof( unsigned short );
    type = msg_header->type;
    number_of_msg_segments = msg_header->num_segs;
    msg_segment_number = msg_header->seg_num;
    buf_offset += METADATA_MSG_SIZE;

    if( msg_segment_number != 1 )
    {
      /* If this is part of a multi-segmented message, skip
         the message header after the first message segment */
      copy_offset = sizeof( libL2_msg_hdr_t );
      copy_size = msg_size - sizeof( libL2_msg_hdr_t );
    }
    else
    {
      /* The first message segment is always used in full. */
      copy_offset = 0;
      copy_size = msg_size;
    }

    /* Fill individual buffers depending on message type */
    if( type == RDA_ADAPT_MSG )
    {
      libL2_debug( "%s: MSG 18: SIZE: %d INDEX: %d SEGMENT: %d OF %d", fx,
                   msg_size, msg18_index, msg_segment_number, number_of_msg_segments );
      memcpy( (char *)(msg18buf+msg18_index),
              (char *)msg_header+copy_offset, copy_size );
      msg18_index += copy_size;
      msg_header = (libL2_msg_hdr_t *)msg18buf;
      msg_header->size = msg18_index;
    }
    else if( type == RDA_CLUTTER_MSG )
    {
      libL2_debug( "%s: MSG 15: SIZE: %d INDEX: %d SEGMENT: %d OF %d", fx,
                   msg_size, msg15_index, msg_segment_number, number_of_msg_segments );
      libL2_debug( "%s: MSG15_INDEX = %d, COPY_SIZE = %d, COPY_OFFSET = %d",
                   fx, msg15_index, msg_size, copy_offset);
      memcpy( (char *)(msg15buf+msg15_index),
              (char *)msg_header+copy_offset, copy_size );
      msg15_index += copy_size;
      msg_header = (libL2_msg_hdr_t *)msg15buf;
      msg_header->size = msg15_index;
    }
    else if( type == RDA_BYPASS_MSG )
    {
      libL2_debug( "%s: MSG 13: SIZE: %d INDEX: %d SEGMENT: %d OF %d", fx,
                   msg_size, msg13_index, msg_segment_number, number_of_msg_segments );
      memcpy( (char *)(msg13buf+msg13_index),
              (char *)msg_header+copy_offset, copy_size );
      msg13_index += copy_size;
      msg_header = (libL2_msg_hdr_t *)msg13buf;
      msg_header->size = msg13_index;
    }
    else if( type == RDA_VCP_MSG )
    {
      libL2_debug( "%s: MSG 5: SIZE: %d INDEX: %d SEGMENT: %d OF %d", fx,
                   msg_size, msg5_index, msg_segment_number, number_of_msg_segments );
      memcpy( (char *)(msg5buf+msg5_index),
              (char *)msg_header+copy_offset, copy_size );
      msg5_index += copy_size;
      msg_header = (libL2_msg_hdr_t *)msg5buf;
      msg_header->size = msg5_index;
    }
    else if( type == RDA_PMD_MSG )
    {
      libL2_debug( "%s: MSG 3: SIZE: %d INDEX: %d SEGMENT: %d OF %d", fx,
                   msg_size, msg3_index, msg_segment_number, number_of_msg_segments );
      memcpy( (char *)(msg3buf+msg3_index),
              (char *)msg_header+copy_offset, copy_size );
      msg3_index += copy_size;
      msg_header = (libL2_msg_hdr_t *)msg3buf;
      msg_header->size = msg3_index;
    }
    else if( type == RDA_STATUS_MSG )
    {
      libL2_debug( "%s: MSG 2: SIZE: %d INDEX: %d SEGMENT: %d OF %d",
                fx, msg_size, msg2_index, msg_segment_number, number_of_msg_segments );
      memcpy( (char *)(msg2buf+msg2_index),
              (char *)msg_header+copy_offset, copy_size );
      msg2_index += copy_size;
      msg_header = (libL2_msg_hdr_t *)msg2buf;
      msg_header->size = msg2_index;
    }
    else if( type != 0 )
    {
      libL2_debug( "%s: Invalid type (%d)", fx, type );
    }
  }

  libL2_debug( "%s: MSG SIZES 18: %d 15: %d 13: %d 5: %d 3: %d 2: %d", fx,
               msg18_index, msg15_index, msg13_index,
               msg5_index, msg3_index, msg2_index );

  /* Byte-swap message memory */
  libL2_debug( "%s: byte swap RDA adapt msg18 1", fx );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_ADAPT_MSG, (char *)msg18buf );
  libL2_debug( "%s: byte swap RDA VCP msg18 2", fx );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)&msg18buf[1328] );
  libL2_debug( "%s: byte swap RDA VCP msg18 3", fx );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)&msg18buf[2500] );
  libL2_debug( "%s: byte swap RDA VCP msg18 4", fx );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)&msg18buf[3672] );
  libL2_debug( "%s: byte swap RDA VCP msg18 5", fx );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)&msg18buf[4844] );
  libL2_debug( "%s: byte swap RDA VCP msg18 6", fx );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)&msg18buf[6016] );
  libL2_debug( "%s: byte swap RDA VCP msg18 7", fx );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)&msg18buf[7188] );
  libL2_debug( "%s: byte swap RDA clutter msg15", fx );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_CLUTTER_MSG, (char *)msg15buf );
  libL2_debug( "%s: byte swap RDA bypass msg13", fx );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_BYPASS_MSG, (char *)msg13buf );
  libL2_debug( "%s: byte swap RDA VCP msg5", fx );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_VCP_MSG, (char *)msg5buf );
  libL2_debug( "%s: byte swap RDA PMD msg3", fx );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_PMD_MSG, (char *)msg3buf );
  libL2_debug( "%s: byte swap RDA STATUS msg2", fx );
  UMC_RDAtoRPG_message_convert_to_internal( RDA_STATUS_MSG, (char *)msg2buf );

  /* Copy byte-swapped message memory to static variable */
  memcpy( (char *) &L2_adapt[0], &msg18buf[sizeof(libL2_msg_hdr_t)], sizeof( libL2_adapt_t ) );
  if( Debug_flag ){ libL2_print_rda_adapt(); }

  memcpy( (char *) &L2_vcp[0], &msg5buf[sizeof(libL2_msg_hdr_t)], sizeof( libL2_vcp_t ) );
  L2_VCP = L2_vcp->vcp_msg_hdr.pattern_number;
  for( i = 0; i < L2_vcp->vcp_elev_data.number_cuts; i++ )
  {
    L2_elevations[i] = L2_vcp->vcp_elev_data.data[i].angle*BAMS_ELEV;
  }
  if( Debug_flag ){ libL2_print_rda_VCP(); }

  memcpy( (char *) &L2_pmd[0], &msg3buf[sizeof(libL2_msg_hdr_t)], sizeof( libL2_pmd_t ) );
  if( Debug_flag ){ libL2_print_rda_pmd(); }

  memcpy( (char *) &L2_clutter[0], &msg15buf[sizeof(libL2_msg_hdr_t)], sizeof( libL2_clutter_map_t ) );
  if( Debug_flag ){ libL2_print_rda_clutter_map( LIBL2_ALL_SEGMENTS, LIBL2_ALL_RADIALS ); }

  memcpy( (char *) &L2_bypass[0], &msg13buf[sizeof(libL2_msg_hdr_t)], sizeof( libL2_bypass_map_t ) );
  if( Debug_flag ){ libL2_print_rda_bypass_map( LIBL2_ALL_SEGMENTS, LIBL2_ALL_RADIALS ); }

  /* There can be multiple RDA status messages, so add to the list */
  Add_status_msg( &msg2buf[0] );

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: All data after the meta data is considered base data and
     will be initially parsed to find cut/radial/moment-specific
     information for this level-II file.
 ************************************************************************/

static int Parse_base_data()
{
  static char *fx = "Parse_base_data";
  short msg_size = 0;
  short type = 0;
  int   offset = VOLUME_HEADER_SIZE + MAX_METADATA_SIZE;
  char *ptr = NULL;
  libL2_msg_hdr_t* msg_header = NULL;

  /* Loop over base data portion of L2_buf */
  while( offset < L2_uncompress_size )
  {
    ptr = &L2_buf[offset+COMMS_HEADER_SIZE];
    msg_header = (libL2_msg_hdr_t*)ptr;
    UMC_RDAtoRPG_message_header_convert( (char *)msg_header );
    msg_size = msg_header->size*sizeof( unsigned short );
    type = msg_header->type;

    if( type == RDA_RADIAL_MSG )
    {
      /* Generic base data radial message */
      libL2_debug( "%s: MSG 31: SIZE: %d", fx, msg_size );
      Process_radial( offset, ptr );

      offset += ( msg_size+COMMS_HEADER_SIZE );
    }
    else if( type == RDA_STATUS_MSG )
    {
      /* RDA Status message */
      libL2_debug( "%s: MSG 2: SIZE: %d", fx, msg_size );
      UMC_RDAtoRPG_message_convert_to_internal( RDA_STATUS_MSG, ptr );
      offset += METADATA_MSG_SIZE;
      Add_status_msg( ptr );
    }
    else if( type != 0 )
    {
      /* Unknown message type */
      libL2_debug( "%s: invalid msg type %d", fx, type );
      return LIBL2_INVALID_MSG_TYPE;
    }
  }

  if( Debug_flag ){ libL2_print_rda_status_msgs(); }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Add RDA Status Message (Msg 2) to array of status messages
     for this level-II file.
 ************************************************************************/

static int Add_status_msg( char *status_msg )
{
  memcpy( &L2_status_msgs[L2_num_status_msgs][0], &status_msg[0], sizeof( libL2_status_t ) );
  L2_num_status_msgs++;

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: For the initial parsing of the level-II data, each radial
     message is passed to this function to extract radial-specific
     information such as number of base moments, list of base moments, etc.
 ************************************************************************/

static void Process_radial( int offset, char *buf )
{
  int i = 0;
  char *ptr = NULL;
  libL2_base_t *bd = (libL2_base_t *) buf;
  libL2_base_hdr_t *bdh = &bd->base;
  libL2_any_t *dblk = NULL;
  char temp_buf[5];

  /* Byte-swap radial message memory */
  UMC_RDAtoRPG_message_convert_to_internal( RDA_RADIAL_MSG, (char *)buf );

  memset( temp_buf, 0, 5 );
  temp_buf[MAX_ICAO_LEN] = '\0';

  if( bdh->status == RS_BEG_ELEV || bdh->status == RS_BEG_VOL ||
      bdh->status == RS_BEG_ELEV_LAST_CUT )
  {
    /* Account for a start of cut/volume */
    L2_num_cuts++;
    Get_radial_info( buf );
    L2_elevation_offsets[bdh->elev_num-1] = offset;
  }
  else if( bdh->status == RS_END_ELEV || bdh->status == RS_END_VOL ||
           bdh->status == RS_PSEUDO_END_ELEV ||
           bdh->status == RS_PSEUDO_END_VOL )
  {
    /* Account for an end of cut/volume */
    L2_num_azimuths[bdh->elev_num-1] = bdh->azi_num;
  }

  /* Get the azimuth of the radial */
  L2_azimuths[bdh->elev_num-1][bdh->azi_num-1] = bdh->azimuth;

  /* Pick apart the radial and save off the data blocks and base data */
  for( i = 0; i < bdh->no_of_datum; i++ )
  {
    ptr = (char *)(buf+sizeof( libL2_msg_hdr_t )+bdh->data[i] );
    dblk = (libL2_any_t *)ptr;

    memcpy( temp_buf, dblk->name, sizeof( dblk->name ) );

    if( strstr( temp_buf, "RVOL" ) != NULL )
    {
      memcpy( &L2_vol[bdh->elev_num-1][bdh->azi_num-1], dblk, sizeof( libL2_RVOL_t ) );
    }
    else if( strstr( temp_buf, "RELV" ) != NULL )
    {
      memcpy( &L2_elv[bdh->elev_num-1][bdh->azi_num-1], dblk, sizeof( libL2_RELV_t ) );
    }
    else if( strstr( temp_buf, "RRAD" ) != NULL )
    {
      memcpy( &L2_rad[bdh->elev_num-1][bdh->azi_num-1], dblk, sizeof( libL2_RRAD_t ) );
    }
  }

}

/************************************************************************
 Description: For the first radial of each cut, extract cut-specific
     information such as number of base moments, list of base moments, etc.
 ************************************************************************/

/* Half degree resolution is "1". One degree resolution is "2".
   Scale by 5 now to make the resolution integer. When ready to
   use and convert to a float, divide by 10.0 */
 
#define	AZ_RES_ENCODE	5

static void Get_radial_info( char *buf )
{
  char *ptr = NULL;
  libL2_base_t *bd = (libL2_base_t *) buf;
  libL2_base_hdr_t *bdh = &bd->base;
  libL2_moment_t *g = NULL;
  char temp_buf[5];
  int num_moments = 0;
  int moment_bitmask = EMPTY_BITMASK;
  int i = 0;
  int j = 0;
  int ar = 0;
  int el_ind = 0;
  int ng = 0;
  int bs = 0;
  int fgr = 0;
  int mom_ind = -1;

  temp_buf[MAX_ICAO_LEN] = '\0';
  el_ind = bdh->elev_num - 1;
  ar = bdh->azimuth_res * AZ_RES_ENCODE;

  for( i = 0; i < bdh->no_of_datum; i++ )
  {
    ptr = (char *)(buf+sizeof( libL2_msg_hdr_t )+bdh->data[i] );
    g = (libL2_moment_t *)ptr;
    ng = g->no_of_gates;
    bs = g->bin_size;
    fgr = g->first_gate_range;

    memcpy( temp_buf, g->name, sizeof( g->name ) );
    if( temp_buf[0] == 'R' )
    {
      continue;
    }
    else if( strcmp( temp_buf, Get_moment_index_string( LIBL2_REF_INDEX ) ) == 0 )
    {
      mom_ind = LIBL2_REF_INDEX;
      num_moments++;
      moment_bitmask = moment_bitmask | REF_BITMASK;
    }
    else if( strcmp( temp_buf, Get_moment_index_string( LIBL2_VEL_INDEX ) ) == 0 )
    {
      mom_ind = LIBL2_VEL_INDEX;
      num_moments++;
      moment_bitmask = moment_bitmask | VEL_BITMASK;
    }
    else if( strcmp( temp_buf, Get_moment_index_string( LIBL2_SPW_INDEX ) ) == 0 )
    {
      mom_ind = LIBL2_SPW_INDEX;
      num_moments++;
      moment_bitmask = moment_bitmask | SPW_BITMASK;
    }
    else if( strcmp( temp_buf,  Get_moment_index_string( LIBL2_ZDR_INDEX ) ) == 0 )
    {
      mom_ind = LIBL2_ZDR_INDEX;
      num_moments++;
      moment_bitmask = moment_bitmask | ZDR_BITMASK;
    }
    else if( strcmp( temp_buf, Get_moment_index_string( LIBL2_PHI_INDEX ) ) == 0 )
    {
      mom_ind = LIBL2_PHI_INDEX;
      num_moments++;
      moment_bitmask = moment_bitmask | PHI_BITMASK;
    }
    else if( strcmp( temp_buf, Get_moment_index_string( LIBL2_RHO_INDEX ) ) == 0 )
    {
      mom_ind = LIBL2_RHO_INDEX;
      num_moments++;
      moment_bitmask = moment_bitmask | RHO_BITMASK;
    }

    L2_azimuth_resolutions[el_ind][mom_ind] = ar;
    L2_gate_resolutions[el_ind][mom_ind] = bs;
    L2_num_gates[el_ind][mom_ind] = ng;
    for( j = 0; j < ng; j++ )
    {
      L2_gates[el_ind][mom_ind][j] = j*bs + fgr;
    }
  }

  L2_num_moments[el_ind] = num_moments;
  L2_moments[el_ind] = moment_bitmask;
}

/************************************************************************
 Description: Extract base data from level-II file. It's named "Re"parse
     because this is the second time the level-II data is being parsed.
     The first time was to get basic information, the second time is to
     actually extract base data.
 ************************************************************************/

static int Reparse_base_data( int cut_index, int moment_index, int radial_number, float *arr )
{
  short msg_size = 0;
  short type = 0;
  int   offset = L2_elevation_offsets[cut_index];
  char *ptr = NULL;
  int i = 0;
  int j = 0;
  int pos = 0;
  int mi = 0;
  libL2_msg_hdr_t* msg_header = NULL;
  libL2_base_t *bd = NULL;
  libL2_base_hdr_t *bdh = NULL;
  libL2_moment_t *g = NULL;
  char temp_buf[5];
 
  temp_buf[4] = '\0';

  if( cut_index == LIBL2_ALL_CUTS )
  {
    offset = L2_elevation_offsets[0];
  }
  else
  {
    offset = L2_elevation_offsets[cut_index];
  }

  /* Loop over base data and extract whatever is needed */
  while( offset < L2_uncompress_size )
  {
    ptr = &L2_buf[offset+COMMS_HEADER_SIZE];
    msg_header = (libL2_msg_hdr_t*)ptr;
    msg_size = msg_header->size*sizeof( unsigned short );
    type = msg_header->type;
    if( type == RDA_RADIAL_MSG )
    {
      offset += ( msg_size + COMMS_HEADER_SIZE);
      memset( temp_buf, 0, 4 );
      bd = (libL2_base_t *) ptr;
      bdh = &bd->base;
      pos = 0;
      if( cut_index == LIBL2_ALL_CUTS || bdh->elev_num-1 == cut_index )
      {
        if( radial_number == LIBL2_ALL_RADIALS || bdh->azi_num == radial_number )
        {
          for( i = 0; i < bdh->no_of_datum; i++ )
          {
            g = (libL2_moment_t *)(ptr+sizeof(libL2_msg_hdr_t)+bdh->data[i]);
            memcpy( temp_buf, g->name, sizeof( g->name ) );
            if( ( mi = Get_moment_index_from_string( temp_buf ) ) < 0 )
            {
              continue;
            }
            else if(  moment_index == LIBL2_ALL_MOMENTS ||
                      mi == moment_index )
            {
              if( g->data_word_size == 8 )
              {
                for( j = 0; j < g->no_of_gates; j++ )
                {
                  if( g->gate.b[j] == 0 )
                  {
                    arr[pos++] = (float) LIBL2_NO_DATA;
                  }
                  else if( g->gate.b[j] == 1 )
                  {
                    arr[pos++] = (float) LIBL2_RF_DATA;
                  }
                  else
                  {
                    arr[pos++] = ( (float) g->gate.b[j] - g->offset ) / g->scale;
                  }
                }
              }
              else if( g->data_word_size == 16 )
              {
                for( j = 0; j < g->no_of_gates; j++ )
                {
                  if( g->gate.u_s[j] == 0 )
                  {
                    arr[pos++] = (float) LIBL2_NO_DATA;
                  }
                  else if( g->gate.u_s[j] == 1 )
                  {
                    arr[pos++] = (float) LIBL2_RF_DATA;
                  }
                  else
                  {
                    arr[pos++] = ( (float) g->gate.u_s[j] - g->offset ) / g->scale;
                  }
                }
              }
              else if( g->data_word_size == 32 && g->scale != 0.0 )
              {
                for( j = 0; j < g->no_of_gates; j++ )
                {
                  if( g->gate.u_i[j] == 0 )
                  {
                    arr[pos++] = (float) LIBL2_NO_DATA;
                  }
                  else if( g->gate.u_i[j] == 1 )
                  {
                    arr[pos++] = (float) LIBL2_RF_DATA;
                  }
                  else
                  {
                    arr[pos++] = ( (float) g->gate.u_i[j] - g->offset ) / g->scale;
                  }
                }
              }
              else
              {
                for( j = 0; j < g->no_of_gates; j++ )
                {
                  if( g->gate.f[j] == 0 )
                  {
                    arr[pos++] = (float) LIBL2_NO_DATA;
                  }
                  else if( g->gate.f[j] == 1 )
                  {
                    arr[pos++] = (float) LIBL2_RF_DATA;
                  }
                  else
                  {
                    arr[pos++] = ( (float) g->gate.f[j] - g->offset ) / g->scale;
                  }
                }
              }
            }
          }
        }
      }
    }
    else if( type == RDA_STATUS_MSG )
    {
      /* Skip past RDA Status messages */
      offset += METADATA_MSG_SIZE;
    }
    else
    {
      /* Unknown message type */
      return LIBL2_INVALID_MSG_TYPE;
    }
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Convert  moment index to a string.
 ************************************************************************/

static char *Get_moment_index_string( int moment_index )
{
  static char *fx = "Get_moment_index_string";
  static char moment_string[5];
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_moment_index( moment_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_moment_index returns %d", fx, ret );
    return NULL;
  }
  else if( moment_index == LIBL2_ALL_MOMENTS )
  {
    Error_code = LIBL2_BAD_MOMENT_INDEX;
    sprintf( Error_msg, "%s: All moments option is invalid (%d)", fx, Error_code );
    libL2_debug( "%s", Error_msg );
    return NULL;
  }

  /* Convert moment_index to a string */
  switch( moment_index )
  {
    case LIBL2_REF_INDEX:
      strcpy( moment_string, "DREF" );
      break;
    case LIBL2_VEL_INDEX:
      strcpy( moment_string, "DVEL" );
      break;
    case LIBL2_SPW_INDEX:
      strcpy( moment_string, "DSW " );
      break;
    case LIBL2_ZDR_INDEX:
      strcpy( moment_string, "DZDR" );
      break;
    case LIBL2_PHI_INDEX:
      strcpy( moment_string, "DPHI" );
      break;
    case LIBL2_RHO_INDEX:
      strcpy( moment_string, "DRHO" );
      break;
    default:
      strcpy( moment_string, "????" );
  }

  libL2_debug( "%s: returning %s", fx, &moment_string[0] );

  return &moment_string[0];
}

/************************************************************************
 Description: Print miscellaneous information of the file.
 ************************************************************************/

int libL2_print_misc()
{
  int i = 0;

  printf( "FILE:            %s\n", libL2_filetype_string() );
  printf( "ICAO:            %s\n", libL2_ICAO() );
  printf( "DATE/TIME:       %s %s\n", libL2_date(), libL2_time() );
  printf( "LATITUDE:        %f\n", libL2_latitude() );
  printf( "LONGITUDE:       %f\n", libL2_longitude() );
  printf( "HEIGHT:          %d (m)\n", libL2_height() );
  printf( "VCP:             %d\n", libL2_VCP() );
  printf( "NUM CUTS:        %d\n", libL2_num_cuts() );
  printf( "COMP RATIO:      %f\n", libL2_compression_ratio() );
  printf( "NUM SEGMENTS:    %d\n", libL2_num_segments() );

  for( i = 0; i < libL2_num_cuts(); i++ )
  {
    printf( "ELEV[%2d]: %5.2f  NUM AZS: %3d  NUM MOMENTS: %2d - %s\n", i, (libL2_elevations())[i], libL2_num_azimuths(i), libL2_num_moments(i), Get_moment_bitmask_string( Get_moments(i) ) );
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print RDA Adaptation message.
 ************************************************************************/

int libL2_print_rda_adapt()
{
  int i = 0;

  /* orda_adpt.h                         */
  /* typedef struct {                    */
  /* RDA_RPG_message_header_t msg_hdr;   */
  /* ORDA_adpt_data_t         rda_adapt; */
  /* } ORDA_adpt_data_msg_t;             */
  /* ORDA_adpt_data_t -> L2_adapt        */

  printf( "RDA ADAPT MSG (MSG TYPE 18)\n" );
  printf( "  ADPT FILE NAME:                               %s\n", L2_adapt->adap_file_name );
  printf( "  ADPT FORMAT:                                  %s\n", L2_adapt->adap_format );
  printf( "  ADPT REVISION:                                %s\n", L2_adapt->adap_revision );
  printf( "  ADPT DATE:                                    %s\n", L2_adapt->adap_date );
  printf( "  ADPT TIME:                                    %s\n", L2_adapt->adap_time );
  printf( "  AZ POS GAIN FACTOR:                           %4.2f\n", L2_adapt->k1 );
  printf( "  LATENCY OF DCU AZ MEASURE:                    %6.4f sec\n", L2_adapt->az_lat );
  printf( "  ELEV POS GAIN FACTOR:                         %4.2f\n", L2_adapt->k3 );
  printf( "  LATENCY OF DCU ELEV MEASURE:                  %6.4f sec\n", L2_adapt->el_lat );
  printf( "  PED PARK POS AZ:                              %6.2f deg\n", L2_adapt->parkaz );
  printf( "  PED PARK POS ELEV:                            %5.2f deg\n", L2_adapt->parkel );
  for( i = 0; i < 11; i++ )
  {
    printf( "  FUEL LEVEL %2d:                                %5.1f %%\n", i, L2_adapt->a_fuel_conv[i] );
  }
  printf( "  EQUIP ALARM MIN TEMP:                         %4.1f C\n", L2_adapt->a_min_shelter_temp );
  printf( "  EQUIP ALARM MAX TEMP:                         %4.1f C\n", L2_adapt->a_max_shelter_temp );
  printf( "  MIN A/C DISCHARGE AIR TEMP DIFF:              %4.1f C\n", L2_adapt->a_min_shelter_ac_temp_diff );
  printf( "  XMTR LEAVING AIR ALARM MAX TEMP:              %4.1f C\n", L2_adapt->a_max_xmtr_air_temp );
  printf( "  RADOME ALARM MAX TEMP:                        %4.1f C\n", L2_adapt->a_max_rad_temp );
  printf( "  MAX RADOME MINUS EXT TEMP DIFF:               %4.1f C\n", L2_adapt->a_max_rad_temp_rise );
  printf( "  PED 28V PS TOLERANCE:                         %4.1f %%\n", L2_adapt->ped_28V_reg_lim );
  printf( "  PED 5V PS TOLERANCE:                          %4.1f %%\n", L2_adapt->ped_5V_reg_lim );
  printf( "  PED 15V PS TOLERANCE:                         %4.1f %%\n", L2_adapt->ped_15V_reg_lim );
  printf( "  GEN SHELTER ALARM MIN TEMP:                   %4.1f C\n", L2_adapt->a_min_gen_room_temp );
  printf( "  GEN SHELTER ALARM MAX TEMP:                   %4.1f C\n", L2_adapt->a_max_gen_room_temp );
  printf( "  DAU 5V PS TOLERANCE:                          %4.1f %%\n", L2_adapt->dau_5V_reg_lim );
  printf( "  DAU 15V PS TOLERANCE:                         %4.1f %%\n", L2_adapt->dau_15V_reg_lim );
  printf( "  DAU 28V PS TOLERANCE:                         %4.1f %%\n", L2_adapt->dau_28V_reg_lim );
  printf( "  ENCODER 5V PS TOLERANCE:                      %5.2f %%\n", L2_adapt->en_5V_reg_lim );
  printf( "  ENCODER 5V PS NOMINAL VOLTAGE:                %4.2f V\n", L2_adapt->en_5V_nom_volts );
  printf( "  RPG CO-LOCATED:                               %s\n", L2_adapt->rpg_co_located );
  printf( "  XMTR SPEC FILTER INSTALLED:                   %s\n", L2_adapt->spec_filter_installed );
  printf( "  TPS INSTALLED:                                %s\n", L2_adapt->tps_installed );
  printf( "  RMS INSTALLED:                                %s\n", L2_adapt->rms_installed );
  printf( "  PERF TEST INTERVAL:                           %2d hr\n", L2_adapt->a_hvdl_tst_int );
  printf( "  RPG LOOP TEST INTERVAL:                       %2d min\n", L2_adapt->a_rpg_lt_int );
  printf( "  REQD INTERVAL STABLE UTIL PWR:                %2d min\n", L2_adapt->a_min_stab_util_pwr_time );
  printf( "  MAX GENERATOR AUTO EXER INTERVAL:             %3d hr\n", L2_adapt->a_gen_auto_exer_interval );
  printf( "  REC SWITCH TO UTIL PWR TIME INTERVAL:         %2d min\n", L2_adapt->a_util_pwr_sw_req_interval );
  printf( "  LOW FUEL LEVEL WARNING:                       %5.1f %%\n", L2_adapt->a_low_fuel_level );
  printf( "  CONFIG CHANNEL NUMBER:                        %1d\n", L2_adapt->config_chan_number );
/*
  printf( "  SPARE-220:                                    %d\n", L2_adapt->spare_220 );
*/
  i = L2_adapt->redundant_chan_config;
  if( i == 1 )
  {
    printf( "  RED CHANNEL CONFIG:                           SINGLE (%d)\n", i );
  }
  else if( i == 2 )
  {
    printf( "  RED CHANNEL CONFIG:                           FAA (%d)\n", i );
  }
  else if( i == 3 )
  {
    printf( "  RED CHANNEL CONFIG:                           NWS-R (%d)\n", i );
  }
  else
  {
    printf( "  RED CHANNEL CONFIG:                           UNKNOWN (%d)\n", i );
  }

  for( i = 0; i < 104; i++ )
  {
    printf( "  ATTEN TABLE-%3d:                              %7.2f dB\n", i, L2_adapt->atten_table[i] );
  }
  for( i = 0; i < 69; i++ )
  {
    printf( "  PATH LOSS-%2d:                                 %6.2f dB\n", i, L2_adapt->path_losses[i] );
  }
  printf( "  NONCON CHANNEL CALIB DIFF:                    %4.2f dB\n", L2_adapt->chan_cal_diff );
  printf( "  SPARE PATH LOSSES:                            %f\n", L2_adapt->path_losses_70_71 );
  printf( "  LOG AMP FACTOR1:                              %6.4f V/dBm\n", L2_adapt->log_amp_factor[0] );
  printf( "  LOG AMP FACTOR2:                              %6.4f V\n", L2_adapt->log_amp_factor[1] );
  printf( "  AME VERT TEST SIG PWR:                        %5.2f dBm\n", L2_adapt->v_ts_cw );
  for( i = 0; i < 13; i++ )
  {
    printf( "  HORIZ RECV NOISE NORMAL-%2d:                   %5.3f\n", i, L2_adapt->rnscale[i] );
  }
  for( i = 0; i < 13; i++ )
  {
    printf( "  TWO-WAY ATMOS LOSS-%2d:                        %7.4f dB/km\n", i, L2_adapt->atmos[i] );
  }
  for( i = 0; i < 12; i++ )
  {
    printf( "  BYP MAP ELEV ANGLE-%2d:                        %6.3f deg\n", i, L2_adapt->el_index[i] );
  }
  printf( "  XMTR FREQUENCY:                               %4d Mhz\n", L2_adapt->tfreq_mhz );
  printf( "  PT CLUTTER SUPP THRESHOLD:                    %4.1f dB\n", L2_adapt->base_data_tcn );
  printf( "  RANGE UNFOLD OVERLAY THRESHOLD:               %4.1f dB\n", L2_adapt->refl_data_tover );
  printf( "  HORIZ TARGET SYS CALIB LONG PULSE:            %6.2f dBZ\n", L2_adapt->tar_h_dbz0_inc_lp );
  printf( "  VERT TARGET SYS CALIB LONG PULSE:             %6.2f dBZ\n", L2_adapt->tar_v_dbz0_inc_lp );
  printf( "  PHI CLUTTER TARGE AZ:                         %5.1f deg\n", L2_adapt->phi_clutter_az );
  printf( "  PHI CLUTTER TARGE ELEV:                       %5.1f deg\n", L2_adapt->phi_clutter_el );
  printf( "  MATCHED FILTER LOSS LONG PULSE:               %5.2f dB\n", L2_adapt->lx_lp );
  printf( "  MATCHED FILTER LOSS SHORT PULSE:              %5.2f dB\n", L2_adapt->lx_sp );
  printf( "  HYDRO REFRACT FACTOR:                         %4.2f\n", L2_adapt->meteor_param );
  printf( "  ANTENNA BEAMWIDTH:                            %4.2f deg\n", L2_adapt->beamwidth );
  printf( "  ANTENNA GAIN (INCL RADOME):                   %5.2f dB\n", L2_adapt->antenna_gain );
  printf( "  USE COHO THRESHOLDS:                          %d\n", L2_adapt->use_coho_thresholding );
  printf( "  VEL CHECK DELTA MAINT LIMIT:                  %3.1f m/s\n", L2_adapt->vel_maint_limit );
  printf( "  SPW CHECK DELTA MAINT LIMIT:                  %3.1f m/s\n", L2_adapt->wth_maint_limit );
  printf( "  VEL CHECK DELTA DEGRADE LIMIT:                %3.1f m/s\n", L2_adapt->vel_degrad_limit );
  printf( "  SPW CHECK DELTA DEGRADE LIMIT:                %3.1f m/s\n", L2_adapt->wth_degrad_limit );
  printf( "  HORIZ SYS NOISE TEMP DEGRADE LIMIT:           %6.1f K\n", L2_adapt->h_noisetemp_dgrad_limit );
  printf( "  HORIZ SYS NOISE TEMP MAINT LIMIT:             %6.1f K\n", L2_adapt->h_noisetemp_maint_limit );
  printf( "  VERT SYS NOISE TEMP DEGRADE LIMIT:            %6.1f K\n", L2_adapt->v_noisetemp_dgrad_limit );
  printf( "  VERT SYS NOISE TEMP MAINT LIMIT:              %6.1f K\n", L2_adapt->v_noisetemp_maint_limit );
  printf( "  KLYSTRON OUTPUT TARGET CONSIS DEGRADE LIMIT:  %4.1f dB\n", L2_adapt->kly_degrade_limit );
  printf( "  COHO PWR AT A1J4:                             %5.2f dBm\n", L2_adapt->ts_coho );
  printf( "  AME HORIZ TEST SIG PWR:                       %5.2f dBm\n", L2_adapt->ts_cw );
  printf( "  RF DRIVE TEST SIG SHORT PULSE:                %5.2f dBm\n", L2_adapt->ts_rf_sp );
  printf( "  RF DRIVE TEST SIG LONG PULSE:                 %5.2f dBm\n", L2_adapt->ts_rf_lp );
  printf( "  STALO PWR AT A1J2:                            %5.2f dBm\n", L2_adapt->ts_stalo );
  printf( "  RF NOISE TEST SIG EXCESS NOISE RATIO:         %5.2f dB\n", L2_adapt->ts_noise );
  printf( "  XMTR PEAK PWR ALARM MAX LEVEL:                %6.2f kW\n", L2_adapt->xmtr_peak_power_high_limit );
  printf( "  XMTR PEAK PWR ALARM MIN LEVEL:                %6.2f kW\n", L2_adapt->xmtr_peak_power_low_limit );
  printf( "  COMPUTE/TARGET HORIZ ddBZ0 DIFF LIMIT:        %4.1f dB\n", L2_adapt->h_dbz0_delta_limit );
  printf( "  BYP MAP NOISE THRESHOLD:                      %4.2f dB\n", L2_adapt->threshold1 );
  printf( "  BYP MAP REJECT RATIO THRESHOLD:               %4.2f dB\n", L2_adapt->threshold2 );
  printf( "  HORIZ CLUTT SUPP DEGRADE LIMIT:               %4.1f dB\n", L2_adapt->h_clut_supp_dgrad_lim );
  printf( "  HORIZ CLUTT SUPP MAINT LIMIT:                 %4.1f dB\n", L2_adapt->h_clut_supp_maint_lim );
  printf( "  TRUE RANGE AT FIRST RANGE BIN:                %5.3f km\n", L2_adapt->range0_value );
  printf( "  XMTR PWR BYTE DATA TO WATTS SCLAE FACTOR:     %9.7f W\n", L2_adapt->xmtr_pwr_mtr_scale );
  printf( "  COMPUTE/TARGET VERT ddBZ0 DIFF LIMIT:         %4.1f dB\n", L2_adapt->v_dbz0_delta_limit );
  printf( "  HORIZ TARGET SYS CALIB SHORT PULSE:           %6.2f dB\n", L2_adapt->tar_h_dbz0_sp );
  printf( "  VERT TARGET SYS CALIB SHORT PULSE:            %6.2f dB\n", L2_adapt->tar_v_dbz0_sp );
  printf( "  SITE PRF SET:                                 %1d\n", L2_adapt->deltaprf );
/*
  printf( "  SPARE-1256:                                   %d\n", L2_adapt->spare1256 );
  printf( "  SPARE-1260:                                   %d\n", L2_adapt->spare1260 );
*/
  printf( "  XMTR OUTPUT PULSE WIDTH SHORT PULSE:          %4d nsec\n", L2_adapt->tau_sp );
  printf( "  XMTR OUTPUT PULSE WIDTH LONG PULSE:           %4d nsec\n", L2_adapt->tau_lp );
  printf( "  NUM 1/4km CORRUPT BINS AT SWEEP END:          %2d bins\n", L2_adapt->nc_dead_value );
  printf( "  RF DRIVE PULSE WIDTH SHORT PULSE:             %4d nsec\n", L2_adapt->tau_rf_sp );
  printf( "  RF DRIVE PULSE WIDTH LONG PULSE:              %4d nsec\n", L2_adapt->tau_rf_lp );
  printf( "  CLUTTER MAP ELEV BNDRY SEG 1/2:               %4.2f deg\n", L2_adapt->seg1lim );
  printf( "  SITE LAT SECS:                                %7.4f sec\n", L2_adapt->slatsec );
  printf( "  SITE LONG SECS:                               %7.4f sec\n", L2_adapt->slonsec );
  printf( "  PHI CLUTTER RANGE:                            %d\n", L2_adapt->phi_clutter_range );
  printf( "  SITE LAT DEGS:                                %2d deg\n", L2_adapt->slatdeg );
  printf( "  SITE LAT MINS:                                %2d min\n", L2_adapt->slatmin );
  printf( "  SITE LONG DEGS:                               %3d deg\n", L2_adapt->slongdeg );
  printf( "  SITE LONG MINS:                               %2d min\n", L2_adapt->slongmin );
  printf( "  SITE LAT DIR:                                 %s\n", L2_adapt->slatdir );
  printf( "  SITE LONG DIR:                                %s\n", L2_adapt->slondir );
  printf( "  INDEX TO CURRENT VCP:                         %d\n", L2_adapt->vc_ndx );

/*
  printf( "  :  %s", L2_adapt->vcpat11 );
  printf( "  :  %s", L2_adapt->vcpat21 );
  printf( "  :  %s", L2_adapt->vcpat31 );
  printf( "  :  %s", L2_adapt->vcpat32 );
  printf( "  :  %s", L2_adapt->vcpat300 );
  printf( "  :  %s", L2_adapt->vcpat301 );
*/

  printf( "  AZ BORESIGHT CORR FACTOR:                     %6.3f deg\n", L2_adapt->az_correction_factor );
  printf( "  ELEV BORESIGHT CORR FACTOR:                   %6.3f deg\n", L2_adapt->el_correction_factor );
  printf( "  ICAO:                                         %c%c%c%c\n", L2_adapt->site_name[0], L2_adapt->site_name[1], L2_adapt->site_name[2], L2_adapt->site_name[3] );
  printf( "  ELEV ANGLE MIN:                               %9.5d deg\n", L2_adapt->ant_manual_setup_ielmin );
  printf( "  ELEV ANGLE MAX:                               %9.5d deg\n", L2_adapt->ant_manual_setup_ielmax );
  printf( "  AZ VELOCITY MAX:                              %3d deg/s\n", L2_adapt->ant_manual_setup_fazvelmax );
  printf( "  ELEV VELOCITY MAX:                            %2d deg/s\n", L2_adapt->ant_manual_setup_felvelmax );
  printf( "  SITE GROUND HEIGHT:                           %5d m (ASL)\n", L2_adapt->ant_manual_setup_ignd_hgt );
  printf( "  SITE RADAR HEIGHT:                            %4d m (ASL)\n", L2_adapt->ant_manual_setup_irad_hgt );
  for( i = 0; i < 75; i++ )
  {
/*
    printf( "  SPARE-8496[%02d]:                               %d\n", i, L2_adapt->spare8496[i] );
*/
  }
  printf( "  WAVEGUIDE LENGTH:                             %4d\n", L2_adapt->rvp8NV_iwaveguide_length );
  for( i = 0; i < 11; i++ )
  {
    printf( "  VERT REC NOISE SCALE-%2d:                      %f\n", i, L2_adapt->v_rnscale[i] );
  }
  printf( "  VEL UNFOLDING OVERLAY THRESHOLD:              %4.1f dB\n", L2_adapt->vel_data_tover );
  printf( "  SPW UNFOLDING OVERLAY THRESHOLD:              %4.1f dB\n", L2_adapt->width_data_tover );
  for( i = 0; i < 3; i++ )
  {
/*
    printf( "  SPARE-8752[%1d]:                                %d\n", i, L2_adapt->spare_8752[i] );
*/
  }
  printf( "  START RANGE FOR FIRST DOPPLER RADIAL:         %7.3f km\n", L2_adapt->doppler_range_start );
  printf( "  MAX INDEX FOR ELEV INDEX PARAMS:              %2d\n", L2_adapt->max_el_index );
  printf( "  CLUTTER MAP ELEV BNDRY SEG 2/3:               %4.2f deg\n", L2_adapt->seg2lim );
  printf( "  CLUTTER MAP ELEV BNDRY SEG 3/4:               %4.2f deg\n", L2_adapt->seg3lim );
  printf( "  CLUTTER MAP ELEV BNDRY SEG 4/5:               %4.2f deg\n", L2_adapt->seg4lim );
  printf( "  NUM ELEV SEGS IN CLUTTER MAP:                 %1d\n", L2_adapt->nbr_el_segments );
  printf( "  HORIZ RECV NOISE LONG PULSE:                  %5.1f dBm\n", L2_adapt->h_noise_long );
  printf( "  ANTENNA NOISE TEMP:                           %5.1f K\n", L2_adapt->ant_noise_temp );
  printf( "  HORIZ RECV NOISE SHORT PULSE:                 %5.1f dBm\n", L2_adapt->h_noise_short );
  printf( "  HORIZ RECV NOISE TOLERANCE:                   %3.1f dB\n", L2_adapt->h_noise_tolerance );
  printf( "  HORIZ DYNAMIC RANGE MIN:                      %4.1f dB\n", L2_adapt->h_min_dyn_range );
  printf( "  AUX GENERATOR INSTALLED:                      %s\n", L2_adapt->gen_installed );
  printf( "  AUX GENERATOR AUTO EXER ENAB:                 %s\n", L2_adapt->gen_exercies );
  printf( "  VERT RECV NOISE TOLERANCE:                    %3.1f dB\n", L2_adapt->v_noise_tolerance );
  printf( "  VERT DYNAMIC RANGE MIN:                       %4.1f dB\n", L2_adapt->v_min_dyn_range );
  printf( "  ZDR BIAS DEGRADE LIMIT:                       %4.1f dB\n", L2_adapt->zdr_bias_degraded_lim );
  for( i = 0; i < 4; i++ )
  {
/*
    printf( "  SPARE-8836[%1d]:                                %d\n", i, L2_adapt->spare8836[i] );
*/
  }
  printf( "  VERT RECV NOISE LONG PULSE:                   %5.1f dBm\n", L2_adapt->v_noise_long );
  printf( "  VERT RECV NOISE SHORT PULSE:                  %f5.1 dBm\n", L2_adapt->v_noise_short );
  printf( "  ZDR UNFOLDING OVERLAY THRESHOLD:              %6.2f dB\n", L2_adapt->zdr_data_tover );
  printf( "  PHI UNFOLDING OVERLAY THRESHOLD:              %6.2f dB\n", L2_adapt->phi_data_tover );
  printf( "  RHO UNFOLDING OVERLAY THRESHOLD:              %6.2f dB\n", L2_adapt->rho_data_tover );
  printf( "  STALO PWR DEGRADE LIMIT:                      %4.2f V\n", L2_adapt->stalo_pwr_dgrad_lim );
  printf( "  STALO PWR MAINT LIMIT:                        %4.2f V\n", L2_adapt->stalo_pwr_maint_lim );
  printf( "  HORIZ PWR SENSE MIN:                          %5.2f dBm\n", L2_adapt->min_h_pwr_sense );
  printf( "  VERT PWR SENSE MIN:                           %5.2f dBm\n", L2_adapt->min_v_pwr_sense );
  printf( "  HORIZ PWR SENSE CALIB OFFSET:                 %7.2f dB\n", L2_adapt->min_h_pwr_sense_off );
  printf( "  VERT PWR SENSE CALIB OFFSET:                  %f7.2 dB\n", L2_adapt->min_v_pwr_sense_off );
/*
  printf( "  SPARE-8888:                                   %d\n", L2_adapt->spare_8888 );
*/
  printf( "  RF PALLET BROADBAND LOSS:                     %6.2f dB\n", L2_adapt->rf_pallet_bb_loss );
  printf( "  ZDR CHECK FAILURE THRESHOLD:                  %5.2f dB\n", L2_adapt->zdr_check_thr );
  printf( "  PHI CHECK FAILURE THRESHOLD:                  %5.2f deg\n", L2_adapt->phi_check_thr );
  printf( "  RHO CHECK FAILURE THRESHOLD:                  %4.2f\n", L2_adapt->rho_check_thr );
  for( i = 0; i < 13; i++ )
  {
/*
    printf( "  SPARE-8808[%02d]:                               %d\n", i, L2_adapt->spare_8808[i] );
*/
  }
  printf( "  AME PS TOLERANCE:                             %4.1f %%\n", L2_adapt->ame_ps_tolerance );
  printf( "  MAX AME INT ALARM TEMP:                       %4.1f C\n", L2_adapt->ame_max_temp );
  printf( "  MIN AME INT ALARM TEMP:                       %4.1f C\n", L2_adapt->ame_min_temp );
  printf( "  MAX AME RECV MOD ALARM TEMP:                  %4.1f C\n", L2_adapt->rcvr_mod_max_temp );
  printf( "  MIN AME RECV MOD ALARM TEMP:                  %4.1f C\n", L2_adapt->rcvr_mod_min_temp );
  printf( "  MAX AME BITE MOD ALARM TEMP:                  %4.1f C\n", L2_adapt->bite_mod_max_temp );
  printf( "  MIN AME BITE MOD ALARM TEMP:                  %4.1f C\n", L2_adapt->bite_mod_min_temp );
  printf( "  DEFAULT POLARIZATION:                         %d\n", L2_adapt->default_polarization );
  printf( "  HORIZ TR LIMITER DEGRADE LIMIT:               %5.2f V\n", L2_adapt->h_tr_limit_degraded_lim );
  printf( "  HORIZ TR LIMITER MAINT LIMIT:                 %5.2f V\n", L2_adapt->h_tr_limit_maint_lim );
  printf( "  VERT TR LIMITER DEGRADE LIMIT:                %5.2f V\n", L2_adapt->v_tr_limit_degraded_lim );
  printf( "  VERT TR LIMITER MAINT LIMIT:                  %5.2f V\n", L2_adapt->v_tr_limit_maint_lim );
  printf( "  AME PELT CURRENT TOLERANCE:                   %5.1f %%\n", L2_adapt->ame_current_tol );
  printf( "  HORIZ POLARIZATION:                           %d\n", L2_adapt->h_only_polarization );
  printf( "  VERT POLARIZATION:                            %d\n", L2_adapt->v_only_polarization );
  for( i = 0; i < 2; i ++ )
  {
/*
    printf( "  SPARE-8809[%1d]:                                %d\n", i, L2_adapt->spare_8809[i] );
*/
  }
  printf( "  ANTENNA REFLECTOR BIAS:                       %6.2f dB\n", L2_adapt->reflector_bias );
  for( i = 0; i < 109; i++ )
  {
/*
    printf( "  SPARE-8810[%03d]:                              %d\n", i, L2_adapt->spare_8810[i] );
*/
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print RDA Clutter Map message.
 ************************************************************************/

int libL2_print_rda_clutter_map( int segment_number, int radial_number )
{
  static char *fx = "libL2_print_rda_clutter_map";
  int i = 0;
  int j = 0;
  int k = 0;
  int clm_ptr = 0;
  short *clm_map = NULL;
  int ret = LIBL2_NO_ERROR;

  /* orda_clutter_map.h                */
  /* typedef struct {                  */
  /* RDA_RPG_message_header_t msg_hdr; */
  /* ORDA_clutter_map_t           map; */
  /* } ORDA_clutter_map_msg_t;         */
  /* ORDA_clutter_map_t -> L2_clutter  */

  libL2_clutter_map_segment_t *clutter_elev_segment;
  libL2_clutter_map_filter_t *clutter_filter;

  if( ( ret = Validate_clutter_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_clutter_map_segment_number returns %d", fx, ret );
    return ret;
  }
  else if( ( ret = Validate_clutter_map_radial_number( segment_number, radial_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_clutter_map_radial_number returns %d", fx, ret );
    return ret;
  }

  printf( "RDA CLUTTER MSG (MSG TYPE 15)\n" );
  printf( "  DATE:       %05d (%s)\n", L2_clutter->date, Convert_date( L2_clutter->date ) );
  /* Time is minutes past midnight. Convert to milliseconds for string. */
  printf( "  TIME:       %05d (%s)\n", L2_clutter->time, Convert_milliseconds( L2_clutter->time * 60 * 1000 ) );
  printf( "  NUM SEG:    %d\n", L2_clutter->num_elevation_segs );

  /*
    Data is stored by elevation segment (up to 5).
    Each elevation segment has a 1-D array with 360
    radials with each containing up to 25 zones.
    Zones are processed starting with bin 0.
    Each zone has a filter code and a stop range.
    Each zone is either:
      0 (Bypass clutter filters -- no clutter filtering)
      1 (Bypass map in control -- use Bypass Map to determine if clutter)
      2 (Force filter -- force clutter filtering regardless of Bypass Map)
  */

  clm_map = (short *) &L2_clutter->data[0];
  clm_ptr = 0;

  for( i = 0; i < libL2_clutter_map_num_segments(); i++ )
  {
    for( j = 0; j < libL2_clutter_map_num_azimuths(i+1); j++ )
    {
      clutter_elev_segment = ( libL2_clutter_map_segment_t * ) &clm_map[clm_ptr];
      if( ( segment_number == LIBL2_ALL_SEGMENTS || segment_number == i+1 ) &&
          ( radial_number == LIBL2_ALL_RADIALS || radial_number == j+1 ) )
      {
        printf( "ELEVATION SEGMENT: %d\n", i+1 );
        for( k = 0; k < clutter_elev_segment->num_zones; k++ )
        {
          clutter_filter = ( libL2_clutter_map_filter_t *) &clutter_elev_segment->filter[k];
          printf( "  AZ %3d ZONE: %2d OP: %d RNG: %3d\n", j+1, k+1, clutter_filter->op_code, clutter_filter->range );
        }
      }
      clm_ptr += ( ( clutter_elev_segment->num_zones*sizeof( libL2_clutter_map_filter_t )/sizeof( short ) )+1 );
    }
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print RDA Bypass Map message.
 ************************************************************************/

int libL2_print_rda_bypass_map( int segment_number, int radial_number )
{
  static char *fx = "libL2_print_rda_bypass_map";
  int i = 0;
  int j = 0;
  int k = 0;
  int l = 0;
  int sn = 0;
  int rn = 0;
  short dataval;
  char printbuf[CLUTTER_MAP_NUM_GATES+1];
  int ret = LIBL2_NO_ERROR;
  ORDA_bypass_map_segment_t bypass_seg;

  /* rda_rpg_clutter_map.h                */
  /* typedef struct {                     */
  /* RDA_RPG_message_header_t msg_hdr;    */
  /* ORDA_bypass_map_t        bypass_map; */
  /* } ORDA_bypass_map_msg_t;             */
  /* ORDA_bypass_map_t -> L2_bypass       */

  if( ( ret = Validate_bypass_map_segment_number( segment_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_bypass_map_segment_number returns %d", fx, ret );
    return ret;
  }
  else if( ( ret = Validate_bypass_map_radial_number( segment_number, radial_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_bypass_map_radial_number returns %d", fx, ret );
    return ret;
  }

  printf( "RDA BYPASS MSG (MSG TYPE 13)\n" );
  printf( "  DATE:       %05d (%s)\n", L2_bypass->date, Convert_date( L2_bypass->date ) );
  /* Time is minutes past midnight. Convert to milliseconds for string. */
  printf( "  TIME:       %05d (%s)\n", L2_bypass->time, Convert_milliseconds( L2_bypass->time * 60 * 1000 ) );
  printf( "  NUM SEGS:   %d\n", L2_bypass->num_segs );

  /*
    Data is stored by elevation segment (up to 5).
    Each elevation segment has a 2-D array (num radials x halfwords per radial).
    There are 360 radials, each with 32 halfwords.
    The 32 halfwords equate to 512 bits, one for each range bin.
    Each bin is either:
      0 (perform clutter filtering.
      1 (bypass clutter filtering)
  */

  printbuf[CLUTTER_MAP_NUM_GATES] = '\0';
  for( i = 0; i < L2_bypass->num_segs; i++ )
  {
    if( segment_number == LIBL2_ALL_SEGMENTS || segment_number == i+1 )
    {
      sn = i+1;
      bypass_seg = L2_bypass->segment[i];
      printf( "  SEGMENT: %d\n", bypass_seg.seg_num );
      for( j = 0; j < libL2_bypass_map_num_azimuths(sn); j++ )
      {
        if( radial_number == LIBL2_ALL_RADIALS || radial_number == j+1 )
        {
          rn = j+1;
          l = 0;
          for( k = 0; k < BYPASS_MAP_HW_PER_RADIAL; k++ )
          {
            dataval = bypass_seg.data[j][k];
            printbuf[l] = ( dataval & BIT_1_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_2_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_3_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_4_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_5_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_6_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_7_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_8_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_9_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_10_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_11_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_12_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_13_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_14_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_15_MASK ) ? '1' : '0'; l++;
            printbuf[l] = ( dataval & BIT_16_MASK ) ? '1' : '0'; l++;
          }
          printf( "SEG: %1d  RAD: %03d DATA: %s\n", sn, rn, printbuf );
        }
        else if( radial_number != LIBL2_ALL_RADIALS && radial_number < j+1 )
        {
          break;
        }
      }
    }
    else if( segment_number != LIBL2_ALL_SEGMENTS && segment_number < i+1 )
    {
      break;
    }
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print RDA VCP message.
 ************************************************************************/

int libL2_print_rda_VCP()
{
  int i = 0;
  unsigned char uc = 0;
  unsigned short us = 0;
  short s = 0;
  char *buf = NULL;
  int pos = 0;

  /* rpg_vcp.h                                 */
  /* typedef struct {                          */
  /* VCP_message_header_t  vcp_msg_hdr;        */
  /* VCP_elevation_cut_header_t vcp_elev_data; */
  /* } VCP_ICD_msg_t -> L2_vcp                 */

  libL2_vcp_hdr_t elev_hdr = L2_vcp->vcp_msg_hdr;
  libL2_vcp_elev_hdr_t elev_data = L2_vcp->vcp_elev_data;

  printf( "RDA VCP MSG (MSG TYPE 5)\n" );
  printf( "  MSG SIZE:            %3d halfwords\n", elev_hdr.msg_size );
  printf( "  PATTERN TYPE:        %1d\n", elev_hdr.pattern_type );
  printf( "  PATTERN NUM:         %3d\n", elev_hdr.pattern_number );
  printf( "  NUM CUTS:            %2d\n", elev_data.number_cuts );
  printf( "  CLUTTER GROUP:       %1d\n", elev_data.group );
  uc = elev_data.doppler_res;
  if( uc == (int) 2 )
  {
    printf( "  DOPPLER RES:         0.5 m/s (%d)\n", (int) uc );
  }
  else if( uc == (int) 4 )
  {
    printf( "  DOPPLER RES:         1.0 m/s (%d)\n", (int) uc );
  }
  else
  {
    printf( "  DOPPLER RES:         UNKNOWN (%d)\n", (int) uc );
  }
  uc = elev_data.pulse_width;
  if( uc == (int) 2 )
  {
    printf( "  PULSE WIDTH:         SHORT (%d)\n", (int) uc );
  }
  else if( uc == (int) 4 )
  {
    printf( "  PULSE WIDTH:         LONG (%d)\n", (int) uc );
  }
  else
  {
    printf( "  PULSE WIDTH:         UNKNOWN (%d)\n", (int) uc );
  }
/*
  printf( "  SPARE-7:              %d\n", elev_data.spare7 );
  printf( "  SPARE-8:              %d\n", elev_data.spare8 );
  printf( "  SPARE-9:              %d\n", elev_data.spare9 );
  printf( "  SPARE-10:             %d\n", elev_data.spare10 );
  printf( "  SPARE-11              %d\n", elev_data.spare11 );
*/
  printf( "\n" );

  for( i = 0; i < elev_data.number_cuts; i++ )
  {
    printf( "  CUT[%02d]\n", i+1 );
    us = elev_data.data[i].angle;
    printf( "    ANGLE:             %10.6f deg (%d)\n", (float) us*BAMS_ELEV, us );
    uc = elev_data.data[i].phase;
    if( uc == (int) 0 )
    {
      printf( "    PHASE:             RANDOM PHASE (%d)\n", (int) uc );
    }
    else if( uc == (int) 1 )
    {
      printf( "    PHASE:             CONSTANT PHASE (%d)\n", (int) uc );
    }
    else if( uc == (int) 2 )
    {
      printf( "    PHASE:             SZ2 PHASE (%d)\n", (int) uc );
    }
    else
    {
      printf( "    PHASE:             UNKNOWN (%d)\n", (int) uc );
    }
    uc = elev_data.data[i].waveform;
    if( uc == (int) 1 )
    {
      printf( "    WAVEFORM:          CS (%d)\n", (int) uc );
    }
    else if( uc == (int) 2 )
    {
      printf( "    WAVEFORM:          CD WITH AMB RES (%d)\n", (int) uc );
    }
    else if( uc == (int) 3 )
    {
      printf( "    WAVEFORM:          CD WITHOUT AMB RES (%d)\n", (int) uc );
    }
    else if( uc == (int) 4 )
    {
      printf( "    WAVEFORM:          BATCH (%d)\n", (int) uc );
    }
    else if( uc == (int) 5 )
    {
      printf( "    WAVEFORM:          STAGGERED PULSE PAIR (%d)\n", (int) uc );
    }
    else
    {
      printf( "    WAVEFORM:          UNKNOWN (%d)\n", (int) uc );
    }
    uc = elev_data.data[i].super_res;
    if( ( buf = malloc( 256 * sizeof( char ) ) ) == NULL )
    {
      return LIBL2_MALLOC_FAILED;
    }
    pos = 0;
    if( uc == (int) 0 )
    {
      strcpy( &buf[pos], "DISABLED" );
      pos += strlen( "DISABLED" );
    }
    else
    {
      if( uc & 0x1 )
      {
        strcpy( &buf[pos], "0.5 DEG AZ" );
        pos += strlen( "0.5 DEG AZ" );
      }
      if( uc & 0x2 )
      {
        if( pos == 0 )
        {
          strcpy( &buf[pos], "0.25 KM REF" );
          pos += strlen( "0.25 KM REF" );
        }
        else
        {
          strcpy( &buf[pos], ",0.25 KM REF" );
          pos += strlen( ",0.25 KM REF" );
        }
      }
      if( uc & 0x4 )
      {
        if( pos == 0 )
        {
          strcpy( &buf[pos], "300 KM DOPPLER" );
          pos += strlen( "300 KM DOPPLER" );
        }
        else
        {
          strcpy( &buf[pos], ",300 KM DOPPLER" );
          pos += strlen( ",300 KM DOPPLER" );
        }
      }
      if( uc & 0x8 )
      {
        if( pos == 0 )
        {
          strcpy( &buf[pos], "DP to 300 KM" );
          pos += strlen( "DP to 300 KM" );
        }
        else
        {
          strcpy( &buf[pos], ",DP to 300 KM" );
          pos += strlen( ",DP to 300 KM" );
        }
      }
    }
    if( pos == 0 )
    {
      strcpy( &buf[pos], "UNKNOWN" );
    }
    printf( "    SR:                %s (%d)\n", buf, (int) uc );
    free( buf );
    buf = NULL;
    printf( "    SURV PRF#:         %1d\n", elev_data.data[i].surv_prf_num );
    printf( "    SURV PRF PULSE:    %3d\n", elev_data.data[i].surv_prf_pulse );
    s = elev_data.data[i].azimuth_rate;
    printf( "    AZ RATE:           %7.3f deg/sec (%d)\n", (float)s*BAMS_AZ, s );
    s = elev_data.data[i].refl_thresh;
    printf( "    REF THRESH:        %5.1f dB (%d)\n", (float) s/8.0, s );
    s = elev_data.data[i].vel_thresh;
    printf( "    VEL THRESH:        %5.1f dB (%d)\n", (float) s/8.0, s );
    s = elev_data.data[i].sw_thresh;
    printf( "    SPW THRESH:        %5.1f dB (%d)\n", (float) s/8.0, s );
    s = elev_data.data[i].diff_refl_thresh;
    printf( "    DIFF REFL THRESH:  %5.1f dB (%d)\n", (float) s/8.0, s );
    s = elev_data.data[i].diff_phase_thresh;
    printf( "    DIFF PHASE THRESH: %5.1f dB (%d)\n", (float) s/8.0, s);
    s = elev_data.data[i].corr_coeff_thresh;
    printf( "    CORR COEFF THRESH: %5.1f dB (%d)\n", (float) s/8.0, s );
    s = elev_data.data[i].edge_angle1;
    printf( "    EDGE ANGLE 1:      %10.6f deg (%d)\n", (float) s*BAMS_ELEV, s );
    printf( "    DOP PRF #1:        %1d\n", elev_data.data[i].dopp_prf_num1 );
    printf( "    DOP PRF PULSE #1:  %3d\n", elev_data.data[i].dopp_prf_pulse1 );
/*
    printf( "    SPARE-15:          %d\n", elev_data.data[i].spare15 );
*/
    s = elev_data.data[i].edge_angle2;
    printf( "    EDGE ANGLE 2:      %10.6f deg (%d)\n", (float) s*BAMS_ELEV, s );
    printf( "    DOP PRF #2:        %1d\n", elev_data.data[i].dopp_prf_num2 );
    printf( "    DOP PRF PULSE #2:  %3d\n", elev_data.data[i].dopp_prf_pulse2 );
/*
    printf( "    SPARE-19:          %d\n", elev_data.data[i].spare19 );
*/
    s = elev_data.data[i].edge_angle3;
    printf( "    EDGE ANGLE 3:      %10.6f deg (%d)\n", (float) s*BAMS_ELEV, s );
    printf( "    DOP PRF #3:        %1d\n", elev_data.data[i].dopp_prf_num3 );
    printf( "    DOP PRF PULSE #3:  %3d\n", elev_data.data[i].dopp_prf_pulse3 );
/*
    printf( "    SPARE-23:          %d\n", elev_data.data[i].spare23 );
*/
    printf( "\n" );
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print RDA PMD message.
 ************************************************************************/

int libL2_print_rda_pmd()
{
  int i = 0;
  short s = 0;
  unsigned short us = 0;
  unsigned int ui = 0;

  /* orda_pmd.h                        */
  /* typedef struct {                  */
  /* RDA_RPG_message_header_t msg_hdr; */
  /* Pmd_t                        pmd; */
  /* } orda_pmd_t;                     */
  /* Pmd_t -> L2_pmd                   */

  printf( "RDA PMD MSG (MSG TYPE 3)\n" );
  printf( "COMMS\n" );
/*
  printf( "  SPARE-1:                            %d\n", L2_pmd->spare1 );
*/
  s = L2_pmd->loop_back_test_status;
  if( s == 0 )
  {
    printf( "  LOOPBACK TEST STATUS:               PASS (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  LOOPBACK TEST STATUS:               FAIL (%d)\n", s );
  }
  else if( s == 2 )
  {
    printf( "  LOOPBACK TEST STATUS:               TIMEOUT (%d)\n", s );
  }
  else if( s == 3 )
  {
    printf( "  LOOPBACK TEST STATUS:               NOT TESTED (%d)\n", s );
  }
  else
  {
    printf( "  LOOPBACK TEST STATUS:               UNKNOWN (%d)\n", s );
  }
  printf( "  T1 OUTPUT FRAMES:                   %d octets\n", L2_pmd->t1_output_frames );
  printf( "  T1 INTPUT FRAMES:                   %d octets\n", L2_pmd->t1_input_frames );
  printf( "  ROUTER MEM BYTES USED:              %d bytes\n", L2_pmd->router_mem_used );
  printf( "  ROUTER MEM BYTES FREE:              %d bytes\n", L2_pmd->router_mem_free );
  printf( "  ROUTER MEM UTIL PCT:                %d %%\n", L2_pmd->router_mem_util );
/*
  printf( "  SPARE-12:                           %d\n", L2_pmd->spare12 );
*/
  printf( "  CSU LOSS OF SIGNAL:                 %d\n", L2_pmd->csu_loss_of_signal );
  printf( "  CSU LOSS OF FRAMES:                 %d\n", L2_pmd->csu_loss_of_frames );
  printf( "  CSU YELLOW ALARMS:                  %d\n", L2_pmd->csu_yellow_alarms );
  printf( "  CSU BLUE ALARMS:                    %d\n", L2_pmd->csu_blue_alarms );
  printf( "  CSU 24 HR ERR SECS:                 %d sec\n", L2_pmd->csu_24hr_err_scnds );
  printf( "  CSU 24 HR SEVERE ERR SECS:          %d sec\n", L2_pmd->csu_24hr_sev_err_scnds );
  printf( "  CSU 24 HR SEVERE ERR FRAMING SECS:  %d sec\n", L2_pmd->csu_24hr_sev_err_frm_scnds );
  printf( "  CSU 24 HR UNAVAIL SECS:             %d sec\n", L2_pmd->csu_24hr_unavail_scnds );
  printf( "  CSU 24 HR CONTROLLED SLIP SECS:     %d sec\n", L2_pmd->csu_24hr_cntrld_slip_scnds );
  printf( "  CSU 24 HR PATH CODING VIOLATIONS:   %d\n", L2_pmd->csu_24hr_path_cding_vlns );
  printf( "  CSU 24 HR LINE ERR SECS:            %d sec\n", L2_pmd->csu_24hr_line_err_scnds );
  printf( "  CSU 24 HR BURSTY ERR SECS:          %d sec\n", L2_pmd->csu_24hr_brsty_err_scnds );
  printf( "  CSU 24 HR DEGRADED MINS:            %d min\n", L2_pmd->csu_24hr_degraded_mins );
  printf( "  LAN SWITCH MEM BYTES USED:          %d bytes\n", L2_pmd->lan_switch_mem_used );
  printf( "  LAN SWITCH MEM BYTES FREE:          %d bytes\n", L2_pmd->lan_switch_mem_free );
  printf( "  LAN SWITCH MEM UTIL PCT:            %d %%\n", L2_pmd->lan_switch_mem_util );
/*
  printf( "  SPARE-44:                           %d\n", L2_pmd->spare44 );
*/
  printf( "  NTP REJECT PKTS:                    %d\n", L2_pmd->ntp_rejected_packets );
  printf( "  NTP EST TIME ERR:                   %d usec\n", L2_pmd->ntp_est_time_error );
  printf( "  GPS SATELLITES:                     %d\n", L2_pmd->gps_satellites );
  printf( "  GPS MAX SIGNAL STRENGTH:            %d dB\n", L2_pmd->gps_max_sig_strength );
  s = L2_pmd->ipc_status;
  if( s == 0 )
  {
    printf( "  IPC STATUS:                         OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  IPC STATUS:                         FAIL (%d)\n", s );
  }
  else if( s == 2 )
  {
    printf( "  IPC STATUS:                         N/A (%d)\n", s );
  }
  else
  {
    printf( "  IPC STATUS:                         UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->cmd_chnl_ctrl;
  if( s == 0 )
  {
    printf( "  CMD CHANNEL CONTROL:                N/A (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  CMD CHANNEL CONTROL:                CH 1 (%d)\n", s );
  }
  else if( s == 2 )
  {
    printf( "  CMD CHANNEL CONTROL:                CH 2 (%d)\n", s );
  }
  else
  {
    printf( "  CMD CHANNEL CONTROL:                UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->dau_tst_0;
  if( s == 10 )
  {
    printf( "  DAU TEST 0:                         NORMAL (%d)\n", s );
  }
  else if( s >= 7 && s <= 11 )
  {
    printf( "  DAU TEST 0:                         ACCEPTABLE (%d)\n", s );
  }
  else
  {
    printf( "  DAU TEST 0:                         FAULT (%d)\n", s );
  }
  s = L2_pmd->dau_tst_1;
  if( s == 127 )
  {
    printf( "  DAU TEST 1:                         NORMAL (%d)\n", s );
  }
  else if( s >= 118 && s <= 136 )
  {
    printf( "  DAU TEST 1:                         ACCEPTABLE (%d)\n", s );
  }
  else
  {
    printf( "  DAU TEST 1:                         FAULT (%d)\n", s );
  }
  s = L2_pmd->dau_tst_2;
  if( s == 245 )
  {
    printf( "  DAU TEST 2:                         NORMAL (%d)\n", s );
  }
  else if( s >= 221 && s <= 252 )
  {
    printf( "  DAU TEST 2:                         ACCEPTABLE (%d)\n", s );
  }
  else
  {
    printf( "  DAU TEST 2:                         FAULT (%d)\n", s );
  }
  printf( "AME\n" );
  us = L2_pmd->polarization;
  if( us == 0 )
  {
    printf( "  POLARIZATION:                       HORIZ (%d)\n", us );
  }
  else if( us == 1 )
  {
    printf( "  POLARIZATION:                       HORIZ + VERT (%d)\n", us );
  }
  else if( us == 2 )
  {
    printf( "  POLARIZATION:                       VERT (%d)\n", us );
  }
  else
  {
    printf( "  POLARIZATION:                       UNKNOWN (%d)\n", us );
  }
  printf( "  AME INTERNAL TEMP:                  %5.1f C\n", L2_pmd->internal_temp );
  printf( "  AME RECV MOD TEMP:                  %5.1f C\n", L2_pmd->rec_module_temp );
  printf( "  AME BITE/CAL MOD TEMP:              %5.1f\n C", L2_pmd->bite_cal_module_temp );
  printf( "  AME PELT PULSE WID MOD:             %3d %%\n", L2_pmd->peltier_pulse_width_modulation );

  us = L2_pmd->peltier_status;
  if( us == 0 )
  {
    printf( "  AME PELT STATUS:                    OFF (%d)\n", us );
  }
  else if( us == 1 )
  {
    printf( "  AME PELT STATUS:                    ON (%d)\n", us );
  }
  else
  {
    printf( "  AME PELT STATUS:                    UNKNOWN (%d)\n", us );
  }

  us = L2_pmd->a2d_converter_status;
  if( us == 0 )
  {
    printf( "  AME A/D CONV STATUS:                OK (%d)\n", us );
  }
  else if( us == 1 )
  {
    printf( "  AME A/D CONV STATUS:                FAIL (%d)\n", us );
  }
  else
  {
    printf( "  AME A/D CONV STATUS:                UNKNOWN (%d)\n", us );
  }

  us = L2_pmd->ame_state;
  if( us == 0 )
  {
    printf( "  AME STATE:                          START (%d)\n", us );
  }
  else if( us == 1 )
  {
    printf( "  AME STATE:                          RUNNING (%d)\n", us );
  }
  else if( us == 2 )
  {
    printf( "  AME STATE:                          FLASH (%d)\n", us );
  }
  else if( us == 3 )
  {
    printf( "  AME STATE:                          ERROR (%d)\n", us );
  }
  else
  {
    printf( "  AME STATE:                          UNKNOWN (%d)\n", us );
  }
  printf( "  AME +3.3V PS:                       %4.2f V\n", L2_pmd->p_33vdc_ps );
  printf( "  AME +5V PS:                         %4.2f V\n", L2_pmd->p_50vdc_ps );
  printf( "  AME +6.5V PS:                       %4.2f V\n", L2_pmd->p_65vdc_ps );
  printf( "  AME +15V PS:                        %5.2f V\n", L2_pmd->p_150vdc_ps );
  printf( "  AME +48V PS:                        %5.2f V\n", L2_pmd->p_480vdc_ps );
  printf( "  AME STALO PWR:                      %4.2f V\n", L2_pmd->stalo_power );
  printf( "  PELT CURRENT:                       %5.2f A\n", L2_pmd->peltier_current );
  printf( "  ADC CALIB REF VOLTAGE:              %5.3f V\n", L2_pmd->adc_calib_ref_voltage );
  us = L2_pmd->mode;
  if( us == 0 )
  {
    printf( "  AME MODE:                           READY (%d)\n", us );
  }
  else if( us == 1 )
  {
    printf( "  AME MODE:                           MAINTENANCE (%d)\n", us );
  }
  else
  {
    printf( "  AME MODE:                           UNKNOWN (%d)\n", us );
  }
  us = L2_pmd->peltier_mode;
  if( us == 0 )
  {
    printf( "  AME PELT MODE:                      COOL (%d)\n", us );
  }
  else if( us == 1 )
  {
    printf( "  AME PELT MODE:                      HEAT (%d)\n", us );
  }
  else
  {
    printf( "  AME PELT MODE:                      UNKNOWN (%d)\n", us );
  }
  printf( "  AME PELT INT FAN CURRENT:           %4.2f A\n", L2_pmd->peltier_inside_fan_current );
  printf( "  AME PELT EXT FAN CURRENT:           %4.2f A\n", L2_pmd->peltier_outside_fan_current );
  printf( "  HORIZ TR LIMITER VOLTAGE:           %4.2f V\n", L2_pmd->h_tr_limiter_voltage );
  printf( "  VERT TR LIMITER VOLTAGE:            %4.2f V\n", L2_pmd->v_tr_limiter_voltage );
  printf( "  ADC CALIB OFFSET VOLTAGE:           %7.2f mV\n", L2_pmd->adc_calib_offset_voltage );
  printf( "  ADC CALIB GAIN CORRECTION:          %5.3f\n", L2_pmd->adc_calib_gain_correction );
  printf( "POWER\n" );
  ui = L2_pmd->ups_batt_status;
  if( ui == 2 )
  {
    printf( "  UPS BATTERY STATUS:                 OK (%d)\n", us );
  }
  else if( us == 3 )
  {
    printf( "  UPS BATTERY STATUS:                 LOW (%d)\n", us );
  }
  else
  {
    printf( "  UPS BATTERY STATUS:                 UNKNOWN (%d)\n", us );
  }
  printf( "  UPS TIME ON BATTERY:                %d sec\n", L2_pmd->ups_time_on_batt );
  printf( "  UPS BATTERY TEMP:                   %6.2f C\n", L2_pmd->ups_batt_temp );
  printf( "  UPS OUTPUT VOLTAGE:                 %6.2f V\n", L2_pmd->ups_output_volt );
  printf( "  UPS OUTPUT FREQUENCY:               %5.2f Hz\n", L2_pmd->ups_output_freq );
  printf( "  UPS OUTPUT CURRENT:                 %5.2f A\n", L2_pmd->ups_output_current );
  printf( "  POWER ADMIN LOAD:                   %5.2f A\n", L2_pmd->pwr_admin_load );
  for( i = 0; i < 24; i++ )
  {
/*
    printf( "  SPARE-113[%02d]:                      %d\n", i, L2_pmd->spare113[i] );
*/
  }
  printf( "TRANSMITTER\n" );
  s = L2_pmd->p_5vdc_ps;
  if( s == 0 )
  {
    printf( "  +5 VDC PS:                          OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  +5 VDC PS:                          FAIL (%d)\n", s );
  }
  else
  {
    printf( "  +5 VDC PS:                          UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->p_15vdc_ps;
  if( s == 0 )
  {
    printf( "  +15 VDC PS:                         OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  +15 VDC PS:                         FAIL (%d)\n", s );
  }
  else
  {
    printf( "  +15 VDC PS:                         UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->p_28vdc_ps;
  if( s == 0 )
  {
    printf( "  +28 VDC PS:                         OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  +28 VDC PS:                         FAIL (%d)\n", s );
  }
  else
  {
    printf( "  +28 VDC PS:                         UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->n_15vdc_ps;
  if( s == 0 )
  {
    printf( "  -15 VDC PS:                         OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  -15 VDC PS:                         FAIL (%d)\n", s );
  }
  else
  {
    printf( "  -15 VDC PS:                         UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->p_45vdc_ps;
  if( s == 0 )
  {
    printf( "  +45 VDC PS:                         OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  +45 VDC PS:                         FAIL (%d)\n", s );
  }
  else
  {
    printf( "  +45 VDC PS:                         UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->flmnt_ps_vlt;
  if( s == 0 )
  {
    printf( "  FILAMENT PS VOLTAGE:                OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  FILAMENT PS VOLTAGE:                FAIL (%d)\n", s );
  }
  else
  {
    printf( "  FILAMENT PS VOLTAGE:                UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->vcum_pmp_ps_vlt;
  if( s == 0 )
  {
    printf( "  VACUUM PUMP PS VOLTAGE:             OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  VACUUM PUMP PS VOLTAGE:             FAIL (%d)\n", s );
  }
  else
  {
    printf( "  VACUUM PUMP PS VOLTAGE:             UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->fcs_coil_ps_vlt;
  if( s == 0 )
  {
    printf( "  FOCUS COIL PS VOLTAGE:              OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  FOCUS COIL PS VOLTAGE:              FAIL (%d)\n", s );
  }
  else
  {
    printf( "  FOCUS COIL PS VOLTAGE:              UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->flmnt_ps;
  if( s == 0 )
  {
    printf( "  FILAMENT PS:                        ON (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  FILAMENT PS:                        OFF (%d)\n", s );
  }
  else
  {
    printf( "  FILAMENT PS:                        UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->klystron_warmup;
  if( s == 0 )
  {
    printf( "  KLYSTRON WARMUP:                    NORMAL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  KLYSTRON WARMUP:                    PREHEAT (%d)\n", s );
  }
  else
  {
    printf( "  KLYSTRON WARMUP:                    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->trsmttr_avlble;
  if( s == 0 )
  {
    printf( "  XMTR AVAILABLE:                     YES (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  XMTR AVAILABLE:                     NO (%d)\n", s );
  }
  else
  {
    printf( "  XMTR AVAILABLE:                     UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->wg_swtch_position;
  if( s == 0 )
  {
    printf( "  WG SWITCH POSITION:                 ANTENNAE (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  WG SWITCH POSITION:                 DUMMY LOAD (%d)\n", s );
  }
  else
  {
    printf( "  WG SWITCH POSITION:                 UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->wg_pfn_trsfr_intrlck;
  if( s == 0 )
  {
    printf( "  WG/PFN TRANS INTERLOCK:             OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  WG/PFN TRANS INTERLOCK:             OPEN (%d)\n", s );
  }
  else
  {
    printf( "  WG/PFN TRANS INTERLOCK:             UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->mntnce_mode;
  if( s == 0 )
  {
    printf( "  MAINT MODE:                         NO (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  MAINT MODE:                         YES (%d)\n", s );
  }
  else
  {
    printf( "  MAINT MODE:                         UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->mntnce_reqd;
  if( s == 0 )
  {
    printf( "  MAINT REQD:                         NO (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  MAINT REQD:                         REQUIRED (%d)\n", s );
  }
  else
  {
    printf( "  MAINT REQD:                         UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->pfn_swtch_position;
  if( s == 0 )
  {
    printf( "  PFN SWITCH POSITION:                SHORT PULSE (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  PFN SWITCH POSITION:                LONG PULSE (%d)\n", s );
  }
  else
  {
    printf( "  PFN SWITCH POSITION:                UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->modular_ovrld;
  if( s == 0 )
  {
    printf( "  MODULATOR OVERLOAD:                 OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  MODULATOR OVERLOAD:                 FAIL (%d)\n", s );
  }
  else
  {
    printf( "  MODULATOR OVERLOAD:                 UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->modulator_inv_crnt;
  if( s == 0 )
  {
    printf( "  MODULATOR INV CURRENT:              OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  MODULATOR INV CURRENT:              FAIL (%d)\n", s );
  }
  else
  {
    printf( "  MODULATOR INV CURRENT:              UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->modulator_swtch_fail;
  if( s == 0 )
  {
    printf( "  MODULATOR SWITCH FAIL:              OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  MODULATOR SWITCH FAIL:              FAIL (%d)\n", s );
  }
  else
  {
    printf( "  MODULATOR SWITCH FAIL:              UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->main_pwr_vlt;
  if( s == 0 )
  {
    printf( "  MAIN POWER VOLTAGE:                 OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  MAIN POWER VOLTAGE:                 OVER (%d)\n", s );
  }
  else
  {
    printf( "  MAIN POWER VOLTAGE:                 UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->chrg_sys_fail;
  if( s == 0 )
  {
    printf( "  CHARGING SYS FAIL:                  OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  CHARGING SYS FAIL:                  FAIL (%d)\n", s );
  }
  else
  {
    printf( "  CHARGING SYS FAIL:                  UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->invrs_diode_crnt;
  if( s == 0 )
  {
    printf( "  INV DIODE CURRENT:                  OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  INV DIODE CURRENT:                  FAIL (%d)\n", s );
  }
  else
  {
    printf( "  INV DIODE CURRENT:                  UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->trggr_amp;
  if( s == 0 )
  {
    printf( "  TRIGGER AMP:                        OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  TRIGGER AMP:                        FAIL (%d)\n", s );
  }
  else
  {
    printf( "  TRIGGER AMP:                        UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->circulator_temp;
  if( s == 0 )
  {
    printf( "  CIRCULATOR TEMP:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  CIRCULATOR TEMP:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  CIRCULATOR TEMP:                    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->spctrm_fltr_pressure;
  if( s == 0 )
  {
    printf( "  SPECTRUM FILTER PRESSURE:           OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  SPECTRUM FILTER PRESSURE:           FAIL (%d)\n", s );
  }
  else
  {
    printf( "  SPECTRUM FILTER PRESSURE:           UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->wg_arc_vswr;
  if( s == 0 )
  {
    printf( "  WG ARC/VSWR:                        OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  WG ARC/VSWR:                        FAIL (%d)\n", s );
  }
  else
  {
    printf( "  WG ARC/VSWR:                        UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->cbnt_interlock;
  if( s == 0 )
  {
    printf( "  CABINET INTERLOCK:                  OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  CABINET INTERLOCK:                  OPEN (%d)\n", s );
  }
  else
  {
    printf( "  CABINET INTERLOCK:                  UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->cbnt_air_temp;
  if( s == 0 )
  {
    printf( "  CABINET AIR TEMP:                   OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  CABINET AIR TEMP:                   FAIL (%d)\n", s );
  }
  else
  {
    printf( "  CABINET AIR TEMP:                   UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->cbnt_air_flow;
  if( s == 0 )
  {
    printf( "  CABINET AIRFLOW:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  CABINET AIRFLOW:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  CABINET AIRFLOW:                    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->klystron_crnt;
  if( s == 0 )
  {
    printf( "  KLYSTRON CURRENT:                   ? (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  KLYSTRON CURRENT:                   ? (%d)\n", s );
  }
  else
  {
    printf( "  KLYSTRON CURRENT:                   UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->klystron_flmnt_crnt;
  if( s == 0 )
  {
    printf( "  KLYSTRON FILAMENT CURRENT:          OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  KLYSTRON FILAMENT CURRENT:          FAIL (%d)\n", s );
  }
  else
  {
    printf( "  KLYSTRON FILAMENT CURRENT:          UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->klystron_vacion_crnt;
  if( s == 0 )
  {
    printf( "  KLYSTRON VACION CURRENT:            OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  KLYSTRON VACION CURRENT:            FAIL (%d)\n", s );
  }
  else
  {
    printf( "  KLYSTRON VACION CURRENT:            UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->klystron_air_temp;
  if( s == 0 )
  {
    printf( "  KLYSTRON AIR TEMP:                  OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  KLYSTRON AIR TEMP:                  FAIL (%d)\n", s );
  }
  else
  {
    printf( "  KLYSTRON AIR TEMP:                  UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->klystron_air_flow;
  if( s == 0 )
  {
    printf( "  KLYSTRON AIR FLOW:                  OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  KLYSTRON AIR FLOW:                  FAIL (%d)\n", s );
  }
  else
  {
    printf( "  KLYSTRON AIR FLOW:                  UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->modulator_swtch_mntnce;
  if( s == 0 )
  {
    printf( "  MODULATOR SWITCH MAINT:             OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  MODULATOR SWITCH MAINT:             REQUIRED (%d)\n", s );
  }
  else
  {
    printf( "  MODULATOR SWITCH MAINT:             UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->post_chrg_regulator;
  if( s == 0 )
  {
    printf( "  POST CHARGE REG MAINT:              OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  POST CHARGE REG MAINT:              MAINTENANCE (%d)\n", s );
  }
  else
  {
    printf( "  POST CHARGE REG MAINT:              UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->wg_pressure_humidity;
  if( s == 0 )
  {
    printf( "  WG PRESSURE/HUMIDITY:               OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  WG PRESSURE/HUMIDITY:               FAIL (%d)\n", s );
  }
  else
  {
    printf( "  WG PRESSURE/HUMIDITY:               UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->trsmttr_ovr_vlt;
  if( s == 0 )
  {
    printf( "  XMTR OVERVOLTAGE:                   OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  XMTR OVERVOLTAGE:                   OVER (%d)\n", s );
  }
  else
  {
    printf( "  XMTR OVERVOLTAGE:                   UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->trsmttr_ovr_crnt;
  if( s == 0 )
  {
    printf( "  XMTR OVERCURRENT:                   OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  XMTR OVERCURRENT:                   OVER (%d)\n", s );
  }
  else
  {
    printf( "  XMTR OVERCURRENT:                   UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->fcs_coil_crnt;
  if( s == 0 )
  {
    printf( "  FOCUS COIL CURRENT:                 OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  FOCUS COIL CURRENT:                 FAIL (%d)\n", s );
  }
  else
  {
    printf( "  FOCUS COIL CURRENT:                 UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->fcs_coil_air_flow;
  if( s == 0 )
  {
    printf( "  FOCUS COIL AIRFLOW:                 OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  FOCUS COIL AIRFLOW:                 FAIL (%d)\n", s );
  }
  else
  {
    printf( "  FOCUS COIL AIRFLOW:                 UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->oil_temp;
  if( s == 0 )
  {
    printf( "  OIL TEMP:                           OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  OIL TEMP:                           FAIL (%d)\n", s );
  }
  else
  {
    printf( "  OIL TEMP:                           UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->prf_limit;
  if( s == 0 )
  {
    printf( "  PRF LIMIT:                          OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  PRF LIMIT:                          FAIL (%d)\n", s );
  }
  else
  {
    printf( "  PRF LIMIT:                          UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->trsmttr_oil_lvl;
  if( s == 0 )
  {
    printf( "  XMTR OIL LEVEL:                     OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  XMTR OIL LEVEL:                     FAIL (%d)\n", s );
  }
  else
  {
    printf( "  XMTR OIL LEVEL:                     UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->trsmttr_batt_chrgng;
  if( s == 0 )
  {
    printf( "  XMTR BATTERY CHARGING:              YES (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  XMTR BATTERY CHARGING:              NO (%d)\n", s );
  }
  else
  {
    printf( "  XMTR BATTERY CHARGING:              UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->hv_status;
  if( s == 0 )
  {
    printf( "  HIGH VOLTAGE STATUS:                ON (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  HIGH VOLTAGE STATUS:                OFF (%d)\n", s );
  }
  else
  {
    printf( "  HIGH VOLTAGE STATUS:                UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->trsmttr_recycling_smmry;
  if( s == 0 )
  {
    printf( "  XMTR RECYCLING SUMMARY:             NORMAL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  XMTR RECYCLING SUMMARY:             RECYCLING (%d)\n", s );
  }
  else
  {
    printf( "  XMTR RECYCLING SUMMARY:             UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->trsmttr_inoperable;
  if( s == 0 )
  {
    printf( "  XMTR INOP:                          OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  XMTR INOP:                          INOP (%d)\n", s );
  }
  else
  {
    printf( "  XMTR INOP:                          UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->trsmttr_air_fltr;
  if( s == 0 )
  {
    printf( "  XMTR AIR FILTER:                    DIRTY (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  XMTR AIR FILTER:                    OK (%d)\n", s );
  }
  else
  {
    printf( "  XMTR AIR FILTER:                    UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->zero_tst_bit_0;
  if( s == 0 )
  {
    printf( "  ZERO TEST BIT 0:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ZERO TEST BIT 0:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  ZERO TEST BIT 0:                    UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->zero_tst_bit_1;
  if( s == 0 )
  {
    printf( "  ZERO TEST BIT 1:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ZERO TEST BIT 1:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  ZERO TEST BIT 1:                    UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->zero_tst_bit_2;
  if( s == 0 )
  {
    printf( "  ZERO TEST BIT 2:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ZERO TEST BIT 2:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  ZERO TEST BIT 2:                    UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->zero_tst_bit_3;
  if( s == 0 )
  {
    printf( "  ZERO TEST BIT 3:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ZERO TEST BIT 3:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  ZERO TEST BIT 3:                    UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->zero_tst_bit_4;
  if( s == 0 )
  {
    printf( "  ZERO TEST BIT 4:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ZERO TEST BIT 4:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  ZERO TEST BIT 4:                    UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->zero_tst_bit_5;
  if( s == 0 )
  {
    printf( "  ZERO TEST BIT 5:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ZERO TEST BIT 5:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  ZERO TEST BIT 5:                    UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->zero_tst_bit_6;
  if( s == 0 )
  {
    printf( "  ZERO TEST BIT 6:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ZERO TEST BIT 6:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  ZERO TEST BIT 6:                    UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->zero_tst_bit_7;
  if( s == 0 )
  {
    printf( "  ZERO TEST BIT 7:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ZERO TEST BIT 7:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  ZERO TEST BIT 7:                    UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->one_tst_bit_0;
  if( s == 0 )
  {
    printf( "  ONE TEST BIT 0:                     FAIL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ONE TEST BIT 0:                     OK (%d)\n", s );
  }
  else
  {
    printf( "  ONE TEST BIT 0:                     UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->one_tst_bit_1;
  if( s == 0 )
  {
    printf( "  ONE TEST BIT 1:                     FAIL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ONE TEST BIT 1:                     OK (%d)\n", s );
  }
  else
  {
    printf( "  ONE TEST BIT 1:                     UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->one_tst_bit_2;
  if( s == 0 )
  {
    printf( "  ONE TEST BIT 2:                     FAIL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ONE TEST BIT 2:                     OK (%d)\n", s );
  }
  else
  {
    printf( "  ONE TEST BIT 2:                     UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->one_tst_bit_3;
  if( s == 0 )
  {
    printf( "  ONE TEST BIT 3:                     FAIL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ONE TEST BIT 3:                     OK (%d)\n", s );
  }
  else
  {
    printf( "  ONE TEST BIT 3:                     UNKNOWN (%d)\n", s );
  } 
  s = L2_pmd->one_tst_bit_4;
  if( s == 0 )
  {
    printf( "  ONE TEST BIT 4:                     FAIL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ONE TEST BIT 4:                     OK (%d)\n", s );
  }
  else
  {
    printf( "  ONE TEST BIT 4:                     UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->one_tst_bit_5;
  if( s == 0 )
  {
    printf( "  ONE TEST BIT 5:                     FAIL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ONE TEST BIT 5:                     OK (%d)\n", s );
  }
  else
  {
    printf( "  ONE TEST BIT 5:                     UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->one_tst_bit_6;
  if( s == 0 )
  {
    printf( "  ONE TEST BIT 6:                     FAIL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ONE TEST BIT 6:                     OK (%d)\n", s );
  }
  else
  {
    printf( "  ONE TEST BIT 6:                     UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->one_tst_bit_7;
  if( s == 0 )
  {
    printf( "  ONE TEST BIT 7:                     FAIL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ONE TEST BIT 7:                     OK (%d)\n", s );
  }
  else
  {
    printf( "  ONE TEST BIT 7:                     UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->xmtr_dau_interface;
  if( s == 0 )
  {
    printf( "  XMTR/DAU INTFCE:                    FAIL (%d\n", s);
  }
  else if( s == 1 )
  {
    printf( "  XMTR/DAU INTFCE:                    OK (%d\n", s);
  }
  else
  {
    printf( "  XMTR/DAU INTFCE:                    UNKNOWN (%d\n", s);
  }
  s = L2_pmd->trsmttr_smmry_status;
  if( s == 0 )
  {
    printf( "  XMTR SUMMARY STATUS:                READY (%d\n", s);
  }
  else if( s == 1 )
  {
    printf( "  XMTR SUMMARY STATUS:                ALARM (%d\n", s);
  }
  else if( s == 2 )
  {
    printf( "  XMTR SUMMARY STATUS:                MAINTENANCE (%d\n", s);
  }
  else if( s == 3 )
  {
    printf( "  XMTR SUMMARY STATUS:                RECYCLE (%d\n", s);
  }
  else if( s == 4 )
  {
    printf( "  XMTR SUMMARY STATUS:                PREHEAT (%d\n", s);
  }
  else
  {
    printf( "  XMTR SUMMARY STATUS:                UNKNOWN (%d\n", s);
  }
/*
  printf( "  SPARE-204:                          %d\n", L2_pmd->spare204 );
*/
  printf( "  XMTR RF PWR:                        %7.4f mW\n", L2_pmd->trsmttr_rf_pwr );
  printf( "  HORIZ XMTR PEAK PWR:                %5.1f kW\n", L2_pmd->h_trsmttr_peak_pwr );
  printf( "  XMTR PEAK PWR:                      %5.1f kW\n", L2_pmd->xmtr_peak_pwr );
  printf( "  VERT XMTR PEAK PWR:                 %5.1f kW\n", L2_pmd->v_trsmttr_peak_pwr );
  printf( "  XMTR RF AVG PWR:                    %6.1f W\n", L2_pmd->xmtr_rf_avg_pwr );
  printf( "  XMTR POWER METER ZERO:              %3d\n", L2_pmd->xmtr_pwr_mtr_zero );
/*
  printf( "  SPARE-216:                          %d\n", L2_pmd->spare216 );
*/
  printf( "  XMTR RECYCLE COUNT:                 %6d\n", L2_pmd->xmtr_recycle_cnt );
  printf( "  RECEIVER BIAS:                      %f\n", L2_pmd->receiver_bias );
  printf( "  TRANSMIT IMBALANCE:                 %f\n", L2_pmd->transmit_imbalance );
  for( i = 0; i < 6; i++ )
  {
/*
    printf( "  SPARE-223[%1d]:                       %d\n", i, L2_pmd->spare223[i] );
*/
  }
  printf( "UTILITIES\n" );
  s = L2_pmd->ac_1_cmprsr_shut_off;
  if( s == 0 )
  {
    printf( "  AC #1 COMPRESSOR SHUTOFF:           OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AC #1 COMPRESSOR SHUTOFF:           SHUTOFF (%d)\n", s );
  }
  else
  {
    printf( "  AC #1 COMPRESSOR SHUTOFF:           UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->ac_2_cmprsr_shut_off;
  if( s == 0 )
  {
    printf( "  AC #2 COMPRESSOR SHUTOFF:           OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AC #2 COMPRESSOR SHUTOFF:           SHUTOFF (%d)\n", s );
  }
  else
  {
    printf( "  AC #2 COMPRESSOR SHUTOFF:           UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->gnrtr_mntnce_reqd;
  if( s == 0 )
  {
    printf( "  GENERATOR MAINT REQD:               YES (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  GENERATOR MAINT REQD:               NO (%d)\n", s );
  }
  else
  {
    printf( "  GENERATOR MAINT REQD:               UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->gnrtr_batt_vlt;
  if( s == 0 )
  {
    printf( "  GENERATOR BATTERY VOLTAGE:          LOW (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  GENERATOR BATTERY VOLTAGE:          OK (%d)\n", s );
  }
  else
  {
    printf( "  GENERATOR BATTERY VOLTAGE:          UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->gnrtr_engn;
  if( s == 0 )
  {
    printf( "  GENERATOR ENGINE:                   FAIL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  GENERATOR ENGINE:                   OK (%d)\n", s );
  }
  else
  {
    printf( "  GENERATOR ENGINE:                   UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->gnrtr_vlt_freq;
  if( s == 0 )
  {
    printf( "  GENERATOR VOLT/FREQUENCY:           NOT AVAILABLE (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  GENERATOR VOLT/FREQUENCY:           AVAILABLE (%d)\n", s );
  }
  else
  {
    printf( "  GENERATOR VOLT/FREQUENCY:           UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->pwr_src;
  if( s == 0 )
  {
    printf( "  POWER SOURCE:                       UTILITY (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  POWER SOURCE:                       GENERATOR (%d)\n", s );
  }
  else
  {
    printf( "  POWER SOURCE:                       UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->trans_pwr_src;
  if( s == 0 )
  {
    printf( "  TRANS PWR SOURCE (TPS):             OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  TRANS PWR SOURCE (TPS):             OFF (%d)\n", s );
  }
  else
  {
    printf( "  TRANS PWR SOURCE (TPS):             UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->gen_auto_run_off_switch;
  if( s == 0 )
  {
    printf( "  GENERATOR AUTO/RUN/OFF SWITCH:      MANUAL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  GENERATOR AUTO/RUN/OFF SWITCH:      AUTO (%d)\n", s );
  }
  else
  {
    printf( "  GENERATOR AUTO/RUN/OFF SWITCH:      UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->aircraft_hzrd_lighting;
  if( s == 0 )
  {
    printf( "  AIRCRAFT HAZARD LIGHT:              FAIL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AIRCRAFT HAZARD LIGHT:              OK (%d)\n", s );
  }
  else
  {
    printf( "  AIRCRAFT HAZARD LIGHT:              UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->dau_uart;
  if( s == 0 )
  {
    printf( "  DAU UART:                           OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  DAU UART:                           FAIL (%d)\n", s );
  }
  else
  {
    printf( "  DAU UART:                           UNKNOWN (%d)\n", s );
  }
  for( i = 0; i < 10; i++ )
  {
/*
    printf( "  SPARE-240[%02d]:                      %d\n", i, L2_pmd->spare240[i] );
*/
  }
  printf( "SHELTER\n" );
  s = L2_pmd->equip_shltr_fire_sys;
  if( s == 0 )
  {
    printf( "  EQUIP FIRE DETECT SYS:              OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  EQUIP FIRE DETECT SYS:              FAIL (%d)\n", s );
  }
  else
  {
    printf( "  EQUIP FIRE DETECT SYS:              UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->equip_shltr_fire_smk;
  if( s == 0 )
  {
    printf( "  EQUIP FIRE/SMOKE:                   OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  EQUIP FIRE/SMOKE:                   FIRE (%d)\n", s );
  }
  else
  {
    printf( "  EQUIP FIRE/SMOKE:                   UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->gnrtr_shltr_fire_smk;
  if( s == 0 )
  {
    printf( "  GENERATOR FIRE/SMOKE:               FIRE (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  GENERATOR FIRE/SMOKE:               OK (%d)\n", s );
  }
  else
  {
    printf( "  GENERATOR FIRE/SMOKE:               UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->utlty_vlt_freq;
  if( s == 0 )
  {
    printf( "  UTIL VOLTAGE/FREQUENCY:             NOT AVAILABLE (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  UTIL VOLTAGE/FREQUENCY:             AVAILABLE (%d)\n", s );
  }
  else
  {
    printf( "  UTIL VOLTAGE/FREQUENCY:             UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->site_scrty_alarm;
  if( s == 0 )
  {
    printf( "  SITE SECURITY ALARM:                ALARM (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  SITE SECURITY ALARM:                OK (%d)\n", s );
  }
  else
  {
    printf( "  SITE SECURITY ALARM:                UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->scrty_equip;
  if( s == 0 )
  {
    printf( "  SECURITY EQUIP:                     FAIL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  SECURITY EQUIP:                     OK (%d)\n", s );
  }
  else
  {
    printf( "  SECURITY EQUIP:                     UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->scrty_sys;
  if( s == 0 )
  {
    printf( "  SECURITY SYS:                       DISABLED (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  SECURITY SYS:                       OK (%d)\n", s );
  }
  else
  {
    printf( "  SECURITY SYS:                       UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->rcvr_cnctd_to_antna;
  if( s == 0 )
  {
    printf( "  RECV CONN TO ANTENNA:               CONNECTED (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  RECV CONN TO ANTENNA:               NOT CONNECTED (%d)\n", s );
  }
  else if( s == 2 )
  {
    printf( "  RECV CONN TO ANTENNA:               N/A (%d)\n", s );
  }
  else
  {
    printf( "  RECV CONN TO ANTENNA:               UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->radome_hatch;
  if( s == 0 )
  {
    printf( "  RADOME HATCH:                       OPEN (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  RADOME HATCH:                       CLOSED (%d)\n", s );
  }
  else
  {
    printf( "  RADOME HATCH:                       UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->ac_1_fltr_drty;
  if( s == 0 )
  {
    printf( "  AC #1 FILTER DIRTY:                 DIRTY (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AC #1 FILTER DIRTY:                 OK (%d)\n", s );
  }
  else
  {
    printf( "  AC #1 FILTER DIRTY:                 UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->ac_2_fltr_drty;
  if( s == 0 )
  {
    printf( "  AC #2 FILTER DIRTY:                 DIRTY (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AC #2 FILTER DIRTY:                 OK (%d)\n", s );
  }
  else
  {
    printf( "  AC #2 FILTER DIRTY:                 UNKNOWN (%d)\n", s );
  }
  printf( "  EQUIP TEMP:                         %5.2f C\n", L2_pmd->equip_shltr_temp );
  printf( "  OUTSIDE TEMP:                       %6.2f C\n", L2_pmd->outside_amb_temp );
  printf( "  XMTR LEAVING TEMP:                  %6.2f C\n", L2_pmd->trsmttr_leaving_air_temp );
  printf( "  AC #1 DISCHARGE TEMP:               %5.2f C\n", L2_pmd->ac_1_dschrg_air_temp );
  printf( "  GENERATOR SHELTER TEMP:             %5.2f C\n", L2_pmd->gnrtr_shltr_temp );
  printf( "  RADOME AIR TEMP:                    %6.2f C\n", L2_pmd->radome_air_temp );
  printf( "  AC #2 DISCHARGE TEMP:               %5.2f C\n", L2_pmd->ac_2_dschrg_air_temp );
  printf( "  DAU +15v PS:                        %5.2f V\n", L2_pmd->dau_p_15v_ps );
  printf( "  DAU -15v PS:                        %6.2f V\n", L2_pmd->dau_n_15v_ps );
  printf( "  DAU +28v PS:                        %5.2f V\n", L2_pmd->dau_p_28v_ps );
  printf( "  DAU +5v PS:                         %4.2f V\n", L2_pmd->dau_p_5v_ps );
  printf( "  GENERATOR FUEL LEVEL:               %3d %%\n", L2_pmd->cnvrtd_gnrtr_fuel_lvl );
  for( i = 0; i < 7; i++ )
  {
/*
    printf( "  SPARE-284[%1d]:                       %d\n", i, L2_pmd->spare284[i] );
*/
  }
  printf( "PEDESTAL\n" );
  printf( "  PED +28v PS:                        %5.2f V\n", L2_pmd->pdstl_p_28v_ps );
  printf( "  PED +15v PS:                        %5.2f V\n", L2_pmd->pdstl_p_15v_ps );
  printf( "  ENC +5v PS:                         %5.2f V\n", L2_pmd->encdr_p_5v_ps );
  printf( "  PED +5v PS:                         %4.2f V\n", L2_pmd->pdstl_p_5v_ps );
  printf( "  PED -15v PS:                        %6.2f V\n", L2_pmd->pdstl_n_15v_ps );
  s = L2_pmd->p_150v_ovrvlt;
  if( s == 0 )
  {
    printf( "  +150V OVERVOLTAGE:                  OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  +150V OVERVOLTAGE:                  OVERVOLTAGE (%d)\n", s );
  }
  else
  {
    printf( "  +150V OVERVOLTAGE:                  UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->p_150v_undrvlt;
  if( s == 0 )
  {
    printf( "  +150V UNDERVOLTAGE:                 OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  +150V UNDERVOLTAGE:                 UNDERVOLTAGE (%d)\n", s );
  }
  else
  {
    printf( "  +150V UNDERVOLTAGE:                 UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->elev_srvo_amp_inhbt;
  if( s == 0 )
  {
    printf( "  ELEV SERVO AMP INHIBIT:             NORMAL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV SERVO AMP INHIBIT:             INHIBIT (%d)\n", s );
  }
  else
  {
    printf( "  ELEV SERVO AMP INHIBIT:             UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->elev_srvo_amp_shrt_crct;
  if( s == 0 )
  {
    printf( "  ELEV SERVO AMP SHORT CIR:           NORMAL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV SERVO AMP SHORT CIR:           SHORT CIRCUIT (%d)\n", s );
  }
  else
  {
    printf( "  ELEV SERVO AMP SHORT CIR:           UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->elev_srvo_amp_ovr_temp;
  if( s == 0 )
  {
    printf( "  ELEV SERVO AMP OVERTEMP:            NORMAL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV SERVO AMP OVERTEMP:            OVERTEMP (%d)\n", s );
  }
  else
  {
    printf( "  ELEV SERVO AMP OVERTEMP:            UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->elev_motor_ovr_temp;
  if( s == 0 )
  {
    printf( "  ELEV MOTOR OVERTEMP:                OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV MOTOR OVERTEMP:                OVERTEMP (%d)\n", s );
  }
  else
  {
    printf( "  ELEV MOTOR OVERTEMP:                NORMAL (%d)\n", s );
  }
  s = L2_pmd->elev_stow_pin;
  if( s == 0 )
  {
    printf( "  ELEV STOW PIN:                      OPERATIONAL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV STOW PIN:                      ENGAGED (%d)\n", s );
  }
  else
  {
    printf( "  ELEV STOW PIN:                      UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->elev_pcu_parity;
  if( s == 0 )
  {
    printf( "  ELEV PCU PARITY:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV PCU PARITY:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  ELEV PCU PARITY:                    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->elev_dead_lmt;
  if( s == 0 )
  {
    printf( "  ELEV DEAD LIMIT:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV DEAD LIMIT:                    IN LIMIT (%d)\n", s );
  }
  else
  {
    printf( "  ELEV DEAD LIMIT:                    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->elev_p_nrml_lmt;
  if( s == 0 )
  {
    printf( "  ELEV +NORMAL LIMIT:                 OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV +NORMAL LIMIT:                 IN LIMIT (%d)\n", s );
  }
  else
  {
    printf( "  ELEV +NORMAL LIMIT:                 UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->elev_n_nrml_lmt;
  if( s == 0 )
  {
    printf( "  ELEV -NORMAL LIMIT:                 OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV -NORMAL LIMIT:                 IN LIMIT (%d)\n", s );
  }
  else
  {
    printf( "  ELEV -NORMAL LIMIT:                 UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->elev_encdr_light;
  if( s == 0 )
  {
    printf( "  ELEV ENCODER LIGHT:                 FAIL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV ENCODER LIGHT:                 OK (%d)\n", s );
  }
  else
  {
    printf( "  ELEV ENCODER LIGHT:                 UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->elev_grbx_oil;
  if( s == 0 )
  {
    printf( "  ELEV GEARBOX OIL:                   OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV GEARBOX OIL:                   OIL LEVEL LOW (%d)\n", s );
  }
  else
  {
    printf( "  ELEV GEARBOX OIL:                   UNKNOWN (%d)\n", s );
  }

  s = L2_pmd->elev_handwheel;
  if( s == 0 )
  {
    printf( "  ELEV HANDWHEEL:                     OPERATIONAL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV HANDWHEEL:                     ENGAGED (%d)\n", s );
  }
  else
  {
    printf( "  ELEV HANDWHEEL:                     UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->elev_amp_ps;
  if( s == 0 )
  {
    printf( "  ELEV AMP PS:                        OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  ELEV AMP PS:                        FAIL (%d)\n", s );
  }
  else
  {  
    printf( "  ELEV AMP PS:                        UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->azmth_srvo_amp_inhbt;
  if( s == 0 )
  {
    printf( "  AZ SERVO AMP INHIBIT:               OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AZ SERVO AMP INHIBIT:               INHIBIT (%d)\n", s );
  }
  else
  {  
    printf( "  AZ SERVO AMP INHIBIT:               UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->azmth_srvo_amp_shrt_crct;
  if( s == 0 )
  {
    printf( "  AZ SERVO AMP SHORT CIR:             SHORT CIRCUIT (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AZ SERVO AMP SHORT CIR:             OK (%d)\n", s );
  }
  else
  {  
    printf( "  AZ SERVO AMP SHORT CIR:             UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->azmth_srvo_amp_ovr_temp;
  if( s == 0 )
  {
    printf( "  AZ SERVO OVERTEMP:                  OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AZ SERVO OVERTEMP:                  OVERTEMP (%d)\n", s );
  }
  else
  {
    printf( "  AZ SERVO OVERTEMP:                  UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->azmth_motor_ovr_temp;
  if( s == 0 )
  {
    printf( "  AZ MOTOR OVERTEMP:                  OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AZ MOTOR OVERTEMP:                  OVERTEMP (%d)\n", s );
  }
  else
  {
    printf( "  AZ MOTOR OVERTEMP:                  UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->azmth_stow_pin;
  if( s == 0 )
  {
    printf( "  AZ STOW PIN:                        OPERATIONAL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AZ STOW PIN:                        ENGAGED (%d)\n", s );
  }
  else
  {
    printf( "  AZ STOW PIN:                        UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->azmth_pcu_parity;
  if( s == 0 )
  {
    printf( "  AZ PCU PARITY:                      OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AZ PCU PARITY:                      FAIL (%d)\n", s );
  }
  else
  {
    printf( "  AZ PCU PARITY:                      UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->azmth_encdr_light;
  if( s == 0 )
  {
    printf( "  AZ ENCODER LIGHT:                   OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AZ ENCODER LIGHT:                   FAIL (%d)\n", s );
  }
  else
  {
    printf( "  AZ ENCODER LIGHT:                   UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->azmth_grbx_oil;
  if( s == 0 )
  {
    printf( "  AZ GEARBOX OIL:                     OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AZ GEARBOX OIL:                     OIL LEVEL LOW (%d)\n", s );
  }
  else
  {
    printf( "  AZ GEARBOX OIL:                     UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->azmth_bull_gr_oil;
  if( s == 0 )
  {
    printf( "  AZ BULL GEAR OIL:                   OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AZ BULL GEAR OIL:                   OIL LEVEL LOW (%d)\n", s );
  }
  else
  {
    printf( "  AZ BULL GEAR OIL:                   UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->azmth_handwheel;
  if( s == 0 )
  {
    printf( "  AZ HANDWHEEL:                       OPERATIONAL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AZ HANDWHEEL:                       ENGAGED (%d)\n", s );
  }
  else
  {
    printf( "  AZ HANDWHEEL:                       UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->azmth_srvo_amp_ps;
  if( s == 0 )
  {
    printf( "  AZ SERVO AMP PS:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AZ SERVO AMP PS:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  AZ SERVO AMP PS:                    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->srvo;
  if( s == 0 )
  {
    printf( "  SERVO:                              ON (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  SERVO:                              OFF (%d)\n", s );
  }
  else
  {
    printf( "  SERVO:                              UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->pdstl_intrlock_swtch;
  if( s == 0 )
  {
    printf( "  PED INTERLOCK SWITCH:               OPERATIONAL (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  PED INTERLOCK SWITCH:               SAFE (%d)\n", s );
  }
  else
  {
    printf( "  PED INTERLOCK SWITCH:               UNKNOWN (%d)\n", s );
  }
  printf( "  AZ POS CORRECTION:                  %5.3f deg\n", Decode_angle( L2_pmd->azmth_pos_correction ) );
  printf( "  ELEV POS CORRECTION:                %5.3f deg\n", Decode_angle( L2_pmd->elev_pos_correction ) );
  s = L2_pmd->slf_tst_1_status;
  if( s == 1 )
  {
    printf( "  SELF TEST 1 STATUS:                 NO (%d)\n", s );
  }
  else if( s == 2 )
  {
    printf( "  SELF TEST 1 STATUS:                 OK (%d)\n", s );
  }
  else if( s == 3 )
  {
    printf( "  SELF TEST 1 STATUS:                 FAIL (%d)\n", s );
  }
  else
  {
    printf( "  SELF TEST 1 STATUS:                 UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->slf_tst_2_status;
  if( s == 1 )
  {
    printf( "  SELF TEST 2 STATUS:                 NO (%d)\n", s );
  }
  else if( s == 2 )
  {
    printf( "  SELF TEST 2 STATUS:                 OK (%d)\n", s );
  }
  else if( s == 3 )
  {
    printf( "  SELF TEST 2 STATUS:                 FAIL (%d)\n", s );
  }
  else
  {
    printf( "  SELF TEST 2 STATUS:                 UNKNOWN (%d)\n", s );
  }
  printf( "  SELF TEST 2 DATA:                   %d\n", L2_pmd->slf_tst_2_data );
  for( i = 0; i < 7; i++ )
  {
/*
    printf( "  SPARE-334[%1d]:                       %d\n", i, L2_pmd->spare334[i] );
*/
  }
  printf( "RECEIVER\n" );
  s = L2_pmd->coho_clock;
  if( s == 0 )
  {
    printf( "  COHO/CLOCK:                         OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  COHO/CLOCK:                         FAIL (%d)\n", s );
  }
  else
  {
    printf( "  COHO/CLOCK:                         UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->rf_gnrtr_freq_slct_osc;
  if( s == 0 )
  {
    printf( "  RF GEN FREQ SEL OSCILL:             OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  RF GEN FREQ SEL OSCILL:             FAIL (%d)\n", s );
  }
  else
  {
    printf( "  RF GEN FREQ SEL OSCILL:             UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->rf_gnrtr_rf_stalo;
  if( s == 0 )
  {
    printf( "  RF GEN RF/STALO:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  RF GEN RF/STALO:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  RF GEN RF/STALO:                    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->rf_gnrtr_phase_shft_coho;
  if( s == 0 )
  {
    printf( "  RF GEN PHASE SHIFT COHO:            OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  RF GEN PHASE SHIFT COHO:            FAIL (%d)\n", s );
  }
  else
  {
    printf( "  RF GEN PHASE SHIFT COHO:            UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->p_9v_rcvr_ps;
  if( s == 0 )
  {
    printf( "  +9v RECV PS:                        OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  +9v RECV PS:                        FAIL (%d)\n", s );
  }
  else
  {
    printf( "  +9v RECV PS:                        UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->p_5v_rcvr_ps;
  if( s == 0 )
  {
    printf( "  +5v RECV PS:                        OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  +5v RECV PS:                        FAIL (%d)\n", s );
  }
  else
  {
    printf( "  +5v RECV PS:                        UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->pn_18v_rcvr_ps;
  if( s == 0 )
  {
    printf( "  +/-18v RECV PS:                     OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  +/-18v RECV PS:                     FAIL (%d)\n", s );
  }
  else
  {
    printf( "  +/-18v RECV PS:                     UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->n_9v_rcvr_ps;
  if( s == 0 )
  {
    printf( "  -9v RECV PS:                        OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  -9v RECV PS:                        FAIL (%d)\n", s );
  }
  else
  {
    printf( "  -9v RECV PS:                        UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->p_5v_rcvr_prtctr_ps;
  if( s == 0 )
  {
    printf( "  +5v RECV PROTECT PS:                OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  +5v RECV PROTECT PS:                FAIL (%d)\n", s );
  }
  else
  {
    printf( "  +5v RECV PROTECT PS:                UNKNOWN (%d)\n", s );
  }
/*
  printf( "  SPARE-350:                          %d\n", L2_pmd->spare350 );
*/
  printf( "  HORIZ SHORT PULSE NOISE:            %7.2f dBm\n", L2_pmd->h_shrt_pulse_noise );
  printf( "  HORIZ LONG PULSE NOISE:             %7.2f dBm\n", L2_pmd->h_long_pulse_noise );
  printf( "  HORIZ NOISE TEMP:                   %7.2f K\n", L2_pmd->h_noise_temp );
  printf( "  VERT SHORT PULSE NOISE:             %f\n", L2_pmd->v_shrt_pulse_noise );
  printf( "  VERT LONG PULSE NOISE:              %7.2f dBm\n", L2_pmd->v_long_pulse_noise );
  printf( "  VERT NOISE TEMP:                    %7.2f K\n", L2_pmd->v_noise_temp );
  printf( "CALIBRATION\n" );
  printf( "  HORIZ LINEARITY:                    %6.4f\n", L2_pmd->h_linearity );
  printf( "  HORIZ DYN RANGE:                    %7.3f dB\n", L2_pmd->h_dynamic_range );
  printf( "  HORIZ ddBZ0:                        %7.2f dB\n", L2_pmd->h_delta_dbz0 );
  printf( "  VERT ddBZ0:                         %7.2f dB\n", L2_pmd->v_delta_dbz0 );
  printf( "  KD PEAK MEASURED:                   %6.2f dBm\n", L2_pmd->kd_peak_measured );
  for( i = 0; i < 2; i++ )
  {
/*
    printf( "  SPARE-373[%1d]:                       %d\n", i, L2_pmd->spare373[i] );
*/
  }
  printf( "  HORIZ SHORT PULSE ddBZ0:            %8.4f dBZ\n", L2_pmd->shrt_pls_h_dbz0 );
  printf( "  HORIZ LONG PULSE ddBZ0:             %8.4f dBZ\n", L2_pmd->long_pls_h_dbz0 );
  s = L2_pmd->velocity_prcssd;
  if( s == 0 )
  {
    printf( "  VEL PROCESSED:                      GOOD (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  VEL PROCESSED:                      FAIL (%d)\n", s );
  }
  else
  {
    printf( "  VEL PROCESSED:                      UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->width_prcssd;
  if( s == 0 )
  {
    printf( "  SPW PROCESSED:                      GOOD (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  SPW PROCESSED:                      FAIL (%d)\n", s );
  }
  else
  {
    printf( "  SPW PROCESSED:                      UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->velocity_rf_gen;
  if( s == 0 )
  {
    printf( "  VEL RF GEN:                         GOOD (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  VEL RF GEN:                         FAIL (%d)\n", s );
  }
  else
  {
    printf( "  VEL RF GEN:                         UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->width_rf_gen;
  if( s == 0 )
  {
    printf( "  SPW RF GEN:                         GOOD (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  SPW RF GEN:                         FAIL (%d)\n", s );
  }
  else
  {
    printf( "  SPW RF GEN:                         UNKNOWN (%d)\n", s );
  }
  printf( "  HORIZ I0:                           %9.4f dBm\n", L2_pmd->h_i_naught );
  printf( "  VERT I0:                            %9.4f dBm\n", L2_pmd->v_i_naught );
  printf( "  VERT DYN RANGE:                     %7.3f dB\n", L2_pmd->v_dynamic_range );
  printf( "  VERT SHORT PULSE ddBZ0:             %8.4f dBZ\n", L2_pmd->shrt_pls_v_dbz0 );
  printf( "  VERT LONG PULSE ddBZ0:              %8.4f dBZ\n", L2_pmd->long_pls_v_dbz0 );
  for( i = 0; i < 4; i++ )
  {
/*
    printf( "  SPARE-393[%1d]:                       %d\n", i, L2_pmd->spare393[i] );
*/
  }
  printf( "  HORIZ PWR SENSE:                    %9.4f dBm\n", L2_pmd->h_pwr_sense );
  printf( "  VERT PWR SENSE:                     %9.4f dBm\n", L2_pmd->v_pwr_sense );
  printf( "  ZDR BIAS:                           %9.4f dB\n", L2_pmd->zdr_bias );
  for( i = 0; i < 6; i++ )
  {
/*
    printf( "  SPARE-385[%1d]:                       %d\n", i, L2_pmd->spare385[i] );
*/
  }
  printf( "  HORIZ CLUTT SUPP DELTA:             %6.2f dB\n", L2_pmd->h_cltr_supp_delta );
  printf( "  HORIZ CLUTT SUPP UNFILT PWR:        %6.2f dBZ\n", L2_pmd->h_cltr_supp_ufilt_pwr );
  printf( "  HORIZ CLUTT SUPP FILT PWR:          %6.2f dBZ\n", L2_pmd->h_cltr_supp_filt_pwr );
  printf( "  TRANSMIT BURST PWR:                 %6.2f dBm\n", L2_pmd->trsmit_brst_pwr );
  printf( "  TRANSMIT BURST PHASE:               %6.2f deg\n", L2_pmd->trsmit_brst_phase );
  for( i = 0; i < 6; i++ )
  {
/*
    printf( "  SPARE-419[%1d]:                       %d\n", i, L2_pmd->spare419[i] );
*/
  }
  printf( "  VERT LINEARITY:                     %6.4f\n", L2_pmd->v_linearity );
  for( i = 0; i < 4; i++ )
  {
/*
    printf( "  SPARE-427[%1d]:                       %d\n", i, L2_pmd->spare427[i] );
*/
  }
  printf( "FILE STATUS\n" );
  s = L2_pmd->state_file_rd_stat;
  if( s == 0 )
  {
    printf( "  STATE FILE READ STATUS:             OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  STATE FILE READ STATUS:             FAIL (%d)\n", s );
  }
  else
  {
    printf( "  STATE FILE READ STATUS:             UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->state_file_wrt_stat;
  if( s == 0 )
  {
    printf( "  STATE FILE WRITE STATUS:            OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  STATE FILE WRITE STATUS:            FAIL (%d)\n", s );
  }
  else
  {
    printf( "  STATE FILE WRITE STATUS:            UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->bypass_map_file_rd_stat;
  if( s == 0 )
  {
    printf( "  BYP MAP READ STATUS:                OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  BYP MAP READ STATUS:                FAIL (%d)\n", s );
  }
  else
  {
    printf( "  BYP MAP READ STATUS:                UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->bypass_map_file_wrt_stat;
  if( s == 0 )
  {
    printf( "  BYP MAP WRITE STATUS:               OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  BYP MAP WRITE STATUS:               FAIL (%d)\n", s );
  }
  else
  {
    printf( "  BYP MAP WRITE STATUS:               UNKNOWN (%d)\n", s );
  }
/*
  printf( "  SPARE-435:                          %d\n", L2_pmd->spare435 );
  printf( "  SPARE-436:                          %d\n", L2_pmd->spare436 );
*/
  s = L2_pmd->crnt_adpt_file_rd_stat;
  if( s == 0 )
  {
    printf( "  CURR ADAPT FILT READ STATUS:        OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  CURR ADAPT FILT READ STATUS:        FAIL (%d)\n", s );
  }
  else
  {
    printf( "  CURR ADAPT FILT READ STATUS:        UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->crnt_adpt_file_wrt_stat;
  if( s == 0 )
  {
    printf( "  CURR ADAPT FILT WRITE STATUS:       OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  CURR ADAPT FILT WRITE STATUS:       FAIL (%d)\n", s );
  }
  else
  {
    printf( "  CURR ADAPT FILT WRITE STATUS:       UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->cnsr_zn_file_rd_stat;
  if( s == 0 )
  {
    printf( "  CCZ FILE READ STATUS:               OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  CCZ FILE READ STATUS:               FAIL (%d)\n", s );
  }
  else
  {
    printf( "  CCZ FILE READ STATUS:               UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->cnsr_zn_file_wrt_stat;
  if( s == 0 )
  {
    printf( "  CCZ FILE WRITE STATUS:              OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  CCZ FILE WRITE STATUS:              FAIL (%d)\n", s );
  }
  else
  {
    printf( "  CCZ FILE WRITE STATUS:              UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->rmt_vcp_file_rd_stat;
  if( s == 0 )
  {
    printf( "  REMOTE VCP FILE READ STATUS:        OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  REMOTE VCP FILE READ STATUS:        FAIL (%d)\n", s );
  }
  else
  {
    printf( "  REMOTE VCP FILE READ STATUS:        UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->rmt_vcp_file_wrt_stat;
  if( s == 0 )
  {
    printf( "  REMOTE VCP FILE WRITE STATUS:       OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  REMOTE VCP FILE WRITE STATUS:       FAIL (%d)\n", s );
  }
  else
  {
    printf( "  REMOTE VCP FILE WRITE STATUS:       UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->bl_adpt_file_rd_stat;
  if( s == 0 )
  {
    printf( "  BASELINE ADAPT FILE READ STATUS:    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  BASELINE ADAPT FILE READ STATUS:    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  BASELINE ADAPT FILE READ STATUS:    UNKNOWN (%d)\n", s );
  }
/*
  printf( "  SPARE-444:                          %d\n", L2_pmd->spare444 );
*/
  s = L2_pmd->cf_map_file_rd_stat;
  if( s == 0 )
  {
    printf( "  CLUTT FILT MAP FILE READ STATUS:    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  CLUTT FILT MAP FILE READ STATUS:    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  CLUTT FILT MAP FILE READ STATUS:    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->cf_map_file_wrt_stat;
  if( s == 0 )
  {
    printf( "  CLUTT FILT MAP FILE WRITE STATUS:   OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  CLUTT FILT MAP FILE WRITE STATUS:   FAIL (%d)\n", s );
  }
  else
  {
    printf( "  CLUTT FILT MAP FILE WRITE STATUS:   UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->gnrl_disk_io_err;
  if( s == 0 )
  {
    printf( "  GENERAL DISK I/O ERROR:             OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  GENERAL DISK I/O ERROR:             FAIL (%d)\n", s );
  }
  else
  {
    printf( "  GENERAL DISK I/O ERROR:             UNKNOWN (%d)\n", s );
  }
  for( i = 0; i < 13; i++ )
  {
/*
    printf( "  SPARE-448[%02d]:                      %d\n", i, L2_pmd->spare448[i] );
*/
  }
  printf( "DEVICE STATUS\n" );
  s = L2_pmd->dau_comm_stat;
  if( s == 0 )
  {
    printf( "  DAU COMM STATUS:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  DAU COMM STATUS:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  DAU COMM STATUS:                    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->hci_comm_stat;
  if( s == 0 )
  {
    printf( "  HCI COMM STATUS:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  HCI COMM STATUS:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  HCI COMM STATUS:                    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->pdstl_comm_stat;
  if( s == 0 )
  {
    printf( "  PED COMM STATUS:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  PED COMM STATUS:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  PED COMM STATUS:                    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->sgnl_prcsr_comm_stat;
  if( s == 0 )
  {
    printf( "  SIG PROC COMM STATUS:               OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  SIG PROC COMM STATUS:               FAIL (%d)\n", s );
  }
  else
  {
    printf( "  SIG PROC COMM STATUS:               UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->ame_comm_stat;
  if( s == 0 )
  {
    printf( "  AME COMM STATUS:                    OK (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  AME COMM STATUS:                    FAIL (%d)\n", s );
  }
  else
  {
    printf( "  AME COMM STATUS:                    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->rms_lnk_stat;
  if( s == 0 )
  {
    printf( "  RMS LINK STATUS:                    CONNECTED (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  RMS LINK STATUS:                    NOT CONNECTED (%d)\n", s );
  }
  else
  {
    printf( "  RMS LINK STATUS:                    UNKNOWN (%d)\n", s );
  }
  s = L2_pmd->rpg_lnk_stat;
  if( s == 0 )
  {
    printf( "  RPG LINK STATUS:                    CONNECTED (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  RPG LINK STATUS:                    NOT CONNECTED (%d)\n", s );
  }
  else
  {
    printf( "  RPG LINK STATUS:                    UNKNOWN (%d)\n", s );
  }
/*
  printf( "  SPARE-468:                          %d\n", L2_pmd->spare468[0] );
*/
  printf( "  PERF CHECK TIME:                    %ld sec\n", (long) L2_pmd->perf_check_time );
  for( i = 0; i < 10; i++ )
  {
/*
    printf( "  SPARE-471[%02d]:                      %d\n", i, L2_pmd->spare471[i] );
*/
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print all RDA Status messages.
 ************************************************************************/

int libL2_print_rda_status_msgs()
{
  int i = 0;

  for( i = 0; i < libL2_num_status_msgs(); i++ )
  {
    libL2_print_rda_status_msg( i );
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print specified RDA Status message.
 ************************************************************************/

#define RDA_BUILD_SCALE	100

int libL2_print_rda_status_msg( int i )
{
  int j = 0;
  unsigned short us = 0;
  short s = 0;
  char *buf = NULL;
  int pos = 0;

  /* rda_status.h                      */
  /* typedef struct {                  */
  /* RDA_RPG_message_header_t msg_hdr; */
  /* ....                              */
  /* } ORDA_status_msg_t;              */

  printf( "RDA STATUS MSG (MSG TYPE 2) NUMBER %d\n", i+1 );

  us = L2_status_msgs[i]->rda_status;
  if( us == RS_STARTUP )
  {
    printf( "  RDA STATUS:     STARTUP (%d)\n", us );
  }
  else if( us == RS_STANDBY )
  {
    printf( "  RDA STATUS:     STANDBY (%d)\n", us );
  }
  else if( us == RS_RESTART )
  {
    printf( "  RDA STATUS:     RESTART (%d)\n", us );
  }
  else if( us == RS_OPERATE )
  {
    printf( "  RDA STATUS:     OPERATE (%d)\n", us );
  }
  else if( us == RS_OFFOPER )
  {
    printf( "  RDA STATUS:     OFFLINE OPERATE (%d)\n", us );
  }
  else
  {
    printf( "  RDA STATUS:     UNKNOWN (%d)\n", us );
  }

  us = L2_status_msgs[i]->op_status;
  if( us == OS_INDETERMINATE )
  {
    printf( "  OP STATUS:      INDETERMINATE (%d)\n", us );
  }
  else if( us == OS_ONLINE )
  {
    printf( "  OP STATUS:      ONLINE (%d)\n", us );
  }
  else if( us == OS_MAINTENANCE_REQ )
  {
    printf( "  OP STATUS:      MAINTENANCE REQUIRED (%d)\n", us );
  }
  else if( us == OS_MAINTENANCE_MAN )
  {
    printf( "  OP STATUS:      MAINTENANCE MANDATORY (%d)\n", us );
  }
  else if( us == OS_COMMANDED_SHUTDOWN )
  {
    printf( "  OP STATUS:      COMMANDED SHUTDOWN (%d)\n", us );
  }
  else if( us == OS_INOPERABLE )
  {
    printf( "  OP STATUS:      INOPERABLE (%d)\n", us );
  }
  else if( us == OS_WIDEBAND_DISCONNECT )
  {
    printf( "  OP STATUS:      WIDEBAND DISCONNECT (%d)\n", us );
  }
  else
  {
    printf( "  OP STATUS:      UNKNOWN (%d)\n", us );
  }

  us = L2_status_msgs[i]->control_status;
  if( us == CS_LOCAL_ONLY )
  {
    printf( "  CONTROL STATUS: LOCAL (%d)\n", us );
  }
  else if( us == CS_RPG_REMOTE )
  {
    printf( "  CONTROL STATUS: REMOTE (%d)\n", us );
  }
  else if( us == CS_EITHER )
  {
    printf( "  CONTROL STATUS: EITHER (%d)\n", us );
  }
  else
  {
    printf( "  CONTROL STATUS: UNKNOWN (%d)\n", us );
  }

  us = L2_status_msgs[i]->aux_pwr_state;
  if( us == RS_COMMANDED_SWITCHOVER )
  {
    printf( "  AUX PWR STATE:  COMMANDED SWITCHOVER (%d)\n", us );
  }
  else if( us == AP_UTILITY_PWR_AVAIL )
  {
    printf( "  AUX PWR STATE:  UTILITY POWER AVAILABLE (%d)\n", us );
  }
  else if( us == AP_GENERATOR_ON )
  {
    printf( "  AUX PWR STATE:  GENERATOR ON (%d)\n", us );
  }
  else if( us == AP_TRANS_SWITCH_MAN )
  {
    printf( "  AUX PWR STATE:  TRANSITIONING MANUAL SWITCH (%d)\n", us );
  }
  else if( us == AP_COMMAND_SWITCHOVER )
  {
    printf( "  AUX PWR STATE:  AUX POWER COMMAND SWITCHOVER (%d)\n", us );
  }
  else if( us == AP_SWITCH_AUX_PWR )
  {
    printf( "  AUX PWR STATE:  SWITCH TO AUX POWER (%d)\n", us );
  }
  else
  {
    printf( "  AUX PWR STATE:  UNKNOWN (%d)\n", us );
  }

  printf( "  AVG TRANS PWR:  %d W\n", L2_status_msgs[i]->ave_trans_pwr );

  s = L2_status_msgs[i]->ref_calib_corr;
  printf( "  HOR CALIB COR:  %-7.2f dB (%d)\n", (float)(s/100.0), s );

  us = L2_status_msgs[i]->data_trans_enbld;
  if( ( buf = malloc( 256 * sizeof( char ) ) ) == NULL )
  {
    return LIBL2_MALLOC_FAILED;
  }
  pos = 0;
  if( us & BD_ENABLED_NONE )
  {
    strcpy( &buf[pos], "NONE" );
    pos += strlen( "NONE" );
  }
  if( us & BD_REFLECTIVITY )
  {
    if( pos == 0 )
    {
      strcpy( &buf[pos], "REF" );
      pos += strlen( "REF" );
    }
    else
    {
      strcpy( &buf[pos], ",REF" );
      pos += strlen( ",REF" );
    }
  }
  if( us & BD_VELOCITY )
  {
    if( pos == 0 )
    {
      strcpy( &buf[pos], "VEL" );
      pos += strlen( "VEL" );
    }
    else
    {
      strcpy( &buf[pos], ",VEL" );
      pos += strlen( ",VEL" );
    }
  }
  if( us & BD_WIDTH )
  {
    if( pos == 0 )
    {
      strcpy( &buf[pos], "SPW" );
      pos += strlen( "SPW" );
    }
    else
    {
      strcpy( &buf[pos], ",SPW" );
      pos += strlen( ",SPW" );
    }
  }
  if( pos == 0 )
  {
    strcpy( &buf[pos], "UNKNOWN" );
  }
  printf( "  DATA ENABLED:   %s (%d)\n", buf, us );
  free( buf );
  buf = NULL;

  s = L2_status_msgs[i]->vcp_num;
  if ( s < 0 )
  {
    printf( "  VCP:            %d (LOCAL)\n", s*-1 );
  }
  else
  {
    printf( "  VCP:            %d (REMOTE)\n", s );
  }

  us = L2_status_msgs[i]->rda_control_auth;
  if( us == CA_NO_ACTION )
  {
    printf( "  RDA CONT AUTH:  NO ACTION (%d)\n", us );
  }
  else if( us == CA_LOCAL_CONTROL_REQUESTED )
  {
    printf( "  RDA CONT AUTH:  LOCAL CONTROL REQUESTED (%d)\n", us );
  }
  else if( us == CA_REMOTE_CONTROL_ENABLED )
  {
    printf( "  RDA CONT AUTH:  REMOTE CONTROL ENABLED (%d)\n", us );
  }
  else
  {
    printf( "  RDA CONT AUTH:  UNKNOWN (%d)\n", us );
  }

  s = L2_status_msgs[i]->rda_build_num;
  printf( "  RDA BUILD NUM:  %5.2f (%d)\n", (float)(s/RDA_BUILD_SCALE), s );

  us = L2_status_msgs[i]->op_mode;
  if( us == OP_MAINTENANCE_MODE )
  {
    printf( "  OP MODE:        MAINTENANCE (%d)\n", us );
  }
  else if( us == OP_OPERATIONAL_MODE )
  {
    printf( "  OP MODE:        OPERATIONAL (%d)\n", us );
  }
  else if( us == OP_OFFLINE_MAINTENANCE_MODE )
  {
    printf( "  OP MODE:        OFFLINE MAINTENANCE (%d)\n", us );
  }
  else
  {
    printf( "  OP MODE:        UNKNOWN (%d)\n", us );
  }


  us = L2_status_msgs[i]->super_res;
  if( us == SR_NOCHANGE )
  {
    printf( "  SUPER RES:      NO CHANGE (%d)\n", us );
  }
  else if( us == SR_ENABLED )
  {
    printf( "  SUPER RES:      ENABLED (%d)\n", us );
  }
  else if( us == SR_DISABLED )
  {
    printf( "  SUPER RES:      DISABLED (%d)\n", us );
  }
  else
  {
    printf( "  SUPER RES:      UNKNOWN (%d)\n", us );
  }

  us = L2_status_msgs[i]->cmd;
  if( us == CMD_ENABLED )
  {
    printf( "  CMD:            ENABLED (%d)\n", us );
  }
  else if( us == CMD_DISABLED )
  {
    printf( "  CMD:            DISABLED (%d)\n", us );
  }
  else
  {
    printf( "  CMD:            UNKNOWN (%d)\n", us );
  }

  us = L2_status_msgs[i]->avset;
 if( us == AVSET_ENABLED )
  {
    printf( "  AVSET:          ENABLED (%d)\n", us );
  }
  else if( us == AVSET_DISABLED )
  {
    printf( "  AVSET:          DISABLED (%d)\n", us );
  }
  else
  {
    printf( "  AVSET:          UNKNOWN (%d)\n", us );
  }

  us = L2_status_msgs[i]->rda_alarm;
  if( ( buf = malloc( 256 * sizeof( char ) ) ) == NULL )
  {
    return LIBL2_MALLOC_FAILED;
  }
  pos = 0;
  if( us == AS_NO_ALARMS )
  {
    strcpy( &buf[pos], "NONE" );
    pos += strlen( "NONE" );
  }
  else
  {
    if( us & AS_TOW_UTIL )
    {
      strcpy( &buf[pos], "TOWER/UTIL" );
      pos += strlen( "TOWER/UTIL" );
    }
    if( us & AS_PEDESTAL )
    {
      if( pos == 0 )
      {
        strcpy( &buf[pos], "PED" );
        pos += strlen( "PED" );
      }
      else
      {
        strcpy( &buf[pos], ",PED" );
        pos += strlen( ",PED" );
      }
    }
    if( us & AS_TRANSMITTER )
    {
      if( pos == 0 )
      {
        strcpy( &buf[pos], "TRANS" );
        pos += strlen( "TRANS" );
      }
      else
      {
        strcpy( &buf[pos], ",TRANS" );
        pos += strlen( ",TRANS" );
      }
    }
    if( us & AS_RECV )
    {
      if( pos == 0 )
      {
        strcpy( &buf[pos], "REC" );
        pos += strlen( "REC" );
      }
      else
      {
        strcpy( &buf[pos], ",REC" );
        pos += strlen( ",REC" );
      }
    }
    if( us & AS_RDA_CONTROL )
    {
      if( pos == 0 )
      {
        strcpy( &buf[pos], "RDA CONTROL" );
        pos += strlen( "RDA CONTROL" );
      }
      else
      {
        strcpy( &buf[pos], ",RDA CONTROL" );
        pos += strlen( ",RDA CONTROL" );
      }
    }
    if( us & AS_RPG_COMMUN )
    {
      if( pos == 0 )
      {
        strcpy( &buf[pos], "RPG COMM" );
        pos += strlen( "RPG COMM" );
      }
      else
      {
        strcpy( &buf[pos], ",RPG COMM" );
        pos += strlen( ",RPG COMM" );
      }
    }
    if( us & AS_SIGPROC )
    {
      if( pos == 0 )
      {
        strcpy( &buf[pos], "SIG PROC" );
        pos += strlen( "SIG PROC" );
      }
      else
      {
        strcpy( &buf[pos], ",SIG PROC" );
        pos += strlen( ",SIG PROC" );
      }
    }
  }
  if( pos == 0 )
  {
    strcpy( &buf[pos], "UNKNOWN" );
  }
  printf( "  RDA ALARM SUMM: %s (%d)\n", buf, us );
  free( buf );
  buf = NULL;

  us = L2_status_msgs[i]->command_status;
  if( us == RS_NO_ACKNOWLEDGEMENT )
  {
    printf( "  CMD STATUS:     NO ACKNOWLEDGEMENT (%d)\n", us );
  }
  else if( us == RS_REMOTE_VCP_RECEIVED )
  {
    printf( "  CMD STATUS:     REMOTE VCP RECEIVED (%d)\n", us );
  }
  else if( us == RS_CLUTTER_BYPASS_MAP_RECEIVED )
  {
    printf( "  CMD STATUS:     CLUTTER BYPASS MAP RECEIVED (%d)\n", us );
  }
  else if( us == RS_CLUTTER_CENSOR_ZONES_RECEIVED )
  {
    printf( "  CMD STATUS:     CLUTTER CENSOR ZONES RECEIVED (%d)\n", us );
  }
  else if( us == RS_REDUND_CHNL_STBY_CMD_ACCEPTED )
  {
    printf( "  CMD STATUS:     RED CHANNEL STANDBY CMD ACCEPTED (%d)\n", us );
  }
  else
  {
    printf( "  CMD STATUS:     UNKNOWN (%d)\n", us );
  }

  us = L2_status_msgs[i]->channel_status;
  if( us == RDA_IS_CONTROLLING )
  {
    printf( "  CHANNEL STATUS: CONTROLLING (%d)\n", us );
  }
  else if( us == RDA_IS_NON_CONTROLLING )
  {
    printf( "  CHANNEL STATUS: NON-CONTROLLING (%d)\n", us );
  }
  else
  {
    printf( "  CHANNEL STATUS: UNKNOWN (%d)\n", us );
  }

  us = L2_status_msgs[i]->spot_blanking_status;
  if( us == SB_NOT_INSTALLED )
  {
    printf( "  SPOTBLK STATUS: NOT INSTALLED (%d)\n", us );
  }
  else if( us == SB_ENABLED )
  {
    printf( "  SPOTBLK STATUS: ENABLED (%d)\n", us );
  }
  else if( us == SB_DISABLED )
  {
    printf( "  SPOTBLK STATUS: DISABLED (%d)\n", us );
  }
  else
  {
    printf( "  SPOTBLK STATUS: UNKNOWN (%d)\n", us );
  }

  printf( "  BYPASS DATE:    %s (%05d)\n", Convert_date( L2_status_msgs[i]->bypass_map_date ), L2_status_msgs[i]->bypass_map_date );
  /* Time is minutes past midnight. Convert to milliseconds for string. */
  printf( "  BYPASS TIME:    %s (%05d)\n", Convert_milliseconds( L2_status_msgs[i]->bypass_map_time * 60 * 1000 ), L2_status_msgs[i]->bypass_map_time );
  printf( "  CLUTTER DATE:   %s (%05d)\n", Convert_date( L2_status_msgs[i]->clutter_map_date ), L2_status_msgs[i]->clutter_map_date );
  /* Time is minutes past midnight. Convert to milliseconds for string. */
  printf( "  CLUTTER TIME:   %s (%05d)\n", Convert_milliseconds( L2_status_msgs[i]->clutter_map_time * 60 * 1000 ), L2_status_msgs[i]->clutter_map_time );

  s = L2_status_msgs[i]->vc_ref_calib_corr;
  printf( "  VERT CALIB COR: %-7.2f dB (%d)\n", (float)(s/100.0), s );

  us = L2_status_msgs[i]->tps_status;
  if( us == TP_NOT_INSTALLED )
  {
    printf( "  TPS STATUS:     NOT INSTALLED (%d)\n", us );
  }
  else if( us == TP_OFF )
  {
    printf( "  TPS STATUS:     OFF (%d)\n", us );
  }
  else if( us == TP_OK )
  {
    printf( "  TPS STATUS:     ON (%d)\n", us );
  }
  else
  {
    printf( "  TPS STATUS:     UNKNOWN (%d)\n", us );
  }

  us = L2_status_msgs[i]->rms_control_status;
  if( us == 0 )
  {
    printf( "  RMS CONTROL:    NON-RMS SYSTEM (%d)\n", us );
  }
  else if( us == 2 )
  {
    printf( "  RMS CONTROL:    RMS IN CONTROL (%d)\n", us );
  }
  else if( us == 4 )
  {
    printf( "  RMS CONTROL:    RDA MMI IN CONTROL (%d)\n", us );
  }
  else
  {
    printf( "  RMS CONTROL:    UNKNOWN (%d)\n", us );
  }

  s = L2_status_msgs[i]->perf_check_status;
  if( s == 0 )
  {
    printf( "  PERF CHK STAT   AUTOMATIC (%d)\n", s );
  }
  else if( s == 1 )
  {
    printf( "  PERF CHK STAT   PENDING (%d)\n", s );
  }
  else
  {
    printf( "  PERF CHK STAT   UNKNOWN (%d)\n", s );
  }

  for( j = 0; j < MAX_RDA_ALARMS_PER_MESSAGE; j++ )
  {
    printf( "  ALARM%02d:        %d\n", j+1, L2_status_msgs[i]->alarm_code[j] );
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print message header.
 ************************************************************************/

int libL2_print_msg_header( int cut_index, int radial_number )
{
  return Print_msg( cut_index, radial_number, 0, PRINT_MSG_HDR );
}

/************************************************************************
 Description: Print base data header.
 ************************************************************************/

int libL2_print_base_header( int cut_index, int radial_number )
{
  return Print_msg( cut_index, radial_number, 0, PRINT_BDH );
}

/************************************************************************
 Description: Print moment header.
 ************************************************************************/

int libL2_print_moment_header( int cut_index, int radial_number, int moment_index )
{
  return Print_msg( cut_index, radial_number, moment_index, PRINT_MOMENT_HDR );
}

/************************************************************************
 Description: Print base data.
 ************************************************************************/

int libL2_print_base_data( int cut_index, int radial_number, int moment_index )
{
  return Print_msg( cut_index, radial_number, moment_index, PRINT_BASE_DATA );
}

/************************************************************************
 Description: Print specified part of message.
 ************************************************************************/

int Print_msg( int cut_index, int radial_number, int moment_index, int print_flag )
{
  static char *fx = "libL2_print_msg";
  int ret = LIBL2_NO_ERROR;
  int i = 0;
  int msg_size = 0;
  int type = 0;
  int offset = 0;
  int ci = 0;
  int rn = 0;
  int mi = 0;
  char *ptr = NULL;
  char temp_buf[MAX_ICAO_LEN+1];
  libL2_base_t *bd = NULL;
  libL2_base_hdr_t *bdh = NULL;
  libL2_msg_hdr_t *msg_header = NULL;
  libL2_any_t *dblk = NULL;

  /* Initialize temp_buf. It will hold name of moment. */
  memset( temp_buf, 0, MAX_ICAO_LEN+1 );
  temp_buf[MAX_ICAO_LEN] = '\0';

  /* Ensure passed in arguments are valid */
  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( ( ret = Validate_moment_index( moment_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_moment_index returns %d", fx, ret );
    return ret;
  }
  else if( ( ret = Validate_radial_number( cut_index, radial_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_radial_number returns %d", fx, ret );
    return ret;
  }

  /* Offset depends on if a single or all cuts will be output */
  if( cut_index == LIBL2_ALL_CUTS )
  {
    offset = L2_elevation_offsets[0];
  }
  else
  {
    offset = L2_elevation_offsets[cut_index];
  }

  /* Loop over base data and extract whatever is needed */
  while( offset < L2_uncompress_size )
  {
    ptr = &L2_buf[offset+COMMS_HEADER_SIZE];
    msg_header = (libL2_msg_hdr_t*)ptr;
    msg_size = msg_header->size*sizeof( unsigned short );
    type = msg_header->type;
    if( type == RDA_RADIAL_MSG )
    {
      offset += ( msg_size + COMMS_HEADER_SIZE);
      bd = (libL2_base_t *) ptr;
      bdh = &bd->base;
      if( cut_index == LIBL2_ALL_CUTS || bdh->elev_num-1 == cut_index )
      {
        ci = bdh->elev_num-1;
        if( radial_number == LIBL2_ALL_RADIALS || bdh->azi_num == radial_number )
        {
          rn = bdh->azi_num;
          if( print_flag == PRINT_MSG_HDR )
          {
            printf( "MESSAGE HEADER FOR CUT: %02d RADIAL: %03d\n", ci, rn );
            Print_msg_header( msg_header );
          }
          if( print_flag == PRINT_BDH )
          {
            printf( "GENERIC BASEDATA HEADER CUT: %02d RADIAL: %03d\n", ci, rn );
            Print_base_data_header( bdh );
          }
          for( i = 0; i < bdh->no_of_datum; i++ )
          {
            /* Skip RVOL/RELV/RRAD data blocks */
            dblk = (libL2_any_t *) (ptr+sizeof(libL2_msg_hdr_t)+bdh->data[i] );
            memcpy( &temp_buf[0], dblk->name, sizeof( dblk->name ) );
            if( ( mi = Get_moment_index_from_string( temp_buf ) ) < 0 )
            {
              /* Skip non-base data blocks */
              libL2_debug( "%s: Skipping %s", fx, temp_buf );
              continue;
            }
            else if( moment_index == LIBL2_ALL_MOMENTS  ||
                     mi == moment_index )
            {
              if( print_flag == PRINT_MOMENT_HDR )
              {
                printf( "GENERIC MOMENT HEADER - MOMENT: %s CUT: %02d RADIAL: %03d\n", Get_moment_index_string( mi ), ci, rn );
                Print_moment_header( (libL2_moment_t *) dblk );
              }
              if( print_flag == PRINT_BASE_DATA )
              {
                printf( "BASE DATA %s CUT: %02d RADIAL: %03d\n", Get_moment_index_string( moment_index ), ci, rn );
                Print_base_data( (libL2_moment_t *) dblk );
              }
            }
          }
        }
        else if( radial_number != LIBL2_ALL_RADIALS &&
                   bdh->azi_num >= radial_number )
        {
          /* If only a single radial is desired and the radial
             number has been processed, current search is done */
          break;
        }
      }
      else if( cut_index != LIBL2_ALL_RADIALS &&
                ( bdh->elev_num-1 > cut_index ||
                   bdh->status == RS_END_VOL ||
                   bdh->status == RS_PSEUDO_END_VOL ) )
      {
        /* If only a single cut is desired and the
           cut number changes, current search is done */
        break;
      }
    }
    else if( type == RDA_STATUS_MSG )
    {
      /* Skip past RDA Status messages */
      offset += METADATA_MSG_SIZE;
    }
    else
    {
      /* Unknown message type */
      Error_code = LIBL2_INVALID_MSG_TYPE;
      sprintf( Error_msg, "%s: invalid msg type %d at %d (%d)", fx, type, offset, Error_code );
      libL2_debug( "%s", Error_msg );
      ret = Error_code;
      break;
    }
  }

  return ret;
}

/************************************************************************
 Description: Print moment header for moment of radial.
 ************************************************************************/

static int Print_moment_header( libL2_moment_t *h )
{
  printf( "NAME:         %s\n", h->name );
  printf( "INFO:         %d\n", h->info );
  printf( "# GATES:      %d\n", h->no_of_gates );
  printf( "1ST GATE:     %d\n", h->first_gate_range );
  printf( "BIN SIZE:     %d\n", h->bin_size );
  printf( "TOVER:        %d\n", h->tover );
  printf( "SNR THRESH:   %d\n", h->SNR_threshold );
  printf( "CONTROL FLAG: %d\n", h->control_flag );
  printf( "WORD SIZE:    %d\n", h->data_word_size );
  printf( "SCALE:        %f\n", h->scale );
  printf( "OFFSET:       %f\n", h->offset );

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print base data header for radial.
 ************************************************************************/

static int Print_base_data_header( libL2_base_hdr_t *b )
{
  printf( "ICAO: %c%c%c%c RAD LEN: %d\n",
          b->radar_id[0], b->radar_id[1], b->radar_id[2],
          b->radar_id[3], b->radial_length );
  printf( "DATE: %05d (%s) TIME: %08ld (%s)\n",
          b->date, Convert_date( b->date ), b->time, Convert_milliseconds( b->time ) );
  printf( "AZ#: %d AZ: %6.2f AZ IND: %d ELEV#: %d ELEV: %5.2f\n",
          b->azi_num, b->azimuth, b->azimuth_index,
          b->elev_num, b->elevation );
  printf( "CMP: %d AZ RES: %d STATUS: %d SECTOR#: %d\n",
          b->compress_type, b->azimuth_res,
          b->status, b->sector_num );
  /* radial status possible values:
          RS_BEG_ELEV, RS_INT_ELEV, RS_END_ELEV,
          RS_BEG_VOL, RS_END_VOL RS_BEG_ELEV_LAST_CUT,
          RS_PSEUDO_END_ELEV, RS_PSEUDO_END_VOL */
  printf( "SPOT: %d #DATUM: %d SPARE: %d\n",
          b->spot_blank_flag, b->no_of_datum, b->spare_17 );

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print base data for radial.
 ************************************************************************/

static int Print_base_data( libL2_moment_t *b )
{
  int i = 0;

  if( b->data_word_size == 8 )
  {
    for( i = 0; i < b->no_of_gates; i++ )
    {
      if( i != 0 && i % 10 == 0 ){ printf( "\n" ); }
      if( b->gate.b[i] == 0 )
      {
        printf( " %7.2f", (float) LIBL2_NO_DATA );
      }
      else if( b->gate.b[i] == 1 )
      {
        printf( " %7.2f", (float) LIBL2_RF_DATA );
      }
      else
      {
        printf( " %7.2f" , ( (float) b->gate.b[i] - b->offset ) / b->scale );
      }
    }
  }
  else if( b->data_word_size == 16 )
  {
    for( i = 0; i < b->no_of_gates; i++ )
    {
      if( i != 0 && i  % 10 == 0 ){ printf( "\n" ); }
      if( b->gate.u_s[i] == 0 )
      {
        printf( " %7.2f", (float) LIBL2_NO_DATA );
      }
      else if( b->gate.u_s[i] == 1 )
      {
        printf( " %7.2f", (float) LIBL2_RF_DATA );
      }
      else
      {
        printf(" %7.2f", ( (float) b->gate.u_s[i] - b->offset ) / b->scale );
      }
    }
  }
  else if( b->data_word_size == 32 && b->scale != 0.0 )
  {
    for( i = 0; i < b->no_of_gates; i++ )
    {
      if( i != 0 && i % 10 == 0 ){ printf( "\n" ); }
      if( b->gate.u_i[i] == 0 )
      {
        printf( " %7.2f", (float) LIBL2_NO_DATA );
      }
      else if( b->gate.u_i[i] == 1 )
      {
        printf( " %7.2f", (float) LIBL2_RF_DATA );
      }
      else
      {
        printf( " %7.2f", ( (float) b->gate.u_i[i] - b->offset ) / b->scale );
      }
    }
  }
  else
  {
    for( i = 0; i < b->no_of_gates; i++ )
    {
      if( i != 0 && i % 10 == 0 ){ printf( "\n" ); }
      if( b->gate.f[i] == 0 )
      {
        printf( " %7.2f", (float) LIBL2_NO_DATA );
      }
      else if( b->gate.f[i] == 1 )
      {
        printf( " %7.2f", (float) LIBL2_RF_DATA );
      }
      else
      {
        printf( " %7.2f", ( (float) b->gate.f[i] - b->offset ) / b->scale );
      }
    }
  }
  printf( "\n" );

  return LIBL2_NO_ERROR;
}


/************************************************************************
 Description: Print RDA/RPG message header.
 ************************************************************************/

#define	RDA_LEGACY_DIGITAL_DATA	1

static int Print_msg_header( libL2_msg_hdr_t *m )
{
  printf( "SIZE:               %d HALFWORDS (%d BYTES)\n", m->size, m->size*sizeof(short) );
  printf( "RDA CHANNEL:        %02d ", (int) m->rda_channel );
  if( m->rda_channel == 0 ){ printf( "(LEGACY NON-REDUNDANT)\n" ); }
  else if( m->rda_channel == 1 ){ printf( "(LEGACY RDA1)\n" ); }
  else if( m->rda_channel == 2 ){ printf( "(LEGACY RDA2)\n" ); }
  else if( m->rda_channel == 8 ){ printf( "(ORDA NON-REDUNDANT)\n" ); }
  else if( m->rda_channel == 9 ){ printf( "(OPEN RDA1)\n" ); }
  else if( m->rda_channel == 10 ){ printf( "(OPEN RDA2)\n" ); }
  else{ printf( "(\?\?\?\?)\n" ); }
  printf( "MSG TYPE:           %02d ", (int) m->type );
  if( m->type == RDA_LEGACY_DIGITAL_DATA ){ printf( "(LEGACY DIGITAL DATA\n" ); }
  else if( m->type == RDA_STATUS_MSG ){ printf( "(RDA STATUS)\n" ); }
  else if( m->type == RDA_PMD_MSG ){ printf( "(RDA PMD)\n" ); }
  else if( m->type == RDA_VCP_MSG ){ printf( "(RDA VCP)\n" ); }
  else if( m->type == RDA_BYPASS_MSG ){ printf( "(RDA BYPASS MAP)\n" ); }
  else if( m->type == RDA_CLUTTER_MSG ){ printf( "(RDA CLUTTER MAP)\n" ); }
  else if( m->type == RDA_ADAPT_MSG ){ printf( "(RDA ADAPTATION DATA)\n" ); }
  else if( m->type == RDA_RADIAL_MSG ){ printf( "(ORDA DIGITAL DATA)\n" ); }
  else{ printf( "????\n" ); }
  printf( "SEQUENCE NUMBER:    %d\n", m->sequence_num );
  printf( "JULIAN DATE:        %05d    (%s)\n", m->julian_date, Convert_date( m->julian_date ) );
  printf( "MILLISECONDS:       %08d (%s)\n", m->milliseconds, Convert_milliseconds( m->milliseconds ) );
  printf( "NUMBER OF SEGMENTS: %d\n", m->num_segs );
  printf( "SEGMENT NUMBER:     %d\n", m->seg_num );
  printf( "\n" );

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print RVOL data block.
 ************************************************************************/

int libL2_print_RVOL( int cut_index, int radial_number )
{
  static char *fx = "libL2_print_RVOL";
  int i = 0;
  int j = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( ( ret = Validate_radial_number( cut_index, radial_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_radial_number returns %d", fx, ret );
    return ret;
  }

  for( i = 0; i < libL2_num_cuts(); i++ )
  {
    if( cut_index == LIBL2_ALL_CUTS || cut_index == i )
    {
      for( j = 0; j < libL2_num_azimuths(i); j++ )
      {
        if( radial_number == LIBL2_ALL_RADIALS || radial_number == j+1 )
        {
          printf( "RVOL: CUT %02d RADIAL %03d\n", i, j+1 );
          printf( "  LEN:         %d\n", L2_vol[i][j].len );
          printf( "  MAJOR:       %d\n", L2_vol[i][j].major_version );
          printf( "  MINOR:       %d\n", L2_vol[i][j].minor_version );
          printf( "  LATITUDE:    %f\n", L2_vol[i][j].lat );
          printf( "  LONGITUDE:   %f\n", L2_vol[i][j].lon );
          printf( "  HEIGHT:      %d\n", L2_vol[i][j].height );
          printf( "  FEEDHORN:    %d\n", L2_vol[i][j].feedhorn_height );
          printf( "  CALIB CONST: %f\n", L2_vol[i][j].calib_const );
          printf( "  HORZ TX PWR: %f\n", L2_vol[i][j].horiz_shv_tx_power );
          printf( "  VERT TX PWR: %f\n", L2_vol[i][j].vert_shv_tx_power );
          printf( "  DIFF REFL:   %f\n", L2_vol[i][j].sys_diff_refl );
          printf( "  DIFF PHASE:  %f\n", L2_vol[i][j].sys_diff_phase );
          printf( "  VCP:         %d\n", L2_vol[i][j].vcp_num );
          printf( "  SIG PROD:    %d\n", L2_vol[i][j].sig_proc_states );
        }
        else if( radial_number != LIBL2_ALL_RADIALS && radial_number < j+1 )
        {
          break;
        }
      }
    }
    else if( cut_index != LIBL2_ALL_CUTS && cut_index < i )
    {
      break;
    }
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print RELV data block.
 ************************************************************************/

int libL2_print_RELV( int cut_index, int radial_number )
{
  static char *fx = "libL2_print_RELV";
  int i = 0;
  int j = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( ( ret = Validate_radial_number( cut_index, radial_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_radial_number returns %d", fx, ret );
    return ret;
  }

  for( i = 0; i < libL2_num_cuts(); i++ )
  {
    if( cut_index == LIBL2_ALL_CUTS || cut_index == i )
    {
      for( j = 0; j < libL2_num_azimuths(i); j++ )
      {
        if( radial_number == LIBL2_ALL_RADIALS || radial_number == j+1 )
        {
          printf( "RELV: CUT %02d RADIAL %03d\n", i, j+1 );
          printf( "  LEN:         %d\n", L2_elv[i][j].len );
          printf( "  ATMOS:       %d\n", L2_elv[i][j].atmos );
          printf( "  CALIB CONST: %f\n", L2_elv[i][j].calib_const );
        }
        else if( radial_number != LIBL2_ALL_RADIALS && radial_number < j+1 )
        {
          break;
        }
      }
    }
    else if( cut_index != LIBL2_ALL_CUTS && cut_index < i )
    {
      break;
    }
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Print RRAD data block.
 ************************************************************************/

int libL2_print_RRAD( int cut_index, int radial_number )
{
  static char *fx = "libL2_print_RRAD";
  int i = 0;
  int j = 0;
  int ret = LIBL2_NO_ERROR;

  if( ( ret = Validate_cut_index( cut_index ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_cut_index returns %d", fx, ret );
    return ret;
  }
  else if( ( ret = Validate_radial_number( cut_index, radial_number ) ) != LIBL2_NO_ERROR )
  {
    libL2_debug( "%s: Validate_radial_number returns %d", fx, ret );
    return ret;
  }

  for( i = 0; i < libL2_num_cuts(); i++ )
  {
    if( cut_index == LIBL2_ALL_CUTS || cut_index == i )
    {
      for( j = 0; j < libL2_num_azimuths(i); j++ )
      {
        if( radial_number == LIBL2_ALL_RADIALS || radial_number == j+1 )
        {
          printf( "RRAD: CUT %02d RADIAL %03d\n", i, j+1 );
          printf( "  UNAMB RANGE: %d\n", L2_rad[i][j].unamb_range );
          printf( "  HORZ NOISE:  %f\n", L2_rad[i][j].horiz_noise );
          printf( "  VERT NOISE:  %f\n", L2_rad[i][j].vert_noise );
          printf( "  NYQUIST VEL: %d\n", L2_rad[i][j].nyquist_vel );
          printf( "  SPARE:       %d\n", L2_rad[i][j].spare );
        }
        else if( radial_number != LIBL2_ALL_RADIALS && radial_number < j+1 )
        {
          break;
        }
      }
    }
    else if( cut_index != LIBL2_ALL_CUTS && cut_index < i )
    {
      break;
    }
  }

  return LIBL2_NO_ERROR;
}

/************************************************************************
 Description: Convert julian date to string.
 ************************************************************************/

static char *Convert_date( int julian_date )
{
  static char timebuf[TIME_BUF_LEN];
  time_t num_secs = (julian_date - 1) * NUM_SECS_IN_DAY;

  strftime( timebuf, 64, "%Y/%m/%d", gmtime( &num_secs ) );

  return timebuf;
}

/************************************************************************
 Description: Convert milliseconds to time string.
 ************************************************************************/

static char *Convert_milliseconds( long msecs )
{
  static char timebuf[64];
  time_t num_secs = msecs/1000;

  strftime( timebuf, 64, "%H:%M:%S", gmtime( &num_secs ) );

  return timebuf;
}

/************************************************************************
 Description: Decode coded angle.
 ************************************************************************/

static float Decode_angle( short coded )
{
  float angle = 0.0;

  if( coded & BIT_16_MASK ){ angle += 180.00; }
  if( coded & BIT_15_MASK ){ angle += 90.00; }
  if( coded & BIT_14_MASK ){ angle += 45.00; }
  if( coded & BIT_13_MASK ){ angle += 22.50; }
  if( coded & BIT_12_MASK ){ angle += 11.25; }
  if( coded & BIT_11_MASK ){ angle += 5.625; }
  if( coded & BIT_10_MASK ){ angle += 2.8125; }
  if( coded & BIT_9_MASK ){ angle += 1.40625; }
  if( coded & BIT_8_MASK ){ angle += 0.70313; }
  if( coded & BIT_7_MASK ){ angle += 0.35156; }
  if( coded & BIT_6_MASK ){ angle += 0.17578; }
  if( coded & BIT_5_MASK ){ angle += 0.08789; }
  if( coded & BIT_4_MASK ){ angle += 0.043945; }
  if( coded & BIT_3_MASK ){ angle += 0.00; }
  if( coded & BIT_2_MASK ){ angle += 0.00; }
  if( coded & BIT_1_MASK ){ angle += 0.00; }

  return angle;
}

/************************************************************************
 Description: Print formatted debug messages to stdout.
 ************************************************************************/

static void libL2_debug( const char *format, ... )
{
  char buf[MAX_TEXT_MSG_LEN];
  va_list arg_ptr;

  /* If not debug mode, nothing to do. */
  if( Debug_flag == LIBL2_NO ){ return; }

  /* If no format, nothing to do. */
  if( format == NULL ){ return; }

  /* Extract print format. */
  va_start( arg_ptr, format );
  vsprintf( buf, format, arg_ptr );
  va_end( arg_ptr );

  /* Print message with libL2 header to stdout. */
  fprintf( stdout, "libL2 (DEBUG) >> %s\n", buf );
  fflush( stdout );
}

