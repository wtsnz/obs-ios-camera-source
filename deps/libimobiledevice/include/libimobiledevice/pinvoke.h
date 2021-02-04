/**
* @file libimobiledevice/pinvoke.h
* @brief Helper methods for interacting with libimobiledevice from a P/Invoke method.
* \internal
*
* Copyright (c) 2016 Quamotion bvba, All Rights Reserved.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef PINVOKE_H
#define PINVOKE_H

#include <libimobiledevice/libimobiledevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Error Codes */
typedef enum {
    PINVOKE_E_SUCCESS = 0
} pinvoke_error_t;


/**
* Frees a string that was previously allocated by libimobiledevice.
*
* @param string The string to free.
*
* @return Always returns PINVOKE_E_SUCCESS.
*/
LIBIMOBILEDEVICE_API_MSC pinvoke_error_t pinvoke_free_string(char *string);

/**
* Gets the size of a string that was previously allocated by libimobiledevice.
*
* @param string The string of which to get its size.
* @param length The length of the string, in bytes.
*
* @return Always returns PINVOKE_E_SUCCESS.
*/
LIBIMOBILEDEVICE_API_MSC pinvoke_error_t pinvoke_get_string_length(const char *string, uint64_t *length);

#ifdef __cplusplus
}
#endif

#endif
