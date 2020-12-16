/*
 obs-ios-camera-source
 Copyright (C) 2018    Will Townsend <will@townsend.io>

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

#include <obs-module.h>
#include <obs.hpp>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-ios-camera-plugin", "en-US")

#define IOS_CAMERA_PLUGIN_VERSION "2.7.0"

extern void RegisterIOSCameraSource();

bool obs_module_load(void)
{
    blog(LOG_INFO, "Loading iOS Camera Plugin (version %s)", IOS_CAMERA_PLUGIN_VERSION);
    RegisterIOSCameraSource();
    return true;
}
