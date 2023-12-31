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
 * \file tkm-session-entry.c
 */

#include "tkm-session-entry.h"

/**
 * @enum Session query type
 */
typedef enum _SessionQueryType {
  SESSION_GET_ENTRIES,
  SESSION_GET_TIME_INTERVALS,
  SESSION_GET_DEVICE_DATA,
} SessionQueryType;

/**
 * @enum Session query data object
 */
typedef struct _SessionQueryData {
  SessionQueryType type;
  gpointer response;
} SessionQueryData;

static int session_sqlite_callback (void *data, int argc, char **argv,
                                    char **colname);

TkmSessionEntry *
tkm_session_entry_new (void)
{
  TkmSessionEntry *entry = g_new0 (TkmSessionEntry, 1);

  entry->active = FALSE;
  entry->device_cpus = 1;
  g_ref_count_init (&entry->rc);

  return entry;
}

TkmSessionEntry *
tkm_session_entry_ref (TkmSessionEntry *entry)
{
  g_assert (entry);
  g_ref_count_inc (&entry->rc);
  return entry;
}

void
tkm_session_entry_unref (TkmSessionEntry *entry)
{
  g_assert (entry);

  if (g_ref_count_dec (&entry->rc) == TRUE)
    {
      if (entry->hash != NULL)
        free (entry->hash);

      if (entry->name != NULL)
        free (entry->name);

      g_free (entry);
    }
}

guint
tkm_session_entry_get_index (TkmSessionEntry *entry)
{
  g_assert (entry);
  return entry->idx;
}

void
tkm_session_entry_set_index (TkmSessionEntry *entry, guint val)
{
  g_assert (entry);
  entry->idx = val;
}

const gchar *
tkm_session_entry_get_hash (TkmSessionEntry *entry)
{
  g_assert (entry);
  return entry->hash;
}

void
tkm_session_entry_set_hash (TkmSessionEntry *entry, const gchar *hash)
{
  g_assert (entry);
  g_assert (hash);
  entry->hash = g_strdup (hash);
}

const gchar *
tkm_session_entry_get_name (TkmSessionEntry *entry)
{
  g_assert (entry);
  return entry->name;
}

void
tkm_session_entry_set_name (TkmSessionEntry *entry, const gchar *name)
{
  g_assert (entry);
  g_assert (name);
  entry->name = g_strdup (name);
}

const gchar *
tkm_session_entry_get_device_name (TkmSessionEntry *entry)
{
  g_assert (entry);
  return entry->device_name;
}

void
tkm_session_entry_set_device_name (TkmSessionEntry *entry, const gchar *name)
{
  g_assert (entry);
  g_assert (name);
  entry->device_name = g_strdup (name);
}

guint
tkm_session_entry_get_device_cpus (TkmSessionEntry *entry)
{
  g_assert (entry);
  return entry->device_cpus;
}

void
tkm_session_entry_set_device_cpus (TkmSessionEntry *entry, guint cpus)
{
  g_assert (entry);
  entry->device_cpus = cpus;
}

guint
tkm_session_entry_get_first_timestamp (TkmSessionEntry *entry,
                                       DataTimeSource type)
{
  g_assert (entry);
  switch (type)
    {
    case DATA_TIME_SOURCE_SYSTEM:
      return entry->first_system_ts;

    case DATA_TIME_SOURCE_MONOTONIC:
      return entry->first_monotonic_ts;

    default:
      break;
    }

  return entry->first_receive_ts;
}

void
tkm_session_entry_set_first_timestamp (TkmSessionEntry *entry,
                                       DataTimeSource type, guint ts)
{
  g_assert (entry);
  switch (type)
    {
    case DATA_TIME_SOURCE_SYSTEM:
      entry->first_system_ts = ts;
      break;

    case DATA_TIME_SOURCE_MONOTONIC:
      entry->first_monotonic_ts = ts;
      break;

    case DATA_TIME_SOURCE_RECEIVE:
      entry->first_receive_ts = ts;
      break;

    default:
      break;
    }
}

guint
tkm_session_entry_get_last_timestamp (TkmSessionEntry *entry,
                                      DataTimeSource type)
{
  g_assert (entry);
  switch (type)
    {
    case DATA_TIME_SOURCE_SYSTEM:
      return entry->last_system_ts;

    case DATA_TIME_SOURCE_MONOTONIC:
      return entry->last_monotonic_ts;

    default:
      break;
    }

  return entry->last_receive_ts;
}

void
tkm_session_entry_set_last_timestamp (TkmSessionEntry *entry,
                                      DataTimeSource type, guint ts)
{
  g_assert (entry);
  switch (type)
    {
    case DATA_TIME_SOURCE_SYSTEM:
      entry->last_system_ts = ts;
      break;

    case DATA_TIME_SOURCE_MONOTONIC:
      entry->last_monotonic_ts = ts;
      break;

    case DATA_TIME_SOURCE_RECEIVE:
      entry->last_receive_ts = ts;
      break;

    default:
      break;
    }
}

void
tkm_session_entry_set_active (TkmSessionEntry *entry, gboolean state)
{
  g_assert (entry);
  entry->active = state;
}

gboolean
tkm_session_entry_get_active (TkmSessionEntry *entry)
{
  g_assert (entry);
  return entry->active;
}

static int
session_sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  SessionQueryData *querydata = (SessionQueryData *)data;

  switch (querydata->type)
    {
    case SESSION_GET_ENTRIES:
    {
      GPtrArray **entries = (GPtrArray **)querydata->response;
      g_autoptr (TkmSessionEntry) entry = tkm_session_entry_new ();
      gboolean valid = TRUE;

      for (gint i = 0; i < argc && valid; i++)
        {
          if (argv[i] == NULL)
            {
              valid = FALSE;
              continue;
            }

          if (g_strcmp0 (colname[i], "Name") == 0)
            tkm_session_entry_set_name (entry, argv[i]);
          else if (g_strcmp0 (colname[i], "Hash") == 0)
            tkm_session_entry_set_hash (entry, argv[i]);
          else if (g_strcmp0 (colname[i], "CoreCount") == 0)
            tkm_session_entry_set_device_cpus (
              entry, (guint)g_ascii_strtoull (argv[i], NULL, 10));
        }

      if (valid)
        g_ptr_array_add (*entries, tkm_session_entry_ref (entry));

      break;
    }

    case SESSION_GET_TIME_INTERVALS:
    {
      TkmSessionEntry *entry = (TkmSessionEntry *)querydata->response;

      for (gint i = 0; i < argc; i++)
        {
          if (argv[i] == NULL)
            continue;

          if (g_strcmp0 (colname[i], "MinSysTime") == 0)
            tkm_session_entry_set_first_timestamp (
              entry, DATA_TIME_SOURCE_SYSTEM,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MinMonTime") == 0)
            tkm_session_entry_set_first_timestamp (
              entry, DATA_TIME_SOURCE_MONOTONIC,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MinRecTime") == 0)
            tkm_session_entry_set_first_timestamp (
              entry, DATA_TIME_SOURCE_RECEIVE,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MaxSysTime") == 0)
            tkm_session_entry_set_last_timestamp (
              entry, DATA_TIME_SOURCE_SYSTEM,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MaxMonTime") == 0)
            tkm_session_entry_set_last_timestamp (
              entry, DATA_TIME_SOURCE_MONOTONIC,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MaxRecTime") == 0)
            tkm_session_entry_set_last_timestamp (
              entry, DATA_TIME_SOURCE_RECEIVE,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
        }

      break;
    }

    case SESSION_GET_DEVICE_DATA:
    {
      TkmSessionEntry *entry = (TkmSessionEntry *)querydata->response;

      for (gint i = 0; i < argc; i++)
        {
          if (argv[i] == NULL)
            continue;

          if (g_strcmp0 (colname[i], "Name") == 0)
            tkm_session_entry_set_device_name (entry, argv[i]);
        }

      break;
    }

    default:
      break;
    }

  return 0;
}

static void
session_entry_free (gpointer data)
{
  TkmSessionEntry *e = (TkmSessionEntry *)data;

  g_assert (e);
  tkm_session_entry_unref (e);
}

static void
update_time_intervals (gpointer _entry, gpointer _db)
{
  sqlite3 *db = (sqlite3 *)_db;
  TkmSessionEntry *entry = (TkmSessionEntry *)_entry;
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  SessionQueryData data
    = { .type = SESSION_GET_TIME_INTERVALS, .response = entry };

  g_assert (db);

  sql = g_strdup_printf (
    "SELECT "
    "MIN(SystemTime) AS 'MinSysTime',"
    "MIN(MonotonicTime) AS 'MinMonTime',"
    "MIN(ReceiveTime) AS 'MinRecTime',"
    "MAX(SystemTime) AS 'MaxSysTime',"
    "MAX(MonotonicTime) AS 'MaxMonTime',"
    "MAX(ReceiveTime) AS 'MaxRecTime' "
    "FROM '%s' "
    "WHERE SessionId IS (SELECT Id FROM '%s' WHERE Hash IS '%s' LIMIT 1);",
    TKM_CPUSTAT_TABLE_NAME, TKM_SESSIONS_TABLE_NAME,
    tkm_session_entry_get_hash (entry));
  if (sqlite3_exec (db, sql, session_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_warning (
        "Fail to update time intervals for session '%s'. SQL error %s",
        tkm_session_entry_get_name (entry), query_error);
      sqlite3_free (query_error);
    }
}

static void
update_device_data (gpointer _entry, gpointer _db)
{
  sqlite3 *db = (sqlite3 *)_db;
  TkmSessionEntry *entry = (TkmSessionEntry *)_entry;
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  SessionQueryData data
    = { .type = SESSION_GET_DEVICE_DATA, .response = entry };

  g_assert (db);

  sql = g_strdup_printf (
    "SELECT Name "
    "FROM '%s' "
    "WHERE Id IS (SELECT Device FROM '%s' WHERE Hash IS '%s' LIMIT 1);",
    TKM_DEVICES_TABLE_NAME, TKM_SESSIONS_TABLE_NAME,
    tkm_session_entry_get_hash (entry));
  if (sqlite3_exec (db, sql, session_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_warning ("Fail to update device data for session '%s'. SQL error %s",
                 tkm_session_entry_get_name (entry), query_error);
      sqlite3_free (query_error);
    }
}

GPtrArray *
tkm_session_entry_get_all_entries (sqlite3 *db, GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  GPtrArray *entries = g_ptr_array_new ();
  SessionQueryData data
    = { .type = SESSION_GET_ENTRIES, .response = (gpointer) & entries };

  g_ptr_array_set_free_func (entries, session_entry_free);

  g_assert (db);

  sql = g_strdup_printf ("SELECT * FROM %s", TKM_SESSIONS_TABLE_NAME);
  if (sqlite3_exec (db, sql, session_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_ptr_array_free (entries, TRUE);
      entries = NULL;

      g_set_error (error, g_quark_from_static_string ("SessionGetAll"), 1,
                   "SQL query error");
      g_warning ("Fail to get sessions. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  if (entries != NULL)
    {
      g_ptr_array_foreach (entries, update_time_intervals, db);
      g_ptr_array_foreach (entries, update_device_data, db);
    }

  return entries;
}
