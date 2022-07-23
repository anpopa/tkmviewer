/* tkmv-application.c
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

#include "tkmv-application.h"
#include "tkmv-preferences-window.h"
#include "tkmv-settings.h"
#include "tkmv-types.h"
#include "tkmv-window.h"

static TkmvApplication *tkmv_application_singleton = NULL;

struct _TkmvApplication
{
  GtkApplication parent_instance;

  /* Application settings */
  TkmvSettings *settings;

  /* Main window */
  TkmvWindow *main_window;
};

G_DEFINE_TYPE (TkmvApplication, tkmv_application, ADW_TYPE_APPLICATION)

TkmvApplication *
tkmv_application_new (gchar *application_id, GApplicationFlags flags)
{
  return g_object_new (TKMV_TYPE_APPLICATION, "application-id", application_id,
                       "flags", flags, NULL);
}

static void
tkmv_application_finalize (GObject *object)
{
  TkmvApplication *self = (TkmvApplication *)object;

  tkmv_settings_save (self->settings);
  tkmv_settings_unref (self->settings);

  G_OBJECT_CLASS (tkmv_application_parent_class)->finalize (object);
  tkmv_application_singleton = NULL;
}

static void
tkmv_application_startup (GApplication *application)
{
  G_APPLICATION_CLASS (tkmv_application_parent_class)->startup (application);
}

static void
tkmv_application_activate (GApplication *app)
{
  TkmvApplication *self = TKMV_APPLICATION (app);

  /* It's good practice to check your parameters at the beginning of the
   * function. It helps catch errors early and in development instead of
   * by your users.
   */
  g_assert (GTK_IS_APPLICATION (app));

  /* Get the current window or create one if necessary. */
  self->main_window = TKMV_WINDOW (
      gtk_application_get_active_window (GTK_APPLICATION (app)));
  if (self->main_window == NULL)
    self->main_window
        = g_object_new (TKMV_TYPE_WINDOW, "application", app, NULL);

  /* Ask the window manager/compositor to present the window. */
  gtk_window_present (GTK_WINDOW (self->main_window));
}

static void
tkmv_application_class_init (TkmvApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  object_class->finalize = tkmv_application_finalize;
  app_class->startup = tkmv_application_startup;

  /*
   * We connect to the activate callback to create a window when the
   * application has been launched. Additionally, this callback notifies us
   * when the user tries to launch a "second instance" of the application. When
   * they try to do that, we'll just present any existing window.
   */
  app_class->activate = tkmv_application_activate;
}

static void
tkmv_application_show_about (GSimpleAction *action, GVariant *parameter,
                             gpointer user_data)
{
  TkmvApplication *self = TKMV_APPLICATION (user_data);
  GtkWindow *window = NULL;
  const gchar *authors[] = { "Alin Popa <alin.popa@fxdata.ro>", NULL };
  GtkWidget *image = gtk_image_new_from_resource (
      "/ro/fxdata/taskmonitor/viewer/assets/icons/scalable/application/"
      "tkmviewer-icon.svg");

  TKM_UNUSED (action);
  TKM_UNUSED (parameter);
  g_return_if_fail (TKMV_IS_APPLICATION (self));

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  gtk_show_about_dialog (window, "program-name", "TkmViewer", "authors",
                         authors, "website", "http://www.fxdata.ro", "version",
                         "0.9.0", "comments", "Taskmonitor Viewer",
                         "license_type", GTK_LICENSE_GPL_3_0, "logo",
                         gtk_image_get_paintable (GTK_IMAGE (image)), NULL);
}

static void
tkmv_application_show_preferences (GSimpleAction *action, GVariant *parameter,
                                   gpointer user_data)
{
  TkmvApplication *self = TKMV_APPLICATION (user_data);
  TkmvPreferencesWindow *preferences = NULL;
  GtkWindow *window = NULL;

  TKM_UNUSED (action);
  TKM_UNUSED (parameter);
  g_return_if_fail (TKMV_IS_APPLICATION (self));

  window = gtk_application_get_active_window (GTK_APPLICATION (self));
  preferences = g_object_new (TKMV_TYPE_PREFERENCES_WINDOW, NULL);

  gtk_window_set_transient_for (GTK_WINDOW (preferences), window);
  gtk_window_present (GTK_WINDOW (preferences));
}

static void
recent_files_lookup (gpointer _rf, gpointer _lookup)
{
  TkmvSettingsRecentFile *rf = (TkmvSettingsRecentFile *)_rf;
  RecentFileLookup *lookup = (RecentFileLookup *)_lookup;

  if (tkmv_settings_recent_file_get_index (rf) == lookup->index)
    {
      lookup->found = TRUE;
      lookup->path = tkmv_settings_recent_file_get_path (rf);
    }
}

static void
tkmv_application_init (TkmvApplication *self)
{
  self->settings = tkmv_settings_new ();

  g_autoptr (GSimpleAction) quit_action = g_simple_action_new ("quit", NULL);
  g_signal_connect_swapped (quit_action, "activate",
                            G_CALLBACK (g_application_quit), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (quit_action));

  g_autoptr (GSimpleAction) about_action = g_simple_action_new ("about", NULL);
  g_signal_connect (about_action, "activate",
                    G_CALLBACK (tkmv_application_show_about), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (about_action));

  GSimpleAction *preferences_action
      = g_simple_action_new ("preferences", NULL);
  g_signal_connect (preferences_action, "activate",
                    G_CALLBACK (tkmv_application_show_preferences), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (preferences_action));

  gtk_application_set_accels_for_action (GTK_APPLICATION (self), "app.quit",
                                         (const char *[]){
                                             "<primary>q",
                                             NULL,
                                         });
  /* Set our singletone instance */
  tkmv_application_singleton = self;
}

TkmvApplication *
tkmv_application_instance (void)
{
  return tkmv_application_singleton;
}

TkmvWindow *
tkmv_application_get_main_window (TkmvApplication *app)
{
  g_assert (app);
  return app->main_window;
}

TkmvSettings *
tkmv_application_get_settings (TkmvApplication *app)
{
  g_assert (app);
  return app->settings;
}

void
tkmv_application_open_files (TkmvApplication *app, GList *paths)
{
  g_assert (app);
  g_assert (paths);
}
