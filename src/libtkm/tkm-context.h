/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2022 Alin Popa
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
 * \file tkm-context.h
 */

#pragma once

#include "tkm-entrypool.h"
#include "tkm-session-entry.h"
#include "tkm-settings.h"
#include "tkm-types.h"

#include <glib.h>

G_BEGIN_DECLS

typedef struct _TkmContext
{
  TkmEntryPool *entrypool;
  TkmTaskPool *taskpool;
  TkmSettings *settings;

  GThread *mainthread;
  GMainContext *maincontext;
  GMainLoop *mainloop;
  grefcount rc;
} TkmContext;

TkmContext *tkm_context_new (TkmSettings *settings);
TkmContext *tkm_context_ref (TkmContext *ctx);
void tkm_context_unref (TkmContext *ctx);

GPtrArray *tkm_context_get_session_entries (TkmContext *ctx);
GPtrArray *tkm_context_get_procinfo_entries (TkmContext *ctx);
GPtrArray *tkm_context_get_ctxinfo_entries (TkmContext *ctx);
GPtrArray *tkm_context_get_procacct_entries (TkmContext *ctx);
GPtrArray *tkm_context_get_cpustat_entries (TkmContext *ctx);
GPtrArray *tkm_context_get_meminfo_entries (TkmContext *ctx);
GPtrArray *tkm_context_get_procevent_entries (TkmContext *ctx);
GPtrArray *tkm_context_get_pressure_entries (TkmContext *ctx);
GPtrArray *tkm_context_get_buddyinfo_entries (TkmContext *ctx);
GPtrArray *tkm_context_get_wireless_entries (TkmContext *ctx);
GPtrArray *tkm_context_get_diskstat_entries (TkmContext *ctx);

void tkm_context_execute_action (TkmContext *ctx, TkmAction *action);

void tkm_context_data_lock (TkmContext *ctx);
gboolean tkm_context_data_try_lock (TkmContext *ctx);
void tkm_context_data_unlock (TkmContext *ctx);

G_END_DECLS
