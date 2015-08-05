/*
 * RCS info
 * $Author: jclose $
 * $Locker:  $
 * $Date: 2009/06/29 16:02:05 $
 * $Id: mon_wb_gui_callbacks.h,v 1.1 2009/06/29 16:02:05 jclose Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <gtk/gtk.h>

void
on_window_destroy                      (GtkObject *object,
                                            gpointer   user_data);

void
on_scroll_radiobutton_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_pause_radiobutton_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_freeze_radiobutton_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_message_list_treeview_add           (GtkContainer    *container,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

void
on_available_filters_treeview_add      (GtkContainer    *container,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

void
on_available_filters_treeview_remove   (GtkContainer    *container,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

void
on_push_down_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_push_up_button_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_current_filters_treeview_add        (GtkContainer    *container,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

void
on_current_filters_treeview_remove     (GtkContainer    *container,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

void
on_verbosity_spinbutton_changed        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_log_size_spinbutton_changed         (GtkEditable     *editable,
                                        gpointer         user_data);
gboolean
on_message_list_treeview_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_outer_window_destroy                (GtkObject       *object,
                                        gpointer         user_data);
