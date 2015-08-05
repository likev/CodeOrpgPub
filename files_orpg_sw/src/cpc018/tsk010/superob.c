/*
 * $revision$
 * $state$
 * $Logs$
 */

/************************************************************************
 *	Module:         superob.c					*
 *    									*
 *      Description: main function to drive the superob algorithm       *
 *    									*
 *	Input:       one radial at a time as defined in the        	*
 *		     Base_data_radial structure (see the basedata.h)    *
 *	Output:      a formatted volume-based superob ICD graphical     * 
 *                   product which is forwarded to ORPG for distribution*
 *      returns:     one						*
 *      Globals:     none						*
 *      Notes:       The BUFSIZE (540000) is based on the default       *
 *                   adaptation parameters: Cell Range size=5km         *
 *                                          Cell Azimuth   =6degree	*
 *                                          Maximum Range  =120km   	*
 *                                          Max number of elevation =20 *
 *                   If the above-mentioned adaptation parameters       *
 *                   change, the BUFSIZE should be updated to a proper  *
 *		     value in superob.h file				*
 ************************************************************************/


/***    System include file   						*/

#include <stdio.h>
#include <math.h>

/***    local include file						*/

#include "superob.h"
#include <siteadp.h>


#define MAX_NUM_ELEV 		20       /* max num of elevations      */ 
#define DAY2SECOND		86400    /* one hour = 3600 second     */
#define HOUR2SECOND		3600     /* one hour = 3600 second     */
#define MINUTE2SECOND		60       /* one minute = 60 second     */
#define TWO_PI			360      /* constant value of 2*pi     */ 
#define ONE_THOUSAND		1000.0	 /* constant value 1000        */
#define FOOT2METER		0.3048	 /* convert foot to meter      */


int 
main(int argc,char* argv[]) {
  /* algorithm variable declarations and definitions ----------------- */
  Base_data_radial 
      *basedataPtr=NULL;    /* a pointer to a base data radial structure*/
      
  int ref_enable;           /* variables: used to hold the status of    */
  int vel_enable;           /* the individual moments of base data.     */
  int spw_enable;           /* TESTed after reading first radial        */
  
  int PROCESS=TRUE;         /* boolean used to control the daemon       */
                            /* portion of the program. Process until set*/
                            /* to false.                                */
  int i, j, ii, jj, kk;     /* loop variable                            */
  int count=0;              /* counter: number of radials in elevation  */
  short bin_size=250;       /* doppler bin size, default: 250 meter     */
  short elev_idx=0;         /* variable: used to hold current elev index*/
  float elevation=0.0;      /* float: used to hold float value of elev  */
  int opstatus;             /* variable:  used to hold return values    */
                            /* from API system calls                    */
  int *buffer;              /* pointer: access to allocated memory to   */
                            /* hold a completed ICD product             */
  short radial_status;      /* variable: status of each radial input    */
  char *radial;             /* pointer to radial structures             */
  short *ptr_data;          /* pointer to radial data                   */ 
  Base_data_header *ptr_header;/* pointer to radial header              */ 
   Siteadp_adpt_t site_adapt; /* site information struct */

  int valid_julian_day;	    /* valid time in julian day    		*/ 
  int valid_julian_time;    /* valid time since midnight in second	*/
  int julday;               /* data collection time in julian day       */
  int jultime;              /* data collection time since midnight      */
  int end_time;             /* end time of time window			*/
  int check_end_time;       /* pseudo end time of time window		*/
  int end_day;              /* day  of time window end			*/
  int forward_t;            /* forward distance from time window       */
  int backward_t;           /* backward distance from time window       */
  const int too_far = 300;  /* Threshhold to determine if the coming    *
                             * data is too far away forward from window*/
  int TIME_OK;		    /* logical to see if within time window     */
  int get_in_time_window=0; /* logical to see if ever got in time window*/
  float 
    elev_table[MAX_NUM_ELEV];/* elev table with size of MAX_NUM_ELEV	*/
  int num_elev=0;	    /* num of valid elements in elev table	*/
  float radius=0.0;	    /* radius of data bin			*/
  float dazimuth=0;	    /* azimuth deviation from base value	*/
  float baseangle=0;	    /* base azimuth angle			*/
  int ngatesmax=0;	    /* max num of data bins to consider		*/
  int num_bins=0;           /* real num of data bins to consider	*/
  float decoded_value=0.0;  /* decoded value of wind			*/
  float dtime=0.0;	    /* time deviation from valid time		*/
  int icell=0;		    /* azimuth index of cell			*/
  int jcell=0;		    /* radial index of cell			*/
  int itilt=0;		    /* tilt index of cell			*/
  int return_status=0;      /* hold the return values from a function   */
  int FIRSTTIME=TRUE;	    /* logical to control the time to read adapt*/ 
  int FIRSTRADIAL=TRUE;	    /* logical to set the first valid time      */ 

  /* test / debug variable declarations and definitions ----------------*/
  int TEST=FALSE;           /* test flag: set to true for diag messages */
  int rc = 0;               /* return code from function call           */

  /* Input Adaptation Data						*/
  int	rangemax=0;	    /* maximum distance to consider data        */
  int 	deltr=0;	    /* radial cell dimension			*/
  int   deltaz=0;    	    /* azimuth cell dimension			*/
  int	deltt=0;	    /* radius of time window in seconds		*/
  int   offset=0;           /* data collection time offset in seconds   */
  int	min_sample_size=0;  /* min num of ob to get a cell averaged wind*/
  float stalat=0.0;	    /* latitude of radar site			*/
  float stalon=0.0;         /* longitude of radar site			*/ 
  float stahgt=0.0;	    /* height of radar site 			*/

  int	nr=0;		    /* num of cell along radial			*/ 
  int	naz=0;              /* num of cell azimuths			*/

  /* Output argument lists       					*/
  double ***rangebar=NULL;  /* mean range of cell averaged wind (meters)*/
  double ***superob=NULL;   /* mean cell wind (meters/second)		*/
  double ***superob_square=NULL; /* mean cell wind square  			*/
  double ***superob_std=NULL;  /* mean cell wind standard deviation	*/
  double ***azimuthbar=NULL;   /* mean azimuth of cell averaged wind       */
  double ***tiltsbar=NULL;     /* mean tilt angle of cell averaged wind	*/
  double ***timebar=NULL;      /* mean time deviation from base time 	*/
  int   ***icount=NULL;	       /* num of bins considered to form a cell	*/
  position_t ***cell_pos=NULL;  /* position of each cell (lat, long, height)*/

  /* structure for adaptation data 					*/

  /************************Initializ the elevation table*****************/
  for(i=0;i< MAX_NUM_ELEV; i++)
  elev_table[i]=-999;

  RPGC_init_log_services( argc, argv );

  RPGC_log_msg( GL_INFO, "\nBegin Digital Velocity Algorithm in C\n");

  /* LABEL:REG_INIT  Algorithm Registration and Initialization Section  */
  
  /* register base data type COMBBASE as algorithm input, this input    */
  /* consists of radial data (a message for each individual radial      */
  RPGC_in_data(COMBBASE,RADIAL_DATA);

  /* output will be put in the linear buffer assigned using the constant*/
  /* SUPEROBVEL (see include file) which consists of volume data and the*/
  /* product code, PCODE (also defined within the include file)         */
  RPGC_out_data(SUPEROBVEL,VOLUME_DATA,PCODE);
  
  /* register algorithm infrastructure to read the Scan Summary Array   */
  RPGC_reg_scan_summary();

  /* register site adaptation data */
  rc = RPGC_reg_site_info( &site_adapt );
  if ( rc < 0 )
  {
    RPGC_log_msg( GL_ERROR, "SITE INFO: cannot register adaptation data callback function\n");
  }

  /* Register algorithm adaptation data */
  rc = RPGC_reg_ade_callback( superob_callback_fx,
                              &superob_adapt,
                              SUPEROB_DEA_NAME,
                              ON_CHANGE );
  if ( rc < 0 )
  {
    RPGC_log_msg( GL_ERROR, "superob: cannot register adaptation data callback function\n");
  }

  /* ORPG task initialization routine. Input parameters argc/argv are   */
  /* not used in this algorithm                                         */
  RPGC_task_init(ELEVATION_BASED,argc,argv);
  if(TEST)
  fprintf(stderr, "-> algorithm initialized and proceeding to loop control\n");

  /* while loop that controls how long the task will execute. As long as*/
  /* PROCESS remains true, the task will continue. The task will        */
  /* terminate upon an error condition or if a prespecified limit has   */
  /* been reached. 							*/
  while(PROCESS) {

  /* system call to indicate a data driven algorithm. block algorithm */
  /* until good data is received the product is requested             */
  RPGC_wait_act(WAIT_DRIVING_INPUT);

  /* LABEL:BEGIN_PROCESSING Released from Algorithm Flow Control Loop   */
  if(TEST)
  fprintf(stderr, "-> algorithm passed loop control and begin processing\n");

  /* Assign the adaptation data into its corresponding variables */
  if(FIRSTTIME==TRUE)
   {
   rangemax=superob_adapt.rangemax*ONE_THOUSAND; /* convert km to meters */
   deltr=   superob_adapt.deltr*ONE_THOUSAND;    /* convert km to meters */
   deltaz=  superob_adapt.deltaz;
   deltt=   superob_adapt.deltt*MINUTE2SECOND;      /* convert minutes to seconds*/
   offset=  superob_adapt.offset_min*MINUTE2SECOND; /* offset of data collection time, */
                                                    /* non-zero values produce product earlier */
   min_sample_size=superob_adapt.min_sample_size;
  
   /* get site information from site_adapt */
   stalat=site_adapt.rda_lat/ONE_THOUSAND;
   stalon=site_adapt.rda_lon/ONE_THOUSAND;
   stahgt=site_adapt.rda_elev*FOOT2METER;     /* convert feet to meters */
   if(TEST)
   fprintf(stderr,"lat=%f,lon=%f,elev=%f\n", stalat,stalon,stahgt);

   /* calculate nr,naz 							*/
   nr=rangemax/deltr+1;
   naz=TWO_PI/deltaz;
   if(TEST)
   fprintf(stderr,"nr=%d, naz=%d\n", nr, naz);

   /* dynamically allocate three-dimentional array for 
    * superob, superob_sqaure, superob_std, rangebar,azimuthbar, 
    * tiltsbar, timebar, icount, cell_pos*/
   rangebar=(double ***)calloc(naz, sizeof(double));
   superob=(double ***)calloc(naz, sizeof(double));
   superob_square=(double ***)calloc(naz, sizeof(double));
   superob_std=(double ***)calloc(naz, sizeof(double));
   azimuthbar=(double ***)calloc(naz, sizeof(double));
   tiltsbar=(double ***)calloc(naz, sizeof(double));
   timebar=(double ***)calloc(naz, sizeof(double));
   icount=(int ***)calloc(naz, sizeof(int));
   cell_pos=(position_t ***)calloc(naz, sizeof(position_t));
   if (  rangebar == NULL|| superob ==NULL||
        superob_std==NULL||azimuthbar ==NULL||tiltsbar==NULL||
        timebar==NULL||icount==NULL||cell_pos==NULL) {
            RPGC_log_msg( GL_ERROR, "Calloc Err: Calloc failed \n"); 
            RPGC_abort_because(PROD_MEM_SHED);
            continue;
        }

   for(i=0;i<naz;i++)
   {
   rangebar[i]=(double **)calloc(nr, sizeof(double)); 
   superob[i]=(double **)calloc(nr, sizeof(double));
   superob_square[i]=(double **)calloc(nr, sizeof(double));
   superob_std[i]=(double **)calloc(nr, sizeof(double)); 
   azimuthbar[i]=(double **)calloc(nr, sizeof(double)); 
   tiltsbar[i]=(double **)calloc(nr, sizeof(double)); 
   timebar[i]=(double **)calloc(nr, sizeof(double)); 
   icount[i]=(int **)calloc(nr, sizeof(int)); 
   cell_pos[i]=(position_t **)calloc(nr, sizeof(position_t)); 
   if (  rangebar[i]==NULL||superob[i]==NULL||
        superob_std[i]==NULL||azimuthbar[i]==NULL||tiltsbar[i]==NULL||
        timebar[i]==NULL||icount[i]==NULL||cell_pos[i]==NULL) {
         RPGC_log_msg( GL_ERROR, "Calloc Err: Calloc failed \n");
         RPGC_abort_because(PROD_MEM_SHED);
         goto algorithm_control_loop; /* since we are in a double loop    */
                                      /* 'continue' and 'break' won't work*/
        }
    for(j=0;j<nr;j++)
     {
     rangebar[i][j]=(double *)calloc(MAX_NUM_ELEV, sizeof(double)); 
     superob[i][j]=(double *)calloc(MAX_NUM_ELEV, sizeof(double)); 
     superob_square[i][j]=(double *)calloc(MAX_NUM_ELEV, sizeof(double)); 
     superob_std[i][j]=(double *)calloc(MAX_NUM_ELEV, sizeof(double)); 
     azimuthbar[i][j]=(double *)calloc(MAX_NUM_ELEV, sizeof(double)); 
     tiltsbar[i][j]=(double *)calloc(MAX_NUM_ELEV, sizeof(double)); 
     timebar[i][j]=(double *)calloc(MAX_NUM_ELEV, sizeof(double)); 
     icount[i][j]=(int *)calloc(MAX_NUM_ELEV, sizeof(int)); 
     cell_pos[i][j]=(position_t *)calloc(MAX_NUM_ELEV, sizeof(position_t)); 
     if (rangebar[i][j]==NULL||superob[i][j]==NULL||
        superob_std[i][j]==NULL||azimuthbar[i][j]==NULL||tiltsbar[i][j]==NULL||
        timebar[i][j]==NULL||icount[i][j]==NULL||cell_pos[i][j]==NULL) {
         RPGC_log_msg(GL_ERROR, "Calloc Err: Calloc failed \n");
         RPGC_abort_because(PROD_MEM_SHED);
         goto algorithm_control_loop;/*since we are in triploop         */
                                     /*'continue' and 'break' won't work*/
        }
     } /* END OF for(j=0;j<nr;j++)  */
   }   /* END OF for(i=0;i<naz;i++) */

  /* print out the contents of adaptation data for demonstration purposes */
   if(TEST)
   {
   fprintf(stderr,"->ADAPTATION DATA rangemax= %d\n",rangemax);
   fprintf(stderr,"->ADAPTATION DATA deltr= %d\n",deltr);
   fprintf(stderr,"->ADAPTATION DATA deltaz= %d\n",deltaz);
   fprintf(stderr,"->ADAPTATION DATA deltt= %d\n",deltt);
   fprintf(stderr,"->ADAPTATION DATA offset= %d\n",offset);
   fprintf(stderr,"->ADAPTATION DATA min_sample_size= %d\n",min_sample_size);
   }

   FIRSTTIME=FALSE;
  }/* END OF if(FIRSTTIME==TRUE) */

  /* LABEL:READ_FIRST_RAD Read first radial of the elevation and      */
  /* accomplish the required moments check                            */
  /* ingest one radial of data from the BASEDATA linear buffer. The */
  /* data will be accessed via the basedataPtr pointer              */
  basedataPtr=(Base_data_radial*)RPGC_get_inbuf(BASEDATA,&opstatus);

  /* check radial ingest status before continuing                   */
  if(opstatus!=NORMAL){
      RPGC_abort();        
      continue;
    }
  if(TEST)
  fprintf(stderr, "-> successfully read first radial buffer\n"); 
  
  /* test to see if the required moment (velocity) is enabled   */
  RPGC_what_moments((Base_data_header*)basedataPtr,&ref_enable,
                    &vel_enable,&spw_enable);

  if(vel_enable!=TRUE){
      RPGC_log_msg(GL_ERROR, "Err: Aborting from RPGC_what_moments\n");    
      RPGC_rel_inbuf((void*)basedataPtr);      
      RPGC_abort_because(PROD_DISABLED_MOMENT);    
      continue;
    }
  if(TEST) 
  fprintf(stderr, "-> required moments enabled\n");
     
  if(TEST)
  fprintf(stderr, "=> Setup the first valid time\n");
  /* The first valid time is set up here ONLY ONCE.              *
   * It is updated everytime when the time window is over        */ 
  if(FIRSTRADIAL)
   {
   Base_data_header *this_hdr=(Base_data_header*)basedataPtr;
   int this_julday;               /*data coolection day                    */
   int this_jultime;              /*data collection time in second         */

   /* get data collection day and time for this radial                     */
    this_julday=this_hdr->date;
    this_jultime=this_hdr->time/ONE_THOUSAND;/*convert millisecond to second*/

    create_new_window(this_jultime, this_julday, &valid_julian_time,
                      &valid_julian_day, &end_time, &end_day, deltt, offset);

    if(TEST)
     {
      fprintf(stderr, "Initial time window\n");
      fprintf(stderr, "center_time=%d, center_day=%d\n", valid_julian_time, valid_julian_day); 
      fprintf(stderr, "end_time=%d, end_day=%d\n", end_time, end_day);
     }
    /* Read out the bin size from the base data header                       */
    bin_size = this_hdr->dop_bin_size;
    /* calculate ngatesmax from bin_size                                     */
    ngatesmax=rangemax/bin_size;
    if(TEST)
    fprintf(stderr,"ngatesmax=%d, bin_size=%hd\n",ngatesmax, bin_size);

    /* set flag FIRSTRADIAL to FALSE 			  	             */
    FIRSTRADIAL=FALSE;

   } /* END of if(FIRSTRADIAL)                                               */

    if(TEST)
    fprintf(stderr, "-> begin ELEVATION PROCESSING SEGMENT\n");
    /* ELEVATION PROCESSING SEGMENT. continue to ingest and process          */
    /* individual base radials until either a failure to read valid          */
    /* input data (a radial in correct sequence) or until after reading      */
    /* and processing the last radial in the elevation                       */
    while(TRUE) {
      /* LABEL:PROCESS_RAD Here we process each radial individually          */
      /* any processing that can be accomplished without comparison          */
      /* between other radial can be accomplished here                       */
      
      /* redundant test information for quality control                      */
      if (TEST&&0) {
          fprintf(stderr, "\n  Base Data Information:\n");
          fprintf(stderr, "    Elevation=%f Index=%hd\n",basedataPtr->hdr.elevation,
            basedataPtr->hdr.rpg_elev_ind);
          fprintf(stderr, "    Azimuth=%f\n",basedataPtr->hdr.azimuth);
          fprintf(stderr, "    VCP=%hd\n",basedataPtr->hdr.vcp_num);
          fprintf(stderr, "    St Angle=%hd\n",basedataPtr->hdr.start_angle);
          fprintf(stderr, "    Radial Count=%d  opstatus=%d\n",count,opstatus);
          }

      /* get the current elevation angle & index from the incoming data */
      elev_idx=(short)RPGC_get_current_elev_index();
      elevation=basedataPtr->hdr.elevation; 

      /* for this algorithm...we'll copy the radial header and the      */
      /* velocity structure to the temporary storage array. This    */
      /* routine requires several steps. First, allocate enough memory  */
      /* to hold the radial header plus data. Then copy the header and  */
      /* radial data structure portion into the allocated space. The    */
      /* radial can then be accessed via the pointer in the radial array*/
      radial=(char *)malloc(sizeof(Base_data_radial));
      if (  radial == NULL) {
            RPGC_log_msg(GL_ERROR, "Malloc Err: malloc failed on radial\n");
            RPGC_rel_inbuf((void*)basedataPtr);
            break;
        }
      memcpy(radial,basedataPtr,sizeof(Base_data_header));
      memcpy(radial+sizeof(Base_data_header),
          &basedataPtr->vel,BASEDATA_DOP_SIZE*sizeof(short));

      /* test section: this optional block has been used to double check*/
      /* the integrity of the new radial memory block                   */
      if (TEST&&0) 
          {
          Base_data_header *hdr=(Base_data_header*)radial;
          fprintf(stderr, "  Radial Check:\n");
          fprintf(stderr, "    Azimuth=%f\n",hdr->azimuth);
          fprintf(stderr, "    Start Angle=%hd\n",hdr->start_angle);
          fprintf(stderr, "    Angle Delta=%hd\n",hdr->delta_angle);
          }

   
      /* read the radial status flag                                    */
      radial_status=basedataPtr->hdr.status & 0xf;
      if(TEST&&0) 
      fprintf(stderr, "    radial_status message=%d\n",radial_status);
      /* now that the data that we want from the radial is in temporary */
      /* storage...release the input buffer                             */
      RPGC_rel_inbuf((void*)basedataPtr);

      /* increment the radial counter */
      count++;

   /*------process this radial data------------------------------------*/
   /*
    * update the elevation table with this tilt
    * return num_elev, itilt and updated elev_table
    */
    return_status=
        superob_update_elev_table(elev_table, MAX_NUM_ELEV,
                                  &num_elev, &itilt, elevation, elev_idx);
    if(return_status==-1) 
        {
        free(radial);
        break;
        }

    if(TEST&&1==count)            /*Print out once in each elevation         */
    fprintf(stderr,"itilt=%d, elev_idx=%hd, elev angle=%f\n",itilt,elev_idx,elevation);

    /************Start processing data ************************************/
    /* Process data here, Process the radial data as a whole                 *
     * Read the time info from the header and see if it is in the time window*
     * if it is in the time window, accumulate data in each cell             *
     * if it reaches the end of time window,set next valid time window       *
     *                                     ,do the normalization             *
     *                                     ,calculate lat, lon, height       *
     * Creat final ICD product for superob                                   *
     * reinitialize some data arrays                                         */

    /* process one radial data 			                             */
    ptr_header=(Base_data_header *)radial;
    ptr_data=(short *)(radial+sizeof(Base_data_header));

    /* get collection day and time for this radial                          */
    julday=ptr_header->date;
    jultime=ptr_header->time/ONE_THOUSAND;/*convert millisecond to second   */

    /* set a temperary variable check_end_time to represent the end time    */
    if(end_day > julday) /* accrossing the midnight			    */
    check_end_time = end_time + DAY2SECOND;
    else
    check_end_time = end_time;

    if(jultime > check_end_time || julday > end_day) 
     {/* It reaches the end of time window      			   */
      if(TEST)
      {
      fprintf(stderr,"=> REACHED THE END OF TIME WINDOW\n"); 
      fprintf(stderr,"=> end_time=%d, check_end_time%d\n",end_time,check_end_time); 
      fprintf(stderr,"jultime=%d,julday=%d\n", jultime,julday);
      fprintf(stderr,"valid_julian_time=%d,valid_julian_day=%d,end_day=%d\n",
                      valid_julian_time,valid_julian_day, end_day);
      }

      /* normalize cell values by the num (icount) of                      *
       * bins to form a cell average                                       */
      if(TEST)
      fprintf(stderr,"START OF NORMALIZING DATA\n");
      for (ii=0;ii<naz;ii++)
       for (jj=0;jj<nr;jj++)
        for (kk=0;kk<num_elev;kk++)
         {
          if(icount[ii][jj][kk]> 0)  /* make sure icount is not equal 0   */
           {
            superob[ii][jj][kk]=superob[ii][jj][kk]/icount[ii][jj][kk];
            superob_square[ii][jj][kk]=superob_square[ii][jj][kk]/icount[ii][jj][kk];

            /* calculate standard deviation                               */
            superob_std[ii][jj][kk]=sqrt(fabs(superob_square[ii][jj][kk]-
                                    superob[ii][jj][kk]*superob[ii][jj][kk]));

            rangebar[ii][jj][kk]=rangebar[ii][jj][kk]/icount[ii][jj][kk];

            /* 'azimuthbar' is azimuth deviation at this point           */
            azimuthbar[ii][jj][kk]=azimuthbar[ii][jj][kk]/icount[ii][jj][kk];

            /* calculate cell mean azimuth, which is that                *
             * base elevation angle plus azimuth deviation               */ 
            baseangle = ii*deltaz;
            azimuthbar[ii][jj][kk]=baseangle+azimuthbar[ii][jj][kk];

            tiltsbar[ii][jj][kk]=tiltsbar[ii][jj][kk]/icount[ii][jj][kk];
            timebar[ii][jj][kk]=timebar[ii][jj][kk]/icount[ii][jj][kk];

            /* Calculate the lat, long, azimuth angle, height for each cell*/
            superob_path(rangebar[ii][jj][kk],azimuthbar[ii][jj][kk],
               tiltsbar[ii][jj][kk], stalat, stalon, stahgt, &cell_pos[ii][jj][kk]);
           }

        } /* END of for(kk=0;kk<num_elev;kk++)                              */

        if(TEST)
        fprintf(stderr,"END OF NORMALIZING DATA\n");

      /*  create the final product                                           */
      {
        int result;
        int length=0;
        int vol_num=0;

        /* allocate a partition (accessed by the pointer, buffer) within the  */
        /* SUPEROBVEL linear buffer. error return in opstatus                 */
        buffer=(int*)RPGC_get_outbuf(SUPEROBVEL,BUFSIZE,&opstatus);
        /* check error condition of buffer allocation. abort if abnormal      */
        if(opstatus!=NORMAL)
         {
          RPGC_log_msg(GL_ERROR,"Err: get output buffer\n");
          if(opstatus==NO_MEM)
          RPGC_abort_because(PROD_MEM_SHED);
          else
          RPGC_abort();
          free(radial);
          /* reinitialize to zero                                            */
          superob_initialize(superob,superob_std, superob_square, rangebar,azimuthbar,
                       tiltsbar,timebar,icount, naz, nr, &num_elev, elev_table);
          break;
         }

         if(TEST) 
           fprintf(stderr, "-> successfully obtained output buffer\n");
         /* make sure that the buffer space is initialized to zero            */
         clear_buffer((char*)buffer);

         /* get the current volume number using the following system call     */
         vol_num=RPGC_get_current_vol_num();

         /* building the ICD formatted output product requires a few steps    */
         /* step 1: build the product description block (pdb)                 */
         /* step 2: build the symbology block & data layer                    */
         /* step 3: complete missing values in the pdb                        */
         /* step 4: build the message header block (mhb)                      */
         /* step 5: forward the completed product to the system               */

         /* step 1: build the product description block -uses a system call   */
         if(TEST)
           fprintf(stderr, "\nCreating the pdb block now: vol=%d\n",vol_num);
         RPGC_prod_desc_block((void*)buffer,SUPEROBVEL,vol_num);

         /* step 2: build the Symbology layer.                              */
         /* this routine returns the overall length (thus far) of the       */
         /* product                                                         */
         if(TEST)
           fprintf(stderr, "begin building Symbology layer\n");
         build_symb_layer(buffer,icount,superob,superob_std,cell_pos,
                              timebar,&length,num_elev,naz,nr,
                              min_sample_size,elev_table);

         /* step 3: finish building the product description block by       */
         /* filling in certain values such as elevation index,             */
         /* accumulated product length, etc                  */
         finish_pdb(buffer,elev_idx,length,valid_julian_time,deltt,offset,
                    deltr,deltaz,rangemax,min_sample_size);

         /* generate the product message header (use system call) and input*/
         /* total accumulated product length minus 120 bytes               */
         result=RPGC_prod_hdr((void*)buffer,SUPEROBVEL,&length);
         if(TEST)
           fprintf(stderr, "-> completed product length=%d\n",length);
         /*(this routine adds the length of the product header to the      */
         /* "length" parameter prior to creating the final header)         */

         /* if the creation of the product has been a success              */
         if(result==0)
         {/*success                                                       */
          /* LABEL:OUTPUT_PROD    forward product and close buffer        */
          RPGC_rel_outbuf(buffer,FORWARD);
         }
         else    /* product failure (destroy the buffer & contents)        */
         {
          RPGC_log_msg(GL_ERROR, "product header creation failure\n");
          RPGC_rel_outbuf(buffer,DESTROY);
          RPGC_abort();
         }
      } /* END of of block creating final product   */

      /* set next valid time window                                        */
      if(TEST)
        fprintf(stderr,"=> START TO SET NEXT VALID TIMEWINDOW\n");

      /* calculate the forward time from time window                     */
      if((julday - end_day) ==1)  /* accross the mid-night               */
        {
         forward_t = jultime + 86400 - check_end_time;
        } 
       else if ((julday - end_day) >1) /* forward far away time window   */
        {
         forward_t = too_far+1;
        }
       else                   /* julday ==  end_day                     */ 
        {
         forward_t = jultime - check_end_time;
        }


      if (forward_t >= too_far)
       {

        /* send a message to indicate that jumping forward far awar     */
        LE_send_msg( GL_INFO, "JUMP FORWARD: RECREATE A TIME WINDOW!, forward_t=%d\n",forward_t );
        
        /* create a brand new time window                               */
        create_new_window(jultime, julday, &valid_julian_time,
                          &valid_julian_day, &end_time, &end_day, deltt, offset);
        if(TEST)
         {
          fprintf(stderr, "Forward far away from time window\n");
          fprintf(stderr, "center_time=%d, center_day=%d\n", valid_julian_time, valid_julian_day); 
          fprintf(stderr, "end_time=%d, end_day=%d\n", end_time, end_day);
         }
       }  /* end of if(forward_t >= too_far)                            */
      else   /* set a sequential time window                                */ 
       {
        valid_julian_time=valid_julian_time+2*deltt;/*shift the time window */
        end_time=valid_julian_time+deltt-offset;

        /* test to see if valid_julian_time >= 86400, across the midnight   */
        if(valid_julian_time >=DAY2SECOND)
        {       /* across the midnight                                      */
         valid_julian_day +=1;  /* increate the day by one                  */ 
         valid_julian_time=valid_julian_time % DAY2SECOND;
        }

        /*  test to see if end_time >=86400, across the midnight            */
        if(end_time >=DAY2SECOND)
         { /* across the midnight                                           */
          end_time=end_time % DAY2SECOND;
          end_day = end_day+1;
         }
        if(TEST)
         {
          fprintf(stderr, "Sequential time window\n");
          fprintf(stderr, "center_time=%d, center_day=%d\n", valid_julian_time, valid_julian_day); 
          fprintf(stderr, "end_time=%d, end_day=%d\n", end_time, end_day);
         }
       }

     /* reinitialize aarays to zero                                       */
     superob_initialize(superob,superob_std, superob_square, rangebar,azimuthbar,
                   tiltsbar,timebar,icount, naz, nr, &num_elev, elev_table);
     if(TEST)
     fprintf(stderr, "num_elev = %d, table[1]=%f, table[2]=%f\n", num_elev, elev_table[1], elev_table[2]);
     /* set the flag to false to indicate we are out of time window           */
     get_in_time_window = 0;
    
      if(TEST)
       fprintf(stderr,"Leaving REACHED END OF TIME WINDOW BLOCK\n");
    }   /* END of if(jultime > endtime)                                    */

    /* determine if this radial data is within the time window             */
    TIME_OK=superob_timeok(valid_julian_day,valid_julian_time,
                           julday,jultime, deltt, offset, &dtime);

    if(TIME_OK == TRUE)       /* it is within the time window             */
    {
      get_in_time_window = 1; /* set the flag to indicate that we got at  *
                               * the time window at least once  	  */
      if(TEST&&0) 
      fprintf(stderr,"=>TIME IS OK, calculate cell indexes\n");

      /* calculate azimuth index of each cell                             */
      icell=ptr_header->azimuth/deltaz;

      /* calculate azimuth deviation against base value                   */
      dazimuth = ptr_header->azimuth - deltaz*icell;

      icell =icell % naz;     /* make sure icell range is from 0 to naz   */

      /* There is not requirement for data above 70,000 feet MSL*
       * So we don't process data beyond 70,000 feet            *
       * n_dop_bins in the base data header determine the number*
       * of data bins which is valid                            *
       * So the num_bins is the smaller one of                  *
       * n_dop_bins and ngatesmax                               */
       if(ptr_header->n_dop_bins < ngatesmax)
        {
        num_bins = ptr_header->n_dop_bins;
        }
       else
        num_bins = ngatesmax; 

      for (j=0;j<num_bins;j++)/* loop over each bin in this radial       */
       {
        radius=(j+1)*bin_size;    /* calculate the distance from radar       */
        jcell=radius/deltr;    /* calculate the radial index of each cell */

        /* Test to see if the 'icell', 'jcell' and 'itilt' exceed limits  */
        if(icell>=naz||jcell>=nr||itilt>=MAX_NUM_ELEV)
        {
        RPGC_log_msg(GL_ERROR,"Err: array index exceed the max limit\n");
        RPGC_log_msg(GL_ERROR,"--->icell=%d, jcell=%d, itilt=%d\n",icell,jcell,itilt);
        free(radial);
        goto algorithm_control_loop;
        }
       /* decode the data based on the Doppler resolution                 */
        decoded_value=superob_decode(ptr_data[j],
                                      ptr_header->dop_resolution);
       
        /*  if the value is not -999.99 (0) or +999.99 (1)                *
         *  add on to accumulation for getting sum value for each cell    */
        if((-990.0<decoded_value) && (decoded_value <990.0))
         {
          icount[icell][jcell][itilt]=icount[icell][jcell][itilt]+1;
          superob[icell][jcell][itilt]=superob[icell][jcell][itilt]+ decoded_value;
          superob_square[icell][jcell][itilt]=superob_square[icell][jcell][itilt]+
                                            decoded_value*decoded_value;
          rangebar[icell][jcell][itilt]=rangebar[icell][jcell][itilt]+ radius;
          azimuthbar[icell][jcell][itilt]=azimuthbar[icell][jcell][itilt]+ dazimuth;
          tiltsbar[icell][jcell][itilt]=tiltsbar[icell][jcell][itilt]+ elevation ;
          timebar[icell][jcell][itilt]=timebar[icell][jcell][itilt]+ dtime ;
         }
       } /* END of for(j=0;j<ngatesmax;j++)                               */
     }   /* END of if(Time_OK)                                            */
    else
      {
       if(TEST&&0) 
        fprintf(stderr,"=>TIME IS NOT OK\n");

       /* set this flag to indicate we are out of time window             */
       get_in_time_window = 0;

       /* Check to see how far away backward we got from time window      */
       if ((valid_julian_day - julday) ==1) 
        {
         backward_t = valid_julian_time + 86400 - jultime; 
        }
       else if ((valid_julian_day - julday) > 1)
        {
         backward_t = HOUR2SECOND+1; 
        }
       else if (valid_julian_day - julday ==0)
        {
         backward_t = valid_julian_time - jultime; 
        }
       else 
        backward_t = 0;
       
       /* Check to see if it is backward out of time window                  */
       if (backward_t > HOUR2SECOND || get_in_time_window ==1)
        {
         /* send a message to indicate that jumping backward far awar     */
         LE_send_msg( GL_INFO, "JUMP BACKWARD: RECREATE A TIME WINDOW! backward_t=%d get=%d\n",
                                                backward_t, get_in_time_window );
 
         /* create a brand new time window                               */
         create_new_window(jultime, julday, &valid_julian_time,
                           &valid_julian_day, &end_time, &end_day, deltt, offset);
         if(TEST)
          {
           fprintf(stderr, "Backward far away from time window\n");
           fprintf(stderr, "center_time=%d, center_day=%d\n", valid_julian_time, valid_julian_day); 
           fprintf(stderr, "end_time=%d, end_day=%d\n", end_time, end_day);
          }
 
         /* reinitialize arays to zero                                       */
         superob_initialize(superob,superob_std, superob_square, rangebar,azimuthbar,
                   tiltsbar,timebar,icount, naz, nr, &num_elev, elev_table);

        }

       } /* END of else from if(Time_OK)  			          */      

     /* free each previously allocated radial pointer                     */
     free(radial);
      
      /* if end of elevation or volume then exit loop 			*/
      if(radial_status==GENDEL || radial_status==GENDVOL) 
          {
          if(TEST)
          fprintf(stderr, "-> End of elevation found\n");
          /* exit with opstatus==NORMAL and no ABORT 			*/
          break;
          }
     
      /* LABEL:READ_NEXT_RAD Read the next radial of the elevation      */
      /* ingest one radial of data from the BASEDATA linear buffer. The */
      /* data will be accessed via the basedataPtr pointer              */
      basedataPtr=(Base_data_radial*)RPGC_get_inbuf(BASEDATA,&opstatus);

      /* check radial ingest status before continuing                   */
      if(opstatus!=NORMAL)
        {
          RPGC_abort();
          break;
        }
    if(TEST&&0) 
    fprintf(stderr, "-> successfully read next radial buffer\n");      

      } /* END of while(TRUE)                                          */

  algorithm_control_loop:

    count=0; /* reset radial counter to 0                               */
    
    if(TEST)
    fprintf(stderr, "process reset: ready to start new elevation\n");

    } /* END of while PROCESS == TRUE 					*/

    /* free each previouly allocated array 				*/
    for(i=0;i<naz;i++)
       {
       for(j=0;j<nr;j++)
        {
        free(rangebar[i][j]);
        free(superob[i][j]);
        free(superob_square[i][j]);
        free(superob_std[i][j]);
        free(azimuthbar[i][j]);
        free(tiltsbar[i][j]);
        free(timebar[i][j]);
        free(icount[i][j]);
        free(cell_pos[i][j]);
        } 
       free(rangebar[i]); 
       free(superob[i]); 
       free(superob_square[i]); 
       free(superob_std[i]); 
       free(azimuthbar[i]); 
       free(tiltsbar[i]); 
       free(timebar[i]); 
       free(icount[i]); 
       free(cell_pos[i]); 
       }
    free(rangebar);
    free(superob);
    free(superob_square);
    free(superob_std);
    free(azimuthbar);
    free(tiltsbar);
    free(timebar);
    free(icount);
    free(cell_pos);
  RPGC_log_msg(GL_ERROR, "\nProgram Terminated\n");
return 0;
} /* END of main function                                              */


/************************************************************************
Description:    clear_buffer: initializes a portion of allocated 
                memory to zero
Input:          pointer to the input buffer (already cast to char*)
Output:         none
Returns:        none
Globals:        the constant BUFSIZE is defined in the include file
Notes:          none
************************************************************************/
void clear_buffer(char *buffer) {
  /* zero out the input buffer */
  int i;

  for(i=0;i<BUFSIZE;i++)
    buffer[i]=0;

  return;
  }

/**********************************************************************
Description: create_new__window: create a brand new window based on   *
             current radial time in stead of previous window          *
								      *
Input:      int radial_time, radial time (second)                     *
            int radial_day,  radial date (Julian day)                 *
            int deltt, radius of time_window                          *
            int offset, offset in seconds from top of the hour        *
Outputs:							      *
            int *center_time, center of time window                   *
            int *center_day,  center of time window in date           *
            int *end_time,    end of time window in second            *
            int *end_day,     end of time window in date              *
Returns:                                                              *
Globals:                                                              *
Notes:                                                                *
***********************************************************************/

void create_new_window(int radial_time, int radial_day,
                      int *center_time, int *center_day,
                      int *end_time,    int *end_day,
                      int deltt,        int offset)
{
   int valid_hour;                /*real valid time in hour                */
   int pseudo_valid_time;         /*it is "radial_time" + onehour         */

    pseudo_valid_time=radial_time+HOUR2SECOND; /* add one hour on it       */
    if(pseudo_valid_time >= DAY2SECOND) /* if it crosses midnight          */
     {
     pseudo_valid_time=pseudo_valid_time % DAY2SECOND;
     (*center_day)=radial_day+1;
     }
    else
     (*center_day) = radial_day;

    valid_hour=pseudo_valid_time/HOUR2SECOND;/* truncate the pseudo_valid_time *
                                              *into the top hour for example,*
                                              *midnight, 1 am, 2am .......  */
    (*center_time) =valid_hour*HOUR2SECOND;/*valid time in minutes         */

    (*end_day) = (*center_day);            /*time window accross midnight  */
    (*end_time)=(*center_time)+deltt-offset;

    /* if the time window accoss midnight                             */
    if((*end_time) >= DAY2SECOND)
    {
     (*end_time) = (*end_time) % DAY2SECOND;
     (*end_day) = (*end_day)+1;
    }


}
