/*
 * YADL - Yet Another DLNA Library
 * Copyright (C) 2008 Stefano Passiglia <info@stefanopassiglia.com>
 *
 * This file is part of YADL.
 *
 * YADL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * YADL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlnacpp; if not, write to the Free Software
 * Foundation, Inc, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __PHOTO_H
#define __PHOTO_H

#include "item.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure holding the UPNP photo information
 */
typedef struct photo_info {

   /* Photo profile */
   ITEM_FORMAT photo_format;

} photo_info;

/**
 * Returns an photo_info information structure
 *
 * @param filename stream file name
 * @param item_info A item_info structure
 * @param photo_info A photo_info info structure to be allocated
 *
 * @return DLNA_NO_ERROR if successful or error code otherwise
 */

int photo_getinfo( char *filename, item_info *item, photo_info **info );

/**
 * Cleans up a previously allocated musicTrack information structure
 *
 * @param photo_info photo_info info structure
 *
 */
void photo_freeinfo( photo_info *info );



#ifdef __cplusplus
}
#endif

#endif __MUSICTRACK_H