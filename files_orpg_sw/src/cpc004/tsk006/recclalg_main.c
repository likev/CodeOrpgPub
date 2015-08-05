/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/02/27 21:22:29 $
 * $Id: recclalg_main.c,v 1.24 2009/02/27 21:22:29 steves Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */
/*******************************************************************************
    Filename:   recclalg_main.c
    Author:     Brian Klein

    Modified 7/02/2003 by D.Zittel for compatibility with the Multi-PRF Dealiasing
    Modified 11/26/2003 by Chris Calvert - included callback function and
                                           new dea adaptation data format

    Description
    ===========
    This is the main file for the Radar Echo Classifier (REC) algorithm for
    the target Clutter Likelihood.  The algorithm receives RAWDATA input and
    produces two intermediate output files each elevation.  There is one output
    for reflectivity radials and one for Doppler radials.

    The REC algorithm has been modified for ORPG Build 5.0 to use BASEDATA
    instead of RAWDATA as input so as to take advantage of better Doppler coverage
    provided by the Multi-PRF Dealiasing Algorithm.  Reflectivity radials are used
    that have not been mapped to the Doppler radials.  Dealiased velocity data are
    now used because they don't appear to have a negative impact on algorithm 
    performance.

    For each bin of refelctivity and Doppler data, it produces  a percentage
    likelihood that the bin is of the target type.  In this case, the target is
    Clutter.  The algorithm produces two digital arrays as outout.  There is
    another task (cpc004/tsk007) that takes this digital output to create
    displayable products.

*******************************************************************************/

/*** System Include Files ***/
#include <rpgc.h>
#include <a309.h>
#include <recclalg_arrays.h>
#include <recclalg_adapt.h>
#include <alg_adapt.h>

/*** Local Include Files ***/
#define EXTERN       /* Causes instantiation of adaptation data object here */
		     /* (See recclalg_adapt_object.h)                       */
#include "recclalg_adapt_object.h"
#include "recclalg_constants.h"


/*** Static Variable Declarations ***/
const static short BYTE_MASK = 255; /* Mask to obtain right byte information  */
const static short debugit = FALSE; /* Controls debug output in this file only */


/*** Global Variable Declarations ***/
float               DopplerRes;         /* resolution of Doppler values (from RDA) */
int                 numDBZGates;        /* number of gates (bins) in the reflectivity radial */
int                 numDopGates;        /* number of gates (bins) in the Doppler radial */
Base_data_radial*   ptrRawBuf = NULL;   /* pointer to input buffer of raw base data radial */
char*               ptrRefBuf = NULL;   /* pointer to reflectivity intermediate product output buffer */
char*               ptrDopBuf = NULL;   /* pointer to Doppler intermediate product output buffer */
int  		    RDA_type;           /* Indicates which RDA type is providing
                                           data; 0 = legacy RDA, 1 = ORDA; added in Build 9
                                           to fix indexed radial mapping 5/10/06             */

/*** Function Prototypes ***/
static void InitializeData(void);
void Classify_echo(void);
int recclalg_callback_fx( void *struct_address );

int main(int argc, char* argv[])
{
    /*** Variable Declarations and Definitions ***/
    const int           REFBUFSIZE = 192000; /* in bytes. 400 radials by 230 bins plus adaptation data and headers */
    const int           DOPBUFSIZE = 744000; /* in bytes. 400 radials by 920 bins plus adaptation data and headers */

    short               algProcess = TRUE;  /* loop control for entire algorithm */
    short               radProcess = TRUE;  /* loop control for radial processing */

    int                 apiStatus;          /* status from API calls */
    int                 radialStatus = 0;   /* status extracted from radial (see struct Base_data_hdr) */
    int                 Zradial = 0;        /* reflectivity radial number from header */
    int			Dradial = 0;		/* Doppler radial number from header */
    short               rg;                 /*** for debug output ***/
    short               bn;                 /*** for debug output ***/
    short		dbg;

    short               rngstartsurv = 0;   /* range to the start of first surveillance bin, in meters */
    short               rngstartdop = 0;    /* range to the start of first Doppler bin, in meters */
    short               pad_vals = 0;       /* number of Doppler bins to pad radial */
    int 		rc;                 /* return code from function call */


    fprintf(stderr, "\nBegin Radar Echo Classifier, Target: AP/Clutter \n");

    /* Register for BASEDATA input */
    RPGC_in_data(BASEDATA, RADIAL_DATA);

    /* Register the output data (two linear buffers, one for reflectivity and one for Doppler */
    RPGC_out_data(RECCLDIGREF, ELEVATION_DATA, INT_PROD);
    RPGC_out_data(RECCLDIGDOP, ELEVATION_DATA, INT_PROD);

    /* Register adaptation data */
    rc = RPGC_reg_ade_callback( recclalg_callback_fx,
                                &adapt_cl_target,
                                RECCLALG_DEA_NAME,
                                BEGIN_VOLUME );
    if ( rc < 0 )
      RPGC_log_msg( GL_ERROR, "RECCLALG: cannot register adaptation data callback function\n");
        
    /* Initialize the task */
    RPGC_task_init(ELEVATION_BASED, argc, argv);

    /* Initialize the maximum number of bins in the radials */
    numDBZGates = 0;
    numDopGates = 0;
    
    if (debugit){
      fprintf(stderr, "cpc004/tsk006 Initialized\n");
      fprintf(stderr, "cpc004/tsk006 rda_type = %d\n",RDA_type);
    }

    /* Infinite loop for processing */
    while (algProcess)
    {
        /* Tell the infrastructure to activate task immediately upon start of elevation */
        /* (See RPGC_task_init call) */
        if (debugit) fprintf(stderr, "Waiting for start of elevation...\n");
        RPGC_wait_act(WAIT_DRIVING_INPUT);
        if (debugit) fprintf(stderr, "\n...started\n");
   
        RDA_type = ORPGRDA_get_rda_config( NULL );  /* Find out which type of RDA
  	 				               is providing the data    */

        if( RDA_type == ORPGRDA_LEGACY_CONFIG )
           RPGC_log_msg( GL_INFO, "RDA Configuration: LEGACY\n" );

        else
           RPGC_log_msg( GL_INFO, "RDA Configuration: ORDA\n" );

        /* Read first radial */
        ptrRawBuf = (Base_data_radial*)RPGC_get_inbuf(BASEDATA, &apiStatus);
        
        /* Begin loop of processing radials */
        while (radProcess)
        {
            /* Check pointer and status, clean up if failure */
            if ((ptrRawBuf == NULL) || (apiStatus != NORMAL))
            {
                fprintf(stderr, "ERROR: Aborting because ptrRawBuf NULL or apiStatus = %d\n", apiStatus);
                RPGC_abort();
               
                /* Initialize the working arrays for the next elevation */
                InitializeData();
   
                /* Initialize the maximum number of bins in the radials */
                numDBZGates = 0;
                numDopGates = 0;

                /* Go back to sleep waiting for next elevation */
                break;
            }

            /* Extract the radial status */
            radialStatus = ptrRawBuf->hdr.status;

            if(debugit)fprintf(stderr," \nRadial #%d Status = %d, #s=%d, #d=%d\n",
                 Zradial,radialStatus,ptrRawBuf->hdr.n_surv_bins,ptrRawBuf->hdr.n_dop_bins);
            
            /* Build data arrays for the rest of the processing     */
            /* Make sure the reflectivity radials are NOT(!) mapped to the Doppler radials on this cut */
            if ( !((ptrRawBuf->hdr.msg_type & BYTE_MASK) & REF_INSERT_BIT))
            {

               /* Get the number of reflectivity bins (gates) in this radial */
               if (ptrRawBuf->hdr.n_surv_bins > numDBZGates)
                  numDBZGates = ptrRawBuf->hdr.n_surv_bins;

               /* Limit to maximum number of bins */
               if (numDBZGates > MAX_1KMBINS) numDBZGates = MAX_1KMBINS;

		/* Get the azimuth index number for this radial and make it zero relative */
		Zradial = ptrRawBuf->hdr.azi_num - 1;

                /* Extract the range to the start of the first surveillance bin, in meters. */
                rngstartsurv = ptrRawBuf->hdr.range_beg_surv;
           
                if (debugit) fprintf(stderr,"memcpy surv radial header, hdr.azi_num=%d azimuth=%f, SBflag=%d\n",
                     ptrRawBuf->hdr.azi_num, ptrRawBuf->hdr.azimuth,ptrRawBuf->hdr.spot_blank_flag);

                /* Copy the radial data into the working array (no need for *2...only getting half the radial*/
                if ((ptrRawBuf->hdr.n_surv_bins > 0)  &&
                   ((ptrRawBuf->hdr.spot_blank_flag & SPOT_BLANK_RADIAL) == 0))
                {

                   if (debugit) fprintf(stderr,"memcpy surv radial data\n");
                   memcpy(&Z_array.Z_data[Zradial], &ptrRawBuf->ref, numDBZGates*sizeof(Moment_t));
                   
                }
                
                /* Save the required radial header information for reflectivity output */
                Z_array.rHeader[Zradial].azimuth         = ptrRawBuf->hdr.azimuth;
                Z_array.rHeader[Zradial].start_angle     = ptrRawBuf->hdr.start_angle;
                Z_array.rHeader[Zradial].delta_angle     = ptrRawBuf->hdr.delta_angle;
                Z_array.rHeader[Zradial].spot_blank_flag = ptrRawBuf->hdr.spot_blank_flag;

                /*** debug output ***/
                if (debugit && (Zradial == 318))
                {
                    fprintf(stderr,"  Zradial = %d  azimuth=%f  spot blank=%d  numDBZGates=%d\n",
                            Zradial,
                            Z_array.rHeader[Zradial].azimuth,
                            Z_array.rHeader[Zradial].spot_blank_flag,
                            numDBZGates);
                    for (rg = 0; rg < numDBZGates/10; rg++)
                    {
			for (dbg = 0; dbg < 10; dbg++)
				fprintf(stderr,"%d:",Z_array.Z_data[Zradial][rg*10+dbg]);
			fprintf(stderr,"\n");
                    }
                }/*** end debug output ***/

            } /* end if reflectivity bins present and not mapped */

            if (debugit) fprintf(stderr,"n_dop_bins=%d\n", ptrRawBuf->hdr.n_dop_bins);

            /* Get the number of Doppler bins (gates) in this radial */
            if (ptrRawBuf->hdr.n_dop_bins > numDopGates)
                numDopGates = ptrRawBuf->hdr.n_dop_bins;
            
            /* Limit to maximum number of bins */
            if (numDopGates > (MAX_1KMBINS*MAX_DOP_BIN)) numDopGates = MAX_1KMBINS*MAX_DOP_BIN;

            if (debugit) fprintf(stderr,"   numDopGates=%d\n", numDopGates);

            /* Get the azimuth index number for this radial and make it zero relative */
            Dradial = ptrRawBuf->hdr.azi_num - 1;
     
            /* Extract the range to the start of the first Doppler bin, in meters. */
            rngstartdop = ptrRawBuf->hdr.range_beg_dop;

            pad_vals = 0;
            if( (rngstartsurv < rngstartdop) && (ptrRawBuf->hdr.dop_bin_size != 0)){

               /* This path should never be executed if the Doppler bin size and 
                  surveillance bin size are the same.  Therefore we assume the 
                  surveillance bin size > Doppler bin size.  Need to figure out 
                  how many bins to prepend to the Doppler data. */
               pad_vals = abs( rngstartsurv / ptrRawBuf->hdr.dop_bin_size );
               if (debugit) fprintf(stderr,"pad_vals=%d\n", pad_vals);
            }

            /* Only copy moment data if there are bins present and not spot blanked */
            if ((ptrRawBuf->hdr.n_dop_bins > 0) &&
               ((ptrRawBuf->hdr.spot_blank_flag & SPOT_BLANK_RADIAL) == 0))
            {
               /* Copy the velocity radial data into the working array (*2 for bytes)*/
               memset(&D_array.V_data[Dradial][0][0], 0, pad_vals*sizeof(Moment_t));
               memcpy(&D_array.V_data[Dradial][0][pad_vals], &ptrRawBuf->vel[0], 
                      (numDopGates-pad_vals)*sizeof(Moment_t) );

               /* Copy the spectrum width radial data into the working array (*2 for bytes) */
               memset(&D_array.W_data[Dradial][0][0], 0, pad_vals*sizeof(Moment_t));
               memcpy(&D_array.W_data[Dradial][0][pad_vals], &ptrRawBuf->spw[0], 
                      (numDopGates-pad_vals)*sizeof(Moment_t) );
            }
               
            if (debugit) fprintf(stderr,"memcpy Doppler radial header hdr.azi_num=%d azimuth=%f, SBflag=%d\n",
                   ptrRawBuf->hdr.azi_num, ptrRawBuf->hdr.azimuth,ptrRawBuf->hdr.spot_blank_flag);

            /* Save the required radial header information for Doppler output */
            D_array.rHeader[Dradial].azimuth         = ptrRawBuf->hdr.azimuth;
            D_array.rHeader[Dradial].start_angle     = ptrRawBuf->hdr.start_angle;
            D_array.rHeader[Dradial].delta_angle     = ptrRawBuf->hdr.delta_angle;
            D_array.rHeader[Dradial].spot_blank_flag = ptrRawBuf->hdr.spot_blank_flag;
                
            /*** debug output ***/
            if (debugit && (Dradial == 312))
            {
                fprintf(stderr," Dop Radial Status = %d, \n", radialStatus);
                fprintf(stderr," radial = %d  azimuth=%f  spot blank=%d numDopGates=%d\n",
                         Dradial,
                         D_array.rHeader[Dradial].azimuth,
                         D_array.rHeader[Dradial].spot_blank_flag,
                         numDopGates);
                for (rg = 0; rg < 230; rg++)
                {
                    for (bn = 0; bn < 4; bn++)
                    {
                        fprintf(stderr,"%d v=%d raw=%d ",(rg*4+bn),D_array.V_data[Dradial][rg][bn],ptrRawBuf->vel[rg*4+bn]);
                    }
                    fprintf(stderr,"\n");
                }

            }/*** end debug output ***/

            /* Check for the end of a good elevation or volume. */
            if ((radialStatus == GENDEL ) || (radialStatus == GENDVOL))
            {
                if (debugit) fprintf(stderr, "In main: GENDEL || GENDVOL elev=%d, #dopgates=%d\n",
                                   ptrRawBuf->hdr.target_elev,
                                   numDopGates);

                if ((numDBZGates > 0) && !((ptrRawBuf->hdr.msg_type & BYTE_MASK) & REF_INSERT_BIT))
                {
                   if (debugit) fprintf(stderr,"Saving Refl product header\n");

                   /* Save the required product header information for reflectivity output */
                   Z_array.pHeader.version         = RECCLALG_VERSION;
                   Z_array.pHeader.time            = ptrRawBuf->hdr.time;
                   Z_array.pHeader.date            = ptrRawBuf->hdr.date;
                   Z_array.pHeader.volume_scan_num = ptrRawBuf->hdr.volume_scan_num;
                   Z_array.pHeader.vcp_num         = ptrRawBuf->hdr.vcp_num;
                   Z_array.pHeader.elev_num        = ptrRawBuf->hdr.elev_num;
                   Z_array.pHeader.rpg_elev_ind    = ptrRawBuf->hdr.rpg_elev_ind;
                   Z_array.pHeader.target_elev     = ptrRawBuf->hdr.target_elev;
                   Z_array.pHeader.bin_size        = ptrRawBuf->hdr.surv_bin_size;
                   Z_array.pHeader.cos_ele_short   = ptrRawBuf->hdr.cos_ele * 1000.0;
                   Z_array.pHeader.last_ele_flag   = ptrRawBuf->hdr.last_ele_flag;

                   /* Adjust the number of bins output by the deltaRng value from adaptation data */
                   Z_array.pHeader.n_bins          = numDBZGates - adapt_cl_target.deltaRng;

                }/* end if non-remapped reflectivity radial */

                /* Classify echos at the end of an elevation containing Doppler data */

                if (numDopGates > 0)
                {
                    if (debugit) fprintf(stderr,"Saving Doppler product header\n");

                    /* Save the required product header information for Doppler output */
                    D_array.pHeader.version         = RECCLALG_VERSION;
                    D_array.pHeader.time            = ptrRawBuf->hdr.time;
                    D_array.pHeader.date            = ptrRawBuf->hdr.date;
                    D_array.pHeader.volume_scan_num = ptrRawBuf->hdr.volume_scan_num;
                    D_array.pHeader.vcp_num         = ptrRawBuf->hdr.vcp_num;
                    D_array.pHeader.elev_num        = ptrRawBuf->hdr.elev_num;
                    D_array.pHeader.rpg_elev_ind    = ptrRawBuf->hdr.rpg_elev_ind;
                    D_array.pHeader.target_elev     = ptrRawBuf->hdr.target_elev;
                    D_array.pHeader.bin_size        = ptrRawBuf->hdr.dop_bin_size;
                    D_array.pHeader.cos_ele_short   = ptrRawBuf->hdr.cos_ele * 1000.0;
                    D_array.pHeader.last_ele_flag   = ptrRawBuf->hdr.last_ele_flag;

                    /* Adjust the number of bins output by the deltaAz value from adaptaiton data */
                    D_array.pHeader.n_bins          = numDopGates - adapt_cl_target.deltaBin;
   
                   /* Compute the Doppler resolution conversion factor */                                 
                   DopplerRes = ptrRawBuf->hdr.dop_resolution * DOP_RES_FACTOR;

                   /* See if the Reflectivity product is requested */
                   if (RPGC_check_data(RECCLDIGREF) == NORMAL)
                   {
                      /* Get memory for output buffer that will hold the reflectivity intermediate product */
                      if (debugit) fprintf(stderr,"Calling RPGC_get_outbuf for reflectivity output\n");
                      ptrRefBuf = (char*)RPGC_get_outbuf(RECCLDIGREF, REFBUFSIZE, &apiStatus);
                    
                      /* See if the product is not requested */
                      if (apiStatus == NOT_REQD)
                      {
                         /* Set the pointer to NULL indicating the buffer was not obtained */
                         ptrRefBuf = NULL;
                      }
                      else
                      {
                         /* Check pointer and status, clean-up if failure */
                         if ((ptrRefBuf == NULL) || (apiStatus != NORMAL))
                         {
                            fprintf(stderr,"ERROR: Aborting because ptrRefBuf NULL or apiStatus = %d\n", apiStatus);
                            if (apiStatus == NO_MEM)
                                RPGC_abort_because(PROD_MEM_SHED);
                            else
                                RPGC_abort();
               
                            /* Initialize the working arrays for the next elevation */
                            InitializeData();
   
                            /* Initialize the maximum number of bins in the radials */
                            numDBZGates = 0;
                            numDopGates = 0;

                            /* Go back to sleep waiting for next elevation */
                            break;
                         } /* end if */
                      } /* end else */   
                   } /* end if */   
                   else
                   {
                      /* Set the pointer to NULL indicating the buffer was not obtained */
                      ptrRefBuf = NULL;
                   } /* end else */
                    
                   /* See if the Doppler product is requested */
                   if (RPGC_check_data(RECCLDIGDOP) == NORMAL)
                   {
                      /* Get memory for output buffer that will hold the Doppler intermediate product */
                      if (debugit) fprintf(stderr,"Calling RPGC_get_outbuf for Doppler output\n");
                      ptrDopBuf = (char*)RPGC_get_outbuf(RECCLDIGDOP, DOPBUFSIZE, &apiStatus);
                   
                      /* Check pointer and status, clean-up if failure */
                      if ((ptrDopBuf == NULL) || (apiStatus != NORMAL))
                      {
                         fprintf(stderr,"ERROR: Aborting because ptrDopBuf NULL or apiStatus = %d\n", apiStatus);

                         /* Release the reflectivity output buffer */
                         if (ptrRefBuf) RPGC_rel_outbuf((void*)ptrRefBuf, DESTROY);
                         ptrRefBuf = NULL;

                         if (apiStatus == NO_MEM)
                            RPGC_abort_because(PROD_MEM_SHED);
                         else
                            RPGC_abort();
               
                         /* Initialize the working arrays for the next elevation */
                         InitializeData();
   
                         /* Initialize the maximum number of bins in the radials */
                         numDBZGates = 0;
                         numDopGates = 0;

                         /* Go back to sleep waiting for next elevation */
                         break;
                      } /* end if */
                   } /* end if */
                   else
                   {
                      /* Set the pointer to NULL indicating the buffer was not obtained */
                      ptrDopBuf = NULL;
                   } /* end else */
   
                   /* Put the number of radials received in this elevation into the product headers. */
                   /* Add one to remove the zero relative adjustment we made earlier */
                   Z_array.pHeader.num_radials = Zradial + 1;
                   D_array.pHeader.num_radials = Dradial + 1;

                   /* Generate the intermediate products.  There is one each for */
                   /* raw reflectivity and Doppler cut.                          */
                   if (debugit) fprintf(stderr,"Calling Classify_echo(), #Zrad=%d #Drad=%d\n",
                                     Z_array.pHeader.num_radials, D_array.pHeader.num_radials);

                   Classify_echo();
                    
                   /*** debug output ***/
                   if (debugit && 0)
                   {
                      fprintf(stderr,"*Clut* Zradial = 1  azimuth=%f  spot blank=%d  numDBZGates=%d\n",
                            Z_array.rHeader[1].azimuth,
                            Z_array.rHeader[1].spot_blank_flag,
                            numDBZGates);
                      for (rg = 0; rg < numDBZGates/10; rg++)
                      {
                         for (dbg = 0; dbg < 10; dbg++)
                           fprintf(stderr,"%d:",Z_array.Z_clut[1][rg*10+dbg]);
                         fprintf(stderr,"\n");
                      }
                   }/*** end debug output ***/
                    
                   if (debugit && 0)
                   {
                      fprintf(stderr,"Z_array.pHeader.version: %d\n",Z_array.pHeader.version);
                      fprintf(stderr,"Z_array.pHeader.time: %d\n",Z_array.pHeader.time);
                      fprintf(stderr,"Z_array.pHeader.date = %d\n",Z_array.pHeader.date);
                      fprintf(stderr,"Z_array.pHeader.volume_scan_num = %d\n",Z_array.pHeader.volume_scan_num);
                      fprintf(stderr,"Z_array.pHeader.vcp_num = %d\n",Z_array.pHeader.vcp_num);
                      fprintf(stderr,"Z_array.pHeader.elev_num = %d\n",Z_array.pHeader.elev_num);
                      fprintf(stderr,"Z_array.pHeader.rpg_elev_ind = %d\n",Z_array.pHeader.rpg_elev_ind);
                      fprintf(stderr,"Z_array.pHeader.target_elev = %d\n",Z_array.pHeader.target_elev);
                      fprintf(stderr,"Z_array.pHeader.bin_size = %d\n",Z_array.pHeader.bin_size);
                      fprintf(stderr,"Z_array.pHeader.cos_ele_short = %d/n",Z_array.pHeader.cos_ele_short);
                      fprintf(stderr,"Z_array.pHeader.last_ele_flag = %d\n",Z_array.pHeader.last_ele_flag);
                      fprintf(stderr,"Z_array.pHeader.n_bins = %d\n",Z_array.pHeader.n_bins);
                   }/**end debug output**/
                  
                   /* Put the byte offset to adaptation data into the product headers */
                   Z_array.pHeader.adapt_offset = sizeof(Z_array.pHeader) + sizeof(Z_array.rHeader) + sizeof(Z_array.Z_clut);
                   D_array.pHeader.adapt_offset = sizeof(D_array.pHeader) + sizeof(D_array.rHeader) + sizeof(D_array.D_clut);
  
                   /* Copy the product header information to the proper output buffer */
                   if (ptrRefBuf) memcpy(ptrRefBuf, &Z_array.pHeader, sizeof(Z_array.pHeader));
                   if (ptrDopBuf) memcpy(ptrDopBuf, &D_array.pHeader, sizeof(D_array.pHeader));
                    
                   /* Copy the radial header information to the proper output buffer */
                   if (ptrRefBuf) memcpy(ptrRefBuf+sizeof(Z_array.pHeader),
                	   &Z_array.rHeader, sizeof(Z_array.rHeader));
                   if (ptrDopBuf) memcpy(ptrDopBuf+sizeof(D_array.pHeader),
                    	   &D_array.rHeader, sizeof(D_array.rHeader));
 
                   /* Copy the algorithm results into the proper output buffer */
                   if (ptrRefBuf) memcpy(ptrRefBuf+sizeof(Z_array.pHeader)+sizeof(Z_array.rHeader),
                    	   &Z_array.Z_clut, sizeof(Z_array.Z_clut));
                   if (ptrDopBuf) memcpy(ptrDopBuf+sizeof(D_array.pHeader)+sizeof(D_array.rHeader),
                           &D_array.D_clut, sizeof(D_array.D_clut));
 
                   /* Copy adaptation data to the output buffers */
                   if (ptrRefBuf) memcpy(ptrRefBuf+sizeof(Z_array.pHeader)+sizeof(Z_array.rHeader)+sizeof(Z_array.Z_clut),
                           &adapt_cl_target, sizeof(adapt_cl_target));
                   if (ptrDopBuf) memcpy(ptrDopBuf+sizeof(D_array.pHeader)+sizeof(D_array.rHeader)+sizeof(D_array.D_clut),
                           &adapt_cl_target, sizeof(adapt_cl_target));

                   /* Release the two output buffers and forward the products */
                   if (debugit) fprintf(stderr,"forwarding intermediate products\n");
                   if (ptrRefBuf) RPGC_rel_outbuf((void*)ptrRefBuf, FORWARD);
                   ptrRefBuf = NULL;

                   if (ptrDopBuf) RPGC_rel_outbuf((void*)ptrDopBuf, FORWARD);
                   if (debugit) fprintf(stderr,"Released output buffers\n");
                   ptrDopBuf = NULL;
                    
                   /* Initialize the working arrays for the next elevation */
                   InitializeData();

                   /* Initialize the maximum number of bins in the radials */
                   numDBZGates = 0;
                   numDopGates = 0;

                } /* end if Doppler data */
                if (debugit) fprintf(stderr,"end of Doppler data\n");

                /* Release the input memory now that we've copied the incoming radial */
                /* This is needed here because the break below skips the following buffer release */
                RPGC_rel_inbuf((void*)ptrRawBuf);
                ptrRawBuf = NULL;

                /* Go back to sleep waiting for next elevation */
                break;

            } /* end if end of elevation or volume */

            /* Release the input memory now that we've copied the incoming radial */
            RPGC_rel_inbuf((void*)ptrRawBuf);
            ptrRawBuf = NULL;

            if (debugit) fprintf(stderr,"end if EOE or EOV..reading next radial\n");

            /* Read next radial */
            ptrRawBuf = (Base_data_radial*)RPGC_get_inbuf(BASEDATA, &apiStatus);

        }/* end while processing radials */
        if (debugit) fprintf(stderr,"end while processing radials\n");

        /* Go back to sleep waiting for next elevation */
    }/* end while processing algorithm */

    fprintf(stderr, "End of main reached.  recclalg program terminated\n");

    return (0);
}/* end main */


static void InitializeData()
{
    /** Initializes variables at the beginning of a volume scan and after processing
    the end of each cut containing Doppler data.  **/

    int radial;
    int rangeBin;
    int dopBin;

    if (debugit) fprintf(stderr,"Initializing internal arrays\n");
    
    Z_array.pHeader.version         = REC_NO_DATA_FLAG;
    Z_array.pHeader.time            = REC_NO_DATA_FLAG;
    Z_array.pHeader.date            = REC_NO_DATA_FLAG;
    Z_array.pHeader.volume_scan_num = REC_NO_DATA_FLAG;
    Z_array.pHeader.vcp_num         = REC_NO_DATA_FLAG;
    Z_array.pHeader.elev_num        = REC_NO_DATA_FLAG;
    Z_array.pHeader.rpg_elev_ind    = REC_NO_DATA_FLAG;
    Z_array.pHeader.target_elev     = REC_NO_DATA_FLAG;
    Z_array.pHeader.bin_size        = REC_NO_DATA_FLAG;
    Z_array.pHeader.cos_ele_short   = REC_NO_DATA_FLAG;
    Z_array.pHeader.n_bins          = REC_NO_DATA_FLAG;
    Z_array.pHeader.last_ele_flag   = REC_NO_DATA_FLAG;
    
    D_array.pHeader.version         = REC_NO_DATA_FLAG;
    D_array.pHeader.time            = REC_NO_DATA_FLAG;
    D_array.pHeader.date            = REC_NO_DATA_FLAG;
    D_array.pHeader.volume_scan_num = REC_NO_DATA_FLAG;
    D_array.pHeader.vcp_num         = REC_NO_DATA_FLAG;
    D_array.pHeader.elev_num        = REC_NO_DATA_FLAG;
    D_array.pHeader.rpg_elev_ind    = REC_NO_DATA_FLAG;
    D_array.pHeader.target_elev     = REC_NO_DATA_FLAG;
    D_array.pHeader.bin_size        = REC_NO_DATA_FLAG;
    D_array.pHeader.cos_ele_short   = REC_NO_DATA_FLAG;
    D_array.pHeader.n_bins          = REC_NO_DATA_FLAG;
    D_array.pHeader.last_ele_flag   = REC_NO_DATA_FLAG;

    for (radial = 0; radial < MAX_RADIALS; radial++)
    {
        Z_array.rHeader[radial].azimuth = REC_NO_DATA_FLAG;
        Z_array.rHeader[radial].start_angle     = REC_NO_DATA_FLAG;
		Z_array.rHeader[radial].delta_angle     = REC_NO_DATA_FLAG;
		Z_array.rHeader[radial].spot_blank_flag = REC_NO_DATA_FLAG;

        D_array.rHeader[radial].azimuth = REC_NO_DATA_FLAG;
        D_array.rHeader[radial].start_angle     = REC_NO_DATA_FLAG;
        D_array.rHeader[radial].delta_angle     = REC_NO_DATA_FLAG;
        D_array.rHeader[radial].spot_blank_flag = REC_NO_DATA_FLAG;

        for (rangeBin = 0; rangeBin < MAX_1KMBINS; rangeBin++)
        {
            Z_array.Z_data[radial][rangeBin] = REC_NO_DATA_FLAG;
            Z_array.Z_clut[radial][rangeBin] = REC_NO_DATA_FLAG;

            for (dopBin = 0; dopBin < MAX_DOP_BIN; dopBin++)
            {
                D_array.V_data[radial][rangeBin][dopBin] = REC_NO_DATA_FLAG;
                D_array.W_data[radial][rangeBin][dopBin] = REC_NO_DATA_FLAG;
                D_array.D_clut[radial][rangeBin][dopBin] = REC_NO_DATA_FLAG;
            }/* end for each Dop bin */
        }/* end for each refl bin */
    }/* end for each radial */
}/* end intializeData() */

