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
 * \file tkm-entry.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef struct _TkmSessionEntry
{
  gint idx; /**< Main list index */

  gchar *hash;
  gchar *name;
  guint first_system_ts;
  guint first_monotonic_ts;
  guint first_receive_ts;
  guint last_system_ts;
  guint last_monotonic_ts;
  guint last_receive_ts;
  gboolean active;

  grefcount rc; /**< Reference counter variable  */
} TkmSessionEntry;

TkmSessionEntry *tkm_session_entry_new (gint idx);
TkmSessionEntry *tkm_session_entry_ref (TkmSessionEntry *entry);
void tkm_session_entry_unref (TkmSessionEntry *entry);

const gchar *tkm_session_entry_get_hash (TkmSessionEntry *entry);
void tkm_session_entry_set_hash (TkmSessionEntry *entry, const gchar *hash);

const gchar *tkm_session_entry_get_name (TkmSessionEntry *entry);
void tkm_session_entry_set_name (TkmSessionEntry *entry, const gchar *name);

guint tkm_session_entry_get_first_timestamp (TkmSessionEntry *entry,
                                             DataTimeSource type);
void tkm_session_entry_set_first_timestamp (TkmSessionEntry *entry,
                                            DataTimeSource type, guint ts);

guint tkm_session_entry_get_last_timestamp (TkmSessionEntry *entry,
                                            DataTimeSource type);
void tkm_session_entry_set_last_timestamp (TkmSessionEntry *entry,
                                           DataTimeSource type, guint ts);

void tkm_session_entry_set_active (TkmSessionEntry *entry, gboolean state);
gboolean tkm_session_entry_get_active (TkmSessionEntry *entry);

void tkm_session_entry_set_index (TkmSessionEntry *entry, gint idx);
gint tkm_session_entry_get_index (TkmSessionEntry *entry);

GPtrArray *tkm_session_entry_get_all (sqlite3 *db, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmSessionEntry, tkm_session_entry_unref);

G_END_DECLS
