/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 22:07:30 $
 * $Id: short_isbyte.h,v 1.2 2002/11/26 22:07:30 nolitam Exp $
 * $Revision: 1.2 $
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
$Id: short_isbyte.h,v 1.2 2002/11/26 22:07:30 nolitam Exp $
*************************************************************************/
#ifndef _SHORT_ISBYTE_H_
#define _SHORT_ISBYTE_H_

#include <stdio.h>  /* printf */

/* boolean constants */
#define FALSE 0
#define TRUE  1

/* Local Prototypes ----------------------------------------------------*/
int short_isbyte(short input_word, short* value, int store_position);

#endif




