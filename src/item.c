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

#include "yada.h"

#include "item.h"
#include "musicTrack.h"
#include "videoItem.h"
#include "photo.h"

#include "md5utils.h"
#include "logger.h"

#define IS_PHOTO(stream)   stream->codec->codec_id == CODEC_ID_LJPEG || \
                           stream->codec->codec_id == CODEC_ID_JPEGLS || \
                           stream->codec->codec_id == CODEC_ID_MJPEG || \
                           stream->codec->codec_id == CODEC_ID_MJPEGB || \
                           stream->codec->codec_id == CODEC_ID_PNG || \
                           stream->codec->codec_id == CODEC_ID_GIF || \
                           stream->codec->codec_id == CODEC_ID_TIFF



/**
 * Returns an item information structure
 *
 * @param filename stream file name
 * @param item_info an item info structure to be allocated
 *
 * @return 0 if successful or error code otherwise. 
 */
int item_getinfo( char *filename, item_info **item )
{
   int rc = 0;
   unsigned int idx;
   item_info *ii = NULL;
   AVFormatContext *avcontext;

   logger_log( LOG_TRACE, LOG_MSG("file name: %s"), filename );

   if( (rc = av_open_input_file(&avcontext, filename, NULL, 0, NULL)) < 0 )
   {
      logger_log( LOG_ERROR, LOG_MSG("av_open_input_file failed with return code = %d"), rc );
      return DLNA_INVALID_STREAM;
   }
   if( (rc = av_find_stream_info(avcontext)) < 0 )
   {
      logger_log( LOG_ERROR, LOG_MSG("av_find_stream_info failed with return code = %d"), rc );
      return DLNA_INVALID_STREAM;
   }

#ifdef DLNADEBUG
   dump_format( avcontext, 0, filename, 0 );
#endif

   ii = calloc( 1, sizeof(item_info) );
   if( ii == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("could not allocate item_info") );
      return DLNA_ERROR;
   }
   
   /* Set the format context */
   ii->format_context = avcontext;
   ii->audio_stream_idx = -1;
   ii->video_stream_idx = -1;

   logger_log( LOG_TRACE, LOG_MSG("MD5 hash calculation start...") );

   /* Compute MD5 hash. */
   if( md5_file_digest(ii->id, filename) == 0 )
   {
      return DLNA_INVALID_STREAM;
   }

   /* We can assume it's a valid stream for now, it will be 
    * definitely set by the specific functions. */
   ii->is_valid = 1; 

   /** Walk through the streams and get specific information about them. */
   for( idx = 0; idx < avcontext->nb_streams; idx++ )
   {   
      AVStream *stream;
      enum CodecID codec_type;

      stream = avcontext->streams[idx];
      codec_type = stream->codec->codec_type;

      if( codec_type == CODEC_TYPE_AUDIO )
      {
         musicTrack_info *track_info;

         logger_log( LOG_TRACE, LOG_MSG("item is audio") );

         ii->audio_stream_idx = idx;
         if( musicTrack_getinfo( filename, ii, &track_info ) == DLNA_SUCCESS )
         {
            if( ii->type & ITEM_VIDEO )
            {
               ii->type = ITEM_AUDIOVIDEO;
            }
            else
            {
               ii->type = ITEM_AUDIO;
            }
            ii->class = DLNA_MUSICTRACK_ITEM_CLASS;
            ii->specific_info = track_info;
         }
         else 
         {
            free( ii );
            return DLNA_INVALID_STREAM;
         }
      } /* if( codec_type == CODEC_TYPE_AUDIO ) */
      else
      if( codec_type == CODEC_TYPE_VIDEO )
      {
         /* Is it a photo? */
         if( IS_PHOTO(stream) ) 
         {
            photo_info *photo_info;
         
            logger_log( LOG_TRACE, LOG_MSG("item is a photo") );

            ii->video_stream_idx = idx;
            if( photo_getinfo( filename, ii, &photo_info ) == DLNA_SUCCESS )
            {
               ii->type = ITEM_PHOTO;
               ii->class = DLNA_PHOTO_ITEM_CLASS;
               ii->specific_info = photo_info;
            }
            else 
            {
               free( ii );
               return DLNA_INVALID_STREAM;
            }
         }
         else
         {
            /* It must be a video then */
            videoItem_info *video_info;

            logger_log( LOG_TRACE, LOG_MSG("item is a video") );

            ii->video_stream_idx = idx;
            if( video_getinfo( filename, ii, &video_info ) == DLNA_SUCCESS )
            {
               if( ii->type & ITEM_AUDIO )
               {
                  ii->type = ITEM_AUDIOVIDEO;
               }
               else
               {
                  ii->type = ITEM_VIDEO;
               }
               ii->class = DLNA_VIDEO_ITEM_CLASS;
               ii->specific_info = video_info;
            }
            else 
            {
               free( ii );
               return DLNA_INVALID_STREAM;
            }
         }

         ii->width = stream->codec->width;
         ii->height = stream->codec->height;
         ii->duration = avcontext->duration;
      } /* if( codec_type == CODEC_TYPE_VIDEO ) */

   } /* for loop */

   /* Set common stream information */
   ii->filename = strdup( filename );
   ii->size = avcontext->file_size;
   ii->bitrate = avcontext->bit_rate;

   *item = ii;

   return DLNA_SUCCESS;
} /* item_getinfo */

/**
 * Frees up an item information structure
 *
 * @param item_info item_info description structure
 *
 */
void item_freeinfo( item_info *item )
{
   if( item != NULL )
   {
      av_close_input_stream( item->format_context );
      free( item->filename );
      free( item );
   }
} /* item_freeinfo */
