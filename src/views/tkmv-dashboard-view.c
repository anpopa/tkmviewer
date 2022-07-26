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
#include "tkm-cpustat-entry.h"
#include "tkm-settings.h"
#include "tkmv-application.h"
#include "tkmv-types.h"

#include "libkplot/kplot.h"
#include <math.h>

static void tkmv_dashboard_view_widgets_init (TkmvDashboardView *self);
static void update_instant_cpu_frame (TkmvDashboardView *view);
static void events_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                          int width, int height,
                                          gpointer data);
static void cpu_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                       int width, int height, gpointer data);
static void mem_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                       int width, int height, gpointer data);
static void psi_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                       int width, int height, gpointer data);

struct _TkmvDashboardView
{
  GtkBox parent_instance;

  /* Template widgets */
  GtkDrawingArea *history_events_drawing_area;
  GtkDrawingArea *history_cpu_drawing_area;
  GtkDrawingArea *history_mem_drawing_area;
  GtkDrawingArea *history_psi_drawing_area;

  /* Instant CPU */
  GtkLevelBar *cpu_all_level_bar;
  GtkLabel *cpu_all_level_label;
  GtkLevelBar *cpu_usr_level_bar;
  GtkLabel *cpu_usr_level_label;
  GtkLevelBar *cpu_sys_level_bar;
  GtkLabel *cpu_sys_level_label;
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
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        cpu_all_level_bar);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        cpu_all_level_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        cpu_usr_level_bar);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        cpu_usr_level_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        cpu_sys_level_bar);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        cpu_sys_level_label);
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
                                  events_history_draw_function, self, NULL);
  gtk_drawing_area_set_draw_func (self->history_cpu_drawing_area,
                                  cpu_history_draw_function, self, NULL);
  gtk_drawing_area_set_draw_func (self->history_mem_drawing_area,
                                  mem_history_draw_function, self, NULL);
  gtk_drawing_area_set_draw_func (self->history_psi_drawing_area,
                                  psi_history_draw_function, self, NULL);
}

static void
timestamp_format (double val, char *buf, size_t sz)
{
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());

  g_assert (buf);

  switch (tkmv_settings_get_time_source (settings))
    {
    case DATA_TIME_SOURCE_SYSTEM:
    case DATA_TIME_SOURCE_RECEIVE:
      {
        g_autoptr (GDateTime) dtime
            = g_date_time_new_from_unix_utc ((guint)val);
        g_autofree gchar *text = g_date_time_format (dtime, "%H:%M:%S");
        snprintf (buf, sz, "%s", text);
        break;
      }

    case DATA_TIME_SOURCE_MONOTONIC:
      {
        g_autofree gchar *text = g_strdup_printf ("%u", (guint)val);
        snprintf (buf, sz, "%s", text);
        break;
      }
    }
}

static void
events_history_draw_function (GtkDrawingArea *area, cairo_t *cr, int width,
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
  plotcfg.xticlabelfmt = timestamp_format;

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

static void
cpu_history_draw_function (GtkDrawingArea *area, cairo_t *cr, int width,
                           int height, gpointer data)
{
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  GPtrArray *cpu_data = tkm_context_get_cpustat_entries (context);
  TkmSessionEntry *active_session = NULL;

  struct kpair *points1 = NULL;
  struct kpair *points2 = NULL;
  struct kpair *points3 = NULL;

  struct kplotcfg plotcfg;
  struct kdatacfg d1_cfg;
  struct kdatacfg d2_cfg;
  struct kdatacfg d3_cfg;
  struct kdata *d1;
  struct kdata *d2;
  struct kdata *d3;
  struct kplot *p;
  guint cnt = 0;

  TKMV_UNUSED (area);
  TKMV_UNUSED (data);

  if (sessions != NULL)
    {

      for (guint i = 0; i < sessions->len; i++)
        {
          if (tkm_session_entry_get_active (g_ptr_array_index (sessions, i)))
            {
              active_session = g_ptr_array_index (sessions, i);
            }
        }

      g_assert (active_session);
      g_assert (tkm_session_entry_get_device_cpus (active_session) > 0);
    }

  if (cpu_data != NULL)
    {
      if (cpu_data->len > 0)
        {
          points1 = calloc (cpu_data->len, sizeof (struct kpair));
          points2 = calloc (cpu_data->len, sizeof (struct kpair));
          points3 = calloc (cpu_data->len, sizeof (struct kpair));
        }

      for (guint i = 0; i < cpu_data->len; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, i);
          if (g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu") == 0)
            {
              points1[cnt].x = tkm_cpustat_entry_get_timestamp (
                  entry, tkmv_settings_get_time_source (settings));
              points1[cnt].y = tkm_cpustat_entry_get_all (entry);
              points2[cnt].x = points1[cnt].x;
              points2[cnt].y = tkm_cpustat_entry_get_usr (entry);
              points3[cnt].x = points1[cnt].x;
              points3[cnt].y = tkm_cpustat_entry_get_sys (entry);
              cnt += 1;
            }
        }
    }

  if (points1 == NULL)
    {
      points1 = calloc (1, sizeof (struct kpair));
    }

  if (points2 == NULL)
    {
      points2 = calloc (1, sizeof (struct kpair));
    }

  if (points3 == NULL)
    {
      points3 = calloc (1, sizeof (struct kpair));
    }

  if (cnt == 0)
    {
      points1[cnt].x = 1;
      points1[cnt].y = 0;
      points2[cnt].x = points1[cnt].x;
      points2[cnt].y = 0;
      points3[cnt].x = points1[cnt].x;
      points3[cnt].y = 0;
      cnt += 1;
    }

  d1 = kdata_array_alloc (points1, cnt);
  d2 = kdata_array_alloc (points2, cnt);
  d3 = kdata_array_alloc (points3, cnt);

  kplotcfg_defaults (&plotcfg);
  plotcfg.grid = GRID_ALL;
  plotcfg.extrema = EXTREMA_YMAX | EXTREMA_YMIN;
  plotcfg.extrema_ymin = 0;
  if (active_session != NULL)
    {
      plotcfg.extrema_ymax
          = (100 * tkm_session_entry_get_device_cpus (active_session));
    }
  else
    {
      plotcfg.extrema_ymax = 100;
    }
  plotcfg.xticlabelfmt = timestamp_format;

  p = kplot_alloc (&plotcfg);

  kdatacfg_defaults (&d1_cfg);
  d1_cfg.line.sz = 1.0;
  d1_cfg.line.clr.type = KPLOTCTYPE_RGBA;
  d1_cfg.line.clr.rgba[0] = 1.0;
  d1_cfg.line.clr.rgba[3] = 1.0;

  kdatacfg_defaults (&d2_cfg);
  d2_cfg.line.sz = 1.0;
  d2_cfg.line.clr.type = KPLOTCTYPE_RGBA;
  d2_cfg.line.clr.rgba[2] = 1.0;
  d2_cfg.line.clr.rgba[3] = 1.0;

  kdatacfg_defaults (&d3_cfg);
  d3_cfg.line.sz = 1.0;
  d3_cfg.line.clr.type = KPLOTCTYPE_RGBA;
  d3_cfg.line.clr.rgba[1] = 1.0;
  d3_cfg.line.clr.rgba[3] = 1.0;

  kplot_attach_data (p, d1, KPLOT_LINES, &d1_cfg);
  kplot_attach_data (p, d2, KPLOT_LINES, &d2_cfg);
  kplot_attach_data (p, d3, KPLOT_LINES, &d3_cfg);

  kdata_destroy (d1);
  kdata_destroy (d2);
  kdata_destroy (d3);

  kplot_draw (p, width, height, cr);

  free (points1);
  free (points2);
  free (points3);

  kplot_free (p);
}

static void
mem_history_draw_function (GtkDrawingArea *area, cairo_t *cr, int width,
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

static void
psi_history_draw_function (GtkDrawingArea *area, cairo_t *cr, int width,
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

static void
update_instant_cpu_frame (TkmvDashboardView *view)
{
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  TkmSessionEntry *active_session = NULL;
  GPtrArray *cpu_data = tkm_context_get_cpustat_entries (context);
  TkmCpuStatEntry *entry = NULL;
  gchar buf[64];

  if (cpu_data == NULL)
    return;

  if (cpu_data->len == 0)
    return;

  if (sessions == NULL)
    return;

  for (guint i = 0; i < sessions->len; i++)
    {
      if (tkm_session_entry_get_active (g_ptr_array_index (sessions, i)))
        {
          active_session = g_ptr_array_index (sessions, i);
        }
    }

  g_assert (active_session);
  g_assert (tkm_session_entry_get_device_cpus (active_session) > 0);

  entry = g_ptr_array_index (cpu_data, 0);

  gtk_level_bar_set_value (
      view->cpu_all_level_bar,
      (double)tkm_cpustat_entry_get_all (entry)
          / tkm_session_entry_get_device_cpus (active_session));
  gtk_level_bar_set_value (
      view->cpu_usr_level_bar,
      (double)tkm_cpustat_entry_get_usr (entry)
          / tkm_session_entry_get_device_cpus (active_session));
  gtk_level_bar_set_value (
      view->cpu_sys_level_bar,
      (double)tkm_cpustat_entry_get_sys (entry)
          / tkm_session_entry_get_device_cpus (active_session));

  snprintf (buf, sizeof (buf), "All - %u %%",
            tkm_cpustat_entry_get_all (entry));
  gtk_label_set_text (view->cpu_all_level_label, buf);

  snprintf (buf, sizeof (buf), "Usr - %3u %%",
            tkm_cpustat_entry_get_usr (entry));
  gtk_label_set_text (view->cpu_usr_level_label, buf);

  snprintf (buf, sizeof (buf), "Sys - %3u %%",
            tkm_cpustat_entry_get_sys (entry));
  gtk_label_set_text (view->cpu_sys_level_label, buf);

  g_assert (view);
}

void
tkmv_dashboard_view_update_content (TkmvDashboardView *view)
{
  g_assert (view);

  update_instant_cpu_frame (view);
  gtk_widget_queue_draw (GTK_WIDGET (view->history_cpu_drawing_area));
}
