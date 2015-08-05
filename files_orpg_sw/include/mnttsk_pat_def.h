/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/07/15 19:30:08 $
 * $Id: mnttsk_pat_def.h,v 1.1 2005/07/15 19:30:08 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef MNTTSK_PAT_DEF_H
#define MNTTSK_PAT_DEF_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <orpg.h>

/* Constant Definitions/Macro Definitions/Type Definitions */
#define STARTUP   1
#define RESTART   2
#define CLEAR     3
#define INIT      4

#define NO_CLEAR_TABLE                  0
#define CLEAR_TABLE                     1
#define CFG_NAME_SIZE                 128

#endif /* #ifndef MNTTSK_PAT_DEF_H */
