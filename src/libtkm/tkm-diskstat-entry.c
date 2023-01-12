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
 * \file tkm-diskstat-entry.c
 */

#include "tkm-diskstat-entry.h"

static const gchar *timeSourceColumn[]
  = { "SystemTime", "MonotonicTime", "ReceiveTime" };

/**
 * @enum DiskStat query type
 */
typedef enum _DiskStatQueryType {
  DISKSTAT_GET_ENTRIES,
} DiskStatQueryType;

/**
 * @enum DiskStat query data object
 */
typedef struct _DiskStatQueryData {
  DiskStatQueryType type;
  gpointer response;
} DiskStatQueryData;

static int diskstat_sqlite_callback (void *data, int argc, char **argv,
                                     char **colname);

TkmDiskStatEntry *
tkm_diskstat_entry_new (void)
{
  TkmDiskStatEntry *entry = g_new0 (TkmDiskStatEntry, 1);

  g_ref_count_init (&entry->rc);

  return entry;
}

TkmDiskStatEntry *
tkm_diskstat_entry_ref (TkmDiskStatEntry *entry)
{
  g_assert (entry);
  g_ref_count_inc (&entry->rc);
  return entry;
}

void
tkm_diskstat_entry_unref (TkmDiskStatEntry *entry)
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
tkm_diskstat_entry_get_index (TkmDiskStatEntry *entry)
{
  g_assert (entry);
  return entry->idx;
}

void
tkm_diskstat_entry_set_index (TkmDiskStatEntry *entry, guint val)
{
  g_assert (entry);
  entry->idx = val;
}

const gchar *
tkm_diskstat_entry_get_name (TkmDiskStatEntry *entry)
{
  g_assert (entry);
  return entry->name;
}

void
tkm_diskstat_entry_set_name (TkmDiskStatEntry *entry, const gchar *name)
{
  g_assert (entry);

  if (entry->name != NULL)
    g_free (entry->name);

  entry->name = g_strdup (name);
}

gulong
tkm_diskstat_entry_get_timestamp (TkmDiskStatEntry *entry, DataTimeSource type)
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
tkm_diskstat_entry_set_timestamp (TkmDiskStatEntry *entry, DataTimeSource type,
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

glong
tkm_diskstat_entry_get_data (TkmDiskStatEntry *entry, TkmDiskStatDataType type)
{
  g_assert (entry);

  switch (type)
    {
    case DISKSTAT_DATA_MAJOR:
      return entry->major;

    case DISKSTAT_DATA_MINOR:
      return entry->minor;

    case DISKSTAT_DATA_READS_COMPLETED:
      return entry->reads_completed;

    case DISKSTAT_DATA_READS_MERGED:
      return entry->reads_merged;

    case DISKSTAT_DATA_READS_SPENT_MS:
      return entry->reads_spent_ms;

    case DISKSTAT_DATA_WRITES_COMPLETED:
      return entry->writes_completed;

    case DISKSTAT_DATA_WRITES_MERGED:
      return entry->writes_merged;

    case DISKSTAT_DATA_WRITES_SPENT_MS:
      return entry->writes_spent_ms;

    case DISKSTAT_DATA_IO_INPROGRESS:
      return entry->io_in_progress;

    case DISKSTAT_DATA_IO_SPENT_MS:
      return entry->io_spent_ms;

    case DISKSTAT_DATA_IO_WEIGHTED_MS:
      return entry->io_weighted_ms;

    default:
      break;
    }

  return 0;
}

void
tkm_diskstat_entry_set_data (TkmDiskStatEntry *entry, TkmDiskStatDataType type,
                             glong data)
{
  g_assert (entry);

  switch (type)
    {
    case DISKSTAT_DATA_MAJOR:
      entry->major = data;
      break;

    case DISKSTAT_DATA_MINOR:
      entry->minor = data;
      break;

    case DISKSTAT_DATA_READS_COMPLETED:
      entry->reads_completed = data;
      break;

    case DISKSTAT_DATA_READS_MERGED:
      entry->reads_merged = data;
      break;

    case DISKSTAT_DATA_READS_SPENT_MS:
      entry->reads_spent_ms = data;
      break;

    case DISKSTAT_DATA_WRITES_COMPLETED:
      entry->writes_completed = data;
      break;

    case DISKSTAT_DATA_WRITES_MERGED:
      entry->writes_merged = data;
      break;

    case DISKSTAT_DATA_WRITES_SPENT_MS:
      entry->writes_spent_ms = data;
      break;

    case DISKSTAT_DATA_IO_INPROGRESS:
      entry->io_in_progress = data;
      break;

    case DISKSTAT_DATA_IO_SPENT_MS:
      entry->io_spent_ms = data;
      break;

    case DISKSTAT_DATA_IO_WEIGHTED_MS:
      entry->io_weighted_ms = data;
      break;

    default:
      break;
    }
}

static int
diskstat_sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  DiskStatQueryData *querydata = (DiskStatQueryData *)data;

  switch (querydata->type)
    {
    case DISKSTAT_GET_ENTRIES:
    {
      GPtrArray **entries = (GPtrArray **)querydata->response;
      g_autoptr (TkmDiskStatEntry) entry = tkm_diskstat_entry_new ();

      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "SystemTime") == 0)
            tkm_diskstat_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_SYSTEM,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MonotonicTime") == 0)
            tkm_diskstat_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_MONOTONIC,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ReceiveTime") == 0)
            tkm_diskstat_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_RECEIVE,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "Name") == 0)
            tkm_diskstat_entry_set_name (entry, argv[i]);
          else if (g_strcmp0 (colname[i], "Major") == 0)
            tkm_diskstat_entry_set_data (
              entry, DISKSTAT_DATA_MAJOR,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "Minor") == 0)
            tkm_diskstat_entry_set_data (
              entry, DISKSTAT_DATA_MINOR,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ReadsCompleted") == 0)
            tkm_diskstat_entry_set_data (
              entry, DISKSTAT_DATA_READS_COMPLETED,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ReadsMerged") == 0)
            tkm_diskstat_entry_set_data (
              entry, DISKSTAT_DATA_READS_MERGED,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ReadsSpent") == 0)
            tkm_diskstat_entry_set_data (
              entry, DISKSTAT_DATA_READS_SPENT_MS,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "WritesCompleted") == 0)
            tkm_diskstat_entry_set_data (
              entry, DISKSTAT_DATA_WRITES_COMPLETED,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "WritesMerged") == 0)
            tkm_diskstat_entry_set_data (
              entry, DISKSTAT_DATA_WRITES_MERGED,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "WritesSpent") == 0)
            tkm_diskstat_entry_set_data (
              entry, DISKSTAT_DATA_WRITES_SPENT_MS,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "IOInProgress") == 0)
            tkm_diskstat_entry_set_data (
              entry, DISKSTAT_DATA_IO_INPROGRESS,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "IOSpent") == 0)
            tkm_diskstat_entry_set_data (
              entry, DISKSTAT_DATA_IO_SPENT_MS,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "IOWeightedMs") == 0)
            tkm_diskstat_entry_set_data (
              entry, DISKSTAT_DATA_IO_WEIGHTED_MS,
              g_ascii_strtoll (argv[i], NULL, 10));
        }

      g_ptr_array_add (*entries, tkm_diskstat_entry_ref (entry));
      break;
    }

    default:
      break;
    }

  return 0;
}

static void
diskstat_entry_free (gpointer data)
{
  TkmDiskStatEntry *e = (TkmDiskStatEntry *)data;

  g_assert (e);
  tkm_diskstat_entry_unref (e);
}

GPtrArray *
tkm_diskstat_entry_get_all_entries (sqlite3 *db, const char *session_hash,
                                    DataTimeSource time_source,
                                    gulong start_time, gulong end_time,
                                    GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  GPtrArray *entries = g_ptr_array_new ();
  DiskStatQueryData data
    = { .type = DISKSTAT_GET_ENTRIES, .response = (gpointer) & entries };

  g_ptr_array_set_free_func (entries, diskstat_entry_free);

  g_assert (db);

  sql = g_strdup_printf ("SELECT * FROM '%s' "
                         "WHERE %s >= %lu AND "
                         " %s < %lu AND SessionId IS "
                         "(SELECT Id FROM '%s' WHERE Hash IS '%s' LIMIT 1);",
                         TKM_DISKSTAT_TABLE_NAME,
                         timeSourceColumn[time_source], start_time,
                         timeSourceColumn[time_source], end_time,
                         TKM_SESSIONS_TABLE_NAME, session_hash);
  if (sqlite3_exec (db, sql, diskstat_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_ptr_array_free (entries, TRUE);
      entries = NULL;

      g_set_error (error, g_quark_from_static_string ("DiskStatGetAll"), 1,
                   "SQL query error");
      g_warning ("Fail to get diskstat list. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return entries;
}
