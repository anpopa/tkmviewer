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
 * \file tkm-entrypool.h
 */

#pragma once

#include "tkm-action.h"
#include "tkm-settings.h"
#include "tkm-taskpool.h"
#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

typedef enum _EntryPoolEventType
{
  EPOOL_EVENT_OPEN_DATABASE_FILE,
  EPOOL_EVENT_LOAD_SESSIONS,
  EPOOL_EVENT_LOAD_DATA
} EntryPoolEventType;

typedef gboolean (*TkmEntryPoolCallback) (gpointer _entrypool,
                                          gpointer _event);

typedef struct _TkmEntryPoolEvent
{
  EntryPoolEventType type;
  TkmAction *action;
} TkmEntryPoolEvent;

typedef struct _TkmEntryPool
{
  GSource source;
  GAsyncQueue *queue;

  /* referenced modules */
  GMainContext *context;
  TkmSettings *settings;
  TkmTaskPool *taskpool;
  TkmEntryPoolCallback callback;

  /* entry data pools */
  GMutex entries_lock;
  gchar *input_file;
  sqlite3 *input_database;

  GPtrArray *session_entries;
  GPtrArray *procinfo_entries;
  GPtrArray *procacct_entries;
  GPtrArray *cpustat_entries;
  GPtrArray *meminfo_entries;
  GPtrArray *procevent_entries;
  GPtrArray *pressure_entries;
  GPtrArray *buddyinfo_entries;
  GPtrArray *wireless_entries;
  GPtrArray *diskstat_entries;

  grefcount rc;
} TkmEntryPool;

TkmEntryPool *tkm_entrypool_new (GMainContext *context, TkmTaskPool *taskpool,
                                 TkmSettings *settings);
TkmEntryPool *tkm_entrypool_ref (TkmEntryPool *entrypool);

GPtrArray *tkm_entrypool_get_session_entries (TkmEntryPool *entrypool);
GPtrArray *tkm_entrypool_get_procinfo_entries (TkmEntryPool *entrypool);
GPtrArray *tkm_entrypool_get_procacct_entries (TkmEntryPool *entrypool);
GPtrArray *tkm_entrypool_get_cpustat_entries (TkmEntryPool *entrypool);
GPtrArray *tkm_entrypool_get_meminfo_entries (TkmEntryPool *entrypool);
GPtrArray *tkm_entrypool_get_procevent_entries (TkmEntryPool *entrypool);
GPtrArray *tkm_entrypool_get_pressure_entries (TkmEntryPool *entrypool);
GPtrArray *tkm_entrypool_get_buddyinfo_entries (TkmEntryPool *entrypool);
GPtrArray *tkm_entrypool_get_wireless_entries (TkmEntryPool *entrypool);
GPtrArray *tkm_entrypool_get_diskstat_entries (TkmEntryPool *entrypool);

void tkm_entrypool_unref (TkmEntryPool *entrypool);
void tkm_entrypool_push_action (TkmEntryPool *entrypool, TkmAction *action);

void tkm_entrypool_data_lock (TkmEntryPool *entrypool);
gboolean tkm_entrypool_data_try_lock (TkmEntryPool *entrypool);
void tkm_entrypool_data_unlock (TkmEntryPool *entrypool);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmEntryPool, tkm_entrypool_unref);

G_END_DECLS
