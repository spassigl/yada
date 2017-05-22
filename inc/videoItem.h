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

#ifndef __VIDEO_H
#define __VIDEO_H

#include "item.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Video system */
typedef enum {
   SYSTEM_UNKNOWN,
   SYSTEM_PAL,
   SYSTEM_NTSC
} VIDEO_SYSTEM;

/**
 * Base structure holding the UPNP videoItem information.
 * All others video info structure will derive from this and MUST have
 * this structure as their first member.
 */
typedef struct videoItem_info {

   /* Video profile */
   ITEM_FORMAT video_format;

   VIDEO_SYSTEM video_system;

} videoItem_info;

/**
 * Returns an videoItem_info information structure
 *
 * @param filename stream file name
 * @param item_info A itemInfo structure
 * @param video_info A videoItem_info info structure to be allocated
 *
 * @return DLNA_NO_ERROR if successful or error code otherwise
 */
int video_getinfo( char *filename, item_info *item, videoItem_info **video_info );

/**
 * Cleans up a previously allocated musicTrack information structure
 *
 * @param mp3_info mp3TrackInfo info structure
 *
DLLEXPORT
 */
void video_freeinfo( videoItem_info **video_info );


#ifdef __cplusplus
}
#endif

#endif __VIDEO_H