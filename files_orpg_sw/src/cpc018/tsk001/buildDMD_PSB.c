/*******************************************************************************
    Filename:   buildDMD_PSB.c
    Author:     Brian Klein


    Description
    ===========
    This function will create the DMD product using the generic, xdr based
    product format.  Returns halfword offset to start of PSB, or zero if no
    detected features or -1 if failure.  Regardless, the length is always
    increased by the byte size of a graphic product header.
  
*******************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/14 13:35:24 $
 * $Id: buildDMD_PSB.c,v 1.17 2014/05/14 13:35:24 steves Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */

/*** Global Include Files ***/

#include <math.h>
#include <rpgc.h>
#include <rpgcs.h>
#include <rpgp.h>
#include <assert.h>
#include <rpc/xdr.h>


/*** Local Include Files ***/

#include "mdattnn_params.h"
#include "orpg_product.h"
#include "packet_28.h"
#define FLOAT
#include "rpgcs_latlon.h"


/*** Static Variable Declarations ***/

static const short DMDPROD        = 149;  /* Product code and LB number       */
static const int   DMDOUTSIZE     = 300000;/* bytes, 100 features, no compress*/
static const short debugit        = FALSE;/* Controls debug in this file only.*/
static const short PRINTPROD      = FALSE;/* Controls RPGP_product_t debug    */
static const short PRINTONEAREA   = FALSE;/* Controls RPGP_area_t debug       */
static const short NUM_PROD_PARAM = 5;    /* This is a fixed value.           */
static const short MAX_3D_PARAM   = 36;   /* May have fewer if some undefined.*/
static const short MAX_2D_PARAM   = 9;    /* Its all or nothing with 2D params*/
static const float NOSCALE        = 1.0;
static const float MSI_SCALE      = 1000.;/* Display scale factor for MSI.    */
static const float ELEV_SCALE     = 0.1;  /* Scale factor for elevation angle */
static const short OVERLAP        = 1;    /* Feature overlaps another.        */
static const short PERCENT        = 100;
static const int   NONE           = 0;
static const int   MIN_DMD_RANK   = 1;
static const int   TENTHS_PRCSN   = 1;    /* precision (digits right of decimal */
static const int   HUNDREDTHS_PRCSN = 2;  /* precision (digits right of decimal */
static const int   TEN_K_PRCSN    = 4;    /* precision (digits right of decimal */
static const short NCHAR1         = 1;    /* one character display field      */
static const short NCHAR2         = 2;    /* two character display field      */
static const short NCHAR3         = 3;    /* three character display field    */
static const short NCHAR4         = 4;    /* four character display field     */
static const short NCHAR5         = 5;    /* five character dsiplay field     */
static const short NCHAR8         = 8;    /* eight character display field    */
static const short NCHAR9         = 9;    /* nine character display field     */


/*** Prototypes ***/

void Set3DArea(RPGP_area_t *area, 
               const cplt_t      *ptrCplt,
               const cplt_t      *ptrOldCplt);
int  Set3DParams(RPGP_parameter_t *params,
                 const cplt_t           *ptrCplt,
                 const cplt_t           *ptrOldCplt);
void Set2DParams(RPGP_parameter_t *params, const int num2D,
                 const th_xs_t *ptrTHXS);
int FloatParamChanged(float param1, float param2, const short precision);
int FloatParamIncreased(float param_new, float param_old, const short precision);
void Set_float_param( RPGP_parameter_t *param, char *id, char *name, char *units,
                      const float  value, const short  fld_width, const short precision );
void Set_float_array( RPGP_parameter_t *param, char *id, char *name, char *units, float *value,
                      const int size, const short fld_width, const short precision, const float scale );
void Set_int_param( RPGP_parameter_t *param, char *id, char *name, char *units, int value );
void Set_int_array( RPGP_parameter_t *param, char *id, char *name, char *units, int *value,
                    const int size, const float scale );
void Set_string_param( RPGP_parameter_t *param, char *id, char *name, char *value, short size  ); 

int buildDMD_PSB
(
   const cplt_t*   const ptrCplt,  /*(IN) pointer to MDATTNN data structure   */
   const cplt_t*   const ptrLast,  /*(IN) pointer to MDATTNN data from last   */
                                   /*      elevation of previous volume.      */
         char**          ptrDMD,   /*(IN/OUT) pointer to DMD output product   */
         RPGP_product_t *prod,     /*(IN/OUT) points to generic product struct*/
   const int             num_cplts,/*(IN) number of couplets this elevation   */
   const int             num_old_cplts,/*(IN) number of couplets last volume  */ 
   const float           avg_u,    /*(IN) average U motion (m/sec)            */
   const float           avg_v,    /*(IN) average V motion (m/sec)            */
   const int             last_el,  /*(IN) set to TRUE if last elevation       */
   const int             vcp_num,  /*(IN) Volume coverage pattern number      */
   const int             elev_ind, /*(IN) Elevation index                     */
   const int             vol_num,  /*(IN) Volume number                       */
   const int             rda_elev, /*(IN) Radar Elevation (feet)              */
         char            rpg_name[8], /*(IN) RPG mnemonic                     */
   const int             elev_time[MESO_NUM_ELEVS],/*(IN) elevation start times (sec)*/
         int            *length    /*(OUT) byte length of DMD product         */
) {

   /*** Variable Declarations and Definitions ***/

   int               i, j;       /* Loop index for each input feature.       */
   int               o;          /* Loop index for each output feature.      */
   int               e;          /* Loop index for each elevation angle.     */
   char             *str;
   char             *serial_data;/* holds serialized generic product data.   */
   char            **comp3D;     /* Array of pointers to 3D components.      */
   Symbology_block   sym;        /* Product Symbology Block structure.       */
   RPGP_parameter_t *params;     /* Holds mean cell motion U and V.          */
   packet_28_t       packet28;   /* ICD defined packet 28 for generic data.  */
   Scan_Summary     *scan_sum;   /* Scan Summary Table                       */
   float            *fdata;
   float             avg_spd;
   float             avg_dir;
   cplt_t           *ptrOldCplt;
   int               numDMDfeat; /* number of features output in the product */
   unsigned int      data_len;   /* Symbology block layer length             */
   unsigned int      block_len;  /* Symbology block length                   */
   int               num_bytes;  /* Temporary storage for packet 28 #bytes   */
   time_t            unix_sec;   /* Seconds from midnight, Jan 1, 1970       */
   int               opstat;     /* status of output buffer reallocation     */
   int               shortage;   /* extra bytes needed over original DMD allocation*/
   int               lvcp_num;   /* Local VCP number (may be -vcp_num if SAILS
                                    not allowed)                             */
   int               lelev_ind;  /* Local elevation index.                   */
   int               allow_suppl_scans; /* Set to 1 if processing SAILS cuts.*/
   
   if(debugit) fprintf(stderr,"\nEntering buildDMD_PSB, num_cplts=%d\n",num_cplts);
   assert(ptrCplt != NULL);
   assert(ptrDMD  != NULL);

   /* Advance the length pointer to after the Product Description Block.     */

   *length += sizeof(Graphic_product);

   /* Loop for all features in the input data array.                   */
   /* Count those we add to the DMD product.  The have to meet a       */
   /* strength rank threshold.                                         */

   numDMDfeat = 0;
   for (i = 0; i < num_cplts; i++)
      if (ptrCplt[i].strength_rank  >= MIN_DMD_RANK) numDMDfeat++;

   if(debugit) fprintf(stderr,"\nNumber of non-zero SR features=%d\n",numDMDfeat);
    
   /* Set up the packet 28 header fields.                                    */

   packet28.code  = PACKET_28;
   packet28.align = 0;
    
   /* Set the generic product header fields.                                 */
      
   str = "Digital Mesocyclone Detection";
   prod->name = (char*)malloc(strlen(str) + 1);
   strcpy(prod->name, str);
       
   str = "Data array product output of Mesocyclone Detection Algorithm";
   prod->description = (char*)malloc(strlen(str) + 1);
   strcpy(prod->description, str);
       
   prod->product_id = DMDPROD;
   prod->type       = RPGP_ELEVATION;
   unix_sec         = time(NULL);
   prod->gen_time   = (unsigned int) RPG_TIME_IN_SECONDS(unix_sec);
    
   prod->radar_name = (char*)malloc(strlen(rpg_name) + 1);
   strcpy(prod->radar_name, rpg_name);
   
   /* Complete the RPGC_product_t header information. */
   
   scan_sum = RPGC_get_scan_summary(vol_num);

   /* Elevation index is based on RDA definition of VCP which may 
      contain SAILS cuts.  For this product we need to account for 
      this by calling RPGCS_remap_rpg_elev_index(). */
   lelev_ind = RPGCS_remap_rpg_elev_index( vcp_num, elev_ind );

   lvcp_num = vcp_num;
   if( (allow_suppl_scans = RPGC_allow_suppl_scans()) == 0 )
      lvcp_num = -vcp_num;

   prod->elevation_angle = RPGCS_get_target_elev_ang(lvcp_num, lelev_ind) / 10.;
   RPGCS_xy_to_latlon((float)0.0, (float)0.0, &prod->radar_lat, &prod->radar_lon);  
   prod->radar_height      = rda_elev * FT_TO_M;
   prod->volume_number     = vol_num;
   prod->operation_mode    = scan_sum->weather_mode;
   prod->vcp               = vcp_num;
   prod->elevation_number  = lelev_ind;
   prod->compress_type     = (short)0;
   prod->size_decompressed = (int)0;
   prod->volume_time       = (unsigned int) RPG_JULIAN_DATE(unix_sec) * SECS_IN_DAY + elev_time[0];
   prod->elevation_time    = (unsigned int) RPG_JULIAN_DATE(unix_sec) * SECS_IN_DAY + elev_time[lelev_ind-1];

   /* Allocate array memory for the product parameters.                      */

   prod->numof_prod_params = NUM_PROD_PARAM;
   params = (RPGP_parameter_t*)malloc(NUM_PROD_PARAM * sizeof(RPGP_parameter_t));
   prod->prod_params = params;

   /* Set the product parameters.  These are used for mean motion and last   */
   /* elevation flag.                                                        */
   
   if (avg_u != UNDEFINED && avg_v != UNDEFINED) {
      RPGCS_xy_to_azran_u(avg_u, avg_v, METERS, &avg_spd, METERS, &avg_dir);
      avg_dir = avg_dir + HALF_CIRC;
      if (avg_dir > CIRC) avg_dir -= CIRC;
      if (avg_spd == 0.) avg_dir = 0.;
   }
   else {
      avg_spd = 0.;
      avg_dir = 0.;
   }
   Set_float_param(params  , "avg_dir","Average Direction of Tracked Features",
                  "deg", avg_dir, 5,1);
   Set_float_param(params+1, "avg_spd","Average Speed of Tracked Features",
                   "m/s", avg_spd, 5,1);
   
   Set_int_param(params+2, "last_elev_flag", "Last Elevation Flag", "", last_el);

   /* Build the target elevation angle array.  Note the elevation index    */
   /* starts its count at 1 so we could decrement it for array sizing.     */
      
   fdata = (float*)malloc((lelev_ind) * sizeof(float));
   for (e = 1; e <= lelev_ind; e++)
     fdata[e-1] = RPGCS_get_target_elev_ang(lvcp_num,e);

   Set_float_array(params+3, "elev_angle", "Elevation Angle", "deg", fdata,
                   lelev_ind, NCHAR4, TENTHS_PRCSN, ELEV_SCALE);
   free(fdata);

   /* ...and now the elevation times array. */
   
   Set_int_array(params+4, "elev_time", "Elevation Time", "s", (int *)elev_time,
      lelev_ind, NOSCALE);
 
   /* Set the number of product components field. Each circulation is */
   /* considered a product component.                                 */
       
   prod->numof_components = numDMDfeat;

   /* See if there are any features to put in the product */
      
   if (numDMDfeat == 0) {
      
      /* Build an empty product  */
      
      comp3D = NULL;
   }
   else {
   
     /* Allocate enough array memory for a pointer to each 3D component.*/
    
     comp3D = malloc(numDMDfeat * sizeof(void*));
   }

   /* Set the product components pointer */
   
   prod->components = (void**)comp3D;

   if (debugit&0) fprintf(stderr,"Entering loop numDMDfeat=%d\n",numDMDfeat);

   /* Loop for each input 3D circulation.                             */
   /* Intialize the output feature counter.                           */
   
   o = NONE;
   
   for (i = 0; i < num_cplts; i++)
   {
      /* Shouldn't really need this but to be safe...                 */
      
      if (comp3D == NULL) continue;
      
      /* Skip those below the minumim strength rank threshold.        */
      
      if ((ptrCplt + i)->strength_rank < MIN_DMD_RANK) continue;
      
      /* We use the RPGP_area_t for each 3D circulation.              */
       
      comp3D[o] = malloc(sizeof(RPGP_area_t));
      
      /* See if this feature was also in the previous volume.         */
      /* Even if it was, if the feature has been topped in this       */
      /* volume or this is the last elevation, the old couplet pointer*/
      /* will be NULL so that all feature parameters are set.         */
      
      ptrOldCplt = NULL;
      
      if (((ptrCplt + i)->detection_status != TOPPED) && (!last_el)) {

         for (j = 0; j < num_old_cplts; j++) {
            if ((ptrCplt + i)->meso_event_id == (ptrLast + j)->meso_event_id)
               ptrOldCplt = (cplt_t*)(ptrLast + j);
         }

      } /* end if not topped and not last elevation */
      
      /* Set the 3D circulation event fields.                         */
       
      if (debugit&0) fprintf(stderr,"Calling Set3DArea, ptrCplt+i = %p, ptrOldCplt = %p\n",(ptrCplt+i),ptrOldCplt);
      Set3DArea((RPGP_area_t*)comp3D[o], (ptrCplt + i), ptrOldCplt);
      
      if(PRINTONEAREA && (ptrCplt + i)->meso_event_id == 1)
          RPGP_print_area((RPGP_area_t *)comp3D[o]);

      /* Increment the output feature counter.                        */
      
      o++;
   } /* end for all input features */
   
   if (PRINTPROD) {
     fprintf(stderr,"About to printprod. %p..\n", prod);
     RPGP_print_prod(prod);
     fprintf(stderr,"done printprod\n");
   }
   
   /* We need to serialize the data for transmission.                 */
   /* NOTE: This may be an infrastructure function in the future.     */

   if (debugit&0) fprintf(stderr, "About to serialize\n");
   serial_data = NULL;
   packet28.num_bytes = RPGP_product_serialize(prod, &serial_data);

   if (debugit) fprintf(stderr,"Back from serialization, num_bytes=%d\n",packet28.num_bytes);

   if (serial_data == NULL) {
      RPGC_log_msg(GL_ERROR, "mdaprod: Error serializing DMD product!\n");
      return -1;
   }

   /* Make the output buffer is large enough.  If not, reallocate */
   
   if ((packet28.num_bytes + (*length)) > DMDOUTSIZE) {
      /* Compute the byte shortage.  Add some for pre-ICD header (?) */
      shortage = (packet28.num_bytes + (*length)) - DMDOUTSIZE + 100;
      *ptrDMD= (char*)RPGC_realloc_outbuf((void*)(*ptrDMD),DMDOUTSIZE+shortage,&opstat);

      if(opstat != NORMAL)
      { 
         RPGC_log_msg(GL_ERROR,
          "mdaprod: Error reallocating DMD output buffer, opstat=%d\n", opstat);
         if(opstat == NO_MEM)
            RPGC_abort_because(PROD_MEM_SHED);
         else
            RPGC_abort();
         free(serial_data);
         return -1;
      }    
   } 

   /* Build the first fields of the symbology block but don't copy    */
   /* it to the output buffer yet.  We don't know the length fields.  */
 
   sym.divider       = (short)-1; /* block divider always -1          */
   sym.block_id      = (short) 1; /* always 1 for symbology block     */
   sym.n_layers      = (short) 1; /* we put all features in one layer */
   sym.layer_divider = (short)-1; /* layer divider always -1          */

   /* Compute the length of layer field for the data layer.           */

   data_len = sizeof(packet_28_t) + packet28.num_bytes;
   RPGC_set_product_int( (void *) &sym.data_len, data_len );

   /* Compute the length of block field for the symbology block.      */

   block_len =   sizeof(Symbology_block) + data_len;
   RPGC_set_product_int( (void *) &sym.block_len, block_len );

   /* Copy the completed symbology block header */

   memcpy((*ptrDMD) + (*length), &sym, sizeof(Symbology_block));
   *length += sizeof(Symbology_block);
   
 /* Set the number of bytes in packet 28 */

   num_bytes = packet28.num_bytes;
   RPGC_set_product_int( (void *) &packet28.num_bytes, num_bytes );

   /* Copy the packet into the output product.                        */
   
   memcpy((*ptrDMD) + (*length), &packet28, sizeof(packet_28_t));
   *length += sizeof(packet_28_t);
   
   /* Copy the serialized data.                                       */
   
   memcpy((*ptrDMD) + (*length), serial_data, num_bytes);
   *length += num_bytes;
   
   free(serial_data);
   
   if(debugit) fprintf(stderr,"\nExiting buildDMD_PSB\n");
   /* Return the halfword offset to the start of this data .          */
   
   return ((int)(sizeof(Graphic_product) / 2));
} /* End of buildDMD_PSB() */



void Set3DArea
(
   RPGP_area_t *area,        /*(IN/OUT) A 3D feature in output format      */
   const cplt_t *ptrCplt,    /*(IN) Input data from mdattnn task           */
   const cplt_t *ptrOldCplt  /*(IN) Saved data from previous volume or NULL*/
)
{
   int               num3Dparam;/* Number of 3D parameters placed in buffer   */
   int               num2Dparam;/* Number of 2D parameters placed in buffer   */
   RPGP_parameter_t *params;    /* Points to start of all 3D and 2D parameters*/
   RPGP_location_t  *loc;       /* Points to location in RPGC_location_t form */

   if(debugit&0) fprintf(stderr,"Entering Set3DArea\n");
      
   /* This is an area point type component.                           */
      
   area->comp_type = RPGP_AREA_COMP;
   area->area_type = RPGP_AT_POINT;
   
   /* Set the lat/lon location fields for this feature.               */
   
   area->numof_points = (int)1;
   loc = (RPGP_location_t*)malloc(sizeof(RPGP_location_t));
   area->points       = loc;

   RPGCS_azran_to_latlon(ptrCplt->ll_rng, ptrCplt->ll_azm, &loc->lat, &loc->lon);  
    
   if(debugit&0) fprintf(stderr,"a%f r%f Feature lat: %f, lon: %f\n", 
                 ptrCplt->ll_azm, ptrCplt->ll_rng, loc->lat, loc->lon);

   /* Allocate enough memory for each 3D and 2D parameter.                */
       
   params = (RPGP_parameter_t*)malloc((MAX_3D_PARAM + MAX_2D_PARAM) *
             sizeof(RPGP_parameter_t));
      
   /* Set the 3D circulation parameters.  The function returns the actual */
   /* number of 3D parameters included.                                   */
       
   num3Dparam = Set3DParams(params, ptrCplt, ptrOldCplt);
   if(debugit&0) fprintf(stderr,"num3Dparam=%d\n", num3Dparam);  

   /* Don't add 2D parameters for extrapolated features.  They are only   */
   /* copies of the data from the previous volume.                        */
   /* NOTE:  Make sure this logic matches that for the num2D parameter in */
   /* the function Set3DParams()!                                         */

   if (ptrOldCplt == NULL || ptrCplt->detection_status != EXTRAPOLATED) {
   
      /* Format the 2D parameter data into arrays for each data type.     */
       
      Set2DParams(params+num3Dparam, ptrCplt->num2D, ptrCplt->mda_th_xs);
      
      /* Always the same number of 2D parameters.                         */
      
      num2Dparam = MAX_2D_PARAM;
   
   } else {
   
      num2Dparam = 0;
      
   } /* end if extrapolated     */
   
   /* Reallocate if some parameters were undefined or did not get updated */
   /* and were therefore omitted.                                         */
   
   if ((num3Dparam != MAX_3D_PARAM) || (num2Dparam != MAX_2D_PARAM))
      params = (RPGP_parameter_t*)realloc(params, sizeof(RPGP_parameter_t) * 
               (num3Dparam + num2Dparam));
   
   /* Set the area's pointer to its component parameters.                 */
   
   area->comp_params = params;

   /* Set the number of parameters field.                                 */
       
   area->numof_comp_params = num3Dparam + num2Dparam;
       
   if(debugit&0) fprintf(stderr,"Exiting Set3DArea\n");
   return;
} /*End of Set3DArea() */


int Set3DParams
(
   RPGP_parameter_t *params,    /*(IN/OUT) Start of 3D parameter memory  */
   const cplt_t     *ptrCplt,   /*(IN) Input data from mdattnn task      */
   const cplt_t     *ptrOldCplt /*(IN) Same feature from last volume or NULL */
) {

   float   lat[MAX_PAST]; /* Work arrays for past and forecast positions */
   float   lon[MAX_PAST];
   int     i;             /* Parameter index */
   int     p;             /* Position index  */
   int     all;           /* Set when all parameters must be in the product */
                          /*   This is the case for topped features or when */
                          /*   we've reached the end of volume.             */

   if(debugit&0) fprintf(stderr,"Entering Set3DParams\n");
   
   if (ptrOldCplt == NULL)
      all = YES;
   else
      all = NO;

   /* Set the 3D circulation parameters. NOTE: Adding or removing any         */
   /* parameters in this function will require a change to MAX_3D_PARAM!!     */
   /* Also note that the Meso ID, detection status, Base Azimuth and Base     */
   /* Range parameters are always put in the product but other parameters are */
   /* only set when their value is different than that from the last          */
   /* elevation of the previous volume scan.                                  */
   
   i = 0;
   
   /* Always set the Meso ID parameter.                                   */
   
   Set_int_param(params+i++, "meso_event_id", "Meso ID", "", ptrCplt->meso_event_id);
   
   if (all || (strncmp(ptrCplt->storm_id, ptrOldCplt->storm_id, NCHAR2) != 0))
      Set_string_param(params+i++, "storm_id", "Associated Storm ID",
         (char *) ptrCplt->storm_id, NCHAR2);
   
   if (all || (ptrCplt->age != ptrOldCplt->age))
      Set_int_param(params+i++, "age", "Age", "s", (int)floor((ptrCplt->age * SECPERMIN) + 0.5));

   /* Set the strength and strength rank type if either has changed.      */
   
   if (all || ((ptrCplt->circ_class    != ptrOldCplt->circ_class)    ||
               (ptrCplt->strength_rank != ptrOldCplt->strength_rank))) {
       
      Set_int_param(params+i++, "strength_rank", "Strength Rank", "", ptrCplt->strength_rank);
      if (ptrCplt->circ_class == SHALLOW_CIRC) {
         Set_string_param(params+i++, "strength_rank_type", "Strength Rank Type", "S",
                        NCHAR1);
      } else {
         if (ptrCplt->circ_class == LOW_TOPPED_MESO)
            Set_string_param(params+i++, "strength_rank_type", "Strength Rank Type", "L", NCHAR1);
         else
            Set_string_param(params+i++, "strength_rank_type", "Strength Rank Type", " ", NCHAR1);
      } /* end if */
   } /*end if circ_class */

   /* Always set detection status parameter.                              */

   if (ptrCplt->detection_status == EXTRAPOLATED) {
      Set_string_param(params+i++, "detection_status", "Detection Status", "EXT", NCHAR3);
   } else {
      if (ptrCplt->detection_status == TOPPED)
         Set_string_param(params+i++, "detection_status", "Detection Status", "TOP", NCHAR3);
      else
         Set_string_param(params+i++, "detection_status", "Detection Status", "UPD",
                               NCHAR3);
   } /* end if */

   /* The addition of the num2D parameter MUST match the logic used       */
   /* in function Set3DArea for the 2D data arrays.                       */
   
   if (all || ptrCplt->detection_status != EXTRAPOLATED)
      Set_int_param(params+i++, "num2D", "Number of 2D Features in this 3D Feature",
                    "", ptrCplt->num2D);

   /* Some special handling for tvs status.  If the tvs status of this     */
   /* volume's feature is unknown, use the value from the old feature.     */
   /* The TDA process does not produce output until the third elevation so */
   /* features from this volume will have unknown statuses until the 3rd   */
   /* elevation.                                                           */
   
   if (all) {

      /* Feature is either topped or this is the last elevation.           */

      if (ptrCplt->tvs_near == TVS_NO) {
         Set_string_param(params+i++, "tvs_near", "Associated TVS", "N", NCHAR1);
      } else {
         if (ptrCplt->tvs_near == TVS_YES)
            Set_string_param(params+i++, "tvs_near", "Associated TVS", "Y", NCHAR1);
         else
            Set_string_param(params+i++, "tvs_near", "Associated TVS", "U", NCHAR1);
      } /* endif */
   } else {
   
      /* If the current status is known AND there has been a change since the */
      /* last volume, add the current status.                                 */
       
      if (ptrCplt->tvs_near != TVS_UNKNOWN && ptrCplt->tvs_near != ptrOldCplt->tvs_near) {

         if (ptrCplt->tvs_near == TVS_NO) {
            Set_string_param(params+i++, "tvs_near", "Associated TVS", "N", NCHAR1);
         } else {
            if (ptrCplt->tvs_near == TVS_YES)
               Set_string_param(params+i++, "tvs_near", "Associated TVS", "Y", NCHAR1);
            else
               Set_string_param(params+i++, "tvs_near", "Associated TVS", "U", NCHAR1);
         } /* end if */       
      } /* endif tvs status is known and it has changed since last volume */
   } /* end tvs_near processing */

   /* Treat speed and direction together   */
   
   if (all || (FloatParamChanged(ptrCplt->prop_spd, ptrOldCplt->prop_spd, TENTHS_PRCSN)) || 
              (FloatParamChanged(ptrCplt->prop_dir, ptrOldCplt->prop_dir, TENTHS_PRCSN))) {
      if (ptrCplt->prop_spd != UNDEFINED)
         Set_float_param( params+i++, "prop_spd", "Speed", "m/s", ptrCplt->prop_spd,
                          NCHAR5,TENTHS_PRCSN);

      if (ptrCplt->prop_dir != UNDEFINED)
         Set_float_param(params+i++, "prop_dir", "Direction", "deg", ptrCplt->prop_dir,
                         NCHAR5,TENTHS_PRCSN);
   } /* end if prop_spd OR prop_dir */

   /*  Always set base azimuth and range   */
   
   Set_float_param(params+i++, "ll_azm", "Base Azimuth", "deg", ptrCplt->ll_azm,
                   NCHAR5,TENTHS_PRCSN);
   Set_float_param(params+i++, "ll_rng", "Base Range",   "km",  ptrCplt->ll_rng,
                   NCHAR5,TENTHS_PRCSN);
   
   if (all || (FloatParamChanged(ptrCplt->base, ptrOldCplt->base, TENTHS_PRCSN))) {
      if (ptrCplt->base < 0)
         Set_string_param(params+i++, "lowest_elev", "Base on Lowest Elevation", "Y", NCHAR1);
      else 
         Set_string_param(params+i++, "lowest_elev", "Base on Lowest Elevation", "N", NCHAR1);

      Set_float_param(params+i++, "base", "Base Height", "km", fabs(ptrCplt->base),
                      NCHAR4,TENTHS_PRCSN);
   } /* end if base */
   
   /* Have to check for downgrading of depth here.  NSSL implementation didn't check it. */
      
   if (all || FloatParamIncreased(ptrCplt->depth, ptrOldCplt->depth, TENTHS_PRCSN))
      Set_float_param(params+i++, "depth","Depth", "km", ptrCplt->depth,
                      NCHAR4,TENTHS_PRCSN);
      
   /* Put in the past and forecast positions for all non-extrapolated features*/
   
   if (all || (ptrCplt->detection_status != EXTRAPOLATED)) {
      Set_int_param(params+i++, "num_past_pos", "Number of Past Positions",
                    "", ptrCplt->num_past_pos);

      /* Convert past positions from XY meters from radar to latitude/longitude arrays */
   
      for (p = 0; p < ptrCplt->num_past_pos; p++){
         RPGCS_xy_to_latlon((ptrCplt->past_x[p]*M_TO_KM), 
                            (ptrCplt->past_y[p]*M_TO_KM),
                             &lat[p], &lon[p]);
      }
      
      if (ptrCplt->num_past_pos != 0) {

         /* Put the lat/lon arrays in the product */
      
         Set_float_array(params+i++, "past_lat", "Past Latitude", "deg", lat,
                         ptrCplt->num_past_pos, NCHAR8,TEN_K_PRCSN,NOSCALE);
         Set_float_array(params+i++, "past_lon", "Past Longitude", "deg", lon,
                         ptrCplt->num_past_pos, NCHAR9,TEN_K_PRCSN,NOSCALE);
      } /* end if num_past_pos */
   
      Set_int_param(params+i++, "num_fcst_pos", "Number of Forecast Positions",
                    "",ptrCplt->num_fcst_pos);

      /* Convert forecast positions from XY meters from radar to    */
      /* latitude/longitude arrays.                                 */
   
      for (p = 0; p < ptrCplt->num_fcst_pos; p++){
         RPGCS_xy_to_latlon((ptrCplt->fcst_x[p]*M_TO_KM),
                            (ptrCplt->fcst_y[p]*M_TO_KM),
                             &lat[p], &lon[p]);
      }

      if  (ptrCplt->num_fcst_pos != 0) {
   
         /* Put the lat/lon arrays in the product */
   
         Set_float_array(params+i++, "fcst_lat", "Forecast Latitude", "deg", lat,
                         ptrCplt->num_fcst_pos, NCHAR8,TEN_K_PRCSN,NOSCALE);
         Set_float_array(params+i++, "fcst_lon", "Forecast Longitude", "deg", lon,
                         ptrCplt->num_fcst_pos, NCHAR9,TEN_K_PRCSN,NOSCALE);
      } /*end if num_fcst_pos */
   }/* end if not extrapolated */

   if (all || (FloatParamChanged(ptrCplt->ll_diam, ptrOldCplt->ll_diam, TENTHS_PRCSN)))
      Set_float_param(params+i++, "ll_diam", "Base Diameter", "km",
                      ptrCplt->ll_diam, NCHAR4,TENTHS_PRCSN);
      
   if (all || (FloatParamChanged(ptrCplt->ll_rot_vel, ptrOldCplt->ll_rot_vel, TENTHS_PRCSN)))
      Set_float_param(params+i++, "ll_rot_vel", "Base Rotational Velocity", "m/s",
                      ptrCplt->ll_rot_vel, NCHAR5,TENTHS_PRCSN);
   
   if (all || ((FloatParamIncreased(ptrCplt->max_rot_vel, ptrOldCplt->max_rot_vel, TENTHS_PRCSN)) ||
               (FloatParamChanged(ptrCplt->height_max_rot_vel, ptrOldCplt->height_max_rot_vel, TENTHS_PRCSN)))) {
      Set_float_param(params+i++, "max_rot_vel", "Max Rotational Velocity", "m/s",
                      ptrCplt->max_rot_vel, NCHAR5,TENTHS_PRCSN);
      Set_float_param(params+i++, "height_max_rot_vel", "Height of Max Rotational Velocity", "km",
                      ptrCplt->height_max_rot_vel, NCHAR4,TENTHS_PRCSN);
   } /* end if max_rot_vel OR height_max_rot_vel */
     
   if (all || (FloatParamChanged(ptrCplt->ll_shear, ptrOldCplt->ll_shear, TENTHS_PRCSN)))
      Set_float_param(params+i++, "ll_shear", "Base Shear", "m/s/km",
                      ptrCplt->ll_shear, NCHAR4,TENTHS_PRCSN);
      
   if (all || ((FloatParamIncreased(ptrCplt->max_shear, ptrOldCplt->max_shear, TENTHS_PRCSN))  ||
               (FloatParamChanged(ptrCplt->height_max_shear, ptrOldCplt->height_max_shear, TENTHS_PRCSN)))) {
      Set_float_param(params+i++, "max_shear", "Max Shear", "m/s/km",
                      ptrCplt->max_shear, NCHAR4,TENTHS_PRCSN);
      Set_float_param(params+i++, "height_max_shear", "Height of Max Shear", "km",
                      ptrCplt->height_max_shear, NCHAR4,TENTHS_PRCSN);
   } /* end if max_shear OR height_max_shear */
   
   if (all || (FloatParamChanged(ptrCplt->ll_gtg_vel_dif, ptrOldCplt->ll_gtg_vel_dif, TENTHS_PRCSN)))
      Set_float_param(params+i++, "ll_gtg_vel_dif", "Base Gate-to-Gate Velocity Difference", "m/s",
                      ptrCplt->ll_gtg_vel_dif, NCHAR5,TENTHS_PRCSN);

   if (all || (ptrCplt->display_filter != ptrOldCplt->display_filter)) {
      if (ptrCplt->display_filter == OVERLAP)
         Set_string_param(params+i++, "display_filter", "Overlaps Lower Feature", "N", NCHAR1);
      else
         Set_string_param(params+i++, "display_filter", "Overlaps Lower Feature", "Y", NCHAR1);
   } /* end if display_filter */

   if (all || (FloatParamIncreased (ptrCplt->msi, ptrOldCplt->msi, TEN_K_PRCSN))) 
      Set_int_param(params+i++, "msi", "MSI", "", (int)floor((ptrCplt->msi*MSI_SCALE) + 0.5));
   
   /* Do not add the storm-relative depth parameter until the feature is topped or it increases.*/
   
   if (all || FloatParamIncreased(ptrCplt->storm_rel_depth, ptrOldCplt->storm_rel_depth, HUNDREDTHS_PRCSN)) {
      if (ptrCplt->storm_rel_depth > 0.)
         Set_int_param(params+i++, "storm_rel_depth", "Storm Relative Depth", "%", 
                       floor(ptrCplt->storm_rel_depth * PERCENT + 0.5));
   } /* end if storm_rel_depth */
   
   if (all || (FloatParamIncreased(ptrCplt->ll_convergence, ptrOldCplt->ll_convergence, TENTHS_PRCSN))) {
      if (ptrCplt->ll_convergence != (float)UNDEFINED_SHORT)
         Set_float_param(params+i++, "ll_convergence", "0-2 km ARL Convergence", "m/s",
                         ptrCplt->ll_convergence, NCHAR5,TENTHS_PRCSN);
   } /* end if ll_convergence */

   if (all || (FloatParamIncreased(ptrCplt->ml_convergence, ptrOldCplt->ml_convergence, TENTHS_PRCSN))) {
      if (ptrCplt->ml_convergence != (float)UNDEFINED_SHORT)
         Set_float_param(params+i++, "ml_convergence", "2-4 km ARL Convergence", "m/s",
                         ptrCplt->ml_convergence, NCHAR5,TENTHS_PRCSN);
   } /* end if ml_convergence */

   if(debugit&0) fprintf(stderr,"Exiting Set3DParams\n");
   return i;
} /* End of Set3DParams() */

       
void Set2DParams
(
   RPGP_parameter_t *params, /* (IN/OUT) Start of 2D parameter memory   */
   const int         num2D,  /* (IN) Number of 2D features in this 3D   */
   const th_xs_t    *ptrTHXS /* (IN) Time/Height data from mdattnn task */
) {

   int      i, e;
   int     *idata;
   float   *fdata, *fdata2;

   if(debugit&0) fprintf(stderr,"Entering Set2DParams, num_2D=%d\n",num2D);
   
   /* Allocate enough memory for an array of each data type.              */
   
   idata =   (int*)malloc(num2D * sizeof(int));
   fdata = (float*)malloc(num2D * sizeof(float));
   fdata2 = (float*)malloc(num2D * sizeof(float));
   
   /* Loop though each 2D data type and put the data into array form.     */
   /* Then, place it in the provided parameter as an array.  NOTE: Adding */
   /* or removing any parameters in this function will require a change   */
   /* to MAX_2D_PARAM!! IF YOU FORGET, task will FAIL at run time!!!      */

   i = 0;
   
   for (e = 0; e < num2D; e++)
      idata[e] = ptrTHXS[e].tilt_num; /* Decrement to make zero-relative */
   Set_int_array(params+i++, "tilt_num", "Elevation Index", "", idata, num2D, NOSCALE);

   for (e = 0; e < num2D; e++)
      fdata[e] = (float)ptrTHXS[e].height; /* height was received in meters */
   Set_float_array(params+i++, "height", "2D Height", "km", fdata, num2D,
                   NCHAR4, TENTHS_PRCSN, M_TO_KM);

   for (e = 0; e < num2D; e++)
      idata[e] = ptrTHXS[e].diam;
   Set_int_array(params+i++, "diam", "2D Diameter", "km", idata, num2D, NOSCALE);

   for (e = 0; e < num2D; e++)
      idata[e] = ptrTHXS[e].rot_vel;
   Set_int_array(params+i++, "rot_vel", "2D Rotational Velocity", "m/s", idata, num2D,
                 NOSCALE);

   for (e = 0; e < num2D; e++)
      idata[e] = ptrTHXS[e].shear;
   Set_int_array(params+i++, "shear", "2D Shear", "m/s/km", idata, num2D,
                 NOSCALE);

   for (e = 0; e < num2D; e++)
      idata[e] = ptrTHXS[e].gtgmax;
   Set_int_array(params+i++, "gtgmax", "2D Gate-to-Gate Velocity Difference", "m/s", idata, num2D,
                 NOSCALE);

   for (e = 0; e < num2D; e++)
      idata[e] = ptrTHXS[e].rank;
   Set_int_array(params+i++, "rank", "2D Strength Rank", "", idata, num2D,
                 NOSCALE);
               
   for (e = 0; e < num2D; e++)
      RPGCS_azran_to_latlon(ptrTHXS[e].cr, ptrTHXS[e].ca, &fdata[e], &fdata2[e]);
   Set_float_array(params+i++, "2d_lat", "2D Latitude", "deg", fdata, num2D,
                   NCHAR8, TEN_K_PRCSN, NOSCALE);
   Set_float_array(params+i++, "2d_lon", "2D Longitude", "deg", fdata2, num2D,
                   NCHAR9, TEN_K_PRCSN, NOSCALE);
                 
   free(idata);
   free(fdata);
   free(fdata2);
      
   if(debugit&0) fprintf(stderr,"Exiting Set2DParams\n");
   return;
} /* End of Set2DParams() */


int FloatParamChanged
(
         float param1,   /* (IN) First parameter to be compared     */
         float param2,   /* (IN) Second parameter to be compared    */
   const short precision /* (IN) Max digits right of decimal point. */
) {

         float  rounded_param1;
         float  rounded_param2;
   const double TEN    = 10.;
   const short  ZERO_S = 0;
         double pfactor;
   
   if (precision < ZERO_S) return (int)YES;

   /* Compute precision factor                      */
   
   pfactor = pow(TEN,(double)precision);
   
   /* Set the rounded values for each parameter.    */
   
   rounded_param1 = floor(param1 * pfactor + 0.5) / pfactor;
   rounded_param2 = floor(param2 * pfactor + 0.5) / pfactor;

   if (rounded_param1 != rounded_param2)
     return (int)YES; /* Parameter has changed */
   else
     return (int)NO;  /* Parameter has not changed */
     
} /* End of FloatParamChanged() */


int FloatParamIncreased
(
         float param_new,   /* (IN) Current volume parameter to be compared. */
         float param_old,   /* (IN) Previous volume parameter to be compared.*/
   const short precision    /* (IN) Max digits right of decimal point.       */
) {

         float rounded_param_new;
         float rounded_param_old;
         double pfactor;
   const double TEN    = 10.;
   const short  ZERO_S = 0;
   
   if (precision < ZERO_S) return (int)YES;

   /* Compute precision factor                      */
   
   pfactor = pow(TEN,(double)precision);
   
   /* Set the rounded values for each parameter.    */
   
   rounded_param_new = floor(param_new * pfactor + 0.5) / pfactor;
   rounded_param_old = floor(param_old * pfactor + 0.5) / pfactor;
   
   if (rounded_param_new > rounded_param_old)
     return (int)YES; /* Parameter has increased */
   else
     return (int)NO;  /* Parameter has not increased */
     
} /* End of FloatParamIncreased() */


/*\////////////////////////////////////////////////////////////////////////////////////

   Description:
      Wrapper function for RPGP_set_float_param

////////////////////////////////////////////////////////////////////////////////////\*/
void Set_float_param( RPGP_parameter_t *param,    /* (IN/OUT) Parameter to be set    */
                      char             *id,       /* (IN) Single token parameter ID  */
                      char             *name,     /* (IN) Parameter description      */
                      char             *units,    /* (IN) Units for parameter        */
                      const float       value,    /* (IN) Value of this float        */
                      const short       fld_width,/* (IN) Max number of characters 
                                                     in float field. (0 = no limit)  */
                      const short       precision /* (IN) Max digits right of 
                                                     decimal point.                  */) {

   RPGP_set_float_param( param, id, name, RPGP_TYPE_FLOAT, (void *) &value, 1, 
                         fld_width, precision, 1.0, RPGP_ATTR_UNITS, units, 0 );

}

/*\////////////////////////////////////////////////////////////////////////////////////

   Description:
      Wrapper function for RPGP_set_float_param

////////////////////////////////////////////////////////////////////////////////////\*/
void Set_float_array( RPGP_parameter_t *param,    /* (IN/OUT) Parameter to be set    */
                      char             *id,       /* (IN) Single token parameter ID  */
                      char             *name,     /* (IN) Parameter description      */
                      char             *units,    /* (IN) Units for parameter        */
                      float            *value,    /* (IN) Values for the array       */
                      const int         size,     /* (IN) Number of array elements   */
                      const short       fld_width,/* (IN) Max number characters in
                                                     float. (0 = no limit)      */
                      const short       precision,/* (IN) Max digits right of 
                                                     decimal point              */
                      const float       scale     /* (IN) Scale applied to each 
                                                     array value                */) {


   RPGP_set_float_param( param, id, name, RPGP_TYPE_FLOAT, value, size, fld_width,
                         precision, (float) scale, RPGP_ATTR_UNITS, units, 0 );

}

/*\////////////////////////////////////////////////////////////////////////////////////

   Description:
      Wrapper function for RPGP_set_int_param

////////////////////////////////////////////////////////////////////////////////////\*/
void Set_int_param( RPGP_parameter_t *param, /* (IN/OUT) Parameter to be set    */
                    char             *id,    /* (IN) Single token parameter ID  */
                    char             *name,  /* (IN) Parameter description      */
                    char             *units, /* (IN) Units for parameter        */
                    const int         value  /* (IN) Value of this integer      */) {


   RPGP_set_int_param( param, id, name, RPGP_TYPE_INT, (void *) &value, 1, 1,
                       RPGP_ATTR_UNITS, units, 0 );

}

/*\////////////////////////////////////////////////////////////////////////////////////

   Description:
      Wrapper function for RPGP_set_int_param

////////////////////////////////////////////////////////////////////////////////////\*/
void Set_int_array( RPGP_parameter_t *param, /* (IN/OUT) Parameter to be set    */
                    char             *id,    /* (IN) Single token parameter ID  */
                    char             *name,  /* (IN) Parameter description      */
                    char             *units, /* (IN) Units for parameter        */
                    int              *value, /* (IN) Values for the array       */
                    const int         size,  /* (IN) Number of array elements   */
                    const float       scale  /* (IN) Scale applied to each array
                                                value                      */){

   RPGP_set_int_param( param, id, name, RPGP_TYPE_INT, value, size, scale, 
                       RPGP_ATTR_UNITS, units, 0 );

}

/*\////////////////////////////////////////////////////////////////////////////////////

   Description:
      Wrapper function for RPGP_set_string_param

////////////////////////////////////////////////////////////////////////////////////\*/
void Set_string_param( RPGP_parameter_t *param, /* (IN/OUT) Parameter to be set    */
                       char             *id,    /* (IN) Single token parameter ID  */
                       char             *name,  /* (IN) Parameter description      */
                       char             *value, /* (IN) Value for this string      */
                       short             size   /* (IN) Number of characters       */) {


   RPGP_set_string_param( param, id, name, value, 1, 0 );
}
