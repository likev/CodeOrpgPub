#include <string.h>
#include "asp_view_lib.h"
#include "callbacks.h"
#include "support.h"
#include "asp_view.h"


/******************************************************************************
 *  Description:
 *      Callback for when the open button is clicked.  Brings up the 
 *      File Manager dialog box.
 *  Input:
 *      A registered button/widget
 *  Output:
 *      Brings up the file manager.
 *  Returns:
 *      Nothing
 *  Notes:
 *      Sets the selected path/file in the file_directory_text textbox.
 *  
 *****************************************************************************/
void
on_open_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new("Open File", 
                            GTK_WINDOW(lookup_widget(GTK_WIDGET(button), "outer_window")), 
                            GTK_FILE_CHOOSER_ACTION_OPEN, 
                            GTK_STOCK_CANCEL, 
                            GTK_RESPONSE_CANCEL, 
                            GTK_STOCK_OPEN, 
                            GTK_RESPONSE_ACCEPT, 
                            NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(lookup_widget(GTK_WIDGET(button), "file_directory_text")), filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

/******************************************************************************
 *  Description:
 *      Callback for when the Go button is clicked
 *  Input:
 *      Registered button/widget and user data
 *  Output:
 *      Sets the file (or files located within the directory) listed
 *      in the file_directory_text checkbox in the selected packets 
 *      list, and then calls for an update.
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
void
on_go_button_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
    clear_tree_view(GTK_WIDGET(button), "selected_packets_tree_view");
    clear_tree_filter_view(GTK_WIDGET(button), "message_tree");
    GtkWidget* temp_widget = lookup_widget(GTK_WIDGET(button), "file_directory_text");
    char* filePath = g_strdup(gtk_entry_get_text(GTK_ENTRY(temp_widget)));

    /* People will click the Go button for no reason. */
    if (strlen(filePath) < 1) 
    {
      gtk_editable_delete_text(GTK_EDITABLE(lookup_widget(GTK_WIDGET(button), "file_directory_text")), 0, -1);
      return;
    }

    /* If they enter in a directory */
    if (does_directory_exist(filePath))
    {
        GList *fileList = NULL;

        /* Look for the ASP files */
        get_asp_files_in_dir(filePath, &fileList);
        if (g_list_length(fileList) > 0)
        {
            /* Add them to the select packet list */
            add_glist_of_files_to_packet_list(GTK_WIDGET(button), fileList, filePath);
            set_all_selected_packets_to_main_window(GTK_WIDGET(button));

            /* Cleanup and delete the text */
            g_list_foreach(fileList, (GFunc)my_g_free, NULL);
            g_list_free(fileList);
            gtk_editable_delete_text(GTK_EDITABLE(lookup_widget(GTK_WIDGET(button), "file_directory_text")), 0, -1);
        }
        return;
    }

    /* If it isn't a valid file, then just exit */
    if (does_file_exist(filePath) < 1)
    {
        free(filePath);
        gtk_editable_delete_text(GTK_EDITABLE(lookup_widget(GTK_WIDGET(button), "file_directory_text")), 0, -1);
        return;
    }

    /* Rip it all apart and add it to the list */
    char *basePath;
    char *fileName;
    get_base_path(&basePath, filePath);
    get_file_name(&fileName, filePath);
    if (is_file_a_valid_product(filePath))
        add_unique_packet_to_packets_list(GTK_WIDGET(button), "file", fileName, basePath);
    set_all_selected_packets_to_main_window(GTK_WIDGET(button));
    free(filePath);
    g_free(fileName);
    g_free(basePath);
    gtk_editable_delete_text(GTK_EDITABLE(lookup_widget(GTK_WIDGET(button), "file_directory_text")), 0, -1);
}

/******************************************************************************
 *  Description:
 *      Callback for when the Message Filter button is clicked
 *  Input:
 *      Registered button/widget and user data.
 *  Output:
 *      Brings up the Filter Window
 *  Returns:
 *      Nothing
 *  Notes:
 *      Registers the selected packets tree, message tree and outer window 
 *      with the filter window.  Does this so we can properly call stuff to 
 *      update it all.
 *  
 *****************************************************************************/
void
on_message_filter_button_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
    /* We can't filter anything if there is nothing to filter */
    if (get_num_rows_in_tree_view(GTK_WIDGET(button), "message_tree") <= 0)
        return;
    GtkWidget *filter_window = _create_filter_window();
    GLADE_HOOKUP_OBJECT(filter_window, lookup_widget(GTK_WIDGET(button), "selected_packets_tree_view"), "selected_packets_tree_view");
    GLADE_HOOKUP_OBJECT(filter_window, lookup_widget(GTK_WIDGET(button), "message_tree"), "message_tree");
    GLADE_HOOKUP_OBJECT(filter_window, lookup_widget(GTK_WIDGET(button), "outer_window"), "outer_window");
    GLADE_HOOKUP_OBJECT(lookup_widget(GTK_WIDGET(button), "outer_window"), filter_window, "filter_window");
    gtk_widget_show(filter_window);

    /* Set the spin buttons according to values available in the main list */
    set_spin_buttons(GTK_WIDGET(button), filter_window);
    set_checkboxes(GTK_WIDGET(button), filter_window);
}



/******************************************************************************
 *  Description:
 *      Callback for when the clear button is clicked
 *  Input:
 *      Registered button/widget and user data
 *  Output:
 *      Clears the filters, so all of the data displayed is unfiltered.
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
void
on_clear_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
    /* Reset the filters */
    set_range_start_filter_time(-1);
    set_range_end_filter_time(-1);
    set_match_filter_time(GTK_WIDGET(button), -1);
    reset_ignore_msg_types();

    /* Delete the search text */
    GtkWidget *filter_window = lookup_widget(GTK_WIDGET(button), "filter_window");
    gtk_editable_delete_text(GTK_EDITABLE(lookup_widget(GTK_WIDGET(button), "search_text")), 0, -1);

    /* Refilter now that everything was reset */
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(button), "message_tree")))));
    if (!get_filter_window_status())
        return;

    /* If they reset it while they had the filter window open, we need to reset
     * the spin buttons. */
    set_spin_buttons(GTK_WIDGET(button), filter_window);
    set_checkboxes(GTK_WIDGET(button), filter_window);
}

/******************************************************************************
 *  Description:
 *      Callback for when the text changed in the search box.
 *  Input:
 *      Registered text field/widget and user data.
 *  Output:
 *      Filters the main data according to entered text.
 *  Returns:
 *      Nothing
 *  Notes:
 *      This causes for a case insensitive search to be performed on all
 *      data within the MESSAGE column of the main list.  This occurs at 
 *      or near real time, with no need to click a button.
 *  
 *****************************************************************************/
void
on_search_text_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{
    GtkWidget* temp_widget = lookup_widget(GTK_WIDGET(editable), "search_text");
    /* Get what they entered. */
    char* searchPhrase = g_strdup(gtk_entry_get_text(GTK_ENTRY(temp_widget)));
    /* Set the global variable */
    set_search_string(searchPhrase);
    /* Refilter */
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(editable), "message_tree")))));
    g_free(searchPhrase);
}

/******************************************************************************
 *  Description:
 *      Callback for when the Radar combo has changed.
 *  Input:
 *      Registered combobox/widget and user data.
 *  Output:
 *      Causes the Start and End combos to be updated with files related to 
 *      this radar.
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
void
on_radar_combo_changed                 (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int activeIter = gtk_combo_box_get_active(combobox);
    char *radarName;
    if (activeIter < 0)
    {
        if (!get_text_of_combo_box_entry(GTK_WIDGET(combobox), "radar_combo", &radarName))
            return;
    }
    else if (get_value_of_combo_box(GTK_WIDGET(combobox), "radar_combo", &radarName) < 1)
        return;

    GList *fileList = NULL;
    if (get_asp_subdirs(radarName, &fileList))
    {
        free(radarName);
        return;
    }
    /* Get all of the ASP files in relation to this radar and update */
    if (activeIter < 0)
    {
            GtkTreeModel *model;
            GtkListStore *store;
            model = gtk_combo_box_get_model(combobox);
            store = GTK_LIST_STORE(model);
            GtkTreeIter iter;
            int i;
            for (i = 0; i < gtk_tree_model_iter_n_children(model, NULL); i++)
            {
                if (gtk_tree_model_iter_nth_child(model, &iter, NULL, i))
                {
                    GValue myVal = {0};
                    gtk_tree_model_get_value(model, &iter, 0, &myVal);
                    if (g_ascii_strcasecmp(radarName, g_value_get_string(&myVal)) == 0)
                    {
                        gtk_combo_box_set_active(combobox, i);
                        g_value_unset(&myVal);
                        break;
                    }
                    g_value_unset(&myVal);
                } /* end if gtk_tree_model_iter_nth_child */
            } /*end for i */

            if (i == gtk_tree_model_iter_n_children(model, NULL))
            {
                g_list_foreach(fileList, (GFunc)my_g_free, NULL);
                g_list_free(fileList);
                free(radarName);
                return;
            }
    } /* end activeIter */
    update_combobox_from_glist(GTK_COMBO_BOX(lookup_widget(GTK_WIDGET(combobox), "start_product_combo")), 
            fileList, 1, 0);
    g_list_foreach(fileList, (GFunc)my_g_free, NULL);
    g_list_free(fileList);
    free(radarName);
}


/******************************************************************************
 *  Description:
 *      Callback for when the status bar changes.  Not currently used, 
 *      but could be at a later date.
 *  Input:
 *  Output:
 *  Returns:
 *  Notes:
 *  
 *****************************************************************************/
void
on_status_bar_text_popped              (GtkStatusbar    *statusbar,
                                        guint            context_id,
                                        gchar           *text,
                                        gpointer         user_data)
{
}

/******************************************************************************
 *  Description:
 *      Callback for when the status bar changes.  Not currently used, 
 *      but could be at a later date.
 *  Input:
 *  Output:
 *  Returns:
 *  Notes:
 *  
 *****************************************************************************/
void
on_status_bar_text_pushed              (GtkStatusbar    *statusbar,
                                        guint            context_id,
                                        gchar           *text,
                                        gpointer         user_data)
{
}


/******************************************************************************
 *  Description:
 *      Callback for when the status bar changes.  Not currently used, 
 *      but could be at a later date.
 *  Input:
 *  Output:
 *  Returns:
 *  Notes:
 *  
 *****************************************************************************/
void
on_status_bar_add                      (GtkContainer    *container,
                                        GtkWidget       *widget,
                                        gpointer         user_data)
{
}


/******************************************************************************
 *  Description:
 *      Callback for when the status bar changes.  Not currently used, 
 *      but could be at a later date.
 *  Input:
 *  Output:
 *  Returns:
 *  Notes:
 *  
 *****************************************************************************/
void
on_status_bar_remove                   (GtkContainer    *container,
                                        GtkWidget       *widget,
                                        gpointer         user_data)
{
}

/******************************************************************************
 *  Description:
 *      Callback for when the plus sign is clicked, to add a single packet 
 *      to the list.
 *  Input:
 *      Registered button/widget and user data.
 *  Output:
 *      Sends the packet to the selected packets list.
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
void
on_add_packet_to_list_button_clicked   (GtkButton       *button,
                                        gpointer         user_data)
{
    char *radarName;
    char *packetName;
    char *currentRadar;
    gchar *path;
    path = g_strdup(get_dir_location());

    /* This can occur if they don't have access to the dir_location.  */
    if (get_value_of_combo_box(GTK_WIDGET(button), "radar_combo", &radarName) < 1)
        return;
    if (get_value_of_combo_box(GTK_WIDGET(button), "start_product_combo", &packetName) < 1)
        return;
    if ((radarName == NULL) || (packetName == NULL) || (strlen(radarName) < 1) || (strlen(packetName) < 1))
        return;

    strcat(path, radarName);
    if (get_current_radar_being_displayed(GTK_WIDGET(button), &currentRadar) > 0)
    {
        /* If this is a different radar, then we need to clear it [the list] out.  If not, 
         * then we are just going to add it to the list. */
        if (strcmp(radarName, currentRadar) == 0)
        {
            add_unique_packet_to_packets_list(GTK_WIDGET(button), radarName, packetName, path);
            set_all_selected_packets_to_main_window(GTK_WIDGET(button));
        }
        else
        {
            clear_tree_view(GTK_WIDGET(button), "selected_packets_tree_view");
            clear_tree_filter_view(GTK_WIDGET(button), "message_tree");
            add_unique_packet_to_packets_list(GTK_WIDGET(button), radarName, packetName, path);
            set_all_selected_packets_to_main_window(GTK_WIDGET(button));
        }
        free(currentRadar);
    }
    else
    {
        /* 
         * This can happen if they enter a filename into the file box, then *
         * click the plus whenever nothing is in the packet list.
         */
        clear_tree_view(GTK_WIDGET(button), "selected_packets_tree_view");
        clear_tree_filter_view(GTK_WIDGET(button), "message_tree");
        
        add_unique_packet_to_packets_list(GTK_WIDGET(button), radarName, packetName, path);
        set_all_selected_packets_to_main_window(GTK_WIDGET(button));
    }
    free(radarName);
    free(packetName);
    g_free(path);
}


/******************************************************************************
 *  Description:
 *      Callback for when the minus sign is clicked 
 *  Input:
 *      Registered button/widget and user data
 *  Output:
 *      Removes the selected packet from the list.
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
void
on_subtrack_packet_from_list_button_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data)
{
    
    remove_from_tree_view(GTK_WIDGET(button), "selected_packets_tree_view");
    set_all_selected_packets_to_main_window(GTK_WIDGET(button));
}

/******************************************************************************
 *  Description:
 *      Callback for when the main window is shown
 *  Input:
 *      Registered widget and user data
 *  Output:
 *      Shows the main window.  Also, makes sure the right default data
 *      is brought in.
 *  Returns:
 *      Nothing
 *  Notes:
 *      Kind of a sloppy, but piecing this one apart would be tedious.
 *  
 *****************************************************************************/
void
on_outer_window_show                   (GtkWidget       *widget,
                                        gpointer         user_data)
{
    /* Make sure the filter is set */
    set_match_filter_time(widget, -1);

    /* Fills our radar combo list. */
    fill_radar_combo_list(widget);
    
    /* If they didn't pass anything in, then we are through. */
    if (get_init_file() == NULL)
        return;

    /* This is performing the same checks as when they hit the Go button. */
    if (strlen(get_init_file()) > 0)
    {
        if (does_directory_exist(get_init_file()))
        {
            GList *fileList = NULL;
            get_asp_files_in_dir(get_init_file(), &fileList);
            if (g_list_length(fileList) > 0)
            {
                add_glist_of_files_to_packet_list(GTK_WIDGET(widget), fileList, get_init_file());
                set_all_selected_packets_to_main_window(GTK_WIDGET(widget));
                g_list_foreach(fileList, (GFunc)my_g_free, NULL);
                g_list_free(fileList);
            }
            return;
        }

        if (does_file_exist(get_init_file()) < 1)
            return;

        char *basePath;
        char *fileName;
        get_base_path(&basePath, get_init_file());
        get_file_name(&fileName, get_init_file());
        if (is_file_a_valid_product(get_init_file()))
            add_unique_packet_to_packets_list(GTK_WIDGET(widget), "file", fileName, basePath);
        set_all_selected_packets_to_main_window(GTK_WIDGET(widget));
        g_free(fileName);
        g_free(basePath);
    }

}


/******************************************************************************
 *  Description:
 *      Callback for when then Start Product combobox is changed.
 *  Input:
 *      Registered combobox/widget and user data
 *  Output:
 *      Causes the End Product combobox to be updated with files that 
 *      are older than this one.
 *  Returns:
 *      Nothing.
 *  Notes:
 *  
 *****************************************************************************/
void
on_start_product_combo_changed         (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    char *radarName;
    char *packet;

    /* See which radar and packet this is for */
    if (get_value_of_combo_box(GTK_WIDGET(combobox), "radar_combo", &radarName) < 1)
    {
        if (!get_text_of_combo_box_entry(GTK_WIDGET(combobox), "radar_combo", &radarName))
            return;
    }
    if (get_value_of_combo_box(GTK_WIDGET(combobox), "start_product_combo", &packet) < 1)
        return;
    char *radarPath = g_strdup_printf("%s%s", get_dir_location(), radarName); 
    char *fileName;
    get_file_name(&fileName, g_strdup_printf("%s/%s", radarPath, packet));
    GList *endFiles = NULL;
    get_asp_files_in_dir_after_date(radarPath, &endFiles, pull_date_from_file_name(fileName));
    /* Updating the End Product combobox */
    update_combobox_from_glist(GTK_COMBO_BOX(lookup_widget(GTK_WIDGET(combobox), "end_product_combo")), 
            endFiles, 1, -1);
    free(packet);
    free(fileName);
    free(radarName);
    free(radarPath);
    g_list_foreach(endFiles, (GFunc)my_g_free, NULL);
    g_list_free(endFiles);
}


/******************************************************************************
 *  Description:
 *      Callback for when the End Product combobox is clicked.
 *  Input:
 *      Registered combobox/widget and user data
 *  Output:
 *      Fills the select packets list with the packets between the Start
 *      and End comboboxes, as well as those 2 packets in the comboboxes.
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
void
on_end_product_combo_changed           (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    char *radarName;
    char *startPacketName;
    char *endPacketName;

    /* This can happen if they don't have access to the dir_location */
    if (get_value_of_combo_box(GTK_WIDGET(combobox), "radar_combo", &radarName) < 1)
    {
        if (!get_text_of_combo_box_entry(GTK_WIDGET(combobox), "radar_combo", &radarName))
            return;
    }
    if (get_value_of_combo_box(GTK_WIDGET(combobox), "start_product_combo", &startPacketName) < 1)
    {
        free(radarName);
        return;
    }
    if (get_value_of_combo_box(GTK_WIDGET(combobox), "end_product_combo", &endPacketName) < 1)
    {
        free(radarName);
        return;
    }
   
    if ((endPacketName == NULL) || (strlen(endPacketName) < 1))
    {
        free(radarName);
        return;
    }
    GList *allFiles = NULL;
    char *radarPath = g_strdup_printf("%s%s", get_dir_location(), radarName); 
    /* Get all of the ASP files */
    get_asp_files_within_time_span(radarPath, &allFiles, pull_date_from_file_name(startPacketName), pull_date_from_file_name(endPacketName));
    clear_tree_view(GTK_WIDGET(combobox), "selected_packets_tree_view");

    /* Add them in.  They are added to this order for performance reasons. */
    int i;
    for (i = 0; i < g_list_length(allFiles); i++)
      add_packet_to_selected_packets_list(GTK_WIDGET(combobox), radarName, g_list_nth_data(allFiles, i), radarPath, 1);
    add_packet_to_selected_packets_list(GTK_WIDGET(combobox), radarName, startPacketName, radarPath, 1);
    add_packet_to_selected_packets_list(GTK_WIDGET(combobox), radarName, endPacketName, radarPath, 0);

    /* Show them all */
    set_all_selected_packets_to_main_window(GTK_WIDGET(combobox));

    /* Just delete this in case someone put something in there. */
    gtk_editable_delete_text(GTK_EDITABLE(lookup_widget(GTK_WIDGET(combobox), "file_directory_text")), 0, -1);
    free(radarName);
    free(startPacketName);
    free(endPacketName);
    g_list_foreach(allFiles, (GFunc)my_g_free, NULL);
    g_list_free(allFiles);
    return;
}


/******************************************************************************
 *  Description:
 *      Callback for when the Informational checkbutton is clicked.
 *  Input:
 *      Registered togglebutton/widget and user data
 *  Output:
 *      Filters the data in the message tree, adding or removing this data.
 *  Returns:
 *      Nothing
 *  Notes:
 *      The data is kept when it is checked and removed when unchecked.
 *  
 *****************************************************************************/
void
on_informational_checkbutton_toggled   (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    set_state_of_ignore_msg_types(togglebutton, 0);
    set_all_selected_packets_to_main_window(GTK_WIDGET(togglebutton));
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(togglebutton), "message_tree")))));
}


/******************************************************************************
 *  Description:
 *      Callback for when the General Status checkbutton is clicked.
 *  Input:
 *      Registered togglebutton/widget and user data
 *  Output:
 *      Filters the data in the message tree, adding or removing this data.
 *  Returns:
 *      Nothing
 *  Notes:
 *      The data is kept when it is checked and removed when unchecked.
 *  
 *****************************************************************************/
void
on_general_status_checkbutton_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    set_state_of_ignore_msg_types(togglebutton, 1);
    set_all_selected_packets_to_main_window(GTK_WIDGET(togglebutton));
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(togglebutton), "message_tree")))));
}


/******************************************************************************
 *  Description:
 *      Callback for when the Narrowband Communications checkbutton is clicked.
 *  Input:
 *      Registered togglebutton/widget and user data
 *  Output:
 *      Filters the data in the message tree, adding or removing this data.
 *  Returns:
 *      Nothing
 *  Notes:
 *      The data is kept when it is checked and removed when unchecked.
 *  
 *****************************************************************************/
void
on_narrowband_communications_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    set_state_of_ignore_msg_types(togglebutton, 3);
    set_all_selected_packets_to_main_window(GTK_WIDGET(togglebutton));
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(togglebutton), "message_tree")))));
}


/******************************************************************************
 *  Description:
 *      Callback for when the Warnings/Errors checkbutton is clicked.
 *  Input:
 *      Registered togglebutton/widget and user data
 *  Output:
 *      Filters the data in the message tree, adding or removing this data.
 *  Returns:
 *      Nothing
 *  Notes:
 *      The data is kept when it is checked and removed when unchecked.
 *  
 *****************************************************************************/
void
on_warnings_errors_checkbutton_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    set_state_of_ignore_msg_types(togglebutton, 2);
    set_all_selected_packets_to_main_window(GTK_WIDGET(togglebutton));
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(togglebutton), "message_tree")))));
}


/******************************************************************************
 *  Description:
 *      Callback for when the Load Shed checkbutton is clicked.
 *  Input:
 *      Registered togglebutton/widget and user data
 *  Output:
 *      Filters the data in the message tree, adding or removing this data.
 *  Returns:
 *      Nothing
 *  Notes:
 *      The data is kept when it is checked and removed when unchecked.
 *  
 *****************************************************************************/
void
on_load_shed_checkbutton_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    set_state_of_ignore_msg_types(togglebutton, 6);
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(togglebutton), "message_tree")))));
}


/******************************************************************************
 *  Description:
 *      Callback for when the RPG Maintenance Required checkbutton is clicked.
 *  Input:
 *      Registered togglebutton/widget and user data
 *  Output:
 *      Filters the data in the message tree, adding or removing this data.
 *  Returns:
 *      Nothing
 *  Notes:
 *      The data is kept when it is checked and removed when unchecked.
 *  
 *****************************************************************************/
void
on_maintenance_required_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    set_state_of_ignore_msg_types(togglebutton, 5);
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(togglebutton), "message_tree")))));
}


/******************************************************************************
 *  Description:
 *      Callback for when the RPG Maintenance Mandatory checkbutton is clicked.
 *  Input:
 *      Registered togglebutton/widget and user data
 *  Output:
 *      Filters the data in the message tree, adding or removing this data.
 *  Returns:
 *      Nothing
 *  Notes:
 *      The data is kept when it is checked and removed when unchecked.
 *  
 *****************************************************************************/
void
on_maintenance_mandatory_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    set_state_of_ignore_msg_types(togglebutton, 4);
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(togglebutton), "message_tree")))));
}


/******************************************************************************
 *  Description:
 *      Callback for when the Secondary RDA Alarm checkbutton is clicked.
 *  Input:
 *      Registered togglebutton/widget and user data
 *  Output:
 *      Filters the data in the message tree, adding or removing this data.
 *  Returns:
 *      Nothing
 *  Notes:
 *      The data is kept when it is checked and removed when unchecked.
 *  
 *****************************************************************************/
void
on_secondary_rda_alarm_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    set_state_of_ignore_msg_types(togglebutton, 8);
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(togglebutton), "message_tree")))));
}


/******************************************************************************
 *  Description:
 *      Callback for when the RDA Maintenance Required checkbutton is clicked.
 *  Input:
 *      Registered togglebutton/widget and user data
 *  Output:
 *      Filters the data in the message tree, adding or removing this data.
 *  Returns:
 *      Nothing
 *  Notes:
 *      The data is kept when it is checked and removed when unchecked.
 *  
 *****************************************************************************/
void
on_maintenance_required_rda_alarm_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    set_state_of_ignore_msg_types(togglebutton, 9);
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(togglebutton), "message_tree")))));
}


/******************************************************************************
 *  Description:
 *      Callback for when the RDA Maintenance Mandatory checkbutton is clicked.
 *  Input:
 *      Registered togglebutton/widget and user data
 *  Output:
 *      Filters the data in the message tree, adding or removing this data.
 *  Returns:
 *      Nothing
 *  Notes:
 *      The data is kept when it is checked and removed when unchecked.
 *  
 *****************************************************************************/
void
on_maintenance_mandatory_rda_alarm_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    set_state_of_ignore_msg_types(togglebutton, 10);
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(togglebutton), "message_tree")))));
}


/******************************************************************************
 *  Description:
 *      Callback for when the RDA Inoperable checkbutton is clicked.
 *  Input:
 *      Registered togglebutton/widget and user data
 *  Output:
 *      Filters the data in the message tree, adding or removing this data.
 *  Returns:
 *      Nothing
 *  Notes:
 *      The data is kept when it is checked and removed when unchecked.
 *  
 *****************************************************************************/
void
on_inoperable_rda_alarm_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    set_state_of_ignore_msg_types(togglebutton, 11);
    set_state_of_ignore_msg_types(togglebutton, 12);
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(togglebutton), "message_tree")))));
}

/******************************************************************************
 *  Description:
 *      Callback for when the Filter Ranges button is clicked.
 *  Input:
 *      Registered button/widget and user data
 *  Output:
 *      Filters the main list according to the dates set in the spin buttons.
 *      All rows not within the time specified will be removed.
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
void
on_filter_ranges_button_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
    set_range_start_filter_time(get_range_start_time(GTK_WIDGET(button)));
    set_range_end_filter_time(get_range_end_time(GTK_WIDGET(button)));
    set_match_filter_time(GTK_WIDGET(button), -1);
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(button), "message_tree")))));
}


/******************************************************************************
 *  Description:
 *      Callback for when the Filter Match button is clicked.
 *  Input:
 *      Registered button/widget and user data.
 *  Output:
 *      Filters the main list according to the values specified in the 
 *      spinbuttons.  Any rows that has a time stamp in which the dates does
 *      not match the time given will be removed.  -1 values are wildcards.
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
void
on_filter_match_button_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
    set_range_start_filter_time(-1);
    set_range_end_filter_time(-1);
    set_match_filter_time(GTK_WIDGET(button), 1);
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(button), "message_tree")))));
}

/******************************************************************************
 *  Description:
 *      Callback so we keep our filter.
 *  Input:
 *      Registered model, iterator and data
 *  Output:
 *      A filtered main list.
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
gboolean main_tree_filter_callback(GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    return message_tree_filtering_function(model, iter, NULL);
}

/******************************************************************************
 *  Description:
 *      Callback mask for the destroy.
 *  Input:
 *      Registered object and user data.
 *  Output:
 *      Nothing
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
void 
on_filter_window_destroy               (GtkObject       *object,
                                        gpointer        user_data)
{
    set_filter_window_closed();
}

/******************************************************************************
 *  Description:
 *      Mask for the set_filter_window_open function.
 *  Input:
 *      Registered widget and user data.
 *  Output:
 *      Nothing.
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
void
on_filter_window_show                  (GtkWidget       *widget,
                                        gpointer        user_data)
{
    set_filter_window_open();
}

/******************************************************************************
 *  Description:
 *      Callback for when the Clear All button is clicked.
 *  Input:
 *      Registered button and user data
 *  Output:
 *      Clears all entries and filters.
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
void
on_clear_all_button_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
    set_range_start_filter_time(-1);
    set_range_end_filter_time(-1);
    set_match_filter_time(GTK_WIDGET(button), -1);
    reset_ignore_msg_types();
    clear_tree_view(GTK_WIDGET(button), "selected_packets_tree_view");
    clear_tree_filter_view(GTK_WIDGET(button), "message_tree");
    gtk_editable_delete_text(GTK_EDITABLE(lookup_widget(GTK_WIDGET(button), "file_directory_text")), 0, -1);
    gtk_editable_delete_text(GTK_EDITABLE(lookup_widget(GTK_WIDGET(button), "search_text")), 0, -1);
    GtkWidget *filter_window = lookup_widget(GTK_WIDGET(button), "filter_window");

    /* We need to make sure to kill the filter window.  If we don't, how do we
     * know what to set the spin buttons to?  We have removed the packets. */
    if (filter_window == NULL)
        return;
    gtk_widget_destroy(filter_window);
}

/******************************************************************************
 *  Description:
 *      Callback for when the help button is clicked. 
 *  Input:
 *      Registered button/widget and user data.
 *  Output:
 *      Brings up the Yelp tool to display the man page for this tool.
 *  Returns:
 *      Nothing
 *  Notes:
 *  
 *****************************************************************************/
void
on_help_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
    GError *error = NULL;
    gchar *tmp;
    tmp = g_find_program_in_path("yelp");

    if (tmp) {
        g_free(tmp);
        char* homeDir;
        homeDir = getenv("HOME");
        gchar* command = NULL;
        command = g_strdup_printf("yelp man:%s/man/cat1/asp_view.1", homeDir);
        g_spawn_command_line_async(command, &error);
        g_free(command);
    }
}

