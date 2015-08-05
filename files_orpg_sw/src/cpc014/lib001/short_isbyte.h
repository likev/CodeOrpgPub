/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:25:10 $
 * $Id: short_isbyte.h,v 1.1 2004/01/21 20:25:10 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/************************************************************************
Module:         short_isbyte.h  

Description:    include file for short_isbyte.c which is called by    
                padfront.c, padback.c and radial_run_length_encode.c.
                The module is used to stuff the run length encode
                value into the proper byte of an output buffer word.
                
Authors:        Andy Stern, Software Engineer, Mitretek Systems
                    astern@mitretek.org
                Tom Ganger, Systems Engineer,  Mitretek Systems
                    tganger@mitretek.org
                Version 1.0, October 2000

*************************************************************************/
#ifndef _SHORT_ISBYTE_H_
#define _SHORT_ISBYTE_H_

#include <stdio.h>  /* printf */

/* boolean constants */
#define FALSE 0
#define TRUE  1

/* Local Prototypes ----------------------------------------------------*/
int short_isbyte(short input_word, unsigned short* value, int store_position);

#endif




