/********************************************************************************

    file: read_product_list.c

          This file contains the routines that read the products from the
          RPS file

 ********************************************************************************/

/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2002/08/21 20:15:20 $
 * $Id: nbtcp_read_product_list.c,v 1.3 2002/08/21 20:15:20 garyg Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <nbtcp.h>

extern int errno;


/********************************************************************************

     Description: This routine reads the products from the rps list file

           Input: fp - the file to read the product records from

          Output: prod_record - the product read from the RPS file
                     eof_flag - end of file flag

          Return: 0 on success;  -1 on error and EOF

 ********************************************************************************/

int RPL_read_product_list (FILE *fp, product_msg_info_t *prod_record,
                           int *eof_flag)
{
   char tmp[10];
   char mne[10];
   char desc[10];
   char parm[10];
   char code[10];
   char id[10];
   char format_string [128];
   int i;
   int ret;

      /* build the format specifiers to read the formatted record */

   memset (&tmp, 0, 10);
   sprintf (tmp, "%d", PROD_ID_LEN);
   strcpy (id, "%\0");
   strcat (id, tmp);
   strcat (id, "s \0");

   strcpy (format_string, id);

   memset (&tmp, 0, 10);
   sprintf (tmp, "%d", PROD_CODE_LEN);
   strcpy (code, "%\0");
   strcat (code, tmp);
   strcat (code, "s \0");

   strcat (format_string, code);

   memset (&tmp, 0 ,10);
   sprintf (tmp, "%d", MNE_LEN);
   strcpy (mne, "%\0");
   strcat (mne, tmp);
   strcat (mne, "s \0");

   strcat (format_string, mne);

   memset (&tmp, 0 , 10);
   sprintf (tmp, "%d", STRNG_LEN-1); 
   strcpy (desc, "%\0");
   strcat (desc, tmp);
   strcat (desc, "c\0"); 

   strcat (format_string, desc);

   strcpy (parm, " %s");

   for (i = 0; i < NUMBER_PROD_PARMS; i++) {
      strcat (format_string, parm);
   }

   while (1) { /* loop may be used for future error checking */

      /* populate the record fields using the format specifiers */

      ret = fscanf (fp, format_string, prod_record->prod_id, prod_record->prod_code,
             prod_record->mne, prod_record->descrp, prod_record->params[0],
             prod_record->params[1], prod_record->params[2], prod_record->params[3], 
             prod_record->params[4], prod_record->params[5]);

         /* update the paramter fields that are flagged as "Unused" */

      for (i = 0; i < NUMBER_PROD_PARMS; i++) {
         strncpy (&prod_record->params[i][PROD_PARM_LEN-1], "\0", 1);
         if ((strncmp (prod_record->params[i], "UNU", 3)) == 0)
            snprintf (prod_record->params[i], PROD_PARM_LEN, "%d", PARAM_UNUSED);
      }

      if (ret == EOF)
         *eof_flag = 1;
      else
         *eof_flag = 0;

      break; 
   } 

   if (*eof_flag == 1)
      return (-1);
   else 
      return (0);
}
