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

enum
{
  COLUMN_PROCINFO_INDEX,
  COLUMN_PROCINFO_NAME,
  COLUMN_PROCINFO_PID,
  COLUMN_PROCINFO_PPID,
  COLUMN_PROCINFO_CONTEXT,
  COLUMN_PROCINFO_CPU_TIME,
  COLUMN_PROCINFO_CPU_PERCENT,
  COLUMN_PROCINFO_VMRSS,
  PROCINFO_NUM_COLUMNS
};

enum
{
  COLUMN_CTXINFO_INDEX,
  COLUMN_CTXINFO_NAME,
  COLUMN_CTXINFO_CPU_TIME,
  COLUMN_CTXINFO_CPU_PERCENT,
  COLUMN_CTXINFO_VMRSS,
  CTXINFO_NUM_COLUMNS
};

enum
{
  COLUMN_PROCACCT_INDEX,
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

struct _TkmvProcessesView
{
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
}

static void
procinfo_add_columns (TkmvProcessesView *self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column index */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Index", renderer, "text", COLUMN_PROCINFO_INDEX, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 60);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Name", renderer, "text", COLUMN_PROCINFO_NAME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column pid */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "PID", renderer, "text", COLUMN_PROCINFO_PID, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column ppid */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "PPID", renderer, "text", COLUMN_PROCINFO_PPID, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column context */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Context", renderer, "text", COLUMN_PROCINFO_CONTEXT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 150);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column cputime */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "CPUTime", renderer, "text", COLUMN_PROCINFO_CPU_TIME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column cpupercent */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "CPUPercent", renderer, "text", COLUMN_PROCINFO_CPU_PERCENT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procinfo_treeview, column);

  /* column vmrss */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "RSS", renderer, "text", COLUMN_PROCINFO_VMRSS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->procinfo_treeview, column);
}

static void
procinfo_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  TkmvProcessesView *self = (TkmvProcessesView *)data;
  GtkTreeIter iter;
  guint idx = 0;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    gtk_tree_model_get (GTK_TREE_MODEL (self->procinfo_store), &iter,
                        COLUMN_PROCINFO_INDEX, &idx, -1);

  g_message ("Proc info list selected index: %d", idx);
}

static void
ctxinfo_add_columns (TkmvProcessesView *self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column index */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Index", renderer, "text", COLUMN_CTXINFO_INDEX, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 60);
  gtk_tree_view_append_column (self->ctxinfo_treeview, column);

  /* column name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Name", renderer, "text", COLUMN_CTXINFO_NAME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
  gtk_tree_view_append_column (self->ctxinfo_treeview, column);

  /* column cputime */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "CPUTime", renderer, "text", COLUMN_CTXINFO_CPU_TIME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->ctxinfo_treeview, column);

  /* column cpupercent */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "CPUPercent", renderer, "text", COLUMN_CTXINFO_CPU_PERCENT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->ctxinfo_treeview, column);

  /* column vmrss */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "RSS", renderer, "text", COLUMN_CTXINFO_VMRSS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->ctxinfo_treeview, column);
}

static void
ctxinfo_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  TkmvProcessesView *self = (TkmvProcessesView *)data;
  GtkTreeIter iter;
  guint idx = 0;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    gtk_tree_model_get (GTK_TREE_MODEL (self->procinfo_store), &iter,
                        COLUMN_CTXINFO_INDEX, &idx, -1);

  g_message ("Context info list selected index: %d", idx);
}

static void
procacct_add_columns (TkmvProcessesView *self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column index */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Index", renderer, "text", COLUMN_PROCACCT_INDEX, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 60);
  gtk_tree_view_append_column (self->procacct_treeview, column);

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
  TkmvProcessesView *self = (TkmvProcessesView *)data;
  GtkTreeIter iter;
  guint idx = 0;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    gtk_tree_model_get (GTK_TREE_MODEL (self->procinfo_store), &iter,
                        COLUMN_PROCACCT_INDEX, &idx, -1);

  g_message ("Proc accounting list selected index: %d", idx);
}

static void
create_tables (TkmvProcessesView *self)
{
  /* create procinfo store */
  self->procinfo_store = gtk_list_store_new (
      PROCINFO_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT,
      G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);
  gtk_tree_view_set_model (self->procinfo_treeview,
                           GTK_TREE_MODEL (self->procinfo_store));
  procinfo_add_columns (self);

  /* register selection handler */
  self->procinfo_treeview_select
      = gtk_tree_view_get_selection (self->procinfo_treeview);
  gtk_tree_selection_set_mode (self->procinfo_treeview_select,
                               GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (self->procinfo_treeview_select), "changed",
                    G_CALLBACK (procinfo_selection_changed), self);

  /* create ctxinfo store */
  self->ctxinfo_store
      = gtk_list_store_new (CTXINFO_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_STRING,
                            G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);
  gtk_tree_view_set_model (self->ctxinfo_treeview,
                           GTK_TREE_MODEL (self->ctxinfo_store));
  ctxinfo_add_columns (self);

  /* register selection handler */
  self->ctxinfo_treeview_select
      = gtk_tree_view_get_selection (self->ctxinfo_treeview);
  gtk_tree_selection_set_mode (self->ctxinfo_treeview_select,
                               GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (self->ctxinfo_treeview_select), "changed",
                    G_CALLBACK (ctxinfo_selection_changed), self);

  /* create procacct store */
  self->procacct_store = gtk_list_store_new (
      PROCACCT_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT,
      G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_LONG, G_TYPE_LONG,
      G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG,
      G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG,
      G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG,
      G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG,
      G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG,
      G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG);
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
