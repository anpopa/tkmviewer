/* tkmv-settings.c
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

#include "tkmv-settings.h"
#include "tkmv-types.h"
#include <limits.h>

static void
load_recent_files (TkmvSettings *tkms)
{
  g_autofree gchar *setstr_base = g_settings_get_string (tkms->gsettings, "recent-files");
  g_autofree guchar *setstr = NULL;
  gsize file_count = 0;
  gsize len;

  setstr = g_base64_decode (setstr_base, &len);
  if (setstr != NULL)
    {
      gchar **file_pairs = g_strsplit ((gchar *)setstr, "$FILE$", -1);

      for (gint i = 0; file_pairs[i]; i++)
        {
          gchar **fields = g_strsplit (file_pairs[i], "$KVP$", -1);

          for (gint j = 0; fields[j]; j++)
            {
              g_autoptr (TkmvSettingsRecentFile) file = tkmv_settings_recent_file_new (fields[0], fields[1]);

              if (tkmv_settings_recent_file_valid (file))
                {
                  tkmv_settings_add_recent_file (tkms, file);
                  file_count++;
                }
            }
          g_strfreev (fields);
        }
      g_strfreev (file_pairs);
    }

  TKM_UNUSED (len);
  if ((len) > 0 && (file_count == 0))
    {
      g_warning ("Recent files string is invalid. Reset recent files encoded string");
      g_settings_set_string (tkms->gsettings, "recent-files", "");
    }
}

static void
add_files_to_encoding (gpointer _rf, gpointer _str)
{
  g_autofree gchar *filestr = NULL;
  TkmvSettingsRecentFile *rf = (TkmvSettingsRecentFile *)_rf;
  gchar *str = (gchar *)_str;

  g_assert (rf);
  g_assert (str);

  filestr = g_strdup_printf (
    "%s$KVP$%s", tkmv_settings_recent_file_get_name (rf), tkmv_settings_recent_file_get_path (rf));

  if (strlen (str) > 0)
    {
      g_autofree gchar *oldstr = g_strdup (str);
      g_snprintf (str, NAME_MAX * PATH_MAX * RECENT_FILES_MAX, "%s$FILE$%s", oldstr, filestr);
    }
  else
    g_snprintf (str, NAME_MAX * PATH_MAX * RECENT_FILES_MAX, "%s", filestr);
}

static void
save_recent_files (TkmvSettings *tkms)
{
  g_autofree gchar *setstr_base = NULL;
  g_autofree gchar *setstr = NULL;

  g_assert (tkms);

  setstr = g_new0 (gchar, NAME_MAX * PATH_MAX * RECENT_FILES_MAX);
  g_list_foreach (tkms->recent_files, add_files_to_encoding, setstr);

  setstr_base = g_base64_encode ((guchar *)setstr, strlen (setstr));
  g_settings_set_string (tkms->gsettings, "recent-files", setstr_base);
}

TkmvSettings *
tkmv_settings_new (void)
{
  TkmvSettings *tkms = g_new0 (TkmvSettings, 1);

  tkms->gsettings = g_settings_new ("ro.fxdata.taskmonitor.viewer");
  g_ref_count_init (&tkms->rc);

  load_recent_files (tkms);
  tkmv_settings_load_general_settings (tkms);

  return tkms;
}

TkmvSettings *
tkmv_settings_ref (TkmvSettings *tkms)
{
  g_assert (tkms);
  g_ref_count_inc (&tkms->rc);
  return tkms;
}

static void
recent_files_free (gpointer data)
{
  TkmvSettingsRecentFile *rf = (TkmvSettingsRecentFile *)data;

  g_assert (rf);
  tkmv_settings_recent_file_unref (rf);
}

void
tkmv_settings_unref (TkmvSettings *tkms)
{
  g_assert (tkms);

  if (g_ref_count_dec (&tkms->rc) == TRUE)
    {
      if (tkms->gsettings != NULL)
        g_object_unref (tkms->gsettings);

      if (tkms->recent_files != NULL)
        g_list_free_full (tkms->recent_files, recent_files_free);

      g_free (tkms);
    }
}

void
tkmv_settings_get_main_window_size (TkmvSettings *tkms, gint *width, gint *height)
{
  g_assert (tkms);
  g_assert (width);
  g_assert (height);

  *width = g_settings_get_int (tkms->gsettings, "window-width");
  *height = g_settings_get_int (tkms->gsettings, "window-height");
}

void
tkmv_settings_set_main_window_size (TkmvSettings *tkms, gint width, gint height)
{
  g_assert (tkms);
  g_settings_set_int (tkms->gsettings, "window-width", width);
  g_settings_set_int (tkms->gsettings, "window-height", height);
}

static void
recent_files_lookup (gpointer _rf, gpointer _lookup)
{
  TkmvSettingsRecentFile *rf = (TkmvSettingsRecentFile *)_rf;
  RecentFileLookup *lookup = (RecentFileLookup *)_lookup;

  if (g_strcmp0 (tkmv_settings_recent_file_get_name (rf), lookup->name) == 0)
    lookup->found = TRUE;

  lookup->index++;
}

void
tkmv_settings_add_recent_file (TkmvSettings *tkms, TkmvSettingsRecentFile *rf)
{
  RecentFileLookup lookup
    = { .name = tkmv_settings_recent_file_get_name (rf), .found = FALSE, .index = 0 };
  GList *first = NULL;

  g_assert (tkms);
  g_assert (rf);

  if (!tkmv_settings_recent_file_valid (rf))
    return;

  g_list_foreach (tkms->recent_files, recent_files_lookup, &lookup);
  if (lookup.found)
    return;

  g_assert (tkms->recent_files_count == lookup.index);

  first = g_list_first (tkms->recent_files);
  if (tkms->recent_files_count > 3)
    {
      tkmv_settings_recent_file_unref ((TkmvSettingsRecentFile *)first->data);
      tkms->recent_files = g_list_remove (tkms->recent_files, first);
      tkms->recent_files_count--;
    }

  tkmv_settings_recent_file_set_index (rf, tkms->recent_files_count);
  tkms->recent_files = g_list_append (tkms->recent_files, tkmv_settings_recent_file_ref (rf));
  tkms->recent_files_count++;
}

gsize
tkmv_settings_recent_files_count (TkmvSettings *tkms)
{
  g_assert (tkms);
  return tkms->recent_files_count;
}

GList *
tkmv_settings_get_recent_files (TkmvSettings *tkms)
{
  g_assert (tkms);
  return tkms->recent_files;
}

void
tkmv_settings_load_general_settings (TkmvSettings *tkms)
{
  g_assert (tkms);
}

void
tkmv_settings_store_general_settings (TkmvSettings *tkms)
{
  g_assert (tkms);
}

void
tkmv_settings_save (TkmvSettings *tkms)
{
  g_assert (tkms);
  save_recent_files (tkms);
  tkmv_settings_store_general_settings (tkms);
}
