/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:54 $
 * $Id: message_header.h,v 1.3 2009/05/15 17:37:54 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/* message_header.h */

#ifndef _MESSAGE_HEADER_H_
#define _MESSAGE_HEADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <product.h>

#define FALSE 0
#define TRUE 1

char *month[]={"MMM","Jan","Feb","Mar","Apr","May","Jun","Jul",
               "Aug","Sep","Oct","Nov","Dec"};
                  
int print_message_header(char* buffer);
int print_pdb_header(char* buffer);
extern void calendar_date (short,int*,int*,int*);
extern char *msecs_to_string (int time);
extern char *_88D_secs_to_string(int time);
/* CVT 4.4.1 */
extern int read_orpg_product_int( void *loc, void *value );

#endif


