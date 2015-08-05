/*
 * RCS info
 * $Author$
 * $Locker$
 * $Date$
 * $Id$
 * $Revision$
 * $State$
 */

/************************************************************************
Module:         s3_t2_prod_struct.h

Description:    contains the data structure for reading a radial-based
                intermediate product through a linear buffer. 
                
Authors:        Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                Version 1.0, November 2008
                
$Id$
************************************************************************/

/************************************************************************
This is the structure used to read and write the intermediate product
produced by sample algorithm 3 task 1.
**************************************************************************/

#ifndef _S3_T2_PROD_STRUCT_H_
#define _S3_T2_PROD_STRUCT_H_

/********* Input Intermediate Product Structure *********/

typedef struct {
    /* common header */
    int linearbuffer_id;
    int flag;                /* for use by algorithm. Sample Algorithm 3 */
                             /* uses this to pass a "last elevation" flag*/

    /* radial header */
    int num_range_bins;      /* number of bins in each radial            */
    int num_radials;
    int num_bytes_per_bin;   /* currently always 1                       */

    
} s3_t1_intermed_prod_hdr;

#define S3_T1_DATA_OFFSET 5

    /* in the intermediate product, the actual data follows the header:  */
    /* NOTE: task 1 ensures num_range_bins is always even for alignment  */
    
    /* an un-named series of num_radials 4-byte integers for start angle */
    /* an un-named series of num_radials 4-byte integers for angle delta */
   
    /* an un-named series of (num_radials * num_range_bins) 1-byte       */
    /* integers (unsigned char) for all of the data bins                 */
   
    /* an un-named memory block containing the Base_data_header          */

/********* END Input Intermediate Product Structure *********/




/********* Internal Radial Structure for Task 2 *********/
typedef struct {
    /* common header */
    int linearbuffer_id;
    int flag;                /* for use by algorithm. Sample Algorithm 3 */
                             /* uses this to pass a "last elevation" flag*/

    /* radial header */
    int num_range_bins;      /* number of bins in each radial            */
    int num_radials;
    int num_bytes_per_bin;   /* currently always 1                       */
    
    /* actual data */
    int *start_angle;
    int *angle_delta;
    int **radial_data;      /* each radial allocated individually */ 
    
    /* base data header */
    Base_data_header *bdh;
    
} s3_t2_internal_rad;

/********* END Internal Radial Structure for Task 2 *********/



#endif
