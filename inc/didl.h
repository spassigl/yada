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



#ifndef __DIDL_H
#define __DIDL_H

#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#define DIDL_NAME = "DIDL-Lite";
#define DIDL_XMLNS = "xmlns";
#define DIDL_XMLNS_URL = "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/";
#define DIDL_XMLNS_DC = "xmlns:dc";
#define DIDL_XMLNS_DC_URL = "http://purl.org/dc/elements/1.1/";
#define DIDL_XMLNS_UPNP = "xmlns:upnp";
#define DIDL_XMLNS_UPNP_URL = "urn:schemas-upnp-org:metadata-1-0/upnp/";

#define DIDL_CONTAINER = "container";
#define DIDL_ID = "id";
#define DIDL_SEARCHABLE = "searchable";
#define DIDL_PARENTID = "parentID";
#define DIDL_RESTICTED = "restricted";
	
#define DIDL_OBJECT_CONTAINER = "object.container";

#define DIDL_RES = "res";
#define DIDL_RES_PROTOCOLINFO = "protocolInfo";


#endif __DIDL_H