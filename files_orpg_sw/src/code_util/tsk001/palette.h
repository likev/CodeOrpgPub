/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:51 $
 * $Id: palette.h,v 1.3 2009/05/15 17:52:51 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/* palette.h */
/* data structures for dealing with color palettes */

/* we keep the default palette in the first position of the arrays  */
typedef struct {
    int num_palettes;
    char packet_title[50];
    char **titles, **filenames;
} packet_palette;

#define MAX_PACKETS 45          /* Array size for holding pointers  */
                                /* to each packet structure.        */



