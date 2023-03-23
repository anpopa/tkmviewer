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
 * \file tkm-procevent-entry.c
 */

#include "tkm-procevent-entry.h"

static const gchar *timeSourceColumn[]
  = { "SystemTime", "MonotonicTime", "ReceiveTime" };

/**
 * @enum ProcEvent query type
 */
typedef enum _ProcEventQueryType {
  PROCEVENT_GET_ENTRIES,
} ProcEventQueryType;

/**
 * @enum ProcEvent query data object
 */
typedef struct _ProcEventQueryData {
  ProcEventQueryType type;
  gpointer response;
} ProcEventQueryData;

static int procevent_sqlite_callback (void *data, int argc, char **argv,
                                      char **colname);

TkmProcEventEntry *
tkm_procevent_entry_new (void)
{
  TkmProcEventEntry *entry = g_new0 (TkmProcEventEntry, 1);

  g_ref_count_init (&entry->rc);

  return entry;
}

TkmProcEventEntry *
tkm_procevent_entry_ref (TkmProcEventEntry *entry)
{
  g_assert (entry);
  g_ref_count_inc (&entry->rc);
  return entry;
}

void
tkm_procevent_entry_unref (TkmProcEventEntry *entry)
{
  g_assert (entry);

  if (g_ref_count_dec (&entry->rc) == TRUE)
    {
      g_free (entry);
    }
}

guint
tkm_procevent_entry_get_index (TkmProcEventEntry *entry)
{
  g_assert (entry);
  return entry->idx;
}

void
tkm_procevent_entry_set_index (TkmProcEventEntry *entry, guint val)
{
  g_assert (entry);
  entry->idx = val;
}

gulong
tkm_procevent_entry_get_timestamp (TkmProcEventEntry *entry,
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
tkm_procevent_entry_set_timestamp (TkmProcEventEntry *entry,
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

guint
tkm_procevent_entry_get_data (TkmProcEventEntry *entry,
                              TkmProcEventDataType type)
{
  g_assert (entry);
  switch (type)
    {
    case PEVENT_DATA_FORKS:
      return entry->forks;

    case PEVENT_DATA_EXECS:
      return entry->execs;

    case PEVENT_DATA_EXITS:
      return entry->exits;

    case PEVENT_DATA_UIDS:
      return entry->uids;

    case PEVENT_DATA_GIDS:
      return entry->gids;

    default:
      break;
    }

  return 0;
}

void
tkm_procevent_entry_set_data (TkmProcEventEntry *entry,
                              TkmProcEventDataType type, guint val)
{
  g_assert (entry);
  switch (type)
    {
    case PEVENT_DATA_FORKS:
      entry->forks = val;
      break;

    case PEVENT_DATA_EXECS:
      entry->execs = val;
      break;

    case PEVENT_DATA_EXITS:
      entry->exits = val;
      break;

    case PEVENT_DATA_UIDS:
      entry->uids = val;
      break;

    case PEVENT_DATA_GIDS:
      entry->gids = val;
      break;

    default:
      break;
    }
}

static int
procevent_sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  ProcEventQueryData *querydata = (ProcEventQueryData *)data;

  switch (querydata->type)
    {
    case PROCEVENT_GET_ENTRIES:
    {
      GPtrArray **entries = (GPtrArray **)querydata->response;
      g_autoptr (TkmProcEventEntry) entry = tkm_procevent_entry_new ();
      gboolean valid = TRUE;

      for (gint i = 0; i < argc && valid; i++)
        {
          if (argv[i] == NULL)
            {
              valid = FALSE;
              continue;
            }

          if (g_strcmp0 (colname[i], "SystemTime") == 0)
            tkm_procevent_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_SYSTEM,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MonotonicTime") == 0)
            tkm_procevent_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_MONOTONIC,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ReceiveTime") == 0)
            tkm_procevent_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_RECEIVE,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ForkCount") == 0)
            tkm_procevent_entry_set_data (
              entry, PEVENT_DATA_FORKS,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ExecCount") == 0)
            tkm_procevent_entry_set_data (
              entry, PEVENT_DATA_EXECS,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ExitCount") == 0)
            tkm_procevent_entry_set_data (
              entry, PEVENT_DATA_EXITS,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "UIdCount") == 0)
            tkm_procevent_entry_set_data (
              entry, PEVENT_DATA_UIDS,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "GIdCount") == 0)
            tkm_procevent_entry_set_data (
              entry, PEVENT_DATA_GIDS,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
        }

      if (valid)
        g_ptr_array_add (*entries, tkm_procevent_entry_ref (entry));

      break;
    }

    default:
      break;
    }

  return 0;
}

static void
procevent_entry_free (gpointer data)
{
  TkmProcEventEntry *e = (TkmProcEventEntry *)data;

  g_assert (e);
  tkm_procevent_entry_unref (e);
}

GPtrArray *
tkm_procevent_entry_get_all_entries (sqlite3 *db, const char *session_hash,
                                     DataTimeSource time_source,
                                     gulong start_time, gulong end_time,
                                     GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  GPtrArray *entries = g_ptr_array_new ();
  ProcEventQueryData data
    = { .type = PROCEVENT_GET_ENTRIES, .response = (gpointer) & entries };

  g_ptr_array_set_free_func (entries, procevent_entry_free);

  g_assert (db);

  sql = g_strdup_printf ("SELECT * FROM '%s' "
                         "WHERE %s >= %lu AND "
                         " %s < %lu AND SessionId IS "
                         "(SELECT Id FROM '%s' WHERE Hash IS '%s' LIMIT 1);",
                         TKM_PROCEVENT_TABLE_NAME,
                         timeSourceColumn[time_source], start_time,
                         timeSourceColumn[time_source], end_time,
                         TKM_SESSIONS_TABLE_NAME, session_hash);
  if (sqlite3_exec (db, sql, procevent_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_ptr_array_free (entries, TRUE);
      entries = NULL;

      g_set_error (error, g_quark_from_static_string ("ProcEventGetAll"), 1,
                   "SQL query error");
      g_warning ("Fail to get procevent list. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return entries;
}
