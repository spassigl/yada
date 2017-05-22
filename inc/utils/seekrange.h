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

#ifndef __SEEKRANGE_H
#define __SEEKRANGE_H



/*----------------------------------------------------------------------------
 *
 * Normal Play Time
 *
 *--------------------------------------------------------------------------*/


/*
   Requirement [7.4.13.1]: The syntax of the 
   npt-time token must be as follows:
   • npt time = npt sec | npt hhmmss
   • npt sec = 1*DIGIT [ "." 1*3DIGIT ]
   • npt hhmmss = npt hh ":" npt mm ":" npt ss [ "." 1*3DIGIT ]
   • npt hh = 1*DIGIT ; any positive number
   • npt mm = 1*2DIGIT ; 0-59
   • npt ss = 1*2DIGIT ; 0-59
*/

typedef enum
{
   NPT_INVALID,
   NPT_UNKNOWN,      /* "*" */
   NPT_NOW,          /* "now" */
   NPT_SEC,          /* xxx */
   NPT_SEC_FULL,     /* xxx.yyy */
   NPT_HHMMSS,       /* hh:mm:ss */
   NPT_HHMMSS_FULL   /* hh:mm:ss.xxx */
} NPT_TYPE;

/*
 * npt sec structure.
 */
typedef struct npt_time_sec
{
   unsigned long sec_hi; /* 1*DIGIT */
   unsigned int sec_lo;  /* [ "." 1*3DIGIT ] */
} npt_time_sec;

/*
 * npt hhmmss structure.
 */
typedef struct npt_time_hhmmss
{
   unsigned long hh;    /* npt hh - 1*DIGIT ; any positive number */
   unsigned char mm;    /* npt mm - 1*2DIGIT ; 0-59 */
   unsigned char ss;    /* npt ss - 1*2DIGIT ; 0-59 */
   unsigned int low;    /* [ "." 1*3DIGIT ] */
} npt_time_hhmmss;

/**
 * Structure containing the npt-time
 * as defined in the specification.
 * Use type member to determine which
 * format the npt-time is.
 * If type is NPT_UNKNOWN or NPT_NOW,
 * the rest of the fields are left undefined.
 */
typedef struct npt_time
{
   NPT_TYPE type;
   union
   {
      npt_time_sec secs;
      npt_time_hhmmss hhmmss;
   };
} npt_time;

/**
 * Parse a string into a npt_time structure.
 *
 * @param npt_string A null-terminated string
 *    with a time in the npt format. 
 *    If set to "now" it will just set the NPT_TYPE to NPT_NOW 
 *    and the contents of the other fields will be undefined.
 *    If set to "*"  it will just set the NPT_TYPE to NPT_UNKNOWN
 *    and the contents of the other fields will be undefined.
 * @param npt A pointer to a structure that will 
 *    receive the npt_time details.
 * @return 1 if successful, 0 otherwise.
 */
__declspec(dllexport)
int npt_parse( const char *npt_string, npt_time *npt );

/**
 * Parses a string into an Normal Play Time structure.
 *
 * @param npt The npt time structure to translate into a string.
 *    If NULL, or type is NPT_INVALID, the function will return NULL.
 *    If type is NPT_NOW, the function will return "now".
 *    If type is NPT_UNKNOWN, the function will return "*".
 * @buf The buffer that will store the string representation
 *    of the npt_time. If set to NULL the function will allocate
 *    a new buffer that will store the string.
 * @param buf_len Size of the buffer. Ignored if buf is NULL.
 *    If set to 0 (zero) the function will return NULL.
 *    If buffer length is not enough to store the converted string,
 *    the function will return NULL.
 * @return A pointer to buf.
 */
__declspec(dllexport)
char *npt_tostring( const npt_time *npt, char *buf, unsigned int buf_len );


/*----------------------------------------------------------------------------
 *
 * Bytes Range
 *
 *--------------------------------------------------------------------------*/

/*
   DLNA Requirement [7.4.38.3]: The notation of Range 
   header field for DLNA media transport is [..] as stated below:
   • range-line = "Range" *LWS ":" *LWS range specifier
   • range specifier = byte range specifier
   • byte range specifier = bytes unit "=" byte range set
   • bytes unit = "bytes"
   • byte range set = byte range spec
   • byte range spec = first byte pos "-" [last byte pos]
   • first byte pos = 1*DIGIT
   • last byte pos = 1*DIGIT
   Note that the literal, "bytes", is case sensitive.

   Examples:
   • Range: bytes=1539686400-
   • Range: bytes=1539686400-1540210688
*/

typedef enum
{
   BR_INVALID,

   BR_OPEN,    /* bytes=1539686400- */
   BR_CLOSED   /* bytes=1539686400-1540210688 */
} BR_TYPE;

typedef struct bytes_range
{
   BR_TYPE type;

   unsigned long first;
   unsigned long last;
} bytes_range;

/**
 * Parse a string into a bytes_range structure.
 *
 * @param bytesrange_string A null-terminated string
 *    in the form of a bytes range specifier
 * @param br A pointer to a structure that will receive 
 *    the timeseek_range details.
 * @return 1 if successful, 0 otherwise.
 */
__declspec(dllexport)
int bytesrange_parse( const char *bytesrange_string, bytes_range *br );

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
__declspec(dllexport)
char *bytesrange_tostring( const bytes_range *br, char *buf, unsigned int buf_len );


/*----------------------------------------------------------------------------
 *
 * TimeSeek Range
 *
 *--------------------------------------------------------------------------*/


/*
   DLNA Requirement [7.4.40.3]: The notation of the 
   TimeSeekRange.dlna.org header field is defined 
   as follows. 
   • TimeSeekRange-line = "TimeSeekRange.dlna.org" *LWS ":" *LWS range specifier
   • range specifier = npt range [SP bytes-range]
   • npt range = "npt" "=" npt time "-" [ npt time ] [instance-duration]
   • instance-duration = "/" (npt-time | "*")
   • npt time = <syntax as defined as above>
   • bytes range = "bytes" "=" first byte pos "-" last byte pos instance-length
   • first byte pos = 1*DIGIT
   • last byte pos = 1*DIGIT
   • instance-length = "/" (1*DIGIT | "*")
   Note that literals, "npt" and "bytes", are case sensitive.

   [...]

   Examples:
   • TimeSeekRange.dlna.org : npt=335.11-336.08
   • TimeSeekRange.dlna.org : npt=00:05:35.3-00:05:37.5
   
   Specifying the range value in the combination of npt-sec and npt-hhmmss, e.g.
   335.11-00:05:37.5, is allowed, but not recommended.
*/

typedef enum
{
   TSR_INVALID,

   TSR_NPT,             /* npt=xxxx- */
   TSR_NPT_ID,          /* npt=xxxx-/dddd */
   TSR_NPT_NPT,         /* npt=xxxx-yyyy */
   TSR_NPT_NPT_ID,      /* npt=xxxx-yyyy/dddd */

   TSR_NPT_BYTES,       /* npt=xxxx- bytes=wwww-zzzz/llll */
   TSR_NPT_ID_BYTES,    /* npt=xxxx-/dddd bytes=wwww-zzzz/llll */
   TSR_NPT_NPT_BYTES,   /* npt=xxxx-yyyy bytes=wwww-zzzz/llll */
   TSR_NPT_NPT_ID_BYTES /* npt=xxxx-yyyy/dddd bytes=wwww-zzzz/llll */
} TSR_TYPE;


typedef struct timeseek_range
{
   TSR_TYPE type;

   npt_time npt_start;
   npt_time npt_end;
   npt_time instance_duration;
   
   unsigned long range_start;
   unsigned long range_end;
   npt_time instance_length;  /* Can only get the types NPT_SEC or NPT_UNKNOWN */
} timeseek_range;

/**
 * Parse a string into a timeseek_range structure.
 *
 * @param timeseek_string A null-terminated string
 *    in the form of a timeseek range specifier
 * @param tsr A pointer to a structure that will 
 *    receive the timeseek_range details.
 * @return 1 if successful, 0 otherwise.
 */
__declspec(dllexport)
int timeseek_parse( const char *timeseek_string, timeseek_range *tsr );

/**
 * Returns a string representation of the timeseek_range.
 *
 * @param tsr The timeseek_range structure to translate into a string.
 * @buf The buffer that will store the string representation
 *    of the timeseek range. If set to NULL the function will allocate
 *    a new buffer that will store the string.
 * @param buf_len Size of the buffer. Ignored if buf is NULL.
 * @return A pointer to buf.
 */
__declspec(dllexport)
char *timeseek_tostring( const timeseek_range *tsr, char *buf, unsigned int buf_len );


#endif