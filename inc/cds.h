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


#ifndef __CDS_H
#define __CDS_H


#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/*
 * URL for CDS
 */
#define CDS_CONTROL_URL "/cds/control/ContentDirectory1"
#define CDS_EVENT_URL "/cds/event/ContentDirectory1"
#define CDS_SCPD "cds.xml"

enum
{
   CDS_SUCCESS = 1,
   CDS_402_ERROR = -402, /* (Invalid Args) */
   CDS_501_ERROR = -501, /* (Action Failed) */
   CDS_701_ERROR = -701, /* (No Such Object) */
   CDS_709_ERROR = -709, /* (Unsupported or invalid sort criteria) */
   CDS_720_ERROR = -720  /* (Cannot process the request) */
};


/*
 * Initialize the CDS.
 *
 * @return CDS_SUCCESS if successful, another value otherwise.
 */
int cds_init();

/*
 * Re-initialize the CDS.
 *
 * @return CDS_SUCCESS if successful, another value otherwise.
 */
int cds_reinit();

/**
 * Returns the SCPD description of the CDS service as per
 * the UPnP specifications.
 * @return A pointer to the XML description string
 *    for the CDS service. The returned pointer points
 *    to a static memory area so need not be freed.
 */
char *cds_get_scpd();

/*
 * UPnP ContentDirectory1 Actions.
 * We keep the capitalised name formatting style, deviating
 * from the C formatting adopted in the rest of the YADA
 * implementation to be aligned to the UPnP formatting
 * style.
 */
#define CDS_GET_SEARCH_CAPS_ACTION "GetSearchCapabilities"
#define CDS_GET_SORT_CAPS_ACTION "GetSortCapabilities"
#define CDS_GET_SYSTEM_UPDATE_ID_ACTION "GetSystemUpdateID"
#define CDS_BROWSE_ACTION "Browse"

/* Samsung specific actions. */
#define CDS_SEC_GET_OBJECT_ID_FROM_ID_ACTION "X_GetObjectIDfromIndex"

/**
 * GetSearchCapabilities Action.
 *
 * @param GetSearchCapsResponse The GetSearchCapabilitiesResponse
 *    in XML format. This is a newly allocated buffer
 *    so memory must be freed after calling this function.
 * @return CDS_SUCCESS is successful, or the UPnP defined error codes
 *    for the Browse action: 
 *    402 (Invalid Args)
 *    501 (Action Failed)
 */
int cds_GetSearchCapabilities( char **GetSearchCapabilitiesResponse );

/**
 * GetSortCapabilities Action.
 *
 * @param GetSortCapabilitiesResponse The GetSortCapabilitiesResponse
 *    in XML format. This is a newly allocated buffer
 *    so memory must be freed after calling this function.
 * @return CDS_SUCCESS is successful, or the UPnP defined error codes
 *    for the Browse action: 
 *    402 (Invalid Args)
 *    501 (Action Failed)
 */
int cds_GetSortCapabilities( char **GetSearchCapabilitiesResponse );

/**
 * GetSystemUpdateID Action.
 *
 * @param GetSystemUpdateIDResponse The GetSystemUpdateIDResponse
 *    in XML format. This is a newly allocated buffer
 *    so memory must be freed after calling this function.
 * @return CDS_SUCCESS is successful, or the UPnP defined error codes
 *    for the Browse action: 
 *    402 (Invalid Args)
 *    501 (Action Failed)
 */
int cds_GetSystemUpdateID( char **GetSystemUpdateIDResponse );

/**
 * Browse Action.
 *
 * @param soap_action_body The Browse request body soap action, including the envelope,
 *    in XML format. E.g.
      <s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
         <s:Body>
            <u:Browse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
            ...   
            </u:Browse>
         </s:Body>
      </s:Envelope>
 * @param browse_result The BrowseResponse result body, including the envelope, in 
 *    XML format. E.g.
      <s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
         <s:Body>
            <u:BrowseResponse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
               <Result>
                  ...
               </Result>
               <NumberReturned>...</NumberReturned>
               <TotalMatches>...</TotalMatches>
               <UpdateID>...</UpdateID>
            </u:BrowseResponse>
         </s:Body>
      </s:Envelope>
 * @return CDS_SUCCESS is successful, or the UPnP defined error codes
 *    for the Browse action: 
 *    402 (Invalid Args)
 *    501 (Action Failed)
 *    701 (No Such Object)
 *    709 (Unsupported or invalid sort criteria)
 *    720 (Cannot process the request)
 */
int cds_Browse( char *soap_action_body, char **BrowseResponse );

/**
 * X_GetObjectIDfromIndex Action. This I suppose is used by Samsung
 * MediaRenderer to map child number X to the internal ID used by
 * the Samsung MediaServer (or the Samsung HTTP Streaming Server) and
 * when browsing the contents of a folder on the TV. There is a
 * X_GetObjectIDfromIndex request for each item visible on the 
 * screen in a certain moment in time.
 * I quite do not get why this action is needed, really...
 *
 * @param soap_action_body The request body, including the envelope,
 *    in XML format. This is defined as, e.g.
      <s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
         <s:Body>
            <u:X_GetObjectIDfromIndex xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
               <CategoryType>22</CategoryType>
               <Index>216</Index>
            </u:X_GetObjectIDfromIndex>
         </s:Body>
      </s:Envelope>
 * @param X_GetObjectIDfromIndexResponse The X_GetObjectIDfromIndexResponse
 *    result body in XML format. This is defined as, eg.
      <s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
         <s:Body>
            <u:X_GetObjectIDfromIndexResponse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
               <ObjectID>A_F_0000_217</ObjectID>
            </u:X_GetObjectIDfromIndexResponse>
         </s:Body>
      </s:Envelope>
 * @return CDS_SUCCESS is successful, or 402 otherwise. (Note this is
 *    not documented so we are just guessing a meaningful value).
 */
int cds_X_GetObjectIDfromIndex( char *soap_action_body, char **X_GetObjectIDfromIndexResponse );

/**
 * CDS Action dispatcher.
 *
 * @param soap_action The soap action string (the one in the HTTP header)
 * @param soap_action_body The soap action body, including the envelope, in XML format.
 * @action_response The action response, including the envelope, in XML format.
 * @return CDS_SUCCESS if successful, or another value otherwise, depending on the
 *    UPnP error derived during the action processing.
 */
int cds_dispatch_action( char *soap_action, char *soap_action_body, char **action_response );


DLLEXPORT void cds_test();


#ifdef __cplusplus
}
#endif


#endif __CDS_H