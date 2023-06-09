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
 * \file tkm-ctxinfo-entry.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef enum _TkmCtxInfoDataType {
  CTXINFO_DATA_CPU_TIME,
  CTXINFO_DATA_CPU_PERCENT,
  CTXINFO_DATA_MEM_RSS,
  CTXINFO_DATA_MEM_PSS,
} TkmCtxInfoDataType;

typedef struct _TkmCtxInfoEntry {
  guint idx;
  gchar *name;

  gulong system_time;
  gulong monotonic_time;
  gulong receive_time;

  gchar *id;
  glong cpu_time;
  glong cpu_percent;
  glong mem_rss;
  glong mem_pss;

  grefcount rc;
} TkmCtxInfoEntry;

TkmCtxInfoEntry *tkm_ctxinfo_entry_new (void);
TkmCtxInfoEntry *tkm_ctxinfo_entry_ref (TkmCtxInfoEntry *entry);
void tkm_ctxinfo_entry_unref (TkmCtxInfoEntry *entry);

guint tkm_ctxinfo_entry_get_index (TkmCtxInfoEntry *entry);
void tkm_ctxinfo_entry_set_index (TkmCtxInfoEntry *entry, guint val);
const gchar *tkm_ctxinfo_entry_get_name (TkmCtxInfoEntry *entry);
void tkm_ctxinfo_entry_set_name (TkmCtxInfoEntry *entry, const gchar *name);

gulong tkm_ctxinfo_entry_get_timestamp (TkmCtxInfoEntry *entry,
                                        DataTimeSource type);
void tkm_ctxinfo_entry_set_timestamp (TkmCtxInfoEntry *entry,
                                      DataTimeSource type, gulong val);

const gchar *tkm_ctxinfo_entry_get_id (TkmCtxInfoEntry *entry);
void tkm_ctxinfo_entry_set_id (TkmCtxInfoEntry *entry, const gchar *id);

glong tkm_ctxinfo_entry_get_data (TkmCtxInfoEntry *entry,
                                  TkmCtxInfoDataType type);
void tkm_ctxinfo_entry_set_data (TkmCtxInfoEntry *entry,
                                 TkmCtxInfoDataType type, glong data);

GPtrArray *tkm_ctxinfo_entry_get_all_entries (sqlite3 *db,
                                              const char *session_hash,
                                              DataTimeSource time_source,
                                              gulong start_time,
                                              gulong end_time, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmCtxInfoEntry, tkm_ctxinfo_entry_unref);

G_END_DECLS
