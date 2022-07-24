/* tkmv-dashboard-view.c
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

#include "tkmv-dashboard-view.h"
#include "tkmv-types.h"

#include "libkplot/kplot.h"
#include <math.h>

static void tkmv_dashboard_view_widgets_init (TkmvDashboardView *self);
static void cpu_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                       int width, int height, gpointer data);

struct _TkmvDashboardView
{
  GtkBox parent_instance;

  /* Template widgets */
  GtkDrawingArea *history_events_drawing_area;
  GtkDrawingArea *history_cpu_drawing_area;
  GtkDrawingArea *history_mem_drawing_area;
  GtkDrawingArea *history_psi_drawing_area;
};

G_DEFINE_TYPE (TkmvDashboardView, tkmv_dashboard_view, GTK_TYPE_BOX)

static void
tkmv_dashboard_view_class_init (TkmvDashboardViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (
      widget_class,
      "/ro/fxdata/taskmonitor/viewer/gtk/tkmv-dashboard-view.ui");
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        history_events_drawing_area);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        history_cpu_drawing_area);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        history_mem_drawing_area);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        history_psi_drawing_area);
}

static void
tkmv_dashboard_view_init (TkmvDashboardView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  tkmv_dashboard_view_widgets_init (self);
}

static void
tkmv_dashboard_view_widgets_init (TkmvDashboardView *self)
{
  TKMV_UNUSED (self);
  gtk_drawing_area_set_draw_func (self->history_events_drawing_area,
                                  cpu_history_draw_function, self, NULL);
  gtk_drawing_area_set_draw_func (self->history_cpu_drawing_area,
                                  cpu_history_draw_function, self, NULL);
  gtk_drawing_area_set_draw_func (self->history_mem_drawing_area,
                                  cpu_history_draw_function, self, NULL);
  gtk_drawing_area_set_draw_func (self->history_psi_drawing_area,
                                  cpu_history_draw_function, self, NULL);
}

static void
cpu_history_draw_function (GtkDrawingArea *area, cairo_t *cr, int width,
                           int height, gpointer data)
{
  struct kpair points1[50];
  struct kplotcfg plotcfg;
  struct kdatacfg d1_cfg;
  struct kdata *d1;
  struct kplot *p;
  size_t i;

  TKMV_UNUSED (area);
  TKMV_UNUSED (data);

  for (i = 0; i < 50; i++)
    {
      points1[i].x = i;
      points1[i].y = i;
    }
  d1 = kdata_array_alloc (points1, 50);

  kplotcfg_defaults (&plotcfg);

  plotcfg.grid = GRID_ALL;
  plotcfg.extrema = EXTREMA_YMAX | EXTREMA_YMIN;
  plotcfg.extrema_ymin = 0;
  plotcfg.extrema_ymax = 100;

  p = kplot_alloc (&plotcfg);

  kdatacfg_defaults (&d1_cfg);
  d1_cfg.line.sz = 1.0;
  d1_cfg.line.clr.type = KPLOTCTYPE_RGBA;
  d1_cfg.line.clr.rgba[2] = 1.0;
  d1_cfg.line.clr.rgba[3] = 1.0;

  kplot_attach_data (p, d1, KPLOT_LINES, &d1_cfg);
  kdata_destroy (d1);

  kplot_draw (p, width, height, cr);

  kplot_free (p);
}
