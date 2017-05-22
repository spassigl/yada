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

#include <string.h>

#include "libxml/parser.h"
#include "libxml/tree.h"

#include "getcwd.h"
#include "uuid.h"
#include "upnp-core.h"
#include "xmlutils.h"
#include "logger.h"

#include "httpd.h"

#ifndef WIN32
#  include "limits.h"
#endif

#include "config.h"

/* Configurable paremeters must be placed here. */
typedef struct config_param
{
   char *config_filename;

   char *config_version;
   char *config_uuid;
   char *config_announce_as;

   /* HTTPD configuration parameters. */
   char *httpd_ip_address;
   int httpd_port;
   char *httpd_doc_root_path;
   int httpd_scms_flag;

   /* UPnP configuration parameters. */
   int upnp_check_ip;
   char **upnp_allowed_ips;

   /* CDS parameters. */
   char *cds_service_doc;
   
} config_param;

/* Global configuration parameter variable. */
static config_param g_param;

/* The configuration context. This structure 
 * holds the variables needed during configuration 
 * parsing and so.
 */
typedef struct config_context
{
   xmlDocPtr   doc;
} config_context;

/* Global configuration context variable. */
static config_context g_context;


/*----------------------------------------------------------------------------
 *
 * Configuration Parser
 *
 *--------------------------------------------------------------------------*/

/*
 * UUID value parser
 */
static
int config_parse_uuid( xmlNode *uuid_node )
{
   char *uuid = xmlNodeGetContent( uuid_node );
   if( uuid[0] == 0 )
   {
      /* Need to create a new uuid first. */
      unsigned char new_uuid[37];
      uuid_generate( new_uuid );
      xmlNodeSetContent( uuid_node, new_uuid );
      g_param.config_uuid = xmlNodeGetContent( uuid_node );
   }

   return 0;
} /* config_parse_uuid */

/*
 * announce_as value parser
 */
static
int config_parse_announce_as( xmlNode *announce_as_node )
{
   char *announce_as = xmlNodeGetContent( announce_as_node );
   if( announce_as[0] == 0 )
   {
      /* Default to YADA. */
      unsigned char forced_announce[] = "YADA";
      xmlNodeSetContent( announce_as_node, forced_announce );
      g_param.config_announce_as = xmlNodeGetContent( announce_as_node );
   }

   return 0;
} /* config_parse_uuid */

/*
 * HTTPD configuration settings parser.
 */
static
int config_parse_httpd_settings( xmlNode *httpd_node )
{
   xmlNode *node;

   node = xml_first_node_by_name( httpd_node, "ip_address" );
   if( node )
   {
      char *content = xmlNodeGetContent( node );
      if( !strcmp(content, "any") ) g_param.httpd_ip_address = NULL;
      else g_param.httpd_ip_address = content;
      logger_log( LOG_TRACE, LOG_MSG("ip_address = %s"), g_param.httpd_ip_address );
   }
   else
   {
      /* This should not really happen, but
       * we gracefully set a default value 
       */
      g_param.httpd_ip_address = NULL;
   }

   node = xml_first_node_by_name( httpd_node, "port" );
   if( node )
   {
      g_param.httpd_port = atoi( xmlNodeGetContent( node ) );
      logger_log( LOG_TRACE, LOG_MSG("port = %d"), g_param.httpd_port );
   }
   else
   {
      /* This should not really happen, but
       * we gracefully set a default value 
       */
      g_param.httpd_port = 0;
   }

   node = xml_first_node_by_name( httpd_node, "doc_root_path" );
   if( node )
   {
      g_param.httpd_doc_root_path = xmlNodeGetContent( node );
      if( !g_param.httpd_doc_root_path[0] ) 
         goto _usecwd;
      logger_log( LOG_TRACE, LOG_MSG("doc_root_path = \"%s\""), g_param.httpd_doc_root_path );
   }
   else
_usecwd:
   {
      /* We default to current working directory. 
       * Config file should be parsed at application
       * startup time so this should represent the 
       * right directory with a high degree of confidence.
       */
#ifdef WIN32
      char *path = getcwd( NULL, 0 );
#else
      char path[MAX_PATH];
      getcwd( path, MAX_PATH );
#endif
      g_param.httpd_doc_root_path = strdup( path );
#ifdef WIN32
      free( path );
#endif
      logger_log( LOG_TRACE, LOG_MSG("doc_root_path = \"%s\""), g_param.httpd_doc_root_path );
   }

   return 0;
} /* config_parse_httpd_settings */

/*
 * UPnP configuration settings parser.
 */
static
int config_parse_upnp_settings( xmlNode *upnp_node )
{
   xmlNode *node;

   node = xml_first_node_by_name( upnp_node, "allowed_ips" );
   if( node )
   {
      char *content = xmlGetProp( node, "enforce" );
      if( !strcmp(content, "yes") )
      {
         int num_ip = 0;

         /* Count the number of children first. */
         num_ip = xml_num_children( node );
         g_param.upnp_allowed_ips = calloc( num_ip+1, sizeof(char *) );

         /* Do the assignments now. */
         num_ip = 0;
         node = xml_first_node_by_name( node, "ip" );
         while( node )
         {
            g_param.upnp_allowed_ips[num_ip] = xmlNodeGetContent( node );
            logger_log( LOG_TRACE, LOG_MSG("allowed_ip = %s"), g_param.upnp_allowed_ips[num_ip] );

            num_ip++;
            node = xml_next_sibling_by_name( node, "ip" );
         }
         g_param.upnp_check_ip = 1;
      }
      else
      {
         g_param.upnp_check_ip = 0;
      }
   }
   else
   {
      g_param.upnp_check_ip = 0;
   }

   return 0;
} /* config_parse_upnp_settings */

/*
 * Content Directory Server configuration settings parser.
 */
static
int config_parse_cds_settings( xmlNode *cds_node )
{
      /* FIXME: to be completed. */
   return 0;
} /* config_parse_cds_settings */

/*
 * Content Management Server configuration settings parser.
 */
static
int config_parse_cms_settings( xmlNode *cms_node )
{
      /* FIXME: to be completed. */
   return 0;
} /* config_parse_cds_settings */

/*
 * Main configuration parser.
 */
static
int config_parse()
{
   xmlNode *root_node;
   xmlNode *node;

   root_node = xmlDocGetRootElement( g_context.doc );
   if( root_node == NULL )
   {
      return CONFIG_LOAD_ERROR;
   }

   g_param.config_version = xmlGetProp( root_node, "version" );
   if( g_param.config_version[0] != '1' && g_param.config_version[2] != '0' )
   {
      logger_log( LOG_ERROR, LOG_MSG("wrong YADA version!") );
      return CONFIG_LOAD_ERROR;
   }

   /* 
    * We are not very tolerant. All sections must be there at least. 
    * Section content parsers are a bit more tolerant though.
    */

   /* UUID */
   node = xml_first_node_by_name( root_node, "uuid" );
   if( node )
   {
      config_parse_uuid( node );
   }
   else
   {
      return CONFIG_LOAD_ERROR;
   }

   /* UUID */
   node = xml_first_node_by_name( root_node, "announce_as" );
   if( node )
   {
      config_parse_announce_as( node );
   }
   else
   {
      return CONFIG_LOAD_ERROR;
   }

   /* HTTPD Streaming Server. */
   node = xml_first_node_by_name( root_node, "httpd" );
   if( node )
   {
      config_parse_httpd_settings( node );
   }
   else
   {
      return CONFIG_LOAD_ERROR;
   }

   /* UPnP Protocol. */
   node = xml_first_node_by_name( root_node, "upnp" );
   if( node )
   {
      config_parse_upnp_settings( node );
   }
   else
   {
      return CONFIG_LOAD_ERROR;
   }
   
   /* Content Directory service. */
   node = xml_first_node_by_name( root_node, "cds" );
   if( node )
   {
      config_parse_cds_settings( node );
   }
   else
   {
      return CONFIG_LOAD_ERROR;
   }

   /* Connection Manager service. */
   node = xml_first_node_by_name( root_node, "cms" );
   if( node )
   {
      config_parse_cds_settings( node );
   }
   else
   {
      return CONFIG_LOAD_ERROR;
   }

   xmlFreeNode( root_node );

   return CONFIG_SUCCESS;
} /* config_parse */

/**
 * Load configuration from an xml file.
 *
 * @return If file is not valid or parsing fails,
 *    CONFIG_LOAD_ERROR. Otherwise, CONFIG_SUCCESS.
 */
int config_load( char *filename )
{
   g_param.config_filename = filename;
   g_context.doc = xmlReadFile(filename, "UTF-8", XML_PARSE_NOBLANKS);

   if( g_context.doc == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("error loading configuration file \"%s\""), filename );
      return CONFIG_LOAD_ERROR;
   }

   if( config_parse() != CONFIG_SUCCESS )
   {
      logger_log( LOG_ERROR, LOG_MSG("error while parsing configuration file") );
      return CONFIG_LOAD_ERROR;
   }

   return CONFIG_SUCCESS;
} /* config_load */


int config_save()
{
   /* FIMXE: to be implemented. */
   return 0;
}

int config_create_defaults( char *filename )
{
   /* FIMXE: to be implemented. */
   return 0;
}

void config_unload()
{
}

char *config_get_filename()
{
   return g_param.config_filename;
}

char *config_get_version()
{
   return g_param.config_version;
}

char *config_get_uuid()
{
   return g_param.config_uuid;
}

char *config_get_announce_as()
{
   return g_param.config_announce_as;
}

/* HTTPD configuration parameters. */
char *config_get_ip_address()
{
   return g_param.httpd_ip_address;
}

int config_get_port()
{
   return g_param.httpd_port;
}

char *config_get_doc_root_path()
{
   return g_param.httpd_doc_root_path;
}

/* UPnP configuration parameters. */
char **config_get_allowed_ips()
{
   return g_param.upnp_allowed_ips;
}
