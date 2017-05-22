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

#ifndef __ITEM_H
#define __ITEM_H

#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif WIN32

#ifdef __cplusplus
extern "C" {
#endif

/* Under Win32, define inline to include ffmpeg headers */
#ifdef WIN32
#define inline _inline
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#include "profiles.h"

/** UPnP item type */
typedef enum 
{
   ITEM_UNDEFINED = 0,

   ITEM_VIDEO = 1<<0,
   ITEM_AUDIO = 1<<1,
   ITEM_AUDIOVIDEO = 1<<2,
   ITEM_PHOTO = 1<<3,
   ITEM_PLAYLIST = 1<<4,

   ITEM_CONTAINER = 1<<5,
} ITEM_TYPE;

/** DLNA Item class strings. Actually, they correspond to the UPnP class strings */
typedef char* ITEM_CLASS;
static ITEM_CLASS DLNA_PHOTO_ITEM_CLASS = "object.item.imageItem.photo";
static ITEM_CLASS DLNA_MUSICTRACK_ITEM_CLASS = "object.item.audioItem.musicTrack";
static ITEM_CLASS DLNA_VIDEO_ITEM_CLASS = "object.item.videoItem.movie";

/* 
 * Unique ID for each item, which is
 * actually the MD5 digest of the file
 * name (32 bytes + 1 char for zero termination).
 */
typedef char ITEM_ID[33];

/** The allowed DLNA media formats */
typedef enum 
{

   /* LPCM */
   AUDIO_FORMAT_LPCM,

   /* MP3 */
   AUDIO_FORMAT_MP3,

   /* WMA */
   AUDIO_FORMAT_WMA,
   AUDIO_FORMAT_WMA_PRO,

   /* JPEG, PNG, GIF and TIFF */
   PHOTO_FORMAT_IMAGE_JPEG,
   PHOTO_FORMAT_IMAGE_PNG,
   PHOTO_FORMAT_IMAGE_GIF,
   PHOTO_FORMAT_IMAGE_TIFF,

   /* Video MPEG-1 (mpeg1video), MPEG-2 (mpeg2video), etc */
   VIDEO_FORMAT_MPEG1,
   VIDEO_FORMAT_MPEG2,
   VIDEO_FORMAT_H263,
   VIDEO_FORMAT_MPEG4Part2,
   VIDEO_FORMAT_MPEG4Part10,
   VIDEO_FORMAT_WMV9,
   VIDEO_FORMAT_VC1,

} ITEM_FORMAT;


/** Item validation function - checks if the item complies with a DLNA profile */
typedef int (*item_validation_func)( void * );

/** Returns a DIDL description of the item. */
typedef char *(*item_to_didl_func)();

/**
 * Generic base item information structure, valid for audio, video, photos.
 * TOFIX: implement playlist item types
 */
typedef struct item_info {

   ITEM_TYPE type;
   ITEM_CLASS class;
   ITEM_FORMAT format;
   ITEM_ID id;

   DLNA_ORG_PN profile;

	char *filename;
   char *filepath;

   /* Resource encoding ("res@") properties */
   int64_t size;
   int64_t duration;
   int bitrate;
   int sampleFrequency;
   /* int bitsPerSample */
   int nrAudioChannels;
   int width;
   int height;
   int colorDepth; /* Not sure how to get this one... */
   /* char *protection; */
   char is_valid;

   /* The FFMpeg format context and the indexes for the audio/video streams */
   AVFormatContext *format_context;
   int audio_stream_idx;
   int video_stream_idx;

   /** Validation function */
   item_validation_func validate;

   /** DIDL-ization function. */
   item_to_didl_func to_didl;

   /* 
    * Pointer to a specific information structure for the item. 
    * E.g. in case of a music track, this will point to a musicTrack_info structure
    */
   void *specific_info;

} item_info;

/**
 * Returns an item information structure
 *
 * @param filename stream file name
 * @param item_info an item info structure to be allocated
 *
 * @return 0 if successful or error code otherwise. 
 */
DLLEXPORT
int item_getinfo( char *filename, item_info **item );

/**
 * Frees up an item information structure
 *
 * @param item_info item_info description structure
 *
 */
DLLEXPORT
void item_freeinfo( item_info *item );

/**
 * Helper function to call the validation function
 *
 * @param item_info item_info description structure
 * @return 1 if successful or DLNA_INVALID_STREAM otherwise. 
 *
 */
inline
DLLEXPORT 
int item_validate( item_info *item )
{
   return item->validate( item );
};

/**
 * Helper function
 *
 * @param item_info item_info description structure
 * @return 1 if the item contains an audio track. 
 */
inline
DLLEXPORT
int item_is_audio( item_info *item )
{
   return (item->type & ITEM_AUDIO) != 0;
};

/**
 * Helper function
 *
 * @param item_info item_info description structure
 * @return 1 if the item contains a video track. 
 */
inline
DLLEXPORT
int item_is_video( item_info *item )
{
   return (item->type & ITEM_VIDEO) != 0;
};

/**
 * Helper function
 *
 * @param item_info item_info description structure
 * @return 1 if the item contains a video and an audio track. 
 */
inline
DLLEXPORT
int item_is_audio_video( item_info *item )
{
   return ( item_is_video(item) & item_is_audio(item) );
};

inline
DLLEXPORT
int item_is_photo( item_info *item )
{
   return (item->type & ITEM_PHOTO) != 0;
};


#ifdef __cplusplus
}
#endif

#endif __ITEM_H