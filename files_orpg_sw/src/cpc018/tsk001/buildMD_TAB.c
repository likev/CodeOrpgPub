/*******************************************************************************
    Filename:   buildMD_TAB.c
    Author:     Brian Klein


    Description
    ===========
    This function will format the final product tabular alphanumeric block
    based on the input data array structure.  It returns the halfword offset
    to this block.  See template at the bottom of this file.
  

   The following was used as a template for the MDA TAB:

         1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
                                                                                
                        MESOCYCLONE DETECTION ALGORITHM                         
     RADAR ID: nnn     DATE: mm/dd/yy   TIME: hh:mm:ss    Avg dir/spd: NNN/NNN  
                                                                                

 CIRC  AZRAN   SR STM |-LOW LEVEL-|  |--DEPTH--|  |-MAX RV-| TVS  MOTION   MSI  
  ID   deg/nm     ID  RV   DV  BASE  kft STMREL%  kft    kts     deg/kts        
                                                                                
 NNN  NNN/NNN NNC CC  NNN NNN   <NN  >NN  NNN     NNN    NNN  C  NNN/NNN NNNNN  

*******************************************************************************/

/* 
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/03 22:21:55 $
 * $Id: buildMD_TAB.c,v 1.6 2005/03/03 22:21:55 ryans Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/*** Global Include Files ***/

#include <math.h>
#include <rpgc.h>
#include <assert.h>

#include <mda_adapt.h>
#define EXTERN extern
#include <mda_adapt_object.h>

/*#include <a318buf.h>*/
/*#include <siteadp.h>*/


/*** Local Include Files ***/
#define FLOAT
#include "rpgcs_coordinates.h"
#include "rpgcs_time_funcs.h"
#include "mdattnn_params.h"

/*** Static Variable Declarations ***/

const static short  debugit          = 0;
const static short  LINES_PER_PAGE = 16;   /* max number of lines per page  */
const static short  END_OF_PAGE    = -1;   /* End of page flag              */
const static short  LINE_WIDTH     = 80;   /* Fixed line width for TAB      */
const static float  MSI_SCALE      = 1000.;/* Display scale factor for MSI  */
const static short  LOW_HT_THRESH  = 1;    /* km threshold for < sign       */
const static float  PERCENT        = 100.;
const static float  SEC_PER_HOUR   = 3600.;


typedef struct {
   short divider;    /* block divider (always -1)                           */
   short num_pages;  /* total number of pages in the TAB product            */
   } Alpha_hdr_t;

typedef struct {
   short num_char;  /* number of characters per line                        */
   char data[80];   /* container for one line of TAB output                 */
   } Alpha_data_t;



/*** Function Prototypes ***/

static short writePageHeader(      char*   ptrMD,
                             const int     vol_num,
                             const int     rpg_id,
                             const short   n_pages,
                             const short   avg_spd,
                             const short   avg_dir,
                                   int*    length);

static short writeFeatureLine(const cplt_t  *cplt,
                                    char*   ptrMD,
                                    int*    length);


int buildMD_TAB
(
   const cplt_t* const ptrCplt, /*(IN) pointer to input couplet array        */
         char*   const ptrMD,  /*(OUT) pointer to final product             */
   const int     vol_num, /*(IN) volume scan number                          */
   const int     rpg_id,  /*(IN) rpg id from adaptation data                 */
   const int     num_cplts,/*(IN) number of input couplets                   */
   const float   avg_u,   /*(IN) average u motion of all features            */
   const float   avg_v,   /*(IN) average v motion of all features            */
         int*    length   /*(IN/OUT) byte length before and after TAB        */
)
{
    /*** Variable Declarations and Definitions ***/

    int    f;                  /* loop index for features                    */
    short  n_pages;            /* counter for number of alpha pages          */
    short  n_lines;            /* counter for number of alpha lines          */
    short  n_3Dfeat;           /* counter for number of displayable features */
    int    tab_start;
    int    alpha_start;
    int    result;
    int    tab_len;            /* bytes                                      */
    float  avg_spd_f, avg_dir_f;
    short  avg_spd,   avg_dir;
    
    Alpha_hdr_t         alpha_hdr;
    Tabular_alpha_block tab_hdr;
    Graphic_product     gp;
    Graphic_product*    gpptr = &gp;
    
    assert(ptrCplt != NULL);
    assert(ptrMD  != NULL);

    /* Determine if we need to do anything.                           */

    n_3Dfeat = 0;
    
    for (f = 0; f < num_cplts; f++)
       if (ptrCplt[f].display_filter &&
           ptrCplt[f].strength_rank  >= mda_adapt.min_filter_rank) n_3Dfeat++;

    if (n_3Dfeat <= 0)
       return ((int)0);

    /* Initialize page counter.                                       */

    n_pages = 0;

    /* Initialize the line counter to trigger writing the page header */

    n_lines = LINES_PER_PAGE;

    /* Save the length so we can determine how big this block gets.   */

    tab_start = *length;

    /* Make room for the TAB's headers                                */

    *length += sizeof(Tabular_alpha_block) + sizeof(Graphic_product);

    /* Save this location for the alpha block header.                 */

    alpha_start = *length;

    /*  Account for the alpha block header.                           */

    *length += sizeof(Alpha_hdr_t);
    
    /* Convert the average U and V motion to display format           */
    
    if (avg_u != UNDEFINED && avg_v != UNDEFINED) {
       RPGCS_xy_to_azran_u(avg_u, avg_v, METERS, &avg_spd_f, NMILES, &avg_dir_f);
       
       avg_spd = (short)floor(avg_spd_f * SEC_PER_HOUR + 0.5);
       avg_dir = (short)floor(avg_dir_f + HALF_CIRC + 0.5);
       if (avg_dir > CIRC) avg_dir -= CIRC;
    }   
    else {
       avg_spd = 0;
       avg_dir = 0;
    }
    if (debugit) fprintf(stderr,"avg_spd=%d, avg_dir=%d\n",avg_spd,avg_dir);

    /* Loop for each couplet, checking for displayable features       */

    for (f = 0; f < num_cplts; f++)
    {
       if (ptrCplt[f].display_filter &&
           ptrCplt[f].strength_rank  >= mda_adapt.min_filter_rank)
       {
          /* This feature is displayable.  First see if we         */
          /* need to write a page header.                          */

          if (n_lines == LINES_PER_PAGE)
          {
             /* Write a page header and adjust the counters.       */
             /* The length field will be updated.                  */

             n_lines  = writePageHeader(ptrMD, vol_num, rpg_id, n_pages,
                                        avg_spd, avg_dir, length);
             n_pages++;
          }
             
          /* Write out the feature information.                    */

          n_lines += writeFeatureLine(ptrCplt + f, ptrMD, length);

       } /* end if displayable feature                             */

    } /* end for all couplets */

    /* End this page.                                               */

    memcpy(ptrMD + (*length), &END_OF_PAGE, sizeof(short));
    *length += sizeof(short);

    /* Set up a tabular alphanumeric block header.                   */

    tab_hdr.divider   = (short)-1;   /* Block divider always -1      */
    tab_hdr.block_id  = (short) 3;   /* Block ID for TAB is always 3 */
    tab_len = (int)((*length) - tab_start);
    RPGC_set_product_int( (void *) &tab_hdr.block_len, tab_len );

    /* Copy the TAB header into the buffer.                          */

    memcpy(ptrMD+tab_start, &tab_hdr, sizeof(Tabular_alpha_block));

    /* Complete the alphanumeric data header.                        */

    alpha_hdr.divider   = (short)-1; /* Divider always -1            */
    alpha_hdr.num_pages = n_pages;   /* Number of pages              */

    /* Now we can copy the alpha header into the buffer.             */

    memcpy(ptrMD+alpha_start, &alpha_hdr, sizeof(Alpha_hdr_t));

    /* Copy the existing header into our local version               */

    memcpy(gpptr, ptrMD, sizeof(Graphic_product));

    /* Fill the Product Description Block. (Note that the return     */
    /* value is unused and therefore, unchecked)                     */

    result = RPGC_prod_desc_block((void*)(gpptr), MDAPROD, vol_num);

    /* Put the halfword offset for this block into the PDB.          */
    /* Oddly enough, it goes in the symbology block offset.          */
 
    RPGC_set_product_int( (void *) &gpptr->sym_off,
                          sizeof(Graphic_product) / 2 );

    /* Set the message header block information.                     */
    /* Use a local variable for the length because it is             */
    /* modified by the function call.                                */

    result = RPGC_prod_hdr((void*)gpptr, MDAPROD, &tab_len);

    /* Copy our local version into the output buffer.                */

    memcpy(ptrMD+tab_start+sizeof(Tabular_alpha_block),
           &gp,  sizeof(Graphic_product));

    return (tab_start / 2);
    
} /* end buildMD_TAB() */


static short writePageHeader
   (
         char*      ptrMD,     /*(OUT) Points to a final product      */
   const int        vol_num,    /*(IN) current volume scan number      */
   const int        rpg_id,     /*(IN) rpg id from adaptation data     */
   const short      n_pages,    /*(IN) Number of pages written so far  */
   const short      avg_spd,    /*(IN) Average mesocyclone speed (kts) */
   const short      avg_dir,    /*(IN) Average mesocyclone direction   */
         int*       length      /*(IN/OUT) before/after byte length    */
   )
{
   Alpha_data_t  alpha_line;         /* One line of alphanumeric page data */
   int           day,month,year;     /* for date display                   */
   int           hour,minute,second,mills; /* for time display             */
   Scan_Summary *scan_summary;       /* ptr to scan summary table          */


   /* Convert the time and NEXRAD julian date to display format.           */

   scan_summary = RPGC_get_scan_summary(vol_num);
   RPGCS_julian_to_date(scan_summary->volume_start_date,&year,&month,&day);
   RPGCS_convert_radial_time(scan_summary->volume_start_time*1000,
                             &hour, &minute, &second, &mills);

   /* Set the fixed line width field                                       */

   alpha_line.num_char = LINE_WIDTH;

   /* End the previous page if needed.                                     */

   if (n_pages != 0)
   {
      memcpy(ptrMD + (*length), &END_OF_PAGE, sizeof(short));
      *length += sizeof(short);
   } /* end if end of page */
 
   /* HEADER LINE 1 */

   sprintf(alpha_line.data,
"                        MESOCYCLONE DETECTION ALGORITHM                         ");

   memcpy(ptrMD + (*length), &alpha_line, sizeof(Alpha_data_t));
   *length += sizeof(Alpha_data_t);

   /* HEADER LINE 2 */

   sprintf(alpha_line.data,
"     RADAR ID: %03d     DATE: %02d/%02d/%02d   TIME: %02d:%02d:%02d   Avg dir/spd: %03d/%3d  ",

       rpg_id, month, day, year, hour, minute, second, avg_dir, avg_spd);

   memcpy(ptrMD + (*length), &alpha_line, sizeof(Alpha_data_t));
   *length += sizeof(Alpha_data_t);

   /* HEADER LINE 3 */
   
   sprintf(alpha_line.data,
"                                                                                ");

   memcpy(ptrMD + (*length), &alpha_line, sizeof(Alpha_data_t));
   *length += sizeof(Alpha_data_t);

   /* HEADER LINE 4 */

   sprintf(alpha_line.data,
" CIRC  AZRAN   SR STM |-LOW LEVEL-|  |--DEPTH--|  |-MAX RV-| TVS  MOTION   MSI  ");
   
   memcpy(ptrMD + (*length), &alpha_line, sizeof(Alpha_data_t));
   *length += sizeof(Alpha_data_t);

   /* HEADER LINE 5 */

   sprintf(alpha_line.data,
"  ID   deg/nm     ID  RV   DV  BASE  kft STMREL%%  kft    kts     deg/kts        ");

   memcpy(ptrMD + (*length), &alpha_line, sizeof(Alpha_data_t));
   *length += sizeof(Alpha_data_t);

   /* HEADER LINE 6 */

   sprintf(alpha_line.data,
"                                                                                ");

   memcpy(ptrMD + (*length), &alpha_line, sizeof(Alpha_data_t));
   *length += sizeof(Alpha_data_t);

   return (short)6;

} /* end writePageHeader()  */


static short writeFeatureLine
   (
    const cplt_t  *cplt,        /*(IN) Rapid Update feature        */
          char*   ptrMD,      /*(OUT) Points to a final product  */
          int*    length       /*(IN/OUT) before/after byte length*/
   )
{
   Alpha_data_t  alpha_line;   /* One line of alphanumeric page data  */
   int    event_id;
   int    base;
   int    az;
   int    rng;
   int    srank;
   int    depth;
   int    msi;
   int    ll_gtgdv;
   int    max_rv;
   int    ll_rv;
   int    hgt_max_rv;
   float  spd;
   float  dir;
   int    stm_rel_depth;
   char   storm_id[STM_ID_SIZE];
   char   tvs;
   char   gt_sym;   /* Contains > symbol for depth when the base is on lowest tilt.*/
   char   lt_sym;   /* Contains < symbol for base when the base is on lowest tilt. */
   char   sr_type;  /* Contains S or L or blank for Shallow, Low Topped or neither */

   /* Set the fixed line width.                                              */

   alpha_line.num_char = LINE_WIDTH;

   /* Fill all output fields, converting to the required units if needed.    */

   if(debugit) fprintf(stderr,"u_motion=%f, v_motion=%f\n",cplt->u_motion,cplt->v_motion);
   event_id = cplt->meso_event_id;
   strncpy(storm_id, cplt->storm_id, STM_ID_SIZE);
   base     = floor(fabs(cplt->base  * KM_TO_KFT) + 0.5);
   depth    = floor(cplt->depth * KM_TO_KFT + 0.5);
   az       = floor(cplt->ll_azm + 0.5);
   rng      = floor(cplt->ll_rng * KM_TO_NM + 0.5);
/*   rng      = floor(rng * KM_TO_NM + 0.5);*//* Done this way to match A318ND.FTN */
   srank    = cplt->strength_rank;
   msi      = floor(cplt->msi * MSI_SCALE + 0.5);
   ll_gtgdv = floor(cplt->ll_gtg_vel_dif * MPS_TO_KTS + 0.5);
   max_rv   = floor(cplt->max_rot_vel * MPS_TO_KTS + 0.5);
   hgt_max_rv = floor(cplt->height_max_rot_vel * KM_TO_KFT + 0.5);
   ll_rv    = floor(cplt->ll_rot_vel * MPS_TO_KTS + 0.5);
      
   if (cplt->storm_rel_depth > 0.)
     stm_rel_depth = floor(cplt->storm_rel_depth * PERCENT + 0.5);
   else
     stm_rel_depth = 0;
     
   if (cplt->tvs_near == TVS_YES)
      tvs = 'Y';
   else if(cplt->tvs_near == TVS_NO)
      tvs = 'N';
        else
           tvs = 'U';
      
   if (cplt->base < LOW_HT_THRESH) {
      gt_sym = '>';
      lt_sym = '<';
   }
   else {
      gt_sym = ' ';
      lt_sym = ' ';
   }
   
   if (cplt->circ_class == SHALLOW_CIRC)
      sr_type = 'S';
   else
      if (cplt->circ_class == LOW_TOPPED_MESO)
         sr_type = 'L';
      else
         sr_type = ' ';
   
   if (cplt->u_motion != UNDEFINED && cplt->v_motion != UNDEFINED) {
   
      /* Compute and format the motion fields   */
      RPGCS_xy_to_azran_u(cplt->u_motion, cplt->v_motion, METERS, &spd, NMILES, &dir);
      dir += HALF_CIRC;
      if (dir > CIRC) dir -= CIRC;
      spd *= SEC_PER_HOUR;
      
      if (spd < 5.0) {
         spd = 0.0;
         dir = 0.0;
      }
      
      if (debugit) fprintf(stderr,"spd=%f, dir=%f\n",spd,dir);
  
      /* Fill the output buffer for a feature with storm motion.    */

      sprintf(alpha_line.data,
      " %3d  %03d/%3d %2d%c %s %3d  %3d  %c%2d   %c%2d  %3d     %3d    %3d  %c  %03.0f/%3.0f %5d  ",
      event_id,
      az,
      rng,
      srank,
      sr_type,
      storm_id,
      ll_rv,
      ll_gtgdv,
      lt_sym,
      base,
      gt_sym,
      depth,
      stm_rel_depth,
      hgt_max_rv,
      max_rv,
      tvs,
      dir,
      spd,
      msi);
   }
   else {
      /* Fill the output buffer for a feature without storm motion.    */

      sprintf(alpha_line.data,
      " %3d  %03d/%3d %2d%c %s %3d  %3d  %c%2d   %c%2d  %3d     %3d    %3d  %c          %5d  ",
      event_id,
      az,
      rng,
      srank,
      sr_type,
      storm_id,
      ll_rv,
      ll_gtgdv,
      lt_sym,
      base,
      gt_sym,
      depth,
      stm_rel_depth,
      hgt_max_rv,
      max_rv,
      tvs,
      msi);
   }   

   /* Copy the alphanumeric line into the output buffer.                     */

   memcpy(ptrMD + (*length), &alpha_line, sizeof(Alpha_data_t));
   *length += sizeof(Alpha_data_t);

   return (short)1;

} /* end writeFeatureLine() */
 

