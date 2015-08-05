/***********************************************************************

	This file defines the macros that are needed by the ported RPG
	tasks.

	The contents in this file are derived from a309.inc. The macros 
	must be consistent with those defined there. Thus if a309.inc is 
	modified, this file has to be updated accordingly.

***********************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/06/27 14:05:35 $
 * $Id: a309.h,v 1.72 2014/06/27 14:05:35 steves Exp $
 * $Revision: 1.72 $
 * $State: Exp $
 *
 */

#ifndef A309_H
#define A309_H

/* maximum product generation control message size */
#define PI_N_REQUESTS		20

/* ranges of legacy product ids and associated buffer numbers */
#define LEGACY_MAX_BUFFERNUM 	130
#define LEGACY_MAX_PCODE	100

/* ranges of all product ids and associated buffer numbers */
#define PI_MAX_BUFFERNUM 	ORPGPAT_MAX_PRODUCT_CODE
#define PI_MAX_PCODE		ORPGPAT_MAX_PRODUCT_CODE

/* Keywords for task data availability */
#define INPUT_AVAILABLE          20
#define START_OF_VOLUME_SCAN     21

/* Keywords for CPC2MSG */
#define ALERTPROD                 3
#define CFC_PROD_UNAVAIL          4

/* Keywords for REL_OUTBUF */
#define FORWARD                 100
#define DESTROY                 101

/* Keywords for extended arguments for REL_OUTBUF. */
#define RPG_EXTEND_ARGS         0x10000
#define RPGC_EXTEND_ARGS        RPG_EXTEND_ARGS
#define DISPOSITION_MASK         0xffff
#define EXTENDED_ARGS_MASK   0x7fff0000

/* Keywords for GET_INBUF and GET_OUTBUF */
#define NORMAL      		0
#define TERMINATE   		1
#define NOT_REQD    		1
#define NO_DATA     		2
#define NO_BLOCKS   		3
#define NO_MEM      		4
#define FAULT_ME   		99 
#define RPGC_NORMAL      	NORMAL
#define RPGC_TERMINATE		TERMINATE
#define RPGC_NOT_REQD		NOT_REQD
#define	RPGC_NO_DATA		NO_DATA
#define RPGC_NO_BLOCKS		NO_BLOCKS
#define RPGC_NO_MEM 		NO_MEM
#define RPGC_FAULT_ME		FAULT_ME

/* Buffer Data Content Identifiers */
#define ANY_TYPE  0
#define NO_TYPE   0

/* bits for free text message destinations */
#define FTMD_RDA	(1 << 15)	/* message to RDA */
#define FTMD_RPG	(1 << 14)	/* message to RPG console */
#define FTMD_ALL	(1 << 13)	/* message to all */
#define FTMD_ALL_NB_USERS	(1 << 12)	/* message to all NB users */

#define FTMD_CLASS_5	(1 << 5)	/* message to class 5 */
#define FTMD_CLASS_4	(1 << 4)	/* message to class 4 */
#define FTMD_CLASS_3	(1 << 3)	/* message to class 3 */
#define FTMD_CLASS_2	(1 << 2)	/* message to class 2 */
#define FTMD_CLASS_1	(1 << 1)	/* message to class 1 */

/* product dependent parameters in product description block for free text 
   messages */
#define FTMD_LINE_SPEC_SIZE 7	/* table size of A309_ftmd_line_spec */
#define FTMD_TYPE_SPEC 50	/* location of the parameter used for free text 
				   message destination type specifications */
#ifdef FTMD_NEED_TABLE
static int A309_ftmd_line_spec[FTMD_LINE_SPEC_SIZE] = 
	{26, 27, 29, 46, 47, 48, 49};
				/* location of the parameters used for free text 
				   message destination line specifications */
#endif

/*                                                              
*  DESCRIPTION OF RADIAL STATUS FLAG VARIABLE       
*  NAMES: INCLUDING RADIAL EVALUATION (GOOD OR BAD)
*                                                             
*       GOODBEL - GOOD BEGINNING OF ELEVATION CUT  
*       BADBEL - BAD BEGINNING OF ELEVATION CUT    
*       GOODBVOL - GOOD BEGINNING OF VOLUME SCAN   
*       BADBVOL - BAD BEGINNING OF VOLUME SCAN     
*       GENDEL - GOOD END OF ELEVATION CUT         
*       BENDEL - BAD END OF ELEVATION CUT          
*       GENDVOL - GOOD END OF VOLUME SCAN          
*       BENDVOL - BAD END OF VOLUME SCAN           
*       GOODINT - GOOD INTERMEDIATE RADIAL         
*       BADINT - BAD INTERMEDIATE RADIAL           
*       PGENDEL - PSUEDO GOOD END OF ELEVATION CUT 
*       PGENDVOL - PSUEDO GOOD END OF VOLUME SCAN  
*       PBENDEL - PSUEDO BAD END OF ELEVATION CUT  
*       PBENDVOL - PSUEDO BAD END OF VOLUME SCAN   
*       GOODTHRLO - VALUE OF LOWEST GOOD STATUS    
*       GOODTHRHI - VALUE OF HIGHEST GOOD STATUS   
*
*       GOODBELLC - GOOD BEGINNING OF ELEVATION CUT,
*       BADBELLC - BAD BEGINNING OF ELEVATION CUT,
*                  LAST CUT IN VOLUME SCAN
*                                                              
*       DESCRIPTION OF RADIAL SATUS FLAG VARIABLE       
*       NAMES: STATUS (I.E. SEQUENCE IN SCAN) ONLY      
*              BEG_ELEV - BEGINNING OF ELEVATION SCAN     
*              INT_ELEV - WITHIN ELEVATION SCAN           
*              END_ELEV - END OF ELEVATION SCAN           
*              BEG_VOL  - BEGINNING OF VOLUME SCAN        
*              END_VOL  - END OF VOLUME SCAN              
*              PSEND_ELEV - PSEUDO END OF ELEVATION SCAN  
*              PSEND_VOL  - PSEUDO END OF VOLUME SCAN     
*/

#define GOODBEL     0x00
#define BADBEL      0x80
#define GOODBVOL    0x03
#define BADBVOL     0x83
#define GENDEL      0x02
#define BENDEL      0x82
#define GENDVOL     0x04
#define BENDVOL     0x84
#define GOODBELLC   0x05
#define BADBELLC    0x85
#define GOODINT     0x01
#define BADINT      0x81
#define PGENDEL     0x08
#define PGENDVOL    0x09
#define PBENDEL     0x88
#define PBENDVOL    0x89
#define GOODTHRLO   0x00
#define GOODTHRHI   0x09

/*
 * Radial Status flags
 */
#define BEG_ELEV    0x00
#define INT_ELEV    0x01
#define END_ELEV    0x02
#define BEG_VOL     0x03
#define END_VOL     0x04
#define BEG_ELEV_LC 0x05
#define PSEND_ELEV  0x08
#define PSEND_VOL   0x09


/*
*   Task Abort reason codes and Product Unavailability reasons
*
*   PROD_MEM_SHED         ;PRODUCTS LOST TO MEMORY SHEDDING
*   PROD_CPU_SHED         ;PRODUCTS LOST TO CPU SHEDDING
*   PROD_TASK_FAILED      ;PRODUCTS LOST TO FAILED TASK
*   PROD_TASK_NOT_LOADED  ;PRODUCT TASKS NOT LOADED
*   PROD_CUST_MEM_SHED    ;CUSTOMIZED PROD MEMORY SHED
*   PROD_DISABLED_MOMENT  ;MOMENT DISABLED PRODUCT
*/

#define PROD_MEM_SHED          8
#define PROD_CPU_SHED          9
#define PROD_TASK_FAILED      10
#define PROD_TASK_NOT_LOADED  11
#define PROD_CUST_MEM_SHED    12
#define PROD_DISABLED_MOMENT  13


/* RPG Buffer types */

#define ALRTMSG		 	1 
#define BREF19	 		2 
#define BREF16 			3 
#define BREF20 			4 
#define BREF17 			5 
#define BREF21 			6 
#define BREF18 			7 
#define BSPC28 			8 
#define BSPC29 			9 
#define BSPC30 			10 
#define BVEL25 			11 
#define BVEL22 			12 
#define BVEL26 			13 
#define BVEL23 			14 
#define BVEL27 			15 
#define BVEL24 			16 
#define BASEVGD 		20 
#define COMBATTR 		21 
#define CENTATTR 		22 
#define CRP37 			23 
#define CRP35 			24 
#define CRP38 			25 
#define CRP36 			26 
#define SEGATTR			27
#define ETPRODD 		29 
#define ETTAB 			30 
#define FTXTMSG 		32 
#define HAILCAT 		33 
#define TRENDATR 		34
#define HPLOTS 			35 
#define RFAVLYR1 		36 
#define RFAVLYR2 		37 
#define RFMXLYR1 		38 
#define RFMXLYR2 		39 
#define MES2DATR 		40 
#define MESOATTR 		41 
#define MESOPROD 		42 
#define ALRTPROD 		43 
#define RMXAPLYR 		44 
#define RMXAPPG 		45 
#define TDA1DATR 		46 
#define HAILATTR 		48 
#define STRUCDAT 		49 
#define TRFRCATR 		50 
#define STMTRDAT 		51 
#define PRCIPMSG 		52 
#define RECOMBINED_BASEDATA 	53 
#define RAWDATA 		54 
#define BASEDATA 		55 
#define HYUSPACC 		56
#define HYBRDREF 		57
#define HYBRGREF 		58
#define TVSATTR 		65 
#define REFL_RAWDATA		66
#define COMB_RAWDATA		67
#define SRMRVMAP 		68 
#define SRMRVREG 		69 
#define CPC2MSG  		70 
#define SR_BASEDATA		76
#define SR_COMBBASE 		77
#define SR_REFLDATA 		78 
#define REFLDATA 		79 
#define TVSPROD 		80 
#define VADPARAM 		81 
#define VCS53 			83 
#define VCS52 			84 
#define VILPROD 		85 
#define VILTABL 		86 
#define ITWSDBV 		87 
#define SCRATCH 		89 
#define CRPG 			91 
#define CRCG230 		92 
#define CRCG460 		93 
#define BREF8BIT  		94 
#define COMBBASE 		96 
#define VADTMHGT 		97 
#define CPC10MSG 		98 
#define BVEL8BIT 		99 
#define HYBRSCAN   		101 
#define HYUSPBUF   		102
#define HYACCSCN   		103 
#define HYADJSCN   		104 
#define HY1HRACC   		105 
#define HY3HRACC   		106 
#define HYSTMTOT   		107 
#define HY1HRDIG   		108 
#define HYSUPPLE   		109 
#define VADVER2    		110 
#define VCSR8      		111
#define VCSV8      		112 
#define RFAVLYR3   		116 
#define RFMXLYR3   		117 
#define CFCPROD    		119
#define RECCLDIGREFTAB 		120 
#define RECCLDIGDOPTAB 		121 
#define CRPGAPE    		122
#define CRPAPE97   		123
#define CRPAPE95   		124
#define CRPAPE98   		125
#define CRPAPE96   		126
#define PRFOVLY			127
#define POSEDRCM 		129
#define RADARMSG 130   /*  NOTE:  Reserve this value in the event this product is 
                                  ever resurrected. */
#define RECCLREF    		132
#define RECCLDOP    		133
#define HYDIGSTM    		138
#define MESORUPROD  		139
#define MDAPROD     		141
#define OSWACCUM    		144
#define OSDACCUM    		145
#define SSWACCUM    		146
#define SSDACCUM    		147
#define USWACCUM    		150
#define USDACCUM    		151
#define STATPROD    		152
#define NTDA_EDR                156
#define NTDA_CONF               157
#define SAAACCUM    		280
#define TVSATR_RU   		290
#define MDA1D        		292
#define MDA2D       		293
#define MDA3D       		294
#define MDATTNN     		295
#define MESORUATTR  		296
#define DQA         		297
#define RECCLDIGREF 		298
#define RECCLDIGDOP 		299
#define BASEDATA_ELEV  		301
#define REFLDATA_ELEV  		302
#define COMBBASE_ELEV  		303
#define PRFSEL         		304
#define DUALPOL_BASEDATA	305
#define DUALPOL_COMBBASE	306
#define DUALPOL_REFLDATA	307
#define SR_BASEDATA_ELEV	308
#define SR_REFLDATA_ELEV	309
#define SR_COMBBASE_ELEV	310
#define NTDA_EDR_IP             315
#define NTDA_CONF_IP            316
#define FILTREFL		400




/* 
  USE: PARAMETER FILE CONTAINING PARAMETERIZED   
       VALUES DEFINING OFFSETS INTO THE PRODUCT   
       HEADER FOR PRODUCTS
                                                              
       DESCRIPTION OF OFFSET POSITIONS DEFINED BY      
       THESE PARAMETERS:                            
                                                              
          PRODUCT HEADER OFFSET DESCRIPTIONS:          
                                                             
            MESCDOFF - MESSAGE CODE                    
            DTMESOFF - DATE OF MESSAGE                 
            TMSWOFF - TIME OF MESSAGE (MSW)            
            TLSWOFF - TIME OF MESSAGE (LSW)            
            LGMSWOFF - LENGTH OF MESSAGE (MSW)            
            LGLSWOFF - LENGTH OF MESSAGE (LSW)            
            SRCIDOFF - SOURCE ID-NUMBER               
            DSTIDOFF - DESTINATION ID-NUMBER           
            NBLKSOFF - NUMBER OF BLOCKS IN PRODUCT     
            DIV1OFF - FIRST DIVIDER                    
            LTMSWOFF - LATITUDE OF RADAR (MSW)         
            LTLSWOFF - LATITUDE OF RADAR (LSW)         
            LNMSWOFF - LONGITUDE OF RADAR (MSW)        
            LNLSWOFF - LONGITUDE OF RADAR (LSW)        
            RADHGTOFF - HEIGHT OF RADAR                
            PRDCODOFF - PRODUCT CODE                   
            WTMODOFF - WEATHER MODE                    
            VCPOFF   - VOLUME COVERAGE PATTERN         
            SQNUMOFF - SEQUENCE NUMBER                 
            VSNUMOFF - VOLUME SCAN NUMBER              
            VSDATOFF - VOLUME SCAN DATE                
            VSTMSWOFF - VOLUME SCAN TIME (MSW)         
            VSTLSWOFF - VOLUME SCAN TIME (LSW)         
            GDPRDOFF - GENERATION DATE OF PRODUCT      
            GTMSWOFF - GENERATION TIME OF PRODUCT (MSW)                     
            GTLSWOFF - GENERATION TIME OF PRODUCT (LSW)                     
            AZWINOFF - AZIMUTH OF WINDOW CENTER        
            RNWINOFF - RANGE OF WINDOW CENTER          
            ELINDOFF - ELEVATION INDEX                 
            EAZALOFF - ELEVATION, AZIMUTH, OR ALTITUDE 
            DL1OFF - DATA LEVEL 1                      
            DL2OFF - DATA LEVEL 2                      
            DL3OFF - DATA LEVEL 3                      
            DL4OFF - DATA LEVEL 4                      
            DL5OFF - DATA LEVEL 5                      
            DL6OFF - DATA LEVEL 6                      
            DL7OFF - DATA LEVEL 7                      
            DL8OFF - DATA LEVEL 8                      
            DL9OFF - DATA LEVEL 9                      
            DL10OFF - DATA LEVEL 10                    
            DL11OFF - DATA LEVEL 11                    
            DL12OFF - DATA LEVEL 12                    
            DL13OFF - DATA LEVEL 13                    
            DL14OFF - DATA LEVEL 14                    
            DL15OFF - DATA LEVEL 15                    
            DL16OFF - DATA LEVEL 16                    
            MDL1OFF - MAXIMUM DATA LEVEL 1             
            MDL2OFF - MAXIMUM DATA LEVEL 2             
            MDL3OFF - MAXIMUM DATA LEVEL 3             
            MDL4OFF - MAXIMUM DATA LEVEL 4             
            STSPDOFF - STORM SPEED                     
            STDIROFF - STORM DIRECTION                 
            CALCONMSW- OFFSET TO CALIBRATION CONS.(MSW)
            CALCONLSW- OFFSET TO CALIBRATION CONS.(LSW)
            CNTINTOFF - CONTOUR INTERVAL               
            NMAPSOFF - NUMBER OF MAPS                  
            OPRMSWOFF - OFFSET TO PRODUCT (MSW)        
            OPRLSWOFF - OFFSET TO PRODUCT (LSW)        
            OGMSWOFF - OFFSET TO G. ATTRIBUTES (MSW)   
            OGLSWOFF - OFFSET TO G. ATTRIBUTES (LSW)   
            OTADMSWOFF - OFFSET TO TABULAR/ADAPT (MSW) 
            OTADLSWOFF - OFFSET TO TABULAR/ADAPT (LSW) 
                                                            
         PRODUCT BLOCK OFFSETS                        
                                                              
            DIV2OFF - SECOND DIVIDER                   
            BLOCKIDOFF - PRODUCT BLOCK ID              
            LRMSWOFF - LENGTH OF PRODUCT BLOCK PORTION 
                       OF PRODUCT (MSW)              
            LRLSWOFF - LENGTH OF PRODUCT BLOCK PORTION 
                       OF PRODUCT (LSW)              
            NLYROFF - NUMBER OF LAYERS                 
            LYRDIVOFF - LAYER DIVIDER                  
            LYRLMSWOFF - LAYER LENGTH (MSW)            
            LYRLLSWOFF - LAYER LENGTH )LSW)            
                                                              
         ADDITIONAL PARMETERS:                        
                                                              
            PHEADLNG - NUMBER OF I*2 WORDS IN          
                       PRODUCT HEADER (ITS LENGTH)     
            PCODSTR - START OF PRODUCT                 
            BLKOVRHD-BYTES OF OVERHEAD IN HEADER BLOCKS                            
*/

#define MESCDOFF    0
#define DTMESOFF    1
#define TMSWOFF     2
#define TLSWOFF     3
#define LGMSWOFF    4
#define LGLSWOFF    5
#define SRCIDOFF    6
#define DSTIDOFF    7
#define NBLKSOFF    8
#define DIV1OFF     9
#define LTMSWOFF   10
#define LTLSWOFF   11
#define LNMSWOFF   12
#define LNLSWOFF   13
#define RADHGTOFF  14
#define PRDCODOFF  15
#define WTMODOFF   16
#define VCPOFF     17
#define SQNUMOFF   18
#define VSNUMOFF   19

#define VSDATOFF   20
#define VSTMSWOFF  21
#define VSTLSWOFF  22
#define GDPRDOFF   23
#define GTMSWOFF   24
#define GTLSWOFF   25
#define AZWINOFF   26
#define RNWINOFF   27
#define ELINDOFF   28
#define EAZALOFF   29
#define DL1OFF     30
#define DL2OFF     31
#define DL3OFF     32
#define DL4OFF     33
#define DL5OFF     34
#define DL6OFF     35
#define DL7OFF     36
#define DL8OFF     37
#define DL9OFF     38
#define DL10OFF    39

#define DL11OFF    40
#define DL12OFF    41
#define DL13OFF    42
#define DL14OFF    43
#define DL15OFF    44
#define DL16OFF    45
#define MDL1OFF    46
#define MDL2OFF    47
#define MDL3OFF    48
#define MDL4OFF    49
#define STSPDOFF   50
#define STDIROFF   51
#define CNTINTOFF  52
#define NMAPSOFF   53
#define OPRMSWOFF  54
#define OPRLSWOFF  55
#define OGMSWOFF   56
#define OGLSWOFF   57
#define OTADMSWOFF 58
#define OTADLSWOFF 59
#define DIV2OFF    60
#define BLOCKIDOFF 61
#define LRMSWOFF   62
#define LRLSWOFF   63
#define CALCONMSW  50
#define CALCONLSW  51

#define NLYROFF    64
#define LYRDIVOFF  65
#define LYRLMSWOFF 66
#define LYRLLSWOFF 67

#define PHEADLNG   60
#define PCODSTR    69
#define BLKOVRHD   16


/* scan summary array offsets */
#define SCAN_PARAMS   		12
#define SCAN_DATE     		1
#define SCAN_TIME     		2
#define SCAN_MODE     		3
#define SCAN_VCP      		4
#define SCAN_NUM_ELV  		5
#define SCAN_SB       		6
#define SCAN_SUPER_RES		7

/*
 * Rain Gage Database constants to match a309.inc ...
 *
 * Sizes of tables ...
 * Index of items within 'gage_id' table ...
 */
#define GAGE_ITEMS	7
#define MAXGAGES	50
#define PASSITMS	2

#define GAGEID1		1
#define GAGEID2		2
#define GAGELAT		3
#define GAGELON		4
#define GAGEAZ		5
#define GAGERNG		6
#define GAGERPT		7

#define NREP_PARMS	6
#define NREP_GAGE	50
#define NGAGES_USER	50

/*
 * Status Change Notification (list may not be complete)
 * 
 * NOTE: SCN_NUMBER/100 <= SCN_MAX
 */
#define STATUS_TASK "STATMON "
#define SCN_MAX 35
   /*
    * RPG Software
    */
#define CNNEWSTATE 1000		/* Change in RPG state */
#define CNWEATHERMODE 1100	/* Change in Weather Mode */
#define CNTASKFAIL 1200		/* RPG task failure */
#define CNREDMSG 2600		/* Redundant Configuration Messages */
#define CNPRECIPCAT 3200	/* Precipitation category SCN */

/*
 * Define weather modes 
 *
 */
#define MAINTENANCE_MODE        0
#define TEST_MODE               0
#define CLEAR_AIR_MODE          1
#define PRECIPITATION_MODE      2

/*
 * Global Scaling Parameters
 */
#ifndef FT_TO_KM
#define FT_TO_KM  	0.0003048
#endif
#ifndef FT_TO_M
#define FT_TO_M   	0.30479999
#endif
#ifndef IN_TO_MM
#define IN_TO_MM  	25.4
#endif
#ifndef KM_TO_FT
#define KM_TO_FT  	3280.84
#endif
#ifndef KM_TO_KFT
#define KM_TO_KFT 	3.28084
#endif
#ifndef KM_TO_NM
#define KM_TO_NM  	0.53996
#endif
#ifndef KTS_TO_MPS
#define KTS_TO_MPS  	0.5144562
#endif
#ifndef M_TO_FT
#define M_TO_FT  	3.28084
#endif
#ifndef M_TO_KFT
#define M_TO_KFT  	0.00328084
#endif
#ifndef MM_TO_IN
#define MM_TO_IN  	0.03937008
#endif
#ifndef MPS_TO_KTS
#define MPS_TO_KTS  	1.9438
#endif
#ifndef NM_TO_KM
#define NM_TO_KM  	1.852
#endif
#ifndef KM_TO_NM
#define KM_TO_NM	0.53996
#endif
#ifndef NM_TO_M
#define NM_TO_M   	1852.0
#endif
#ifndef N_TO_KM
#define M_TO_KM  	0.001
#endif
#ifndef ONE_RADIAN
#define ONE_RADIAN  	0.01745329
#define DEGTORAD    	ONE_RADIAN
#endif
#ifndef PI_CONST
#define PI_CONST  	3.141593
#endif
#ifndef ZERO
#define ZERO  		0
#endif
#ifndef HALF
#define HALF  		0.5
#endif
#ifndef ONE
#define ONE  		1
#endif
#ifndef TWO
#define TWO  		2
#endif
#ifndef NOT_MAPPED 
#define NOT_MAPPED 	-1
#endif

#endif 		/* #ifndef A309_H */
