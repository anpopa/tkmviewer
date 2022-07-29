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
 * \file tkm-meminfo-entry.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef enum _TkmMemInfoDataType
{
  MINFO_DATA_MEM_TOTAL,
  MINFO_DATA_MEM_FREE,
  MINFO_DATA_MEM_AVAIL,
  MINFO_DATA_MEM_CACHED,
  MINFO_DATA_MEM_PERCENT,
  MINFO_DATA_SWAP_TOTAL,
  MINFO_DATA_SWAP_FREE,
  MINFO_DATA_SWAP_CACHED,
  MINFO_DATA_SWAP_PERCENT,
  MINFO_DATA_CMA_TOTAL,
  MINFO_DATA_CMA_FREE,
} TkmMemInfoDataType;

typedef struct _TkmMemInfoEntry
{
  guint idx;

  gulong system_time;
  gulong monotonic_time;
  gulong receive_time;

  guint mem_total;
  guint mem_free;
  guint mem_avail;
  guint mem_cached;
  guint mem_percent;
  guint swap_total;
  guint swap_free;
  guint swap_cached;
  guint swap_percent;
  guint cma_total;
  guint cma_free;

  grefcount rc;
} TkmMemInfoEntry;

TkmMemInfoEntry *tkm_meminfo_entry_new (void);
TkmMemInfoEntry *tkm_meminfo_entry_ref (TkmMemInfoEntry *entry);
void tkm_meminfo_entry_unref (TkmMemInfoEntry *entry);

guint tkm_meminfo_entry_get_index (TkmMemInfoEntry *entry);
void tkm_meminfo_entry_set_index (TkmMemInfoEntry *entry, guint val);

gulong tkm_meminfo_entry_get_timestamp (TkmMemInfoEntry *entry,
                                        DataTimeSource type);
void tkm_cmeminfo_entry_set_timestamp (TkmMemInfoEntry *entry,
                                       DataTimeSource type, gulong val);

guint tkm_meminfo_entry_get_data (TkmMemInfoEntry *entry,
                                  TkmMemInfoDataType type);
void tkm_meminfo_entry_set_data (TkmMemInfoEntry *entry,
                                 TkmMemInfoDataType type, guint val);

GPtrArray *tkm_meminfo_entry_get_all_entries (sqlite3 *db,
                                              const char *session_hash,
                                              DataTimeSource time_source,
                                              gulong start_time,
                                              gulong end_time, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmMemInfoEntry, tkm_meminfo_entry_unref);

G_END_DECLS
