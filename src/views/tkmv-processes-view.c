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
#include "tkm-ctxinfo-entry.h"
#include "tkm-procacct-entry.h"
#include "tkm-procinfo-entry.h"

#include "tkm-cpustat-entry.h"
#include "tkm-meminfo-entry.h"
#include "tkm-settings.h"
#include "tkmv-application.h"
#include "tkmv-types.h"

#include "libkplot/kplot.h"
#include "libkplot/extern.h"
#include <math.h>

enum {
  COLUMN_PROCINFO_NAME,
  COLUMN_PROCINFO_PID,
  COLUMN_PROCINFO_PPID,
  COLUMN_PROCINFO_CONTEXT,
  COLUMN_PROCINFO_CPU_TIME,
  COLUMN_PROCINFO_CPU_PERCENT,
  COLUMN_PROCINFO_MEM_RSS,
  COLUMN_PROCINFO_MEM_PSS,
  PROCINFO_NUM_COLUMNS
};

enum {
  COLUMN_CTXINFO_NAME,
  COLUMN_CTXINFO_ID,
  COLUMN_CTXINFO_CPU_TIME,
  COLUMN_CTXINFO_CPU_PERCENT,
  COLUMN_CTXINFO_MEM_RSS,
  COLUMN_CTXINFO_MEM_PSS,
  CTXINFO_NUM_COLUMNS
};

enum {
  COLUMN_PROCACCT_NAME,
  COLUMN_PROCACCT_PID,
  COLUMN_PROCACCT_PPID,
  COLUMN_PROCACCT_UID,
  COLUMN_PROCACCT_GID,
  COLUMN_PROCACCT_UTIME,
  COLUMN_PROCACCT_STIME,
  COLUMN_PROCACCT_CPU_COUNT,
  COLUMN_PROCACCT_CPU_RUN_REAL,
  COLUMN_PROCACCT_CPU_RUN_VIRTUAL,
  COLUMN_PROCACCT_CPU_DELAY_TOTAL,
  COLUMN_PROCACCT_CPU_DELAY_AVG,
  COLUMN_PROCACCT_CORE_MEM,
  COLUMN_PROCACCT_VIRT_MEM,
  COLUMN_PROCACCT_HIGH_WATER_RSS,
  COLUMN_PROCACCT_HIGH_WATER_VM,
  COLUMN_PROCACCT_NVCSW,
  COLUMN_PROCACCT_NIVCSW,
  COLUMN_PROCACCT_SWAPIN_COUNT,
  COLUMN_PROCACCT_SWAPIN_DELAY_TOTAL,
  COLUMN_PROCACCT_SWAPIN_DELAY_AVG,
  COLUMN_PROCACCT_BLKIO_COUNT,
  COLUMN_PROCACCT_BLKIO_DELAY_TOTAL,
  COLUMN_PROCACCT_BLKIO_DELAY_AVG,
  COLUMN_PROCACCT_IO_STORAGE_READ,
  COLUMN_PROCACCT_IO_STORAGE_WRITE,
  COLUMN_PROCACCT_IO_READ_CHAR,
  COLUMN_PROCACCT_IO_WRITE_CHAR,
  COLUMN_PROCACCT_IO_READ_SYSCALLS,
  COLUMN_PROCACCT_IO_WRITE_SYSCALLS,
  COLUMN_PROCACCT_FREEPAGE_COUNT,
  COLUMN_PROCACCT_FREEPAGE_DELAY_TOTAL,
  COLUMN_PROCACCT_FREEPAGE_DELAY_AVG,
  COLUMN_PROCACCT_TRASHING_COUNT,
  COLUMN_PROCACCT_TRASHING_DELAY_TOTAL,
  COLUMN_PROCACCT_TRASHING_DELAY_AVG,
  PROCACCT_NUM_COLUMNS
};

static void tkmv_processes_view_widgets_init (TkmvProcessesView *self);
static void procinfo_add_columns (TkmvProcessesView *self);
static void procinfo_selection_changed (GtkTreeSelection *selection,
                                        gpointer data);
static void ctxinfo_add_columns (TkmvProcessesView *self);
static void ctxinfo_selection_changed (GtkTreeSelection *selection,
                                       gpointer data);
static void procacct_add_columns (TkmvProcessesView *self);
static void procacct_selection_changed (GtkTreeSelection *selection,
                                        gpointer data);
static void create_tables (TkmvProcessesView *self);

static void procinfo_list_store_append_entry (GtkListStore *list_store,
                                              TkmProcInfoEntry *entry,
                                              GtkTreeIter *iter);
static void reload_procinfo_entries (TkmvProcessesView *view,
                                     TkmContext *context);
static void ctxinfo_list_store_append_entry (GtkListStore *list_store,
                                             TkmCtxInfoEntry *entry,
                                             GtkTreeIter *iter);
static void reload_ctxinfo_entries (TkmvProcessesView *view,
                                    TkmContext *context);
static void procacct_list_store_append_entry (GtkListStore *list_store,
                                              TkmProcAcctEntry *entry,
                                              GtkTreeIter *iter);
static void reload_procacct_entries (TkmvProcessesView *view,
                                     TkmContext *context);

static void procinfo_cpu_history_draw_function (GtkDrawingArea *area,
                                                cairo_t *cr, int width,
                                                int height, gpointer data);
static void procinfo_mem_history_draw_function (GtkDrawingArea *area,
                                                cairo_t *cr, int width,
                                                int height, gpointer data);
static void procinfo_cpu_history_draw_function_safe (GtkDrawingArea *area,
                                                     cairo_t *cr, int width,
                                                     int height,
                                                     gpointer data);
static void procinfo_mem_history_draw_function_safe (GtkDrawingArea *area,
                                                     cairo_t *cr, int width,
                                                     int height,
                                                     gpointer data);
static void ctxinfo_cpu_history_draw_function (GtkDrawingArea *area,
                                               cairo_t *cr, int width,
                                               int height, gpointer data);
static void ctxinfo_mem_history_draw_function (GtkDrawingArea *area,
                                               cairo_t *cr, int width,
                                               int height, gpointer data);
static void ctxinfo_cpu_history_draw_function_safe (GtkDrawingArea *area,
                                                    cairo_t *cr, int width,
                                                    int height, gpointer data);
static void ctxinfo_mem_history_draw_function_safe (GtkDrawingArea *area,
                                                    cairo_t *cr, int width,
                                                    int height, gpointer data);

struct _TkmvProcessesView {
  GtkBox parent_instance;

  GtkListStore *procinfo_store;
  GtkListStore *ctxinfo_store;
  GtkListStore *procacct_store;

  /* Template widgets */
  GtkScrolledWindow *procinfo_scrolled_window;
  GtkTreeView *procinfo_treeview;
  GtkTreeSelection *procinfo_treeview_select;
  GtkScrolledWindow *ctxinfo_scrolled_window;
  GtkTreeView *ctxinfo_treeview;
  GtkTreeSelection *ctxinfo_treeview_select;
  GtkScrolledWindow *procacct_scrolled_window;
  GtkTreeView *procacct_treeview;
  GtkTreeSelection *procacct_treeview_select;

  GtkFrame *procinfo_history_cpu_frame;
  GtkDrawingArea *procinfo_history_cpu_drawing_area;
  GtkDrawingArea *procinfo_history_mem_drawing_area;
  GtkDrawingArea *ctxinfo_history_cpu_drawing_area;
  GtkDrawingArea *ctxinfo_history_mem_drawing_area;

  GtkLabel *procinfo_cpu_entry1_label;
  GtkLabel *procinfo_cpu_entry2_label;
  GtkLabel *procinfo_cpu_entry3_label;
  GtkLabel *procinfo_cpu_entry4_label;
  GtkLabel *procinfo_cpu_entry5_label;
  GtkLabel *procinfo_mem_entry1_label;
  GtkLabel *procinfo_mem_entry2_label;
  GtkLabel *procinfo_mem_entry3_label;
  GtkLabel *procinfo_mem_entry4_label;
  GtkLabel *procinfo_mem_entry5_label;

  GtkLabel *ctxinfo_cpu_entry1_label;
  GtkLabel *ctxinfo_cpu_entry2_label;
  GtkLabel *ctxinfo_cpu_entry3_label;
  GtkLabel *ctxinfo_cpu_entry4_label;
  GtkLabel *ctxinfo_cpu_entry5_label;
  GtkLabel *ctxinfo_mem_entry1_label;
  GtkLabel *ctxinfo_mem_entry2_label;
  GtkLabel *ctxinfo_mem_entry3_label;
  GtkLabel *ctxinfo_mem_entry4_label;
  GtkLabel *ctxinfo_mem_entry5_label;
};

G_DEFINE_TYPE (TkmvProcessesView, tkmv_processes_view, GTK_TYPE_BOX)

static void
tkmv_processes_view_class_init (TkmvProcessesViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (
    widget_class,
    "/ro/fxdata/taskmonitor/viewer/gtk/tkmv-processes-view.ui");

  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_treeview);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_treeview_select);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_treeview);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_treeview_select);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procacct_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procacct_treeview);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procacct_treeview_select);

  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_history_cpu_frame);

  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_history_cpu_drawing_area);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_history_mem_drawing_area);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_history_cpu_drawing_area);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_history_mem_drawing_area);

  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_cpu_entry1_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_cpu_entry2_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_cpu_entry3_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_cpu_entry4_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_cpu_entry5_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_mem_entry1_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_mem_entry2_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_mem_entry3_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_mem_entry4_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        procinfo_mem_entry5_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_cpu_entry1_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_cpu_entry2_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_cpu_entry3_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_cpu_entry4_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_cpu_entry5_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_mem_entry1_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_mem_entry2_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_mem_entry3_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_mem_entry4_label);
  gtk_widget_class_bind_template_child (widget_class, TkmvProcessesView,
                                        ctxinfo_mem_entry5_label);
}

static void
tkmv_processes_view_init (TkmvProcessesView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  tkmv_processes_view_widgets_init (self);
}

static void
tkmv_processes_view_widgets_init (TkmvProcessesView *self)
{
  create_tables (self);

  gtk_drawing_area_set_draw_func (self->procinfo_history_cpu_drawing_area,
                                  procinfo_cpu_history_draw_function_safe,
                                  self, NULL);
  gtk_drawing_area_set_draw_func (self->procinfo_history_mem_drawing_area,
                                  procinfo_mem_history_draw_function_safe,
                                  self, NULL);
  gtk_drawing_area_set_draw_func (self->ctxinfo_history_cpu_drawing_area,
                                  ctxinfo_cpu_history_draw_function_safe, self,
                                  NULL);
  gtk_drawing_area_set_draw_func (self->ctxinfo_history_mem_drawing_area,
                                  ctxinfo_mem_history_draw_function_safe, self,
                                  NULL);
  /* ProcAcct tab is optional */
  gtk_widget_set_visible (GTK_WIDGET (self->procacct_scrolled_window), FALSE);
}

static void
procinfo_add_columns (TkmvProcessesView *self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "Name", renderer, "text", COLUMN_PROCINFO_NAME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_PROCINFO_NAME);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column pid */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "PID", renderer, "text", COLUMN_PROCINFO_PID, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_PROCINFO_PID);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column ppid */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "PPID", renderer, "text", COLUMN_PROCINFO_PPID, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_PROCINFO_PPID);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column context */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "Context", renderer, "text", COLUMN_PROCINFO_CONTEXT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_PROCINFO_CONTEXT);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column cputime */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "CPUTime", renderer, "text", COLUMN_PROCINFO_CPU_TIME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_PROCINFO_CPU_TIME);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column cpupercent */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "CPUPercent", renderer, "text", COLUMN_PROCINFO_CPU_PERCENT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_PROCINFO_CPU_PERCENT);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column memrss */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "RSS", renderer, "text", COLUMN_PROCINFO_MEM_RSS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_PROCINFO_MEM_RSS);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column mempss */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "PSS", renderer, "text", COLUMN_PROCINFO_MEM_PSS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_PROCINFO_MEM_PSS);
  gtk_tree_view_append_column (self->procinfo_treeview, column);
}

gboolean
limit_selection_function (GtkTreeSelection *selection, GtkTreeModel *model,
                          GtkTreePath *path, gboolean path_currently_selected,
                          gpointer data)
{
  TKM_UNUSED (model);
  TKM_UNUSED (path);
  TKM_UNUSED (data);

  if (!path_currently_selected)
    {
      if (gtk_tree_selection_count_selected_rows (selection) < 5)
        {
          return TRUE;
        }
      return FALSE;
    }
  return TRUE;
}

static void
procinfo_selection_foreach_get_pid (GtkTreeModel *model, GtkTreePath *path,
                                    GtkTreeIter *iter, gpointer data)
{
  GList **selected = (GList **)data;
  guint pid = 0;

  TKM_UNUSED (path);

  gtk_tree_model_get (model, iter, COLUMN_PROCINFO_PID, &pid, -1);
  *selected = g_list_append (*selected, GINT_TO_POINTER (pid));
}

static void
procinfo_selection_foreach_get_name (GtkTreeModel *model, GtkTreePath *path,
                                     GtkTreeIter *iter, gpointer data)
{
  GList **selected = (GList **)data;
  gchar *name = NULL;

  TKM_UNUSED (path);

  gtk_tree_model_get (model, iter, COLUMN_PROCINFO_NAME, &name, -1);
  *selected = g_list_append (*selected, name);
}

static void
procinfo_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  TkmvProcessesView *self = (TkmvProcessesView *)data;
  GList *selected_pids = NULL;
  GList *selected_names = NULL;
  guint selected_count = 0;

  selected_count = gtk_tree_selection_count_selected_rows (
    self->procinfo_treeview_select);
  gtk_tree_selection_selected_foreach (
    selection, procinfo_selection_foreach_get_pid, &selected_pids);
  gtk_tree_selection_selected_foreach (
    selection, procinfo_selection_foreach_get_name, &selected_names);

  if (selected_count > 0)
    {
      g_autofree gchar *name = g_strdup_printf (
        "%s[%d]", (gchar *)g_list_nth (selected_names, 0)->data,
        GPOINTER_TO_INT (g_list_nth (selected_pids, 0)->data));
      gtk_label_set_text (self->procinfo_cpu_entry1_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_cpu_entry1_label),
                              TRUE);
      gtk_label_set_text (self->procinfo_mem_entry1_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_mem_entry1_label),
                              TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_cpu_entry1_label),
                              FALSE);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_mem_entry1_label),
                              FALSE);
    }

  if (selected_count > 1)
    {
      g_autofree gchar *name = g_strdup_printf (
        "%s[%d]", (gchar *)g_list_nth (selected_names, 1)->data,
        GPOINTER_TO_INT (g_list_nth (selected_pids, 1)->data));
      gtk_label_set_text (self->procinfo_cpu_entry2_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_cpu_entry2_label),
                              TRUE);
      gtk_label_set_text (self->procinfo_mem_entry2_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_mem_entry2_label),
                              TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_cpu_entry2_label),
                              FALSE);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_mem_entry2_label),
                              FALSE);
    }

  if (selected_count > 2)
    {
      g_autofree gchar *name = g_strdup_printf (
        "%s[%d]", (gchar *)g_list_nth (selected_names, 2)->data,
        GPOINTER_TO_INT (g_list_nth (selected_pids, 2)->data));
      gtk_label_set_text (self->procinfo_cpu_entry3_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_cpu_entry3_label),
                              TRUE);
      gtk_label_set_text (self->procinfo_mem_entry3_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_mem_entry3_label),
                              TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_cpu_entry3_label),
                              FALSE);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_mem_entry3_label),
                              FALSE);
    }

  if (selected_count > 3)
    {
      g_autofree gchar *name = g_strdup_printf (
        "%s[%d]", (gchar *)g_list_nth (selected_names, 3)->data,
        GPOINTER_TO_INT (g_list_nth (selected_pids, 3)->data));
      gtk_label_set_text (self->procinfo_cpu_entry4_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_cpu_entry4_label),
                              TRUE);
      gtk_label_set_text (self->procinfo_mem_entry4_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_mem_entry4_label),
                              TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_cpu_entry4_label),
                              FALSE);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_mem_entry4_label),
                              FALSE);
    }

  if (selected_count > 4)
    {
      g_autofree gchar *name = g_strdup_printf (
        "%s[%d]", (gchar *)g_list_nth (selected_names, 4)->data,
        GPOINTER_TO_INT (g_list_nth (selected_pids, 4)->data));
      gtk_label_set_text (self->procinfo_cpu_entry5_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_cpu_entry5_label),
                              TRUE);
      gtk_label_set_text (self->procinfo_mem_entry5_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_mem_entry5_label),
                              TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_cpu_entry5_label),
                              FALSE);
      gtk_widget_set_visible (GTK_WIDGET (self->procinfo_mem_entry5_label),
                              FALSE);
    }

  gtk_widget_queue_draw (GTK_WIDGET (self->procinfo_history_cpu_drawing_area));
  gtk_widget_queue_draw (GTK_WIDGET (self->procinfo_history_mem_drawing_area));

  g_list_free (selected_pids);
  g_list_free_full (selected_names, g_free);
}

static void
ctxinfo_add_columns (TkmvProcessesView *self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "Name", renderer, "text", COLUMN_CTXINFO_NAME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_CTXINFO_NAME);
  gtk_tree_view_append_column (self->ctxinfo_treeview, column);

  /* column id */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ID", renderer, "text",
                                                     COLUMN_CTXINFO_ID, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_CTXINFO_ID);
  gtk_tree_view_append_column (self->ctxinfo_treeview, column);

  /* column cputime */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "CPUTime", renderer, "text", COLUMN_CTXINFO_CPU_TIME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_CTXINFO_CPU_TIME);
  gtk_tree_view_append_column (self->ctxinfo_treeview, column);

  /* column cpupercent */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "CPUPercent", renderer, "text", COLUMN_CTXINFO_CPU_PERCENT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_CTXINFO_CPU_PERCENT);
  gtk_tree_view_append_column (self->ctxinfo_treeview, column);

  /* column memrss */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "RSS", renderer, "text", COLUMN_CTXINFO_MEM_RSS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_CTXINFO_MEM_RSS);
  gtk_tree_view_append_column (self->ctxinfo_treeview, column);

  /* column mempss */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "PSS", renderer, "text", COLUMN_CTXINFO_MEM_PSS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_CTXINFO_MEM_PSS);
  gtk_tree_view_append_column (self->ctxinfo_treeview, column);
}

static void
ctxinfo_selection_foreach_get_id (GtkTreeModel *model, GtkTreePath *path,
                                  GtkTreeIter *iter, gpointer data)
{
  GList **selected = (GList **)data;
  gchar *id = NULL;

  TKM_UNUSED (path);

  gtk_tree_model_get (model, iter, COLUMN_CTXINFO_ID, &id, -1);
  *selected = g_list_append (*selected, id);
}

static void
ctxinfo_selection_foreach_get_name (GtkTreeModel *model, GtkTreePath *path,
                                    GtkTreeIter *iter, gpointer data)
{
  GList **selected = (GList **)data;
  gchar *name = NULL;

  TKM_UNUSED (path);

  gtk_tree_model_get (model, iter, COLUMN_CTXINFO_NAME, &name, -1);
  *selected = g_list_append (*selected, name);
}

static void
ctxinfo_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  TkmvProcessesView *self = (TkmvProcessesView *)data;
  GList *selected_ids = NULL;
  GList *selected_names = NULL;
  guint selected_count = 0;

  selected_count
    = gtk_tree_selection_count_selected_rows (self->ctxinfo_treeview_select);
  gtk_tree_selection_selected_foreach (
    selection, ctxinfo_selection_foreach_get_id, &selected_ids);
  gtk_tree_selection_selected_foreach (
    selection, ctxinfo_selection_foreach_get_name, &selected_names);

  if (selected_count > 0)
    {
      g_autofree gchar *name = g_strdup_printf (
        "%s[%s]", (gchar *)g_list_nth (selected_names, 0)->data,
        (gchar *)g_list_nth (selected_ids, 0)->data);
      gtk_label_set_text (self->ctxinfo_cpu_entry1_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_cpu_entry1_label),
                              TRUE);
      gtk_label_set_text (self->ctxinfo_mem_entry1_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_mem_entry1_label),
                              TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_cpu_entry1_label),
                              FALSE);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_mem_entry1_label),
                              FALSE);
    }

  if (selected_count > 1)
    {
      g_autofree gchar *name = g_strdup_printf (
        "%s[%s]", (gchar *)g_list_nth (selected_names, 1)->data,
        (gchar *)g_list_nth (selected_ids, 1)->data);
      gtk_label_set_text (self->ctxinfo_cpu_entry2_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_cpu_entry2_label),
                              TRUE);
      gtk_label_set_text (self->ctxinfo_mem_entry2_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_mem_entry2_label),
                              TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_cpu_entry2_label),
                              FALSE);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_mem_entry2_label),
                              FALSE);
    }

  if (selected_count > 2)
    {
      g_autofree gchar *name = g_strdup_printf (
        "%s[%s]", (gchar *)g_list_nth (selected_names, 2)->data,
        (gchar *)g_list_nth (selected_ids, 2)->data);
      gtk_label_set_text (self->ctxinfo_cpu_entry3_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_cpu_entry3_label),
                              TRUE);
      gtk_label_set_text (self->ctxinfo_mem_entry3_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_mem_entry3_label),
                              TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_cpu_entry3_label),
                              FALSE);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_mem_entry3_label),
                              FALSE);
    }

  if (selected_count > 3)
    {
      g_autofree gchar *name = g_strdup_printf (
        "%s[%s]", (gchar *)g_list_nth (selected_names, 3)->data,
        (gchar *)g_list_nth (selected_ids, 3)->data);
      gtk_label_set_text (self->ctxinfo_cpu_entry4_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_cpu_entry4_label),
                              TRUE);
      gtk_label_set_text (self->ctxinfo_mem_entry4_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_mem_entry4_label),
                              TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_cpu_entry4_label),
                              FALSE);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_mem_entry4_label),
                              FALSE);
    }

  if (selected_count > 4)
    {
      g_autofree gchar *name = g_strdup_printf (
        "%s[%s]", (gchar *)g_list_nth (selected_names, 4)->data,
        (gchar *)g_list_nth (selected_ids, 4)->data);
      gtk_label_set_text (self->ctxinfo_cpu_entry5_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_cpu_entry5_label),
                              TRUE);
      gtk_label_set_text (self->ctxinfo_mem_entry5_label, name);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_mem_entry5_label),
                              TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_cpu_entry5_label),
                              FALSE);
      gtk_widget_set_visible (GTK_WIDGET (self->ctxinfo_mem_entry5_label),
                              FALSE);
    }

  gtk_widget_queue_draw (GTK_WIDGET (self->ctxinfo_history_cpu_drawing_area));
  gtk_widget_queue_draw (GTK_WIDGET (self->ctxinfo_history_mem_drawing_area));

  g_list_free_full (selected_names, g_free);
}

static void
procacct_add_columns (TkmvProcessesView *self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "Name", renderer, "text", COLUMN_PROCACCT_NAME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column pid */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "PID", renderer, "text", COLUMN_PROCACCT_PID, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column ppid */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "PPID", renderer, "text", COLUMN_PROCACCT_PPID, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column uid */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "UID", renderer, "text", COLUMN_PROCACCT_UID, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column gid */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "GID", renderer, "text", COLUMN_PROCACCT_GID, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column utime */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "UTime", renderer, "text", COLUMN_PROCACCT_UTIME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column stime */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "STime", renderer, "text", COLUMN_PROCACCT_STIME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column cpucount */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "CPUCnt", renderer, "text", COLUMN_PROCACCT_CPU_COUNT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column cpurunreal */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "CPURunReal", renderer, "text", COLUMN_PROCACCT_CPU_RUN_REAL, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column cpurunvirt */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "CPURunVirt", renderer, "text", COLUMN_PROCACCT_CPU_RUN_VIRTUAL, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column cpudelaytotal */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "CPUDelayTotal", renderer, "text", COLUMN_PROCACCT_CPU_DELAY_TOTAL,
    NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column cpudelayavg */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "CPUDelayAvg", renderer, "text", COLUMN_PROCACCT_CPU_DELAY_AVG, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column coremem */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "CoreMem", renderer, "text", COLUMN_PROCACCT_CORE_MEM, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column virtmem */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "VirtMem", renderer, "text", COLUMN_PROCACCT_VIRT_MEM, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column hiwaterrss */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "HiWaterRSS", renderer, "text", COLUMN_PROCACCT_HIGH_WATER_RSS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column hiwatervm */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "HiWaterVM", renderer, "text", COLUMN_PROCACCT_HIGH_WATER_VM, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column nvcsw */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "NVCSW", renderer, "text", COLUMN_PROCACCT_NVCSW, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column nivcsw */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "NIVCSW", renderer, "text", COLUMN_PROCACCT_NIVCSW, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column swapincount */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "SwapinCount", renderer, "text", COLUMN_PROCACCT_SWAPIN_COUNT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column swapindelaytotal */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "SwapinDelayTotal", renderer, "text", COLUMN_PROCACCT_SWAPIN_DELAY_TOTAL,
    NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column swapindelayavg */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "SwapinDelayAvg", renderer, "text", COLUMN_PROCACCT_SWAPIN_DELAY_AVG,
    NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column blkiocount */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "BlkIoCount", renderer, "text", COLUMN_PROCACCT_BLKIO_COUNT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column blkiodelaytotal */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "BlkIoDelayTotal", renderer, "text", COLUMN_PROCACCT_BLKIO_DELAY_TOTAL,
    NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column blkiodelayavg */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "BlkIoDelayAvg", renderer, "text", COLUMN_PROCACCT_BLKIO_DELAY_AVG,
    NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column iostorageread */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "IOStorageRead", renderer, "text", COLUMN_PROCACCT_IO_STORAGE_READ,
    NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column iostoragewrite */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "IOStorageWrite", renderer, "text", COLUMN_PROCACCT_IO_STORAGE_WRITE,
    NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column ioreadchr */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "IOReadChr", renderer, "text", COLUMN_PROCACCT_IO_READ_CHAR, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column iowritechr */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "IOWriteChr", renderer, "text", COLUMN_PROCACCT_IO_WRITE_CHAR, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column ioreadsyscall */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "IOReadSyscall", renderer, "text", COLUMN_PROCACCT_IO_READ_SYSCALLS,
    NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column iowritesyscall */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "IOWriteSyscall", renderer, "text", COLUMN_PROCACCT_IO_WRITE_SYSCALLS,
    NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column freepagecount */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "FreePageCount", renderer, "text", COLUMN_PROCACCT_FREEPAGE_COUNT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column freepagedelaytotal */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "FreePageDelayTotal", renderer, "text",
    COLUMN_PROCACCT_FREEPAGE_DELAY_TOTAL, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column freepagedelayavg */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "FreePageDelayAvg", renderer, "text", COLUMN_PROCACCT_FREEPAGE_DELAY_AVG,
    NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column trashingcount */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "TrashingCount", renderer, "text", COLUMN_PROCACCT_TRASHING_COUNT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column trashingdelaytotal */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "TrashingDelayTotal", renderer, "text",
    COLUMN_PROCACCT_TRASHING_DELAY_TOTAL, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);

  /* column trashingdelayavg */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "TrashingDelayAvg", renderer, "text", COLUMN_PROCACCT_TRASHING_DELAY_AVG,
    NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procacct_treeview, column);
}

static void
procacct_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  TKMV_UNUSED (selection);
  TKMV_UNUSED (data);
}

int
procinfo_sort_iter_compare_func (GtkTreeModel *model, GtkTreeIter *a,
                                 GtkTreeIter *b, gpointer userdata)
{
  int sortcol = GPOINTER_TO_INT (userdata);
  int ret = 0; /* default if egual */

  switch (sortcol)
    {
    case COLUMN_PROCINFO_NAME:
    {
      char *name1, *name2;

      gtk_tree_model_get (model, a, COLUMN_PROCINFO_NAME, &name1, -1);
      gtk_tree_model_get (model, b, COLUMN_PROCINFO_NAME, &name2, -1);

      if (name1 == NULL || name2 == NULL)
        {
          if (name1 == NULL && name2 == NULL)
            break;   /* both equal => ret = 0 */

          ret = (name1 == NULL) ? -1 : 1;
        }
      else
        {
          ret = g_utf8_collate (name1, name2);
        }

      g_free (name1);
      g_free (name2);
    }
    break;

    case COLUMN_PROCINFO_CONTEXT:
    {
      char *name1, *name2;

      gtk_tree_model_get (model, a, COLUMN_PROCINFO_CONTEXT, &name1, -1);
      gtk_tree_model_get (model, b, COLUMN_PROCINFO_CONTEXT, &name2, -1);

      if (name1 == NULL || name2 == NULL)
        {
          if (name1 == NULL && name2 == NULL)
            break;   /* both equal => ret = 0 */

          ret = (name1 == NULL) ? -1 : 1;
        }
      else
        {
          ret = g_utf8_collate (name1, name2);
        }

      g_free (name1);
      g_free (name2);
    }
    break;

    case COLUMN_PROCINFO_PID:
    {
      guint pid1, pid2;

      gtk_tree_model_get (model, a, COLUMN_PROCINFO_PID, &pid1, -1);
      gtk_tree_model_get (model, b, COLUMN_PROCINFO_PID, &pid2, -1);

      if (pid1 != pid2)
        {
          ret = (pid1 > pid2) ? 1 : -1;
        }
    }
    break;

    case COLUMN_PROCINFO_PPID:
    {
      guint ppid1, ppid2;

      gtk_tree_model_get (model, a, COLUMN_PROCINFO_PPID, &ppid1, -1);
      gtk_tree_model_get (model, b, COLUMN_PROCINFO_PPID, &ppid2, -1);

      if (ppid1 != ppid2)
        {
          ret = (ppid1 > ppid2) ? 1 : -1;
        }
    }
    break;

    case COLUMN_PROCINFO_CPU_TIME:
    {
      guint val1, val2;

      gtk_tree_model_get (model, a, COLUMN_PROCINFO_CPU_TIME, &val1, -1);
      gtk_tree_model_get (model, b, COLUMN_PROCINFO_CPU_TIME, &val2, -1);

      if (val1 != val2)
        {
          ret = (val1 > val2) ? 1 : -1;
        }
    }
    break;

    case COLUMN_PROCINFO_CPU_PERCENT:
    {
      guint val1, val2;

      gtk_tree_model_get (model, a, COLUMN_PROCINFO_CPU_PERCENT, &val1, -1);
      gtk_tree_model_get (model, b, COLUMN_PROCINFO_CPU_PERCENT, &val2, -1);

      if (val1 != val2)
        {
          ret = (val1 > val2) ? 1 : -1;
        }
    }
    break;

    case COLUMN_PROCINFO_MEM_RSS:
    {
      guint val1, val2;

      gtk_tree_model_get (model, a, COLUMN_PROCINFO_MEM_RSS, &val1, -1);
      gtk_tree_model_get (model, b, COLUMN_PROCINFO_MEM_RSS, &val2, -1);

      if (val1 != val2)
        {
          ret = (val1 > val2) ? 1 : -1;
        }
    }
    break;

    case COLUMN_PROCINFO_MEM_PSS:
    {
      guint val1, val2;

      gtk_tree_model_get (model, a, COLUMN_PROCINFO_MEM_PSS, &val1, -1);
      gtk_tree_model_get (model, b, COLUMN_PROCINFO_MEM_PSS, &val2, -1);

      if (val1 != val2)
        {
          ret = (val1 > val2) ? 1 : -1;
        }
    }
    break;

    default:
      g_return_val_if_reached (0);
    }

  return ret;
}

int
ctxinfo_sort_iter_compare_func (GtkTreeModel *model, GtkTreeIter *a,
                                GtkTreeIter *b, gpointer userdata)
{
  int sortcol = GPOINTER_TO_INT (userdata);
  int ret = 0; /* default if egual */

  switch (sortcol)
    {
    case COLUMN_CTXINFO_NAME:
    {
      char *name1, *name2;

      gtk_tree_model_get (model, a, COLUMN_CTXINFO_NAME, &name1, -1);
      gtk_tree_model_get (model, b, COLUMN_CTXINFO_NAME, &name2, -1);

      if (name1 == NULL || name2 == NULL)
        {
          if (name1 == NULL && name2 == NULL)
            break;   /* both equal => ret = 0 */

          ret = (name1 == NULL) ? -1 : 1;
        }
      else
        {
          ret = g_utf8_collate (name1, name2);
        }

      g_free (name1);
      g_free (name2);
    }
    break;

    case COLUMN_CTXINFO_ID:
    {
      char *name1, *name2;

      gtk_tree_model_get (model, a, COLUMN_CTXINFO_ID, &name1, -1);
      gtk_tree_model_get (model, b, COLUMN_CTXINFO_ID, &name2, -1);

      if (name1 == NULL || name2 == NULL)
        {
          if (name1 == NULL && name2 == NULL)
            break;   /* both equal => ret = 0 */

          ret = (name1 == NULL) ? -1 : 1;
        }
      else
        {
          ret = g_utf8_collate (name1, name2);
        }

      g_free (name1);
      g_free (name2);
    }
    break;

    case COLUMN_CTXINFO_CPU_TIME:
    {
      guint val1, val2;

      gtk_tree_model_get (model, a, COLUMN_CTXINFO_CPU_TIME, &val1, -1);
      gtk_tree_model_get (model, b, COLUMN_CTXINFO_CPU_TIME, &val2, -1);

      if (val1 != val2)
        {
          ret = (val1 > val2) ? 1 : -1;
        }
    }
    break;

    case COLUMN_CTXINFO_CPU_PERCENT:
    {
      guint val1, val2;

      gtk_tree_model_get (model, a, COLUMN_CTXINFO_CPU_PERCENT, &val1, -1);
      gtk_tree_model_get (model, b, COLUMN_CTXINFO_CPU_PERCENT, &val2, -1);

      if (val1 != val2)
        {
          ret = (val1 > val2) ? 1 : -1;
        }
    }
    break;

    case COLUMN_CTXINFO_MEM_RSS:
    {
      guint val1, val2;

      gtk_tree_model_get (model, a, COLUMN_CTXINFO_MEM_RSS, &val1, -1);
      gtk_tree_model_get (model, b, COLUMN_CTXINFO_MEM_RSS, &val2, -1);

      if (val1 != val2)
        {
          ret = (val1 > val2) ? 1 : -1;
        }
    }
    break;

    case COLUMN_CTXINFO_MEM_PSS:
    {
      guint val1, val2;

      gtk_tree_model_get (model, a, COLUMN_CTXINFO_MEM_PSS, &val1, -1);
      gtk_tree_model_get (model, b, COLUMN_CTXINFO_MEM_PSS, &val2, -1);

      if (val1 != val2)
        {
          ret = (val1 > val2) ? 1 : -1;
        }
    }
    break;

    default:
      g_return_val_if_reached (0);
    }

  return ret;
}

static void
create_tables (TkmvProcessesView *self)
{
  /* create procinfo store */
  self->procinfo_store = gtk_list_store_new (
    PROCINFO_NUM_COLUMNS, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT,
    G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);
  gtk_tree_view_set_model (self->procinfo_treeview,
                           GTK_TREE_MODEL (self->procinfo_store));
  procinfo_add_columns (self);

  /* register selection handler */
  self->procinfo_treeview_select
    = gtk_tree_view_get_selection (self->procinfo_treeview);
  gtk_tree_selection_set_mode (self->procinfo_treeview_select,
                               GTK_SELECTION_MULTIPLE);
  gtk_tree_selection_set_select_function (
    self->procinfo_treeview_select, limit_selection_function, NULL, NULL);

  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->procinfo_store), COLUMN_PROCINFO_NAME,
    procinfo_sort_iter_compare_func, GINT_TO_POINTER (COLUMN_PROCINFO_NAME),
    NULL);
  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->procinfo_store), COLUMN_PROCINFO_PID,
    procinfo_sort_iter_compare_func, GINT_TO_POINTER (COLUMN_PROCINFO_PID),
    NULL);
  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->procinfo_store), COLUMN_PROCINFO_PPID,
    procinfo_sort_iter_compare_func, GINT_TO_POINTER (COLUMN_PROCINFO_PPID),
    NULL);
  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->procinfo_store), COLUMN_PROCINFO_CONTEXT,
    procinfo_sort_iter_compare_func,
    GINT_TO_POINTER (COLUMN_PROCINFO_CONTEXT), NULL);
  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->procinfo_store), COLUMN_PROCINFO_CPU_TIME,
    procinfo_sort_iter_compare_func,
    GINT_TO_POINTER (COLUMN_PROCINFO_CPU_TIME), NULL);
  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->procinfo_store), COLUMN_PROCINFO_CPU_PERCENT,
    procinfo_sort_iter_compare_func,
    GINT_TO_POINTER (COLUMN_PROCINFO_CPU_PERCENT), NULL);
  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->procinfo_store), COLUMN_PROCINFO_MEM_RSS,
    procinfo_sort_iter_compare_func, GINT_TO_POINTER (COLUMN_PROCINFO_MEM_RSS),
    NULL);
  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->procinfo_store), COLUMN_PROCINFO_MEM_PSS,
    procinfo_sort_iter_compare_func, GINT_TO_POINTER (COLUMN_PROCINFO_MEM_PSS),
    NULL);
  gtk_tree_sortable_set_sort_column_id (
    GTK_TREE_SORTABLE (self->procinfo_store), COLUMN_PROCINFO_CPU_PERCENT,
    GTK_SORT_DESCENDING);

  g_signal_connect (G_OBJECT (self->procinfo_treeview_select), "changed",
                    G_CALLBACK (procinfo_selection_changed), self);

  /* create ctxinfo store */
  self->ctxinfo_store
    = gtk_list_store_new (CTXINFO_NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
                          G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);
  gtk_tree_view_set_model (self->ctxinfo_treeview,
                           GTK_TREE_MODEL (self->ctxinfo_store));
  ctxinfo_add_columns (self);

  /* register selection handler */
  self->ctxinfo_treeview_select
    = gtk_tree_view_get_selection (self->ctxinfo_treeview);
  gtk_tree_selection_set_mode (self->ctxinfo_treeview_select,
                               GTK_SELECTION_MULTIPLE);
  gtk_tree_selection_set_select_function (
    self->ctxinfo_treeview_select, limit_selection_function, NULL, NULL);

  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->ctxinfo_store), COLUMN_CTXINFO_NAME,
    ctxinfo_sort_iter_compare_func, GINT_TO_POINTER (COLUMN_CTXINFO_NAME),
    NULL);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (self->ctxinfo_store),
                                   COLUMN_CTXINFO_ID,
                                   ctxinfo_sort_iter_compare_func,
                                   GINT_TO_POINTER (COLUMN_CTXINFO_ID), NULL);
  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->ctxinfo_store), COLUMN_CTXINFO_CPU_TIME,
    ctxinfo_sort_iter_compare_func,
    GINT_TO_POINTER (COLUMN_CTXINFO_CPU_TIME), NULL);
  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->ctxinfo_store), COLUMN_CTXINFO_CPU_PERCENT,
    ctxinfo_sort_iter_compare_func,
    GINT_TO_POINTER (COLUMN_CTXINFO_CPU_PERCENT), NULL);
  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->ctxinfo_store), COLUMN_CTXINFO_MEM_RSS,
    ctxinfo_sort_iter_compare_func, GINT_TO_POINTER (COLUMN_CTXINFO_MEM_RSS),
    NULL);
  gtk_tree_sortable_set_sort_func (
    GTK_TREE_SORTABLE (self->ctxinfo_store), COLUMN_CTXINFO_MEM_PSS,
    ctxinfo_sort_iter_compare_func, GINT_TO_POINTER (COLUMN_CTXINFO_MEM_PSS),
    NULL);
  gtk_tree_sortable_set_sort_column_id (
    GTK_TREE_SORTABLE (self->ctxinfo_store), COLUMN_CTXINFO_CPU_PERCENT,
    GTK_SORT_DESCENDING);

  g_signal_connect (G_OBJECT (self->ctxinfo_treeview_select), "changed",
                    G_CALLBACK (ctxinfo_selection_changed), self);

  /* create procacct store */
  self->procacct_store = gtk_list_store_new (
    PROCACCT_NUM_COLUMNS, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT,
    G_TYPE_UINT, G_TYPE_UINT, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG,
    G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG,
    G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG,
    G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG,
    G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG,
    G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG,
    G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG);
  gtk_tree_view_set_model (self->procacct_treeview,
                           GTK_TREE_MODEL (self->procacct_store));
  procacct_add_columns (self);

  /* register selection handler */
  self->procacct_treeview_select
    = gtk_tree_view_get_selection (self->procacct_treeview);
  gtk_tree_selection_set_mode (self->procacct_treeview_select,
                               GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (self->procacct_treeview_select), "changed",
                    G_CALLBACK (procacct_selection_changed), self);
}

static void
procinfo_list_store_append_entry (GtkListStore *list_store,
                                  TkmProcInfoEntry *entry, GtkTreeIter *iter)
{
  gtk_list_store_append (list_store, iter);
  gtk_list_store_set (
    list_store, iter, COLUMN_PROCINFO_NAME,
    tkm_procinfo_entry_get_name (entry), COLUMN_PROCINFO_PID,
    tkm_procinfo_entry_get_data (entry, PINFO_DATA_PID),
    COLUMN_PROCINFO_PPID,
    tkm_procinfo_entry_get_data (entry, PINFO_DATA_PPID),
    COLUMN_PROCINFO_CONTEXT, tkm_procinfo_entry_get_context (entry),
    COLUMN_PROCINFO_CPU_TIME,
    tkm_procinfo_entry_get_data (entry, PINFO_DATA_CPU_TIME),
    COLUMN_PROCINFO_CPU_PERCENT,
    tkm_procinfo_entry_get_data (entry, PINFO_DATA_CPU_PERCENT),
    COLUMN_PROCINFO_MEM_RSS,
    tkm_procinfo_entry_get_data (entry, PINFO_DATA_MEM_RSS),
    COLUMN_PROCINFO_MEM_PSS,
    tkm_procinfo_entry_get_data (entry, PINFO_DATA_MEM_PSS), -1);
}

static void
reload_procinfo_entries (TkmvProcessesView *view, TkmContext *context)
{
  GPtrArray *entries = tkm_context_get_procinfo_entries (context);
  GtkTreeIter iter;

  if (entries == NULL)
    return;

  if (entries->len == 0)
    return;

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->procinfo_treeview), NULL);
  gtk_list_store_clear (view->procinfo_store);

  /* get first entry */
  TkmProcInfoEntry *firstEntry = g_ptr_array_index (entries, 0);

  tkm_procinfo_entry_set_index (firstEntry, 0);
  procinfo_list_store_append_entry (view->procinfo_store, firstEntry, &iter);

  for (guint i = 1; i < entries->len; i++)
    {
      TkmProcInfoEntry *entry = g_ptr_array_index (entries, i);

      tkm_procinfo_entry_set_index (entry, i);
      if (tkm_procinfo_entry_get_timestamp (entry, DATA_TIME_SOURCE_MONOTONIC)
          == tkm_procinfo_entry_get_timestamp (firstEntry,
                                               DATA_TIME_SOURCE_MONOTONIC))
        {
          procinfo_list_store_append_entry (view->procinfo_store, entry,
                                            &iter);
        }
      else
        {
          break;
        }
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->procinfo_treeview),
                           GTK_TREE_MODEL (view->procinfo_store));
}

static void
ctxinfo_list_store_append_entry (GtkListStore *list_store,
                                 TkmCtxInfoEntry *entry, GtkTreeIter *iter)
{
  gtk_list_store_append (list_store, iter);
  gtk_list_store_set (
    list_store, iter, COLUMN_CTXINFO_NAME,
    tkm_ctxinfo_entry_get_name (entry), COLUMN_CTXINFO_ID,
    tkm_ctxinfo_entry_get_id (entry), COLUMN_CTXINFO_CPU_TIME,
    tkm_ctxinfo_entry_get_data (entry, CTXINFO_DATA_CPU_TIME),
    COLUMN_CTXINFO_CPU_PERCENT,
    tkm_ctxinfo_entry_get_data (entry, CTXINFO_DATA_CPU_PERCENT),
    COLUMN_CTXINFO_MEM_RSS,
    tkm_ctxinfo_entry_get_data (entry, CTXINFO_DATA_MEM_RSS),
    COLUMN_CTXINFO_MEM_PSS,
    tkm_ctxinfo_entry_get_data (entry, CTXINFO_DATA_MEM_PSS), -1);
}

static void
reload_ctxinfo_entries (TkmvProcessesView *view, TkmContext *context)
{
  GPtrArray *entries = tkm_context_get_ctxinfo_entries (context);
  GtkTreeIter iter;

  if (entries == NULL)
    return;

  if (entries->len == 0)
    return;

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->ctxinfo_treeview), NULL);
  gtk_list_store_clear (view->ctxinfo_store);

  /* get first entry */
  TkmCtxInfoEntry *firstEntry = g_ptr_array_index (entries, 0);

  tkm_ctxinfo_entry_set_index (firstEntry, 0);
  ctxinfo_list_store_append_entry (view->ctxinfo_store, firstEntry, &iter);

  for (guint i = 1; i < entries->len; i++)
    {
      TkmCtxInfoEntry *entry = g_ptr_array_index (entries, i);

      tkm_ctxinfo_entry_set_index (entry, i);
      if (tkm_ctxinfo_entry_get_timestamp (entry, DATA_TIME_SOURCE_MONOTONIC)
          == tkm_ctxinfo_entry_get_timestamp (firstEntry,
                                              DATA_TIME_SOURCE_MONOTONIC))
        {
          ctxinfo_list_store_append_entry (view->ctxinfo_store, entry, &iter);
        }
      else
        {
          break;
        }
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->ctxinfo_treeview),
                           GTK_TREE_MODEL (view->ctxinfo_store));
}

static void
procacct_list_store_append_entry (GtkListStore *list_store,
                                  TkmProcAcctEntry *entry, GtkTreeIter *iter)
{
  gtk_list_store_append (list_store, iter);
  gtk_list_store_set (
    list_store, iter, COLUMN_PROCACCT_NAME,
    tkm_procacct_entry_get_name (entry), COLUMN_PROCACCT_PID,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_PID),
    COLUMN_PROCACCT_PPID,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_PPID),
    COLUMN_PROCACCT_UID, tkm_procacct_entry_get_data (entry, PACCT_DATA_UID),
    COLUMN_PROCACCT_GID, tkm_procacct_entry_get_data (entry, PACCT_DATA_GID),
    COLUMN_PROCACCT_UTIME,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_UTIME),
    COLUMN_PROCACCT_STIME,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_STIME),
    COLUMN_PROCACCT_CPU_COUNT,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_CPU_COUNT),
    COLUMN_PROCACCT_CPU_RUN_REAL,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_CPU_RUN_REAL),
    COLUMN_PROCACCT_CPU_RUN_VIRTUAL,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_CPU_RUN_VIRTUAL),
    COLUMN_PROCACCT_CPU_DELAY_TOTAL,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_CPU_DELAY_TOTAL),
    COLUMN_PROCACCT_CPU_DELAY_AVG,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_CPU_DELAY_AVG),
    COLUMN_PROCACCT_CORE_MEM,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_CORE_MEM),
    COLUMN_PROCACCT_VIRT_MEM,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_VIRT_MEM),
    COLUMN_PROCACCT_HIGH_WATER_RSS,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_HIGH_WATER_RSS),
    COLUMN_PROCACCT_NVCSW,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_NVCSW),
    COLUMN_PROCACCT_NIVCSW,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_NIVCSW),
    COLUMN_PROCACCT_SWAPIN_COUNT,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_SWAPIN_COUNT),
    COLUMN_PROCACCT_SWAPIN_DELAY_TOTAL,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_SWAPIN_DELAY_TOTAL),
    COLUMN_PROCACCT_SWAPIN_DELAY_AVG,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_SWAPIN_DELAY_AVG),
    COLUMN_PROCACCT_BLKIO_COUNT,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_BLKIO_COUNT),
    COLUMN_PROCACCT_BLKIO_DELAY_TOTAL,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_BLKIO_DELAY_TOTAL),
    COLUMN_PROCACCT_BLKIO_DELAY_AVG,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_BLKIO_DELAY_AVG),
    COLUMN_PROCACCT_IO_STORAGE_READ,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_IO_STORAGE_READ),
    COLUMN_PROCACCT_IO_STORAGE_WRITE,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_IO_STORAGE_WRITE),
    COLUMN_PROCACCT_IO_READ_CHAR,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_IO_READ_CHAR),
    COLUMN_PROCACCT_IO_WRITE_CHAR,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_IO_WRITE_CHAR),
    COLUMN_PROCACCT_IO_READ_SYSCALLS,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_IO_READ_SYSCALLS),
    COLUMN_PROCACCT_IO_WRITE_SYSCALLS,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_IO_WRITE_SYSCALLS),
    COLUMN_PROCACCT_FREEPAGE_COUNT,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_FREEPAGE_COUNT),
    COLUMN_PROCACCT_FREEPAGE_DELAY_TOTAL,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_FREEPAGE_DELAY_TOTAL),
    COLUMN_PROCACCT_FREEPAGE_DELAY_AVG,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_FREEPAGE_DELAY_AVG),
    COLUMN_PROCACCT_TRASHING_COUNT,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_TRASHING_COUNT),
    COLUMN_PROCACCT_TRASHING_DELAY_TOTAL,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_TRASHING_DELAY_TOTAL),
    COLUMN_PROCACCT_TRASHING_DELAY_AVG,
    tkm_procacct_entry_get_data (entry, PACCT_DATA_TRASHING_DELAY_AVG), -1);
}

static void
reload_procacct_entries (TkmvProcessesView *view, TkmContext *context)
{
  GPtrArray *entries = tkm_context_get_procacct_entries (context);
  GtkTreeIter iter;

  if (entries == NULL)
    {
      gtk_widget_set_visible (GTK_WIDGET (view->procacct_scrolled_window), FALSE);
      return;
    }

  if (entries->len == 0)
    {
      gtk_widget_set_visible (GTK_WIDGET (view->procacct_scrolled_window), FALSE);
      return;
    }

  gtk_widget_set_visible (GTK_WIDGET (view->procacct_scrolled_window), TRUE);

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->procacct_treeview), NULL);
  gtk_list_store_clear (view->procacct_store);

  /* get first entry */
  TkmProcAcctEntry *firstEntry = g_ptr_array_index (entries, 0);

  tkm_procacct_entry_set_index (firstEntry, 0);
  procacct_list_store_append_entry (view->procacct_store, firstEntry, &iter);

  for (guint i = 1; i < entries->len; i++)
    {
      TkmProcAcctEntry *entry = g_ptr_array_index (entries, i);

      tkm_procacct_entry_set_index (entry, i);

      /* The procacct entry come when ready so we should group the entries with */
      /* some delay in mind. For now we use a 3 seconds delay since */
      /* slowLaneInterval is normally much higher (eg 10s) */
      if (tkm_procacct_entry_get_timestamp (entry, DATA_TIME_SOURCE_MONOTONIC)
          <= tkm_procacct_entry_get_timestamp (firstEntry,
                                               DATA_TIME_SOURCE_MONOTONIC)
          + 3)
        {
          procacct_list_store_append_entry (view->procacct_store, entry,
                                            &iter);
        }
      else
        {
          break;
        }
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->procacct_treeview),
                           GTK_TREE_MODEL (view->procacct_store));
}

static void
timestamp_format_procview (double val, char *buf, size_t sz)
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
memory_format_procview (double val, char *buf, size_t sz)
{
  g_assert (buf);
  snprintf (buf, sz, "%u KB", (guint)val);
}

static void
percent_format_procview (double val, char *buf, size_t sz)
{
  g_assert (buf);
  snprintf (buf, sz, "%u %%", (guint)val);
}

static void
procinfo_cpu_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                    int width, int height, gpointer data)
{
  TkmContext *context
    = tkmv_application_get_context (tkmv_application_instance ());
  TkmvSettings *settings
    = tkmv_application_get_settings (tkmv_application_instance ());
  TkmvProcessesView *self = (TkmvProcessesView *)data;
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  GPtrArray *proc_info_data = tkm_context_get_procinfo_entries (context);
  TkmSessionEntry *active_session = NULL;

  struct kdata *d1 = NULL;
  struct kdata *d2 = NULL;
  struct kdata *d3 = NULL;
  struct kdata *d4 = NULL;
  struct kdata *d5 = NULL;

  struct kplotcfg plotcfg;
  struct kplot *p = NULL;

  guint selected_count = 0;
  GList *selected_pids = NULL;

  TKMV_UNUSED (area);

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

  selected_count = gtk_tree_selection_count_selected_rows (
    self->procinfo_treeview_select);
  gtk_tree_selection_selected_foreach (self->procinfo_treeview_select,
                                       procinfo_selection_foreach_get_pid,
                                       &selected_pids);

  if (proc_info_data != NULL)
    {
      g_autofree guint *entry_index_set = calloc (proc_info_data->len, sizeof(guint));
      guint entry_count = 0;

      if (proc_info_data->len > 0)
        {
          if (selected_count > 0)
            {
              for (guint i = 0; i < proc_info_data->len; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, i);

                  if (tkm_procinfo_entry_get_data (entry, PINFO_DATA_PID)
                      == GPOINTER_TO_INT (g_list_nth (selected_pids, 0)->data))
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d1 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_procinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d1->pairs[i].x = timestamp;
                  d1->pairs[i].y = tkm_procinfo_entry_get_data (
                    entry, PINFO_DATA_CPU_PERCENT);
                }
            }

          if (selected_count > 1)
            {
              entry_count = 0;
              for (guint i = 0; i < proc_info_data->len; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, i);

                  if (tkm_procinfo_entry_get_data (entry, PINFO_DATA_PID)
                      == GPOINTER_TO_INT (g_list_nth (selected_pids, 1)->data))
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d2 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_procinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d2->pairs[i].x = timestamp;
                  d2->pairs[i].y = tkm_procinfo_entry_get_data (
                    entry, PINFO_DATA_CPU_PERCENT);
                }
            }

          if (selected_count > 2)
            {
              entry_count = 0;
              for (guint i = 0; i < proc_info_data->len; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, i);

                  if (tkm_procinfo_entry_get_data (entry, PINFO_DATA_PID)
                      == GPOINTER_TO_INT (g_list_nth (selected_pids, 2)->data))
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d3 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_procinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d3->pairs[i].x = timestamp;
                  d3->pairs[i].y = tkm_procinfo_entry_get_data (
                    entry, PINFO_DATA_CPU_PERCENT);
                }
            }

          if (selected_count > 3)
            {
              entry_count = 0;
              for (guint i = 0; i < proc_info_data->len; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, i);

                  if (tkm_procinfo_entry_get_data (entry, PINFO_DATA_PID)
                      == GPOINTER_TO_INT (g_list_nth (selected_pids, 3)->data))
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d4 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_procinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d4->pairs[i].x = timestamp;
                  d4->pairs[i].y = tkm_procinfo_entry_get_data (
                    entry, PINFO_DATA_CPU_PERCENT);
                }
            }

          if (selected_count > 4)
            {
              entry_count = 0;
              for (guint i = 0; i < proc_info_data->len; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, i);

                  if (tkm_procinfo_entry_get_data (entry, PINFO_DATA_PID)
                      == GPOINTER_TO_INT (g_list_nth (selected_pids, 4)->data))
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d5 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_procinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d5->pairs[i].x = timestamp;
                  d5->pairs[i].y = tkm_procinfo_entry_get_data (
                    entry, PINFO_DATA_CPU_PERCENT);
                }
            }
        }
    }

  g_list_free (selected_pids);

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
  plotcfg.xticlabelfmt = timestamp_format_procview;
  plotcfg.yticlabelfmt = percent_format_procview;

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
procinfo_mem_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                    int width, int height, gpointer data)
{
  TkmContext *context
    = tkmv_application_get_context (tkmv_application_instance ());
  TkmvSettings *settings
    = tkmv_application_get_settings (tkmv_application_instance ());
  TkmvProcessesView *self = (TkmvProcessesView *)data;
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  GPtrArray *proc_info_data = tkm_context_get_procinfo_entries (context);
  TkmSessionEntry *active_session = NULL;

  struct kdata *d1 = NULL;
  struct kdata *d2 = NULL;
  struct kdata *d3 = NULL;
  struct kdata *d4 = NULL;
  struct kdata *d5 = NULL;

  struct kplotcfg plotcfg;
  struct kplot *p = NULL;

  guint selected_count = 0;
  GList *selected_pids = NULL;

  TKMV_UNUSED (area);

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

  selected_count = gtk_tree_selection_count_selected_rows (
    self->procinfo_treeview_select);
  gtk_tree_selection_selected_foreach (self->procinfo_treeview_select,
                                       procinfo_selection_foreach_get_pid,
                                       &selected_pids);

  if (proc_info_data != NULL)
    {
      g_autofree guint *entry_index_set = calloc (proc_info_data->len, sizeof(guint));
      guint entry_count = 0;

      if (proc_info_data->len > 0)
        {
          if (selected_count > 0)
            {
              for (guint i = 0; i < proc_info_data->len; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, i);

                  if (tkm_procinfo_entry_get_data (entry, PINFO_DATA_PID)
                      == GPOINTER_TO_INT (g_list_nth (selected_pids, 0)->data))
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d1 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_procinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d1->pairs[i].x = timestamp;
                  d1->pairs[i].y = tkm_procinfo_entry_get_data (
                    entry, PINFO_DATA_MEM_RSS);
                }
            }

          if (selected_count > 1)
            {
              entry_count = 0;
              for (guint i = 0; i < proc_info_data->len; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, i);

                  if (tkm_procinfo_entry_get_data (entry, PINFO_DATA_PID)
                      == GPOINTER_TO_INT (g_list_nth (selected_pids, 1)->data))
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d2 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_procinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d2->pairs[i].x = timestamp;
                  d2->pairs[i].y = tkm_procinfo_entry_get_data (
                    entry, PINFO_DATA_MEM_RSS);
                }
            }

          if (selected_count > 2)
            {
              entry_count = 0;
              for (guint i = 0; i < proc_info_data->len; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, i);

                  if (tkm_procinfo_entry_get_data (entry, PINFO_DATA_PID)
                      == GPOINTER_TO_INT (g_list_nth (selected_pids, 2)->data))
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d3 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_procinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d3->pairs[i].x = timestamp;
                  d3->pairs[i].y = tkm_procinfo_entry_get_data (
                    entry, PINFO_DATA_MEM_RSS);
                }
            }

          if (selected_count > 3)
            {
              entry_count = 0;
              for (guint i = 0; i < proc_info_data->len; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, i);

                  if (tkm_procinfo_entry_get_data (entry, PINFO_DATA_PID)
                      == GPOINTER_TO_INT (g_list_nth (selected_pids, 3)->data))
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d4 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_procinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d4->pairs[i].x = timestamp;
                  d4->pairs[i].y = tkm_procinfo_entry_get_data (
                    entry, PINFO_DATA_MEM_RSS);
                }
            }

          if (selected_count > 4)
            {
              entry_count = 0;
              for (guint i = 0; i < proc_info_data->len; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, i);

                  if (tkm_procinfo_entry_get_data (entry, PINFO_DATA_PID)
                      == GPOINTER_TO_INT (g_list_nth (selected_pids, 4)->data))
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d5 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmProcInfoEntry *entry
                    = g_ptr_array_index (proc_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_procinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d5->pairs[i].x = timestamp;
                  d5->pairs[i].y = tkm_procinfo_entry_get_data (
                    entry, PINFO_DATA_MEM_RSS);
                }
            }
        }
    }

  g_list_free (selected_pids);

  kplotcfg_defaults (&plotcfg);
  plotcfg.grid = GRID_ALL;
  plotcfg.extrema = EXTREMA_YMIN;
  plotcfg.extrema_ymin = 0;
  plotcfg.xticlabelfmt = timestamp_format_procview;
  plotcfg.yticlabelfmt = memory_format_procview;

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
ctxinfo_cpu_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                   int width, int height, gpointer data)
{
  TkmContext *context
    = tkmv_application_get_context (tkmv_application_instance ());
  TkmvSettings *settings
    = tkmv_application_get_settings (tkmv_application_instance ());
  TkmvProcessesView *self = (TkmvProcessesView *)data;
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  GPtrArray *ctx_info_data = tkm_context_get_ctxinfo_entries (context);
  TkmSessionEntry *active_session = NULL;

  struct kdata *d1 = NULL;
  struct kdata *d2 = NULL;
  struct kdata *d3 = NULL;
  struct kdata *d4 = NULL;
  struct kdata *d5 = NULL;

  struct kplotcfg plotcfg;
  struct kplot *p = NULL;

  guint selected_count = 0;
  GList *selected_ids = NULL;

  TKMV_UNUSED (area);

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

  selected_count
    = gtk_tree_selection_count_selected_rows (self->ctxinfo_treeview_select);
  gtk_tree_selection_selected_foreach (self->ctxinfo_treeview_select,
                                       ctxinfo_selection_foreach_get_id,
                                       &selected_ids);

  if (ctx_info_data != NULL)
    {
      g_autofree guint *entry_index_set = calloc (ctx_info_data->len, sizeof(guint));
      guint entry_count = 0;

      if (ctx_info_data->len > 0)
        {
          if (selected_count > 0)
            {
              for (guint i = 0; i < ctx_info_data->len; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, i);

                  if (g_strcmp0 (tkm_ctxinfo_entry_get_id (entry),
                                 (gchar *)g_list_nth (selected_ids, 0)->data)
                      == 0)
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d1 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_ctxinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d1->pairs[i].x = timestamp;
                  d1->pairs[i].y = tkm_ctxinfo_entry_get_data (
                    entry, CTXINFO_DATA_CPU_PERCENT);
                }
            }

          if (selected_count > 1)
            {
              entry_count = 0;
              for (guint i = 0; i < ctx_info_data->len; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, i);

                  if (g_strcmp0 (tkm_ctxinfo_entry_get_id (entry),
                                 (gchar *)g_list_nth (selected_ids, 1)->data)
                      == 0)
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d2 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_ctxinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d2->pairs[i].x = timestamp;
                  d2->pairs[i].y = tkm_ctxinfo_entry_get_data (
                    entry, CTXINFO_DATA_CPU_PERCENT);
                }
            }

          if (selected_count > 2)
            {
              entry_count = 0;
              for (guint i = 0; i < ctx_info_data->len; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, i);

                  if (g_strcmp0 (tkm_ctxinfo_entry_get_id (entry),
                                 (gchar *)g_list_nth (selected_ids, 2)->data)
                      == 0)
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d3 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_ctxinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d3->pairs[i].x = timestamp;
                  d3->pairs[i].y = tkm_ctxinfo_entry_get_data (
                    entry, CTXINFO_DATA_CPU_PERCENT);
                }
            }

          if (selected_count > 3)
            {
              entry_count = 0;
              for (guint i = 0; i < ctx_info_data->len; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, i);

                  if (g_strcmp0 (tkm_ctxinfo_entry_get_id (entry),
                                 (gchar *)g_list_nth (selected_ids, 3)->data)
                      == 0)
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d4 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_ctxinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d4->pairs[i].x = timestamp;
                  d4->pairs[i].y = tkm_ctxinfo_entry_get_data (
                    entry, CTXINFO_DATA_CPU_PERCENT);
                }
            }

          if (selected_count > 4)
            {
              entry_count = 0;
              for (guint i = 0; i < ctx_info_data->len; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, i);

                  if (g_strcmp0 (tkm_ctxinfo_entry_get_id (entry),
                                 (gchar *)g_list_nth (selected_ids, 4)->data)
                      == 0)
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d5 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_ctxinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d5->pairs[i].x = timestamp;
                  d5->pairs[i].y = tkm_ctxinfo_entry_get_data (
                    entry, CTXINFO_DATA_CPU_PERCENT);
                }
            }
        }
    }

  g_list_free_full (selected_ids, g_free);

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
  plotcfg.xticlabelfmt = timestamp_format_procview;
  plotcfg.yticlabelfmt = percent_format_procview;

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
ctxinfo_mem_history_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                   int width, int height, gpointer data)
{
  TkmContext *context
    = tkmv_application_get_context (tkmv_application_instance ());
  TkmvSettings *settings
    = tkmv_application_get_settings (tkmv_application_instance ());
  TkmvProcessesView *self = (TkmvProcessesView *)data;
  GPtrArray *sessions = tkm_context_get_session_entries (context);
  GPtrArray *ctx_info_data = tkm_context_get_ctxinfo_entries (context);
  TkmSessionEntry *active_session = NULL;

  struct kdata *d1 = NULL;
  struct kdata *d2 = NULL;
  struct kdata *d3 = NULL;
  struct kdata *d4 = NULL;
  struct kdata *d5 = NULL;

  struct kplotcfg plotcfg;
  struct kplot *p = NULL;

  guint selected_count = 0;
  GList *selected_ids = NULL;

  TKMV_UNUSED (area);

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

  selected_count
    = gtk_tree_selection_count_selected_rows (self->ctxinfo_treeview_select);
  gtk_tree_selection_selected_foreach (self->ctxinfo_treeview_select,
                                       ctxinfo_selection_foreach_get_id,
                                       &selected_ids);

  if (ctx_info_data != NULL)
    {
      g_autofree guint *entry_index_set = calloc (ctx_info_data->len, sizeof(guint));
      guint entry_count = 0;

      if (ctx_info_data->len > 0)
        {
          if (selected_count > 0)
            {
              for (guint i = 0; i < ctx_info_data->len; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, i);

                  if (g_strcmp0 (tkm_ctxinfo_entry_get_id (entry),
                                 (gchar *)g_list_nth (selected_ids, 0)->data)
                      == 0)
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d1 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_ctxinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d1->pairs[i].x = timestamp;
                  d1->pairs[i].y = tkm_ctxinfo_entry_get_data (
                    entry, CTXINFO_DATA_MEM_RSS);
                }
            }

          if (selected_count > 1)
            {
              entry_count = 0;
              for (guint i = 0; i < ctx_info_data->len; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, i);

                  if (g_strcmp0 (tkm_ctxinfo_entry_get_id (entry),
                                 (gchar *)g_list_nth (selected_ids, 1)->data)
                      == 0)
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d2 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_ctxinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d2->pairs[i].x = timestamp;
                  d2->pairs[i].y = tkm_ctxinfo_entry_get_data (
                    entry, CTXINFO_DATA_MEM_RSS);
                }
            }

          if (selected_count > 2)
            {
              entry_count = 0;
              for (guint i = 0; i < ctx_info_data->len; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, i);

                  if (g_strcmp0 (tkm_ctxinfo_entry_get_id (entry),
                                 (gchar *)g_list_nth (selected_ids, 2)->data)
                      == 0)
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d3 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_ctxinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d3->pairs[i].x = timestamp;
                  d3->pairs[i].y = tkm_ctxinfo_entry_get_data (
                    entry, CTXINFO_DATA_MEM_RSS);
                }
            }

          if (selected_count > 3)
            {
              entry_count = 0;
              for (guint i = 0; i < ctx_info_data->len; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, i);

                  if (g_strcmp0 (tkm_ctxinfo_entry_get_id (entry),
                                 (gchar *)g_list_nth (selected_ids, 3)->data)
                      == 0)
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d4 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_ctxinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d4->pairs[i].x = timestamp;
                  d4->pairs[i].y = tkm_ctxinfo_entry_get_data (
                    entry, CTXINFO_DATA_MEM_RSS);
                }
            }

          if (selected_count > 4)
            {
              entry_count = 0;
              for (guint i = 0; i < ctx_info_data->len; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, i);

                  if (g_strcmp0 (tkm_ctxinfo_entry_get_id (entry),
                                 (gchar *)g_list_nth (selected_ids, 4)->data)
                      == 0)
                    {
                      entry_index_set[entry_count++] = i;
                    }
                }

              if (entry_count > 0)
                {
                  d5 = kdata_array_alloc (NULL, entry_count);
                }

              for (guint i = 0; i < entry_count; i++)
                {
                  TkmCtxInfoEntry *entry
                    = g_ptr_array_index (ctx_info_data, entry_index_set[i]);
                  gulong timestamp = tkm_ctxinfo_entry_get_timestamp (
                    entry, tkmv_settings_get_time_source (settings));

                  d5->pairs[i].x = timestamp;
                  d5->pairs[i].y = tkm_ctxinfo_entry_get_data (
                    entry, CTXINFO_DATA_MEM_RSS);
                }
            }
        }
    }

  g_list_free_full (selected_ids, g_free);

  kplotcfg_defaults (&plotcfg);
  plotcfg.grid = GRID_ALL;
  plotcfg.extrema = EXTREMA_YMIN;
  plotcfg.extrema_ymin = 0;
  plotcfg.xticlabelfmt = timestamp_format_procview;
  plotcfg.yticlabelfmt = memory_format_procview;

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
procinfo_cpu_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr,
                                         int width, int height, gpointer data)
{
  TkmContext *context
    = tkmv_application_get_context (tkmv_application_instance ());

  if (tkm_context_data_try_lock (context))
    {
      procinfo_cpu_history_draw_function (area, cr, width, height, data);
      tkm_context_data_unlock (context);
    }
}

static void
procinfo_mem_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr,
                                         int width, int height, gpointer data)
{
  TkmContext *context
    = tkmv_application_get_context (tkmv_application_instance ());

  if (tkm_context_data_try_lock (context))
    {
      procinfo_mem_history_draw_function (area, cr, width, height, data);
      tkm_context_data_unlock (context);
    }
}

static void
ctxinfo_cpu_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr,
                                        int width, int height, gpointer data)
{
  TkmContext *context
    = tkmv_application_get_context (tkmv_application_instance ());

  if (tkm_context_data_try_lock (context))
    {
      ctxinfo_cpu_history_draw_function (area, cr, width, height, data);
      tkm_context_data_unlock (context);
    }
}

static void
ctxinfo_mem_history_draw_function_safe (GtkDrawingArea *area, cairo_t *cr,
                                        int width, int height, gpointer data)
{
  TkmContext *context
    = tkmv_application_get_context (tkmv_application_instance ());

  if (tkm_context_data_try_lock (context))
    {
      ctxinfo_mem_history_draw_function (area, cr, width, height, data);
      tkm_context_data_unlock (context);
    }
}

void
tkmv_processes_reload_entries (TkmvProcessesView *view, TkmContext *context)
{
  guint selected_count = 0;

  reload_procinfo_entries (view, context);
  reload_ctxinfo_entries (view, context);
  reload_procacct_entries (view, context);

  /* select first entry in proc and context tables */
  selected_count = gtk_tree_selection_count_selected_rows (
    view->procinfo_treeview_select);
  if (selected_count == 0)
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (0, -1);
      gtk_tree_selection_select_path (view->procinfo_treeview_select, path);
    }

  selected_count
    = gtk_tree_selection_count_selected_rows (view->ctxinfo_treeview_select);
  if (selected_count == 0)
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (0, -1);
      gtk_tree_selection_select_path (view->ctxinfo_treeview_select, path);
    }
}

