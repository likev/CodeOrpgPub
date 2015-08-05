/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 22:07:24 $
 * $Id: recclprods_color_table.h,v 1.2 2002/11/26 22:07:24 nolitam Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/************************************************************************
Module:        recclprods_color_table.h

Description:   include file for the REC color table generator of 
               cpc004/tsk007 which creates output for the Radar Echo Classifier.
               This file contains module specific include file listings, 
               function prototypes and constant definitions.
                
CCR#:          NA98-35001
               
Authors:       Andy Stern, Software Engineer, Mitretek Systems
                   astern@mitretek.org
               Tom Ganger, Systems Engineer,  Mitretek Systems
                   tganger@mitretek.org
               Version 1.0, January 2002
               
History:       
               Initial implementation 1/31/02 - Stern
               
$Id: recclprods_color_table.h,v 1.2 2002/11/26 22:07:24 nolitam Exp $
************************************************************************/
#ifndef _RECCLPRODS_COLOR_TABLE_H_
#define _RECCLPRODS_COLOR_TABLE_H_

#include <a309.h>  /* for product ID values  */

/* Local Prototypes ----------------------------------------------------*/

void create_color_table(short *clr,int);

#endif


