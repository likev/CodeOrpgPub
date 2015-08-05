/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:46:08 $
 * $Id: help.h,v 1.5 2008/03/13 22:46:08 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* help.h */

#ifndef _HELP_H_
#define _HELP_H_

#include <Xm/Xm.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/ScrollBar.h>
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
/* for obtaining file size */
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "global.h"

/* extern screen_data *sd1, *sd2, *sd; */
extern int verbose_flag;

extern char config_dir[255];

/* prototypes */
void help_window_callback(Widget w, XtPointer client_data, XtPointer call_data);

#endif
