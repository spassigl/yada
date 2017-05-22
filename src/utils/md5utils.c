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
#include <sys/stat.h>

#include "md5.h"


static char HEX_CHARS[] = {'0', '1', '2', '3',
                           '4', '5', '6', '7',
                           '8', '9', 'a', 'b',
                           'c', 'd', 'e', 'f',};

/**
 * Creates a MD5 hash from a file, represented as a sequence 
 * of 32 hexadecimal digits.
 * @param digest A 32-byte character array that will
 *       store the MD5 hash.
 * @param filename The filename to create the digest of.
 * return 1 if successful, 0 otherwise.
 */
int md5_file_digest( unsigned char digest[], char *filename )
{
   FILE *f;
   MD5_CTX ctx;
   unsigned char *buf = 0;
   unsigned int buf_size = 0;
   int read = 0;
   struct stat file_info;
   unsigned char hash[16];
   int i, j;

   /* Try to open the file first. */
   f = fopen( filename, "rb" );
   if( f == NULL )
   {
      return 0;
   }

   stat( filename, &file_info );

   /* Allocate a reasonable buffer based
    * on file size. */
   if( file_info.st_size < 1048576 ) /* 1M */
      buf_size = (unsigned int)file_info.st_size;
   else
      buf_size = 1048576;
      
   buf = malloc( buf_size );
   if( buf == NULL )
   {
      return 0;
   }

   /* MD5 loop. */
   MD5Init( &ctx );
   while( (read = fread(buf, sizeof(char), buf_size, f)) > 0 )
   {
      MD5Update( &ctx, buf, read );
   }
   MD5Final( hash, &ctx );
   
   /* Close the file handle. */
   fclose( f );

   /* Transform the hash into an hexadecimal digest. 
    * We could use sprintf() but better be faster! */
   for( i = 0, j = 0; i < 16; i++ )
   {
      digest[j++] = HEX_CHARS[(hash[i] >> 4) & 0x0f];
      digest[j++] = HEX_CHARS[hash[i] & 0x0f];
   }
   digest[32] = 0;

   return 1;
} /* md5_file_digest */

/**
 * Creates a MD5 hash from a string message, represented as a sequence 
 * of 32 hexadecimal digits.
 * @param digest A 32-byte characted array that will
 *       store the MD5 hash.
 * @param string The string to create the digest of.
 * return 1 if successful, 0 otherwise.
 */
int md5_message_digest( unsigned char digest[], char *message )
{
   MD5_CTX ctx;
   unsigned char hash[16];
   int i, j;

   if( (message == NULL) || (message[0] == 0) )
   {
      return 0;
   }

   /* MD5 loop. */
   MD5Init( &ctx );
   MD5Update( &ctx, message, strlen(message) );
   MD5Final( hash, &ctx );
   

   /* Transform the hash into an hexadecimal digest. */
   for( i = 0, j = 0; i < 16; i++ )
   {
      digest[j++] = HEX_CHARS[(hash[i] >> 4) & 0x0f];
      digest[j++] = HEX_CHARS[hash[i] & 0x0f];
   }
   digest[32] = 0;

   return 1;
} /* md5_string_digest */