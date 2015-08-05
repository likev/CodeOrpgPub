/*
 * $revision$
 * $state$
 * $Logs$
 */

/************************************************************************
Module:         superob.h

Description:    include file for the Superob algorithm
                superob.c. This file contains module 
                specific include file listings, function prototypes 
                and constant definitions.
************************************************************************/

#ifndef SUPEROB_H
#define SUPEROB_H

/* system includes ---------------------------------------------------- */
#include <stdio.h>

/* ORPG   includes ---------------------------------------------------- */
#include <rpg_globals.h>
#include <rpgc.h>
#include <basedata.h>
#include <alg_adapt.h>

/* -------------------------------------------------------------------- */
/* the structure of a base data radial message, which is ingested by the
   algorithm, is defined in  basedata.h  as follows: */

/*  #define BASEDATA_HD_SIZE    52      size of basedata header in shorts 
    #define BASEDATA_REF_SIZE  460      size of refl field in shorts 
    #define BASEDATA_DOP_SIZE  920      size of vel or spw field in shorts 				  
    #define BASEDATA_SIZE     2352      base data message size in shorts 

    typedef struct {

       Base_data_header hdr;
       short            ref[BASEDATA_REF_SIZE];
       short            vel[BASEDATA_DOP_SIZE];
       short            spw[BASEDATA_DOP_SIZE];

    } Base_data_radial; */
   
  /* define a structure for time */
  typedef struct {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  } time_type;


/* The contents of Base_data_header is also defined in basedata.h  */ 
/* -------------------------------------------------------------------- */


/* local  includes ---------------------------------------------------- */
#include "superob_adapt.h"
#include "superob_path.h"
#include "superob_build_symb_layer.h"


/* structure for adaptation data */
  superob_adapt_t superob_adapt;


/* ORPG constants not defined in include files ------------------------ */


/* Algorithm specifid constants/definitions --------------------------- */
#define PCODE 136           /* ORPG product code, must match the  */
                            /* product_tables configuration file  */
                            /* entry for this product             */
#define SUPEROBVEL 136      /* ORPG Linear Buffer Code, normally */
                            /* defined in a309.h , must match    */
                            /* the product_tables configuration  */
                            /* file entry for this product       */

                            
#define BUFSIZE 540000      /* estimated output size for the ICD product */
                            /* based on 6 degree azimuth and 5 km range */
                            /* and 20 elevations per volume, the maximum*/
                            /* range is 120km   			*/

/* function prototypes ------------------------------------------------ */
void clear_buffer(char*);
 int    superob_update_elev_table(float[], int, int*, int*, float, int);
 void   superob_thin_data(int***, int , int, int, int);
 float  superob_decode(short, short);
 void   superob_path(double, double, double, float, float,float, position_t *);
 int    superob_timeok(int, int, int, int, int, int, float *);
 void   superob_initialize(double***,double***, double***, double***,double***,
                                 double***,double***,int***, int, int, int*, float[]);
 void   build_symb_layer(int* ,int ***, double ***,double***,position_t***,
                            double***,int *, int, int, int, int, float[]);

 void create_new_window(int, int, int*, int*, int*, int*, int, int);

#endif


