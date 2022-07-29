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
 * \file tkm-cpustat-entry.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef struct _TkmCpuStatEntry
{
  guint idx;
  gchar *name;

  gulong system_time;
  gulong monotonic_time;
  gulong receive_time;

  guint all;
  guint sys;
  guint usr;

  grefcount rc;
} TkmCpuStatEntry;

TkmCpuStatEntry *tkm_cpustat_entry_new (void);
TkmCpuStatEntry *tkm_cpustat_entry_ref (TkmCpuStatEntry *entry);
void tkm_cpustat_entry_unref (TkmCpuStatEntry *entry);

guint tkm_cpustat_entry_get_index (TkmCpuStatEntry *entry);
void tkm_cpustat_entry_set_index (TkmCpuStatEntry *entry, guint val);
const gchar *tkm_cpustat_entry_get_name (TkmCpuStatEntry *entry);
void tkm_cpustat_entry_set_name (TkmCpuStatEntry *entry, const gchar *name);

gulong tkm_cpustat_entry_get_timestamp (TkmCpuStatEntry *entry,
                                        DataTimeSource type);
void tkm_cpustat_entry_set_timestamp (TkmCpuStatEntry *entry,
                                      DataTimeSource type, gulong val);

guint tkm_cpustat_entry_get_all (TkmCpuStatEntry *entry);
void tkm_cpustat_entry_set_all (TkmCpuStatEntry *entry, guint val);
guint tkm_cpustat_entry_get_sys (TkmCpuStatEntry *entry);
void tkm_cpustat_entry_set_sys (TkmCpuStatEntry *entry, guint val);
guint tkm_cpustat_entry_get_usr (TkmCpuStatEntry *entry);
void tkm_cpustat_entry_set_usr (TkmCpuStatEntry *entry, guint val);

GPtrArray *tkm_cpustat_entry_get_all_entries (sqlite3 *db,
                                              const char *session_hash,
                                              DataTimeSource time_source,
                                              gulong start_time,
                                              gulong end_time, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmCpuStatEntry, tkm_cpustat_entry_unref);

G_END_DECLS
