/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:39 $
 * $Id: map.h,v 1.7 2009/05/15 17:52:39 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* map.h */

#ifndef _MAP_H_
#define _MAP_H_

#include <stdio.h>
#include <Xm/Xm.h>
#include <math.h>
#include "global.h"
/* CVG 9.0 - added to use default palette */
#include "packet_definitions.h"


#define	DEGRAD	(3.14159265/180.0)
#define RADAR_POS_OFFSET (96+20)

/*  these definitions must agree with decode.c */
#define POLITICAL 0  /*  PB */
#define ADMIN 1      /*  AB */
#define WATER 2      /*  WB */
#define STREAMS 3    /*  ST */
#define ROADS 4      /*  RD */
#define RAIL 5       /*  RR */
#define CULTURAL 6    /*  CF */
#define HYPSOG 7      /*  HP */

extern int road_d, rail_d, polit_d, culture_d, admin_d, co_d;
extern int water_d, stream_d;




extern XColor display_colors[256];



extern char map_filename[255];
extern screen_data *sd1, *sd2, *sd3;
extern Dimension pwidth, pheight, barwidth;
extern Pixel black_color, white_color, green_color, red_color, grey_color, blue_color, yellow_color; 

extern Pixel orange_color, cyan_color, magenta_color, brown_color, light_grey_color;
extern Pixel snow_color, indian_red_color, ghost_white_color;

extern Display *display;
extern Pixmap pixmap;
extern GC gc;
extern assoc_array_i *product_res;

extern screen_data *sd;
/* extern screen_data *sd1, *sd2, *sd3; */

extern int verbose_flag;

/* prototypes */
void display_map(int screen);

void draw_map_line();

void earth_to_radar_coord(float latitude, float longitude, float reference_latitude,
			  float reference_longitude, float *azimuth, float *range);
			  
extern int read_word(char *buffer,int *offset);
extern float res_index_to_res(int res);

/* CVG 9.0 */
extern int open_default_palette(int packet_type);

extern void setup_palette(FILE *the_palette_file, int packet_type, int legend_block);

#endif
