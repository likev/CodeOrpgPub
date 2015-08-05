/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2014/07/21 20:19:18 $
 * $Id: libl2.h,v 1.5 2014/07/21 20:19:18 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef LIBL2_H
#define LIBL2_H

/* System include file definitions */

/* Local include file definitions */

#include <orpg.h>

/* Enums */

enum { LIBL2_NO, LIBL2_YES };

enum {
  LIBL2_REF_INDEX,
  LIBL2_VEL_INDEX,
  LIBL2_SPW_INDEX,
  LIBL2_ZDR_INDEX,
  LIBL2_PHI_INDEX,
  LIBL2_RHO_INDEX,
  LIBL2_NUM_MOMENTS
};

enum {
  LIBL2_L2_FILENAME_NULL = -600,
  LIBL2_L2_FILENAME_TOO_LONG,
  LIBL2_L2_UNKNOWN_FORMAT,
  LIBL2_STAT_L2_FILE_FAILED,
  LIBL2_OPEN_L2_FILE_FAILED,
  LIBL2_STAT_L2_FILE_SIZE_ERROR,
  LIBL2_MALLOC_FAILED,
  LIBL2_REALLOC_FAILED,
  LIBL2_L2_FILE_READ_ERROR,
  LIBL2_NO_L2_HEADER,
  LIBL2_BUNZIP_FAILED,
  LIBL2_GUNZIP_FAILED,
  LIBL2_UNCOMPRESS_FAILED,
  LIBL2_BAD_CUT_INDEX,
  LIBL2_BAD_MOMENT_INDEX,
  LIBL2_BAD_MOMENT_BITMASK,
  LIBL2_BAD_SEGMENT_NUMBER,
  LIBL2_INVALID_MSG_TYPE,
  LIBL2_ICAO_NOT_DEFINED,
  LIBL2_NO_DATA_CUTS,
  LIBL2_NO_STATUS_MSGS,
  LIBL2_NO_PMD_MSG,
  LIBL2_NO_VCP_MSG,
  LIBL2_NO_ADAPT_MSG,
  LIBL2_NO_BYPASS_MAP_MSG,
  LIBL2_NO_CLUTTER_MAP_MSG,
  LIBL2_NO_MOMENTS_IN_FLAG,
  LIBL2_BAD_RADIAL_NUMBER
};

enum {
  LIBL2_NEXRAD,
  LIBL2_GZIP,
  LIBL2_BZIP2,
  LIBL2_COMPRESS,
  LIBL2_UNCOMPRESS,
  LIBL2_UNKNOWN_FORMAT
};

enum {
  LIBL2_ELEV_SEGMENT_NUMBER_ONE = 1,
  LIBL2_ELEV_SEGMENT_NUMBER_TWO,
  LIBL2_ELEV_SEGMENT_NUMBER_THREE,
  LIBL2_ELEV_SEGMENT_NUMBER_FOUR,
  LIBL2_ELEV_SEGMENT_NUMBER_FIVE,
};

enum {
  LIBL2_ALL_SEGMENTS = -10,
  LIBL2_ALL_CUTS,
  LIBL2_ALL_RADIALS,
  LIBL2_ALL_MOMENTS
};

/* Defines */

#define	LIBL2_NO_ERROR			0
#define	LIBL2_MAX_FILE_NAME_LEN		256
#define	LIBL2_NO_DATA			-999
#define	LIBL2_RF_DATA			-888

/* Structs */

typedef ORDA_status_msg_t          libL2_status_t;
typedef Pmd_t                      libL2_pmd_t;
typedef VCP_ICD_msg_t              libL2_vcp_t;
typedef ORDA_adpt_data_t           libL2_adapt_t;
typedef ORDA_clutter_map_t         libL2_clutter_map_t;
typedef ORDA_bypass_map_t          libL2_bypass_map_t;
typedef Generic_vol_t              libL2_RVOL_t;
typedef Generic_elev_t             libL2_RELV_t;
typedef Generic_rad_t              libL2_RRAD_t;
typedef ORDA_bypass_map_segment_t  libL2_bypass_map_segment_t;
typedef ORDA_clutter_map_segment_t libL2_clutter_map_segment_t;
typedef ORDA_clutter_map_filter_t  libL2_clutter_map_filter_t;
typedef Generic_basedata_t         libL2_base_t;
typedef Generic_basedata_header_t  libL2_base_hdr_t;
typedef Generic_moment_t           libL2_moment_t;
typedef Generic_any_t              libL2_any_t;
typedef RDA_RPG_message_header_t   libL2_msg_hdr_t;
typedef VCP_message_header_t       libL2_vcp_hdr_t;
typedef VCP_elevation_cut_header_t libL2_vcp_elev_hdr_t;

/* Prototypes */

/*
  Nomenclature for arguments to pass:
  c_i - cut index (zero-indexed)
  m_i - moment index (zero-indexed)
  s_n - segment number (one-indexed)
  r_n - radial (one-indexed)
  m_b - moment bitmask (hexadecimal)
*/

/* Level-II File/Associated Data */

int                   libL2_read( char * );
int                   libL2_filetype();
char                 *libL2_filetype_string();
char                 *libL2_ICAO();
time_t                libL2_epoch();
char                 *libL2_date();
char                 *libL2_time();
float                 libL2_latitude();
float                 libL2_longitude();
int                   libL2_height();
int                   libL2_VCP();
int                   libL2_num_cuts();
float                 libL2_compression_ratio();

/* Meta-Data */

int                   libL2_num_status_msgs();
libL2_status_t      **libL2_status_msgs();
libL2_pmd_t          *libL2_pmd_msg();
libL2_vcp_t          *libL2_vcp_msg();
libL2_adapt_t        *libL2_adapt_msg();
libL2_bypass_map_t   *libL2_bypass_map_msg();
libL2_clutter_map_t  *libL2_clutter_map_msg();

/* Elevation-based Data */

float                *libL2_elevations();
int                   libL2_num_azimuths( int c_i );
float                *libL2_azimuths( int c_i );
int                   libL2_num_gates( int c_i, int m_i );
float                *libL2_gates( int c_i, int m_i );
int                   libL2_num_moments( int c_i );
float                 libL2_azimuth_resolutions( int c_i, int m_i );
float                 libL2_gate_resolutions( int c_i, int m_i );

/* Base Moments */

int                   libL2_cut_has_REF( int c_i );
int                   libL2_cut_has_VEL( int c_i );
int                   libL2_cut_has_SPW( int c_i );
int                   libL2_cut_has_ZDR( int c_i );
int                   libL2_cut_has_PHI( int c_i );
int                   libL2_cut_has_RHO( int c_i );

/* Radial Data */

libL2_RVOL_t         *libL2_rvol( int c_i, int r_n );
libL2_RELV_t         *libL2_relv( int c_i, int r_n );
libL2_RRAD_t         *libL2_rrad( int c_i, int r_n );
float                *libL2_base_data( int c_i, int m_i, int r_n );

/* RDA Elevation Segments */

int                   libL2_num_segments();
float                 libL2_segment_limits( int s_n );

/* RDA Bypass Map */

int                   libL2_bypass_map_num_segments();
int                  *libL2_bypass_map( int s_n, int r_n );
int                   libL2_bypass_map_num_azimuths( int s_n );
float                *libL2_bypass_map_azimuths( int s_n );
int                   libL2_bypass_map_num_gates( int s_n );
float                *libL2_bypass_map_gates( int s_n );

/* RDA Clutter Map */

int                   libL2_clutter_map_num_segments();
int                  *libL2_clutter_map( int s_n, int r_n );
int                   libL2_clutter_map_num_azimuths( int s_n );
float                *libL2_clutter_map_azimuths( int s_n );
int                   libL2_clutter_map_num_gates( int s_n );
float                *libL2_clutter_map_gates( int s_n );

/* Error handling */

char                 *libL2_error_msg();
int                   libL2_error_code();

/* Debugging */

void                  libL2_debug_on();
void                  libL2_debug_off();

/* Output */

int                   libL2_print_misc();
int                   libL2_print_rda_adapt();
int                   libL2_print_rda_clutter_map( int s_n, int r_n );
int                   libL2_print_rda_bypass_map( int s_n, int r_n );
int                   libL2_print_rda_VCP();
int                   libL2_print_rda_pmd();
int                   libL2_print_rda_status_msgs();
int                   libL2_print_rda_status_msg( int number );
int                   libL2_print_msg_header( int c_i, int r_n );
int                   libL2_print_base_header( int c_i, int r_n );
int                   libL2_print_moment_header( int c_i, int r_n, int m_i );
int                   libL2_print_base_data( int c_i, int r_n, int m_i );
int                   libL2_print_RVOL( int c_i, int r_n );
int                   libL2_print_RELV( int c_i, int r_n );
int                   libL2_print_RRAD( int c_i, int r_n );

#endif
