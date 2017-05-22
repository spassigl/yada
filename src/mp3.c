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
#include "mp3.h"

/**
 * Verify the mp3 to be compliant with the DLNA spec
 *
 * @param info mp3 item_info structure
 *
 * @return DLNA_STREAM_NOT_VALID if not valid, any other value otherwise
 */
int mp3_validate( item_info *info )
{
   AVCodecContext *ac = info->format_context->streams[info->audio_stream_idx]->codec;

   /* mono and stereo only */
   if( ac->channels > 2 ) 
   {
      return DLNA_INVALID_STREAM;
   }

   /* Allowed frequencies: 32000, 41000, 48000 */
   if( ac->sample_rate != 32000 && 
       ac->sample_rate != 44100 && 
       ac->sample_rate != 48000 )
   {
      return DLNA_INVALID_STREAM;
   }
  
   /* Allowed bit rate: 32000,40000,48000,56000,64000,
                        80000,96000,112000,128000,160000,
                        192000,224000,256000,320000 */
   if( ac->bit_rate != 32000  && 
       ac->bit_rate != 40000  && 
       ac->bit_rate != 48000  && 
       ac->bit_rate != 56000  && 
       ac->bit_rate != 64000  && 
       ac->bit_rate != 80000  && 
       ac->bit_rate != 96000  && 
       ac->bit_rate != 112000 && 
       ac->bit_rate != 128000 && 
       ac->bit_rate != 160000 && 
       ac->bit_rate != 192000 && 
       ac->bit_rate != 224000 && 
       ac->bit_rate != 256000 && 
       ac->bit_rate != 320000 )
   {
      return DLNA_INVALID_STREAM;
   }
  
   return 1;
} /* mp3_validate */
