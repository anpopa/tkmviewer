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
 * \file tkm-taskpool.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>

G_BEGIN_DECLS

typedef struct _TkmTaskPool {
  GThreadPool *pool;
  guint max_threads;
  gpointer context;
  grefcount rc;
} TkmTaskPool;

TkmTaskPool *tkm_taskpool_new (guint max_threads, gpointer context);
TkmTaskPool *tkm_taskpool_ref (TkmTaskPool *p);
void tkm_taskpool_unref (TkmTaskPool *p);
gboolean tkm_taskpool_push (TkmTaskPool *p, gpointer task);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmTaskPool, tkm_taskpool_unref);

G_END_DECLS
