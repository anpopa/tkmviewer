/* tkmv-settings.h
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

#pragma once

#include <gio/gio.h>
#include <glib.h>

#include "tkm-settings.h"
#include "tkmv-settings-recent-file.h"

G_BEGIN_DECLS

#define RECENT_FILES_MAX 5

typedef struct _TkmvSettings {
  GSettings *gsettings;
  TkmSettings *tkm_settings;

  GList *recent_files;
  gsize recent_files_count;
  gboolean auto_timeline_refresh;

  grefcount rc;
} TkmvSettings;

TkmvSettings *tkmv_settings_new (void);
TkmvSettings *tkmv_settings_ref (TkmvSettings *tkms);
void tkmv_settings_unref (TkmvSettings *tkms);

void tkmv_settings_save (TkmvSettings *tkms);
TkmSettings *tkmv_settings_get_tkm_settings (TkmvSettings *tkms);

void tkmv_settings_get_main_window_size (TkmvSettings *tkms, gint *width,
                                         gint *height);
void tkmv_settings_set_main_window_size (TkmvSettings *tkms, gint width,
                                         gint height);

GList *tkmv_settings_get_recent_files (TkmvSettings *tkms);
void tkmv_settings_add_recent_file (TkmvSettings *tkms,
                                    TkmvSettingsRecentFile *rf);
gsize tkmv_settings_recent_files_count (TkmvSettings *tkms);

DataTimeSource tkmv_settings_get_time_source (TkmvSettings *tkms);
void tkmv_settings_set_time_source (TkmvSettings *tkms, DataTimeSource ts);
DataTimeInterval tkmv_settings_get_time_interval (TkmvSettings *tkms);
void tkmv_settings_set_time_interval (TkmvSettings *tkms, DataTimeInterval ti);
gboolean tkmv_settings_get_auto_timeline_refresh (TkmvSettings *tkms);
void tkmv_settings_set_auto_timeline_refresh (TkmvSettings *tkms,
                                              gboolean state);

void tkmv_settings_load_general_settings (TkmvSettings *tkms);
void tkmv_settings_store_general_settings (TkmvSettings *tkms);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmvSettings, tkmv_settings_unref);

G_END_DECLS
