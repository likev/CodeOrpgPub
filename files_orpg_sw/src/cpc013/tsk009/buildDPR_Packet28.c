/*
 */

#include "buildDPR_SymBlk.h"
#include "qperate_func_prototypes.h"

/******************************************************************************
    Filename: buildDPR_Packet28.c

    Description:
    ============
        Prod_Description() sets the generic product header fields.

    20090123 Prod_Description() was replaced by a call to
    RPGP_build_RPGP_product_t(). It was commented out in case we need to
    revisit it in ther future. Tom Ganger says: "The helper function
    RPGP_build_RPGP_product_t() complies with the ICD. I recommend it's use."
    Brian Klein says: "... if your products or processing require a unique
    method for recording times or other pieces of information, that those unique
    data items be made product parameters. That is the specific purpose of
    product parameters for the generic packet. I wouldn't recommend storing
    an 'average volume time' in the field defined as the 'volume scan time'
    as they are, as you describe, two different things."

    Inputs:
        int              vol_num  - Volume scan number
        int              vcp_num  - Volume Coverage Pattern number
        Rate_Buf_t*      rate_out - rate output buffer
        Siteadp_adpt_t*  sadpt    - site adaptation data

    Output:
        RPGP_product_t*  ptrXDR   - pointer to XDR formatted generic product

    Change History
    ==============
    DATE        VERSION   PROGRAMMER          NOTES
    ----        -------   ----------          -----
    06/01/07    0000      Cham Pham           Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).
    01/27/09    0001      James Ward          Commented out in favor of
                                              RPGP_build_RPGP_product_t().
******************************************************************************/

/*
 * void Prod_Description(RPGP_product_t* ptrXDR, int vol_num, int vcp_num,
 *                       Rate_Buf_t* rate_out, Siteadp_adpt_t* sadpt)
 * {
 *    char*         str;      -* Holds working strings   *-
 *    short         mode;     -* weather mode            *-
 *    unsigned int  gen_time; -* product generation time *-
 *    Scan_Summary* scan_sum; -* Scan Summary Table      *-
 *
 *    -* Product Name *-
 *
 *    str = "Digital Precipitation Rate (DPR)";
 *    ptrXDR->name = (char*) malloc( strlen(str) + 1 );
 *    strcpy(ptrXDR->name, str );
 *
 *    -* Product Description *-
 *
 *    str = "Data array product output of QPE RATE algorithm";
 *    ptrXDR->description = (char*) malloc( strlen(str) + 1 );
 *    strcpy(ptrXDR->description, str );
 *
 *    -* Product Code *-
 *
 *    ptrXDR->product_id = DPRCODE;
 *
 *    -* Type of Product (i.e Volume) *-
 *
 *    ptrXDR->type = RPGP_VOLUME;
 *
 *    -* 20080827 Ward - Instead of using the Unix seconds at the product
 *     * generation time (RPGP_product_t->gen_time), use our calculated
 *     * average elevation time (rate_supl.time). Straight Unix time causes
 *     * a prod_cmpr to differ in halfword 121. Old code:
 *     *
 *     * Product Generation time "since midnight GMT Jan. 1, 1970 (UNIX Time)"
 *     *
 *     * time_t  unix_sec; -* Seconds from midnight, Jan 1, 1970 *-
 *     *
 *     * unix_sec = time(NULL);
 *     * ptrXDR->gen_time = (unsigned int) RPG_TIME_IN_SECONDS( unix_sec );
 *     *
 *     * Note: RPG_TIME_IN_SECONDS(), defined in ./include/orpg_def.h,
 *     * converts Unix seconds to seconds since midnight.
 *     *
 *     * 20090116 Tom Ganger points out that "Appendix E to the Class 1 ICD
 *     * states (via a note) that all times in the generic product structure
 *     * header should be in Unix time (seconds after 1/1/1970). Old code:
 *     *
 *     * gen_time = (unsigned int) RPG_TIME_IN_SECONDS(rate_out->rate_supl.time);
 *     *-
 *
 *    gen_time = (unsigned int) RPG_TIME_IN_SECONDS(rate_out->rate_supl.time);
 *
 *    RPGC_set_product_int( (void *) &(ptrXDR->gen_time), gen_time);
 *
 *    -* Radar Name *-
 *
 *    ptrXDR->radar_name = (char*) malloc(strlen(sadpt->rpg_name) + 1 );
 *    strcpy(ptrXDR->radar_name, sadpt->rpg_name );
 *
 *    -* Radar latitude and longitude *-
 *
 *    RPGCS_xy_to_latlon((float) 0.0, (float) 0.0,
 *                       &ptrXDR->radar_lat, &ptrXDR->radar_lon);
 *
 *    -* Radar height *-
 *
 *    ptrXDR->radar_height = sadpt->rda_elev * FT_TO_M;
 *
 *    -* Volume Scan Time in Seconds *-
 *
 *    ptrXDR->volume_time = (unsigned int) rate_out->rate_supl.time;
 *
 *    -* Elevation Time (Unused) *-
 *
 *    ptrXDR->elevation_time = (unsigned int) 0;
 *
 *    -* Elevation angle to build hybrid rate *-
 *
 *    ptrXDR->elevation_angle = (float) rate_out->rate_supl.highest_elang;
 *
 *    -* Volume scan number *-
 *
 *    ptrXDR->volume_number = (int) vol_num;
 *
 *    -* Operation mode *-
 *
 *   scan_sum = RPGC_get_scan_summary( vol_num );
 *   mode     = (short) scan_sum->weather_mode;
 *
 *   switch( mode )
 *   {
 *      case MAINTENANCE_MODE:
 *         ptrXDR->operation_mode = RPGP_OP_MAINTENANCE;
 *         break;
 *
 *      case CLEAR_AIR_MODE:
 *         ptrXDR->operation_mode = RPGP_OP_CLEAR_AIR;
 *         break;
 *
 *      case PRECIPITATION_MODE:
 *      default:
 *         ptrXDR->operation_mode = RPGP_OP_WEATHER;
 *         break;
 *   }
 *
 *   -* Volume Coverage Pattern *-
 *
 *   ptrXDR->vcp = (short) vcp_num;
 *
 *   -* Elevation Number within the VCP (Unused) *-
 *
 *   ptrXDR->elevation_number = (short) 0;
 *
 *   -* Compression type (Unused) *-
 *
 *   ptrXDR->compress_type = (short) 0;
 *
 *   -* Size after decompressing (Unused) *-
 *
 *   ptrXDR->size_decompressed = (int) 0;
 *
 * } -* end Prod_Description( ) ------------------------------ *-
 */

/******************************************************************************
    Filename: buildDPR_Packet28.c

    Description:
    ============
        Build_RadialComp() creates radial data product component.

    Inputs:
       unsigned short   RateData[][MAX_BINS] - Rain Rate data
                                               (1000th of inches/hr)
    Output:
       RPGP_product_t*  ptrXDR               - pointer to XDR formatted
                                               generic product

    Change History
    ==============
    DATE        VERSION   PROGRAMMER          NOTES
    --------    -------   ----------          -----
    20070601    0000      Cham Pham           Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).
    20100107    0001      James Ward          Fix for CCR NA09-00054
                                              CHANGE PACKET 28 RADIAL ANGLE DEFINITION
    "The radial angle definition for the radial component specifies center angle.
     This is inconsistent with all other radial-based packet definitions.
     Reason for Change: Change the definition for radial angle from 'center' to 'leading edge'.
     Leading edge is defined as the counter-clockwise edge angle for the start of the radial."
******************************************************************************/

void Build_RadialComp(RPGP_product_t* ptrXDR,
                      unsigned short RateData[MAX_AZM][MAX_BINS])
{
   int   rad, bin;            /* Variables hold loop index for rate data  */
   unsigned short *rate_data; /* Array to hold rain rate data             */
   char  *str = '\0';         /* Contain string                           */

   static unsigned int rpgp_rad_size   = sizeof(RPGP_radial_t);
   static unsigned int rpgp_data_size  = sizeof(RPGP_radial_data_t);
   static unsigned int rpgp_param_size = sizeof(RPGP_parameter_t);

   RPGP_radial_t* radial;

   #ifdef QPERATE_DEBUG
      fprintf( stderr, "Enter Build_RadialComp() ..............\n" );
   #endif

   /* We use the radial component for DPR product */

   if(rpgp_rad_size > 0)
      radial = malloc(rpgp_rad_size);
   else
      radial = NULL;

   /* Allocate array memory for the component type */

   if(NUM_COMP > 0)
      ptrXDR->components = (void **) malloc (sizeof (char *) * NUM_COMP);
   else
      ptrXDR->components = NULL;

   ptrXDR->components[0] = (char *) radial;

   /* This is radial component type. Initialize radial component */

   radial->comp_type = RPGP_RADIAL_COMP;

   /* Component Description */

   str = "Rate Data array product output";
   radial->description = (char*)malloc( strlen(str) + 1 );
   strcpy( radial->description, str );

   /* Size of each range bin meters */

   radial->bin_size = (float) 250.0;

   /* Range of the center of the first bin in meters */

   radial->first_range = (float) 125.0;

   #ifdef QPERATE_DEBUG
   {
      fprintf( stderr, "\nRADIAL COMPONENT FIELDS\n" );
      fprintf( stderr, "radial->comp_type: %d\n", radial->comp_type );
      fprintf( stderr, "radial->description %s\n", radial->description );
      fprintf( stderr, "radial->bin_size: %.f\n", radial->bin_size );
      fprintf( stderr, "radial->first_range: %.f\n", radial->first_range );
   }
   #endif

   /* Set number of component parameter */

   radial->numof_comp_params = NUM_COMP_PARAM;

   /* Allocate array memory for the component parameters. */

   if((rpgp_param_size > 0) && (NUM_COMP_PARAM > 0))
      radial->comp_params = malloc(rpgp_param_size * NUM_COMP_PARAM);
   else
      radial->comp_params = NULL;

   /* Set component parameter attribute */

   #ifdef QPERATE_DEBUG
   if(radial->numof_comp_params == 0)
   {
      fprintf ( stderr, "DPR generic product format does not have component "
                        "parameter\n" );
   }
   #endif
   /* Set number of radials */

   radial->numof_radials = (int) MAX_AZM;

   /* Allocate array memory for the radial data. */

   if((radial->numof_radials> 0) && (rpgp_data_size > 0))
      radial->radials = malloc(radial->numof_radials * rpgp_data_size);
   else
      radial->radials = NULL;

   /* List of radials */

   for ( rad = 0; rad < radial->numof_radials; rad++ )
   {
      /* 20100107 CCR NA09-00054
       *
       * old code:
       *
       * radial->radials[rad].azimuth   = (float) 0.5 + rad;
       */
      radial->radials[rad].azimuth   = (float) rad;
      radial->radials[rad].elevation = (float) 0.0;
      radial->radials[rad].width     = (float) 1.0;
      radial->radials[rad].n_bins    = (int)   MAX_BINS;

      #ifdef QPERATE_DEBUG
      {
         fprintf( stderr, "\nLIST OF RADIALS \n" );
         fprintf( stderr, "radial->numof_radials: %d\n",
                          radial->numof_radials );
         fprintf( stderr, "radial->radials[%d].azimuth %.1f\n",
                          rad, radial->radials[rad].azimuth );
         fprintf( stderr, "radial->radials[%d].elevation %.2f\n",
                          rad, radial->radials[rad].elevation );
         fprintf( stderr, "radial->radials[%d].width: %.0f\n",
                          rad, radial->radials[rad].width );
         fprintf( stderr, "radial->radials[%d].n_bins: %d\n",
                          rad, radial->radials[rad].n_bins );
      }
      #endif

      /* Set the rain rate data attributes in text format */

      str = "type = ushort; Unit = inches/hour";
      radial->radials[rad].bins.attrs = (char *)malloc( strlen(str) + 1 );
      strcpy( radial->radials[rad].bins.attrs, str );

      #ifdef QPERATE_DEBUG
         fprintf( stderr, "radial->radials->bins.attrs: %s\n",
                          radial->radials[rad].bins.attrs );
      #endif

      /* Allocate array memory for bin data. */

      if(radial->radials[rad].n_bins > 0)
      {
         radial->radials[rad].bins.data = malloc(radial->radials[rad].n_bins *
                                              sizeof(unsigned short));
      }
      else
         radial->radials[rad].bins.data = NULL;

      rate_data = (unsigned short *) radial->radials[rad].bins.data;

      for ( bin = 0; bin < radial->radials[rad].n_bins; bin++ )
      {
          rate_data[bin] = RateData[rad][bin];
      }

   } /* end rad loop */

   #ifdef QPERATE_DEBUG
      fprintf( stderr, "\nExit Build_RadialComp() \n" );
   #endif

} /* end Build_RadialComp () ------------------------------- */
