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
 * \file tkm-diskstat-entry.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef enum _TkmDiskStatDataType
{
  DISKSTAT_DATA_MAJOR,
  DISKSTAT_DATA_MINOR,
  DISKSTAT_DATA_READS_COMPLETED,
  DISKSTAT_DATA_READS_MERGED,
  DISKSTAT_DATA_READS_SPENT_MS,
  DISKSTAT_DATA_WRITES_COMPLETED,
  DISKSTAT_DATA_WRITES_MERGED,
  DISKSTAT_DATA_WRITES_SPENT_MS,
  DISKSTAT_DATA_IO_INPROGRESS,
  DISKSTAT_DATA_IO_SPENT_MS,
  DISKSTAT_DATA_IO_WEIGHTED_MS,
} TkmDiskStatDataType;

typedef struct _TkmDiskStatEntry
{
  guint idx;
  gchar *name;

  gulong system_time;
  gulong monotonic_time;
  gulong receive_time;

  glong major;
  glong minor;
  glong reads_completed;
  glong reads_merged;
  glong reads_spent_ms;
  glong writes_completed;
  glong writes_merged;
  glong writes_spent_ms;
  glong io_in_progress;
  glong io_spent_ms;
  glong io_weighted_ms;

  grefcount rc;
} TkmDiskStatEntry;

TkmDiskStatEntry *tkm_diskstat_entry_new (void);
TkmDiskStatEntry *tkm_diskstat_entry_ref (TkmDiskStatEntry *entry);
void tkm_diskstat_entry_unref (TkmDiskStatEntry *entry);

guint tkm_diskstat_entry_get_index (TkmDiskStatEntry *entry);
void tkm_diskstat_entry_set_index (TkmDiskStatEntry *entry, guint val);
const gchar *tkm_diskstat_entry_get_name (TkmDiskStatEntry *entry);
void tkm_diskstat_entry_set_name (TkmDiskStatEntry *entry, const gchar *name);

gulong tkm_diskstat_entry_get_timestamp (TkmDiskStatEntry *entry,
                                         DataTimeSource type);
void tkm_diskstat_entry_set_timestamp (TkmDiskStatEntry *entry,
                                       DataTimeSource type, gulong val);

glong tkm_diskstat_entry_get_data (TkmDiskStatEntry *entry,
                                   TkmDiskStatDataType type);
void tkm_diskstat_entry_set_data (TkmDiskStatEntry *entry,
                                  TkmDiskStatDataType type, glong data);

GPtrArray *tkm_diskstat_entry_get_all_entries (
    sqlite3 *db, const char *session_hash, DataTimeSource time_source,
    gulong start_time, gulong end_time, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmDiskStatEntry, tkm_diskstat_entry_unref);

G_END_DECLS
