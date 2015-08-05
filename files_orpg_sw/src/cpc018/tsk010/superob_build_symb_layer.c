/*
 * $revision$
 * $state$
 * $Logs$
 */


/***************************************************************************
 *Module:       superob_build_symb_layer.c (SuperOb algorithm)             *
 *									   *
 *Description:  this module contains two routines. the first is used to    *
 *              construct the product Symbology Block layer    	           *
 *              packet. the second is used to complete the construction of *
 *              the product description block (which is a part of the ICD  *
 *              product header)						   *
 *Input:        buffer     pointer to the output buffer   		   *
 *              icount   number of bin to form a cell                      * 
 *              superob  mean wind in each cell                            * 
 *              superob_std   standar deviation of mean wind               * 
 *              int ***cell_pos   struct for position of cell              *
 *		timebar  time deviation from base time 	                   *
 *		azimuthbar mean azimuth angle			           *
 *Output:       length is used to return the accumulated length of         *
 *              the constructed product                                    *
 *Returns:      none							   * 
 *Notes:         							   * 
 ***************************************************************************/

#include "superob_build_symb_layer.h"
#include "superob.h"

  void 
  build_symb_layer(int* buffer,int ***icount,double ***superob,
  double ***superob_std, position_t ***cell_pos, double ***timebar,
  int *length,int num_elev,int naz,int nr,
  int min_sample_size, float elev_table[]) 
  {
  /* build the symbology layer      		                        */ 

  typedef struct {
  int lat;             /* latitude of each cell				*/
  int lon;             /* longitute of each cell			*/
  short height;        /* heigh of each cell				*/
  short wind;          /* mean wind of each cell			*/
  short wind_std;      /* std of mean wind at each cell			*/
  short dtime;         /* time deviation from base time			*/
  unsigned short azimuth; /* azimuth angle of each cell			*/
  } superob_cell;
  
  /* variable declarations and definitions ---------------------------- */
  const short packet_code=27; /* packet code				*/
  short elev_angle;     /* elevation angle				*/

  sym_block sym;          /* symbology block struct                     */
  int bytes_per_block=2;   /* Size of each block in bytes default = 2   */
  int size_valid_superob=0;/* size of valid superob data in bytes       */
  superob_cell superob_cells;/*data struct of spuerob cell		*/
  int num_points=0;       /* num of valid superob cells in one elevation*/

  char* ptr=NULL;           /* character pointer to storage buffer      */
  int i,j,k;                /* loop counters                            */
  int offset=0;             /* pointer offset into output buffer        */
  int block_offset=0;       /* offset for block size into output buffer */

  /* treat the int* buffer as a char* buffer to more easily access byte */
  /* size words */
  ptr=(char*)buffer;

  sym.divider=(short)-1;      /* block divider (constant value)         */
  sym.block_id=(short)1;      /* block ID=1 (constant value)            */

  /* block length: depend on data lenth, so only known after store all data*/
  RPGC_set_product_int( (void *) &sym.block_length, 0 );
  sym.num_layers=(short)1;    /* number of data layers included         */
  sym.divider2=(short)-1;     /* block divider (constant value)         */

  /* layer length: depend on data length, so only known after store all data*/
  RPGC_set_product_int( (void *) &sym.layer_length, 0 );

  /* copy the symbology block header (16 bytes) into the output buffer  */
  memcpy(ptr+120,&sym,16);

  /* Message Header and PDB use 120 Bytes + 16 bytes symb. block header */
  offset=120+16;

  /* Output data of all elevations in ICD format 27                     */
  for(k=0;k<num_elev;k++)
   {
   memcpy(ptr+offset,&packet_code,2);
   offset+=2;
   block_offset = offset;/*which is to be used to update bytes_per_block*/
   /*Right now bytes_per_block=2, which will be updated a few line below*/
   RPGC_set_product_int( (void *) ptr+offset, (unsigned int) bytes_per_block );
   offset+=4; 

   /* elevation angle s multiplied by '10' and roundly truncated so that *
    * the precission is 0.1 degree                                       */ 
    elev_angle=(short)((elev_table[k]+0.05)*10);
   /* copy elevation angle to memory                                   */
    memcpy(ptr+offset,&elev_angle,2);
    offset+=2;

   for(i=0;i<naz;i++)
     for(j=0;j<nr;j++)
      {
      if(icount[i][j][k]>=min_sample_size)
      {
      /* the data precision for lat/lon is 0.001 degree, round truncate*/
      /* for example, 35.6324=35.632, -97.1625-097.163		       */
      superob_cells.lat=(int)((cell_pos[i][j][k].lat+0.0005)*1000); 
      superob_cells.lon=(int)((cell_pos[i][j][k].lon+0.0005)*1000); 
      /*date precision for the hight is 1 m, round truncate           */
      superob_cells.height=(short)(cell_pos[i][j][k].height+0.5); 
      /* data precision for the average wind is 0.01 m/s, round truncate*/
      superob_cells.wind=(short)((superob[i][j][k]+0.005)*100); 
      /* data precision for STD of mean wind is 1 m/s, round truncate */
      superob_cells.wind_std=(short)(superob_std[i][j][k]+0.5); 
      /* data precision for time deviation is 1 second, round truncate */
      superob_cells.dtime=(short)(timebar[i][j][k]+0.5); 

      /* data precision for mean azimuth is 0.01 degree, round truncate*/
      /* convert azimuth angle range -180~+180 degree to 0 ~ 359.9 degree*/
      if(cell_pos[i][j][k].azimuth < 0)
      superob_cells.azimuth=(unsigned short)((cell_pos[i][j][k].azimuth+360+0.005)*100); 
      else
      superob_cells.azimuth=(unsigned short)((cell_pos[i][j][k].azimuth+0.005)*100);

      /* make sure data will fit in buffer			      */
      if(offset+sizeof(superob_cell)>BUFSIZE)
        {
         RPGC_log_msg(GL_ERROR,"ERR: Superob product truncated. Increase Buffer size\n");
         return;
        } 

      /* copy data struct superob_cells to memory                       */
      /* sizeof(superob_cell) is '20',                                  * 
       * but real size of superob_cell is '18'                          *
       * This is why we deduct '2' from sizeof(superob_cell)            */ 
      memcpy(ptr+offset,&superob_cells,(sizeof(superob_cell)-2));

      /* For latitude and longitude, must make sure integers are stored according to ICD. */
      RPGC_set_product_int( (void *) (ptr+offset), (unsigned int) superob_cells.lat );
      RPGC_set_product_int( (void *) (ptr+offset+4), (unsigned int) superob_cells.lon );

      offset=offset+sizeof(superob_cell)-2;

      num_points++;

      } /* END of if(icount[i][j][k]>min_sample_size)                   */
  
      } /* END of for(j=0;j<nr;j++)                                     */

    if(num_points !=0)  /* there are valid cells in this elevation      */
    {
    /* total size of data in bytes at this elevation                    */
    size_valid_superob=num_points*(sizeof(superob_cell)-2);

    /* bytes_per_block = size_valid_superob + size of elevation angle(2 bytes)*/
    bytes_per_block=size_valid_superob+2; 
     
    /* update the memory value of bytes_per_block now		       */ 
    RPGC_set_product_int( (void *) (ptr+block_offset), 
                          (unsigned int) bytes_per_block );

    /* initialize num_points  to zero and assign bytes_per_block with 2*/
    num_points=0;
    bytes_per_block = 2;

    } /* END of if(num_points !=0)                                     */

  }  /* END of for(k=0;k<num_elev;k++)                                */

    /* calculate the length of message                                 */
    *length=offset-120;

    /* update block length and layer length now                        */
    RPGC_set_product_int( (void *) &sym.block_length, *length );
    /* layer length: block_length - 16 (sym block length)              */
    RPGC_set_product_int( (void *) &sym.layer_length, *length-16 ); 

    /* copy the updated symbology block into the output buffer         */
   memcpy(ptr+120,&sym,16);

  }


/************************************************************************
Description:    finish_pdb fills in values within the Graphic_product
                structure which were not filled in during the system call
Input:          int* buffer: pointer to the output buffer
                short elev_index: observed elevation index
                float elevation: floating point elevation value
Output:         completed buffer with ICD formatting
Returns:        none
Globals:        none
Notes:          none
************************************************************************/

void 
finish_pdb(int* buffer,short elev_ind, int prod_len, int valid_julian_time,
           int deltt,int offset,int deltr,int deltaz,int rangemax,int min_sample_size)
  {
  /* complete entering values into the product description block (pdb)
     Enter: threshold levels, product dependent parameters, version and
     block offsets for symbology, graphic and tabular attribute.        */

  /* cast the msg header and pdb block to the pointer hdr               */
  Graphic_product* hdr=(Graphic_product*)buffer;

  /* enter the data threshold values                                    */
  hdr->level_1=(short)0;
  hdr->level_2=(short)0;
  hdr->level_3=(short)0;
  hdr->level_4=(short)0;
  hdr->level_5=(short)0;
  hdr->level_6=(short)0;
  hdr->level_7=(short)0;
  hdr->level_8=(short)0;
  hdr->level_9=(short)0;
  hdr->level_10=(short)0;
  hdr->level_11=(short)0;
  hdr->level_12=(short)0;
  hdr->level_13=(short)0;
  hdr->level_14=(short)0;
  hdr->level_15=(short)0;
  hdr->level_16=(short)0;

  /* product dependent parameters
     these will follow the base products 
     (pdp1)=base time        (pdp2)= Time Radius
     (pdp4)=cell range size  (pdp5)= Cell azimuth size
     (pdp6)=maximum range    (pdp7)= minimum num of points       */
  hdr->param_1=(short)(valid_julian_time/60); /* convert it to minutes  */
  hdr->param_2=(short)(deltt/60);             /* convert it to minutes  */
  hdr->param_3=(short)(offset/60);            /* convert it to minutes  */
  hdr->param_4=(short)(deltr/1000);           /* convert it to km       */
  hdr->param_5=(short)(deltaz);
  hdr->param_6=(short)(rangemax/1000);       /* convert it to km       */
  hdr->param_7=(short)(min_sample_size);
  

  /* number of blocks in product = 3                                    */
  hdr->n_blocks=(short)3;

  /* message length                                                     */
  RPGC_set_product_int( (void *) &hdr->msg_len, prod_len );

  /* ICD block offsets                                                  */
  RPGC_set_product_int( (void *) &hdr->sym_off, 60 );
  RPGC_set_product_int( (void *) &hdr->gra_off, 0 );
  RPGC_set_product_int( (void *) &hdr->tab_off, 0 );

  /* elevation index                                                    */
  hdr->elev_ind=(short)elev_ind;

  return;
  }


