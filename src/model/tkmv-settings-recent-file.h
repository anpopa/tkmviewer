/* settings-recent-file.h
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

#include <glib.h>

G_BEGIN_DECLS

typedef struct _RecentFileLookup
{
  const gchar *name;
  const gchar *path;
  gboolean found;
  guint index;
} RecentFileLookup;

typedef struct _TkmvSettingsRecentFile
{
  gchar *name;
  gchar *path;
  guint idx;
  grefcount rc;
} TkmvSettingsRecentFile;

TkmvSettingsRecentFile *tkmv_settings_recent_file_new (const gchar *name,
                                                       const gchar *path);
TkmvSettingsRecentFile *
tkmv_settings_recent_file_ref (TkmvSettingsRecentFile *rf);
void tkmv_settings_recent_file_unref (TkmvSettingsRecentFile *rf);
void tkmv_settings_recent_file_set_index (TkmvSettingsRecentFile *rf,
                                          guint idx);
const gchar *tkmv_settings_recent_file_get_name (TkmvSettingsRecentFile *rf);
const gchar *tkmv_settings_recent_file_get_path (TkmvSettingsRecentFile *rf);
guint tkmv_settings_recent_file_get_index (TkmvSettingsRecentFile *rf);
gboolean tkmv_settings_recent_file_valid (TkmvSettingsRecentFile *rf);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmvSettingsRecentFile,
                               tkmv_settings_recent_file_unref);

G_END_DECLS
