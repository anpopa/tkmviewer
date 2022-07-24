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
 * \file tkm-action.h
 */

#pragma once

#include "tkm-types.h"

#include <gio/gio.h>
#include <glib.h>

G_BEGIN_DECLS

typedef enum _ActionType
{
  ACTION_OPEN_DATABASE_FILE,
  ACTION_LOAD_SESSIONS,
  ACTION_LOAD_DATA,
  ACTION_TERMINATE
} ActionType;

typedef enum _ActionStatusType
{
  ACTION_STATUS_PROGRESS,
  ACTION_STATUS_FAILED,
  ACTION_STATUS_COMPLETE,
} ActionStatusType;

typedef struct _TkmAction
{
  ActionType type;
  GList *args;
  gpointer user_data;
  gpointer callback;
  grefcount rc;
} TkmAction;

typedef void (*TkmActionStatusCallback) (ActionStatusType status_type,
                                         TkmAction *action);

TkmAction *tkm_action_new (ActionType type, GList *args,
                           TkmActionStatusCallback callback,
                           gpointer user_data);
TkmAction *tkm_action_ref (TkmAction *action);
void tkm_action_unref (TkmAction *action);
ActionType tkm_action_get_type (TkmAction *action);
GList *tkm_action_get_args (TkmAction *action);
TkmActionStatusCallback tkm_action_get_callback (TkmAction *action);
gpointer tkm_action_get_user_data (TkmAction *action);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmAction, tkm_action_unref);

G_END_DECLS
