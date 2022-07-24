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
 * \file tkm-context.c
 */

#include "tkm-context.h"

#include <unistd.h>

gpointer
context_event_loop_thread (gpointer arg)
{
  TkmContext *ctx = (TkmContext *)arg;

  g_debug ("Tkm main context start");
  g_main_loop_run (ctx->mainloop);
  g_debug ("Tkm main context end");

  return NULL;
}

TkmContext *
tkm_context_new (TkmSettings *settings)
{
  TkmContext *ctx = g_new0 (TkmContext, 1);

  g_ref_count_init (&ctx->rc);

  ctx->maincontext = g_main_context_new ();
  ctx->mainloop = g_main_loop_new (ctx->maincontext, FALSE);
  ctx->taskpool = tkm_taskpool_new (sysconf (_SC_NPROCESSORS_ONLN), ctx);
  ctx->entrypool
      = tkm_entrypool_new (ctx->maincontext, ctx->taskpool, settings);
  ctx->settings = tkm_settings_ref (settings);

  ctx->mainthread
      = g_thread_new ("TkmContextThread", context_event_loop_thread, ctx);

  return ctx;
}

TkmContext *
tkm_context_ref (TkmContext *ctx)
{
  g_assert (ctx);
  g_ref_count_inc (&ctx->rc);
  return ctx;
}

void
tkm_context_unref (TkmContext *ctx)
{
  g_assert (ctx);

  if (g_ref_count_dec (&ctx->rc) == TRUE)
    {
      if (ctx->maincontext != NULL)
        g_main_context_unref (ctx->maincontext);

      if (ctx->mainloop != NULL)
        {
          g_main_loop_quit (ctx->mainloop);
          g_main_loop_unref (ctx->mainloop);
        }

      if (ctx->mainthread != NULL)
        {
          g_thread_join (ctx->mainthread);
          g_thread_unref (ctx->mainthread);
        }

      if (ctx->taskpool != NULL)
        tkm_taskpool_unref (ctx->taskpool);

      if (ctx->settings != NULL)
        tkm_settings_unref (ctx->settings);

      g_free (ctx);
    }
}

void
tkm_context_execute_action (TkmContext *ctx, TkmAction *action)
{
  g_assert (ctx);
  g_assert (action);

  switch (action->type)
    {
    case ACTION_OPEN_DATABASE_FILE:
    case ACTION_LOAD_SESSIONS:
    case ACTION_LOAD_DATA:
      tkm_entrypool_push_action (ctx->entrypool, action);
      break;

    default:
      g_error ("Invalid action type");
      break;
    }
}

GPtrArray *
tkm_context_get_session_entries (TkmContext *ctx)
{
  g_assert (ctx);
  return tkm_entrypool_get_session_entries (ctx->entrypool);
}

GPtrArray *
tkm_context_get_procinfo_entries (TkmContext *ctx)
{
  g_assert (ctx);
  return tkm_entrypool_get_procinfo_entries (ctx->entrypool);
}

GPtrArray *
tkm_context_get_procacct_entries (TkmContext *ctx)
{
  g_assert (ctx);
  return tkm_entrypool_get_procacct_entries (ctx->entrypool);
}

GPtrArray *
tkm_context_get_cpustat_entries (TkmContext *ctx)
{
  g_assert (ctx);
  return tkm_entrypool_get_cpustat_entries (ctx->entrypool);
}

GPtrArray *
tkm_context_get_meminfo_entries (TkmContext *ctx)
{
  g_assert (ctx);
  return tkm_entrypool_get_meminfo_entries (ctx->entrypool);
}

GPtrArray *
tkm_context_get_procevent_entries (TkmContext *ctx)
{
  g_assert (ctx);
  return tkm_entrypool_get_procevent_entries (ctx->entrypool);
}

GPtrArray *
tkm_context_get_buddyinfo_entries (TkmContext *ctx)
{
  g_assert (ctx);
  return tkm_entrypool_get_buddyinfo_entries (ctx->entrypool);
}

GPtrArray *
tkm_context_get_wireless_entries (TkmContext *ctx)
{
  g_assert (ctx);
  return tkm_entrypool_get_wireless_entries (ctx->entrypool);
}

void
tkm_context_data_lock (TkmContext *ctx)
{
  g_assert (ctx);
  tkm_entrypool_data_lock (ctx->entrypool);
}

gboolean
tkm_context_data_try_lock (TkmContext *ctx)
{
  g_assert (ctx);
  return tkm_entrypool_data_try_lock (ctx->entrypool);
}

void
tkm_context_data_unlock (TkmContext *ctx)
{
  g_assert (ctx);
  tkm_entrypool_data_unlock (ctx->entrypool);
}
