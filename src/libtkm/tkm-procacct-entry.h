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
 * \file tkm-procacct-entry.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef enum _TkmProcAcctDataType {
  PACCT_DATA_PID,
  PACCT_DATA_PPID,
  PACCT_DATA_UID,
  PACCT_DATA_GID,
  PACCT_DATA_UTIME,
  PACCT_DATA_STIME,
  PACCT_DATA_CPU_COUNT,
  PACCT_DATA_CPU_RUN_REAL,
  PACCT_DATA_CPU_RUN_VIRTUAL,
  PACCT_DATA_CPU_DELAY_TOTAL,
  PACCT_DATA_CPU_DELAY_AVG,
  PACCT_DATA_CORE_MEM,
  PACCT_DATA_VIRT_MEM,
  PACCT_DATA_HIGH_WATER_RSS,
  PACCT_DATA_HIGH_WATER_VM,
  PACCT_DATA_NVCSW,
  PACCT_DATA_NIVCSW,
  PACCT_DATA_SWAPIN_COUNT,
  PACCT_DATA_SWAPIN_DELAY_TOTAL,
  PACCT_DATA_SWAPIN_DELAY_AVG,
  PACCT_DATA_BLKIO_COUNT,
  PACCT_DATA_BLKIO_DELAY_TOTAL,
  PACCT_DATA_BLKIO_DELAY_AVG,
  PACCT_DATA_IO_STORAGE_READ,
  PACCT_DATA_IO_STORAGE_WRITE,
  PACCT_DATA_IO_READ_CHAR,
  PACCT_DATA_IO_WRITE_CHAR,
  PACCT_DATA_IO_READ_SYSCALLS,
  PACCT_DATA_IO_WRITE_SYSCALLS,
  PACCT_DATA_FREEPAGE_COUNT,
  PACCT_DATA_FREEPAGE_DELAY_TOTAL,
  PACCT_DATA_FREEPAGE_DELAY_AVG,
  PACCT_DATA_TRASHING_COUNT,
  PACCT_DATA_TRASHING_DELAY_TOTAL,
  PACCT_DATA_TRASHING_DELAY_AVG,
} TkmProcAcctDataType;

typedef struct _TkmProcAcctEntry {
  guint idx;
  gchar *name;

  gulong system_time;
  gulong monotonic_time;
  gulong receive_time;

  glong pid;
  glong ppid;
  glong uid;
  glong gid;
  glong utime;
  glong stime;
  glong cpu_count;
  glong cpu_run_real;
  glong cpu_run_virtual;
  glong cpu_delay_total;
  glong cpu_delay_avg;
  glong core_mem;
  glong virt_mem;
  glong high_water_rss;
  glong high_water_vm;
  glong nvcsw;
  glong nivcsw;
  glong swapin_count;
  glong swapin_delay_total;
  glong swapin_delay_avg;
  glong blkio_count;
  glong blkio_delay_total;
  glong blkio_delay_avg;
  glong io_storage_read;
  glong io_storage_write;
  glong io_read_char;
  glong io_write_char;
  glong io_read_syscalls;
  glong io_write_syscalls;
  glong freepage_count;
  glong freepage_delay_total;
  glong freepage_delay_avg;
  glong trashing_count;
  glong trashing_delay_total;
  glong trashing_delay_avg;

  grefcount rc;
} TkmProcAcctEntry;

TkmProcAcctEntry *tkm_procacct_entry_new (void);
TkmProcAcctEntry *tkm_procacct_entry_ref (TkmProcAcctEntry *entry);
void tkm_procacct_entry_unref (TkmProcAcctEntry *entry);

guint tkm_procacct_entry_get_index (TkmProcAcctEntry *entry);
void tkm_procacct_entry_set_index (TkmProcAcctEntry *entry, guint val);
const gchar *tkm_procacct_entry_get_name (TkmProcAcctEntry *entry);
void tkm_procacct_entry_set_name (TkmProcAcctEntry *entry, const gchar *name);

gulong tkm_procacct_entry_get_timestamp (TkmProcAcctEntry *entry,
                                         DataTimeSource type);
void tkm_procacct_entry_set_timestamp (TkmProcAcctEntry *entry,
                                       DataTimeSource type, gulong val);

glong tkm_procacct_entry_get_data (TkmProcAcctEntry *entry,
                                   TkmProcAcctDataType type);
void tkm_procacct_entry_set_data (TkmProcAcctEntry *entry,
                                  TkmProcAcctDataType type, glong data);

GPtrArray *tkm_procacct_entry_get_all_entries (
  sqlite3 *db, const char *session_hash, DataTimeSource time_source,
  gulong start_time, gulong end_time, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmProcAcctEntry, tkm_procacct_entry_unref);

G_END_DECLS
