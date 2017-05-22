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


/*
 * Internal definitions that do not need to be exported
 */


#ifndef __INTERNALS_H
#define __INTERNALS_H


/** DLNA Play Speed (DLNA.ORG_PS) values. */
typedef enum 
{
  /* 
   * This is actually not allowed to be used as a
   * value for the MM protocolInfo field,
   * see DLNA Requirement [7.3.35.1]:
   * [...] If the media transport component
   * (either for a server, client) does not support 
   * additional server-side play speeds beyond "1" 
   * for the context of the protocolInfo, then [...] 
   * (i.e. "DLNA.ORG_PS=1" is prohibited).
   */
   DLNA_ORG_PLAY_SPEED_NORMAL = 1
} DLNA_ORG_PS;

/** DLNA Conversion (DLNA.ORG_CI) flags. */
typedef enum 
{
   DLNA_CONVERSION_NONE       = 0,
   DLNA_CONVERSION_TRANSCODED = 1
} DLNA_ORG_CI;

/** DLNA Operation (DLNA.ORG_OP) flags. */
typedef enum 
{
   DLNA_OPERATION_NONE     = 0x00,
   DLNA_OPERATION_RANGE    = 0x01,
   DLNA_OPERATION_TIMESEEK = 0x10
} DLNA_ORG_OP;

/** DLNA Flags (DLNA.ORG_FLAGS). */
typedef enum 
{
   DLNA_FLAG_SENDER_PACED              = (1 << 31),
   DLNA_FLAG_TIME_BASED_SEEK           = (1 << 30),
   DLNA_FLAG_BYTE_BASED_SEEK           = (1 << 29),
   DLNA_FLAG_PLAY_CONTAINER            = (1 << 28),
   DLNA_FLAG_S0_INCREASE               = (1 << 27),
   DLNA_FLAG_SN_INCREASE               = (1 << 26),
   DLNA_FLAG_RTSP_PAUSE                = (1 << 25),
   DLNA_FLAG_STREAMING_TRANSFER_MODE   = (1 << 24),
   DLNA_FLAG_INTERACTIVE_TRANSFER_MODE = (1 << 23),
   DLNA_FLAG_BACKGROUND_TRANSFER_MODE  = (1 << 22),
   DLNA_FLAG_CONNECTION_STALL          = (1 << 21),
   DLNA_FLAG_DLNA_V15                  = (1 << 20)
} DLNA_ORG_FLAGS;



#endif