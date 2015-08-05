/* ping_output.h */

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:52 $
 * $Id: png_output.h,v 1.6 2009/05/15 17:52:52 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifndef _PNG_OUTPUT_H_
#define _PNG_OUTPUT_H_

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
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#include <gd.h>

#include "global.h"


extern int standalone_flag;

extern int palette_size;
extern Dimension pwidth, pheight, barwidth;

extern int img_size;

extern int global_palette_size_1;
extern int global_palette_size_2;
extern int global_palette_size_3;
extern XColor global_display_colors_1[256];
extern XColor global_display_colors_2[256];
extern XColor global_display_colors_3[256];
extern Display *display;
extern GC gc;

extern Widget dshell1, dshell2;
extern Widget dshell3;
extern screen_data *sd, *sd1, *sd2;
extern screen_data *sd3;
screen_data *psd=NULL;
extern char disk_last_filename[300];


/* prototypes */
void png_output_file_select_callback(Widget w, XtPointer client_data, XtPointer call_data);
void png_output_file_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);
void png_output_file_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void output_image_to_png(char *filename);

int check_png_libs(int install_type); 

extern int global_pixel_to_color(int the_pixel, int *found);

extern void help_window_callback(Widget w, XtPointer client_data, XtPointer call_data);

#endif

