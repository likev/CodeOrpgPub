/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:22:22 $
 * $Id: packet_28_cvg.h,v 1.1 2009/05/15 17:22:22 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/* packet_28_cvg.h */

#ifndef _DECODE_PACKET_28_H_
#define _DECODE_PACKET_28_H_

#include <Xm/MessageB.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <sys/types.h>
#include <string.h>

#include "global.h"
#include "packet_definitions.h"
#include "coord_conv.h"
/* CVG 9.0 */
#include "packet_28.h"

#define	DEGRAD	(3.14159265/180.0)
/* #define RESCALE_FACTOR (460.0/2048.0) */
#define RESCALE_FACTOR (0.25)  /* 1/4 km */


/* MISC OFFSETS into ICD product header, not all used */
/* #define HEADER_DATE_OFFSET (96+2)  */
/* #define HEADER_TIME_OFFSET (96+4)  */
/* #define HEADER_ID_OFFSET   (96+12) */
#define RADAR_POS_OFFSET (96+20)
/* #define VCP_OFFSET         (96+34) */
/* #define THRESHOLD_OFFSET   (96+60) */


/* Index into symbol_pkt28.plt           */
#define P28_BLACK  0
#define P28_WHITE  1
#define P28_YELLOW 2
#define P28_RED    3
#define P28_GREEN  4
#define P28_BLUE   5
#define P28_ORANGE 6
#define P28_GRAY   7


extern screen_data *sd1, *sd2, *sd;
extern int verbose_flag;
extern char config_dir[255];
extern assoc_array_i *digital_legend_flag;
extern assoc_array_s *digital_legend_file;
extern assoc_array_s *dig_legend_file_2;

extern  int     palette_size;

extern assoc_array_s *palettes;
extern char config_dir[255];

/*  from orpg file packet_28.h */
/* typedef struct       */
/* {                    */
/*    short  code;      */   /* Packet code                                   */
/*    short  align;     */
/*    int    num_bytes; */   /* Byte length not including self or packet code */
/* } packet_28_t;       */


extern int area_label;
extern int area_symbol;
extern int area_line_type;
extern int include_points_flag;

extern	Display		*display;
extern	GC		gc;
extern	XColor 	        display_colors[];
extern	Dimension	width,pwidth,pheight;

extern Pixmap area_prev_pix;

extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;

extern int verbose_flag;
extern assoc_array_i *product_res;
extern assoc_array_s *product_mnemonics;

extern  Pixel  white_color, black_color;

extern Boolean norm_format1, norm_format2, bkgd_format1, bkgd_format2;
extern Boolean norm_format3, bkgd_format3;

/*  This would include orpg_product.h (don't want to do this in CVG ) */
/*  A source of RPGP_print function prototypes which we also do not use. */
/* #include <rpgp.h> */
#include "cvg_orpg_product.h"

/* provides the following functions, among others */

/***************************************************************
    MODIFIED NAMES OF PUBLIC FUNCTIONS IN orpg_products.h TO AVOID COLLISIONS
****************************************************************/
extern int cvg_RPGP_product_deserialize (char *serial_data,
                        int size, void **prod);
extern int cvg_RPGP_product_free (void *prod);




/* prototypes */
void packet_28_skip(char *buffer,int *offset);
void display_packet_28(int packet, int index, int replay);

void display_radial(int index, int location_flag, int replay);

extern int read_generic_legend(FILE *leg_file, char *generic_legend_filename, 
                                               int method, int frame_location);

int decode_generic_radial(int index, int location_flag);
void output_generic_radial(int location_flag);
void delete_generic_radial();
void allocate_and_load_screen_radial_data(generic_d_type array_type);
void print_a_radial(int rad_num);
int lookup_gen_color_u(unsigned int d_level);
int lookup_gen_color_s(int d_level);    /*  not implemented */
int lookup_gen_color_f(float d_level);  /*  not implemented */
int lookup_gen_color_d(double d_level); /*  not implemented */


void display_grid(int index, int location_flag);


void display_area(int index, int location_flag);
void draw_area_symbol(int s_type, int s_color, int x_pix, int y_pix);
void draw_area_label(char *label, int x_pix, int y_pix);
void draw_area_line(XPoint  *points, int n_points, int a_type, 
                                             int l_color, int l_type);

void display_text(int index, int location_flag);


void display_table(int index, int location_flag);


void display_event(int index, int location_flag);


extern void setup_palette(FILE *the_palette_file, int packet_type, int pref_demo);

extern short read_half(char *buffer,int *offset);

extern float res_index_to_res(int res);

/* CVG 9.0 - added new function supporting GEOGRAPHIC_PRODUCT */
extern void geo_scale_and_center(int screen_width, int screen_height, 
                    float screen_zoom, int screen_x_center, int screen_y_center, 
                          float base_resolution, float product_resolution,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);


extern int _88D_Round ( float r ); 

/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);
extern int read_word_flip(char *buffer,int *offset);
extern void MISC_short_swap( void *buf, int size);


extern Widget shell;


#endif
