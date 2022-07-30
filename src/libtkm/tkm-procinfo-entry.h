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
 * \file tkm-procinfo-entry.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef enum _TkmProcInfoDataType
{
  PINFO_DATA_PID,
  PINFO_DATA_PPID,
  PINFO_DATA_CPU_TIME,
  PINFO_DATA_CPU_PERCENT,
  PINFO_DATA_VMRSS,
} TkmProcInfoDataType;

typedef struct _TkmProcInfoEntry
{
  guint idx;
  gchar *name;

  gulong system_time;
  gulong monotonic_time;
  gulong receive_time;

  glong pid;
  glong ppid;
  gchar *context;
  glong cpu_time;
  glong cpu_percent;
  glong vm_rss;

  grefcount rc;
} TkmProcInfoEntry;

TkmProcInfoEntry *tkm_procinfo_entry_new (void);
TkmProcInfoEntry *tkm_procinfo_entry_ref (TkmProcInfoEntry *entry);
void tkm_procinfo_entry_unref (TkmProcInfoEntry *entry);

guint tkm_procinfo_entry_get_index (TkmProcInfoEntry *entry);
void tkm_procinfo_entry_set_index (TkmProcInfoEntry *entry, guint val);
const gchar *tkm_procinfo_entry_get_name (TkmProcInfoEntry *entry);
void tkm_procinfo_entry_set_name (TkmProcInfoEntry *entry, const gchar *name);

gulong tkm_procinfo_entry_get_timestamp (TkmProcInfoEntry *entry,
                                         DataTimeSource type);
void tkm_procinfo_entry_set_timestamp (TkmProcInfoEntry *entry,
                                       DataTimeSource type, gulong val);

const gchar *tkm_procinfo_entry_get_context (TkmProcInfoEntry *entry);
void tkm_procinfo_entry_set_context (TkmProcInfoEntry *entry,
                                     const gchar *context);

glong tkm_procinfo_entry_get_data (TkmProcInfoEntry *entry,
                                   TkmProcInfoDataType type);
void tkm_procinfo_entry_set_data (TkmProcInfoEntry *entry,
                                  TkmProcInfoDataType type, glong data);

GPtrArray *tkm_procinfo_entry_get_all_entries (
    sqlite3 *db, const char *session_hash, DataTimeSource time_source,
    gulong start_time, gulong end_time, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmProcInfoEntry, tkm_procinfo_entry_unref);

G_END_DECLS
