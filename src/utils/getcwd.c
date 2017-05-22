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

#ifdef WIN32

#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>

#include <stdlib.h>

#include "getcwd.h"

/**
 * Windows implementation of the POSIX getcwd().
 *
 * @param buf The buffer that will receive the 
 *    current path. If NULL, a new buffer will be 
 *    allocated and will be returned by the function.
 * @param size. Size of the buffer
 * @return A pointer to the path buffer
 */
char *getcwd( char *buf, size_t size )
{
   DWORD len;
   DWORD res;
   LPTSTR path;

   len = GetCurrentDirectory( 0, NULL );
   path = calloc( len, sizeof(TCHAR) );
   res = GetCurrentDirectory( (DWORD)len, path );

   if( buf == NULL )
   {
      if( res > 0 )
      {
         char *tmp = calloc( len, sizeof(char) );
         wcstombs( tmp, path, len );
         free( path );
         return tmp;
      }
      else
      {
         return NULL;
      }
   }
   else
   {
      if( res > 0 )
      {
         wcstombs( buf, path, len );
         free( path );
         return buf;
      }
      else
      {
         return NULL;
      }
   }
}

#endif