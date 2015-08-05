/*
 * RCS info
 * $Author: ccalvert $
 * $Date: 2010/09/02 14:12:09 $
 * $Locker:  $
 * $Id: superob_adapt.h,v 1.8 2010/09/02 14:12:09 ccalvert Exp $
 * $revision$
 * $state$
 * $Logs$
 */


#ifndef SUPEROB_ADAPT_H
#define SUPEROB_ADAPT_H


#define SUPEROB_DEA_NAME "alg.superob"


typedef struct superob_adapt
{
 int rangemax;           /*#  @name "Maximum Range" @desc "max radar range" @units "km" @min 60  @max 230 @default 120 */
 int deltr;              /*#  @name "Cell Range Size" @desc "radial dimention of averaging cell" @units "km" @min 1  @max 10 @default 5 */
 int offset_min;         /*#  @name "Data Collection Offset" @desc "offset for data collection time" @units "min" @min 0 @max 59 @default 15 */
 int deltaz;             /*#  @name "Cell Azimuth Size" @desc "azimuthal dimention of cell" @units "degrees" @min 2  @max 12 @default 6 */
 int deltt;              /*#  @name "Time Radius" @desc "radius of time window" @units "mins" @min 5 @max 90 @default 30*/
 int min_sample_size;    /*#  @name "Minimum Number of Points" @desc "minumum sample size" @min 20 @max 200 @default 50*/
} superob_adapt_t;


#endif

