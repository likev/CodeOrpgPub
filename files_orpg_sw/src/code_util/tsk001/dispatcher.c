/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/08/19 15:11:41 $
 * $Id: dispatcher.c,v 1.10 2009/08/19 15:11:41 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */
/* dispatcher.c */

#include "dispatcher.h"


/* accept the packet number that should be displayed. 
 * the input value (int packet) corresponds to the
 * index of the packet definition array 
 */
/* called from the following in prod_display.c
 *   the plot_image function
 *   the add_or_page_gab function
 *   the add_tab function
 * called from the following in display_GAB.c
 *   the display_GAB function
 * called from the following in packet_23.c
 *   the display_packet_23 function
 * called from the following in packet_24.c 
 *   the display_packet_24 function
 */

void dispatch_packet_type(int packet, int offset, int replay) 
{
    if(verbose_flag)
         printf("\n Dispatch Packet = %d or (%x Hex) with offset = %d\n",
                packet,packet,offset);


    /* CVG 9.0 - only check for an overriding configured palette for   */
    /* those packets that the product configuration dialog permits, the  */
    /* first list must agree with the assocpacket_opt buttons in prefs.c */
    /* and the product_edit_fill_fields options in prefs_edit.c          */
    /* the alternative is to allways call open_config_or_default_palette */
    switch(packet) {
    /* cvg 9.1 - added packet 4 - wind barb */
    /* 4 */  case WIND_BARB_DATA:
    /* 6 */  case LINKED_VECTOR_NO_VALUE:
    /* 8 */  case TEXT_AND_SPECIAL_SYMBOL_TEXT_UNIFORM_VALUE:
    /* 9 */  case LINKED_VECTOR_UNIFORM_VALUE:
    /* 10 */ case UNLINKED_VECTOR_UNIFORM_VALUE:
    /* 16 */ case DIGITAL_RADIAL_DATA_ARRAY:
    /* 17 */ case DIGITAL_PRECIP_DATA_ARRAY: 
    /* 20 */ case POINT_FEATURE_DATA:
    /* 41 */ case GENERIC_RADIAL_DATA:
    /* 42 */ case GENERIC_GRID_DATA:
    /* 43 */ case GENERIC_AREA_DATA:
    /* 51 */ case CONTOUR_VECTOR_LINKED:
    /* 53 */ case RADIAL_DATA_16_LEVELS: 
    /* 54 */ case RASTER_DATA_7:
    /* 55 */ case RASTER_DATA_F:
                {
                /* CVG 9.0 - changed function name from setup_colors */
                open_config_or_default_palette(packet,FALSE);
                break;
                }
    /* CVG 9.0 - ADDED FOLLOWING NO OP for clarity */
    /*          these packets dispatch other packets */
    /* 23 */ case SCIT_PAST_POSITION_DATA:
    /* 24 */ case SCIT_FORECAST_POSITION_DATA:
    /* 56 */ case TABULAR_ALPHA_BLOCK:
    /* 57 */ case GRAPHIC_ALPHA_BLOCK:
    /* 58 */ case STAND_ALONE_TAB_ALPHA_BLOCK:
                break;
             default: 
                open_default_palette(packet);
                break;
    }
    

      
       
      /* DELETING IMAGE DATA WAS MOVED TO PLOT_IMAGE() IN PROD_DISPLAY.C */
      /*                       AND ONLY BE DONE IF REPLAY IS FALSE       */
      






    switch(packet) {
      case TEXT_AND_SPECIAL_SYMBOL_TEXT_NO_VALUE:
          display_packet_1(packet, offset);
          break;
      case TEXT_AND_SPECIAL_SYMBOL_SYMBOL_NO_VALUE:
          display_packet_2(packet, offset);
          break;
      case MESOCYCLONE_DATA:
          display_meso_data(packet, offset);
          break;
      case WIND_BARB_DATA:
          display_packet_4(packet, offset);
          break;
      case VECTOR_ARROW_DATA:
          display_packet_5(packet, offset);
          break;
      case LINKED_VECTOR_NO_VALUE:
          display_packet_6(packet, offset);
          break;
      case UNLINKED_VECTOR_NO_VALUE:
          display_packet_7(packet, offset);
          break;
      case TEXT_AND_SPECIAL_SYMBOL_TEXT_UNIFORM_VALUE:
          display_packet_8(packet, offset);
          break;
      case LINKED_VECTOR_UNIFORM_VALUE:
          display_packet_9(packet, offset);
          break;
      case UNLINKED_VECTOR_UNIFORM_VALUE:
          display_packet_10(packet, offset);
          break;
      case CORRELATED_SHEAR_MESO:
          display_shear_3d_data(packet, offset);
          break;
      case TVS_DATA:
          display_tvs_data(packet, offset);
          break;

      /* packet 13 positive hail data, not used */
      
      /* packet 14 probable hail data, not used */

      case STORM_ID_DATA:
          display_storm_id_data(packet, offset);
          break;
      case DIGITAL_RADIAL_DATA_ARRAY:
          if(display_color_bars==TRUE) {
                display_packet_16(packet, offset, replay);
            } else {
                fprintf(stderr," ERROR    ERROR   ERROR   ERROR   ERROR   ERROR\n"
                              " There Is No Configured Palette For Packet 16\n"
                              " ----------------------------------------------\n");
            }
          break;
      case DIGITAL_PRECIP_DATA_ARRAY:
          if(display_color_bars==TRUE) {
                display_packet_17(packet, offset, replay);
            } else {
                fprintf(stderr," ERROR    ERROR   ERROR   ERROR   ERROR   ERROR\n"
                              " There Is No Configured Palette For Packet 17\n"
                              " ----------------------------------------------\n");
            }
          break;
      case PRECIP_RATE_DATA_ARRAY:
          display_packet_18(packet, offset);
          break;
      case HDA_HAIL_DATA:
          display_hail_data(packet, offset);
          break;
      /*** NEW PACKET ***/
      case POINT_FEATURE_DATA:
          display_packet_20(packet, offset);
          break;
      /* packet 21 cell trend data, not displayed graphically */
      
      /* packet 22 cell trend volume scan time, not displayed graphically */
      
      case SCIT_PAST_POSITION_DATA:
          display_packet_23(packet, offset);
          break;
      case SCIT_FORECAST_POSITION_DATA:
          display_packet_24(packet, offset);
          break;
      case STI_CIRCLE_DATA:
          display_packet_25(packet, offset);
          break;
      case ETVS_DATA:
          display_etvs_data(packet, offset);
          break;
      case GENERIC_RADIAL_DATA:
          if(display_color_bars==TRUE) {
                display_packet_28(packet, offset, replay);
            } else {
                fprintf(stderr," ERROR    ERROR   ERROR   ERROR   ERROR   ERROR\n"
                              " No Configured Palette For Generic Radial Componet\n"
                              " ----------------------------------------------\n");
            }
          break;
      case GENERIC_GRID_DATA:
          if(display_color_bars==TRUE) {
                display_packet_28(packet, offset, replay);
            } else {
                fprintf(stderr," ERROR    ERROR   ERROR   ERROR   ERROR   ERROR\n"
                              " No Configured Palette For Generic Grid Componet\n"
                              " ----------------------------------------------\n");
            }
          break;
      case GENERIC_AREA_DATA:
      case GENERIC_TEXT_DATA:
      case GENERIC_TABLE_DATA:
      case GENERIC_EVENT_DATA:
          display_packet_28(packet, offset, replay);
          break;
      case CONTOUR_VECTOR_COLOR:
          display_packet_0802(packet, offset);
          break;
      case CONTOUR_VECTOR_LINKED:
          display_packet_0E03(packet, offset);
          break;
      case CONTOUR_VECTOR_UNLINKED:
          display_packet_3501(packet, offset);
          break;
      case RADIAL_DATA_16_LEVELS:
          if(display_color_bars==TRUE) {
                display_radial_rle(packet, offset, replay);
            } else {
                fprintf(stderr," ERROR    ERROR   ERROR   ERROR   ERROR   ERROR\n"
                              " No Configured Palette For RLE Radial Packet \n"
                              " ----------------------------------------------\n");
            }
          break;
      case RASTER_DATA_7:
      case RASTER_DATA_F:
          if(display_color_bars==TRUE) {
                display_raster(packet, offset, replay);
            } else {
                fprintf(stderr," ERROR    ERROR   ERROR   ERROR   ERROR   ERROR\n"
                              " No Configured Palette For RLE Raster Packet\n"
                              " ----------------------------------------------\n");
            }
          break;
      case GRAPHIC_ALPHA_BLOCK:
          display_GAB(offset);
          break;
      case TABULAR_ALPHA_BLOCK:
          display_TAB(offset);
          break;
/* SATAP PARTIALLY supported */
      case STAND_ALONE_TAB_ALPHA_BLOCK:
          display_TAB(offset);
          break;
      case DIGITAL_RASTER_DATA:
          display_digital_raster(packet, offset);
          break;

      default: 
          if(verbose_flag)
              printf("Warning: packet %d could not be handled!\n", packet);  
          break;

    } /*  end switch */




}



