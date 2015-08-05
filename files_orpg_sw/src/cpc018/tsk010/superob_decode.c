/*
 * RCS info
 * $Author: steves $
 * $Date: 2002/08/01 17:11:30 $
 * $Locker:  $
 * $Id: superob_decode.c,v 1.3 2002/08/01 17:11:30 steves Exp $
 * $revision$
 * $state$
 * $Logs$
 */

#include <rpgc.h>

/************************************************************************
 *      Module:  superob_decode.c                                        *
 *                                                                       *
 *      Description:  This module is used to  decode data based on the   *
 *                    Doppler resolution                                 *
 *									 *
 *      Input:        num: value to be decoded         			 *
 *                    res: Doppler resolution				 *
 *      Output:       decoded value        			         *
 *      return:       decoded value                                      *
 *                                                                       *
 ************************************************************************/

#define DOPPLER_RESOLUTION_LOW 2
#define DOPPLER_RESOLUTION_HIGH 1


float
superob_decode (short num, short res)
{
     float   value;   /* the decoded value which will be returned             */

        switch (res) {

            case DOPPLER_RESOLUTION_LOW :  /* low doppler resolution situation */
                if(num==0)                 /* a flag for "data below Threshhold"*/
                   value=-999.99;
                else if(num==1)            /* a flag for "signal overlaid       */
                   value=999.99;
                else
                value = (num-129);
                break;

            case DOPPLER_RESOLUTION_HIGH : /* high doppler resolution        */
                if(num==0)                 /* a flag for "data below Threshhold"*/
                  value=-999.99;
                else if(num==1)            /* a flag for "signal overlaid"   */
                  value=999.99;
                else
                value = ((num/2.0)-64.5);
                break;                       

            default :                    /* the doppler resolution is not correct*/

                RPGC_log_msg(GL_ERROR, "The resolution number is not correct");
                value = -10000.0; 
                break;

        }

        return value;
}

