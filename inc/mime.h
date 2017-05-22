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


#ifndef __DLNAMIME_H
#define __DLNAMIME_H

#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif


/*
 * Defines the MIME Types recognized by DLNA
 */

static char *MIME_IMAGE_JPEG	          = "image/jpeg";
static char *MIME_IMAGE_PNG			    = "image/png";

static char *MIME_AUDIO_3GP			    = "audio/3gpp";
static char *MIME_AUDIO_ADTS		       = "audio/vnd.dlna.adts";
static char *MIME_AUDIO_ATRAC           = "audio/x-sony-oma";
static char *MIME_AUDIO_DOLBY_DIGITAL   = "audio/vnd.dolby.dd-raw";
static char *MIME_AUDIO_LPCM            = "audio/L16";
static char *MIME_AUDIO_MPEG            = "audio/mpeg";
static char *MIME_AUDIO_MPEG_4          = "audio/mp4";
static char *MIME_AUDIO_WMA             = "audio/x-ms-wma";

static char *MIME_VIDEO_3GP             = "video/3gpp";
static char *MIME_VIDEO_ASF             = "video/x-ms-asf";
static char *MIME_VIDEO_MPEG            = "video/mpeg";
static char *MIME_VIDEO_MPEG_4          = "video/mp4";
static char *MIME_VIDEO_MPEG_TS         = "video/vnd.dlna.mpeg-tts";
static char *MIME_VIDEO_WMV             = "video/x-ms-wmv";

typedef enum
{
   foo
} MIME_TYPE;

#endif __DLNAMIME_H