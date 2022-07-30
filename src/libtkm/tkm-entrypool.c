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
 * \file tkm-entrypool.c
 */

#include "tkm-entrypool.h"
#include "tkm-buddyinfo-entry.h"
#include "tkm-cpustat-entry.h"
#include "tkm-ctxinfo-entry.h"
#include "tkm-diskstat-entry.h"
#include "tkm-meminfo-entry.h"
#include "tkm-pressure-entry.h"
#include "tkm-procevent-entry.h"
#include "tkm-procinfo-entry.h"
#include "tkm-session-entry.h"
#include "tkm-wireless-entry.h"

#include <fcntl.h>

/**
 * @brief Post new event
 *
 * @param entrypool A pointer to the entrypool object
 * @param action Action the event has to process
 */
static void post_entrypool_event (TkmEntryPool *entrypool,
                                  TkmEntryPoolEvent *event);

/**
 * @brief GSource prepare function
 */
static gboolean entrypool_source_prepare (GSource *source, gint *timeout);

/**
 * @brief GSource dispatch function
 */
static gboolean entrypool_source_dispatch (GSource *source,
                                           GSourceFunc callback,
                                           gpointer _entrypool);

/**
 * @brief GSource callback function
 */
static gboolean entrypool_source_callback (gpointer _entrypool,
                                           gpointer _event);

/**
 * @brief GSource destroy notification callback function
 */
static void entrypool_source_destroy_notify (gpointer _entrypool);

/**
 * @brief Async queue destroy notification callback function
 */
static void entrypool_queue_destroy_notify (gpointer _entrypool);

/**
 * @brief Close database
 */
static void close_database (TkmEntryPool *entrypool);

/**
 * @brief Process open database
 */
static void do_open_database_file (TkmEntryPool *entrypool,
                                   TkmEntryPoolEvent *event);

/**
 * @brief Load sessions
 */
static void do_load_sessions (TkmEntryPool *entrypool,
                              TkmEntryPoolEvent *event);

/**
 * @brief Load data
 */
static void do_load_data (TkmEntryPool *entrypool, TkmEntryPoolEvent *event);

/**
 * @brief GSourceFuncs vtable
 */
static GSourceFuncs entrypool_source_funcs = {
  entrypool_source_prepare, NULL, entrypool_source_dispatch, NULL, NULL, NULL,
};

static void
post_entrypool_event (TkmEntryPool *entrypool, TkmEntryPoolEvent *event)
{
  g_assert (entrypool);
  g_assert (event);

  g_async_queue_push (entrypool->queue, event);
  if (entrypool->context != NULL)
    g_main_context_wakeup (entrypool->context);
}

static gboolean
entrypool_source_prepare (GSource *source, gint *timeout)
{
  TkmEntryPool *entrypool = (TkmEntryPool *)source;

  TKM_UNUSED (timeout);

  return (g_async_queue_length (entrypool->queue) > 0);
}

static gboolean
entrypool_source_dispatch (GSource *source, GSourceFunc callback,
                           gpointer _entrypool)
{
  TkmEntryPool *entrypool = (TkmEntryPool *)source;
  gpointer event = g_async_queue_try_pop (entrypool->queue);

  TKM_UNUSED (callback);
  TKM_UNUSED (_entrypool);

  if (event == NULL)
    return G_SOURCE_CONTINUE;

  return entrypool->callback (entrypool, event) == TRUE ? G_SOURCE_CONTINUE
                                                        : G_SOURCE_REMOVE;
}

static gboolean
entrypool_source_callback (gpointer _entrypool, gpointer _event)
{
  TkmEntryPool *entrypool = (TkmEntryPool *)_entrypool;
  TkmEntryPoolEvent *event = (TkmEntryPoolEvent *)_event;

  g_assert (entrypool);
  g_assert (event);

  switch (event->type)
    {
    case EPOOL_EVENT_OPEN_DATABASE_FILE:
      do_open_database_file (entrypool, event);
      break;

    case EPOOL_EVENT_LOAD_SESSIONS:
      do_load_sessions (entrypool, event);
      break;

    case EPOOL_EVENT_LOAD_DATA:
      do_load_data (entrypool, event);
      break;

    default:
      break;
    }

  tkm_action_unref (event->action);
  g_free (event);

  return TRUE;
}

static void
main_entries_free (TkmEntryPool *entrypool)
{
  g_assert (entrypool);

  if (entrypool->procinfo_entries != NULL)
    {
      g_ptr_array_free (entrypool->procinfo_entries, TRUE);
      entrypool->procinfo_entries = NULL;
    }

  if (entrypool->ctxinfo_entries != NULL)
    {
      g_ptr_array_free (entrypool->ctxinfo_entries, TRUE);
      entrypool->ctxinfo_entries = NULL;
    }

  if (entrypool->procacct_entries != NULL)
    {
      g_ptr_array_free (entrypool->procacct_entries, TRUE);
      entrypool->procacct_entries = NULL;
    }

  if (entrypool->cpustat_entries != NULL)
    {
      g_ptr_array_free (entrypool->cpustat_entries, TRUE);
      entrypool->cpustat_entries = NULL;
    }

  if (entrypool->meminfo_entries != NULL)
    {
      g_ptr_array_free (entrypool->meminfo_entries, TRUE);
      entrypool->meminfo_entries = NULL;
    }

  if (entrypool->procevent_entries != NULL)
    {
      g_ptr_array_free (entrypool->procevent_entries, TRUE);
      entrypool->procevent_entries = NULL;
    }

  if (entrypool->pressure_entries != NULL)
    {
      g_ptr_array_free (entrypool->pressure_entries, TRUE);
      entrypool->pressure_entries = NULL;
    }

  if (entrypool->buddyinfo_entries != NULL)
    {
      g_ptr_array_free (entrypool->buddyinfo_entries, TRUE);
      entrypool->buddyinfo_entries = NULL;
    }

  if (entrypool->wireless_entries != NULL)
    {
      g_ptr_array_free (entrypool->wireless_entries, TRUE);
      entrypool->wireless_entries = NULL;
    }

  if (entrypool->diskstat_entries != NULL)
    {
      g_ptr_array_free (entrypool->diskstat_entries, TRUE);
      entrypool->diskstat_entries = NULL;
    }
}

static void
do_load_sessions (TkmEntryPool *entrypool, TkmEntryPoolEvent *event)
{
  TkmActionStatusCallback callback = tkm_action_get_callback (event->action);
  g_autoptr (GError) error = NULL;

  g_assert (entrypool);
  g_assert (event);

  tkm_entrypool_data_lock (entrypool);

  if (entrypool->session_entries != NULL)
    {
      g_ptr_array_free (entrypool->session_entries, FALSE);
      entrypool->session_entries = NULL;
    }

  entrypool->session_entries
      = tkm_session_entry_get_all_entries (entrypool->input_database, &error);

  tkm_entrypool_data_unlock (entrypool);

  if (error != NULL)
    {
      g_warning ("Fail to load sessions %s", error->message);
      if (callback != NULL)
        callback (ACTION_STATUS_FAILED, event->action);
    }
  else
    {
      if (callback != NULL)
        callback (ACTION_STATUS_COMPLETE, event->action);
    }
}

static void
do_load_data (TkmEntryPool *entrypool, TkmEntryPoolEvent *event)
{
  TkmActionStatusCallback callback = tkm_action_get_callback (event->action);
  TkmSessionEntry *active_session = NULL;
  const gchar *session_hash = NULL;
  gulong start_timestamp = 0;
  gulong end_timestamp = 0;
  gulong last_timestamp = 0;
  GList *ts_node = NULL;
  GList *args = NULL;

  g_assert (entrypool);
  g_assert (event);

  args = tkm_action_get_args (event->action);
  g_assert (args);

  session_hash = (const gchar *)(g_list_first (args)->data);

  ts_node = g_list_next (args);
  start_timestamp = g_ascii_strtoull (ts_node->data, NULL, 10);

  tkm_entrypool_data_lock (entrypool);

  /* cleanup existing entries */
  main_entries_free (entrypool);

  for (guint i = 0; i < entrypool->session_entries->len; i++)
    {
      if (tkm_session_entry_get_active (
              g_ptr_array_index (entrypool->session_entries, i)))
        {
          active_session = g_ptr_array_index (entrypool->session_entries, i);
        }
    }
  last_timestamp = tkm_session_entry_get_last_timestamp (
      active_session, tkm_settings_get_data_time_source (entrypool->settings));

  switch (tkm_settings_get_data_time_interval (entrypool->settings))
    {
    case DATA_TIME_INTERVAL_10S:
      end_timestamp = (start_timestamp + 10) < last_timestamp
                          ? (start_timestamp + 10)
                          : last_timestamp;
      break;
    case DATA_TIME_INTERVAL_1M:
      end_timestamp = (start_timestamp + 60) < last_timestamp
                          ? (start_timestamp + 60)
                          : last_timestamp;
      break;
    case DATA_TIME_INTERVAL_10M:
      end_timestamp = (start_timestamp + 600) < last_timestamp
                          ? (start_timestamp + 600)
                          : last_timestamp;
      break;
    case DATA_TIME_INTERVAL_1H:
      end_timestamp = (start_timestamp + 3600) < last_timestamp
                          ? (start_timestamp + 3600)
                          : last_timestamp;
      break;
    case DATA_TIME_INTERVAL_24H:
      end_timestamp = (start_timestamp + 86400) < last_timestamp
                          ? (start_timestamp + 86400)
                          : last_timestamp;
      break;
    default:
      end_timestamp = last_timestamp;
      break;
    }

  entrypool->cpustat_entries = tkm_cpustat_entry_get_all_entries (
      entrypool->input_database, session_hash,
      tkm_settings_get_data_time_source (entrypool->settings), start_timestamp,
      end_timestamp, NULL);

  entrypool->meminfo_entries = tkm_meminfo_entry_get_all_entries (
      entrypool->input_database, session_hash,
      tkm_settings_get_data_time_source (entrypool->settings), start_timestamp,
      end_timestamp, NULL);

  entrypool->procevent_entries = tkm_procevent_entry_get_all_entries (
      entrypool->input_database, session_hash,
      tkm_settings_get_data_time_source (entrypool->settings), start_timestamp,
      end_timestamp, NULL);

  entrypool->pressure_entries = tkm_pressure_entry_get_all_entries (
      entrypool->input_database, session_hash,
      tkm_settings_get_data_time_source (entrypool->settings), start_timestamp,
      end_timestamp, NULL);

  entrypool->buddyinfo_entries = tkm_buddyinfo_entry_get_all_entries (
      entrypool->input_database, session_hash,
      tkm_settings_get_data_time_source (entrypool->settings), start_timestamp,
      end_timestamp, NULL);

  entrypool->wireless_entries = tkm_wireless_entry_get_all_entries (
      entrypool->input_database, session_hash,
      tkm_settings_get_data_time_source (entrypool->settings), start_timestamp,
      end_timestamp, NULL);

  entrypool->diskstat_entries = tkm_diskstat_entry_get_all_entries (
      entrypool->input_database, session_hash,
      tkm_settings_get_data_time_source (entrypool->settings), start_timestamp,
      end_timestamp, NULL);

  entrypool->procinfo_entries = tkm_procinfo_entry_get_all_entries (
      entrypool->input_database, session_hash,
      tkm_settings_get_data_time_source (entrypool->settings), start_timestamp,
      end_timestamp, NULL);

  entrypool->ctxinfo_entries = tkm_ctxinfo_entry_get_all_entries (
      entrypool->input_database, session_hash,
      tkm_settings_get_data_time_source (entrypool->settings), start_timestamp,
      end_timestamp, NULL);

  tkm_entrypool_data_unlock (entrypool);

  if (callback != NULL)
    callback (ACTION_STATUS_COMPLETE, event->action);
}

static void
close_database (TkmEntryPool *entrypool)
{
  if (entrypool->input_file != NULL)
    {
      g_free (entrypool->input_file);
      entrypool->input_file = NULL;
    }

  if (entrypool->input_database != NULL)
    {
      sqlite3_close (entrypool->input_database);
      entrypool->input_database = NULL;
    }
}

static void
do_open_database_file (TkmEntryPool *entrypool, TkmEntryPoolEvent *event)
{
  TkmActionStatusCallback callback = tkm_action_get_callback (event->action);
  GList *args = NULL;

  g_assert (entrypool);
  g_assert (event);

  args = tkm_action_get_args (event->action);
  g_assert (args);

  close_database (entrypool);

  entrypool->input_file
      = g_strdup ((const gchar *)(g_list_first (args)->data));
  if (sqlite3_open_v2 (entrypool->input_file, &entrypool->input_database,
                       SQLITE_OPEN_READONLY, NULL))
    {
      g_warning ("Cannot open database at path %s", entrypool->input_file);
      if (callback != NULL)
        callback (ACTION_STATUS_FAILED, event->action);
    }
  else
    {
      if (callback != NULL)
        callback (ACTION_STATUS_COMPLETE, event->action);
    }
}

static void
entrypool_source_destroy_notify (gpointer _entrypool)
{
  TkmEntryPool *entrypool = (TkmEntryPool *)_entrypool;

  g_assert (entrypool);
  g_debug ("EntryPool destroy notification");

  tkm_entrypool_unref (entrypool);
}

static void
entrypool_queue_destroy_notify (gpointer _entrypool)
{
  TKM_UNUSED (_entrypool);
  g_debug ("EntryPool queue destroy notification");
}

TkmEntryPool *
tkm_entrypool_new (GMainContext *context, TkmTaskPool *taskpool,
                   TkmSettings *settings)
{
  TkmEntryPool *entrypool = (TkmEntryPool *)g_source_new (
      &entrypool_source_funcs, sizeof (TkmEntryPool));

  g_assert (entrypool);

  g_ref_count_init (&entrypool->rc);
  g_mutex_init (&entrypool->entries_lock);
  entrypool->callback = entrypool_source_callback;
  entrypool->queue = g_async_queue_new_full (entrypool_queue_destroy_notify);
  entrypool->taskpool = tkm_taskpool_ref (taskpool);
  entrypool->settings = tkm_settings_ref (settings);
  entrypool->context = context;
  entrypool->input_file = NULL;
  entrypool->input_database = NULL;

  entrypool->session_entries = NULL;
  entrypool->procinfo_entries = NULL;
  entrypool->ctxinfo_entries = NULL;
  entrypool->procacct_entries = NULL;
  entrypool->cpustat_entries = NULL;
  entrypool->meminfo_entries = NULL;
  entrypool->procevent_entries = NULL;
  entrypool->pressure_entries = NULL;
  entrypool->buddyinfo_entries = NULL;
  entrypool->wireless_entries = NULL;
  entrypool->diskstat_entries = NULL;

  g_source_set_callback (TKM_EVENT_SOURCE (entrypool), NULL, entrypool,
                         entrypool_source_destroy_notify);
  g_source_attach (TKM_EVENT_SOURCE (entrypool), context);

  return entrypool;
}

TkmEntryPool *
tkm_entrypool_ref (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  g_ref_count_inc (&entrypool->rc);
  return entrypool;
}

void
tkm_entrypool_unref (TkmEntryPool *entrypool)
{
  g_assert (entrypool);

  if (g_ref_count_dec (&entrypool->rc) == TRUE)
    {
      if (entrypool->taskpool != NULL)
        tkm_taskpool_unref (entrypool->taskpool);

      if (entrypool->settings != NULL)
        tkm_settings_unref (entrypool->settings);

      if (entrypool->input_file != NULL)
        g_free (entrypool->input_file);

      main_entries_free (entrypool);

      g_async_queue_unref (entrypool->queue);
      g_source_unref (TKM_EVENT_SOURCE (entrypool));
    }
}

void
tkm_entrypool_push_action (TkmEntryPool *entrypool, TkmAction *action)
{
  TkmEntryPoolEvent *e = NULL;

  g_assert (entrypool);

  e = g_new0 (TkmEntryPoolEvent, 1);

  switch (action->type)
    {
    case ACTION_OPEN_DATABASE_FILE:
      e->type = EPOOL_EVENT_OPEN_DATABASE_FILE;
      break;

    case ACTION_LOAD_SESSIONS:
      e->type = EPOOL_EVENT_LOAD_SESSIONS;
      break;

    case ACTION_LOAD_DATA:
      e->type = EPOOL_EVENT_LOAD_DATA;
      break;

    default:
      break;
    }

  e->action = tkm_action_ref (action);

  post_entrypool_event (entrypool, e);
}

GPtrArray *
tkm_entrypool_get_session_entries (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  return entrypool->session_entries;
}

GPtrArray *
tkm_entrypool_get_procinfo_entries (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  return entrypool->procinfo_entries;
}

GPtrArray *
tkm_entrypool_get_ctxinfo_entries (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  return entrypool->ctxinfo_entries;
}

GPtrArray *
tkm_entrypool_get_procacct_entries (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  return entrypool->procacct_entries;
}

GPtrArray *
tkm_entrypool_get_cpustat_entries (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  return entrypool->cpustat_entries;
}

GPtrArray *
tkm_entrypool_get_meminfo_entries (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  return entrypool->meminfo_entries;
}

GPtrArray *
tkm_entrypool_get_procevent_entries (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  return entrypool->procevent_entries;
}

GPtrArray *
tkm_entrypool_get_pressure_entries (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  return entrypool->pressure_entries;
}

GPtrArray *
tkm_entrypool_get_buddyinfo_entries (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  return entrypool->buddyinfo_entries;
}

GPtrArray *
tkm_entrypool_get_wireless_entries (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  return entrypool->wireless_entries;
}

GPtrArray *
tkm_entrypool_get_diskstat_entries (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  return entrypool->diskstat_entries;
}

void
tkm_entrypool_data_lock (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  g_mutex_lock (&entrypool->entries_lock);
}

gboolean
tkm_entrypool_data_try_lock (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  return g_mutex_trylock (&entrypool->entries_lock);
}

void
tkm_entrypool_data_unlock (TkmEntryPool *entrypool)
{
  g_assert (entrypool);
  g_mutex_unlock (&entrypool->entries_lock);
}
