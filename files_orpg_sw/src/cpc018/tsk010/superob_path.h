
/*
 * RCS info
 * $Author: cheryls $
 * $Date: 2002/03/07 14:06:33 $
 * $Locker:  $
 * $Id: superob_path.h,v 1.1 2002/03/07 14:06:33 cheryls Exp $
 * $revision$
 * $state$
 * $Logs$
 */


/************************************************************************
 *	Module:         path.h
 *
 *	Description: include file for the path.c. this file contain
 *                   a structure type "position"
 ************************************************************************/
# ifndef SUPEROB_PATH_H
# define SUPEROB_PATH_H

typedef struct {
      float lat;
      float lon;
      float height;
      float azimuth;
    } position_t;

# endif
