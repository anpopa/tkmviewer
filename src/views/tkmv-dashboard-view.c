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
#include "libkplot/extern.h"
#include <math.h>

#define KPOINTS_OPTIMIZATION_START_LIMIT (1024)

static void tkmv_dashboard_view_widgets_init (TkmvDashboardView *self);
static void update_current_values_frame (TkmvDashboardView *view);
static void cores_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                         int width, int height,
                                         gpointer data);
static void events_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                          int width, int height,
                                          gpointer data);
static void cpu_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                       int width, int height, gpointer data);
static void mem_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                       int width, int height, gpointer data);
static void psi_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                       int width, int height, gpointer data);
static void cores_history_draw_function_safe (GtkDrawingArea *area,
                                              cairo_t *cr, int width,
                                              int height, gpointer data);
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

struct _TkmvDashboardView {
  GtkBox parent_instance;

  /* Template widgets */
  GtkDrawingArea *history_cores_drawing_area;
  GtkDrawingArea *history_events_drawing_area;
  GtkDrawingArea *history_cpu_drawing_area;
  GtkDrawingArea *history_mem_drawing_area;
  GtkDrawingArea *history_psi_drawing_area;

  /* Current data */
  GtkLevelBar *cpu_all_level_bar;
  GtkLabel *cpu_all_level_label;
  GtkLevelBar *cpu_usr_level_bar;
  GtkLabel *cpu_usr_level_label;
  GtkLevelBar *cpu_sys_level_bar;
  GtkLabel *cpu_sys_level_label;
  GtkLevelBar *cpu_iow_level_bar;
  GtkLabel *cpu_iow_level_label;
  GtkLevelBar *mem_level_bar;
  GtkLabel *mem_level_label;
  GtkLevelBar *swap_level_bar;
  GtkLabel *swap_level_label;

  /* Core labels */
  GtkLabel *core0_entry_label;
  GtkLabel *core1_entry_label;
  GtkLabel *core2_entry_label;
  GtkLabel *core3_entry_label;
  GtkLabel *core4_entry_label;
  GtkLabel *core5_entry_label;
  GtkLabel *core6_entry_label;
  GtkLabel *core7_entry_label;
  GtkLabel *core8_entry_label;
  GtkLabel *core9_entry_label;
  GtkLabel *core10_entry_label;
  GtkLabel *core11_entry_label;
  GtkLabel *core12_entry_label;
  GtkLabel *core13_entry_label;
  GtkLabel *core14_entry_label;
  GtkLabel *core15_entry_label;
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
                                        history_cores_drawing_area);
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
                                        cpu_iow_level_bar);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        cpu_iow_level_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        mem_level_bar);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        mem_level_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        swap_level_bar);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        swap_level_label);

  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core0_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core1_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core2_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core3_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core4_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core5_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core6_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core7_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core8_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core9_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core10_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core11_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core12_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core13_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core14_entry_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView,
                                        core15_entry_label);
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
  gtk_drawing_area_set_draw_func (self->history_cores_drawing_area,
                                  cores_history_draw_function_safe, self,
                                  NULL);
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
cores_update_labels (TkmvDashboardView *self, TkmSessionEntry *active_session)
{
  g_assert (active_session);
  const guint cpu_count = tkm_session_entry_get_device_cpus (active_session);

  if (cpu_count > 0)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core0_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core0_entry_label), FALSE);
    }

  if (cpu_count > 1)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core1_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core1_entry_label), FALSE);
    }

  if (cpu_count > 2)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core2_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core2_entry_label), FALSE);
    }

  if (cpu_count > 3)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core3_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core3_entry_label), FALSE);
    }

  if (cpu_count > 4)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core4_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core4_entry_label), FALSE);
    }

  if (cpu_count > 5)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core5_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core5_entry_label), FALSE);
    }

  if (cpu_count > 6)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core6_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core6_entry_label), FALSE);
    }

  if (cpu_count > 7)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core7_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core7_entry_label), FALSE);
    }

  if (cpu_count > 8)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core8_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core8_entry_label), FALSE);
    }

  if (cpu_count > 9)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core9_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core9_entry_label), FALSE);
    }

  if (cpu_count > 10)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core10_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core10_entry_label), FALSE);
    }

  if (cpu_count > 11)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core11_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core11_entry_label), FALSE);
    }

  if (cpu_count > 12)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core12_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core12_entry_label), FALSE);
    }

  if (cpu_count > 13)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core13_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core13_entry_label), FALSE);
    }

  if (cpu_count > 14)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core14_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core14_entry_label), FALSE);
    }

  if (cpu_count > 15)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core15_entry_label), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->core15_entry_label), FALSE);
    }
}

static void
cores_history_draw_function (GtkDrawingArea *area, cairo_t *cr, int width,
                             int height, gpointer data)
{
  TkmContext *context
    = tkmv_application_get_context (tkmv_application_instance ());
  TkmvSettings *settings
    = tkmv_application_get_settings (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  GPtrArray *cpu_data = tkm_context_get_cpustat_entries (context);
  TkmSessionEntry *active_session = NULL;

  struct kdata *d1 = NULL; /* core0 */
  struct kdata *d2 = NULL; /* core1 */
  struct kdata *d3 = NULL; /* core2 */
  struct kdata *d4 = NULL; /* core3 */
  struct kdata *d5 = NULL; /* core4 */
  struct kdata *d6 = NULL; /* core5 */
  struct kdata *d7 = NULL; /* core6 */
  struct kdata *d8 = NULL; /* core7 */
  struct kdata *d9 = NULL; /* core8 */
  struct kdata *d10 = NULL; /* core9 */
  struct kdata *d11 = NULL; /* core10 */
  struct kdata *d12 = NULL; /* core11 */
  struct kdata *d13 = NULL; /* core12 */
  struct kdata *d14 = NULL; /* core13 */
  struct kdata *d15 = NULL; /* core14 */
  struct kdata *d16 = NULL; /* core15 */

  struct kplotcfg plotcfg;
  struct kplot *p;

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

      cores_update_labels (TKMV_DASHBOARD_VIEW (data), active_session);
    }

  if (cpu_data != NULL)
    {
      const guint cpu_count = tkm_session_entry_get_device_cpus (active_session);

      g_autofree guint *core0_entry_index_set = NULL;
      guint core0_entry_count = 0;
      g_autofree guint *core1_entry_index_set = NULL;
      guint core1_entry_count = 0;
      g_autofree guint *core2_entry_index_set = NULL;
      guint core2_entry_count = 0;
      g_autofree guint *core3_entry_index_set = NULL;
      guint core3_entry_count = 0;
      g_autofree guint *core4_entry_index_set = NULL;
      guint core4_entry_count = 0;
      g_autofree guint *core5_entry_index_set = NULL;
      guint core5_entry_count = 0;
      g_autofree guint *core6_entry_index_set = NULL;
      guint core6_entry_count = 0;
      g_autofree guint *core7_entry_index_set = NULL;
      guint core7_entry_count = 0;
      g_autofree guint *core8_entry_index_set = NULL;
      guint core8_entry_count = 0;
      g_autofree guint *core9_entry_index_set = NULL;
      guint core9_entry_count = 0;
      g_autofree guint *core10_entry_index_set = NULL;
      guint core10_entry_count = 0;
      g_autofree guint *core11_entry_index_set = NULL;
      guint core11_entry_count = 0;
      g_autofree guint *core12_entry_index_set = NULL;
      guint core12_entry_count = 0;
      g_autofree guint *core13_entry_index_set = NULL;
      guint core13_entry_count = 0;
      g_autofree guint *core14_entry_index_set = NULL;
      guint core14_entry_count = 0;
      g_autofree guint *core15_entry_index_set = NULL;
      guint core15_entry_count = 0;

      if (cpu_count > 0)
        core0_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 1)
        core1_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 2)
        core2_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 3)
        core3_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 4)
        core4_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 5)
        core5_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 6)
        core6_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 7)
        core7_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 8)
        core8_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 9)
        core9_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 10)
        core10_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 11)
        core11_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 12)
        core12_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 13)
        core13_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 14)
        core14_entry_index_set = calloc (cpu_data->len, sizeof(guint));
      if (cpu_count > 15)
        core15_entry_index_set = calloc (cpu_data->len, sizeof(guint));

      if (cpu_data->len < KPOINTS_OPTIMIZATION_START_LIMIT)
        {
          for (guint i = 0; i < cpu_data->len; i++)
            {
              TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, i);

              if ((cpu_count > 0) && g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu0") == 0)
                {
                  core0_entry_index_set[core0_entry_count++] = i;
                }
              else if ((cpu_count > 1) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu1") == 0)
                {
                  core1_entry_index_set[core1_entry_count++] = i;
                }
              else if ((cpu_count > 2) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu2") == 0)
                {
                  core2_entry_index_set[core2_entry_count++] = i;
                }
              else if ((cpu_count > 3) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu3") == 0)
                {
                  core3_entry_index_set[core3_entry_count++] = i;
                }
              else if ((cpu_count > 4) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu4") == 0)
                {
                  core4_entry_index_set[core4_entry_count++] = i;
                }
              else if ((cpu_count > 5) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu5") == 0)
                {
                  core5_entry_index_set[core5_entry_count++] = i;
                }
              else if ((cpu_count > 6) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu6") == 0)
                {
                  core6_entry_index_set[core6_entry_count++] = i;
                }
              else if ((cpu_count > 7) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu7") == 0)
                {
                  core7_entry_index_set[core7_entry_count++] = i;
                }
              else if ((cpu_count > 8) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu8") == 0)
                {
                  core8_entry_index_set[core8_entry_count++] = i;
                }
              else if ((cpu_count > 9) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu9") == 0)
                {
                  core9_entry_index_set[core9_entry_count++] = i;
                }
              else if ((cpu_count > 10) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu10") == 0)
                {
                  core10_entry_index_set[core10_entry_count++] = i;
                }
              else if ((cpu_count > 11) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu11") == 0)
                {
                  core11_entry_index_set[core11_entry_count++] = i;
                }
              else if ((cpu_count > 12) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu12") == 0)
                {
                  core12_entry_index_set[core12_entry_count++] = i;
                }
              else if ((cpu_count > 13) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu13") == 0)
                {
                  core13_entry_index_set[core13_entry_count++] = i;
                }
              else if ((cpu_count > 14) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu14") == 0)
                {
                  core14_entry_index_set[core14_entry_count++] = i;
                }
              else if ((cpu_count > 15) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu15") == 0)
                {
                  core15_entry_index_set[core15_entry_count++] = i;
                }
            }
        }
      else
        {
          for (guint i = 0; i < cpu_data->len; i++)
            {
              TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, i);

              if ((cpu_count > 0) && g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu0") == 0)
                {
                  if (core0_entry_count == 0)
                    {
                      core0_entry_index_set[core0_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data, core0_entry_index_set[core0_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core0_entry_index_set[core0_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 1) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu1") == 0)
                {
                  if (core1_entry_count == 0)
                    {
                      core1_entry_index_set[core1_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data, core1_entry_index_set[core1_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core1_entry_index_set[core1_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 2) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu2") == 0)
                {
                  if (core2_entry_count == 0)
                    {
                      core2_entry_index_set[core2_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data, core2_entry_index_set[core2_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core2_entry_index_set[core2_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 3) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu3") == 0)
                {
                  if (core3_entry_count == 0)
                    {
                      core3_entry_index_set[core3_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data, core3_entry_index_set[core3_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core3_entry_index_set[core3_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 4) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu4") == 0)
                {
                  if (core4_entry_count == 0)
                    {
                      core4_entry_index_set[core4_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data, core4_entry_index_set[core4_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core4_entry_index_set[core4_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 5) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu5") == 0)
                {
                  if (core5_entry_count == 0)
                    {
                      core5_entry_index_set[core5_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data, core5_entry_index_set[core5_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core5_entry_index_set[core5_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 6) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu6") == 0)
                {
                  if (core6_entry_count == 0)
                    {
                      core6_entry_index_set[core6_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data, core6_entry_index_set[core6_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core6_entry_index_set[core6_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 7) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu7") == 0)
                {
                  if (core7_entry_count == 0)
                    {
                      core7_entry_index_set[core7_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data, core7_entry_index_set[core7_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core7_entry_index_set[core7_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 8) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu8") == 0)
                {
                  if (core8_entry_count == 0)
                    {
                      core8_entry_index_set[core8_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data, core8_entry_index_set[core8_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core8_entry_index_set[core8_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 9) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu9") == 0)
                {
                  if (core9_entry_count == 0)
                    {
                      core9_entry_index_set[core9_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data, core9_entry_index_set[core9_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core9_entry_index_set[core9_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 10) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu10") == 0)
                {
                  if (core10_entry_count == 0)
                    {
                      core10_entry_index_set[core10_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data,
                                           core10_entry_index_set[core10_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core10_entry_index_set[core10_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 11) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu11") == 0)
                {
                  if (core11_entry_count == 0)
                    {
                      core11_entry_index_set[core11_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data,
                                           core11_entry_index_set[core11_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core11_entry_index_set[core11_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 12) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu12") == 0)
                {
                  if (core12_entry_count == 0)
                    {
                      core12_entry_index_set[core12_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data,
                                           core12_entry_index_set[core12_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core12_entry_index_set[core12_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 13) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu13") == 0)
                {
                  if (core13_entry_count == 0)
                    {
                      core13_entry_index_set[core13_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data,
                                           core13_entry_index_set[core13_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core13_entry_index_set[core13_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 14) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu14") == 0)
                {
                  if (core14_entry_count == 0)
                    {
                      core14_entry_index_set[core14_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data,
                                           core14_entry_index_set[core14_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core14_entry_index_set[core14_entry_count++] = i;
                        }
                    }
                }
              else if ((cpu_count > 15) &&
                       g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu15") == 0)
                {
                  if (core15_entry_count == 0)
                    {
                      core15_entry_index_set[core15_entry_count++] = i;
                    }
                  else
                    {
                      TkmCpuStatEntry *prev_entry =
                        g_ptr_array_index (cpu_data,
                                           core15_entry_index_set[core15_entry_count - 1]);

                      if (tkm_cpustat_entry_get_all (entry)
                          != tkm_cpustat_entry_get_all (prev_entry))
                        {
                          core15_entry_index_set[core15_entry_count++] = i;
                        }
                    }
                }
            }
        }

      if (core0_entry_count > 0)
        {
          d1 = kdata_array_alloc (NULL, core0_entry_count);
        }
      if (core1_entry_count > 0)
        {
          d2 = kdata_array_alloc (NULL, core1_entry_count);
        }
      if (core2_entry_count > 0)
        {
          d3 = kdata_array_alloc (NULL, core2_entry_count);
        }
      if (core3_entry_count > 0)
        {
          d4 = kdata_array_alloc (NULL, core3_entry_count);
        }
      if (core4_entry_count > 0)
        {
          d5 = kdata_array_alloc (NULL, core4_entry_count);
        }
      if (core5_entry_count > 0)
        {
          d6 = kdata_array_alloc (NULL, core5_entry_count);
        }
      if (core6_entry_count > 0)
        {
          d7 = kdata_array_alloc (NULL, core6_entry_count);
        }
      if (core7_entry_count > 0)
        {
          d8 = kdata_array_alloc (NULL, core7_entry_count);
        }
      if (core8_entry_count > 0)
        {
          d9 = kdata_array_alloc (NULL, core8_entry_count);
        }
      if (core9_entry_count > 0)
        {
          d10 = kdata_array_alloc (NULL, core9_entry_count);
        }
      if (core10_entry_count > 0)
        {
          d11 = kdata_array_alloc (NULL, core10_entry_count);
        }
      if (core11_entry_count > 0)
        {
          d12 = kdata_array_alloc (NULL, core11_entry_count);
        }
      if (core12_entry_count > 0)
        {
          d13 = kdata_array_alloc (NULL, core12_entry_count);
        }
      if (core13_entry_count > 0)
        {
          d14 = kdata_array_alloc (NULL, core13_entry_count);
        }
      if (core14_entry_count > 0)
        {
          d15 = kdata_array_alloc (NULL, core14_entry_count);
        }
      if (core15_entry_count > 0)
        {
          d16 = kdata_array_alloc (NULL, core15_entry_count);
        }

      for (guint i = 0; i < core0_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core0_entry_index_set[i]);

          d1->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d1->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core1_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core1_entry_index_set[i]);

          d2->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d2->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core2_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core2_entry_index_set[i]);

          d3->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d3->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core3_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core3_entry_index_set[i]);

          d4->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d4->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core4_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core4_entry_index_set[i]);

          d5->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d5->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core5_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core5_entry_index_set[i]);

          d6->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d6->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core6_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core6_entry_index_set[i]);

          d7->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d7->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core7_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core7_entry_index_set[i]);

          d8->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d8->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core8_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core8_entry_index_set[i]);

          d9->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d9->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core9_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core9_entry_index_set[i]);

          d10->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d10->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core10_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core10_entry_index_set[i]);

          d11->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d11->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core11_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core11_entry_index_set[i]);

          d12->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d12->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core12_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core12_entry_index_set[i]);

          d13->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d13->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core13_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core13_entry_index_set[i]);

          d14->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d14->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core14_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core14_entry_index_set[i]);

          d15->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d15->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
      for (guint i = 0; i < core15_entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, core15_entry_index_set[i]);

          d16->pairs[i].x = tkm_cpustat_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d16->pairs[i].y = tkm_cpustat_entry_get_all (entry);
        }
    }

  kplotcfg_defaults (&plotcfg);
  plotcfg.grid = GRID_ALL;
  plotcfg.extrema = EXTREMA_YMAX | EXTREMA_YMIN;
  plotcfg.extrema_ymin = 0;
  plotcfg.extrema_ymax = 100;
  plotcfg.xticlabelfmt = timestamp_format;
  plotcfg.yticlabelfmt = percent_format;

  p = kplot_alloc (&plotcfg);

  if (d1 != NULL)
    {
      struct kdatacfg d1_cfg;

      kdatacfg_defaults (&d1_cfg);
      d1_cfg.line.sz = 1.0;
      d1_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d1_cfg.line.clr.rgba[0] = 1.0;
      d1_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d1, KPLOT_LINES, &d1_cfg);
    }
  if (d2 != NULL)
    {
      struct kdatacfg d2_cfg;

      kdatacfg_defaults (&d2_cfg);
      d2_cfg.line.sz = 1.0;
      d2_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d2_cfg.line.clr.rgba[2] = 1.0;
      d2_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d2, KPLOT_LINES, &d2_cfg);
    }
  if (d3 != NULL)
    {
      struct kdatacfg d3_cfg;

      kdatacfg_defaults (&d3_cfg);
      d3_cfg.line.sz = 1.0;
      d3_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d3_cfg.line.clr.rgba[1] = 1.0;
      d3_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d3, KPLOT_LINES, &d3_cfg);
    }
  if (d4 != NULL)
    {
      struct kdatacfg d4_cfg;

      kdatacfg_defaults (&d4_cfg);
      d4_cfg.line.sz = 1.0;
      d4_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d4_cfg.line.clr.rgba[0] = 0.4;
      d4_cfg.line.clr.rgba[1] = 0.0;
      d4_cfg.line.clr.rgba[2] = 0.6;
      d4_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d4, KPLOT_LINES, &d4_cfg);
    }
  if (d5 != NULL)
    {
      struct kdatacfg d5_cfg;

      kdatacfg_defaults (&d5_cfg);
      d5_cfg.line.sz = 1.0;
      d5_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d5_cfg.line.clr.rgba[0] = 0.9;
      d5_cfg.line.clr.rgba[1] = 0.4;
      d5_cfg.line.clr.rgba[2] = 0.3;
      d5_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d5, KPLOT_LINES, &d5_cfg);
    }
  if (d6 != NULL)
    {
      struct kdatacfg d6_cfg;

      kdatacfg_defaults (&d6_cfg);
      d6_cfg.line.sz = 1.0;
      d6_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d6_cfg.line.clr.rgba[0] = 1.0;
      d6_cfg.line.clr.rgba[1] = 0.639;
      d6_cfg.line.clr.rgba[2] = 0.0;
      d6_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d6, KPLOT_LINES, &d6_cfg);
    }
  if (d7 != NULL)
    {
      struct kdatacfg d7_cfg;

      kdatacfg_defaults (&d7_cfg);
      d7_cfg.line.sz = 1.0;
      d7_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d7_cfg.line.clr.rgba[0] = 0.494;
      d7_cfg.line.clr.rgba[1] = 0.145;
      d7_cfg.line.clr.rgba[2] = 0.325;
      d7_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d7, KPLOT_LINES, &d7_cfg);
    }
  if (d8 != NULL)
    {
      struct kdatacfg d8_cfg;

      kdatacfg_defaults (&d8_cfg);
      d8_cfg.line.sz = 1.0;
      d8_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d8_cfg.line.clr.rgba[0] = 0.114;
      d8_cfg.line.clr.rgba[1] = 0.169;
      d8_cfg.line.clr.rgba[2] = 0.325;
      d8_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d8, KPLOT_LINES, &d8_cfg);
    }
  if (d9 != NULL)
    {
      struct kdatacfg d9_cfg;

      kdatacfg_defaults (&d9_cfg);
      d9_cfg.line.sz = 1.0;
      d9_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d9_cfg.line.clr.rgba[0] = 0.0;
      d9_cfg.line.clr.rgba[1] = 0.529;
      d9_cfg.line.clr.rgba[2] = 0.318;
      d9_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d9, KPLOT_LINES, &d9_cfg);
    }
  if (d10 != NULL)
    {
      struct kdatacfg d10_cfg;

      kdatacfg_defaults (&d10_cfg);
      d10_cfg.line.sz = 1.0;
      d10_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d10_cfg.line.clr.rgba[0] = 1.0;
      d10_cfg.line.clr.rgba[1] = 0.8;
      d10_cfg.line.clr.rgba[2] = 0.667;
      d10_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d10, KPLOT_LINES, &d10_cfg);
    }
  if (d11 != NULL)
    {
      struct kdatacfg d11_cfg;

      kdatacfg_defaults (&d11_cfg);
      d11_cfg.line.sz = 1.0;
      d11_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d11_cfg.line.clr.rgba[0] = 0.671;
      d11_cfg.line.clr.rgba[1] = 0.322;
      d11_cfg.line.clr.rgba[2] = 0.212;
      d11_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d11, KPLOT_LINES, &d11_cfg);
    }
  if (d12 != NULL)
    {
      struct kdatacfg d12_cfg;

      kdatacfg_defaults (&d12_cfg);
      d12_cfg.line.sz = 1.0;
      d12_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d12_cfg.line.clr.rgba[0] = 0.761;
      d12_cfg.line.clr.rgba[1] = 0.765;
      d12_cfg.line.clr.rgba[2] = 0.78;
      d12_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d12, KPLOT_LINES, &d12_cfg);
    }
  if (d13 != NULL)
    {
      struct kdatacfg d13_cfg;

      kdatacfg_defaults (&d13_cfg);
      d13_cfg.line.sz = 1.0;
      d13_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d13_cfg.line.clr.rgba[0] = 0.514;
      d13_cfg.line.clr.rgba[1] = 0.463;
      d13_cfg.line.clr.rgba[2] = 0.612;
      d13_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d13, KPLOT_LINES, &d13_cfg);
    }
  if (d14 != NULL)
    {
      struct kdatacfg d14_cfg;

      kdatacfg_defaults (&d14_cfg);
      d14_cfg.line.sz = 1.0;
      d14_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d14_cfg.line.clr.rgba[0] = 1.0;
      d14_cfg.line.clr.rgba[1] = 0.0;
      d14_cfg.line.clr.rgba[2] = 0.302;
      d14_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d14, KPLOT_LINES, &d14_cfg);
    }
  if (d15 != NULL)
    {
      struct kdatacfg d15_cfg;

      kdatacfg_defaults (&d15_cfg);
      d15_cfg.line.sz = 1.0;
      d15_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d15_cfg.line.clr.rgba[0] = 0.373;
      d15_cfg.line.clr.rgba[1] = 0.341;
      d15_cfg.line.clr.rgba[2] = 0.31;
      d15_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d15, KPLOT_LINES, &d15_cfg);
    }
  if (d16 != NULL)
    {
      struct kdatacfg d16_cfg;

      kdatacfg_defaults (&d16_cfg);
      d16_cfg.line.sz = 1.0;
      d16_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d16_cfg.line.clr.rgba[0] = 0.161;
      d16_cfg.line.clr.rgba[1] = 0.678;
      d16_cfg.line.clr.rgba[2] = 1.0;
      d16_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d16, KPLOT_LINES, &d16_cfg);
    }

  kplot_draw (p, width, height, cr);

  if (d1 != NULL)
    kdata_destroy (d1);
  if (d2 != NULL)
    kdata_destroy (d2);
  if (d3 != NULL)
    kdata_destroy (d3);
  if (d4 != NULL)
    kdata_destroy (d4);
  if (d5 != NULL)
    kdata_destroy (d5);
  if (d6 != NULL)
    kdata_destroy (d6);
  if (d7 != NULL)
    kdata_destroy (d7);
  if (d8 != NULL)
    kdata_destroy (d8);
  if (d9 != NULL)
    kdata_destroy (d9);
  if (d10 != NULL)
    kdata_destroy (d10);
  if (d11 != NULL)
    kdata_destroy (d11);
  if (d12 != NULL)
    kdata_destroy (d12);
  if (d13 != NULL)
    kdata_destroy (d13);
  if (d14 != NULL)
    kdata_destroy (d14);
  if (d15 != NULL)
    kdata_destroy (d15);
  if (d16 != NULL)
    kdata_destroy (d16);

  kplot_free (p);
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

  struct kdata *d1 = NULL; /* Forks */
  struct kdata *d2 = NULL; /* Execs */
  struct kdata *d3 = NULL; /* Exits */
  struct kdata *d4 = NULL; /* UIDs */
  struct kdata *d5 = NULL; /* GIDs */

  struct kplotcfg plotcfg;
  struct kplot *p;

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
      g_autofree guint *entry_index_set = calloc (events_data->len, sizeof(guint));
      guint entry_count = 0;

      if (events_data->len < KPOINTS_OPTIMIZATION_START_LIMIT)
        {
          for (guint i = 0; i < events_data->len; i++)
            entry_index_set[entry_count++] = i;
        }
      else
        {
          entry_index_set[entry_count++] = 0;
          for (guint i = 1; i < events_data->len; i++)
            {
              TkmProcEventEntry *prev_entry =
                g_ptr_array_index (events_data, entry_index_set[entry_count - 1]);
              TkmProcEventEntry *entry = g_ptr_array_index (events_data, i);
              gboolean should_add = false;

              if (tkm_procevent_entry_get_data (entry, PEVENT_DATA_FORKS)
                  != tkm_procevent_entry_get_data (prev_entry, PEVENT_DATA_FORKS))
                {
                  should_add = true;
                }
              else if (tkm_procevent_entry_get_data (entry, PEVENT_DATA_EXECS)
                       != tkm_procevent_entry_get_data (prev_entry, PEVENT_DATA_EXECS))
                {
                  should_add = true;
                }
              else if (tkm_procevent_entry_get_data (entry, PEVENT_DATA_EXITS)
                       != tkm_procevent_entry_get_data (prev_entry, PEVENT_DATA_EXITS))
                {
                  should_add = true;
                }
              else if (tkm_procevent_entry_get_data (entry, PEVENT_DATA_UIDS)
                       != tkm_procevent_entry_get_data (prev_entry, PEVENT_DATA_UIDS))
                {
                  should_add = true;
                }
              else if (tkm_procevent_entry_get_data (entry, PEVENT_DATA_GIDS)
                       != tkm_procevent_entry_get_data (prev_entry, PEVENT_DATA_GIDS))
                {
                  should_add = true;
                }

              if (should_add || (i == (events_data->len - 1)))
                {
                  entry_index_set[entry_count++] = i;
                }
            }
        }

      if (entry_count > 0)
        {
          g_debug ("Dashboard procevent EVENTS entry_count = %u", entry_count);
          d1 = kdata_array_alloc (NULL, entry_count);
          d2 = kdata_array_alloc (NULL, entry_count);
          d3 = kdata_array_alloc (NULL, entry_count);
          d4 = kdata_array_alloc (NULL, entry_count);
          d5 = kdata_array_alloc (NULL, entry_count);
        }

      for (guint i = 0; i < entry_count; i++)
        {
          TkmProcEventEntry *entry = g_ptr_array_index (events_data, entry_index_set[i]);

          d1->pairs[i].x = tkm_procevent_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d1->pairs[i].y
            = tkm_procevent_entry_get_data (entry, PEVENT_DATA_FORKS);
          d2->pairs[i].x = d1->pairs[i].x;
          d2->pairs[i].y
            = tkm_procevent_entry_get_data (entry, PEVENT_DATA_EXECS);
          d3->pairs[i].x = d1->pairs[i].x;
          d3->pairs[i].y
            = tkm_procevent_entry_get_data (entry, PEVENT_DATA_EXITS);
          d4->pairs[i].x = d1->pairs[i].x;
          d4->pairs[i].y
            = tkm_procevent_entry_get_data (entry, PEVENT_DATA_UIDS);
          d5->pairs[i].x = d1->pairs[i].x;
          d5->pairs[i].y
            = tkm_procevent_entry_get_data (entry, PEVENT_DATA_GIDS);
        }
    }

  kplotcfg_defaults (&plotcfg);
  plotcfg.grid = GRID_ALL;
  plotcfg.extrema = EXTREMA_YMIN;
  plotcfg.extrema_ymin = 0;
  plotcfg.xticlabelfmt = timestamp_format;

  p = kplot_alloc (&plotcfg);

  if (d1 != NULL)
    {
      struct kdatacfg d1_cfg;

      kdatacfg_defaults (&d1_cfg);
      d1_cfg.line.sz = 1.0;
      d1_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d1_cfg.line.clr.rgba[0] = 1.0;
      d1_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d1, KPLOT_LINES, &d1_cfg);
    }
  if (d2 != NULL)
    {
      struct kdatacfg d2_cfg;

      kdatacfg_defaults (&d2_cfg);
      d2_cfg.line.sz = 1.0;
      d2_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d2_cfg.line.clr.rgba[2] = 1.0;
      d2_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d2, KPLOT_LINES, &d2_cfg);
    }
  if (d3 != NULL)
    {
      struct kdatacfg d3_cfg;

      kdatacfg_defaults (&d3_cfg);
      d3_cfg.line.sz = 1.0;
      d3_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d3_cfg.line.clr.rgba[1] = 1.0;
      d3_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d3, KPLOT_LINES, &d3_cfg);
    }
  if (d4 != NULL)
    {
      struct kdatacfg d4_cfg;

      kdatacfg_defaults (&d4_cfg);
      d4_cfg.line.sz = 1.0;
      d4_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d4_cfg.line.clr.rgba[0] = 0.4;
      d4_cfg.line.clr.rgba[1] = 0.0;
      d4_cfg.line.clr.rgba[2] = 0.6;
      d4_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d4, KPLOT_LINES, &d4_cfg);
    }
  if (d5 != NULL)
    {
      struct kdatacfg d5_cfg;

      kdatacfg_defaults (&d5_cfg);
      d5_cfg.line.sz = 1.0;
      d5_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d5_cfg.line.clr.rgba[0] = 0.9;
      d5_cfg.line.clr.rgba[1] = 0.4;
      d5_cfg.line.clr.rgba[2] = 0.3;
      d5_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d5, KPLOT_LINES, &d5_cfg);
    }

  kplot_draw (p, width, height, cr);

  kdata_destroy (d1);
  kdata_destroy (d2);
  kdata_destroy (d3);
  kdata_destroy (d4);
  kdata_destroy (d5);

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

  struct kdata *d1 = NULL; /* all */
  struct kdata *d2 = NULL; /* usr */
  struct kdata *d3 = NULL; /* sys */
  struct kdata *d4 = NULL; /* iow */

  struct kplotcfg plotcfg;
  struct kplot *p;

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
      g_autofree guint *entry_index_set = calloc (cpu_data->len, sizeof(guint));
      guint entry_count = 0;

      if (cpu_data->len < KPOINTS_OPTIMIZATION_START_LIMIT)
        {
          for (guint i = 0; i < cpu_data->len; i++)
            {
              TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, i);

              if (g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu") == 0)
                {
                  entry_index_set[entry_count++] = i;
                }
            }
        }
      else
        {
          entry_index_set[entry_count++] = 0;
          for (guint i = 1; i < cpu_data->len; i++)
            {
              TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, i);

              if (g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu") == 0)
                {
                  TkmCpuStatEntry *prev_entry =
                    g_ptr_array_index (cpu_data, entry_index_set[entry_count - 1]);
                  gboolean should_add = false;

                  if (tkm_cpustat_entry_get_all (entry)
                      != tkm_cpustat_entry_get_all (prev_entry))
                    {
                      should_add = true;
                    }
                  else if (tkm_cpustat_entry_get_usr (entry)
                           != tkm_cpustat_entry_get_usr (prev_entry))
                    {
                      should_add = true;
                    }
                  else if (tkm_cpustat_entry_get_sys (entry)
                           != tkm_cpustat_entry_get_sys (prev_entry))
                    {
                      should_add = true;
                    }
                  else if (tkm_cpustat_entry_get_iow (entry)
                           != tkm_cpustat_entry_get_iow (prev_entry))
                    {
                      should_add = true;
                    }

                  if (should_add)
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }
            }
        }

      if (entry_count > 0)
        {
          g_debug ("Dashboard cpustat CPU entry_count = %u", entry_count);
          d1 = kdata_array_alloc (NULL, entry_count);
          d2 = kdata_array_alloc (NULL, entry_count);
          d3 = kdata_array_alloc (NULL, entry_count);
          d4 = kdata_array_alloc (NULL, entry_count);
        }

      for (guint i = 0; i < entry_count; i++)
        {
          TkmCpuStatEntry *entry = g_ptr_array_index (cpu_data, entry_index_set[i]);

          if (g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu") == 0)
            {
              d1->pairs[i].x = tkm_cpustat_entry_get_timestamp (
                entry, tkmv_settings_get_time_source (settings));
              d1->pairs[i].y = tkm_cpustat_entry_get_all (entry);
              d2->pairs[i].x = d1->pairs[i].x;
              d2->pairs[i].y = tkm_cpustat_entry_get_usr (entry);
              d3->pairs[i].x = d1->pairs[i].x;
              d3->pairs[i].y = tkm_cpustat_entry_get_sys (entry);
              d4->pairs[i].x = d1->pairs[i].x;
              d4->pairs[i].y = tkm_cpustat_entry_get_iow (entry);
            }
        }
    }

  kplotcfg_defaults (&plotcfg);
  plotcfg.grid = GRID_ALL;
  plotcfg.extrema = EXTREMA_YMAX | EXTREMA_YMIN;
  plotcfg.extrema_ymin = 0;
  plotcfg.extrema_ymax = 100;
  plotcfg.xticlabelfmt = timestamp_format;
  plotcfg.yticlabelfmt = percent_format;

  p = kplot_alloc (&plotcfg);

  if (d1 != NULL)
    {
      struct kdatacfg d1_cfg;

      kdatacfg_defaults (&d1_cfg);
      d1_cfg.line.sz = 1.0;
      d1_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d1_cfg.line.clr.rgba[0] = 1.0;
      d1_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d1, KPLOT_LINES, &d1_cfg);
    }
  if (d2 != NULL)
    {
      struct kdatacfg d2_cfg;

      kdatacfg_defaults (&d2_cfg);
      d2_cfg.line.sz = 1.0;
      d2_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d2_cfg.line.clr.rgba[2] = 1.0;
      d2_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d2, KPLOT_LINES, &d2_cfg);
    }
  if (d3 != NULL)
    {
      struct kdatacfg d3_cfg;

      kdatacfg_defaults (&d3_cfg);
      d3_cfg.line.sz = 1.0;
      d3_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d3_cfg.line.clr.rgba[1] = 1.0;
      d3_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d3, KPLOT_LINES, &d3_cfg);
    }
  if (d4 != NULL)
    {
      struct kdatacfg d4_cfg;

      kdatacfg_defaults (&d4_cfg);
      d4_cfg.line.sz = 1.0;
      d4_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d4_cfg.line.clr.rgba[0] = 0.4;
      d4_cfg.line.clr.rgba[1] = 0.0;
      d4_cfg.line.clr.rgba[2] = 0.6;
      d4_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d4, KPLOT_LINES, &d4_cfg);
    }

  kplot_draw (p, width, height, cr);

  kdata_destroy (d1);
  kdata_destroy (d2);
  kdata_destroy (d3);
  kdata_destroy (d4);

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

  struct kdata *d1 = NULL; /* MemTotal */
  struct kdata *d2 = NULL; /* MemFree */
  struct kdata *d3 = NULL; /* MemAvail */
  struct kdata *d4 = NULL; /* SwapTotal */
  struct kdata *d5 = NULL; /* SwapFree */

  struct kplotcfg plotcfg;
  struct kplot *p;

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
      g_autofree guint *entry_index_set = calloc (mem_data->len, sizeof(guint));
      guint entry_count = 0;

      if (mem_data->len < KPOINTS_OPTIMIZATION_START_LIMIT)
        {
          for (guint i = 0; i < mem_data->len; i++)
            entry_index_set[entry_count++] = i;
        }
      else
        {
          entry_index_set[entry_count++] = 0;
          for (guint i = 1; i < mem_data->len; i++)
            {
              TkmMemInfoEntry *prev_entry =
                g_ptr_array_index (mem_data, entry_index_set[entry_count - 1]);
              TkmMemInfoEntry *entry = g_ptr_array_index (mem_data, i);
              gboolean should_add = false;

              if (tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_TOTAL)
                  != tkm_meminfo_entry_get_data (prev_entry, MINFO_DATA_MEM_TOTAL))
                {
                  should_add = true;
                }
              else if (tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_FREE)
                       != tkm_meminfo_entry_get_data (prev_entry, MINFO_DATA_MEM_FREE))
                {
                  should_add = true;
                }
              else if (tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_AVAIL)
                       != tkm_meminfo_entry_get_data (prev_entry, MINFO_DATA_MEM_AVAIL))
                {
                  should_add = true;
                }
              else if (tkm_meminfo_entry_get_data (entry, MINFO_DATA_SWAP_TOTAL)
                       != tkm_meminfo_entry_get_data (prev_entry, MINFO_DATA_SWAP_TOTAL))
                {
                  should_add = true;
                }
              else if (tkm_meminfo_entry_get_data (entry, MINFO_DATA_SWAP_FREE)
                       != tkm_meminfo_entry_get_data (prev_entry, MINFO_DATA_SWAP_FREE))
                {
                  should_add = true;
                }

              if (should_add || (i == (mem_data->len - 1)))
                {
                  entry_index_set[entry_count++] = i;
                }
            }
        }

      if (entry_count > 0)
        {
          g_debug ("Dashboard meminfo MEM entry_count = %u", entry_count);
          d1 = kdata_array_alloc (NULL, entry_count);
          d2 = kdata_array_alloc (NULL, entry_count);
          d3 = kdata_array_alloc (NULL, entry_count);
          d4 = kdata_array_alloc (NULL, entry_count);
          d5 = kdata_array_alloc (NULL, entry_count);
        }

      for (guint i = 0; i < entry_count; i++)
        {
          TkmMemInfoEntry *entry = g_ptr_array_index (mem_data, entry_index_set[i]);

          d1->pairs[i].x = tkm_meminfo_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d1->pairs[i].y
            = tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_TOTAL);
          d2->pairs[i].x = d1->pairs[i].x;
          d2->pairs[i].y
            = tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_FREE);
          d3->pairs[i].x = d1->pairs[i].x;
          d3->pairs[i].y
            = tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_AVAIL);
          d4->pairs[i].x = d1->pairs[i].x;
          d4->pairs[i].y
            = tkm_meminfo_entry_get_data (entry, MINFO_DATA_SWAP_TOTAL);
          d5->pairs[i].x = d1->pairs[i].x;
          d5->pairs[i].y
            = tkm_meminfo_entry_get_data (entry, MINFO_DATA_SWAP_FREE);
        }
    }

  kplotcfg_defaults (&plotcfg);
  plotcfg.grid = GRID_ALL;
  plotcfg.extrema = EXTREMA_YMIN;
  plotcfg.extrema_ymin = 0;
  plotcfg.xticlabelfmt = timestamp_format;
  plotcfg.yticlabelfmt = memory_format;

  p = kplot_alloc (&plotcfg);

  if (d1 != NULL)
    {
      struct kdatacfg d1_cfg;

      kdatacfg_defaults (&d1_cfg);
      d1_cfg.line.sz = 1.0;
      d1_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d1_cfg.line.clr.rgba[0] = 1.0;
      d1_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d1, KPLOT_LINES, &d1_cfg);
    }
  if (d2 != NULL)
    {
      struct kdatacfg d2_cfg;

      kdatacfg_defaults (&d2_cfg);
      d2_cfg.line.sz = 1.0;
      d2_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d2_cfg.line.clr.rgba[2] = 1.0;
      d2_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d2, KPLOT_LINES, &d2_cfg);
    }
  if (d3 != NULL)
    {
      struct kdatacfg d3_cfg;

      kdatacfg_defaults (&d3_cfg);
      d3_cfg.line.sz = 1.0;
      d3_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d3_cfg.line.clr.rgba[1] = 1.0;
      d3_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d3, KPLOT_LINES, &d3_cfg);
    }
  if (d4 != NULL)
    {
      struct kdatacfg d4_cfg;

      kdatacfg_defaults (&d4_cfg);
      d4_cfg.line.sz = 1.0;
      d4_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d4_cfg.line.clr.rgba[0] = 0.4;
      d4_cfg.line.clr.rgba[1] = 0.0;
      d4_cfg.line.clr.rgba[2] = 0.6;
      d4_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d4, KPLOT_LINES, &d4_cfg);
    }
  if (d5 != NULL)
    {
      struct kdatacfg d5_cfg;

      kdatacfg_defaults (&d5_cfg);
      d5_cfg.line.sz = 1.0;
      d5_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d5_cfg.line.clr.rgba[0] = 0.9;
      d5_cfg.line.clr.rgba[1] = 0.4;
      d5_cfg.line.clr.rgba[2] = 0.3;
      d5_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d5, KPLOT_LINES, &d5_cfg);
    }

  kplot_draw (p, width, height, cr);

  kdata_destroy (d1);
  kdata_destroy (d2);
  kdata_destroy (d3);
  kdata_destroy (d4);
  kdata_destroy (d5);

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

  struct kdata *d1 = NULL; /* CPUSome10 */
  struct kdata *d2 = NULL; /* CPUSome60 */
  struct kdata *d3 = NULL; /* MEMSome10 */
  struct kdata *d4 = NULL; /* MEMSome60 */
  struct kdata *d5 = NULL; /* IOSome10 */
  struct kdata *d6 = NULL; /* IOSome60 */

  struct kplotcfg plotcfg;
  struct kplot *p = NULL;

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
      g_autofree guint *entry_index_set = calloc (psi_data->len, sizeof(guint));
      guint entry_count = 0;

      if (psi_data->len < KPOINTS_OPTIMIZATION_START_LIMIT)
        {
          for (guint i = 0; i < psi_data->len; i++)
            entry_index_set[entry_count++] = i;
        }
      else
        {
          entry_index_set[entry_count++] = 0;
          for (guint i = 1; i < psi_data->len; i++)
            {
              TkmPressureEntry *prev_entry =
                g_ptr_array_index (psi_data, entry_index_set[entry_count - 1]);
              TkmPressureEntry *entry = g_ptr_array_index (psi_data, i);
              gboolean should_add = false;

              if (tkm_pressure_entry_get_data_avg (entry, PSI_DATA_CPU_SOME_AVG10)
                  != tkm_pressure_entry_get_data_avg (prev_entry, PSI_DATA_CPU_SOME_AVG10))
                {
                  should_add = true;
                }
              else if (tkm_pressure_entry_get_data_avg (entry, PSI_DATA_CPU_SOME_AVG60)
                       != tkm_pressure_entry_get_data_avg (prev_entry, PSI_DATA_CPU_SOME_AVG60))
                {
                  should_add = true;
                }
              else if (tkm_pressure_entry_get_data_avg (entry, PSI_DATA_MEM_SOME_AVG10)
                       != tkm_pressure_entry_get_data_avg (prev_entry, PSI_DATA_MEM_SOME_AVG10))
                {
                  should_add = true;
                }
              else if (tkm_pressure_entry_get_data_avg (entry, PSI_DATA_MEM_SOME_AVG60)
                       != tkm_pressure_entry_get_data_avg (prev_entry, PSI_DATA_MEM_SOME_AVG60))
                {
                  should_add = true;
                }
              else if (tkm_pressure_entry_get_data_avg (entry, PSI_DATA_IO_SOME_AVG10)
                       != tkm_pressure_entry_get_data_avg (prev_entry, PSI_DATA_IO_SOME_AVG10))
                {
                  should_add = true;
                }
              else if (tkm_pressure_entry_get_data_avg (entry, PSI_DATA_IO_SOME_AVG60)
                       != tkm_pressure_entry_get_data_avg (prev_entry, PSI_DATA_IO_SOME_AVG60))
                {
                  should_add = true;
                }

              if (should_add || (i == (psi_data->len - 1)))
                {
                  entry_index_set[entry_count++] = i;
                }
            }
        }

      if (entry_count > 0)
        {
          g_debug ("Dashboard pressure PSI entry_count = %u", entry_count);
          d1 = kdata_array_alloc (NULL, entry_count);
          d2 = kdata_array_alloc (NULL, entry_count);
          d3 = kdata_array_alloc (NULL, entry_count);
          d4 = kdata_array_alloc (NULL, entry_count);
          d5 = kdata_array_alloc (NULL, entry_count);
          d6 = kdata_array_alloc (NULL, entry_count);
        }

      for (guint i = 0; i < entry_count; i++)
        {
          TkmPressureEntry *entry = g_ptr_array_index (psi_data, entry_index_set[i]);

          d1->pairs[i].x = tkm_pressure_entry_get_timestamp (
            entry, tkmv_settings_get_time_source (settings));
          d1->pairs[i].y = tkm_pressure_entry_get_data_avg (
            entry, PSI_DATA_CPU_SOME_AVG10);
          d2->pairs[i].x = d1->pairs[i].x;
          d2->pairs[i].y = tkm_pressure_entry_get_data_avg (
            entry, PSI_DATA_CPU_SOME_AVG60);
          d3->pairs[i].x = d1->pairs[i].x;
          d3->pairs[i].y = tkm_pressure_entry_get_data_avg (
            entry, PSI_DATA_MEM_SOME_AVG10);
          d4->pairs[i].x = d1->pairs[i].x;
          d4->pairs[i].y = tkm_pressure_entry_get_data_avg (
            entry, PSI_DATA_MEM_SOME_AVG60);
          d5->pairs[i].x = d1->pairs[i].x;
          d5->pairs[i].y = tkm_pressure_entry_get_data_avg (
            entry, PSI_DATA_IO_SOME_AVG10);
          d6->pairs[i].x = d1->pairs[i].x;
          d6->pairs[i].y = tkm_pressure_entry_get_data_avg (
            entry, PSI_DATA_IO_SOME_AVG60);
        }
    }

  kplotcfg_defaults (&plotcfg);
  plotcfg.grid = GRID_ALL;
  plotcfg.extrema = EXTREMA_YMIN;
  plotcfg.extrema_ymin = 0;
  plotcfg.xticlabelfmt = timestamp_format;
  plotcfg.yticlabelfmt = pressure_format;

  p = kplot_alloc (&plotcfg);

  if (d1 != NULL)
    {
      struct kdatacfg d1_cfg;

      kdatacfg_defaults (&d1_cfg);
      d1_cfg.line.sz = 1.0;
      d1_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d1_cfg.line.clr.rgba[0] = 1.0;
      d1_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d1, KPLOT_LINES, &d1_cfg);
    }
  if (d2 != NULL)
    {
      struct kdatacfg d2_cfg;

      kdatacfg_defaults (&d2_cfg);
      d2_cfg.line.sz = 1.0;
      d2_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d2_cfg.line.clr.rgba[2] = 1.0;
      d2_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d2, KPLOT_LINES, &d2_cfg);
    }
  if (d3 != NULL)
    {
      struct kdatacfg d3_cfg;

      kdatacfg_defaults (&d3_cfg);
      d3_cfg.line.sz = 1.0;
      d3_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d3_cfg.line.clr.rgba[1] = 1.0;
      d3_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d3, KPLOT_LINES, &d3_cfg);
    }
  if (d4 != NULL)
    {
      struct kdatacfg d4_cfg;

      kdatacfg_defaults (&d4_cfg);
      d4_cfg.line.sz = 1.0;
      d4_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d4_cfg.line.clr.rgba[0] = 0.4;
      d4_cfg.line.clr.rgba[1] = 0.0;
      d4_cfg.line.clr.rgba[2] = 0.6;
      d4_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d4, KPLOT_LINES, &d4_cfg);
    }
  if (d5 != NULL)
    {
      struct kdatacfg d5_cfg;

      kdatacfg_defaults (&d5_cfg);
      d5_cfg.line.sz = 1.0;
      d5_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d5_cfg.line.clr.rgba[0] = 0.9;
      d5_cfg.line.clr.rgba[1] = 0.4;
      d5_cfg.line.clr.rgba[2] = 0.3;
      d5_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d5, KPLOT_LINES, &d5_cfg);
    }
  if (d6 != NULL)
    {
      struct kdatacfg d6_cfg;

      kdatacfg_defaults (&d6_cfg);
      d6_cfg.line.sz = 1.0;
      d6_cfg.line.clr.type = KPLOTCTYPE_RGBA;
      d6_cfg.line.clr.rgba[0] = 1.0;
      d6_cfg.line.clr.rgba[1] = 0.9;
      d6_cfg.line.clr.rgba[2] = 0.1;
      d6_cfg.line.clr.rgba[3] = 1.0;

      kplot_attach_data (p, d6, KPLOT_LINES, &d6_cfg);
    }

  kplot_draw (p, width, height, cr);

  kdata_destroy (d1);
  kdata_destroy (d2);
  kdata_destroy (d3);
  kdata_destroy (d4);
  kdata_destroy (d5);
  kdata_destroy (d6);

  kplot_free (p);
}

static void
update_current_values_frame (TkmvDashboardView *view)
{
  TkmContext *context
    = tkmv_application_get_context (tkmv_application_instance ());
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  TkmSessionEntry *active_session = NULL;
  GPtrArray *cpu_data = tkm_context_get_cpustat_entries (context);
  GPtrArray *mem_data = tkm_context_get_meminfo_entries (context);
  TkmCpuStatEntry *cpu_entry = NULL;
  TkmMemInfoEntry *mem_entry = NULL;
  gchar buf[64];

  g_assert (view);

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

  cpu_entry = g_ptr_array_index (cpu_data, 0);
  g_assert (cpu_entry); /* we should have an entry */

  /* CPU Data */
  gtk_level_bar_set_value (
    view->cpu_all_level_bar,
    (double)tkm_cpustat_entry_get_all (cpu_entry));
  gtk_level_bar_set_value (
    view->cpu_usr_level_bar,
    (double)tkm_cpustat_entry_get_usr (cpu_entry));
  gtk_level_bar_set_value (
    view->cpu_sys_level_bar,
    (double)tkm_cpustat_entry_get_sys (cpu_entry));
  gtk_level_bar_set_value (
    view->cpu_iow_level_bar,
    (double)tkm_cpustat_entry_get_iow (cpu_entry));

  snprintf (buf, sizeof(buf), "%3u %%",
            tkm_cpustat_entry_get_all (cpu_entry));
  gtk_label_set_text (view->cpu_all_level_label, buf);

  snprintf (buf, sizeof(buf), "%3u %%",
            tkm_cpustat_entry_get_usr (cpu_entry));
  gtk_label_set_text (view->cpu_usr_level_label, buf);

  snprintf (buf, sizeof(buf), "%3u %%",
            tkm_cpustat_entry_get_sys (cpu_entry));
  gtk_label_set_text (view->cpu_sys_level_label, buf);

  snprintf (buf, sizeof(buf), "%3u %%",
            tkm_cpustat_entry_get_iow (cpu_entry));
  gtk_label_set_text (view->cpu_iow_level_label, buf);

  /* Memory Data */
  if (mem_data == NULL)
    return;

  if (mem_data->len == 0)
    return;

  mem_entry = g_ptr_array_index (mem_data, 0);
  g_assert (mem_entry); /* we should have an entry */

  gtk_level_bar_set_value (view->mem_level_bar,
                           100
                           - (double)tkm_meminfo_entry_get_data (
                             mem_entry, MINFO_DATA_MEM_PERCENT));
  gtk_level_bar_set_value (
    view->swap_level_bar,
    (double)tkm_meminfo_entry_get_data (mem_entry, MINFO_DATA_SWAP_PERCENT));

  snprintf (buf, sizeof(buf), "%3u %%",
            100 - tkm_meminfo_entry_get_data (mem_entry, MINFO_DATA_MEM_PERCENT));
  gtk_label_set_text (view->mem_level_label, buf);

  snprintf (buf, sizeof(buf), "%3u %%",
            tkm_meminfo_entry_get_data (mem_entry, MINFO_DATA_SWAP_PERCENT));
  gtk_label_set_text (view->swap_level_label, buf);
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

static void
cores_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr,
                                  int width, int height, gpointer data)
{
  TkmContext *context
    = tkmv_application_get_context (tkmv_application_instance ());

  if (tkm_context_data_try_lock (context))
    {
      cores_history_draw_function (area, cr, width, height, data);
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

  update_current_values_frame (view);
  gtk_widget_queue_draw (GTK_WIDGET (view->history_cpu_drawing_area));
  gtk_widget_queue_draw (GTK_WIDGET (view->history_mem_drawing_area));
  gtk_widget_queue_draw (GTK_WIDGET (view->history_cores_drawing_area));
  gtk_widget_queue_draw (GTK_WIDGET (view->history_events_drawing_area));
  gtk_widget_queue_draw (GTK_WIDGET (view->history_psi_drawing_area));
}

