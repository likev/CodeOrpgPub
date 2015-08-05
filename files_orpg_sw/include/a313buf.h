/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2008/01/07 23:19:36 $
 * $Id: a313buf.h,v 1.5 2008/01/07 23:19:36 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef A313BUF_H
#define A313BUF_H

/**********************************************************
*                VIL/ECHO TOPS COMMON                     *
*                   PARAMETER FILE                        *
*                       A313P0                            *
*                                                         *
**********************************************************/
/*
  MAXBINS = The maximum number of bins to be processed.
  NCOL = The number of analysis box columns, to the
         nearest even integer value.
  NROW = The number of analysis box rows, to the nearest
         even integer value.
  BOXHGHT = The height (y-direction) of a VIL/ECHO TOPS
            analysis BOX, in km.
  BOXWDTH = The width (x-direction) of a VIL/ECHO TOPS
            analysis BOX, in km.
  MAXRNG = Maximum range of the VIL/ECHO TOPS analysis, in km.
  NUMW = The number of fullwords in the
         bitmap (OVERLAY) that checks for the echo top
         found at the maximum elevation.
*/
#define MAXBINS 		460 
#define MAXRNG			230 
#define BOXHGHT 		4 
#define BOXWDTH			4
#define NCOL			(((MAXRNG+(BOXWDTH-1))/BOXWDTH)*2 )
#define NROW			(((MAXRNG+(BOXHGHT-1))/BOXHGHT)*2 )
#define NUMW			(((NCOL*NROW)+31)/32 )
#define HALFCOL 		(NCOL*0.5) 
#define HALFROW			(NROW*0.5)
#define SHIFTFAC		65536
#define RSS 			1000
#define RSSKM			1.0
#define GRID_CENTER_I 		((HALFCOL+1)*SHIFTFAC )
#define GRID_CENTER_J 		((HALFROW+1)*SHIFTFAC )

/*
 
  Common supplimental array positional parameters
 
  ONCO = The pointer to the number of columns in both the VIL
         and Echo Tops supplimental variable arrays.
  ONRO = The pointer to the number of rows in both the VIL
         and Echo Tops supplimental variable arrays.
  OMSR = The pointer to the maximum analysis slant range
         in both the VIL and Echo Tops supplimental variable
         arrays.
  ONZE = The pointer to the minimum reflectivity threshold in
         both the VIL and Echo Tops supplimental varaible
         arrays.
*/
#define ONCO			1
#define ONRO			2
#define OMSR			3
#define ONZE			4

/**********************************************************
*                  ECHO TOPS OUTPUT                       *
*                     PARAMETERS                          *
*                                                         *
**********************************************************/
/*
  Dimensional Parameters
 
  NETCOL = The number of columns in the Echo tops product.
  NETROW = The number of rows in the Echo Tops product.
  NETSIZ = The size of the Echo Tops supplemental variable
           array.
*/
#define NETCOL			NCOL
#define NETROW			NROW
#define NETSIZ			7 

/*
 
  Offset Parameters
 
  OSET = The offset to the array of parameters describing
         the size, maximum value and thresholds of the
         Echo Tops product.
  OVET = The offset to the array of Echo Tops values.
  OBIT = The offset to the maximum Echo Top/last elevation
         identifier Bit Map.
*/
 
#define OSET			0 
#define OBIT			NETSIZ
#define OVET			(NUMW+NETSIZ)

/*
  Local supplimental array positional parameters
 
  OMET = The pointer into the Echo Tops supplimental variable
         array containing the maximum echo top value.
  OMETC = The pointer into the Echo Tops supplimental variable
          array containing the maximum echo top column position.
  OMETR = The pointer into the Echo Tops supplimental variable
          array containing the maximum echo top row position.
*/
#define OMET			5 
#define OMETC			6 
#define OMETR			7 


/*********************************************************
                     ECHO TOPS OUTPUT                    *
                     FILE STRUCTURE                      *
                                                         *
*********************************************************/
typedef struct {
   int ncols;        /* Number of cols */
   int nrows;        /* Number of rows */
   int max_rng;      /* Max analysis slant range (km) */
   int min_refl;     /* Min reflectivity threshold (dBZ * 10) */
   int max_ht;       /* Max echo top value (kft) */
   int max_col_posn; /* Max echo top column position */
   int max_row_posn; /* Max echo top row position */
   int overlay[NUMW]; /* Bit map of echo tops found at max elev angle */
   short etval[NETROW][NETCOL]; /* Echo top values */
} Echo_top_params_t;



/*********************************************************
                     VIL OUTPUT                          *
                     PARAMETERS                          *
                                                         *
*********************************************************/
/*
  Dimensional Parameters
 
  NVILCOL = The number of columns in the VIL product.
  NVILROW = The number of rows in the VIL product.
  NVILSIZ = The size of the VIL supplemental variable array.
 
*/
#define NVILCOL			NCOL
#define NVILROW			NROW
#define NVILSIZ			8 

/*
  Offset Parameters
 
  OSVI = The offset to an array describing the size,
         the maximum and thresholds of the VIL product.
  OVVI = The offset to the array of VIL values.
 
*/
#define OSVI			0
#define OVVI			NVILSIZ 
 
/*
  Local supplimental array positional parameters
 
  OMVT = The pointer into the VIL supplimental variable
         array containing the maximum VIL threshold value.
  OMVI = The pointer into the VIL supplimental variable
         array containing the maximum VIL value.
  OMVIC = The pointer into the VIL supplimental variable
          array containing the maximum VIL column position.
  OMVIR = The pointer into the VIL supplimental variable
          array containing the maximum VIL row position.
*/
 
#define OMVT			5 
#define OMVI			6 
#define OMVIC			7 
#define OMVIR			8 

#endif
