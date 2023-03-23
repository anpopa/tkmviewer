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
 * \file tkm-pressure-entry.c
 */

#include "tkm-pressure-entry.h"

static const gchar *timeSourceColumn[]
  = { "SystemTime", "MonotonicTime", "ReceiveTime" };

/**
 * @enum Pressure query type
 */
typedef enum _PressureQueryType {
  PRESSURE_GET_ENTRIES,
} PressureQueryType;

/**
 * @enum Pressure query data object
 */
typedef struct _PressureQueryData {
  PressureQueryType type;
  gpointer response;
} PressureQueryData;

static int pressure_sqlite_callback (void *data, int argc, char **argv,
                                     char **colname);

TkmPressureEntry *
tkm_pressure_entry_new (void)
{
  TkmPressureEntry *entry = g_new0 (TkmPressureEntry, 1);

  g_ref_count_init (&entry->rc);

  return entry;
}

TkmPressureEntry *
tkm_pressure_entry_ref (TkmPressureEntry *entry)
{
  g_assert (entry);
  g_ref_count_inc (&entry->rc);
  return entry;
}

void
tkm_pressure_entry_unref (TkmPressureEntry *entry)
{
  g_assert (entry);

  if (g_ref_count_dec (&entry->rc) == TRUE)
    {
      g_free (entry);
    }
}

guint
tkm_pressure_entry_get_index (TkmPressureEntry *entry)
{
  g_assert (entry);
  return entry->idx;
}

void
tkm_pressure_entry_set_index (TkmPressureEntry *entry, guint val)
{
  g_assert (entry);
  entry->idx = val;
}

gulong
tkm_pressure_entry_get_timestamp (TkmPressureEntry *entry, DataTimeSource type)
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
tkm_pressure_entry_set_timestamp (TkmPressureEntry *entry, DataTimeSource type,
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
tkm_pressure_entry_get_data_total (TkmPressureEntry *entry,
                                   TkmPressureDataType type)
{
  g_assert (entry);
  switch (type)
    {
    case PSI_DATA_CPU_SOME_TOTAL:
      return entry->cpu_some_total;

    case PSI_DATA_CPU_FULL_TOTAL:
      return entry->cpu_full_total;

    case PSI_DATA_MEM_SOME_TOTAL:
      return entry->mem_some_total;

    case PSI_DATA_MEM_FULL_TOTAL:
      return entry->mem_full_total;

    case PSI_DATA_IO_SOME_TOTAL:
      return entry->io_some_total;

    case PSI_DATA_IO_FULL_TOTAL:
      return entry->io_full_total;

    default:
      break;
    }

  return 0;
}

void
tkm_pressure_entry_set_data_total (TkmPressureEntry *entry,
                                   TkmPressureDataType type, guint val)
{
  g_assert (entry);
  switch (type)
    {
    case PSI_DATA_CPU_SOME_TOTAL:
      entry->cpu_some_total = val;
      break;

    case PSI_DATA_CPU_FULL_TOTAL:
      entry->cpu_full_total = val;
      break;

    case PSI_DATA_MEM_SOME_TOTAL:
      entry->mem_some_total = val;
      break;

    case PSI_DATA_MEM_FULL_TOTAL:
      entry->mem_full_total = val;
      break;

    case PSI_DATA_IO_SOME_TOTAL:
      entry->io_some_total = val;
      break;

    case PSI_DATA_IO_FULL_TOTAL:
      entry->io_full_total = val;
      break;

    default:
      break;
    }
}

gfloat
tkm_pressure_entry_get_data_avg (TkmPressureEntry *entry,
                                 TkmPressureDataType type)
{
  g_assert (entry);
  switch (type)
    {
    case PSI_DATA_CPU_SOME_AVG10:
      return entry->cpu_some_avg10;

    case PSI_DATA_CPU_SOME_AVG60:
      return entry->cpu_some_avg60;

    case PSI_DATA_CPU_SOME_AVG300:
      return entry->cpu_some_avg300;

    case PSI_DATA_CPU_FULL_AVG10:
      return entry->cpu_full_avg10;

    case PSI_DATA_CPU_FULL_AVG60:
      return entry->cpu_full_avg60;

    case PSI_DATA_CPU_FULL_AVG300:
      return entry->cpu_full_avg300;

    case PSI_DATA_MEM_SOME_AVG10:
      return entry->mem_some_avg10;

    case PSI_DATA_MEM_SOME_AVG60:
      return entry->mem_some_avg60;

    case PSI_DATA_MEM_SOME_AVG300:
      return entry->mem_some_avg300;

    case PSI_DATA_MEM_FULL_AVG10:
      return entry->mem_full_avg10;

    case PSI_DATA_MEM_FULL_AVG60:
      return entry->mem_full_avg60;

    case PSI_DATA_MEM_FULL_AVG300:
      return entry->mem_full_avg300;

    case PSI_DATA_IO_SOME_AVG10:
      return entry->io_some_avg10;

    case PSI_DATA_IO_SOME_AVG60:
      return entry->io_some_avg60;

    case PSI_DATA_IO_SOME_AVG300:
      return entry->io_some_avg300;

    case PSI_DATA_IO_FULL_AVG10:
      return entry->io_full_avg10;

    case PSI_DATA_IO_FULL_AVG60:
      return entry->io_full_avg60;

    case PSI_DATA_IO_FULL_AVG300:
      return entry->io_full_avg300;

    default:
      break;
    }

  return 0;
}

void
tkm_pressure_entry_set_data_avg (TkmPressureEntry *entry,
                                 TkmPressureDataType type, gfloat val)
{
  g_assert (entry);
  switch (type)
    {
    case PSI_DATA_CPU_SOME_AVG10:
      entry->cpu_some_avg10 = val;
      break;

    case PSI_DATA_CPU_SOME_AVG60:
      entry->cpu_some_avg60 = val;
      break;

    case PSI_DATA_CPU_SOME_AVG300:
      entry->cpu_some_avg300 = val;
      break;

    case PSI_DATA_CPU_FULL_AVG10:
      entry->cpu_full_avg10 = val;
      break;

    case PSI_DATA_CPU_FULL_AVG60:
      entry->cpu_full_avg60 = val;
      break;

    case PSI_DATA_CPU_FULL_AVG300:
      entry->cpu_full_avg300 = val;
      break;

    case PSI_DATA_MEM_SOME_AVG10:
      entry->mem_some_avg10 = val;
      break;

    case PSI_DATA_MEM_SOME_AVG60:
      entry->mem_some_avg60 = val;
      break;

    case PSI_DATA_MEM_SOME_AVG300:
      entry->mem_some_avg300 = val;
      break;

    case PSI_DATA_MEM_FULL_AVG10:
      entry->mem_full_avg10 = val;
      break;

    case PSI_DATA_MEM_FULL_AVG60:
      entry->mem_full_avg60 = val;
      break;

    case PSI_DATA_MEM_FULL_AVG300:
      entry->mem_full_avg300 = val;
      break;

    case PSI_DATA_IO_SOME_AVG10:
      entry->io_some_avg10 = val;
      break;

    case PSI_DATA_IO_SOME_AVG60:
      entry->io_some_avg60 = val;
      break;

    case PSI_DATA_IO_SOME_AVG300:
      entry->io_some_avg300 = val;
      break;

    case PSI_DATA_IO_FULL_AVG10:
      entry->io_full_avg10 = val;
      break;

    case PSI_DATA_IO_FULL_AVG60:
      entry->io_full_avg60 = val;
      break;

    case PSI_DATA_IO_FULL_AVG300:
      entry->io_full_avg300 = val;
      break;

    default:
      break;
    }
}

static int
pressure_sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  PressureQueryData *querydata = (PressureQueryData *)data;

  switch (querydata->type)
    {
    case PRESSURE_GET_ENTRIES:
    {
      GPtrArray **entries = (GPtrArray **)querydata->response;
      g_autoptr (TkmPressureEntry) entry = tkm_pressure_entry_new ();
      gboolean valid = TRUE;

      for (gint i = 0; i < argc && valid; i++)
        {
          if (argv[i] == NULL)
            {
              valid = FALSE;
              continue;
            }

          if (g_strcmp0 (colname[i], "SystemTime") == 0)
            tkm_pressure_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_SYSTEM,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MonotonicTime") == 0)
            tkm_pressure_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_MONOTONIC,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ReceiveTime") == 0)
            tkm_pressure_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_RECEIVE,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "CPUSomeAvg10") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_CPU_SOME_AVG10,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "CPUSomeAvg60") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_CPU_SOME_AVG60,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "CPUSomeAvg300") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_CPU_SOME_AVG300,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "CPUSomeTotal") == 0)
            tkm_pressure_entry_set_data_total (
              entry, PSI_DATA_CPU_SOME_TOTAL,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "CPUFullAvg10") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_CPU_FULL_AVG10,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "CPUFullAvg60") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_CPU_FULL_AVG60,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "CPUFullAvg300") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_CPU_FULL_AVG300,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "CPUFullTotal") == 0)
            tkm_pressure_entry_set_data_total (
              entry, PSI_DATA_CPU_FULL_TOTAL,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MEMSomeAvg10") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_MEM_SOME_AVG10,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "MEMSomeAvg60") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_MEM_SOME_AVG60,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "MEMSomeAvg300") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_MEM_SOME_AVG300,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "MEMSomeTotal") == 0)
            tkm_pressure_entry_set_data_total (
              entry, PSI_DATA_MEM_SOME_TOTAL,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MEMFullAvg10") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_MEM_FULL_AVG10,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "MEMFullAvg60") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_MEM_FULL_AVG60,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "MEMFullAvg300") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_MEM_FULL_AVG300,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "MEMFullTotal") == 0)
            tkm_pressure_entry_set_data_total (
              entry, PSI_DATA_MEM_FULL_TOTAL,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "IOSomeAvg10") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_IO_SOME_AVG10,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "IOSomeAvg60") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_IO_SOME_AVG60,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "IOSomeAvg300") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_IO_SOME_AVG300,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "IOSomeTotal") == 0)
            tkm_pressure_entry_set_data_total (
              entry, PSI_DATA_IO_SOME_TOTAL,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "IOFullAvg10") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_IO_FULL_AVG10,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "IOFullAvg60") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_IO_FULL_AVG60,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "IOFullAvg300") == 0)
            tkm_pressure_entry_set_data_avg (
              entry, PSI_DATA_IO_FULL_AVG300,
              (gfloat)g_ascii_strtod (argv[i], NULL));
          else if (g_strcmp0 (colname[i], "IOFullTotal") == 0)
            tkm_pressure_entry_set_data_total (
              entry, PSI_DATA_IO_FULL_TOTAL,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
        }

      if (valid)
        g_ptr_array_add (*entries, tkm_pressure_entry_ref (entry));

      break;
    }

    default:
      break;
    }

  return 0;
}

static void
pressure_entry_free (gpointer data)
{
  TkmPressureEntry *e = (TkmPressureEntry *)data;

  g_assert (e);
  tkm_pressure_entry_unref (e);
}

GPtrArray *
tkm_pressure_entry_get_all_entries (sqlite3 *db, const char *session_hash,
                                    DataTimeSource time_source,
                                    gulong start_time, gulong end_time,
                                    GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  GPtrArray *entries = g_ptr_array_new ();
  PressureQueryData data
    = { .type = PRESSURE_GET_ENTRIES, .response = (gpointer) & entries };

  g_ptr_array_set_free_func (entries, pressure_entry_free);

  g_assert (db);

  sql = g_strdup_printf ("SELECT * FROM '%s' "
                         "WHERE %s >= %lu AND "
                         " %s < %lu AND SessionId IS "
                         "(SELECT Id FROM '%s' WHERE Hash IS '%s' LIMIT 1);",
                         TKM_PRESSURE_TABLE_NAME,
                         timeSourceColumn[time_source], start_time,
                         timeSourceColumn[time_source], end_time,
                         TKM_SESSIONS_TABLE_NAME, session_hash);
  if (sqlite3_exec (db, sql, pressure_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_ptr_array_free (entries, TRUE);
      entries = NULL;

      g_set_error (error, g_quark_from_static_string ("PressureGetAll"), 1,
                   "SQL query error");
      g_warning ("Fail to get pressure list. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return entries;
}
