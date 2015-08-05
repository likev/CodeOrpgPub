/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:57 $
 * $Id: recclalg_adapt.h,v 1.8 2007/01/30 22:56:57 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

#ifndef RECCLALG_ADAPT_H
#define RECCLALG_ADAPT_H


#define RECCLALG_DEA_NAME "alg.recclalg"


/*******************************
 *  AP/Clutter Target          *
 *******************************/   
typedef struct
   {
   /* Reflectivity based thresholds */
   double  Ztxtr0;               /*# @name "Texture of Reflectivity 0% Value"
                                    @desc "Texture value that represents 0% likelihood of a ground clutter target."
                                    @min 0 @max 80 @default 0
				    @precision 1
                                */


   double  Ztxtr1;		/*# @name "Texture of Reflectivity 100% Value"
                                    @desc "Texture value that represents 100% likelihood of a ground clutter target."
                                    @min 0 @max 80 @default 45
				    @precision 1
                                */
  
   double  Zsign0;               /*# @name "Sign of Reflectivity Change 0% Value"
                                    @desc "Absolute value of ZSIGN that represents 0% likelihood of a ground clutter target."
                                    @min 0.0 @max 1.0 @default 0.6
                                    @precision 1
                                */
   
   double  Zsign1;               /*# @name "Sign of Reflectivity Change 100% Value"
                                    @desc "Absolute value of ZSIGN that represents 100% likelihood of a ground clutter target."
                                    @min 0.0 @max 1.0 @default 0.0
                                    @precision 1
                                */

   double  Zspin0;               /*# @name "Reflectivity Spin Change 0% Value"
                                    @desc "(Spin Change-50) value that represents 0% likelihood of a ground clutter target."
				    @min 0.0 @max 100.0 @default 50.0
				    @precision 1
				*/

   double  Zspin1;		/*# @name "Reflectivity Spin Change 100% Value"
				    @desc "(Spin Change-50) value that represents 100% likelihood of a ground clutter target."
				    @min 0.0 @max 100.0 @default 0.0
				    @precision 1
				*/
				
   /* Spin pattern characteristic thresholds */				
   double  ZspinThr;             /*# @name "Spin Change Threshold"
   				    @desc "Threshold of spin change needed to be met in order to be included in the ZSPIN calclulation."
   				    @min 0.0 @max 20.0 @default 2.0
   				    @precision 1
   				*/
   				
   double  ZThr;			/*# @name "Spin Reflectivity Threshold"
   				    @desc "Threshold of reflectivity needed to be met in order to be included in the ZSPIN calculation."
   				    @units "dBZ" @min 0.0 @max 20.0 @default 5.0
   				    @precision 1
   	        		*/
   
   /* Velocity based thresholds */
   double  Vmean0;               /*# @name "Mean Velocity 0% Value"
                                    @desc "Absolute value of Doppler velocity that represents 0% likelihood of a ground clutter target."
                                    @units "m/s" @min 0.0 @max 10.0 @default 2.3
                                    @precision 1
                                */
   
   double  Vmean1;               /*# @name "Mean Velocity 100% Value"
                                    @desc "Absolute value of Doppler velocity that represents 100% likelihood of a ground clutter target."
                                    @units "m/s" @min 0.0 @max 10.0 @default 0.0
                                    @precision 1
                                */
   
   double  Vstdv0;               /*# @name "Standard Deviation of Velocity 0% Value"
                                    @desc "Standard deviation of velocity value that represents 0% likelihood of a ground clutter target."
                                    @units "m/s" @min 0.0 @max 5.0 @default 0.7
                                    @precision 1
                                */
   
   double  Vstdv1;              /*# @name "Standard Deviation of Velocity 100% Value"
                                   @desc "Standard deviation of velocity value that represents 100% likelihood of a ground clutter target."
                                   @units "m/s" @min 0.0 @max 5.0 @default 0.0
                                   @precision 1
                               */
      
  /* Spectrum Width based thresholds */
   double  Wmean0;              /*# @name "Mean Spectrum Width 0% Value"
                                   @desc "Spectrum width value that represents 0% likelihood of a ground clutter target."
                                   @units "m/s" @min 0.0 @max 5.0 @default 3.2
                                   @precision 1
                                */
   
   double  Wmean1;              /*# @name "Mean Spectrum Width 100% Value"
                                   @desc "Spectrum width value that represents 100% likelihood of a ground clutter target."
                                   @units "m/s" @min 0.0 @max 5.0 @default 0.0
                                   @precision 1
                                */
   
  /*  Define the target category weighting for each pattern characteristic */
   double  ZtxtrWt;             /*# @name "Texture of Reflectivity Weight"
                                   @desc "The relative contribution of texture in the output likelihood of an AP/clutter target."
                                   @min 0.0 @max 1.0 @default 1.0
                                   @precision 2
                                */
   
   double  ZsignWt;             /*# @name "Sign of Reflectivity Weight"
                                   @desc "The relative contribution of Sign of Refl. in the output likelihood of a ground clutter target."
                                   @min 0.0 @max 1.0 @default 1.0
                                   @precision 2
                                */
   
   double  ZspinWt;             /*# @name "Reflectivity Spin Change Weight"
                                   @desc "The relative contribution of Refl. Spin Change in the output likelihood of a ground clutter target."
                                   @min 0.0 @max 1.0 @default 1.0
                                   @precision 2
                                */

   double  VmeanWt;             /*# @name "Mean Velocity Weight"
                                   @desc "The relative contribution of Doppler velocity in the output likelihood of a ground clutter target."
                                   @min 0.0 @max 1.0 @default 1.0
                                   @precision 2
                               */
    
   double  VstdvWt;             /*# @name "Standard Deviation of Velocity Weight"
                                   @desc "The relative contribution of the standard deviation of velocity in the output likelihood of a ground clutter target."
                                   @min 0.0 @max 1.0 @default 1.0
                                   @precision 2
                               */
   
   double  WmeanWt;             /*# @name "Mean Spectrum Width Weight"
                                   @desc "The relative contribution of spectrum width in the output likelihood of a ground clutter target."
                                   @min 0.0 @max 1.0 @default 1.0
                                   @precision 2
                               */

   /* Extents for radial processing */           
   short  deltaAz;             /*# @name "Azimuthal Extent (in radials)"
                                   @desc "Number of radials (+/-) used to define the azumithal dimension of the region over which the pattern characteristics of texture, mean sign delta reflectivity and standard deviation of velocity are computed."
                                   @min 1 @max 4 @default 1
                               */    
                            
   short  deltaRng;            /*# @name "Reflectivity Range Extent"
                                   @desc "Number of reflectivity bins (+/-) used to define the range dimension of the region over which the pattern characteristics of texture and mean sign delta reflectivity are computed."
                                   @units "bins" @min 1 @max 4 @default 2
                               */
                            
   short  deltaBin;            /*# @name "Doppler Range Extent"
			           @desc "Number of Doppler bins (+/-) used to define the range dimension of the region over which the pattern characteristic of standard deviation of velocity is computed."
		                   @units "bins" @min 1 @max 8 @default 4
	                       */
                            
   } rec_cl_target_t;
   


#endif /* RECCLALG_ADAPT_H */
