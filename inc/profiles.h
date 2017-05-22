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

#ifndef __PROFILES_H
#define __PROFILES_H

#include "mime.h"

/** 
 * DLNA Profile (DLNA_ORG.PN) categories. 
 * Actual DLNA_ORG.PN values will correspond
 * to the stringification of the enum values.
 *
 * Still to be fully compliant with DLNA 1.5 specs...
 */

/*
 * Do NOT reorder, just add to the end.
 * If you reorder, please reorder things in 
 * profiles.c as well!
 */
typedef enum
{
   PN_INVALID,

   /* JPEG profiles. */
   JPEG_SM,
   JPEG_MED,
   JPEG_LRG,
   JPEG_TN,
   JPEG_SM_ICO,
   JPEG_LRG_ICO,

   /* PNG profiles. */
   PNG_TN,
   PNG_SM_ICO,
   PNG_LRG_ICO,
   PNG_LRG,

   /* AC-3 profiles. */
   AC3,

   /* AMR profiles. */
   AMR_3GPP,
   AMR_WBplus,

   /* ATRAC3plus rofiles. */
   ATRAC3plus,

   /* LPCM profiles. */
   LPCM,

   /* MP3 profiles. */
   MP3,
   MP3X,

   /* MPEG1 profiles. */
   MPEG1,

   /* MPEG2 profiles. */
   MPEG_PS_NTSC,
   MPEG_PS_NTSC_XAC3,
   MPEG_PS_PAL,
   MPEG_PS_PAL_XAC3,
   MPEG_TS_MP_LL_AAC,
   MPEG_TS_MP_LL_AAC_T,
   MPEG_TS_MP_LL_AAC_ISO,
   MPEG_TS_SD_EU,
   MPEG_TS_SD_EU_T,
   MPEG_TS_SD_EU_ISO,
   MPEG_TS_SD_NA,
   MPEG_TS_SD_NA_T,
   MPEG_TS_SD_NA_ISO,
   MPEG_TS_SD_NA_XAC3,
   MPEG_TS_SD_NA_XAC3_T,
   MPEG_TS_SD_NA_XAC3_ISO,
   MPEG_TS_HD_NA,
   MPEG_TS_HD_NA_T,
   MPEG_TS_HD_NA_ISO,
   MPEG_TS_HD_NA_XAC3,
   MPEG_TS_HD_NA_XAC3_T,
   MPEG_TS_HD_NA_XAC3_ISO,
   MPEG_ES_PAL, 
   MPEG_ES_NTSC,
   MPEG_ES_PAL_XAC3,
   MPEG_ES_NTSC_XAC3,

   PN_LAST 

} DLNA_ORG_PN;

/**
 * Returns a string representation of the 
 * profile ID.
 *
 * @param pn Profile ID (one of the DLNA_ORG_PN values).
 * @return A pointer to a static buffer containing
 *    the string value of the profile, or NULL if the
 *    profile ID is not valid.
 */
char *profile_tostring( DLNA_ORG_PN pn );

MIME_TYPE profile_to_mime( DLNA_ORG_PN pn );

#endif