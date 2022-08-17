/* tkmv-window.c
 *
 * Copyright 2022 Alin Popa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tkmv-types.h"

#include "tkmv-config.h"
#include "tkmv-window.h"

#include "tkmv-application.h"
#include "tkmv-settings.h"

#include "views/tkmv-dashboard-view.h"
#include "views/tkmv-processes-view.h"
#include "views/tkmv-systeminfo-view.h"

static void window_views_init (TkmvWindow *self);
static void window_toolbar_init (TkmvWindow *self);
static void tkmv_window_update_toolbar (TkmvWindow *window);
static gboolean update_views_content_invoke (gpointer _self);
static void tools_visible_child_changed (GObject *stack, GParamSpec *pspec,
                                         TkmvWindow *self);
static void tools_session_list_changed (GtkComboBox *self,
                                        gpointer _tkmv_window);
static void tools_time_source_changed (GtkComboBox *self,
                                       gpointer _tkmv_window);
static void tools_time_interval_changed (GtkComboBox *self,
                                         gpointer _tkmv_window);
static void tools_timestamp_scale_value_changed (GtkRange *self,
                                                 gpointer _tkmv_window);
static void tools_set_timestamp_text (TkmvWindow *self, DataTimeSource source,
                                      guint timestamp_sec);

static void load_window_size (TkmvWindow *self);
static void open_file_menu_add_file (gpointer _rf, gpointer _window);
static void close_window_signal (TkmvWindow *self, gpointer user_data);
static void open_file_menu_add_file (gpointer _rf, gpointer _window);
static void update_open_button_menu (TkmvWindow *self);
static void on_open_file_response (GtkDialog *dialog, int response,
                                   gpointer user_data);
static void open_button_clicked (GtkButton *self, gpointer user_data);
static void notify_system_supports_color_schemes_cb (TkmvWindow *self);
static gboolean window_progress_spinner_start_invoke (gpointer _self);
static gboolean window_progress_spinner_stop_invoke (gpointer _self);

struct _TkmvWindow
{
  AdwApplicationWindow parent_instance;

  /* Progress spinner counter */
  gint spinner_counter;

  AdwSplitButton *open_button;
  GMenu *open_button_menu_model;

  /* Main views */
  TkmvDashboardView *dashboard_view;
  TkmvProcessesView *processes_view;
  TkmvSysteminfoView *systeminfo_view;

  /* Template widgets */
  GtkViewport *dashboard_viewport;
  GtkViewport *processes_viewport;
  GtkViewport *systeminfo_viewport;

  GtkToggleButton *tools_button;
  GtkSearchBar *tools_bar;
  GtkSpinner *main_spinner;

  /* Toolbar widgets */
  GtkButton *session_info_button;
  GtkComboBoxText *session_list_combobox;
  GtkComboBoxText *time_source_combobox;
  GtkComboBoxText *time_interval_combobox;
  GtkLabel *timestamp_label;
  GtkScale *timestamp_scale;
  GtkLabel *timestamp_text;
  GtkAdjustment *timestamp_scale_adjustment;
  GtkButton *timeline_refresh_button;

  /* Session info */
  GtkDialog *session_info_dialog;
  GtkEntry *session_info_device_name;
  GtkEntryBuffer *info_device_name_entry_buffer;
  GtkEntry *session_info_device_cpus;
  GtkEntryBuffer *info_device_cpus_entry_buffer;
  GtkEntry *session_info_session_name;
  GtkEntryBuffer *info_session_name_entry_buffer;
  GtkEntry *session_info_session_hash;
  GtkEntryBuffer *info_session_hash_entry_buffer;
  GtkEntry *session_info_session_start;
  GtkEntryBuffer *info_session_start_entry_buffer;
  GtkEntry *session_info_session_end;
  GtkEntryBuffer *info_session_end_entry_buffer;
};

G_DEFINE_TYPE (TkmvWindow, tkmv_window, ADW_TYPE_APPLICATION_WINDOW)

static void
tkmv_window_class_init (TkmvWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (
      widget_class, "/ro/fxdata/taskmonitor/viewer/gtk/tkmv-window.ui");
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        dashboard_viewport);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        processes_viewport);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        systeminfo_viewport);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        tools_button);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow, tools_bar);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow, open_button);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        main_spinner);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        session_info_button);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        session_list_combobox);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        time_source_combobox);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        time_interval_combobox);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        timestamp_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        timestamp_scale);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        timestamp_text);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        timeline_refresh_button);

  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        session_info_dialog);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        session_info_device_name);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        session_info_device_cpus);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        session_info_session_name);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        session_info_session_hash);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        session_info_session_start);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        session_info_session_end);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        info_device_name_entry_buffer);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        info_device_cpus_entry_buffer);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        info_session_name_entry_buffer);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        info_session_hash_entry_buffer);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        info_session_start_entry_buffer);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        info_session_end_entry_buffer);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow,
                                        timestamp_scale_adjustment);

  /* Bind callbacks */
  gtk_widget_class_bind_template_callback (widget_class,
                                           tools_visible_child_changed);
}

static void
tkmv_window_init (TkmvWindow *self)
{
  AdwStyleManager *manager = adw_style_manager_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));

  window_views_init (self);
  window_toolbar_init (self);
  load_window_size (self);

  self->open_button_menu_model = g_menu_new ();
  update_open_button_menu (self);

  g_signal_connect_object (
      manager, "notify::system-supports-color-schemes",
      G_CALLBACK (notify_system_supports_color_schemes_cb), self,
      G_CONNECT_SWAPPED);
  notify_system_supports_color_schemes_cb (self);

  g_signal_connect (G_OBJECT (self), "destroy",
                    G_CALLBACK (close_window_signal), NULL);
  g_signal_connect (G_OBJECT (self->open_button), "clicked",
                    G_CALLBACK (open_button_clicked), self);

  g_object_bind_property (self->tools_button, "active", self->tools_bar,
                          "search-mode-enabled", G_BINDING_BIDIRECTIONAL);
}

static void
window_views_init (TkmvWindow *self)
{
  self->dashboard_view = g_object_new (TKMV_TYPE_DASHBOARD_VIEW, NULL);
  gtk_viewport_set_child (self->dashboard_viewport,
                          GTK_WIDGET (self->dashboard_view));

  self->processes_view = g_object_new (TKMV_TYPE_PROCESSES_VIEW, NULL);
  gtk_viewport_set_child (self->processes_viewport,
                          GTK_WIDGET (self->processes_view));

  self->systeminfo_view = g_object_new (TKMV_TYPE_SYSTEMINFO_VIEW, NULL);
  gtk_viewport_set_child (self->systeminfo_viewport,
                          GTK_WIDGET (self->systeminfo_view));
}

static void
tools_session_list_changed (GtkComboBox *self, gpointer _tkmv_window)
{
  TkmvWindow *window = (TkmvWindow *)_tkmv_window;
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  TkmSessionEntry *active_session = NULL;

  g_assert (window);

  if (gtk_combo_box_get_active_id (self) == NULL)
    return;

  for (guint i = 0; i < sessions->len; i++)
    {
      if (tkm_session_entry_get_active (g_ptr_array_index (sessions, i)))
        {
          active_session = g_ptr_array_index (sessions, i);
        }
    }

  if (active_session == NULL)
    return;

  if (g_strcmp0 (gtk_combo_box_get_active_id (self),
                 tkm_session_entry_get_hash (active_session))
      == 0)
    return;

  for (guint i = 0; i < sessions->len; i++)
    {
      TkmSessionEntry *session = g_ptr_array_index (sessions, i);

      if (g_strcmp0 (gtk_combo_box_get_active_id (self),
                     tkm_session_entry_get_hash (session))
          == 0)
        {
          tkm_session_entry_set_active (session, TRUE);
        }
      else
        {
          tkm_session_entry_set_active (session, FALSE);
        }
    }

  tkmv_application_load_data (
      tkmv_application_instance (), gtk_combo_box_get_active_id (self),
      gtk_range_get_value (GTK_RANGE (window->timestamp_scale)));
}

static void
tools_time_source_changed (GtkComboBox *self, gpointer _tkmv_window)
{
  TkmvWindow *window = (TkmvWindow *)_tkmv_window;
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  TkmSessionEntry *active_session = NULL;

  g_assert (window);

  tkmv_settings_set_time_source (settings,
                                 (guint)gtk_combo_box_get_active (self));

  if (sessions == NULL)
    return;

  if (sessions->len == 0)
    return;

  for (guint i = 0; i < sessions->len; i++)
    {
      if (tkm_session_entry_get_active (g_ptr_array_index (sessions, i)))
        {
          active_session = g_ptr_array_index (sessions, i);
        }
    }

  g_assert (active_session);

  gtk_range_set_range (
      GTK_RANGE (window->timestamp_scale),
      tkm_session_entry_get_first_timestamp (
          active_session, tkmv_settings_get_time_source (settings)),
      tkm_session_entry_get_last_timestamp (
          active_session, tkmv_settings_get_time_source (settings)));
  gtk_range_set_value (
      GTK_RANGE (window->timestamp_scale),
      tkm_session_entry_get_first_timestamp (
          active_session, tkmv_settings_get_time_source (settings)));

  tkmv_application_load_data (
      tkmv_application_instance (),
      tkm_session_entry_get_hash (active_session),
      gtk_range_get_value (GTK_RANGE (window->timestamp_scale)));
}

static void
tools_time_interval_changed (GtkComboBox *self, gpointer _tkmv_window)
{
  TkmvWindow *window = (TkmvWindow *)_tkmv_window;
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  TkmSessionEntry *active_session = NULL;

  g_assert (window);

  tkmv_settings_set_time_interval (settings,
                                   (guint)gtk_combo_box_get_active (self));

  if (sessions == NULL)
    return;

  if (sessions->len == 0)
    return;

  for (guint i = 0; i < sessions->len; i++)
    {
      if (tkm_session_entry_get_active (g_ptr_array_index (sessions, i)))
        {
          active_session = g_ptr_array_index (sessions, i);
        }
    }

  g_assert (active_session);

  gtk_range_set_range (
      GTK_RANGE (window->timestamp_scale),
      tkm_session_entry_get_first_timestamp (
          active_session, tkmv_settings_get_time_source (settings)),
      tkm_session_entry_get_last_timestamp (
          active_session, tkmv_settings_get_time_source (settings)));
  gtk_range_set_value (
      GTK_RANGE (window->timestamp_scale),
      tkm_session_entry_get_first_timestamp (
          active_session, tkmv_settings_get_time_source (settings)));

  tkmv_application_load_data (
      tkmv_application_instance (),
      tkm_session_entry_get_hash (active_session),
      gtk_range_get_value (GTK_RANGE (window->timestamp_scale)));
}

static void
tools_timestamp_scale_value_changed (GtkRange *self, gpointer _tkmv_window)
{
  TkmvWindow *window = (TkmvWindow *)_tkmv_window;
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  TkmSessionEntry *active_session = NULL;

  g_assert (window);

  if (sessions == NULL)
    return;

  if (sessions->len == 0)
    return;

  for (guint i = 0; i < sessions->len; i++)
    {
      if (tkm_session_entry_get_active (g_ptr_array_index (sessions, i)))
        {
          active_session = g_ptr_array_index (sessions, i);
        }
    }

  g_assert (active_session);

  tools_set_timestamp_text (window, tkmv_settings_get_time_source (settings),
                            gtk_range_get_value (self));

  if (tkmv_settings_get_auto_timeline_refresh (settings))
    {
      tkmv_application_load_data (tkmv_application_instance (),
                                  tkm_session_entry_get_hash (active_session),
                                  gtk_range_get_value (self));
    }
}

// Function to open a dialog box with a message
static void
session_info_dialog (GtkButton *self, gpointer _tkmv_window)
{
  TkmvWindow *window = (TkmvWindow *)_tkmv_window;
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  TkmSessionEntry *active_session = NULL;

  g_assert (window);

  if (sessions == NULL)
    return;

  if (sessions->len == 0)
    return;

  for (guint i = 0; i < sessions->len; i++)
    {
      if (tkm_session_entry_get_active (g_ptr_array_index (sessions, i)))
        {
          active_session = g_ptr_array_index (sessions, i);
        }
    }

  g_assert (active_session);

  TKMV_UNUSED (self);

  gtk_entry_buffer_set_text (
      window->info_device_name_entry_buffer,
      tkm_session_entry_get_device_name (active_session), -1);
  gtk_entry_buffer_set_text (window->info_session_name_entry_buffer,
                             tkm_session_entry_get_name (active_session), -1);
  gtk_entry_buffer_set_text (window->info_session_hash_entry_buffer,
                             tkm_session_entry_get_hash (active_session), -1);

  do
    {
      gchar buf[32];
      snprintf (buf, sizeof (buf), "%u",
                tkm_session_entry_get_device_cpus (active_session));
      gtk_entry_buffer_set_text (window->info_device_cpus_entry_buffer, buf,
                                 -1);
    }
  while (FALSE);

  do
    {
      g_autoptr (GDateTime) dtime = g_date_time_new_from_unix_utc (
          tkm_session_entry_get_first_timestamp (active_session,
                                                 DATA_TIME_SOURCE_SYSTEM));
      g_autofree gchar *text = g_date_time_format (dtime, "%A, %F %H:%M:%S");
      gtk_entry_buffer_set_text (window->info_session_start_entry_buffer, text,
                                 -1);
    }
  while (FALSE);

  do
    {
      g_autoptr (GDateTime) dtime = g_date_time_new_from_unix_utc (
          tkm_session_entry_get_last_timestamp (active_session,
                                                DATA_TIME_SOURCE_SYSTEM));
      g_autofree gchar *text = g_date_time_format (dtime, "%A, %F %H:%M:%S");
      gtk_entry_buffer_set_text (window->info_session_end_entry_buffer, text,
                                 -1);
    }
  while (FALSE);

  gtk_window_set_title (GTK_WINDOW (window->session_info_dialog),
                        "Session information");
  gtk_window_set_modal (GTK_WINDOW (window->session_info_dialog), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (window->session_info_dialog),
                                GTK_WINDOW (window));
  gtk_window_set_hide_on_close (GTK_WINDOW (window->session_info_dialog),
                                TRUE);

  gtk_widget_show (GTK_WIDGET (window->session_info_dialog));
}

static void
timeline_refresh_button_clicked (GtkButton *self, gpointer user_data)
{
  TkmvWindow *window = (TkmvWindow *)user_data;

  TKMV_UNUSED (self);
  g_assert (window);

  tkmv_window_request_update_data (window);
}

static void
window_toolbar_init (TkmvWindow *self)
{
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());

  gtk_combo_box_text_remove_all (self->session_list_combobox);
  gtk_combo_box_set_active (GTK_COMBO_BOX (self->time_source_combobox),
                            (gint)tkmv_settings_get_time_source (settings));
  gtk_combo_box_set_active (GTK_COMBO_BOX (self->time_interval_combobox),
                            (gint)tkmv_settings_get_time_interval (settings));

  g_signal_connect (G_OBJECT (self->session_info_button), "clicked",
                    G_CALLBACK (session_info_dialog), self);
  g_signal_connect (G_OBJECT (self->session_list_combobox), "changed",
                    G_CALLBACK (tools_session_list_changed), self);
  g_signal_connect (G_OBJECT (self->time_source_combobox), "changed",
                    G_CALLBACK (tools_time_source_changed), self);
  g_signal_connect (G_OBJECT (self->time_interval_combobox), "changed",
                    G_CALLBACK (tools_time_interval_changed), self);
  g_signal_connect (G_OBJECT (self->timestamp_scale), "value-changed",
                    G_CALLBACK (tools_timestamp_scale_value_changed), self);
  g_signal_connect (G_OBJECT (self->timeline_refresh_button), "clicked",
                    G_CALLBACK (timeline_refresh_button_clicked), self);
}

static void
tools_visible_child_changed (GObject *stack, GParamSpec *pspec,
                             TkmvWindow *self)
{
  TKMV_UNUSED (stack);
  TKMV_UNUSED (pspec);
  TKMV_UNUSED (self);
}

static void
load_window_size (TkmvWindow *self)
{
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());
  gint width, height;

  g_assert (self);

  tkmv_settings_get_main_window_size (settings, &width, &height);
  gtk_window_set_default_size (GTK_WINDOW (self), width, height);
}

static void
close_window_signal (TkmvWindow *self, gpointer user_data)
{
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());
  gint width, height;

  g_assert (self);
  TKMV_UNUSED (user_data);

  gtk_window_get_default_size (GTK_WINDOW (self), &width, &height);
  tkmv_settings_set_main_window_size (settings, width, height);
}

static void
open_file_menu_add_file (gpointer _rf, gpointer _window)
{
  TkmvSettingsRecentFile *rf = (TkmvSettingsRecentFile *)_rf;
  TkmvWindow *window = (TkmvWindow *)_window;
  const gchar *action = NULL;

  g_assert (rf);
  g_assert (window);

  switch (tkmv_settings_recent_file_get_index (rf))
    {
    case 0:
      action = "app.open_recent_file_1";
      break;

    case 1:
      action = "app.open_recent_file_2";
      break;

    case 2:
      action = "app.open_recent_file_3";
      break;

    default:
      break;
    }

  g_menu_append (window->open_button_menu_model,
                 tkmv_settings_recent_file_get_name (rf), action);
}

static void
update_open_button_menu (TkmvWindow *self)
{
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());

  g_assert (self);

  g_menu_remove_all (self->open_button_menu_model);

  if (tkmv_settings_recent_files_count (settings) == 0)
    adw_split_button_set_menu_model (self->open_button, NULL);
  else
    {
      GList *recent_files = tkmv_settings_get_recent_files (settings);
      g_list_foreach (recent_files, open_file_menu_add_file, self);
      adw_split_button_set_menu_model (
          self->open_button, G_MENU_MODEL (self->open_button_menu_model));
    }
}

static void
on_open_file_response (GtkDialog *dialog, int response, gpointer user_data)
{
  TkmvWindow *self = (TkmvWindow *)user_data;
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());

  g_assert (self);

  if (response == GTK_RESPONSE_ACCEPT)
    {
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
      GFile *file = gtk_file_chooser_get_file (chooser);

      if (file != NULL)
        {
          g_autoptr (TkmvSettingsRecentFile) rf
              = tkmv_settings_recent_file_new (g_file_get_basename (file),
                                               g_file_get_path (file));

          tkmv_settings_add_recent_file (settings, rf);
          tkmv_application_open_file (tkmv_application_instance (),
                                      g_file_get_path (file));
        }

      g_object_unref (file);
    }

  update_open_button_menu (self);
  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
open_button_clicked (GtkButton *self, gpointer user_data)
{
  TkmvWindow *window = (TkmvWindow *)user_data;

  g_autoptr (GtkFileFilter) filter = NULL;
  GtkWidget *dialog;

  TKMV_UNUSED (self);

  dialog = gtk_file_chooser_dialog_new (
      "Open File", GTK_WINDOW (user_data), GTK_FILE_CHOOSER_ACTION_OPEN,
      "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), FALSE);

  filter = gtk_file_filter_new ();
  gtk_file_filter_add_pattern (filter, "*.tkm.db");
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  gtk_widget_show (dialog);

  g_signal_connect (dialog, "response", G_CALLBACK (on_open_file_response),
                    window);
}

static void
notify_system_supports_color_schemes_cb (TkmvWindow *self)
{
  AdwStyleManager *manager = adw_style_manager_get_default ();
  gboolean supports
      = adw_style_manager_get_system_supports_color_schemes (manager);

  TKMV_UNUSED (self);

  if (supports)
    adw_style_manager_set_color_scheme (manager, ADW_COLOR_SCHEME_DEFAULT);
}

static gboolean
window_progress_spinner_start_invoke (gpointer _self)
{
  TkmvWindow *window = (TkmvWindow *)_self;

  g_assert (window);

  gtk_spinner_set_spinning (window->main_spinner, TRUE);
  g_atomic_int_inc (&window->spinner_counter);

  return FALSE;
}

void
tkmv_window_progress_spinner_start (TkmvWindow *window)
{
  g_assert (window);
  g_main_context_invoke (NULL, window_progress_spinner_start_invoke, window);
}

static gboolean
window_progress_spinner_stop_invoke (gpointer _self)
{
  TkmvWindow *window = (TkmvWindow *)_self;

  g_assert (window);

  if (g_atomic_int_dec_and_test (&window->spinner_counter))
    gtk_spinner_set_spinning (window->main_spinner, FALSE);

  return FALSE;
}

void
tkmv_window_progress_spinner_stop (TkmvWindow *window)
{
  g_assert (window);
  g_main_context_invoke (NULL, window_progress_spinner_stop_invoke, window);
}

static void
tools_set_timestamp_text (TkmvWindow *self, DataTimeSource source,
                          guint timestamp_sec)
{
  g_assert (self);

  switch (source)
    {
    case DATA_TIME_SOURCE_SYSTEM:
    case DATA_TIME_SOURCE_RECEIVE:
      {
        g_autoptr (GDateTime) dtime
            = g_date_time_new_from_unix_utc (timestamp_sec);
        g_autofree gchar *text = g_date_time_format (dtime, "%H:%M:%S");
        gtk_label_set_text (self->timestamp_text, text);
        break;
      }

    case DATA_TIME_SOURCE_MONOTONIC:
      {
        g_autofree gchar *text = g_strdup_printf ("%u", timestamp_sec);
        gtk_label_set_text (self->timestamp_text, text);
        break;
      }
    }
}

static void
tkmv_window_update_toolbar (TkmvWindow *window)
{
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  TkmSessionEntry *active_session = NULL;

  g_assert (window);

  gtk_combo_box_text_remove_all (window->session_list_combobox);
  for (guint i = 0; i < sessions->len; i++)
    {
      TkmSessionEntry *session = g_ptr_array_index (sessions, i);

      gtk_combo_box_text_append (window->session_list_combobox,
                                 tkm_session_entry_get_hash (session),
                                 tkm_session_entry_get_name (session));

      if (tkm_session_entry_get_active (session))
        {
          active_session = g_ptr_array_index (sessions, i);
        }
    }

  if (active_session == NULL)
    {
      active_session = g_ptr_array_index (sessions, 0);
      tkm_session_entry_set_active (active_session, TRUE);
    }

  if (g_strcmp0 (gtk_combo_box_get_active_id (
                     GTK_COMBO_BOX (window->session_list_combobox)),
                 tkm_session_entry_get_hash (active_session))
      != 0)
    {
      gtk_combo_box_set_active_id (
          GTK_COMBO_BOX (window->session_list_combobox),
          tkm_session_entry_get_hash (active_session));
    }

  if (gtk_combo_box_get_active (GTK_COMBO_BOX (window->time_source_combobox))
      != (gint)tkmv_settings_get_time_source (settings))
    {
      gtk_combo_box_set_active (
          GTK_COMBO_BOX (window->time_source_combobox),
          (gint)tkmv_settings_get_time_source (settings));
    }

  if (gtk_combo_box_get_active (GTK_COMBO_BOX (window->time_interval_combobox))
      != (gint)tkmv_settings_get_time_interval (settings))
    {
      gtk_combo_box_set_active (
          GTK_COMBO_BOX (window->time_interval_combobox),
          (gint)tkmv_settings_get_time_interval (settings));
    }

  gtk_range_set_range (
      GTK_RANGE (window->timestamp_scale),
      tkm_session_entry_get_first_timestamp (
          active_session, tkmv_settings_get_time_source (settings)),
      tkm_session_entry_get_last_timestamp (
          active_session, tkmv_settings_get_time_source (settings)));
}

static gboolean
update_views_content_invoke (gpointer _self)
{
  TkmvWindow *window = (TkmvWindow *)_self;
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());

  g_assert (window);

  tkmv_window_update_toolbar (window);
  tkmv_dashboard_view_update_content (window->dashboard_view);
  tkmv_systeminfo_reload_entries (window->systeminfo_view, context);
  tkmv_processes_reload_entries (window->processes_view, context);

  tkm_context_data_unlock (context);

  return FALSE;
}

void
tkmv_window_update_views_content (TkmvWindow *window)
{
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());

  g_assert (window);

  tkm_context_data_lock (context);
  g_main_context_invoke (NULL, update_views_content_invoke, window);
}

void
tkmv_window_request_update_data (TkmvWindow *window)
{
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  TkmSessionEntry *active_session = NULL;

  g_assert (window);

  if (sessions == NULL)
    return;

  if (sessions->len == 0)
    return;

  for (guint i = 0; i < sessions->len; i++)
    {
      if (tkm_session_entry_get_active (g_ptr_array_index (sessions, i)))
        {
          active_session = g_ptr_array_index (sessions, i);
        }
    }

  g_assert (active_session);

  tkmv_application_load_data (
      tkmv_application_instance (),
      tkm_session_entry_get_hash (active_session),
      gtk_range_get_value (GTK_RANGE (window->timestamp_scale)));
}
