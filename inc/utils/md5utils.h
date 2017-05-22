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

#ifndef __MD5UTILS_H
#define __MD5UTILS_H

/**
 * Creates a MD5 hash from a file, represented as a sequence 
 * of 32 hexadecimal digits.
 * @param digest A 32-byte characted array that will
 *       store the MD5 hash.
 * @param filename The filename to create the digest of.
 * return 1 if successful, 0 otherwise.
 */
int md5_file_digest( unsigned char digest[], char *filename );

/**
 * Creates a MD5 hash from a string message, represented as a sequence 
 * of 32 hexadecimal digits.
 * @param digest A 32-byte characted array that will
 *       store the MD5 hash.
 * @param string The string to create the digest of.
 * return 1 if successful, 0 otherwise.
 */
int md5_message_digest( unsigned char digest[], char *message );

#endif