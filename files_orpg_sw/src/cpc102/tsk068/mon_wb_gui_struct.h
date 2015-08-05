/*
 * RCS info 
 * $Author: jclose $
 * $Locker:  $
 * $Date: 2009/06/29 16:02:05 $
 * $Id: mon_wb_gui_struct.h,v 1.1 2009/06/29 16:02:05 jclose Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
#include <gtk/gtk.h>

/* enum for the Filter textviews */
enum 
{
    FILTER_TYPE_COL = 0, 
    FILTER_COLOR_COL, 
    FILTER_ID_COL, 
    FILTER_NUM_COLS
};

/* enum for the constant field textview */
enum
{
    CONST_FIELD_COL = 0,
    CONST_VALUE_COL,
    CONST_COLOR_COL, 
    CONST_NUM_COLS
};

/* enum for the message list textview */
enum
{
    MSG_LIST_TIME_COL = 0, 
    MSG_LIST_MSG_ID_COL, 
    MSG_LIST_MSG_COL, 
    MSG_LIST_COLOR_COL, 
    MSG_LIST_MSG_TYPE_ID_COL, 
    MSG_LIST_MSG_READ_LEN, 
    MSG_LIST_NUM_COLS
};

/* A struct to hold our arguments, to be used 
 * for passing info between threads */
typedef struct
{
    int argc;
    char **argv;
} argstruct;

/* A struct to hold our pointers */
typedef struct
{
    GtkTreeView *message_list_treeview;
    GtkTreeView *constant_treeview;
    GtkTextView *main_message_display_textview;
    GtkTreeView *available_filters_treeview;
    GtkTreeView *current_filters_treeview;
    GtkSpinButton *verbosity_spinbutton;
    GtkSpinButton *log_size_spinbutton;
    GtkRadioButton *scroll_radiobutton;
    GtkRadioButton *pause_radiobutton;
    GtkRadioButton *freeze_radiobutton;
    int toExit;
    int toFreeze;
} guiPointerStruct;

