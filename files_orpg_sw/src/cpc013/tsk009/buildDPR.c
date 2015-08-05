/*
 */

/*** Local Include Files ***/

#include "buildDPR_SymBlk.h"
#include "dp_lib_func_prototypes.h"
#include "qperate_func_prototypes.h"

/******************************************************************************
    Filename: buildDPR.c

    Description:
    ============
        buildDPR() performs the buffer control operations for the Digital
    Precipitation Rate (DPR) product.  It is generated every volume.

    Func calls:
       int buildDPR_SymBlk (char **ptrDPR, RPGP_product_t* ptrXDR,
                            int vol_num, int vcp_num, int *length,
                            unsigned short RateData[MAX_AZM][MAX_BINS],
                            Rate_Buf_t* rate_out, Siteadp_adpt_t* sadpt);
    Inputs:
       int             vol_num   - Volume scan number
       int             vcp_num   - Volume Coverage Pattern number
       Rate_Buf_t*     rate_out  - rate output buffer
       Siteadp_adpt_t* sadpt     - site adaptation data

    Returns: FUNCTION_SUCCEEDED (0) or FUNCTION_FAILED (1)

    Change History
    ==============
    DATE        VERSION   PROGRAMMER  NOTES
    ----------  -------   ----------  -----
    06/01/2007    0000    Cham Pham   Initial implementation for
                                      dual-polarization project
                                      (ORPG Build 11).
    06/12/2008    0001    James Ward  Added printing of min/max for
                                      debugging purposes
    01/21/2009    0002    James Ward  Move the prcp_detected_flg up
                                      to the params[2]  high byte,
                                      move the params[6] bias_flag
                                      to the params[2] low byte,
                                      and move the mean field bias
                                      to params[6].
******************************************************************************/

#define INC_SCAL 1000 /* scale factor */

int buildDPR_prod(int vol_num, int vcp_num, Rate_Buf_t* rate_out,
                  Siteadp_adpt_t* sadpt)
{
  char *ptrDPR = NULL;           /* Pointer to DPR product buffer        */
  Graphic_product *gpptr = NULL; /* Pointer to Graphic_product struct    */
  RPGP_product_t  *ptrXDR;       /* XDR formatted generic product struct */
  int   length;                  /* Number of bytes used (minus PDB)     */
  int   offsetPSB = 0;           /* Halfword offset to symbology block   */
  int   opstat = RPGC_NORMAL;    /* Return from RPGC status calls        */
  short result = 0;              /* Holds result from return function    */
  short params[10] = {0};        /* Product dependent parameters         */
  int   i, j;                    /* indices                              */
  int   mini, minj;              /* min indices                          */
  int   maxi, maxj;              /* max indices                          */
  int   temp_val;                /* temporary value holder               */
  char  msg[200];                /* stderr message                       */
  int   prod_id;
  int   julian_date, mins_since_midnight;
  unsigned short minRate = 0;
  unsigned short maxRate = 0;
  unsigned short RateData[MAX_AZM][MAX_BINS];
  unsigned char* ptemp; /* temporary pointer to facilitate sharing of
                         * high/low bytes in a halfword */

  static unsigned int graphic_size = sizeof(Graphic_product);
  static unsigned int rpgp_size    = sizeof(RPGP_product_t);

  #ifdef QPERATE_DEBUG
     fprintf( stderr, "Beginning buildDPR_prod() ...............\n" );
  #endif

  /* See if the DPR product is to be generated */

  if (RPGC_check_data_by_name("DPRPROD") == RPGC_NORMAL)
  {
     ptrDPR = (char *) RPGC_get_outbuf_by_name( "DPRPROD", DPRSIZE, &opstat );

     if(opstat != RPGC_NORMAL)
     {
        RPGC_log_msg(GL_ERROR,
                     "buildDPR_prod: Error obtaining output buffer (%d)\n",
                     opstat);

        if (opstat == NO_MEM)
           RPGC_abort_because(PROD_MEM_SHED);
        else
           RPGC_abort();

        return(FUNCTION_FAILED);
     }

     /* If we got here, opstat is RPGC_NORMAL.
      * Convert Rate Scan to 1000th of inches and find max rate value. */

     minRate = USHRT_MAX;
     maxRate = 0;

     mini = 0;
     minj = 0;
     maxi = 0;
     maxj = 0;

     for(i = 0; i < MAX_AZM; i++)
     {
       for (j = 0; j < MAX_BINS; j++)
       {
         if(rate_out->RateComb[i][j] == QPE_NODATA)
         {
            /* DPR doesn't have a NODATA flag value, so *
             * set it to the zero (lowest) rate         */

            RateData[i][j] = 0;
         }
         else /* got a good rate, which could be 0 */
         {
            temp_val = (int) RPGC_NINT(rate_out->RateComb[i][j] * INC_SCAL);

            /* Make sure we don't scale away small values */

            if((temp_val == 0) && (rate_out->RateComb[i][j] != 0.0))
               temp_val = 1;

            /* Check min/max */

            if(temp_val < minRate)
            {
               mini    = i;
               minj    = j;
               minRate = (unsigned short) temp_val;
            }

            if(temp_val > maxRate)
            {
               maxi    = i;
               maxj    = j;
               maxRate = (unsigned short) temp_val;
            }

            /* Sanity checks. */

           if(temp_val > USHRT_MAX)
              RateData[i][j] = USHRT_MAX;
           else if(temp_val < 0) /* this should not happen */
              RateData[i][j] = (unsigned short) 0;
           else
              RateData[i][j] = temp_val;

         } /* end if have a good rate */
       } /* end for all bins */
     } /* end for all radials */

     /* Print out min/max, for AWIPS debugging */

     sprintf(msg,
             "DPR MIN[%d][%d] %d (1000s in/hour), MAX[%d][%d] %d %s\n",
             mini, minj, minRate,
             maxi, maxj, maxRate,
             "(1000s in/hour)");

     RPGC_log_msg(GL_INFO, msg);
     #ifdef QPERATE_DEBUG
        fprintf(stderr, msg);
     #endif

     /* Get the product ID from the data name. */

     prod_id = RPGC_get_id_from_name("DPRPROD");

     /* Allocate the RPGP_product_t structure (generic format product) */

     if(rpgp_size > 0)
        ptrXDR = (RPGP_product_t*) malloc(rpgp_size);
     else
        ptrXDR = NULL;

     if(ptrXDR == NULL)
     {
        RPGC_log_msg(GL_ERROR,
                     "buildDPR_prod: Error obtaining XDR buffer for DPR product\n");

        RPGC_abort_because(PROD_MEM_SHED);

        return(FUNCTION_FAILED);
     }

     length = 0;

     /* Build DPR radial in generic product format */

     offsetPSB = buildDPR_SymBlk(&ptrDPR, ptrXDR, vol_num, vcp_num,
                                 &length, RateData, rate_out, sadpt);

     #ifdef QPERATE_DEBUG
     {
        fprintf(stderr,"LENGTH symblk: %d; offsetPSB: %d\n",length,offsetPSB);
        fprintf(stderr,"max_rate: %d\n", maxRate );
     }
     #endif

     if ( offsetPSB >= 0 )
     {
        /* Set some of the Product Description Block fields. */

        RPGC_prod_desc_block( (void*)ptrDPR, prod_id, vol_num );

        /* Put the halfword offsets for each block into the PDB. */

        gpptr = (Graphic_product *)ptrDPR;
        RPGC_set_product_int( (void *) &gpptr->sym_off, offsetPSB );
        RPGC_set_product_int( (void *) &gpptr->gra_off, 0 );    /* Not used */
        RPGC_set_product_int( (void *) &gpptr->tab_off, 0 );    /* Not used */

        /* Set the product version in the high byte of halfword 54 (n_maps),
         * preserving the spot blank flag in the low byte. ptemp is a
         * temporary pointer to facilitate code writing */

        ptemp = (unsigned char *) &(gpptr->n_maps);

        *ptemp = (unsigned char) rate_out->rate_supl.vol_sb;

        ptemp += 1; /* move up 1 byte */

        *ptemp = DPR_VERSION;

        /* gpptr->level_1 should be set to hex 447A 0000 */

        RPGC_set_product_float((void*) &(gpptr->level_1), INC_SCAL);
        RPGC_set_product_float((void*) &(gpptr->level_3), 0.0);

        /* According to Brian Klein, level_5 = hw 35 is reserved   *
         * by the FAA to store a logarithmic scale for Digital Vil */

        gpptr->level_6 = USHRT_MAX; /* hw 36, 65535 = maximum data level */
        gpptr->level_7 = 0;         /* hw 37 */
        gpptr->level_8 = 0;         /* hw 38 */

        /* Set the product dependent parameters. We are using this area to *
         * record the only rate supplemental data to the DPR product.      */

        UNIX_time_to_julian_mins(rate_out->rate_supl.time,
                                 &julian_date, &mins_since_midnight);

        params[0] = check_date((short) julian_date);

        params[1] = check_time((short) mins_since_midnight);

        /* 20090121 Ward Move the prcp_detected_flg up to the params[2]
         * high byte, move the params[6] bias_flag to the params[2] low byte,
         * and move the mean field bias to params[6]. */

        ptemp = (unsigned char *) &(params[2]);

        *ptemp = (unsigned char) rate_out->qpe_adapt.adj_adapt.bias_flag;

        ptemp += 1; /* move up 1 byte */

        *ptemp = rate_out->rate_supl.prcp_detected_flg;

        params[3] = check_max_inst_precip(maxRate);

        params[4] = (short) RPGC_NINT((
                                SCALE_HYBR_RATE_FILLED *
                                check_hybr_rate_filled(
                                rate_out->rate_supl.pct_hybrate_filled)));

        params[5] = (short) RPGC_NINT((SCALE_HIGHEST_ELEV_USED *
                                       check_highest_elev_used(
                                       rate_out->rate_supl.highest_elang)));

        /* 20090402 Ward On some Erin runs, only on the first volume,
         * the bias is reported as 0. This shows up in the prod_cmpr
         * as a difference in halfword 49 (cvt halfword 50).
         * This must have to do with timing when the bias_info is
         * filled via RPGC_itc_in(), a depreciated mechanism. When
         * the bias is used, a better mechanism should be used, perhaps
         * fetched right before the call to buildDPR_prod()? */

        params[6] = (short) RPGC_NINT(SCALE_MEAN_FIELD_BIAS /* 100.0 */ *
                            check_mean_field_bias(rate_out->qpe_adapt.bias_info.bias));

        #ifdef QPERATE_DEBUG
        {
          int i;
          fprintf(stderr, "level1 = %d; level2 = %d; level3 = %d\n",
                           gpptr->level_1, gpptr->level_2, gpptr->level_3);

          for ( i = 0; i < 10; ++i )
             fprintf( stderr, "param[%d] = %d\n", i, params[i] );
        }
        #endif

        result = RPGC_set_dep_params( (void*)ptrDPR, params );

        /* Fills the Message Header Block. Subtract the header length
         * because the function doesn't expect it. */

        length -= graphic_size;

        #ifdef QPERATE_DEBUG
           fprintf(stderr,"\nLENGTH (%d); prod_id ( %d )\n",
                          length, prod_id);
        #endif

        result = RPGC_prod_hdr( (void*)ptrDPR, prod_id, &length );

        #ifdef QPERATE_DEBUG
           fprintf(stderr,"\nLENGTH (%d); result ( %d )\n",
                          length, result);
        #endif

        /* Upon successful completion of the product release buffer */

        if ( result == 0 )
        {
           RPGC_rel_outbuf( ptrDPR, FORWARD|EXTENDED_ARGS_MASK, length+100 );
           ptrDPR = NULL;
        }
        else
        {
           if (ptrDPR != NULL)
           {
             RPGC_rel_outbuf(ptrDPR, DESTROY);
             ptrDPR = NULL;
             RPGC_log_msg(GL_ERROR, "buildDPR_prod: DPRPROD outbuf destroyed\n");
           }

           RPGC_abort_dataname_because("DPRPROD", PROD_MEM_SHED);
        }

        /* Release any memory allocated on behalf of the RPGP_product_t */

        RPGP_product_free((void *) ptrXDR);

     }
     else /* must've been a failure building the product. */
     {
        RPGC_rel_outbuf( ptrDPR, DESTROY );
        ptrDPR = NULL;

        if(ptrXDR != NULL)
        {
           RPGP_product_free((void *) ptrXDR);
           ptrXDR = NULL;

           RPGC_log_msg(GL_ERROR, "buildDPR_prod: DPRPROD outbuf destroyed (2)\n");
        }

        RPGC_abort_dataname_because("DPRPROD", PROD_MEM_SHED);

     } /* end offsetPSB < 0 */

   } /* end if DPR product to be generated */

   return(FUNCTION_SUCCEEDED);

} /* end buildDPR_prod() -------------------------------------- */
