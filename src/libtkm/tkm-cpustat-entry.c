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
 * \file tkm-cpustat-entry.c
 */

#include "tkm-cpustat-entry.h"

static const gchar *timeSourceColumn[]
    = { "SystemTime", "MonotonicTime", "ReceiveTime" };

/**
 * @enum CpuStat query type
 */
typedef enum _CpuStatQueryType
{
  CPUSTAT_GET_ENTRIES,
} CpuStatQueryType;

/**
 * @enum CpuStat query data object
 */
typedef struct _CpuStatQueryData
{
  CpuStatQueryType type;
  gpointer response;
} CpuStatQueryData;

static int cpustat_sqlite_callback (void *data, int argc, char **argv,
                                    char **colname);

TkmCpuStatEntry *
tkm_cpustat_entry_new (void)
{
  TkmCpuStatEntry *entry = g_new0 (TkmCpuStatEntry, 1);

  g_ref_count_init (&entry->rc);

  return entry;
}

TkmCpuStatEntry *
tkm_cpustat_entry_ref (TkmCpuStatEntry *entry)
{
  g_assert (entry);
  g_ref_count_inc (&entry->rc);
  return entry;
}

void
tkm_cpustat_entry_unref (TkmCpuStatEntry *entry)
{
  g_assert (entry);

  if (g_ref_count_dec (&entry->rc) == TRUE)
    {
      if (entry->name != NULL)
        g_free (entry->name);

      g_free (entry);
    }
}

guint
tkm_cpustat_entry_get_index (TkmCpuStatEntry *entry)
{
  g_assert (entry);
  return entry->idx;
}

void
tkm_cpustat_entry_set_index (TkmCpuStatEntry *entry, guint val)
{
  g_assert (entry);
  entry->idx = val;
}

const gchar *
tkm_cpustat_entry_get_name (TkmCpuStatEntry *entry)
{
  g_assert (entry);
  return entry->name;
}

void
tkm_cpustat_entry_set_name (TkmCpuStatEntry *entry, const gchar *name)
{
  g_assert (entry);

  if (entry->name != NULL)
    g_free (entry->name);

  entry->name = g_strdup (name);
}

gulong
tkm_cpustat_entry_get_timestamp (TkmCpuStatEntry *entry, DataTimeSource type)
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
tkm_cpustat_entry_set_timestamp (TkmCpuStatEntry *entry, DataTimeSource type,
                                 gulong val)
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

guint
tkm_cpustat_entry_get_all (TkmCpuStatEntry *entry)
{
  g_assert (entry);
  return entry->all;
}

void
tkm_cpustat_entry_set_all (TkmCpuStatEntry *entry, guint val)
{
  g_assert (entry);
  entry->all = val;
}

guint
tkm_cpustat_entry_get_sys (TkmCpuStatEntry *entry)
{
  g_assert (entry);
  return entry->sys;
}

void
tkm_cpustat_entry_set_sys (TkmCpuStatEntry *entry, guint val)
{
  g_assert (entry);
  entry->sys = val;
}

guint
tkm_cpustat_entry_get_usr (TkmCpuStatEntry *entry)
{
  g_assert (entry);
  return entry->usr;
}

void
tkm_cpustat_entry_set_usr (TkmCpuStatEntry *entry, guint val)
{
  g_assert (entry);
  entry->usr = val;
}

static int
cpustat_sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  CpuStatQueryData *querydata = (CpuStatQueryData *)data;

  switch (querydata->type)
    {
    case CPUSTAT_GET_ENTRIES:
      {
        GPtrArray **entries = (GPtrArray **)querydata->response;
        g_autoptr (TkmCpuStatEntry) entry = tkm_cpustat_entry_new ();

        for (gint i = 0; i < argc; i++)
          {
            if (g_strcmp0 (colname[i], "SystemTime") == 0)
              tkm_cpustat_entry_set_timestamp (
                  entry, DATA_TIME_SOURCE_SYSTEM,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "MonotonicTime") == 0)
              tkm_cpustat_entry_set_timestamp (
                  entry, DATA_TIME_SOURCE_MONOTONIC,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "ReceiveTime") == 0)
              tkm_cpustat_entry_set_timestamp (
                  entry, DATA_TIME_SOURCE_RECEIVE,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "CPUStatName") == 0)
              tkm_cpustat_entry_set_name (entry, argv[i]);
            else if (g_strcmp0 (colname[i], "SystemTime") == 0)
              tkm_cpustat_entry_set_timestamp (
                  entry, DATA_TIME_SOURCE_SYSTEM,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "MonotonicTime") == 0)
              tkm_cpustat_entry_set_timestamp (
                  entry, DATA_TIME_SOURCE_MONOTONIC,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "ReceiveTime") == 0)
              tkm_cpustat_entry_set_timestamp (
                  entry, DATA_TIME_SOURCE_RECEIVE,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "CPUStatAll") == 0)
              tkm_cpustat_entry_set_all (
                  entry, (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "CPUStatSys") == 0)
              tkm_cpustat_entry_set_sys (
                  entry, (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "CPUStatUsr") == 0)
              tkm_cpustat_entry_set_usr (
                  entry, (guint)g_ascii_strtoull (argv[i], NULL, 10));
          }

        g_ptr_array_add (*entries, tkm_cpustat_entry_ref (entry));
        break;
      }

    default:
      break;
    }

  return 0;
}

static void
cpustat_entry_free (gpointer data)
{
  TkmCpuStatEntry *e = (TkmCpuStatEntry *)data;

  g_assert (e);
  tkm_cpustat_entry_unref (e);
}

GPtrArray *
tkm_cpustat_entry_get_all_entries (sqlite3 *db, const char *session_hash,
                                   DataTimeSource time_source,
                                   gulong start_time, gulong end_time,
                                   GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  GPtrArray *entries = g_ptr_array_new ();
  CpuStatQueryData data
      = { .type = CPUSTAT_GET_ENTRIES, .response = (gpointer)&entries };

  g_ptr_array_set_free_func (entries, cpustat_entry_free);

  g_assert (db);

  sql = g_strdup_printf ("SELECT * FROM '%s' "
                         "WHERE %s >= %lu AND "
                         " %s < %lu AND SessionId IS "
                         "(SELECT Id FROM '%s' WHERE Hash IS '%s' LIMIT 1);",
                         TKM_CPUSTAT_TABLE_NAME, timeSourceColumn[time_source],
                         start_time, timeSourceColumn[time_source], end_time,
                         TKM_SESSIONS_TABLE_NAME, session_hash);
  if (sqlite3_exec (db, sql, cpustat_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_ptr_array_free (entries, TRUE);
      entries = NULL;

      g_set_error (error, g_quark_from_static_string ("CpuStatGetAll"), 1,
                   "SQL query error");
      g_warning ("Fail to get cpustat list. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return entries;
}
