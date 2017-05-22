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


#ifndef __CMS_H
#define __CMS_H


#ifdef __cplusplus
extern "C" {
#endif

#include "upnp-types.h"

/*
 * URL for CMS
 */
#define CMS_CONTROL_URL "/cms/control/ConnectionManager1"
#define CMS_EVENT_URL "/cms/event/ConnectionManager1"
#define CMS_SCPD "cms.xml"

enum
{
   CMS_SUCCESS = 1
};

/**
 * Returns the SCPD description of the CMS service as per
 * the UPnP specifications.
 * @return A pointer to the XML description string
 *    for the CMS service. The returned pointer points
 *    to a static memory area so need not be freed.
 */
char *cms_get_scpd();

/*
 * UPnP ConnectionManager1 Actions.
 * We keep the capitalised name formatting style, deviating
 * from the C formatting adopted in the rest of the YADA
 * implementation to be aligned to the UPnP formatting
 * style.
 */

/**
 * GetProtocolInfo Action.
 *
 * @param client The client socket invoking the action.
 * @param SourceProtocolInfo A pointer to a string that will 
 *    receive the SourceProtocolInfo string. Pointer will
 *    point to a static string so it must not be freed it after
 *    its use.
 * @param SinkProtocolInfo A pointer to a string that will 
 *    receive the SinkProtocolInfo string. Pointer will
 *    point to a static string so it must not be freed it after
 *    its use.
 */
void cms_GetProtocolInfo( int client, /*out*/ char **SourceProtocolInfo, 
                                      /*out*/ char **SinkProtocolInfo );

/**
 * GetCurrentConnectionIDs Action.
 *
 * @param client The client socket invoking the action.
 * @param ConnectionIDs The array of ConnectionID returned
 *    by this function.
 */
void cms_GetCurrentConnectionIDs( int client, /*out*/ i4 **ConnectionIDs );

/**
 * GetCurrentConnectionInfo Action.
 * Note that this CMS does not implement the PrepareForConnection
 * action therefore many parameters will take the default
 * values as per the UPnP specs.
 *
 * @param client The client socket invoking the action.
 * @param ConnectionID The ConnectionID for which the
 *    information are requested.
 * @param RcsID The RcsID for which the
 *    information are requested. It will be set to -1.
 * @param AVTransportID The AVTransportID for which the
 *    information are requested. It will be set to -1-
 * @param ProtocolInfo The ProtocolInfo for which the
 *    information are requested. It will be set to the same
 *    value returned by the GetProtocolInfo action.
 * @param PeerConnectionManager The ConnectionID for which the
 *    information are requested. It will be set to NULL.
 * @param PeerConnectionID The ConnectionID for which the
 *    information are requested. It will be set to -1.
 * @param Direction The Direction for which the
 *    information are requested. It will be set to "Output".
 * @param Status The Status for which the
 *    information are requested.
 */
int cms_GetCurrentConnectionInfo( int client, /*in*/ i4 ConnectionID, 
                                              /*out*/ ui4 *RcsID, 
                                              /*out*/ i4 *AVTransportID, 
                                              /*out*/ char **ProtocolInfo, 
                                              /*out*/ char **PeerConnectionManager, 
                                              /*out*/ i4 *PeerConnectionID, 
                                              /*out*/ char **Direction, 
                                              /*out*/ char **Status );

#ifdef __cplusplus
}
#endif


#endif __CDS_H