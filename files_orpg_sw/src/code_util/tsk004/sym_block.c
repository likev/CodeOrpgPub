/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:56 $
 * $Id: sym_block.c,v 1.8 2009/05/15 17:37:56 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/* sym_block.c */

#include "sym_block.h"

extern msg_data md;


int print_symbology_header(char *buffer) {
  /* display the symbology block header */
  Sym_hdr *sh=malloc(10);

  if(sh==NULL) {
    fprintf(stderr,"ERROR: Unable to allocate memory\n");
    return(FALSE);
  }

  if(buffer==NULL) {
     fprintf(stderr,"ERROR: The product buffer is empty\n");
     free(sh);
     return(FALSE);
  }

  if(md.symb_offset==0) {
     fprintf(stderr,"Symbology Offset is 0 indicating no block available\n");
     free(sh);
     return(TRUE);
  }

 /* symbology block header */
  memcpy(sh,buffer+216,10);
/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
       MISC_short_swap(sh,5);
       sh->block_length=INT_SSWAP(sh->block_length);
#endif
	 

  fprintf(stderr,"\n\n");
  fprintf(stderr,"***************************************************\n");
  fprintf(stderr,"******** SYMBOLOGY BLOCK HEADER (10 bytes) ********\n");
  fprintf(stderr,"***************************************************\n");
  fprintf(stderr,"        ----------------------- SYMBOLOGY BLOCK HEADER  ------------------------\n");
  fprintf(stderr,"                                                        Decimal       Hexadecimal\n");
  fprintf(stderr,"61      divider\t\t\t\t\t\t%-10hd or 0x%04hX\n",sh->divider,sh->divider);
  fprintf(stderr,"62      blockID\t\t\t\t\t\t%-10hd or 0x%04hX\n",sh->blockID,sh->blockID);
  fprintf(stderr,"63-64   blocklength\t\t\t\t\t%-10d or 0x%08X\n",sh->block_length,sh->block_length);
  fprintf(stderr,"65      num layers\t\t\t\t\t%-10hd or 0x%04hX\n\n",sh->n_layers,sh->n_layers);

      
  free(sh);
  return(TRUE);
  
} /*  end print_symbology_header */



 
int print_symbology_block(char *buffer,int *flag) {
  /* print out the contents of the symbology block (after the header). */
  
  /*flag array components:                                                      */
  /* see corresponding enumerated types in cvt.h                                */
  /* ----------------------------------------------------------------           */
  /* comments also in cvt_dispatcher.c                                          */
  /* ----------------------------------------------------------------           */
  /*   index 0 typecode        {NOPART,RADIAL,ROW,GAB,TAB,TABV,SATAP}           */
  /*   index 1 start field     integer val of start                             */
  /*   index 2 end field       0 or integer val of end                          */
  /*   index 3 'all' flag      {FALSE,TRUE}                                     */
  /*   index 4 degree flag     {FALSE,TRUE}                                     */
  /*   index 5 format          {NOMOD,RLE,BSCAN}                                */
  /* CVG 4.4 - added -1 for generic product */
  /*   index 6 layer#          {0:all   -1:generic   or layer # in symb block}  */
  /*   index 7 scale data      {1=R, 2=0.5m/s V, 3=1m/s V, 4=SW}                */
  /* CVT 4.4 */
  /*                           (5=FDECODE, 6=PDECODE                            */
  /*   index 8 component list  {LIST_NONE,LIST_ALL,LIST_AREA,LIST_RAD           */
  /*                            LIST_TEXT,LIST_TABLE,LIST_GRID,LIST_EVENT)      */
  /*   index 9 component print {PRINT_NONE,PRINT_ALL,PRINT_AREA,PRINT_RAD       */
  /*                            PRINT_TEXT,PRINT_TABLE,PRINT_GRID,PRINT_EVENT)  */
  /*   index 10 component idx  {integer val of component index}                 */
  /*   index 11 prod param print  {FALSE,TRUE}                                  */
  /*   index 12 comp param print  {FALSE,TRUE}                                  */
  /*                                                                            */
  /* flag[0] specifies the product block (GAB, TAB, TABV) to display            */
  /*         or that the product is a radial / raster data packet.              */
  /*                                                                            */
  /* flag[1] beginning radial / row (0 is not specified)                        */
  /*                                                                            */
  /* flgg[2] end radial / row (0 is not specified)                              */
  /*                                                                            */
  /* flag[3] specifies that all radials / rows be displayed                     */
  /*                                                                            */
  /* flag[4] specified whether the radial selectors are in degrees              */
  /*                                                                            */
  /* flag[5] requsts either the bscan output of radial data (flag[5] == 2) or   */
  /*         leaving radial or raster data in RLE (flag[5] == 1).               */
  /*                                                                            */
  /* flag[6] specifies the symbology layer to display 'layer X',                */
  /*         0 means all layers, -1 means generic product 'generic'             */
  /*                                                                            */
  /* For data packet 16:                                                        */
  /* flag[7] designates data decoding for reflectivity, 0.5 m/s velocity,       */
  /*         1.0 m/s velocity and spectrum width using the formulas are from    */
  /*         the base data ICD. The default is no decoding, flag[7] == 0. Using */
  /*         the modifier 'scaleX' (scaler, scalev1, scalev2, scalesw) will     */
  /*         apply the scaling / decode routine to each bin prior to display.   */
  /* For data packet 16 and the generic radial component:                       */
  /* CVT 4.4 */
  /* flag[7] designates data decoding using the Scale Offset parameters.        */
  /*         'pdecode' stipulates using parameters from within the product      */
  /*         'fdecode' stipulates using parameters from a user supplied file    */
  /*                                                                            */
  /* flag[8] designates how many decimal places are selected for the decoded    */
  /*         value.  (-1 not specified)                                         */
  /*                                                                            */
  /* flag[9] designates which components of a generic product should be listed  */
  /*         'list_all' lists all components, other values list all components  */
  /*         of a specific type: 'list_area', 'list_rad', 'list_text',          */
  /*         'list_table', list_grid' and 'list_event'                          */
  /*                                                                            */
  /* flag[10] designates which components of a generic product should be        */
  /*         printed. 'print_all' prints all components, other values print all */
  /*         components of a specific type: 'print_area', 'print_rad',          */
  /*         'print_text', 'print_table', 'print_grid' and 'print_event'        */
  /*                                                                            */
  /* flag[11] specifies a component index (int) which can modify other options  */
  /*          (-1 not specified)                                                */
  /*                                                                            */
  /* flag[12] specifies whether the product parameters should not be printed    */
  /*          'no_pparams'                                                      */
  /*                                                                            */
  /* flag[13] specifies whether the component parameters should not be printed  */
  /*          'no_cparams'                                                      */
  /* ----------------------------------------------------------------           */
 

  int offset=0;
  int base_offset=0;
  short divider;
  int i;
  int TEST=FALSE;
  int end_offset=0;

  if(TEST==TRUE)
    fprintf(stderr,"inside symbology: flag0=%i flag1=%i flag2=%i flag3=%i flag6=%i\n",
                                      flag[0],flag[1],flag[2],flag[3],flag[6]);


  if(buffer==NULL) {
     fprintf(stderr,"ERROR: The product buffer is empty\n");
     return(FALSE);
  }

  if(md.symb_offset==0) {
    fprintf(stderr,"Symbology Offset is 0 indicating no block available\n");
    return(TRUE);
  }
   
  if(md.symb_offset>400000 || md.symb_offset<60) {
    fprintf(stderr,"Symbology Offset is out of range\n");
    return(FALSE);
  }


  if(md.num_layers>18 || md.num_layers<1) {
     fprintf(stderr,"WARNING: Number of Layers (%d) is out of range (1-18).\n",
                                                            md.num_layers);
     fprintf(stderr,"         CVT will accept up to 30 layers for development purposes.\n");
  }

  /* cvt 4.4 - replaced 30 with MAX_LAYERS */
  if(md.num_layers>MAX_LAYERS) {
  	fprintf(stderr,"\nERROR - Number of Symbology Block Layers Exceeds %d\n", 
  	                                                                  MAX_LAYERS);
  	fprintf(stderr,"        Aborting Product Display\n\n");
  	return(FALSE);
  }

  /* beginning point of the first layer will be at 96+(symb_offset *2) + 10 */
  offset=96 + (2 * md.symb_offset) + 10;
  base_offset=offset;
  if(TEST) fprintf(stderr,"first layer divider offset at %d\n",offset);
  
  if(TEST) 
    fprintf(stderr,"-> Begin Layer Processing. Number of layers to process: %hd\n",
       md.num_layers);

  if(flag[6]>0) 
    fprintf(stderr,"-> Set Processing ONLY for Layer Number %i\n\n",flag[6]);
    
  /*CVT 4.4 - with the 'generic' flag, layer 1 is processed */
  if(flag[6]==-1) {
    flag[6] = 1;
    fprintf(stderr,"-> Generic Flag, Processing for Layer 1 \n\n");
  }
  
  /* CVT 4.4 - with 'radial' or 'row' flags, all layers are printed flag[6]==0  */
  /* CVT 4.4 - added ability to explicitly select 'layer 0' in cvt_dispatcher.c */
  if(flag[6]==0)
    fprintf(stderr,"-> Set Processing for All Layers\n\n");
  
  for(i=1;i<=md.num_layers;i++) {
    int length=0;
    unsigned short packet_code;

    divider=read_half(buffer,&offset);
    if(TEST) fprintf(stderr,"divider=%hd\n",divider);

    /* get the symbology layer length */
    length=read_word(buffer,&offset);

    if(length==0) {
        fprintf(stderr,
            "CAUTION- Non standard structure layer %d is empty. -CAUTION\n", i);
        continue;
    }

    packet_code=read_half(buffer,&offset);
    if(TEST) 
       fprintf(stderr,"length of layer %d is %d bytes    "
                      "packet code=%x hex or %d decimal\n",
            i,length,packet_code,packet_code);

    /* if flag[6]!=0 then only the layer # within flag[6] is printed */
    /* if glag[6]==0, then all layer are printed                     */
    if(flag[6]>0 && flag[6]!=i) {
      if(TEST) fprintf(stderr,"skipping layer %d\n",i);
      offset-=2; /* back up offset pointer to beginning of data layer */
      offset+=length; /* advance pointer to beginning of next block */
      continue;
      }
   
    /* calculate ending offset value (current offset+ layer length-2)
     * subtract 2 to account for reading the packet code */
    end_offset=offset+length-2;
    if(TEST) {
       fprintf(stderr,"curent offset value=%d or %x hex\n",offset,offset);
       fprintf(stderr,"End offset target value=%d or %x hex\n",end_offset,end_offset);
       }
   
    while(offset<end_offset) {
      if(TEST)
         fprintf(stderr,"*** enter symbology switch: current offset=%x end_offset=%x\n",
            offset,end_offset);
      switch(packet_code) {
          
        case 1:           fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_1(buffer,&offset);
                          break;
        case 2:           fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_2(buffer,&offset,flag);
                          break;
        case 3:           fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_3(buffer,&offset);
                          break;
        case 4:           fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_4(buffer,&offset);
                          break;       
        case 5:           fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_5(buffer,&offset);
                          break;                                       
        case 6:           fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_6(buffer,&offset);
                          break; 
        case 7:           fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_7(buffer,&offset);
                          break;
        case 8:           fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_8(buffer,&offset);
                          break;  
        case 9:           fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_9(buffer,&offset);
                          break;
        case 10:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_10(buffer,&offset);
                          break;
        case 11:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_11(buffer,&offset);
                          break;
        case 12:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_12(buffer,&offset);
                          break;
        case 13:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_13(buffer,&offset);
                          break;
        case 14:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_14(buffer,&offset);
                          break;
        case 15:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_15(buffer,&offset,flag);
                          break;
        case 16:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_16(buffer,&offset,flag);
                          break;
        case 17:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_17(buffer,&offset,flag);
                          break;
        case 18:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_18(buffer,&offset,flag);
                          break;
        case 19:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_19(buffer,&offset,flag);
                          break;
        case 20:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_20(buffer,&offset);
                          break; 
        case 23:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_23(buffer,&offset);
                          break;  
        case 24:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_24(buffer,&offset);
                          break;
        case 25:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_25(buffer,&offset);
                          break;
        case 26:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_26(buffer,&offset);
                          break;
        case 27:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          packet_27(buffer,&offset);
                          break;
        case 28:          fprintf(stderr,"packet code %hd found\n",packet_code);
                          /* CVT 4.4 - added the flag parameter */
                          packet_28(buffer,&offset,flag);
                          break;
        case 0x0802:      fprintf(stderr,"packet code %x found\n",packet_code);
                          packet_0802(buffer,&offset);
                          break;     
        case 0x3501:      fprintf(stderr,"packet code %x found\n",packet_code);
                          packet_3501(buffer,&offset);
                          break;                             
        case 0x0E03:      fprintf(stderr,"packet code %x found\n",packet_code);
                          packet_0E03(buffer,&offset);
                          break;                             
    
        case 0xAF1F:      fprintf(stderr,"packet code %x found\n",packet_code);
                          packet_AF1F(buffer,&offset,flag);
                          break;

        case 0xBA0F:
        case 0xBA07:      fprintf(stderr,"packet code %x found\n",packet_code);
	                       packet_BA07(buffer,&offset,flag);
                          break;
        default:
                          fprintf(stderr,"WARNING: Packet Code %d (%x Hex) was "
                                         "not found for processing\n",
                           packet_code,packet_code);
      } /* end switch */

      if(TEST)
         fprintf(stderr,"*** end of switch: offset=%x  end_offset=%x\n",
                                                              offset,end_offset);
     
      if(offset<end_offset){
         packet_code=read_half(buffer,&offset);
         if(TEST) fprintf(stderr,"New packet number=%hd at offset %x\n",
                                                           packet_code,offset-2);
         }

    else 
       break;
       
    }  /* end while */ 

  } /* end for loop */ 

  return(TRUE);
}

