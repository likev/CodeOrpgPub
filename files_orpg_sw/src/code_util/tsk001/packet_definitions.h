/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:50 $
 * $Id: packet_definitions.h,v 1.7 2009/05/15 17:52:50 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
#ifndef _PACKET_DEFINITIONS_H_
#define _PACKET_DEFINITIONS_H_


/* ===============================================================
This section lists the available packet types (excluding map data)
based upon the current ICD. For traditional data the packet code 
column indicates the packet value that must be included within each 
packet structure.  For generic product components the code column
is the component number plus 2800.

The order will be maintained within the enumeration structure.

DIGITAL_RASTER_DATA has not been defined and is not currently used 

 Packet
Idx Code      Enumerated Structure Name  
--------------------------------------------------------------
0    0        NOT USED
1    1        TEXT_AND_SPECIAL_SYMBOL_TEXT_NO_VALUE  
2    2        TEXT_AND_SPECIAL_SYMBOL_SYMBOL_NO_VALUE
3    3        MESOCYCLONE_DATA  
4    4        WIND_BARB_DATA    
5    5        VECTOR_ARROW_DATA 
6    6        LINKED_VECTOR_NO_VALUE  
7    7        UNLINKED_VECTOR_NO_VALUE
8    8        TEXT_AND_SPECIAL_SYMBOL_TEXT_UNIFORM_VALUE 
9    9        LINKED_VECTOR_UNIFORM_VALUE   
10   10       UNLINKED_VECTOR_UNIFORM_VALUE 
11   11       CORRELATED_SHEAR_MESO (3D)    
12   12       TVS_DATA 
13   13       POSITIVE_HAIL_DATA       No Longer Used  
14   14       PROBABLE_HAIL_DATA       No Longer Used  
15   15       STORM_ID_DATA              
16   16       DIGITAL_RADIAL_DATA_ARRAY  
17   17       DIGITAL_PRECIP_DATA_ARRAY  
18   18       PRECIP_RATE_DATA_ARRAY 
19   19       HDA_HAIL_DATA 
20   20       POINT_FEATURE_DATA 
21   21       CELL_TREND_DATA                Not Graphically Displayed 
22   22       CELL_TREND_VOLUME_SCAN_TIME    Not Graphically Displayed 
23   23       SCIT_PAST_POSITION_DATA 
24   24       SCIT_FORECAST_POSITION_DATA  
25   25       STI_CIRCLE_DATA   
26   26       ETVS_DATA
27   27       SUPEROB_WIND_DATA 
28   28       GENERIC_PRODUCT_DATA
29   29       UNDEF_B
30   30       UNDEF_C
31   31       UNDEF_D
32   32       UNDEF_E
33   33       UNDEF_F
34   34       UNDEF_G
35   35       UNDEF_H
36   36       UNDEF_I
37   37       UNDEF_J
38   38       UNDEF_K
39   39       UNDEF_L
40   40       UNDEF_M
41   2801     GENERIC_RADIAL_DATA  
42   2802     GENERIC_GRID_DATA    
43   2803     GENERIC_AREA_DATA    
44   2804     GENERIC_TEXT_DATA    
45   2805     GENERIC_TABLE_DATA   
46   2806     GENERIC_EVENT_DATA   
47   47       UNDEF_T
48   48       UNDEF_U
49   49       UNDEF_V
50   0802x    CONTOUR_VECTOR_COLOR    
51   0E03x    CONTOUR_VECTOR_LINKED   
52   3501x    CONTOUR_VECTOR_UNLINKED 
53   AF1Fx    RADIAL_DATA_16_LEVELS   
54   BA07x    RASTER_DATA_7           
55   BA0Fx    RASTER_DATA_F           
56            TABULAR_ALPHA_BLOCK     
57            GRAPHIC_ALPHA_BLOCK 
58            STAND_ALONE_TAB_ALPHA_BLOCK 
59   59       Digital Raster Data
60   60       BACKGROUND_MAP_DATA
61   61       AZIMUTH_LINE_RANGE_RING
62   62       LEGEND_TEXT_DATA
============================================================== 
PREVIOUS DEFINITION:                     
27   0802x    CONTOUR_VECTOR_COLOR       
28   0E03x    CONTOUR_VECTOR_LINKED      
29   3501x    CONTOUR_VECTOR_UNLINKED    
30   AF1Fx    RADIAL_DATA_16_LEVELS      
31   BA07x    RASTER_DATA_7   
32   BA0Fx    RASTER_DATA_F   
33            Tabular Alphanumeric Block 
34            Graphic Alphanumeric Block 
============================================================= */

/* enumeration of the data packet structures: used to access the 
array of pointers which point to the structures */
/* THIS LIST CORRESPONDS TO A CHARACTER ARRAY IN PACKETSELECT.H           */
/*     The relative position in both lists must be maintained.            */
/*                                                                        */
/*     If the position of the packets with Hex values (CONTOUR_VECTOR_,   */
/*     RADIAL_DATA_16, RASTER_DATA_, etc.) or the generic component data  */
/*     is changed:                                                        */
/*          A. the function transfer_packet_code() in packetselect.c      */
/*             must be modified.                                          */
/*          B. CVG configuration files prod_config and palette_list       */
/*             must be modified.                                          */
/*                                                                        */
/*     The affect of changing the position of the GAB and TAB is unknown. */


/*** NEW PACKET ***/
enum {NO_VAL,TEXT_AND_SPECIAL_SYMBOL_TEXT_NO_VALUE,
    TEXT_AND_SPECIAL_SYMBOL_SYMBOL_NO_VALUE,MESOCYCLONE_DATA,WIND_BARB_DATA,
    VECTOR_ARROW_DATA,LINKED_VECTOR_NO_VALUE,
    UNLINKED_VECTOR_NO_VALUE,TEXT_AND_SPECIAL_SYMBOL_TEXT_UNIFORM_VALUE,
    LINKED_VECTOR_UNIFORM_VALUE,UNLINKED_VECTOR_UNIFORM_VALUE,
    CORRELATED_SHEAR_MESO,TVS_DATA,POSITIVE_HAIL_DATA,PROBABLE_HAIL_DATA,
    STORM_ID_DATA,DIGITAL_RADIAL_DATA_ARRAY,DIGITAL_PRECIP_DATA_ARRAY,
    PRECIP_RATE_DATA_ARRAY,HDA_HAIL_DATA,POINT_FEATURE_DATA,CELL_TREND_DATA,
    CELL_TREND_VOLUME_SCAN_TIME,SCIT_PAST_POSITION_DATA,
    SCIT_FORECAST_POSITION_DATA,STI_CIRCLE_DATA,ETVS_DATA,SUPEROB_WIND_DATA,
    GENERIC_PRODUCT_DATA,UNDEF_B,UNDEF_C,UNDEF_D,
    UNDEF_E,UNDEF_F,UNDEF_G,UNDEF_H,
    UNDEF_I,UNDEF_J,UNDEF_K,UNDEF_L,
    UNDEF_M,GENERIC_RADIAL_DATA,GENERIC_GRID_DATA,GENERIC_AREA_DATA,
    GENERIC_TEXT_DATA,GENERIC_TABLE_DATA,GENERIC_EVENT_DATA,UNDEF_T,
    UNDEF_U,UNDEF_V,
    CONTOUR_VECTOR_COLOR,CONTOUR_VECTOR_LINKED,CONTOUR_VECTOR_UNLINKED,
    RADIAL_DATA_16_LEVELS,RASTER_DATA_7,RASTER_DATA_F,
    TABULAR_ALPHA_BLOCK,GRAPHIC_ALPHA_BLOCK,STAND_ALONE_TAB_ALPHA_BLOCK,
    DIGITAL_RASTER_DATA,BACKGROUND_MAP_DATA,AZIMUTH_LINE_RANGE_RING,
    LEGEND_TEXT_DATA};
 

#endif

