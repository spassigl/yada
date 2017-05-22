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

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  define socket_t SOCKET
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  define socket_t int;
#endif

#include "logger.h"

#include "upnp-core.h"
#include "httpd.h"

#include "config.h"

#include "cds.h"
#include "cms.h"

#include "yada.h"



/* 
 * Plain SCPD for YADA. Does not include Samsung or XBOX 
 * extensions. 
 */
#define YADA_PLAIN_SCPD \
"<?xml version=\"1.0\"?>\n" \
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\" xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">\n" \
"<specVersion>\n" \
"<major>1</major>\n" \
"<minor>0</minor>\n" \
"</specVersion>\n" \
"<device>\n" \
"<dlna:X_DLNADOC>DMS-1.50</dlna:X_DLNADOC>\n" \
"<deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>\n" \
"<friendlyName>YADA DLNA Server</friendlyName>\n" \
"<manufacturer>S. Passiglia 2009</manufacturer>\n" \
"<manufacturerURL>http://www.stefanopassiglia.com</manufacturerURL>\n" \
"<modelDescription>DLNA MediaServer</modelDescription>\n" \
"<modelName>YADA</modelName>\n" \
"<modelNumber>1.0</modelNumber>\n" \
"<modelURL>http://www.stefanopassiglia.com/yada</modelURL>\n" \
"<serialNumber>YADA-10</serialNumber>\n" \
"<UDN>uuid:%s</UDN>\n" \
"<serviceList>\n" \
"<service>\n" \
"<serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>\n" \
"<serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>\n" \
"<controlURL>"CDS_CONTROL_URL"\n" \
"<eventSubURL>"CDS_EVENT_URL"\n" \
"<SCPDURL>cds.xml</SCPDURL>\n" \
"</service>\n" \
"<service>\n" \
"<serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>\n" \
"<serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>\n" \
"<controlURL>"CMS_CONTROL_URL"\n" \
"<eventSubURL>"CMS_EVENT_URL"\n" \
"<SCPDURL>cms.xml</SCPDURL>\n" \
"</service>\n" \
"</serviceList>\n" \
"</device>\n" \
"</root>\n"

/* 
 * SCPD for Samsung control points
 * - see the sec namespace
 * The behaviour is undocumented so... ;-)
 */
#define YADA_SAMSUNG_SCPD \
"<?xml version=\"1.0\"?>\n" \
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\" xmlns:sec=\"http://www.sec.co.kr/dlna\" xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">\n" \
"<specVersion>\n" \
"<major>1</major>\n" \
"<minor>0</minor>\n" \
"</specVersion>\n" \
"<device>\n" \
"<dlna:X_DLNADOC>DMS-1.50</dlna:X_DLNADOC>\n" \
"<deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>\n" \
"<friendlyName>YADA DLNA Server</friendlyName>\n" \
"<manufacturer>S. Passiglia 2009</manufacturer>\n" \
"<manufacturerURL>http://www.stefanopassiglia.com</manufacturerURL>\n" \
"<modelDescription>DLNA MediaServer</modelDescription>\n" \
"<modelName>YADA</modelName>\n" \
"<modelNumber>1.0</modelNumber>\n" \
"<modelURL>http://www.stefanopassiglia.com/yada</modelURL>\n" \
"<serialNumber>YADA-10</serialNumber>\n" \
"<sec:ProductCap>smi,DCM10,getMediaInfo.sec,getCaptionInfo.sec</sec:ProductCap>\n" \
"<UDN>uuid:%s</UDN>\n" \
"<serviceList>\n" \
"<service>\n" \
"<serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>\n" \
"<serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>\n" \
"<controlURL>/control/ContentDirectory1</controlURL>\n" \
"<eventSubURL>/event/ContentDirectory1</eventSubURL>\n" \
"<SCPDURL>cds.xml</SCPDURL>\n" \
"</service>\n" \
"<service>\n" \
"<serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>\n" \
"<serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>\n" \
"<controlURL>/control/ConnectionManager1</controlURL>\n" \
"<eventSubURL>/event/ConnectionManager1</eventSubURL>\n" \
"<SCPDURL>cms.xml</SCPDURL>\n" \
"</service>\n" \
"</serviceList>\n" \
"</device>\n" \
"</root>\n"

/* 
 * SCPD for Microsoft control points
 */
#define YADA_XMS_SCPD \
"<?xml version=\"1.0\"?>\n" \
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\" xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">\n" \
"<specVersion>\n" \
"<major>1</major>\n" \
"<minor>0</minor>\n" \
"</specVersion>\n" \
"<device>\n" \
"<dlna:X_DLNADOC>DMS-1.50</dlna:X_DLNADOC>\n" \
"<deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>\n" \
"<friendlyName>YADA DLNA Server</friendlyName>\n" \
"<manufacturer>S. Passiglia 2009</manufacturer>\n" \
"<manufacturerURL>http://www.stefanopassiglia.com</manufacturerURL>\n" \
"<modelDescription>DLNA MediaServer</modelDescription>\n" \
"<modelName>YADA</modelName>\n" \
"<modelNumber>1.0</modelNumber>\n" \
"<modelURL>http://www.stefanopassiglia.com/yada</modelURL>\n" \
"<serialNumber>YADA-10</serialNumber>\n" \
"<UDN>uuid:%s</UDN>\n" \
"<serviceList>\n" \
"<service>\n" \
"<serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>\n" \
"<serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>\n" \
"<controlURL>/control/cds</controlURL>\n" \
"<eventSubURL>/event/cds</eventSubURL>\n" \
"<SCPDURL>cds.xml</SCPDURL>\n" \
"</service>\n" \
"<service>\n" \
"<serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>\n" \
"<serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>\n" \
"<controlURL>/control/cms</controlURL>\n" \
"<eventSubURL>/event/cms</eventSubURL>\n" \
"<SCPDURL>cms.xml</SCPDURL>\n" \
"</service>\n" \
"<service>\n" \
"<serviceType>urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1</serviceType>\n" \
"<serviceId>urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar</serviceId>\n" \
"<SCPDURL>msr.xml</SCPDURL>\n" \
"<controlURL>/control/msr</controlURL>\n" \
"<eventSubURL>/event/msr</eventSubURL>\n" \
"</service>\n" \
"</serviceList>\n" \
"</device>\n" \
"</root>\n"

#define YADA_SCPD_FILENAME "yada.xml"

/**
 * Initialize the Socket engine,needed under Windows
 * @return DLNA_SUCCESS or DLNA_INIT_ERROR
 */
static
int yada_socket_init()
{
#ifdef WIN32
   WSADATA wsa_data;
   if( WSAStartup(MAKEWORD(2,2), &wsa_data) != 0 )
   {
      logger_log( LOG_ERROR, LOG_MSG("error during winsock initialization!") );
      return DLNA_INIT_ERROR;
   }

   return DLNA_SUCCESS;
#else
   return DLNA_SUCCESS;
#endif
} /* dlna_socket_init() */

/**
 * Terminates the winsock engine under Windows
 */
static
void yada_socket_cleanup()
{
#ifdef WIN32
   WSACleanup();
#endif
} /* dlna_socket_cleanup */

/**
 * Creates the description document.
 * If file exists already, do not touch it.
 *
 * @return DLNA_SUCCESS if the file
 *    could be successfully created,
 *    DLNA_INIT_ERROR otherwise
 */
static
int yada_create_SCPD()
{
   struct stat file_info;
   int l = strlen( config_get_doc_root_path() ) + 1 + strlen( YADA_SCPD_FILENAME ) + 1;
   char *file = calloc( l, sizeof(char) );

#ifdef WIN32
   sprintf( file, "%s\\%s", config_get_doc_root_path(), YADA_SCPD_FILENAME );
#else
   sprintf( file, "%s/%s", config_get_doc_root_path(), YADA_SCPD_FILENAME );
#endif
   if( stat( file, &file_info ) == -1 )
   {
      /* Need to create the file. */
      FILE *scpd;

      scpd = fopen( file, "wt" );
      if( scpd != NULL )
      {
         fprintf( scpd, YADA_PLAIN_SCPD, config_get_uuid() );
         fflush( scpd );
         fclose( scpd );
      }
      else
      {
         return DLNA_INIT_ERROR;
      }
   }

   return DLNA_SUCCESS;
}


/**
 * Initialize the DMS.
 *
 * @param 
 *
 * @return 0 if successful or error code
 */
int yada_init( char *config_file )
{
   int rc = 0;

   upnp_init_param upnp_param; 
   httpd_init_param httpd_param;

   /* Initialize logger. */
   logger_init();

   if( config_load( config_file ) != CONFIG_SUCCESS )
   {
      logger_log( LOG_ERROR, LOG_MSG("problems reading configuration file %s"), config_file );
      return DLNA_INIT_ERROR;
   }

   /* Initialize the Content Directory "Server" */
   if( cds_init() != CDS_SUCCESS )
   {
      return DLNA_INIT_ERROR;
   }

   yada_create_SCPD();
   //cms_create_SCPD();
   //cds_create_SCPD();

   /* Initialize socket engine (on Windows). */
   if( yada_socket_init() != DLNA_SUCCESS )
   {
      return DLNA_INIT_ERROR;
   }

   /* Start the HTTP server. */
   httpd_param.ip_address = config_get_ip_address();
   httpd_param.port = config_get_port();
   httpd_param.doc_root = config_get_doc_root_path();
   httpd_server_start( &httpd_param );

   /*
    * Set the UPnP initialization parameters 
    * and start the UPnP engine. config_get_ip_address()
    * and config_get_port() could have given NULL and 0, 
    * respectively, so let's assign now what the HTTPD
    * server decided to use. 
    */
   upnp_param.ip_address = httpd_get_ip_address();
   upnp_param.port = httpd_get_port();
   upnp_param.location = httpd_get_root_name();
   strcpy( upnp_param.uuid, config_get_uuid() );
   upnp_param.allowed_ips = config_get_allowed_ips();
   if( (rc = upnp_init(&upnp_param)) != UPNP_SUCCESS )
   {
     logger_log( LOG_ERROR, LOG_MSG("upnp_init failed") );
     return DLNA_INIT_ERROR;
   }

   return rc;
} /* yada_init */

/**
 * Re-initialize the DMS.
 *
 * @param config_file xml configuration file
 *
 * @return 0 if successful or error code
 */
int yada_reinit( char *config_file )
{
   yada_shutdown();
   return yada_init( config_file );
} /* yada_reinit

/**
 * Terminates the DMS.
 */
void yada_shutdown()
{
   config_unload();
   upnp_shutdown();
   httpd_server_stop();
   yada_socket_cleanup();
} /* yada_shutdown */
