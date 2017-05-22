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

#ifndef __CONFIG_H
#define __CONFIG_H

enum
{
   CONFIG_SUCCESS = 0,
   CONFIG_LOAD_ERROR = -1
};

int config_load( char *filename );
int config_save();
int config_create_defaults( char *filename );
void config_unload();

char *config_get_version();
char *config_get_uuid();
char *config_get_announce_as();

/* HTTPD configuration parameters. */
char *config_get_ip_address();
int config_get_port();
char *config_get_doc_root_path();

/* UPnP configuration parameters. */
char **config_get_allowed_ips();

#endif
