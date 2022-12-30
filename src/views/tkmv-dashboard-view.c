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

#include <webkit2/webkit2.h>

struct _TkmvDashboardView {
  GtkBox parent_instance;

  WebKitWebView *webview;
};

G_DEFINE_TYPE (TkmvDashboardView, tkmv_dashboard_view, GTK_TYPE_BOX)

static void
tkmv_dashboard_view_class_init (TkmvDashboardViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (
    widget_class, "/ro/fxdata/taskmonitor/viewer/gtk/tkmv-dashboard-view.ui");
  gtk_widget_class_bind_template_child (widget_class, TkmvDashboardView, webview);
}

static void
initialize_web_extensions (WebKitWebContext *context, gpointer user_data)
{
  g_autofree gchar *bpath = g_strdup_printf ("%s/src/webext", TKMV_BUILD_ROOT);
  static guint32 unique_id = 0;

  TKMV_UNUSED (user_data);

  if (g_file_test (bpath, G_FILE_TEST_IS_DIR))
    webkit_web_context_set_web_extensions_directory (context, bpath);
  else
    webkit_web_context_set_web_extensions_directory (context, TKMV_WEBEXT_PATH);

  webkit_web_context_set_web_extensions_initialization_user_data (
    context, g_variant_new_uint32 (unique_id++));
}

static void
tkmv_dashboard_view_init (TkmvDashboardView *self)
{
  g_autofree gchar *path = g_strdup_printf ("file:///%s/app/%s/data/views/resources/index.html",
                                            getenv ("XDG_RUNTIME_DIR"),
                                            getenv ("FLATPAK_ID"));

  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (webkit_web_context_get_default (),
                    "initialize-web-extensions",
                    G_CALLBACK (initialize_web_extensions),
                    NULL);

  webkit_web_view_load_uri (self->webview, path);
}

void
tkmv_dashboard_view_update_content (TkmvDashboardView *view)
{
  g_assert (view);
}
