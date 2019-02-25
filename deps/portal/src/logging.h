/*
portal
Copyright (C) 2018-2019	Will Townsend <will@townsend.io>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#ifdef DEBUG
    #define PORTAL_DEBUG_LOG_ENABLED 1
#else
    #define PORTAL_DEBUG_LOG_ENABLED 0
#endif

#define portal_log(format, ...) \
    do { if (PORTAL_DEBUG_LOG_ENABLED) fprintf(stderr, "%s:%d:%s(): " format, __FILE__, __LINE__, __func__, ## __VA_ARGS__); } while (0)
