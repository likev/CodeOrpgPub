/*
 * RCS info
 * $Author$
 * $Locker$
 * $Date$
 * $Id$
 * $Revision$
 * $State$
 */

/*************************************************************************
Module:         s1_print_diagnostics.c

Description:    these functions are used in debugging by displaying 
                progress in populating ICD product header messages 
                product description block, symbology block and
                product output to file.
                
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org

Version 1.1,  June 2004    T. Ganger
              Linux update

Version 1.2   March 2006   T. Ganger
              Added symbology block print function and a product to disk
                   output function.
              Renamed module and other misc cleanup    
              
Version 1.3   February 2008    T. Ganger
              Replaced C++ style comments using '//' for ANSI compliance.
              Created unique filenames with respect to other algorithms.
              
Version 1.4   February 2009    T. Ganger    (Sample Algorithm ver 1.21)
              Added function to print dual pol data parameters.
                
$Id$
*************************************************************************/
 
#include "s1_print_diagnostics.h"

/*************************************************************************
Description:    print_message_header prints out the contents of the ORPG
                message header block (the first block in an ICD formatted
                graphic)
Input:          char* buffer is a character pointer to the buffer where 
                the ICD formatted product is located.
Output:         to display device only
Returns:        returns TRUE upon success and FALSE otherwise
Globals:        none
Notes:          none
*************************************************************************/

int print_message_header(char* buffer) {
  /* print out the contents of the ORPG message Header                  */
  /* the product should be in the buffer...if not return FALSE          */
  Graphic_product *gp;   /* pointer to a Graphic_product structure      */

  int time,len;
  /* return if buffer pointer is NULL                                   */
  if(buffer==NULL) {
    fprintf(stderr,"ERROR: The product buffer is empty\n");
    return(FALSE);
  }
    
  /* cast a Graphic_product pointer from the input buffer               */
  gp=(Graphic_product *)(buffer);
  
  fprintf(stderr,"\n------------------   ORPG Message Header Block -------------------\n");
  fprintf(stderr,"NEXRAD Message Code\t\t\t%hd\n",gp->msg_code);
  fprintf(stderr,"Modified Julian Date\t\t\t%hd\t(not set til transmission)\n",
    gp->msg_date);
  /* BUILD 6 LINUX change */    
  RPGC_get_product_int( (void*) &gp->msg_time , &time );
    fprintf(stderr,"Num Secs after Midnight \t\t%d\t(not set til transmission)\n",
                 time);              
  /* BUILD 6 LINUX change */                
  RPGC_get_product_int( (void*) &gp->msg_len , &len );                 
    fprintf(stderr,"Message Length in bytes \t\t%d\n",
                 len); 
                             
  fprintf(stderr,"ID of the Source\t\t\t%hd\t(not set til transmission)\n",
    gp->src_id);
  fprintf(stderr,"ID of the Receiver\t\t\t%hd\t(not set til transmission)\n",
    gp->dest_id);
  fprintf(stderr,"Number of Blocks\t\t\t%hd\n",gp->n_blocks);

  
  return(TRUE);
  
}

/************************************************************************
Description:    print_pdb_header prints out the product description block
                portion of the ICD formatted graphic header block
Input:          char* buffer is a character pointer to the buffer where 
                the ICD formatted product is located.
Output:         to display device only
Returns:        returns TRUE upon success and FALSE otherwise
Globals:        none
Notes:          none
************************************************************************/

int print_pdb_header(char* buffer) {
  /* print out the contents of the ORPG product description block Header*/
  /* the product should be in the buffer...if not return FALSE          */
  Graphic_product *gp;   /* pointer to a Graphic_product structure      */
  
  /* BUILD 6 LINUX change */
  int lat,lon,vtime,gtime,sym,gra,tab;
  
  /* return as an error if a NULL pointer has been passed as input      */
  if(buffer==NULL) {
    fprintf(stderr,"ERROR: The product buffer is empty\n");
    return(FALSE);
  }
    
  /* cast a pointer to a Graphic_product structure type                 */
  gp=(Graphic_product *)(buffer);
  
  fprintf(stderr,"\n------------------ORPG Product Description Block -----------------\n");
  fprintf(stderr,"pbd divider\t\t\t\t%hd\n",gp->divider);
  
  /* BUILD 6 LINUX change */
  RPGC_get_product_int( (void*) &gp->latitude , &lat );
  RPGC_get_product_int( (void*) &gp->longitude , &lon );
  fprintf(stderr,"Radar Latitude\t\t\t\t%8.3f\n",lat/1000.0);
  fprintf(stderr,"Radar Longitude\t\t\t\t%8.3f\n",lon/1000.0);  
  
  fprintf(stderr,"Radar Height (MSL)\t\t\t%hd\n",gp->height);
  fprintf(stderr,"Internal Product Code\t\t\t%hd\n",gp->prod_code);
  fprintf(stderr,"Operational Weather Mode\t\t%hd\n",gp->op_mode);
  fprintf(stderr,"VCP Number\t\t\t\t%hd\n",gp->vcp_num);
  fprintf(stderr,"Request Sequence Number\t\t\t%hd\n",gp->seq_num);
  fprintf(stderr,"Volume Number\t\t\t\t%hd\n",gp->vol_num);
  fprintf(stderr,"Volume Date\t\t\t\t%hd\n",gp->vol_date);
  
  /* BUILD 6 LINUX change */
/* the following looks weird but it works */
  RPGC_get_product_int( (void*) &gp->vol_time_ms , &vtime );
  fprintf(stderr,"Volume Time (seconds after Mid)\t\t%d\n", vtime);
   
  fprintf(stderr,"Product Generation Date\t\t\t%hd\n",gp->gen_date);
  
  /* BUILD 6 LINUX change */
  RPGC_get_product_int( (void*) &gp->gen_time , &gtime );
  fprintf(stderr,"Product Gen Time (seconds after Mid)\t%d\n", gtime);
  
  fprintf(stderr,"Product Dependent Parameter 1\t\t%hd\n",gp->param_1);
  fprintf(stderr,"Product Dependent Parameter 2\t\t%hd\n",gp->param_2);
  fprintf(stderr,"Volume Elevation Index\t\t\t%hd\n",gp->elev_ind);
  fprintf(stderr,"Product Dependent Parameter 3\t\t%hd\n",gp->param_3);
  fprintf(stderr,"Data Level Threshold 1\t\t\t%hd\n", gp->level_1);
  fprintf(stderr,"Data Level Threshold 2\t\t\t%hd\n", gp->level_2);
  fprintf(stderr,"Data Level Threshold 3\t\t\t%hd\n", gp->level_3);
  fprintf(stderr,"Data Level Threshold 4\t\t\t%hd\n", gp->level_4);
  fprintf(stderr,"Data Level Threshold 5\t\t\t%hd\n", gp->level_5);
  fprintf(stderr,"Data Level Threshold 6\t\t\t%hd\n", gp->level_6);
  fprintf(stderr,"Data Level Threshold 7\t\t\t%hd\n", gp->level_7);
  fprintf(stderr,"Data Level Threshold 8\t\t\t%hd\n", gp->level_8);
  fprintf(stderr,"Data Level Threshold 9\t\t\t%hd\n", gp->level_9);
  fprintf(stderr,"Data Level Threshold 10\t\t\t%hd\n",gp->level_10);
  fprintf(stderr,"Data Level Threshold 11\t\t\t%hd\n",gp->level_11);
  fprintf(stderr,"Data Level Threshold 12\t\t\t%hd\n",gp->level_12);
  fprintf(stderr,"Data Level Threshold 13\t\t\t%hd\n",gp->level_13);
  fprintf(stderr,"Data Level Threshold 14\t\t\t%hd\n",gp->level_14);
  fprintf(stderr,"Data Level Threshold 15\t\t\t%hd\n",gp->level_15);
  fprintf(stderr,"Data Level Threshold 16\t\t\t%hd\n",gp->level_16);
  fprintf(stderr,"Product Dependent Parameter 4\t\t%hd\n",gp->param_4);
  fprintf(stderr,"Product Dependent Parameter 5\t\t%hd\n",gp->param_5);
  fprintf(stderr,"Product Dependent Parameter 6\t\t%hd\n",gp->param_6);
  fprintf(stderr,"Product Dependent Parameter 7\t\t%hd\n",gp->param_7);
  fprintf(stderr,"Product Dependent Parameter 8\t\t%hd\n",gp->param_8);
  fprintf(stderr,"Product Dependent Parameter 9\t\t%hd\n",gp->param_9);
  fprintf(stderr,"Product Dependent Parameter 10\t\t%hd\n",gp->param_10);
  fprintf(stderr,"Number of Map Pieces\t\t\t%hd\n",gp->n_maps);
  
  /* BUILD 6 LINUX change */  
  RPGC_get_product_int( (void*) &gp->sym_off , &sym );
  RPGC_get_product_int( (void*) &gp->gra_off , &gra );
  RPGC_get_product_int( (void*) &gp->tab_off , &tab );
  
  fprintf(stderr,"Symbology Offset (bytes)\t\t%d\n",sym);
  fprintf(stderr,"Graphic Block Offset (bytes)\t\t%d\n",gra);
  fprintf(stderr,"Tabular Block Offset (bytes)\t\t%d\n",tab);  
  
  return(TRUE);
  
}





/************************************************************************
Description:    print_symbology_header prints out the header portion of
                the symbology block and the first layer header.
Input:          char* c_ptr is a character pointer to the beginning
                of the symbology block
Output:         to display device only
Returns:        returns TRUE upon success and FALSE otherwise
Globals:        none
Notes:          none
************************************************************************/
int print_symbology_header(char *c_ptr) {

  unsigned int read_length;
  unsigned short *short_ptr;
  sym_block *sym = (sym_block *) c_ptr; 

  /* return as an error if a NULL pointer has been passed as input      */
  if(sym==NULL) {
    fprintf(stderr,"ERROR: The symbology block pointer is null\n");
    return(FALSE);
  }
  
  short_ptr = (unsigned short *)c_ptr; 
  
  fprintf(stderr,"\n");
  fprintf(stderr,"Symb hdr.divider is      %d(%d)\n",sym->divider,
                       short_ptr[0]);     
  fprintf(stderr,"Symb hdr.block_id is     %d(%d)\n",sym->block_id,
                       short_ptr[1]);
  RPGC_get_product_int((void*)&sym->block_length, (void*)&read_length);
  fprintf(stderr,"Symb hdr.block_length is %d(%d:%d)\n",read_length,
                       short_ptr[2], short_ptr[3]);
  fprintf(stderr,"Symb hdr.num_layers is   %d(%d)\n",sym->num_layers,
                       short_ptr[4]);
  fprintf(stderr,"    Layer 1 divider is   %d(%d)\n",sym->divider2,
                       short_ptr[5]);
  RPGC_get_product_int((void*)&sym->layer_length, (void*)&read_length);
  fprintf(stderr,"    Layer 1 length is    %d(%d:%d)\n",read_length,
                       short_ptr[6], short_ptr[7]);

  return(TRUE);
  
}




/************************************************************************
Description:  product_to_disk outputs product to a binary disk file
              the location of the file is the working directory. This
              is the directory from which the ORPG was launched (or
              from where the algorithm task was started if not 
              executed when the ORPG was started).
              
Inputs:    char* output_buf pointer to buffer containing the product
           int   outlen     length in bytes of the product
           char* pname      product name used to build the filename
                            (40 characters max)
           short ele_idx    the elevation index 
                      
Output:    a file named <pname>.out_<ele_idx> in the working directory
Returns:        
Globals:        none
Notes:          none
************************************************************************/
void product_to_disk(char * output_buf, int outlen, char *pname, short ele_idx) {

FILE *outfile;      /* filepointer used for diagnostic output        */
char filename[48];  /* variable used to hold output filename         */
   
  fprintf(stderr,"Writing product contents to binary file, elevation %d.\n",
                                                                   ele_idx);
  sprintf(filename,"%s.out_%d",pname, ele_idx);
  if((outfile = fopen(filename, "w")) != NULL) {
     /* copy the data out of the buffer in memory to a file     */
     fwrite(output_buf, outlen, 1, outfile);
     fclose(outfile);
  }

}





/* Sample Alg 1.21 - Test Reading of Dual Pol Moment header Fields */
/* The API function RPGC_get_radar_data() was not used */
void test_read_dp_moment_params(Base_data_radial *radialPtr) {
    
int i;

Generic_moment_t *gmPtr = NULL;
Base_data_header *hdr = (Base_data_header *) radialPtr;

int num_offsets = hdr->no_moments;
    
    fprintf(stderr,"\nPrinting Parameters for %d dp moments\n", num_offsets);
    
    for(i=0; i<num_offsets; i++) {
        
        if(hdr->offsets[i] == 0) {
            fprintf(stderr,"  Error - offset %d is 0\n", i);
        
        } else {
            gmPtr = (Generic_moment_t*)( (char*) radialPtr + hdr->offsets[i]);
            
            fprintf(stderr,"\n  Name of moment is '%c%c%c%c'\n", gmPtr->name[0], 
                                gmPtr->name[1], gmPtr->name[2], gmPtr->name[3]);
            fprintf(stderr,"    Number of Gates: %hu\n", gmPtr->no_of_gates);
            fprintf(stderr,"    First bin range: %d,  Bin size: %d\n", 
                                   gmPtr->first_gate_range, gmPtr->bin_size);
            fprintf(stderr,"    Control Flag: %hu (0 - None, 1- recomb az, etc.)\n", 
                                            (unsigned short)gmPtr->control_flag);
            fprintf(stderr,"    Data Word Size: %hu (bits)\n", 
                                          (unsigned short)gmPtr->data_word_size);
            fprintf(stderr,"    Scale: %f,  Offset: %f\n", 
                                   gmPtr->scale, gmPtr->offset);
        } /* end else printing parameters */
        
    } /* end for */
    
    fprintf(stderr,"\nFinished Printing Parameters for dp moments\n\n");
    
} /* end test_read_dp_moment_params */





