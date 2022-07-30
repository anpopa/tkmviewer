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
 * \file tkm-wireless-entry.c
 */

#include "tkm-wireless-entry.h"

static const gchar *timeSourceColumn[]
    = { "SystemTime", "MonotonicTime", "ReceiveTime" };

/**
 * @enum Wireless query type
 */
typedef enum _WirelessQueryType
{
  WIRELESS_GET_ENTRIES,
} WirelessQueryType;

/**
 * @enum Wireless query data object
 */
typedef struct _WirelessQueryData
{
  WirelessQueryType type;
  gpointer response;
} WirelessQueryData;

static int wireless_sqlite_callback (void *data, int argc, char **argv,
                                     char **colname);

TkmWirelessEntry *
tkm_wireless_entry_new (void)
{
  TkmWirelessEntry *entry = g_new0 (TkmWirelessEntry, 1);

  g_ref_count_init (&entry->rc);

  return entry;
}

TkmWirelessEntry *
tkm_wireless_entry_ref (TkmWirelessEntry *entry)
{
  g_assert (entry);
  g_ref_count_inc (&entry->rc);
  return entry;
}

void
tkm_wireless_entry_unref (TkmWirelessEntry *entry)
{
  g_assert (entry);

  if (g_ref_count_dec (&entry->rc) == TRUE)
    {
      if (entry->name != NULL)
        g_free (entry->name);
      if (entry->status != NULL)
        g_free (entry->status);

      g_free (entry);
    }
}

guint
tkm_wireless_entry_get_index (TkmWirelessEntry *entry)
{
  g_assert (entry);
  return entry->idx;
}

void
tkm_wireless_entry_set_index (TkmWirelessEntry *entry, guint val)
{
  g_assert (entry);
  entry->idx = val;
}

const gchar *
tkm_wireless_entry_get_name (TkmWirelessEntry *entry)
{
  g_assert (entry);
  return entry->name;
}

void
tkm_wireless_entry_set_name (TkmWirelessEntry *entry, const gchar *name)
{
  g_assert (entry);

  if (entry->name != NULL)
    g_free (entry->name);

  entry->name = g_strdup (name);
}

gulong
tkm_wireless_entry_get_timestamp (TkmWirelessEntry *entry, DataTimeSource type)
{
  g_assert (entry);
  switch (type)
    {
    case DATA_TIME_SOURCE_SYSTEM:
      return entry->system_time;
    case DATA_TIME_SOURCE_MONOTONIC:
      return entry->monotonic_time;
    default:
      break;
    }

  return entry->receive_time;
}

void
tkm_wireless_entry_set_timestamp (TkmWirelessEntry *entry, DataTimeSource type,
                                  gulong val)
{
  g_assert (entry);
  switch (type)
    {
    case DATA_TIME_SOURCE_SYSTEM:
      entry->system_time = val;
      break;
    case DATA_TIME_SOURCE_MONOTONIC:
      entry->monotonic_time = val;
      break;
    case DATA_TIME_SOURCE_RECEIVE:
      entry->receive_time = val;
      break;
    default:
      break;
    }
}

const gchar *
tkm_wireless_entry_get_status (TkmWirelessEntry *entry)
{
  g_assert (entry);
  return entry->status;
}

void
tkm_wireless_entry_set_status (TkmWirelessEntry *entry, const gchar *status)
{
  g_assert (entry);

  if (entry->status != NULL)
    g_free (entry->status);

  entry->status = g_strdup (status);
}

glong
tkm_wireless_entry_get_data (TkmWirelessEntry *entry, TkmWirelessDataType type)
{
  g_assert (entry);

  switch (type)
    {
    case WLAN_DATA_QUALITY_LINK:
      return entry->quality_link;
    case WLAN_DATA_QUALITY_LEVEL:
      return entry->quality_level;
    case WLAN_DATA_QUALITY_NOISE:
      return entry->quality_noise;
    case WLAN_DATA_DISCARDED_NWID:
      return entry->discarded_nwid;
    case WLAN_DATA_DISCARDED_CRYPT:
      return entry->discarded_crypt;
    case WLAN_DATA_DISCARDED_FRAG:
      return entry->discarded_frag;
    case WLAN_DATA_DISCARDED_MISC:
      return entry->discarded_misc;
    case WLAN_DATA_MISSED_BEACON:
      return entry->missed_beacon;
    default:
      break;
    }

  return 0;
}

void
tkm_wireless_entry_set_data (TkmWirelessEntry *entry, TkmWirelessDataType type,
                             glong data)
{
  g_assert (entry);

  switch (type)
    {
    case WLAN_DATA_QUALITY_LINK:
      entry->quality_link = data;
      break;
    case WLAN_DATA_QUALITY_LEVEL:
      entry->quality_level = data;
      break;
    case WLAN_DATA_QUALITY_NOISE:
      entry->quality_noise = data;
      break;
    case WLAN_DATA_DISCARDED_NWID:
      entry->discarded_nwid = data;
      break;
    case WLAN_DATA_DISCARDED_CRYPT:
      entry->discarded_crypt = data;
      break;
    case WLAN_DATA_DISCARDED_FRAG:
      entry->discarded_frag = data;
      break;
    case WLAN_DATA_DISCARDED_MISC:
      entry->discarded_misc = data;
      break;
    case WLAN_DATA_MISSED_BEACON:
      entry->missed_beacon = data;
      break;
    default:
      break;
    }
}

static int
wireless_sqlite_callback (void *data, int argc, char **argv, char **colname)
{
  WirelessQueryData *querydata = (WirelessQueryData *)data;

  switch (querydata->type)
    {
    case WIRELESS_GET_ENTRIES:
      {
        GPtrArray **entries = (GPtrArray **)querydata->response;
        g_autoptr (TkmWirelessEntry) entry = tkm_wireless_entry_new ();

        for (gint i = 0; i < argc; i++)
          {
            if (g_strcmp0 (colname[i], "Name") == 0)
              tkm_wireless_entry_set_name (entry, argv[i]);
            else if (g_strcmp0 (colname[i], "Status") == 0)
              tkm_wireless_entry_set_status (entry, argv[i]);
            else if (g_strcmp0 (colname[i], "QualityLink") == 0)
              tkm_wireless_entry_set_data (
                  entry, WLAN_DATA_QUALITY_LINK,
                  g_ascii_strtoll (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "QualityLevel") == 0)
              tkm_wireless_entry_set_data (
                  entry, WLAN_DATA_QUALITY_LEVEL,
                  g_ascii_strtoll (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "QualityNoise") == 0)
              tkm_wireless_entry_set_data (
                  entry, WLAN_DATA_QUALITY_NOISE,
                  g_ascii_strtoll (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "DiscardedNWId") == 0)
              tkm_wireless_entry_set_data (
                  entry, WLAN_DATA_DISCARDED_NWID,
                  g_ascii_strtoll (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "DiscardedCrypt") == 0)
              tkm_wireless_entry_set_data (
                  entry, WLAN_DATA_DISCARDED_CRYPT,
                  g_ascii_strtoll (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "DiscardedFrag") == 0)
              tkm_wireless_entry_set_data (
                  entry, WLAN_DATA_DISCARDED_FRAG,
                  g_ascii_strtoll (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "DiscardedMisc") == 0)
              tkm_wireless_entry_set_data (
                  entry, WLAN_DATA_DISCARDED_MISC,
                  g_ascii_strtoll (argv[i], NULL, 10));
            else if (g_strcmp0 (colname[i], "MissedBeacon") == 0)
              tkm_wireless_entry_set_data (
                  entry, WLAN_DATA_MISSED_BEACON,
                  g_ascii_strtoll (argv[i], NULL, 10));
          }

        g_ptr_array_add (*entries, tkm_wireless_entry_ref (entry));
        break;
      }

    default:
      break;
    }

  return 0;
}

static void
wireless_entry_free (gpointer data)
{
  TkmWirelessEntry *e = (TkmWirelessEntry *)data;

  g_assert (e);
  tkm_wireless_entry_unref (e);
}

GPtrArray *
tkm_wireless_entry_get_all_entries (sqlite3 *db, const char *session_hash,
                                    DataTimeSource time_source,
                                    gulong start_time, gulong end_time,
                                    GError **error)
{
  g_autofree gchar *sql = NULL;
  gchar *query_error = NULL;
  GPtrArray *entries = g_ptr_array_new ();
  WirelessQueryData data
      = { .type = WIRELESS_GET_ENTRIES, .response = (gpointer)&entries };

  g_ptr_array_set_free_func (entries, wireless_entry_free);

  g_assert (db);

  sql = g_strdup_printf ("SELECT * FROM '%s' "
                         "WHERE %s >= %lu AND "
                         " %s < %lu AND SessionId IS "
                         "(SELECT Id FROM '%s' WHERE Hash IS '%s' LIMIT 1);",
                         TKM_WIRELESS_TABLE_NAME,
                         timeSourceColumn[time_source], start_time,
                         timeSourceColumn[time_source], end_time,
                         TKM_SESSIONS_TABLE_NAME, session_hash);
  if (sqlite3_exec (db, sql, wireless_sqlite_callback, &data, &query_error)
      != SQLITE_OK)
    {
      g_ptr_array_free (entries, TRUE);
      entries = NULL;

      g_set_error (error, g_quark_from_static_string ("WirelessGetAll"), 1,
                   "SQL query error");
      g_warning ("Fail to get wireless list. SQL error %s", query_error);
      sqlite3_free (query_error);
    }

  return entries;
}
