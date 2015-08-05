/* dispatcher.h */

/*
 * RCS info
 * $Author: 
 * $Date: 2009/08/19 15:11:41 $
 * $Id: dispatcher.h,v 1.10 2009/08/19 15:11:41 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

/**Solaris 8 change**/
#include <stdio.h>

#include "packet_definitions.h"
#include "global.h"

extern screen_data *sd;
/* extern screen_data *sd1, *sd2, *sd3; */

extern int verbose_flag;
extern int overlay_flag;


/* CVG 9.1 */
extern int display_color_bars;



/* prototypes */

void dispatch_packet_type(int packet, int offset, int replay); 

/* CVG 9.0 - changed function name from setup_colors */
extern void open_config_or_default_palette(int packet_type, int legend_flag);
/* CVG 9.0 - ADDED */
extern int open_default_palette(int packet_type);

extern void delete_raster();
extern void delete_radial_rle();
extern void delete_packet_16();
extern void delete_packet_17();
extern void delete_generic_radial();
extern void display_packet_1(int packet, int offset);
extern void display_packet_2(int packet, int offset);
extern void display_meso_data(int packet, int offset);
extern void display_packet_4(int packet, int offset);
extern void display_packet_5(int packet, int offset);
extern void display_packet_6(int packet, int offset);
extern void display_packet_7(int packet, int offset);
extern void display_packet_8(int packet, int offset);
extern void display_packet_9(int packet, int offset);
extern void display_packet_10(int packet, int offset);
extern void display_shear_3d_data(int packet, int offset);
extern void display_tvs_data(int packet, int offset);
extern void display_storm_id_data(int packet, int offset);
extern void display_packet_16(int packet, int offset, int replay);
extern void display_packet_17(int packet, int offset, int replay);
extern void display_packet_18(int packet, int offset);
extern void display_hail_data(int packet, int offset);
/*** NEW PACKET ***/
extern void display_packet_20(int packet, int offset);
extern void display_packet_23(int packet, int offset);
extern void display_packet_24(int packet, int offset);
extern void display_packet_25(int packet, int offset);
extern void display_etvs_data(int packet, int offset);
extern void display_packet_28(int packet, int index, int replay);
extern void display_packet_0802(int packet, int offset);
extern void display_packet_0E03(int packet, int offset);
extern void display_packet_3501(int packet, int offset);
extern void display_raster(int packet, int offset, int replay);
extern void display_radial_rle(int packet, int offset, int replay);
extern void display_GAB(int offset);
extern void display_TAB(int offset);
extern void display_digital_raster(int packet, int offset);

#endif


