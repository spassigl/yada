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

#include "logger.h"

#include "yada.h"
#include "item.h"
#include "musicTrack.h"
#include "mp3.h"
#include "lpcm.h"


/**
 * Probes an audio track profile
 */
static 
int musicTrack_probeFormat( item_info *track_info );


/**
 * Returns a musicTrack information structure
 *
 * @param filename stream file name
 * @param item_info A item_info structure
 * @param track_info A musicTrack_info info structure to be allocated
 *
 * @return DLNA_NO_ERROR if successful or error code otherwise
 */
int musicTrack_getinfo( char *filename, item_info *item, musicTrack_info **track_info )
{
   musicTrack_info *mti = NULL;
   int rc = 0;

   /* Probe the profile for this track */
   ITEM_FORMAT format = musicTrack_probeFormat( item );

   /* Set the validation function for the item. If it is already found not NULL, 
    * means this is an audio track embedded in a video. */
   if( item->validate == NULL )
   {
      switch( format )
      {
         case AUDIO_FORMAT_MP3:
            item->validate = mp3_validate;
            break;
         case AUDIO_FORMAT_LPCM:
            item->validate = lpcm_validate;
            break;
         case AUDIO_FORMAT_WMA:
         case AUDIO_FORMAT_WMA_PRO:
            break;
         default:
            /* This is not an audio item! */
            item->is_valid = 0;
            return DLNA_ERROR;
      }
   }

   /* Time to allocate the musicTrack_info structure and fill it in */
   mti = calloc( 1, sizeof(musicTrack_info) );
   if( mti == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("could not allocate musicTrack_info") );      
      return DLNA_ERROR;
   }

   /* Set the specific infomation for this music track */
   mti->audio_format = format;
   mti->artist = item->format_context->author;
   mti->album = item->format_context->album;
   mti->title = item->format_context->title;
   mti->originalTrackNumber = item->format_context->track;
   mti->genre = item->format_context->genre;
   
   *track_info = mti;

   return DLNA_SUCCESS;
} /* musicTrack_getinfo */

/**
 * Cleans up a previously allocated musicTrack_info structure
 *
 * @param track_info musicTrack_info structure
 *
 */
void musicTrack_freeinfo( musicTrack_info *track_info )
{
   if( track_info != NULL )
   {
      free( track_info );
   }
} /* musicTrack_freeinfo */

/*****************************************************************************/

static
ITEM_FORMAT musicTrack_probeFormat( item_info *item )
{
   return AUDIO_FORMAT_MP3;
}