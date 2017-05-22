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

#ifndef __HTTPD_H
#define __HTTPD_H

#ifdef __cplusplus
extern "C" {
#endif


/* Error codes */
enum
{
   HTTPD_SUCCESS = 0,
   HTTPD_INIT_ERROR = -1,
   HTTPD_SOCKET_ERROR = -2,
   HTTPD_MEMORY_ERROR = -3,
   HTTPD_CALLBACK_ERROR = -4,

   HTTPD_400_ERROR = -400,
   HTTPD_402_ERROR = -402,
   HTTPD_404_ERROR = -404,
   HTTPD_416_ERROR = -416,
   HTTPD_500_ERROR = -500,
   HTTPD_501_ERROR = -501,
   HTTPD_701_ERROR = -701,
   HTTPD_709_ERROR = -709,
   HTTPD_720_ERROR = -720
};

/** 
 * These typedefs define the callbacks the HTTP
 * server will invoke when a POST request is received
 * that must be handled by either the connection
 * manager or the content directory server.
 * The callback parameter is a pointer to the
 * POST message body.
 *
 * Return value is either HTTPD_SUCCESS or HTTPD_CALLBACK_ERROR.
 */
typedef int (*connection_manager_cb) (char *);
typedef int (*content_directory_cb) (char *);

/**
 * The initialization parameters
 * for the HTTP server.
 */
typedef struct httpd_init_param
{
   /* IP address and port. */
   char *ip_address;
   int port;

   /* The "/" location. */
   char *doc_root;

   /* Callbacks */
   connection_manager_cb conn_mgr_cb;
   content_directory_cb cont_dir_cb;
} httpd_init_param;

/**
 * Starts the HTTP server binding it to the specified address and port.
 *
 * @param init_param The HTTPD initialization parameter structure. Can
 *       NOT be NULL;
 * @return HTTPD_SUCCESS if successful, HTTPD_INIT_ERROR otherwise.
 */
int httpd_server_start( httpd_init_param *init_param );

/**
 * Terminates the HTTP server
 */
void httpd_server_stop();

/**
 * @return the IP address of the server.
 */
char *httpd_get_ip_address();

/**
 * @return the port number the server is listening on.
 */
int httpd_get_port();

/**
 * @return The server name string.
 */
char *httpd_get_name();

/**
 * @return The server version string in the form
 * major"."minor
 */
char *httpd_get_version();

/**
 * @return The server root directory alias name. 
 */
char *httpd_get_root_name();


#ifdef __cplusplus
}
#endif

#endif