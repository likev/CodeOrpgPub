/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:12:19 $
 * $Id: input_buffer_struct.h,v 1.1 2004/01/21 20:12:19 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/*********************************************************************
File    : input_buffer_struct.h
Created : Aug. 7,2003
Details : A file that defines the input structure for the hybrid scan buffer.
	  This is defined in a separate file rather than in saa_main.h to avoid a
	  circular dependency between saa_main.c and saa.c
*********************************************************************/

#ifndef SAA_INPUT_BUFFER_STRUCT_H
#define SAA_INPUT_BUFFER_STRUCT_H

#include <stdlib.h>
#include <rpgc.h>
#include "epre_main.h"

/*renaming EPRE_buf_t defined in epre_main.h to HYBSCAN_buf_t  */
typedef EPRE_buf_t HYBSCAN_buf_t;

/*local copy of the input buffer*/
HYBSCAN_buf_t hybscan_buf;

typedef EPRE_supl_t HYBSCAN_suppl_t;

HYBSCAN_suppl_t hybscan_suppl;


#endif
