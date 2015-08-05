/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/07 17:54:03 $
 * $Id: mda_ru.h,v 1.1 2004/01/07 17:54:03 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/************************************************************************
Module:         mda_ru.h

Description:    include file for  mda rapid update  
      
************************************************************************/

#ifndef MDA_RU_H 
#define MDA_RU_H


#define 	MESO_MAX_FEAT	500 /* the maximum allowable 2D features */



typedef struct {
  float ca;
  float cr;
  float cx;
  float cy;
  float ht;
  float dia;
  float rot_vel;
  float shr;
  float gtgmax;
  float rank;
  float avg_conv;
  float max_conv;
  } feature2d_t;

typedef struct {
  int meso_event_id; 
  int time_code; 
  float ll_azm;
  float ll_rng;
} Prev_newcplt;


#endif



