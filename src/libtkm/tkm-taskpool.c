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
 * \file tkm-taskpool.c
 */

#include "tkm-taskpool.h"
#include "tkm-context.h"
#include "tkm-task.h"

static void
thread_worker (gpointer task, gpointer context)
{
  TkmTask *t = TKM_TASK (task);
  gboolean run_status = TRUE;

  g_assert (task);
  g_assert (context);

  if (t->exec_cb != NULL)
    {
      TkmTaskExecCallback callback = (TkmTaskExecCallback)t->exec_cb;
      run_status = callback (t, context);
    }

  if (t->status_cb != NULL)
    {
      TkmTaskStatusCallback callback = (TkmTaskStatusCallback)t->status_cb;
      callback (
          ((run_status == TRUE) ? TASK_STATUS_COMPLETE : TASK_STATUS_FAILED),
          t);
    }

  g_mutex_lock (&t->mutex);
  t->complete = TRUE;
  g_cond_signal (&t->cond);
  g_mutex_unlock (&t->mutex);
}

TkmTaskPool *
tkm_taskpool_new (guint max_threads, gpointer context)
{
  TkmTaskPool *p = g_new0 (TkmTaskPool, 1);

  p->max_threads = max_threads;
  p->context = tkm_context_ref (context);
  p->pool = g_thread_pool_new (thread_worker, p->context, p->max_threads, TRUE,
                               NULL);
  g_ref_count_init (&p->rc);

  g_assert (p->pool);

  return p;
}

TkmTaskPool *
tkm_taskpool_ref (TkmTaskPool *p)
{
  g_assert (p);
  g_ref_count_inc (&p->rc);
  return p;
}

void
tkm_taskpool_unref (TkmTaskPool *p)
{
  g_assert (p);

  if (g_ref_count_dec (&p->rc) == TRUE)
    {
      if (p->context != NULL)
        tkm_context_unref (p->context);

      if (p->pool)
        g_thread_pool_free (p->pool, TRUE, TRUE);

      g_free (p);
    }
}

gboolean
tkm_taskpool_push (TkmTaskPool *p, gpointer task)
{
  return g_thread_pool_push (p->pool, task, NULL);
}
