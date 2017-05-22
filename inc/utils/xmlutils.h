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

#ifndef __XMLUTILS_H
#define __XMLUTILS_H

#include "libxml/parser.h"
#include "libxml/tree.h"


xmlNode *xml_first_node_by_name( xmlNode *root_node, const char *name );
xmlNode *xml_next_sibling_by_name( xmlNode *node, const char *node_name );
unsigned int xml_num_children( xmlNode *node );

/**
 * Given a soap action, inclusive of the soap envelope,
 * returns the Body node content as an XML node.
 *
 * @param soap_action The entire soap action
 * @return An XML node with the SOAP body, or NULL
 */
xmlNode *xml_get_soap_body( char *soap_action );


#endif