/* display_TAB.h */

/*
 * RCS info
 * $Author:
 * $Date: 2008/03/13 22:45:55 $
 * $Id: display_TAB.h,v 1.7 2008/03/13 22:45:55 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#ifndef _DISPLAY_TAB_H_
#define _DISPLAY_TAB_H_

#include <stdlib.h>

#include <stdio.h>
#include <product.h>
#include <Xm/Xm.h>
#include <Xm/MwmUtil.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include "global.h"

#define MAX_NUM_LINES 17

typedef struct {
    Widget label;
    int *cur_page;
    char *product;
    int offset;
int tabscreen;
} tab_info;

int tabheight = 255, tabwidth = 580;
extern int tab1x, tab1y, tab2x, tab2y;
extern int tab3x, tab3y;

/* extern int disp_width, disp_height;  // size of X display */

extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;
extern int verbose_flag;

extern assoc_array_i *msg_type_list;

extern Widget shell;

extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);
extern int read_word(char *buffer,int *offset);

void display_TAB(int offset);
void tab_next_callback(Widget w, XtPointer client_data, XtPointer call_data);
void tab_prev_callback(Widget w, XtPointer client_data, XtPointer call_data);
void tab_destroy_callback(Widget w, XtPointer client_data, XtPointer call_data);
void close_tab_window(int screen);
int get_tab_page(int offset, int tab_page, int tabscreen);

#endif



