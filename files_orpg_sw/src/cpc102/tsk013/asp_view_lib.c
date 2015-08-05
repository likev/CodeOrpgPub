#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "asp_view_lib.h"
#include "asp_view.h"
#include "support.h"

static int ignore_msg_types[14] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1};
static time_t range_start_filter_time = -1;
static time_t range_end_filter_time = -1;
static struct tm match_filter_time;
static int filter_window_open = 0;
static char *search_string;
static char *dir_location;
static char *init_file;


static char* string_trim(char *str);
static char* substring(const char* str, size_t begin, size_t len);
static int does_contain_case_insensitive_substring(char *a, char *b);
static int get_message_type_index(const char* message_type);
static char* map_message_type_to_color(const char* message_type);
static time_t get_oldest_packet_date(GtkWidget *widget);
static time_t get_newest_packet_date(GtkWidget *widget);
static void get_content_of_first_row_of_tree_model(GtkWidget *widget, char *tree_view_name, int col, char **returnVal);
static void get_content_of_last_row_of_tree_model(GtkWidget *widget, char *tree_view_name, int col, char **returnVal);
static void get_all_rows_of_tree_model_column(GtkWidget *widget, char *tree_view_name, int col, GList **returnVal);
/******************************************************************************
 *  Description:
 *      Sets the global variable search_string.
 *  Input:
 *      String to set.
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void set_search_string(char *var)
{
    search_string = g_strndup(var, 63);   
}

/******************************************************************************
 *  Description:
 *      Returns the global variable search_string
 *  Input:
 *      Nothing
 *  Returns:
 *      The global variable search_string.
 *  Notes:
 *****************************************************************************/
char *get_search_string()
{
    return search_string;
}

/******************************************************************************
 *  Description:
 *      Sets the global variable dir_location
 *  Input:
 *      A string to set
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void set_dir_location(char *var)
{
    dir_location = g_strdup(var);
    if (!g_str_has_suffix(get_dir_location(), "/\0"))
        dir_location = g_strconcat(dir_location, "/\0", NULL);
}

/******************************************************************************
 *  Description:
 *      Sets the global variable init_file
 *  Input:
 *      The string to copy
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void set_init_file(char *var)
{
    init_file = g_strndup(var, strlen(var));
}

/******************************************************************************
 *  Description:
 *      Returns the global variable dir_location
 *  Input:
 *      Nothing
 *  Returns:
 *      The global variable dir_location
 *  Notes:
 *****************************************************************************/
char * get_dir_location()
{
    return dir_location;
}

/******************************************************************************
 *  Description:
 *      Returns the global variable init_file.
 *  Input:
 *      Nothing
 *  Returns:
 *      Returns the global variable init_file
 *  Notes:
 *****************************************************************************/
char * get_init_file()
{
    return init_file;
}

/******************************************************************************
 *  Description:
 *      Trims whitespace
 *  Input:
 *      A string
 *  Returns:
 *      A string with leading and trailing whitespace
 *  Notes:
 *****************************************************************************/
static char* string_trim(char *str)
{
    return g_strchomp(g_strchug(str));
}

/******************************************************************************
 *  Description:
 *      Returns a substring
 *  Input:
 *      str - The string to look at
 *      begin - The place to start the substring
 *      len - The length of the substring
 *  Returns:
 *      A substring
 *  Notes:
 *****************************************************************************/
static char* substring(const char* str, size_t begin, size_t len)
{
    if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len))
        return 0;
    return g_strndup(str + begin, len);
}

/******************************************************************************
 *  Description:
 *      Performs a case insensitive comparison to see if b exists in a
 *  Input:
 *      a - The string to contain the substring
 *      b - The substring to look for.
 *  Returns:
 *      Returns 0 if a substring does not exist, 1 if it does.
 *  Notes:
 *****************************************************************************/
static int does_contain_case_insensitive_substring(char *a, char *b)
{
    char *first = g_ascii_strup(a, strlen(a));
    char *second = g_ascii_strup(b, strlen(b));
    char *result = strstr(first, second);
    g_free(first);
    g_free(second);
    if (result == NULL)
      return 0;
    return 1;
}

/******************************************************************************
 *  Description:
 *      Sets the values for the global variable ignore_msg_types to all 1's.
 *  Input:
 *      Nothing
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void reset_ignore_msg_types()
{
    ignore_msg_types[0] = 1;
    ignore_msg_types[1] = 1;
    ignore_msg_types[2] = 1;
    ignore_msg_types[3] = 1;
    ignore_msg_types[4] = 1;
    ignore_msg_types[5] = 1;
    ignore_msg_types[6] = 1;
    ignore_msg_types[7] = 1;
    ignore_msg_types[8] = 1;
    ignore_msg_types[9] = 1;
    ignore_msg_types[10] = 1;
    ignore_msg_types[11] = 1;
    ignore_msg_types[12] = 1;
    ignore_msg_types[13] = 1;
}

/******************************************************************************
 *  Description:
 *      Gets the index of the ignore_msg_types according to the message type.
 *  Input:
 *      A string which holds the message type
 *  Returns:
 *      Returns the index
 *  Notes:
 *****************************************************************************/
static int get_message_type_index(const char* message_type)
{
    if (strcmp(message_type, "RPG INFO") == 0)
        return 0;
    if (strcmp(message_type, "RPG GEN STATUS") == 0)
        return 1;
    if (strcmp(message_type, "RPG WARNING") == 0)
        return 2;
    if (strcmp(message_type, "NB COMMS") == 0)
        return 3;
    if (strcmp(message_type, "RPG MAM ALARM") == 0)
        return 4;
    if (strcmp(message_type, "RPG MAR ALARM") == 0)
        return 5;
    if (strcmp(message_type, "RPG LS ALARM") == 0)
        return 6;
    if (strcmp(message_type, "RPG ALARM CLEARED") == 0)
        return 7;
    if (strcmp(message_type, "RDA SECONDARY ALARM") == 0)
        return 8;
    if (strcmp(message_type, "RDA MAR ALARM") == 0)
        return 9;
    if (strcmp(message_type, "RDA MAM ALARM") == 0)
        return 10;
    if (strcmp(message_type, "RDA INOP ALARM") == 0)
        return 11;
    if (strcmp(message_type, "RDA NA ALARM") == 0)
        return 12;
    if (strcmp(message_type, "RDA ALARM CLEARED") == 0)
        return 13;
    return 0;
}

/******************************************************************************
 *  Description:
 *      Maps the message type to a color
 *  Input:
 *      The message type to map
 *  Returns:
 *      The color that this message maps to
 *  Notes:
 *****************************************************************************/
static char* map_message_type_to_color(const char* message_type)
{
        if (strcmp(message_type, "RPG INFO") == 0)
          return "gray";
        if (strcmp(message_type, "RPG GEN STATUS") == 0)
          return "tan";
        if (strcmp(message_type, "RPG WARNING") == 0)
          return "yellow";
        if (strcmp(message_type, "NB COMMS") == 0)
          return "green3";
        if (strcmp(message_type, "RPG MAM ALARM") == 0)
          return "orange";
        if (strcmp(message_type, "RPG MAR ALARM") == 0)
          return "yellow";
        if (strcmp(message_type, "RPG LS ALARM") == 0)
          return "deep sky blue";
        if (strcmp(message_type, "RPG ALARM CLEARED") == 0)
          return "light green";
        if (strcmp(message_type, "RDA SECONDARY ALARM") == 0)
          return "white";
        if (strcmp(message_type, "RDA MAR ALARM") == 0)
          return "yellow";
        if (strcmp(message_type, "RDA MAM ALARM") == 0)
          return "orange";
        if (strcmp(message_type, "RDA INOP ALARM") == 0)
          return "red1";
        if (strcmp(message_type, "RDA NA ALARM") == 0)
          return "red1";
        if (strcmp(message_type, "RDA ALARM CLEARED") == 0)
          return "light green";
        return "tan";
}

/******************************************************************************
 *  Description:
 *      This sets the value of a given index according to the status of 
 *      a toggle button.  
 *  Input:
 *      togglebutton - The toggle button that is to be set
 *      index - The index of ignore_msg_types to change
 *  Returns:
 *      Nothing
 *  Notes:
 *      A status of 1 is checked (the value is shown), and 0 is unchecked.
 *****************************************************************************/
void set_state_of_ignore_msg_types(GtkToggleButton *togglebutton, int index)
{
    if (gtk_toggle_button_get_active(togglebutton))
      ignore_msg_types[index] = 1;
    else
      ignore_msg_types[index] = 0;
}

/******************************************************************************
 *  Description:
 *      Sets the global variable filter_window_open to 1. 
 *  Input:
 *      Nothing
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void set_filter_window_open()
{
    filter_window_open = 1;
}

/******************************************************************************
 *  Description:
 *      Sets the global variable filter_window_open to -1
 *  Input:
 *      Nothing
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void set_filter_window_closed()
{
    filter_window_open = 0;
}

/******************************************************************************
 *  Description:
 *      Returns the value of filter_window_open
 *  Input:
 *      Nothing
 *  Returns:
 *      Returns the value of filter_window_open
 *  Notes:
 *****************************************************************************/
int get_filter_window_status()
{
    return filter_window_open;
}

/******************************************************************************
 *  Description:
 *      Sets the global variable range_start_filter_time
 *  Input:
 *      A time_t value
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void set_range_start_filter_time(time_t f_time)
{
    range_start_filter_time = f_time;
}

/******************************************************************************
 *  Description:
 *      Sets the global variable range_end_filter_time
 *  Input:
 *      A time_t value
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void set_range_end_filter_time(time_t f_time)
{
    range_end_filter_time = f_time;
}

/******************************************************************************
 *  Description:
 *      This function sets the values in the global variable match_filter_time.
 *  Input:
 *      A valid, registered widget
 *      option - An integer that decides whether we are just resetting the 
 *                  values or actually setting them.
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void set_match_filter_time(GtkWidget *widget, int option)
{
    match_filter_time.tm_year = -1;
    match_filter_time.tm_mon = -1;
    match_filter_time.tm_mday = -1;
    match_filter_time.tm_hour = -1;
    match_filter_time.tm_min = -1;
    match_filter_time.tm_sec = -1;

    /* We are setting with the values from the spinbuttons */
    if (option > 0)
    {
        match_filter_time.tm_year = get_spin_button_value(widget, "select_dates_match_year_spinbutton");
        match_filter_time.tm_mon = get_spin_button_value(widget, "select_dates_match_month_spinbutton") - 1;
        match_filter_time.tm_mday = get_spin_button_value(widget, "select_dates_match_day_spinbutton");
        match_filter_time.tm_hour = get_spin_button_value(widget, "select_dates_match_hour_spinbutton");
        match_filter_time.tm_min = get_spin_button_value(widget, "select_dates_match_minute_spinbutton");
        match_filter_time.tm_sec = get_spin_button_value(widget, "select_dates_match_second_spinbutton");

        /* There is no 0 day */
        if (match_filter_time.tm_mday == 0)
            match_filter_time.tm_mday = 1;
    }
    else
    {
        match_filter_time.tm_year = -1;
        match_filter_time.tm_mon = -1;
        match_filter_time.tm_mday = -1;
        match_filter_time.tm_hour = -1;
        match_filter_time.tm_min = -1;
        match_filter_time.tm_sec = -1;
    }
}

/******************************************************************************
 *  Description:
 *      Returns the global variable range_start_filter_time
 *  Input:
 *      Nothing
 *  Returns:
 *      Returns the global variable range_start_filter_time
 *  Notes:
 *****************************************************************************/
int get_range_start_filter_time()
{
    return range_start_filter_time;
}

/******************************************************************************
 *  Description:
 *      Returns the global variable range_end_filter_time
 *  Input:
 *      Nothing
 *  Returns:
 *      Returns the global variable range_end_filter_time
 *  Notes:
 *****************************************************************************/
int get_range_end_filter_time()
{
    return range_end_filter_time;
}

/******************************************************************************
 *  Description:
 *      Returns the global variable get_match_filter_time
 *  Input:
 *      Nothing
 *  Returns:
 *      Returns the global variable get_match_filter_time
 *  Notes:
 *****************************************************************************/
struct tm get_match_filter_time()
{
    return match_filter_time;
}

/******************************************************************************
 *  Description:
 *      This method is used for sorting.  It is a case insensitive comparison
 *      between the two parameters.
 *  Input:
 *      2 strings
 *  Returns:
 *      0 if they are equal, not 0 if they are not equal.
 *  Notes:
 *****************************************************************************/
int sort_alphabetically(const void *a, const void * b)
{
    return g_ascii_strcasecmp((char *)a, (char *)b);
}

/******************************************************************************
 *  Description:
 *      This function calls the gtk_main_quit for a given thread.
 *  Input:
 *      A Widget and user data
 *  Returns:
 *      0
 *  Notes:
 *****************************************************************************/
gint destroy(GtkWidget *widget, gpointer data)
{   
    gtk_main_quit();
    return 0;
}

/******************************************************************************
 *  Description:
 *      This function looks at a file name, and based upon its values, it 
 *      creates a time_t out of those values.
 *  Input:
 *      A string holding the file name
 *  Returns:
 *      A time_t structure
 *  Notes:
 *****************************************************************************/
time_t pull_date_from_file_name(char *fileName)
{
    /* Expected format is: WXYZ.YYYY_MM_DD_HH_mm.ASP */
    if (strlen(fileName) != 25)
        return 0;
    gchar **tokens = g_strsplit_set(fileName, ".", -1);
    /* Rip apart the date stuff */
    gchar **dates = g_strsplit_set(tokens[1], "_", -1);
    int year = atoi(dates[0]);
    int mon = atoi(dates[1]);
    int day = atoi(dates[2]);
    int hour = atoi(dates[3]);
    int min = atoi(dates[4]);
    time_t myTime = entries_to_time_t(year, mon - 1, day, hour, min, 0);
    g_strfreev(dates);
    g_strfreev(tokens);
    return myTime;
}

/******************************************************************************
 *  Description:
 *      This function returns an integer representing the month based upon 
 *      the string that you pass to it (0 - January, 1 - February, ...)
 *  Input:
 *      A string that holds the month
 *  Returns:
 *      An integer that represents the index
 *  Notes:
 *****************************************************************************/
int convert_month_string_to_int(char *month)
{
    if (g_ascii_strcasecmp("Jan", month) == 0)
        return 0;
    if (g_ascii_strcasecmp("Feb", month) == 0)
        return 1;
    if (g_ascii_strcasecmp("Mar", month) == 0)
        return 2;
    if (g_ascii_strcasecmp("Apr", month) == 0)
        return 3;
    if (g_ascii_strcasecmp("May", month) == 0)
        return 4;
    if (g_ascii_strcasecmp("Jun", month) == 0)
        return 5;
    if (g_ascii_strcasecmp("Jul", month) == 0)
        return 6;
    if (g_ascii_strcasecmp("Aug", month) == 0)
        return 7;
    if (g_ascii_strcasecmp("Sep", month) == 0)
        return 8;
    if (g_ascii_strcasecmp("Oct", month) == 0)
        return 9;
    if (g_ascii_strcasecmp("Nov", month) == 0)
        return 10;
    if (g_ascii_strcasecmp("Dec", month) == 0)
        return 11;
    return 0;
}

/******************************************************************************
 *  Description:
 *      This function converts a string based date and time into a time_t
 *      structure.
 *  Input:
 *      A string holding the date 
 *      A string holding the time
 *  Returns:
 *      A time_t structure representing the time and date
 *  Notes:
 *****************************************************************************/
time_t convert_string_date_to_time_t(char *date, char *time)
{
    gchar **splitDate = g_strsplit_set(g_strchug(g_strchomp(date)), " ,", 0);
    gchar **splitTime = g_strsplit(g_strchug(g_strchomp(time)), ":", 0);
    int dates[6];
    dates[0] = atoi(splitDate[2]) + 2000;
    dates[1] = convert_month_string_to_int(splitDate[0]);
    dates[2] = atoi(splitDate[1]);
    dates[3] = atoi(splitTime[0]);
    if (dates[3] == 0)
      dates[3] = 23;
    dates[4] = atoi(splitTime[1]);
    dates[5] = atoi(splitTime[2]);
    time_t retTime = entries_to_time_t(dates[0], dates[1], dates[2], dates[3], dates[4], dates[5]);
    g_strfreev(splitDate);
    g_strfreev(splitTime);
    return retTime;
}

/******************************************************************************
 *  Description:
 *      This function takes in 6 integer values and converts them into a 
 *      time_t structure
 *  Input:
 *      Integers representing the year, month, day, hour, minute and second
 *  Returns:
 *      A time_t structure representing the date
 *  Notes:
 *      Originally, the logic to double check the values wasn't in this, but
 *          it makes sense to add it.  If data is corrupted, we can "semi" fix it
 *          here by correcting it.  More time complexity, but it makes sure all 
 *          data is within allowable ranges.
 *****************************************************************************/
time_t entries_to_time_t (int year, int mon, int day, int hour, int min, int sec)
{ 
    struct tm newTM;
    /* The year is supposed to be: (current year - 1900), so if they 
     * didn't pass us something greater than 1900, then we are fine. */
    if (year > 1900)
        newTM.tm_year = year - 1900;
    else if (year > 100)
        newTM.tm_year = year;
    /* If it is less than 100, then we just got the ones or tens digit */
    else
        newTM.tm_year = year + 100;
    newTM.tm_mon = mon;
    newTM.tm_mday = day;
    newTM.tm_hour = hour;
    newTM.tm_min = min;
    newTM.tm_sec = sec;
    newTM.tm_isdst = -1;

    /* Logic checks to make sure our values are in bounds */
    if (newTM.tm_hour > 23)
      newTM.tm_hour = 23;
    if (newTM.tm_min > 59)
      newTM.tm_min = 59;
    if (newTM.tm_sec > 59)
      newTM.tm_sec= 59;
    if (((newTM.tm_mon == 0) || (newTM.tm_mon == 2) || (newTM.tm_mon == 4) || 
          (newTM.tm_mon == 6) || (newTM.tm_mon == 7) || (newTM.tm_mon == 9) || 
          (newTM.tm_mon == 11)) && (newTM.tm_mday > 31))
      newTM.tm_mday = 31;
    else if ((newTM.tm_mon == 1) && ((newTM.tm_year % 4) == 0) && (newTM.tm_mday > 29))
      newTM.tm_mday = 29;
    else if ((newTM.tm_mon == 1) && (newTM.tm_mday > 28))
      newTM.tm_mday = 28;
    else if (newTM.tm_mday > 30)
      newTM.tm_mday = 30;


    if (newTM.tm_year < 0)  
        newTM.tm_year = 0;
    if (newTM.tm_mon < 0)
        newTM.tm_mon = 0;
    if (newTM.tm_mday < 0) 
        newTM.tm_mday = 0;
    if (newTM.tm_hour < 0)  
        newTM.tm_hour = 0;
    if (newTM.tm_min < 0) 
        newTM.tm_min= 0;
    if (newTM.tm_sec < 0)  
        newTM.tm_sec = 0;

    time_t start_time = mktime(&newTM);
    return start_time;
}

/******************************************************************************
 *  Description:
 *      Returns the number of rows in a given tree view.
 *  Input:
 *      A registered widget
 *      The name of the tree view
 *  Returns:
 *      An integer representing the number of rows in the tree view.
 *  Notes:
 *****************************************************************************/
int get_num_rows_in_tree_view(GtkWidget *widget, char *tree_view_name)
{
    GtkTreeView* tree_view = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(widget), tree_view_name));
    GtkTreeModel *tree_filter = gtk_tree_view_get_model(tree_view);
    return gtk_tree_model_iter_n_children(tree_filter, NULL);
}

/******************************************************************************
 *  Description:
 *      Checks to see if a directory exists.
 *  Input:
 *      A string representing the path to the directory.
 *  Returns:
 *      0 if the directory does not exist.
 *      1 if the directory exists.
 *  Notes:
 *****************************************************************************/
int does_directory_exist(char *path)
{
    DIR* dir;
    dir = opendir(path);
    if (dir == NULL)
        return 0;
    closedir(dir);
    return 1;
}

/******************************************************************************
 *  Description:
 *      This function checks to see if this directory has any files in it 
 *      with an asp/ASP file extenstion.
 *  Input:
 *      A string representing the directory path
 *  Returns:
 *      1 if the directory does have ASP files
 *      0 if the directory does not have ASP files
 *  Notes:
 *****************************************************************************/
int does_dir_contain_asp_files(char * myDir)
{
    DIR* dir;
    struct dirent* entry;
    struct stat buf;
    dir = opendir(myDir);
    if (dir == NULL)
    {
        closedir(dir);
        return 0;
    }
    while ((entry = readdir(dir)) != 0)
    {
        lstat(entry->d_name, &buf);
        if (does_contain_case_insensitive_substring(entry->d_name, ".ASP"))
        {
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

/******************************************************************************
 *  Description:
 *      This function checks to see if a file exists.
 *  Input:
 *      The path to the file
 *  Returns:
 *      1 if the file exists.
 *      0 if the file does not exist.
 *  Notes:
 *****************************************************************************/
int does_file_exist(char *filename)
{
    FILE *file;
    if ((file = fopen(filename, "r")))
    {
        fclose(file);
        return 1;
    }
    fclose(file);
    return 0;
}

/******************************************************************************
 *  Description:
 *      This function finds all of the ASP files in a directory and 
 *      sets them to the GList that is passed to it.
 *  Input:
 *      myDir - the directory to look in
 *      dataArray - A pointer to the GList that is to hold the file names
 *      start_time - The time in which all files must be older.
 *  Output:
 *      dataArray will be a GList of filenames
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void get_asp_files_in_dir_after_date(char *myDir, GList **dataArray, time_t start_time)
{
    GError* err;
    GDir* dir;
    GDir* year_dir;
    GDir* month_dir;

    gchar* year_entry = NULL; 
    gchar* month_entry = NULL;
    time_t file_time;

    dir = g_dir_open(myDir, 0, &err);
    if (dir == NULL)
    {
        fprintf(stderr, "Error opening %s: %s\n", myDir, err->message);
        g_error_free(err);
        return;
    }
    
    while (1)
    {
        const gchar* year = g_dir_read_name(dir);
        if (year == NULL)
            break;

        year_entry = g_build_filename(myDir, year, NULL);
        if (g_file_test(year_entry, G_FILE_TEST_IS_DIR) == TRUE)
        {
            year_dir = g_dir_open(year_entry, 0, &err);

            if (year_dir == NULL)
            {
                fprintf(stderr, "Error opening %s%s: %s\n", myDir, year, err->message);
                g_error_free(err);
                g_dir_close(dir);
                g_free(year_entry);
                return;
            }
            while(1)
            {
                const gchar* month = g_dir_read_name(year_dir);

                if (month == NULL)
                  break;

                month_entry = g_build_filename(myDir, year, month, NULL);

                if (does_dir_contain_asp_files(month_entry))
                {
                    month_dir = g_dir_open(month_entry, 0, &err);
                    if (month_dir == NULL)
                    {
                        fprintf(stderr, "Error opening %s: %s\n", month_entry, err->message);
                        g_error_free(err);
                        g_free(month_entry);
                        break;
                    }

                    while (1)
                    {
                        const gchar* file = g_dir_read_name(month_dir);
                        if (file == NULL)
                          break;
                        if (does_contain_case_insensitive_substring((gchar *)file, ".ASP"))
                        {
                            file_time = pull_date_from_file_name((gchar *)file);
                            if (difftime(start_time, file_time) > 0)
                                *dataArray = g_list_insert_sorted(*dataArray, g_strdup(file), sort_alphabetically);
                        }
                    }
                    g_dir_close(month_dir);
                } 
                g_free(month_entry);
            } 
            g_free(year_entry);
            g_dir_close(year_dir);
        } 
    } 
    g_dir_close(dir);
    return;
}

/******************************************************************************
 *  Description:
 *      This function gets all files in a directory that are within a certain
 *      time range.  
 *  Input:
 *      myDir - The directory to look in 
 *      dataArray - A Glist to copy the files to 
 *      start_time - The most recent time acceptable
 *      end_time - The oldest time acceptable
 *  Output:
 *      dataArray will be a GList of file names.
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void get_asp_files_within_time_span(char *myDir, GList **dataArray, time_t start_time, time_t end_time)
{
    GError* err;
    GDir* dir;
    GDir* year_dir;
    GDir* month_dir;

    gchar* year_entry = NULL; 
    gchar* month_entry = NULL;
    time_t file_time;

    dir = g_dir_open(myDir, 0, &err);
    if (dir == NULL)
    {
        fprintf(stderr, "Error opening %s: %s\n", myDir, err->message);
        g_error_free(err);
        return;
    }
    
    while (1)
    {
        const gchar* year = g_dir_read_name(dir);
        if (year == NULL)
            break;

        year_entry = g_build_filename(myDir, year, NULL);
        if (g_file_test(year_entry, G_FILE_TEST_IS_DIR) == TRUE)
        {
            year_dir = g_dir_open(year_entry, 0, &err);

            if (year_dir == NULL)
            {
                fprintf(stderr, "Error opening %s%s: %s\n", myDir, year, err->message);
                g_error_free(err);
                g_dir_close(dir);
                g_free(year_entry);
                return;
            }
            while(1)
            {
                const gchar* month = g_dir_read_name(year_dir);

                if (month == NULL)
                  break;

                month_entry = g_build_filename(myDir, year, month, NULL);

                if (does_dir_contain_asp_files(month_entry))
                {
                    month_dir = g_dir_open(month_entry, 0, &err);
                    if (month_dir == NULL)
                    {
                        fprintf(stderr, "Error opening %s: %s\n", month_entry, err->message);
                        g_error_free(err);
                        g_free(month_entry);
                        break;
                    }

                    while (1)
                    {
                        const gchar* file = g_dir_read_name(month_dir);
                        if (file == NULL)
                          break;
                        if (does_contain_case_insensitive_substring((gchar *)file, ".ASP"))
                        {
                            file_time = pull_date_from_file_name((gchar *)file);
                            if ((difftime(start_time, file_time) > 0) && (difftime(file_time, end_time) > 0))
                                *dataArray = g_list_insert_sorted(*dataArray, g_strdup(file), sort_alphabetically);
                        }
                    }
                    g_dir_close(month_dir);
                } 
                g_free(month_entry);
            } 
            g_free(year_entry);
            g_dir_close(year_dir);
        } 
    } 
    g_dir_close(dir);
    return;
}

/******************************************************************************
 *  Description:
 *      This function gets all ASP files beneath this directory
 *      in ASP.
 *  Input:
 *      radarName - the name of the radar
 *      dirList - A pointer to a GList, which will hold the filenames.
 *  Output:
 *      dirList will be a GList of file names
 *  Returns:
 *      0 on success, 1 on failure 
 *  Notes:
 *****************************************************************************/
int get_asp_subdirs(char *radarName, GList **dirList)
{
    GError* err;
    GDir* dir;
    GDir* year_dir;

    gchar* myDir = g_build_filename(get_dir_location(), radarName, NULL);
    gchar* year_entry = NULL; 
    gchar* month_entry = NULL;

    /* /import/orpg/ASP_Prod/ICAO */
    dir = g_dir_open(myDir, 0, &err);
    if (dir == NULL)
    {
        fprintf(stderr, "Error opening %s: %s\n", myDir, err->message);
        g_free(myDir);
        g_error_free(err);
        return 1;
    }

    
    while (1)
    {
        /* YYYY */
        const gchar* year = g_dir_read_name(dir);
        if (year == NULL)
            break;

        year_entry = g_build_filename(myDir, year, NULL);
        /* We are in the years */
        if (g_file_test(year_entry, G_FILE_TEST_IS_DIR) == TRUE)
        {
            year_dir = g_dir_open(year_entry, 0, &err);

            if (year_dir == NULL)
            {
                fprintf(stderr, "Error opening %s%s: %s\n", myDir, year, err->message);
                g_error_free(err);
                g_dir_close(dir);
                g_free(myDir);
                g_free(year_entry);
                return 1;
            }
            while(1)
            {
                const gchar* month = g_dir_read_name(year_dir);
                if (month == NULL)
                  break;

                month_entry = g_build_filename(myDir, year, month, NULL);

                if (does_dir_contain_asp_files(month_entry))
                    get_asp_files_in_dir(month_entry, dirList);
                g_free(month_entry);
            } /* end while */
            g_free(year_entry);
            g_dir_close(year_dir);
        } /* end if */
    } /* end while */
    g_dir_close(dir);
    g_free(myDir);
    return 0;
} /* end get_asp_subdirs */

/******************************************************************************
 *  Description:
 *      This function gets all files in a directory where the files end
 *      in ASP.
 *  Input:
 *      myDir - the directory to look in
 *      dataArray - A pointer to a GList, which will hold the filenames.
 *  Output:
 *      dataArray will be a GList of file names
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
int get_asp_files_in_dir(char *myDir, GList **dataArray)
{
    DIR* dir;
    struct dirent* entry;
    struct stat buf;
    char *myPath;
    if (g_strrstr(myDir, "/") == NULL)
        myPath = g_build_filename(get_dir_location(), myDir, NULL);
    else
        myPath = g_strdup(myDir);
    dir = opendir(myPath);
    if (dir == NULL)
    {
        closedir(dir);
        free(myPath);
        return 0;
    }
    while ((entry = readdir(dir)) != 0)
    {
        lstat(entry->d_name, &buf);
        if (does_contain_case_insensitive_substring(entry->d_name, ".ASP"))
            *dataArray = g_list_insert_sorted(*dataArray, g_strdup(entry->d_name), sort_alphabetically);
    }
    closedir(dir);
    g_free(myPath);
    return 1;
}

/******************************************************************************
 *  Description:
 *      This function takes in a file path, and gets the path up to the 
 *      final forward-slash /. 
 *  Input:
 *      retVal - A pointer to a char * that we can copy to.
 *      path - The path that we are looking at
 *  Output:
 *      Sets reVal to a path
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void get_base_path(char **retVal, char *path)
{
    gchar *pos = g_strdup(path);
    pos = g_strstr_len(g_strreverse(pos), -1, "/");
    g_strreverse(pos);
    if (pos == NULL)
        return;
    *retVal = g_strdup(substring(pos, 0, strlen(pos) - 1));
}

/******************************************************************************
 *  Description:
 *      Gets the file name in a path.
 *  Input:
 *      retVal - a pointer to a char * that we can set
 *      path - The path that holds the filename 
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void get_file_name(char **retVal, char *path)
{
    gchar *pos = g_strdup(path);
    pos = g_strrstr(pos, "/");
    *retVal = g_strdup(substring(pos, 1, strlen(pos) - 1));
}

/******************************************************************************
 *  Description:
 *      This is a function that is used to free links in a GList
 *  Input:
 *      a - a string to be freed
 *      b - nothing
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void my_g_free(const void *a, const void *b)
{
    g_free((void *)a);
}

/******************************************************************************
 *  Description:
 *      This function clears a tree view. 
 *  Input:
 *      widget - A registered widget
 *      tree_view_name - The name of the tree view to clear
 *  Output:
 *      The tree view is cleared and no longer has any elements.
 *  Returns:
 *      1
 *  Notes:
 *****************************************************************************/
int clear_tree_view(GtkWidget *widget, char *tree_view_name)
{
    GtkTreeView *tree_view = GTK_TREE_VIEW(lookup_widget(widget, tree_view_name));
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeStore *tree_store = GTK_TREE_STORE(model);
    gtk_tree_store_clear(tree_store);
    return 1;
}

/******************************************************************************
 *  Description:
 *      This function clears a tree filter view
 *  Input:
 *      widget - A registered widget
 *      tree_view_name - THe name of the tree view to clear
 *  Output:
 *      The tree filter view is cleared.
 *  Returns:
 *      1
 *  Notes:
 *****************************************************************************/
int clear_tree_filter_view(GtkWidget *widget, char *tree_view_name)
{
    GtkTreeView *tree_view = GTK_TREE_VIEW(lookup_widget(widget, tree_view_name));
    GtkTreeModel *model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(tree_view)));
    GtkTreeStore *tree_store = GTK_TREE_STORE(model);
    gtk_tree_store_clear(tree_store);
    return 1;
}

/******************************************************************************
 *  Description:
 *      This function gets the value of a combobox.
 *  Input:
 *      widget - A registered widget
 *      comboName - The name of the combobox
 *      myValue - A pointer to a char * that we can set
 *  Output:
 *      Sets myValue to the value of the combobox
 *  Returns:
 *      1 if successful
 *      0 if not successful.
 *  Notes:
 *****************************************************************************/
int get_value_of_combo_box(GtkWidget *widget, char *comboName, char **myValue)
{
    GtkComboBox *myComboBox = GTK_COMBO_BOX(lookup_widget(widget, comboName));
    GtkTreeIter iter;
    GValue myGValue = {0};
    if (!gtk_combo_box_get_active_iter(myComboBox, &iter))
       return 0; 
    gtk_tree_model_get_value(gtk_combo_box_get_model(myComboBox), &iter, 0, &myGValue);
    gchar *tempString = g_strdup(g_value_get_string(&myGValue));
    *myValue = malloc(sizeof(char *) * (strlen(tempString) + 1));
    memcpy(*myValue, tempString, strlen(tempString) + 1);
    g_free(tempString);
    return 1;
}

/******************************************************************************
 *  Description:
 *      This function gets the text of a of a combobox entry.
 *  Input:
 *      widget - A registered widget
 *      comboName - The name of the combobox
 *      myValue - A pointer to a char * that we can set
 *  Output:
 *      Sets myValue to the value of the combobox
 *  Returns:
 *      1 if successful
 *      0 if not successful.
 *  Notes:
 *****************************************************************************/
int get_text_of_combo_box_entry(GtkWidget *widget, char *comboName, char **myValue)
{
    GtkComboBox *myComboBox = GTK_COMBO_BOX(lookup_widget(widget, comboName));
    *myValue = gtk_combo_box_get_active_text(myComboBox);
    if (*myValue == NULL)
        return 0;
    return 1;
}

/******************************************************************************
 *  Description:
 *      This function gets the current radar being displayed in the 
 *      selected packets tree view (the highlighted row).
 *  Input:
 *      widget - A registered widget
 *      radarName - A pointer to a char * that can be set.
 *  Output:
 *      Sets radarName to the value of the radar column that is selected
 *  Returns:
 *      The string length of the radar name if it is successful
 *      0 if not successful
 *  Notes:
 *****************************************************************************/
int get_current_radar_being_displayed(GtkWidget *widget, char **radarName)
{
    GtkTreeView* tree_view = GTK_TREE_VIEW(lookup_widget(widget, "selected_packets_tree_view"));
    GtkTreeStore *tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_view));
    GtkTreeIter iter;
    gchar *tempString;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tree_store), &iter)) 
        gtk_tree_model_get(GTK_TREE_MODEL(tree_store), &iter, 0, &tempString, -1);
    else
        return 0;
    *radarName = g_strdup(tempString);
    g_free(tempString);
    return strlen(*radarName);
}

/******************************************************************************
 *  Description:
 *      This function fills the radar combo list with values.  It looks for 
 *      subdirectories in the dir_location, and those become the names of 
 *      the radar. 
 *  Input:
 *      widget - A registered widget
 *  Output:
 *      The Radar combobox will be filled with the radar names
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void fill_radar_combo_list(GtkWidget *widget)
{
    DIR* dir;
    struct dirent* entry;
    struct stat buf;
    char *tempString;
    gchar *upperCaseStr;
    GList * myGlist = NULL;
    dir = opendir(get_dir_location());
    if (dir == NULL)
    {
        closedir(dir);
        return;
    }
    while ((entry = readdir(dir)) != 0)
    {
        lstat(entry->d_name, &buf);
        if ((S_ISDIR(buf.st_mode)) && (strcmp(entry->d_name, "log") != 0) 
            && (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0))
            if (g_list_find_custom(myGlist, entry->d_name, sort_alphabetically) == NULL)
            {
                tempString = g_strdup_printf("%s%s", get_dir_location(), entry->d_name);
                if (does_dir_contain_asp_files(tempString))
                {
                    upperCaseStr = g_ascii_strup(entry->d_name, strlen(entry->d_name));
                    myGlist = g_list_insert_sorted(myGlist, g_strdup(upperCaseStr), sort_alphabetically);
                    g_free(upperCaseStr);
                }
                g_free(tempString);
            }
    }
    closedir(dir);

    /* If we get to here, then they supplied a default location that is not
     * working.  So we will try it with ours. */
    if ((g_list_length(myGlist) == 0) && (strcmp(get_dir_location(), "/import/orpg/ASP_Prod/\0") != 0))
    {
        set_dir_location("/import/orpg/ASP_Prod/\0");
        dir = opendir(get_dir_location());
        if (dir == NULL)
        {
            closedir(dir);
            return;
        }
        while ((entry = readdir(dir)) != 0)
        {
            lstat(entry->d_name, &buf);
            if ((S_ISDIR(buf.st_mode)) && (strcmp(entry->d_name, "log") != 0) 
                && (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0))
                if (g_list_find_custom(myGlist, entry->d_name, sort_alphabetically) == NULL)
                {
                    tempString = g_strdup_printf("%s%s", get_dir_location(), entry->d_name);
                    if (does_dir_contain_asp_files(tempString))
                    {
                        upperCaseStr = g_ascii_strup(entry->d_name, strlen(entry->d_name));
                        myGlist = g_list_insert_sorted(myGlist, g_strdup(upperCaseStr), sort_alphabetically);
                        g_free(upperCaseStr);
                    }
                    g_free(tempString);
                }
        }
        closedir(dir);
    }

    GtkComboBox *myCombo = GTK_COMBO_BOX(lookup_widget(widget, "radar_combo"));
    int i;
    for (i = 0; i < g_list_length(myGlist); i++)
        gtk_combo_box_append_text(myCombo, g_list_nth_data(myGlist, i));

    /* Important if we can't connect to /import/orpg/ASP_Prod */
    if (g_list_length(myGlist) > 0)
        gtk_combo_box_set_active(myCombo, 0);
    g_list_foreach(myGlist, (GFunc)my_g_free, NULL);
    g_list_free(myGlist); 
}

/******************************************************************************
 *  Description:
 *      Thif function takes in a GList and sets its values in a combo box.
 *  Input:
 *      combobox - A combobox pointer
 *      myList - A GList of strings
 *      order - Do we prepend or append (append is faster)
 *      start_pos - Where do we set the active item
 *  Output:
 *  Returns:
 *  Notes:
 *****************************************************************************/
void update_combobox_from_glist(GtkComboBox *combobox, GList *myList, int order, int start_pos)
{
    int i;
    GtkTreeModel *store;
    store = gtk_combo_box_get_model(combobox);
    gtk_list_store_clear(GTK_LIST_STORE(store));
    if (order == 1)
        for (i = 0; i < g_list_length(myList); i++)
          gtk_combo_box_prepend_text(combobox, g_list_nth_data(myList, i));
    else
        for (i = 0; i < g_list_length(myList); i++)
          gtk_combo_box_append_text(combobox, g_list_nth_data(myList, i));
    if (g_list_length(myList) > 0)
        gtk_combo_box_set_active(combobox, start_pos);
}

/******************************************************************************
 *  Description:
 *      This function gets the value of a spin button
 *  Input:
 *      widget - A registered widget
 *      spinbutton - The name of the spinbutton
 *  Returns:
 *      Returns an integer that represents the value of the spinbutton.
 *  Notes:
 *****************************************************************************/
int get_spin_button_value(GtkWidget *widget, char * spinbutton)
{
    int val = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(lookup_widget(widget, spinbutton)));
    return val;
}

/******************************************************************************
 *  Description:
 *      This function makes the calls to set the values of the spinbuttons
 *  Input:
 *      widget - A registered widget
 *      filter_window - The filter window widget
 *  Returns:
 *      None
 *  Notes:
 *****************************************************************************/
void set_spin_buttons(GtkWidget *widget, GtkWidget *filter_window)
{
    set_range_start_spinbuttons(widget, filter_window);
    set_range_end_spinbuttons(widget, filter_window);
    set_match_spinbuttons(widget, filter_window);
}

/******************************************************************************
 *  Description:
 *      This function sets the value of a spinbutton
 *  Input:
 *      widget - A registered widget
 *      spinbutton - The name of the spinbutton we are setting
 *      value - The value that the spinbutton should be.
 *  Returns:
 *      None
 *  Notes:
 *****************************************************************************/
void set_spin_button_value(GtkWidget *widget, char *spinbutton, int value)
{
    GtkSpinButton *spinner;
    spinner = GTK_SPIN_BUTTON(lookup_widget(widget, spinbutton));
    gtk_spin_button_set_value(spinner, value);
}

/******************************************************************************
 *  Description:
 *      This function gets the value of the range start spinbuttons and 
 *      converts them to a time_t value.
 *  Input:
 *      widget - A registered widget
 *  Returns:
 *      A time_t struct
 *  Notes:
 *****************************************************************************/
time_t get_range_start_time(GtkWidget *widget)
{
    int year = get_spin_button_value(widget, "select_dates_range_year_start_spinbutton");
    int month = get_spin_button_value(widget, "select_dates_range_month_start_spinbutton") - 1;
    int day = get_spin_button_value(widget, "select_dates_range_day_start_spinbutton");
    int hour = get_spin_button_value(widget, "select_dates_range_hour_start_spinbutton");
    int minute = get_spin_button_value(widget, "select_dates_range_minute_start_spinbutton");
    int second = get_spin_button_value(widget, "select_dates_range_second_start_spinbutton");
    time_t start_time = entries_to_time_t(year, month, day, hour, minute, second);
    return start_time;
}

/******************************************************************************
 *  Description:
 *      This function gets the value of the range end spinbuttons and 
 *      converts them to a time_t value.
 *  Input:
 *      widget - A registered widget
 *  Returns:
 *      A time_t struct
 *  Notes:
 *****************************************************************************/
time_t get_range_end_time(GtkWidget *widget)
{
    int year = get_spin_button_value(widget, "select_dates_range_year_end_spinbutton");
    int month = get_spin_button_value(widget, "select_dates_range_month_end_spinbutton") - 1;
    int day = get_spin_button_value(widget, "select_dates_range_day_end_spinbutton");
    int hour = get_spin_button_value(widget, "select_dates_range_hour_end_spinbutton");
    int minute = get_spin_button_value(widget, "select_dates_range_minute_end_spinbutton");
    int second = get_spin_button_value(widget, "select_dates_range_second_end_spinbutton");
    time_t end_time = entries_to_time_t(year, month, day, hour, minute, second);
    return end_time;
}

/******************************************************************************
 *  Description:
 *      This function gets the value of the match spinbuttons.
 *  Input:
 *      widget - A registered widget
 *  Returns:
 *      A time_t structure.
 *  Notes:
 *****************************************************************************/
time_t get_match_spinbutton_time(GtkWidget *widget)
{
    int year = get_spin_button_value(widget, "select_dates_match_year_spinbutton");
    int month = get_spin_button_value(widget, "select_dates_match_month_spinbutton") - 1;
    int day = get_spin_button_value(widget, "select_dates_match_day_spinbutton");
    int hour = get_spin_button_value(widget, "select_dates_match_hour_spinbutton");
    int minute = get_spin_button_value(widget, "select_dates_match_minute_spinbutton");
    int second = get_spin_button_value(widget, "select_dates_match_second_spinbutton");
    time_t match_time = entries_to_time_t(year, month, day, hour, minute, second);
    return match_time;
}

/******************************************************************************
 *  Description:
 *      This function checks to see if a file is actually a valid ASP product.
 *  Input:
 *      filePath - the path to the file
 *  Returns:
 *      0 if the file is not an ASP product
 *      non 0 if the file is valid
 *  Notes:
 *****************************************************************************/
int is_file_a_valid_product(char *filePath)
{
    char **packet;
    char *radarName;
    int numPackets = 0;
    int result = generate_packet(&packet, &radarName, &numPackets, filePath);
    int i;
    if (numPackets == 0)
        return 0;
    for (i = 0; i < numPackets; i++)
        free(packet[i]);
    free(packet);
    free(radarName);
    return result;
}

/******************************************************************************
 *  Description:
 *      This function removes a row from a tree view.  It has to be a 
 *      selected for it to be removed. 
 *  Input:
 *      widget - A registered widget
 *      tree_view_name - The name of the tree view
 *  Output:
 *      A tree view with a row removed
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void remove_from_tree_view(GtkWidget *widget, char *tree_view_name)
{
    GtkTreeIter iter;
    GtkTreeView *tree_view = GTK_TREE_VIEW(lookup_widget(widget, tree_view_name));
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeSelection *select;
    GtkTreeRowReference *reference;
    GtkTreePath *path;
    GList *pathList, *refList;
    gint row;

    select = gtk_tree_view_get_selection(tree_view);
    pathList = gtk_tree_selection_get_selected_rows(select, NULL);
    pathList = g_list_first(pathList);
    refList = NULL;

    /* Add all of the row references */
    while (pathList) {
        reference = gtk_tree_row_reference_new(model, (GtkTreePath *)pathList->data);
        refList = g_list_append(refList, (void *)reference);
        pathList = g_list_next(pathList);
    }

    refList = g_list_first(refList);
    while (refList) 
    {
        path = gtk_tree_row_reference_get_path((GtkTreeRowReference *)refList->data);
        row = gtk_tree_path_get_indices(path)[0];
        if (gtk_tree_model_get_iter(model, &iter, path)) {
            gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
        }
        gtk_tree_path_free(path);
        refList = g_list_next(refList);
    }

    g_list_foreach(refList, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(refList);
    g_list_foreach(pathList, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(pathList);
}

/******************************************************************************
 *  Description:
 *      This function adds a packet to the select packet list.  
 *  Input:
 *      widget - A registered widget
 *      radarName - The name of the radar that coincides with the packet
 *      packetName - The name of the packet
 *      path - The path to the packet
 *  Output:
 *      This adds a packet to the select_packets_tree_view
 *  Returns:
 *      Return 1 if it works.
 *      0 if this doesnt work.
 *  Notes:
 *****************************************************************************/
int add_unique_packet_to_packets_list(GtkWidget *widget, char *radarName, char *packetName, char *path)
{
    GtkTreeView* tree_view = GTK_TREE_VIEW(lookup_widget(widget, "selected_packets_tree_view"));
    GtkTreeStore *tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_view));
    GtkTreeIter iter;
    gchar *tempString;
    if ((radarName == NULL) || (packetName == NULL))
      return 0;

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tree_store), &iter)) 
    {
        do 
        {
            gtk_tree_model_get(GTK_TREE_MODEL(tree_store), &iter, 1, &tempString, -1);
            if (strcmp(tempString, packetName) == 0)
            {
                gtk_tree_view_set_cursor(tree_view, gtk_tree_model_get_path(GTK_TREE_MODEL(tree_store), &iter), NULL, FALSE);
                g_free(tempString);
                return 0;
            }
            g_free(tempString);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(tree_store), &iter));
    }
    gtk_tree_store_prepend(tree_store, &iter, NULL);
    gtk_tree_store_set(tree_store, &iter,
                0, radarName,
                1, packetName, 
                2, path, -1);
    gtk_tree_view_set_cursor(tree_view, gtk_tree_model_get_path(GTK_TREE_MODEL(tree_store), &iter), NULL, FALSE);
    return 1;
}

/******************************************************************************
 *  Description:
 *      This function adds a packet to the select packet list.  
 *  Input:
 *      widget - A registered widget
 *      radarName - The name of the radar that coincides with the packet
 *      packetName - The name of the packet
 *      path - The path to the packet
 *      prepend - Do we prepend or append the packet
 *  Output:
 *      This adds a packet to the select_packets_tree_view
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void add_packet_to_selected_packets_list(GtkWidget *widget, char *radarName, char *packetName, char *path, int prepend)
{
    GtkTreeView* tree_view = GTK_TREE_VIEW(lookup_widget(widget, "selected_packets_tree_view"));
    GtkTreeStore *tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_view));
    GtkTreeIter iter;
    if (prepend)
        gtk_tree_store_prepend(tree_store, &iter, NULL);
    else    
        gtk_tree_store_append(tree_store, &iter, NULL);
    gtk_tree_store_set(tree_store, &iter,
                0, radarName,
                1, packetName, 
                2, path, -1);

}

/******************************************************************************
 *  Description:
 *      This function gets the newest packet date (i.e. top row) of the 
 *      main message list.
 *  Input:
 *      widget - A registered widget
 *  Returns:
 *      Returns a time_t representation of the time in the top row
 *  Notes:
 *****************************************************************************/
static time_t get_newest_packet_date(GtkWidget *widget)
{
    char *dateVal;
    char *timeVal;
    get_content_of_first_row_of_tree_model(widget, "message_tree", 0, &dateVal);
    get_content_of_first_row_of_tree_model(widget, "message_tree", 1, &timeVal);
    time_t returnTime = convert_string_date_to_time_t(dateVal, timeVal);
    free(dateVal);
    free(timeVal);
    return returnTime;
}

/******************************************************************************
 *  Description:
 *      This function gets the oldest packet date (i.e. bottom row) of the 
 *      main message list.
 *  Input:
 *      widget - A registered widget
 *  Returns:
 *      Returns a time_t representation of the time in the bottom row
 *  Notes:
 *****************************************************************************/
static time_t get_oldest_packet_date(GtkWidget *widget)
{ 
    char *dateVal;
    char *timeVal;
    get_content_of_last_row_of_tree_model(widget, "message_tree", 0, &dateVal);
    get_content_of_last_row_of_tree_model(widget, "message_tree", 1, &timeVal);
    time_t returnTime = convert_string_date_to_time_t(dateVal, timeVal);
    free(dateVal);
    free(timeVal);
    return returnTime;
}

/******************************************************************************
 *  Description:
 *      This function sets the values of the spinbuttons that perform matching.
 *  Input:
 *      widget - a registered widget
 *      filter_window - The filter_window widget
 *  Output:
 *      Changed spinbuttons
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void set_match_spinbuttons(GtkWidget *widget, GtkWidget *filter_window)
{
    time_t newestTime = get_newest_packet_date(widget);
    struct tm *newTM;
    newTM = localtime(&newestTime);
    GtkSpinButton *spinner;
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_match_year_spinbutton"));
    gtk_spin_button_set_value(spinner, newTM->tm_year - 100);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_match_month_spinbutton"));
    gtk_spin_button_set_value(spinner, newTM->tm_mon + 1);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_match_day_spinbutton"));
    gtk_spin_button_set_value(spinner, newTM->tm_mday);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_match_hour_spinbutton"));
    gtk_spin_button_set_value(spinner, newTM->tm_hour);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_match_minute_spinbutton"));
    gtk_spin_button_set_value(spinner, newTM->tm_min);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_match_second_spinbutton"));
    gtk_spin_button_set_value(spinner, newTM->tm_sec);
}

/******************************************************************************
 *  Description:
 *      This function sets the values of the spin buttons that represent the 
 *      newest/earliest packet time.
 *  Input:
 *      widget - A registered widget
 *      filter_window - The filter window widget
 *  Output:
 *      Updated spinbuttons
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void set_range_start_spinbuttons(GtkWidget *widget, GtkWidget *filter_window)
{
    time_t newestTime = get_newest_packet_date(widget);
    struct tm *newTM;
    newTM = localtime(&newestTime);
    GtkSpinButton *spinner;
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_range_year_start_spinbutton"));
    gtk_spin_button_set_value(spinner, newTM->tm_year - 100);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_range_month_start_spinbutton"));
    gtk_spin_button_set_value(spinner, newTM->tm_mon + 1);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_range_day_start_spinbutton"));
    gtk_spin_button_set_value(spinner, newTM->tm_mday);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_range_hour_start_spinbutton"));
    gtk_spin_button_set_value(spinner, newTM->tm_hour);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_range_minute_start_spinbutton"));
    gtk_spin_button_set_value(spinner, newTM->tm_min);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_range_second_start_spinbutton"));
    gtk_spin_button_set_value(spinner, newTM->tm_sec);
}

/******************************************************************************
 *  Description:
 *      This function sets the values of the spin buttons that represent the 
 *      oldest/latest packet time.
 *  Input:
 *      widget - A registered widget
 *      filter_window - The filter window widget
 *  Output:
 *      Updated spinbuttons
 *  Notes:
 *****************************************************************************/
void set_range_end_spinbuttons(GtkWidget *widget, GtkWidget *filter_window)
{
    time_t oldestTime = get_oldest_packet_date(widget);
    struct tm *oldTM;
    oldTM = localtime(&oldestTime);
    GtkSpinButton *spinner;
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_range_year_end_spinbutton"));
    gtk_spin_button_set_value(spinner, oldTM->tm_year - 100);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_range_month_end_spinbutton"));
    gtk_spin_button_set_value(spinner, oldTM->tm_mon + 1);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_range_day_end_spinbutton"));
    gtk_spin_button_set_value(spinner, oldTM->tm_mday);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_range_hour_end_spinbutton"));
    gtk_spin_button_set_value(spinner, oldTM->tm_hour);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_range_minute_end_spinbutton"));
    gtk_spin_button_set_value(spinner, oldTM->tm_min);
    spinner = GTK_SPIN_BUTTON(lookup_widget(filter_window, "select_dates_range_second_end_spinbutton"));
    gtk_spin_button_set_value(spinner, oldTM->tm_sec);
}

/******************************************************************************
 *  Description:
 *      This value sets the checkbuttons.  The values are based upon the 
 *      values in the ignore_msg_types array.
 *  Input:
 *      widget - A registered widget
 *      filter_window - The filter_window widget
 *  Output:
 *      The checkboxes will be checked/unchecked accordingly
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void set_checkboxes(GtkWidget *widget, GtkWidget *filter_window)
{
    if (ignore_msg_types[0])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "informational_checkbutton")), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "informational_checkbutton")), FALSE);
    if (ignore_msg_types[1])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "general_status_checkbutton")), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "general_status_checkbutton")), FALSE);
    if (ignore_msg_types[2])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "warnings_errors_checkbutton")), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "warnings_errors_checkbutton")), FALSE);
    if (ignore_msg_types[3])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "narrowband_communications_checkbutton")), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "narrowband_communications_checkbutton")), FALSE);
    if (ignore_msg_types[4])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "maintenance_mandatory_checkbutton")), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "maintenance_mandatory_checkbutton")), FALSE);
    if (ignore_msg_types[5])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "maintenance_required_checkbutton")), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "maintenance_required_checkbutton")), FALSE);
    if (ignore_msg_types[6])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "load_shed_checkbutton")), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "load_shed_checkbutton")), FALSE);
    if (ignore_msg_types[8])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "secondary_rda_alarm_checkbutton")), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "secondary_rda_alarm_checkbutton")), FALSE);
    if (ignore_msg_types[9])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "maintenance_required_rda_alarm_checkbutton")), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "maintenance_required_rda_alarm_checkbutton")), FALSE);
    if (ignore_msg_types[10])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "maintenance_mandatory_rda_alarm_checkbutton")), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "maintenance_mandatory_rda_alarm_checkbutton")), FALSE);
    if (ignore_msg_types[11])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "inoperable_rda_alarm_checkbutton")), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(filter_window, "inoperable_rda_alarm_checkbutton")), FALSE);
}

/******************************************************************************
 *  Description:
 *      This function gets the content of the first row of a tree view
 *  Input:
 *      widget - A registered widget
 *      tree_view_name - The name of the tree view 
 *      col - The column that we want
 *      returnVal - a pointer to a char * that we are setting
 *  Output:
 *      returnVal will be set and will hold the data being requested.
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
static void get_content_of_first_row_of_tree_model(GtkWidget *widget, char *tree_view_name, int col, char **returnVal)
{
    GtkTreeView* tree_view = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(widget), tree_view_name));
    GtkTreeModel *tree_model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(tree_model, &iter))
    {
        GValue myValue = {0};
        gtk_tree_model_get_value(tree_model, &iter, col, &myValue);
        *returnVal = g_strdup(g_value_get_string(&myValue));
        g_value_unset(&myValue);
    }
}

/******************************************************************************
 *  Description:
 *      This function gets the data in the last row of a tree model.
 *  Input:
 *      widget - A registered widget
 *      tree_view_name - The name of the tree view
 *      col - The column of the tree view
 *      returnVal - A pointer to a char * that we can set
 *  Output:
 *      returnVal will be set
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
static void get_content_of_last_row_of_tree_model(GtkWidget *widget, char *tree_view_name, int col, char **returnVal)
{
    GtkTreeView* tree_view = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(widget), tree_view_name));
    GtkTreeModel *tree_model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    if (gtk_tree_model_iter_nth_child(tree_model, &iter, NULL, gtk_tree_model_iter_n_children(tree_model, NULL) - 1))
    {
        GValue myValue = {0};
        gtk_tree_model_get_value(tree_model, &iter, col, &myValue);
        *returnVal = g_strdup(g_value_get_string(&myValue));
        g_value_unset(&myValue);
    }
}

/******************************************************************************
 *  Description:
 *      This function gets all of the information in a tree model and sets 
 *      it in a GList
 *  Input:
 *      widget - A registered widget
 *      tree_view_name - The name of the tree view
 *      col - The column that holds the information
 *      returnVal - A pointer to a GList that we can set
 *  Output:
 *      returnVal will be set
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
static void get_all_rows_of_tree_model_column(GtkWidget *widget, char *tree_view_name, int col, GList **returnVal)
{
    GtkTreeView* tree_view = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(widget), tree_view_name));
    GtkTreeModel *tree_model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(tree_model, &iter))
        do 
        {
            GValue myValue = {0};
            gtk_tree_model_get_value(tree_model, &iter, col, &myValue);
            *returnVal = g_list_append(*returnVal, g_strdup(g_value_get_string(&myValue)));
            g_value_unset(&myValue);
        } while(gtk_tree_model_iter_next(tree_model, &iter));
}

/******************************************************************************
 *  Description:
 *      Gets the number of elements in the selected packets list
 *  Input:
 *      widget - A registered widget
 *  Returns:
 *      The number of rows
 *  Notes:
 *****************************************************************************/
int get_tree_selected_packets_list_size(GtkWidget *widget)
{
    GtkTreeView* tree_view = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(widget), "selected_packets_tree_view"));
    GtkTreeModel *tree_model = gtk_tree_view_get_model(tree_view);
    return gtk_tree_model_iter_n_children(tree_model, NULL);
}

/******************************************************************************
 *  Description:
 *      This function takes in a time_t, converts it to a tm struct, and then 
 *      compares the values to those of of their corresponding match 
 *      spinbutton times
 *  Input:
 *      widget - A registered widget
 *      rowTime - The time being compared against
 *  Returns:
 *      Returns 1 if the time matches those in the spin buttons
 *      returns 0 otherwise
 *  Notes:
 *****************************************************************************/
int compare_match_values_to_spinbuttons(GtkWidget *widget, time_t rowTime)
{
    struct tm *tp;
    tp = localtime(&rowTime);
    int retVal = 1;
    if (match_filter_time.tm_year >= 0)
        retVal = (((match_filter_time.tm_year + 100) == tp->tm_year) && retVal) ? 1 : 0;
    if (match_filter_time.tm_mon >= 0)
        retVal = ((match_filter_time.tm_mon == tp->tm_mon) && retVal) ? 1 : 0;
    if (match_filter_time.tm_mday > 0)
        retVal = ((match_filter_time.tm_mday == tp->tm_mday) && retVal) ? 1 : 0;
    if (match_filter_time.tm_hour >= 0)
        retVal = ((match_filter_time.tm_hour == tp->tm_hour) && retVal) ? 1 : 0;
    if (match_filter_time.tm_min >= 0)
        retVal = ((match_filter_time.tm_min == tp->tm_min) && retVal) ? 1 : 0;
    if (match_filter_time.tm_sec >= 0)
        retVal = ((match_filter_time.tm_sec == tp->tm_sec) && retVal) ? 1 : 0;
    return retVal;
}

/******************************************************************************
 *  Description:
 *      This is the tree model filter function that is used.  
 *  Input:
 *      model - The model that we are filtering
 *      iter - A valid iterator
 *      data - user data
 *  Returns:
 *      True if the row should be seen
 *      False if the row should be invisible
 *  Notes:
 *      This is our filter function, so it needs to be as quick as possible.  No
 *      lookups here.  
 *****************************************************************************/
gboolean message_tree_filtering_function(GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    gchar *date_string;
    gchar *time_string;
    gchar *type_string;
    gchar *main_string;
    time_t rowTime;
    int returnVal = 0;

    gtk_tree_model_get(model, iter, 
                       0, &date_string, 
                       1, &time_string, 
                       2, &main_string, 
                       4, &type_string, -1);

    if (type_string == NULL)
    {
        g_free(date_string);
        g_free(time_string);
        g_free(type_string);
        return FALSE;
    }

    rowTime = convert_string_date_to_time_t(date_string, time_string);
    if (ignore_msg_types[get_message_type_index(type_string)] > 0)
    {
        if (get_filter_window_status() > 0)
        {
            if ((range_start_filter_time > 0) && (range_end_filter_time > 0))
            {
                if ((rowTime <= range_start_filter_time) && (rowTime >= range_end_filter_time))
                    returnVal = 1;
            }
            else if ((match_filter_time.tm_sec >= 0) || (match_filter_time.tm_min>= 0) || (match_filter_time.tm_hour >= 0) || (match_filter_time.tm_mday >= 0) || (match_filter_time.tm_mon >= 0) || (match_filter_time.tm_year>= 0))
                returnVal = compare_match_values_to_spinbuttons(GTK_WIDGET(data), rowTime);
            else if (ignore_msg_types[get_message_type_index(type_string)])
                returnVal = 1;
            else
                returnVal = 0;
        }
        else if (ignore_msg_types[get_message_type_index(type_string)])
            returnVal = 1;
        else
            returnVal = 0;
    }
    else
        returnVal = 0;
 
    if (search_string != NULL)
        if ((strlen(search_string) > 0) && returnVal)
        {
            if (does_contain_case_insensitive_substring(main_string, search_string))
                returnVal = 1;
            else
                returnVal = 0;
        }

    g_free(date_string);
    g_free(time_string);
    g_free(type_string);
    g_free(main_string);

    if (returnVal == 1)
        return ( TRUE );
    return ( FALSE );
}

/******************************************************************************
 *  Description:
 *  Input:
 *  Output:
 *  Returns:
 *  Notes:
 *****************************************************************************/
void add_glist_of_files_to_packet_list(GtkWidget *widget, GList *fileList, char *basePath)
{
    if (g_list_length(fileList) <= 0)
        return;
    int i;
      for (i = 0; i < g_list_length(fileList); i++)
          add_unique_packet_to_packets_list(widget, "file", g_list_nth_data(fileList, i), basePath);
}

/******************************************************************************
 *  Description:
 *      This function takes all of the packets in the selected packet list, 
 *      generates the packet for them, and then inserts them into the 
 *      main tree.
 *  Input:
 *      widget - A registered widget
 *      tree_store - The main tree store
 *  Output:
 *      The main list will be updated
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void update_main_from_select_packets_list(GtkWidget *widget, GtkTreeStore *tree_store)
{
    GList *packetList = NULL;
    get_all_rows_of_tree_model_column(widget, "selected_packets_tree_view", 1, &packetList);
    GList *pathList = NULL;
    get_all_rows_of_tree_model_column(widget, "selected_packets_tree_view", 2, &pathList);
    struct tm* timeinfo;
    time_t rawtime;

    if (g_list_length(packetList) > 0)
    {
        int i, j, packetLen;
        char **packet;
        char *radar;
        char *radarName;
        gchar *filename;
        get_current_radar_being_displayed(widget, &radar);
        for (i = g_list_length(packetList) - 1; i >= 0; i--)
        {
            rawtime = pull_date_from_file_name((char *)g_list_nth_data(packetList, i));
            timeinfo = localtime(&rawtime);
            filename = g_strdup_printf("%s/%d/%02d/%s", (char *)g_list_nth_data(pathList, i), timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, (char *)g_list_nth_data(packetList, i));
            if (generate_packet(&packet, &radarName, &packetLen, filename) > 0)
            {
                for (j = 0; j < packetLen; j++)
                {
                    /* Make sure we have valid information */
                    if (g_utf8_validate(packet[j], strlen(packet[j]), NULL))
                        add_row_to_main_tree_store(&tree_store, packet[j]);
                    free(packet[j]);
                }
                free(packet);
            }
            g_free(filename);
        }
        free(radar);
        free(radarName);
    }
    g_list_foreach(packetList, (GFunc)my_g_free, NULL);
    g_list_free(packetList);
    g_list_foreach(pathList, (GFunc)my_g_free, NULL);
    g_list_free(pathList);
}

/******************************************************************************
 *  Description:
 *      This function gets all of the pointers that are necessary to 
 *      use the update_main_from_select_packets_list function.
 *  Input:
 *      widget - A registered widget
 *  Output:
 *      The main tree store will be updated
 *  Returns:
 *      Nothing
 *  Notes:
 *      This function does the insertion into the main tree a little 
 *      differently.  Instead of just clearing the model, inserting the rows,
 *      and then refiltering, we actually detach the tree store from the 
 *      tree view, add data to the tree store, and then reattach it.  This 
 *      provides a huge speed increase when we are talking about a large 
 *      number of rows.  
 *****************************************************************************/
void set_all_selected_packets_to_main_window(GtkWidget *widget)
{
    GtkTreeView* tree_view = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(widget), "message_tree"));
    GtkTreeModel *tree_filter = gtk_tree_view_get_model(tree_view);
    GtkTreeStore *tree_store = GTK_TREE_STORE(gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(tree_filter)));
    /* Without this, big problems occur if the packet passed isn't valid */
    if (get_tree_selected_packets_list_size(widget) < 1)
        return;
    g_object_ref(G_OBJECT(tree_store));

    /* Detach the model */
    gtk_tree_view_set_model(tree_view, NULL);

    /* Clear the model */
    gtk_tree_store_clear(tree_store);
    update_main_from_select_packets_list(widget, tree_store);

    /* Recreate the filter and add the tree store to the filter */
    tree_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tree_store), NULL);
    gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(tree_filter), 
            (GtkTreeModelFilterVisibleFunc)message_tree_filtering_function, NULL, NULL);

    if (gtk_tree_model_iter_n_children(gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(tree_filter)), NULL) > 0)
        gtk_tree_view_set_model(tree_view, tree_filter);
    g_object_unref(G_OBJECT(tree_filter));
    g_object_unref(G_OBJECT(tree_store));
}

/******************************************************************************
 *  Description:
 *      This function takes in data to add to a main tree store.  It 
 *      breaks apart the string according to how it is packaged in the 
 *      ASP product, and then performs the insert.
 *  Input:
 *      tree_store - The tree store that we are adding to.
 *      row - The message that we are adding
 *  Output:
 *      The tree store will have another row.
 *  Returns:
 *      Nothing
 *  Notes:
 *****************************************************************************/
void add_row_to_main_tree_store(GtkTreeStore **tree_store, char *row)
{
    GtkTreeIter iter;
    int firstPipe, openBracketLoc, closeBracketLoc, endTimeLoc = -1;
    char *message_type;
    firstPipe = strcspn(row, "|");
    openBracketLoc = strcspn(row, "[");
    closeBracketLoc = strcspn(row, "]");
    endTimeLoc = strcspn(row, ">");
    if ((firstPipe >= 0) && (openBracketLoc >= 0) && (closeBracketLoc >= 0) && (endTimeLoc >= 0))
    {
        message_type = substring(row, 0, firstPipe);
        gtk_tree_store_prepend(*tree_store, &iter, NULL);
        gtk_tree_store_set(*tree_store, &iter,
                 0, string_trim(substring(row, firstPipe + 2, openBracketLoc - firstPipe - 3)),
                 1, substring(row, openBracketLoc + 1, closeBracketLoc - openBracketLoc - 1),
                 2, substring(row, closeBracketLoc + 5, strlen(row) - closeBracketLoc - 6),
                 3, map_message_type_to_color(message_type),
                 4, message_type, -1);
        free(message_type);
    }
}

