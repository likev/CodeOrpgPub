/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 21:43:49 $
 * $Id: recclprods_ref.c,v 1.3 2002/11/26 21:43:49 nolitam Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/*******************************************************************************
Module:        recclprods_ref.c

Description:   reflectivity product generation driver for cpc004/tsk007 for
               the REC (Radar Echo Classifier) task. From this module, the ICD
               compliant product is generated and a success/failure result code
               is returned to the main calling routine.
               
CCR#:          NA98-35001
               
Authors:       Andy Stern, Software Engineer, Mitretek Systems
                   astern@mitretek.org
               Tom Ganger, Systems Engineer,  Mitretek Systems
                   tganger@mitretek.org
               Version 1.0, January 2002
               
History:
               Initial implementation 1/31/02 - Stern
               
$Id: recclprods_ref.c,v 1.3 2002/11/26 21:43:49 nolitam Exp $
*******************************************************************************/


/* local include file */
#include "recclprods_ref.h"


/*******************************************************************************
Description:      generate_refl_output is the main generation module for the
                  creation of product 132, the clutter likelihood reflectivity
                  product. Each step of the ICD product generation is detailed
                  including function calls for creating the symbology and TAB
                  blocks.
                  
Input:            char* inbuf       pointer to the input buffer containing the
                                    intermediate data buffer
                  int max_bufsize   maximum buffer size for outbuf
                  
Output:           char* output      pointer to the output buffer where the 
                                    final product will be created
                  
Returns:          returns REC_SUCCESS or REC_FAILURE
                  
Globals:          none
Notes:            none
*******************************************************************************/
int generate_refl_output(char *inbuf, char *outbuf,int max_bufsize) {
   int i,j,k;           /* loop variables                                     */
   int result;          /* variable to hold function call results             */
   int DEBUG=FALSE;     /* flag to turn on debugging output to stderr         */
   Rec_prod_header_t *phdr;   /* pointer to an intermediate product header    */
   int length=0;        /* length accumulator                                 */
   int product_length;  /* length of the final/completed product              */
   short *radial;       /* short int pointer used to access inbuf             */
   int offset=0;        /* used to hold the access/offset point for structs   */
   short radial_data[MAX_RADIALS][MAX_1KMBINS]; /* input radial container     */
   int num_radials=0;   /* contains calculated number of radials with data    */
   int tab_offset;      /* holds the offset for the beginning of the TAB block*/
   
   /* obtain access to a product header which is the first structure within   */
   /* the intermediate product buffer                                         */
   phdr=(Rec_prod_header_t*)inbuf;
   
   RPGC_log_msg( GL_INFO, "REC refl: begin creating product for volume: %d\n",
                 phdr->volume_scan_num);
   
   /* building the ICD formatted output product requires a few steps ======== */
   /* step 1: build the product description block (pdb)                       */
   /* step 2: build the symbology block & data layer                          */
   /* step 3: build the tabular alphanumeric block                            */
   /* step 4: complete missing values in the pdb                              */
   /* step 5: build the message header block (mhb)                            */
   /* step 6: forward the completed product to the system                     */
   
   /* step 1: build the product description block -uses an ORPG system call   */
   /* which requires passing in a pointer to the output buffer, the product   */
   /* code and the current volume scan number                                 */
   RPGC_prod_desc_block((void*)outbuf,RECCLREF,phdr->volume_scan_num);
   
   /* step 2: build the symbology layer & RLE (run length encode) radial data */
   /* packet (AF1F). The first part of this process is to translate the data  */
   /* within the input buffer to the radial container array which is then fed */
   /* into the symbology block generator.                                     */
   
   /* calculate the offset into the input buffer where the radial data begins */
   offset=sizeof(Rec_prod_header_t)+sizeof(Rec_rad_header_t)*MAX_RADIALS;
   
   if(DEBUG)
      fprintf(stderr,
         "offset size:  prod_hdr=%d  MAX_RADIALS*rad_hdr=%d  total=%d\n",
         sizeof(Rec_prod_header_t),sizeof(Rec_rad_header_t)*MAX_RADIALS,offset);
   
   
   /* we can determine total number of radials to process and compare it to   */
   /* value sent via the intermediate product header. this assures that we    */
   /* won't run out the end of the array                                      */
   num_radials=radials_to_process(inbuf);
   
   if(DEBUG)
      fprintf(stderr,"REC REF  calculated radials=%d   prod 298 radials=%d\n",
              num_radials,phdr->num_radials);
   /* array bounds quality control routines                                   */
#if 0
   if(num_radials!=phdr->num_radials) {
      fprintf(stderr,"RECCLPROD_REF ERROR: num_radial correlation error\n");
      fprintf(stderr,"  calculated radials=%d   prod 298 radials=%d\n",
         num_radials,phdr->num_radials);
      return(REC_FAILURE);
      }
#endif
   if(phdr->num_radials>=MAX_RADIALS) {
      RPGC_log_msg( GL_ERROR,
                    "RECCLPROD_REF ERROR: num_radials is greater than MAX: %d\n",
                    phdr->num_radials);
      return(REC_FAILURE);
      }
   
   /* load the raw values into the radial_data container (array)              */
   /* from the calculated offset position assign a pointer called radial      */
   radial=(short*)(inbuf+offset);
   
   k=0;  /* used to increment through the 1 dimensional array                 */
   for(i=0;i<MAX_RADIALS;i++) {
      for(j=0;j<MAX_1KMBINS;j++) {
         radial_data[i][j]=(short)radial[k];
         k++;
         }
      }
   
   if(DEBUG) {
      fprintf(stderr,"\narray loaded first radial to follow\n");
      for(i=0;i<MAX_1KMBINS;i++)
         fprintf(stderr," %02X",(unsigned)radial_data[0][i]);
      fprintf(stderr,"\n\n");
      }
      
   /* call the symbology layer generator. the length of the product will      */
   /* be returned through "length"                                            */
   result=build_symbology_layer((short*)outbuf,inbuf,phdr->num_radials,&length,
      radial_data,RECCLREF,max_bufsize);
   
   /* save the offset for the start of the TAB block                          */
   tab_offset=length;
   
   /* step 3: enter the TAB block generator                                   */
   product_length=generate_TAB(inbuf,outbuf,tab_offset,RECCLREF,max_bufsize);
   
   if(DEBUG)
      fprintf(stderr,"rec refl:accumulated product length =%d\n",
         product_length);
   
   /* step 4: finish building the product description block by filling in     */
   /* certain values such as elevation index, target elev and block offsets   */
   if(DEBUG)
      fprintf(stderr,"before finish_pbd: length=%d tab_offset=%d\n",
         product_length, tab_offset);
   finish_pdb((short*)outbuf,phdr->rpg_elev_ind,phdr->target_elev,length,
      tab_offset,RECCLREF);
      
   /* generate the product message header (system call) and input total       */
   /* accumulated product length (minus 120 bytes) for the MHB & PDB          */
   product_length-=120;
   result=RPGC_prod_hdr((void*)outbuf,RECCLREF,&product_length);
   if(DEBUG)
      fprintf(stderr,"-> refl completed product length=%d  result=%d\n",
              product_length,result);
   
   return(result);
   }



