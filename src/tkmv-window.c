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
static void tools_visible_child_changed (GObject *stack, GParamSpec *pspec,
                                         TkmvWindow *self);
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
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());
  TkmvWindow *self = (TkmvWindow *)user_data;

  g_assert (self);

  if (response == GTK_RESPONSE_ACCEPT)
    {
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
      g_autoptr (GListModel) files = gtk_file_chooser_get_files (chooser);
      GList *paths = NULL;

      for (guint i = 0; i < g_list_model_get_n_items (files); i++)
        {
          GFile *file = g_list_model_get_item (files, i);
          g_autoptr (TkmvSettingsRecentFile) rf
              = tkmv_settings_recent_file_new (g_file_get_basename (file),
                                               g_file_get_path (file));

          paths = g_list_append (paths, g_strdup (g_file_get_path (file)));
          tkmv_settings_add_recent_file (settings, rf);

          g_message ("File selected '%s'", g_file_get_path (file));

          g_object_unref (file);
        }

      if (paths != NULL)
        tkmv_application_open_files (tkmv_application_instance (), paths);
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
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

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
dltl_window_progress_spinner_start (TkmvWindow *window)
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
dltl_window_progress_spinner_stop (TkmvWindow *window)
{
  g_assert (window);
  g_main_context_invoke (NULL, window_progress_spinner_stop_invoke, window);
}
