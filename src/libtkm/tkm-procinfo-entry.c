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
 * \file tkm-procinfo-entry.c
 */

#include "tkm-procinfo-entry.h"

static const gchar *timeSourceColumn[]
  = { "SystemTime", "MonotonicTime", "ReceiveTime" };

/**
 * @enum ProcInfo query type
 */
typedef enum _ProcInfoQueryType {
  PROCINFO_GET_ENTRIES,
} ProcInfoQueryType;

/**
 * @enum ProcInfo query data object
 */
typedef struct _ProcInfoQueryData {
  ProcInfoQueryType type;
  gpointer response;
} ProcInfoQueryData;

static int procinfo_sqlite_callback (void *data, int argc, char **argv,
                                     char **colname);

TkmProcInfoEntry *
tkm_procinfo_entry_new (void)
{
  TkmProcInfoEntry *entry = g_new0 (TkmProcInfoEntry, 1);

  g_ref_count_init (&entry->rc);

  return entry;
}

TkmProcInfoEntry *
tkm_procinfo_entry_ref (TkmProcInfoEntry *entry)
{
  g_assert (entry);
  g_ref_count_inc (&entry->rc);
  return entry;
}

void
tkm_procinfo_entry_unref (TkmProcInfoEntry *entry)
{
  g_assert (entry);

  if (g_ref_count_dec (&entry->rc) == TRUE)
    {
      if (entry->name != NULL)
        g_free (entry->name);
      if (entry->context != NULL)
        g_free (entry->context);

      g_free (entry);
    }
}

guint
tkm_procinfo_entry_get_index (TkmProcInfoEntry *entry)
{
  g_assert (entry);
  return entry->idx;
}

void
tkm_procinfo_entry_set_index (TkmProcInfoEntry *entry, guint val)
{
  g_assert (entry);
  entry->idx = val;
}

const gchar *
tkm_procinfo_entry_get_name (TkmProcInfoEntry *entry)
{
  g_assert (entry);
  return entry->name;
}

void
tkm_procinfo_entry_set_name (TkmProcInfoEntry *entry, const gchar *name)
{
  g_assert (entry);

  if (entry->name != NULL)
    g_free (entry->name);

  entry->name = g_strdup (name);
}

gulong
tkm_procinfo_entry_get_timestamp (TkmProcInfoEntry *entry, DataTimeSource type)
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
tkm_procinfo_entry_set_timestamp (TkmProcInfoEntry *entry, DataTimeSource type,
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

const gchar *
tkm_procinfo_entry_get_context (TkmProcInfoEntry *entry)
{
  g_assert (entry);
  return entry->context;
}

void
tkm_procinfo_entry_set_context (TkmProcInfoEntry *entry, const gchar *context)
{
  g_assert (entry);

  if (entry->context != NULL)
    g_free (entry->context);

  entry->context = g_strdup (context);
}

glong
tkm_procinfo_entry_get_data (TkmProcInfoEntry *entry, TkmProcInfoDataType type)
{
  g_assert (entry);
  switch (type)
    {
    case PINFO_DATA_PID:
      return entry->pid;

    case PINFO_DATA_PPID:
      return entry->ppid;

    case PINFO_DATA_CPU_TIME:
      return entry->cpu_time;

    case PINFO_DATA_CPU_PERCENT:
      return entry->cpu_percent;

    case PINFO_DATA_VMRSS:
      return entry->vm_rss;

    default:
      break;
    }

  return 0;
}

void
tkm_procinfo_entry_set_data (TkmProcInfoEntry *entry, TkmProcInfoDataType type,
                             glong val)
{
  g_assert (entry);
  switch (type)
    {
    case PINFO_DATA_PID:
      entry->pid = val;
      break;

    case PINFO_DATA_PPID:
      entry->ppid = val;
      break;

    case PINFO_DATA_CPU_TIME:
      entry->cpu_time = val;
      break;

    case PINFO_DATA_CPU_PERCENT:
      entry->cpu_percent = val;
      break;

    case PINFO_DATA_VMRSS:
      entry->vm_rss = val;
      break;

    default:
      break;
    }
}

static int
procinfo_sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  ProcInfoQueryData *querydata = (ProcInfoQueryData *)data;

  switch (querydata->type)
    {
    case PROCINFO_GET_ENTRIES:
    {
      GPtrArray **entries = (GPtrArray **)querydata->response;
      g_autoptr (TkmProcInfoEntry) entry = tkm_procinfo_entry_new ();
      gboolean valid = TRUE;

      for (gint i = 0; i < argc && valid; i++)
        {
          if (argv[i] == NULL)
            {
              valid = FALSE;
              continue;
            }

          if (g_strcmp0 (colname[i], "SystemTime") == 0)
            tkm_procinfo_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_SYSTEM,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MonotonicTime") == 0)
            tkm_procinfo_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_MONOTONIC,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ReceiveTime") == 0)
            tkm_procinfo_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_RECEIVE,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "Comm") == 0)
            tkm_procinfo_entry_set_name (entry, argv[i]);
          else if (g_strcmp0 (colname[i], "ContextName") == 0)
            tkm_procinfo_entry_set_context (entry, argv[i]);
          else if (g_strcmp0 (colname[i], "PID") == 0)
            tkm_procinfo_entry_set_data (
              entry, PINFO_DATA_PID, g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "PPID") == 0)
            tkm_procinfo_entry_set_data (
              entry, PINFO_DATA_PPID, g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "CpuTime") == 0)
            tkm_procinfo_entry_set_data (
              entry, PINFO_DATA_CPU_TIME,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "CpuPercent") == 0)
            tkm_procinfo_entry_set_data (
              entry, PINFO_DATA_CPU_PERCENT,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "VmRSS") == 0)
            tkm_procinfo_entry_set_data (
              entry, PINFO_DATA_VMRSS,
              g_ascii_strtoll (argv[i], NULL, 10));
        }

      if (valid)
        g_ptr_array_add (*entries, tkm_procinfo_entry_ref (entry));

      break;
    }

    default:
      break;
    }

  return 0;
}

static void
procinfo_entry_free (gpointer data)
{
  TkmProcInfoEntry *e = (TkmProcInfoEntry *)data;

  g_assert (e);
  tkm_procinfo_entry_unref (e);
}

GPtrArray *
tkm_procinfo_entry_get_all_entries (sqlite3 *db, const char *session_hash,
                                    DataTimeSource time_source,
                                    gulong start_time, gulong end_time,
                                    GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  GPtrArray *entries = g_ptr_array_new ();
  ProcInfoQueryData data
    = { .type = PROCINFO_GET_ENTRIES, .response = (gpointer) & entries };

  g_ptr_array_set_free_func (entries, procinfo_entry_free);

  g_assert (db);

  sql = g_strdup_printf ("SELECT * FROM '%s' "
                         "WHERE %s >= %lu AND "
                         " %s < %lu AND SessionId IS "
                         "(SELECT Id FROM '%s' WHERE Hash IS '%s' LIMIT 1);",
                         TKM_PROCINFO_TABLE_NAME,
                         timeSourceColumn[time_source], start_time,
                         timeSourceColumn[time_source], end_time,
                         TKM_SESSIONS_TABLE_NAME, session_hash);
  if (sqlite3_exec (db, sql, procinfo_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_ptr_array_free (entries, TRUE);
      entries = NULL;

      g_set_error (error, g_quark_from_static_string ("ProcInfoGetAll"), 1,
                   "SQL query error");
      g_warning ("Fail to get procinfo list. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return entries;
}
