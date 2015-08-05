/**************************************************************************

      Module:   le_pipe.c

 Description:   Pipe stdin or some other text file to an LE_log_file

 Assumptions:


**************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/02/12 22:05:27 $
 * $Id: le_pipe.c,v 1.12 2004/02/12 22:05:27 jing Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

/*
 * System Include Files/Local Include Files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*  Local includes */
#include <orpg.h>
#include <orpgerr.h>
#include <orpgmisc.h>
#include <infr.h>
#include <le.h>


/*  1 if the input file is stdin */
int input_is_stdin = 0;


/*
 * Static Globals
 */

/*  Only 200 characters long because of a bug in lelb_mon
	with log messages greater than 200 characters */
#define LOCAL_NAME_SIZE 200

/**  Prototype  */
static int Read_options(int argc, char **argv,

			int* verbose_level, int* error_code, int* error_no, FILE** input_file, int* sleep_time, int* permissions, int* group_id, int* owner_id);

/*  Termination signal handler - reports failure
    @param(in) exit_code - exit code passed by ORPGTASK_reg_hdlr
*/
void on_terminate(int exit_code)
{
        LE_send_msg(GL_INFO, "le_pipe: exit abnormally, exit code %d\n", exit_code);
}

/*  fgetc function that retries I/O if a system call is interrupted
    @param(in) file - file to read
    @returns the read character or EOF if end of file is encountered
*/
int fgetc_retry(FILE* file)
{
	int retry_count = 0;
	int ch;
	errno = 0;
	ch = fgetc(file);
	while ((ch == EOF) && ferror(file) && (errno == EINTR))
	{
	    errno = 0;
	    ch = fgetc(file);
	    /*  This code is to make doubly sure we do not end
		up in an infinite loop.
		le_pipe was hogging the CPU in some rare
		circumstances.
	    */
	    retry_count++;
	    if (retry_count > 30)
	    {
		LE_send_msg(GL_INFO, "le_pipe: quitting retries because retry count is too great\n");
 		break;
	    }
	}
	return(ch);
}

/**
	Main program.
	Reads input file and writes to LE log file in a loop.
        Long input lines are separated into multiple LE messages.
        @param(in) argc - number of arguments
        @param(in) argv - arguments
**/
int main (int argc, char **argv)
{
	int verbose;
	FILE* input;
	int error_code;
	int error_no;
	int ch = 0;
	char data_buf[LOCAL_NAME_SIZE];
	int data_pos = 0;
	int sleep_time = 0;
	int permissions = 0664;
	int group_id=-1;
	int owner_id=-1;

	if (!Read_options(argc, argv, &verbose, &error_code, &error_no, &input, &sleep_time, &permissions, &group_id, &owner_id))
            exit (0);

        /*  Catch termination signals so we can clean up our ping processes */
        if (ORPGTASK_reghdlr(on_terminate) < 0)
                fprintf(stderr, "le_pipe: Could not register for termination signals");

	/*  Loop forever if we are reading from stdin  */
	ch = fgetc_retry(input);
	while (ch != EOF)
	{
	      data_pos = 0;
	      strcpy(data_buf, "");
	      while ((ch != EOF) && (data_pos < LOCAL_NAME_SIZE - 1) && (ch != '\n'))
	      {
		 /*  We can get an EOF character on a read error */
	            data_buf[data_pos] = ch;
	            data_pos++;
		    ch = fgetc_retry(input);
	      }

	      data_buf[data_pos] = '\0';
	      if (data_pos >= 0)
	      {
	      	  if (verbose)
		     fprintf(stderr, "le_pipe: Logging message (%s|%d) %s\n", (error_code == GL_STATUS)?"GL_STATUS":"GL_ERROR", error_no, data_buf);
	      	  LE_send_msg(error_code | error_no, "%s", data_buf);
	      }
	      
	      ch = fgetc_retry(input);

	      if (sleep_time > 0)
	      {
		 if (verbose)
		    fprintf(stderr, "le_pipe: Sleeping for %d milli-seconds\n", sleep_time);
		 msleep(sleep_time);
	      }
	}
	exit (0);

/*END of main()*/
}

/**  Open/Create the specified log file
     @param name(in) - name of the task to log messages for
     @param n_msgs(in) - max no of messages in the log file if we create the log file
     @param avg_msg_size(in) - average message size
     @returns -   1 if successful, 0 otherwise
*/
static int open_le_log_file(const char* name, int instance_no, int n_msgs, int avg_msg_size, int force_create, int verbose, int permissions, int group_id, int owner_id)
{
	/*  Code copied from ORPGMISC_LE_init,
	    Replace this code with ORPGMISC_le_init when we go to the new mrpg implementation
	*/
	char lelb_path[LOCAL_NAME_SIZE];
	LB_attr attr;
	int success = 0;
	int fd;
	int status;

	if ((status = LE_filepath (name, instance_no, lelb_path, LOCAL_NAME_SIZE)) < 0)
	{
	   fprintf(stderr, "le_pipe: could not open log file for %s\n", name);
	   fprintf(stderr, "le_pipe: LE_filepath failed with error %d, the LE_DIR_EVENT variable is probably not defined\n", status);
	}
	else
	{
	    if (n_msgs <= 0)
	       n_msgs = 25;

	    attr.maxn_msgs = n_msgs;
	    attr.msg_size = avg_msg_size;
	    attr.types = 0;
	    attr.remark[0] = '\0';
	    attr.mode = permissions;
	    attr.tag_size = LB_DEFAULT_NRS << NRA_SIZE_SHIFT;

	    if (force_create)
	    {
	       if (verbose)
	          fprintf(stderr, "le_pipe: remove lb %s\n", lelb_path);
	       LB_remove(lelb_path);
	       fd = -1;
	    }
	    else
		fd = LB_open (lelb_path, LB_READ, NULL);
	    if (fd < 0)
	    {	/* we create it */
	    	if (verbose)
	    	   fprintf(stderr, "le_pipe: LB_open failed (%d) when opening existing file %s\n", fd, lelb_path);
		fd = LB_open (lelb_path, LB_CREATE, &(attr));
		if (fd < 0)
		    fprintf(stderr, "le_pipe: could not open or create %s, error = %d\n", lelb_path, fd);
		else
		if (verbose)
		   fprintf(stdout, "le_pipe: created new log file %s\n", lelb_path);
	    }
	    if (fd >= 0)
	    {
	    	/*  Close LB and allow LE_init to open it again */
	        LB_close(fd);

		/*  Change group and/or owner if necessary */
		status = 0;
		if ((group_id >= 0) && (owner_id >= 0))
		{
		  status = chown(lelb_path, owner_id, group_id);
		  if (status < 0)
		    fprintf(stderr, "le_pipe: could not set group id %d and owner id %d for file %s\n", group_id, owner_id, lelb_path);
                }

		if (status >= 0)
		{
	           if (instance_no > 0)
	              LE_instance(instance_no);

 	           status = LE_init(1, (char**)&name);
	           if (status < 0)
		      fprintf(stderr, "le_pipe: could not initialize LE for %s, error = %d\n", name, status) ;
		   else
		      success = 1;
		}
   	    }
	}
	return (success);
}

/**  Open the input file
     @param name(in) - input file name
     @returns - a handle to the input file is successful, NULL otherwise.
*/
static FILE* open_input_file(const char* name)
{
	FILE* ret_file;
	if (strcmp(name, "stdin") == 0)
	{
	   input_is_stdin = 1;
	   return(stdin);
	}
	else
	{
	   ret_file = fopen(name, "r");
	   if (ret_file == NULL)
	      fprintf(stderr, "le_pipe: could not open input file %s\n", name);
	   return(ret_file);
	}
}

/**  Get an integer argument value
	@param arg(in) - input argument
	@param int_value(out) - output integer value
	@param arg_flag(in) - flag text to display on error
	@returns 1 if successful, 0 otherwise
*/
static int get_int_arg(const char* arg, int* int_value, const char* arg_flag)
{
	int retval = 1;
	int temp_value;
	char* end_str;

   	temp_value = strtol(arg, &end_str, 10);
	if (optarg == end_str)
	{
	   retval = 0;
	   fprintf(stdout, "le_pipe: %s must be numeric for flag %s\n", arg, arg_flag);
	}
	else
	   *int_value = temp_value;
	return(retval);
}
/**  Get an octal integer argument value
	@param arg(in) - input argument
	@param int_value(out) - output integer value
	@param arg_flag(in) - flag text to display on error
	@returns 1 if successful, 0 otherwise
*/
static int get_oct_arg(const char* arg, int* int_value, const char* arg_flag)
{
	int retval = 1;
	int temp_value;
	char* end_str;

   	temp_value = strtol(arg, &end_str, 8);
	if (optarg == end_str)
	{
	   retval = 0;
	   fprintf(stdout, "le_pipe: %s must be an octal number for flag %s\n", arg, arg_flag);
	}
	else
	   *int_value = temp_value;
	return(retval);
}

/**  Get a string argument value		*/
static int get_str_arg(const char* arg, char* dest_str)
{
	int str_length;
	str_length = strlen(arg);
	if (str_length > LOCAL_NAME_SIZE - 1)
	   str_length = LOCAL_NAME_SIZE - 1;
	strncpy(dest_str, optarg, str_length);
	dest_str[str_length] = '\0';
	return(1);
}


/**
	Read the command-line options variables.
        @param(in) argc - number of arguments
        @param(in) argv - arguments
        @param(out) verbose_level - verbosity level
	@param(out) error_code - LE error code for every message generated from
				 the input file.
	@param(out) error_no - Error number that is embedded in the LE error code
	@param(out) input_file - Name of the input file.
	@param(out) sleep_time - Amount of time to sleep between reads of the input file.

      Output: none
     Returns: 1 upon success, 0 otherwise
     Globals: Prog_name
       Notes:
 **************************************************************************/
static int Read_options(int argc, char **argv,
			int* verbose_level, int* error_code, int* error_no, FILE** input_file, int* sleep_time, int* permissions, int* group_id, int* owner_id)
{
	int retval = 1;
	int no_of_msgs = 25;
	int avg_msg_size = 0;
	int instance_no = -1;
	char file_name[LOCAL_NAME_SIZE] = "stdin";
	char task_name[LOCAL_NAME_SIZE] = "le_pipe";
	char error_text[LOCAL_NAME_SIZE] = "GL_ERROR";
	int input;
	int is_gl_error;
	int is_gl_status;
	int force_create = 0;
	*verbose_level = 0;
	*error_code = GL_INFO;
	*error_no = 0;
	*input_file = NULL;
	*permissions=0664;

	while ((input = getopt(argc,argv,"a:che:f:g:i:m:n:o:t:s:vw:")) != -1)
	{
      	    switch(input)
	    {
	    	case 'a':  retval = get_int_arg(optarg, &avg_msg_size, "-a");
	    		   break;
		case 'c':  force_create = 1;
			   break;
		case 'e':  get_str_arg(optarg, error_text);
			   is_gl_error = (strcmp(error_text, "GL_ERROR") == 0);
			   is_gl_status = (strcmp(error_text, "GL_STATUS") == 0);
			   retval = (is_gl_error || is_gl_status);
			   if (is_gl_error)
				*error_code = GL_ERROR;
			   else
			   if (is_gl_status)
			   	*error_code = GL_STATUS;
			   else
			         fprintf(stdout, "le_pipe: %s is not valid for the -e flag, must be GL_ERROR or GL_STATUS\n", optarg);
			   break;
	    	case 'f':  retval = get_str_arg(optarg, file_name);
	    		   break;
		case 'i':  retval = get_int_arg(optarg, &instance_no, "-i");
			   break;
		case 'g':  retval = get_int_arg(optarg, group_id, "-g");
			   break;
		case 'm':  retval = get_oct_arg(optarg, permissions, "-m");
			   break;
		case 'n':  retval = get_int_arg(optarg, &no_of_msgs, "-n");
			   break;
		case 'o':  retval = get_int_arg(optarg, error_no, "-o");
			   break;
		case 's':  retval = get_int_arg(optarg, sleep_time, "-s");
			   break;
		case 't':  retval = get_str_arg(MISC_string_basename(optarg), task_name);
			   break;
		case 'v':  *verbose_level = 1;
		           break;
		case 'w':  retval = get_int_arg(optarg, owner_id, "-w");
			   break;
		default:
			retval = 0;
			printf ("\n\tUsage:\t%s [options]\n",argv[0]);
			printf ("\n\tDescription:\n");
			printf ("\n\t\tWrite contents of a text file to a log file\n");
			printf ("\n\t\tUse the -c option to contiuously monitor the input file\n");
			printf ("\n\t\tA new log file will be created if it does not already exist\n");
			printf ("\n\tOptions:\n\n");
			printf ("\t\t-h 	       \t\tprint usage information\n");
			printf ("\t\t-a avg_msg_size   \taverage msg size for a newly created output log file (default: unspecified)\n");
			printf ("\t\t-c 	       \t\tcreate a new log file even if it already exists (default: do not force creation)\n");
			printf ("\t\t-e error_type     \tGL_ERROR or GL_STATUS (default: GL_INFO)\n");
			printf ("\t\t-f file_name      \tinput file name (default: stdin)\n");
			printf ("\t\t-g group_id       \t\tgroup id to use if log file is created\n");
			printf ("\t\t-i instance no    \tinstance number (default: unspecified)\n");
			printf ("\t\t-m mode	       \t\tpermissions if log file is created(in octal)\n");
			printf ("\t\t-n no_of_msgs     \tmax no of msgs for a newly created output log file (default: 25)\n");
			printf ("\t\t-o error_no       \terror number for GL_ERROR log messages\n");
			printf ("\t\t-s sleep_time     \tnumber of milli-seconds to sleep between reads of the input file (default: %d)\n", *sleep_time);
			printf ("\t\t-t task_name      \ttask name to log message for (default: le_pipe)\n");
			printf ("\t\t-v 	       \t\tturn verbosity on\n");
			printf ("\t\t-w owner	       \t\towner id to use if log file is created\n");
			break ;
	    }
	    if (!retval)
	       break;
        }

        if (retval)
        {
	   retval = open_le_log_file(task_name, instance_no, no_of_msgs, avg_msg_size, force_create, *verbose_level, *permissions, *group_id, *owner_id);
	   if (retval)
           {
              *input_file = open_input_file(file_name);
              retval = (input_file != NULL);
           }
       }
       return(retval);
       /*END of Read_options()*/
}
