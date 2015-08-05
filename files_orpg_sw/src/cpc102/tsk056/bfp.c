/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2007/01/05 23:03:08 $
 * $Id: bfp.c,v 1.6 2007/01/05 23:03:08 ryans Exp $
 * $Revision: 1.6 $
 * $State: Exp $
*/


#include "bfp.h"

/* Global Variables */ 
static int Screen_Output = BFP_FALSE; 
static int ByteSwapNeeded = BFP_FALSE;

/* SMI functions */
extern SMI_info_t* BFP_smi_info (char *name, void *data);

int main() 
{
   int			err				= 0;
   int			selection			= -1;


   /* Establish the function that will be used to get
      structure meta information (SMI) */
   SMIA_set_smi_func (BFP_smi_info);

   while (selection != EXIT_CODE)
   {
      /* reset screen variable */
      Screen_Output = BFP_FALSE;

      if ((system ("clear")) >= 0)
      {
         fprintf (stderr, "*****************************\n");
         fprintf (stderr, "*                           *\n");
         fprintf (stderr, "*  Binary File Processor    *\n");
         fprintf (stderr, "*  User Interface           *\n");
         fprintf (stderr, "*                           *\n");
         fprintf (stderr, "*****************************\n");
      }

      fprintf (stderr, "\n\n");

      fprintf (stderr, " 1 - Interpret Binary File\n");
      fprintf (stderr, " 2 - Modify Binary File\n");
      fprintf (stderr, " 3 - Search Binary File\n");
      fprintf (stderr, " 4 - Extract Data from Binary File\n");
      fprintf (stderr, " 5 - Print Structure Info\n");
      fprintf (stderr, "%d - Quit\n\n", EXIT_CODE);
      
      fprintf(stderr, "Enter selection:  ");
      if ( (err = scanf("%d", &selection)) != 1)
      {
         fprintf(stderr, "Problem interpreting selection.\n");
         exit(1);
      }
      fprintf(stderr, "\n");

      switch (selection)
      {
         case 1:
            interpret_bin_file();
            break;

         case 2:
            modify_bin_file();
            break;

         case 3:
            search_bin_file();
            break;

         case 4:
            extract_bin_data();
            break;

         case 5:
            print_struct_info();
            break;

         case EXIT_CODE:
            break;

         default:
            fprintf (stderr, "Invalid selection\n");
            sleep (3);
            break;
      }
   }

   return 0;

} /* end main */
   

/********************************************************************************

     interpret_bin_file

********************************************************************************/

int interpret_bin_file()
{
   int err;
   int selection;
   int store_select;
   int ret_val;
   char in_file_name[MAX_FILE_NAME_SIZE];
   char* out_file_name = NULL;
   
         
   ret_val = -1;

   if ((system ("clear")) >= 0)
   {
      fprintf (stderr, "********************************\n");
      fprintf (stderr, "*                              *\n");
      fprintf (stderr, "*  BFP - Interpret Binary File *\n");
      fprintf (stderr, "*                              *\n");
      fprintf (stderr, "********************************\n");
   }

   fprintf (stderr, "\n\n");

   printf("Enter input file path and name (default path is .):\n");
   if ( (err = scanf("%s", in_file_name)) != 1)
   {
      fprintf(stderr, "Problem interpreting input file name.\n");
      sleep(3);
      return(BFP_FAILURE);
   }
   printf("\n");

   printf(" 1: Interpret as ASCII characters \n");
   printf(" 2: Interpret as 1-byte integers \n");
   printf(" 3: Interpret as 2-byte integers \n");
   printf(" 4: Interpret as 4-byte integers \n");
   printf(" 5: Interpret as structured data \n");
   printf("%d: Return to main menu \n\n", EXIT_CODE);
      
   printf("Enter selection:  ");
   if ( (err = scanf("%d", &selection)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   /* if user chose to return to main menu, return */
   if ( selection == EXIT_CODE )
   {
      return (BFP_SUCCESS);
   }


   if ( selection == 5 )
   {  /* only going to send interpreted structured data to the screen */
      Screen_Output = BFP_TRUE;
   }
   else
   {
      printf("\n");
      printf("Where do you want the output to go?\n");
      printf(" 1: Standard out (screen)\n");
      printf(" 2: Store in file\n");
      printf("%d: Return to main menu\n", EXIT_CODE);
      if ( (err = scanf("%d", &store_select)) != 1)
      {
         fprintf(stderr, "Problem interpreting selection.\n");
         sleep(3);
         store_select = 1;
      }

      printf("\n");

      switch (store_select)
      {
         case 1:
            out_file_name = NULL;
            Screen_Output = BFP_TRUE;
            break;

         case 2:
            Screen_Output = BFP_FALSE;
            out_file_name = malloc(MAX_FILE_NAME_SIZE);
            printf("Enter output file path and name (default path is .):\n");

            if ( (err = scanf("%s", out_file_name)) != 1)
            {
               fprintf(stderr, "Problem interpreting output file name.\n");
               sleep(3);
               return(BFP_FAILURE);
            }
            break;

         case EXIT_CODE:
            break;

         default:
            fprintf (stderr, "Invalid selection\n");
            break;
      }
   }

   switch (selection)
   {
      case 1:
         interpret_bin_to_ascii( in_file_name, out_file_name );
         break;   
         
      case 2:
         interpret_bin_to_byte( in_file_name, out_file_name );
         break;   
          
      case 3:
         interpret_bin_to_short( in_file_name, out_file_name );
         break;   
          
      case 4:
         interpret_bin_to_int( in_file_name, out_file_name );
         break;   
          
      case 5:
         interpret_bin_to_struct( in_file_name );
         break;   

      case EXIT_CODE: 
         break;

      default:
         fprintf (stderr, "Invalid Selection...Please try again\n");
         sleep (2);
         break;
   }

   /* free memory if necessary */
   if ( out_file_name != NULL )
   {
      free (out_file_name);
   }

   return 0;

} /* end interpret_bin_file */


/********************************************************************************

     modify_bin_file

********************************************************************************/

int modify_bin_file()
{
   int err;
   int do_over;
   int fsize;
   int file_id;
   int selection;
   int byte_offset;
   int hw_offset;
   int num_shorts;
   char file_name[MAX_FILE_NAME_SIZE];
         
   do_over = BFP_TRUE;   /* init do_over flag */

   printf("\n");

   /* get file name */
   printf("Enter file path and name (default path is .):\n");
   if ( (err = scanf("%s", file_name)) != 1)
   {
      fprintf(stderr, "Problem interpreting file name.\n");
      sleep(3);
      return (BFP_FAILURE);
   }
   printf("\n");

   /* open the file for read/write */
   if ( (file_id = open( file_name, O_RDWR )) < 0 )
   {
      fprintf(stderr, "Error opening %s (errno %d)\n", file_name, errno);
      sleep(3);
      return (BFP_FAILURE);
   }

   /* get file size */
   fsize = lseek( file_id, 0, SEEK_END );

   if ((system ("clear")) >= 0)
   {
      fprintf (stderr, "********************************\n");
      fprintf (stderr, "*                              *\n");
      fprintf (stderr, "*  BFP - Modify Binary File    *\n");
      fprintf (stderr, "*                              *\n");
      fprintf (stderr, "********************************\n");
   }

   fprintf (stderr, "\n\n");

   printf(" 1: Modify character data \n");
   printf(" 2: Modify 1-byte integer data \n");
   printf(" 3: Modify 2-byte integer data \n");
   printf(" 4: Modify 4-byte integer data \n");
   printf(" 5: Modify structured data \n");
   printf(" 6: Add data\n");
   printf("%d: Return to main menu \n\n", EXIT_CODE);
   
   printf("Enter selection:  ");
   if ( (err = scanf("%d", &selection)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return (BFP_FAILURE);
   }

   if ( selection != EXIT_CODE )
   {
      if ( selection < 5 )
      {
         /* get byte offset of data to be replaced */
         while ( do_over == BFP_TRUE )
         {
            fprintf(stderr,
               "Enter the byte offset (0-based) of the byte to be replaced:\n");
            if ( (err = scanf("%d", &byte_offset)) != 1)
            {
               fprintf(stderr, "Problem interpreting byte_offset.\n");
               sleep(3);
               return (BFP_FAILURE);
            }
      
            if ( byte_offset > fsize )
            { 
               fprintf(stderr, "There are only %d bytes in the file.\n\n", fsize);
               do_over = BFP_TRUE;
            }
            else
            {
               do_over = BFP_FALSE;
            }
         }
      }
      else if ( selection == 6 )
      {
         /* get halfword offset of data to be added */
         while ( do_over == BFP_TRUE )
         {
            fprintf(stderr,
               "Enter the halfword offset (0-based) of the data to be added:\n");
            if ( (err = scanf("%d", &hw_offset)) != 1)
            {
               fprintf(stderr, "Problem interpreting hw_offset.\n");
               sleep(3);
               return (BFP_FAILURE);
            }
      
            if ( (hw_offset*sizeof(short)) > fsize )
            { 
               fprintf(stderr, "There are only %d bytes in the file.\n\n", fsize);
               do_over = BFP_TRUE;
            }
            else
            {
               do_over = BFP_FALSE;
            }
         }

         /* reset loop sentinel */
         do_over = BFP_TRUE;

         /* get number of shorts to be added */
         while ( do_over == BFP_TRUE )
         {
            /* prompt the user to enter the number of shorts to add */
            fprintf(stderr,
               "Enter the number of unsigned shorts you want to add:\n");
            if ( (err = scanf("%d", (int*) &num_shorts)) != 1)
            {
               fprintf(stderr, "Problem interpreting num_shorts.\n");
               sleep(3);
               return(BFP_FAILURE);
            }
            else
            {
               do_over = BFP_FALSE;
            }
         }
      }

      switch (selection)
      {
         case 1:
            modify_char_data( file_id, fsize, byte_offset);
            break;   
         
         case 2:
            modify_1byte_int_data( file_id, fsize, byte_offset);
            break;   
          
         case 3:
            modify_2byte_int_data( file_id, fsize, byte_offset);
            break;   
          
         case 4:
            modify_4byte_int_data( file_id, fsize, byte_offset);
            break;   
          
         case 5:
            modify_struct_data(file_id, fsize);
            break;   
   
         case 6:
            add_data(file_id, fsize, hw_offset, num_shorts);
            break;   
   
         case EXIT_CODE: 
            break;
   
         default:
            fprintf (stderr, "Invalid Selection...Please try again\n");
            sleep (2);
            break;
      }
   }

   return 0;

} /* end modify_bin_file */


/********************************************************************************

     search_bin_file

********************************************************************************/

int search_bin_file()
{
   int err;
   int selection;
   char in_file_name[MAX_FILE_NAME_SIZE];
   
         
   if ((system ("clear")) >= 0)
   {
      fprintf (stderr, "********************************\n");
      fprintf (stderr, "*                              *\n");
      fprintf (stderr, "*  BFP - Search Binary File    *\n");
      fprintf (stderr, "*                              *\n");
      fprintf (stderr, "********************************\n");
   }

   fprintf (stderr, "\n\n");

   printf("Enter input file path and name (default path is .):\n");
   if ( (err = scanf("%s", in_file_name)) != 1)
   {
      fprintf(stderr, "Problem interpreting input file name.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   fprintf (stderr, "\n\n");

   printf(" 1: Search for ASCII character data \n");
   printf(" 2: Search for numeric data \n");
   printf("%d: Return to main menu \n\n", EXIT_CODE);
   
   printf("Enter selection:  ");
   if ( (err = scanf("%d", &selection)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return(BFP_FAILURE);
   }
   printf("\n");

      
   switch (selection)
   {
      case 1:
         search_char_data(in_file_name);
         break;   
      
      case 2:
         search_numeric_data(in_file_name);
         break;   
       
      case EXIT_CODE: 
         break;

      default:
         fprintf (stderr, "Invalid Selection...Please try again\n");
         sleep (2);
         break;
   }

   return 0;

} /* end search_bin_file */


/********************************************************************************

     extract_bin_data

********************************************************************************/

int extract_bin_data()
{
   int fsize;
   int err;
   int format_select;
   int loc_select;
   int start_byte = 0;
   int num_bytes = 0;
   int infile_id = 0;

   int outfile_id = 0;

   char infile_name[MAX_FILE_NAME_SIZE];
   char outfile_name[MAX_FILE_NAME_SIZE];
   
   if ((system ("clear")) >= 0)
   {
      fprintf (stderr, "********************************\n");
      fprintf (stderr, "*                              *\n");
      fprintf (stderr, "*  BFP - Extract Binary Data   *\n");
      fprintf (stderr, "*                              *\n");
      fprintf (stderr, "********************************\n");
   }

   fprintf (stderr, "\n\n");

   printf("Enter input file path and name (default path is .):  ");
   if ( (err = scanf("%s", infile_name)) != 1)
   {
      fprintf(stderr, "Problem interpreting input file name.\n");
      sleep(3);
      return(BFP_FAILURE);
   }
   printf("\n");

   /* open the file for read access */
   if ((infile_id = open( infile_name, O_RDONLY )) < 0 )
   {
      fprintf(stderr,
         "extract_bin_data: Error opening %s (errno %d).\n", infile_name, errno);
      sleep(2);
      return (BFP_FAILURE);
   }

   fsize = lseek (infile_id, 0, SEEK_END);    /* find file size */

   /* print the file size for user's convenience */
   fprintf(stderr, "\nFile size (bytes): %d\n\n", fsize);


   printf("Enter start byte offset (0-based):  ");
   if ( (err = scanf("%d", &start_byte)) != 1)
   {
      fprintf(stderr, "Problem interpreting start byte.\n");
      sleep(3);
      return(BFP_FAILURE);
   }
   printf("\n");

   printf("Enter number of bytes to extract:  ");
   if ( (err = scanf("%d", &num_bytes)) != 1)
   {
      fprintf(stderr, "Problem interpreting num bytes.\n");
      sleep(3);
      return(BFP_FAILURE);
   }
  
   if ( (start_byte + num_bytes) > fsize )
   {
      fprintf(stderr, "Error: file size is only %d\n", fsize);
      sleep(2);
      return (0);
   }

   printf("\n");

   printf("Specify output format:\n");
   printf(" 1: Output as ASCII characters\n");
   printf(" 2: Output as 1-byte integers\n");
   printf("%d: Cancel and return to main menu \n\n", EXIT_CODE);
   
   printf("Enter selection:  ");
   if ( (err = scanf("%d", &format_select)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return(BFP_FAILURE);
   }
   printf("\n");

   printf("Specify output location:\n");
   printf(" 1: Output to screen\n");
   printf(" 2: Store in file\n");
   printf("%d: Cancel and return to main menu \n\n", EXIT_CODE);
   
   printf("Enter selection:  ");
   if ( (err = scanf("%d", &loc_select)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   switch (loc_select)
   {
      case 1:
         outfile_id = STDOUT_FILENO;
         Screen_Output = BFP_TRUE;
         break;

      case 2:
         /* prompt user to enter output file name */
         printf("\n\nEnter output file path and name (default path is .):  ");
         if ( (err = scanf("%s", outfile_name)) != 1)
         {
            fprintf(stderr, "Problem interpreting output file name.\n");
            sleep(3);
            return(BFP_FAILURE);
         }
         printf("\n");

         /* open file with write access */
         if ((outfile_id = open( outfile_name, O_CREAT | O_WRONLY, 0777 )) < 0 )
         {
            fprintf(stderr,
               "extract_bin_data: Error opening %s (errno %d).\n", outfile_name, errno);
            sleep(2);
            return (BFP_FAILURE);
         }
         break;

      case EXIT_CODE:
         return (BFP_SUCCESS);
         break;

      default:
         fprintf (stderr, "Invalid Selection...Please try again\n");
         sleep (2);
         break;
   }


   switch (format_select)
   {
      case 1:
         extract_char_data(infile_id, start_byte, num_bytes, outfile_id);
         break;   
      
      case 2:
         extract_1byte_int_data(infile_id, start_byte, num_bytes, outfile_id);
         break;

      case EXIT_CODE:
         break;

      default:
         fprintf (stderr, "Invalid Selection...Please try again\n");
         sleep (2);
         break;
   }

   return 0;

} /* end extract_bin_data */


/********************************************************************************

     interpret_bin_to_ascii

********************************************************************************/
int interpret_bin_to_ascii( char* in_file_name, char* out_file_name)
{
   int ret_val = BFP_SUCCESS;
   int in_fd;
   int out_fd;
   int fsize;
   char* buf;

   /* Attempt to open input file for read access */
   in_fd = open (in_file_name, O_RDONLY);   /* open the file for read only */
   if (in_fd < 0) 
   {
      fprintf (stderr, "open file %s failed (errno %d)\n", in_file_name, errno);
      sleep(3);
      return(BFP_FAILURE);
   }

   if ( out_file_name != NULL )
   {
      /* Attempt to open output file for write access */
      out_fd = open (out_file_name, O_CREAT | O_WRONLY, 0777);
      if (out_fd < 0) 
      {
         fprintf (stderr, "open file %s failed (errno %d)\n", out_file_name, errno);
         sleep(3);
         return(BFP_FAILURE);
      }
   }
   else
   {
      out_fd = fileno(stderr);
   }

   /* Read input data and store in internal buffer */
   fsize = lseek (in_fd, 0, SEEK_END);    /* find file size */
   buf = (char *)malloc (fsize);          /* allocate memory space */
   if (buf == NULL)
   { 
      fprintf (stderr, "malloc (%d) failed\n", fsize);
      sleep(3);
      return(BFP_FAILURE);
   }

   lseek (in_fd, 0, SEEK_SET);  /* move the file read pointer to the beginning of
                                the file */

   /* read the entire file into memory */
   if (read (in_fd, buf, fsize) != fsize) 
   {
      fprintf (stderr, "read failed (errno %d)\n", errno);
      sleep(3);
      return(BFP_FAILURE);
   } 

   if ( Screen_Output == BFP_TRUE )
   {
      fprintf(stderr, "============= Contents of file: ==================\n");
   }

   /* Write as ASCII text to stderr or the output file */
   if ( write (out_fd, buf, fsize) != fsize )
   {
      fprintf (stderr, "Write failed (errno %d)\n", errno);
      sleep(3);
      return(BFP_FAILURE);
   } 

   /* If output is to screen, pause so the user can see it before 
      it gets cleared! */
   if ( Screen_Output == BFP_TRUE )
   {
      fprintf(stderr, "\n==================================================\n");
      
      pause_user();  /* pause til user hits Enter */
   }

   /* free memory */
   if ( buf != NULL )
   {
      free (buf);
   }

   return ret_val;

} /* end interpret_bin_to_ascii */


/********************************************************************************

     interpret_bin_to_byte

********************************************************************************/
int interpret_bin_to_byte( char* in_file_name, char* out_file_name )
{
   int		err;
   int		in_fd;
   int		fsize;
   int		byte_index;
   char*	buf;
   int		ret_val		= BFP_SUCCESS;
   int		format_select;
   FILE*	out_fp		= NULL;


   /* Attempt to open input file for read access */
   in_fd = open (in_file_name, O_RDONLY);   /* open the file for read only */
   if (in_fd < 0) 
   {
      fprintf (stderr, "open %s failed (errno %d)\n", in_file_name, errno);
      sleep(3);
      return(BFP_FAILURE);
   }

   if ( out_file_name != NULL )
   {
      /* Attempt to open output file for write access */
      out_fp = fopen (out_file_name, "w");  /* open the file for write only */
      if (out_fp == NULL) 
      {
         fprintf (stderr, "open %s failed (errno %d)\n", out_file_name, errno);
         sleep(3);
         return(BFP_FAILURE);
      }
   }

   /* Read input data and store in internal buffer */
   fsize = lseek (in_fd, 0, SEEK_END);    /* find file size */
   buf = (char *) malloc(fsize);          /* allocate memory space */
   if (buf == NULL)
   { 
      fprintf (stderr, "malloc (%d) failed\n", fsize);
      sleep(3);
      return(BFP_FAILURE);
   }

   lseek (in_fd, 0, SEEK_SET);  /* move the file read pointer to the beginning of
                                the file */

   /* read the entire file into memory */
   if (read (in_fd, buf, fsize) != fsize) 
   {
      fprintf (stderr, "read failed (errno %d)\n", errno);
      sleep(3);
      return(BFP_FAILURE);
   } 

   /* prompt the user to choose between hex and decimal output */
   printf("\n");

   printf("Specify output format:\n");
   printf(" 1: Decimal\n");
   printf(" 2: Hexidecimal\n");
   printf("%d: Cancel and return to main menu \n\n", EXIT_CODE);
   
   printf("Enter selection:  ");
   if ( (err = scanf("%d", &format_select)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return(BFP_FAILURE);
   }
   printf("\n");

   if ( Screen_Output == BFP_TRUE )
   {
      fprintf(stderr, "============= Contents of file: ==================\n");

      for ( byte_index = 1; byte_index <= fsize; byte_index++ )
      {
         /* Write as ASCII text to stderr or the output file */
         if ( format_select == 1 )
         {  /* decimal */
            fprintf(stderr, "%03d ", *(unsigned char*)(buf + (byte_index-1)));
         }
         else 
         {  /* hexidecimal */
            fprintf(stderr, "%02x ", *(unsigned char*)(buf + (byte_index-1)));
         }
   
         if ( (byte_index % NUM_BYTES_PER_ROW) == 0 )
            fprintf(stderr, "\n");
      }

      fflush(stderr);  /* flush any straggling data */


      /* Pause so the user can see it before it gets cleared! */

      fprintf(stderr, "\n==================================================\n");

      pause_user();  /* pause til user user presses Enter */

   }
   else
   {
      /* write data to file in readable ascii format */
      for ( byte_index = 1; byte_index <= fsize; byte_index++ )
      {
         /* Write as ASCII text to output file */
         if ( format_select == 1 )
         {  /* decimal */
            fprintf(out_fp, "%03d ", *(unsigned char*)(buf + (byte_index-1)));
         }
         else 
         {  /* hexidecimal */
            fprintf(out_fp, "%02x ", *(unsigned char*)(buf + (byte_index-1)));
         }
   
         if ( (byte_index % NUM_BYTES_PER_ROW) == 0 )
            fprintf(out_fp, "\n");
      }

      fflush(out_fp);  /* flush any straggling data */
   }

   /* free memory */
   if ( buf != NULL )
   {
      free (buf);
   }

   return ret_val;

} /* end interpret_bin_to_byte */


/********************************************************************************

     interpret_bin_to_short

********************************************************************************/
int interpret_bin_to_short( char* in_file_name, char* out_file_name )
{
   int		err;
   int		in_fd;
   int		fsize;
   int		short_index;
   short*	buf;
   int		ret_val		= BFP_SUCCESS;
   int		format_select;
   FILE*	out_fp		= NULL;


   /* Attempt to open input file for read access */
   in_fd = open (in_file_name, O_RDONLY);   /* open the file for read only */
   if (in_fd < 0) 
   {
      fprintf (stderr, "open %s failed (errno %d)\n", in_file_name, errno);
      sleep(3);
      return(BFP_FAILURE);
   }

   if ( out_file_name != NULL )
   {
      /* Attempt to open output file for write access */
      out_fp = fopen (out_file_name, "w");  /* open the file for write only */
      if (out_fp == NULL) 
      {
         fprintf (stderr, "open %s failed (errno %d)\n", out_file_name, errno);
         sleep(3);
         return(BFP_FAILURE);
      }
   }

   /* Read input data and store in internal buffer */
   fsize = lseek (in_fd, 0, SEEK_END);    /* find file size */
   buf = (short *) malloc(fsize);          /* allocate memory space */
   if (buf == NULL)
   { 
      fprintf (stderr, "malloc (%d) failed\n", fsize);
      sleep(3);
      return(BFP_FAILURE);
   }

   lseek (in_fd, 0, SEEK_SET);  /* move the file read pointer to the beginning of
                                the file */

   /* read the entire file into memory */
   if (read (in_fd, buf, fsize) != fsize) 
   {
      fprintf (stderr, "read failed (errno %d)\n", errno);
      sleep(3);
      return(BFP_FAILURE);
   } 

   /* prompt the user to choose between hex and decimal output */
   printf("\n");

   printf("Specify output format:\n");
   printf(" 1: Decimal\n");
   printf(" 2: Hexidecimal\n");
   printf("%d: Cancel and return to main menu \n\n", EXIT_CODE);
   
   printf("Enter selection:  ");
   if ( (err = scanf("%d", &format_select)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return(BFP_FAILURE);
   }
   printf("\n");

   if ( Screen_Output == BFP_TRUE )
   {
      fprintf(stderr, "============= Contents of file: ==================\n");

      for ( short_index = 1; short_index <= (fsize/sizeof(short)); short_index++ )
      {
         /* Write as ASCII text to stderr or the output file */
         if ( format_select == 1 )
         {  /* decimal */
            fprintf(stderr, "%05d ", *(unsigned short*)(buf + (short_index-1)));
         }
         else 
         {  /* hexidecimal */
            fprintf(stderr, "%04x ", *(unsigned short*)(buf + (short_index-1)));
         }
   
         if ( (short_index % NUM_SHORTS_PER_ROW) == 0 )
            fprintf(stderr, "\n");
      }

      fflush(stderr);  /* flush any straggling data */


      /* Pause so the user can see it before it gets cleared! */

      fprintf(stderr, "\n==================================================\n");

      pause_user();  /* pause til user user presses Enter */
   }
   else
   {
      /* write data to file in readable ascii format */
      for ( short_index = 1; short_index <= fsize; short_index++ )
      {
         /* Write as ASCII text to output file */
         if ( format_select == 1 )
         {  /* decimal */
            fprintf(out_fp, "%05d ", *(unsigned short*)(buf + (short_index-1)));
         }
         else 
         {  /* hexidecimal */
            fprintf(out_fp, "%04x ", *(unsigned short*)(buf + (short_index-1)));
         }
   
         if ( (short_index % NUM_SHORTS_PER_ROW) == 0 )
            fprintf(out_fp, "\n");
      }

      fflush(out_fp);  /* flush any straggling data */
   }

   /* free memory */
   if ( buf != NULL )
   {
      free (buf);
   }

   return ret_val;

} /* interpret_bin_to_short */


/********************************************************************************

     interpret_bin_to_int

********************************************************************************/
int interpret_bin_to_int( char* in_file_name, char* out_file_name )
{
   int		err;
   int		in_fd;
   int		fsize;
   int		int_index;
   int*		buf;
   int		ret_val		= BFP_SUCCESS;
   int		format_select;
   FILE*	out_fp		= NULL;


   /* Attempt to open input file for read access */
   in_fd = open (in_file_name, O_RDONLY);   /* open the file for read only */
   if (in_fd < 0) 
   {
      fprintf (stderr, "open %s failed (errno %d)\n", in_file_name, errno);
      sleep(3);
      return(BFP_FAILURE);
   }

   if ( out_file_name != NULL )
   {
      /* Attempt to open output file for write access */
      out_fp = fopen (out_file_name, "w");  /* open the file for write only */
      if (out_fp == NULL) 
      {
         fprintf (stderr, "open %s failed (errno %d)\n", out_file_name, errno);
         sleep(3);
         return(BFP_FAILURE);
      }
   }

   /* Read input data and store in internal buffer */
   fsize = lseek (in_fd, 0, SEEK_END);    /* find file size */
   buf = (int *) malloc(fsize);          /* allocate memory space */
   if (buf == NULL)
   { 
      fprintf (stderr, "malloc (%d) failed\n", fsize);
      sleep(3);
      return(BFP_FAILURE);
   }

   lseek (in_fd, 0, SEEK_SET);  /* move the file read pointer to the beginning of
                                the file */

   /* read the entire file into memory */
   if (read (in_fd, buf, fsize) != fsize) 
   {
      fprintf (stderr, "read failed (errno %d)\n", errno);
      sleep(3);
      return(BFP_FAILURE);
   } 

   /* prompt the user to choose between hex and decimal output */
   printf("\n");

   printf("Specify output format:\n");
   printf(" 1: Decimal\n");
   printf(" 2: Hexidecimal\n");
   printf("%d: Cancel and return to main menu \n\n", EXIT_CODE);
   
   printf("Enter selection:  ");
   if ( (err = scanf("%d", &format_select)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return(BFP_FAILURE);
   }
   printf("\n");

   if ( Screen_Output == BFP_TRUE )
   {
      fprintf(stderr, "============= Contents of file: ==================\n");

      for ( int_index = 1; int_index <= (fsize/sizeof(int)); int_index++ )
      {
         /* Write as ASCII text to stderr or the output file */
         if ( format_select == 1 )
         {  /* decimal */
            fprintf(stderr, "%10d ", *(unsigned int*)(buf + (int_index-1)));
         }
         else 
         {  /* hexidecimal */
            fprintf(stderr, "%08x ", *(unsigned int*)(buf + (int_index-1)));
         }
   
         if ( (int_index % NUM_INTS_PER_ROW) == 0 )
            fprintf(stderr, "\n");
      }

      fflush(stderr);  /* flush any straggling data */


      /* Pause so the user can see it before it gets cleared! */

      fprintf(stderr, "\n==================================================\n");

      pause_user();  /* pause til user presses Enter */
   }
   else
   {
      /* write data to file in readable ascii format */
      for ( int_index = 1; int_index <= (fsize/sizeof(int)); int_index++ )
      {
         /* Write as ASCII text to output file */
         if ( format_select == 1 )
         {  /* decimal */
            fprintf(out_fp, "%10d ", *(unsigned int*)(buf + (int_index-1)));
         }
         else 
         {  /* hexidecimal */
            fprintf(out_fp, "%08x ", *(unsigned int*)(buf + (int_index-1)));
         }
   
         if ( (int_index % NUM_INTS_PER_ROW) == 0 )
            fprintf(out_fp, "\n");
      }

      fflush(out_fp);  /* flush any straggling data */
   }

   /* free memory */
   if ( buf != NULL )
   {
      free (buf);
   }

   return ret_val;

} /* end interpret_bin_to_int */


/********************************************************************************

     interpret_bin_to_struct

********************************************************************************/
int interpret_bin_to_struct( char* in_file_name )
{
   int		i		= 0;
   int		fd		= 0;
   int		rd_sz		= 0;
   int		fld_idx		= 0;
   int		err		= 0;
   int		offset		= 0;
   int		fld_nitems	= 0;
   int		fld_type	= 0;
   int		fld_size	= 0;
   int		fld_size_bytes	= 0;
   int		fld_offset	= 0;
   int		data_size_bytes	= 0;
   void*	data_buf	= NULL;
   void*	fld_value	= NULL;
   char*	fld_type_name	= NULL;
   SMI_info_t*	struct_info	= NULL;
   char		struct_name[MAX_STRUCT_NAME_SIZE];
   struct stat  fstat_buf;


   /* Attempt to open input file for read access */
   fd = open (in_file_name, O_RDONLY);   /* open the file for read only */
   if (fd < 0) 
   {
      fprintf (stderr,
         "interpret_bin_to_struct: open %s failed (errno %d)\n",
         in_file_name, errno);
      sleep(3);
      return (BFP_FAILURE);
   }

   /* if architecture is Li'l Endian, prompt the user to choose the format of the 
      data (Li'l vs Big Endian) */
   if ( (ByteSwapNeeded = Set_byte_swap_flag()) == BFP_FAILURE )
   {
      fprintf(stderr,
         "interpret_bin_to_struct: Problem setting byte swap flag.\n");
      ByteSwapNeeded = BFP_FALSE;
      sleep(3);
      return (BFP_FAILURE);
   }

   /* prompt the user to enter the structure that is used in the input file */
   fprintf(stderr, "Enter structure name: ");
   if ( (err = scanf("%s", struct_name)) != 1)
   {
      fprintf(stderr,
         "interpret_bin_to_struct: Problem interpreting struct name.\n");
      sleep(3);
      return (BFP_FAILURE);
   }
     
   /* prompt the user to enter the byte offset to the structured data */
   fprintf(stderr,
      "Enter the byte offset of the structured data in the file (0-based): ");
   if ( (err = scanf("%d", &offset)) != 1)
   {
      fprintf(stderr,
         "interpret_bin_to_struct: Problem interpreting offset.\n");
      sleep(3);
      return (BFP_FAILURE);
   }
 
   /* retrieve the file size in bytes */
   err = fstat(fd, &fstat_buf);
   if (err < 0)
   {
      fprintf(stderr,
         "interpret_bin_to_struct: call to fstat failed for file %s\n",
         in_file_name);
      return(BFP_FAILURE);
   }
   data_size_bytes = fstat_buf.st_size - offset;

   /* allocate buffer to hold data */
   data_buf = (void *)malloc(data_size_bytes);
   if ( data_buf == NULL )
   {
      fprintf (stderr, "malloc (%d) failed\n", data_size_bytes);
      sleep(3);
      return(BFP_FAILURE);
   }

   /* read the data from the file */    
   lseek(fd, offset, SEEK_SET);
   if ( (rd_sz = read(fd, data_buf, data_size_bytes)) != data_size_bytes)
   {
      fprintf(stderr, "interpret_bin_to_struct: Problem reading file data.\n");
      sleep(3);
      return (BFP_FAILURE);
   }

   /* byte swap the data if necessary */
   if (ByteSwapNeeded == BFP_TRUE)
   {
      /* call SMIA function to byte swap the structured data */
      err = SMIA_bswap_input( struct_name, data_buf, data_size_bytes );
   }

   /* using smi, get and print the structure metadata */
   struct_info = BFP_smi_info (struct_name, NULL);

   if ( struct_info != NULL )
   {
      fld_type_name = (char*) malloc (MAX_STRUCT_NAME_SIZE);
      fprintf(stderr, "\n%s info:\n\n", struct_name);
      fprintf(stderr, "\t%-10s%3s%-15d\n", "size (bytes)", " = ",
         struct_info->size);
      fprintf(stderr, "\t%-10s%3s%-15d\n", "num fields", " = ",
         struct_info->n_fields);
      fprintf(stderr, "\n\t%-10s\n",
         "---------------------------------------------");
      fprintf(stderr, "\t%-10s\n", "field data   ");
      fprintf(stderr, "\t%-10s\n",
         "---------------------------------------------");
      for ( fld_idx = 0; fld_idx < struct_info->n_fields; fld_idx++ )
      {
         fld_size = ((struct_info->fields)+fld_idx)->size;
         fld_nitems = ((struct_info->fields)+fld_idx)->n_items;
         fld_type_name = ((struct_info->fields)+fld_idx)->type;
         fld_offset = ((struct_info->fields)+fld_idx)->offset;

         /* determine total byte size of the field and allocate sufficient space */
         fld_size_bytes = fld_nitems * fld_size;
         fld_value = (void *) malloc ( fld_size_bytes );

         /* print field name */
         fprintf(stderr, " \t%-20s%3s", ((struct_info->fields)+fld_idx)->name, " = ");

         /* make copy of field value */
         memcpy(fld_value, data_buf + fld_offset, fld_size_bytes);

         /* depending on data type, determine and print field value */
         if ( strcmp(CHAR_TYPE_NAME, fld_type_name) == 0)
         {
            fld_type = CHAR_TYPE;
      
            if ( fld_nitems == 1 )
            {
               fprintf(stderr, "%#x\n", *(char*)fld_value);
            }
            else if ( fld_nitems > 1 )
            {
               int first_fld = BFP_TRUE;
               for ( i = 0; i < fld_nitems; i++ )
               {
                  if ( first_fld )
                  {
                     fprintf(stderr, "[%d]:%c (%#x)\n", i,
                        *(((char*)(fld_value)) + i),
                        *(((char*)(fld_value)) + i));
                     first_fld = BFP_FALSE;
                  }
                  else
                  {
                     fprintf(stderr, "%31s[%d]:%c (%#x)\n", " ", i,
                        *(((char*)(fld_value)) + i),
                        *(((char*)(fld_value)) + i));
                  }
               }
            }
         }
         else if (strcmp(UCHAR_TYPE_NAME, fld_type_name) == 0)
         {
            fld_type = UCHAR_TYPE;

            if ( fld_nitems == 1 )
            {
               fprintf(stderr, "%#x\n", *(unsigned char*)fld_value);
            }
            else if ( fld_nitems > 1 )
            {
               int first_fld = BFP_TRUE;
               for ( i = 0; i < fld_nitems; i++ )
               {
                  if ( first_fld )
                  {
                     fprintf(stderr, "[%d]:%c (%#x)\n", i,
                        *(((unsigned char*)(fld_value)) + i),
                        *(((unsigned char*)(fld_value)) + i));
                     first_fld = BFP_FALSE;
                  }
                  else
                  {
                     fprintf(stderr, "%31s[%d]:%c (%#x)\n", " ", i,
                        *(((unsigned char*)(fld_value)) + i),
                        *(((unsigned char*)(fld_value)) + i));
                  }
               }
            }
         }
         else if ( strcmp(SHORT_TYPE_NAME, fld_type_name) == 0)
         {
            fld_type = SHORT_TYPE;
      
            if ( fld_nitems == 1 )
            {
               fprintf(stderr, "%d (%#x)\n", *(short*)fld_value,
                  *(short*)fld_value);
            }
            else if ( fld_nitems > 1 )
            {
               int first_fld = BFP_TRUE;
               for ( i = 0; i < fld_nitems; i++ )
               {
                  if ( first_fld )
                  {
                     fprintf(stderr, "[%d]:%d (%#x)\n", i,
                        *(((short*)(fld_value)) + i),
                        *(((short*)(fld_value)) + i));
                     first_fld = BFP_FALSE;
                  }
                  else
                  {
                     fprintf(stderr, "%31s[%d]:%d (%#x)\n", " ", i,
                        *(((short*)(fld_value)) + i),
                        *(((short*)(fld_value)) + i));
                  }
               }
            }
         }
         else if ( strcmp(USHORT_TYPE_NAME, fld_type_name) == 0)
         {
            fld_type = USHORT_TYPE;
      
            if ( fld_nitems == 1 )
            {
               fprintf(stderr, "%u (%#x)\n", *(unsigned short*)fld_value,
                  *(unsigned short*)fld_value);
            }
            else if ( fld_nitems > 1 )
            {
               int first_fld = BFP_TRUE;
               for ( i = 0; i < fld_nitems; i++ )
               {
                  if ( first_fld )
                  {
                     fprintf(stderr, "[%d]:%u (%#x)\n", i,
                        *(((unsigned short*)(fld_value)) + i),
                        *(((unsigned short*)(fld_value)) + i));
                     first_fld = BFP_FALSE;
                  }
                  else
                  {
                     fprintf(stderr, "%31s[%d]:%u (%#x)\n", " ", i,
                        *(((unsigned short*)(fld_value)) + i),
                        *(((unsigned short*)(fld_value)) + i));
                  }
               }
            }
         }
         else if ( strcmp(INT_TYPE_NAME, fld_type_name) == 0)
         {
            fld_type = INT_TYPE;
      
            if ( fld_nitems == 1 )
            {
               fprintf(stderr, "%d (%#x)\n", *(int*)fld_value,
                  *(int*)fld_value);
            }
            else if ( fld_nitems > 1 )
            {
               int first_fld = BFP_TRUE;
               for ( i = 0; i < fld_nitems; i++ )
               {
                  if ( first_fld )
                  {
                     fprintf(stderr, "[%d]:%d (%#x)\n", i,
                        *(((int*)(fld_value)) + i),
                        *(((int*)(fld_value)) + i ));
                     first_fld = BFP_FALSE;
                  }
                  else
                  {
                     fprintf(stderr, "%31s[%d]:%d (%#x)\n", " ", i,
                        *(((int*)(fld_value)) + i),
                        *(((int*)(fld_value)) + i ));
                  }
               }
            }
         }
         else if ( strcmp(UINT_TYPE_NAME, fld_type_name) == 0)
         {
            fld_type = UINT_TYPE;
      
            if ( fld_nitems == 1 )
            {
               fprintf(stderr, "%u (%#x)\n", *(unsigned int*)fld_value,
                  *(unsigned int*)fld_value);
            }
            else if ( fld_nitems > 1 )
            {
               int first_fld = BFP_TRUE;
               for ( i = 0; i < fld_nitems; i++ )
               {
                  if ( first_fld )
                  {
                     fprintf(stderr, "[%d]:%u (%#x)\n", i,
                        *(((unsigned int*)(fld_value)) + i),
                        *(((unsigned int*)(fld_value)) + i));
                     first_fld = BFP_FALSE;
                  }
                  else
                  {
                     fprintf(stderr, "%31s[%d]:%u (%#x)\n", " ", i,
                        *(((unsigned int*)(fld_value)) + i),
                        *(((unsigned int*)(fld_value)) + i));
                  }
               }
            }
         }
         else if ( strcmp(FLOAT_TYPE_NAME, fld_type_name) == 0)
         {
            fld_type = FLOAT_TYPE;
      
            if ( fld_nitems == 1 )
            {
               fprintf(stderr, "%f\n", *(float*)fld_value);
            }
            else if ( fld_nitems > 1 )
            {
               int first_fld = BFP_TRUE;
               for ( i = 0; i < fld_nitems; i++ )
               {
                  if ( first_fld )
                  {
                     fprintf(stderr, "[%d]:%f\n", i,
                        *(((float*)(fld_value)) + i));
                     first_fld = BFP_FALSE;
                  }
                  else
                  {
                     fprintf(stderr, "%31s[%d]:%f\n", " ", i,
                        *(((float*)(fld_value)) + i));
                  }
               }
            }
         }
         else
         {
            fprintf(stderr, "Unrecognized data type.\n");
         }

      }
   }
   else
   {
      fprintf(stderr, "interpret_bin_to_struct: structure %s not found.\n",
         struct_name);
      sleep(3);
      return(BFP_FAILURE);
   }

   /* free memory */
   if ( fld_value != NULL )
      free(fld_value);
   if ( data_buf != NULL )
      free(data_buf);

   /* close file */
   close(fd);

   pause_user(); /* Pause til user presses Enter */

   return 0;

} /* end interpret_bin_to_struct */


/********************************************************************************

     modify_char_data

********************************************************************************/
int modify_char_data( int fd, int size, int offset )
{
   int  err;
   int  num_chars;
   int  size_ok = BFP_FALSE;
   char select1;
   char select2;
   char *buf = NULL;
   char *new_str = NULL;
   char *old_str = NULL;


   while ( size_ok != BFP_TRUE )
   {
      fprintf(stderr, "Enter the number of characters you want to replace:\n");
      if ( (err = scanf("%d", &num_chars)) != 1)
      {
         fprintf(stderr, "Problem interpreting num chars.\n");
         sleep(3);
         return (BFP_FAILURE);
      }

      if ( offset + num_chars > size )
      { 
         fprintf(stderr, "File size (%d) is too small for the ", size);
         fprintf(stderr, "offset (%d) and number of ", offset);
         fprintf(stderr, "characters (%d) you entered.\n\n", num_chars);
         fprintf(stderr, "Try again (y/n)? ");
         if ( (err = scanf("%s", &select1)) != 1)
         {
            fprintf(stderr, "Problem interpreting selection.\n");
            sleep(3);
            return (BFP_FAILURE);
         }
         if ( select1 == 'n' )
         { 
            return 0;
         }
      }
      else
      {
         size_ok = BFP_TRUE; 
      }
   }

   /* Extract the data from the file and have the user verify that this is what
      they wish to change. */

   /* seek to the proper location in the file */
   lseek(fd, offset, SEEK_SET);

   /* extract data */
   old_str = (char *)malloc(num_chars + 1);
   if ( read (fd, old_str, num_chars) != num_chars )
   {
      fprintf(stderr, "modify_char_data: problem extracting data (errno %d).\n",
         errno);
      sleep(3);
      return (BFP_FAILURE);
   }

   fprintf(stderr, "============= Data to be Replaced: ==================\n");

   fprintf(stderr, "%s", old_str);
   fprintf(stderr, "\n");

   fprintf(stderr, "=====================================================\n");

   fprintf(stderr, "Is this the data you want to replace (y/n)? ");
   if ( (err = scanf("%s", &select2)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   if ( select2 == 'y' )
   {
      /* Reset file pointer to proper location */
      lseek(fd, offset, SEEK_SET);

      /* allocate buffer to hold new data */
      buf = (char *)malloc(num_chars + 1);
      new_str = (char *)malloc(num_chars + 1);

      /* prompt user to enter new data */
      fprintf(stderr, "\n\nNOTE: Entering tabs or spaces currently does not work properly.\n");
      fprintf(stderr, "You can use a different character in the meantime\n");
      fprintf(stderr, "(for instance an underscore) and then use the tool to later\n");
      fprintf(stderr, "modify that byte and turn it into the numerical equivalent\n");
      fprintf(stderr, "of a space (0x20).\n");
      fprintf(stderr, "\nEnter new %d character string:\n", num_chars);

      if ( (err = scanf("%s", buf)) != 1)
      {
         fprintf(stderr, "Problem interpreting buffer.\n");
         sleep(3);
         return(BFP_FAILURE);
      }

      /* save only the indicated # of characters */
      strncpy(new_str, buf, num_chars);

      /* seek to the proper location in the file and write the new data */
      write(fd, new_str, num_chars);
   }

   /* free memory */
   if ( old_str != NULL )
   {
      free (old_str);
   }

   if ( buf != NULL )
   {
      free (buf);
   }

   if ( new_str != NULL )
   {
      free (new_str);
   }

   return 0;

} /* end modify_char_data */


/********************************************************************************

     modify_1byte_int_data

********************************************************************************/
int modify_1byte_int_data( int fd, int size, int offset )
{
   char			select;
   int			err;
   int			do_over = BFP_TRUE;
   unsigned char	orig_byte;
   unsigned int		new_value;


   /* extract the value to be replaced and verify with the user */
   lseek(fd, offset, SEEK_SET);
   if (read(fd, &orig_byte, sizeof(unsigned char)) != sizeof(unsigned char))
   {
      fprintf(stderr, "Problem extracting 1-byte integer.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   fprintf(stderr, "========== Data to be Replaced (in Decimal): ========\n");

   fprintf(stderr, "%02x", orig_byte);
   fprintf(stderr, "\n");

   fprintf(stderr, "=====================================================\n");

   fprintf(stderr, "Is this the data you want to replace (y/n)? ");
   if ( (err = scanf("%s", &select)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   if ( select == 'y' )
   {
      do_over = BFP_TRUE;

      while ( do_over == BFP_TRUE )
      {
         /* prompt user to enter new data */
         fprintf(stderr, "Enter 2 new hex digits (format: d4, 0a, etc.):\n");
         if ( (err = scanf("%2x", (int*) &new_value)) != 1)
         {
            fprintf(stderr, "Problem interpreting buffer.\n");
            sleep(3);
            return(BFP_FAILURE);
         }

         /* verify not greater than MAX_VAL_UCHAR */
         if ( new_value <= MAX_VAL_UCHAR) 
         {
            orig_byte = (unsigned char)new_value;
            lseek(fd, offset, SEEK_SET);
            /* write new value */
            if ( (err = write(fd, &orig_byte, sizeof(unsigned char))) !=
               sizeof(unsigned char))
            {
               fprintf(stderr, "Problem writing new 1-byte int value.\n");
               sleep(3);
               return(BFP_FAILURE);
            }

            /* reset do over flag */
            do_over = BFP_FALSE;
         }
      }
   }

   return 0;

} /* end modify_1byte_int_data */


/********************************************************************************

     modify_2byte_int_data

********************************************************************************/
int modify_2byte_int_data( int fd, int size, int offset )
{
   char			select;
   int			err;
   int			do_over = BFP_TRUE;
   unsigned short	orig_value;
   unsigned int		new_value;

   /* extract the value to be replaced and verify with the user */
   lseek(fd, offset, SEEK_SET);
   if (read(fd, &orig_value, sizeof(unsigned short)) != sizeof(unsigned short))
   {
      fprintf(stderr, "Problem extracting 2-byte integer.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   fprintf(stderr, "========== Data to be Replaced (in Hexidecimal): ========\n");

   fprintf(stderr, "%04x", orig_value);
   fprintf(stderr, "\n");

   fprintf(stderr, "=====================================================\n");

   fprintf(stderr, "Is this the data you want to replace (y/n)? ");
   if ( (err = scanf("%s", &select)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   if ( select == 'y' )
   {
      do_over = BFP_TRUE;

      while ( do_over == BFP_TRUE )
      {
         /* prompt user to enter new data */
         fprintf(stderr, "Enter 4 new hex digits (format: a0d4 or 0xa0d4):\n");
         if ( (err = scanf("%4x", (int*) &new_value)) != 1)
         {
            fprintf(stderr, "Problem interpreting buffer.\n");
            sleep(3);
            return(BFP_FAILURE);
         }

         /* verify not greater than MAX_VAL_USHORT */
         if ( new_value <= MAX_VAL_USHORT ) 
         {
            orig_value = (unsigned short)new_value;
            lseek(fd, offset, SEEK_SET);
            /* write new value */
            if ( (err = write(fd, &orig_value, sizeof(unsigned short))) !=
               sizeof(unsigned short))
            {
               fprintf(stderr, "Problem writing new 2-byte int value.\n");
               sleep(3);
               return(BFP_FAILURE);
            }

            /* reset do over flag */
            do_over = BFP_FALSE;
         }
      }
   }

   return 0;
} /* end modify_2byte_int_data */


/********************************************************************************

     modify_4byte_int_data

********************************************************************************/
int modify_4byte_int_data(int fd, int size, int offset)
{
   char			select;
   int			err;
   int			do_over = BFP_TRUE;
   unsigned int		orig_value;
   unsigned int		new_value;


   /* extract the value to be replaced and verify with the user */
   lseek(fd, offset, SEEK_SET);
   if (read(fd, &orig_value, sizeof(unsigned int)) != sizeof(unsigned int))
   {
      fprintf(stderr, "Problem extracting 4-byte integer.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   fprintf(stderr, "========== Data to be Replaced (in Hexidecimal): ========\n");

   fprintf(stderr, "%08x", orig_value);
   fprintf(stderr, "\n");

   fprintf(stderr, "=====================================================\n");

   fprintf(stderr, "Is this the data you want to replace (y/n)? ");
   if ( (err = scanf("%s", &select)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   if ( select == 'y' )
   {
      do_over = BFP_TRUE;

      while ( do_over == BFP_TRUE )
      {
         /* prompt user to enter new data */
         fprintf(stderr, "Enter 8 new hex digits (format: a0d4bcf1 or 0xa0d4bcf1):\n");
         if ( (err = scanf("%8x", (unsigned int*) &new_value)) != 1)
         {
            fprintf(stderr, "Problem interpreting buffer.\n");
            sleep(3);
         }
         else
         {
            /* reset do over flag */
            do_over = BFP_FALSE;
         }
      }
   }

   return 0;
} /* end modify_4byte_int_data */


/********************************************************************************

     modify_struct_data

********************************************************************************/
int modify_struct_data(int fd, int size)
{
   int		rd_sz		= 0;
   int		fld_idx		= 0;
   int		err		= 0;
   int		offset		= 0;
   int		fld_nitems	= 0;
   int		fld_size	= 0;
   int		fld_offset	= 0;
   int		fld_type	= 0;
   int		fld_size_bytes	= 0;
   int		arr_index	= 0;
   char		select		= 0;
   void*	fld_value	= NULL;
   char*	fld_name	= NULL;
   char*	fld_type_name	= NULL;
   SMI_info_t*	struct_info	= NULL;
   char		struct_name[MAX_STRUCT_NAME_SIZE];
   char		struct_field[MAX_STRUCT_NAME_SIZE];


   /* if architecture is Li'l Endian, prompt the user to choose the format of the
      data (Li'l vs Big Endian) */
   if ( (ByteSwapNeeded = Set_byte_swap_flag()) == BFP_FAILURE )
   {
      fprintf(stderr,
         "modify_struct_data: Problem setting byte swap flag.\n");
      ByteSwapNeeded = BFP_FALSE;
      sleep(3);
      return (BFP_FAILURE);
   }

   /* prompt the user to enter the structure that is used in the input file */
   fprintf(stderr, "Enter structure name: ");
   if ( (err = scanf("%s", struct_name)) != 1)
   {
      fprintf(stderr, "modify_struct_data: Problem interpreting struct name.\n");
      sleep(3);
      return (BFP_FAILURE);
   }
     
   /* prompt the user to enter the byte offset to the structured data */
   fprintf(stderr,
      "Enter the byte offset of the structured data in the file (0-based): ");
   if ( (err = scanf("%d", &offset)) != 1)
   {
      fprintf(stderr, "modify_struct_data: Problem interpreting offset.\n");
      sleep(3);
      return (BFP_FAILURE);
   }
    
   /* using smi, need to get and print the structure field names */
   struct_info = BFP_smi_info (struct_name, NULL);

   if ( struct_info != NULL )
   {
      int first_fld = BFP_TRUE;
      fprintf(stderr, "\n%s info:\n", struct_name);
      fprintf(stderr, "%20s %-15d\n", "size (bytes) =", struct_info->size);
      fprintf(stderr, "%20s %-15d\n", "num fields =", struct_info->n_fields);
      fprintf(stderr, "%20s ", "field names =");
      for ( fld_idx = 0; fld_idx < struct_info->n_fields; fld_idx++ )
      {
         if ( first_fld )
         {
            fprintf(stderr, "%-20s\n", ((struct_info->fields)+fld_idx)->name);
            first_fld = BFP_FALSE;
         }
         else
         {
            fprintf(stderr, "%20s %-20s\n", " ",
               ((struct_info->fields)+fld_idx)->name);
         }
      }
   }
   else
   {
      fprintf(stderr, "modify_struct_data: structure %s not found.\n",
         struct_name);
      sleep(3);
      return(BFP_FAILURE);
   }

   /* then prompt the user to enter a structure field to be modified */
   fprintf(stderr, "\nEnter structure field name you wish to modify: ");
   if ( (err = scanf("%s", struct_field)) != 1)
   {
      fprintf(stderr, "modify_struct_data: Problem interpreting struct field.\n");
      sleep(3);
      return (BFP_FAILURE);
   }

   /* allocate memory for fld info */ 
   fld_name = (char*) malloc (MAX_STRUCT_NAME_SIZE);
   fld_type_name = (char*) malloc (MAX_STRUCT_NAME_SIZE);

   /* find the field data */
   for ( fld_idx = 0; fld_idx < struct_info->n_fields; fld_idx++ )
   {
      fld_name = ((struct_info->fields)+fld_idx)->name;
      if ( strcmp(struct_field, fld_name) == 0 )
      {
         fprintf(stderr, "\n%s info:\n", fld_name);
         fld_type_name = ((struct_info->fields)+fld_idx)->type;
         fprintf(stderr, "\n%20s %-15s\n", "type =", fld_type_name);
         fld_nitems = ((struct_info->fields)+fld_idx)->n_items;
         fprintf(stderr, "%20s% -15d\n", "num items =", fld_nitems);
         fld_size = ((struct_info->fields)+fld_idx)->size;
         fprintf(stderr, "%20s% -15d\n", "size =", fld_size);
         fld_offset = ((struct_info->fields)+fld_idx)->offset;
         fprintf(stderr, "%20s% -15d\n", "offset =", fld_offset);

         break; /* found what we wanted, break out of loop */
      }
   }
 
   /* determine total byte size of the field and allocate sufficient space */
   fld_size_bytes = fld_nitems * fld_size;
   fld_value = (void *) malloc ( fld_size_bytes );

   /* read the field value from the file */
   lseek(fd, offset+fld_offset, SEEK_SET);
   if ( (rd_sz = read(fd, fld_value, fld_size_bytes)) != fld_size_bytes )
   {
      fprintf(stderr, "modify_struct_data: Problem reading field value.\n");
      sleep(3);
      return (BFP_FAILURE);
   }

   /* process according to field data type */
   if ( strcmp(CHAR_TYPE_NAME, fld_type_name) == 0)
   {
      fld_type = CHAR_TYPE;

      if ( fld_nitems == 1 )
      {
         fprintf(stderr, "%20s %#x\n", "current value =", *(char*)fld_value);
      }
      else if ( fld_nitems > 1 )
      {
         int i;
         fprintf(stderr, "%20s\n", "current values =");
         for ( i = 0; i < fld_nitems; i++ )
         {
            fprintf(stderr, "%21s[%d]:%c (%#x)\n", " ", i,
               *(((char*)(fld_value)) + i),
               *(((char*)(fld_value)) + i));
         }
      
         /* prompt user to enter array element to change */
         fprintf(stderr, "\nWhich array value do you want to change?\n");
         fprintf(stderr, "Enter the 0-based index number (i.e. Enter 0\n");
         fprintf(stderr, "to change the value stored at %s\[0]): ", fld_name);
         if ( (err = scanf("%d", &arr_index)) != 1)
         {
            fprintf(stderr,
               "modify_struct_data: Problem interpreting array index.\n");
            sleep(3);
            return (BFP_FAILURE);
         }

         /* print value to be changed */
         fprintf(stderr, "\nCurrent value of %s[%d]: %c (%#x)\n", fld_name, arr_index,
            *(((char*)(fld_value)) + arr_index),
            *(((char*)(fld_value)) + arr_index));
      }
   }
   else if (strcmp(UCHAR_TYPE_NAME, fld_type_name) == 0)
   {
      fld_type = UCHAR_TYPE;

      if ( fld_nitems == 1 )
      {
         fprintf(stderr, "%20s %#x\n", "current value =",
            *(unsigned char*)fld_value);
      }
      else if ( fld_nitems > 1 )
      {
         int i;
         fprintf(stderr, "%20s\n", "current values =");
         for ( i = 0; i < fld_nitems; i++ )
         {
            fprintf(stderr, "%21s[%d]:%c (%#x)\n", " ", i,
               *(((unsigned char*)(fld_value)) + i),
               *(((unsigned char*)(fld_value)) + i));
         }
      
         /* prompt user to enter array element to change */
         fprintf(stderr, "\nWhich array value do you want to change?\n");
         fprintf(stderr, "Enter the 0-based index number (i.e. Enter 0\n");
         fprintf(stderr, "to change the value stored at %s\[0]): ", fld_name);
         if ( (err = scanf("%d", &arr_index)) != 1)
         {
            fprintf(stderr,
               "modify_struct_data: Problem interpreting array index.\n");
            sleep(3);
            return (BFP_FAILURE);
         }

         /* print value to be changed */
         fprintf(stderr, "\nCurrent value of %s[%d]: %c (%#x)\n", fld_name,
            arr_index,
            *(((unsigned char*)(fld_value)) + arr_index),
            *(((unsigned char*)(fld_value)) + arr_index));
      }
   }
   else if ( strcmp(SHORT_TYPE_NAME, fld_type_name) == 0)
   {
      fld_type = SHORT_TYPE;

      if ( fld_nitems == 1 )
      {
         /* byte swap value if necessary */
         if ( ByteSwapNeeded == BFP_TRUE )
         {
            MISC_swap_shorts( 1, (short *)fld_value );
         }

         fprintf(stderr, "%20s %d (%#x)\n", "current value =",
            *(short*)fld_value, *(short*)fld_value);
      }
      else if ( fld_nitems > 1 )
      {
         int i;
         fprintf(stderr, "%20s\n", "current values = ");
         for ( i = 0; i < fld_nitems; i++ )
         {
            /* byte swap value if necessary */
            if ( ByteSwapNeeded == BFP_TRUE )
            {
               MISC_swap_shorts( 1, (short *)(fld_value + i) );
            }

            fprintf(stderr, "%21s[%d]:%c (%#x)\n", " ", i,
               *(((short*)(fld_value)) + i),
               *(((short*)(fld_value)) + i));
         }
      
         /* prompt user to enter array element to change */
         fprintf(stderr, "\nWhich array value do you want to change?\n");
         fprintf(stderr, "Enter the 0-based index number (i.e. Enter 0\n");
         fprintf(stderr, "to change the value stored at %s\[0]): ", fld_name);
         if ( (err = scanf("%d", &arr_index)) != 1)
         {
            fprintf(stderr,
               "modify_struct_data: Problem interpreting array index.\n");
            sleep(3);
            return (BFP_FAILURE);
         }

         /* print value to be changed */
         fprintf(stderr, "\nCurrent value of %s[%d]: %d (%#x)\n", fld_name,
            arr_index,
            *(((short*)(fld_value)) + arr_index),
            *(((short*)(fld_value)) + arr_index));
      }
   }
   else if ( strcmp(USHORT_TYPE_NAME, fld_type_name) == 0)
   {
      fld_type = USHORT_TYPE;

      if ( fld_nitems == 1 )
      {
         /* byte swap value if necessary */
         if ( ByteSwapNeeded == BFP_TRUE )
         {
            MISC_swap_shorts( 1, (short *) fld_value );
         }

         fprintf(stderr, "%20s %u (%#x)\n", "current value =",
            *(unsigned short*)fld_value, *(unsigned short*)fld_value);
      }
      else if ( fld_nitems > 1 )
      {
         int i;
         fprintf(stderr, "%20s\n", "current values =");
         for ( i = 0; i < fld_nitems; i++ )
         {
            /* byte swap value if necessary */
            if ( ByteSwapNeeded == BFP_TRUE )
            {
               MISC_swap_shorts( 1, (short *)(fld_value + i) );
            }

            fprintf(stderr, "%21s[%d]:%u (%#x)\n", " ", i,
               *(((unsigned short*)(fld_value)) + i),
               *(((unsigned short*)(fld_value)) + i));
         }
      
         /* prompt user to enter array element to change */
         fprintf(stderr, "\nWhich array value do you want to change?\n");
         fprintf(stderr, "Enter the 0-based index number (i.e. Enter 0\n");
         fprintf(stderr, "to change the value stored at %s\[0]): ", fld_name);
         if ( (err = scanf("%d", &arr_index)) != 1)
         {
            fprintf(stderr,
               "modify_struct_data: Problem interpreting array index.\n");
            sleep(3);
            return (BFP_FAILURE);
         }

         /* print value to be changed */
         fprintf(stderr, "\nCurrent value of %s[%d]: %u (%#x)\n", fld_name,
            arr_index,
            *(((unsigned short*)(fld_value)) + arr_index),
            *(((unsigned short*)(fld_value)) + arr_index));
      }
   }
   else if ( strcmp(INT_TYPE_NAME, fld_type_name) == 0)
   {
      fld_type = INT_TYPE;

      if ( fld_nitems == 1 )
      {
         /* byte swap value if necessary */
         if ( ByteSwapNeeded == BFP_TRUE )
         {
            MISC_swap_longs( 1, (long *)fld_value );
         }

         fprintf(stderr, "\nCurrent value of %s: %d (%#x)\n", fld_name,
            *(int*)fld_value, *(int*)fld_value);
      }
      else if ( fld_nitems > 1 )
      {
         int i;
         fprintf(stderr, "%20s\n", "current values =");
         for ( i = 0; i < fld_nitems; i++ )
         {
            /* byte swap value if necessary */
            if ( ByteSwapNeeded == BFP_TRUE )
            {
               MISC_swap_longs( 1, (long *)(fld_value + i) );
            }

            fprintf(stderr, "%21s[%d]:%d (%#x)\n", " ", i,
               *(((int*)(fld_value)) + i),
               *(((int*)(fld_value)) + i ));
         }
      
         /* prompt user to enter array element to change */
         fprintf(stderr, "\nWhich array value do you want to change?\n");
         fprintf(stderr, "Enter the 0-based index number (i.e. Enter 0\n");
         fprintf(stderr, "to change the value stored at %s\[0]): ", fld_name);
         if ( (err = scanf("%d", &arr_index)) != 1)
         {
            fprintf(stderr,
               "modify_struct_data: Problem interpreting array index.\n");
            sleep(3);
            return (BFP_FAILURE);
         }

         /* print value to be changed */
         fprintf(stderr, "\nCurrent value of %s[%d]: %d (%#x)\n", fld_name,
            arr_index,
            *(((int*)(fld_value)) + arr_index),
            *(((int*)(fld_value)) + arr_index));
      }
   }
   else if ( strcmp(UINT_TYPE_NAME, fld_type_name) == 0)
   {
      fld_type = UINT_TYPE;

      if ( fld_nitems == 1 )
      {
         /* byte swap value if necessary */
         if ( ByteSwapNeeded == BFP_TRUE )
         {
            MISC_swap_longs( 1, (long *)fld_value );
         }

         fprintf(stderr, "\nCurrent value of %s: %u (%#x)\n", fld_name,
            *(unsigned int*)fld_value, *(unsigned int*)fld_value);
      }
      else if ( fld_nitems > 1 )
      {
         int i;
         fprintf(stderr, "%20s\n", "current values =");
         for ( i = 0; i < fld_nitems; i++ )
         {
            /* byte swap value if necessary */
            if ( ByteSwapNeeded == BFP_TRUE )
            {
               MISC_swap_longs( 1, (long *)(fld_value + i) );
            }

            fprintf(stderr, "%21s[%d]:%u (%#x)\n", " ", i,
               *(((unsigned int*)(fld_value)) + i),
               *(((unsigned int*)(fld_value)) + i));
         }
      
         /* prompt user to enter array element to change */
         fprintf(stderr, "\nWhich array value do you want to change?\n");
         fprintf(stderr, "Enter the 0-based index number (i.e. Enter 0\n");
         fprintf(stderr, "to change the value stored at %s\[0]): ", fld_name);
         if ( (err = scanf("%d", &arr_index)) != 1)
         {
            fprintf(stderr,
               "modify_struct_data: Problem interpreting array index.\n");
            sleep(3);
            return (BFP_FAILURE);
         }

         /* print value to be changed */
         fprintf(stderr, "\nCurrent value of %s[%d]: %u (%#x)\n", fld_name,
            arr_index,
            *(((unsigned int*)(fld_value)) + arr_index),
            *(((unsigned int*)(fld_value)) + arr_index));
      }
   }
   else if ( strcmp(FLOAT_TYPE_NAME, fld_type_name) == 0)
   {
      fld_type = FLOAT_TYPE;

      if ( fld_nitems == 1 )
      {
         /* byte swap value if necessary */
         if ( ByteSwapNeeded == BFP_TRUE )
         {
            MISC_swap_floats( 1, (float *)fld_value );
         }

         fprintf(stderr, "\nCurrent value of %s: %f\n", fld_name,
            *(float*)fld_value);
      }
      else if ( fld_nitems > 1 )
      {
         int i;
         fprintf(stderr, "%20s\n", "current values =");
         for ( i = 0; i < fld_nitems; i++ )
         {
            /* byte swap value if necessary */
            if ( ByteSwapNeeded == BFP_TRUE )
            {
               MISC_swap_floats( 1, (float *)(fld_value + i) );
            }

            fprintf(stderr, "%21s[%d]:%f\n", " ", i,
               *(((float*)(fld_value)) + i));
         }
      
         /* prompt user to enter array element to change */
         fprintf(stderr, "\nWhich array value do you want to change?\n");
         fprintf(stderr, "Enter the 0-based index number (i.e. Enter 0\n");
         fprintf(stderr, "to change the value stored at %s\[0]): ", fld_name);
         if ( (err = scanf("%d", &arr_index)) != 1)
         {
            fprintf(stderr,
               "modify_struct_data: Problem interpreting array index.\n");
            sleep(3);
            return (BFP_FAILURE);
         }

         /* print value to be changed */
         fprintf(stderr, "\nCurrent value of %s[%d]: %f\n", fld_name,
            arr_index,
            *(((float*)(fld_value)) + arr_index));
      }
   }
   else
   {
      fprintf(stderr, "modify_struct_data: data type not found.\n");
      sleep(3);
      return (BFP_FAILURE);
   }

   /* verify that the user wishes to modify it */
   fprintf(stderr, "\nIs this the data you want to replace (y/n)? ");
   if ( (err = scanf("%s", &select)) != 1)
   {
      fprintf(stderr, "modify_struct_data: Problem interpreting selection.\n");
      sleep(3);
      return (BFP_FAILURE);
   }

   if ( select == 'y' )
   {
      switch ( fld_type )
      {
         case CHAR_TYPE:
         { 
            short temp_val = 0;
            char char_val = 0;

            /* read in the new data from the user */
            fprintf(stderr, "\nEnter new value (hexidecimal): ");

            if ( (err = scanf("%hx", &temp_val )) != 1)
            {
               fprintf(stderr,
                  "modify_struct_data: Problem interpreting new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }
            
            /* cast data to the char value */
            char_val = (char)temp_val;

            /* write the new data to the file */
            lseek(fd, offset+fld_offset+arr_index, SEEK_SET);

            /* write new value */
            if ( (err = write(fd, &char_val, sizeof(char))) !=
               sizeof(char))
            {
               fprintf(stderr, "Problem writing new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }

            break;
         }

         case UCHAR_TYPE:
         {
            unsigned short temp_val = 0;
            unsigned char uchar_val = 0;

            /* read in the new data from the user */
            fprintf(stderr, "\nEnter new value (hexidecimal): ");

            if ( (err = scanf("%hx", &temp_val)) != 1)
            {
               fprintf(stderr,
                  "modify_struct_data: Problem interpreting new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }

            /* cast data to uchar val */       
            uchar_val = (unsigned char)temp_val;
    
            /* write the new data to the file */
            lseek(fd, offset+fld_offset+arr_index, SEEK_SET);

            /* write new value */
            if ( (err = write(fd, &uchar_val, sizeof(unsigned char))) !=
               sizeof(unsigned char))
            {
               fprintf(stderr, "Problem writing new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }

            break;
         }

         case SHORT_TYPE:
         {
            short short_val = 0;

            /* read in the new data from the user */
            fprintf(stderr, "\nEnter new value (hexidecimal): ");

            if ( (err = scanf("%hx", &short_val )) != 1)
            {
               fprintf(stderr,
                  "modify_struct_data: Problem interpreting new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }
 
            /* write the new data to the file */
            lseek(fd, offset+fld_offset+(arr_index*sizeof(short)), SEEK_SET);

            /* byte swap value if necessary */
            if ( ByteSwapNeeded == BFP_TRUE )
            {
               MISC_swap_shorts( 1, &short_val );
            }

            /* write new value */
            if ( (err = write(fd, &short_val, sizeof(short))) !=
               sizeof(short))
            {
               fprintf(stderr, "Problem writing new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }

            break;
         }

         case USHORT_TYPE:
         {
            unsigned short ushort_val = 0;

            /* read in the new data from the user */
            fprintf(stderr, "\nEnter new value (hexidecimal): ");

            if ( (err = scanf("%hx", &ushort_val)) != 1)
            {
               fprintf(stderr,
                  "modify_struct_data: Problem interpreting new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }

            /* write the new data to the file */
            lseek(fd, offset+fld_offset+(arr_index*sizeof(unsigned short)),
               SEEK_SET);

            /* byte swap value if necessary */
            if ( ByteSwapNeeded == BFP_TRUE )
            {
               MISC_swap_shorts( 1, &ushort_val );
            }

            /* write new value */
            if ( (err = write(fd, &ushort_val, sizeof(unsigned short))) !=
               sizeof(unsigned short))
            {
               fprintf(stderr, "Problem writing new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }

            break;
         }

         case INT_TYPE:
         {
            int int_val = 0;

            /* read in the new data from the user */
            fprintf(stderr, "\nEnter new value (hexidecimal): ");

            if ( (err = scanf("%x", &int_val)) != 1)
            {
               fprintf(stderr,
                  "modify_struct_data: Problem interpreting new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }

            /* write the new data to the file */
            lseek(fd, offset+fld_offset+(arr_index * sizeof(int)), SEEK_SET);

            /* byte swap value if necessary */
            if ( ByteSwapNeeded == BFP_TRUE )
            {
               MISC_swap_longs( 1, (long *)&int_val );
            }

            if ( (err = write(fd, &int_val, sizeof(int))) !=
               sizeof(int))
            {
               fprintf(stderr, "Problem writing new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }

            break;
         }

         case UINT_TYPE:
         {
            unsigned int uint_val = 0;

            /* read in the new data from the user */
            fprintf(stderr, "\nEnter new value (hexidecimal): ");

            if ( (err = scanf("%x", &uint_val)) != 1)
            {
               fprintf(stderr,
                  "modify_struct_data: Problem interpreting new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }

            /* write the new data to the file */
            lseek(fd, offset+fld_offset+(arr_index*sizeof(unsigned int)),
               SEEK_SET);

            /* byte swap value if necessary */
            if ( ByteSwapNeeded == BFP_TRUE )
            {
               MISC_swap_longs( 1, (long *)&uint_val );
            }

            /* write new value */
            if ( (err = write(fd, &uint_val, sizeof(unsigned int))) !=
               sizeof(unsigned int))
            {
               fprintf(stderr, "Problem writing new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }

            break;
         }

         case FLOAT_TYPE:
         {
            float float_val = 0.0;

            /* read in the new data from the user */
            fprintf(stderr, "\nEnter new value: ");

            if ( (err = scanf("%f", &float_val)) != 1)
            {
               fprintf(stderr,
                  "modify_struct_data: Problem interpreting new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }

            /* write the new data to the file */
            lseek(fd, offset+fld_offset+(arr_index*sizeof(float)),
               SEEK_SET);

            /* byte swap value if necessary */
            if ( ByteSwapNeeded == BFP_TRUE )
            {
               MISC_swap_floats( 1, &float_val );
            }

            /* write new value */
            if ( (err = write(fd, &float_val, sizeof(float))) !=
               sizeof(float))
            {
               fprintf(stderr, "Problem writing new value.\n");
               sleep(3);
               return (BFP_FAILURE);
            }

            break;
         }

         default:
            fprintf(stderr, "modify_struct_data: Invalid data type.\n");
            sleep(3);
            return (BFP_FAILURE);

      }
   }

   /* Free heap memory */
   if ( fld_value != NULL )
   {
      free(fld_value);
   }

   pause_user(); /* Pause til user presses Enter */

   return 0;

} /* end modify_struct_data */


/********************************************************************************

     add_data

********************************************************************************/
int add_data(int fd, int size, int hw_off, int nshorts)
{
   char			select;
   int                  rd_sz        = 0;          /* read size */
   int			err          = 0;
   int			do_over      = BFP_TRUE;
   int			num_idx      = 0;
   unsigned short       *new_values  = NULL;
   unsigned short       *new_data    = NULL;
   unsigned short	orig_data    = 0;

   /* seek to the location specified by the user */
   lseek(fd, hw_off * sizeof(short), SEEK_SET);

   /* if location is beyond EOF, make sure the user knows it */
   if ( (hw_off*sizeof(short)) >= size )
   {
      fprintf(stderr, "You're adding data at the end of the file.\n");
      if ( hw_off*sizeof(short) > size )
      {
         hw_off = floor((double)size/2.0);
         
         /* if there's an odd number of bytes in the file, print a msg telling
            the user what the impact will be */
         if ( size%2 != 0 )
         {
            fprintf(stderr, "Since the file has an odd number of bytes (%d), it ", size);
            fprintf(stderr, "does not end on a halfword boundary.\nTherefore the ");
            fprintf(stderr, "appending process will overwrite the last byte of\n");
            fprintf(stderr, "original data. To compensate for this, make your ");
            fprintf(stderr, "first new byte of data equal to\n");
            fprintf(stderr, "the last byte in the original data.\n");
         }
      }
   }
   else
   {
      if ((rd_sz = read(fd, &orig_data, sizeof(unsigned short))) !=
         sizeof(unsigned short))
      {
         fprintf(stderr, "Problem reading halfword (ret=%d).\n",rd_sz);
         sleep(3);
         return(BFP_FAILURE);
      }

      fprintf(stderr, "You're adding data just before the halfword with this value:\n");

      fprintf(stderr, "0x%2X", orig_data);
      fprintf(stderr, "\n");
   }

   fprintf(stderr, "Is this correct (y/n)? ");
   if ( (err = scanf("%s", &select)) != 1)
   {
      fprintf(stderr, "Problem interpreting selection.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   /* allocate buffer to hold new values */
   new_values = (unsigned short*)calloc(nshorts, sizeof(unsigned short));

   if ( select == 'y' )
   {
      do_over = BFP_TRUE;

      while ( do_over == BFP_TRUE )
      {
         /* prompt user to enter new data */
         if ( nshorts == 1 )
         {
            fprintf(stderr, "Enter %d new value (in hex):\n", nshorts);
         }
         else if ( nshorts > 1 )
         {
            fprintf(stderr, "Enter %d new values (in hex):\n", nshorts);
         }
         
         for ( num_idx = 0; num_idx < nshorts; num_idx++ )
         {
            fprintf(stderr, "Enter number: ");
            if ((err=scanf("%4hx",(unsigned short*)(new_values+num_idx))) != 1)
            {
               fprintf(stderr,
                  "add_data: Problem interpreting new value (err %d).\n", err);
               sleep(3);
               return(BFP_FAILURE);
            }
         }

         /* create new buffer to hold all the data in the file plus the user's
            new data */
         new_data = calloc(size + (nshorts*sizeof(unsigned short)), 1);

         /* read the file's original contents - up to the offset - into the temp bfr */
         lseek(fd,0,SEEK_SET);
         if ((rd_sz = read (fd, (void *)new_data, hw_off * sizeof(short))) !=
            (hw_off * sizeof(short)))
         {
            fprintf (stderr, "read failed (ret %d, errno %d)\n", rd_sz, errno);
            sleep(3);
            return(BFP_FAILURE);
         } 
      
         /* now copy the new values into the buffer */
         memcpy((void *)new_data + (hw_off*sizeof(short)), (void *)new_values,
            (nshorts * sizeof(unsigned short)));

         /* now read the remainder of the file into the buffer */
         if ((rd_sz = read (fd, (void *)new_data + (hw_off*sizeof(short)) +
            (nshorts*sizeof(unsigned short)), size - (hw_off*sizeof(short)))) !=
            (size - (hw_off*sizeof(short)))) 
         {
            fprintf (stderr, "read failed (rd_sz %d, errno %d)\n", rd_sz, errno);
            sleep(3);
            return(BFP_FAILURE);
         } 

         /* write new data to file */
         lseek(fd, 0, SEEK_SET);
         if ( (err = write(fd, (void *)new_data, size+(nshorts * sizeof(unsigned short)))) !=
               size+(nshorts * sizeof(unsigned short)))
         {
            fprintf(stderr, "add_data: Problem writing new values.\n");
            sleep(3);
            return(BFP_FAILURE);
         }

         /* reset do over flag */
         do_over = BFP_FALSE;
      }
   }

   /* free memory */
   free(new_values);
   free(new_data);

   return 0;

} /* end add_data */


/********************************************************************************

     search_char_data

********************************************************************************/
int search_char_data( char* infile_name )
{
   int		err		= 0;
   int		fsize		= 0;
   int		str_length	= 0;
   int		prefix_size	= 0;
   int		postfix_size	= 0;
   int		occur_count	= 0;
   FILE*	fp		= NULL;
   char*	in_buf		= NULL;
   char*	search_str	= NULL;
   char*	search_ptr	= NULL;
   void*	read_bfr	= NULL;



   /* open the file for read only */
   fp = fopen(infile_name, "r");
   if (fp == NULL)
   {
      fprintf(stderr, "search_char_data: error opening input file.\n");
      sleep(3);
      return(-1);
   }

   /* find the file size */
   fsize = file_size(infile_name);
   if ( fsize < 0 )
   {
      fprintf(stderr, "search_char_data: could not get input file size.\n");
      sleep(3);
      return (BFP_FAILURE);
   }

   /* allocate the appropriate amount of buffer space */
   read_bfr = (void *)malloc(fsize);

   /* read the file data into a buffer */
   if ( fread(read_bfr, fsize, 1, fp) != 1 )
   {
      fprintf(stderr, "search_char_data: Problem reading input file.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   /* cast char buffer */
   in_buf = (char*) read_bfr;

   /* allocate space for search string */
   search_str = (char *)malloc(MAX_SEARCH_STRING_BYTES);

   /* prompt the user to enter the string to search for */
   fprintf(stderr, "Enter the search string (%d character maximum): ",
      MAX_SEARCH_STRING_BYTES);
   if ( (err = scanf("%s", search_str)) != 1)
   {
      fprintf(stderr, "Problem interpreting buffer.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   /* determine length of search string */
   str_length = strlen(search_str);

   if ( (search_ptr = strstr( in_buf, search_str )) == NULL )
   {
      fprintf(stderr, "Search string not found.\n");
   }
   else
   {
      /* print important note about byte base and control chars */
      fprintf(stderr, "\n\nNOTE: Byte positions are 0-based and control\n");
      fprintf(stderr, "characters are included in the count.\n\n");
      while ( search_ptr != NULL )
      {
         occur_count++;
         postfix_size = strlen(search_ptr);
         prefix_size = fsize - postfix_size;
         fprintf(stderr,
            "Occurrence %d found at byte position %d.\n",
            occur_count, prefix_size);

         /* increment ptr past this occurrence so it doesn't find it again */
         search_ptr = search_ptr + str_length;
                                                  
         search_ptr = strstr( search_ptr, search_str );
      }
   }

   pause_user();    /* pause til the user presses Enter */

   /* free memory */
   if ( read_bfr != NULL )
   {
      free(read_bfr);
   }
   if ( search_str != NULL )
   {
      free(search_str);
   }
 
   return 0;

} /* end search_char_data */


/********************************************************************************

     search_numeric_data

********************************************************************************/
int search_numeric_data(char* infile_name )
{
   int		i		= 0;
   int		err		= 0;
   int		fsize		= 0;
   int		fd		= 0;
   int		type_select	= 0;
   int		found		= BFP_FALSE;
   unsigned int	search_val	= 0;
   char*	in_buf		= NULL;


   /* open the file for read only */
   fd = open(infile_name, O_RDONLY);
   if (fd < 0)
   {
      fprintf(stderr, "search_numeric_data: error opening input file.\n");
      sleep(3);
      return(-1);
   }

   /* find the file size */
   fsize = file_size(infile_name);
   if ( fsize < 0 )
   {
      fprintf(stderr, "search_numeric_data: could not get input file size.\n");
      sleep(3);
      return (BFP_FAILURE);
   }

   /* allocate the appropriate amount of buffer space */
   in_buf = (char *) malloc (fsize);

   /* read the file data into a buffer */
   if ( read(fd, in_buf, fsize) != fsize )
   {
      fprintf(stderr, "search_char_data: Problem reading input file.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   /* prompt the user to enter the numeric value to search for */
   fprintf(stderr, "Enter the hex value to search for (%d digit maximum): ",
      MAX_SEARCH_HEX_DIGITS);
   if ( (err = scanf("%x", &search_val)) != 1)
   {
      fprintf(stderr, "Problem interpreting buffer.\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   /* determine required data type */
   if ( search_val <= MAX_VAL_UCHAR )
      type_select = CHAR_TYPE;
   else if ( (search_val > MAX_VAL_UCHAR ) &&
            (search_val <= MAX_VAL_USHORT ) )
      type_select = SHORT_TYPE;
   else if ( search_val > MAX_VAL_USHORT )
      type_select = INT_TYPE;
   else
   {
      fprintf(stderr, "search_numeric_data: Could not identify type.\n");
      return (BFP_FAILURE);
   }

   /* print msg about byte base being 0 */
   fprintf(stderr, "\nNOTE: byte values are 0-based.\n\n");

   /* print results delimiter */
   fprintf(stderr, "====================== Results ======================\n");

   /* cycle through the bytes and search for value */
   switch ( type_select )
   {
      case CHAR_TYPE:

         for ( i = 0; i < fsize; i++ )
         {
            if ( (unsigned char)search_val == *(unsigned char*)(in_buf + i) )
            {
               fprintf(stderr, "Found value at byte %d\n", i);
               found = BFP_TRUE;
            }
         }
         break;

      case SHORT_TYPE:

         for ( i = 0; i < (fsize/sizeof(short)); i++ )
         {
            if ( (unsigned short)search_val == 
               *(unsigned short*)(in_buf + (i * sizeof(short))) )
            {
               fprintf(stderr, "Found value starting at byte %d\n",
                  (i*sizeof(short)));
               found = BFP_TRUE;
            }
         }
         break;

      case INT_TYPE:

         for ( i = 0; i < (fsize/sizeof(int)); i++ )
         {
            if ( (unsigned int)search_val == 
               *(unsigned int*)(in_buf + (i * sizeof(int))) )
            {
               fprintf(stderr, "Found value starting at byte %d\n",
                  (i * sizeof(int)));
               found = BFP_TRUE;
            }
         }
         break;

      default:
         fprintf(stderr, "search_numeric_data: invalid data type.\n");
         return(BFP_FAILURE);
         break;
   }

   if ( found == BFP_FALSE )
   {
      fprintf(stderr, "\nSearch value not found.\n");
   }

   fprintf(stderr, "=====================================================\n");

   /* pause til the user presses Enter */
   pause_user();

   /* free memory */
   if ( in_buf != NULL)
   {
      free(in_buf);
   }

   return 0;

} /* end search_numeric_data */


/********************************************************************************

     extract_char_data

********************************************************************************/
int extract_char_data(int in_id, int startbyte, int numbytes, int out_id )
{
   int		err	= 0;
   char*	buf	= NULL;

   /* allocate sufficient memory space */
   buf = (char *)malloc (numbytes + 1);
   if (buf == NULL)
   { 
      fprintf (stderr, "extract_char_data: malloc failed\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   lseek (in_id, startbyte, SEEK_SET);  /* move the file read pointer */

   /* read the file contents into memory */
   if (read (in_id, buf, numbytes) != numbytes) 
   {
      fprintf (stderr, "read failed (errno %d)\n", errno);
      sleep(3);
      return(BFP_FAILURE);
   } 
 
   /* tack on the terminating character */
   strncat(buf, "\0", 1);

   if ( Screen_Output == BFP_TRUE )
   {
      fprintf(stderr, "=============== Extracted Data: ==================\n");

      /* write extracted data to out_id */
      fprintf(stderr, "%s\n", buf);
      fflush(stderr);

      fprintf(stderr, "\n==================================================\n");

      pause_user();  /* pause til user user presses Enter */
   }
   else
   {  /* write data to output file */

      if ( (err = write( out_id, buf, numbytes )) != numbytes)
      {
         fprintf(stderr, "Error writing extracted data to output file.\n");
         sleep (2);
         return (BFP_FAILURE);
      }
   }

   /* free memory */
   if ( buf != NULL )
      free(buf);

   return 0;

} /* end extract_char_data */


/********************************************************************************

     extract_1byte_int_data

********************************************************************************/
int extract_1byte_int_data(int in_id, int startbyte, int numbytes, int out_id )
{
   int		err	= 0;
   char*	buf	= NULL;

   /* allocate sufficient memory space */
   buf = (char *)malloc (numbytes + 1);
   if (buf == NULL)
   { 
      fprintf (stderr, "extract_1byte_int_data: malloc failed\n");
      sleep(3);
      return(BFP_FAILURE);
   }

   lseek (in_id, startbyte, SEEK_SET);  /* move the file read pointer */

   /* read the file contents into memory */
   if (read (in_id, buf, numbytes) != numbytes) 
   {
      fprintf (stderr, "read failed (errno %d)\n", errno);
      sleep(3);
      return(BFP_FAILURE);
   } 

   if ( Screen_Output == BFP_TRUE )
   {
      int  i;

      fprintf(stderr, "=============== Extracted Data: ==================\n");

      /* write extracted data to out_id */
      for ( i = 0; i < numbytes; i++ )
      {
         fprintf(stderr, "byte %d = %3d (%#02x)\n", i, *(unsigned char*)(buf+i),
            *(unsigned char*)(buf+i));
      }

      fprintf(stderr, "\n==================================================\n");

      pause_user();  /* pause til user user presses Enter */

   }
   else
   {  /* write data to output file */

      if ( (err = write( out_id, buf, numbytes )) != numbytes)
      {
         fprintf(stderr, "Error writing extracted data to output file.\n");
         sleep (2);
         return (BFP_FAILURE);
      }
   }

   /* free memory */
   if ( buf != NULL )
      free(buf);

   return 0;

} /* end extract_1byte_int_data */


/********************************************************************************

     get_char - from K&R's "The C Programming Language", 2nd Ed.

********************************************************************************/
int get_char()
{
   char c;
   return (read(0, &c, 1) == 1) ? (unsigned char) c : EOF;
}


/********************************************************************************
* Name:
*   file_size
*
* Description:
*   Return file size of input file
*
* Inputs:
*   char* filename - input file name
*
* Outputs:
*   None
*
* Returns:  
*   On success returns integer equal to number of bytes in file.
*   On failure returns integer equal to -1.
********************************************************************************/
int file_size( char* filename )
{
   int		ret_val	= 0;
   struct stat	stbuf;

   if ( stat( filename, &stbuf ) == -1 )
   {
      fprintf(stderr, "file_size: cannot access %s\n", filename);
      ret_val = -1;
   }
   else
   {
      ret_val = stbuf.st_size;
   }

   return ret_val;

} /* end file_size */


/********************************************************************************
* Name:
*   pause_user
*
* Description:
*   Pauses and doesn't return til the user presses Enter
*
* Inputs:
*
* Outputs:
*
* Returns:  
*   void
********************************************************************************/
void pause_user()
{
   char key_press;

   fprintf(stderr, "\n\nPress Enter to continue");
   do 
   {
      /*
         an infinite loop that does nothing except sleep and wait for the
         user to hit the Enter key
      */

      key_press = get_char();

      if ( (int)key_press == 10 ) 
         break;

      sleep(1);
   } while (1);
   return;
} /* end pause_user */


/********************************************************************************
* Name:
*   print_struct_info
*
* Description:
*   Prints structure info about structure that user enters
*
* Inputs:
*
* Outputs:
*
* Returns:  
*   void
********************************************************************************/
void print_struct_info()
{
   int          err             = 0;
   int          fld_idx         = 0;
   SMI_info_t*	struct_info	= NULL;
   char		struct_name[MAX_STRUCT_NAME_SIZE];

   /* prompt the user to enter the structure that is used in the input file */
   fprintf(stderr, "Enter structure name: ");
   if ( (err = scanf("%s", struct_name)) != 1)
   {
      fprintf(stderr, "print_struct_info: Problem interpreting struct name.\n");
      sleep(3);
      return;
   }
     
    
   /* using smi, need to get and print the structure field names */
   struct_info = BFP_smi_info (struct_name, NULL);

   if ( struct_info != NULL )
   {
      int first_fld = BFP_TRUE;
      fprintf(stderr, "\n%s info:\n", struct_name);
      fprintf(stderr, "%20s %-15d\n", "size (bytes) =", struct_info->size);
      fprintf(stderr, "%20s %-15d\n", "num fields =", struct_info->n_fields);
      fprintf(stderr, "%20s ", "field names =");
      for ( fld_idx = 0; fld_idx < struct_info->n_fields; fld_idx++ )
      {
         if ( first_fld )
         {
            fprintf(stderr, "%-20s\n", ((struct_info->fields)+fld_idx)->name);
            first_fld = BFP_FALSE;
         }
         else
         {
            fprintf(stderr, "%20s %-20s\n", " ",
               ((struct_info->fields)+fld_idx)->name);
         }
      }
   }
   else
   {
      fprintf(stderr, "print_struct_info: structure %s not found.\n",
         struct_name);
      sleep(3);
      return;
   }

   pause_user();

   return;

} /* end print_stuct_info */


/********************************************************************************
* Name:
*    Set_byte_swap_flag
*
* Description:
*    Determines whether or not byte swapping the data is necessary.
*
* Inputs:
*    None
*
* Outputs:
*    None
*
* Returns:  
*    BFP_TRUE    - byte swapping is needed
*    BFP_FALSE   - byte swapping not needed
*    BFP_FAILURE - error occurred
********************************************************************************/
int Set_byte_swap_flag()
{
   int err = 0;
   int format_select = 0;
   int ret_val = BFP_FALSE;

   #ifdef LITTLE_ENDIAN_MACHINE
      printf("\n");
      printf("Specify data format:\n");
      printf(" 1: Little Endian\n");
      printf(" 2: Big Endian\n");
      printf("Enter selection:  ");
      if ( (err = scanf("%d", &format_select)) != 1)
      {
         fprintf(stderr, "Problem interpreting selection.\n");
         sleep(3);
         return(BFP_FAILURE);
      }
      switch (format_select)
      {
         case 1:
            ret_val = BFP_FALSE;
            break;
         case 2:
            /* we need to byte swap field values before displaying them */
            ret_val = BFP_TRUE;
            break;
         default:
            fprintf(stderr, "Incorrect entry.\n");
            sleep(2);
            ret_val = BFP_FAILURE;
            break;
      }
   #endif

   return (ret_val);
} /* end Set_byte_swap_flag */

