/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/02/24 14:37:34 $
 * $Id: hca_callback_fx.c,v 1.6 2011/02/24 14:37:34 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/******************************************************************************
 *      Module:         Hca_callback_fx                                       *
 *      Author:         Brian Klein, Yukuan Song                              *
 *      Created:        May 2007                                              *
 *      References:     NSSL Source code in C++                               *
 *                      ORPG HCA AEL                                          *
 *                                                                            *
 *      Description:    This is the callback function for the HCA             *
 *                                                                            *
 *      Change History:                                                       *
 ******************************************************************************/



/*** System Include Files ***/
#include "rpgc.h"
#include "alg_adapt.h"
#include "hca_local.h"
#include "hca_adapt.h"

/*** Local Include Files ***/

#ifndef C_NO_UNDERSCORE

/***#ifdef LINUX
#define Hca_callback_fx hca_callback_fx__
#endif   ***/

#endif

/****************************************************************************
    Description:
       HCA callback function for its adaption parameters
    Input:
       pointer to hca_adapt_t
    Output:
    Returns:
    Globals:
    Notes:
  ***************************************************************************/

int Hca_callback_fx( void *struct_address )
{
  double get_value = 0.0;  /* used to get data element's value */
  double  get_array[NUM_CLASSES] = {0.0};
  double  get_array2[NUM_FL_INPUTS*NUM_X] = {0.0};
  int ret = -1;            /* return status */
  int i;
  hca_adapt_t *hca = (hca_adapt_t *)struct_address;



  /* Get hca data elements. */

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_V_GC",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_V_GC = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_V_GC, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".max_Z_RA",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> max_Z_RA = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: max_Z_RA unavailable, abort task\n" );
    RPGC_abort_task();
  }


  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_RHO_RA",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_RHO_RA = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_RHO_RA unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_PHIDP_RA",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_PHIDP_RA = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_PHIDP_RA unavailable, abort task\n" );
    RPGC_abort_task();
  }


  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_Z_RH",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_Z_RH = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_Z_RH unavailable, abort task\n" );
    RPGC_abort_task();
  }
  
  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_Z_HR",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_Z_HR = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_Z_HR unavailable, abort task\n" );
    RPGC_abort_task();
  }
  
  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_Zdr_HR",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_Zdr_HR = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_Zdr_HR unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".max_Z_IC",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> max_Z_IC = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: max_Z_IC unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_Z_GR",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_Z_GR = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_Z_GR unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".max_Z_GR",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> max_Z_GR = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: max_Z_GR unavailable, abort task\n" );
    RPGC_abort_task();
  }


  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".max_Zdr_GR",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> max_Zdr_GR = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: max_Zdr_GR unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_Z_BD",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_Z_BD = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_Z_BD unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_Zdr_BD",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_Zdr_BD = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_Zdr_BD unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_Z_WS",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_Z_WS = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_Z_WS unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_Zdr_WS",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_Zdr_WS = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_Zdr_WS unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".max_Rhohv_BI",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> max_Rhohv_BI = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: max_Rhohv_BI unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".max_Z_BI",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> max_Z_BI = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_INFO, "HCA: max_Z_BI unavailable, using default value\n" );
    hca -> max_Z_BI = (int)35;
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".max_Zdr_DS",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> max_Zdr_DS = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: max_Zdr_DS unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_Agg",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_Agg = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_Agg unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_Dif_Agg",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_Dif_Agg = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_Dif_Agg unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".min_snr",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> min_snr = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: min_snr unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".atten_control",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> atten_control = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_INFO, "HCA: atten_control unavailable, using default value\n" );
    hca -> atten_control = (int)0;
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".f1_a",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> f1_a = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: f1_a unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".f1_b",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> f1_b = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: f1_b unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".f1_c",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> f1_c = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: f1_c unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".f2_a",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> f2_a = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: f2_a unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".f2_b",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> f2_b = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: f2_b unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".f2_c",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> f2_c = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: f2_c unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".f3_a",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> f3_a = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: f3_a unavailable, abort task\n" );
    RPGC_abort_task();
  }

 ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".f3_b",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> f3_b = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: f3_b unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".f3_c",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> f3_c = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: f3_c unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".g1_b",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> g1_b = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: g1_b unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".g1_c",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> g1_c = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: g1_c unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".g2_b",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> g2_b = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: g2_b unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".g2_c",
                                 &get_value );
  if( ret == 0 )
  {
    hca -> g2_c = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: g2_c unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".weight_Z",
                                 get_array );
  if( ret == 0 )
  {
    for (i = FIRST_FL_CLASS; i <= LAST_FL_CLASS; i++)
       hca -> weight_Z[i] = ( float )get_array[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: weight_Z unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".weight_Zdr",
                                 get_array );
  if( ret == 0 )
  {
    for (i = FIRST_FL_CLASS; i <= LAST_FL_CLASS; i++)
       hca -> weight_Zdr[i] = ( float )get_array[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: weight_Zdr unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".weight_RHOhv",
                                 get_array );
  if( ret == 0 )
  {
    for (i = FIRST_FL_CLASS; i <= LAST_FL_CLASS; i++)
       hca -> weight_RHOhv[i] = ( float )get_array[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: weight_RHOhv unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".weight_LKdp",
                                 get_array );
  if( ret == 0 )
  {
    for (i = FIRST_FL_CLASS; i <= LAST_FL_CLASS; i++)
       hca -> weight_LKdp[i] = ( float )get_array[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: weight_LKdp unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".weight_SDZ",
                                 get_array );
  if( ret == 0 )
  {
    for (i = FIRST_FL_CLASS; i <= LAST_FL_CLASS; i++)
       hca -> weight_SDZ[i] = ( float )get_array[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: weight_SDZ unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".weight_SDPHIdp",
                                 get_array );
  if( ret == 0 )
  {
    for (i = FIRST_FL_CLASS; i <= LAST_FL_CLASS; i++)
       hca -> weight_SDPHIdp[i] = ( float )get_array[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: weight_SDPHIdp unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memRA",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memRA[i] = ( float )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memRA unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memHR",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memHR[i] = ( float )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memHR unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memRH",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memRH[i] = ( float )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memRH unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memBD",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memBD[i] = ( float )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memBD unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memBI",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memBI[i] = ( float )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memBI unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memGC",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memGC[i] = ( float )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memGC unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memDS",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memDS[i] = ( float )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memDS unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memWS",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memWS[i] = ( float )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memWS unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memIC",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memIC[i] = ( float )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memIC unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memGR",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memGR[i] = ( float )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memGR unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memFlagRA",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memFlagRA[i] = ( int )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memFlagRA unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memFlagHR",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memFlagHR[i] = ( int )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memFlagHR unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memFlagRH",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memFlagRH[i] = ( int )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memFlagRH unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memFlagBD",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memFlagBD[i] = ( int )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memFlagBD unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memFlagBI",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memFlagBI[i] = ( int )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memFlagBI unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memFlagGC",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memFlagGC[i] = ( int )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memFlagGC unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memFlagDS",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memFlagDS[i] = ( int )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memFlagDS unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memFlagWS",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memFlagWS[i] = ( int )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memFlagWS unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memFlagIC",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memFlagIC[i] = ( int )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memFlagIC unavailable, abort task\n" );
    RPGC_abort_task();
  }

  ret = RPGC_ade_get_values( HCA_DEA_NAME,
                                 ".memFlagGR",
                                 get_array2 );
  if( ret == 0 )
  {
    for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
       hca -> memFlagGR[i] = ( int )get_array2[i];
  }
  else
  {
    LE_send_msg( GL_ERROR, "HCA: memFlagGR unavailable, abort task\n" );
    RPGC_abort_task();
  }


/**
   fprintf( stderr, "HCA: min_V_GC \t\t= %f\n", hca -> min_V_GC );
   fprintf( stderr, "HCA: max_Z_RA \t\t= %d\n", hca -> max_Z_RA );
   LE_send_msg( GL_INFO, "HCA: min_Z_RH \t\t= %d\n", hca -> min_Z_RH );
   LE_send_msg( GL_INFO, "HCA: min_Z_HR \t\t= %d\n", hca -> min_Z_HR );
   LE_send_msg( GL_INFO, "HCA: min_Zdr_HR \t\t= %f\n", hca -> min_Zdr_HR );
   LE_send_msg( GL_INFO, "HCA: max_Z_IC \t\t= %d\n", hca -> max_Z_IC );
   LE_send_msg( GL_INFO, "HCA: min_Z_GR \t\t= %d\n", hca -> min_Z_GR );
   LE_send_msg( GL_INFO, "HCA: max_Z_GR \t\t= %d\n", hca -> max_Z_GR );
   LE_send_msg( GL_INFO, "HCA: max_Zdr_GR \t\t= %f\n", hca -> max_Zdr_GR );
   LE_send_msg( GL_INFO, "HCA: min_Z_BD \t\t= %d\n", hca -> min_Z_BD );
   LE_send_msg( GL_INFO, "HCA: min_Zdr_BD \t\t= %f\n", hca -> min_Zdr_BD );
   LE_send_msg( GL_INFO, "HCA: min_Z_WS \t\t= %d\n", hca -> min_Z_WS );
   LE_send_msg( GL_INFO, "HCA: min_Zdr_WS \t\t= %f\n", hca -> min_Zdr_WS );
   LE_send_msg( GL_INFO, "HCA: max_Rhohv_BI \t\t= %f\n", hca -> max_Rhohv_BI );
   LE_send_msg( GL_INFO, "HCA: max_Zdr_DS \t\t= %f\n", hca -> max_Zdr_DS );
   LE_send_msg( GL_INFO, "HCA: min_Agg \t\t= %f\n", hca -> min_Agg );
   LE_send_msg( GL_INFO, "HCA: min_snr \t\t= %f\n", hca -> min_snr );
   LE_send_msg( GL_INFO, "HCA: f1_a \t\t= %f\n", hca -> f1_a );
   LE_send_msg( GL_INFO, "HCA: f1_b \t\t= %f\n", hca -> f1_b );
   LE_send_msg( GL_INFO, "HCA: f1_c \t\t= %f\n", hca -> f1_c );
   LE_send_msg( GL_INFO, "HCA: f2_a \t\t= %f\n", hca -> f2_a );
   LE_send_msg( GL_INFO, "HCA: f2_b \t\t= %f\n", hca -> f2_b );
   LE_send_msg( GL_INFO, "HCA: f2_c \t\t= %f\n", hca -> f2_c );
   LE_send_msg( GL_INFO, "HCA: f3_a \t\t= %f\n", hca -> f3_a );
   LE_send_msg( GL_INFO, "HCA: f3_b \t\t= %f\n", hca -> f3_b );
   LE_send_msg( GL_INFO, "HCA: f3_c \t\t= %f\n", hca -> f3_c );
   LE_send_msg( GL_INFO, "HCA: g1_b \t\t= %f\n", hca -> g1_b );
   LE_send_msg( GL_INFO, "HCA: g1_c \t\t= %f\n", hca -> g1_c );
   LE_send_msg( GL_INFO, "HCA: g2_b \t\t= %f\n", hca -> g2_b );
   LE_send_msg( GL_INFO, "HCA: g2_c \t\t= %f\n", hca -> g2_c );

   for (i = 0; i < NUM_CLASSES; i++)
      LE_send_msg( GL_INFO, "HCA: weight_Z[%d] \t\t= %f\n", i, hca -> weight_Z[i] );
   for (i = 0; i < NUM_CLASSES; i++)
      LE_send_msg( GL_INFO, "HCA: weight_Zdr[%d] \t\t= %f\n", i, hca -> weight_Zdr[i] );
   for (i = 0; i < NUM_CLASSES; i++)
      LE_send_msg( GL_INFO, "HCA: weight_RHOhv[%d] \t\t= %f\n", i, hca -> weight_RHOhv[i] );
   for (i = 0; i < NUM_CLASSES; i++)
      LE_send_msg( GL_INFO, "HCA: weight_LKdp[%d] \t\t= %f\n", i, hca -> weight_LKdp[i] );
   for (i = 0; i < NUM_CLASSES; i++)
      LE_send_msg( GL_INFO, "HCA: weight_SDZ[%d] \t\t= %f\n", i, hca -> weight_SDZ[i] );
   for (i = 0; i < NUM_CLASSES; i++)
      LE_send_msg( GL_INFO, "HCA: weight_SDPHIdp[%d] \t\t= %f\n", i, hca -> weight_SDPHIdp[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memRA[%d] \t\t= %f\n", i, hca -> memRA[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memHR[%d] \t\t= %f\n", i, hca -> memHR[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memRH[%d] \t\t= %f\n", i, hca -> memRH[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memBD[%d] \t\t= %f\n", i, hca -> memBD[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memBI[%d] \t\t= %f\n", i, hca -> memBI[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memGC[%d] \t\t= %f\n", i, hca -> memGC[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memDS[%d] \t\t= %f\n", i, hca -> memDS[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memWS[%d] \t\t= %f\n", i, hca -> memWS[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memIC[%d] \t\t= %f\n", i, hca -> memIC[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memGR[%d] \t\t= %f\n", i, hca -> memGR[i] );

   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memFlagRA[%d] \t\t= %d\n", i, hca -> memFlagRA[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memFlagHR[%d] \t\t= %d\n", i, hca -> memFlagHR[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memFlagRH[%d] \t\t= %d\n", i, hca -> memFlagRH[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memFlagBD[%d] \t\t= %d\n", i, hca -> memFlagBD[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memFlagBI[%d] \t\t= %d\n", i, hca -> memFlagBI[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memFlagGC[%d] \t\t= %d\n", i, hca -> memFlagGC[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memFlagDS[%d] \t\t= %d\n", i, hca -> memFlagDS[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memFlagWS[%d] \t\t= %d\n", i, hca -> memFlagWS[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memFlagIC[%d] \t\t= %d\n", i, hca -> memFlagIC[i] );
   for (i = 0; i < NUM_FL_INPUTS*NUM_X; i++)
      LE_send_msg( GL_INFO, "HCA: memFlagGR[%d] \t\t= %d\n", i, hca -> memFlagGR[i] );

**/

  return 0;
}
