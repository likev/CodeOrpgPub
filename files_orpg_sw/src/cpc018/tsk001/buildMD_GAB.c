/*******************************************************************************
    Filename:   buildMD_GAB.c
    Author:     Brian Klein


    Description
    ===========
    This function will format the final product graphic alphanumeric block
    based on the input data array structure.  It returns the halfword offset
    to this block.

    The GAB is composed of 5 rows of information for each feature.  There
    are 6 features displayed per page like this:

         1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
                                                                                
 CIR STMID XXX    XX XXX    XX XXX    XX XXX    XX XXX    XX XXX    XX          
 SR   LLRV XXC   XXX XXC   XXX XXC   XXX XXC   XXX XXC   XXX XXC   XXX          
 AZ    RAN XXX   XXX XXX   XXX XXX   XXX XXX   XXX XXX   XXX XXX   XXX          
 HGT  MXRV XX    XXX XX    XXX XX    XXX XX    XXX XX    XXX XX    XXX          
 BASE DPTH <XX   >XX <XX   >XX <XX   >XX <XX   >XX <XX   >XX <XX   >XX          

Note:  The above template does not show the lines drawn between the rows
       and columns of the GAB.
*******************************************************************************/

/* 
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/03 22:21:51 $
 * $Id: buildMD_GAB.c,v 1.3 2005/03/03 22:21:51 ryans Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*** Global Include Files ***/

#include <math.h>
#include <rpgc.h>
#include <assert.h>
#include <a318buf.h>

#include <mda_adapt.h>
#define EXTERN extern
#include <mda_adapt_object.h>

/*** Local Include Files ***/

#include "packet_8.h"
#include "packet_10.h"
#include "mdattnn_params.h"


/*** Static Variable Declarations ***/
#define MIN(x,y) ( (x) < (y) ? (x) : (y) ) /* returns the lesser of x and y */
                               /* NOTE: These values came from the   */
                               /*   legacy meso product and are      */
                               /*   defined in cpc018/tsk004.        */
#define NUM_HORIZ_VEC  6       /* Number of horizontal vectors       */
#define NUM_VERT_VEC   8       /* Number of vertical vectoirs        */
#define I_INIT         4       /* Pixel increment in I direction     */
#define J_INIT         0       /* Pixel increment in J direction     */
#define I_WIDTH        70      /* I direction field width in pixels  */
#define J_HEIGHT       10      /* J direction field height in pixels */
#define I_LENGTH       490     /* I direction vector length          */
#define J_LENGTH       50      /* J direction vector length          */
#define YELLOW         3       /* Color value for grid               */


const static short FEATS_PER_PAGE = 6;  /* max number of features per page   */
const static short J_INC          = 10; /* pixel increment in J (Y) direction*/
const static short TEXT_LINE_BYTES= 86; /* fixed line length in bytes        */
const static short GAB_PAGE_BYTES = 574;/* fixed page length in bytes        */
const static short MAX_GAB_FEATS  = 36;
const static short GAB_ALIGN      = 2;  /* Alignment bytes for sizeof(GAB)   */

/*         circulation types[# types][# characters plus 1]                          */

const static char  gab_headers[5][11] = {" CIR STMID",
                                         " SR   LLRV",
                                         " AZ    RAN",
                                         " HGT  MXRV",
                                         " BASE DPTH"};


int buildMD_GAB
(
   const cplt_t* const ptrCplt,  /*(IN) pointer to an array of couplet structures  */
         char*   const ptrMD,    /*(OUT) pointer to an MDA final product           */
         int     num_cplts,/*(IN) number of couplets input                   */
         int    *length    /*(IN/OUT) byte length before and after GAB       */
)
{
    /*** Variable Declarations and Definitions ***/

    Graphic_alpha_block gab_block;
    cplt_t             *gab_feat_data = NULL; /* Features to be output      */
    packet_8_t          text_packet;
    packet_10_hdr_t     grid_header;
    packet_10_data_t    grid_points;
    char                data_header[11]   =  {"          "};
    char                data_field[6][11] = {{"          "},{"          "},
                                             {"          "},{"          "},
                                             {"          "},{"          "}};
    const char          blanks[11]        =  {"          "};
    char                data_row[81] = 
{"                                                                                "};

    short  page;        /* Page of GAB table.                               */
    int    gab_start;
    int    grid_start;
    int    i_az, i_rg, i_base, i_depth, i_llrv, i_mxrv, i_htmxrv;
    unsigned int block_len;
    char   gt_sym, lt_sym;    /* Holds > and < or a blank                   */
    char   sr_type;           /* Holds S, L or blank                        */
    short  i, j, fld, feat, v;
    short  field_height, field_width;
    short  total_pages;
    short  n_3Dfeat;    /* Counter for number of 3D features to output.     */
    
    
    assert(ptrCplt != NULL);
    assert(ptrMD  != NULL);
    

    /* Set up the initial GAB block header.  It will be copied to the       */
    /* output buffer at the end when we know the number of pages.           */

    gab_block.divider  = -1;
    gab_block.block_id =  2;

    /* Loop for all features in the input data array.                   */
    /* Count those we display.  This allows us to allocate a block of   */
    /* memory that will hold all displayable features.                  */

    n_3Dfeat = 0;
    for (i = 0; i < num_cplts; i++)
       if (ptrCplt[i].display_filter &&
           ptrCplt[i].strength_rank  >= mda_adapt.min_filter_rank) n_3Dfeat++;
       
    /* There is a limit of 6 pages of GAB data.  Do not exceed this limit. */

    n_3Dfeat = MIN(n_3Dfeat,MAX_GAB_FEATS);
    
    /* Return if no features */
    
    if (n_3Dfeat <= 0) return ((int)0);
    
    /* Allocate a buffer for just the number of features to display.       */
   
    gab_feat_data = (cplt_t*)malloc(sizeof(cplt_t) * n_3Dfeat);    

    /* Jump out if no buffer */
    
    if (gab_feat_data == NULL) return ((int)0);
    
    /* Copy the displayable features to the buffer.                         */
    
    j = 0;
    for (i = 0; i < num_cplts; i++)
      if (ptrCplt[i].display_filter &&
          ptrCplt[i].strength_rank  >= mda_adapt.min_filter_rank)
      {
      gab_feat_data[j] = ptrCplt[i];
      j++;
      if (j == n_3Dfeat) break;
      };

    /* At this point, we have an array (gab_feat_data) that has all the     */
    /* features we need to output according to the display filter.          */
    /* We now begin building the GAB block.                                 */
  
    /* Save the current length so we know where to copy the block header    */
    /* when we are done.                                                    */

    gab_start = *length;
    
    /* Make room for the GAB's header. (Account for alignment bytes)        */

    *length += sizeof(Graphic_alpha_block) - GAB_ALIGN;

    /* Determine the number of GAB pages needed.                            */

    total_pages = (double)(n_3Dfeat-1) / (double)FEATS_PER_PAGE + 1.0;

    /* Loop for each page of data in the GAB.                               */

    for (page = 1; page <= total_pages; page++)
    {
       /* Put the page number (starts at 1) in the output buffer and        */
       /* increase the byte length.                                         */

       memcpy(ptrMD+(*length), &page, sizeof(short));
       *length += sizeof(short);

       /* Put the page byte length in here.  Its a fixed length.            */

       memcpy(ptrMD+(*length), &GAB_PAGE_BYTES, sizeof(short));
       *length += sizeof(short);

       /* Set initial and fixed values for text packet.                     */

       text_packet.hdr.code       = PACKET_8;
       text_packet.hdr.num_bytes  = TEXT_LINE_BYTES; /* Fixed byte size     */
       text_packet.data.color_val = 0;               /* Color value         */
       text_packet.data.pos_i     = 0;               /* i position (fixed)  */
       text_packet.data.pos_j     = 1;               /* Initial j position  */


       /*** ROW 0:    CIR STMID ***/

       sprintf(data_header, gab_headers[0], sizeof(gab_headers[0]));

       fld = 0;
       for (feat = (FEATS_PER_PAGE*(page-1));
            feat < (FEATS_PER_PAGE*page);
            feat++)
       {
          if (feat < n_3Dfeat)
             sprintf(data_field[fld],
                    " %3d    %s",
                     gab_feat_data[feat].meso_event_id,
                     gab_feat_data[feat].storm_id);
          else
             sprintf(data_field[fld],"%s",blanks);

          fld++;  /* Increment the field counter */

       } /* end for row 0 features */

       sprintf(data_row,"%s%s%s%s%s%s%s%s",
              data_header,
              data_field[0], data_field[1], data_field[2],
              data_field[3], data_field[4], data_field[5], blanks);

       /* Copy packet information into the buffer.                          */

       memcpy(ptrMD + (*length), &text_packet, sizeof(packet_8_t));
       *length += sizeof(packet_8_t);

       /* Copy character data into the output buffer.                       */

       memcpy(ptrMD + (*length), &data_row, sizeof(data_row)-1);
       *length += sizeof(data_row)-1;

       /*** ROW 1:  SR LLRV    ***/

       text_packet.data.pos_j += J_INC;   /* Increment j position  */
                  
       sprintf(data_header, gab_headers[1], sizeof(gab_headers[1]));

       fld = 0;
       for (feat = (FEATS_PER_PAGE*(page-1));
            feat < (FEATS_PER_PAGE*page);
            feat++)
       {
          if (feat < n_3Dfeat)
          {
             i_llrv = floor(gab_feat_data[feat].ll_rot_vel * MPS_TO_KTS + 0.5);
             
             if (gab_feat_data[feat].circ_class == SHALLOW_CIRC)
                sr_type = 'S';
             else
                if (gab_feat_data[feat].circ_class == LOW_TOPPED_MESO)
                   sr_type = 'L';
                else
                    sr_type = ' ';
                    
             sprintf(data_field[fld],
                     "  %2d%c  %3d",
                     gab_feat_data[feat].strength_rank, sr_type,
                     i_llrv);
           }          
           else
              sprintf(data_field[fld],"%s",blanks);
              
          fld++;  /* Increment the field counter */

       } /* end for row 1 features */
            
       sprintf(data_row,"%s%s%s%s%s%s%s%s",
              data_header,
              data_field[0], data_field[1], data_field[2],
              data_field[3], data_field[4], data_field[5], blanks);

       /* Copy packet information into the buffer.                           */

       memcpy(ptrMD + (*length), &text_packet, sizeof(packet_8_t));
       *length += sizeof(packet_8_t);

       /* Copy character data into the output buffer.                        */

       memcpy(ptrMD + (*length), &data_row, sizeof(data_row)-1);
       *length += sizeof(data_row)-1;
 
       /*** ROW 2:  AZ   RAN   ***/

       text_packet.data.pos_j += J_INC;   /* Increment j position  */
                  
       sprintf(data_header, gab_headers[2], sizeof(gab_headers[2]));

       fld = 0;
       for (feat = (FEATS_PER_PAGE*(page-1));
            feat < (FEATS_PER_PAGE*page);
            feat++)
       {
          if (feat < n_3Dfeat)
          {
             i_az = (int)floor(gab_feat_data[feat].ll_azm + 0.5);
             i_rg = (int)floor(gab_feat_data[feat].ll_rng * KM_TO_NM + 0.5);
             sprintf(data_field[fld],
                     " %03d   %3d",
                     i_az,
                     i_rg);
          }
          else
             sprintf(data_field[fld],"%s",blanks);

          fld++;  /* Increment the field counter */

       } /* end for row 2 features */
 
       sprintf(data_row,"%s%s%s%s%s%s%s%s",
              data_header,
              data_field[0], data_field[1], data_field[2],
              data_field[3], data_field[4], data_field[5], blanks);

       /* Copy packet information into the buffer.                          */

       memcpy(ptrMD + (*length), &text_packet, sizeof(packet_8_t));
       *length += sizeof(packet_8_t);

       /* Copy character data into the output buffer.                       */

       memcpy(ptrMD + (*length), &data_row, sizeof(data_row)-1);
       *length += sizeof(data_row)-1;
       

       /*** ROW 3:  HGT  MXRV    ***/

       text_packet.data.pos_j += J_INC;   /* Increment j position  */
                  
       sprintf(data_header, gab_headers[3], sizeof(gab_headers[3]));

       fld = 0;
       for (feat = (FEATS_PER_PAGE*(page-1));
            feat < (FEATS_PER_PAGE*page);
            feat++)
       {
          if (feat < n_3Dfeat)
          {
             i_mxrv = floor(gab_feat_data[feat].max_rot_vel * MPS_TO_KTS + 0.5);
             i_htmxrv = floor(gab_feat_data[feat].height_max_rot_vel * KM_TO_KFT + 0.5);
             sprintf(data_field[fld],
                     "  %2d   %3d",
                     i_htmxrv,
                     i_mxrv);
           }          
           else
              sprintf(data_field[fld],"%s",blanks);
              
          fld++;  /* Increment the field counter */

       } /* end for row 3 features */
            
       sprintf(data_row,"%s%s%s%s%s%s%s%s",
              data_header,
              data_field[0], data_field[1], data_field[2],
              data_field[3], data_field[4], data_field[5], blanks);

       /* Copy packet information into the buffer.                           */

       memcpy(ptrMD + (*length), &text_packet, sizeof(packet_8_t));
       *length += sizeof(packet_8_t);

       /* Copy character data into the output buffer.                        */

       memcpy(ptrMD + (*length), &data_row, sizeof(data_row)-1);
       *length += sizeof(data_row)-1;

       
       /*** ROW 4:  BASE DPTH   ***/

       text_packet.data.pos_j += J_INC;   /* Increment j position  */
                  
       sprintf(data_header, gab_headers[4], sizeof(gab_headers[4]));

       fld = 0;
       for (feat = (FEATS_PER_PAGE*(page-1));
            feat < (FEATS_PER_PAGE*page);
            feat++)
       {
          if (feat < n_3Dfeat)
          {
             if (gab_feat_data[feat].base < 0.0) {
                gt_sym = '>';
                lt_sym = '<';
             }
             else {
                gt_sym = ' ';
                lt_sym = ' ';
             }
             
             i_base  = floor(fabs(gab_feat_data[feat].base * KM_TO_KFT) + 0.5);
             i_depth = floor(gab_feat_data[feat].depth * KM_TO_KFT + 0.5);
             sprintf(data_field[fld],
                     " %c%2d   %c%2d",
                       lt_sym, i_base, gt_sym, i_depth);
          }
          else
             sprintf(data_field[fld],"%s",blanks);
              
          fld++;  /* Increment the field counter */

       } /* end for row 4 features */
            
       sprintf(data_row,"%s%s%s%s%s%s%s%s",
              data_header,
              data_field[0], data_field[1], data_field[2],
              data_field[3], data_field[4], data_field[5], blanks);

       /* Copy packet information into the buffer.                          */

       memcpy(ptrMD + (*length), &text_packet, sizeof(packet_8_t));
       *length += sizeof(packet_8_t);

       /* Copy character data into the output buffer.                       */

       memcpy(ptrMD + (*length), &data_row, sizeof(data_row)-1);
       *length += sizeof(data_row)-1;
       

       /*** GRID VECTORS (modeled after A3CM29.FTN) ***/

       /* Save current position for packet 10 start of horizontal vectors.   */

       grid_start = *length;

       /* Move up to begining of horizontal vector endpoint positions.       */

       *length += sizeof(packet_10_hdr_t);

       /* Add horizontal vectors.                                            */

       grid_points.beg_i = I_INIT;
       grid_points.beg_j = J_INIT;
       field_height      = 0;
       
       for (v = 0; v < NUM_HORIZ_VEC; v++)
       {
          grid_points.beg_j = grid_points.beg_j + field_height;
          grid_points.end_i = grid_points.beg_i + I_LENGTH;
          grid_points.end_j = grid_points.beg_j;
          field_height = J_HEIGHT;
          
          memcpy(ptrMD + (*length), &grid_points, sizeof(packet_10_data_t));
          *length += sizeof(packet_10_data_t);
       } /* end for all horizontal vectors */

       /* Complete the packet 10 header */

       grid_header.code      = PACKET_10;
       grid_header.num_bytes = (NUM_HORIZ_VEC * BYTES_PER_VEC) +
                                                       sizeof(grid_header.val);
       grid_header.val       = YELLOW;

       /* Copy the packet 10 header into the output buffer.                  */
       /* The length was already accounted for.                              */
 
       memcpy(ptrMD + grid_start, &grid_header, sizeof(packet_10_hdr_t));

       /* Save current position for packet 10 start of vertical vectors.     */

       grid_start = *length;

       /* Move up to begining of vertical vector endpoint positions.         */

       *length += sizeof(packet_10_hdr_t);

       /* Add vertical vectors.                                              */

       grid_points.beg_i = I_INIT;
       grid_points.beg_j = J_INIT;
       field_width       = 0;

       for (v = 0; v < NUM_VERT_VEC; v++)
       {
          grid_points.beg_i = grid_points.beg_i + field_width;
          grid_points.end_i = grid_points.beg_i;
          grid_points.end_j = grid_points.beg_j + J_LENGTH;
          field_width       = I_WIDTH;
          
          memcpy(ptrMD + (*length), &grid_points, sizeof(packet_10_data_t));
          *length += sizeof(packet_10_data_t);
       }

       /* Complete the packet 10 header.                                     */

       grid_header.code      = PACKET_10;
       grid_header.num_bytes = (NUM_VERT_VEC * BYTES_PER_VEC) +
                                                       sizeof(grid_header.val);
       grid_header.val       = YELLOW;

       /* Copy the packet 10 header into the output buffer.                  */
       /* The length was already accounted for.                              */

       memcpy(ptrMD + grid_start, &grid_header, sizeof(packet_10_hdr_t));

    } /* end for all pages */

    /* Compute the byte length of this block.  Account for the page number   */
    /* and length of page fields for each page.  Also account for the GAB    */
    /* alignment bytes.                                                      */

    block_len = (total_pages *
                (GAB_PAGE_BYTES + sizeof(short) + sizeof(short)) +
                sizeof(Graphic_alpha_block) - GAB_ALIGN);

    RPGC_set_product_int( (void *) &gab_block.block_len, block_len );

    gab_block.n_pages   = total_pages;

    /* Copy the block header into the output buffer.                         */

    memcpy(ptrMD + gab_start, &gab_block, sizeof(Graphic_alpha_block)-2);

    /* Free the segment of memory.                                           */

    free(gab_feat_data);
    gab_feat_data = NULL;

    return (gab_start / 2);
    
} /* end buildMD_GAB() */
