/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:17:34 $
 * $Id: display_SATAP.c,v 1.2 2003/02/06 18:17:34 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* display_SATAP.c Stand Alone Tabular Alphanumeric Product */

#include "display_SATAP.h"


void display_SATAP(char *buffer) {
  /* display information contained within the
   Stand Alone Tabular Alphanumeric Product */
/*  short length; */
  int i,page,numpages,process;
/*  short value; */
  int offset=216;
/*  int TEST=FALSE; */

  /* move offset to beginning of SATAP block */
  printf("\nStand Alone Tabular Alphanumeric Product\n");

  printf("Block Divider =   %hd\n",read_half(buffer,&offset));
  numpages=read_half(buffer,&offset);
  printf("Number of Pages:  %hd\n",numpages);

  /* repeat for each page */
  for(page=1;page<=numpages;page++) {
    process=TRUE;
    printf("\nStand Alone Tabular Alphanumeric Product - Page: %d\n",page);

    while(process) {
      int num=read_half(buffer,&offset); /* num of chars in current line */
      if(num==-1) break;
      
      for(i=0;i<num;i++) {
        printf("%c",read_byte(buffer,&offset));
        }
      printf("\n");

      } /* end while process */

    } /* end for page */

  printf("\n");
  printf("SATAP Message Complete\n");
  }


