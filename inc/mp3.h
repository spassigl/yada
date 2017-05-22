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

#ifndef __MP3_H
#define __MP3_H

#include "musicTrack.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Verify the mp3 to be compliant with the DLNA spec
 *
 * @param info item_info info structure
 *
 * @return DLNA_STREAM_NOT_VALID if not valid, any other value otherwise
 */
int mp3_validate( item_info *info );


#ifdef __cplusplus
}
#endif

#endif __DLNAMP3_H