/* tkmv-window.c
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

#include "tkmv-types.h"

#include "tkmv-config.h"
#include "tkmv-window.h"

#include "views/tkmv-dashboard-view.h"
#include "views/tkmv-processes-view.h"
#include "views/tkmv-systeminfo-view.h"

static void tkmv_window_views_init (TkmvWindow *self);
static void tkmv_tools_visible_child_changed (GObject *stack, GParamSpec *pspec, TkmvWindow *self);

struct _TkmvWindow
{
  AdwApplicationWindow  parent_instance;

  /* Main views */
  TkmvDashboardView  *dashboard_view;
  TkmvProcessesView  *processes_view;
  TkmvSysteminfoView *systeminfo_view;

  /* Template widgets */
  GtkViewport *dashboard_viewport;
  GtkViewport *processes_viewport;
  GtkViewport *systeminfo_viewport;

  GtkToggleButton *tools_button;
  GtkSearchBar    *tools_bar;
};

G_DEFINE_TYPE (TkmvWindow, tkmv_window, ADW_TYPE_APPLICATION_WINDOW)

static void
tkmv_window_class_init (TkmvWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/ro/fxdata/taskmonitor/viewer/gtk/tkmv-window.ui");
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow, dashboard_viewport);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow, processes_viewport);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow, systeminfo_viewport);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow, tools_button);
  gtk_widget_class_bind_template_child (widget_class, TkmvWindow, tools_bar);

  /* Bind callbacks */
  gtk_widget_class_bind_template_callback (widget_class, tkmv_tools_visible_child_changed);
}

static void
tkmv_window_init (TkmvWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  tkmv_window_views_init(self);

  g_object_bind_property (self->tools_button, "active",
                          self->tools_bar, "search-mode-enabled",
                          G_BINDING_BIDIRECTIONAL);
}

static void
tkmv_window_views_init (TkmvWindow *self)
{
  self->dashboard_view = g_object_new (TKMV_TYPE_DASHBOARD_VIEW, NULL);
  gtk_viewport_set_child (self->dashboard_viewport, GTK_WIDGET (self->dashboard_view));

  self->processes_view = g_object_new (TKMV_TYPE_PROCESSES_VIEW, NULL);
  gtk_viewport_set_child (self->processes_viewport, GTK_WIDGET (self->processes_view));

  self->systeminfo_view = g_object_new (TKMV_TYPE_SYSTEMINFO_VIEW, NULL);
  gtk_viewport_set_child (self->systeminfo_viewport, GTK_WIDGET (self->systeminfo_view));
}

static void
tkmv_tools_visible_child_changed (GObject     *stack,
                                  GParamSpec  *pspec,
                                  TkmvWindow  *self)
{
  TKM_UNUSED (stack);
  TKM_UNUSED (pspec);
  TKM_UNUSED (self);
}
