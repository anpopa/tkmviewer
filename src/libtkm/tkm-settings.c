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
 * \file tkm-settings.c
 */

#include "tkm-settings.h"

TkmSettings *
tkm_settings_new (void)
{
  TkmSettings *settings = g_new0 (TkmSettings, 1);

  g_ref_count_init (&settings->rc);

  return settings;
}

TkmSettings *
tkm_settings_ref (TkmSettings *settings)
{
  g_assert (settings);
  g_ref_count_inc (&settings->rc);
  return settings;
}

void
tkm_settings_unref (TkmSettings *settings)
{
  g_assert (settings);

  if (g_ref_count_dec (&settings->rc) == TRUE)
    {
      g_free (settings);
    }
}

DataTimeSource
tkm_settings_get_data_time_source (TkmSettings *settings)
{
  g_assert (settings);
  return settings->time_source;
}

void
tkm_settings_set_data_time_source (TkmSettings *settings, DataTimeSource ts)
{
  g_assert (settings);
  settings->time_source = ts;
}

DataTimeInterval
tkm_settings_get_data_time_interval (TkmSettings *settings)
{
  g_assert (settings);
  return settings->time_interval;
}

void
tkm_settings_set_data_time_interval (TkmSettings *settings,
                                     DataTimeInterval ti)
{
  g_assert (settings);
  settings->time_interval = ti;
}
