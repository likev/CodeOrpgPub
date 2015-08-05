/*
 * RCS info
 * $Author: jclose $
 * $Locker:  $
 * $Date: 2009/06/29 16:02:05 $
 * $Id: mon_wb_gui_callbacks.c,v 1.1 2009/06/29 16:02:05 jclose Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include "mon_wb_gui_callbacks.h"
#include "mon_wb_gui.h"

/******************************************************************************
 * Function Name: on_scroll_radio_button_toggled
 *
 * Description: This function gets called when the "scroll" radio button 
 *              gets clicked.  The important thing is to focus the list 
 *              on the top row, and make sure to unset our "Freeze".
 *
 *****************************************************************************/
void
on_scroll_radiobutton_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    g_mutex_lock(wbmutex);
    gtk_tree_view_scroll_to_point(ptrList->message_list_treeview, 0, 0);
    ptrList->toFreeze = 0;
    g_mutex_unlock(wbmutex);
}

/******************************************************************************
 * Function Name: on_pause_radiobutton_toggled
 *
 * Description: This function gets called when the user clicks the 
 *              "Pause" radio button.  This function actually doesn't change 
 *              anything, but it does need to make sure to set our toFreeze
 *              variable back to 0.
 *
 *****************************************************************************/
void
on_pause_radiobutton_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    g_mutex_lock(wbmutex);
    ptrList->toFreeze = 0;
    g_mutex_unlock(wbmutex);
}

/******************************************************************************
 * Function Name: on_freeze_radiobutton_toggled
 *
 * Description: Gets called when the user clicks the freeze button.  We need 
 *              to set the appropriate variable.
 *
 *****************************************************************************/
void
on_freeze_radiobutton_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    g_mutex_lock(wbmutex);
    ptrList->toFreeze = 1;
    g_mutex_unlock(wbmutex);
}

/******************************************************************************
 * Function Name: on_push_down_button_clicked            
 *
 * Description: This function gets called when the user clicks the down arrow
 *              on the options tab.  It will subsequently remove the row 
 *              from the top and add it to the bottom.
 *
 *****************************************************************************/
void
on_push_down_button_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
    g_mutex_lock(wbmutex);
    add_filter_to_list();
    g_mutex_unlock(wbmutex);
}


/******************************************************************************
 * Function Name: on_push_up_button_clicked              
 *
 * Description: This function gets called when the user clicks the up arrow 
 *              in the options tab.  It will remove the row from the bottom
 *              area and add it back to the top area.
 *
 *****************************************************************************/
void
on_push_up_button_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
    g_mutex_lock(wbmutex);
    remove_filter_from_list();
    g_mutex_unlock(wbmutex);
}

/******************************************************************************
 * Function Name: on_verbosity_spinbutton_changed
 *
 * Description: This function gets called when the user changes the verbosity
 *              value.  We just need to get the value and then pass it to the 
 *              other thread. 
 *
 *****************************************************************************/
void
on_verbosity_spinbutton_changed        (GtkEditable     *editable,
                                        gpointer         user_data)
{
    GtkSpinButton *spin = GTK_SPIN_BUTTON(editable);
    int val = gtk_spin_button_get_value(spin);
    g_mutex_lock(wbmutex);
    set_verbose_level(val);
    g_mutex_unlock(wbmutex);
}


/******************************************************************************
 * Function Name: on_log_size_spinbutton_changed         
 *
 * Description: This function gets called when the log size toggle button gets
 *              changed.  It is a little tricky though, because if the user 
 *              gets crazy and starts clicking the down arrow, making the
 *              values go by too fast, then the actual value can change here
 *              too quickly, causing problems later on.  So as a rule, we only
 *              let the user change the value to the size of the current number
 *              of rows in the message tree - 1.
 *
 *****************************************************************************/
void
on_log_size_spinbutton_changed         (GtkEditable     *editable,
                                        gpointer         user_data)
{
    g_mutex_lock(wbmutex);
    int temp = gtk_spin_button_get_value(GTK_SPIN_BUTTON(editable));
    while (temp <= gtk_tree_model_iter_n_children(gtk_tree_view_get_model(ptrList->message_list_treeview), NULL) + 1)
    {
        free(g_list_nth_data(messageList, g_list_length(messageList) - 1));
        messageList = g_list_delete_link(messageList, g_list_last(messageList));
        remove_last_row_from_tree_view(ptrList->message_list_treeview);
    }
    bufferSize = gtk_spin_button_get_value(GTK_SPIN_BUTTON(editable));
    g_mutex_unlock(wbmutex);
}

/******************************************************************************
 * Function Name: on_message_list_treeview_button_press_event
 *
 * Description: This function handles our clicks in the main message list 
 *              area.  If the user double clicks a row, we are going to popup  
 *              a new window with the values.  This will allow the user to 
 *              compare different packets without having to constantly be 
 *              clicking.
 *
 *****************************************************************************/
gboolean
on_message_list_treeview_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    /* Make sure it's a double click */
    if (event->type == GDK_2BUTTON_PRESS)
    {
        unsigned int msgID = 0;
        int msgLen = 0;
        int rowNum = -1;
        char* msgBuf = NULL;
        
        /* Lock it down */
        g_mutex_lock(wbmutex);
        rowNum = get_selected_indice(ptrList->message_list_treeview);
        msgID = get_int_val_in_col_from_tree_view(ptrList->message_list_treeview, 1);
        msgLen = get_int_val_in_col_from_tree_view(ptrList->message_list_treeview, 5);

        /* If the "scroll" button is clicked, move the list back to the top */
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ptrList->scroll_radiobutton)))
            gtk_tree_view_scroll_to_point(ptrList->message_list_treeview, 0, 0);
        if (rowNum >= 0)
        {
            msgBuf = (char *)malloc(msgLen);

            /* Get the data from the GList */
            if (msgBuf != NULL)
                memcpy(msgBuf, g_list_nth_data(messageList, rowNum), msgLen);
            else
            {
                fprintf(stderr, "There was a problem with malloc.  msgLen = %d\n", msgLen);
                g_mutex_unlock(wbmutex);
                return FALSE;
            }
        }
        else
        {
            g_mutex_unlock(wbmutex);
            return FALSE;
        }

        /* Pass the data to the Wrapper */
        if (msgID > 0)
            ProcessReadRequestWrapper(msgBuf, msgLen, msgID);
        g_mutex_unlock(wbmutex);
        free(msgBuf);

        /* Popup a new window */
        /* Get the CFG_DIR where the xml is located */
        char* cfgdir = NULL;
        char* gladeFile = NULL;
        cfgdir = getenv("CFG_DIR");
        if (cfgdir == NULL)
        {
            fprintf(stderr, " * Could not find mon_wb_gui.glade file. Please check CFG_DIR environmental variable.\n");
            return FALSE;
        }
        gladeFile = g_strdup_printf("%s/%s", cfgdir, "mon_wb_gui.xml");

        GladeXML* builder;
        GtkWidget* new_window;
        GtkTextView* txt_view;
        builder = glade_xml_new(gladeFile, "popup_window", NULL);

        if (builder == NULL)
        {
            free(gladeFile);
            return FALSE;
        }

        glade_xml_signal_autoconnect(builder);
        new_window = glade_xml_get_widget(builder, "popup_window");
        txt_view = GTK_TEXT_VIEW(glade_xml_get_widget(builder, "popup_window_textfield"));
        g_object_unref(G_OBJECT(builder));
        set_popup_window(txt_view, new_window);
        g_free(gladeFile);
    }
    return FALSE;
}

/******************************************************************************
 * Function Name: on_message_list_treeview_cursor_changed
 *
 * Description: This function gets called when the user single clicks on a 
 *              row in the message list treeview.  It does the same thing
 *              as the above function, but only updates the main text view.
 *
 *****************************************************************************/
void
on_message_list_treeview_cursor_changed
                                        (GtkTreeView     *treeview,
                                         gpointer         user_data)
{
    unsigned int msgID = 0;
    int msgLen = 0;
    int rowNum = -1;
    char* msgBuf = NULL;
     
    g_mutex_lock(wbmutex);
    /* Get these values from the global variable */
    rowNum = get_selected_indice(ptrList->message_list_treeview);
    msgID = get_int_val_in_col_from_tree_view(ptrList->message_list_treeview, 1);
    msgLen = get_int_val_in_col_from_tree_view(ptrList->message_list_treeview, 5);
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ptrList->scroll_radiobutton)))
        gtk_tree_view_scroll_to_point(ptrList->message_list_treeview, 0, 0);
    if (rowNum >= 0)
    {
        msgBuf = (char *)malloc(msgLen);
        /* Copy this data so we can pass it to the lb thread. */
        if (msgBuf != NULL)
            memcpy(msgBuf, g_list_nth_data(messageList, rowNum), msgLen);
        else
        {
            fprintf(stderr, "There was a problem with malloc.  msgLen = %d\n", msgLen);
            g_mutex_unlock(wbmutex);
            return;
        }
    }
    else
    {
        g_mutex_unlock(wbmutex);
        return;
    }   
    /* Pass the info to the lb thread */
    if (msgID > 0)
        ProcessReadRequestWrapper(msgBuf, msgLen, msgID);
    g_mutex_unlock(wbmutex);
    free(msgBuf);
}

/******************************************************************************
 * Function Name: on_outer_window_destroy                
 *
 * Description: This gets called when the main window is exited.  It lets
 *              the other thread know that it is time to quit.
 *
 *****************************************************************************/
void
on_outer_window_destroy                (GtkObject       *object,
                                        gpointer         user_data)
{
    g_mutex_lock(wbmutex);
    ptrList->toExit = 1;
    g_mutex_unlock(wbmutex);
    gtk_main_quit();
}

