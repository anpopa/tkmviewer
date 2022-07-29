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
 * \file tkm-meminfo-entry.c
 */

#include "tkm-meminfo-entry.h"

static const gchar *timeSourceColumn[]
    = { "SystemTime", "MonotonicTime", "ReceiveTime" };

/**
 * @enum MemInfo query type
 */
typedef enum _MemInfoQueryType
{
  MEMINFO_GET_ENTRIES,
} MemInfoQueryType;

/**
 * @enum MemInfo query data object
 */
typedef struct _MemInfoQueryData
{
  MemInfoQueryType type;
  gpointer response;
} MemInfoQueryData;

static int meminfo_sqlite_callback (void *data, int argc, char **argv,
                                    char **colname);

TkmMemInfoEntry *
tkm_meminfo_entry_new (void)
{
  TkmMemInfoEntry *entry = g_new0 (TkmMemInfoEntry, 1);

  g_ref_count_init (&entry->rc);

  return entry;
}

TkmMemInfoEntry *
tkm_meminfo_entry_ref (TkmMemInfoEntry *entry)
{
  g_assert (entry);
  g_ref_count_inc (&entry->rc);
  return entry;
}

void
tkm_meminfo_entry_unref (TkmMemInfoEntry *entry)
{
  g_assert (entry);

  if (g_ref_count_dec (&entry->rc) == TRUE)
    {
      g_free (entry);
    }
}

guint
tkm_meminfo_entry_get_index (TkmMemInfoEntry *entry)
{
  g_assert (entry);
  return entry->idx;
}

void
tkm_meminfo_entry_set_index (TkmMemInfoEntry *entry, guint val)
{
  g_assert (entry);
  entry->idx = val;
}

gulong
tkm_meminfo_entry_get_timestamp (TkmMemInfoEntry *entry, DataTimeSource type)
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
tkm_meminfo_entry_set_timestamp (TkmMemInfoEntry *entry, DataTimeSource type,
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
tkm_meminfo_entry_get_data (TkmMemInfoEntry *entry, TkmMemInfoDataType type)
{
  g_assert (entry);
  switch (type)
    {
    case MINFO_DATA_MEM_TOTAL:
      return entry->mem_total;

    case MINFO_DATA_MEM_FREE:
      return entry->mem_free;

    case MINFO_DATA_MEM_AVAIL:
      return entry->mem_avail;

    case MINFO_DATA_MEM_CACHED:
      return entry->mem_cached;

    case MINFO_DATA_MEM_PERCENT:
      return entry->mem_percent;

    case MINFO_DATA_SWAP_TOTAL:
      return entry->swap_total;

    case MINFO_DATA_SWAP_FREE:
      return entry->swap_free;

    case MINFO_DATA_SWAP_CACHED:
      return entry->swap_cached;

    case MINFO_DATA_SWAP_PERCENT:
      return entry->swap_percent;

    case MINFO_DATA_CMA_TOTAL:
      return entry->cma_total;

    case MINFO_DATA_CMA_FREE:
      return entry->cma_free;

    default:
      break;
    }

  return 0;
}

void
tkm_meminfo_entry_set_data (TkmMemInfoEntry *entry, TkmMemInfoDataType type,
                            guint val)
{
  g_assert (entry);
  switch (type)
    {
    case MINFO_DATA_MEM_TOTAL:
      entry->mem_total = val;
      break;

    case MINFO_DATA_MEM_FREE:
      entry->mem_free = val;
      break;

    case MINFO_DATA_MEM_AVAIL:
      entry->mem_avail = val;
      break;

    case MINFO_DATA_MEM_CACHED:
      entry->mem_cached = val;
      break;

    case MINFO_DATA_MEM_PERCENT:
      entry->mem_percent = val;
      break;

    case MINFO_DATA_SWAP_TOTAL:
      entry->swap_total = val;
      break;

    case MINFO_DATA_SWAP_FREE:
      entry->swap_free = val;
      break;

    case MINFO_DATA_SWAP_CACHED:
      entry->swap_cached = val;
      break;

    case MINFO_DATA_SWAP_PERCENT:
      entry->swap_percent = val;
      break;

    case MINFO_DATA_CMA_TOTAL:
      entry->cma_total = val;
      break;

    case MINFO_DATA_CMA_FREE:
      entry->cma_free = val;
      break;

    default:
      break;
    }
}

static int
meminfo_sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  MemInfoQueryData *querydata = (MemInfoQueryData *)data;

  switch (querydata->type)
    {
    case MEMINFO_GET_ENTRIES:
      {
        GPtrArray **entries = (GPtrArray **)querydata->response;
        g_autoptr (TkmMemInfoEntry) entry = tkm_meminfo_entry_new ();

        for (gint i = 0; i < argc; i++)
          {
            if (g_strcmp0 (colname[i], "SystemTime") == 0)
              tkm_meminfo_entry_set_timestamp (
                  entry, DATA_TIME_SOURCE_SYSTEM,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "MonotonicTime") == 0)
              tkm_meminfo_entry_set_timestamp (
                  entry, DATA_TIME_SOURCE_MONOTONIC,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "ReceiveTime") == 0)
              tkm_meminfo_entry_set_timestamp (
                  entry, DATA_TIME_SOURCE_RECEIVE,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "MemTotal") == 0)
              tkm_meminfo_entry_set_data (
                  entry, MINFO_DATA_MEM_TOTAL,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "MemFree") == 0)
              tkm_meminfo_entry_set_data (
                  entry, MINFO_DATA_MEM_FREE,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "MemAvail") == 0)
              tkm_meminfo_entry_set_data (
                  entry, MINFO_DATA_MEM_AVAIL,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "MemCached") == 0)
              tkm_meminfo_entry_set_data (
                  entry, MINFO_DATA_MEM_CACHED,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "MemAvailPercent") == 0)
              tkm_meminfo_entry_set_data (
                  entry, MINFO_DATA_MEM_PERCENT,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "SwapTotal") == 0)
              tkm_meminfo_entry_set_data (
                  entry, MINFO_DATA_SWAP_TOTAL,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "SwapFree") == 0)
              tkm_meminfo_entry_set_data (
                  entry, MINFO_DATA_SWAP_FREE,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "SwapCached") == 0)
              tkm_meminfo_entry_set_data (
                  entry, MINFO_DATA_SWAP_CACHED,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "CmaTotal") == 0)
              tkm_meminfo_entry_set_data (
                  entry, MINFO_DATA_CMA_TOTAL,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "CmaFree") == 0)
              tkm_meminfo_entry_set_data (
                  entry, MINFO_DATA_CMA_FREE,
                  (guint)g_ascii_strtoull (argv[i], NULL, 10));
          }

        g_ptr_array_add (*entries, tkm_meminfo_entry_ref (entry));
        break;
      }

    default:
      break;
    }

  return 0;
}

static void
meminfo_entry_free (gpointer data)
{
  TkmMemInfoEntry *e = (TkmMemInfoEntry *)data;

  g_assert (e);
  tkm_meminfo_entry_unref (e);
}

GPtrArray *
tkm_meminfo_entry_get_all_entries (sqlite3 *db, const char *session_hash,
                                   DataTimeSource time_source,
                                   gulong start_time, gulong end_time,
                                   GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  GPtrArray *entries = g_ptr_array_new ();
  MemInfoQueryData data
      = { .type = MEMINFO_GET_ENTRIES, .response = (gpointer)&entries };

  g_ptr_array_set_free_func (entries, meminfo_entry_free);

  g_assert (db);

  sql = g_strdup_printf ("SELECT * FROM '%s' "
                         "WHERE %s >= %lu AND "
                         " %s < %lu AND SessionId IS "
                         "(SELECT Id FROM '%s' WHERE Hash IS '%s' LIMIT 1);",
                         TKM_MEMINFO_TABLE_NAME, timeSourceColumn[time_source],
                         start_time, timeSourceColumn[time_source], end_time,
                         TKM_SESSIONS_TABLE_NAME, session_hash);
  if (sqlite3_exec (db, sql, meminfo_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_ptr_array_free (entries, TRUE);
      entries = NULL;

      g_set_error (error, g_quark_from_static_string ("MemInfoGetAll"), 1,
                   "SQL query error");
      g_warning ("Fail to get meminfo list. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return entries;
}
