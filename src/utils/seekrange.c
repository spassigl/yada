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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "seekrange.h"



/*----------------------------------------------------------------------------
 *
 * Normal Play Time
 *
 *--------------------------------------------------------------------------*/


/**
 * Parse a string into a npt_time structure.
 *
 * @param npt_string A null-terminated string
 *    with a time in the npt format. 
 *    If set to "now" it will just set the NPT_TYPE to NPT_NOW 
 *    and the contents of the other fields are undefined.
 *    If set to "*"  it will just set the NPT_TYPE to NPT_UNKNOWN
 *    and the contents of the other fields are undefined.
 * @param npt A pointer to a structure that will 
 *    receive the npt_time details.
 * @return 1 if successful, 0 otherwise.
 */
int npt_parse( const char *npt_string, npt_time *npt )
{
   int res;

   if( npt_string == NULL || npt == NULL )
   {
      return 0;
   }

   if( npt_string[0] == '*' )
   {
      npt->type = NPT_UNKNOWN;
   }
   else
   if( strncmp( npt_string, "now", 3 ) == 0 )
   {
      npt->type = NPT_NOW;
   }
   else
   if( strchr( npt_string, (int)':' ) == NULL )
   {
      /* This is an npt sec time representation. */
      if( strchr( npt_string, (int)'.' ) != NULL )
      {
         npt->type = NPT_SEC_FULL;
         res = ( sscanf( npt_string, "%u.%u", &npt->secs.sec_hi, &npt->secs.sec_lo ) < 2 ) ? 0 : 1;
      }
      else
      {
         npt->type = NPT_SEC;
         res = ( sscanf( npt_string, "%u", &npt->secs.sec_hi ) < 1 ) ? 0 : 1;
      }

      /* Sanity check. */
      if( res == 0 )
      {
         npt->type = NPT_INVALID;
         return 0;
      }
   }
   else
   {
      /* This is an npt hhmmss time representation. */
      if( strchr( npt_string, (int)'.' ) != NULL )
      {
         npt->type = NPT_HHMMSS_FULL;
         res = ( sscanf( npt_string, "%u:%u:%u.%u", &npt->hhmmss.hh, &npt->hhmmss.mm, &npt->hhmmss.ss, &npt->hhmmss.low ) < 4 ) ? 0 : 1;
      }
      else
      {
         npt->type = NPT_HHMMSS;
         res = ( sscanf( npt_string, "%u:%u:%u", &npt->hhmmss.hh, &npt->hhmmss.mm, &npt->hhmmss.ss ) < 3 ) ? 0 : 1;
      }

      /* Sanity check. */
      if( res == 0 )
      {
         npt->type = NPT_INVALID;
         return 0;
      }

      /* Further sanity check. */
      if( npt->hhmmss.mm > 59 || npt->hhmmss.ss > 59 )
      {
         npt->type = NPT_INVALID;
         return 0;
      }
   }

   return 1;
} /* npt_parse */

/**
 * Returns a string representation of the npt_time.
 *
 * @param npt The npt time structure to translate into a string.
 *    If NULL, or type is NPT_INVALID, the function will return NULL.
 *    If type is NPT_NOW, the function will return "now".
 *    If type is NPT_UNKNOWN, the function will return "*".
 * @buf The buffer that will store the string representation
 *    of the npt_time. If set to NULL the function will allocate
 *    a new buffer that will store the string.
 * @param buf_len Size of the buffer. Ignored if buf is NULL.
 *    If set to 0 (zero) and buf is not NULL, the function will return NULL.
 *    If buffer length is not enough to store the converted string,
 *    the function will return NULL.
 * @return A pointer to buf.
 */
char *npt_tostring( const npt_time *npt, char *buf, unsigned int buf_len )
{
   char nptstring[40]; /* This should be a reasonable length! */

   if ( npt == NULL )
   {
      return NULL;
   }

   if( (buf != NULL) && (buf_len == 0) )
   {
      return NULL;
   }

   switch( npt->type )
   {
      case NPT_INVALID:
         return NULL;

      case NPT_UNKNOWN:
         strcpy( nptstring, "*" );
         break;

      case NPT_NOW:
         strcpy( nptstring, "now" );
         break;

      case NPT_SEC:
         sprintf( nptstring, "%u", npt->secs.sec_hi );
         break;

      case NPT_SEC_FULL:
         sprintf( nptstring, "%u.%u", npt->secs.sec_hi, npt->secs.sec_lo );
         break;

      case NPT_HHMMSS:
         sprintf( nptstring, "%u.%02u.%02u", npt->hhmmss.hh, npt->hhmmss.mm, npt->hhmmss.ss );
         break;

      case NPT_HHMMSS_FULL:
         sprintf( nptstring, "%u.%02u.%02u.%u", npt->hhmmss.hh, npt->hhmmss.mm, npt->hhmmss.ss, npt->hhmmss.low );
         break;

      default:
         return NULL;
   }

   if( buf == NULL )
   {
      buf = strdup( nptstring );
   }
   else
   {
      if( buf_len < strlen( nptstring ) )
         return NULL;
      else
         strcpy( buf, nptstring );
   }

   return buf;
} /* npt_tostring */


/*----------------------------------------------------------------------------
 *
 * Bytes Range
 *
 *--------------------------------------------------------------------------*/

/**
 * Parse a string into a bytes_range structure.
 *
 * @param bytesrange_string A null-terminated string
 *    in the form of a bytes range specifier
 * @param br A pointer to a structure that will receive 
 *    the timeseek_range details.
 * @return 1 if successful, 0 otherwise.
 */
int bytesrange_parse( const char *bytesrange_string, bytes_range *br )
{
   /* Look for the "bytes=" string. Be case sensitive. */
   if( strncmp(bytesrange_string, "bytes=", 6) != 0 )
   {
      br->type = BR_INVALID;
      return 0;
   }

   /* 
    * Read the byte range specifiers and make sure the dash
    * is in the string.
    */
   if( sscanf( bytesrange_string+6, "%u-%u", &br->first, &br->last ) < 2 )
   {
      char dash;

      /* That did not work, is maybe a BR_OPEN type? */
      if( (sscanf( bytesrange_string+6, "%u%c", &br->first, &dash ) < 2) || (dash != '-') )
      {
         br->type = BR_INVALID;
         return 0;
      }
      else
      {
         br->type = BR_OPEN;
      }
   }
   else
   {
      br->type = BR_CLOSED;
   }

   return 1;
   
} /* bytesrange_parse */

/**
 * Returns a string representation of the bytes_range.
 *
 * @param tsr The timeseek_range structure to translate into a string.
 * @buf The buffer that will store the string representation
 *    of the bytes range. If set to NULL the function will allocate
 *    a new buffer that will store the string.
 * @param buf_len Size of the buffer. Ignored if buf is NULL.
 * @return A pointer to buf.
 */
char *bytesrange_tostring( const bytes_range *br, char *buf, unsigned int buf_len )
{
   char brstring[80]; /* 80 should be a reasonable length! */

   if ( br == NULL )
   {
      return NULL;
   }

   if( (buf != NULL) && (buf_len == 0) )
   {
      return NULL;
   }

   switch( br->type )
   {
      case BR_INVALID:
         return NULL;

      case BR_OPEN:
         sprintf( brstring, "bytes=%u-", br->first );
         break;

      case BR_CLOSED:
         sprintf( brstring, "bytes=%u-%u", br->last );
         break;
   }

   if( buf == NULL )
   {
      buf = strdup( brstring );
   }
   else
   {
      if( buf_len < strlen( brstring ) )
         return NULL;
      else
         strcpy( buf, brstring );
   }

   return buf;
} /* bytesrange_tostring */


/*----------------------------------------------------------------------------
 *
 * Timeseek Range
 *
 *--------------------------------------------------------------------------*/


/**
 * Parse a string into a timeseek_range structure.
 *
 * @param timeseek_string A null-terminated string
 *    in the form of a timeseek range specifier
 * @param tsr A pointer to a structure that will 
 *    receive the timeseek_range details.
 * @return 1 if successful, 0 otherwise.
 */
int timeseek_parse( const char *timeseek_string, timeseek_range *tsr )
{
   int res;
   char *bytes;
   char *minus;
   char *npt;
   char nptstart[80];

   /* Look for the "npt=" string. Be case sensitive. */
   if( strstr(timeseek_string, "npt=") == NULL )
   {
      tsr->type = TSR_INVALID;
      return 0;
   }

   /* 
    * Look for a bytes-range first. This way we know 
    * better how to parse the rest of the string. 
    */
   bytes = strstr( timeseek_string, "bytes=" );

   /* Now look for a '-' character. There must be
    * at least one belonging to the npt range specifier.
    * If no '-' is present the string is malformed and
    * therefore we return an error.
    */
   minus = strchr( timeseek_string, (int)'-' );
   if( minus == NULL )
   {
      tsr->type = TSR_INVALID;
      return 0;
   }

   /* Ok we have a '-'. Be sure is not the one 
    * belonging to the bytes-range specifier.
    * In this case we would have a malformed string
    * again, something like:
    *    npt=310.1 bytes=1234-5678
    */
   if( (bytes != NULL) && (minus >= bytes) )
   {
      tsr->type = TSR_INVALID;
      return 0;
   }

   /* 
    * Let's parse the first npt string now. Null-terminate where 
    * the minus is. This is to make sure npt_parse does not fail 
    * in case we are facing a mixed npt range (npt sec and npt hhmmss)
    * which is not recommended by the DLNA specs, but never say never...
    */
   strncpy( nptstart, timeseek_string+4, minus-timeseek_string-4 );
   nptstart[minus-timeseek_string-4] = 0;
   res = npt_parse( nptstart, &tsr->npt_start );
   if( res == 0 )
   {
      tsr->type = TSR_INVALID;
      return 0;
   }

   /* 
    * If the next character after the '-' is either
    * NULL, a whitespace or a '/' or any other character 
    * which is not a digit, there is no npt end range
    * specifier. 
    */
   npt = minus+1;

   if( *npt == 0 )
   {
      /* We're done. */
      return 1;
   }
   else
   if( isdigit(*npt) )
   {
      /* Try to parse the range end specifier. */
      res = npt_parse( npt, &tsr->npt_end );
      if( res == 0 )
      {
         /* 
          * npt_end is misspelled. Abort
          * parsing and return an error.
          */
         tsr->type = TSR_INVALID;
         return 0;
      }
      else
      {
         /* Assume there's no instance-duration specified for now. */
         if( bytes != NULL )
         {
            tsr->type = TSR_NPT_NPT_BYTES;
         }
         else
         {
            tsr->type = TSR_NPT_NPT;
         }

         /* 
          * Go ahead and find either a space or a '/', skipping
          * '.' and ':' and LWS that can be part of an npt-time.
          * Any other character terminates the npt-range parsing.
          */
         while( *npt && (*npt != ' ') && (*npt != '/') && (*npt != '\r') && (*npt != '\n')
                || (*npt == '.') || (*npt == ':') || isdigit(*npt) ) npt++;

         /* Check the rest against a instance-duration string. */
         if( *npt == '/' )
         {
            /* Parse the instant-duration specifier. */
            res = npt_parse( npt+1, &tsr->instance_duration );
            if( res == 0 )
            {
               /* 
                * npt_end is misspelled. Abort
                * parsing and return an error.
                */
               tsr->type = TSR_INVALID;
               return 0;
            }
            else
            {
               if( bytes != NULL )
               {
                  tsr->type = TSR_NPT_NPT_ID_BYTES;
               }
               else
               {
                  tsr->type = TSR_NPT_NPT_ID;
               }
            }
         }
         else
         if( (*npt != '\r') && (*npt != '\n') )
         {
            /* 
             * If it is not LWS then there's 
             * garbage at the end of the npt-range. 
             * Abort parsing and return an error.
             */
            tsr->type = TSR_INVALID;
            return 0;
         }

      }
   }
   else
   if( *npt == ' ' )
   {
      /* 
       * If there is a space there must be a
       * bytes-range specifier.
       */
      if( bytes != NULL ) 
      {
         tsr->type = TSR_NPT_BYTES;
      }
      else
      {
         tsr->type = TSR_INVALID;
         return 0;
      }
   }
   else
   if( *npt == '/' )
   {
      /* Parse the instant-duration specifier. */
      res = npt_parse( npt+1, &tsr->instance_duration );
      if( res == 0 )
      {
         /* 
          * npt_end is misspelled. Abort
          * parsing and return an error.
          */
         tsr->type = TSR_INVALID;
         return 0;
      }
      else
      {
         if( bytes != NULL )
         {
            tsr->type = TSR_NPT_ID_BYTES;
         }
         else
         {
            tsr->type = TSR_NPT_ID;
         }
      }
   }

   /* 
    * Now parse the bytes-range specifier. DLNA specs
    * are really convoluted, as they define yet another
    * bytes-range with the only difference of the 
    * instance-length additional (and mandatory) field and
    * where start and end are both needed.
    * Doh.
    * Anyways, this is an easy job.
    */
   if( bytes != NULL )
   {
      unsigned long ul;

      /* Try with the 1234-5678/1234 format first. */
      if( sscanf( bytes, "bytes=%u-%u/%u", &tsr->range_start, &tsr->range_end, &ul ) < 3 )
      {
         char star;

         /* That did not work. Try the 1234-5678/* then. */
         if( (sscanf( bytes, "bytes=%u-%u/%c", &tsr->range_start, &tsr->range_end, &star ) < 3) || (star != '*') )
         {
            /* 
             * That did not work either, it 
             * must be a mispelled bytes-range then. 
             */
            tsr->type = TSR_INVALID;
            return 0;
         }
         else
         {
            tsr->instance_length.type = NPT_UNKNOWN;
         }
      }
      else
      {
         /* There is only left to assign the instance-length. */
         tsr->instance_length.type = NPT_SEC;
         tsr->instance_length.secs.sec_hi = ul;
      }
   }
   return 1;
} /* timeseek_parse */

/**
 * Returns a string representation of the timeseek_range.
 *
 * @param tsr The timeseek_range structure to translate into a string.
 * @buf The buffer that will store the string representation
 *    of the timeseek range. If set to NULL the function will allocate
 *    a new buffer that will store the string.
 * @param buf_len Size of the buffer. Ignored if buf is NULL.
 *    If set to 0 (zero) and buf is not NULL, the function will return NULL.
 *    If buffer length is not enough to store the converted string,
 *    the function will return NULL.
 * @return A pointer to buf.
 */
char *timeseek_tostring( const timeseek_range *tsr, char *buf, unsigned int buf_len )
{
   char tsrstring[80]; /* 80 should be a reasonable length! */
   char *nptstart,
        *nptend,
        *nptduration,
        *nptlength;

   if ( tsr == NULL  )
   {
      return NULL;
   }

   if( (buf != NULL) && (buf_len == 0) )
   {
      return NULL;
   }

   switch( tsr->type )
   {
      case TSR_INVALID:
         return NULL;

      case TSR_NPT:             /* npt=xxxx- */
         nptstart = npt_tostring( &tsr->npt_start, NULL, 0 );
         sprintf( tsrstring, "npt=%s-", nptstart );
         free( nptstart );
         break;

      case TSR_NPT_ID:          /* npt=xxxx-/dddd */
         nptstart = npt_tostring( &tsr->npt_start, NULL, 0 );
         nptduration = npt_tostring( &tsr->instance_duration, NULL, 0 );
         sprintf( tsrstring, "npt=%s-/%s", nptstart, nptduration );
         free( nptstart );
         free( nptduration );
         break;

      case TSR_NPT_NPT:         /* npt=xxxx-yyyy */
         nptstart = npt_tostring( &tsr->npt_start, NULL, 0 );
         nptend = npt_tostring( &tsr->npt_end, NULL, 0 );
         sprintf( tsrstring, "npt=%s-%s", nptstart, nptend );
         free( nptstart );
         free( nptend );
         break;

      case TSR_NPT_NPT_ID:      /* npt=xxxx-yyyy/dddd */
         nptstart = npt_tostring( &tsr->npt_start, NULL, 0 );
         nptend = npt_tostring( &tsr->npt_end, NULL, 0 );
         nptduration = npt_tostring( &tsr->instance_duration, NULL, 0 );
         sprintf( tsrstring, "npt=%s-%s/%s", nptstart, nptend, nptduration );
         free( nptstart );
         free( nptend );
         free( nptduration );
         break;

      case TSR_NPT_BYTES:       /* npt=xxxx- bytes=wwww-zzzz/llll */
         nptstart = npt_tostring( &tsr->npt_start, NULL, 0 );
         nptlength = npt_tostring( &tsr->instance_length, NULL, 0 );
         sprintf( tsrstring, "npt=%s- bytes=%u-%u/%s", nptstart, tsr->range_start, tsr->range_end, nptlength );
         free( nptstart );
         free( nptlength );
         break;

      case TSR_NPT_ID_BYTES:    /* npt=xxxx-/dddd bytes=wwww-zzzz/llll */
         nptstart = npt_tostring( &tsr->npt_start, NULL, 0 );
         nptduration = npt_tostring( &tsr->instance_duration, NULL, 0 );
         nptlength = npt_tostring( &tsr->instance_length, NULL, 0 );
         sprintf( tsrstring, "npt=%s-/%s bytes=%u-%u/%s", nptstart, nptduration, tsr->range_start, tsr->range_end, nptlength );
         free( nptstart );
         free( nptduration );
         free( nptlength );
         break;

      case TSR_NPT_NPT_BYTES:   /* npt=xxxx-yyyy bytes=wwww-zzzz/llll */
         nptstart = npt_tostring( &tsr->npt_start, NULL, 0 );
         nptend = npt_tostring( &tsr->npt_end, NULL, 0 );
         nptlength = npt_tostring( &tsr->instance_length, NULL, 0 );
         sprintf( tsrstring, "npt=%s-%s bytes=%u-%u/%s", nptstart, nptend, tsr->range_start, tsr->range_end, nptlength );
         free( nptstart );
         free( nptend );
         free( nptlength );
         break;

      case TSR_NPT_NPT_ID_BYTES: /* npt=xxxx-yyyy/dddd bytes=wwww-zzzz/llll */
         nptstart = npt_tostring( &tsr->npt_start, NULL, 0 );
         nptend = npt_tostring( &tsr->npt_end, NULL, 0 );
         nptduration = npt_tostring( &tsr->instance_duration, NULL, 0 );
         nptlength = npt_tostring( &tsr->instance_length, NULL, 0 );
         sprintf( tsrstring, "npt=%s-%s/%s bytes=%u-%u/%s", nptstart, nptend, nptduration, tsr->range_start, tsr->range_end, nptlength );
         free( nptstart );
         free( nptend );
         free( nptduration );
         free( nptlength );
         break;
   }
   
   if( buf == NULL )
   {
      buf = strdup( tsrstring );
   }
   else
   {
      if( buf_len < strlen( tsrstring ) )
         return NULL;
      else
         strcpy( buf, tsrstring );
   }

   return buf;
} /* timeseek_tostring */