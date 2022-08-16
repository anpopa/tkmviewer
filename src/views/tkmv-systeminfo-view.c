/* tkmv-systeminfo-view.c
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

#include "tkmv-systeminfo-view.h"
#include "tkm-buddyinfo-entry.h"
#include "tkm-cpustat-entry.h"
#include "tkm-diskstat-entry.h"
#include "tkm-meminfo-entry.h"
#include "tkm-wireless-entry.h"

enum
{
  COLUMN_CPUINFO_NAME,
  COLUMN_CPUINFO_ALL,
  COLUMN_CPUINFO_SYS,
  COLUMN_CPUINFO_USR,
  CPUINFO_NUM_COLUMNS
};

enum
{
  COLUMN_MEMINFO_NAME,
  COLUMN_MEMINFO_MEM_TOTAL,
  COLUMN_MEMINFO_MEM_FREE,
  COLUMN_MEMINFO_MEM_AVAIL,
  COLUMN_MEMINFO_MEM_CACHED,
  COLUMN_MEMINFO_MEM_PERCENT,
  COLUMN_MEMINFO_SWAP_TOTAL,
  COLUMN_MEMINFO_SWAP_FREE,
  COLUMN_MEMINFO_SWAP_CACHED,
  COLUMN_MEMINFO_SWAP_PERCENT,
  COLUMN_MEMINFO_CMA_TOTAL,
  COLUMN_MEMINFO_CMA_FREE,
  MEMINFO_NUM_COLUMNS
};

enum
{
  COLUMN_BUDDYINFO_NAME,
  COLUMN_BUDDYINFO_ZONE,
  COLUMN_BUDDYINFO_DATA,
  BUDDYINFO_NUM_COLUMNS
};

enum
{
  COLUMN_WLANINFO_NAME,
  COLUMN_WLANINFO_STATUS,
  COLUMN_WLANINFO_QUALITY_LINK,
  COLUMN_WLANINFO_QUALITY_LEVEL,
  COLUMN_WLANINFO_QUALITY_NOISE,
  COLUMN_WLANINFO_DISCARDED_NWID,
  COLUMN_WLANINFO_DISCARDED_CRYPT,
  COLUMN_WLANINFO_DISCARDED_FRAG,
  COLUMN_WLANINFO_DISCARDED_MISC,
  COLUMN_WLANINFO_MISSED_BEACON,
  WLANINFO_NUM_COLUMNS
};

enum
{
  COLUMN_DISKINFO_NAME,
  COLUMN_DISKINFO_MAJOR,
  COLUMN_DISKINFO_MINOR,
  COLUMN_DISKINFO_READS_COMPLETED,
  COLUMN_DISKINFO_READS_MERGED,
  COLUMN_DISKINFO_READS_SPENT_MS,
  COLUMN_DISKINFO_WRITES_COMPLETED,
  COLUMN_DISKINFO_WRITES_MERGED,
  COLUMN_DISKINFO_WRITES_SPENT_MS,
  COLUMN_DISKINFO_IO_INPROGRESS,
  COLUMN_DISKINFO_IO_SPENT_MS,
  COLUMN_DISKINFO_IO_WEIGHTED_MS,
  DISKINFO_NUM_COLUMNS
};

static void tkmv_systeminfo_view_widgets_init (TkmvSysteminfoView *self);
static void cpuinfo_add_columns (TkmvSysteminfoView *self);
static void cpuinfo_selection_changed (GtkTreeSelection *selection,
                                       gpointer data);
static void meminfo_add_columns (TkmvSysteminfoView *self);
static void meminfo_selection_changed (GtkTreeSelection *selection,
                                       gpointer data);
static void buddyinfo_add_columns (TkmvSysteminfoView *self);
static void buddyinfo_selection_changed (GtkTreeSelection *selection,
                                         gpointer data);
static void wlaninfo_add_columns (TkmvSysteminfoView *self);
static void wlaninfo_selection_changed (GtkTreeSelection *selection,
                                        gpointer data);
static void diskinfo_add_columns (TkmvSysteminfoView *self);
static void diskinfo_selection_changed (GtkTreeSelection *selection,
                                        gpointer data);
static void create_tables (TkmvSysteminfoView *self);

static void cpuinfo_list_store_append_entry (GtkListStore *list_store,
                                             TkmCpuStatEntry *entry,
                                             GtkTreeIter *iter);
static void reload_cpuinfo_entries (TkmvSysteminfoView *view,
                                    TkmContext *context);
static void meminfo_list_store_append_entry (GtkListStore *list_store,
                                             TkmMemInfoEntry *entry,
                                             GtkTreeIter *iter);
static void reload_meminfo_entries (TkmvSysteminfoView *view,
                                    TkmContext *context);
static void buddyinfo_list_store_append_entry (GtkListStore *list_store,
                                               TkmBuddyInfoEntry *entry,
                                               GtkTreeIter *iter);
static void reload_buddyinfo_entries (TkmvSysteminfoView *view,
                                      TkmContext *context);
static void wlaninfo_list_store_append_entry (GtkListStore *list_store,
                                              TkmWirelessEntry *entry,
                                              GtkTreeIter *iter);
static void reload_wlaninfo_entries (TkmvSysteminfoView *view,
                                     TkmContext *context);
static void diskinfo_list_store_append_entry (GtkListStore *list_store,
                                              TkmDiskStatEntry *entry,
                                              GtkTreeIter *iter);
static void reload_diskinfo_entries (TkmvSysteminfoView *view,
                                     TkmContext *context);

struct _TkmvSysteminfoView
{
  GtkBox parent_instance;

  GtkListStore *cpuinfo_store;
  GtkListStore *meminfo_store;
  GtkListStore *buddyinfo_store;
  GtkListStore *wlaninfo_store;
  GtkListStore *diskinfo_store;

  /* Template widgets */
  GtkScrolledWindow *cpuinfo_scrolled_window;
  GtkTreeView *cpuinfo_treeview;
  GtkTreeSelection *cpuinfo_treeview_select;
  GtkScrolledWindow *meminfo_scrolled_window;
  GtkTreeView *meminfo_treeview;
  GtkTreeSelection *meminfo_treeview_select;
  GtkScrolledWindow *buddyinfo_scrolled_window;
  GtkTreeView *buddyinfo_treeview;
  GtkTreeSelection *buddyinfo_treeview_select;
  GtkScrolledWindow *wlaninfo_scrolled_window;
  GtkTreeView *wlaninfo_treeview;
  GtkTreeSelection *wlaninfo_treeview_select;
  GtkScrolledWindow *diskinfo_scrolled_window;
  GtkTreeView *diskinfo_treeview;
  GtkTreeSelection *diskinfo_treeview_select;
};

G_DEFINE_TYPE (TkmvSysteminfoView, tkmv_systeminfo_view, GTK_TYPE_BOX)

static void
tkmv_systeminfo_view_class_init (TkmvSysteminfoViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (
      widget_class,
      "/ro/fxdata/taskmonitor/viewer/gtk/tkmv-systeminfo-view.ui");

  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        cpuinfo_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        cpuinfo_treeview);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        cpuinfo_treeview_select);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        meminfo_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        meminfo_treeview);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        meminfo_treeview_select);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        buddyinfo_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        buddyinfo_treeview);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        buddyinfo_treeview_select);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        wlaninfo_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        wlaninfo_treeview);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        wlaninfo_treeview_select);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        diskinfo_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        diskinfo_treeview);
  gtk_widget_class_bind_template_child (widget_class, TkmvSysteminfoView,
                                        diskinfo_treeview_select);
}

static void
tkmv_systeminfo_view_init (TkmvSysteminfoView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  tkmv_systeminfo_view_widgets_init (self);
}

static void
tkmv_systeminfo_view_widgets_init (TkmvSysteminfoView *self)
{
  create_tables (self);
}

static void
cpuinfo_add_columns (TkmvSysteminfoView *self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Name", renderer, "text", COLUMN_CPUINFO_NAME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
  gtk_tree_view_append_column (self->cpuinfo_treeview, column);

  /* column all */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("ALL", renderer, "text",
                                                     COLUMN_CPUINFO_ALL, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->cpuinfo_treeview, column);

  /* column sys */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("SYS", renderer, "text",
                                                     COLUMN_CPUINFO_SYS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->cpuinfo_treeview, column);

  /* column usr */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("USR", renderer, "text",
                                                     COLUMN_CPUINFO_USR, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->cpuinfo_treeview, column);
}

static void
cpuinfo_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  TkmvSysteminfoView *self = (TkmvSysteminfoView *)data;
  GtkTreeIter iter;
  g_autofree gchar *name = NULL;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    gtk_tree_model_get (GTK_TREE_MODEL (self->cpuinfo_store), &iter,
                        COLUMN_CPUINFO_NAME, &name, -1);

  g_message ("CPU info list selected index: %s", name);
}

static void
meminfo_add_columns (TkmvSysteminfoView *self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Name", renderer, "text", COLUMN_MEMINFO_NAME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
  gtk_tree_view_append_column (self->meminfo_treeview, column);

  /* column memtotal */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "MemTotal", renderer, "text", COLUMN_MEMINFO_MEM_TOTAL, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->meminfo_treeview, column);

  /* column memfree */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "MemFree", renderer, "text", COLUMN_MEMINFO_MEM_FREE, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->meminfo_treeview, column);

  /* column memavail */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "MemAvail", renderer, "text", COLUMN_MEMINFO_MEM_AVAIL, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->meminfo_treeview, column);

  /* column swaptotal */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "SwapTotal", renderer, "text", COLUMN_MEMINFO_SWAP_TOTAL, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->meminfo_treeview, column);

  /* column swapfree */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "SwapFree", renderer, "text", COLUMN_MEMINFO_SWAP_FREE, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->meminfo_treeview, column);

  /* column swapcached */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "SwapCached", renderer, "text", COLUMN_MEMINFO_SWAP_CACHED, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->meminfo_treeview, column);

  /* column swappercent */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "SwapPercent", renderer, "text", COLUMN_MEMINFO_SWAP_PERCENT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->meminfo_treeview, column);

  /* column cmatotal */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "CMATotal", renderer, "text", COLUMN_MEMINFO_CMA_TOTAL, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->meminfo_treeview, column);

  /* column cmafree */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "CMAFree", renderer, "text", COLUMN_MEMINFO_CMA_FREE, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->meminfo_treeview, column);
}

static void
meminfo_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  TkmvSysteminfoView *self = (TkmvSysteminfoView *)data;
  GtkTreeIter iter;
  g_autofree gchar *name = NULL;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    gtk_tree_model_get (GTK_TREE_MODEL (self->meminfo_store), &iter,
                        COLUMN_MEMINFO_NAME, &name, -1);

  g_message ("Memory info list selected index: %s", name);
}

static void
buddyinfo_add_columns (TkmvSysteminfoView *self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Name", renderer, "text", COLUMN_BUDDYINFO_NAME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
  gtk_tree_view_append_column (self->buddyinfo_treeview, column);

  /* column zone */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Zone", renderer, "text", COLUMN_BUDDYINFO_ZONE, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->buddyinfo_treeview, column);

  /* column data */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Data", renderer, "text", COLUMN_BUDDYINFO_DATA, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->buddyinfo_treeview, column);
}

static void
buddyinfo_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  TkmvSysteminfoView *self = (TkmvSysteminfoView *)data;
  GtkTreeIter iter;
  g_autofree gchar *name = NULL;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    gtk_tree_model_get (GTK_TREE_MODEL (self->buddyinfo_store), &iter,
                        COLUMN_BUDDYINFO_NAME, &name, -1);

  g_message ("Buddy info list selected index: %s", name);
}

static void
wlaninfo_add_columns (TkmvSysteminfoView *self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Name", renderer, "text", COLUMN_WLANINFO_NAME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
  gtk_tree_view_append_column (self->wlaninfo_treeview, column);

  /* column status */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Status", renderer, "text", COLUMN_WLANINFO_STATUS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->wlaninfo_treeview, column);

  /* column qualitylink */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "QALink", renderer, "text", COLUMN_WLANINFO_QUALITY_LINK, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->wlaninfo_treeview, column);

  /* column qualitylevel */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "QALevel", renderer, "text", COLUMN_WLANINFO_QUALITY_LEVEL, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->wlaninfo_treeview, column);

  /* column qualitynoise */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "QANoise", renderer, "text", COLUMN_WLANINFO_QUALITY_NOISE, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->wlaninfo_treeview, column);

  /* column discardednwid */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "DiscNWID", renderer, "text", COLUMN_WLANINFO_DISCARDED_NWID, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->wlaninfo_treeview, column);

  /* column discardedcrypt */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "DiscCrypt", renderer, "text", COLUMN_WLANINFO_DISCARDED_CRYPT, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->wlaninfo_treeview, column);

  /* column discardedfrag */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "DiscFrag", renderer, "text", COLUMN_WLANINFO_DISCARDED_FRAG, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->wlaninfo_treeview, column);

  /* column discardedmisc */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "DiscMisc", renderer, "text", COLUMN_WLANINFO_DISCARDED_MISC, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->wlaninfo_treeview, column);

  /* column missedbracon */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "MissedBeacon", renderer, "text", COLUMN_WLANINFO_MISSED_BEACON, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (self->wlaninfo_treeview, column);
}

static void
wlaninfo_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  TkmvSysteminfoView *self = (TkmvSysteminfoView *)data;
  GtkTreeIter iter;
  g_autofree gchar *name = NULL;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    gtk_tree_model_get (GTK_TREE_MODEL (self->wlaninfo_store), &iter,
                        COLUMN_WLANINFO_NAME, &name, -1);

  g_message ("Wireless info list selected index: %s", name);
}

static void
diskinfo_add_columns (TkmvSysteminfoView *self)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Name", renderer, "text", COLUMN_DISKINFO_NAME, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_expand (GTK_TREE_VIEW_COLUMN (column), TRUE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_DISKINFO_NAME);
  gtk_tree_view_append_column (self->diskinfo_treeview, column);

  /* column major */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Major", renderer, "text", COLUMN_DISKINFO_MAJOR, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_DISKINFO_MAJOR);
  gtk_tree_view_append_column (self->diskinfo_treeview, column);

  /* column minor */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "Minor", renderer, "text", COLUMN_DISKINFO_MINOR, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_DISKINFO_MINOR);
  gtk_tree_view_append_column (self->diskinfo_treeview, column);

  /* column readscompleted */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "ReadsCompleted", renderer, "text", COLUMN_DISKINFO_READS_COMPLETED,
      NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_DISKINFO_READS_COMPLETED);
  gtk_tree_view_append_column (self->diskinfo_treeview, column);

  /* column readsmerged */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "ReadsMerged", renderer, "text", COLUMN_DISKINFO_READS_MERGED, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_DISKINFO_READS_MERGED);
  gtk_tree_view_append_column (self->diskinfo_treeview, column);

  /* column readsspentms */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "ReadsSpentMS", renderer, "text", COLUMN_DISKINFO_READS_SPENT_MS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_DISKINFO_READS_SPENT_MS);
  gtk_tree_view_append_column (self->diskinfo_treeview, column);

  /* column writescompleted */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "WritesCompleted", renderer, "text", COLUMN_DISKINFO_WRITES_COMPLETED,
      NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_DISKINFO_WRITES_COMPLETED);
  gtk_tree_view_append_column (self->diskinfo_treeview, column);

  /* column writesmerged */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "WritesMerged", renderer, "text", COLUMN_DISKINFO_WRITES_MERGED, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_DISKINFO_WRITES_MERGED);
  gtk_tree_view_append_column (self->diskinfo_treeview, column);

  /* column writesspentms */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "WritesSpentMS", renderer, "text", COLUMN_DISKINFO_WRITES_SPENT_MS,
      NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_DISKINFO_WRITES_SPENT_MS);
  gtk_tree_view_append_column (self->diskinfo_treeview, column);

  /* column ioinprogress */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "IOInProgress", renderer, "text", COLUMN_DISKINFO_IO_INPROGRESS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_DISKINFO_IO_INPROGRESS);
  gtk_tree_view_append_column (self->diskinfo_treeview, column);

  /* column iospentms */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "IOSpentMS", renderer, "text", COLUMN_DISKINFO_IO_SPENT_MS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_DISKINFO_IO_SPENT_MS);
  gtk_tree_view_append_column (self->diskinfo_treeview, column);

  /* column ioweightedms */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
      "IOWeightedMS", renderer, "text", COLUMN_DISKINFO_IO_WEIGHTED_MS, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                   GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column),
                                           COLUMN_DISKINFO_IO_WEIGHTED_MS);
  gtk_tree_view_append_column (self->diskinfo_treeview, column);
}

static void
diskinfo_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  TkmvSysteminfoView *self = (TkmvSysteminfoView *)data;
  GtkTreeIter iter;
  g_autofree gchar *name = NULL;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    gtk_tree_model_get (GTK_TREE_MODEL (self->diskinfo_store), &iter,
                        COLUMN_DISKINFO_NAME, &name, -1);

  g_message ("Disk info list selected index: %s", name);
}

int
diskinfo_sort_iter_compare_func (GtkTreeModel *model, GtkTreeIter *a,
                                 GtkTreeIter *b, gpointer userdata)
{
  int sortcol = GPOINTER_TO_INT (userdata);
  int ret = 0; /* default if egual */

  switch (sortcol)
    {
    case COLUMN_DISKINFO_NAME:
      {
        char *name1, *name2;

        gtk_tree_model_get (model, a, COLUMN_DISKINFO_NAME, &name1, -1);
        gtk_tree_model_get (model, b, COLUMN_DISKINFO_NAME, &name2, -1);

        if (name1 == NULL || name2 == NULL)
          {
            if (name1 == NULL && name2 == NULL)
              break; /* both equal => ret = 0 */

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

    case COLUMN_DISKINFO_MAJOR:
      {
        glong val1, val2;

        gtk_tree_model_get (model, a, COLUMN_DISKINFO_MAJOR, &val1, -1);
        gtk_tree_model_get (model, b, COLUMN_DISKINFO_MAJOR, &val2, -1);

        if (val1 != val2)
          {
            ret = (val1 > val2) ? 1 : -1;
          }
      }
      break;

    case COLUMN_DISKINFO_MINOR:
      {
        glong val1, val2;

        gtk_tree_model_get (model, a, COLUMN_DISKINFO_MINOR, &val1, -1);
        gtk_tree_model_get (model, b, COLUMN_DISKINFO_MINOR, &val2, -1);

        if (val1 != val2)
          {
            ret = (val1 > val2) ? 1 : -1;
          }
      }
      break;

    case COLUMN_DISKINFO_IO_SPENT_MS:
      {
        glong val1, val2;

        gtk_tree_model_get (model, a, COLUMN_DISKINFO_IO_SPENT_MS, &val1, -1);
        gtk_tree_model_get (model, b, COLUMN_DISKINFO_IO_SPENT_MS, &val2, -1);

        if (val1 != val2)
          {
            ret = (val1 > val2) ? 1 : -1;
          }
      }
      break;
    case COLUMN_DISKINFO_READS_MERGED:
      {
        glong val1, val2;

        gtk_tree_model_get (model, a, COLUMN_DISKINFO_READS_MERGED, &val1, -1);
        gtk_tree_model_get (model, b, COLUMN_DISKINFO_READS_MERGED, &val2, -1);

        if (val1 != val2)
          {
            ret = (val1 > val2) ? 1 : -1;
          }
      }
      break;
    case COLUMN_DISKINFO_WRITES_MERGED:
      {
        glong val1, val2;

        gtk_tree_model_get (model, a, COLUMN_DISKINFO_WRITES_MERGED, &val1,
                            -1);
        gtk_tree_model_get (model, b, COLUMN_DISKINFO_WRITES_MERGED, &val2,
                            -1);

        if (val1 != val2)
          {
            ret = (val1 > val2) ? 1 : -1;
          }
      }
      break;
    case COLUMN_DISKINFO_IO_INPROGRESS:
      {
        glong val1, val2;

        gtk_tree_model_get (model, a, COLUMN_DISKINFO_IO_INPROGRESS, &val1,
                            -1);
        gtk_tree_model_get (model, b, COLUMN_DISKINFO_IO_INPROGRESS, &val2,
                            -1);

        if (val1 != val2)
          {
            ret = (val1 > val2) ? 1 : -1;
          }
      }
      break;
    case COLUMN_DISKINFO_IO_WEIGHTED_MS:
      {
        glong val1, val2;

        gtk_tree_model_get (model, a, COLUMN_DISKINFO_IO_WEIGHTED_MS, &val1,
                            -1);
        gtk_tree_model_get (model, b, COLUMN_DISKINFO_IO_WEIGHTED_MS, &val2,
                            -1);

        if (val1 != val2)
          {
            ret = (val1 > val2) ? 1 : -1;
          }
      }
      break;
    case COLUMN_DISKINFO_READS_SPENT_MS:
      {
        glong val1, val2;

        gtk_tree_model_get (model, a, COLUMN_DISKINFO_READS_SPENT_MS, &val1,
                            -1);
        gtk_tree_model_get (model, b, COLUMN_DISKINFO_READS_SPENT_MS, &val2,
                            -1);

        if (val1 != val2)
          {
            ret = (val1 > val2) ? 1 : -1;
          }
      }
      break;
    case COLUMN_DISKINFO_WRITES_SPENT_MS:
      {
        glong val1, val2;

        gtk_tree_model_get (model, a, COLUMN_DISKINFO_WRITES_SPENT_MS, &val1,
                            -1);
        gtk_tree_model_get (model, b, COLUMN_DISKINFO_WRITES_SPENT_MS, &val2,
                            -1);

        if (val1 != val2)
          {
            ret = (val1 > val2) ? 1 : -1;
          }
      }
      break;
    case COLUMN_DISKINFO_READS_COMPLETED:
      {
        glong val1, val2;

        gtk_tree_model_get (model, a, COLUMN_DISKINFO_READS_COMPLETED, &val1,
                            -1);
        gtk_tree_model_get (model, b, COLUMN_DISKINFO_READS_COMPLETED, &val2,
                            -1);

        if (val1 != val2)
          {
            ret = (val1 > val2) ? 1 : -1;
          }
      }
      break;
    case COLUMN_DISKINFO_WRITES_COMPLETED:
      {
        glong val1, val2;

        gtk_tree_model_get (model, a, COLUMN_DISKINFO_WRITES_COMPLETED, &val1,
                            -1);
        gtk_tree_model_get (model, b, COLUMN_DISKINFO_WRITES_COMPLETED, &val2,
                            -1);

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
create_tables (TkmvSysteminfoView *self)
{
  /* create cpuinfo store */
  self->cpuinfo_store
      = gtk_list_store_new (CPUINFO_NUM_COLUMNS, G_TYPE_STRING, G_TYPE_UINT,
                            G_TYPE_UINT, G_TYPE_UINT);
  gtk_tree_view_set_model (self->cpuinfo_treeview,
                           GTK_TREE_MODEL (self->cpuinfo_store));
  cpuinfo_add_columns (self);

  /* register selection handler */
  self->cpuinfo_treeview_select
      = gtk_tree_view_get_selection (self->cpuinfo_treeview);
  gtk_tree_selection_set_mode (self->cpuinfo_treeview_select,
                               GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (self->cpuinfo_treeview_select), "changed",
                    G_CALLBACK (cpuinfo_selection_changed), self);

  /* create meminfo store */
  self->meminfo_store = gtk_list_store_new (
      MEMINFO_NUM_COLUMNS, G_TYPE_STRING, G_TYPE_ULONG, G_TYPE_ULONG,
      G_TYPE_ULONG, G_TYPE_ULONG, G_TYPE_ULONG, G_TYPE_ULONG, G_TYPE_ULONG,
      G_TYPE_ULONG, G_TYPE_ULONG, G_TYPE_ULONG, G_TYPE_ULONG);
  gtk_tree_view_set_model (self->meminfo_treeview,
                           GTK_TREE_MODEL (self->meminfo_store));
  meminfo_add_columns (self);

  /* register selection handler */
  self->meminfo_treeview_select
      = gtk_tree_view_get_selection (self->meminfo_treeview);
  gtk_tree_selection_set_mode (self->meminfo_treeview_select,
                               GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (self->meminfo_treeview_select), "changed",
                    G_CALLBACK (meminfo_selection_changed), self);

  /* create buddyinfo store */
  self->buddyinfo_store = gtk_list_store_new (
      BUDDYINFO_NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_view_set_model (self->buddyinfo_treeview,
                           GTK_TREE_MODEL (self->buddyinfo_store));
  buddyinfo_add_columns (self);

  /* register selection handler */
  self->buddyinfo_treeview_select
      = gtk_tree_view_get_selection (self->buddyinfo_treeview);
  gtk_tree_selection_set_mode (self->buddyinfo_treeview_select,
                               GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (self->buddyinfo_treeview_select), "changed",
                    G_CALLBACK (buddyinfo_selection_changed), self);

  /* create wlaninfo store */
  self->wlaninfo_store
      = gtk_list_store_new (WLANINFO_NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
                            G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT,
                            G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
  gtk_tree_view_set_model (self->wlaninfo_treeview,
                           GTK_TREE_MODEL (self->wlaninfo_store));
  wlaninfo_add_columns (self);

  /* register selection handler */
  self->wlaninfo_treeview_select
      = gtk_tree_view_get_selection (self->wlaninfo_treeview);
  gtk_tree_selection_set_mode (self->wlaninfo_treeview_select,
                               GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (self->wlaninfo_treeview_select), "changed",
                    G_CALLBACK (wlaninfo_selection_changed), self);

  /* create diskinfo store */
  self->diskinfo_store = gtk_list_store_new (
      DISKINFO_NUM_COLUMNS, G_TYPE_STRING, G_TYPE_LONG, G_TYPE_LONG,
      G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG,
      G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG, G_TYPE_LONG);
  gtk_tree_view_set_model (self->diskinfo_treeview,
                           GTK_TREE_MODEL (self->diskinfo_store));
  diskinfo_add_columns (self);

  /* register selection handler */
  self->diskinfo_treeview_select
      = gtk_tree_view_get_selection (self->diskinfo_treeview);
  gtk_tree_selection_set_mode (self->diskinfo_treeview_select,
                               GTK_SELECTION_SINGLE);

  gtk_tree_sortable_set_sort_func (
      GTK_TREE_SORTABLE (self->diskinfo_store), COLUMN_DISKINFO_NAME,
      diskinfo_sort_iter_compare_func, GINT_TO_POINTER (COLUMN_DISKINFO_NAME),
      NULL);
  gtk_tree_sortable_set_sort_func (
      GTK_TREE_SORTABLE (self->diskinfo_store), COLUMN_DISKINFO_MINOR,
      diskinfo_sort_iter_compare_func, GINT_TO_POINTER (COLUMN_DISKINFO_MINOR),
      NULL);
  gtk_tree_sortable_set_sort_func (
      GTK_TREE_SORTABLE (self->diskinfo_store), COLUMN_DISKINFO_MAJOR,
      diskinfo_sort_iter_compare_func, GINT_TO_POINTER (COLUMN_DISKINFO_MAJOR),
      NULL);
  gtk_tree_sortable_set_sort_func (
      GTK_TREE_SORTABLE (self->diskinfo_store), COLUMN_DISKINFO_IO_SPENT_MS,
      diskinfo_sort_iter_compare_func,
      GINT_TO_POINTER (COLUMN_DISKINFO_IO_SPENT_MS), NULL);
  gtk_tree_sortable_set_sort_func (
      GTK_TREE_SORTABLE (self->diskinfo_store), COLUMN_DISKINFO_READS_MERGED,
      diskinfo_sort_iter_compare_func,
      GINT_TO_POINTER (COLUMN_DISKINFO_READS_MERGED), NULL);
  gtk_tree_sortable_set_sort_func (
      GTK_TREE_SORTABLE (self->diskinfo_store), COLUMN_DISKINFO_WRITES_MERGED,
      diskinfo_sort_iter_compare_func,
      GINT_TO_POINTER (COLUMN_DISKINFO_WRITES_MERGED), NULL);
  gtk_tree_sortable_set_sort_func (
      GTK_TREE_SORTABLE (self->diskinfo_store), COLUMN_DISKINFO_IO_INPROGRESS,
      diskinfo_sort_iter_compare_func,
      GINT_TO_POINTER (COLUMN_DISKINFO_IO_INPROGRESS), NULL);
  gtk_tree_sortable_set_sort_func (
      GTK_TREE_SORTABLE (self->diskinfo_store), COLUMN_DISKINFO_READS_SPENT_MS,
      diskinfo_sort_iter_compare_func,
      GINT_TO_POINTER (COLUMN_DISKINFO_READS_SPENT_MS), NULL);
  gtk_tree_sortable_set_sort_func (
      GTK_TREE_SORTABLE (self->diskinfo_store),
      COLUMN_DISKINFO_READS_COMPLETED, diskinfo_sort_iter_compare_func,
      GINT_TO_POINTER (COLUMN_DISKINFO_READS_COMPLETED), NULL);
  gtk_tree_sortable_set_sort_func (
      GTK_TREE_SORTABLE (self->diskinfo_store),
      COLUMN_DISKINFO_WRITES_SPENT_MS, diskinfo_sort_iter_compare_func,
      GINT_TO_POINTER (COLUMN_DISKINFO_WRITES_SPENT_MS), NULL);
  gtk_tree_sortable_set_sort_func (
      GTK_TREE_SORTABLE (self->diskinfo_store),
      COLUMN_DISKINFO_WRITES_COMPLETED, diskinfo_sort_iter_compare_func,
      GINT_TO_POINTER (COLUMN_DISKINFO_WRITES_COMPLETED), NULL);
  gtk_tree_sortable_set_sort_column_id (
      GTK_TREE_SORTABLE (self->diskinfo_store),
      COLUMN_DISKINFO_WRITES_COMPLETED, GTK_SORT_DESCENDING);

  g_signal_connect (G_OBJECT (self->diskinfo_treeview_select), "changed",
                    G_CALLBACK (diskinfo_selection_changed), self);
}

static void
cpuinfo_list_store_append_entry (GtkListStore *list_store,
                                 TkmCpuStatEntry *entry, GtkTreeIter *iter)
{
  gtk_list_store_append (list_store, iter);
  gtk_list_store_set (list_store, iter, COLUMN_CPUINFO_NAME,
                      tkm_cpustat_entry_get_name (entry), COLUMN_CPUINFO_ALL,
                      tkm_cpustat_entry_get_all (entry), COLUMN_CPUINFO_SYS,
                      tkm_cpustat_entry_get_sys (entry), COLUMN_CPUINFO_USR,
                      tkm_cpustat_entry_get_usr (entry), -1);
}

static void
reload_cpuinfo_entries (TkmvSysteminfoView *view, TkmContext *context)
{
  GPtrArray *entries = tkm_context_get_cpustat_entries (context);
  GtkTreeIter iter;

  if (entries == NULL)
    return;

  if (entries->len == 0)
    return;

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->cpuinfo_treeview), NULL);
  gtk_list_store_clear (view->cpuinfo_store);

  /* our first entry is "cpu" with overall values */
  TkmCpuStatEntry *statEntry = g_ptr_array_index (entries, 0);
  tkm_cpustat_entry_set_index (statEntry, 0);
  cpuinfo_list_store_append_entry (view->cpuinfo_store, statEntry, &iter);

  for (guint i = 1; i < entries->len; i++)
    {
      TkmCpuStatEntry *entry = g_ptr_array_index (entries, i);

      tkm_cpustat_entry_set_index (entry, i);

      /* we only list first entry of cpu with cores */
      if (g_strcmp0 (tkm_cpustat_entry_get_name (entry), "cpu") != 0)
        {
          cpuinfo_list_store_append_entry (view->cpuinfo_store, entry, &iter);
        }
      else
        {
          break;
        }
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->cpuinfo_treeview),
                           GTK_TREE_MODEL (view->cpuinfo_store));
}

static void
meminfo_list_store_append_entry (GtkListStore *list_store,
                                 TkmMemInfoEntry *entry, GtkTreeIter *iter)
{
  gtk_list_store_append (list_store, iter);
  gtk_list_store_set (
      list_store, iter, COLUMN_MEMINFO_NAME, "main", COLUMN_MEMINFO_MEM_TOTAL,
      tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_TOTAL),
      COLUMN_MEMINFO_MEM_FREE,
      tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_FREE),
      COLUMN_MEMINFO_MEM_AVAIL,
      tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_AVAIL),
      COLUMN_MEMINFO_MEM_CACHED,
      tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_CACHED),
      COLUMN_MEMINFO_MEM_PERCENT,
      tkm_meminfo_entry_get_data (entry, MINFO_DATA_MEM_PERCENT),
      COLUMN_MEMINFO_SWAP_TOTAL,
      tkm_meminfo_entry_get_data (entry, MINFO_DATA_SWAP_TOTAL),
      COLUMN_MEMINFO_SWAP_FREE,
      tkm_meminfo_entry_get_data (entry, MINFO_DATA_SWAP_FREE),
      COLUMN_MEMINFO_SWAP_CACHED,
      tkm_meminfo_entry_get_data (entry, MINFO_DATA_SWAP_CACHED),
      COLUMN_MEMINFO_SWAP_PERCENT,
      tkm_meminfo_entry_get_data (entry, MINFO_DATA_SWAP_PERCENT),
      COLUMN_MEMINFO_CMA_TOTAL,
      tkm_meminfo_entry_get_data (entry, MINFO_DATA_CMA_TOTAL),
      COLUMN_MEMINFO_CMA_FREE,
      tkm_meminfo_entry_get_data (entry, MINFO_DATA_CMA_FREE), -1);
}

static void
reload_meminfo_entries (TkmvSysteminfoView *view, TkmContext *context)
{
  GPtrArray *entries = tkm_context_get_meminfo_entries (context);
  GtkTreeIter iter;

  if (entries == NULL)
    return;

  if (entries->len == 0)
    return;

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->meminfo_treeview), NULL);
  gtk_list_store_clear (view->meminfo_store);

  TkmMemInfoEntry *entry = g_ptr_array_index (entries, 0);
  tkm_meminfo_entry_set_index (entry, 0);
  meminfo_list_store_append_entry (view->meminfo_store, entry, &iter);

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->meminfo_treeview),
                           GTK_TREE_MODEL (view->meminfo_store));
}

static void
buddyinfo_list_store_append_entry (GtkListStore *list_store,
                                   TkmBuddyInfoEntry *entry, GtkTreeIter *iter)
{
  gtk_list_store_append (list_store, iter);
  gtk_list_store_set (
      list_store, iter, COLUMN_BUDDYINFO_NAME,
      tkm_buddyinfo_entry_get_name (entry), COLUMN_BUDDYINFO_ZONE,
      tkm_buddyinfo_entry_get_zone (entry), COLUMN_BUDDYINFO_DATA,
      tkm_buddyinfo_entry_get_data (entry), -1);
}

static void
reload_buddyinfo_entries (TkmvSysteminfoView *view, TkmContext *context)
{
  GPtrArray *entries = tkm_context_get_buddyinfo_entries (context);
  GtkTreeIter iter;

  if (entries == NULL)
    return;

  if (entries->len == 0)
    return;

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->buddyinfo_treeview), NULL);
  gtk_list_store_clear (view->buddyinfo_store);

  /* get first entry */
  TkmBuddyInfoEntry *firstEntry = g_ptr_array_index (entries, 0);
  tkm_buddyinfo_entry_set_index (firstEntry, 0);
  buddyinfo_list_store_append_entry (view->buddyinfo_store, firstEntry, &iter);

  for (guint i = 1; i < entries->len; i++)
    {
      TkmBuddyInfoEntry *entry = g_ptr_array_index (entries, i);

      tkm_buddyinfo_entry_set_index (entry, i);
      if (tkm_buddyinfo_entry_get_timestamp (entry, DATA_TIME_SOURCE_MONOTONIC)
          == tkm_buddyinfo_entry_get_timestamp (firstEntry,
                                                DATA_TIME_SOURCE_MONOTONIC))
        {
          buddyinfo_list_store_append_entry (view->buddyinfo_store, entry,
                                             &iter);
        }
      else
        {
          break;
        }
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->buddyinfo_treeview),
                           GTK_TREE_MODEL (view->buddyinfo_store));
}

static void
wlaninfo_list_store_append_entry (GtkListStore *list_store,
                                  TkmWirelessEntry *entry, GtkTreeIter *iter)
{
  gtk_list_store_append (list_store, iter);
  gtk_list_store_set (
      list_store, iter, COLUMN_WLANINFO_NAME,
      tkm_wireless_entry_get_name (entry), COLUMN_WLANINFO_STATUS,
      tkm_wireless_entry_get_status (entry), COLUMN_WLANINFO_QUALITY_LINK,
      tkm_wireless_entry_get_data (entry, WLAN_DATA_QUALITY_LINK),
      COLUMN_WLANINFO_QUALITY_LEVEL,
      tkm_wireless_entry_get_data (entry, WLAN_DATA_QUALITY_LEVEL),
      COLUMN_WLANINFO_QUALITY_NOISE,
      tkm_wireless_entry_get_data (entry, WLAN_DATA_QUALITY_NOISE),
      COLUMN_WLANINFO_DISCARDED_NWID,
      tkm_wireless_entry_get_data (entry, WLAN_DATA_DISCARDED_NWID),
      COLUMN_WLANINFO_DISCARDED_CRYPT,
      tkm_wireless_entry_get_data (entry, WLAN_DATA_DISCARDED_CRYPT),
      COLUMN_WLANINFO_DISCARDED_FRAG,
      tkm_wireless_entry_get_data (entry, WLAN_DATA_DISCARDED_FRAG),
      COLUMN_WLANINFO_DISCARDED_MISC,
      tkm_wireless_entry_get_data (entry, WLAN_DATA_DISCARDED_MISC),
      COLUMN_WLANINFO_MISSED_BEACON,
      tkm_wireless_entry_get_data (entry, WLAN_DATA_MISSED_BEACON), -1);
}

static void
reload_wlaninfo_entries (TkmvSysteminfoView *view, TkmContext *context)
{
  GPtrArray *entries = tkm_context_get_wireless_entries (context);
  GtkTreeIter iter;

  if (entries == NULL)
    return;

  if (entries->len == 0)
    return;

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->wlaninfo_treeview), NULL);
  gtk_list_store_clear (view->wlaninfo_store);

  /* get first entry */
  TkmWirelessEntry *firstEntry = g_ptr_array_index (entries, 0);
  tkm_wireless_entry_set_index (firstEntry, 0);
  wlaninfo_list_store_append_entry (view->wlaninfo_store, firstEntry, &iter);

  for (guint i = 1; i < entries->len; i++)
    {
      TkmWirelessEntry *entry = g_ptr_array_index (entries, i);

      tkm_wireless_entry_set_index (entry, i);
      if (tkm_wireless_entry_get_timestamp (entry, DATA_TIME_SOURCE_MONOTONIC)
          == tkm_wireless_entry_get_timestamp (firstEntry,
                                               DATA_TIME_SOURCE_MONOTONIC))
        {
          wlaninfo_list_store_append_entry (view->wlaninfo_store, entry,
                                            &iter);
        }
      else
        {
          break;
        }
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->wlaninfo_treeview),
                           GTK_TREE_MODEL (view->wlaninfo_store));
}

static void
diskinfo_list_store_append_entry (GtkListStore *list_store,
                                  TkmDiskStatEntry *entry, GtkTreeIter *iter)
{
  gtk_list_store_append (list_store, iter);
  gtk_list_store_set (
      list_store, iter, COLUMN_DISKINFO_NAME,
      tkm_diskstat_entry_get_name (entry), COLUMN_DISKINFO_MAJOR,
      tkm_diskstat_entry_get_data (entry, DISKSTAT_DATA_MAJOR),
      COLUMN_DISKINFO_MINOR,
      tkm_diskstat_entry_get_data (entry, DISKSTAT_DATA_MINOR),
      COLUMN_DISKINFO_READS_COMPLETED,
      tkm_diskstat_entry_get_data (entry, DISKSTAT_DATA_READS_COMPLETED),
      COLUMN_DISKINFO_READS_MERGED,
      tkm_diskstat_entry_get_data (entry, DISKSTAT_DATA_READS_MERGED),
      COLUMN_DISKINFO_READS_SPENT_MS,
      tkm_diskstat_entry_get_data (entry, DISKSTAT_DATA_READS_SPENT_MS),
      COLUMN_DISKINFO_WRITES_COMPLETED,
      tkm_diskstat_entry_get_data (entry, DISKSTAT_DATA_WRITES_COMPLETED),
      COLUMN_DISKINFO_WRITES_MERGED,
      tkm_diskstat_entry_get_data (entry, DISKSTAT_DATA_WRITES_MERGED),
      COLUMN_DISKINFO_WRITES_SPENT_MS,
      tkm_diskstat_entry_get_data (entry, DISKSTAT_DATA_WRITES_SPENT_MS),
      COLUMN_DISKINFO_IO_INPROGRESS,
      tkm_diskstat_entry_get_data (entry, DISKSTAT_DATA_IO_INPROGRESS),
      COLUMN_DISKINFO_IO_SPENT_MS,
      tkm_diskstat_entry_get_data (entry, DISKSTAT_DATA_IO_SPENT_MS),
      COLUMN_DISKINFO_IO_WEIGHTED_MS,
      tkm_diskstat_entry_get_data (entry, DISKSTAT_DATA_IO_WEIGHTED_MS), -1);
}

static void
reload_diskinfo_entries (TkmvSysteminfoView *view, TkmContext *context)
{
  GPtrArray *entries = tkm_context_get_diskstat_entries (context);
  GtkTreeIter iter;

  if (entries == NULL)
    return;

  if (entries->len == 0)
    return;

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->diskinfo_treeview), NULL);
  gtk_list_store_clear (view->diskinfo_store);

  /* get first entry */
  TkmDiskStatEntry *firstEntry = g_ptr_array_index (entries, 0);
  tkm_diskstat_entry_set_index (firstEntry, 0);
  diskinfo_list_store_append_entry (view->diskinfo_store, firstEntry, &iter);

  for (guint i = 1; i < entries->len; i++)
    {
      TkmDiskStatEntry *entry = g_ptr_array_index (entries, i);

      tkm_diskstat_entry_set_index (entry, i);
      if (tkm_diskstat_entry_get_timestamp (entry, DATA_TIME_SOURCE_MONOTONIC)
          == tkm_diskstat_entry_get_timestamp (firstEntry,
                                               DATA_TIME_SOURCE_MONOTONIC))
        {
          diskinfo_list_store_append_entry (view->diskinfo_store, entry,
                                            &iter);
        }
      else
        {
          break;
        }
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->diskinfo_treeview),
                           GTK_TREE_MODEL (view->diskinfo_store));
}

void
tkmv_systeminfo_reload_entries (TkmvSysteminfoView *view, TkmContext *context)
{
  reload_cpuinfo_entries (view, context);
  reload_meminfo_entries (view, context);
  reload_buddyinfo_entries (view, context);
  reload_wlaninfo_entries (view, context);
  reload_diskinfo_entries (view, context);
}
