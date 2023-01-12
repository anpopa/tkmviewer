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
 * \file tkm-task.h
 */

#pragma once

#include "tkm-taskpool.h"
#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>

G_BEGIN_DECLS

typedef enum _TaskStatusType {
  TASK_STATUS_PROGRESS,
  TASK_STATUS_FAILED,
  TASK_STATUS_COMPLETE,
} TaskStatusType;

typedef struct _TkmTask {
  gboolean run_status;
  GMutex mutex;
  GCond cond;
  gboolean complete;
  gpointer status_cb;
  gpointer exec_cb;
} TkmTask;

typedef void (*TkmTaskStatusCallback) (TaskStatusType status_type,
                                       TkmTask *task);
typedef gboolean (*TkmTaskExecCallback) (TkmTask *task, gpointer context);

void tkm_task_init (TkmTask *task, gpointer status_cb, gpointer exec_cb);
gboolean tkm_task_run (TkmTask *task, TkmTaskPool *pool);
gboolean tkm_task_run_wait (TkmTask *task, TkmTaskPool *pool);
void tkm_task_wait (TkmTask *task);
gboolean tkm_task_run_status (TkmTask *task);

#define TKM_TASK(x) (TkmTask *)(x)
