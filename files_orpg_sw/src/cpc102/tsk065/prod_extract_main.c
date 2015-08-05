/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 19:49:48 $
 * $Id: prod_extract_main.c,v 1.10 2005/12/27 19:49:48 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
*/


/**
	Module:

		Monitor the product database and write a specified list
		of products into data files when they are generated.


**/

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include <infr.h>
#include <orpg.h>
#include <prod_gen_msg.h>

/*  Maximum length of fixed length strings within this program  */
#define MAX_STR_LENGTH 255


/**  Prototype for reading the command line options
	refer to the implementation below for detailed comments  **/
static int Read_options(int argc, char **argv);

/**  Prototype for product selection routine  **/
int select_product(int prod_code);

/**  Prototype for routine that parses the product selection configuration file  */
int read_prod_code_file(const char* file_name);

void report_selected_products();

/**  Function that processes message changes in the product database  **/
void on_lb_change(int lb_fd, LB_id_t msg_id, int msg_len, void* user_data);

static char* output_dir = NULL;
static int orpg_format = 0;
static int size_of_index = 500;
static char* index_name = NULL;
static char* prod_select_file = NULL;


int main(int argc, char* argv[])
{
	int status;
	int product_db_lb_fd;
	const char* db_name;

	/**  Read command line options   **/
	if (!Read_options(argc, argv))
            exit (0);

        if (prod_select_file != NULL)
        {
            if (read_prod_code_file(prod_select_file) == 0)
               exit(1);
        }

        if ((status = ORPGMISC_init (argc, argv, 500, 0, -1, 0)) < 0)
	{
            LE_send_msg (GL_ERROR, "ORPGMISC_init failed: %d\n", status);
            exit (GL_EXIT_FAILURE);
        }

        report_selected_products();

	/*  Force ORPGPAT to register for its events
		This was necessary because ORPGPAT hangs if
		it is called during lb notification processing
		without this call.
	 */
	ORPGPAT_get_code(1);

	/*  Force ORPGVST to register it events - not sure if this is
	    necessary */
	ORPGVST_get_elevation(1);

	db_name = ORPGCFG_dataid_to_path(ORPGDAT_PRODUCTS, NULL);
	if ((db_name != NULL) && (strlen(db_name) >  0))
	{

	    status = product_db_lb_fd = LB_open(db_name, LB_READ, NULL);
	    if (status >= 0)
	    {

		/*  Register for notification services */
		status = LB_UN_register(product_db_lb_fd, LB_ANY, on_lb_change);

		if (status == LB_SUCCESS)
		{
		    /**  Wait forever  */
		    while (1)
		    	msleep(10000);
		}
		else
		   fprintf(stderr, "Error %d when registering for notification on %s\n",
			status, db_name);

	    }
	    else
		fprintf(stderr, "Error %d when opening %s\n",
			status, db_name);
	}
	else
	   fprintf(stderr, "Could not obtain file name for data id %d\n", ORPGDAT_PRODUCTS);
	return(0);
}

/*  Wrap a string into severl LE msgs  */
void LE_send_wrapped_string(int code, char* str, int wrap_count)
{
	int total_length = strlen(str);
	int total_count = 0;
	char* curr_str = str;
	char saved_ch = 0;
	int str_too_long;

	while (total_count < total_length)
	{
	   str_too_long = (strlen(curr_str) > wrap_count);
	   if (str_too_long)
	   {
		saved_ch = curr_str[wrap_count];
		curr_str[wrap_count] = '\0';
	   }
	   LE_send_msg(code, "%s", curr_str);
	   total_count += strlen(curr_str);
	   if (str_too_long)
	   {
	      curr_str[wrap_count] = saved_ch;
	      curr_str += wrap_count;
	   }
	   else
	      curr_str += strlen(str);
	}
}

/**  fwrite function that retries if the I/O operation is
	interuppted  */
size_t fwrite_retry(const void *ptr, size_t size,  size_t  nitems, FILE *stream)
{
	size_t ret;
	int count = 0;
	errno=0;
	do
	{
	   ret = fwrite(ptr, size, nitems, stream);
	   count++;
	} while ((ret == 0) && (errno = EINTR) && (count < 30));
	return(ret);
}

/*#  Open/Create Index LB  */
int get_index_lb()
{
	static int lb_fd = -1;

	if (lb_fd < 0)
	{
	   char* lb_name = NULL;

	   lb_name = STR_copy(lb_name, output_dir);
	   if (lb_name[strlen(lb_name) - 1] != '/')
	      lb_name = STR_cat(lb_name, "/");
	   lb_name = STR_cat(lb_name, index_name);

	   lb_fd = LB_open(lb_name, LB_READ | LB_WRITE, NULL);
	   if (lb_fd < 0)
	   {
		LB_attr attr;
		strcpy(attr.remark, "Index LB");

	    	attr.mode = 0666;
		attr.msg_size = 0;
		attr.maxn_msgs = size_of_index;
		attr.types = LB_FILE;
		attr.tag_size = (30 << NRA_SIZE_SHIFT);

		lb_fd = LB_open(lb_name, LB_READ | LB_WRITE | LB_CREATE, &attr);
		if (lb_fd < 0)
		{
			LE_send_msg(GL_ERROR, "Error %d creating index lb %s", lb_fd, lb_name);
		}
	   }

	}
	return(lb_fd);
}

typedef struct
{
	/*#  ORPG ICD product code  */
	int prod_code;

	/*#   @desc WDSSII XML type string  */
	char wdss_type[MAX_STR_LENGTH];

} wdss_type_rec_t;

/*  Reflecitivy products are not in this list because
    they are the default  */
static wdss_type_rec_t wdss_types[] =
{
   { 22, "Velocity" },         /* OKAY  */
   { 23, "Velocity" },
   { 24, "Velocity" },
   { 25, "Velocity" },
   { 26, "Velocity" },
   { 27, "Velocity" },
   { 28, "SpectrumWidth" },
   { 29, "SpectrumWidth" },
   { 30, "SpectrumWidth" },
/*   { 31, "USP" },	*/
/*   { 32, "DHR" },	*/
/*   { 33, "HSR" },	*/
/*   { 34, "CFC" },	*/
   { 35, "CR" },            	/*  displayed, does not look okay */
   { 36, "CR" },		/*  displayed, does not look okay */
   { 37, "CR"  },		/*  displayed, does not look okay */
   { 38, "CR" },		/*  displayed, does not look okay */
   { 39, "CRC" },		/*  Not supported  */
   { 40, "CRC" },		/*  Not supported  */
   { 41, "ET" },		/*  Displayed - okay ?  */
   { 42, "ET" },               	/*  ETC - No Formatter for ETC */
   { 43, "Reflectivity" },	/*  SWR - No Formatter for SWR */
   { 44, "Velocity" },		/*  SWV - No Formatter */
   { 45, "SpectrumWidth" },	/*  SWW - No Formatter */
   { 46, "Reflectivity" },	/*  SWS - No Formatter */
   { 47, "SWP" },		/*  SWP - No Formatter */
   { 48, "VWP" },		/*  VWP - No Formatter */
   { 49, "ET" },		/*  CM  - No Formatter */
   { 50, "ET" },      		/*  CS  - No Formatter */
   { 51, "ET" },		/*  VCS - No Formatter */
   { 52, "ET" },		/*  SCS - No Formatter */
   { 53, "WER" },		/*  WER - No Formatter?  */
   { 55, "Reflectivity" },      /*  SRR - NO Formatter */
   { 56, "SRM" }, 		/*  Displays - does not look okay  */
   { 57, "VIL" },               /*  Displays  */
   { 58, "STI_TABLE" },         /*  No Formatter  */
   { 59, "HI_TABLE" },		/*  No Formatter  */
   { 60, "MesoTable" },		/*  No Formatter  */
   { 61, "TVS_TABLE" },		/*  No Formatter  */
   { 62, "SS" },                /*  No Formatter  */
   { 63, "ET" },		/*  LRA1, No Formatter  */
   { 64, "ET" },		/*  LRA2, No Formatter  */
   { 65, "LRM1" },		/*  Displayed, does not look correct */
   { 66, "LRM2" },		/*  Displayed, does not look correct  */
   { 67, "APR" },               /*  Displayed, does not look correct  */
   { 73, "UAM" },               /*  Not generated  */
   { 74, "RCM" },               /*  No Formatter  */
   { 75, "FTM" },		/*  Not generated  */
   { 77, "PUP_FTM" },		/*  Not generated  */
   { 78, "OHP" },               /*  Did not test  */
   { 79, "THP" },		/*  Did not test  */
   { 80, "STP" },		/*  Did not test  */
   { 81, "DPA" },               /*  DPA - Received invalid ProductMgmtTabPage ptr */
   { 82, "SPD" },		/*  Not generated  */
   { 83, "IRM" },		/*  No formatter  */
   { 84, "VAD" },               /*  No formatter  */
   { 85, "ET" },		/*  RCS No formatter */
   { 86, "ET" },                /*  VCS No formatter */
   { 87, "ET" },                /*  CS No Formatter */
   { 88, "CSC" },            	/*  No Formatter */
   { 89, "ET" },		/*  No Formatter  */
   { 90, "LRM3" },		/*  Displayed, does not look correct  */
   { 93, "Velocity" },		/*  Did not work  */
   { 95, "CR" },                /*  Displayed, does not look correct  */
   { 96, "CR" },		/*  Displayed, does not look correct  */
   { 97, "CR" },		/*  Displayed, does not look correct  */
   { 98, "CR" },		/*  Displayed, does not look correct  */
   { 133, "Velocity" },		/*  CLR, did not work  */
   { 134, "VIL" }                /*  Displayed  */
};

int no_of_wdss_types = sizeof(wdss_types)/ sizeof(wdss_type_rec_t);


char* get_type_string(Prod_header* product)
{
	int prod_code = ORPGPAT_get_code(product->g.prod_id);
	int found = 0;
	int i;
	for (i = 0; (i < no_of_wdss_types); i++)
	{
	   found = (wdss_types[i].prod_code == prod_code);
	   if (found)
               break;
	}

	if (found)
	    return(wdss_types[i].wdss_type);
	else
	    return ("Reflectivity");
}

/*#  Generate an xml string   */
static int generate_xml_string(Prod_header* product, const char* file_name)
{
	static char* xml_string = NULL;
        int rda_elev_indx;
	char text[100];
	struct tm* time;
	char* desc = NULL;
	int i;
	int status;
	int lb_fd;
	float elevation;
	xml_string = STR_copy(xml_string, "<item>\n  <time fractional=\"0.000000\"> ");
	sprintf(text, "%d", (int) product->elev_t);
	xml_string = STR_cat(xml_string, text);
	xml_string = STR_cat(xml_string, " </time>\n  <params>NIDS FlatFile ");
	xml_string = STR_cat(xml_string, output_dir);
	xml_string = STR_cat(xml_string, " ");
	xml_string = STR_cat(xml_string, get_type_string(product));
	xml_string = STR_cat(xml_string, " ");
	xml_string = STR_cat(xml_string, file_name);
	xml_string = STR_cat(xml_string, " </params>\n  <selections> ");
	time = gmtime((const time_t*)&product->elev_t);
	sprintf(text, "%4.4d%2.2d%2.2d-%2.2d%2.2d%2.2d ",
		   time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
		   time->tm_hour, time->tm_min, time->tm_sec);
	xml_string = STR_cat(xml_string, text);

	desc = STR_copy(desc, ORPGPAT_get_description(product->g.prod_id, STRIP_MNEMONIC));
	for (i=0; (i < strlen(desc)); i++)
	{
	    if (desc[i] == ' ')
		desc[i] = '_';
	}
	xml_string = STR_cat(xml_string, desc);
	STR_free(desc);
        rda_elev_indx = ORPGVST_get_rda_index(product->g.elev_ind);
	elevation = ((float)ORPGVST_get_elevation(rda_elev_indx)) / 10;
	if (ORPGPAT_get_type(product->g.prod_id) == 0)
	   elevation = 0.0;
	sprintf(text, " %2.2f </selections>\n</item>\n", elevation);
	xml_string = STR_cat(xml_string, text);

	lb_fd = get_index_lb();
	if (lb_fd >= 0)
	{
		LE_send_msg(GL_INFO, "Writing xml string");
		LE_send_wrapped_string(GL_INFO, xml_string, 150);
		status = LB_write(lb_fd, xml_string, strlen(xml_string) + 1, LB_ANY);
		if (status < 0)
		   LE_send_msg(GL_INFO, "Error (%d) writing to index lb", status);
	}
	return(lb_fd);
}


/*  Generate the output file   */
static int generate_output_file(const char* file_name, void* product, int product_len)
{
	static char* complete_file_name = NULL;
	int len;
	FILE* file;
	int ret = 0;
	static char* mkdir_command = NULL;

	if (mkdir_command == NULL)
	{
	   mkdir_command = STR_copy(mkdir_command, "mkdir -p ");
	   mkdir_command = STR_cat(mkdir_command, output_dir);
	   system(mkdir_command);
	}

	complete_file_name = STR_copy(complete_file_name, output_dir);
	len = strlen(complete_file_name);
	if ((len > 0) && (complete_file_name[len-1] != '/'))
	   complete_file_name = STR_cat(complete_file_name, "/");
	complete_file_name = STR_cat(complete_file_name, file_name);
	LE_send_msg(GL_INFO, "Open %s", complete_file_name);
	file = fopen(complete_file_name, "w");
	if (file != NULL)
	{
	   LE_send_msg(GL_INFO, "Write product, length = %d", product_len);
	   errno = 0;
	   ret = fwrite_retry(product, product_len, 1, file);
	   if (ret == 0)
	   {
	      LE_send_msg(GL_ERROR, "Error %d writing file:", errno);
	      LE_send_wrapped_string(GL_ERROR, complete_file_name, 150);
	   }
	   fclose(file);
	   chmod(complete_file_name, 0666);
	}
	else
	{
	   LE_send_msg(GL_ERROR, "Error %d opening file:", errno);
	   LE_send_wrapped_string(GL_ERROR, complete_file_name, 150);
	}

	return(ret != 0);
}


static int prod_count = 0;

/**
	LB_notification callback - updates the status of each client
	@param lb_fd(in) - FD that event occcurred on
	@param msg_id(in) - message id that has changed
	@param msg_len(in) - message length
	@param user_data - user data, should be NULL

 */
void on_lb_change(int lb_fd, LB_id_t msg_id, int msg_len, void* user_data)
{

	int retval;
	Prod_header* product;
	static char* file_name = NULL;
	char text[100];
	struct tm* time;
	float elevation;
	int i;
	int rda_elev_indx;
	prod_count++;
        retval = LB_read(lb_fd, (void*)&product, LB_ALLOC_BUF, msg_id);
        if (retval >= 0)
	{
		LE_send_msg(GL_INFO, "Generated product id %d, msg id = %d\n", product->g.prod_id, msg_id);
		LE_send_msg(GL_INFO, "Product code = %d\n", ORPGPAT_get_code(product->g.prod_id));
		LE_send_msg(GL_INFO, "Prod desc = %s\n", ORPGPAT_get_description(product->g.prod_id, STRIP_MNEMONIC));
		LE_send_msg(GL_INFO, "Prod count = %d\n", prod_count);

                if (!select_product(ORPGPAT_get_code(product->g.prod_id)))
                {
                   LE_send_msg(GL_INFO, "Product code %d not selected for extraction", ORPGPAT_get_code(product->g.prod_id));
                   return;
                }
                else
                   LE_send_msg(GL_INFO, "Product code %d is selected for extraction", ORPGPAT_get_code(product->g.prod_id));

		file_name = STR_copy(file_name, ORPGPAT_get_description(product->g.prod_id, STRIP_MNEMONIC));
		for (i = 0; (i < strlen(file_name)); i++)
		{
		    if ((isspace((int)file_name[i])) || (file_name[i] == ':')
			|| (file_name[i] == '/'))
			file_name[i] = '_';
		    else if (file_name[i] == '.')
			file_name[i] = '.';
		    else if (!isalnum((int)file_name[i]))
		    {
		     	file_name[i] = '\0';
		        break;
		    }
		}

                rda_elev_indx = ORPGVST_get_rda_index(product->g.elev_ind);
		elevation = ((float)ORPGVST_get_elevation(rda_elev_indx)) / 10.0;
		LE_send_msg(GL_INFO, "Elev index = %d, rda elev indx = %d, elevation = %f", rda_elev_indx, product->g.elev_ind, elevation); 
		if (ORPGPAT_get_type(product->g.prod_id) == 0)
		    elevation = 0.0;
		sprintf(text, "_%2.1f", elevation);
		file_name = STR_cat(file_name, text);

		time = gmtime((const time_t*)&product->elev_t);
		sprintf(text, "_%4.4d%2.2d%2.2d_%2.2d%2.2d%2.2d",
		   time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
		   time->tm_hour, time->tm_min, time->tm_sec);

		file_name = STR_cat(file_name, text);

		LE_send_msg(GL_INFO, "File_name=%s\n", file_name);
		LE_send_msg(GL_INFO, "Size of msg=%d, size of product=%d\n", retval, product->g.len);

		if (product->g.len > 0)
		{
		   LE_send_msg(GL_INFO, "Writing %s format\n", orpg_format?"ORPG":"ICD");
		   if (orpg_format)
		      generate_output_file(file_name, ((char*)product), product->g.len);
		   else
		      generate_output_file(file_name, ((char*)product) + sizeof(Prod_header), product->g.len - sizeof(Prod_header));

		   if (index_name != NULL)
		      generate_xml_string(product, file_name);
		}
		else
		   LE_send_msg(GL_INFO, "Skip writing file %s because size is %d\n", file_name, product->g.len);

		free(product);
	}
	else
	   LE_send_msg(GL_INFO, "Read error %d on msg id %d\n", retval, msg_id);
}



/**  Read the command line options
     @param(in) argc - number of arguments
     @param(in) argv - arguments
**/
static int Read_options(int argc, char **argv)
{
	int retval = 1;
	char* endstr;
	int input;
	opterr = 0;
	while ((input = getopt(argc,argv,"hoO:w:p:s:")) != -1)
	{
      	     switch(input)
	     {

		   case 'o': orpg_format=1;
			break;

		   case 'O':  output_dir = STR_copy(output_dir, optarg);
			      break;

		   case 'p':  prod_select_file = STR_copy(prod_select_file, optarg);
			      break;

		   case 's':  size_of_index = strtol(optarg, &endstr, 10);
                              if (endstr == optarg)
				  size_of_index = 500;
			      break;

		   case 'w':  index_name = STR_copy(index_name, optarg);
			      break;

         	   default:
			retval = 0;
			printf ("\n\tUsage:\t%s [options] -O output_dir\n",argv[0]);
			printf ("\n\tDescription:\n");
			printf ("\n\t\tWrite ORPG selected ORPG products to files in\n");
			printf ("\n\t\the specified output directory as they are generated.\n");
			printf ("\n\tOptions:\n\n");
			printf ("\t\t-h 	      \t\tPrint usage information\n");
			printf ("\t\t-o 	      \t\tGenerate products in orpg format\n");
			printf ("\t\t-O 	      \t\tOutput directory\n");
			printf ("\t\t-p select_file   \tSpecify a product selection file\n");
			printf ("\t\t\t\t\tthat allows extraction of specific products\n");
			printf ("\t\t-s index_size    \tSpecify the number of products\n");
			printf ("\t\t	 	      \tmaintained by the index\n");
			printf ("\t\t-w index_name    \tGenerate a wdssII index file\n");
			printf ("\t\t\t\t\tindex_name is the name of the wdssII index file\n");
			return(retval);
			break ;
	      }
        }

	if (output_dir == NULL)
	{
	   printf("Error, -O option is required\n");
	   exit(1);
	}

	return(retval);
}

#define MAX_PRODS 500
static int selected_prods[MAX_PRODS];
static int no_of_prods = 0;

void report_selected_products()
{
    if (no_of_prods == 0)
        LE_send_msg(GL_INFO, "All products selected");
    else
    {
        int i;
        char num_buf[50];
        char* output_str=NULL;
        output_str = STR_copy(output_str, "");
        for (i = 0; i < no_of_prods; i++)
        {
            if (i % 8 == 0)
               output_str = STR_cat(output_str, "Selected prods: ");
            sprintf(num_buf, "%d", selected_prods[i]);
            output_str = STR_cat(output_str, num_buf);
            if (i % 8 == 7)
            {
               LE_send_msg(GL_INFO, output_str);
               output_str = STR_copy(output_str, "");
            }
            else
            {
               if (i < no_of_prods - 1)
                  output_str = STR_cat(output_str, ",");
            }
        }
        if (no_of_prods % 8 != 0)
           LE_send_msg(GL_INFO, output_str);

    }
}

/**  @desc Determine if a product code should be extracted
     @param(in) prod_code - product code
     @returns - 1 if the product is selected, 0 otherwise
*/
int select_product(int prod_code)
{
    int i;
    int ret = 0;
    if (no_of_prods == 0)
       return(1);

    for (i=0; (i < no_of_prods); i++)
    {

         if (selected_prods[i] == prod_code)
         {
             ret = 1;
             break;
         }
    }
    return(ret);
}

/**  @desc Read a configuration file that contains a list of product codes to be extracted
           # is a valid comment on the first line
     @param(in) file_name - Name of the product selection file
     @returns - 1 if successful.  0 otherwise.
**/
int read_prod_code_file(const char* file_name)
{
	char line[10000];
	char* ch;
	int line_no = 1;
	int ret = 1;
        char* end_char;
        int prod_code;
        FILE* file;

	file = fopen(file_name, "r");
	if (file != NULL)
	{
	     while (fgets(line, sizeof(line), file) != NULL)
	     {
	        if (no_of_prods > 500)
                {
                    printf("Warning: Maximum number(500) of product selections exceeded\n");
                    break;
                }

		ch = &line[0];

		if ((*ch == '#') || (*ch == '\n'))
		   continue;

		while (*ch != '\0')
		{
		   while ((*ch != '\0') && (!isdigit((int)(*ch))))
		      ch++;

                   prod_code = strtol(ch, &end_char, 10);
                   if (end_char > ch)
                   {
                       selected_prods[no_of_prods] = prod_code;
                       no_of_prods++;
                       ch = end_char;
                   }

		}
		line_no++;
	     }
	     fclose(file);
	}
	else
	{
		ret = 0;
		printf("Error %d opening %s\n", errno, file_name);
	}
	return(ret);
}




