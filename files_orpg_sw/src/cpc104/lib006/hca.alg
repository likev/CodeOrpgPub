# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2011/09/26 21:35:39 $
# $Id: hca.alg,v 1.5 2011/09/26 21:35:39 ccalvert Exp $
# $Revision: 1.5 $
# $State: Exp $

    alg_name 		value = HCA 

    min_V_GC            value = 1.0;	
			name = Minimum GC Velocity;	
			type = float;
			unit = m/s;
			range = [0.0, 5.0];	
                        accuracy = [0.1];
			description = Minimum allowable velocity for a sample volume to be classified as Ground Clutter;

    max_Z_RA            value = 50;
                        name = Maximum RA Reflectivity;
                        type = int;
                        unit = dBZ;
                        range = [45, 50];
                        description = the maximum allowable reflectivity for a sample volume to be classified as Light or Moderate Rain;

    min_RHO_RA          value = 0.94;
                        name = Minimum RA RHO;
                        type = float;
                        range = [0.50, 1.00];
                        description = Minimum allowable FL INPUT TYPE(Correlation Coefficient) for a SAMPLE VOLUME to be classified as HYDRO CLASS(RA);

    min_PHIDP_RA        value = 100;
                        name = Minimum RA PHIDP;
                        type = int;
                        range = [80, 120];
                        description = Minimum allowable FL INPUT TYPE(Differential Phase) for a SAMPLE VOLUME to be classified as HYDRO CLASS(RA);


    min_Z_RH            value = 30;
                        name = Minimum RH Reflectivity;
                        type = int;
                        unit = dBZ;
                        range = [25, 50];
                        description = the minimum allowable reflectivity for a sample volume to be classified as Rain and Hail;

    min_Z_HR            value = 30;
                        name = Minimum HR Reflectivity;
                        type = int;
                        unit = dBZ;
                        range = [25, 35];
                        description = the minimum allowable reflectivity for a sample volume to be classified as Heavy Rain;

    min_Zdr_HR          value = 1.0;
                        name = Minimum HR Diff. Reflectivy;
                        type = float;
                        unit = dB; 
                        range = [0.0, 3.0];
                        accuracy = [0.1];
                        description = the minimum allowable differential reflectivity for a sample volume to be classified as Heavy Rain;

    max_Z_IC            value = 40;
                        name = Maximum IC Reflectivity;
                        type = int;
                        unit = dBZ;
                        range = [35, 45];
                        description = the maximum allowable relectivity for a sample volume to be classified as Ice Crystals;

   min_Z_GR             value = 10;
                        name = Minimum GR Reflectivity;
                        type = int;
                        unit = dBZ;
                        range = [5, 15];
                        description = the lower limit of a range of reflectivity for a sample volume to be classified as Graupel;

   max_Z_GR             value = 60;
                        name = Maximum GR Reflectivity;
                        type = int;
                        unit = dBZ;
                        range = [55, 65];
                        description = the upper limit of a range of reflectivity for a sample volume to be eliminated as Gaupel;

   max_Zdr_GR           value = 2.0;
                        name = Maximum GR Diff. Reflectivity;
                        type = float;
                        unit = dB;
                        range = [0.0, 4.0];
                        accuracy = [0.1];
                        description = the maximum allowable differential reflectivity for a sample volume to be classified as Graupel;

   min_Z_BD             value = 15;
                        name = Minimum BD Reflectivity;
                        type = int;
                        unit = dBZ;
                        range = [0, 20];
                        description = the minimum allowable reflectivity for a sample volume to be classified as Big Drops;

   min_Zdr_BD           value = 0.5;
                        name = Minimum BD Diff. Reflectivity;
                        type = float;
                        unit = dB;
                        range = [0.0, 3.0];
                        accuracy = [0.1];
                        description = the minimum allowable differential reflectivity for a sample volume to be classified as Big Drops;

   min_Z_WS             value = 20;
                        name = Minimum WS Reflectivity;
                        type = int;
                        unit = dBZ;
                        range = [15, 25];
                        description = the minimum allowable reflectivity for a sample volume to be classified as Wet Snow;

   min_Zdr_WS           value = 0.0;
                        name = Minimum WS Diff. Reflectivity;
                        type = float;
                        unit = dB;
                        range = [-1.0, 1.0];
                        accuracy = [0.1];
                        description = the minimum allowable differential reflectivity for a sample volume to be classified as Wet Snow;

   max_Rhohv_BI         value = 0.97;
                        name = Maximum BI Correlation Coefficent;
                        type = float;
                        range = [0.90, 1.0];
                        accuracy = [0.01];
                        description = the maximum allowable correlation coefficient for a sample volume to be classified as Biological;

   max_Z_BI             value = 35;
                        name = Maximum BI Reflectivity;
                        type = int;
                        range = [30, 60];
                        description = the maximum allowable reflectivity for a sample volume to be classified as Biological;

   max_Zdr_DS           value = 2.0;
                        name = Maximum DS Diff. Reflectivity;
                        type = float;
                        unit = dB;
                        range = [0.0, 4.0];
                        accuracy = [0.1];
                        description = the maximum allowable differential reflectivity for a sample volume to be classified as Dry Snow;


   min_Agg              value = 0.4;
                        name = Minimum Aggregation; 
                        type = float;
                        range = [0.3, 0.7];
                        accuracy = [0.1];
                        description = the minimum value of the maximum weighted membership aggregation required to assign a HYDRO CLASS TYPE other than Unknown;

   min_Dif_Agg          value = 0.001;
                        name = Minimum Difference of Aggregation;
                        type = float;
                        range = [0.001, 0.005];
                        accuracy = [0.001];
                        description = the minimum difference between the maximum weighted membership aggregation and the next highest weighted membership aggregation in order to assign a HYDRO CLASS TYPE other than Unknown;

   min_snr              value = 5.0;
                        name = Minimum Signal-to-noise;
                        type = float;
                        unit = dB;
                        range = [0.0, 10.0];
                        accuracy = [0.1];
                        description = the minimum allowable signal-to-noise ratio in the horizaontal channel for a SAMPLE VOLUME to be classified as anything other than HYDRO CLASS (NE), in dB;

   atten_control        value = Off;
                        name = Attenuation Adjustment Control Flag;
                        type = string;
			range = {Off, On};
                        enum = 0, 1;
                        description = when set to Off, the reflectivity threhsold on BI classification is applied to all radials. When set to On, the reflectivity threshold on BI classification is applied only to highly attenuated radials.

   f1_a                 value = 0.000750;
                        name =  Coefficient a of equation f1;
                        type = float;
                        range = [-10.0, 10.0];
                        accuracy = [0.000001];
                        description = Coefficient a of equation f1;

  f1_b                  value = 0.0025;
                        name = Coefficient b of equation f1;
                        type = float;
                        range = [-10, 10];
                        accuracy = [0.0001];
                        description = Coefficient b of equation f1;

  f1_c                  value = -0.5;
                        name = Coefficient c of equation f1;
                        type = float;
                        range = [-10.0, 10.0];
                        accuracy = [0.1];
                        description = Coefficient c of equation f1;

  f2_a                  value = 0.00292;
                        name =  Coefficient a of equation f2;
                        type = float;
                        range = [-10.0, 10.0];
                        accuracy = [0.00001];
                        description = Coefficient a of equation f2;
  
  f2_b                  value = -0.0481;
                        name =  Coefficient b of equation f2;
                        type = float;
                        range = [-10.0, 10.0];
                        accuracy = [0.0001];
                        description = Coefficient b of equation f2;;

  f2_c                  value = 0.68;
                        name =  Coefficient c of equation f2;
                        type = float;
                        range = [-10.0, 10.0];
                        accuracy = [0.01];
                        description = Coefficient c of equation f2;


  f3_a                  value = 0.000485;
                        name =  Coefficient a of equation f3;
                        type = float;
                        range = [-10.0, 10.0];
                        accuracy = [0.000001];
                        description = Coefficient a of equation f3;

  f3_b                  value = 0.0667;
                        name =  Coefficient b of equation f3;
                        type = float;
                        range = [-10.0, 10.0];
                        accuracy = [0.0001];
                        description = Coefficient b of equation f3;

  f3_c                  value = 1.42;
                        name =  Coefficient c of equation f3;
                        type = float;
                        range = [-10.0, 10.0];
                        accuracy = [0.01];
                        description = Coefficient c of equation f3;

  g1_b                  value = 0.8;
                        name =  Coefficient b of equation g1;
                        type = float;
                        range = [-10.0, 10.0];
                        accuracy = [0.1];
                        description = Coefficient b of equation g1;

  g1_c                  value = -44.0;
                        name =  Coefficient c of equation g1;
                        type = float;
                        range = [-100.0, 100.0];
                        accuracy = [0.1];
                        description = Coefficient c of equation g1;

  g2_b                  value = 0.5;
                        name =  Coefficient b of equation g2;
                        type = float;
                        range = [-10.0, 10.0];
                        accuracy = [0.1];
                        description = Coefficient b of equation g2;

  g2_c                  value = -22.0;
                        name =  Coefficient c of equation g2;
                        type = float;
                        range = [-100.0, 100.0];
                        accuracy = [0.1];
                        description = Coefficient c of equation g2;

  weight_Z              value = 0.0, 0.0, 1.0, 1.0, 1.0, 0.8, 0.4, 0.2, 1.0, 0.6, 1.0, 0.8, 0.0, 0.0;
                        name = Reflectivity Weight Array;
                        type = float;
                        range = [0.0, 1.0];
                        accuracy = [0.1];
                        description = Weight assigned to Z for each Hydro Class Type;

  weight_Zdr            value = 0.0, 0.0, 0.8, 0.8, 0.8, 1.0, 0.6, 0.4, 0.8, 0.8, 0.6, 1.0, 0.0, 0.0;
                        name = Differential Reflectivity Weight Array;
                        type = float;
                        range = [0.0, 1.0];
                        accuracy = [0.1];
                        description = Weight assigned to Zdr for each Hydro Class Type;

  weight_RHOhv          value = 0.0, 0.0, 0.6, 0.6, 0.6,0.6, 1.0, 1.0, 0.6, 1.0, 0.4, 0.4, 0.0, 0.0;
                        name = Correlation Coefficent Weight Array;
                        type = float;
                        range = [0.0, 1.0];
                        accuracy = [0.1];
                        description = Weight assigned to RHOhv for each Hydro Class Type;

  weight_LKdp           value = 0.0, 0.0, 0.0, 1.0, 1.0,0.0, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0;
                        name = Log of Specific Differential Phase Weight Array;
                        type = float;
                        range = [0.0, 1.0];
                        accuracy = [0.1];
                        description = Weight assigned to LKdp for each Hydro Class Type;

  weight_SDZ            value = 0.0, 0.0, 0.2, 0.2, 0.2,0.2, 0.8,0.6, 0.2, 0.2, 0.2, 0.2, 0.0, 0.0;
                        name = Texture of Diff. Reflectivity Weight Array;
                        type = float;
                        range = [0.0, 1.0];
                        accuracy = [0.1];
                        description = Weight assigned to SDZ for each Hydro Class Type;

  weight_SDPHIdp        value = 0.0, 0.0, 0.2, 0.2, 0.2,0.2, 0.8,0.8, 0.2, 0.2, 0.2, 0.2, 0.0, 0.0;
                        name = Texture of Diff. Phase Shift Weight Array;
                        type = float;
                        range = [0.0, 1.0];
                        accuracy = [0.1];
                        description = Weight assigned to SDPHIdp for each Hydro Class Type;


  memRA                 value =	5.00, 10.00, 45.00, 50.00,
				-0.30, 0.00, 0.00, 0.50,
				-1.00, 0.00, 0.00, 1.00,
				0.95, 0.97, 1.00, 1.01,
				0.00, 0.50, 3.00, 6.00,
				0.00, 1.00, 15.00, 30.00;
                        name = Membership points for RA;
                        type = float;
                        range = [-100.0, 100.0];
                        accuracy = [0.01];
                        description = Membership defintion points for light and moderate rain;

  memHR                 value =	40.00, 45.00, 55.00, 60.00,
				-0.30, 0.00, 0.00, 0.50,
				-1.00, 0.00, 0.00, 1.00,
				0.92, 0.95, 1.00, 1.01,
				0.00, 0.50, 3.00, 6.00,
				0.00, 1.00, 15.00, 30.00;
                        name = Membership points for HR;
                        type = float;
                        range = [-100.0, 100.0];
                        accuracy = [0.01];
                        description = Membership definition points for heavy rain;

  memRH                 value =	45.00, 50.00, 75.00, 80.00,
				-0.3, 0.00, 0.00, 0.50,
				-10.00, -4.00, 0.00, 1.00,
				0.85, 0.90, 1.00, 1.01,
				0.00, 0.50, 3.00, 6.00,
				0.00, 1.00, 15.00, 30.00;
                        name = Membership points for RH;
                        type = float;
                        range = [-100.0, 100.0];
                        accuracy = [0.01];
                        description = Membership defintion points for rain and hail;

  memBD                 value =	10.00, 15.00, 45.00, 50.00,
				-0.30, 0.00, 0.00, 1.00,
				-1.00, 0.00, 0.00, 1.00,
				0.92, 0.95, 1.00, 1.01,
				0.00, 0.50, 3.00, 6.00,
				0.00, 1.00, 15.00, 30.00;
                        name = Membership points for BD; 
                        type = float;
                        range = [-100.0, 100.0];
                        accuracy = [0.01];
                        description = Membership definition points for big drops;

  memBI                 value =	5.00, 10.00, 20.00, 30.00,
				0.00, 0.00, 10.00, 12.00,
				-30.00, -25.00, 10.00, 20.00,
				0.30, 0.50, 0.85, 0.90,
				1.00, 2.00, 4.00, 7.00,
				8.00, 10.00, 40.00, 60.00;
                        name = Membership points for BI;
                        type = float;
                        range = [-100.0, 100.0];
                        accuracy = [0.01];
                        description = Membership definition points for biological;

  memGC                 value =	15.00, 20.00, 70.00, 80.00,
				-4.00, -2.00, 1.00, 2.00,
				-30.00, -25.00, 10.00, 20.00,
				0.50, 0.60, 0.90, 0.95,
				2.00, 4.00, 10.00, 15.00,
				30.00, 40.00, 50.00, 60.00;
                        name = Membership points for GC;
                        type = float;
                        range = [-100.0, 100.0];
                        accuracy = [0.01];
                        description = Membership definition points for ground clutter;

  memDS                 value = 5.00, 10.00, 35.00, 40.00,
				-0.30, 0.00, 1.30, 1.60,
				-30.00, -25.00, 10.00, 20.00,
				0.95, 0.98, 1.00, 1.01,
				0.00, 0.50, 3.00, 6.00,
				0.00, 1.00, 15.00, 30.00;
                        name = Membership points for DS;
                        type = float;
                        range = [-100.0, 100.0];
                        accuracy = [0.01];
                        description = Membership definition points for Dry Snow;

  memWS                 value =	25.000, 30.000, 40.000, 50.000,
				0.500, 1.000, 2.000, 3.000,
				-30.000, -25.000, 10.000, 20.000,
				0.880, 0.920, 0.950, 0.985,
				0.000, 0.500, 3.000, 6.000,
				0.000, 1.000, 15.000, 30.000;
                        name = Membership points for WS;
                        type = float;
                        range = [-100.0, 100.0];
                        accuracy = [0.001];
                        description = Membership definition points for wet snow;

  memIC                 value =	0.00, 5.00, 20.00, 25.00,
				0.10, 0.40, 3.00, 3.30,
				-5.00, 0.00, 10.00, 15.00,
				0.95, 0.98, 1.00, 1.01,
				0.00, 0.50, 3.00, 6.00,
				0.00, 1.00, 15.00, 30.00;
                        name = Membership points for IC;
                        type = float;
                        range = [-100.0, 100.0];
                        accuracy = [0.01];
                        description = Membership definition points for ice crystals;

  memGR                 value =	25.00, 35.00, 50.00, 55.00,
				-0.30, 0.00, 0.00, 0.30,
				-30.00, -25.00, 10.00, 20.00,
				0.90, 0.97, 1.00, 1.01,
				0.00, 0.50, 3.00, 6.00,
				0.00, 1.00, 15.00, 30.00;
                        name = Membership points for GR;
                        type = float;
                        range = [-100.0, 100.0];
                        accuracy = [0.01];
                        description = Membership definition points for graupel;

   memFlagRA           value =	none,none,none,none,
				f1, f1, f2, f2,
				g1, g1, g2, g2,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none;
                       range = {none, f1, f2, f3, g1, g2};
                       enum = 0, 1, 2, 3, 4, 5;
                       name = 2-dimensional membership flag for RA; 
                       type = string;
   
   memFlagHR           value =	none,none,none,none,
				f1, f1, f2, f2,
				g1, g1, g2, g2,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none;
                       range = {none, f1, f2, f3, g1, g2};
                       enum = 0, 1, 2, 3, 4, 5;
                       name = 2-dimensional membership flag for HR; 
                       type = string;
   
   memFlagRH           value =	none,none,none,none,
				none,none, f1, f1,
				none,none, g1, g1,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none;
                       range = {none, f1, f2, f3, g1, g2};
                       enum = 0, 1, 2, 3, 4, 5;
                       name = 2-dimensional membership flag for RH;
                       type = string;

   memFlagBD           value =	none,none,none,none,
				f2, f2, f3, f3,
				g1, g1, g2, g2,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none;
                       range = {none, f1, f2, f3, g1, g2};
                       enum = 0, 1, 2, 3, 4, 5;
                       name = 2-dimensional membership flag for BD; 
                       type = string;

   memFlagBI           value =	none,none,none,none,
				none,f3,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none;
                       range = {none, f1, f2, f3, g1, g2};
                       enum = 0, 1, 2, 3, 4, 5;
                       name = 2-dimensional membership flag for BI; 
                       type = string;

  memFlagGC            value =	none,none,none,none,
		 		none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none;
                       range = {none, f1, f2, f3, g1, g2};
                       enum = 0, 1, 2, 3, 4, 5;
                       name = 2-dimensional membership flag for GC; 
                       type = string;

  memFlagDS            value =	none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none;
                       range = {none, f1, f2, f3, g1, g2};
                       enum = 0, 1, 2, 3, 4, 5;
                       name =  2-dimensional membership flag for DS; 
                       type = string;

  memFlagWS            value =	none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none;
                       range = {none, f1, f2, f3, g1, g2};
                       enum = 0, 1, 2, 3, 4, 5;
                       name = 2-dimensional membership flag for WS; 
                       type = string;

  memFlagIC            value =	none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none;
                       range = {none, f1, f2, f3, g1, g2};
                       enum = 0, 1, 2, 3, 4, 5;
                       name = 2-dimensional membership flag for IC; 
                       type = string;

 memFlagGR             value =	none,none,none,none,
				none,none, f1, f1,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none,
				none,none,none,none;
                       range = {none, f1, f2, f3, g1, g2};
                       enum = 0, 1, 2, 3, 4, 5;
                       name = 2-dimensional membership flag for GR; 
                       type = string;

