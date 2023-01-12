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
 * \file tkm-settings.h
 */

#pragma once

#include <gio/gio.h>
#include <glib.h>

#include "tkm-types.h"

G_BEGIN_DECLS

typedef struct _TkmSettings {
  DataTimeSource time_source;
  DataTimeInterval time_interval;

  grefcount rc;
} TkmSettings;

TkmSettings *tkm_settings_new (void);
TkmSettings *tkm_settings_ref (TkmSettings *settings);
void tkm_settings_unref (TkmSettings *settings);

DataTimeSource tkm_settings_get_data_time_source (TkmSettings *settings);
void tkm_settings_set_data_time_source (TkmSettings *settings,
                                        DataTimeSource ts);
DataTimeInterval tkm_settings_get_data_time_interval (TkmSettings *settings);
void tkm_settings_set_data_time_interval (TkmSettings *settings,
                                          DataTimeInterval ti);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TkmSettings, tkm_settings_unref);

G_END_DECLS
