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
 * \file tkm-action.c
 */

#include "tkm-action.h"

TkmAction *
tkm_action_new (ActionType type, GList *args, TkmActionStatusCallback callback,
                gpointer user_data)
{
  TkmAction *action = g_new0 (TkmAction, 1);

  action->type = type;
  action->args = args;
  action->callback = callback;
  action->user_data = user_data;

  g_ref_count_init (&action->rc);

  return action;
}

TkmAction *
tkm_action_ref (TkmAction *action)
{
  g_assert (action);
  g_ref_count_inc (&action->rc);
  return action;
}

static void
args_free (gpointer data)
{
  g_free (data);
}

void
tkm_action_unref (TkmAction *action)
{
  g_assert (action);

  if (g_ref_count_dec (&action->rc) == TRUE)
    {
      if (action->args)
        g_list_free_full (action->args, args_free);

      g_free (action);
    }
}

ActionType
tkm_action_get_type (TkmAction *action)
{
  g_assert (action);
  return action->type;
}

GList *
tkm_action_get_args (TkmAction *action)
{
  g_assert (action);
  return action->args;
}

TkmActionStatusCallback
tkm_action_get_callback (TkmAction *action)
{
  g_assert (action);
  return (TkmActionStatusCallback)action->callback;
}

gpointer
tkm_action_get_user_data (TkmAction *action)
{
  g_assert (action);
  return action->user_data;
}
