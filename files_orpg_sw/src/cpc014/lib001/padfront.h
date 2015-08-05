/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:25:04 $
 * $Id: padfront.h,v 1.1 2004/01/21 20:25:04 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/************************************************************************
Module:         padfront.h  

Description:    include file for padfront.c that pads missing bytes to 
                0 at the beginning of a run length encoded radial         
                
Authors:        Andy Stern, Software Engineer, Mitretek Systems
                    astern@mitretek.org
                Tom Ganger, Systems Engineer,  Mitretek Systems
                    tganger@mitretek.org
                Version 1.0, October 2000

*************************************************************************/

#ifndef _PADFRONT_H_
#define _PADFRONT_H_

#include <stdio.h>
#include <math.h>  /* for fmod */
#include <stdlib.h> /* for malloc */

#define FALSE 0
#define TRUE  1

int pad_front(int start_index, int buff_step, short* outbuf, int outbuf_index,
      int* sub_from_start);

#endif

