/*******************************************************************************
    Filename:   buildMD_PSB.c
    Author:     Brian Klein


    Description
    ===========
    This function will format the final product symbology block based on
    the input data array structure.  Returns a halfword offset to the symbology
    block, as defined in the RPG to Class 1 ICD Product Description Block
  
*******************************************************************************/

/* 
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/03 22:21:52 $
 * $Id: buildMD_PSB.c,v 1.5 2005/03/03 22:21:52 ryans Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/*** Global Include Files ***/

#include <math.h>
#include <rpgc.h>
#include <assert.h>

#include <mda_adapt.h>
#define EXTERN extern
#include <mda_adapt_object.h>


/*** Local Include Files ***/

#include "packet_8.h"
#include "packet_20.h"
#include "mdattnn_params.h"


/*** Static Variable Declarations ***/

const static short debugit         = FALSE; /* Controls debug in this file only     */
const static short STRONG_THRESH = 5; /* Strngth rank thrshld for thick symbol*/
const static short LOW_HGT_THRESH = 1;/* km threshold for spiked symbol       */
const static short WHITE         = 1; /* color for meso ID text               */
const static short PAST_PACKET   = 23;/* past position packet code            */
const static short FCST_PACKET   = 24;/* forecast postion packet code         */
const static float MIN_SPEED     = 2.5;/* m/sec min speed for forecast tracks */
const static float M_TO_QKM      = .004;
const static float KM_TO_QKM     = 4.0;
const static short SYMBOL_PACKET = 2; /* Packet code for special symbols      */
const static short VECTOR_PACKET = 6; /* Packet code for linked vectors       */
const static short SYM_PACKET_LEN= 10;/* Byte length of special symbol packet */
const static short SYM_DATA_LEN  = 6; /* Byte length of special symbol data   */
const static short VEC_PACKET_LEN= 8; /* Byte length of minimum vector packet */
const static short PAST_SYMBOL   = 0x2420;
const static short FCST_SYMBOL   = 0x2520;



int buildMD_PSB
(
   const cplt_t* const ptrCplt,  /*(IN) pointer to MDATTNN data structure    */
         char*   const ptrMD,   /*(OUT) pointer to MDA final product        */
         int           num_cplts,/*(IN) volume scan number                   */
         int*          length    /*(IN/OUT) byte length of ptrMD before     */
                                 /*  and after adding the symbology block    */
)
{
    /*** Variable Declarations and Definitions ***/

    packet_20_t packet20; /* ICD defined packet 20 for point features        */
    packet_8_t  packet8;  /* ICD defined packet 8 for meso ID                */
    char        id_char[5];/* characters of meso ID plus NULL byte           */
    int         odd;      /* Flag set to YES if odd number of char. in ID    */
    int         charBytes;/* Number of bytes needed for meso ID characters   */
    int         i;        /* Loop index for each feature                     */
    int         psb_start;/* Starting index for the product symbology block  */
    int         lyr_start; /* Starting index for the data layer packets      */
    Symbology_block sym;  /* struct for the symbology block                  */
    int    hw_block_len;  /* return halfword block length                    */
    int         n_3Dfeat;
    int         past_cnt; /* Counter for number of past positions            */
    int         fcst_cnt; /* Counter for number of forecast postions         */
    unsigned int data_len; /* Symbology block layer length                   */
    short       packet_len;
    short       hw_data_len;
    short       cur_x, ix;
    short       cur_y, iy;
    short       p, f;
    
    assert(ptrCplt != NULL);
    assert(ptrMD  != NULL);
    
    if(debugit) fprintf(stderr,"\nEntering buildMDA_PSB.c\n");

    /* Save the starting index for this block and advance the length.  */
    psb_start = *length + sizeof(Graphic_product);
    *length  += sizeof(Graphic_product);
    
    /* Loop for all features in the input data array.                   */
    /* Count those we display.                                          */

    n_3Dfeat = 0;
    for (i = 0; i < num_cplts; i++)
       if (ptrCplt[i].display_filter &&
           ptrCplt[i].strength_rank  >= mda_adapt.min_filter_rank) n_3Dfeat++;
    
    /* Nothing to display */   
    if (n_3Dfeat == 0) return 0;

    /* Advance the length to include the symbology block header.  */

    *length  += sizeof(Symbology_block);

    /* Build the first fields of the symbology block but don't copy    */
    /* it to the output buffer yet.  We don't know the length fields.  */
 
    sym.divider       = (short)-1; /* block divider always -1          */
    sym.block_id      = (short) 1; /* always 1 for symbology block     */
    sym.n_layers      = (short) 1; /* we put all features in one layer */
    sym.layer_divider = (short)-1; /* layer divider always -1          */

    /* Save this position for the start of layer.                      */

    lyr_start = *length;

    /* Loop for each feature                                           */

    for (i = 0; i < num_cplts; i++)
    {
       /* Skip this feature if the display filter says so.             */

       if(debugit) fprintf(stderr," display filter = %d\n",ptrCplt[i].display_filter);
       if (ptrCplt[i].display_filter &&
           ptrCplt[i].strength_rank  >= mda_adapt.min_filter_rank)
       {
          /* Get the current position and convert to 1/4 kilometers    */
          
          cur_x = (short)floor(ptrCplt[i].llx * KM_TO_QKM + 0.5);
          cur_y = (short)floor(ptrCplt[i].lly * KM_TO_QKM + 0.5);
             
          /* Set up the packet 20 header fields                        */

          packet20.hdr.code       = PACKET_20;
          packet20.hdr.num_bytes  = BYTES_PER_FEAT;

          /* Set I and J positions in quarter kilometer units          */
          
          packet20.point.pos_i = cur_x;
          packet20.point.pos_j = cur_y;

          /* Compute the radius field as a function of the diameter    */
          /* (in quarter kilometers)                                   */

          packet20.point.attrib = (short)floor(ptrCplt[i].ll_diam * 2.0 + 0.5);

          /* Set the point feature type based on strength rank and base */

          if (ptrCplt[i].strength_rank >= STRONG_THRESH)
          {
             if (ptrCplt[i].base <= LOW_HGT_THRESH)
                packet20.point.type = (short)MDA_STRONG_LOW;
             else
                packet20.point.type = (short)MDA_STRONG_HIGH;

             /* Mesos require a meso ID to accompany them.       */
             /* The ICD requires that the packet be padded with  */
             /* zeros to fill out the halfword when an odd       */
             /* number of characters is output.                  */
             /* Determine if we have an odd number of characters */

             charBytes = 2;
             odd = NO;
             if (ptrCplt[i].meso_event_id < 10 ) {
                odd = YES;
             } else if (ptrCplt[i].meso_event_id > 99) {
                odd = YES;
                charBytes = 4;
             }
                
             packet8.hdr.code      = PACKET_8;
             packet8.hdr.num_bytes = sizeof(packet_8_data_t) + charBytes;
             packet8.data.color_val= WHITE;      
             packet8.data.pos_i    = packet20.point.pos_i;
             packet8.data.pos_j    = packet20.point.pos_j;
             
             if (odd)
               sprintf(id_char,"%d ", ptrCplt[i].meso_event_id);
             else
               sprintf(id_char,"%d", ptrCplt[i].meso_event_id);

             /* Copy the packet 20 structure to the output buffer.      */

             memcpy(ptrMD+(*length), &packet20, sizeof(packet_20_t));
             *length += sizeof(packet_20_t);

             /* Copy the packet 8 structure to the output buffer.       */

             memcpy(ptrMD+(*length), &packet8, sizeof(packet_8_t));
             *length += sizeof(packet_8_t);
             
             /* Copy the meso ID characters to the output buffer.       */
             
             memcpy(ptrMD+(*length), id_char, charBytes);
             *length += charBytes;

          } /* end if mesocyclone */
          else
          {
             packet20.point.type = (short)MDA_WEAK;

             /* Copy the packet 20 structure to the output buffer.      */

             memcpy(ptrMD+(*length), &packet20, sizeof(packet_20_t));
             *length += sizeof(packet_20_t);

          } /* end else */
          if(debugit) fprintf(stderr,"couplet %d with rank %d was displayed: az/ran=%3.0f/%3.0f diam=%3.0fkm\n",
                 ptrCplt[i].meso_event_id, ptrCplt[i].strength_rank, 
                 ptrCplt[i].ll_azm, ptrCplt[i].ll_rng, ptrCplt[i].ll_diam);\
          
          /* Add past positions, if any                                 */
          
          past_cnt = ptrCplt[i].num_past_pos;
          if(debugit) fprintf(stderr,"num_past_pos=%d\n",past_cnt);
          
          if (past_cnt > 0)
          {
             memcpy(ptrMD+(*length), &PAST_PACKET, sizeof(short));
             *length += sizeof(short);
             
             /* Compute the byte length of the past postion packet section. */
             
             packet_len = past_cnt * SYM_PACKET_LEN +
                          past_cnt * (sizeof(short) + sizeof(short)) + VEC_PACKET_LEN;

             memcpy(ptrMD+(*length), &packet_len, sizeof(short));
             *length += sizeof(short);
             
             /* Loop for each past position, placing the symbol packet  */
             /* in the buffer.                                          */
             
             for (p = 0; p < past_cnt; p++)
             {
                /* Put the past postion packet in the buffer.           */
             
                memcpy(ptrMD+(*length), &SYMBOL_PACKET, sizeof(short));
                *length += sizeof(short);
                             
                memcpy(ptrMD+(*length), &SYM_DATA_LEN, sizeof(short));
                *length += sizeof(short);
             
                /* Convert units to quarter kilometers.                 */
             
                ix = (short)floor(ptrCplt[i].past_x[p] * M_TO_QKM + 0.5); 
                iy = (short)floor(ptrCplt[i].past_y[p] * M_TO_QKM + 0.5); 
                
                memcpy(ptrMD+(*length), &ix, sizeof(short));
                *length += sizeof(short);
                memcpy(ptrMD+(*length), &iy, sizeof(short));
                *length += sizeof(short);
                
                /* Place special symbol for past postions in the buffer.*/
                
                memcpy(ptrMD+(*length), &PAST_SYMBOL, sizeof(short));
                *length += sizeof(short);
             }
             
             /* Put the past position vector packet code in the buffer. */
             
             memcpy(ptrMD+(*length), &VECTOR_PACKET, sizeof(short));
             *length += sizeof(short);
             
             /* Compute and store the length of the vector data.        */
             /* The first two shorts are the starting point x/y.        */
             /* The second two shorts are the end point x/y.            */
                
             hw_data_len = 
               sizeof(short) + sizeof(short) + (past_cnt * (sizeof(short) + sizeof(short)));
             memcpy(ptrMD+(*length), &hw_data_len, sizeof(short));
             *length += sizeof(short);

             /* Store the starting position for the vectors.            */
             
             memcpy(ptrMD+(*length), &cur_x, sizeof(short));
             *length += sizeof(short);
             memcpy(ptrMD+(*length), &cur_y, sizeof(short));
             *length += sizeof(short);
             
             /* Loop for each past position, placing the symbol         */
             /* packet in the buffer.                                   */
             
             for (p = 0; p < past_cnt; p++)
             {
                /* Convert units to quarter kilometers.                 */
             
                ix = (short)floor(ptrCplt[i].past_x[p] * M_TO_QKM + 0.5); 
                iy = (short)floor(ptrCplt[i].past_y[p] * M_TO_QKM + 0.5); 
                
                memcpy(ptrMD+(*length), &ix, sizeof(short));
                *length += sizeof(short);
                memcpy(ptrMD+(*length), &iy, sizeof(short));
                *length += sizeof(short);
             }

          }/* end of past position processing                           */
          
          /* Add forecast positions, if any (Supress them if slow mover)*/
          
          fcst_cnt = ptrCplt[i].num_fcst_pos;
          if(debugit) fprintf(stderr,"num_fcst_pos=%d\n",fcst_cnt);
          if (fcst_cnt > 0 && ptrCplt[i].prop_spd > MIN_SPEED)
          {
             memcpy(ptrMD+(*length), &FCST_PACKET, sizeof(short));
             *length += sizeof(short);
             
             /* Compute the byte length of the forecast postion packet section. */
             
             packet_len = fcst_cnt * SYM_PACKET_LEN +
                          fcst_cnt * (sizeof(short) + sizeof(short)) + VEC_PACKET_LEN;
             
             memcpy(ptrMD+(*length), &packet_len, sizeof(short));
             *length += sizeof(short);
             
             for (p = 0; p < fcst_cnt; p++)
             {
                /* Put the forecast postion packet in the buffer.       */
             
                memcpy(ptrMD+(*length), &SYMBOL_PACKET, sizeof(short));
                *length += sizeof(short);
                             
                memcpy(ptrMD+(*length), &SYM_DATA_LEN, sizeof(short));
                *length += sizeof(short);
             
                /* Convert units to quarter kilometers.                 */
             
                ix = (short)floor(ptrCplt[i].fcst_x[p] * M_TO_QKM + 0.5); 
                iy = (short)floor(ptrCplt[i].fcst_y[p] * M_TO_QKM + 0.5); 
                
                memcpy(ptrMD+(*length), &ix, sizeof(short));
                *length += sizeof(short);
                memcpy(ptrMD+(*length), &iy, sizeof(short));
                *length += sizeof(short);
                
                /* Place special symbol for forecast postions in the buffer.*/
                
                memcpy(ptrMD+(*length), &FCST_SYMBOL, sizeof(short));
                *length += sizeof(short);
             }
             
             /* Put the forecast position vector packet code in the buffer. */
             
             memcpy(ptrMD+(*length), &VECTOR_PACKET, sizeof(short));
             *length += sizeof(short);
             
             /* Compute and store the length of the vector data.        */
             /* The first two shorts are the starting point x/y.        */
             /* The second two shorts are the end point x/y.            */
                             
             hw_data_len = 
               sizeof(short) + sizeof(short) + (fcst_cnt * (sizeof(short) + sizeof(short)));
             memcpy(ptrMD+(*length), &hw_data_len, sizeof(short));
             *length += sizeof(short);

             /* Store the starting position for the vectors.            */
             
             memcpy(ptrMD+(*length), &cur_x, sizeof(short));
             *length += sizeof(short);
             memcpy(ptrMD+(*length), &cur_y, sizeof(short));
             *length += sizeof(short);
             
             /* Loop for each forecast position, placing the symbol     */
             /* packet in the buffer.                                   */
             
             for (f = 0; f < fcst_cnt; f++)
             {
                /* Convert units to quarter kilometers.                 */
             
                ix = (short)floor(ptrCplt[i].fcst_x[f] * M_TO_QKM + 0.5); 
                iy = (short)floor(ptrCplt[i].fcst_y[f] * M_TO_QKM + 0.5); 
                
                memcpy(ptrMD+(*length), &ix, sizeof(short));
                *length += sizeof(short);
                memcpy(ptrMD+(*length), &iy, sizeof(short));
                *length += sizeof(short);
             }

          }/* end of forecast position processing                       */
       }
       else
       {
       if(debugit) fprintf(stderr,"couplet %d with rank %d was filtered: az/ran = %3.0f/%3.0f\n",
              ptrCplt[i].meso_event_id, ptrCplt[i].strength_rank, 
              ptrCplt[i].ll_azm, ptrCplt[i].ll_rng);

       } /* end if display filter */

    } /* end for all features */
    
    /* Compute the length of layer field for the data layer.               */

    data_len = (unsigned int) (*length - lyr_start);
    RPGC_set_product_int( (void *) &sym.data_len, data_len ); 

    /* Compute the length of block field for the symbology block.          */

    RPGC_set_product_int( (void *) &sym.block_len, 
                          sizeof(Symbology_block) + data_len );

    /* Copy the completed symbology block header */

    memcpy(ptrMD + psb_start, &sym, sizeof(Symbology_block));

    /* The halfword offset to this block is returned.  This block follows  */
    /* the Graphic_product structure.                                      */

    hw_block_len = (int)(sizeof(Graphic_product) / 2);

    return hw_block_len;
    
} /* end buildMD_PSB() */

