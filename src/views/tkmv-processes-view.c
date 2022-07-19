/* tkmv-processes-view.c
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

#include "tkmv-processes-view.h"

struct _TkmvProcessesView
{
  GtkBox  parent_instance;

  /* Template widgets */
};

G_DEFINE_TYPE (TkmvProcessesView, tkmv_processes_view, GTK_TYPE_BOX)

static void
tkmv_processes_view_class_init (TkmvProcessesViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/ro/fxdata/taskmonitor/viewer/gtk/tkmv-processes-view.ui");
}

static void
tkmv_processes_view_init (TkmvProcessesView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
