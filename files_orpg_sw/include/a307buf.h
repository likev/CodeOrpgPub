/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/07/13 15:31:29 $
 * $Id: a307buf.h,v 1.5 2007/07/13 15:31:29 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef A307BUF_H
#define A307BUF_H

/********************************************************************
   DIMENSIONS OF THE COMPOSITE REFLECTIVITY POLAR GRID:
********************************************************************/
#define NRADS			360
#define NBINS			460
 
/********************************************************************
   PARAMETERS USED AS OFFSETS TO HEADER & GRID IN THE POLAR-GRID-BUFFER.
   NOTE: OFFSETS REPRESENT I*4 WORDS!!!
      PHDR    - OFFSET TO 1ST WORD IN POLAR GRID BUFFER HEADER
      PGRID   - OFFSET TO 1ST WORD IN POLAR GRID
 
*********************************************************************/
#define PHDR			0
#define PGRID			1
 
/* Structure definition for Composite Reflectivity Polar Grid. */
typedef struct crpg {

   float calib_const;

   short polgrid[NRADS][NBINS];

} Crpg_t;

/********************************************************************
   PARAMETERS FILE FOR COMPOSITE REFLECTIVITY CARTESIAN GRID
   SCRATCH BUFFER HEADER:
 
   NOTE: THE HEADER IS WRITTEN IN I*4 WORDS!!!
 
     CHDR        - OFFSET TO CARTESIAN GRID BUFFER HEADER
     CGRID       - OFFSET TO 1ST WORD OF CARTESIAN GRID
     HDRLNG      - NUMBER OF I*4 WORDS IN HEADER
 
     GRID_COL -    INDEX OF NUMBER OF COLUMNS IN GRID
     GRID_ROW -    INDEX OF NUMBER OF ROWS IN GRID
     GRID_RES -    INDEX OF GRID RESOLUTION
     GRID_MAXVAL - INDEX OF MAXIMUM DATA VALUE
     GRID_MAXI   - INDEX OF I COORDINATE OF LATEST MAXVAL
     GRID_MAXJ   - INDEX OF J COORDINATE OF LATEST MAXVAL
     VS_NUM      - INDEX OF VOLUME-SCAN NUMBER
     VS_DAT      - INDEX OF VOLUME-SCAN DATE
     VS_TIM      - INDEX OF VOLUME-SCAN TIME
     GRID_CALCON - CALIBRATION CONSTANT IN ONE I*4 WORD
 
     CGRIDHDR    - ARRAY CONTAINING GRID-BUFFER HEADER
 
********************************************************************/
#define CHDR			0 
#define CGRID			10 
#define HDRLNG			10

#define GRID_COL		0 
#define GRID_ROW		1 
#define GRID_RES		2
#define GRID_MAXVAL		3 
#define GRID_MAXI		4 
#define GRID_MAXJ		5
#define VS_NUM			6 
#define VS_DAT			7 
#define VS_TIM			8
#define GRID_CALCON		9
 
#endif
