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
 * \file tkm-buddyinfo-entry.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef struct _TkmBuddyInfoEntry {
  guint idx;
  gchar *name;

  gulong system_time;
  gulong monotonic_time;
  gulong receive_time;

  gchar *zone;
  gchar *data;

  grefcount rc;
} TkmBuddyInfoEntry;

TkmBuddyInfoEntry *tkm_buddyinfo_entry_new (void);
TkmBuddyInfoEntry *tkm_buddyinfo_entry_ref (TkmBuddyInfoEntry *entry);
void tkm_buddyinfo_entry_unref (TkmBuddyInfoEntry *entry);

guint tkm_buddyinfo_entry_get_index (TkmBuddyInfoEntry *entry);
void tkm_buddyinfo_entry_set_index (TkmBuddyInfoEntry *entry, guint val);
const gchar *tkm_buddyinfo_entry_get_name (TkmBuddyInfoEntry *entry);
void tkm_buddyinfo_entry_set_name (TkmBuddyInfoEntry *entry,
                                   const gchar *name);

gulong tkm_buddyinfo_entry_get_timestamp (TkmBuddyInfoEntry *entry,
                                          DataTimeSource type);
void tkm_buddyinfo_entry_set_timestamp (TkmBuddyInfoEntry *entry,
                                        DataTimeSource type, gulong val);

const gchar *tkm_buddyinfo_entry_get_zone (TkmBuddyInfoEntry *entry);
void tkm_buddyinfo_entry_set_zone (TkmBuddyInfoEntry *entry,
                                   const gchar *zone);
const gchar *tkm_buddyinfo_entry_get_data (TkmBuddyInfoEntry *entry);
void tkm_buddyinfo_entry_set_data (TkmBuddyInfoEntry *entry,
                                   const gchar *data);

GPtrArray *tkm_buddyinfo_entry_get_all_entries (
  sqlite3 *db, const char *session_hash, DataTimeSource time_source,
  gulong start_time, gulong end_time, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmBuddyInfoEntry, tkm_buddyinfo_entry_unref);

G_END_DECLS
