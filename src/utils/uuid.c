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

/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * Very quick and simple Version 4 uuid generator.
   Since Version 4 UUID is based on random numbers, 
   this function will return different UUIDs every time.

   Field                  Data Type     Octet  Note
                                        #

   time_low               unsigned 32   0-3    The low field of the
                          bit integer          timestamp.

   time_mid               unsigned 16   4-5    The middle field of the
                          bit integer          timestamp.

   time_hi_and_version    unsigned 16   6-7    The high field of the
                          bit integer          timestamp multiplexed
                                               with the version number.

   clock_seq_hi_and_rese  unsigned 8    8      The high field of the
   rved                   bit integer          clock sequence
                                               multiplexed with the
                                               variant.

   clock_seq_low          unsigned 8    9      The low field of the
                          bit integer          clock sequence.

   node                   unsigned 48   10-15  The spatially unique
                          bit integer          node identifier.

  .  Set the 2 most significant bits (bits numbered 6 and 7) of the
     clock_seq_hi_and_reserved to 0 and 1, respectively.

  .  Set the 4 most significant bits (bits numbered 12 to 15 inclusive)
     of the time_hi_and_version field to the 4-bit version number
     corresponding to the UUID version being created, as shown in the
     table below.

       Msb0  Msb1   Msb2  Msb3   Version

        0      1     0      0       4  

 */
void uuid_generate(unsigned char *uuid)
{
    unsigned char b[16]; /* Random bytes array */
    int i;

    srand((unsigned) time(0));

    for (i = 0; i < 16; i++) {
        b[i] = (unsigned char)((double) rand() / (RAND_MAX + 1) * 255);
    }

    /* Set the 4 most significant bits (bits numbered 12 to 15 inclusive)
       of the time_hi_and_version field to 0 1 0 0 respectively. */
    b[6] &= 0x0f;
    b[6] |= 0x40;

    /* Set the 2 most significant bits (bits numbered 6 and 7) of the
       clock_seq_hi_and_reserved to 0 and 1, respectively. */
    b[8] &= 0x3f;
    b[8] |= 0x80;

    sprintf(uuid, 
       "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
       b[0], b[1], b[2], b[3], b[4], 
       b[5], b[6], b[7], b[8], b[9], 
       b[10], b[11], b[12], b[13], b[14], b[15]);

} /* uuid_generate */
