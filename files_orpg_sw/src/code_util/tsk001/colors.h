/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/08/19 15:11:40 $
 * $Id: colors.h,v 1.9 2009/08/19 15:11:40 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */
/* colors.h */

#ifndef _COLORS_H_
#define _COLORS_H_

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Scale.h>
#include <Xm/DrawnB.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "palette.h"
#include "global.h"
/* CVG 9.1 */
#include "packet_definitions.h"

/* #define	DEGRAD	(3.14159265/180.0) */
/* defined in palette.h */
/* #define MAX_PACKETS 45    */ /* Array size for holding pointers  */
                                /* to each packet structure.        */
assoc_array_s *palettes;

extern char config_dir[255];
extern	Display	*display;

extern  Colormap cmap;
extern int palette_size;
extern XColor display_colors[256];
extern XColor global_display_colors_1[256];
extern XColor global_display_colors_2[256];
extern XColor global_display_colors_3[256];
extern int global_palette_size_1;
extern int global_palette_size_2;
extern int global_palette_size_3;
extern int global_packet_list[100];
extern int global_packet_list_2[100];
extern int global_packet_list_3[100];

extern assoc_array_s *configured_palette;
extern assoc_array_s *config_palette_2;

extern assoc_array_i *associated_packet;
extern assoc_array_i *digital_legend_flag;

/* CVG 9.1 */
extern assoc_array_s *override_palette;
extern assoc_array_i *override_packet;
extern int display_color_bars;

extern int overlay_flag;

extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;

extern int verbose_flag;

/* prototypes */

int global_pixel_to_color(int the_pixel, int *found);

/* CVG 9.0 - changed function name from setup_colors */
void open_config_or_default_palette(int input_type, int legend_flag);
/* CVG 9.0 - ADDED */
int open_default_palette(int packet_type);

void setup_palette(FILE *the_palette_file, int packet_type, int legend_block);
void load_palette_list();


extern void read_to_eol(FILE *list_file, char *buf);

#endif


