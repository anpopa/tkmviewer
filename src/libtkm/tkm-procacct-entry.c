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
 * \file tkm-procacct-entry.c
 */

#include "tkm-procacct-entry.h"

static const gchar *timeSourceColumn[]
  = { "SystemTime", "MonotonicTime", "ReceiveTime" };

/**
 * @enum ProcAcct query type
 */
typedef enum _ProcAcctQueryType {
  PROCACCT_GET_ENTRIES,
} ProcAcctQueryType;

/**
 * @enum ProcAcct query data object
 */
typedef struct _ProcAcctQueryData {
  ProcAcctQueryType type;
  gpointer response;
} ProcAcctQueryData;

static int procacct_sqlite_callback (void *data, int argc, char **argv,
                                     char **colname);

TkmProcAcctEntry *
tkm_procacct_entry_new (void)
{
  TkmProcAcctEntry *entry = g_new0 (TkmProcAcctEntry, 1);

  g_ref_count_init (&entry->rc);

  return entry;
}

TkmProcAcctEntry *
tkm_procacct_entry_ref (TkmProcAcctEntry *entry)
{
  g_assert (entry);
  g_ref_count_inc (&entry->rc);
  return entry;
}

void
tkm_procacct_entry_unref (TkmProcAcctEntry *entry)
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
tkm_procacct_entry_get_index (TkmProcAcctEntry *entry)
{
  g_assert (entry);
  return entry->idx;
}

void
tkm_procacct_entry_set_index (TkmProcAcctEntry *entry, guint val)
{
  g_assert (entry);
  entry->idx = val;
}

const gchar *
tkm_procacct_entry_get_name (TkmProcAcctEntry *entry)
{
  g_assert (entry);
  return entry->name;
}

void
tkm_procacct_entry_set_name (TkmProcAcctEntry *entry, const gchar *name)
{
  g_assert (entry);

  if (entry->name != NULL)
    g_free (entry->name);

  entry->name = g_strdup (name);
}

gulong
tkm_procacct_entry_get_timestamp (TkmProcAcctEntry *entry, DataTimeSource type)
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
tkm_procacct_entry_set_timestamp (TkmProcAcctEntry *entry, DataTimeSource type,
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
tkm_procacct_entry_get_data (TkmProcAcctEntry *entry, TkmProcAcctDataType type)
{
  g_assert (entry);
  switch (type)
    {
    case PACCT_DATA_PID:
      return entry->pid;

    case PACCT_DATA_PPID:
      return entry->ppid;

    case PACCT_DATA_UID:
      return entry->uid;

    case PACCT_DATA_GID:
      return entry->gid;

    case PACCT_DATA_UTIME:
      return entry->utime;

    case PACCT_DATA_STIME:
      return entry->stime;

    case PACCT_DATA_CPU_COUNT:
      return entry->cpu_count;

    case PACCT_DATA_CPU_RUN_REAL:
      return entry->cpu_run_real;

    case PACCT_DATA_CPU_RUN_VIRTUAL:
      return entry->cpu_run_virtual;

    case PACCT_DATA_CPU_DELAY_TOTAL:
      return entry->cpu_delay_total;

    case PACCT_DATA_CPU_DELAY_AVG:
      return entry->cpu_delay_avg;

    case PACCT_DATA_CORE_MEM:
      return entry->core_mem;

    case PACCT_DATA_VIRT_MEM:
      return entry->virt_mem;

    case PACCT_DATA_HIGH_WATER_RSS:
      return entry->high_water_rss;

    case PACCT_DATA_HIGH_WATER_VM:
      return entry->high_water_vm;

    case PACCT_DATA_NVCSW:
      return entry->nvcsw;

    case PACCT_DATA_NIVCSW:
      return entry->nivcsw;

    case PACCT_DATA_SWAPIN_COUNT:
      return entry->swapin_count;

    case PACCT_DATA_SWAPIN_DELAY_TOTAL:
      return entry->swapin_delay_total;

    case PACCT_DATA_SWAPIN_DELAY_AVG:
      return entry->swapin_delay_avg;

    case PACCT_DATA_BLKIO_COUNT:
      return entry->blkio_count;

    case PACCT_DATA_BLKIO_DELAY_TOTAL:
      return entry->blkio_delay_total;

    case PACCT_DATA_BLKIO_DELAY_AVG:
      return entry->blkio_delay_avg;

    case PACCT_DATA_IO_STORAGE_READ:
      return entry->io_storage_read;

    case PACCT_DATA_IO_STORAGE_WRITE:
      return entry->io_storage_write;

    case PACCT_DATA_IO_READ_CHAR:
      return entry->io_read_char;

    case PACCT_DATA_IO_WRITE_CHAR:
      return entry->io_write_char;

    case PACCT_DATA_IO_READ_SYSCALLS:
      return entry->io_read_syscalls;

    case PACCT_DATA_IO_WRITE_SYSCALLS:
      return entry->io_write_syscalls;

    case PACCT_DATA_FREEPAGE_COUNT:
      return entry->freepage_count;

    case PACCT_DATA_FREEPAGE_DELAY_TOTAL:
      return entry->freepage_delay_total;

    case PACCT_DATA_FREEPAGE_DELAY_AVG:
      return entry->freepage_delay_avg;

    case PACCT_DATA_TRASHING_COUNT:
      return entry->trashing_count;

    case PACCT_DATA_TRASHING_DELAY_TOTAL:
      return entry->trashing_delay_total;

    case PACCT_DATA_TRASHING_DELAY_AVG:
      return entry->trashing_delay_avg;

    default:
      break;
    }

  return 0;
}

void
tkm_procacct_entry_set_data (TkmProcAcctEntry *entry, TkmProcAcctDataType type,
                             glong val)
{
  g_assert (entry);
  switch (type)
    {
    case PACCT_DATA_PID:
      entry->pid = val;
      break;

    case PACCT_DATA_PPID:
      entry->ppid = val;
      break;

    case PACCT_DATA_UID:
      entry->uid = val;
      break;

    case PACCT_DATA_GID:
      entry->gid = val;
      break;

    case PACCT_DATA_UTIME:
      entry->utime = val;
      break;

    case PACCT_DATA_STIME:
      entry->stime = val;
      break;

    case PACCT_DATA_CPU_COUNT:
      entry->cpu_count = val;
      break;

    case PACCT_DATA_CPU_RUN_REAL:
      entry->cpu_run_real = val;
      break;

    case PACCT_DATA_CPU_RUN_VIRTUAL:
      entry->cpu_run_virtual = val;
      break;

    case PACCT_DATA_CPU_DELAY_TOTAL:
      entry->cpu_delay_total = val;
      break;

    case PACCT_DATA_CPU_DELAY_AVG:
      entry->cpu_delay_avg = val;
      break;

    case PACCT_DATA_CORE_MEM:
      entry->core_mem = val;
      break;

    case PACCT_DATA_VIRT_MEM:
      entry->virt_mem = val;
      break;

    case PACCT_DATA_HIGH_WATER_RSS:
      entry->high_water_rss = val;
      break;

    case PACCT_DATA_HIGH_WATER_VM:
      entry->high_water_vm = val;
      break;

    case PACCT_DATA_NVCSW:
      entry->nvcsw = val;
      break;

    case PACCT_DATA_NIVCSW:
      entry->nivcsw = val;
      break;

    case PACCT_DATA_SWAPIN_COUNT:
      entry->swapin_count = val;
      break;

    case PACCT_DATA_SWAPIN_DELAY_TOTAL:
      entry->swapin_delay_total = val;
      break;

    case PACCT_DATA_SWAPIN_DELAY_AVG:
      entry->swapin_delay_avg = val;
      break;

    case PACCT_DATA_BLKIO_COUNT:
      entry->blkio_count = val;
      break;

    case PACCT_DATA_BLKIO_DELAY_TOTAL:
      entry->blkio_delay_total = val;
      break;

    case PACCT_DATA_BLKIO_DELAY_AVG:
      entry->blkio_delay_avg = val;
      break;

    case PACCT_DATA_IO_STORAGE_READ:
      entry->io_storage_read = val;
      break;

    case PACCT_DATA_IO_STORAGE_WRITE:
      entry->io_storage_write = val;
      break;

    case PACCT_DATA_IO_READ_CHAR:
      entry->io_read_char = val;
      break;

    case PACCT_DATA_IO_WRITE_CHAR:
      entry->io_write_char = val;
      break;

    case PACCT_DATA_IO_READ_SYSCALLS:
      entry->io_read_syscalls = val;
      break;

    case PACCT_DATA_IO_WRITE_SYSCALLS:
      entry->io_write_syscalls = val;
      break;

    case PACCT_DATA_FREEPAGE_COUNT:
      entry->freepage_count = val;
      break;

    case PACCT_DATA_FREEPAGE_DELAY_TOTAL:
      entry->freepage_delay_total = val;
      break;

    case PACCT_DATA_FREEPAGE_DELAY_AVG:
      entry->freepage_delay_avg = val;
      break;

    case PACCT_DATA_TRASHING_COUNT:
      entry->trashing_count = val;
      break;

    case PACCT_DATA_TRASHING_DELAY_TOTAL:
      entry->trashing_delay_total = val;
      break;

    case PACCT_DATA_TRASHING_DELAY_AVG:
      entry->trashing_delay_avg = val;
      break;

    default:
      break;
    }
}

static int
procacct_sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  ProcAcctQueryData *querydata = (ProcAcctQueryData *)data;

  switch (querydata->type)
    {
    case PROCACCT_GET_ENTRIES:
    {
      GPtrArray **entries = (GPtrArray **)querydata->response;
      g_autoptr (TkmProcAcctEntry) entry = tkm_procacct_entry_new ();

      for (gint i = 0; i < argc; i++)
        {
          if (g_strcmp0 (colname[i], "SystemTime") == 0)
            tkm_procacct_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_SYSTEM,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "MonotonicTime") == 0)
            tkm_procacct_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_MONOTONIC,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ReceiveTime") == 0)
            tkm_procacct_entry_set_timestamp (
              entry, DATA_TIME_SOURCE_RECEIVE,
              (guint)g_ascii_strtoull (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "AcComm") == 0)
            tkm_procacct_entry_set_name (entry, argv[i]);
          else if (g_strcmp0 (colname[i], "AcPid") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_PID, g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "AcPPid") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_PPID, g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "AcUid") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_UID, g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "AcGid") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_GID, g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "AcUTime") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_UTIME,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "AcSTime") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_STIME,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "CpuCount") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_CPU_COUNT,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "CpuRunRealTotal") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_CPU_RUN_REAL,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "CpuRunVirtualTotal") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_CPU_RUN_VIRTUAL,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "CpuDelayTotal") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_CPU_DELAY_TOTAL,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "CpuDelayAverage") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_CPU_DELAY_AVG,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "CoreMem") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_CORE_MEM,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "VirtMem") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_VIRT_MEM,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "HiwaterRss") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_HIGH_WATER_RSS,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "HiwaterVm") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_HIGH_WATER_VM,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "Nvcsw") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_NVCSW,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "Nivcsw") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_NIVCSW,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "SwapinCount") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_SWAPIN_COUNT,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "SwapinDelayTotal") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_SWAPIN_DELAY_TOTAL,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "SwapinDelayAverage") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_SWAPIN_DELAY_AVG,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "BlkIOCount") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_BLKIO_COUNT,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "BlkIODelayTotal") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_BLKIO_DELAY_TOTAL,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "BlkIODelayAverage") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_BLKIO_DELAY_AVG,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "IOStorageReadBytes") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_IO_STORAGE_READ,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "IOStorageWriteBytes") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_IO_STORAGE_WRITE,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "IOReadChar") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_IO_READ_CHAR,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "IOWriteChar") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_IO_WRITE_CHAR,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "IOReadSyscalls") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_IO_READ_SYSCALLS,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "IOWriteSyscalls") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_IO_WRITE_SYSCALLS,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "FreePagesCount") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_FREEPAGE_COUNT,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "FreePagesDelayTotal") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_FREEPAGE_DELAY_TOTAL,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "FreePagesDelayAverage") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_FREEPAGE_DELAY_AVG,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ThrashingCount") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_TRASHING_COUNT,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ThrashingDelayTotal") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_TRASHING_DELAY_TOTAL,
              g_ascii_strtoll (argv[i], NULL, 10));
          else if (g_strcmp0 (colname[i], "ThrashingDelayAverage") == 0)
            tkm_procacct_entry_set_data (
              entry, PACCT_DATA_TRASHING_DELAY_AVG,
              g_ascii_strtoll (argv[i], NULL, 10));
        }

      g_ptr_array_add (*entries, tkm_procacct_entry_ref (entry));
      break;
    }

    default:
      break;
    }

  return 0;
}

static void
procacct_entry_free (gpointer data)
{
  TkmProcAcctEntry *e = (TkmProcAcctEntry *)data;

  g_assert (e);
  tkm_procacct_entry_unref (e);
}

GPtrArray *
tkm_procacct_entry_get_all_entries (sqlite3 *db, const char *session_hash,
                                    DataTimeSource time_source,
                                    gulong start_time, gulong end_time,
                                    GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  GPtrArray *entries = g_ptr_array_new ();
  ProcAcctQueryData data
    = { .type = PROCACCT_GET_ENTRIES, .response = (gpointer) & entries };

  g_ptr_array_set_free_func (entries, procacct_entry_free);

  g_assert (db);

  sql = g_strdup_printf ("SELECT * FROM '%s' "
                         "WHERE %s >= %lu AND "
                         " %s < %lu AND SessionId IS "
                         "(SELECT Id FROM '%s' WHERE Hash IS '%s' LIMIT 1);",
                         TKM_PROCACCT_TABLE_NAME,
                         timeSourceColumn[time_source], start_time,
                         timeSourceColumn[time_source], end_time,
                         TKM_SESSIONS_TABLE_NAME, session_hash);
  if (sqlite3_exec (db, sql, procacct_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_ptr_array_free (entries, TRUE);
      entries = NULL;

      g_set_error (error, g_quark_from_static_string ("ProcAcctGetAll"), 1,
                   "SQL query error");
      g_warning ("Fail to get procacct list. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return entries;
}
