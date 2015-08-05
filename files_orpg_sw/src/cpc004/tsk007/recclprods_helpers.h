/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 22:07:26 $
 * $Id: recclprods_helpers.h,v 1.3 2002/11/26 22:07:26 nolitam Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/************************************************************************
Module:        recclprods_helpers.h

Description:   include file for the REC helper module of 
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
               
$Id: recclprods_helpers.h,v 1.3 2002/11/26 22:07:26 nolitam Exp $
************************************************************************/
#ifndef _RECCLPRODS_HELPERS_H_
#define _RECCLPRODS_HELPERS_H_


#include <stdio.h>
#include <stdlib.h>
#include <rpgc.h>

#include <recclalg_arrays.h>

int radials_to_process(char *);
short get_max_dop(short *);

#endif


