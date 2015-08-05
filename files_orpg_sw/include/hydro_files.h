/*  
	This file defines variables for several hydromet related algorithms.   	
*/
#include <terrain_blockage.h>

/*
  Beam Blockage Algorithm
*/

/*  Product elevation angle limits (in tenth of a degree). 	*/
#define MIN_ELEV		-10 
#define MAX_ELEV		200 

/*  Beam weight array size.					*/
#define	BWT_ARRAY_SIZE		31

/*  Beam weight table parameters.			 	*/
#define	HORIZ_SHAPE		-8.0	/*  Horizontal beam shape factor. 	*/
#define	VERT_SHAPE		-4.0	/*  Vertical beam shape factor.		*/
#define	BEAM_WIDTH		0.9	/*  Half power beam width (degrees).	*/

/*  Beam propagation information	*/ 
#define	EARTH_RADIUS		6371 	/*  Earth radius (kilometers).		*/
#define REFRACTIVE_INDEX	1.21	/*  Refractive index factor.		*/

/*  Terrain and beam blockage data definitions.	*/
#define ORPGDAT_TERRAIN_DAT     3333 

#define MAX_AZIMS         3600
#define MAX_RANGE          230

typedef struct terrain_data {
   short height[MAX_AZIMS][MAX_RANGE];
} terrain_data_t;

