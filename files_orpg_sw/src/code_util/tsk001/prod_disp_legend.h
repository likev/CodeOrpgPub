/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:22:22 $
 * $Id: prod_disp_legend.h,v 1.1 2009/05/15 17:22:22 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/* prod_disp_legend.h */ 

#ifndef _PROD_DISP_LEGEND_H_
#define _PROD_DISP_LEGEND_H_

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <limits.h>

#include "global.h"
#include "packet_definitions.h"

extern Display *display;
extern GC gc;
extern screen_data *sd;
extern int verbose_flag;

extern Pixel  grey_color;

/* declared in cvg.h */
extern int palette_size;  /* the number of colors in the palette */


/* color stuff */
extern XColor display_colors[256];

/* declared in packet_16.c, to be used also for legend display */
double color_scale;  /* the ratio of number of levels / the number of colors */

extern int font_height;  

#define MIN_FLAG_HT 12  /* Min bar height for flag values also in prod_display.h */
#define MIN_LBL_INCR 24 /* Minimum increment between non-flag threshold labels*/

/* LEGEND SCOPE VARIABLES ****************************************************/

unsigned short thresh[16]; /* CVG 8.7 moved global */

/* The following parameters are set when the corresponding legend ***********/
/* configuration file is read.  If drawing the legend from the PREFS_FRAME  */
/* they are used by the corresponding display functions.                    */
/* ***************************************************************************/
/* If drawing the legend from the PRODUCT_FRAME, the corresponding variables */
/* in the screen data are used instead; permitting elminating the reading of */
/* the legend only once during the display of the product with a few changes */
/* to the display functions for packet 16, 17, and the radial component in   */
/* packet 28.                                                                */

int type_leg_file;  /*  type of file, Methods 4, 5, 6 */
int num_leg_colors; /*  number of colors in legend, Methods 4,5,6  */

/* set by reading generic legend, used by display generic legend */
int  param_source;


int num_lead_flags;  /*  all Methods */
int num_trail_flags; /*  all Methods */

/* decoding parameters added */
union { /* not needed for Method 5. Used for Method 4 and 6 - need screen data */
    unsigned int ui;
    int          si;
    double       dd;
} min_lvl;
/* CVG 8.7 FOR FUTURE USE - also need screen data */
union { /* not needed for Method 5. Used for Method 4 and 6 */
    unsigned int ui;
    int          si;
    double       dd;
} max_lvl;

unsigned int max_level;
float        leg_Scale;
float        leg_Offset;
float        prod_Scale;
float        prod_Offset;
int          prod_n_l_flags;
int          prod_n_t_flags;
unsigned int prod_max_level;


int leg_min_val;       /*  min data value - Method 1 only */
int leg_inc;           /*  increment between values, Method 1 only */
int leg_min_val_scale; /*  scale of min data value, Method 1 only */
int leg_inc_scale;     /*  scale of increment, Method 1 only */

/* tot_num_lvls if needed for generic method, */
/* is calculated from maximum value           */
unsigned long tot_num_lvls; /*  Number of data levels (incl flags), Methods 1, 2, 3 */


/* for signed data type (Method 4) NOT YET IMPLEMENTED */
   int si_dl[MAX_N_COLOR+1];
/* for unsigned data type (Method 5) CVG 8.7 also used by Methods 2 & 3 */
   unsigned int ui_dl[MAX_N_COLOR+1];
/* for real data type (Method 6) NOT YET IMPLEMENTED */
   double d_dl[MAX_N_COLOR+1];

int bar_color[MAX_N_COLOR+1];       /*  color at this threshold */

label_string leg_label[MAX_N_COLOR+1]; /*  threshold label string  */

/*****************************************************************************/



/* FUNCTION PROTOTYPES */

void display_calculated_dig_labels(Drawable canvas, int h,
                                 int x, int y, int frame_loc);
void display_explicit_dig_labels(Drawable canvas, int h,
                                 int x, int y, int frame_loc);
void draw_digital_color_bars(Drawable canvas, int h,
                                 int x, int y, int frame_loc);
                                 
int read_generic_legend(FILE *leg_file, char *generic_legend_filename, 
                                 int method, int frame_location);
/* CVG 8.9 - added prod_id param */
void display_generic_legend(Drawable canvas, int n_val_hgt, int x_in, int y_in,
                                 int method, int prod_id, int frame_location);

/* CVG 8.9 added */
extern int open_legend_block_palette(int prod_id, int frame_loc);

int read_digital_legend(FILE *leg_file, char *digital_legend_filename, 
                                               int method, int frame_location);
void display_rle_labels(Drawable canvas, int h_in, int x_in, int y_in,  
                             int frame_location);
void draw_rle_color_bars(Drawable canvas, int h_in, int x_in, int y_in,  
                             int frame_location);

double decode_data_level(unsigned int data_level, int parameter_source, 
                                                         int frame_loc);


extern void read_to_eol(FILE *list_file, char *buf);


#endif 
