
/********************************************************************

    This module is for task prfbmap.

********************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/01/30 17:15:55 $
 * $Id: prfbmap_buffer_control.c,v 1.6 2014/01/30 17:15:55 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#include <rpgcs.h>
#include <basedata.h>

#define PRFBMAP_BUFFER_CONTROL_C
#include <prf_bitmap.h>

#define SCRATCHSIZ		((460+(4*230))*sizeof(int))

/* Indices for information in Allowable PRF table.  
   See data structure Vcp_alwblprf_t in rdacnt.h. */
#define NUMALPRF		1
#define ALWBL_OFF		2

/* Static Function Prototypes. */
static int A30545_allowable_prfs();

/*\//////////////////////////////////////////////////////////////////////

   Description:
      Buffer control routine for PRF Bit Map product.

///////////////////////////////////////////////////////////////////////\*/
void A30541_buffer_control(){

    /* **************** L o c a l   d e c l a r a t i o n s ************* */
    int bufsiz, *bdataptr = NULL;
    int abortit, ref_flag, vel_flag, wid_flag;
    int build_product;

    int *bufptr = NULL, *scrptr = NULL;
    int opstat, relstat;

    /* **************** E x e c u t a b l e   c o d e ******************* */
    opstat = NORMAL;
    Endelcut = 0;
    relstat = FORWARD;
    build_product = 0;

    /* Check if there are allowable PRFs for this vcp.  Function 
       returns 0 if no allowable prfs. */
    build_product = A30545_allowable_prfs();

    /* Determine if anything to do, if not abort the whole thing. */
    abortit = 1;
    if( build_product )
        abortit = 0;

    if( !abortit ){

        /* Get scratch buffer: */
        scrptr = RPGC_get_outbuf( SCRATCH, SCRATCHSIZ, &opstat );

        if( opstat != NORMAL ){

            /* Abort if cannot acquire scratch buffer */
            RPGC_abort();
            return;

        }

        /* Get outbuf buffer */
        bufsiz = sizeof( Prfbmap_prod_t ) + 3;
        bufptr = RPGC_get_outbuf_by_name( "PRFOVLY", bufsiz, &opstat );

        if (opstat != NORMAL) {

            /* Abort if cannot acquire output buffer */
            RPGC_abort();
            return;

        }

        /* Initialize the output buffer to all zeroes.  */
        memset( bufptr, 0, bufsiz );

        /* Check if the current VCP is an SZ2 VCP and if so, do some 
           initialization. */
        Is_SZ2 = 0;
        if( (Vol_stat.vol_cov_patt >= VCP_MIN_SZ2) 
                              &&
            (Vol_stat.vol_cov_patt <= VCP_MAX_SZ2) ){

            /* Set flag to indicate SZ2 VCP. */
            Is_SZ2 = 1;

            /* Read clutter/bypass maps ... needed to determine where
               SZ2 recovery can occur. */
            if( SZ2_read_clutter() < 0 )
               Is_SZ2 = 0;

            LE_send_msg( GL_INFO, "Processing SZ2 VCP. \n" );
                
            Z_snr = DEF_Z_SNR;
            V_snr = DEF_V_SNR;

            VCP_ICD_msg_t *vcp = (VCP_ICD_msg_t *) 
                                 RPGCS_get_vcp_data( Vol_stat.vol_cov_patt );
            if( vcp != NULL ){

               VCP_elevation_cut_data_t *elev =
                   (VCP_elevation_cut_data_t *) &vcp->vcp_elev_data.data[0];
               Z_snr = elev->refl_thresh/8.0f;

               elev = (VCP_elevation_cut_data_t *) &vcp->vcp_elev_data.data[1];
               V_snr = elev->vel_thresh/8.0f;

               /* Free the memory ... we are done with it. */
               free(vcp);

            }

            LE_send_msg( GL_INFO, "The VCP Z SNR Threshold: %f, V SNR Threshold: %f\n",
                         Z_snr, V_snr );

        }

        /* Request input buffers (radial base data) and process them.
           Do until opstat <> normal .or. endelcut */
        bdataptr = RPGC_get_inbuf_by_name( "REFLDATA",  &opstat );

        /* If the input data stream has been cancelled for some reason,   
           release and destroy all output buffers obtained and abort.   
           Otherwise proceed with processing. */
        if (opstat == NORMAL) {

            /* Check for base reflectivity disabled. */
            RPGC_what_moments( (Base_data_header *) bdataptr, &ref_flag, 
                               &vel_flag, &wid_flag );
            if( !ref_flag ){

                /* Moment disabled....Release input buffer, output buffer 
                   and scratch buffer and abort. */
                RPGC_rel_inbuf( bdataptr );
                RPGC_rel_outbuf( bufptr,  DESTROY );
                RPGC_rel_outbuf( scrptr, DESTROY );
                RPGC_abort_because( PROD_DISABLED_MOMENT );
                return;

            }

        }
        else{

            /* No PRFs are selectable for this VCP. */
            RPGC_abort();
            return;

        }

        /* Call the product generation control routine. */
        while(1){ 

            if( opstat == NORMAL ){

                /* Call product generation control function. */
                A30549_product_generation_control( (char *) bufptr, (char *) bdataptr, 
                                                   (char *) scrptr );

                /* Release the input radial. */
                RPGC_rel_inbuf( bdataptr );

                /* Test for the end of the Do Until. */
                if( !Endelcut ){

                    bdataptr = RPGC_get_inbuf_by_name( "REFLDATA", &opstat);
                    continue;

                }

            }

            if( opstat == NORMAL ) 
                relstat = FORWARD;
         
            else 
                relstat = DESTROY;
         
            RPGC_rel_outbuf( bufptr, relstat );
            RPGC_rel_outbuf( scrptr, DESTROY );

            /* Break out of while() loop. */
            break;

        }

    }
    else
        RPGC_abort();
         
    /* Return to caller. */
    return;

/* End of A30541_buffer_control(). */
}

/*\//////////////////////////////////////////////////////////////////////

   Description:
      THIS MODULE RETURNS 1 IF PRF IS ALLOWABLE FOR THIS VCP

///////////////////////////////////////////////////////////////////////\*/
static int A30545_allowable_prfs(){

    int i;
    short *alwblprf = NULL;

    /* ************** E x e c u t a b l e   c o d e ************************ */
    PS_rpgvcpid = Vol_stat.rpgvcpid - 1;

    if( (alwblprf = ORPGVCP_allowable_prf_ptr( PS_rpgvcpid )) == NULL ){

        RPGC_log_msg( GL_INFO, "ORPGVCP_allowable_prf_ptr( %d ) Returned NULL\n",
                      PS_rpgvcpid );

        return 0; 

    }

    /* Get the count of allowable PRFs */
    Num_alwbprfs = alwblprf[NUMALPRF];
LE_send_msg( 0, "Num_alwbprfs: %d\n", Num_alwbprfs );

    /* Check if number is smaller than the maximum number allowed.  If 
       not, return.  If number is not at least 2, also return. */
    if( Num_alwbprfs > MAX_ALWBPRFS ) 
        return 0 ;

    else if( Num_alwbprfs <= 1 )
        return 0 ;

    /* Copy each PRF allowable for the current VCP */
    for( i = 0; i < Num_alwbprfs; i++ ) 
{
        Allowable_prfs[i] = alwblprf[i+ALWBL_OFF];
LE_send_msg( 0, "Allowable_prfs[%d]: %d\n", i, Allowable_prfs[i] );
}

    return 1;

/* End of A30545_allowable_prfs(). */
}
