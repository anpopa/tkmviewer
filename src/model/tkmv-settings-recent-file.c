/* settings-recent-file.c
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

#include "tkmv-settings-recent-file.h"

TkmvSettingsRecentFile *
tkmv_settings_recent_file_new (const gchar *name, const gchar *path)
{
  TkmvSettingsRecentFile *rf = g_new0 (TkmvSettingsRecentFile, 1);

  rf->name = g_strdup (name);
  rf->path = g_strdup (path);
  g_ref_count_init (&rf->rc);

  return rf;
}

TkmvSettingsRecentFile *
tkmv_settings_recent_file_ref (TkmvSettingsRecentFile *rf)
{
  g_assert (rf);
  g_ref_count_inc (&rf->rc);
  return rf;
}

void
tkmv_settings_recent_file_unref (TkmvSettingsRecentFile *rf)
{
  g_assert (rf);

  if (g_ref_count_dec (&rf->rc) == TRUE)
    {
      if (rf->name != NULL)
        g_free (rf->name);

      if (rf->path != NULL)
        g_free (rf->path);

      g_free (rf);
    }
}

const gchar *
tkmv_settings_recent_file_get_name (TkmvSettingsRecentFile *rf)
{
  g_assert (rf);
  return rf->name;
}
const gchar *
tkmv_settings_recent_file_get_path (TkmvSettingsRecentFile *rf)
{
  g_assert (rf);
  return rf->path;
}

gboolean
tkmv_settings_recent_file_valid (TkmvSettingsRecentFile *rf)
{
  g_assert (rf);

  if (g_file_test (rf->path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
    return TRUE;

  return FALSE;
}

guint
tkmv_settings_recent_file_get_index (TkmvSettingsRecentFile *rf)
{
  g_assert (rf);
  return rf->idx;
}

void
tkmv_settings_recent_file_set_index (TkmvSettingsRecentFile *rf, guint idx)
{
  g_assert (rf);
  rf->idx = idx;
}
