#include <gtk/gtk.h>


void
on_radar_combo_changed                 (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_filter_button_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_clear_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_go_button_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_open_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_open_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_go_button_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_message_filter_button_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_clear_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_search_text_editing_done            (GtkCellEditable *celleditable,
                                        gpointer         user_data);

void
on_search_text_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_search_text_delete_text             (GtkEditable     *editable,
                                        gint             start_pos,
                                        gint             end_pos,
                                        gpointer         user_data);

void
on_search_text_insert_text             (GtkEditable     *editable,
                                        gchar           *new_text,
                                        gint             new_text_length,
                                        gpointer         position,
                                        gpointer         user_data);

void
on_search_text_paste_clipboard         (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_radar_combo_editing_done            (GtkCellEditable *celleditable,
                                        gpointer         user_data);

void
on_radar_combo_changed                 (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_message_combo_changed               (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_status_bar_text_popped              (GtkStatusbar    *statusbar,
                                        guint            context_id,
                                        gchar           *text,
                                        gpointer         user_data);

void
on_status_bar_text_pushed              (GtkStatusbar    *statusbar,
                                        guint            context_id,
                                        gchar           *text,
                                        gpointer         user_data);

void
on_status_bar_add                      (GtkContainer    *container,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

void
on_status_bar_remove                   (GtkContainer    *container,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

void
on_add_packet_to_list_button_clicked   (GtkButton       *button,
                                        gpointer         user_data);

void
on_subtrack_packet_from_list_button_clicked
                                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_view_specific_packet_button_clicked (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_outer_window_focus                  (GtkWidget       *widget,
                                        GtkDirectionType  direction,
                                        gpointer         user_data);

void
on_outer_window_show                   (GtkWidget       *widget,
                                        gpointer         user_data);
void
on_start_product_combo_changed         (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_end_product_combo_changed           (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_informational_checkbutton_toggled   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_general_status_checkbutton_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_narrowband_communications_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);
void
on_warnings_errors_checkbutton_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_load_shed_checkbutton_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_maintenance_required_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_maintenance_mandatory_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_secondary_rda_alarm_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_maintenance_required_rda_alarm_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_maintenance_mandatory_rda_alarm_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_inoperable_rda_alarm_checkbutton_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_select_dates_range_year_start_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_range_month_start_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_range_day_start_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_range_hour_start_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_range_minute_start_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_range_second_start_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_range_year_end_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_range_month_end_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_range_day_end_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_range_hour_end_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_range_minute_end_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_range_second_end_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_match_year_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_match_month_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_match_day_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_match_hour_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_match_minute_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_select_dates_match_second_spinbutton_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);
void
on_select_dates_range_year_start_spinbutton_change_value
                                        (GtkSpinButton   *spinbutton,
                                        GtkScrollType    scroll,
                                        gpointer         user_data);

void
on_select_dates_range_year_start_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);
void
on_select_dates_range_month_start_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_range_day_start_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_range_hour_start_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_range_minute_start_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_range_second_start_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_range_year_end_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_range_month_end_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_range_day_end_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_range_hour_end_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_range_minute_end_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_range_second_end_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_match_year_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_match_month_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_match_day_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_match_hour_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_match_minute_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_select_dates_match_second_spinbutton_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);
void
on_filter_ranges_button_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_filter_match_button_clicked         (GtkButton       *button,
                                        gpointer         user_data);

gboolean
main_tree_filter_callback              (GtkTreeModel    *model, 
                                        GtkTreeIter     *iter, 
                                        gpointer        data);

void
on_filter_window_destroy               (GtkObject       *object, 
                                        gpointer        user_data);

void
on_filter_window_show                  (GtkWidget       *widget, 
                                        gpointer        user_data);

void
on_clear_all_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_help_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);
