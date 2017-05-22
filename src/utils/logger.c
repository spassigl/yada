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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "logger.h"
#include "pthread.h"

static pthread_mutex_t g_log_mutex;

/*
 * Log-management related global variables
 */
static int g_log_initialized = 0;
static int g_loglevel = LOG_MIN;
static FILE *g_logfile = NULL;


/**
 * Library log function
 *
 * Initialization routine
 */
void logger_init()
{
   if( g_log_initialized == 0 )
   {
      g_logfile = stdout;
      pthread_mutex_init( &g_log_mutex, NULL );

      g_log_initialized = 1;
   }
}; /* logger_init */

/**
 * Set log level
 *
 * @param level Either LOG_DISABLED or a value between LOG_ERROR and LOG_TRACE
 */
void logger_set_log_level( int level )
{
   if( level == LOG_DISABLED ) g_loglevel = LOG_DISABLED;
   else if( level < LOG_MIN ) g_loglevel = LOG_MIN;
   else if( level > LOG_MAX ) g_loglevel = LOG_MAX;
   else g_loglevel = level;
} /* logger_set_log_level */


/**
 * Library log function
 *
 * @param filename The log file
 */
void logger_set_log_file( char *filename )
{
   pthread_mutex_lock( &g_log_mutex );

   if( g_logfile != stdout )
   {
      fflush( g_logfile );
      fclose( g_logfile );
   }
   if( !(g_logfile = fopen(filename, "a")) ) 
   {    
      fprintf( stderr, LOG_MSG("ERROR - failed to open \"%s\" for logging, reverting to stdout\n"), filename );
      g_logfile = stdout;
   }
      
   pthread_mutex_unlock( &g_log_mutex );
} /* logger_set_log_file */


/**
 * Library log function
 *
 * @param level A value between LOG_MIN and LOG_MAX
 * @param fmt Format string (a la printf format string)
 */
void logger_log( int level, char *fmt, ... )
{  
   if( g_log_initialized == 0 )
   {
      fprintf( stderr, LOG_MSG("ERROR - logger not initialized\n") );
      return;
   }
   
   if( (level > LOG_DISABLED) && (level <= g_loglevel) )
   {
      va_list arglist;
      time_t curr_time;
      struct tm *t;
      char timestr[26];
      static char *error_level_string[] = { "ERROR", "WARN ", "INFO ", "DEBUG", "TRACE" };

      pthread_mutex_lock( &g_log_mutex );
 
      /* Compose time string */
      time( &curr_time );
      t = localtime( &curr_time );
      strcpy( timestr, asctime(t) );
      timestr[24] = 0; /* Get rid of trailing \n\0 characters returned by asctime */

      /* Print out the log line... */

      fprintf( g_logfile, "%s [YADL] %s - ", timestr, error_level_string[level-1] );

      va_start( arglist, fmt );
      vfprintf( g_logfile, fmt, arglist );
      va_end( arglist );

      /* ...with a trailing new line */
      fprintf( g_logfile, "\n" );

      pthread_mutex_unlock( &g_log_mutex );
   }
} /* logger_log */


/**
 * Library log function
 */
void logger_close()
{
   if( (g_logfile != NULL) && (g_logfile != stdout) )
   {
      pthread_mutex_lock( &g_log_mutex );
      fflush( g_logfile );
      fclose( g_logfile );
      g_logfile = NULL;
      g_log_initialized = 0;
      pthread_mutex_unlock( &g_log_mutex );

      pthread_mutex_destroy( &g_log_mutex );
   }
} /* logger_close */

