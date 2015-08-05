/*
 * RCS info
 * $Author: jclose $
 * $Locker:  $
 * $Date: 2009/06/29 16:02:04 $
 * $Id: mon_wb_gui.c,v 1.1 2009/06/29 16:02:04 jclose Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
#include "mon_wb_gui.h"

/******************************************************************************
 * File Name: 
 *   mon_wb_gui.c
 *
 * Description: 
 *   mon_wb_gui is a GUI wrapper for the mon_wb program.  It is 
 *   multi-threaded, with the GUI running in the main thread and the mon_wb 
 *   code running as a spawned thread.  Essentially, mon_wb_gui tries to buffer
 *   each packet coming in to mon_wb, then when the user desires, it runs
 *   the packet through the mon_wb code and captures the output, displaying it
 *   onto the screen.  The packets are captured and held in a GList.  
 *
 *****************************************************************************/

/******************************************************************************
 * GLOBAL VARIABLES
 *****************************************************************************/
/* Our mutex */
GMutex* wbmutex = NULL;
/* The pointers to all of our GUI componenets */
guiPointerStruct* ptrList = NULL;
/* The linked list of all of our raw data */
GList* messageList = NULL;
/* The message type values that will be filtered */
GList* filterList = NULL;
/* The initial allowable size of our message list */
int bufferSize = 100;

/*******************************************************************************
* Function Name:	
*	main
*
* Description:
*	Acts as the controller for the mon_wb tool. 
*	
* Inputs:
*	None
*	
* Returns:
*	0 upon successful completion.
*	-1 upon encountering a terminating error.	
*
* Author:
*	R. Solomon, RSIS.
*
* History:
*	03-14-2003, R. Solomon.  Created.
*******************************************************************************/
int main( int argc, char** argv)
{
    /* Get the CFG_DIR where the xml is located */
    char* cfgdir = NULL; 
    char* gladeFile = NULL; 
    cfgdir = getenv("CFG_DIR");
    if (cfgdir == NULL)
    {
        fprintf(stderr, " * Could not find mon_wb_gui.glade file. Please check CFG_DIR environmental variable.\n");
        return (0);
    }
    gladeFile = g_strdup_printf("%s/%s", cfgdir, "mon_wb_gui.xml");

    GladeXML*   builder = NULL;
    GtkWidget*  outer_window;
    GThread *lb_thread;
    GError *err1 = NULL;
    if (!g_thread_supported())
    {
        g_thread_init(NULL);
        gdk_threads_init();
    }
    wbmutex = g_mutex_new();

    gtk_set_locale();
    gtk_init( &argc, &argv );

    /* Initialize our variables */
    argstruct* args;
    args = g_slice_new(argstruct);
    ptrList = g_slice_new(guiPointerStruct);

    /* This is building our GUI */
    builder = glade_xml_new( gladeFile, "outer_window", NULL );
    if (builder == NULL)
    {
        free(gladeFile);
        g_slice_free(guiPointerStruct, ptrList);
        g_slice_free(argstruct, args);
        g_mutex_free(wbmutex);
        return (0);
    }
    glade_xml_signal_autoconnect( builder );
    outer_window = glade_xml_get_widget( builder, "outer_window" );

    /* Sets our values for our globals */
    popup_txtview = NULL;
    set_global_pointers(builder);
    add_constants_renderer(builder);
    add_message_list_renderer(builder);
    add_top_filter_renderer(builder);
    add_bottom_filter_renderer(builder);
    initiate_constant_columns(outer_window);
    initiate_available_filters_columns(outer_window);

    /* So we can pass this to the lb thread */
    args->argc = argc;
    args->argv = argv;
    g_object_unref(G_OBJECT(builder));

    /* Create our other thread. */
    if ((lb_thread = g_thread_create((GThreadFunc)lb_main, (void *)args, TRUE, &err1)) == NULL)
    {
        fprintf(stdout, "lb Thread creation failed: %s!\n", err1->message);
        g_error_free(err1);
        g_slice_free(guiPointerStruct, ptrList);
        g_slice_free(argstruct, args);
        return(0);
    }

    gtk_widget_show( outer_window );
    gdk_threads_enter();
    gtk_main();
    gdk_threads_leave();

    /* This is when the program is ready to exit */
    g_thread_join(lb_thread);

    /* Free up our memory */
    g_mutex_free(wbmutex);
    free(gladeFile); 
    g_slice_free(guiPointerStruct, ptrList);
    g_slice_free(argstruct, args);
    g_list_foreach(messageList, (GFunc)inner_g_list_free, NULL);
    g_list_free(messageList);
    g_list_free(filterList);
    return( 0 );
}

/******************************************************************************
 * Function Name: inner_g_list_free
 *      
 * Description: This function gets called when we need to free our GLists
 *
 * Inputs: void pointers
 *
 * Returns: None
 *
 *****************************************************************************/
void inner_g_list_free(const void *a, const void *b)
{
    g_free((void *)a);
}

/******************************************************************************
 * Function Name: set_global_pointers
 *
 * Description: This function sets all of our pointers in the ptrList struct.
 *              We initiate the lock in this function.  The GladeXMl pointer
 *              must still be active in this function.
 *
 * Inputs: GladeXML* - A pointer to the glade xml file so we can lookup 
 *                     the pointers.
 *
 * Returns: None
 *
 *****************************************************************************/
void set_global_pointers(GladeXML *builder)
{
    g_mutex_lock(wbmutex);
    ptrList->message_list_treeview = GTK_TREE_VIEW(glade_xml_get_widget(builder, "message_list_treeview"));
    ptrList->constant_treeview = GTK_TREE_VIEW(glade_xml_get_widget(builder, "constant_treeview"));
    ptrList->main_message_display_textview = 
        GTK_TEXT_VIEW(glade_xml_get_widget(builder, "main_message_display_textview"));
    ptrList->available_filters_treeview = GTK_TREE_VIEW(glade_xml_get_widget(builder, "available_filters_treeview"));  
    ptrList->current_filters_treeview = GTK_TREE_VIEW(glade_xml_get_widget(builder, "current_filters_treeview")); 
    ptrList->log_size_spinbutton = GTK_SPIN_BUTTON(glade_xml_get_widget(builder, "log_size_spinbutton"));
    ptrList->verbosity_spinbutton = GTK_SPIN_BUTTON(glade_xml_get_widget(builder, "verbosity_spinbutton"));
    ptrList->scroll_radiobutton = GTK_RADIO_BUTTON(glade_xml_get_widget(builder, "scroll_radiobutton"));
    ptrList->pause_radiobutton = GTK_RADIO_BUTTON(glade_xml_get_widget(builder, "pause_radiobutton"));
    ptrList->freeze_radiobutton = GTK_RADIO_BUTTON(glade_xml_get_widget(builder, "freeze_radiobutton"));
    ptrList->toExit = 0;
    ptrList->toFreeze = 0;
    g_mutex_unlock(wbmutex);
}

/******************************************************************************
 * Function Name: get_int_val_in_col_from_tree_view
 *
 * Description: This function gets a value from a treeview for a particular
 *              column.  It is a generic function, and is only useful for 
 *              columns in which you know you are retrieving ints. A 
 *              row must be selected for this to work.
 *
 * Inputs: 
 *      GtkTreeView * - A pointer to a particular tree view
 *      int - The column number that we are getting (0 based)
 *
 * Returns:
 *      int - The value of the cell
 *
 *****************************************************************************/
int get_int_val_in_col_from_tree_view(GtkTreeView *tree_view, int col)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeSelection *selection;
    selection = gtk_tree_view_get_selection(tree_view);
    int myVal = -1;
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
        gtk_tree_model_get(model, &iter, col, &myVal, -1);
    return myVal;
}

/******************************************************************************
 * Function Name: get_string_in_col_from_tree_view
 *
 * Description: This function gets the value of a particular column in a 
 *              tree view.  A row must be selected for this to work.
 *              You must also be sure that the column is a string.
 *
 * Inputs: 
 *      GtkTreeView* - The tree view that we are retrieving 
 *      int - The column number that we are getting
 *      char** - A pointer to a char* buffer.  This is essentially what 
 *                  we are returning.
 *
 * Returns: 
 *      int - -1 for failure, 1 for success 
 *
 *****************************************************************************/
int get_string_in_col_from_tree_view(GtkTreeView *tree_view, int col, char** buf)
{ 
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeSelection *selection;
    selection = gtk_tree_view_get_selection(tree_view);
    gchar* myVal = NULL;
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
        gtk_tree_model_get(model, &iter, col, &myVal, -1);
    else
    {
        g_free(myVal);
        return -1;
    }
    *buf = g_strdup(myVal);
    g_free(myVal);
    return 1;
}

/******************************************************************************
 * Function Name: map_message_type_to_color
 *
 * Description: This function maps the message type to the color being 
 *              displayed for it.  
 *
 * Inputs: int - The type of message
 *
 * Returns: char* - The color.
 *
 *****************************************************************************/
char* map_message_type_to_color(int msgTypeID)
{
    switch(msgTypeID)
    {
        case 1:
          return "pink";
        case 2:
          return "yellow";
        case 3:
          return "white";
        case 4:
          return "tan";
        case 5:
          return "deep sky blue";
        case 11:
          return "orange";
        case 13:
          return "red1";
        case 15:
          return "light green";
        case 18:
          return "green3";
        case 31:
          return "gray";
        default:
          return "gray";
    }
}

/******************************************************************************
 * Function Name: map_message_type_to_description
 *
 * Description: This function maps a message type to its description.
 *
 * Inputs: int - The message type
 *
 * Returns: char* - The description of this message type.
 *
 *****************************************************************************/
char* map_message_type_to_description(int msgTypeID)
{
    switch(msgTypeID)
    {
        case 1:
            return "Digital Radar Data";
        case 2:
            return "RDA Status Data";
        case 3:
            return "Performance/Maintenance Data";
        case 4:
            return "Console Message";
        case 5:
            return "Volume Coverage Pattern (RDA->RPG)";
        case 11:
            return "Loop Back Test (RDA->RPG)";
        case 13:
            return "Clutter Filter Bypass Map";
        case 15:
            return "Clutter Filter Notch Width Map";
        case 18:
            return "RDA Adaptation Data";
        case 31:
            return "Generic Digital Radar Data";
        default:
            return "Generic Digital Radar Data";
    }
}

/******************************************************************************
 * Function Name: add_row_to_available_filter_tree
 *
 * Description: This function adds a row to the filter tree (the top one).
 *
 * Inputs: char* - The value (or description) of this row
 *         char* - The color
 *         int - The message type
 *
 * Returns: None
 *
 *****************************************************************************/
void add_row_to_available_filter_tree(char* val, char* color, int col_id)
{
    GtkTreeView* tree_view = ptrList->available_filters_treeview;
    GtkTreeStore* tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_view));
    GtkTreeIter iter;
    gtk_tree_store_prepend(tree_store, &iter, NULL);
    gtk_tree_store_set(tree_store, &iter, 
                       FILTER_TYPE_COL, val, 
                       FILTER_COLOR_COL, color,
                       FILTER_ID_COL, col_id, -1);
}

/******************************************************************************
 * Function Name: add_row_to_current_filter_tree
 *
 * Description: This function adds a row to the current filter tree (bottom).
 *
 * Inputs: char* - The value (description) of this row
 *         char* - The color of this row
 *         int - The message type
 *
 * Returns: None
 *
 *****************************************************************************/
void add_row_to_current_filter_tree(char* val, char* color, int col_id)
{
    GtkTreeView* tree_view = ptrList->current_filters_treeview;
    GtkTreeStore* tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_view));
    GtkTreeIter iter;
    gtk_tree_store_prepend(tree_store, &iter, NULL);
    gtk_tree_store_set(tree_store, &iter, 
                       FILTER_TYPE_COL, val, 
                       FILTER_COLOR_COL, color,
                       FILTER_ID_COL, col_id, -1);
}

/******************************************************************************
 * Function Name: add_row_to_constant_tree
 *
 * Description: This function adds a row to the constant tree view.  
 *
 * Inputs: 
 *      GtkWidget* - This is a valid, registered widget
 *      char* - The field (description) in the left column
 *      char* - The value in the right column
 *      char* - The color of the row.
 *
 * Returns: None
 *
 *****************************************************************************/
void add_row_to_constant_tree(GtkWidget* widget, char* field, char* value, char* color)
{
    GladeXML *self = glade_get_widget_tree(widget);
    GtkTreeView* tree_view = GTK_TREE_VIEW(glade_xml_get_widget(self, "constant_treeview"));
    GtkTreeStore* tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_view));
    GtkTreeIter iter;
    gtk_tree_store_prepend(tree_store, &iter, NULL);
    gtk_tree_store_set(tree_store, &iter, 
                       CONST_FIELD_COL, field, 
                       CONST_VALUE_COL, value, 
                       CONST_COLOR_COL, color, -1);
}

/******************************************************************************
 * Function Name: add_top_filter_renderer
 *
 * Description: This function adds the renderer for the "available" filters, 
 *              or the top window in the Options tab. 
 *
 * Inputs: GladeXML* - A GladeXML pointer that has sucked in our glade file.
 *
 * Returns: None
 *
 *****************************************************************************/
void add_top_filter_renderer(GladeXML *builder)
{
    GtkTreeView *tree_view;
    GtkTreeStore *tree_store;
    GtkCellRenderer *cell_renderer;

    tree_view = GTK_TREE_VIEW(glade_xml_get_widget(builder, "available_filters_treeview"));
    cell_renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree_view, 
            FILTER_TYPE_COL, "PACKET TYPES CURRENTLY SHOWING", cell_renderer, "text", 
            FILTER_TYPE_COL, "background", FILTER_COLOR_COL, NULL);
    tree_store = gtk_tree_store_new(FILTER_NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(tree_store));
}

/******************************************************************************
 * Function Name: add_bottom_filter_renderer
 *
 * Description: This function adds the bottom (current filters) renderer. 
 *
 * Inputs: GladeXML* - A GladeXML pointer that has sucked in our glade file.
 *
 * Returns: None
 *
 *****************************************************************************/
void add_bottom_filter_renderer(GladeXML *builder)
{
    GtkTreeView *tree_view;
    GtkTreeStore *tree_store;
    GtkCellRenderer *cell_renderer;

    tree_view = GTK_TREE_VIEW(glade_xml_get_widget(builder, "current_filters_treeview"));
    cell_renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree_view, 
            FILTER_TYPE_COL, "PACKET TYPES CURRENTLY FILTERED OUT", cell_renderer, "text", 
            FILTER_TYPE_COL, "background", FILTER_COLOR_COL, NULL);
    tree_store = gtk_tree_store_new(FILTER_NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(tree_store));
}

/******************************************************************************
 * Function Name: add_constants_renderer
 *
 * Description: This function adds the constants renderer (the area on 
 *              the top left of the main window). 
 *
 * Inputs: GladeXML* - A GladeXML pointer that has sucked in our glade file.
 *
 * Returns: None
 *
 *****************************************************************************/
void add_constants_renderer(GladeXML *builder)
{
    GtkTreeView *tree_view;
    GtkTreeStore *tree_store;
    GtkCellRenderer *cell_renderer;

    tree_view = GTK_TREE_VIEW(glade_xml_get_widget(builder, "constant_treeview"));
    cell_renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tree_view, 
            CONST_FIELD_COL, "FIELD", cell_renderer, "text", 
            CONST_FIELD_COL, "background", CONST_COLOR_COL, NULL);
    gtk_tree_view_insert_column_with_attributes(tree_view, 
            CONST_VALUE_COL, "VALUE", cell_renderer, "text", 
            CONST_VALUE_COL, "background", CONST_COLOR_COL, NULL);
    tree_store = gtk_tree_store_new(CONST_NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(tree_store));
}

/******************************************************************************
 * Function Name: add_message_list_renderer
 *
 * Description: This function adds the renderer of the message list.
 *
 * Inputs: GladeXML* - A GladeXML pointer that has sucked in our glade file.
 *
 * Returns: None
 *
 *****************************************************************************/
void add_message_list_renderer(GladeXML *builder)
{
    GtkTreeView *tree_view;
    GtkTreeStore *tree_store;
    GtkCellRenderer *cell_renderer;

    tree_view = GTK_TREE_VIEW(glade_xml_get_widget(builder, "message_list_treeview"));
    cell_renderer = gtk_cell_renderer_text_new();
    
    gtk_tree_view_insert_column_with_attributes(tree_view, 
                MSG_LIST_TIME_COL, "TIME", cell_renderer, "text", 
                MSG_LIST_TIME_COL, "background", MSG_LIST_COLOR_COL, NULL);
    gtk_tree_view_insert_column_with_attributes(tree_view, 
                MSG_LIST_MSG_ID_COL, "MESSAGE ID", cell_renderer, "text", 
                MSG_LIST_MSG_ID_COL, "background", MSG_LIST_COLOR_COL, NULL);
    gtk_tree_view_insert_column_with_attributes(tree_view, 
                MSG_LIST_MSG_COL, "MESSAGE", cell_renderer, "text", 
                MSG_LIST_MSG_COL, "background", MSG_LIST_COLOR_COL, NULL);

    tree_store = gtk_tree_store_new(MSG_LIST_NUM_COLS, G_TYPE_STRING, 
                G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(tree_view, GTK_TREE_MODEL(tree_store));
}

/******************************************************************************
 * Function Name: get_selected_indice
 *
 * Description: This function is pretty useful.  It gets the row index of 
 *              the selected row in a tree view. 
 *
 * Inputs: GtkTreeView* - The tree view we are looking at
 *
 * Returns:
 *      int - The index.  -1 if failure.
 *
 *****************************************************************************/
int get_selected_indice(GtkTreeView* tree_view)
{
    GtkTreePath* path;
    GtkTreeModel* model;
    GtkTreeIter iter;
    gint row = -1;
    model = gtk_tree_view_get_model(tree_view);

    if (!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(tree_view), NULL, &iter))
        return -1;
    path = gtk_tree_model_get_path(model, &iter);
    row = gtk_tree_path_get_indices(path)[0];
    return (int)row;
}

/******************************************************************************
 * Function Name: set_verbose_level
 *
 * Description: A wrapper for the _set_verbose_level function. 
 *
 * Inputs: int - The verbose level
 *
 * Returns: None
 *
 *****************************************************************************/
void set_verbose_level(int verb)
{
    _set_verbose_level(verb);
}

/******************************************************************************
 * Function Name: get_verbose_level
 *
 * Description: A wrapper for the _get_verbose_level function.
 *
 * Inputs: None
 *
 * Returns: int - The verbose level
 *
 *****************************************************************************/
int get_verbose_level()
{
    return _get_verbose_level();
}

/******************************************************************************
 * Function Name: update_verbosity_spinbutton
 *
 * Description: This function updates the verbosity spinbutton with 
 *              the verbosity level.  It should only be called on startup, 
 *              only if the user has passed in command line arguments that
 *              set the verbosity level. 
 *
 * Inputs: int - The verbosity level
 *
 * Returns: None
 *
 *****************************************************************************/
void update_verbosity_spinbutton(int level)
{
    gtk_spin_button_set_value(ptrList->verbosity_spinbutton, (gdouble)level);
}

/******************************************************************************
 * Function Name: updateIntConstantColumn
 *
 * Description: This function updates a row in the constants area (top left), 
 *              but it is only for integer values.  This function attempts
 *              to get rid of leading 0's by manipulating the length given
 *              to sprintf. 
 *
 * Inputs: int - The index (row number)
 *         int - The value
 *
 * Returns: None
 *
 *****************************************************************************/
void updateIntConstantColumn(int index, int value)
{
    int length = 2;
    if (index == 0)
    {
        if (value > 99)
            length = 3;
    }
    else
        if (value < 100)
            length = 1;

    if (index == 5)
    {
        if (value > 99999)
            length = 6;
        else if (value > 9999)
            length = 5;
        else if (value > 999)
            length = 4;
        else if (value > 999)
            length = 3;
    }
    char buf[length + 1];
    sprintf(buf, "%0*d", length, value);
    buf[length] = '\0';
    GtkTreeIter iter;
    gdk_threads_enter();
    g_mutex_lock(wbmutex);
    gtk_tree_model_iter_nth_child(gtk_tree_view_get_model(ptrList->constant_treeview), &iter, NULL, index);
    gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(ptrList->constant_treeview)), &iter, 
                       CONST_VALUE_COL, buf, -1);
    g_mutex_unlock(wbmutex);
    gdk_threads_leave();
}

/******************************************************************************
 * Function Name: updateFloatConstantColumn
 *
 * Description: This function updates a row in the constants area (top left), 
 *              but it is only for float values.  This function attempts
 *              to get rid of leading 0's by manipulating the length given
 *              to sprintf. 
 *
 * Inputs: int - The index (row number)
 *         float - The value
 *
 * Returns: None
 *
 *****************************************************************************/
void updateFloatConstantColumn(int index, float value)
{
    GtkTreeIter iter;
    char buf[7];
    sprintf(buf, "%3.2f", value);
    buf[6] = '\0';
    gdk_threads_enter();
    g_mutex_lock(wbmutex);
    gtk_tree_model_iter_nth_child(gtk_tree_view_get_model(ptrList->constant_treeview), &iter, NULL, index);
    gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(ptrList->constant_treeview)), &iter, 
                       CONST_VALUE_COL, buf, -1);
    g_mutex_unlock(wbmutex);
    gdk_threads_leave();
}

/******************************************************************************
 * Function Name: updateStringConstantColumn
 *
 * Description: This function updates a row in the constants area (top left), 
 *              but it is only for string values.  This function attempts
 *              to get rid of leading 0's by manipulating the length given
 *              to sprintf.
 *
 * Inputs: int - The index (row number)
 *         char* - The value
 *
 * Returns: None
 *
 *****************************************************************************/
void updateStringConstantColumn(int index, char* value)
{
    GtkTreeIter iter;
    gdk_threads_enter();
    g_mutex_lock(wbmutex);
    gtk_tree_model_iter_nth_child(gtk_tree_view_get_model(ptrList->constant_treeview), &iter, NULL, index);
    gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(ptrList->constant_treeview)), &iter, 
                       CONST_VALUE_COL, value, -1);
    g_mutex_unlock(wbmutex);
    gdk_threads_leave();
}

/******************************************************************************
 * Function Name: update_textfield
 *
 * Description: This function updates the text in the main text field.  
 *              It is also responsible for showing any popup windows.
 *
 * Inputs: char* - The text to display
 *
 * Returns: None
 *
 *****************************************************************************/
void update_textfield(char* txt)
{
    /* We need this, or we get problems if an LB is corrupted. */
    if (!g_utf8_validate(txt, strlen(txt), NULL))
        return;
    GtkTextBuffer *buffer;

    /* If we are supposed to have a popup window, we need to 
     * create that too. */
    if (popup_txtview != NULL)
    {
        buffer = gtk_text_view_get_buffer(popup_txtview);
        gtk_text_buffer_set_text(buffer, txt, -1);
        gtk_widget_show(popup_txtwindow);
        popup_txtview = NULL;
        popup_txtwindow = NULL;
    }
    GtkTextView *text_view;
    text_view = GTK_TEXT_VIEW(ptrList->main_message_display_textview);
    buffer = gtk_text_view_get_buffer(text_view);
    gtk_text_buffer_set_text(buffer, txt, -1);
}

/******************************************************************************
 * Function Name: initiate_constant_columns
 *
 * Description: This initiates the constant values.  
 *
 * Inputs: GtkWidget* - A registered widget
 *
 * Returns: None
 *
 *****************************************************************************/
void initiate_constant_columns(GtkWidget* widget)
{
    add_row_to_constant_tree(widget, "Elevation #", "0", "gray"); /*index 6 */
    add_row_to_constant_tree(widget, "SR", "0", "white"); /*index 5 */
    add_row_to_constant_tree(widget, "Moments Enabled", "0", "gray"); /*index 4 */
    add_row_to_constant_tree(widget, "Wave Form", "0", "white"); /*index 3 */
    add_row_to_constant_tree(widget, "Collection Time", "0", "gray"); /*index 2 */
    add_row_to_constant_tree(widget, "Elevation Angle", "0", "white"); /*index 1 */
    add_row_to_constant_tree(widget, "VCP", "0", "gray"); /*index 0 */
}

/******************************************************************************
 * Function Name: initiate_available_filters_columns
 *
 * Description: This function initiates all of the filter rows that are 
 *              supposed to go into the "available" filters area.
 *
 * Inputs: None
 *
 * Returns: None
 *
 *****************************************************************************/
void initiate_available_filters_columns()
{
    add_row_to_available_filter_tree(map_message_type_to_description(1), map_message_type_to_color(1), 1);
    add_row_to_available_filter_tree(map_message_type_to_description(2), map_message_type_to_color(2), 2);
    add_row_to_available_filter_tree(map_message_type_to_description(3), map_message_type_to_color(3), 3);
    add_row_to_available_filter_tree(map_message_type_to_description(4), map_message_type_to_color(4), 4);
    add_row_to_available_filter_tree(map_message_type_to_description(5), map_message_type_to_color(5), 5);
    add_row_to_available_filter_tree(map_message_type_to_description(11), map_message_type_to_color(11), 11);
    add_row_to_available_filter_tree(map_message_type_to_description(13), map_message_type_to_color(13), 13);
    add_row_to_available_filter_tree(map_message_type_to_description(15), map_message_type_to_color(15), 15);
    add_row_to_available_filter_tree(map_message_type_to_description(18), map_message_type_to_color(18), 18);
    add_row_to_available_filter_tree(map_message_type_to_description(31), map_message_type_to_color(31), 31);
}

/******************************************************************************
 * Function Name: remove_first_row_from_tree_view
 *
 * Description: This function simply removes the first row from a tree view.
 *
 * Inputs: GtkTreeView* - The tree view that we are manipulating.
 *
 * Returns: None
 *
 *****************************************************************************/
void remove_first_row_from_tree_view(GtkTreeView* tree_view)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    gtk_tree_model_get_iter_first(model, &iter);
    gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
}

/******************************************************************************
 * Function Name: remove_last_row_from_tree_view
 *
 * Description: This function simply removes the last row from a tree view.
 *
 * Inputs: GtkTreeView* - The tree view that we are manipulating.
 *
 * Returns: None
 *
 *****************************************************************************/
void remove_last_row_from_tree_view(GtkTreeView* tree_view)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    gtk_tree_model_iter_nth_child(model, &iter, NULL, gtk_tree_model_iter_n_children(model, NULL) - 1);
    gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
}

/******************************************************************************
 * Function Name: add_row_to_message_list_tree
 *
 * Description: This function adds a row to the message list tree. 
 *              It double checks the length of the tree view. If the length
 *              is too big, it will remove a row.
 *
 * Inputs: 
 *      GtkTreeView* - The tree view that we are adding to (message list)
 *      char* - The time to go in the left column
 *      int - The message id. It will go in the second column.
 *      char* - This will be the label, or what goes in the 3rd column.
 *      int - The message type. This value will be hidden.
 *      int - The message length.  This value will be hidden.
 *      int - The order. 0 means we prepend values to the beginngin, 1 
 *              means we append to the end.
 *
 * Returns: None
 *
 *****************************************************************************/
void add_row_to_message_list_tree(GtkTreeView* tree_view, char* time, int id, 
    char* label, int msgTypeID, int messageLen, int order)
{
    GtkTreeStore* tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(tree_view));
    GtkTreeIter iter;
    if (order > 0)
    {
        if (gtk_tree_model_iter_n_children(gtk_tree_view_get_model(tree_view), NULL) >= bufferSize)
            remove_first_row_from_tree_view(tree_view);
        gtk_tree_store_append(tree_store, &iter, NULL);
    }
    else
    {
        if (gtk_tree_model_iter_n_children(gtk_tree_view_get_model(tree_view), NULL) >= bufferSize)
            remove_last_row_from_tree_view(tree_view);
        gtk_tree_store_prepend(tree_store, &iter, NULL);
    }
    gtk_tree_store_set(tree_store, &iter, 
                       MSG_LIST_TIME_COL, time, 
                       MSG_LIST_MSG_ID_COL, id, 
                       MSG_LIST_MSG_COL, label, 
                       MSG_LIST_COLOR_COL, map_message_type_to_color(msgTypeID), 
                       MSG_LIST_MSG_TYPE_ID_COL, msgTypeID, 
                       MSG_LIST_MSG_READ_LEN, messageLen, -1);
}

/******************************************************************************
 * Function Name: add_packet_to_list
 *
 * Description: This function adds a packet to the list.  It is different than
 *              the previous function because it handles our internal GList
 *              as well.  If the size is too big, it will remove a link.
 *
 * Inputs: 
 *      char* - The packet (raw data)
 *      char* - The label message.  This will be what we see in the message
 *                  list.
 *      char* - The time.
 *      int - The message ID.
 *      int - The message length.
 *      int - The order. 0 means we prepend, 1 means we append.
 *
 * Returns: None
 *
 *****************************************************************************/
void add_packet_to_list(char* packet, char* labelMsg, char* disp, int msgID, int messageType, int msgLen, int order)
{
    if (is_message_type_in_filter(messageType))
        return;

    char* space;
    space = malloc(msgLen);
    memcpy(space, packet, msgLen);
    if (order > 0)
    {
        /* Make sure our length is right */
        if (gtk_tree_model_iter_n_children(gtk_tree_view_get_model(ptrList->message_list_treeview), NULL) >= bufferSize)
        {
            free(g_list_nth_data(messageList, 0));
            messageList = g_list_delete_link(messageList, g_list_first(messageList));
        }
        messageList = g_list_append(messageList, space);
    }
    else
    {
        /* Make sure our length is right */
        if (gtk_tree_model_iter_n_children(gtk_tree_view_get_model(ptrList->message_list_treeview), NULL) >= bufferSize)
        {
            free(g_list_nth_data(messageList, g_list_length(messageList) - 1));
            messageList = g_list_delete_link(messageList, g_list_last(messageList));
        }
        messageList = g_list_prepend(messageList, space);
    }

    add_row_to_message_list_tree(ptrList->message_list_treeview, 
                                 disp, msgID, labelMsg, messageType, msgLen, order);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ptrList->scroll_radiobutton)))
        gtk_tree_view_scroll_to_point(ptrList->message_list_treeview, 0, 0);

    gdouble min, max;
    gtk_spin_button_get_range(ptrList->log_size_spinbutton, &min, &max);
    if ((min == gtk_spin_button_get_value_as_int(ptrList->log_size_spinbutton)) && (min > 0))
        gtk_spin_button_set_range(ptrList->log_size_spinbutton, gtk_tree_model_iter_n_children(gtk_tree_view_get_model(ptrList->message_list_treeview), NULL) - 1, max);
    else
        gtk_spin_button_set_range(ptrList->log_size_spinbutton, gtk_tree_model_iter_n_children(gtk_tree_view_get_model(ptrList->message_list_treeview), NULL), max);
}

/******************************************************************************
 * Function Name: add_filter_to_list
 *
 * Description: This function looks at the row that has been selected in the
 *              top area, and it adds it to the bottom row.
 *
 * Inputs: - None
 *
 * Returns: None
 *
 *****************************************************************************/
void add_filter_to_list()
{
    GtkTreeIter iter;
    GtkTreeSelection *select;
    GtkTreeModel *tree_model;
    GValue myVal = {0};
    int selectedType = 0;

    /* Get the selection in the top area */
    tree_model = gtk_tree_view_get_model(ptrList->available_filters_treeview);
    select = gtk_tree_view_get_selection(ptrList->available_filters_treeview);
    if (!gtk_tree_selection_get_selected(select, NULL, &iter))
        return;
    gtk_tree_model_get_value(tree_model, &iter, FILTER_ID_COL, &myVal);
    selectedType = g_value_get_int(&myVal);
    g_value_unset(&myVal);

    /* Remove it from the top area */
    gtk_tree_store_remove(GTK_TREE_STORE(tree_model), &iter);   

    /* Add it to the bottom area */
    add_row_to_current_filter_tree(map_message_type_to_description(selectedType), 
        map_message_type_to_color(selectedType), selectedType);
    add_element_to_filter_list(selectedType);
    remove_packet_type_from_message_list(selectedType);
}

/******************************************************************************
 * Function Name: remove_filter_from_list
 *
 * Description: This function removes a row from the bottom area and adds
 *              it to the top. 
 *
 * Inputs: None
 *
 * Returns: None
 *
 *****************************************************************************/
void remove_filter_from_list()
{
    GtkTreeIter iter;
    GtkTreeSelection *select = NULL;
    GtkTreeModel *tree_model;
    GValue myVal = {0};
    int selectedType = 0;
    tree_model = gtk_tree_view_get_model(ptrList->current_filters_treeview);
    select = gtk_tree_view_get_selection(ptrList->current_filters_treeview);
    if (!gtk_tree_selection_get_selected(select, NULL, &iter))
        return;
    gtk_tree_model_get_value(tree_model, &iter, FILTER_ID_COL, &myVal);
    selectedType = g_value_get_int(&myVal);
    g_value_unset(&myVal);
    gtk_tree_store_remove(GTK_TREE_STORE(tree_model), &iter);   
    add_row_to_available_filter_tree(map_message_type_to_description(selectedType), 
        map_message_type_to_color(selectedType), selectedType);
    remove_element_from_filter_list(selectedType);
}

/******************************************************************************
 * Function Name: add_element_to_filter_list
 *
 * Description: This function adds an element to the filter list Glist. 
 *              This GList will let us know what packet types we are
 *              supposed to filter out.
 *
 * Inputs: int - The filter number/message type
 *
 * Returns: None
 *
 *****************************************************************************/
void add_element_to_filter_list(int filterNum)
{
    filterList = g_list_prepend(filterList, (gpointer)filterNum);
}

/******************************************************************************
 * Function Name: remove_element_from_filter_list
 *
 * Description: This function removes a link from the GList.
 *
 * Inputs: int - The filter number/message type
 *
 * Returns: None
 *
 *****************************************************************************/
void remove_element_from_filter_list(int filterNum)
{
    filterList = g_list_remove(filterList, (gpointer)filterNum);
}

/******************************************************************************
 * Function Name: is_message_type_in_filter
 *
 * Description: This function checks to see if a message type is in the 
 *              GList.  This isn't really the most optimal way of doing this, 
 *              but the list is so small that it's not a big deal.
 *
 * Inputs: int - The message type (what will be the gpointer value)
 *
 * Returns: int - 1 if it exists, 0 if it doesnt
 *
 *****************************************************************************/
int is_message_type_in_filter(int msgType)
{
    int i;
    for (i = 0; i < g_list_length(filterList); i++)
        if ((int)g_list_nth_data(filterList, i) == msgType)
            return 1;
    return 0;
}

/******************************************************************************
 * Function Name: remove_packet_type_from_message_list
 *
 * Description: This function removes a packet type from the message list.  
 *              This is the function that essentially does our filtering.  
 *              Once packets are removed, they are lost forever. 
 *
 * Inputs: int - The message type that we are filtering out.
 *
 * Returns: None
 *
 *****************************************************************************/
void remove_packet_type_from_message_list(int msgType)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(ptrList->message_list_treeview);
    int counter, temp;
    counter = 0;
    GValue val = {0};
    GList *refList = NULL;

    /* Make sure we have stuff to filter */
    if (gtk_tree_model_get_iter_first(model, &iter))
        do
        {
            gtk_tree_model_get_value(model, &iter, MSG_LIST_MSG_TYPE_ID_COL, &val);
            temp = g_value_get_int(&val);
            g_value_unset(&val);
            /* If the message type is the same as our filter value */
            if (temp == msgType)
                refList = g_list_prepend(refList, (gpointer)counter);
            counter++;
        } while (gtk_tree_model_iter_next(model, &iter));


    while (refList)
    {
        gtk_tree_model_iter_nth_child(model, &iter, NULL, (int)refList->data);
        gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
        messageList = g_list_delete_link(messageList, g_list_nth(messageList, counter));
        refList = g_list_next(refList);
    }
    g_list_free(refList);
}

/******************************************************************************
 * Function Name: set_popup_window
 *
 * Description: This function sets the popup window pointers.  This lets 
 *              us know that we have something to show.
 *
 * Inputs: GtkTextView* - The pointer to the newly created text view
 *         GtkWidget* - The window that we can show.
 *
 * Returns: None
 *
 *****************************************************************************/
void set_popup_window(GtkTextView* txtView, GtkWidget* txtwindow)
{
    popup_txtview = txtView;
    popup_txtwindow = txtwindow;
}

