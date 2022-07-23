/* tkmv-types.h
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

#pragma once

#include <glib.h>

G_BEGIN_DECLS

#ifndef TKM_UNUSED
#define TKM_UNUSED(x) (void)(x)
#endif

#define TKM_EVENT_SOURCE(x) (GSource *)(x)

typedef enum _TkmStatus
{
  TKM_STATUS_ERROR = -1,
  TKM_STATUS_OK
} TkmlStatus;

G_END_DECLS
