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
 * \file tkm-procevent-entry.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef enum _TkmProcEventDataType
{
  PEVENT_DATA_FORKS,
  PEVENT_DATA_EXECS,
  PEVENT_DATA_EXITS,
  PEVENT_DATA_UIDS,
  PEVENT_DATA_GIDS,
} TkmProcEventDataType;

typedef struct _TkmProcEventEntry
{
  gulong system_time;
  gulong monotonic_time;
  gulong receive_time;

  guint forks;
  guint execs;
  guint exits;
  guint uids;
  guint gids;

  grefcount rc;
} TkmProcEventEntry;

TkmProcEventEntry *tkm_procevent_entry_new (void);
TkmProcEventEntry *tkm_procevent_entry_ref (TkmProcEventEntry *entry);
void tkm_procevent_entry_unref (TkmProcEventEntry *entry);

gulong tkm_procevent_entry_get_timestamp (TkmProcEventEntry *entry,
                                          DataTimeSource type);
void tkm_procevent_entry_set_timestamp (TkmProcEventEntry *entry,
                                        DataTimeSource type, gulong val);

guint tkm_procevent_entry_get_data (TkmProcEventEntry *entry,
                                    TkmProcEventDataType type);
void tkm_procevent_entry_set_data (TkmProcEventEntry *entry,
                                   TkmProcEventDataType type, guint val);

GPtrArray *tkm_procevent_entry_get_all_entries (
    sqlite3 *db, const char *session_hash, DataTimeSource time_source,
    gulong start_time, gulong end_time, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmProcEventEntry, tkm_procevent_entry_unref);

G_END_DECLS
