/************************************************************************
 *									*
 *	Module:  read_bhdr.c
 *									*
 *	Description:  This module contains a collection of functions	*
 *		      to manipulate the internal basedata message for	*
 *		      the Base Data Display Tool.			*
 *									*
 ************************************************************************/
#include <orpg.h>
#define SIZEOF_BASEDATA             (MAX_GENERIC_BASEDATA_SIZE*sizeof(short))

/* Static variables */
static char Data[SIZEOF_BASEDATA]; /* Radial data storage (1 beam) */
static Generic_moment_t *ghdr = NULL;
static Base_data_header *Hdr = NULL;
static int argArray[26];

/* Functions */
void print_help();
int is_linear_buffer(char* filename);
int get_user_selection();
void decipher_input(char* argInput);
void fill_gdh_value_array(char valArrPtr[4][32]);
void print_gdh();
void print_bdh();

/************************************************************************
 *  FUNCTION: main
 *
 *  DESCRIPTION:
 *      The main function.  Performs the logic for reading in the 
 *      command line args and then performing the infinite loop of 
 *      reporting the linear buffer contents.
 *
 *  INPUTS:
 *      @argc - The number of command line arguments
 *      @argv - The command line arguments
 *  RETURNS:
 *      @int - The status of this program.
 *
 ************************************************************************/
int main( int argc, char **argv )
{
    /* *
     * Initial variables.  
     *   @status - The status of the lb reads, and also
     *     temporarily the holder of whether or not the 
     *     user passed a linear buffer in the command line.
     *   @i, @j - Temp variables
     *   @lbChoice - Holds the linear buffer that the user 
     *     enters in when they are prompted.
     *   @lbid - The id of the linear buffer we are opening.
     *   @filename - The filename of the linear buffer that is
     *     passed from the command line (if it is passed)
     *   @myInputString - Placeholder for all of the arguments.
     * */
    int status = 0;
    char data[SIZEOF_BASEDATA];
    int i, j;
    int lbChoice = 0;
    LB_id_t lbid;
    char filename[512];
    char myInputString[64];

    /*  Did they pass us a linear buffer */
    if (argc > 1) 
        status = is_linear_buffer(argv[argc - 1]);

    /* If they did, then we don't need that last argument */
    if (status > 0)
        i = argc - 1;
    else
        i = argc;

    /* *
     * This is to account for the fact that the user could 
     *   enter '-abcdefg' or '-a -b -c -d'...  We also need
     *   to check and see if they enter in a --help, and if so, 
     *   stop right there, print out the help, and exit.
     * */
    for (j = 1; j < i; j++)
    {
        strcat(myInputString, argv[j]);
        if (strstr(argv[j], "--help") != NULL)
        {
            print_help();
            exit(1);
        }
    }
  
    /* *
     * If they didn't enter in a linear buffer into the command
     *   line, then we need to prompt them for one.
     * */
    if (status < 1)
    {
        while ((lbChoice = get_user_selection()) < 0) {
            /* Do nothing */
        }
        /* ORPGDA_lbname converts a data_id number to the 
         * linear buffer file name & location according to the 
         * product attribute table. */
        sprintf(filename, "%s", ORPGDA_lbname(lbChoice));
    }
    else
        sprintf(filename,"%s",argv[argc-1]);
  
    /* See what they entered in */
    if (i > 1)
        decipher_input(myInputString);
    else
        for (j = 0; j < 26; j++)
            argArray[j] = 1;

    printf("FILE NAME: %s\n",filename);

    /*  (void*) is making Data a void pointer, or a pointer that can accept any
     *  data type.  */
    memset((void*)(&Data[0]), 0, SIZEOF_BASEDATA);

    /* Typecast Data as a Base_data_header, which is a char array. */
    Hdr = (Base_data_header *) Data;

    /* Open the linear buffer for reading. */
    lbid = LB_open( filename, LB_READ, NULL );
    printf("LBID: %d\n",lbid);

    while( 1 )
    {
        while( status >= 0 )
        {
            /* Read the LB and print some basic info */
            status = LB_read( lbid, (char *) data, SIZEOF_BASEDATA, LB_NEXT );
            printf("READ ID: %d MSG_ID: %d STATUS: %d\n",lbid,LB_NEXT,status);

            /* If good data read, copy to static storage and reset radial info. */
            if( status > 0 )
                memcpy( &Data[ 0 ], &data[ 0 ], status );

            Hdr = (Base_data_header *) Data;

            /* Print the normal info */
            print_bdh();

            /* Print the offset info */
            if( Hdr->msg_type & DUALPOL_TYPE )
            {
                /*  no_moments tells us how many offsets there are */
                for( i = 0; i < Hdr->no_moments; i++ )
                {
                    ghdr = (Generic_moment_t *) &Data[ Hdr->offsets[ i ] ];
                    print_gdh();
                }
            }
            printf("\n");
        }
        sleep(1);
        status = 0;
    }

    return status;
}

/************************************************************************
 *  FUNCTION: get_user_selection
 *
 *  DESCRIPTION:
 *      This function prompts the user to make a selection.  If the 
 *      user does not put in an input that has been defined, it just
 *      asks them again.  Inputting a 0 means they want to exit.
 *
 *  INPUTS:
 *      Nothing
 *
 *  RETURNS:
 *      @int - Returns the number of the linear buffer that they 
 *             want to read.  0 if they want to exit, -1 if they 
 *             didn't use a predefined linear buffer.
 *
 ************************************************************************/
int get_user_selection()
{
    int userChoice = 0;
    /*  Here, we need to issue a prompt */   
    printf("You did not enter in a valid linear buffer.  Please choose\n");
    printf(" a number from the following list of linear buffers.\n");
    printf(" Enter a '0' to quit:\n");
    printf("   #53  - RECOMBINED_RAWDATA\n");
    printf("   #54  - RAWDATA\n");
    printf("   #55  - BASEDATA\n");
    printf("   #59  - RECOMBINED_BASEDATA\n");
    printf("   #76  - SR_BASEDATA\n");
    printf("   #305 - DUALPOL_BASEDATA\n");
    scanf("%d", &userChoice);
   
    if ((userChoice != 53) && (userChoice != 54) && (userChoice != 55) && (userChoice != 59) && (userChoice != 76) && (userChoice != 305) && (userChoice != 0))
       return -1;    
    if (userChoice == 0)
       exit(1);
    return userChoice;
}   

/************************************************************************
 *  FUNCTION: decipher_input
 *
 *  DESCRIPTION:
 *      This function recognizes what the user input (flags) from the 
 *      command line.  The function assumes a mapping of the typical
 *      alphabet-to-numbers mapping (0=a, 1=b, 2=c...).  For each 
 *      letter input as an argument, this function sets that respective
 *      position in argArray to a 1.
 *
 *  INPUTS:
 *      @argInput - The argument string from the command line.
 *
 *  RETURNS:
 *      Nothing.
 *
 ************************************************************************/
void decipher_input(char* argInput)
{
    char* alpha = "abcdefghijklmnopqrstuvwxyz";
    char* upperAlpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"; 
    int i;

    for (i = 0; i < strlen(alpha); i++)
        if ((strchr(argInput, alpha[i]) != NULL) || (strchr(argInput, upperAlpha[i]) != NULL))
          argArray[i] = 1;
}

/************************************************************************
 *  FUNCTION: is_linear_buffer
 *
 *  DESCRIPTION:
 *      This function tests to see if the argument passed to it is a 
 *      linear buffer.  It simply tries to open the linear buffer, and
 *      based upon the success/failure of that, it returns a -1 (no)
 *      and a 1 (yes).
 *
 *  INPUTS:
 *      @filename - The location of the linear buffer
 *  RETURNS:
 *      @int - 1 if the argument is a linear buffer, else -1.
 *
 ************************************************************************/
int is_linear_buffer(char* filename)
{
    LB_id_t lbid;
    lbid = LB_open( filename, LB_READ, NULL );
    if ((int)lbid < 0)
        return -1;
    LB_close(lbid);
    return 1;
}

/************************************************************************
 *  FUNCTION: print_help
 *
 *  DESCRIPTION:
 *      This function prints out the help if the user issued a 
 *      --help in the command line.
 *
 *  INPUTS:
 *      Nothing
 *
 *  RETURNS:
 *      Nothing
 *
 ************************************************************************/
void print_help()
{
    printf("%s","Usage:\n"); 
    printf("%s", "-a    azimuth\n");
    printf("%s", "-b    begin_vol_time\n");
    printf("%s", "-c    volume_scan_num\n");
    printf("%s", "-d    date\n");
    printf("%s", "-e    elevation\n");
    printf("%s", "-f    start_angle\n");
    printf("%s", "-g    n_dop_bins\n");
    printf("%s", "-h    n_surv_bins\n");
    printf("%s", "-i    dop_bin_size\n");
    printf("%s", "-j    no_moments\n");
    printf("%s", "-k    time\n");
    printf("%s", "-l    msg_len\n");
    printf("%s", "-m    msg_type\n");
    printf("%s", "-n    nyquist_vel\n");
    printf("%s", "-o    generic moment no_of_gates\n");
    printf("%s", "-p    version\n");
    printf("%s", "-q    weather_mode\n");
    printf("%s", "-r    rpg_elev_ind\n");
    printf("%s", "-s    status\n");
    printf("%s", "-t    generic moment name\n");
    printf("%s", "-u    generic moment no_of_gates\n");
    printf("%s", "-v    generic moment bin_size\n");
    printf("%s", "-w    generic moment control_flag\n");
    printf("%s", "-x    generic moment scale\n");
    printf("%s", "-y    generic moment offset\n");
    printf("%s", "-z    generic moment data_word_size\n");
}

/************************************************************************
 *  FUNCTION: fill_value_array
 *
 *  DESCRIPTION:
 *      This function copies all of the Base_data_header data into 
 *      the char pointer passed into the function.
 *
 *  INPUTS:
 *      @valArrPtr - A pointer to a two dimensional character array.
 *
 *  RETURNS:
 *      Nothing
 *
 ************************************************************************/
void fill_value_array(char valArrPtr[19][32])
{
    sprintf(valArrPtr[0],  "azimuth=%5.4f", Hdr->azimuth);
    sprintf(valArrPtr[1],  "begin_vol_time=%d", Hdr->begin_vol_time);
    sprintf(valArrPtr[2],  "volume_scan_num=%d", Hdr->volume_scan_num);
    sprintf(valArrPtr[3],  "date=%d", Hdr->date);
    sprintf(valArrPtr[4],  "elevation=%5.2f", Hdr->elevation);
    sprintf(valArrPtr[5],  "start_angle=%d", Hdr->start_angle);
    sprintf(valArrPtr[6],  "n_dop_bins=%d", Hdr->n_dop_bins);
    sprintf(valArrPtr[7],  "n_surv_bins=%d", Hdr->n_surv_bins);
    sprintf(valArrPtr[8],  "dop_bin_size=%d", Hdr->dop_bin_size);
    sprintf(valArrPtr[9],  "no_moments=%d", Hdr->no_moments);
    sprintf(valArrPtr[10], "time=%d", Hdr->time);
    sprintf(valArrPtr[11], "msg_len=%d", Hdr->msg_len);
    sprintf(valArrPtr[12], "msg_type=%d", Hdr->msg_type);
    sprintf(valArrPtr[13], "nyquist_vel=%d", Hdr->nyquist_vel);
    sprintf(valArrPtr[14], "no_moments=%d", Hdr->no_moments);
    sprintf(valArrPtr[15], "version=%d", Hdr->version);
    sprintf(valArrPtr[16], "weather_mode=%d", Hdr->weather_mode);
    sprintf(valArrPtr[17], "rpg_elev_ind=%d", Hdr->rpg_elev_ind);
    sprintf(valArrPtr[18], "status=%d", Hdr->status);
}

/************************************************************************
 *  FUNCTION: fill_gdh_value_array
 *
 *  DESCRIPTION:
 *      This function copies all of the Generic_moment_t data into 
 *      the char pointer passed into the function.
 *
 *  INPUTS:
 *      @valArrPtr - A pointer to a two dimensional character array.
 *
 *  RETURNS:
 *      Nothing
 *
 ************************************************************************/
void fill_gdh_value_array(char valArrPtr[7][32])
{
    char moment_name[ 5 ];
    memcpy( &moment_name[ 0 ], &(ghdr->name[0]), 4 );
    moment_name[ 4 ] = '\0';
    sprintf(valArrPtr[0], "moment_name=%s", moment_name);
    sprintf(valArrPtr[1], "no_of_gates=%d", ghdr->no_of_gates);
    sprintf(valArrPtr[2], "bin_size=%d", ghdr->bin_size);
    sprintf(valArrPtr[3], "control_flag=%d", (unsigned int)ghdr->control_flag);
    sprintf(valArrPtr[4], "scale=%5.2f", ghdr->scale);
    sprintf(valArrPtr[5], "offset=%3.4f", ghdr->offset);
    sprintf(valArrPtr[6], "data_word_size=%d", (unsigned int)ghdr->data_word_size);
}

/************************************************************************
 *  FUNCTION: print_bdh
 *
 *  DESCRIPTION:
 *      This function prints out the data for the Base_data_header 
 *      structure.  It simply creates all of the Base_data_header data, 
 *      then prints it out according to what the user requested 
 *      according to the command line arguments.
 *
 *  INPUTS:
 *      None
 *
 *  RETURNS:
 *      None
 *
 ************************************************************************/
void print_bdh()
{
    /* *
     * We could probably get away with not using the 
     *   tmpArr, but it makes the code much easier and
     *   simplifies the logic, so we will take the 32 byte 
     *   hit.
     * */
    char valArray[19][32] = {""};
    char tmpArr[32] = "";
    int currStringLen = 0;
    int i;

    /* *
     * We get all of the data and pick what we want versus
     *   trying to only retrieve the requested data.  We 
     *   take a small hit on memory, but it's worth it 
     *   due to the simplified logic.
     * */
    fill_value_array(valArray);

    /* *
     * For each possible piece of requested info (through 'status')
     * */
    for (i = 0; i < 19; i++)
    {
        /* Zero out our temporary array */
        memset( tmpArr, '\0', sizeof(tmpArr)); 

        /*  If the user requested it... */
        if (argArray[i] == 1)
        {
            /* Copy it to the tmp array */
            strcpy(tmpArr, valArray[i]);

            /* Just basic formatting. Helps things to line up. */
            if (( 2 - (strlen(tmpArr) % 2)) == 1)
              strcat(tmpArr, " ");
            strcat(tmpArr, " | ");

            /* * 
             * If this line has exceeded the 80 character limit, 
             *   then we need to start a new line.  
             * */
            if ((currStringLen + strlen(tmpArr)) > 80)
            {
                printf("\n");
                currStringLen = 0;
            }
            printf("%s", tmpArr);

            /* *
             * Keep track of how many things we've printed on
             *  this line.
             * */
            currStringLen+=strlen(tmpArr);
        }
    }

    /* This needs to be here because this function gets called 
     * no matter what, and if we don't check to make sure we 
     * have actually printed data, then we are just printing 
     * another newline, and that makes the data look funny.*/
    if (currStringLen > 0)
        printf("\n");
}


/************************************************************************
 *  FUNCTION: print_gdh
 *
 *  DESCRIPTION:
 *      This function prints out the data for the Generic_moment_t 
 *      structure.  It simply creates all of the Generic_moment_t data, 
 *      then prints it out according to what the user requested 
 *      according to the command line arguments.
 *
 *  INPUTS:
 *      None
 *
 *  RETURNS:
 *      None
 *
 ************************************************************************/
void print_gdh()
{
    /* *
     * We could probably get away with not using the 
     *   tmpArr, but it makes the code much easier and
     *   simplifies the logic, so we will take the 32 byte 
     *   hit.
     * */
    char valArray[7][32] = {""};
    char tmpArr[32] = "";
    int currStringLen = 0;
    int i;

    /* *
     * We get all of the data and pick what we want versus
     *   trying to only retrieve the requested data.  We 
     *   take a small hit on memory, but it's worth it 
     *   due to the simplified logic.
     * */
    fill_gdh_value_array(valArray);

    /* *
     * For each possible piece of requested info (as far as offset info goes)
     * */
    for (i = 19; i < 26; i++)
    {
        /* Zero out our temporary array */
        memset( tmpArr, '\0', sizeof(tmpArr)); 

        /* If the user requested it... */
        if (argArray[i] == 1)
        {
            /* Copy it to the tmp array */
            strcpy(tmpArr, valArray[i - 19]);

            /* Just basic formatting. Helps things to line up. */
            if (( 2 - (strlen(tmpArr) % 2)) == 1)
              strcat(tmpArr, " ");
            strcat(tmpArr, " | ");

            /* * 
             * If this line has exceeded the 80 character limit, 
             *   then we need to start a new line.  
             * */
            if ((currStringLen + strlen(tmpArr)) > 80)
            {
                printf("\n");
                currStringLen = 0;
            }
            printf("%s", tmpArr);

            /* *
             * Keep track of how many things we've printed on
             *  this line.
             * */
            currStringLen+=strlen(tmpArr);
        }
    }

    printf("\n");
}

