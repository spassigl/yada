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

#include <malloc.h>
#include <string.h>

/* Under Win32, define inline to include ffmpeg headers */
#ifdef WIN32
#define inline _inline
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#include "logger.h"
#include "yada.h"
#include "videoItem.h"
#include "mp1video.h"
#include "mp2video.h"


/**
 * Probes a video item profile
 */
static
int videoItem_probeFormat( void * );

/**
 * Probes a video system profile
 */
static
VIDEO_SYSTEM videoItem_probeSystem( item_info *item );

/**
 * Returns a videoItem_info information structure
 *
 * @param filename stream file name
 * @param item_info A item_info structure
 * @param video_info A videoItem_info info structure to be allocated
 *
 * @return DLNA_NO_ERROR if successful or error code otherwise
 */
int video_getinfo( char *filename, item_info *item, videoItem_info **video_info )
{
   videoItem_info *vii = NULL;
   int rc = 0;

   /* Probe the profile for this track */
   ITEM_FORMAT format = videoItem_probeFormat( item );

   /* Set the validation function for the item */
   switch( format )
   {
      case VIDEO_FORMAT_MPEG1:
         item->validate = mp1video_validate;
         break;
      case VIDEO_FORMAT_MPEG2:
         item->validate = mp2video_validate;
         break;
      default:
         /* This is not an audio item! */
         item->is_valid = 0;
         return DLNA_ERROR;
   }

   /* Time to allocate the videoItem_info structure and fill it in */
   vii = calloc( 1, sizeof(videoItem_info) );
   if( vii == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("could not allocate musicTrack_info") );      
      return DLNA_ERROR;
   }

   /* Set the specific infomation for this video track */
   vii->video_format = format;
   vii->video_system = videoItem_probeSystem( item );

   *video_info = vii;

   return DLNA_SUCCESS;
} /* video_getinfo */


/**
 * Cleans up a previously allocated photo_info structure
 *
 * @param photo_info photo_info structure
 *
 */
void video_freeinfo( videoItem_info **video_info )
{
   if( video_info != NULL )
   {
      free( video_info );
   }
} /* video_freeinfo */

/*****************************************************************************/

static
ITEM_FORMAT videoItem_probeFormat( item_info *item )
{
   switch( item->format_context->streams[item->video_stream_idx]->codec->codec_id )
   {
      case CODEC_ID_MPEG1VIDEO:
        return VIDEO_FORMAT_MPEG1;
         break;
      case CODEC_ID_MPEG2VIDEO:
         return VIDEO_FORMAT_MPEG2;
         break;
      default:
         /*
         logger_log( LOG_ERROR, LOG_MSG("File %s format not accepted (codec %d)"), item_info->filename, item_info->video_stream_idx]->codec->codec_id );
         */
         return -1;
   };
} /* videoItem_probeProfile */

static
VIDEO_SYSTEM videoItem_probeSystem( item_info *item )
{
   AVStream *stream = item->format_context->streams[item->video_stream_idx];

   if( (stream->r_frame_rate.num == 25) && (stream->r_frame_rate.den == 1) ) 
   {
      return SYSTEM_PAL;
   }
   else
   if( (stream->r_frame_rate.num == 30000) && (stream->r_frame_rate.den == 1001) ) 
   {
      return SYSTEM_NTSC;
   }
   else
   {
      logger_log( LOG_ERROR, LOG_MSG("File: %s system unknown (frame rate: %.2f)"), 
                                item->filename, stream->r_frame_rate.num/stream->r_frame_rate.den );
      return SYSTEM_UNKNOWN;
   }
} /* videoItem_probeSystem */