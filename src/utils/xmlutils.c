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

#include "xmlutils.h"


xmlNode *xml_first_node_by_name( xmlNode *root_node, const char *node_name )
{
   xmlNode *cur_node = NULL;
   for( cur_node = root_node->children; cur_node; cur_node = cur_node->next )
   {
      if( !strcmp(cur_node->name, node_name) ) break;
   }

   return cur_node;
} /* xml_first_node_by_name */

xmlNode *xml_next_sibling_by_name( xmlNode *node, const char *node_name )
{
   xmlNode *cur_node = NULL;
   for( cur_node = node->next; cur_node; cur_node = cur_node->next )
   {
      if( !strcmp(cur_node->name, node_name) ) break;
   }

   return cur_node;
} /* xml_next_sibling_by_name */

unsigned int xml_num_children( xmlNode *root_node )
{
   xmlNode *cur_node = NULL;
   unsigned int count = 0;

   for( cur_node = root_node->children; cur_node; cur_node = cur_node->next) 
      if( cur_node->type == XML_ELEMENT_NODE ) count++;

   return count;
} /* xml_num_children */


xmlNode *xml_get_soap_body( char *soap_action )
{
   xmlDoc *doc;
   xmlNode *root_node, 
           *body_node;

   doc = xmlReadMemory( soap_action, strlen(soap_action), "", NULL, 0 );
   if( doc == NULL )
   {
      return NULL;
   }

   root_node = xmlDocGetRootElement( doc );
   if( root_node == NULL )
   {
      return NULL;
   }

   /* Get the Body node. */
   body_node = xml_first_node_by_name( root_node, "Body" );
   if( body_node == NULL )
   {
      return NULL;
   }

   return body_node;
} /* cds_get_soap_body */



static
char *xml_get_string_attribute( xmlNodePtr node, const char *attribute )
{
   xmlAttr *attr = node->properties;
   while( attr ) 
   {
      if( strcmp(attr->name, attribute) == 0 )
      {
         return attr->children->content;
      }
      attr = attr->next;
   } 
   return NULL;
}

static
long xml_get_long_attribute( xmlNodePtr node, const char *attribute )
{
   xmlAttr *attr = node->properties;
   while( attr ) 
   {
      if( strcmp(attr->name, attribute) == 0 )
      {
         return atol( attr->children->content );
      }
      attr = attr->next;
   } 
   return 0;
}
