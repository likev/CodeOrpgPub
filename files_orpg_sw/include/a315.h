/*
 * Module: a315.h
 * cpc015 Storm Centroids structures
 *
 *      The contents in this file are derived from a315buf.inc. The macros
 *      must be consistent with those defined there. Thus, if a315.inc is
 *      modified, this file has to be updated accordingly.
 *
 * Notes:
 *	CENTRATTR - Storm Centroids Attributes
 *      The field names come from A315P5C in a315buf.inc
 *
 */


#ifndef A315_H
#define A315_H

#include <orpgctype.h>


#define NSTM_CHAR       14      /* number of attributes per storm            */
#define NSTM_MAX       100	/* maximum number of storms                  */

/*
 * CENTRATTR Header
 */
typedef struct {
   fint  voltime; 		/* Volume Scan Time, msecs, Julian           */
   fint  nstorms;		/* Number of storms this volume              */
   fint  ncompstk;              /* Number of components in linked list stack */
} centattr_header_t;

typedef struct {
   freal azm;			/* Azimuth of centroid (deg)                 */
   freal ran;                   /* Range of centroid (km)                    */
   freal xcn;                   /* Centroid's X position (km from radar)     */
   freal ycn;                   /* Centroid's Y position (km from radar)     */
   freal zcn;                   /* Centroid's Z position (height ARL, km)    */
   freal mrf;                   /* Max reflectivity (dBZ)                    */
   freal rfh;                   /* Height of max reflectivity (km, ARL)      */
   freal vil;                   /* Cell-based VIL (kg/m**2)                  */
   freal ncp;                   /* Number of components                      */
   freal ent;                   /* Entry location of centroid in the stack   */
   freal top;                   /* Top height (km)                           */
   freal lct;                   /* Top flag < 0 if top on highest elevation  */
   freal bas;                   /* Base height (km)                          */
   freal lcb;                   /* Base flag < 0 if base on lowest elevation */
} stormain_t;

typedef struct {
   centattr_header_t hdr;           /* header (see above)                    */
   stormain_t        stm[NSTM_MAX]; /* storm centroid attributes (see above) */
} centattr_t;

#endif /* DO NOT REMOVE! */
