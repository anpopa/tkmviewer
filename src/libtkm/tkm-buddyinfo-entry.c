/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019-2022 Alin Popa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \file tkm-buddyinfo-entry.c
 */

#include "tkm-buddyinfo-entry.h"

static const gchar *timeSourceColumn[]
    = { "SystemTime", "MonotonicTime", "ReceiveTime" };

/**
 * @enum BuddyInfo query type
 */
typedef enum _BuddyInfoQueryType
{
  BUDDYINFO_GET_ENTRIES,
} BuddyInfoQueryType;

/**
 * @enum BuddyInfo query data object
 */
typedef struct _BuddyInfoQueryData
{
  BuddyInfoQueryType type;
  gpointer response;
} BuddyInfoQueryData;

static int buddyinfo_sqlite_callback (void *data, int argc, char **argv,
                                      char **colname);

TkmBuddyInfoEntry *
tkm_buddyinfo_entry_new (void)
{
  TkmBuddyInfoEntry *entry = g_new0 (TkmBuddyInfoEntry, 1);

  g_ref_count_init (&entry->rc);

  return entry;
}

TkmBuddyInfoEntry *
tkm_buddyinfo_entry_ref (TkmBuddyInfoEntry *entry)
{
  g_assert (entry);
  g_ref_count_inc (&entry->rc);
  return entry;
}

void
tkm_buddyinfo_entry_unref (TkmBuddyInfoEntry *entry)
{
  g_assert (entry);

  if (g_ref_count_dec (&entry->rc) == TRUE)
    {
      if (entry->name != NULL)
        g_free (entry->name);
      if (entry->zone != NULL)
        g_free (entry->zone);
      if (entry->data != NULL)
        g_free (entry->data);

      g_free (entry);
    }
}

guint
tkm_buddyinfo_entry_get_index (TkmBuddyInfoEntry *entry)
{
  g_assert (entry);
  return entry->idx;
}

void
tkm_buddyinfo_entry_set_index (TkmBuddyInfoEntry *entry, guint val)
{
  g_assert (entry);
  entry->idx = val;
}

const gchar *
tkm_buddyinfo_entry_get_name (TkmBuddyInfoEntry *entry)
{
  g_assert (entry);
  return entry->name;
}

void
tkm_buddyinfo_entry_set_name (TkmBuddyInfoEntry *entry, const gchar *name)
{
  g_assert (entry);

  if (entry->name != NULL)
    g_free (entry->name);

  entry->name = g_strdup (name);
}

gulong
tkm_buddyinfo_entry_get_timestamp (TkmBuddyInfoEntry *entry,
                                   DataTimeSource type)
{
  g_assert (entry);
  switch (type)
    {
    case DATA_TIME_SOURCE_SYSTEM:
      return entry->system_time;
    case DATA_TIME_SOURCE_MONOTONIC:
      return entry->monotonic_time;
    default:
      break;
    }

  return entry->receive_time;
}

void
tkm_buddyinfo_entry_set_timestamp (TkmBuddyInfoEntry *entry,
                                   DataTimeSource type, gulong val)
{
  g_assert (entry);
  switch (type)
    {
    case DATA_TIME_SOURCE_SYSTEM:
      entry->system_time = val;
      break;
    case DATA_TIME_SOURCE_MONOTONIC:
      entry->monotonic_time = val;
      break;
    case DATA_TIME_SOURCE_RECEIVE:
      entry->receive_time = val;
      break;
    default:
      break;
    }
}

const gchar *
tkm_buddyinfo_entry_get_zone (TkmBuddyInfoEntry *entry)
{
  g_assert (entry);
  return entry->zone;
}

void
tkm_buddyinfo_entry_set_zone (TkmBuddyInfoEntry *entry, const gchar *zone)
{
  g_assert (entry);

  if (entry->zone != NULL)
    g_free (entry->zone);

  entry->zone = g_strdup (zone);
}

const gchar *
tkm_buddyinfo_entry_get_data (TkmBuddyInfoEntry *entry)
{
  g_assert (entry);
  return entry->data;
}

void
tkm_buddyinfo_entry_set_data (TkmBuddyInfoEntry *entry, const gchar *data)
{
  g_assert (entry);

  if (entry->data != NULL)
    g_free (entry->data);

  entry->data = g_strdup (data);
}

static int
buddyinfo_sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  BuddyInfoQueryData *querydata = (BuddyInfoQueryData *)data;

  switch (querydata->type)
    {
    case BUDDYINFO_GET_ENTRIES:
      {
        GPtrArray **entries = (GPtrArray **)querydata->response;
        g_autoptr (TkmBuddyInfoEntry) entry = tkm_buddyinfo_entry_new ();

        for (gint i = 0; i < argc; i++)
          {
            if (g_strcmp0 (colname[i], "Name") == 0)
              tkm_buddyinfo_entry_set_name (entry, argv[i]);
            else if (g_strcmp0 (colname[i], "Zone") == 0)
              tkm_buddyinfo_entry_set_zone (entry, argv[i]);
            else if (g_strcmp0 (colname[i], "Data") == 0)
              tkm_buddyinfo_entry_set_data (entry, argv[i]);
          }

        g_ptr_array_add (*entries, tkm_buddyinfo_entry_ref (entry));
        break;
      }

    default:
      break;
    }

  return 0;
}

static void
buddyinfo_entry_free (gpointer data)
{
  TkmBuddyInfoEntry *e = (TkmBuddyInfoEntry *)data;

  g_assert (e);
  tkm_buddyinfo_entry_unref (e);
}

GPtrArray *
tkm_buddyinfo_entry_get_all_entries (sqlite3 *db, const char *session_hash,
                                     DataTimeSource time_source,
                                     gulong start_time, gulong end_time,
                                     GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  GPtrArray *entries = g_ptr_array_new ();
  BuddyInfoQueryData data
      = { .type = BUDDYINFO_GET_ENTRIES, .response = (gpointer)&entries };

  g_ptr_array_set_free_func (entries, buddyinfo_entry_free);

  g_assert (db);

  sql = g_strdup_printf ("SELECT * FROM '%s' "
                         "WHERE %s >= %lu AND "
                         " %s < %lu AND SessionId IS "
                         "(SELECT Id FROM '%s' WHERE Hash IS '%s' LIMIT 1);",
                         TKM_BUDDYINFO_TABLE_NAME,
                         timeSourceColumn[time_source], start_time,
                         timeSourceColumn[time_source], end_time,
                         TKM_SESSIONS_TABLE_NAME, session_hash);
  if (sqlite3_exec (db, sql, buddyinfo_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_ptr_array_free (entries, TRUE);
      entries = NULL;

      g_set_error (error, g_quark_from_static_string ("BuddyInfoGetAll"), 1,
                   "SQL query error");
      g_warning ("Fail to get buddyinfo list. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return entries;
}
