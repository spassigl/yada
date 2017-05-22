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

#ifndef __LOGGER_H
#define __LOGGER_H

#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Log level
 */
enum
{
   LOG_DISABLED = 0,
   LOG_ERROR = 1,
   LOG_WARN = 2,
   LOG_INFO = 3,
   LOG_DEBUG = 4,
   LOG_TRACE = 5,
   LOG_MIN = LOG_ERROR,
   LOG_MAX = LOG_TRACE
};

#define LOG_MSG( msg ) "["__FUNCTION__"] " msg

/**
 * Library log function
 *
 * Initialization routine
 */
DLLEXPORT
void logger_init();

/**
 * Library log function
 *
 * @param filename The log file
 */
DLLEXPORT
void logger_set_log_file( char *filename );

/**
 * Set library log level
 *
 * @param level Either LOG_NONE or a value between LOG_MIN and LOG_MAX
 *
 */
DLLEXPORT
void logger_set_log_level( int level );

/**
 * Library log function. Logs strings either on console or in the
 * file set by DLNA_set_log_file
 *
 * @param level Either LOG_NONE or a value between LOG_MIN and LOG_MAX
 * @param fmt Format string (a la printf format string)
 */
DLLEXPORT
void logger_log( int level, char *fmt, ... );

/**
 * Library log function
 */
DLLEXPORT
void logger_close();


#ifdef __cplusplus
}
#endif

#endif