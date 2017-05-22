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


#ifndef __DLNA_H
#define __DLNA_H

#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Error codes
 */
enum
{
   DLNA_SUCCESS = 0,
   DLNA_INIT_ERROR = -1,
   DLNA_INVALID_STREAM = -2,
   DLNA_MEMORY_ERROR = -3,
   DLNA_SHARE_ERROR = -4,
   DLNA_ERROR = -99
};

/**
 * Initialize the YADA DLNA server.
 *
 * @param init_param Initialization parameters. Can be NULL, and/or
 *                   members can be NULL or 0. This is the behaviour in 
 *                   this case:
 *                   - if ip_address is NULL, the first IP address will be
 *                     chosen
 *                   - if port is 0 (zero), the first available port will be
 *                     used
 *
 * @return DLNA_SUCCESS if successful or DLNA_INIT_ERR
 */
DLLEXPORT
int yada_init( char *config_file );


/**
 * Terminates the DLNA library.
 *
 */
DLLEXPORT
void yada_shutdown();

/**
 * Re-initialize the DLNA library.
 *
 * @param init_param Initialization parameters
 *
 * @return DLNA_SUCCESS if successful or DLNA_INIT_ERR
 */
DLLEXPORT
int yada_reinit( char *config_file );


/**
 * Shares a media file.
 *
 * @param file Path to the file to share
 *
 * @return DLNA_SUCCESS if successful or DLNA_SHARE_ERROR otherwise
 */
DLLEXPORT
int yada_share_file( char *file );

/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif __DLNA_H
