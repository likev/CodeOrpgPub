/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/09/10 20:48:03 $
 * $Id: hci_decode_product.h,v 1.19 2012/09/10 20:48:03 ccalvert Exp $
 * $Revision: 1.19 $
 * $State: Exp $
 */

#ifndef	HCI_DECODE_PROD_H
#define	HCI_DECODE_PROD_H

/* Local include files */

#include <prod_gen_msg.h>
#include <rpgc.h>

#define	VCP21				21 
#define	VCP11				11
#define	VCP31				31 
#define	VCP32				32


/* Product Header Offset defintions */

#define	MESSAGE_CODE_OFFSET		0
#define	SOURCE_ID_OFFSET		6
#define	LATITUDE_OFFSET			10
#define	LONGITUDE_OFFSET		12
#define	RADAR_HEIGHT_OFFSET		14
#define	MODE_OFFSET			16
#define	VCP_OFFSET			17
#define	DATE_OFFSET			20
#define	TIME_OFFSET			21
#define	PARAM_1				26
#define	PARAM_2				27
#define	ELEVATION_OFFSET		28
#define	PARAM_3				29
#define	DATA_LEVEL_OFFSET		30
#define	OFFSET_TO_SYMBOLOGY		54
#define	OFFSET_TO_GRAFATTR		56
#define	OFFSET_TO_TABULAR		58 
                    
/* Symbology Block Offset defintions */

#define	BLOCK1_SIZE_OFFSET		2
#define	NUM_LAYERS_OFFSET		4
#define	LAYER1_SIZE_OFFSET		6
#define	IMAGE_TYPE_OFFSET		8

/* Tabular Alphanumeric Block Offset defintions */

#define	TABULAR_PAGE_OFFSET		65
#define	SA_TABULAR_PAGE_OFFSET		1

/* Packet offset definitions */

#define	PACKET_SIZE_OFFSET		1   
#define	PACKET_OVERHEAD_BYTES		4


/* Radial rle data packet header offset defintions */

#define	NUMBER_OF_BINS_OFFSET		2
#define	RANGE_INTERVAL_OFFSET		5
#define	NUMBER_OF_RADIALS_OFFSET	6
#define	RADIAL_RLE_OFFSET		7

/* Radial rle packet data offset definitions */

#define	AZIMUTH_ANGLE_OFFSET		1
#define	AZIMUTH_DELTA_OFFSET		2
#define	RADIAL_RLE_HEADER_OFFSET	3


/* Raster data packet header offset definitions */

#define	I_START_OFFSET			3
#define	J_START_OFFSET			4
#define	X_SCALE_OFFSET			5
#define	Y_SCALE_OFFSET			6
#define	NUMBER_OF_ROWS_OFFSET		9
#define	RASTER_RLE_OFFSET		11

/* Raster packet data offset definitions */

#define	BYTES_IN_ROW_OFFSET		1
#define	RASTER_RLE_HEADER_OFFSET	1

/*Define graphic attribute block offsets */

#define	BLOCK_ID_OFFSET			1
#define	BLOCK_LENGTH_OFFSET		2
#define	NUM_PAGES_OFFSET		4
#define	FIRST_PAGE_OFFSET		5
#define	FIRST_PAGE_LENGTH_OFFSET	6
#define	PAGE_HEADER_DATA_SIZE		2
#define	FIRST_PACKET_OFFSET		7
#define	FIRST_PACKET_LENGTH_OFFSET	8
#define	LINES_PER_PAGE			6
 

/* Packet Code 2 definitions */

#define	SPECIAL_SYMBOL_PACKET		2
#define	PACKET2_XPOS			2
#define	PACKET2_YPOS			3


/* Packet Code 3/11 definitions */

#define	PACKET3_XPOS			2
#define	PACKET3_YPOS			3
#define	PACKET3_SIZE			4
#define	HW_PER_MESO_SYMBOL		3 


/* Packet Code 12 definitions */

#define	PACKET12_XPOS			2
#define	PACKET12_YPOS			3

/* Packet Code 4 definitions */

#define	BLOCK4_OVERHEAD			2
#define	PACKET4_SIZE			5
#define	PACKET4_COLOR_OFFSET		0
#define	PACKET4_TIME_OFFSET		1
#define	PACKET4_HEIGHT_OFFSET		2
#define	PACKET4_DIRECTION_OFFSET	3
#define	PACKET4_SPEED_OFFSET		4


/* Packet Code 8 definitions */

#define	PACKET8_LENGTH			1
#define	PACKET8_TEXT_VALUE		2
#define	PACKET8_XPOS			3
#define	PACKET8_YPOS			4
#define	PACKET8_HEADER_LENGTH		6
#define	PACKET8_TEXT_START		5


/* Packet Code 15 definitions */

#define	PACKET15_XPOS			2
#define	PACKET15_YPOS			3
#define	PACKET15_STORMID		4


/* Packet Code 19 definitions */

#define	PACKET19_POH			4
#define	PACKET19_PSH			5
#define	PACKET19_MHS			6

/* Flag value for slow moving storms */

#define	SLOW_MOVER			-999

/* Product types */

#define	RADIAL_TYPE			0xAF1F
#define	RASTER_TYPE			0xBA0F
#define	DHR_TYPE			32
#define	SWP_TYPE			47
#define	VWP_TYPE			48
#define	STI_TYPE			58
#define	HI_TYPE				59
#define	MESO_TYPE			60
#define	TVS_TYPE			61
#define	SS_TYPE				62
#define	FTM_TYPE			75
#define	PUP_TYPE			77
#define	SPD_TYPE			82
#define	DBV_TYPE			94
#define	DR_TYPE				94
#define	DV_TYPE				99
#define	CLR_TYPE			132
#define	CLD_TYPE			133
#define	DVL_TYPE			134
#define	STAND_ALONE_TYPE		5
#define	GENERIC_RADIAL_TYPE		996
#define	GENERIC_HIRES_RADIAL_TYPE	997
#define	GENERIC_RASTER_TYPE		998
#define	GENERIC_HIRES_RASTER_TYPE	999

typedef struct {
   int             number_of_radials;

   unsigned short *radial_data[400];
   int             data_elements;
   float          *azimuth_angle;
   float          *azimuth_width;
   float           range_interval;

} radial_rle_t;

typedef struct {
   int             number_of_rows;
   int             number_of_columns;
   int             x_start;
   int             x_scale;
   int             y_start;
   int             y_scale;
   unsigned short *raster_data[464];

} raster_rle_t;

typedef struct {
   int             number_of_radials;

   unsigned short *radial_data[400];
   int             data_elements;
   float          *azimuth_angle;
   float          *azimuth_width;
   float           range_interval;

} dhr_rle_t;

typedef struct {
   int             color;
   int             time;
   int             height;
   int             direction;
   int             speed;
} wind_t;

typedef struct {
   int             number_of_heights;
   int             number_of_times;
   char            times[11][5];
   char            heights[30][3];
   wind_t          barb[30][11];
} vwp_t;

typedef struct {
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
} sti_t;

typedef struct {
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
} hi_t;

typedef struct {
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
} meso_t;

typedef struct {
   short           number_tvs;
   union{
      char         storm_id[20][4];
      short        storm_id_hw[20][2];
   } equiv;
   short           tvs_xpos[20];
   short           tvs_ypos[20];
} tvs_t;

typedef struct {
   short           number_storms;
   union{
      char         swp[100][4];
      short        swp_hw[100][2];
   } equiv;
   short           xpos[100];
   short           ypos[100];
   short           text_value[100];
} swp_t;


#define	MAX_LEVEL_LENGTH		7
#define	MAX_DATA_LEVELS			16
#define	MAX_UNITS_LEN			6
#define	MAX_PRODUCT_NAME_LEN		80
#define	MAX_ANNOTATE_LINE_LEN		22

enum {
  PROD_DATE_INDEX,
  PROD_TIME_INDEX,
  PROD_ID_INDEX,
  PROD_LAT_INDEX,
  PROD_LON_INDEX,
  PROD_ELEV_HEIGHT_INDEX,
  PROD_MODE_INDEX,
  PROD_VCP_INDEX,
  PROD_CUT_ANGLE_INDEX,
  PROD_DATA_MAX_INDEX,
  PROD_DATA_SUPP1_INDEX,
  PROD_DATA_SUPP2_INDEX,
  PROD_DATA_SUPP3_INDEX,
  PROD_NUM_ANNOTATE_LINES
};

typedef struct {
   short          product_code;
   char           *data_levels[ MAX_DATA_LEVELS ];
   int            num_data_levels;
   char           units[MAX_UNITS_LEN];
   float          elevation;
   char           product_name[MAX_PRODUCT_NAME_LEN];
   float          x_reso;
   float          y_reso;
   float          center_azimuth;
   float          center_range;
   unsigned char  full_screen;
   int            number_of_lines;
   char           *text[PROD_NUM_ANNOTATE_LINES];

} product_pertinent_t;

typedef struct {
   int number_of_lines;
   char *text[200];
} graphic_attr_t;

typedef struct {
   int number_of_pages;
   int *number_of_lines;
   char ***text;
} tabular_attr_t;

/* Function Prototypes */

int   hci_read_concurrent_product_file( char *, short ** );
float *hci_product_azimuth_data();
float *hci_product_azimuth_width();
int   hci_product_code();
float hci_product_elevation();
int   hci_product_elevation_index();
short hci_product_vcp();
int   hci_product_data_elements();
int   hci_product_data_levels();
float hci_product_range_interval();
int   hci_product_date();
int   hci_product_radials();
float hci_product_resolution();
int   hci_product_time();
int   hci_product_type();
short hci_product_param_1();
short hci_product_param_2();
short hci_product_param_3();
short hci_product_param_4();
short hci_product_param_5();
short hci_product_param_6();
short hci_product_param_7();
short hci_product_param_8();
short hci_product_param_9();
short hci_product_param_10();
int   hci_load_product_data( int, int, short * );
int   hci_load_product_io_status();
void  hci_get_seg_channel( short *, int *, int * );
void  calendar_date( short, int *, int *, int * );
unsigned short **hci_product_radial_data();

/* The following group of functions are used to get product
   attribute information. */

int  hci_product_attribute_num_data_levels();
char *hci_product_attribute_data_level( int );
char *hci_product_attribute_units();
int  hci_product_attribute_number_of_lines();
char *hci_product_attribute_text( int );

/* The following group of functions are used to get product
   tabular information (i.e., free text message, PUP message,
   other stand-alone product types. */

int  hci_product_tabular_pages();
int  hci_product_tabular_lines( int );
char *hci_product_tabular_line( int, int );
int  hci_decode_product( short * );

#endif
