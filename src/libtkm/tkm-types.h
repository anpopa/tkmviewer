/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019-2022 Alin Popa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \file tkm-types.h
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

#ifndef TKM_UNUSED
#define TKM_UNUSED(x) (void)(x)
#endif

#define TKM_EVENT_SOURCE(x) (GSource *)(x)

#define TKM_DEVICES_TABLE_NAME "tkmDevices"
#define TKM_SESSIONS_TABLE_NAME "tkmSessions"
#define TKM_PROCEVENT_TABLE_NAME "tkmProcEvent"
#define TKM_CPUSTAT_TABLE_NAME "tkmSysProcStat"
#define TKM_MEMINFO_TABLE_NAME "tkmSysProcMemInfo"
#define TKM_PRESSURE_TABLE_NAME "tkmSysProcPressure"
#define TKM_BUDDYINFO_TABLE_NAME "tkmSysProcBuddyInfo"
#define TKM_WIRELESS_TABLE_NAME "tkmSysProcWireless"
#define TKM_DISKSTAT_TABLE_NAME "tkmSysProcDiskStats"
#define TKM_PROCINFO_TABLE_NAME "tkmProcInfo"
#define TKM_PROCACCT_TABLE_NAME "tkmProcAcct"
#define TKM_CTXINFO_TABLE_NAME "tkmContextInfo"

typedef enum _DataTimeSource {
  DATA_TIME_SOURCE_SYSTEM,
  DATA_TIME_SOURCE_MONOTONIC,
  DATA_TIME_SOURCE_RECEIVE
} DataTimeSource;

typedef enum _DataTimeInterval {
  DATA_TIME_INTERVAL_10S,
  DATA_TIME_INTERVAL_1M,
  DATA_TIME_INTERVAL_10M,
  DATA_TIME_INTERVAL_1H,
  DATA_TIME_INTERVAL_24H,
  DATA_TIME_INTERVAL_NOLIMIT
} DataTimeInterval;

typedef enum _TkmStatus {
  TKM_STATUS_ERROR = -1,
  TKM_STATUS_OK
} TkmStatus;

G_END_DECLS
