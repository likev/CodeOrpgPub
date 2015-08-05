/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/03/03 20:15:57 $
 * $Id: mem_use.h,v 1.1 2009/03/03 20:15:57 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef MEM_USE_H
#define MEM_USE_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* Global Variables.  */

/* Function Prototypes. */
int MU_initialize( int pid, int long_listing );
int MU_display_smaps();
void MU_print_max_values();

#endif
