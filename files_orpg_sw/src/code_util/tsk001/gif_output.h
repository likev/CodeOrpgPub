/* gif_output.h *
 *
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:37 $
 * $Id: gif_output.h,v 1.8 2009/05/15 17:52:37 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */
 
#ifndef _GIF_OUTPUT_H_
#define _GIF_OUTPUT_H_

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Label.h>
#include <Xm/PanedW.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/FileSB.h>
#include <Xm/SelectioB.h>
#include <Xm/BulletinB.h>
#include <Xm/MessageB.h>
#include <stdio.h>
/**Solaris 8 change**/
#include <stdlib.h>

#include <sys/stat.h>

#include <gd.h>


#include "global.h"


/* CVG 9.0 */
extern int standalone_flag;

extern int palette_size;
extern Dimension pwidth, pheight, barwidth;

extern XColor global_display_colors_1[256];
extern XColor global_display_colors_2[256];
extern XColor global_display_colors_3[256];
extern int global_palette_size_1;
extern int global_palette_size_2;
extern int global_palette_size_3;
extern Display *display;

extern char disk_last_filename[300];



extern GC gc;

extern Widget dshell1, dshell2;
extern Widget dshell3;
extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;
screen_data *gsd=NULL;
extern int verbose_flag;

/* prototypes */
void gif_output_file_select_callback(Widget w, XtPointer client_data, XtPointer call_data);
void gif_output_file_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);
void gif_output_file_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void output_image_to_gif(char *filename);
/* CVG 9.0 */
int check_gif_libs(int install_type);

extern void dispatch_packet_type(int packet, int offset); 
extern void draw_az_lines();
extern void draw_range_rings();
extern void display_map();

extern int global_pixel_to_color(int the_pixel, int *found);


extern void help_window_callback(Widget w, XtPointer client_data, XtPointer call_data);

#endif
