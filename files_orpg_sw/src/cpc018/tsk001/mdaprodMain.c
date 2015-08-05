/*****************************************************************************
    Filename:   mdaprodMain.c
    Author:     Brian Klein


    Description
    ===========
    This is the main function for the Mesocyclone Detection products.
    There is a legacy-like product (MD, #141) that contains a PSB, GAB and
    TAB and it is produced at the end of each volume scan.
    
    There is also a "generic format" product (DMD, #???) that is essentially
    a copy of the internal C-structure new_cplt, one structure for each
    circulation.  It is generated every elevation and serves as the rapid
    update output.
    
    The task has one input LB from the mdattnn task.
    
*****************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/10 15:52:39 $
 * $Id: mdaprodMain.c,v 1.22 2014/12/10 15:52:39 steves Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */
 
/*** System Include Files ***/

#include <rpgc.h>
#include <rpgcs.h>
#include <a309.h>
#include <mdattnn_params.h>
#include <siteadp.h>
#include <mda_adapt.h>
#include <rpc/xdr.h>   /* for generic product format of DMD product */
#include "mdaprodMain.h"


/*** Local Include Files ***/


/*** Static Variable Declarations ***/

const static short DMDPROD = 149;   /* Product code for DMD                 */
const static short debugit = FALSE; /* Controls debug in this file only     */
const static int   GOOD    = 0;     /* Good return status                   */
const int    MDAOUTSIZE    = 50000; /* bytes, based on 100 features and 
                                       display strength rank less than 5    */
const int    DMDOUTSIZE    = 300000;/* bytes, 100 features, no compression  */
const short  MAINPROCESS   = TRUE;  /* loop control for entire process      */
const short  DMD_VERSION   = 0x100; /* Version 1 (added 2D locations)       */
const short  MD_VERSION    = 0x100; /* Version 1 (added storm ID)           */

/*** Global Variable Declarations ***/
#define EXTERN       /* Causes instantiation of adaptation data object here */
                     /* (See mda_adapt_object.h)                            */  


#include "mda_adapt_object.h"
#include "orpg_product.h"
        
/*** Function Prototypes ***/
extern int buildMD_PSB(const cplt_t* const ptrCplt,             
                             char*   const ptrMD,
                             int     num_cplts,
                             int*    length);
extern int buildMD_GAB(const cplt_t* const ptrCplt, 
                             char*   const ptrMD,
                             int     num_cplts,
                             int*    length);
extern int buildMD_TAB(const cplt_t* const ptrCplt,
                             char*   const ptrMD,
                       const int     vol_num,
                       const int     rpg_id,
                       const int     num_cplts,
                       const float   avg_u,
                       const float   avg_v,
                             int*    length);
extern int buildDMD_PSB(const cplt_t* const ptrCplt,
                        const cplt_t* const ptrLast,
                              char**        ptrDMD,
                        RPGP_product_t *ptrXDR,
                        const int     num_cplts,
                        const int     num_old_cplts,
                        const float   avg_u,
                        const float   avg_v,
                        const int     last_elev_flag,
                        const int     vcp_num,
                        const int     elev_num,
                        const int     vol_num,
                        const int     rda_elev,
                        const char   *rpg_name,
                        const int     elev_time[],
                              int    *length);
 
        
int main(int argc, char* argv[])
{
    /*** Variable Declarations and Definitions ***/

    int             *ptrIn    = NULL; /* pointer to start of input buffer    */
    cplt_t          *ptrCplt  = NULL; /* pointer to struct output by mdattnn */
    cplt_t          *ptrLast  = NULL; /* input from last elev in previous vol*/
    char            *ptrMD    = NULL; /* pointer to final MD  product        */
    Graphic_product *gpptr    = NULL;
    char            *ptrDMD   = NULL; /* pointer to DMD product              */
    RPGP_product_t  *ptrXDR;          /* XDR formatted generic product struct*/
    Siteadp_adpt_t   site_adpt;       /* For site information struct         */

    int   result;
    int   vol_num;
    int   elev_ind;
    int   vcp_num;
    int   last_index;
    int   last_el_flag;
    int   num_old_cplts = 0;
    int   num_cplts;
    float avg_u = 0.;
    float avg_v = 0.;
    int   rc;               /* Adaptation data return value                 */
    int   length;           /* number of bytes used (minus PDB)             */
    int   offsetPSB;        /* halfword offset to symbology block           */
    int   offsetGAB;        /* halfword offset to graphic alpha block       */
    int   offsetTAB;        /* halfword offset to tabular alpha block       */
    int   opstatMDATTNN = 0;
    int   opstatMD      = 0;
    int   opstatDMD     = 0;/* return status from RPGC calls.               */
    short params[10] = {0}; /* product dependent parameters                 */
    int   elev_time[MESO_NUM_ELEVS]; /* elevation start time array (sec)*/
    int   i;
    short vers;

    /* Register for inputs.                    */

    RPGC_in_data(MDATTNN, ELEVATION_DATA);

    /* Register for output.                    */

    RPGC_out_data(DMDPROD, ELEVATION_DATA, DMDPROD);
    RPGC_out_data(MDAPROD, VOLUME_DATA, MDAPROD);
    
    /* Initialize message logging.             */
    
    RPGC_init_log_services(argc, argv);

    /* Register for Scan Summary Table access. */

    RPGC_reg_scan_summary();

    /* Register for site adaptation data.  We need the radar id for */
    /* the tabular alphnumeric product (See buildMDA_TAB.c) and     */
    /* the radar name and height for the DMD product.               */

    rc = RPGC_reg_site_info( &site_adpt );
    if ( rc < 0 )
    {
      RPGC_log_msg( GL_ERROR, "SITE INFO: cannot register adaptation data callback function\n");
    }

    /* Register for MDA adaptation data        */

    rc = RPGC_reg_ade_callback( mda_callback_fx,
                                &mda_adapt,
                                MDA_DEA_NAME,
                                BEGIN_VOLUME );
    if ( rc < 0 )
        RPGC_log_msg( GL_ERROR, "MDA: cannot register adaptation data callback function\n");

    /* Initialize the task.                     */

    RPGC_task_init(ELEVATION_BASED, argc, argv);
    
    RPGC_log_msg(GL_INFO, "cpc018/tsk001 Initialized\n");

    /* Infinite loop for processing.            */

    while (MAINPROCESS)
    {
       /* Tell the infrastructure to activate task immediately upon */
       /* receipt of input data.                                    */

       if (debugit) fprintf(stderr, "Waiting for input data...\n");
       RPGC_wait_act(WAIT_DRIVING_INPUT);
       if (debugit) fprintf(stderr,"\n...started***************************\n");

       /* Obtain the available input buffer.    */

       ptrIn = (int*)RPGC_get_inbuf(MDATTNN,&opstatMDATTNN);

       /* Error check from get inbuf.           */
         
       if(opstatMDATTNN != NORMAL)
       {
          RPGC_log_msg( GL_INFO,
           "mdaprod: Error obtaining input buffer MDATTNN, opstat=%d\n",opstatMDATTNN);
          RPGC_abort();
          continue;
       }
       
       /* Get the volume number.  It is needed for API calls later */
       
       vol_num  = RPGC_get_buffer_vol_num((void*)ptrIn);
       
       /* Get the number of couplets.  Its the first 4 bytes.      */
       
       num_cplts = ((mda_ttnn_hdr_t*)ptrIn)->num_cplts;
       if (debugit) fprintf(stderr, "mdaprod, num_cplts=%d, vol=%d\n",num_cplts,vol_num);
       
       /* Get the average U and V motion.  Its the next 8 bytes.   */
       
       avg_u = ((mda_ttnn_hdr_t*)ptrIn)->avg_u;
       avg_v = ((mda_ttnn_hdr_t*)ptrIn)->avg_v;
       if (debugit) fprintf(stderr, "mdaprod, avg_u=%f avg_v=%f\n",avg_u,avg_v);

       /* The elevation time array is next in the buffer.          */
       
       for (i = 0; i < MESO_NUM_ELEVS; i++)
          elev_time[i] = ((mda_ttnn_hdr_t*)ptrIn)->elev_time[i];
       if (debugit) 
         for (i = 0; i <MESO_NUM_ELEVS; i++)
           fprintf(stderr, " elev_time[%d]=%d ",i,elev_time[i]);
       
       /* Skip over the number of couplets, average U and V, and the */
       /* elevation time integers.                                   */
       
       ptrCplt   = (cplt_t*)(ptrIn+3+MESO_NUM_ELEVS);
       if (debugit) fprintf(stderr, "\nptrCplt =%p \n",ptrCplt);
              
       /* Get the VCP number using the buffer pointer. Then get the    */
       /* index so we can get the elevation angle that ends up in the  */
       /* product dependent parameters array and determine if this is  */
       /* the last elevation of the volume.                            */
          
       vcp_num    = RPGC_get_buffer_vcp_num((void*)ptrIn);
       RPGC_is_buffer_from_last_elev( (void*)ptrIn, &elev_ind, &last_index );

       last_el_flag = 0;
       if (last_index == elev_ind)  last_el_flag = 1;
       if (debugit) fprintf(stderr, "elev_ind=%d last_el_flag =%d \n",elev_ind,last_el_flag);

       /* See if the DMD product is to be generated */
       
       if (RPGC_check_data(DMDPROD) == NORMAL)
       {
         
          /* Obtain an output buffer for DMDPROD.   */
 
          ptrDMD = (char*)RPGC_get_outbuf(DMDPROD,DMDOUTSIZE,&opstatDMD);

          if(opstatDMD != NORMAL)
          { 
             RPGC_log_msg(GL_INFO,
              "mdaprod: Error obtaining output buffer, opstatDMD=%d\n", opstatDMD);
             if(opstatDMD == NO_MEM)
                RPGC_abort_because(PROD_MEM_SHED);
             else
                RPGC_abort();
             RPGC_rel_inbuf(ptrIn);
             ptrIn    = NULL;
             ptrCplt  = NULL;
             continue;
          }    
          else
          {
             /* Perform DMD product processing */
   
             /* Allocate the RPGP_product_t structure (generic format product) */
         
             ptrXDR = (RPGP_product_t*)malloc(sizeof(RPGP_product_t));
       
             if (ptrXDR == NULL)
             {
                RPGC_log_msg( GL_ERROR,
                 "mdaprod: Error obtaining XDR buffer for DMD product\n");
                RPGC_abort_because(PROD_MEM_SHED);
                RPGC_rel_inbuf(ptrIn);
                continue;
             }

             length = 0;
                          
             offsetPSB = buildDMD_PSB(ptrCplt, ptrLast, &ptrDMD, ptrXDR, num_cplts,
                                      num_old_cplts, avg_u, avg_v, last_el_flag,
                                      vcp_num, elev_ind, vol_num, site_adpt.rda_elev,
                                      site_adpt.rpg_name, elev_time, &length);
             if (offsetPSB >= GOOD) {
          
                /* Set some of the Product Description Block fields.            */
       
                RPGC_prod_desc_block((void*)ptrDMD, DMDPROD, vol_num);

                /* Put the halfword offsets for each block into the PDB.        */

                gpptr = (Graphic_product *)ptrDMD;
                RPGC_set_product_int( (void *) &gpptr->sym_off, offsetPSB );
                RPGC_set_product_int( (void *) &gpptr->gra_off, 0 );    /* Not used */
                RPGC_set_product_int( (void *) &gpptr->tab_off, 0 );    /* Not used */

                /* Set the product version in the high byte, preserving the */
                /* spot blank flag (n_maps) in the low byte.                         */
                
                vers = gpptr->n_maps + DMD_VERSION;
                gpptr->n_maps = vers;
                       
                /* Set the product dependent parameters.  We are using this area  */
                /* to record the only adaptation data parameter applicable to the */
                /* DMD product and the elevation angle.                           */

                params[0] = mda_adapt.min_refl;
                params[1] = 0;        /* unused */
                params[2] = (short)RPGCS_get_target_elev_ang(vcp_num, elev_ind);
                result = RPGC_set_dep_params((void*)ptrDMD, params);

                /* Fill the Message Header Block. (This result is used so check it)*/
                /* (Subtract the header length because function doesn't expect it.)*/

                length -= sizeof(Graphic_product);
                result  = RPGC_prod_hdr((void*)ptrDMD, DMDPROD, &length);
          
                /* Upon successful completion of the product release buffer   */

                if(result == GOOD)
                {
                   RPGC_rel_outbuf(ptrDMD,FORWARD);
                   ptrDMD = NULL;
                }
                else
                {
                   RPGC_log_msg(GL_INFO,"mdaprod: DMD outbuf destroyed\n");
                   RPGC_rel_outbuf(ptrDMD,DESTROY);
                   ptrDMD = NULL;
                   RPGC_abort_datatype_because(DMDPROD, PROD_MEM_SHED);
                }
             
                /* Release any memory allocated on behalf of the RPGP_product_t    */

                RPGP_product_free((void *) ptrXDR);
             }
             else
             {
                /* Must have been a failure building the product.               */
             
                RPGC_rel_outbuf(ptrDMD,DESTROY);
                ptrDMD = NULL;
                ptrXDR = NULL;
                RPGC_abort_datatype_because(DMDPROD, PROD_MEM_SHED);
             }
          } /* endif DMD product to be generated */
       } /* end of DMD product processing */
          
       if (last_el_flag)
       {
          /* Release memory from any previous saved data.                 */
             
          num_old_cplts = 0;
          if (ptrLast) free(ptrLast);
             
          /* Allocate memory for this input data.  This will be used in   */
          /* the next volume to check for updated data in buildDMD_PSB.   */
             
          ptrLast = (cplt_t*)malloc(sizeof(cplt_t) * num_cplts);
             
          /* Save the data for use in the next volume.                     */
             
          if (ptrLast) {
             memcpy(ptrLast, ptrCplt, (sizeof(cplt_t) * num_cplts));
             num_old_cplts = num_cplts;
          }

          /* Obtain an output buffer for MDAPROD.   */
 
          ptrMD = (char*)RPGC_get_outbuf(MDAPROD,MDAOUTSIZE,&opstatMD);

          if(opstatMD != NORMAL)
          {
             RPGC_log_msg(GL_INFO,
              "mdaprod: Error obtaining output buffer for MD, opstat=%d\n", opstatMD);
          }
          else
          {
             /* Perform MD product processing */

             length = 0;
       
             /* Build the final product symbology block.                 */
       
             offsetPSB = buildMD_PSB(ptrCplt, ptrMD, num_cplts, &length);
             if (debugit) fprintf(stderr, "mdaprod, offsetPSB=%d\n",offsetPSB);
       
             /* Set the some of the Product Description Block fields             */
             /* We do this here so it is available for copying in the TAB header */
       
             RPGC_prod_desc_block((void*)ptrMD, MDAPROD, vol_num);
       
             /* Build the final product graphic alphanumeric block.      */

             offsetGAB = buildMD_GAB(ptrCplt, ptrMD, num_cplts, &length);
             if (debugit) fprintf(stderr, "mdaprod, offsetGAB=%d\n",offsetGAB);

             /* Build the final product tabular alphanumeric block.      */
             offsetTAB = 0;
             offsetTAB = buildMD_TAB(ptrCplt, ptrMD, vol_num, site_adpt.rpg_id,
                                      num_cplts, avg_u, avg_v, &length);
             if (debugit) fprintf(stderr, "mdaprod, offsetTAB=%d\n",offsetTAB);
   
             /* Put the halfword offsets for each block into the PDB.    */

             gpptr = (Graphic_product *)ptrMD;
             RPGC_set_product_int( (void *) &gpptr->sym_off, offsetPSB );
             RPGC_set_product_int( (void *) &gpptr->gra_off, offsetGAB );
             RPGC_set_product_int( (void *) &gpptr->tab_off, offsetTAB );
    
             /* Set the product version in the high byte, preserving the */
             /* spot blank flag (n_maps) in the low byte.                         */
              
             vers = gpptr->n_maps + MD_VERSION;
             gpptr->n_maps = vers;
             
             /* Set the product dependent parameters.  We are using this area    */
             /* to record the adaptation data parameters.                        */

             params[0] = mda_adapt.min_refl;
             params[1] = mda_adapt.overlap_filter_on;
             params[2] = mda_adapt.min_filter_rank;
             if (debugit) fprintf(stderr, "params[0] - [2]:,%hd, %hd, %hd\n",params[0],params[1],params[2]);
             result = RPGC_set_dep_params((void*)ptrMD, params);
      
             /* Fill the Message Header Block. (This result is used so check it) */
             /* (Subtract the header length because function doesn't expect it.) */

             length -= sizeof(Graphic_product);
             result  = RPGC_prod_hdr((void*)ptrMD, MDAPROD, &length);
 
             /* Upon successful completion of the product release buffer   */

             if(result == GOOD)
             {
                RPGC_rel_outbuf(ptrMD,FORWARD);
                ptrMD = NULL;
             }
             else
             {
                RPGC_log_msg(GL_INFO,"mdaprod: MD outbuf destroyed\n");
                RPGC_rel_outbuf(ptrMD,DESTROY);
                ptrMD = NULL;
                RPGC_abort_datatype_because(MDAPROD, PROD_MEM_SHED);
             }

          } /* end of MD processing */
             
       } /* end if last elevation */

       /* Release the input memory */

       if (debugit) fprintf(stderr,"mdaprod: releasing MDATTNN input\n");
       RPGC_rel_inbuf(ptrIn);
       ptrIn  = NULL;
      
    } /* end while mainProcess */

    RPGC_log_msg(GL_INFO, "End of main reached, mdaprod terminated\n");
    return (0);
} /* end mdaprodMain */
