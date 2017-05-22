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

#ifndef __MUSICTRACK_H
#define __MUSICTRACK_H

#include "item.h"

#ifdef __cplusplus
extern "C" {
#endif



/**
 * Base structure holding the UPNP musicTrack information.
 * All others audio info structure will derive from this and MUST have
 * this structure as their first member.
 */
typedef struct musicTrack_info {

   ITEM_FORMAT audio_format;

   char *genre;
   char *description;

   char *artist;
   char *album;
   char *title;
   int originalTrackNumber;

} musicTrack_info;

/**
 * Returns an musicTrack information structure
 *
 * @param filename stream file name
 * @param item_info A item_info structure
 * @param track_info A musicTrack_info info structure to be allocated
 *
 * @return DLNA_NO_ERROR if successful or error code otherwise
 */
int musicTrack_getinfo( char *filename, item_info *item, musicTrack_info **track_info );

/**
 * Cleans up a previously allocated musicTrack information structure
 *
 * @param dmi musicTrack info structure
 *
 */
void musicTrack_freeinfo( musicTrack_info *track_info );


#ifdef __cplusplus
}
#endif

#endif __MUSICTRACK_H