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
#include "tkm-meminfo-entry.h"
#include "tkm-pressure-entry.h"
#include "tkm-procevent-entry.h"
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
static void events_history_draw_function_safe (GtkDrawingArea *area,
                                               cairo_t *cr, int width,
                                               int height, gpointer data);
static void cpu_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr,
                                            int width, int height,
                                            gpointer data);
static void mem_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr,
                                            int width, int height,
                                            gpointer data);
static void psi_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr,
                                            int width, int height,
                                            gpointer data);

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

  /* Instant MEM */
  GtkLevelBar *mem_level_bar;
  GtkLabel *mem_level_label;
  GtkLevelBar *swap_level_bar;
  GtkLabel *swap_level_label;
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

  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        mem_level_bar);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        mem_level_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        swap_level_bar);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        swap_level_label);
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
                                  events_history_draw_function_safe, self,
                                  NULL);
  gtk_drawing_area_set_draw_func (self->history_cpu_drawing_area,
                                  cpu_history_draw_function_safe, self, NULL);
  gtk_drawing_area_set_draw_func (self->history_mem_drawing_area,
                                  mem_history_draw_function_safe, self, NULL);
  gtk_drawing_area_set_draw_func (self->history_psi_drawing_area,
                                  psi_history_draw_function_safe, self, NULL);
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
memory_format (double val, char *buf, size_t sz)
{
  g_assert (buf);
  snprintf (buf, sz, "%u MB", (guint)val / (1024));
}

static void
pressure_format (double val, char *buf, size_t sz)
{
  g_assert (buf);
  snprintf (buf, sz, "%.3f %%", val);
}

static void
percent_format (double val, char *buf, size_t sz)
{
  g_assert (buf);
  snprintf (buf, sz, "%u %%", (guint)val);
}

static void
events_history_draw_function (GtkDrawingArea *area, cairo_t *cr, int width,
                              int height, gpointer data)
{
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  GPtrArray *events_data = tkm_context_get_procevent_entries (context);
  TkmSessionEntry *active_session = NULL;

  struct kpair *points1 = NULL; /* Forks */
  struct kpair *points2 = NULL; /* Execs */
  struct kpair *points3 = NULL; /* Exits */
  struct kpair *points4 = NULL; /* UIDs */
  struct kpair *points5 = NULL; /* GIDs */

  struct kplotcfg plotcfg;
  struct kdatacfg d1_cfg;
  struct kdatacfg d2_cfg;
  struct kdatacfg d3_cfg;
  struct kdatacfg d4_cfg;
  struct kdatacfg d5_cfg;
  struct kdata *d1;
  struct kdata *d2;
  struct kdata *d3;
  struct kdata *d4;
  struct kdata *d5;
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

  if (events_data != NULL)
    {
      if (events_data->len > 0)
        {
          points1 = calloc (events_data->len, sizeof (struct kpair));
          points2 = calloc (events_data->len, sizeof (struct kpair));
          points3 = calloc (events_data->len, sizeof (struct kpair));
          points4 = calloc (events_data->len, sizeof (struct kpair));
          points5 = calloc (events_data->len, sizeof (struct kpair));
        }

      for (guint i = 0; i < events_data->len; i++)
        {
          TkmProcEventEntry *entry = g_ptr_array_index (events_data, i);

          points1[cnt].x = tkm_procevent_entry_get_timestamp (
              entry, tkmv_settings_get_time_source (settings));
          points1[cnt].y
              = tkm_procevent_entry_get_data (entry, PEVENT_DATA_FORKS);
          points2[cnt].x = points1[cnt].x;
          points2[cnt].y
              = tkm_procevent_entry_get_data (entry, PEVENT_DATA_EXECS);
          points3[cnt].x = points1[cnt].x;
          points3[cnt].y
              = tkm_procevent_entry_get_data (entry, PEVENT_DATA_EXITS);
          points4[cnt].x = points1[cnt].x;
          points4[cnt].y
              = tkm_procevent_entry_get_data (entry, PEVENT_DATA_UIDS);
          points5[cnt].x = points1[cnt].x;
          points5[cnt].y
              = tkm_procevent_entry_get_data (entry, PEVENT_DATA_GIDS);
          cnt += 1;
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

  if (points4 == NULL)
    {
      points4 = calloc (1, sizeof (struct kpair));
    }

  if (points5 == NULL)
    {
      points5 = calloc (1, sizeof (struct kpair));
    }

  if (cnt == 0)
    {
      points1[cnt].x = 1;
      points1[cnt].y = 0;
      points2[cnt].x = points1[cnt].x;
      points2[cnt].y = 0;
      points3[cnt].x = points1[cnt].x;
      points3[cnt].y = 0;
      points4[cnt].x = points1[cnt].x;
      points4[cnt].y = 0;
      points5[cnt].x = points1[cnt].x;
      points5[cnt].y = 0;
      cnt += 1;
    }

  d1 = kdata_array_alloc (points1, cnt);
  d2 = kdata_array_alloc (points2, cnt);
  d3 = kdata_array_alloc (points3, cnt);
  d4 = kdata_array_alloc (points4, cnt);
  d5 = kdata_array_alloc (points5, cnt);

  kplotcfg_defaults (&plotcfg);
  plotcfg.grid = GRID_ALL;
  plotcfg.extrema = EXTREMA_YMIN;
  plotcfg.extrema_ymin = 0;
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

  kdatacfg_defaults (&d4_cfg);
  d4_cfg.line.sz = 1.0;
  d4_cfg.line.clr.type = KPLOTCTYPE_RGBA;
  d4_cfg.line.clr.rgba[0] = 0.4;
  d4_cfg.line.clr.rgba[1] = 0.0;
  d4_cfg.line.clr.rgba[2] = 0.6;
  d4_cfg.line.clr.rgba[3] = 1.0;

  kdatacfg_defaults (&d5_cfg);
  d5_cfg.line.sz = 1.0;
  d5_cfg.line.clr.type = KPLOTCTYPE_RGBA;
  d5_cfg.line.clr.rgba[0] = 0.9;
  d5_cfg.line.clr.rgba[1] = 0.4;
  d5_cfg.line.clr.rgba[2] = 0.3;
  d5_cfg.line.clr.rgba[3] = 1.0;

  kplot_attach_data (p, d1, KPLOT_LINES, &d1_cfg);
  kplot_attach_data (p, d2, KPLOT_LINES, &d2_cfg);
  kplot_attach_data (p, d3, KPLOT_LINES, &d3_cfg);
  kplot_attach_data (p, d4, KPLOT_LINES, &d4_cfg);
  kplot_attach_data (p, d5, KPLOT_LINES, &d5_cfg);

  kdata_destroy (d1);
  kdata_destroy (d2);
  kdata_destroy (d3);
  kdata_destroy (d4);
  kdata_destroy (d5);

  kplot_draw (p, width, height, cr);

  free (points1);
  free (points2);
  free (points3);
  free (points4);
  free (points5);

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
  plotcfg.yticlabelfmt = percent_format;

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
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  GPtrArray *mem_data = tkm_context_get_meminfo_entries (context);
  TkmSessionEntry *active_session = NULL;

  struct kpair *points1 = NULL; /* MemTotal */
  struct kpair *points2 = NULL; /* MemFree */
  struct kpair *points3 = NULL; /* MemAvail */
  struct kpair *points4 = NULL; /* SwapTotal */
  struct kpair *points5 = NULL; /* SwapFree */

  struct kplotcfg plotcfg;
  struct kdatacfg d1_cfg;
  struct kdatacfg d2_cfg;
  struct kdatacfg d3_cfg;
  struct kdatacfg d4_cfg;
  struct kdatacfg d5_cfg;
  struct kdata *d1;
  struct kdata *d2;
  struct kdata *d3;
  struct kdata *d4;
  struct kdata *d5;
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

  if (mem_data != NULL)
    {
      if (mem_data->len > 0)
        {
          points1 = calloc (mem_data->len, sizeof (struct kpair));
          points2 = calloc (mem_data->len, sizeof (struct kpair));
          points3 = calloc (mem_data->len, sizeof (struct kpair));
          points4 = calloc (mem_data->len, sizeof (struct kpair));
          points5 = calloc (mem_data->len, sizeof (struct kpair));
        }

      for (guint i = 0; i < mem_data->len; i++)
        {
          TkmMemInfoEntry *entry = g_ptr_array_index (mem_data, i);

          points1[cnt].x = tkm_meminfo_entry_get_timestamp (
              entry, tkmv_settings_get_time_source (settings));
          points1[cnt].y
              = tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_TOTAL);
          points2[cnt].x = points1[cnt].x;
          points2[cnt].y
              = tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_FREE);
          points3[cnt].x = points1[cnt].x;
          points3[cnt].y
              = tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_AVAIL);
          points4[cnt].x = points1[cnt].x;
          points4[cnt].y
              = tkm_meminfo_entry_get_data (entry, MINFO_DATA_SWAP_TOTAL);
          points5[cnt].x = points1[cnt].x;
          points5[cnt].y
              = tkm_meminfo_entry_get_data (entry, MINFO_DATA_SWAP_FREE);
          cnt += 1;
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

  if (points4 == NULL)
    {
      points4 = calloc (1, sizeof (struct kpair));
    }

  if (points5 == NULL)
    {
      points5 = calloc (1, sizeof (struct kpair));
    }

  if (cnt == 0)
    {
      points1[cnt].x = 1;
      points1[cnt].y = 0;
      points2[cnt].x = points1[cnt].x;
      points2[cnt].y = 0;
      points3[cnt].x = points1[cnt].x;
      points3[cnt].y = 0;
      points4[cnt].x = points1[cnt].x;
      points4[cnt].y = 0;
      points5[cnt].x = points1[cnt].x;
      points5[cnt].y = 0;
      cnt += 1;
    }

  d1 = kdata_array_alloc (points1, cnt);
  d2 = kdata_array_alloc (points2, cnt);
  d3 = kdata_array_alloc (points3, cnt);
  d4 = kdata_array_alloc (points4, cnt);
  d5 = kdata_array_alloc (points5, cnt);

  kplotcfg_defaults (&plotcfg);
  plotcfg.grid = GRID_ALL;
  plotcfg.extrema = EXTREMA_YMIN;
  plotcfg.extrema_ymin = 0;
  plotcfg.xticlabelfmt = timestamp_format;
  plotcfg.yticlabelfmt = memory_format;

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

  kdatacfg_defaults (&d4_cfg);
  d4_cfg.line.sz = 1.0;
  d4_cfg.line.clr.type = KPLOTCTYPE_RGBA;
  d4_cfg.line.clr.rgba[0] = 0.4;
  d4_cfg.line.clr.rgba[1] = 0.0;
  d4_cfg.line.clr.rgba[2] = 0.6;
  d4_cfg.line.clr.rgba[3] = 1.0;

  kdatacfg_defaults (&d5_cfg);
  d5_cfg.line.sz = 1.0;
  d5_cfg.line.clr.type = KPLOTCTYPE_RGBA;
  d5_cfg.line.clr.rgba[0] = 0.9;
  d5_cfg.line.clr.rgba[1] = 0.4;
  d5_cfg.line.clr.rgba[2] = 0.3;
  d5_cfg.line.clr.rgba[3] = 1.0;

  kplot_attach_data (p, d1, KPLOT_LINES, &d1_cfg);
  kplot_attach_data (p, d2, KPLOT_LINES, &d2_cfg);
  kplot_attach_data (p, d3, KPLOT_LINES, &d3_cfg);
  kplot_attach_data (p, d4, KPLOT_LINES, &d4_cfg);
  kplot_attach_data (p, d5, KPLOT_LINES, &d5_cfg);

  kdata_destroy (d1);
  kdata_destroy (d2);
  kdata_destroy (d3);
  kdata_destroy (d4);
  kdata_destroy (d5);

  kplot_draw (p, width, height, cr);

  free (points1);
  free (points2);
  free (points3);
  free (points4);
  free (points5);

  kplot_free (p);
}

static void
psi_history_draw_function (GtkDrawingArea *area, cairo_t *cr, int width,
                           int height, gpointer data)
{
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  TkmvSettings *settings
      = tkmv_application_get_settings (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  GPtrArray *psi_data = tkm_context_get_pressure_entries (context);
  TkmSessionEntry *active_session = NULL;

  struct kpair *points1 = NULL; /* CPUSome10 */
  struct kpair *points2 = NULL; /* CPUSome60 */
  struct kpair *points3 = NULL; /* MEMSome10 */
  struct kpair *points4 = NULL; /* MEMSome60 */
  struct kpair *points5 = NULL; /* IOSome10 */
  struct kpair *points6 = NULL; /* IOSome60 */

  struct kplotcfg plotcfg;
  struct kdatacfg d1_cfg;
  struct kdatacfg d2_cfg;
  struct kdatacfg d3_cfg;
  struct kdatacfg d4_cfg;
  struct kdatacfg d5_cfg;
  struct kdatacfg d6_cfg;
  struct kdata *d1;
  struct kdata *d2;
  struct kdata *d3;
  struct kdata *d4;
  struct kdata *d5;
  struct kdata *d6;
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

  if (psi_data != NULL)
    {
      if (psi_data->len > 0)
        {
          points1 = calloc (psi_data->len, sizeof (struct kpair));
          points2 = calloc (psi_data->len, sizeof (struct kpair));
          points3 = calloc (psi_data->len, sizeof (struct kpair));
          points4 = calloc (psi_data->len, sizeof (struct kpair));
          points5 = calloc (psi_data->len, sizeof (struct kpair));
          points6 = calloc (psi_data->len, sizeof (struct kpair));
        }

      for (guint i = 0; i < psi_data->len; i++)
        {
          TkmPressureEntry *entry = g_ptr_array_index (psi_data, i);

          points1[cnt].x = tkm_pressure_entry_get_timestamp (
              entry, tkmv_settings_get_time_source (settings));
          points1[cnt].y = tkm_pressure_entry_get_data_avg (
              entry, PSI_DATA_CPU_SOME_AVG10);
          points2[cnt].x = points1[cnt].x;
          points2[cnt].y = tkm_pressure_entry_get_data_avg (
              entry, PSI_DATA_CPU_SOME_AVG60);
          points3[cnt].x = points1[cnt].x;
          points3[cnt].y = tkm_pressure_entry_get_data_avg (
              entry, PSI_DATA_MEM_SOME_AVG10);
          points4[cnt].x = points1[cnt].x;
          points4[cnt].y = tkm_pressure_entry_get_data_avg (
              entry, PSI_DATA_MEM_SOME_AVG60);
          points5[cnt].x = points1[cnt].x;
          points5[cnt].y = tkm_pressure_entry_get_data_avg (
              entry, PSI_DATA_IO_SOME_AVG10);
          points6[cnt].x = points1[cnt].x;
          points6[cnt].y = tkm_pressure_entry_get_data_avg (
              entry, PSI_DATA_IO_SOME_AVG60);
          cnt += 1;
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

  if (points4 == NULL)
    {
      points4 = calloc (1, sizeof (struct kpair));
    }

  if (points5 == NULL)
    {
      points5 = calloc (1, sizeof (struct kpair));
    }

  if (points6 == NULL)
    {
      points6 = calloc (1, sizeof (struct kpair));
    }

  if (cnt == 0)
    {
      points1[cnt].x = 1;
      points1[cnt].y = 0;
      points2[cnt].x = points1[cnt].x;
      points2[cnt].y = 0;
      points3[cnt].x = points1[cnt].x;
      points3[cnt].y = 0;
      points4[cnt].x = points1[cnt].x;
      points4[cnt].y = 0;
      points5[cnt].x = points1[cnt].x;
      points5[cnt].y = 0;
      points6[cnt].x = points1[cnt].x;
      points6[cnt].y = 0;
      cnt += 1;
    }

  d1 = kdata_array_alloc (points1, cnt);
  d2 = kdata_array_alloc (points2, cnt);
  d3 = kdata_array_alloc (points3, cnt);
  d4 = kdata_array_alloc (points4, cnt);
  d5 = kdata_array_alloc (points5, cnt);
  d6 = kdata_array_alloc (points6, cnt);

  kplotcfg_defaults (&plotcfg);
  plotcfg.grid = GRID_ALL;
  plotcfg.extrema = EXTREMA_YMIN;
  plotcfg.extrema_ymin = 0;
  plotcfg.xticlabelfmt = timestamp_format;
  plotcfg.yticlabelfmt = pressure_format;

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

  kdatacfg_defaults (&d4_cfg);
  d4_cfg.line.sz = 1.0;
  d4_cfg.line.clr.type = KPLOTCTYPE_RGBA;
  d4_cfg.line.clr.rgba[0] = 0.4;
  d4_cfg.line.clr.rgba[1] = 0.0;
  d4_cfg.line.clr.rgba[2] = 0.6;
  d4_cfg.line.clr.rgba[3] = 1.0;

  kdatacfg_defaults (&d5_cfg);
  d5_cfg.line.sz = 1.0;
  d5_cfg.line.clr.type = KPLOTCTYPE_RGBA;
  d5_cfg.line.clr.rgba[0] = 0.9;
  d5_cfg.line.clr.rgba[1] = 0.4;
  d5_cfg.line.clr.rgba[2] = 0.3;
  d5_cfg.line.clr.rgba[3] = 1.0;

  kdatacfg_defaults (&d6_cfg);
  d6_cfg.line.sz = 1.0;
  d6_cfg.line.clr.type = KPLOTCTYPE_RGBA;
  d6_cfg.line.clr.rgba[0] = 1.0;
  d6_cfg.line.clr.rgba[1] = 0.9;
  d6_cfg.line.clr.rgba[2] = 0.1;
  d6_cfg.line.clr.rgba[3] = 1.0;

  kplot_attach_data (p, d1, KPLOT_LINES, &d1_cfg);
  kplot_attach_data (p, d2, KPLOT_LINES, &d2_cfg);
  kplot_attach_data (p, d3, KPLOT_LINES, &d3_cfg);
  kplot_attach_data (p, d4, KPLOT_LINES, &d4_cfg);
  kplot_attach_data (p, d5, KPLOT_LINES, &d5_cfg);
  kplot_attach_data (p, d6, KPLOT_LINES, &d6_cfg);

  kdata_destroy (d1);
  kdata_destroy (d2);
  kdata_destroy (d3);
  kdata_destroy (d4);
  kdata_destroy (d5);
  kdata_destroy (d6);

  kplot_draw (p, width, height, cr);

  free (points1);
  free (points2);
  free (points3);
  free (points4);
  free (points5);
  free (points6);

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
            tkm_cpustat_entry_get_all (entry)
                / tkm_session_entry_get_device_cpus (active_session));
  gtk_label_set_text (view->cpu_all_level_label, buf);

  snprintf (buf, sizeof (buf), "Usr - %3u %%",
            tkm_cpustat_entry_get_usr (entry)
                / tkm_session_entry_get_device_cpus (active_session));
  gtk_label_set_text (view->cpu_usr_level_label, buf);

  snprintf (buf, sizeof (buf), "Sys - %3u %%",
            tkm_cpustat_entry_get_sys (entry)
                / tkm_session_entry_get_device_cpus (active_session));
  gtk_label_set_text (view->cpu_sys_level_label, buf);

  g_assert (view);
}

static void
update_instant_mem_frame (TkmvDashboardView *view)
{
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  TkmSessionEntry *active_session = NULL;
  GPtrArray *mem_data = tkm_context_get_meminfo_entries (context);
  TkmMemInfoEntry *entry = NULL;
  gchar buf[64];

  if (mem_data == NULL)
    return;

  if (mem_data->len == 0)
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

  entry = g_ptr_array_index (mem_data, 0);

  gtk_level_bar_set_value (view->mem_level_bar,
                           100
                               - (double)tkm_meminfo_entry_get_data (
                                   entry, MINFO_DATA_MEM_PERCENT));
  gtk_level_bar_set_value (
      view->swap_level_bar,
      (double)tkm_meminfo_entry_get_data (entry, MINFO_DATA_SWAP_PERCENT));

  snprintf (buf, sizeof (buf), "Memory - %u %%",
            100 - tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_PERCENT));
  gtk_label_set_text (view->mem_level_label, buf);

  snprintf (buf, sizeof (buf), "Swap - %u %%",
            tkm_meminfo_entry_get_data (entry, MINFO_DATA_SWAP_PERCENT));
  gtk_label_set_text (view->swap_level_label, buf);

  g_assert (view);
}

static void
cpu_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr, int width,
                                int height, gpointer data)
{
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());

  if (tkm_context_data_try_lock (context))
    {
      cpu_history_draw_function (area, cr, width, height, data);
      tkm_context_data_unlock (context);
    }
}

static void
mem_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr, int width,
                                int height, gpointer data)
{
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());

  if (tkm_context_data_try_lock (context))
    {
      mem_history_draw_function (area, cr, width, height, data);
      tkm_context_data_unlock (context);
    }
}

static void
psi_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr, int width,
                                int height, gpointer data)
{
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());

  if (tkm_context_data_try_lock (context))
    {
      psi_history_draw_function (area, cr, width, height, data);
      tkm_context_data_unlock (context);
    }
}

static void
events_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr,
                                   int width, int height, gpointer data)
{
  TkmContext *context
      = tkmv_application_get_context (tkmv_application_instance ());

  if (tkm_context_data_try_lock (context))
    {
      events_history_draw_function (area, cr, width, height, data);
      tkm_context_data_unlock (context);
    }
}

static void psi_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr,
                                            int width, int height,
                                            gpointer data);

void
tkmv_dashboard_view_update_content (TkmvDashboardView *view)
{
  g_assert (view);

  update_instant_cpu_frame (view);
  gtk_widget_queue_draw (GTK_WIDGET (view->history_cpu_drawing_area));

  update_instant_mem_frame (view);
  gtk_widget_queue_draw (GTK_WIDGET (view->history_mem_drawing_area));

  gtk_widget_queue_draw (GTK_WIDGET (view->history_events_drawing_area));
  gtk_widget_queue_draw (GTK_WIDGET (view->history_psi_drawing_area));
}
