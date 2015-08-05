/*
*  RCS info
*  $Author: steves $
*  $Date: 2014/03/18 14:40:39 $
*  $Locker:  $
*  $Id: user_sel_LRM.c,v 1.20 2014/03/18 14:40:39 steves Exp $
*  $Revision: 1.20 $
*  $State: Exp $
*  $Log: user_sel_LRM.c,v $
*  Revision 1.20  2014/03/18 14:40:39  steves
*  remove compiler warnings
*
*  Revision 1.19  2008/10/31 19:49:07  cmn
*  Checking in new updates received from BK - cjh for cc 10/31/08
*
*  Revision 1.18  2008/10/06 21:32:27  cmn
*  Check in fix for ROT issues 2039/2040 - cjh for cc 10/06/08
*
*  Revision 1.16  2006/08/20 13:41:33  cmn
*  Check in update - approved by RST 8/15/06 - cjh for ss 8/20/06
*
*  Revision 1.15  2006/07/25 20:31:49  cheryls
*  Fix for missing radials
*
*  Revision 1.14  2006/02/16 23:28:51  steves
*  issue 2-961
*
*  Revision 1.13  2006/01/03 17:09:45  steves
*  issue 2-744
*
*  Revision 1.12  2004/02/10 22:09:35  ccalvert
*  code cleanup
*
*  Revision 1.11  2004/02/05 22:57:08  ccalvert
*  new dea format
*
*  Revision 1.10  2004/01/27 00:58:03  ccalvert
*  make site adapt dea format
*
*  Revision 1.9  2004/01/12 16:52:42  steves
*  issue 2-282
*
*  Revision 1.8  2003/01/08 17:30:09  steves
*  issue 2-109
*
*  Revision 1.7  2002/09/12 17:47:50  steves
*  issue 1-987
*
*  Revision 1.6  2002/09/12 16:59:31  steves
*  issue 1-987
*
*  Revision 1.5  2002/09/12 15:58:41  steves
*  issue 1-987
*
*  Revision 1.4  2002/07/31 20:50:53  steves
*  issue 1-987
*
*  Revision 1.1  2002/02/28 19:04:06  nshen
*  Initial revision
*
*
*/


/****************************************************************
**
**	File:		user_sel_LRM.c
**	Author:		Ning Shen
**	Date:		December 6, 2001
**	
**	Description
**	===========
**
**	The algorithm generates the layer composite reflectivity
**	maximum product based on the user selected top and bottom
**	boundaries.
**	
**	The program handles maximum 10 user requests per volume scan.
**	The CCR number for this product is NA96-20601.
**
**	Inputs:
**		radials of base reflectivity data -- REFLDATA;
**		top and bottom boundaries -- user request message.
**	Output:
**		ICD-formatted layer product - 0xAF1F radial data packet.
**
**	Change History
**	==============
**
**	 
******************************************************************/

#include<rpgc.h>
  /************** rpgc.h includes following header files ******
  **
  ** en.h, basedata.h, malrm.h, rpg_globals.h
  ** 
  ** in rpg_globals.h there are many others:
  **
  ** stdio.h, stdlib.h time.h, ctype.h, string.h,
  ** basedata.h, rpg_port.h, prod_gen_msg.h, prod_request.h,
  ** a309.h, rpg_vcp.h, orpg.h, rpg.h, mrpg.h
  **
  */ 
#include<product.h>
#include<math.h>
#include<siteadp.h>

#include"user_sel_LRM.h"


#define LOG
#undef  LOG     /* Turn off log msg. To turn on, comment out this line. */
#define DEBUG
#undef  DEBUG   /* Turn off debug msg. To turn on, comment out this line. */ 

  /* 
  ** Estimated output buffer size in bytes. 
  ** the estimated buffer size consists of the following data:
  ** (pre ICD header       96 bytes not included)
  ** Msg Hdr/PDB           120 bytes
  ** Symb block             10 bytes
  ** Layer header            6 bytes
  ** Packet header          14 bytes
  ** Packet data         84960 bytes
  **              ((230 bins + 6 header bytes) * 360 radials)
  ** subtotal            85110 bytes 
  ** allocated size 90000 byte to allow overflow 
  */
#define OUT_BUF_SIZE 90000    

/************** local global variables ******************/

  /*                                
  ** This buffer holds reflectivity
  ** value for each layer product up
  ** to 10 products per volume.
  */
static short Polar_grid_buf[MAX_REQS][AZIMUTH][RANGE1]; 

  /*
  ** Structure to hold user request information.
  ** Request_list->ua_prod_code  = product_code
  ** Request_list->ua_dep_parm_0 = parameter 1
  ** Request_list->ua_dep_parm_1 = parameter 2
  ** Request_list->ua_dep_parm_2 = parameter 3
  ** Request_list->ua_dep_parm_3 = parameter 4
  ** Request_list->ua_dep_parm_4 = parameter 5
  ** Request_list->ua_dep_parm_5 = parameter 6
  ** Request_list->ua_elev_index = elevation index
  ** Request_list->ua_req_number = request number
  ** Request_list->ua_spare      = not used
  */
static User_array_t * Request_list;      

static boundary_t Boundary[MAX_REQS];    /* hold boundaries for each request */ 
 
static int Radar_height;     /* radar's elevation (feet above MSL) */
static int Request_count;    /* total number of the requests */
static int Volume_num;       /* sequential volume number (1 ~ 80) */
static int Max_refl;         /* max value of reflectivity of the volume */
static unsigned char Color_code[MAX_NUM_DBZ];  /* hold the 256 digital values */
static Siteadp_adpt_t site_adapt;

/************* local function prototypes ****************/

static void Init_color_table(unsigned char *);  
static void Initialize();
static void Process_control();
static void Validate_boundary();
static void Get_lower_upper_bins(Base_data_radial *, layer_t *);
static void Process_radial(Base_data_radial *, int, layer_t [], short);
static void Assemble_ICD(int);
static void Build_sym_block(unsigned char *, int *, int);
static int  Run_length_encode(int, int, packet_data_t *);
static void Finish_pdb(unsigned char *, int);
static void Free(void *);
static void Print_prod_header(unsigned char *, int);
static void Print_requests(int);
static int  Get_user_requests();
int user_sel_LRM_site_info_callback_fx( void * );


/*********************** main() **************************/

int main(int argc, char * argv[])
{
  int rc = 0; /* return code */

    /* register base data type REFLDATA as input */
  RPGC_in_data(REFLDATA, RADIAL_DATA); 

    /* register output linear buffer and volume based output */
  RPGC_out_data(ULR, VOLUME_DATA, PCODE);

    /* register algorithm infrastructure to read the Scan Summary table */
  RPGC_reg_scan_summary();

    /* register site adaptation data and get the radar height in feet */
  rc = RPGC_reg_site_info( &site_adapt );
  if ( rc < 0 )
  {
    RPGC_log_msg( GL_INFO, "SITE INFO: cannot register adaptation data callback function\n");
  }

    /* ORPG task initialization routine */
  RPGC_task_init(VOLUME_BASED, argc, argv);

    /*
    ** Initialize Color_code[] to 16 level color code. 
    ** Color code 1 is reserved for missing data 
    */
  Init_color_table(Color_code);
 
    /* Algorithm Control Loop(ACL) */
  while(TRUE)
  {
      /* initialize the global variables */
    Initialize();

    #ifdef DEBUG
      fprintf(stderr, "\n>main(): entering RPGC_wait_act()\n");
    #endif

      /* 
      ** system call to block the algorithm until good data is received
      ** and the product is requested
      */
    RPGC_wait_act(WAIT_DRIVING_INPUT);
    
      /* get radar height */
    Radar_height = site_adapt.rda_elev;

    #ifdef LOG
      fprintf(stderr, "\n>main(): unlocked from RPGC_wait_act()");
      fprintf(stderr, "\n>Radar height = %d ft\n", Radar_height); 
    #endif

      /* here is the entry point of the algorithm logic */
    Process_control();
  }   /* end of while(TRUE) control loop */

  return 0;

}   /* end of main() */


/************************* Init_color_table() ***********************
**
**  Description:	Initialize the Color_code[] to 16 color code.
**			Color code 1 is reserved for missing data
**			and color code 0 is for background color or
**			threshold code.
**  Input:		pointer to Color_code[]
**  Output:		Color_code[]
**  Returns:		No
**  Globals:		No
**  Notes:		This color table covers:
**
**			0 --> < -32 dBZ (0 ~ 2)
**			1 --> missing data
**			2 --> >= -32 dBZ (2)   and < -5 dBZ (56)
**			3 --> >=  -5 dBZ (56)  and <  0 dBZ (66)
**			4 --> >=   0 dBZ (66)  and <  5 dBZ (76)
**			5 --> >=   5 dBZ (76)  and < 10 dBZ (86)
**			6 --> >=  10 dBZ (86)  and < 15 dBZ (96)
**			7 --> >=  15 dBZ (96)  and < 20 dBZ (106)
**			8 --> >=  20 dBZ (106) and < 25 dBZ (116)
**			9 --> >=  25 dBZ (116) and < 30 dBZ (126)
**			10 -> >=  30 dBZ (126) and < 35 dBZ (136)
**			11 -> >=  35 dBZ (136) and < 40 dBZ (146)
**			12 -> >=  40 dBZ (146) and < 45 dBZ (156)
**			13 -> >=  45 dBZ (156) and < 50 dBZ (166)
**			14 -> >=  50 dBZ (166) and < 55 dBZ (176)
**			15 -> >=  55 dBZ (176)  
**
*/

void Init_color_table(unsigned char * table)
{
  int i;

  for(i = 0; i < MAX_NUM_DBZ; i++)
  {
    if(i < THRESHOLD)
    {
      table[i] = 0;       
    }
    else if((i >= THRESHOLD) && (i < MIN_DBZ_INDEX))
    {
      table[i] = 2;
    }
    else if((i >= MIN_DBZ_INDEX) && 
            (i < (MIN_DBZ_INDEX + 1 * INDEX_DELTA)))
    {
      table[i] = 3;
    }
    else if((i >= (MIN_DBZ_INDEX + 1 * INDEX_DELTA)) && 
            (i < (MIN_DBZ_INDEX + 2 * INDEX_DELTA)))
    {
      table[i] = 4;
    } 
    else if((i >= (MIN_DBZ_INDEX + 2 * INDEX_DELTA)) && 
            (i < (MIN_DBZ_INDEX + 3 * INDEX_DELTA)))
    {
      table[i] = 5;
    } 
    else if((i >= (MIN_DBZ_INDEX + 3 * INDEX_DELTA)) && 
            (i < (MIN_DBZ_INDEX + 4 * INDEX_DELTA)))
    {
      table[i] = 6;
    } 
    else if((i >= (MIN_DBZ_INDEX + 4 * INDEX_DELTA)) && 
            (i < (MIN_DBZ_INDEX + 5 * INDEX_DELTA)))
    {
      table[i] = 7;
    } 
    else if((i >= (MIN_DBZ_INDEX + 5 * INDEX_DELTA)) && 
            (i < (MIN_DBZ_INDEX + 6 * INDEX_DELTA)))
    {
      table[i] = 8;
    } 
    else if((i >= (MIN_DBZ_INDEX + 6 * INDEX_DELTA)) && 
            (i < (MIN_DBZ_INDEX + 7 * INDEX_DELTA)))
    {
      table[i] = 9;
    } 
    else if((i >= (MIN_DBZ_INDEX + 7 * INDEX_DELTA)) && 
            (i < (MIN_DBZ_INDEX + 8 * INDEX_DELTA)))
    {
      table[i] = 10;
    } 
    else if((i >= (MIN_DBZ_INDEX + 8 * INDEX_DELTA)) && 
            (i < (MIN_DBZ_INDEX + 9 * INDEX_DELTA)))
    {
      table[i] = 11;
    } 
    else if((i >= (MIN_DBZ_INDEX + 9 * INDEX_DELTA)) && 
            (i < (MIN_DBZ_INDEX + 10 * INDEX_DELTA)))
    {
      table[i] = 12;
    } 
    else if((i >= (MIN_DBZ_INDEX + 10 * INDEX_DELTA)) && 
            (i < (MIN_DBZ_INDEX + 11 * INDEX_DELTA)))
    {
      table[i] = 13;
    } 
    else if((i >= (MIN_DBZ_INDEX + 11 * INDEX_DELTA)) && 
            (i < (MIN_DBZ_INDEX + 12 * INDEX_DELTA)))
    {
      table[i] = 14;
    } 
    else if(i >= (MIN_DBZ_INDEX + 12 * INDEX_DELTA)) 
    {
      table[i] = 15;
    } 
  }   /* end of for(i = 0; i < MAX_NUM_DBZ; i++) */
}   /* end of Init_color_table() */


/************************* Initialize() **************************
**
**  Description:	initialize the local global variables.
**  Input:		No
**  Output:		No
**  Returns:		No
**  Globals:		Polar_grid_buf[][][], Request_list, 
**			Request_count, Max_refl, Boundary[].
**  Notes:		It runs everytime the control loop returns.
** 
*/

void Initialize()
{
  short i, j, k;

  for(i = 0; i < MAX_REQS; i++)
    for(j = 0; j < AZIMUTH; j++)
      for(k = 0; k < RANGE1; k++)
        Polar_grid_buf[i][j][k] = INIT_VALUE;
 
    /* free the memory allocated by RPGC_get_customizing_data() */
  Free((void *)Request_list);

  for(i = 0; i < MAX_REQS; i++)
  {
    Boundary[i].top = 0;
    Boundary[i].bottom = 0;
  }

  Request_count = 0;

  Max_refl = INIT_VALUE; 
}    /* end of Initialize() */

/************************ Process_control() ***********************
**
**  Description:	It reads in user requests and radial data, 
**			and processes them then assembles the final 
**			products. It returns the CONTINUE to the 
** 			calling function to continue the control loop.
**  Input:		No
**  Output:		No
**  Returns:		No	
**  Globals:		Request_list, Volume_num, Request_count,
**			Polar_grid_buf[][][], Color_code[].
**  Notes:
** 
*/

void Process_control()
{
  Base_data_radial * radial_data;     /* a pointer to a base data radial structure */
  layer_t bins[MAX_REQS];      /* array to hold start bin and end bin for each request */
  int ref_enable,
      vel_enable,
      spw_enable;
  int opstatus;  
  short pre_az = 0,            /* keep track of azimuth index in Polar_grid_buf */
        az;                    /* actual azimuth */
  short i;

    /* get user requests */
  if(Get_user_requests() == EXIT)
    return;

    /* loop to process whole volume for all the requests */
  while(TRUE)
  {
    short radial_status;
 
      /* get base radial data */
    radial_data = (Base_data_radial *)RPGC_get_inbuf(BASEDATA, &opstatus);

      /* check returned opstatus */
    if(opstatus != NORMAL)
    {
      RPGC_log_msg( GL_INFO, ">>>Process_control(): opstatus -> %d\n", opstatus);
      RPGC_log_msg( GL_INFO, ">>>Process_control(): Aborting from RPGC_get_inbuf()\n");
      RPGC_abort();  
      RPGC_log_msg( GL_INFO, ">>>Process_control(): called RPGC_abort()\n");
      return;
    }

      /* check to see if the reflectivity is enabled */
    RPGC_what_moments((Base_data_header *)radial_data, &ref_enable,
                    &vel_enable, &spw_enable);

    if(ref_enable != TRUE)
    {
      RPGC_log_msg( GL_INFO, ">>>Process_control(): Aborting from RPGC_what_moments()\n");
      RPGC_rel_inbuf((void *)radial_data);
      RPGC_log_msg( GL_INFO, ">>>Process_control(): released inbuf\n");
      RPGC_abort_because(PROD_DISABLED_MOMENT);
      RPGC_log_msg( GL_INFO, ">>>Process_control(): Aborted for disabled moment\n");
      return; 
    }

      /* 
      ** Check the header to see if this radial 
      ** is the beginning of the new volume or elevation.
      ** If so, calculate the beginning and ending bins for
      ** this elevation and every request.
      */
    radial_status =  radial_data->hdr.status;

    az = (short)(radial_data->hdr.azimuth) % AZIMUTH;   

    if((radial_status == BEG_VOL) || (radial_status == BEG_ELEV))
    {
      #ifdef DEBUG
        if(radial_status == BEG_VOL)
          fprintf(stderr, "\n>New VOLUME begins. Elevation --------> %.2f\n", 
                  radial_data->hdr.elevation);
        else
          fprintf(stderr, "\n>New ELEVATION cut begins. Elevation -------> %.2f\n", 
                  radial_data->hdr.elevation);
      #endif

      if(radial_status == BEG_VOL)
        Volume_num = RPGC_get_buffer_vol_num((void *)radial_data);

      Get_lower_upper_bins(radial_data, (layer_t *)&bins);  

        /* update pre_az */
      pre_az = az;  
    }
   
      /* 
      ** Radial processing loop goes through each radial
      ** for all the requests 
      */
    for(i = 0; i < Request_count; i++)
      Process_radial(radial_data, i, bins, pre_az); 

      /* update pre_az */
    pre_az = az;

      /* release input buffer */
    RPGC_rel_inbuf((void *)radial_data);
      
      /* check to see if it is end of volume */
    if(radial_status == END_VOL) 
    {
      #ifdef DEBUG   
        fprintf(stderr, "\n>End of volume scan\n");
      #endif
      
      break;
    } 
  }   /* end of while(TRUE) volume loop */

    /* loop to assemble final products for each request */ 
  for(i = 0; i < Request_count; i++)
    Assemble_ICD(i);
 
  return; 
}    /* end of Process_control() */ 

/*********************** Get_user_requests() ****************************
**
**  Description:	It calls RPGC_get_customizing_data() to read in
**			user requests. It validate the user requests to 
**			make sure the layer boundaries are in the right
**			range.	
**  Input:		No
**  Output:		No
**  Returns:		EXIT on no request, CONTINUE otherwise.	
**  Globals:		Request_list, Request_count	
**  Notes:	
**
*/

int Get_user_requests()
{
  int total_in_count = 0;      /* total number of requests being read in */
  int match_all = -1;

    /* Call ORPG API to get user requests */
  Request_list = (User_array_t *)RPGC_get_customizing_data(match_all, &total_in_count);

    /* if there is no request for this product, return */ 
  if(Request_list == NULL)
  {
    RPGC_log_msg( GL_INFO, ">>>Process_control(): no request found\n"); 
    return EXIT;
  }
 
  #ifdef DEBUG 
    fprintf(stderr, "\n>Process_control(): read in requests");
    fprintf(stderr, "\n>total requests -> %d\n", total_in_count); 
  #endif

    /* get the number of the requests not exceeding MAX_REQS */
  if(total_in_count > MAX_REQS)
    Request_count = MAX_REQS;
  else
    Request_count = total_in_count;

    /* validate user requests */
  Validate_boundary();  

  #ifdef LOG
    fprintf(stderr, "\n Process_control(): number of requests --> %d\n", Request_count);
    for(i = 0; i < Request_count; i++)
    {
      fprintf(stderr, "\n>Process_control(): after request validation\n");

      Print_requests(i);
    }
  #endif

  return CONTINUE;
}    /* end of Get_user_requests() */


/*********************** Validate_boundary() ***************************
**
**  Description:	It validates the boundaries user requested to
**			make sure they are in right range(>= Radar's
**			elevation and <= 70000 ft) with layer depth is 
**			not less than the minimum depth of 1000 feet.
**  Input:		No
**  Output:		No
**  Returns:		No
**  Globals:		Request_list, Boundary[], Radar_height, Request_count
**  Notes:		After this validation, all the boundaries in Boundary[]
**			are above ground level converted from MSL.
**
*/

void Validate_boundary()
{
  int i; 
  int b1, b2;
  int radar_elev;
  int roundup = 500;   /* 500 feet */

    /* roundup radar's height (in feet) */ 
  if(Radar_height >= THOUSAND)
  {
    if((Radar_height % THOUSAND) < roundup)
      radar_elev = Radar_height - (Radar_height % THOUSAND); 
    else
      radar_elev = Radar_height + (THOUSAND - (Radar_height % THOUSAND)); 
  }
  else
  {
    if((Radar_height % THOUSAND) < roundup)
      radar_elev = BOTTOM_LIMIT; 
    else
      radar_elev = THOUSAND; 
  }
 
    /* 
    ** Validate the layer boundaries for all requests.
    ** By substracting the radar's height from the boundaries,
    ** all the boundaries after validating are the level above 
    ** the the ground level.
    */
  for(i = 0; i < Request_count; i++)
  {
    if(Request_count > 1)
    {
      b1 = Request_list[i].ua_dep_parm_0 * THOUSAND;
      b2 = Request_list[i].ua_dep_parm_1 * THOUSAND;
    }
    else
    {
      b1 = Request_list->ua_dep_parm_0 * THOUSAND;
      b2 = Request_list->ua_dep_parm_1 * THOUSAND;
    }

    if(b1 < b2)
    {
      if(b1 >= TOP_LIMIT)   /* both > TOP_LIMIT */
      {
        Boundary[i].top = TOP_LIMIT;
        Boundary[i].bottom = Boundary[i].top - INTERVAL;
      }
      else if(b2 <= radar_elev)   /* both < BOTTOM_LIMIT */
      {
        Boundary[i].bottom = BOTTOM_LIMIT;
        Boundary[i].top = INTERVAL;       
      }
      else
      {
        if(b1 <= radar_elev)
          Boundary[i].bottom = BOTTOM_LIMIT;
        else 
          Boundary[i].bottom = b1 - radar_elev;
       
        if(b2 >= TOP_LIMIT)
          Boundary[i].top = TOP_LIMIT;
        else 
          Boundary[i].top = b2 - radar_elev;
      }
    }
    else if(b1 > b2) 
    {
      if(b2 >= TOP_LIMIT)   /* both > TOP_LIMIT */
      {
        Boundary[i].top = TOP_LIMIT;
        Boundary[i].bottom = Boundary[i].top - INTERVAL;
      }
      else if(b1 <= radar_elev)   /* both < BOTTOM_LIMIT */
      {
        Boundary[i].bottom = BOTTOM_LIMIT;
        Boundary[i].top = INTERVAL;
      }
      else
      {
        if(b2 <= radar_elev)
          Boundary[i].bottom = BOTTOM_LIMIT;
        else
          Boundary[i].bottom = b2 - radar_elev;
    
        if(b1 >= TOP_LIMIT)
          Boundary[i].top = TOP_LIMIT;
        else
          Boundary[i].top = b1 - radar_elev;
      }
    }
    else     /* b1 == b2 */ 
    {
      if(b1 >= TOP_LIMIT)
      {
        Boundary[i].bottom = TOP_LIMIT - INTERVAL;
        Boundary[i].top = TOP_LIMIT;
      }
      else if(b1 <= radar_elev)
      {
        Boundary[i].bottom = BOTTOM_LIMIT;
        Boundary[i].top = INTERVAL;
      }
      else
      {
        Boundary[i].bottom = b1 - radar_elev;
        Boundary[i].top = Boundary[i].bottom + INTERVAL;
      }
    }   /* end of if(b1 < b2) else */ 
  }   /* end of for() loop */
}    /* end of Validate_boundary() */ 


/********************** Get_lower_upper_bins() ************************
**
**  Description:	It calculates the beginning bin and ending bin 
**			for each elevation and each request.
**  Input:		Pointer to Base_data_radial input buffer.
**  Output:		Array of layer_t type which contains beginning
**			and ending bins for each user request at a
**			particular elevation cut.
**  Returns:		No	
**  Globals:		Boundary[], Request_count.
**  Notes:		It runs only at the beginning of new volume or
** 			beginning of each elevation cut.
**
*/

void Get_lower_upper_bins(Base_data_radial * ptr, layer_t * layer)
{
  short index = 0;
  int height1, height2, ran1, ran2;
  int first_bin_range, bin_size; 
  float temp1, temp2, temp3;
  float elev;

    /* get bin size and the range to the first bin in meters */
  bin_size = ptr->hdr.surv_bin_size; 
  first_bin_range = ptr->hdr.surv_range * bin_size;

    /*
    ** The equation used to calculate range is as follows.
    ** H = SR * Sin(E) + SR * SR /(2 * IR * RE)
    ** SR = -IR * RE * Sin(E) + sqrt((IR * RE * Sin(E)) * (IR * RE * Sin(E)) +
    **                                                     2 * IR * RE * H);
    ** where
    **       H --- height
    **       SR -- slant range
    **       E --- elevation
    **       IR = 1.21
    **       RE = 6371 km
    */

  temp1 = IR * RE * THOUSAND;    /* in meter */

    /* calculate starting bin and ending bin for each request */
  for( index = 0; index < Request_count; index++)
  {
    height1 = Boundary[index].bottom;
    height2 = Boundary[index].top;   

      /*
      ** Get elevation angle for the center of the beam.
      ** For calculating starting bin, add the half of the
      ** beam width to the center angle.
      ** For calculating ending bin, subtract the half of
      ** the beam width from the center angle.
      */
    if(height1 == BOTTOM_LIMIT) 
    {
      layer->start_bin = 1;
    }
    else
    {
      elev = ptr->hdr.elevation + HALF_BW; 
 
      temp2 = (float)(sin((double)(elev * DEGREE_TO_RADIAN)));
      temp3 = temp2 * temp2 + 2 * height1 * M_PER_FT / temp1;
      ran1 = temp1 * ((float)(sqrt((double)temp3)) - temp2); 

      if(ran1 >= (RANGE2 * THOUSAND))  /* the elevation is below the layer */ 
      {
        layer->start_bin = (short)(-1);
        layer->end_bin = (short)-1;
        layer++;
        continue;  
      }
      else
        layer->start_bin = (short)((ran1 - first_bin_range)/bin_size) + 1;
    }  /* end of if(height == BOTTOM_LIMIT) */

    if(ptr->hdr.elev_num == 1)     /* first cut */
    { 
      elev = ptr->hdr.elevation + HALF_BW; 
 
      temp2 = (float)(sin((double)(elev * DEGREE_TO_RADIAN)));
      temp3 = temp2 * temp2 + 2 * height2 * M_PER_FT / temp1;
      ran1 = temp1 * ((float)(sqrt((double)temp3)) - temp2); 

      elev = ptr->hdr.elevation;

      temp2 = (float)(sin((double)(elev * DEGREE_TO_RADIAN)));
      temp3 = temp2 * temp2 + 2 * height2 * M_PER_FT / temp1;
      ran2 = temp1 * ((float)(sqrt((double)temp3)) - temp2); 
      
      ran2 = ran2 + (ran2 - ran1);
    }
    else
    {
      elev = ptr->hdr.elevation - HALF_BW;

      if(elev <= 0)
        elev = SMALL_VALUE;

      temp2 = (float)(sin((double)(elev * DEGREE_TO_RADIAN)));
      temp3 = temp2 * temp2 + 2 * height2 * M_PER_FT / temp1;
      ran2 = temp1 * ((float)(sqrt((double)temp3)) - temp2); 
    }
 
    if(ran2 > (RANGE2 * THOUSAND))
      ran2 = RANGE2 * THOUSAND;

    layer->end_bin = (short)((ran2 - first_bin_range)/bin_size) + 1;
    if (layer->end_bin > ptr->hdr.n_surv_bins-1)
       layer->end_bin = ptr->hdr.n_surv_bins-1;  
   
    #ifdef DEBUG
      fprintf(stderr, "\n>Get_lower_upper_bins(): ");
      fprintf(stderr, "\n>height1 = %d, height2 = %d", height1, height2); 
      fprintf(stderr, "\n>ran1 = %d meter, ran2 = %d meter, start_bin = %d, end_bin = %d\n",
              ran1, ran2, layer->start_bin, layer->end_bin);
    #endif

      /* advance the pointer to next 4 bytes */
    if(index < (Request_count - 1))
      layer++; 
  }   /* end of for() loop */ 
}    /* end of Get_lower_upper_bins() */

/*********************** Process_radial() ****************************
**
**  Description:	It projects each radial bin, which is between 
**			the beginning and ending bins, to the 
**			Polar_grid_buf and keeps the maximum value.
**  Inputs:		data -- pointer to input radial data,
**               	req_index -- index of request
**	         	bin -- array of starting bin and ending bin for 
**		        each request
**  Outputs:		No
**  Returns:		No
**  Globals:		Polar_grid_buf[][][]
**  Notes:
**      	
*/

void Process_radial(Base_data_radial * data, int req_index, layer_t bin[MAX_REQS], short pre_az)
{
  short i, az;
  short starting_bin, ending_bin;
  short value, range;
  short bin_size = data->hdr.surv_bin_size;
  short az_delta = 0;

  starting_bin = bin[req_index].start_bin;
  ending_bin = bin[req_index].end_bin;

  if(starting_bin >= RANGE2)
    starting_bin = RANGE2 - 1;

  if(ending_bin >= RANGE2)
    ending_bin = RANGE2 - 1;

    /*
    ** check starting_bin. If starting_bin < 0, 
    ** this elevation cut is lower than the bottom
    ** boundary for this request. Retrun. 
    */
  if(starting_bin < 0)
    return;

    /* get the azimuth in polar coordinate */
  az = (short)(data->hdr.azimuth) % AZIMUTH;

    /* 
    ** Handle fat radials (missing slots), the missing slot get
    ** backfilled with current radial only upto 2 degrees in width.
    ** Added by Ning Shen on July 24, 2006 
    */
  if (az != pre_az)
  {
    if (az < pre_az)
      az_delta = az + AZIMUTH - pre_az;

    else
      az_delta = az - pre_az;

      /* only fill the 2 degree fat radial */
    if (az_delta < 3 && az_delta > 1)
    {
      int temp_az = pre_az;
      int count = az_delta;
 
      while (count > 0)
      {
        count--;

        temp_az = (temp_az + 1) % AZIMUTH;

        for(i = starting_bin; i <= ending_bin; i++)
        {
          value = data->ref[i];

            /* To handle spot blanking - added Sept. 15, 2008 */
          if(data->hdr.spot_blank_flag & SPOT_BLANK_RADIAL)
            value = 0;

          if((value > (MAX_NUM_DBZ - 1)) || (value < 0))
          value = 0;

            /*
            ** In case that the number of bins in a particular
            ** radial might be less than the number calculated
            ** at the start of elevation in the function
            ** Get_lower_upper_bins(). -- added on Oct. 28, 2008.
            */
          if(i > data->hdr.n_surv_bins - 1)
            value = 0;

          range = (short)((((i * bin_size + data->hdr.surv_range * bin_size)
                             * data->hdr.cos_ele) / THOUSAND));

          if(range < RANGE1)
          {
            if(Polar_grid_buf[req_index][temp_az][range] < value)
              Polar_grid_buf[req_index][temp_az][range] = value;
          }
          else
            break;
        }  /* end of for(i = starting_bin; i <= ending_bin; i++) loop */
      }  /* end of while (count > 0) */ 
    }  /* end of if (az_delta < 3 && az_delta > 1) */ 
  }  /* end of handle fat radial block if(az != pre_az) */
   
    /* project each radial bin value to Polar_grid_buf[][][] */
  for(i = starting_bin; i <= ending_bin; i++)
  {
    value = data->ref[i];

      /* To handle spot blanking - added Sept. 15, 2008 */
    if(data->hdr.spot_blank_flag & SPOT_BLANK_RADIAL)
      value = 0;

    if((value > (MAX_NUM_DBZ - 1)) || (value < 0))
      value = 0;

      /*
      ** In case that the number of bins in a particular
      ** radial might be less than the number calculated
      ** at the start of elevation in the function
      ** Get_lower_upper_bins(). -- added on Oct. 28, 2008.
      */
    if(i > data->hdr.n_surv_bins - 1)
      value = 0;

    range = (short)((((i * bin_size + data->hdr.surv_range * bin_size) 
                      * data->hdr.cos_ele) / THOUSAND));  

    if(range < RANGE1)    
    {
      if(Polar_grid_buf[req_index][az][range] < value)
        Polar_grid_buf[req_index][az][range] = value;
    }
    else
      break;    
  }  /* end of for() loop */  
}   /* end of Process_radial() */   

/******************** Assemble_ICD() ***************************
**
**  Description:	It assembles the final ICD format product
**			then forwards the product to database.
**  Input:		user request index
**  Output:		No
**  Returns:		CONTINUE on success, EXIT otherwise.	
**  Globals:		Volume_num, Request_list.
**  Notes:
**	
*/

void Assemble_ICD(int req_index)
{
  int opstat, length = 0; 
  unsigned char * out_buf_ptr;
 
    /* get output buffer */
  if(Request_count > 1)
    out_buf_ptr = (unsigned char *)RPGC_get_outbuf_for_req(ULR, OUT_BUF_SIZE, 
                                                         &Request_list[req_index],
                                                         &opstat);
  else
    out_buf_ptr = (unsigned char *)RPGC_get_outbuf_for_req(ULR, OUT_BUF_SIZE, 
                                                         Request_list,
                                                         &opstat);
  if(opstat != NORMAL)
  {
    RPGC_log_msg( GL_INFO, ">>>Assemble_ICD(): aborting from RPGC_get_outbuf_for_req(). opstat = %d\n",
            opstat);

    /* Once RPGC_abort_request() function is available, it should be here */
    RPGC_abort_request( &Request_list[req_index], opstat ); 
    return;
  } 
 
  #ifdef LOG 
    fprintf(stderr, "\n>Creating the PDB for request %d and volume %d\n", 
            req_index, Volume_num); 
  #endif

    /* system call to build the PDB */
  RPGC_prod_desc_block((void *)out_buf_ptr, ULR, Volume_num);

    /* build product symbology block */
  Build_sym_block(out_buf_ptr, &length, req_index);

    /* finish the product description block */
  Finish_pdb(out_buf_ptr, req_index);

    /* system call to build product message header */
  opstat = RPGC_prod_hdr((void *)out_buf_ptr, ULR, &length); 

  #ifdef LOG
    fprintf(stderr, "\n>Completed product length = %d\n", length);
  #endif

  if(opstat == 0)   /* success */
  {
    #ifdef LOG
      Print_prod_header(out_buf_ptr, req_index);
    #endif

      /* forward product and close output buffer */
    RPGC_rel_outbuf((void *)out_buf_ptr, FORWARD); 
  }
  else              /* failure  */
  {
    RPGC_log_msg( GL_INFO, ">>>Assemble_ICD(): product header creation failure\n");
    RPGC_rel_outbuf((void *)out_buf_ptr, DESTROY);

    /* Once RPGC_abort_request() is available, it should be here */
    RPGC_abort_request( &Request_list[req_index], opstat ); 
  }

  return;
}    /* end of Assemble_ICD() */

/********************* Build_sym_block() *****************************
**
**  Description:	It builds the symbology block.
**	
**  Inputs:		pointer to output buffer,
**	   		request index to Request_list 
**  Output: 		length of symbology block
**  Returns:		No
**  Globals:		Polar_grid_buf[][][]
**  Notes:
**
*/

void Build_sym_block(unsigned char * out_buf, int * length, int req_index)
{
  int i, k, num_hw;
  int offset = 0;
  unsigned char * ptr = out_buf;
  unsigned int temp;

  sym_block_t   sym_block;       /* product symbology structure */
  packet_hdr_t  p_hdr;           /* packet header structure */
  packet_data_t p_data;          /* packet layer data structure */

    /* get the sym block header information */
  sym_block.block_divider = (short)-1;
  sym_block.block_ID = (short)1;

    /*
    ** sym block length: 16 bytes + packet header 12 + (230 + 6) * radials 360
    ** However, each radial may contain different number of halfwords
    ** due to the run length encoding. So the block_length and the layer_length
    ** should be updated after each radial packet data being processed.
    ** Here only the initial value is assigned to the block_length. 
    */  
  temp = PCKT_DATA_ENTRY - SYM_BLK_ENTRY; 
  RPGC_set_product_int( (void *) &sym_block.block_length, temp );
  sym_block.num_layer = (short)1;
  sym_block.layer_divider = (short)-1;

    /* get the packet header */
  p_hdr.packet_code = PACKET_CODE;  
  p_hdr.first_bin_index = 0;
  p_hdr.num_bins = RANGE1;      /* 230 */
  p_hdr.icenter = 0;
  p_hdr.jcenter = 0;
  p_hdr.scale_factor = 1000;
  p_hdr.num_radials = AZIMUTH;  /* 360 */

    /* copy the packet header(14 bytes) into the output buffer */
  memcpy(ptr+PCKT_HDR_ENTRY, &p_hdr, sizeof(packet_hdr_t));

    /* get the initial value of the offset */
  offset = PCKT_DATA_ENTRY;

    /* process packet data radial by radial */
  for(i = 0; i < AZIMUTH; i++)
  {
      /* run length encoding */
    num_hw = Run_length_encode(req_index, i, &p_data);
    
    k = 2 * num_hw;
    
      /* copy data packet into output buffer */
    memcpy(ptr+offset, &p_data, (k + PCKT_DATA_HDR));

      /* update the sym_block.block_length */
    temp += (k + PCKT_DATA_HDR);
    RPGC_set_product_int( (void *) &sym_block.block_length, temp );

      /* update the offset */
    offset += (k + PCKT_DATA_HDR);
 
  }  /* end of for(i = 0; i < AZIMUTH; i++) */

    /* get the actual sym_block.layer_length */ 
  temp -= sizeof(sym_block_t);
  RPGC_set_product_int( (void *) &sym_block.layer_length, temp );

    /* copy the symbology block header(16 bytes) into the output buffer */ 
  memcpy(ptr+SYM_BLK_ENTRY, &sym_block, sizeof(sym_block_t)); 

    /* set total length of symbology block */
  *length = offset - SYM_BLK_ENTRY;
 
}    /* end of Build_sym_block() */ 


/************************** Finish_pdb() *****************************
**
**  Description:	This function finishes the rest part of the Product 
**			Description Block.
**  Input:		pointer to output buffer, request index
**  Output:		No
**  Returns:		No
**  Globals:		Request_list	
**  Notes:
**
*/

void Finish_pdb(unsigned char * out_buf, int req_index)
{
  Graphic_product * hdr = (Graphic_product *)out_buf;

    /* enter threshold values */
  hdr->level_1 = (short)TH1;
  hdr->level_2 = (short)TH2;
  hdr->level_3 = (short)TH3;
  hdr->level_4 = (short)TH4;
  hdr->level_5 = (short)TH5;
  hdr->level_6 = (short)TH6;
  hdr->level_7 = (short)TH7;
  hdr->level_8 = (short)TH8;
  hdr->level_9 = (short)TH9;
  hdr->level_10 = (short)TH10;
  hdr->level_11 = (short)TH11;
  hdr->level_12 = (short)TH12;
  hdr->level_13 = (short)TH13;
  hdr->level_14 = (short)TH14;
  hdr->level_15 = (short)TH15;
  hdr->level_16 = (short)TH16;

    /* decoding to dBZ value */
  Max_refl = Max_refl / 2 - 33;

    /* enter maximum reflectivity, bottom, and top boundaries */
  hdr->param_4 = (short)Max_refl;

  if(Request_count > 1)
  {
    hdr->param_1 = Request_list[req_index].ua_dep_parm_0;
    hdr->param_2 = Request_list[req_index].ua_dep_parm_1;
  }
  else
  {
    hdr->param_1 = Request_list->ua_dep_parm_0;
    hdr->param_2 = Request_list->ua_dep_parm_1;
  }

    /* actual boundaries used for internal calculation */
  hdr->param_5 = (short)(Boundary[req_index].bottom / THOUSAND);
  hdr->param_6 = (short)(Boundary[req_index].top / THOUSAND);

    /* enter number of blocks in the message */
  hdr->n_blocks = (short)3;

    /* enter ICD block offset in number of halfword */
  RPGC_set_product_int( (void *) &hdr->sym_off, SYM_BLK_ENTRY / 2 );
  RPGC_set_product_int( (void *) &hdr->gra_off, 0 );
  RPGC_set_product_int( (void *) &hdr->tab_off, 0 );

    /* reset Max_refl to INIT_VALUE for next request */
  Max_refl = INIT_VALUE;
 
}  /* end of Finish_pdb() */


/*********************** Run_length_encode() *************************
**
**  Description:	Run length encoding the radial bins to the ICD
**			formatted packet data.
**  Input:		request index -- req_index,
**			azimuth index -- az 
**  Output:		packet data buffer -- packet 
**  Returns:		number of halfwords in packet data structure 
**  Globals:		Polar_grid_buf[][][], Color_code[], Max_refl 
**  Notes:		Color code 1 is reserved for missing data.
**			Missing data are indicated by -99 in 
**			Polar_grid_buf[][]][].
**
*/

int Run_length_encode(int req_index, int az, packet_data_t * packet)
{
  short run = 1, code = 0;
  short m_run = 1, m_code = 1;
  short i, index, k = 0;
  short data_flag = FALSE, missing_flag = FALSE;
  short pad = 0x0;
  
    /* get packet data header */
  packet->start_angle = (short)(az * 10);  /* scaled */ 
  packet->angle_delta = (short)10;         /* scaled */ 

    /* loop to process each radial bin */
  for(i = 0; i < RANGE1; i++)
  {
      /* get bin value */
    index = Polar_grid_buf[req_index][az][i];

      /* get maximum reflectivity */
    if(Max_refl < index)
      Max_refl = index;

    if(index >= 0)   /* normal data */
    {
      if(missing_flag)    /* previous bin(s) contains no data */
      {
        m_run = (m_run << 4);
        packet->data[k++] = (unsigned char)(m_run + m_code);
        m_run = 1;
        missing_flag = FALSE; 
      }

      if((i == 0) || (data_flag == FALSE))
      {
        code = Color_code[index];    /* get color code */
        run = 1;
      }
      else if((code == Color_code[index]) && (run < (MAX_NUM_CCODE - 1)))
      {
        run++;
      }
      else
      {
        run = (run << 4);
        packet->data[k++] = (unsigned char)(run + code);

          /* update code and run */
        code = Color_code[index];
        run = 1; 
      }

        /* set data_flag */
      data_flag = TRUE; 
    }
    else             /* missing data */
    {
      if(data_flag)    /* previous bin contains normal data */
      {
        run = (run << 4);
        packet->data[k++] = (unsigned char)(run + code);

          /* update run and set data_flag to FALSE */
        run = 1;
        data_flag = FALSE;
      }

      if(missing_flag)    /* previous bin contains no data */
      {
        if(m_run < (MAX_NUM_CCODE - 1))
          m_run++;
        else
        {
          m_run = (m_run << 4);
          packet->data[k++] = (unsigned char)(m_run + m_code);
          m_run = 1;
        }        
      }
      else
        m_run = 1;

        /* set missing_flag */
      missing_flag = TRUE;

    }   /* end of if(index >= 0) else */
 
  }  /* end of for(i = 0; i < RANGE1; i++) */

    /* handle the end of loop condition */
  if(missing_flag)
  {
    m_run = (m_run << 4);
    packet->data[k] = (unsigned char)(m_run + m_code);
    m_run = 1;
  }
  else if(data_flag)
  {
    run = (run << 4);
    packet->data[k] = (unsigned char)(run + code);
    run = 1;
  }
    
    /* make an even bytes ( k starts at 0) */
  if((k % 2) == 0)
  {
    k += 1;
    packet->data[k] = pad;  /* background color */
  }
  
    /* get number of halfwords for packet data header */
  packet->num_2bytes = (short)((k + 1) / 2);  

  return packet->num_2bytes;
}   /* end of Run_length_encode() */

/********************** Free() ***************************************
**
**  Description:	It frees memory pointed by ptr.
**  Input:		void * ptr.
**  Output:		No
**  Retruns:		No
**  Globals:		No
**  Notes:		
**
*/

void Free(void * ptr)
{
  if(ptr != NULL)
  {
    free(ptr);
    ptr = NULL;
  }
}   /* end of Free() */

/********************** Print_prod_header() **************************
**
**  Description:	It prints the information about the ICD header.
**  Input:		pointer to output buffer, request index
**  Output:		No
**  Returns:		No
**  Globals:		No
**  Notes:
**
*/

void Print_prod_header(unsigned char * out_buf, int req_index)
{
  Graphic_product * hdr = (Graphic_product *)out_buf;

  fprintf(stderr, "\n\n\tULR Product Header Information for Rquest %d\n", req_index);
  fprintf(stderr, "\n\t----------------------------------------------\n");
  fprintf(stderr, "\n\tMessage Header:\n");
  fprintf(stderr, "\n\t(1)Message code --------------------> %d", hdr->msg_code);
  fprintf(stderr, "\n\t(2)Message date --------------------> %d", hdr->msg_date);
  fprintf(stderr, "\n\t(3&4)Message time ------------------> %d", hdr->msg_time);
  fprintf(stderr, "\n\t(5&6)Message length ----------------> %d", hdr->msg_len);
  fprintf(stderr, "\n\t(7)Sender ID -----------------------> %d", hdr->src_id);
  fprintf(stderr, "\n\t(8)Receiver ID ---------------------> %d", hdr->dest_id);
  fprintf(stderr,"\n\t(9)Number of blocks ----------------> %d", hdr->n_blocks);

  fprintf(stderr, "\n\n\tProduct Description Block:\n");
  fprintf(stderr, "\n\t(10)Divider ------------------------> %d", hdr->divider);
  fprintf(stderr, "\n\t(11&12)Radar latitude --------------> %d", hdr->latitude);
  fprintf(stderr, "\n\t(13&14)Radar longitude -------------> %d", hdr->longitude);
  fprintf(stderr, "\n\t(15)Radar height -------------------> %d", hdr->height);
  fprintf(stderr, "\n\t(16)Product code(msg_code) ---------> %d", hdr->prod_code);
  fprintf(stderr, "\n\t(17)Weather mode -------------------> %d", hdr->op_mode);

  if(hdr->op_mode == 0)
    fprintf(stderr, " --- maintenance mode");
  else if(hdr->op_mode == 1)
    fprintf(stderr, " --- clear air mode");
  else if(hdr->op_mode == 2)
    fprintf(stderr, " --- precipitation mode");

  fprintf(stderr, "\n\t(18)VCP ----------------------------> %d", hdr->vcp_num);
  fprintf(stderr, "\n\t(19)Request sequence number --------> %d", hdr->seq_num);
  fprintf(stderr, "\n\t(20)Volume counter -----------------> %d", hdr->vol_num);
  fprintf(stderr, "\n\t(21)Volume date --------------------> %d", hdr->vol_date);
  fprintf(stderr, "\n\t(22)Volume time(MS2B) --------------> %d", hdr->vol_time_ms);
  fprintf(stderr, "\n\t(23)Volume time(LS2B) --------------> %d", hdr->vol_time_ls);
  fprintf(stderr, "\n\t(24)Product generation date --------> %d", hdr->gen_date);
  fprintf(stderr, "\n\t(25&26)Product generation time -----> %d", hdr->gen_time);
  fprintf(stderr, "\n\t(27)Parameter 1(bottom in kft) -----> %d", hdr->param_1);
  fprintf(stderr, "\n\t(28)Parameter 2(top in kft) --------> %d", hdr->param_2);
  fprintf(stderr, "\n\t(29)RPG elevation index ------------> %d", hdr->elev_ind);
  fprintf(stderr, "\n\t(30)Parameter 3 --------------------> %d", hdr->param_3);
  fprintf(stderr, "\n\t(31)Threshold 1 --------------------> %x", hdr->level_1);
  fprintf(stderr, "\n\t(32)Threshold 2 --------------------> %x", hdr->level_2);
  fprintf(stderr, "\n\t(33)Threshold 3 --------------------> %x", hdr->level_3);
  fprintf(stderr, "\n\t(34)Threshold 4 --------------------> %x", hdr->level_4);
  fprintf(stderr, "\n\t(35)Threshold 5 --------------------> %x", hdr->level_5);
  fprintf(stderr, "\n\t(36)Threshold 6 --------------------> %x", hdr->level_6);
  fprintf(stderr, "\n\t(37)Threshold 7 --------------------> %x", hdr->level_7);
  fprintf(stderr, "\n\t(38)Threshold 8 --------------------> %x", hdr->level_8);
  fprintf(stderr, "\n\t(39)Threshold 9 --------------------> %x", hdr->level_9);
  fprintf(stderr, "\n\t(40)Threshold 10 -------------------> %x", hdr->level_10);
  fprintf(stderr, "\n\t(41)Threshold 11 -------------------> %x", hdr->level_11);
  fprintf(stderr, "\n\t(42)Threshold 12 -------------------> %x", hdr->level_12);
  fprintf(stderr, "\n\t(43)Threshold 13 -------------------> %x", hdr->level_13);
  fprintf(stderr, "\n\t(44)Threshold 14 -------------------> %x", hdr->level_14);
  fprintf(stderr, "\n\t(45)Threshold 15 -------------------> %x", hdr->level_15);
  fprintf(stderr, "\n\t(46)Threshold 16 -------------------> %x", hdr->level_16);
  fprintf(stderr, "\n\t(47)Parameter 4 (Max Refl in dBZ) --> %d", hdr->param_4);
  fprintf(stderr, "\n\t(48)Parameter 5 (bottom in kft) ----> %d", hdr->param_5);
  fprintf(stderr, "\n\t(49)Parameter 6 (top in kft) -------> %d", hdr->param_6);
  fprintf(stderr, "\n\t(50)Parameter 7 --------------------> %d", hdr->param_7);
  fprintf(stderr, "\n\t(51)Parameter 8 --------------------> %d", hdr->param_8);
  fprintf(stderr, "\n\t(52)Parameter 9 --------------------> %d", hdr->param_9);
  fprintf(stderr, "\n\t(53)Parameter 10 -------------------> %d", hdr->param_10);
  fprintf(stderr, "\n\t(54)Number of maps -----------------> %d", hdr->n_maps);
  fprintf(stderr, "\n\t(55&56)Symbology block offset ------> %d", hdr->sym_off);
  fprintf(stderr, "\n\t(57&58)Graphic block offset --------> %d", hdr->gra_off);
  fprintf(stderr, "\n\t(59&60)Tabular block offset --------> %d", hdr->tab_off);
  fprintf(stderr, "\n");

}   /* end of Print_prod_header() */


/******************** Print_requests() ***************************
**
**  Description:	It prints out the contents of the Request_list 
**  			and Boundary[].
**  Input:		request index
**  Output:		No
**  Retruns:		No
**  Globals:		Request_list, Boundary[]
**  Notes:
**
*/

void Print_requests(int index)
{
  fprintf(stderr, "\n\n\tUser Requests\n");
  fprintf(stderr, "\t-------------\n");

  if(Request_count > 1)
  {
    fprintf(stderr, "\n\tP_code --------------> %d", Request_list[index].ua_prod_code); 
    fprintf(stderr, "\n\tparam1 --------------> %d", Request_list[index].ua_dep_parm_0);
    fprintf(stderr, "\n\tparam2 --------------> %d", Request_list[index].ua_dep_parm_1);
    fprintf(stderr, "\n\tparam3 --------------> %d", Request_list[index].ua_dep_parm_2);
    fprintf(stderr, "\n\tparam4 --------------> %d", Request_list[index].ua_dep_parm_3);
    fprintf(stderr, "\n\tparam5 --------------> %d", Request_list[index].ua_dep_parm_4);
    fprintf(stderr, "\n\tparam6 --------------> %d", Request_list[index].ua_dep_parm_5);
    fprintf(stderr, "\n\televation index -----> %d", Request_list[index].ua_elev_index);
    fprintf(stderr, "\n\trequest number ------> %d", Request_list[index].ua_req_number);
  }
  else
  {
    fprintf(stderr, "\n\tP_code --------------> %d", Request_list->ua_prod_code); 
    fprintf(stderr, "\n\tparam1 --------------> %d", Request_list->ua_dep_parm_0);
    fprintf(stderr, "\n\tparam2 --------------> %d", Request_list->ua_dep_parm_1);
    fprintf(stderr, "\n\tparam3 --------------> %d", Request_list->ua_dep_parm_2);
    fprintf(stderr, "\n\tparam4 --------------> %d", Request_list->ua_dep_parm_3);
    fprintf(stderr, "\n\tparam5 --------------> %d", Request_list->ua_dep_parm_4);
    fprintf(stderr, "\n\tparam6 --------------> %d", Request_list->ua_dep_parm_5);
    fprintf(stderr, "\n\televation index -----> %d", Request_list->ua_elev_index);
    fprintf(stderr, "\n\trequest number ------> %d", Request_list->ua_req_number);
  }

  fprintf(stderr, "\n");

  fprintf(stderr, "\n\tbottom -----> %d", Boundary[index].bottom);
  fprintf(stderr, "\n\ttop --------> %d", Boundary[index].top);
  fprintf(stderr, "\n"); 
}    /* end of Print_requests() */


/******************** end of user_selectable_LRM.c ********************/

