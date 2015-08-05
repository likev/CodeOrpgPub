/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2010/08/04 14:08:16 $ */
/* $Id: basvlcty.c,v 1.12 2010/08/04 14:08:16 ccalvert Exp $ */
/* $Revision: 1.12 $ */
/* $State: Exp $ */

#include <basvlcty.h>

/* Local function prototypes */
void Assign_bvelptrs (int pindx);

/************************************************************************

   Description:
     Buffer control routine for Base Velocity mapping program

   Returns:
      Always returns 0.

************************************************************************/
int a30721_buffer_control()
{
  int ref_flag, wid_flag, vel_flag;
  int *bdataptr;
  int no_mem_abort, pindx, opstat, abortit;

  opstat = NORMAL;
  Endelcut = 0;

  for (pindx = 0; pindx < NUMPRODS; ++pindx) 
  {
    Prod_info[pindx].pflag = 0;
    Prod_info[pindx].no_mem_flag = 0;
    Prod_info[pindx].bvelptrs = NULL;
    Prod_info[pindx].radcount = 0;
  }

  /* REQUEST ALL POSSIBLE OUTPUT BUFFERS TO SEE AT WHAT RANGES, */
  /* RESOLUTIONS, AND DATA LEVELS BASE VELOCITY MAPS NEED TO BE */
  /* GENERATED: */
  pindx = 0;

  /* PRODUCT CODE 22, 0 TO 60 KM. RADIUS (.25X.25 KM. RESOLUTION), */
  /* 8 DATA LEVEL BASE VELOCITY PRODUCT BUFFER: */
  Assign_bvelptrs(pindx); ++pindx;

  /* PRODUCT CODE 23, 0 TO 115 KM. RADIUS (.5X.5 KM. RESOLUTION), */
  /* 8 DATA LEVEL BASE VELOCITY PRODUCT BUFFER: */
  Assign_bvelptrs(pindx); ++pindx;

  /* PRODUCT CODE 24, 0 TO 230 KM. RADIUS (1X1 KM. RESOLUTION), */
  /* 8 DATA LEVEL BASE VELOCITY MAP PRODUCT BUFFER: */
  Assign_bvelptrs(pindx); ++pindx;

  /* PRODUCT CODE 25, 0 TO 60 KM. RADIUS (.25X.25 KM. RESOLUTION), */
  /* 16 DATA LEVEL BASE VELOCITY MAP PRODUCT BUFFER: */
  Assign_bvelptrs(pindx); ++pindx;

  /* PRODUCT CODE 26, 0 TO 115 KM. RADIUS (.5X.5 KM. RESOLUTION), */
  /* 16 DATA LEVEL BASE VELOCITY MAP PRODUCT BUFFER: */
  Assign_bvelptrs(pindx); ++pindx;

  /* PRODUCT CODE 27, 0 TO 230 KM. RADIUS (1X1 KM. RESOLUTION), */
  /* 16 DATA LEVEL BASE VELOCITY MAP PRODUCT BUFFER: */
  Assign_bvelptrs(pindx);

  /* DETERMINE IF ANYTHING TO DO, IF NOT ABORT THE WHOLE THING */
  abortit = 1;
  no_mem_abort = 1;

  for (pindx = 0; pindx < NUMPRODS; ++pindx) 
  {
    if (Prod_info[pindx].pflag) 
      abortit = 0;

    if (Prod_info[pindx].no_mem_flag) 
      no_mem_abort = 0;
  }

  if (!abortit) 
  {
    /* REQUEST INPUT BUFFERS (RADIAL BASE DATA) AND PROCESS THEM */
    /* UNTIL THE END OF THE ELEVATION CUT IS REACHED: */
    bdataptr = RPGC_get_inbuf_by_name("BASEDATA", &opstat);

    /* CHECK FOR BASE VELOCITY DISABLED */
    if (opstat == NORMAL) 
    {
      RPGC_what_moments( (Base_data_header *) bdataptr, &ref_flag,
                         &vel_flag, &wid_flag );
      if (!vel_flag) 
      {
        /* MOMENT DISABLED....RELEASE INPUT BUFFER AND DO ABORT PROCESSING */
        RPGC_rel_inbuf(bdataptr);
        a30726_rel_prodbuf(DESTROY);
        RPGC_abort_because( PROD_DISABLED_MOMENT );
        return 0;
      }
    }

    /* CALL THE PRODUCT GENERATION CONTROL ROUTINE: */
    while(1)
    {
      if (opstat == NORMAL) 
      {
        a30722_product_generation_control(bdataptr);

        /* RELEASE THE INPUT RADIAL: */
        RPGC_rel_inbuf(bdataptr);
      } else
      {
        /* IF THE INPUT DATA STREAM HAS BEEN CANCELED FOR SOME REASON, */
        /* THEN RELEASE AND DESTROY ALL OF THE PRODUCT BUFFERS OBTAINED, */
        /* AND RETURN TO THE PARAMETER TRAP ROUTER ROUTINE TO WAIT FOR */
        /* FURTHER INPUT: */
        a30726_rel_prodbuf(DESTROY);
        return 0;
      }

      /* TEST FOR THE END OF THE ELEVATION CUT (IF THE LAST RADIAL IN */
      /* THE ELEVATION CUT HAS JUST BEEN PROCESSED, THEN RELEASE AND */
      /* FORWARD ALL OF THE PRODUCT BUFFERS FOR THE PRODUCTS */
      /* GENERATED): */
      if (Endelcut) 
      {
        a30726_rel_prodbuf(FORWARD);
        return 0;
      }

      /* RETRIEVE NEXT INPUT RADIAL: */
      bdataptr = RPGC_get_inbuf( BASEDATA, &opstat );
    }

  /* ENDS THE ABORTIT IF BLOCK */
  } else 
  {
    /* DO ABORT PROCESSING. */
    RPGC_abort_because( opstat );
  }

  /* RETURN TO THE PARAMETER TRAP ROUTER ROUTINE TO DETERMINE IF */
  /* ANY BASE VELOCITY MAPS ARE TO BE GENERATED ON THE NEXT */
  /* ELEVATION CUT: */
  return 0;
}

/************************************************************************

   Description:
      Function to acquire output buffer for product request. 
 
   Inputs:
     pindx - product index

   Returns:
      Void function

************************************************************************/
void Assign_bvelptrs (int pindx)
{
  int opstat = NORMAL;

  Prod_info[pindx].bvelptrs = RPGC_get_outbuf_by_name(Prod_info[pindx].pname, 
                                                      Prod_info[pindx].psize, 
                                                      &opstat);
  if (opstat == NORMAL) 
  {
    Prod_info[pindx].pflag = 1;
  } 
  else if (opstat == NO_MEM) 
  {
    /* IF NO MEMORY FOR PRODUCT BUFFER, ABORT DATATYPE */
    RPGC_abort_dataname_because(Prod_info[pindx].pname, NO_MEM);
    Prod_info[pindx].no_mem_flag = 1;
  }
}


/************************************************************************

   Description:
      Product Generation Control routine for Base Velocity
 
   Inputs:
      bdataptr - pointer to basedata radial.

   Returns:
      Always returns 0.

************************************************************************/
int a30722_product_generation_control(void *bdataptr)
{

  static int maxnbins[NUMPRODS] = { 240,460,920,240,460,920 };

    /* Local variables */
  int start_index, end_index, nrlebyts;
  int delta, pindx, start, stop_index, excess;

  /* LOCAL STATIC VARIABLES. */
  static int pbuffind[NUMPRODS], clthtind[NUMPRODS], nrleb[NUMPRODS];
  static short numbins[NUMPRODS], velresol;

  Base_data_header *radhead = (Base_data_header *) bdataptr;
  short *vel_data = (short *) ( (char *) bdataptr + radhead->vel_offset);

  int elevindex, vcpnumber;
  double elang, coselev;

/* *********************** E X E C U T A B L E  ********************* */



  /* BEGINNING OF PRODUCT INITIALIZATION: */
  if ((radhead->status == GOODBVOL) || (radhead->status == GOODBEL))
  {

    /* INITIALIZE  THE MAX DATA LEVEL VARIABLES TO BIASED ZERO */
    Mxneg60 = ZPOINT1;
    Mxneg115 = ZPOINT1;
    Mxneg230 = ZPOINT1;
    Mxpos60 = ZPOINT1;
    Mxpos115 = ZPOINT1;
    Mxpos230 = ZPOINT1;

    /* BUFFER INDEX COUNTER AND NUMBER OF RUN-LENGTH ENCODED BYTES */
    /* COUNTER INITIALIZATION FOR ALL PRODUCTS: */
    for (pindx = 0; pindx < NUMPRODS; ++pindx) 
    {
      pbuffind[pindx] = (sizeof(Graphic_product) + sizeof(Symbology_block)
                         + sizeof(packet_af1f_hdr_t))/sizeof(short);
      nrleb[pindx] = 0;
      numbins[pindx] = 0;
      Prod_info[pindx].radcount = 0;
    }

    /* INITIALIZE COLOR LOOK-UP AND THRESHOLD TABLE INDICIES: */
    a30727_get_colorix(radhead, clthtind);

    /* INITIALIZE THE VELOCITY RESOLUTION */
    velresol = radhead->dop_resolution;

    /* BUILD ALL NECESSARY PRODUCT HEADERS: */
    for (pindx = 0; pindx < NUMPRODS; ++pindx) 
    {
      if (Prod_info[pindx].pflag)
      {
        numbins[pindx] = RPGC_num_rad_bins(bdataptr, Prod_info[pindx].maxbins, 
                                           Prod_info[pindx].radstep, 2);
        maxnbins[pindx] = numbins[pindx] * Prod_info[pindx].radstep;
        a30723_product_header(bdataptr, clthtind[pindx], pindx, numbins[pindx]); 
      }
    }

    /* COSINE OF ELEVATION ANGLE COMPUTATION FOR COMBINATION WITH */
    /* SCALE FACTOR FOR THE VERTICAL CORRELATION OF BASE VELOCITY */
    /* MAPS: */
    vcpnumber = RPGC_get_buffer_vcp_num( bdataptr );
    elevindex = RPGC_get_buffer_elev_index( bdataptr );

    /*  GET ELEVATION ANGLE FROM VOLUME COVERAGE PATTERN AND ELEVATION */
    /*  INDEX NUMBER */
    Elmeas = (short) RPGCS_get_target_elev_ang( vcpnumber, elevindex ); 
    elang = Elmeas * 0.1f;
    coselev = cos( elang * DEGTORAD );
    Vc1 = (short) (coselev * 1000.0);
    Vc2 = (short) (coselev * 1000.0);

  } /* END OF PRODUCT INITIALIZATION: */

  /* PERFORM INDIVIDUAL RADIAL PROCESSING: */

  /* RETRIEVE START ANGLE AND DELTA ANGLE MEASUREMENTS FROM THE */
  /* INPUT RADIAL BUFFER FOR THIS RADIAL: */
  start = radhead->start_angle;
  delta = radhead->delta_angle;

  /*  CALCULATE THE START AND END OF THE GOOD DATA */
  /*  FRST_VEL IS THE START INDEX OF GOOD DATA */
  /*  END_VEL IS THE INDEX OF THE LAST GOOD VEL DATA */
  start_index = radhead->dop_range - 1;
  end_index = start_index + radhead->n_dop_bins - 1;
  if( end_index > BASEDATA_VEL_SIZE )
     end_index = BASEDATA_VEL_SIZE;

  /* PERFORM MAXIMUM DATA LEVEL OPERATIONS FOR THIS RADIAL: */
  a30724_maxdl(bdataptr, start_index, end_index);

  /* RUN-LENGTH ENCODE THIS RADIAL FOR ALL PRODUCTS REQUESTED: */
  for (pindx = 0; pindx < NUMPRODS; ++pindx) 
  {
    if (Prod_info[pindx].pflag)
    {
      /*    CALCULATE WORDS AVAILABLE IN BUFFER */
      excess = Prod_info[pindx].psize - (nrleb[pindx] + 150);
      if (excess > Prod_info[pindx].est_per_rad)
      {

        /* Computing MIN */
        stop_index = end_index;
        if( (maxnbins[pindx] - 1) < end_index )
          stop_index = maxnbins[pindx] - 1;

        /* INCREMENT THE RADIAL COUNT: */
        Prod_info[pindx].radcount++;

        RPGC_run_length_encode(start, delta, vel_data, start_index, 
            stop_index, maxnbins[pindx], Prod_info[pindx].radstep, 
            &color_data.coldat[clthtind[pindx]][0], 
            &nrlebyts, pbuffind[pindx], (short *) Prod_info[pindx].bvelptrs);
        
        /*      UPDATE COUNTERS */
        nrleb[pindx] += nrlebyts;
        pbuffind[pindx] += nrlebyts / 2;

      }
    }
  }

  /* TEST FOR THE LAST RADIAL IN THE ELEVATION CUT: */
  if( (radhead->status == GENDVOL) || (radhead->status == GENDEL) )
  {
    Endelcut = 1;

    /* DO FOR ALL PRODUCTS REQUESTED: */
    for (pindx = 0; pindx < NUMPRODS; ++pindx) 
    {
      if (Prod_info[pindx].pflag) 
      {
        a30725_end_of_product_processing(nrleb[pindx], pindx, velresol);
      }
    }
  }

  /* RETURN TO BUFFER CONTROL ROUTINE: */
  return 0;
}

/************************************************************************

   Description:
      Product Header production routine for BASE VELOCITY MAP program
 
   Inputs:
     bdataptr - pointer to basedata radial.
     trshind - Color Table threshold index.
     pindx - product index.
     numbins - number of range bins in the product.

   Returns:
      Always returns 0.

************************************************************************/
int a30723_product_header(void *bdataptr, int trshind, int pindx, 
                          short numbins)
{
  int volno, offset;
  short *outbuff = (short *) Prod_info[pindx].bvelptrs;

  Graphic_product *phd = (Graphic_product *) outbuff;
    Symbology_block *sym = 
        (Symbology_block *) (((char *) outbuff) + sizeof(Graphic_product));
    packet_af1f_hdr_t *packet_af1f = 
        (packet_af1f_hdr_t *) (((char *) outbuff) + 
                        sizeof(Graphic_product) + sizeof(Symbology_block));

    /* FILL OUT THE PRODUCT DESCRIPTION BLOCK. */
    volno = RPGC_get_buffer_vol_num( bdataptr );
    RPGC_prod_desc_block( outbuff, Prod_info[pindx].prod_id, volno );

     /* STORE DATA LEVEL THRESHOLD CODES. */
    phd->level_1 = color_data.thresh[trshind][0];
    phd->level_2 = color_data.thresh[trshind][1];
    phd->level_3 = color_data.thresh[trshind][2];
    phd->level_4 = color_data.thresh[trshind][3];
    phd->level_5 = color_data.thresh[trshind][4];
    phd->level_6 = color_data.thresh[trshind][5];
    phd->level_7 = color_data.thresh[trshind][6];
    phd->level_8 = color_data.thresh[trshind][7];
    phd->level_9 = color_data.thresh[trshind][8];
    phd->level_10 = color_data.thresh[trshind][9];
    phd->level_11 = color_data.thresh[trshind][10];
    phd->level_12 = color_data.thresh[trshind][11];
    phd->level_13 = color_data.thresh[trshind][12];
    phd->level_14 = color_data.thresh[trshind][13];
    phd->level_15 = color_data.thresh[trshind][14];
    phd->level_16 = color_data.thresh[trshind][15];

    /* SET OFFSET TO PRODUCT SYMBOLOGY BLOCK, GRAPHIC ALPHANUMERIC
       AND TABULAR ALPHANUMERIC. */
    offset = sizeof(Graphic_product) / sizeof(short);
    RPGC_set_prod_block_offsets( outbuff, offset, 0, 0 );

    /* PRODUCT BLOCK DIVIDER, BLOCK ID, NUMBER OF LAYERS AND LAYER 
       DIVIDER. */
    sym->divider = -1;
    sym->block_id = 1;
    sym->n_layers = 1;
    sym->layer_divider = -1;

    /* BUILD THE RADIAL HEADER FOR THIS PRODUCT: 
       STORE OPERATION CODE, RANGE TO FIRST BIN, NUMBER OF RANGE 
       BINS AND I,J CENTER OF SWEEP. */
    packet_af1f->code = 44831;
    packet_af1f->index_first_range = 0;
    packet_af1f->num_range_bins = numbins;
    packet_af1f->i_center = 256;
    packet_af1f->j_center = 280;

    /* LENGTH OF LAYER,LENGTH OF BLOCK,LENGTH OF MESSAGE 
       SCALE FACTOR,NUMBER OF RADIALS ARE STORED IN THE 
       END-OF-PRODUCT MODULE. */

    return 0;

} 

/* Note: These are bin indices ... so we subtract 1 for zero-indexed
         arrays. */
#define MAXB230    919
#define MAXB115    459
#define MAXB60     239

/************************************************************************

   Description:
     Maximum data level routine for Base Velocity 
 
   Inputs:
     bdataptr - pointer to basedata radial.
     frst_vel - first velocity bin
     end_vel - last velocity bin

   Returns:
     Always returns 0
      

************************************************************************/
int a30724_maxdl(char *bdataptr, int frst_vel, int end_vel)
{
  /* Local variables */
  int sb230, sb115, endb60, endb230, endb115, binindx;

  Base_data_header *bdh = (Base_data_header *) bdataptr;
  short *radial = (short *) (bdataptr + bdh->vel_offset);

  /* EXECUTABLE CODE: */

  /*  CALCULATE LOOP LIMITS BASED UPON THE START AND END OF GOOD DATA */

  /* Computing MIN */
  endb60 = MAXB60;
  if (endb60 > end_vel)
    endb60 = end_vel;

  endb115 = MAXB115;
  if (endb115 > end_vel)
    endb115 = end_vel;

  endb230 = MAXB230;
  if ( endb230 > end_vel)
    endb230 = end_vel;

  sb115 = endb60 + 1;
  if (sb115 > end_vel)
    sb115 = end_vel;

  sb230 = endb115 + 1;
  if (sb230 > end_vel)
    sb230 = end_vel;

LE_send_msg( 0, "frst_vel: %d, sb115: %d, sb230: %d\n",
             frst_vel, sb115, sb230 );
LE_send_msg( 0, "end_vel: %d, endb60: %d, endb115: %d, endb230: %d\n",
             end_vel, endb60, endb115, endb230 );

  /* TEST FOR 230 KM. RANGE PRODUCTS: */
  if (Prod_info[2].pflag || Prod_info[5].pflag) 
  {
    for (binindx = frst_vel; binindx <= endb60; ++binindx) 
    {
      if (radial[binindx] > 1) 
      {
        if (radial[binindx] < 129) 
        {
          if (radial[binindx] < Mxneg60) 
            Mxneg60 = radial[binindx];
        } else 
        {
          if (radial[binindx] > Mxpos60) 
            Mxpos60 = radial[binindx];
        }
      }
    }

    if (Mxneg60 < Mxneg115) 
      Mxneg115 = Mxneg60;
    if (Mxpos60 > Mxpos115) 
      Mxpos115 = Mxpos60;

    for (binindx = sb115; binindx <= endb115; ++binindx) 
    {
      if (radial[binindx] > 1) 
      {
        if (radial[binindx] < 129) 
        {
          if (radial[binindx] < Mxneg115) 
            Mxneg115 = radial[binindx];
        } else 
        {
          if (radial[binindx] > Mxpos115) 
            Mxpos115 = radial[binindx];
        }
      }
    }

    if (Mxneg115 < Mxneg230) 
      Mxneg230 = Mxneg115;
    if (Mxpos115 > Mxpos230) 
      Mxpos230 = Mxpos115;

    for (binindx = sb230; binindx <= endb230; ++binindx) 
    {
      if (radial[binindx] > 1) 
      {
        if (radial[binindx] < 129) 
        {
          if (radial[binindx] < Mxneg230) 
            Mxneg230 = radial[binindx];
        } else 
        {
          if (radial[binindx] > Mxpos230) 
            Mxpos230 = radial[binindx];
        }
      }
    }

  } else if (Prod_info[1].pflag || Prod_info[4].pflag) 
  {
    for (binindx = frst_vel; binindx <= endb60; ++binindx) 
    {
      if (radial[binindx] > 1) 
      {
        if (radial[binindx] < 129) 
        {
          if (radial[binindx] < Mxneg60) 
            Mxneg60 = radial[binindx];
        } else 
        {
          if (radial[binindx] > Mxpos60) 
            Mxpos60 = radial[binindx];
        }
      }
    }

    if (Mxneg60 < Mxneg115) 
      Mxneg115 = Mxneg60;
    if (Mxpos60 > Mxpos115) 
      Mxpos115 = Mxpos60;

    for (binindx = sb115; binindx <= endb115; ++binindx) 
    {
      if (radial[binindx] > 1) 
      {
        if (radial[binindx] < 129) 
        {
          if (radial[binindx] < Mxneg115) 
            Mxneg115 = radial[binindx];
        } else 
        {
          if (radial[binindx] > Mxpos115) 
            Mxpos115 = radial[binindx];
        }
      }
    }
  } else 
  {
    for (binindx = frst_vel; binindx <= endb60; ++binindx) 
    {
      if (radial[binindx] > 1) 
      {
        if (radial[binindx] < 129) 
        {
          if (radial[binindx] < Mxneg60) 
            Mxneg60 = radial[binindx];
        } else 
        {
          if (radial[binindx] > Mxpos60) 
            Mxpos60 = radial[binindx];
        }
      }
    }
  }
  return 0;
}

/************************************************************************

   Description:
     Maximum data level routine for Base Velocity mapping program. 
 
   Inputs:
      nrleb - number of run-length-encoded bytes.
      pindx - product index.
      vres - velocity resolution.

   Returns:
     Always returns 0.
      

************************************************************************/
int a30725_end_of_product_processing(int nrleb, int pindx, int vres)
{
  /* LOCAL DECLARATIONS: */
  short params[10];
  int bytecnt, pid;
  float maxval;

  short *outbuff = (short *) Prod_info[pindx].bvelptrs;

  packet_af1f_hdr_t *packet_af1f = (packet_af1f_hdr_t *) (((char *) outbuff) +
                                    sizeof(Graphic_product) +
                                    sizeof(Symbology_block));
  Symbology_block *sym = (Symbology_block *) (((char *) outbuff) +
                          sizeof(Graphic_product));




  /* EXECUTABLE CODE: */
  
  /* INITIALIZE THE PRODUCT DEPENDENT PARAMETERS TO 0. */
  memset( params, 0, 10*sizeof(short) );

  /* ASSIGN THE ELEVATION ANGLE MEASUREMENT TO THE PRODUCT HEADER: */
  params[2] = Elmeas;

  /* GET THE PRODUCT ID. */
  pid = Prod_info[pindx].prod_id;

  /* ASSIGN THE SCALE FACTOR (WITH THE VERTICAL CORRELATION */
  /* CORRECTION ALREADY APPLIED FOR THIS ELEVATION ANGLE) TO THE */
  /* DISPLAY HEADER: */
  if (pid == BVEL22 || pid == BVEL25) 
      packet_af1f->scale_factor = Vc1;
  else 
      packet_af1f->scale_factor = Vc2;

  /* ASSIGN THE MAXIMUM DATA LEVEL FOR THIS PRODUCT TO THE PRODUCT */
  /* HEADER: */
  if (pid == BVEL22 || pid == BVEL25) 
  {
    RPGCS_set_velocity_reso (vres);
    maxval = RPGCS_velocity_to_ms(Mxneg60);
    params[3] = (short) RPGC_NINT(maxval*MPS_TO_KTS);
    maxval = RPGCS_velocity_to_ms(Mxpos60);
    params[4] = (short) RPGC_NINT(maxval*MPS_TO_KTS);
  } else if (pid == BVEL23 || pid == BVEL26) 
  {
    RPGCS_set_velocity_reso (vres);
    maxval = RPGCS_velocity_to_ms(Mxneg115);
    params[3] = (short) RPGC_NINT(maxval*MPS_TO_KTS);
    maxval = RPGCS_velocity_to_ms(Mxpos115);
    params[4] = (short) RPGC_NINT(maxval*MPS_TO_KTS);
  } else 
  {
    RPGCS_set_velocity_reso (vres);
    maxval = RPGCS_velocity_to_ms(Mxneg230);
    params[3] = (short) RPGC_NINT(maxval*MPS_TO_KTS);
    maxval = RPGCS_velocity_to_ms(Mxpos230);
    params[4] = (short) RPGC_NINT(maxval*MPS_TO_KTS);
  }

  /* SET THE PRODUCT DEPENDENT PARAMETERS. */
  RPGC_set_dep_params(outbuff, params);

  /* ASSIGN THE RADIAL COUNT FOR THIS PRODUCT TO THE DISPLAY HEADER: */
  packet_af1f->num_radials = Prod_info[pindx].radcount;

  /*     CALCULATE AND STORE THE PRODUCT MESSAGE LENGTH, THE */
  /*     PRODUCT BLOCK LENGTH, AND THE PRODUCT LAYER LENGTH. */

  /*     LENGTH OF PRODUCT LAYER */
  bytecnt = nrleb + sizeof(packet_af1f_hdr_t);
  RPGC_set_product_int(&sym->data_len, bytecnt);

  /*     LENGTH OF BLOCK */
  bytecnt += sizeof(Symbology_block);
  RPGC_set_product_int(&sym->block_len, bytecnt);

  /*     LENGTH OF MESSAGE */
  RPGC_prod_hdr(outbuff, pid, &bytecnt);

  /* RETURN TO THE PRODUCT GENERATION CONTROL ROUTINE: */
  return 0;
}

/************************************************************************

   Description:
     Routine to release the product output buffer, and either forward 
     or destroy the buffer upon release, for Base Velocity.
      
   Inputs:
      bvelptrs - pointers to output products
      relstat - release disposition

   Returns:
     Always returns 0.

************************************************************************/
int a30726_rel_prodbuf(int relstat)
{
  int pindx;

  /* IF INPUT DATA STREAM WAS INTERUPTED FOR SOME REASON, THEN */
  /* RELEASE AND DESTROY ALL PRODUCT BUFFERS OBTAINED, ELSE */
  /* RELEASE AND FORWARD ALL FINISHED PRODUCTS TO ON-LINE STORAGE: */
  for( pindx = 0; pindx < NUMPRODS; ++pindx )
  {
    if(Prod_info[pindx].pflag){

      RPGC_rel_outbuf( Prod_info[pindx].bvelptrs, relstat );
      Prod_info[pindx].bvelptrs = NULL;
      Prod_info[pindx].pflag = Prod_info[pindx].no_mem_flag = 0;

     }
  }

  return 0;
}

/************************************************************************

   Description:
      Get Color Index routine for Base Velocity
 
   Inputs:
      radhead - pointer to bbase data header of the product
      clthtind - Table of Color Table Indexes

   Returns:
      Always returns 0.

************************************************************************/
int a30727_get_colorix( Base_data_header *radhead, int *clthtind )
{
  /* SET UP COLOR INDEX DEPENDING UPON THE */
  /* VELOCITY RESOLUTION */

  /* Function Body */
  if (radhead->dop_resolution == 1) 
  {
    if (radhead->weather_mode == CLEAR_AIR_MODE) 
    {
      clthtind[0] = VELCL81;
      clthtind[1] = VELCL81;
      clthtind[2] = VELCL81;
      clthtind[3] = VELCL161;
      clthtind[4] = VELCL161;
      clthtind[5] = VELCL161;
    } else 
    {
      /* SET UP INDEX FOR NON CLEAR AIR RESOLUTION 1 */
      clthtind[0] = VELNC81;
      clthtind[1] = VELNC81;
      clthtind[2] = VELNC81;
      clthtind[3] = VELNC161;
      clthtind[4] = VELNC161;
      clthtind[5] = VELNC161;
    }
  } else 
  {
    /* VELOCITY RESOLUTION IS 2 */
    if (radhead->weather_mode == CLEAR_AIR_MODE) 
    {
      clthtind[0] = VELCL82;
      clthtind[1] = VELCL82;
      clthtind[2] = VELCL82;
      clthtind[3] = VELCL162;
      clthtind[4] = VELCL162;
      clthtind[5] = VELCL162;
    } else 
    {
      /* SET INDEX FOR NON-CLEAR AIR RESOLUTION 2 */
      clthtind[0] = VELNC82;
      clthtind[1] = VELNC82;
      clthtind[2] = VELNC82;
      clthtind[3] = VELNC162;
      clthtind[4] = VELNC162;
      clthtind[5] = VELNC162;
    }
  }
    return 0;
}
