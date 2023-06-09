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
 * \file tkm-ctxinfo-entry.c
 */

#include "tkm-ctxinfo-entry.h"

static const gchar *timeSourceColumn[]
  = { "SystemTime", "MonotonicTime", "ReceiveTime" };

/**
 * @enum CtxInfo query type
 */
typedef enum _CtxInfoQueryType {
  CTXINFO_GET_ENTRIES,
} CtxInfoQueryType;

/**
 * @enum CtxInfo query data object
 */
typedef struct _CtxInfoQueryData {
  CtxInfoQueryType type;
  gpointer response;
} CtxInfoQueryData;

static int ctxinfo_sqlite_callback (void *data, int argc, char **argv,
                                    char **colname);

TkmCtxInfoEntry *
tkm_ctxinfo_entry_new (void)
{
  TkmCtxInfoEntry *entry = g_new0 (TkmCtxInfoEntry, 1);

  g_ref_count_init (&entry->rc);

  return entry;
}

TkmCtxInfoEntry *
tkm_ctxinfo_entry_ref (TkmCtxInfoEntry *entry)
{
  g_assert (entry);
  g_ref_count_inc (&entry->rc);
  return entry;
}

void
tkm_ctxinfo_entry_unref (TkmCtxInfoEntry *entry)
{
  g_assert (entry);

  if (g_ref_count_dec (&entry->rc) == TRUE)
    {
      if (entry->name != NULL)
        g_free (entry->name);
      if (entry->id != NULL)
        g_free (entry->id);

      g_free (entry);
    }
}

guint
tkm_ctxinfo_entry_get_index (TkmCtxInfoEntry *entry)
{
  g_assert (entry);
  return entry->idx;
}

void
tkm_ctxinfo_entry_set_index (TkmCtxInfoEntry *entry, guint val)
{
  g_assert (entry);
  entry->idx = val;
}

const gchar *
tkm_ctxinfo_entry_get_name (TkmCtxInfoEntry *entry)
{
  g_assert (entry);
  return entry->name;
}

void
tkm_ctxinfo_entry_set_name (TkmCtxInfoEntry *entry, const gchar *name)
{
  g_assert (entry);

  if (entry->name != NULL)
    g_free (entry->name);

  entry->name = g_strdup (name);
}

gulong
tkm_ctxinfo_entry_get_timestamp (TkmCtxInfoEntry *entry, DataTimeSource type)
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
tkm_ctxinfo_entry_set_timestamp (TkmCtxInfoEntry *entry, DataTimeSource type,
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
tkm_ctxinfo_entry_get_id (TkmCtxInfoEntry *entry)
{
  g_assert (entry);
  return entry->id;
}

void
tkm_ctxinfo_entry_set_id (TkmCtxInfoEntry *entry, const gchar *id)
{
  g_assert (entry);

  if (entry->id != NULL)
    g_free (entry->id);

  entry->id = g_strdup (id);
}

glong
tkm_ctxinfo_entry_get_data (TkmCtxInfoEntry *entry, TkmCtxInfoDataType type)
{
  g_assert (entry);
  switch (type)
    {
    case CTXINFO_DATA_CPU_TIME:
      return entry->cpu_time;

    case CTXINFO_DATA_CPU_PERCENT:
      return entry->cpu_percent;

    case CTXINFO_DATA_MEM_RSS:
      return entry->mem_rss;

    case CTXINFO_DATA_MEM_PSS:
      return entry->mem_pss;

    default:
      break;
    }

  return 0;
}

void
tkm_ctxinfo_entry_set_data (TkmCtxInfoEntry *entry, TkmCtxInfoDataType type,
                            glong val)
{
  g_assert (entry);
  switch (type)
    {
    case CTXINFO_DATA_CPU_TIME:
      entry->cpu_time = val;
      break;

    case CTXINFO_DATA_CPU_PERCENT:
      entry->cpu_percent = val;
      break;

    case CTXINFO_DATA_MEM_RSS:
      entry->mem_rss = val;
      break;

    case CTXINFO_DATA_MEM_PSS:
      entry->mem_pss = val;
      break;

    default:
      break;
    }
}

static int
ctxinfo_sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  CtxInfoQueryData *querydata = (CtxInfoQueryData *)data;

  switch (querydata->type)
    {
    case CTXINFO_GET_ENTRIES:
    {
      GPtrArray **entries = (GPtrArray **)querydata->response;
      g_autoptr (TkmCtxInfoEntry) entry = tkm_ctxinfo_entry_new ();
      gboolean valid = TRUE;

      for (gint i = 0; i < argc && valid; i++)
        {
          if (argv[i] == NULL)
            {
              valid = FALSE;
              continue;
            }

          if (g_strcmp0 (colname[i], "SystemTime") == 0)
            tkm_ctxinfo_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_SYSTEM,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MonotonicTime") == 0)
            tkm_ctxinfo_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_MONOTONIC,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ReceiveTime") == 0)
            tkm_ctxinfo_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_RECEIVE,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ContextId") == 0)
            {
              g_autofree gchar *id = g_strdup_printf (
                "%lx", g_ascii_strtoull (argv[i], NULL, 10));
              tkm_ctxinfo_entry_set_id (entry, id);
            }
          else if (g_strcmp0 (colname[i], "ContextName") == 0)
            tkm_ctxinfo_entry_set_name (entry, argv[i]);
          else if (g_strcmp0 (colname[i], "TotalCpuTime") == 0)
            tkm_ctxinfo_entry_set_data (entry, CTXINFO_DATA_CPU_TIME,
                                        g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "TotalCpuPercent") == 0)
            tkm_ctxinfo_entry_set_data (entry, CTXINFO_DATA_CPU_PERCENT,
                                        g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "TotalMemRSS") == 0)
            tkm_ctxinfo_entry_set_data (entry, CTXINFO_DATA_MEM_RSS,
                                        g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "TotalMemPSS") == 0)
            tkm_ctxinfo_entry_set_data (entry, CTXINFO_DATA_MEM_PSS,
                                        g_ascii_strtoll (argv[i], NULL, 10));
        }

      if (valid)
        g_ptr_array_add (*entries, tkm_ctxinfo_entry_ref (entry));

      break;
    }

    default:
      break;
    }

  return 0;
}

static void
ctxinfo_entry_free (gpointer data)
{
  TkmCtxInfoEntry *e = (TkmCtxInfoEntry *)data;

  g_assert (e);
  tkm_ctxinfo_entry_unref (e);
}

GPtrArray *
tkm_ctxinfo_entry_get_all_entries (sqlite3 *db, const char *session_hash,
                                   DataTimeSource time_source,
                                   gulong start_time, gulong end_time,
                                   GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  GPtrArray *entries = g_ptr_array_new ();
  CtxInfoQueryData data
    = { .type = CTXINFO_GET_ENTRIES, .response = (gpointer) & entries };

  g_ptr_array_set_free_func (entries, ctxinfo_entry_free);

  g_assert (db);

  sql = g_strdup_printf ("SELECT * FROM '%s' "
                         "WHERE %s >= %lu AND "
                         " %s < %lu AND SessionId IS "
                         "(SELECT Id FROM '%s' WHERE Hash IS '%s' LIMIT 1);",
                         TKM_CTXINFO_TABLE_NAME, timeSourceColumn[time_source],
                         start_time, timeSourceColumn[time_source], end_time,
                         TKM_SESSIONS_TABLE_NAME, session_hash);
  if (sqlite3_exec (db, sql, ctxinfo_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_ptr_array_free (entries, TRUE);
      entries = NULL;

      g_set_error (error, g_quark_from_static_string ("CtxInfoGetAll"), 1,
                   "SQL query error");
      g_warning ("Fail to get ctxinfo list. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return entries;
}
