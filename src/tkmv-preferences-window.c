/* tkmv-preferences-window.c
 *
 * Copyright 2021-2022 Alin Popa
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

#include "tkmv-preferences-window.h"
#include "tkmv-types.h"

#include "tkmv-application.h"
#include "tkmv-settings.h"

struct _TkmvPreferencesWindow {
  AdwPreferencesWindow parent_instance;

  /* General */
};

G_DEFINE_TYPE (TkmvPreferencesWindow, tkmv_preferences_window,
               ADW_TYPE_PREFERENCES_WINDOW)

static void
tkmv_preferences_window_finalize (GObject *object)
{
  TkmvSettings *settings = tkmv_application_get_settings (
    tkmv_application_instance ());

  tkmv_settings_save (settings);

  G_OBJECT_CLASS (tkmv_preferences_window_parent_class)->finalize (object);
}

static void
tkmv_preferences_window_class_init (TkmvPreferencesWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = tkmv_preferences_window_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/ro/fxdata/taskmonitor/viewer/gtk/tkmv-preferences-window.ui");
}

static void
tkmv_preferences_window_load_settings (TkmvPreferencesWindow *self)
{
  TkmvSettings *settings = tkmv_application_get_settings (
    tkmv_application_instance ());

  g_assert (self);

  tkmv_settings_load_general_settings (settings);
}

static void
tkmv_preferences_window_init (TkmvPreferencesWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  tkmv_preferences_window_load_settings (self);
}
