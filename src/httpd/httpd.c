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

/* 
 * An HTTP 1.1 Streaming server compliant to most of the DLNA 1.5 specifications.
 *
 * v0.5
 * Not implemented, or partially implemented, standard headers include:
 * - PlaySpeed.dlna.org - only normal play speed (DLNA.ORG_PS=1) is supported for now
 * - realTimeInfo.dlna.org - this is only sent in the HTTP response, as "DLNA.ORG_TLAG=*"

 * Samsung specific headers that are correctly interpreted and taken care of include:
 * - getMediaInfo.sec
 * - getCaptionInfo.sec
 *
 */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  define socket_t SOCKET
#  define millisleep(x) SleepEx(x, TRUE)
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <unistd.h>
#  define socket_t int
#  define millisleep(x) usleep((x)*1000)
#endif

#include "pthread.h"

#include "logger.h"

#include "libxml/parser.h"
#include "libxml/tree.h"

#ifdef WIN32
#  include "strncasecmp.h"
#  include "getcwd.h"
#endif

#include "seekrange.h"

#include "httpd.h"

#include "cds.h"
#include "cms.h"

/* Server name and version. */

#define HTTPD_SERVER_NAME "YADA-HTTP"
#define HTTPD_SERVER_VERSION_MAJOR "1"
#define HTTPD_SERVER_VERSION_MINOR "0"
#define HTTPD_SERVER_VERSION HTTPD_SERVER_VERSION_MAJOR"."HTTPD_SERVER_VERSION_MINOR

/* Web root directory alias ("/") */
#define HTTPD_WEB_ROOT  "Web"

/**
 * HTTP Methods
 */
typedef enum
{
   HTTP_METHOD_UNKNOWN,
   HTTP_METHOD_HEAD,
   HTTP_METHOD_GET,
   HTTP_METHOD_POST
} HTTP_METHOD;

/**
 * HTTP version
 */
typedef enum
{
   HTTP_VERSION_UNKNOWN,
   HTTP_VERSION_10,
   HTTP_VERSION_11
} HTTP_VERSION;

/*
 * HTTP Transfer mode.
 */
typedef enum
{
   TM_STREAMING,
   TM_INTERACTIVE,
   TM_BACKGROUND
} TRANSFER_MODE;

/**
 * The HTTP header
 */
typedef struct http_headers
{
   HTTP_METHOD method;
   char *method_uri;
   HTTP_VERSION version;
   char *date;
   char *user_agent;
   long content_length;
   int chunked;
   char *soap_action;

   /* 
    * DLNA headers allowed in the standard. 
    */
   timeseek_range tsr;
   bytes_range br;
   char *friendly_name;
   TRANSFER_MODE transfer_mode;
} http_headers;

/**
 * The HTTP message body.
 */
typedef struct http_message_body
{
   long content_length;
   unsigned char *message;
} http_message_body;

/**
 * The HTTP complete message (headers+body);
 */
typedef struct http_message
{
   http_headers      *headers;
   http_message_body *body;
} http_message;

/**
 * The buffer size used when reading from
 * sockets. A 2KB buffer should work OK 
 * for most requests.
 */
#define HTTP_SOCKET_BUFFER_SIZE 2048

/**
 * DLNA Requirement [7.4.47.1]: HTTP Client and 
 * Server Endpoints must use a total HTTP 
 * header size that is less than or equal to 
 * 8192 bytes (8 KB) when sending an HTTP 
 * request or HTTP response.
 */
#define HTTP_HEADERS_MAX_SIZE 8192

/** 
 * The web server context.
 */
typedef struct httpd_context
{
   unsigned char httpd_initialized;

   /* IP address and port */
   char ip_address[16];
   int port;

   /* Root path */
   char *doc_root_path;

   /* Thread variables */
   pthread_t httpd_thread; /* The actual server thread */
   int httpd_run; /* This will be set to 0 to stop the discover thread */
   pthread_mutex_t httpd_mutex;

   /* Error message to return. */
   int error_code;

   /* DLNA standard headers */
   int content_features; /* getcontentFeatures.dlna.org */
   int timeseek_range;  /* TimeSeekRange.dlna.org */
   int bytes_range; /* Range */
   int transfer_mode; /* transferMode.dlna.org */

   /* 
    * Samsung specific headers 
    * (not in the standard).
    */
   int sec_getmediainfo;
   int sec_getcaptioninfo;
} httpd_context;

/** 
 * Global context variable 
 */
static httpd_context g_context = { 0 };

/**
 * Reset working context.
 * Do not reset httpd_initialized though,
 * we assume that is still valid.
 */
static
void httpd_reset_context()
{
   g_context.error_code = 0;
   g_context.content_features = 0;
   g_context.timeseek_range = 0;
   g_context.bytes_range = 0;

   g_context.sec_getmediainfo = 0;
   g_context.sec_getcaptioninfo = 0;
} /* httpd_reset_context */


/*----------------------------------------------------------------------------
 *
 * HTTP Standard Headers and Body
 *
 * As per DLNA Requirement [7.4.23.2]: HTTP/1.1 Server Endpoints used for media 
 * transport should return HTTP version 1.1 in the response header, 
 * regardless of the version specified in the HTTP client's request
 *
 *--------------------------------------------------------------------------*/

#define HTTP_200_MSG_HEADERS \
   "HTTP/1.1 200 OK\r\n"\
   "Connection: close\r\n" \
   "Content-Length: %d\r\n"\
   "Content-Type: text/xml; charset=\"utf-8\"\r\n"\
   "Date: %s\r\n"\
   "EXT: \r\n"\
   "Server: " HTTPD_SERVER_NAME "/" HTTPD_SERVER_VERSION "\r\n" \
   "\r\n"

#define HTTP_400_MSG_HEADERS \
   "HTTP/1.1 400 BAD REQUEST\r\n" \
   "Connection: close\r\n" \
   "Content-Length: 0\r\n" \
   "Server: " HTTPD_SERVER_NAME "/" HTTPD_SERVER_VERSION "\r\n" \
   "\r\n"
#define HTTP_400_MSG_BODY \
   ""

#define HTTP_401_MSG_HEADERS \
   "HTTP/1.1 401 UNAUTHORIZED\r\n" \
   "Connection: close\r\n" \
   "Content-Length: 0\r\n" \
   "Server: " HTTPD_SERVER_NAME "/" HTTPD_SERVER_VERSION "\r\n" \
   "\r\n"
#define HTTP_401_MSG_BODY \
   ""

#define HTTP_402_MSG_HEADERS \
   "HTTP/1.1 402 Invalid Arguments\r\n" \
   "Connection: close\r\n" \
   "Content-Length: 0\r\n" \
   "Server: " HTTPD_SERVER_NAME "/" HTTPD_SERVER_VERSION "\r\n" \
   "\r\n"
#define HTTP_402_MSG_BODY \
   ""

#define HTTP_404_MSG_HEADERS \
   "HTTP/1.1 404 NOT FOUND\r\n" \
   "Connection: close\r\n" \
   "Content-Length: 0\r\n" \
   "Server: " HTTPD_SERVER_NAME "/" HTTPD_SERVER_VERSION "\r\n" \
   "\r\n"
#define HTTP_404_MSG_BODY \
   ""

#define HTTP_416_MSG_HEADERS \
   "HTTP/1.1 416 Requested Range Not Satisfiable\r\n" \
   "Connection: close\r\n" \
   "Content-Length: 0\r\n" \
   "Server: " HTTPD_SERVER_NAME "/" HTTPD_SERVER_VERSION "\r\n" \
   "\r\n"
#define HTTP_416_MSG_BODY \
   ""

#define HTTP_500_MSG_HEADERS \
   "HTTP/1.1 500 INTERNAL SERVER ERROR\r\n" \
   "Connection: close\r\n" \
   "Content-Length: 0\r\n" \
   "Server: " HTTPD_SERVER_NAME "/" HTTPD_SERVER_VERSION "\r\n" \
   "\r\n"
#define HTTP_500_MSG_BODY \
   ""

/*----------------------------------------------------------------------------
 *
 * Private socket functions
 *
 *--------------------------------------------------------------------------*/

/**
 * Initialize the Socket engine,needed under Windows
 * @return HTTPD_SUCCESS or HTTPD_INIT_ERROR
 */
static
int httpd_socket_init()
{
#ifdef WIN32
   WSADATA wsa_data;
   if( WSAStartup(MAKEWORD(2,2), &wsa_data) != 0 )
   {
      logger_log( LOG_ERROR, LOG_MSG("error during winsock initialization!") );
      return HTTPD_INIT_ERROR;
   }

   return HTTPD_SUCCESS;
#else
   return HTTPD_SUCCESS;
#endif
} /* httpd_socket_init() */

/**
 * Terminates the winsock engine under Windows
 */
static
void httpd_socket_cleanup()
{
#ifdef WIN32
   WSACleanup();
#endif
} /* httpd_socket_cleanup */


/**
 * Creates a server TCP socket that is used by the HTTP thread
 *
 * @return A Socket descriptor or -1 otherwise
 */
static
socket_t httpd_new_server_socket()
{
   socket_t httpd_socket = 0;
   struct sockaddr_in serv_addr;
   int addr_len = sizeof(serv_addr);

   /* Create the socket */
   httpd_socket = socket( AF_INET, SOCK_STREAM, 0 );
   if( httpd_socket == -1 )
   {
      logger_log( LOG_ERROR, LOG_MSG("error creating httpd socket") );
      return httpd_socket;
   }

   /* Bind it to the specified port. */
   memset( &serv_addr, 0, sizeof(serv_addr) );
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons( g_context.port );
   serv_addr.sin_addr.s_addr = inet_addr( g_context.ip_address );
   if( bind(httpd_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 )
   {
      logger_log( LOG_ERROR, LOG_MSG("error during bind") );
      shutdown( httpd_socket, SD_BOTH );
      closesocket( httpd_socket );
      return -1;
   }

   /* Set the queue length. */
   if( listen(httpd_socket, 5) < 0 )
   {
      logger_log( LOG_ERROR, LOG_MSG("error during listen") );
      shutdown( httpd_socket, SD_BOTH );
      closesocket( httpd_socket );
      return -1;
   }

   /* 
    * If port was zero, system has assigned a random 
    * port so let's get hold of it now.
    */
   if( g_context.port == 0 )
   {
      if( getsockname(httpd_socket, (struct sockaddr *)&serv_addr, &addr_len) < 0 )
      {
         logger_log( LOG_ERROR, LOG_MSG("error during getsockname!") );
         shutdown( httpd_socket, SD_BOTH );
         closesocket( httpd_socket );
         return -1;
      }
      g_context.port = ntohs( serv_addr.sin_port );
   }

   return httpd_socket;
} /* httpd_new_server_socket */


/**
 * Get the first IP address in the server machine
 *
 * @return The IP address string. 
 */
static
char *httpd_get_local_ip()
{
   char hostname[255];
   struct hostent *host_entry;
   static char ip_address[16];

   gethostname( hostname, 255 );
   if( host_entry = gethostbyname( hostname ) )
   {
      strcpy( ip_address, inet_ntoa( *(struct in_addr *)*host_entry->h_addr_list ) );
      return ip_address;
   }
   else
   {
      return NULL;
   }
} /* httpd_get_local_ip */


/*----------------------------------------------------------------------------
 *
 * Private header/body management and parsing functions
 *
 *--------------------------------------------------------------------------*/


static char test_get[] = 
"GET /DMS/SamsungDmsDesc.xml HTTP/1.0\r\n"
"HOST: 192.168.1.100:52235\r\n"
"USER-AGENT: SamsungWiselinkPro/1.0\r\n"
"ACCEPT-LANGUAGE: en-us\r\n\r\n";

static char test_post[] = 
"POST /upnp/control/ContentDirectory1 HTTP/1.0\r\n"
"HOST: 192.168.1.100:52235\r\n"
"CONTENT-LENGTH: 415\r\n"
"CONTENT-TYPE: text/xml;charset=\"utf-8\"\r\n"
"USER-AGENT: DLNADOC/1.50\r\n"
"SOAPACTION: \"urn:schemas-upnp-org:service:ContentDirectory:1#Browse\"\r\n\r\n"
"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:Browse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\"><ObjectID>V_F</ObjectID><BrowseFlag>BrowseDirectChildren</BrowseFlag><Filter>*</Filter><StartingIndex>0</StartingIndex><RequestedCount>0</RequestedCount><SortCriteria></SortCriteria></u:Browse></s:Body></s:Envelope>\r\n";

/** 
 * Gets the next token from a buffer. A token is 
 * surrounded by whitespace or adjacent to a \r
 * character. Note that \r\n is not a token
 * per se, but this function will correctly
 * stop at the character pointing the \r and will return
 * a length of 2.
 *
 * A typical use of this function is as follows:
 *
 * len = 0;
 * token = buf;
 * token = httpd_next_header_token( token+len, &len );
 * while( len > ) {
 *    token = httpd_next_header_token( token+len, &len );
 *    ....
 * }
 *
 * @param buf The buffer where the tokens are parsed from.
 * @param len A pointer to an int that will be set as the token length.
 * @return The pointer to the first character of the token.
 */
static
char *httpd_next_header_token( unsigned char *buf, int *len )
{
   char *token;
   int idx = 0;

   token = buf;

   while( (token[idx] != 0) && isspace(token[idx]) && (token[idx] != '\r') ) idx++;
   token += idx;
   while( (token[idx] != 0) && !isspace(token[idx]) && (token[idx] != '\n') ) idx++;

   /* If idx == 0, we may have found a \r\n couple. */
   if( (idx == 0) && (token[0] == '\r') && (token[1] == '\n') )
   {
      /* Length is actually 2. */
      idx = 2;
   }

   *len = idx;
   return token;
} /* httpd_next_header_token */

/**
 * Further header verification.
 * This functions makes sure that not allowed headers 
 * combinations were not sent by the client endpoint (e.g. 
 * sending realTimeInfo header when requesting an interactive 
 * transfer, etc.
 * In case of an error, the function sets the context error_code 
 * variable to the appropriate value.
 *
 * @param headers The HTTP headers structure
 * @return HTTPD_SUCCESS if successful, HTTP_XXX_ERROR
 *    otherwise, being XXX an HTTP error code.
 */
static 
int httpd_validate_headers( http_headers *headers )
{
   /*
    * DLNA Requirement [7.4.75.2]: An HTTP Server Endpoint 
    * receiving the following headers as part of an Interactive 
    * Transfer must respond with the HTTP response code 400 (Bad Request).
      • TimeSeekRange.dlna.org
      • PlaySpeed.dlna.org
      • realTimeInfo.dlna.org
    *
    * and
    *
    * DLNA Requirement [7.4.78.2]: An HTTP Server Endpoint 
    * receiving the following headers as part of a Background 
    * Transfer must respond with the HTTP response code 400 (Bad Request).
      • TimeSeekRange.dlna.org
      • PlaySpeed.dlna.org
      • realTimeInfo.dlna.org
    *
    */
   if( g_context.transfer_mode && 
      ( (headers->transfer_mode == TM_INTERACTIVE) || (headers->transfer_mode == TM_BACKGROUND) ) )
   {
      /* TimeSeekRange.dlna.org is the only one currently implemented. */
      if( g_context.timeseek_range )
      {
         g_context.error_code = 400;
         return HTTPD_400_ERROR;
      }
   }

   return HTTPD_SUCCESS;
} /* httpd_validate_headers */

/**
 * Parse headers from a buffer containing an entire HTTP request. 
 * Allocates memory for the header structure and its content. 
 * Memory must be released using http_free_headers function.
 *
 * @param buf The HTTP request
 * @param headers Header structure that will be allocated. 
 *    It must be freed with http_free_headers.
 * @return The offset in the buffer where the headers section ends - i.e.
 *    the index after the final \r\n\r\n string.
 */
static
int httpd_parse_headers( unsigned char *buf, http_headers **headers )
{
   char *token;
   int len = 0;
   int done = 0;

   *headers = calloc( 1, sizeof(http_headers) );
   (*headers)->content_length = -1;

   token = buf;
   token = httpd_next_header_token( buf, &len );
   while( (done == 0) )
   {

      logger_log( LOG_TRACE, LOG_MSG("Received header: %s"), token );

      /*---------------------------------------------------------------------
       *
       * DLNA STANDARD HEADERS
       *
       *--------------------------------------------------------------------*/

      /* We strncmp "GET " instead of "GET" to distinguish a GET message 
         from a getXXX DLNA header in the message. */
      if( (strncasecmp(token, "GET ", 4) == 0) || (strncasecmp(token, "POST", 4) == 0) )
      {
         if( (token[0] == 'G') || (token[0] == 'g') ) (*headers)->method = HTTP_METHOD_GET;
         else (*headers)->method = HTTP_METHOD_POST;

         /* Parse URI. */
         token = httpd_next_header_token( token+len, &len );
         (*headers)->method_uri = calloc( len+1, sizeof(char) );
         memcpy((*headers)->method_uri, token, len );

         token = httpd_next_header_token( token+len, &len );
         if( token[7] == '0' ) (*headers)->version = HTTP_VERSION_10;
         else (*headers)->version = HTTP_VERSION_11;

         /* Eat \r\n */
         token = httpd_next_header_token( token+len, &len );
      }
      else
      if( (strncasecmp(token, "HEAD", 4) == 0) )
      {
         (*headers)->method = HTTP_METHOD_POST;

         /* Parse URI. */
         token = httpd_next_header_token( token+len, &len );
         (*headers)->method_uri = calloc( len+1, sizeof(char) );
         memcpy((*headers)->method_uri, token, len );

         /* Eat \r\n */
         token = httpd_next_header_token( token+len, &len );
      }
      else
      if( (strncasecmp(token, "User-Agent", 10) == 0) )
      {
         token = httpd_next_header_token( token+len, &len );
         (*headers)->user_agent = calloc( len+1, sizeof(char) );
         memcpy((*headers)->user_agent, token, len );

         /* Eat \r\n */
         token = httpd_next_header_token( token+len, &len );
      }
      else
      if( (strncasecmp(token, "Content-Length", 14) == 0) )
      {
         token = httpd_next_header_token( token+len, &len );
         (*headers)->content_length = atol( token );

         /* Eat \r\n */
         token = httpd_next_header_token( token+len, &len );
      }
      else
      if( (strncasecmp(token, "Transfer-Encoding", 17) == 0) )
      {
         /* If both Transfer-Encoding and Content-Length 
          * are received, the latter MUST be ignored as
          * per the HTTP specs.
          */
         (*headers)->chunked = 1;
         if( (*headers)->content_length != -1 ) (*headers)->content_length = -1;
      }
      else
      if( (strncasecmp(token, "SOAPACTION", 10) == 0) )
      {
         token = httpd_next_header_token( token+len, &len );
         (*headers)->soap_action = calloc( len+1, sizeof(char) );
         memcpy((*headers)->soap_action, token, len );

         /* Eat \r\n */
         token = httpd_next_header_token( token+len, &len );
      }
      else
      if( (strncasecmp(token, "getcontentFeatures.dlna.org", 27) == 0) )
      {
         int val = atoi( httpd_next_header_token( token+len, &len ) );
         if( val != 1 )
         {
            logger_log( LOG_ERROR, LOG_MSG("getcontentFeatures header error, setting error to 400") );

            /* DLNA Requirement [7.4.26.5]: If an HTTP Server Endpoint 
             * receives any value except "1" in the 
             * getcontentFeatures.dlna.org header it must return an 
             * error code response of 400 (Bad Request).
             */
            g_context.error_code = 400;

            /* We can break here as request is malformed. */
            break;
         }
         else
         {
            g_context.content_features = 1;
         }

         /* Eat \r\n */
         token = httpd_next_header_token( token+len, &len );
      }
      else
      if( (strncasecmp(token, "TimeSeekRange.dlna.org", 22) == 0) )
      {
         token = httpd_next_header_token( token+len, &len );
         if ( timeseek_parse(token, &(*headers)->tsr) == 0 )
         {
            logger_log( LOG_ERROR, LOG_MSG("TimeSeekRange header error, setting error to 416") );

            /* DLNA guidelines do not specify what to do in case of
             * a malformed header. We decided to comply with 
             * DLNA Requirement [7.4.40.8]: If an HTTP Server Endpoint 
             * supports the TimeSeekRange.dlna.org header and the 
             * requested time range is not valid for the resource 
             * with URI specified in the HTTP GET request, then the 
             * HTTP streaming server must respond with the HTTP 
             * response error code of: 416 (Requested Range Not Satisfiable).
             */
            g_context.error_code = 416;

            /* We can break here as request is malformed. */
            break;
         }
         else
         {
            g_context.timeseek_range = 1;
         }
      }
      else
      if( (strncasecmp(token, "Range", 5) == 0) )
      {
         token = httpd_next_header_token( token+len, &len );
         if ( bytesrange_parse(token, &(*headers)->br) == 0 )
         {
            logger_log( LOG_ERROR, LOG_MSG("Range header error, setting error to 416") );
               
            /* Same as per the TimeSeekRange.dlna.org header. */
            g_context.error_code = 416;

            /* We can break here as request is malformed. */
            break;
         }
         else
         {
            g_context.bytes_range = 1;
         }
      }
      else
      if( (strncasecmp(token, "friendlyName.dlna.org", 21) == 0) )
      {
         token = httpd_next_header_token( token+len, &len );
         (*headers)->friendly_name = calloc( len+1, sizeof(char) );
         memcpy((*headers)->friendly_name, token, len );
      }
      else
      if( (strncasecmp(token, "transferMode.dlna.org", 21) == 0) )
      {
         token = httpd_next_header_token( token+len, &len );
         if( strncmp( token, "Streaming", 9 ) == 0 )
         {
            (*headers)->transfer_mode = TM_STREAMING;
         }
         else
         if( strncmp( token, "Interactive", 11 ) == 0 )
         {
            (*headers)->transfer_mode = TM_INTERACTIVE;
         }
         else
         if( strncmp( token, "Background", 10 ) == 0 )
         {
            (*headers)->transfer_mode = TM_BACKGROUND;
         }
         else
         {
            logger_log( LOG_ERROR, LOG_MSG("Unsupported transferMode value, setting error to 400") );

            /* DLNA guidelines do not specify how to cope with
             * a misspecified transferMode header, so the most
             * natural thing we can do is to respond with error 
             * code 400 (Bad Request).
             */
             g_context.error_code = 400;

            /* We can break here as request is malformed. */
            break;
         }
         g_context.transfer_mode = 1;
      }

      /*---------------------------------------------------------------------
       *
       * SAMSUNG SPECIFIC HEADERS
       *
       *--------------------------------------------------------------------*/

      else
      if( (strncasecmp(token, "getMediaInfo.sec", 16) == 0) )
      {
         g_context.sec_getmediainfo = 1;
      }
      else
      if( (strncasecmp(token, "getCaptionInfo.sec", 18) == 0) )
      {
         g_context.sec_getcaptioninfo = 1;
      }
      

      /*---------------------------------------------------------------------
       *
       * OTHER HEADERS
       *
       *--------------------------------------------------------------------*/

      else
      {
         /* 
          * A header we do not recognize. 
          * As per DLNA Requirement [7.4.21.1]: "HTTP Client 
          * and Server Endpoints must be tolerant of 
          * unknown HTTP headers".
          * So just swallow it up until the terminating \r\n. 
          */

         logger_log( LOG_TRACE, LOG_MSG("header is unsupported, ignoring it") );

         do
         {
            token = httpd_next_header_token( token+len, &len );
         } while( token[0] != '\r' );
      }

      token = httpd_next_header_token( token+len, &len );
      if( strncmp(token, "\r\n", 2) == 0 )
      {
         /* Found headers end, we're done. */
         token += 2;
         done = 1;
      }
   } /* while( done == 0) */

   /* Final verification of not allowed headers combinations. */
   httpd_validate_headers( *headers );

   return token-buf;
} /* http_parse_headers */

/**
 * Parse an HTTP message body from a buffer containing the HTTP
 * message body. 
 * Allocates memory for the body structure and its content. 
 * If Transfer-Encoding is chunked, the function parses each
 * chunk and rebuilds the entire message.
 *
 * @param buf The HTTP message body
 * @param content_length The content length as stated in the
 *    headers, or HTTP_CONTENT_CHUNKED if Transfer-Encoding is 
 *    chunked.
 * @param body Body structure that will be allocated. 
 * @return The length of the message body.
 */
static
long httpd_parse_body( unsigned char *buf, long content_length, http_message_body **body )
{
   unsigned char *full_body = NULL;
   long body_length = 0;

   if( content_length == -1 )
   {
      /* 
       * Message is a chunked POST. Read every chunk 
       * and build the entire message.
       */
      long next_size = 0;
      int len = 0;
      unsigned char *token;

      /* 
       * Use httpd_next_header_token as we need
       * to parse the chunk headers giving us
       * their lengths.
       */
      token = buf;
      token = httpd_next_header_token( buf, &len );
      for( ; ; )
      {
         /* 
          * What we just read into token is actually a
          * hex value. Need to convert it into a decimal 
          * value before using it.
          */
         next_size = strtol( token, NULL, 16 );
         if( next_size == 0 ) break;

         /* Eat up everything until the terminating \r\n included. */
         do
         {
            token = httpd_next_header_token( token+len, &len );
         } while( token[0] != '\r' );
         token = httpd_next_header_token( token+len, &len );

         /* 
          * Reallocate the body buffer, append the 
          * current chunk and zero-terminate. 
          */
         full_body = (unsigned char *)realloc( full_body, body_length+next_size+1 );
         memcpy( full_body+body_length, token, next_size );
         full_body[body_length+next_size] = 0;

         /* Eat up everything until the terminating \r\n included. */
         do
         {
            token = httpd_next_header_token( token+next_size, &len );
         } while( token[0] != '\r' );
         token = httpd_next_header_token( token+len, &len );

         /* 
          * Go ahead and parse the next chunk length. 
          * Update the body length read so far. 
          */
         body_length += next_size;
      }
   }
   else
   if( content_length > 0 )
   {
      /* Non chunked POST. Easy job. */
      body_length = content_length;
      full_body = (unsigned char *)calloc( body_length+1, sizeof(unsigned char) );
      memcpy( full_body, buf, body_length );
   }
   else
   {
      /* Must be a GET then. */
      body_length = 0;
      full_body = NULL;
   }

   /* We're done, copy things back into the body structure. */
   *body = (http_message_body *)calloc( 1, sizeof(http_message_body) );
   (*body)->content_length = body_length;
   (*body)->message = full_body;

   return body_length;
} /* httpd_parse_body */

/**
 * Parses an HTTP message from a buffer containing the HTTP
 * entire message.
 * Allocates memory for the message structure and its content.
 * Memory must be frees with httpd_free_http_message.
 * Returns the combined header+body length, taking correctly
 * into account if the body is transferred in chunks.
 *
 * @param buf The HTTP message.
 * @param message http_message structure that will be allocated. 
 * @return The length of the parsed message.
 */
static
long httpd_parse_http_message( unsigned char *buf, http_message **message )
{
   long length = 0;

   *message = (http_message *)calloc( 1, sizeof(http_message) );
   length = httpd_parse_headers( buf, &((*message)->headers) );
   return length + httpd_parse_body( buf+length, (*message)->headers->content_length, &((*message)->body) );
} /* httpd_parse_http_message */

/**
 * Frees up the headers structure allocated 
 * by http_parse_headers
 *
 * @param headers Header structure that will be freed
 */
static
void httpd_free_headers( http_headers *headers )
{
   if( headers == NULL ) return;
   if( headers->method_uri ) free( headers->method_uri );
   if( headers->user_agent ) free( headers->user_agent );
   if( headers->soap_action ) free( headers->soap_action );
   if( headers->friendly_name ) free( headers->friendly_name );
   free( headers );
} /* http_free_headers */

/**
 * Frees up the body structure allocated 
 * by http_parse_body
 *
 * @param body Body structure that will be freed
 */
static
void httpd_free_body( http_message_body *body )
{
   if( body == NULL ) return;
   if( body->message != NULL )  free( body->message );
   free( body );
} /* httpd_free_body */

/**
 * Frees up the http message structure allocated 
 * by the various parsing functions
 *
 * @param message Message structure that will be freed
 */
static
void httpd_free_http_message( http_message *message )
{
   if( message == NULL ) return;
   if( message->headers != NULL ) httpd_free_headers( message->headers );
   if( message->body != NULL ) free( message->body );
   free( message );
} /* httpd_free_http_message */


/*----------------------------------------------------------------------------
 *
 * Private file management functions
 *
 *--------------------------------------------------------------------------*/

/**
 * Builds the HTTP time string.
 * This function is not reentrant.
 */
static
char *httpd_build_http_time()
{
   static char *days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
   static char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	static char http_time[30];

	time_t curr_time = time( NULL );
	struct tm *gm_time = gmtime( &curr_time );
	sprintf ( http_time, "%s, %02d %s %04d %02d:%02d:%02d GMT",
			    days[gm_time->tm_wday],
			    gm_time->tm_mday,
             months [gm_time->tm_mon],
             1900 + gm_time->tm_year,
             gm_time->tm_hour,
             gm_time->tm_min,
             gm_time->tm_sec );
			  
	return http_time;
} /* httpd_build_http_time */

static
int httpd_send_header_and_body( socket_t client_sock, char *headers, char *body )
{
   int res = 0;

   res = send( client_sock, headers, strlen(headers), 0 );
   if( res > 0 )
      res *= send( client_sock, body, strlen(body), 0 );

   if( res < 0 )
   {
      logger_log( LOG_ERROR, LOG_MSG("failed sending message to client") );
   }

   return res;
} /* httpd_send_header_and_body */

/**
 * Sends an HTTP 200 OK message back to the client.
 *
 * @param client_sock The client socket to use to send 
 *    the response back.
 * @param body The message body
 * @return HTTP_SUCCESS if successful, or another value otherwise.
 */
static
int httpd_send_200_OK( socket_t client_sock, unsigned char *body )
{
   char msg_header[512];

   sprintf( msg_header, HTTP_200_MSG_HEADERS, strlen( body ), httpd_build_http_time() );

   return httpd_send_header_and_body( client_sock, msg_header, body );
} /* httpd_send_200_OK */


/*----------------------------------------------------------------------------
 *
 * HEAD, GET and POST processors
 *
 *--------------------------------------------------------------------------*/


/**
 * HEAD message processor.
 *
 * @param client_sock The client socket to use to send responses back.
 * @param message The HTTP message from the client.
 * @return HTTP_SUCCESS if successful, or HTTP_XXX_ERROR otherwise, being
 *    XXX an HTTP error code.
 */
static
int httpd_process_head( socket_t client_sock, http_message *message )
{
   return 0;
}

/**
 * GET message processor.
 *
 * @param client_sock The client socket to use to send responses back.
 * @param message The HTTP message from the client.
 * @return HTTP_SUCCESS if successful, or HTTP_XXX_ERROR otherwise, being
 *    XXX an HTTP error code.
 */
static
int httpd_process_get( socket_t client_sock, http_message *message )
{
   if( strstr(message->headers->method_uri, CDS_SCPD) )
   {
      /* Return the CDS description XML. */
      httpd_send_200_OK( client_sock, cds_get_scpd() );
   }
   else
   if( strstr(message->headers->method_uri, CMS_SCPD) )
   {
      /* Return the CDS description XML. */
      //httpd_send_200_OK( client_sock, cms_get_scpd() );
   }
   else
   {
      /* Need to stream a resource. */
      FILE *resource;
      int len = strlen(g_context.doc_root_path) + strlen(message->headers->method_uri) + 1;
      char *filename = (char *)calloc( len, sizeof(char) );

      /* 
       * Build the file name and try to open it. 
       * Root path is already terminated with a '/'. 
       */
      sprintf( filename, "%s%s", g_context.doc_root_path, message->headers->method_uri );
      resource = fopen( filename, "r" );
      if( resource == NULL )
      {
         httpd_send_header_and_body( client_sock, 
                                     HTTP_404_MSG_HEADERS, 
                                     HTTP_404_MSG_BODY );
         return HTTPD_404_ERROR;
      }

      /* OK, file is found. Guess file type. */
   }
   return HTTPD_SUCCESS;
}

/**
 * POST message processor.
 *
 * @param client_sock The client socket to use to send responses back.
 * @param message The HTTP message from the client.
 * @return HTTP_SUCCESS if successful, or HTTP_XXX_ERROR otherwise, being
 *    XXX an HTTP error code.
 */
static
int httpd_process_post( socket_t client_sock, http_message *message )
{
   if( strcmp(message->headers->method_uri, CDS_CONTROL_URL) == 0 )
   {
      /* 
       * SOAP Action must be CDS action.
       */
      int res;
      char *cds_response;
      
      res = cds_dispatch_action( message->headers->soap_action, message->body->message, &cds_response );
      if( res == CDS_SUCCESS )
      {
         httpd_send_200_OK( client_sock, cds_response );
      }
      else
      {
         httpd_send_header_and_body( client_sock, HTTP_500_MSG_HEADERS, HTTP_500_MSG_BODY );
      }
   }
   return 0;
}


/*----------------------------------------------------------------------------
 *
 * HTTP Thread
 *
 *--------------------------------------------------------------------------*/

/**
 * Reads an HTTP message from a socket, and builds the header and
 * body structure.
 *
 * @param client_sock The client socket to read the message from.
 * @param message A pointer to the HTTP message structure. The structure
 *    will be allocated and it must be freed with http_free_message.
 * @return HTTPD_SUCCESS is successful, any other value otherwise.
 */
static
int httpd_read_http_message( socket_t client_sock, http_message **message )
{
   int done = 0;
   int recvd = 0;
   int curr_recvd = 0;
   int recv_error = 0;
   unsigned char tmpbuf[HTTP_SOCKET_BUFFER_SIZE];
   unsigned char *fullmsg = NULL;
   int body_offset = 0;

   while( done == 0 )
   {
      curr_recvd = recv( client_sock, tmpbuf, HTTP_SOCKET_BUFFER_SIZE, 0 );
#ifdef WIN32
      if( curr_recvd == SOCKET_ERROR )
      {
         int err =  WSAGetLastError();
         if( (err != WSAEMSGSIZE) && (err != WSAEWOULDBLOCK) ) 
#else
      if( curr_recvd < 0 )
      {
         int err = errno;
         if( (err != EAGAIN) && (err != EWOULDBLOCK) ) 
#endif
         {
            /* Critical error, we're done. */
            logger_log( LOG_ERROR, LOG_MSG("could not receive message from socket, error code: %d"), err );
            recv_error = 1;
         }
         else 
         {
            /* A non critical error has occurred, 
             * let's sleep for a little while and 
             * try again... 
             */
           millisleep( 10 );
           continue;
         }
      } /* if( curr_recvd... */

      if( recv_error == 1 )
      {
         return -1;
      }

      if( curr_recvd == 0 )
      {
         done = 1;
      }
      else
      {
         /* Append buffer we just read. */
         fullmsg = (unsigned char *)realloc( fullmsg, (recvd+curr_recvd+1) * sizeof(char) );
         memcpy( fullmsg+recvd, tmpbuf, curr_recvd * sizeof(char) );
         fullmsg[recvd+curr_recvd] = 0;

         recvd += curr_recvd;
         curr_recvd = 0;
      }
      
   } /* while( done == 0 ) */

   /* 
    * We received the entire HTTP message. Time to 
    * parse it. This will generate header and body structure.
    */
   httpd_parse_http_message( fullmsg, message );

   /* 
    * Message is parsed, so get rid of the 
    * allocated buffer.
    */
   free( fullmsg );

   return HTTPD_SUCCESS;
} /* read_http_message */

/*
 * HTTP thread procedure.
 *
 * @param arg Unused.
 * @return The arg parameter.
 */
static
void *httpd_thread_proc( void *arg )
{
#if 0
   long l;
   http_message *message;

   l = httpd_parse_http_message( test_get, &message );
   httpd_free_http_message( message );

   l = httpd_parse_http_message( test_post, &message );
   httpd_free_http_message( message );

   l = httpd_parse_http_message( test_post_chunked, &message );
   httpd_free_http_message( message );
#endif

   socket_t httpd_sock;
   socket_t client_sock;
   struct sockaddr_in cli_addr;
   int cli_len = sizeof( cli_addr );
   http_message *message;
   
   httpd_sock = httpd_new_server_socket();
   if( httpd_sock < 0 )
   {
      logger_log( LOG_ERROR, LOG_MSG("could not create HTTP socket, exiting thread") );
      return NULL;
   }
   
   logger_log( LOG_INFO, LOG_MSG("HTTP server running on %s:%d"), g_context.ip_address, g_context.port );

   pthread_mutex_lock( &g_context.httpd_mutex );
   g_context.httpd_initialized = 1;
   pthread_mutex_unlock( &g_context.httpd_mutex );

   while( g_context.httpd_run )
   {
      pthread_mutex_lock( &g_context.httpd_mutex );

      client_sock = accept( httpd_sock, (struct sockaddr *)&cli_addr, &cli_len );
      if( client_sock > 0 )
      {
         logger_log( LOG_TRACE, LOG_MSG("http connection from %s:%d"), inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port) );
         
         /* TBD: Authorize Client. */

         /* Reset working context. */
         httpd_reset_context();
         
         /* Read and parse the message. */
         httpd_read_http_message( httpd_sock, &message );

         /* Act upon the received request. */
         switch( message->headers->method )
         {
            case HTTP_METHOD_HEAD:
               httpd_process_head( client_sock, message );
               break;

            case HTTP_METHOD_GET:
               httpd_process_get( client_sock, message );
               break;

            case HTTP_METHOD_POST:
               httpd_process_post( client_sock, message );
               break;
         }

         /* Free up memory. */
         httpd_free_http_message( message );
      }
      else
      {
         logger_log( LOG_ERROR, LOG_MSG("could not accept from clients") );
      }

      /* 
       * This streaming server implementation does not support
       * persistent connections so as per DLNA Requirement [7.2.8.5]: 
       * "The HTTP servers of UPnP endpoints (devices and control points) 
       * that do not support persistent connections must answer 
       * the first HTTP request from the requesting UPnP control point 
       * and close the TCP connection to correctly ignore other requests."
       */
      shutdown( client_sock, SD_BOTH );
      closesocket( client_sock );

      pthread_mutex_unlock( &g_context.httpd_mutex );
   } /* while( g_context.httpd_run ) */

   logger_log( LOG_INFO, LOG_MSG("httpd server now stopped") );

   return arg;
} /* httpd_thread_proc */


/*----------------------------------------------------------------------------
 *
 * Public functions
 *
 *--------------------------------------------------------------------------*/


/**
 * Starts the HTTP server binding it to the specified address and port.
 *
 * @param init_param The HTTPD initialization parameter structure. Can
 *       NOT be NULL.
 * @return HTTPD_SUCCESS if successful, HTTPD_INIT_ERROR otherwise.
 */
int httpd_server_start( httpd_init_param *init_param )
{
   logger_log( LOG_INFO, LOG_MSG("starting HTTP server...") );

   if( httpd_socket_init() != HTTPD_SUCCESS )
   {
      return HTTPD_INIT_ERROR;
   }

   if( init_param->ip_address == NULL )
   {
      strcpy( g_context.ip_address, httpd_get_local_ip() );
   }
   else
   {
      strcpy( g_context.ip_address, init_param->ip_address );
   }
   g_context.port = init_param->port;
   g_context.doc_root_path = init_param->doc_root;

   pthread_mutex_init( &g_context.httpd_mutex, NULL );
   if( pthread_create(&g_context.httpd_thread, NULL, httpd_thread_proc, NULL) != 0 ) 
   {
      logger_log( LOG_ERROR, LOG_MSG("could not start HTTP thread") );
      /* This is a critical error. */
      return HTTPD_INIT_ERROR;
   }

   /* Wait for the thread to fully start up. */
   logger_log( LOG_INFO, LOG_MSG("waiting for HTTP server to come up...") );
   while( g_context.httpd_initialized == 0 )
   {
      millisleep( 50 ); /* 50ms */
   }

   return HTTPD_SUCCESS;
} /* httpd_server_start */


/**
 * Terminates the HTTP server.
 */
void httpd_server_stop()
{
   if( g_context.httpd_initialized == 1 )
   {
      logger_log( LOG_INFO, LOG_MSG("stopping httpd server...") );
      pthread_mutex_lock( &g_context.httpd_mutex );

      g_context.httpd_run = 0;
      g_context.httpd_initialized = 0;

      pthread_mutex_unlock( &g_context.httpd_mutex );
      pthread_cancel( g_context.httpd_thread );
      pthread_mutex_destroy( &g_context.httpd_mutex );

      httpd_socket_cleanup();
   }
} /* httpd_server_stop */


/**
 * @return The IP address of the server.
 */
char *httpd_get_ip_address()
{
   return g_context.ip_address;
} /* httpd_get_ip_address */

/**
 * @return The port number the server is listening on.
 */
int httpd_get_port()
{
   return g_context.port;
} /* httpd_get_port */

/**
 * @return The server name string.
 */
char *httpd_get_name()
{
   return HTTPD_SERVER_NAME;
} /* httpd_get_name */

/**
 * @return The server version string in the form
 * major"."minor
 */
char *httpd_get_version()
{
   return HTTPD_SERVER_VERSION;
} /* httpd_get_version */

/**
 * @return The server root directory alias name. 
 */
char *httpd_get_root_name()
{
   return HTTPD_WEB_ROOT;
} /* httpd_get_root_name */
