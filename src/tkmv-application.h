/* tkmv-application.h
 *
 * Copyright 2022 Alin Popa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <adwaita.h>

#include "tkm-context.h"
#include "tkmv-settings.h"
#include "tkmv-window.h"

G_BEGIN_DECLS

#define TKMV_TYPE_APPLICATION (tkmv_application_get_type ())

G_DECLARE_FINAL_TYPE (TkmvApplication, tkmv_application, TKMV, APPLICATION,
                      AdwApplication)

TkmvApplication *tkmv_application_new (gchar * application_id,
                                       GApplicationFlags flags);
TkmvApplication *tkmv_application_instance (void);
TkmvWindow *tkmv_application_get_main_window (TkmvApplication *app);
TkmContext *tkmv_application_get_context (TkmvApplication *app);
TkmvSettings *tkmv_application_get_settings (TkmvApplication *app);

void tkmv_application_open_file (TkmvApplication *app, const gchar *path);
void tkmv_application_load_sessions (TkmvApplication *app);
void tkmv_application_load_data (TkmvApplication *app,
                                 const gchar *session_hash, guint start_time);

G_END_DECLS
