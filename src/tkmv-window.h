/* tkmv-window.h
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
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TKMV_TYPE_WINDOW (tkmv_window_get_type ())

G_DECLARE_FINAL_TYPE (TkmvWindow, tkmv_window, TKMV, WINDOW,
                      AdwApplicationWindow)

void tkmv_window_update_toolbar (TkmvWindow *window);
void tkmv_window_progress_spinner_start (TkmvWindow *window);
void tkmv_window_progress_spinner_stop (TkmvWindow *window);

G_END_DECLS
