/*
 * RCS info
 * $Author: eddief $
 * $Locker:  $
 * $Date: 2002/08/07 21:30:16 $
 * $Id: rle.h,v 1.25 2002/08/07 21:30:16 eddief Exp $
 * $Revision: 1.25 $
 * $State: Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>

#define vcp21   21
#define vcp11   11
#define vcp31   31
#define vcp32   32


/* Product Header Offset defintions */
#define  message_code_offset    0
#define  source_id_offset       6
#define  latitude_offset       10
#define  longitude_offset      12
#define  radar_height_offset   14
#define  mode_offset           16
#define  vcp_offset            17
#define  date_offset           20
#define  time_offset           21
#define  elevation_offset      28
#define  data_level_offset     30
#define  min_data_value        30	/* For hires digital products */
#define  data_increment        31	/* For hires digital products */
#define  num_data_values       32	/* For hires digital products */
#define  dvl_linear_coeff      30	/* For digital VIL product */
#define  dvl_linear_offset     31	/* For digital VIL product */
#define  dvl_log_start         32	/* For digital VIL product */
#define  dvl_log_coeff         33	/* For digital VIL product */
#define  dvl_log_offset        34	/* For digital VIL product */
#define  offset_to_symbology   54
#define  offset_to_grafattr    56
#define  offset_to_tabular     58

/* Symbology Block Offset defintions */
#define block1_size_offset      2
#define num_layers_offset       4
#define layer1_size_offset      6
#define image_type_offset       8

/* Tabular Alphanumeric Block Offset defintions */
#define tabular_page_offset    65
#define sa_tabular_page_offset  1

/* Packet offset definitions */
#define packet_size_offset      1
#define packet_overhead_bytes   4


/* Radial rle data packet header offset defintions */
#define number_of_bins_offset       2
#define range_interval_offset       5
#define number_of_radials_offset    6
#define radial_rle_offset           7

/* Radial rle packet data offset definitions */
#define azimuth_angle_offset         1
#define azimuth_delta_offset         2
#define radial_rle_header_offset     3


/* Raster data packet header offset definitions */
#define i_start_offset              3
#define j_start_offset              4
#define x_scale_offset              5
#define y_scale_offset              6
#define number_of_rows_offset       9
#define raster_rle_offset           11

/* Raster packet data offset definitions */
#define bytes_in_row_offset          1
#define raster_rle_header_offset     1

/*Define graphic attribute block offsets */
#define block_id_offset              1
#define block_length_offset          2
#define num_pages_offset             4
#define first_page_offset            5
#define first_page_length_offset     6
#define page_header_data_size        2
#define first_packet_offset          7
#define first_packet_length_offset   8
#define LINES_PER_PAGE               6

/* Packet Code 2 definitions */
#define special_symbol_packet        2
#define packet2_xpos                 2
#define packet2_ypos                 3


/* Packet Code 3/11 definitions */
#define packet3_xpos                 2
#define packet3_ypos                 3
#define packet3_size                 4
#define HW_PER_MESO_SYMBOL           3


/* Packet Code 12 definitions */
#define packet12_xpos                2
#define packet12_ypos                3


/* Packet Code 4 definitions */
#define block4_overhead              2
#define packet4_size                 5
#define packet4_color_offset         0
#define packet4_time_offset          1
#define packet4_height_offset        2
#define packet4_direction_offset     3
#define packet4_speed_offset         4

/* Packet Code 8 definitions */
#define packet8_length               1
#define packet8_text_value           2
#define packet8_xpos                 3
#define packet8_ypos                 4
#define packet8_header_length        6
#define packet8_text_start           5

/* Packet Code 15 definitions */
#define packet15_xpos                2
#define packet15_ypos                3
#define packet15_stormid             4


/* Packet Code 19 definitions */
#define packet19_poh                 4
#define packet19_psh                 5
#define packet19_mhs                 6

/* Flag value for slow movers */
#define slow_mover                -999

/* Define storage for pointers to various blocks in product */
extern int symbology_block;
extern int grafattr_block;
extern int tabular_block;


/* Product types */
extern unsigned short radial_type;
extern unsigned short raster_type;
extern unsigned short vwp_type;
extern unsigned short sti_type;
extern unsigned short hi_type;
extern unsigned short meso_type;
extern unsigned short tvs_type;
extern unsigned short swp_type;
extern unsigned short dhr_type;
extern unsigned short dbv_type;
extern unsigned short dr_type;
extern unsigned short dv_type;
extern unsigned short dvl_type;
extern unsigned short ss_type;
extern unsigned short ftm_type;
extern unsigned short pup_type;
extern unsigned short spd_type;
extern unsigned short clr_type;
extern unsigned short cld_type;
extern unsigned short generic_radial_type;
extern unsigned short generic_hires_radial_type;
extern unsigned short generic_raster_type;
extern unsigned short generic_hires_raster_type;
extern unsigned short stand_alone_type;

/* Global data */
struct radial_rle{
   int             number_of_radials;

   unsigned short *radial_data[400];
   int             data_elements;
   float          *azimuth_angle;
   float          *azimuth_width;
   float           range_interval;

};

struct raster_rle{
   int             number_of_rows;
   int             number_of_columns;
   int             x_start;
   int             x_scale;
   int             y_start;
   int             y_scale;
   unsigned short *raster_data[464];

};

struct dhr_rle{
   int             number_of_radials;

   unsigned short *radial_data[400];
   int             data_elements;
   float          *azimuth_angle;
   float          *azimuth_width;
   float           range_interval;

};

struct wind{
   int             color;
   int             time;
   int             height;
   int             direction;
   int             speed;
};

struct vwp{
   int             number_of_heights;
   int             number_of_times;
   char            times[11][5];
   char            heights[30][3];

   struct wind     barb[30][11];
};

struct sti{
   short           number_storms;
   union{
      char         storm_id[100][4];
      short        storm_id_hw[100][2];
   } equiv;
   short           curr_xpos[100];
   short           curr_ypos[100];
   short           number_past_pos[100];
   short           past_xpos[100][10];
   short           past_ypos[100][10];
   short           number_forcst_pos[100];
   short           forcst_xpos[100][4];
   short           forcst_ypos[100][4];
};

struct hi{
   short           number_storms;
   union{
      char         storm_id[100][4];
      short        storm_id_hw[100][2];
   } equiv;
   short           curr_xpos[100];
   short           curr_ypos[100];
   short           prob_hail[100];
   short           prob_svr[100];
   short           hail_size[100];
};

struct meso{
   short           number_mesos;
   short           number_3Dshears;
   union{
      char         storm_id[20][4];
      short        storm_id_hw[20][2];
   } equiv;
   short           meso_xpos[20];
   short           meso_ypos[20];
   short           meso_size[20];
   short           a3D_xpos[20];
   short           a3D_ypos[20];
   short           a3D_size[20];
};

struct tvs{
   short           number_tvs;
   union{
      char         storm_id[20][4];
      short        storm_id_hw[20][2];
   } equiv;
   short           tvs_xpos[20];
   short           tvs_ypos[20];
};

struct swp{
   short           number_storms;
   union{
      char         swp[100][4];
      short        swp_hw[100][2];
   } equiv;
   short           xpos[100];
   short           ypos[100];
   short           text_value[100];
};

#define max_level_length      7
#define max_data_levels      16

struct product_pertinent{
   short          product_code;
   char           *data_levels[ max_data_levels ];
   int            num_data_levels;
   char           units[6];
   float          elevation;
   char           product_name[128];
   float          x_reso;
   float          y_reso;
   float          center_azimuth;
   float          center_range;
   unsigned char  full_screen;
   int            number_of_lines;
   char           *text[50];
   float	  min_value;
   float	  inc_value;
   int		  num_levels;
   float          linear_coeff;
   float          linear_offset;
   float          log_coeff;
   float          log_offset;
   int            log_start;

};

struct graphic_attr{
   int number_of_lines;
   char *text[200];
};

struct tabular_attr{
   int number_of_pages;
   int *number_of_lines;
   char ***text;
};

/* Define character string and halfword equivalence */
union char_short{
   char byte_data[82];
   short integer_data[41];
};

/* Define Pointer to radial and raster data structures */
extern struct radial_rle *radial_image;
extern struct raster_rle *raster_image;
extern struct dhr_rle *dhr_image;


/* Define pointer to VWP data structure */
extern struct vwp *vwp_image;

/* Define pointer to STI data structure */
extern struct sti *sti_image;

/* Define pointer to HI data structure */
extern struct hi *hi_image;

/* Define Pointer to MESO data structure */
extern struct meso *meso_image;

/* Define Pointer to TVS data structure */
extern struct tvs *tvs_image;

/* Define Pointer to SWP data structure */
extern struct swp *swp_image;

/* Define pointer to product_pertinent data structure */
extern struct product_pertinent* attribute;

/* Define pointer to graphic attribute data structure */
extern struct graphic_attr *gtab;

/* Define pointer to tabular alphanumeric data structure */
extern struct tabular_attr *ttab;

/* Elevation Angle data */
extern float vcp21_elev[];
extern float vcp11_elev[];
extern float vcp31_elev[];
extern float vcp32_elev[];


/* Product identifying data */
extern char *product_name[];

/* Resolution table */
extern float xy_azran_reso[][2]; 


/* Product color levels table */
extern int data_level_tab[];


/* Function Prototypes */
int decode_product( short *product_data );
int polar_slice( char *filename );
int read_concurrent_product_file( char *filename, short **product_data );
int decode_header( short *product_data );
int decode_radial_rle( short *product_data );
int decode_raster_rle( short *product_data );
void decode_data_level( short level, char *decoded_level );
void level_to_ASCII( float scale_factor, unsigned char value, char *string);
void convert_time( int timevalue, int *hours, int *minutes, int *seconds );
void calendar_date( short date, int *day, int *month, int *year );
void free_memory( short product_code );
void decode_grafattr_block( short *product_data );
void decode_tabular_block( short *product_data,
                           unsigned short product_type );
void decode_text_packet_8( short *product_data, 
                           char **packet_text,
                           int *line_number,
                           int *col_number );
void product_dependent( short *product_data );
void degrees_minutes_seconds( float latlong, int *deg, int *min, int *sec );
void decode_vwp( short *product_data, short product_offset, int layer1_size );
void decode_sti( short *product_data, short product_offset, int layer1_size );
void decode_hi( short *product_data, short product_offset, int layer1_size );
void decode_meso( short *product_data, short product_offset, int layer1_size );
void decode_tvs( short *product_data, short product_offset, int layer1_size );
void decode_swp( short *product_data, short product_offset, int layer1_size );
void decode_dhr( short *product_data );
