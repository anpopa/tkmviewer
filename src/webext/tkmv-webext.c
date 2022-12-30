/* tkmv-webext.c
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

#include <gtk/gtk.h>
#include <webkit2/webkit-web-extension.h>

static void
user_message_received (WebKitWebExtension *extension, WebKitUserMessage *message,
                       gpointer user_data)
{
  TKMV_UNUSED (extension);
  TKMV_UNUSED (user_data);
  TKMV_UNUSED (message);
  g_message ("User message received");
}

static void
window_object_cleared_callback (WebKitScriptWorld *world,
                                WebKitWebPage *web_page,
                                WebKitFrame *frame,
                                gpointer user_data)
{
  JSCContext *jsContext;
  JSCValue *globalObject;

  TKMV_UNUSED (web_page);
  TKMV_UNUSED (user_data);

  jsContext = webkit_frame_get_js_context_for_script_world (frame, world);
  globalObject = jsc_context_get_global_object (jsContext);

  /* Use JSC API to add the JavaScript code you want */
  TKMV_UNUSED (globalObject);
}

static void
web_page_created_callback (WebKitWebExtension *extension,
                           WebKitWebPage *web_page,
                           gpointer user_data)
{
  TKMV_UNUSED (user_data);
  TKMV_UNUSED (extension);

  g_print ("Page %ld created for %s\n",
           webkit_web_page_get_id (web_page),
           webkit_web_page_get_uri (web_page));
}

G_MODULE_EXPORT void
webkit_web_extension_initialize (WebKitWebExtension *extension)
{
  g_signal_connect (extension, "page-created", G_CALLBACK (web_page_created_callback), NULL);

  g_signal_connect (extension, "user-message-received", G_CALLBACK (user_message_received), NULL);

  g_signal_connect (webkit_script_world_get_default (),
                    "window-object-cleared",
                    G_CALLBACK (window_object_cleared_callback),
                    NULL);
}
