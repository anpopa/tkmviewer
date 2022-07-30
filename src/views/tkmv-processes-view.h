/* tkmv-processes-view.h
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
#include <tkm-context.h>

G_BEGIN_DECLS

#define TKMV_TYPE_PROCESSES_VIEW (tkmv_processes_view_get_type ())

G_DECLARE_FINAL_TYPE (TkmvProcessesView, tkmv_processes_view, TKMV,
                      PROCESSES_VIEW, GtkBox)

void tkmv_processes_reload_entries (TkmvProcessesView *view,
                                    TkmContext *context);

G_END_DECLS
