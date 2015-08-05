/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2004/03/03 17:59:32 $
 * $Id: cvt_display_TAB.c,v 1.4 2004/03/03 17:59:32 cheryls Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/* cvt_display_TAB.c */

#include "cvt_display_TAB.h"


void display_TAB(char *buffer,int *offset, int verbose) {
  /* display information contained within the
   Tabular Alphanumeric Block */
/*  short length; */
  int i,page,numpages,process;
  int divider, bid, blen;
  int len_read=0;
  int num_lines =0;  /* used to catch page parsing errors */
  int page_parse_error = FALSE;
/*  short value; */
  int TEST=FALSE;

  /* move offset to beginning of TAB block */
  printf("\nTabular Alphanumeric Block\n");

  if(TEST) {
     printf("Offset value = %d  2*tab_offset=%d\n",*offset,2*md.tab_offset);
     }
  
  divider = read_half(buffer,offset);
  bid = read_half(buffer,offset);
  blen = read_word(buffer,offset);
     
  printf("Block Divider =   %hd\n",divider);
  printf("Block ID =        %hd\n",bid);
  printf("Length of Block = %d bytes\n",blen);

  if((divider != -1) || (bid != 3)) {
     fprintf(stderr, "\nTAB  ERROR   TAB  ERROR   TAB  ERROR\n");
     fprintf(stderr, "ERROR Entering TAB Block, Either entry offset\n");
     fprintf(stderr, " is incorrect or TAB divider and ID are Incorrect\n");
     return;
  }

   if(verbose==TRUE) { 
     fprintf(stderr, "\nVerbose TAB Display selected, the following are the \n");
     fprintf(stderr, "     repeated MHB and PDB contained within the TAB\n");
     print_message_header(buffer + *offset - 96);
     print_pdb_header(buffer + *offset - 96);	
     fprintf(stderr, "\nEnd of repeated MHB and PDB, continue with TAB header\n");
   } 
  
  /* advance offset pointer beyond the message header block and the
     product description block */
     *offset+=120;
     if(TEST) {
        printf("new offset = %d\n",*offset);
     }
  
     
  printf("Block Divider =   %hd\n",read_half(buffer,offset));    
  numpages=read_half(buffer,offset);
  printf("Number of Pages:  %hd\n",numpages);

  /* repeat for each page */
  for(page=1;page<=numpages;page++) {
    process=TRUE;
    printf("\nTabular Alphanumeric Block - Page: %d\n",page);

    num_lines=0;
    
    while(process) {     
      int num=read_half(buffer,offset); /* num of chars in current line */

      len_read = len_read +2 + num;
      if(len_read >= blen) {
      	fprintf(stderr, "\nTAB  ERROR   TAB  ERROR   TAB  ERROR\n");
      	fprintf(stderr, "ERROR PARSING TAB PAGE Number %d\n", page);
        fprintf(stderr, " Number of Characters to be read exeeds block length\n");
        page_parse_error = TRUE;
      	break;
      	
      }
      
      if(num > 80) {
      	fprintf(stderr, "\nTAB  ERROR   TAB  ERROR   TAB  ERROR\n");
      	fprintf(stderr, "ERROR PARSING TAB PAGE Number %d\n", page);
        fprintf(stderr, " Number of Characters Exceed 80 on Line %d\n",
                num_lines+1);
        page_parse_error = TRUE;
      	break;
      }
      	
      /* if a max size page we exit after checking for divider */     
      if(num_lines==MAX_NUM_LINES) {
        if(num==-1) 
          break; 
        else {
          fprintf(stderr, "\nTAB  ERROR   TAB  ERROR   TAB  ERROR\n");
          fprintf(stderr, "ERROR PARSING TAB PAGE Number %d\n", page);
          fprintf(stderr, " or Number of Lines Exceed Limit of 17\n");
          fprintf(stderr, "Did Not Find End-Of-Page (-1) Divider\n");
          page_parse_error = TRUE;
      	  break;
      	}
      }	
   	
      if(num==-1) break;   /* catch a short page */
           
      for(i=0;i<num;i++) {
        printf("%c",read_byte(buffer,offset));
        }
      printf("\n");
      
      num_lines++;

      } /* end while process */

    if(page_parse_error==TRUE)
        break;  

  } /* end for page */

  printf("\n");
  printf("TAB Message Complete\n");
}


