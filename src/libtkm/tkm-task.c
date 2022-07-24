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
 * \file tkm-task.c
 */

#include "tkm-task.h"

void
tkm_task_init (TkmTask *task, gpointer status_cb, gpointer exec_cb)
{
  g_assert (task);
  g_mutex_init (&task->mutex);
  g_cond_init (&task->cond);
  task->status_cb = status_cb;
  task->exec_cb = exec_cb;
  task->run_status = TRUE;
}

gboolean
tkm_task_run (TkmTask *task, TkmTaskPool *pool)
{
  g_assert (task);
  g_assert (pool);

  return tkm_taskpool_push (pool, task);
}

gboolean
tkm_task_run_wait (TkmTask *task, TkmTaskPool *pool)
{
  g_assert (task);
  g_assert (pool);

  if (tkm_taskpool_push (pool, task))
    {
      tkm_task_wait (task);
      return TRUE;
    }

  return FALSE;
}

void
tkm_task_wait (TkmTask *task)
{
  g_assert (task);

  g_mutex_lock (&task->mutex);
  while (!task->complete)
    g_cond_wait (&task->cond, &task->mutex);
  g_mutex_unlock (&task->mutex);
}

gboolean
tkm_task_run_status (TkmTask *task)
{
  g_assert (task);
  return task->run_status;
}
