/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:40 $
 * $Id: overlay.h,v 1.7 2009/05/15 17:52:40 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* overlay.h */

#ifndef _OVERLAY_H_
#define _OVERLAY_H_

#include <stdio.h>
#include <Xm/Xm.h>
#include "global.h"
/* CVG 9.0 - added to use default palette */
#include "packet_definitions.h"


/* CVG 9.0 - added to open a palette file locally */
extern char config_dir[255];
extern XColor display_colors[256];



extern Dimension pwidth, pheight, barwidth;
extern Pixel white_color;
extern Display *display;
extern GC gc;

extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;

extern int verbose_flag;

extern Boolean norm_format1, norm_format2, bkgd_format1, bkgd_format2;
extern Boolean norm_format3, bkgd_format3;

/* prototypes */
void draw_az_lines(int screen);
void draw_range_rings(int screen);

extern float res_index_to_res(int res);

/* CVG 9.0 - added */
extern int open_default_palette(int packet_type);



#endif
