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

#ifndef __UPNP_CORE_H
#define __UPNP_CORE_H


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Typedefs and defines
 */

typedef struct upnp_init_param
{
   /* URL to the UPnP description of the root device will be
    *  http://ip_address:port/location */
   char *ip_address;
   int port;
   char *location;

   /* The server UUID */
   unsigned char uuid[37];

   /* Allowed control points IPs. */
   char **allowed_ips;
} upnp_init_param;


/* Error codes */
enum
{
   UPNP_SUCCESS = 0,
   UPNP_INIT_ERROR = -1,
   UPNP_SOCKET_ERROR = -2,
   UPNP_SERVER_ERROR = -3,
   UPNP_INVALID_MESSAGE_ERROR = -4,
   UPNP_INVALID_MSEARCH_MESSAGE_ERROR = -5
};

/*
 * Micro-UPnP APIs
 */

/**
 * Initialize the UPnP engine, starting the SSDP discovery server on port 1900
 *
 * @param init_param The initialization parameters. Need to specify the host IP address 
                     to use, in string format, for example "192.168.0.1". If ip_address field is NULL
                     the first available ip address will be selected.
                     If port field is 0, a default port will be used
                     If init_param is NULL, the first availabe ip address and port will be used.
 *
 * #return UPNP_SUCCESS if all the threads are started successfully, UPNP_INIT_ERROR otherwise. 
 */
int upnp_init( upnp_init_param *init_param );

/**
 * Shuts down the UPNP engine
 */
void upnp_shutdown();

/**
 * @return the UPnP product name
 */
char *upnp_get_product_name();

/**
 * @return the UPnP product version
 */
char *upnp_get_product_version();

/**
 * @return 0 if device is not allowed, 1 otherwise.
 */
unsigned char upnp_is_allowed_device( char *ip_address );

#ifdef __cplusplus
}
#endif

#endif __UPNP_CORE_H
