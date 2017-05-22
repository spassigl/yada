/*
 * dlnacpp: C++ implementation of the DLNA standards.
 * Copyright (C) 2009 Stefano Passiglia <info@stefanopassiglia.com>
 *
 * This file is part of dlnacpp.
 *
 * dlnacpp is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * dlnacpp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlnacpp; if not, write to the Free Software
 * Foundation, Inc, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef __MP2VIDEO_H
#define __MP2VIDEO_H

#include "videoItem.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Verify the mp3 to be compliant with the DLNA spec
 *
 * @param info itemInfo info structure
 *
 * @return DLNA_STREAM_NOT_VALID if not valid, any other value otherwise
 */
int mp2video_validate( item_info *item );


#ifdef __cplusplus
}
#endif

#endif __MP2VIDEO_H