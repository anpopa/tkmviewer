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
 * \file tkm-wireless-entry.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef enum _TkmWirelessDataType
{
  WLAN_DATA_QUALITY_LINK,
  WLAN_DATA_QUALITY_LEVEL,
  WLAN_DATA_QUALITY_NOISE,
  WLAN_DATA_DISCARDED_NWID,
  WLAN_DATA_DISCARDED_CRYPT,
  WLAN_DATA_DISCARDED_FRAG,
  WLAN_DATA_DISCARDED_MISC,
  WLAN_DATA_MISSED_BEACON
} TkmWirelessDataType;

typedef struct _TkmWirelessEntry
{
  guint idx;
  gchar *name;

  gulong system_time;
  gulong monotonic_time;
  gulong receive_time;

  gchar *status;
  glong quality_link;
  glong quality_level;
  glong quality_noise;
  glong discarded_nwid;
  glong discarded_crypt;
  glong discarded_frag;
  glong discarded_misc;
  glong missed_beacon;

  grefcount rc;
} TkmWirelessEntry;

TkmWirelessEntry *tkm_wireless_entry_new (void);
TkmWirelessEntry *tkm_wireless_entry_ref (TkmWirelessEntry *entry);
void tkm_wireless_entry_unref (TkmWirelessEntry *entry);

guint tkm_wireless_entry_get_index (TkmWirelessEntry *entry);
void tkm_wireless_entry_set_index (TkmWirelessEntry *entry, guint val);
const gchar *tkm_wireless_entry_get_name (TkmWirelessEntry *entry);
void tkm_wireless_entry_set_name (TkmWirelessEntry *entry, const gchar *name);

gulong tkm_wireless_entry_get_timestamp (TkmWirelessEntry *entry,
                                         DataTimeSource type);
void tkm_wireless_entry_set_timestamp (TkmWirelessEntry *entry,
                                       DataTimeSource type, gulong val);

const gchar *tkm_wireless_entry_get_status (TkmWirelessEntry *entry);
void tkm_wireless_entry_set_status (TkmWirelessEntry *entry,
                                    const gchar *name);
glong tkm_wireless_entry_get_data (TkmWirelessEntry *entry,
                                   TkmWirelessDataType type);
void tkm_wireless_entry_set_data (TkmWirelessEntry *entry,
                                  TkmWirelessDataType type, glong data);

GPtrArray *tkm_wireless_entry_get_all_entries (
    sqlite3 *db, const char *session_hash, DataTimeSource time_source,
    gulong start_time, gulong end_time, GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmWirelessEntry, tkm_wireless_entry_unref);

G_END_DECLS
