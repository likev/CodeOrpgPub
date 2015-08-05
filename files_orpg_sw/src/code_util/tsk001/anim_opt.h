/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:45:02 $
 * $Id: anim_opt.h,v 1.7 2008/03/13 22:45:02 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* anim_opt.h */


#ifndef _ANIM_OPT_H_
#define _ANIM_OPT_H_

#include <Xm/Xm.h>
#include <Xm/TextF.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/FileSB.h>
#include <Xm/ToggleB.h>
#include <Xm/MessageB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/SelectioB.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>



#include "global.h"

extern Widget dshell1, dshell2, shell;
extern Widget dshell3;

/* extern int selected_screen; */

extern int linked_flag;


/* the animation data structures for each screen, see global.h for typedef */
extern anim_data anim1, anim2, anim3;
/* anim_data *anim; */

/* stuff for time series animation */


/* CVG 6.5 */
/*  keep these ---------------------------------- */
int user_reset_time_s1, user_reset_time_s2;
int user_reset_time_s3;
/*  ---------------------------------------------                                    */
/* prototypes */                   
                                   
void time_option_callback(Widget w, XtPointer client_data, XtPointer call_data);
void open_time_opt_dialog(int screen_N);

void file_option_callback(Widget w, XtPointer client_data, XtPointer call_data);
                                   

void time_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void ts_pref_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);

void loop2_callback(Widget w, XtPointer client_data, XtPointer call_data);
void loop3_callback(Widget w, XtPointer client_data, XtPointer call_data);
void loop4_callback(Widget w, XtPointer client_data, XtPointer call_data);
void loop5_callback(Widget w, XtPointer client_data, XtPointer call_data);
void loop6_callback(Widget w, XtPointer client_data, XtPointer call_data);
void loop8_callback(Widget w, XtPointer client_data, XtPointer call_data);
void loop10_callback(Widget w, XtPointer client_data, XtPointer call_data);
void loop_no_limit_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void reset_time_series(int screen_num, int type_init);

extern void help_window_callback(Widget w, XtPointer client_data, XtPointer call_data);


#endif
