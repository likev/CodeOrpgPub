/*$<
 *======================================================================= 
 * 
 *   (c) Copyright, 2012 Massachusetts Institute of Technology.
 *       This material may be reproduced by or for the 
 *       U.S. Government pursuant to the copyright license 
 *       under the clause at 252.227-7013 (Jun. 1995).
 *
 *=======================================================================
 *
 *
 *   FILE: create_grib_lb.h
 *
 *   AUTHOR:  Michael F Donovan
 *
 *   CREATED:  Nov 15, 2011; Initial Version
 *
 *   DESCRIPTION:
 *
 *   This file contains module specific include file listings, function 
 *   prototypes and constant definitions for constructing and writing 
 *   the external model derived grid messages.
 *
 *
 *=======================================================================
 *$>
 */

#ifndef CREATE_GRID_LB_H
#define CREATE_GRID_LB_H

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <bzlib.h>
#include <misc.h>
#include <orpgsite.h>
#include <rpgcs_time_funcs.h>
#include <rpgcs.h>
#include <orpg_product.h>

#define MAX_BLOCK_SIZE 9

typedef struct {
    short msgcode;
    short date_of_msg;
    short time_of_msg_mshw;
    short time_of_msg_lshw;
    short length_of_msg_mshw;
    short length_of_msg_lshw;
    short src_id;
    short dst_id;
    short no_of_blocks;
    } nexrad_header;

typedef struct {
    short block_div;
    short block_id;
    short spare1;
    short comp_type;
    int decomp_size;
    } Ext_msg_hdr;

typedef struct {
    short packet_code;
    short spare2;
    int num_bytes;
    } Packet_29;

RPGP_ext_data_t *prod;

/* Function Prototypes. */
int send_grid_to_lb( int ext_grid_type, RPGP_ext_data_t *ext_data );
int setup_packet( unsigned char **packet, int *packet_len );
int compress_packet( unsigned char *msg, int msg_len, unsigned char *new_msg, int *new_msg_len );
void construct_ext_msg( int msg_len, int packet_len, nexrad_header *msg_hdr, unsigned char *msg );
int write_to_file( int ext_grid_type, unsigned char *env_data_msg, nexrad_header msg_hdr );

int get_msg_date( short *date_of_msg, int *seconds_of_day );

void int_to_char2( unsigned char *msg, short var2, int *indx );
void int_to_char4( unsigned char *msg, int var4, int *indx );

#endif
