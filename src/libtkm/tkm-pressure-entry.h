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
 * \file tkm-pressure-entry.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef enum _TkmPressureDataType {
  PSI_DATA_CPU_SOME_AVG10,
  PSI_DATA_CPU_SOME_AVG60,
  PSI_DATA_CPU_SOME_AVG300,
  PSI_DATA_CPU_SOME_TOTAL,
  PSI_DATA_CPU_FULL_AVG10,
  PSI_DATA_CPU_FULL_AVG60,
  PSI_DATA_CPU_FULL_AVG300,
  PSI_DATA_CPU_FULL_TOTAL,
  PSI_DATA_MEM_SOME_AVG10,
  PSI_DATA_MEM_SOME_AVG60,
  PSI_DATA_MEM_SOME_AVG300,
  PSI_DATA_MEM_SOME_TOTAL,
  PSI_DATA_MEM_FULL_AVG10,
  PSI_DATA_MEM_FULL_AVG60,
  PSI_DATA_MEM_FULL_AVG300,
  PSI_DATA_MEM_FULL_TOTAL,
  PSI_DATA_IO_SOME_AVG10,
  PSI_DATA_IO_SOME_AVG60,
  PSI_DATA_IO_SOME_AVG300,
  PSI_DATA_IO_SOME_TOTAL,
  PSI_DATA_IO_FULL_AVG10,
  PSI_DATA_IO_FULL_AVG60,
  PSI_DATA_IO_FULL_AVG300,
  PSI_DATA_IO_FULL_TOTAL,
} TkmPressureDataType;

typedef struct _TkmPressureEntry {
  guint idx;

  gulong system_time;
  gulong monotonic_time;
  gulong receive_time;

  gfloat cpu_some_avg10;
  gfloat cpu_some_avg60;
  gfloat cpu_some_avg300;
  guint cpu_some_total;
  gfloat cpu_full_avg10;
  gfloat cpu_full_avg60;
  gfloat cpu_full_avg300;
  guint cpu_full_total;
  gfloat mem_some_avg10;
  gfloat mem_some_avg60;
  gfloat mem_some_avg300;
  guint mem_some_total;
  gfloat mem_full_avg10;
  gfloat mem_full_avg60;
  gfloat mem_full_avg300;
  guint mem_full_total;
  gfloat io_some_avg10;
  gfloat io_some_avg60;
  gfloat io_some_avg300;
  guint io_some_total;
  gfloat io_full_avg10;
  gfloat io_full_avg60;
  gfloat io_full_avg300;
  guint io_full_total;

  grefcount rc;
} TkmPressureEntry;

TkmPressureEntry *tkm_pressure_entry_new (void);
TkmPressureEntry *tkm_pressure_entry_ref (TkmPressureEntry *entry);
void tkm_pressure_entry_unref (TkmPressureEntry *entry);

guint tkm_pressure_entry_get_index (TkmPressureEntry *entry);
void tkm_pressure_entry_set_index (TkmPressureEntry *entry, guint val);

gulong tkm_pressure_entry_get_timestamp (TkmPressureEntry *entry,
                                         DataTimeSource type);
void tkm_pressure_entry_set_timestamp (TkmPressureEntry *entry,
                                       DataTimeSource type, gulong val);

guint tkm_pressure_entry_get_data_total (TkmPressureEntry *entry,
                                         TkmPressureDataType type);
void tkm_pressure_entry_set_data_total (TkmPressureEntry *entry,
                                        TkmPressureDataType type, guint val);

gfloat tkm_pressure_entry_get_data_avg (TkmPressureEntry *entry,
                                        TkmPressureDataType type);
void tkm_pressure_entry_set_data_avg (TkmPressureEntry *entry,
                                      TkmPressureDataType type, gfloat val);

GPtrArray *tkm_pressure_entry_get_all_entries (
  sqlite3 *db, const char *session_hash, DataTimeSource time_source,
  gulong start_time, gulong end_time, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmPressureEntry, tkm_pressure_entry_unref);

G_END_DECLS
