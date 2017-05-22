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
#include "lpcm.h"

/**
 * Verify the mp3 to be compliant with the DLNA spec
 *
 * @param mp3_info mp3 info structure
 *
 * @return DLNA_STREAM_NOT_VALID if not valid, any other value otherwise
 */
int lpcm_validate( item_info *info )
{
   AVCodecContext *ac = info->format_context->streams[info->audio_stream_idx]->codec;

   if( ac == NULL || ac->codec_type != CODEC_TYPE_AUDIO ) 
   {
      return DLNA_INVALID_STREAM;
   }

   /* mime audio/L16 is allowed, thus using 16-bit signed 
      representation in network byte order  */
   if( ac->codec_id != CODEC_ID_PCM_S16BE &&
       ac->codec_id != CODEC_ID_PCM_S16LE )
   {
      return DLNA_INVALID_STREAM;
   }
  
   /* mono and stereo only */
   if( ac->channels > 2 ) 
   {
      return DLNA_INVALID_STREAM;
   }

   /* 16-bit signed sample format */
   if( ac->sample_fmt != SAMPLE_FMT_S16 )
   {
      return DLNA_INVALID_STREAM;
   }

   if( ac->sample_rate < 8000 || ac->sample_rate > 48000 )
   {
      return DLNA_INVALID_STREAM;
   }
  
   return 1;
} /* mp3_validate */
