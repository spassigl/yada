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

/**
 * Extremely simple implementation of a UPnP stack.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  define socket_t SOCKET
#  define millisleep(x) SleepEx(x, TRUE)
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  define socket_t int
#  define millisleep(x) usleep((x)*1000)
#endif

#include "pthread.h"

#include "logger.h"

#include "upnp-core.h"

#define UPNP_PRODUCT_NAME "YADA-UPNP"
#define UPNP_PRODUCT_VERSION "1.0"

#define UPNP_MULTICAST_ADDR "239.255.255.250"
#define UPNP_SSDP_PORT "1900"

/* Notification Types (NT) for root devices and embedded devices and services */
#define UPNP_ROOTDEVICE_NT "upnp:rootdevice"
#define UPNP_MEDIASERVER_NT "urn:schemas-upnp-org:device:MediaServer:1"
#define UPNP_CONTENTDIRECTORY_NT "urn:schemas-upnp-org:service:ContentDirectory:1"
#define UPNP_CONNECTIONMANAGER_NT "urn:schemas-upnp-org:service:ConnectionManager:1"
#ifdef UPNP_ADD_MS_X
#endif
typedef enum
{
    INVALID_NT,
    ROOTDEVICE_NT,
    MEDIASERVER_NT,
    CONTENTDIRECTORY_NT,
    CONNECTIONMANAGER_NT
#ifdef UPNP_ADD_MS_X
#endif
} UPNP_NT_TYPE;

/* Search Types (ST) for root devices and embedded devices and services */
#define UPNP_ALL_ST "ssdp:all"
#define UPNP_ROOTDEVICE_ST "upnp:rootdevice"
#define UPNP_MEDIASERVER_ST "urn:schemas-upnp-org:device:MediaServer:1"
#define UPNP_CONTENTDIRECTORY_ST "urn:schemas-upnp-org:service:ContentDirectory:1"
#define UPNP_CONNECTIONMANAGER_ST "urn:schemas-upnp-org:service:ConnectionManager:1"
typedef enum
{
    INVALID_ST,
    ALL_ST,
    ROOTDEVICE_ST,
    MEDIASERVER_ST,
    CONTENTDIRECTORY_ST,
    CONNECTIONMANAGER_ST
#ifdef UPNP_ADD_MS_X
#endif
} UPNP_ST_TYPE;

/* XML Description file names. */
#define UPNP_ROOTDEVICE_XML "yada.xml"
#define UPNP_CONTENTDIRECTORY_XML "cds.xml"
#define UPNP_CONNECTIONMANAGER_XML "cms.xml"

/* Helper macros to define which message has been received */
#define UPNP_MSEARCH_RECEIVED(buf) strncmp(buf, "M-SEARCH", 8)
#define UPNP_NOTIFY_RECEIVED(buf) strncmp(buf, "NOTIFY", 8)

/*
 * Validity of the advertisements, in seconds. This 
 * will be set in the CACHE-CONTROL max-age header.
 * DLNA Requirement [7.2.4.6]: The CACHE-CONTROL value 
 * should be at least 1800, as recommended in the 
 * UPnP device architecture.
 */
#define UPNP_MAX_AGE "1800"


/* 
 * ssdp:alive message templates 
 */

#define UPNP_ALIVE_MSG_NO_NT \
   "NOTIFY * HTTP/1.1\r\n" \
   "HOST: " UPNP_MULTICAST_ADDR ":" UPNP_SSDP_PORT "\r\n" \
   "CACHE-CONTROL: max-age="UPNP_MAX_AGE"\r\n" \
   "LOCATION: http://%s:%d/%s/"UPNP_ROOTDEVICE_XML"\r\n" \
   "NT: uuid:%s\r\n" \
   "NTS: ssdp:alive\r\n" \
   "USN: uuid:%s\r\n" \
   "SERVER: %s UPnP/1.0 "UPNP_PRODUCT_NAME"/"UPNP_PRODUCT_VERSION"\r\n" \
   "CONTENT-LENGTH: 0\r\n\r\n"


#define UPNP_ALIVE_MSG(NT) \
   "NOTIFY * HTTP/1.1\r\n" \
   "HOST: " UPNP_MULTICAST_ADDR ":" UPNP_SSDP_PORT "\r\n" \
   "CACHE-CONTROL: max-age="UPNP_MAX_AGE"\r\n" \
   "LOCATION: http://%s:%d/%s/"UPNP_ROOTDEVICE_XML"\r\n" \
   "NT: " NT "\r\n" \
   "NTS: ssdp:alive\r\n" \
   "USN: uuid:%s::" NT "\r\n" \
   "SERVER: %s UPnP/1.0 "UPNP_PRODUCT_NAME"/"UPNP_PRODUCT_VERSION"\r\n" \
   "CONTENT-LENGTH: 0\r\n\r\n"

/*
 * ssdp:byebye message templates
 */
#define UPNP_BYEBYE_MSG(NT) \
   "NOTIFY * HTTP/1.1\r\n" \
   "HOST: " UPNP_MULTICAST_ADDR ":" UPNP_SSDP_PORT "\r\n" \
   "NT: " NT "\r\n" \
   "NTS: ssdp:byebye\r\n" \
   "USN: uuid:%s::" NT "\r\n" \
   "CONTENT-LENGTH: 0\r\n\r\n"

/*
 * M-SEARCH reply message
 */
#define UPNP_MSEARCH_REPLY_MSG(ST)\
   "HTTP/1.1 200 OK\r\n" \
   "CACHE-CONTROL: max-age = 1800\r\n" \
   "EXT:\r\n" \
   "LOCATION: http://%s:%d/%s/%s\r\n" \
   "ST: " ST "\r\n" \
   "USN: uuid:%s::" ST "\r\n" \
   "SERVER: %s UPnP/1.0 "UPNP_PRODUCT_NAME"/"UPNP_PRODUCT_VERSION"\r\n" \
   "CONTENT-LENGTH: 0\r\n\r\n"

/*
 * Globals
 */

typedef struct upnp_context
{
    /* 1 if upnp is initialised, 0 otherwise */
    unsigned char upnp_initialized;

    /* Initialization parameters */
    upnp_init_param init_param;

    /* The UPnP server uuid */
    unsigned char uuid[37];

    /* OS Name and Version */
    char os_name_version[80];

    /* Discover thread variables */
    pthread_t discover_thread;  /* The actual server thread */
    int discover_run;           /* This will be set to 0 to stop the discover thread */
    pthread_mutex_t discover_mutex;

    /* Alive thread variables */
    pthread_t alive_thread;     /* The actual alive thread */
    int alive_run;              /* This will be set to 0 to stop the alive thread */
    pthread_mutex_t alive_mutex;
} upnp_context;

/* Set upnp_initialized to 0 */
static upnp_context g_context = { 0 };


/*
 *
 * Private APIs
 *
 */



/**
 * Initialize the Socket engine,needed under Windows
 * @return UPNP_SUCCESS or UPNP_SOCKET_ERROR
 */
static int upnp_socket_init()
{
#ifdef WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return UPNP_SOCKET_ERROR;
    }

    return UPNP_SUCCESS;
#else
    return UPNP_SUCCESS;
#endif
}                               /* upnp_socket_initws() */

/**
 * Terminates the winsock engine under Windows
 */
static void upnp_socket_cleanup()
{
#ifdef WIN32
    WSACleanup();
#endif
}                               /* upnp_socket_cleanupws */

/**
 * Creates a multicast datagram socket 
 * and binds it to the given ip address and port 1900
 */
static socket_t upnp_new_ssdp_server_socket()
{
    socket_t s;
    struct sockaddr_in serv_addr;
    struct in_addr mcaddr;
    int res;
    struct ip_mreq mreq;
    unsigned char ttl = 1;
    unsigned char yes = 1;

    logger_log(LOG_TRACE, LOG_MSG(""));

    /* Create a datagram socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);

    /* Tell the socket to reuse address */
    res = setsockopt(s, IPPROTO_IP, SO_REUSEADDR, &yes, sizeof(yes));

    /* Bind the socket to a specific port */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(UPNP_SSDP_PORT));
    res = bind(s, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (res < 0) {
        logger_log(LOG_ERROR, LOG_MSG("could not bind socket, error %d"),
                   res);
        /* This is a critical error */
        shutdown(s, SD_BOTH);
        return UPNP_SOCKET_ERROR;
    }

    /* Set the socket as belonging to a multicast group */
    mreq.imr_multiaddr.s_addr = inet_addr(UPNP_MULTICAST_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    res =
        setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *) &mreq,
                   sizeof(mreq));
    if (res < 0) {
        logger_log(LOG_ERROR,
                   LOG_MSG
                   ("could not add multicast membership to socket, error %d"),
                   res);
        /* This is another critical error */
        shutdown(s, SD_BOTH);
        return UPNP_SOCKET_ERROR;
    }

    /* Set multicast interface */
    memset((void *) &mcaddr, 0, sizeof(struct in_addr));
    mcaddr.s_addr = inet_addr(g_context.init_param.ip_address);
    res =
        setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char *) &mcaddr,
                   sizeof(struct in_addr));
    if (res < 0) {
        logger_log(LOG_ERROR,
                   LOG_MSG
                   ("could not set multicast interface for the socket, error %d"),
                   res);
        /* This is another critical error */
        shutdown(s, SD_BOTH);
        return UPNP_SOCKET_ERROR;
    }

    /* Set TTL */
    res = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    res = setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *) &yes, sizeof(yes));
    if (res < 0) {
        logger_log(LOG_ERROR,
                   LOG_MSG
                   ("could not set broadcast flag for the socket, error %d"),
                   res);
        /* This is another critical error */
        shutdown(s, SD_BOTH);
        return UPNP_SOCKET_ERROR;
    }

    /* Make socket non-blocking */
#ifdef WIN32
    res = ioctlsocket(s, FIONBIO, (u_long *) & yes);
#else
    res = fcntl(s, F_GETFL, 0);
    res = fcntl(s, F_SETFL, res | O_NONBLOCK);
#endif

    return s;
}                               /* upnp_new_ssdp_server_socket */

/**
 * Creates a multicast datagram socket that is used to send upnp messages
 *
 * @return A Socket descriptor or -1 otherwise
 */
static socket_t upnp_new_ssdp_client_socket()
{
    socket_t ssdp_socket;
    unsigned long reply_addr;
    /* 
     * As per UPnP specs:
     * "To limit network congestion, the time-to-live (TTL) of each 
     * IP packet for each multicast message SHOULD default to 2 and
     * SHOULD be configurable.
     */
    int ttl = 2;

    ssdp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (ssdp_socket < 0) {
        logger_log(LOG_TRACE,
                   LOG_MSG("error creating socket to send alive messages"));
        return UPNP_SOCKET_ERROR;
    }

    /* Set multicast interface */
    reply_addr = inet_addr(g_context.init_param.ip_address);
    setsockopt(ssdp_socket, IPPROTO_IP, IP_MULTICAST_IF, (char *) &reply_addr,
               sizeof(reply_addr));

    /* Set multicast ttl */
    setsockopt(ssdp_socket, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &ttl,
               sizeof(ttl));

    return ssdp_socket;
}                               /* upnp_new_ssdp_client_socket */

/** 
 * Does the actual sending of upnp messages over the wire
 *
 * @param s The socket used to send, created in upnp_send_alive()
 * @param msg_fmt The upnp message format, see #defines
 */
static int upnp_send_message(socket_t s, char *ip_address, int port, 
                             char *fmt, ...)
{
    char msg[512];
    int msglen;
    int res = 0;
    struct sockaddr_in dest_addr;
    int socklen = sizeof(struct sockaddr_in);
    va_list arglist;


    /* Create the destination address */
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(ip_address);
    dest_addr.sin_port = htons(port);

    /* Create the message */
    va_start(arglist, fmt);
    msglen = vsprintf(msg, fmt, arglist);
    va_end(arglist);
    if (msglen < 0) {
        logger_log(LOG_ERROR, LOG_MSG("error during message composition"));
        return UPNP_INVALID_MESSAGE_ERROR;
    }

    /* Send the message */
    res = sendto(s, msg, msglen, 0, (struct sockaddr *) &dest_addr, socklen);
    if (res < 0) {
        logger_log(LOG_ERROR, LOG_MSG("error during sendto, res: %d"), res);
    }

    return res > 0 ? UPNP_SUCCESS : UPNP_INVALID_MESSAGE_ERROR;
}                               /* upnp_send_message */


/****************************************************************************
 *
 * upnp:alive processors
 *
 ****************************************************************************/

/*
 * Sends an advertisement group of upnp::alive 
 * messages. This sums up to the 3+2d+k messages 
 * needed for a full advertisement set. In this 
 * case d=0 (no embedded devices) and k=2 so we 
 * send out a total of 5 ssdp:alive messages.
 *
 * @return UPNP_SUCCESS if successful, 
 *    any other value otherwise.
 */
static int upnp_send_alive()
{
    int res = UPNP_SUCCESS;
    int ssdp_port = atoi(UPNP_SSDP_PORT);
    int count = 0;
    socket_t ssdp_socket = upnp_new_ssdp_client_socket();

    logger_log(LOG_TRACE, LOG_MSG("sending alive messages"));

    /* Send an advertisment set and a duplicate set. */
    for(count = 0; count<2; count++) {
           
       /*
        * 3 Root device discovery messages:
        * - upnp:rootdevice
        * - uuid:device-UUID
        * - urn:schemas-upnp-org:device:deviceType:ver
        */
       logger_log(LOG_TRACE, LOG_MSG("sending alive for " UPNP_ROOTDEVICE_NT));
       res += upnp_send_message(ssdp_socket, UPNP_MULTICAST_ADDR, ssdp_port,
                                UPNP_ALIVE_MSG(UPNP_ROOTDEVICE_NT),
                                g_context.init_param.ip_address,
                                g_context.init_param.port,
                                g_context.init_param.location, g_context.uuid,
                                g_context.os_name_version);
       logger_log(LOG_TRACE, LOG_MSG("sending alive for uuid"));
       res += upnp_send_message(ssdp_socket, UPNP_MULTICAST_ADDR, ssdp_port,
                                UPNP_ALIVE_MSG_NO_NT,
                                g_context.init_param.ip_address,
                                g_context.init_param.port,
                                g_context.init_param.location, g_context.uuid,
                                g_context.uuid, g_context.os_name_version);
       logger_log(LOG_TRACE,
                  LOG_MSG("sending alive for " UPNP_MEDIASERVER_NT));
       res += upnp_send_message(ssdp_socket, UPNP_MULTICAST_ADDR, ssdp_port,
                                UPNP_ALIVE_MSG(UPNP_MEDIASERVER_NT),
                                g_context.init_param.ip_address,
                                g_context.init_param.port,
                                g_context.init_param.location, g_context.uuid,
                                g_context.os_name_version);

       /* 
        * A discovery message for each distinct 
        * service types.
        */

       /* Content Directory. */
       logger_log(LOG_TRACE,
                  LOG_MSG("sending alive for " UPNP_CONTENTDIRECTORY_NT));
       res += upnp_send_message(ssdp_socket, UPNP_MULTICAST_ADDR, ssdp_port,
                               UPNP_ALIVE_MSG(UPNP_CONTENTDIRECTORY_NT),
                               g_context.init_param.ip_address,
                               g_context.init_param.port,
                               g_context.init_param.location, g_context.uuid,
                               g_context.os_name_version);

       /* Connection Manager. */
       logger_log(LOG_TRACE, LOG_MSG("sending alive for " UPNP_CONNECTIONMANAGER_NT));
       res += upnp_send_message(ssdp_socket, UPNP_MULTICAST_ADDR, ssdp_port,
                                UPNP_ALIVE_MSG(UPNP_CONNECTIONMANAGER_NT),
                                g_context.init_param.ip_address,
                                g_context.init_param.port,
                                g_context.init_param.location, g_context.uuid,
                                g_context.os_name_version);
    }

    shutdown(ssdp_socket, SD_BOTH);
    closesocket(ssdp_socket);

    /* res should be zero (UPNP_SUCCESS) */
    return res;
}                               /* upnp_send_alive */


/*
 * Alive thread function
 * Sends announcements periodically
 */
static void *upnp_alive_thread_proc(void *arg)
{
    int sleep_time_ms;
    int res;

    srand((unsigned) time(NULL));

    /* Compute advertisement refresh interval. 
     * From the UPnP specs:
     * "it is RECOMMENDED that such refreshing of advertisements be done 
     * at a randomly-distributed interval of less than one-half of the 
     * advertisement expiration time"
     * Set this between 10sec and UPNP_MAX_AGE/2
     */
    sleep_time_ms = atoi(UPNP_MAX_AGE);
    sleep_time_ms =
        (int)((double) rand() / (RAND_MAX + 1) * (sleep_time_ms / 2 - 10) + 10);
    sleep_time_ms *= 1000;
    logger_log(LOG_INFO,
               LOG_MSG("advertisement refresh interval: %d seconds"),
               sleep_time_ms / 1000);

    /* Send initial advertisement group. */
    pthread_mutex_lock(&g_context.alive_mutex);
    res = upnp_send_alive();
    pthread_mutex_unlock(&g_context.alive_mutex);
    if (res != UPNP_SUCCESS) {
        logger_log(LOG_ERROR,
                   LOG_MSG
                   ("failed to send alive messages, retrying in %d sec"),
                   sleep_time_ms / 1000);
    }

    /* Repeat forever or until thread is killed. */
    while (g_context.alive_run) {
        millisleep(sleep_time_ms);

        pthread_mutex_lock(&g_context.alive_mutex);
        res = upnp_send_alive();
        pthread_mutex_unlock(&g_context.alive_mutex);

        if (res != UPNP_SUCCESS) {
            logger_log(LOG_ERROR,
                       LOG_MSG
                       ("failed to send alive messages, retrying in %d sec"),
                       sleep_time_ms / 1000);
        }
    }

    logger_log(LOG_INFO, LOG_MSG("alive thread now stopped"));

    return arg;
}                               /* upnp_alive_thread_proc */


/****************************************************************************
 *
 * upnp:byebye processors
 *
 ****************************************************************************/


/*
 * Sends upnp::byebye packets
 */
static int upnp_send_byebye()
{
    int res = UPNP_SUCCESS;
    int port = atoi(UPNP_SSDP_PORT);

    socket_t ssdp_socket = upnp_new_ssdp_client_socket();

    logger_log(LOG_TRACE, LOG_MSG("sending byebye messages"));

    logger_log(LOG_TRACE, LOG_MSG("sending byebye for " UPNP_ROOTDEVICE_NT));
    res += upnp_send_message(ssdp_socket, UPNP_MULTICAST_ADDR, port,
                             UPNP_BYEBYE_MSG(UPNP_ROOTDEVICE_NT),
                             g_context.uuid);

    logger_log(LOG_TRACE, LOG_MSG("sending byebye for " UPNP_MEDIASERVER_NT));
    res += upnp_send_message(ssdp_socket, UPNP_MULTICAST_ADDR, port,
                             UPNP_BYEBYE_MSG(UPNP_MEDIASERVER_NT),
                             g_context.uuid);

    logger_log(LOG_TRACE,
               LOG_MSG("sending byebye for " UPNP_CONNECTIONMANAGER_NT));
    res += upnp_send_message(ssdp_socket, UPNP_MULTICAST_ADDR, port,
                             UPNP_BYEBYE_MSG(UPNP_CONNECTIONMANAGER_NT),
                             g_context.uuid);

    logger_log(LOG_TRACE,
               LOG_MSG("sending byebye for " UPNP_CONTENTDIRECTORY_NT));
    res += upnp_send_message(ssdp_socket, UPNP_MULTICAST_ADDR, port,
                             UPNP_BYEBYE_MSG(UPNP_CONTENTDIRECTORY_NT),
                             g_context.uuid);

    shutdown(ssdp_socket, SD_BOTH);
    closesocket(ssdp_socket);

    return res;
}                               /* upnp_send_byebye */


/****************************************************************************
 *
 * M-SEARCH processors
 *
 ****************************************************************************/

/* example M-SEARCH message 
static char *test_msearch = 
"M-SEARCH * HTTP/1.1\r\n"
"HOST: 239.255.255.250:1900\r\n"
"MAN: \"ssdp:discover\"\r\n"
"MX: 3\r\n"
"ST: urn:schemas-upnp-org:device:MediaServer:1\r\n"
"CONTENT-LENGTH: 0\r\n\r\n";
*/

static UPNP_ST_TYPE upnp_get_msearch_type(char *buf)
{
    char *st;

    /* MAN is required, just checking for it to be part of the message */
    if (strstr(buf, "MAN") == NULL)
        return INVALID_ST;

    /* ST is required. Get the message type */ ;
    if ((st = strstr(buf, "ST")) == NULL)
        return INVALID_ST;
    if (strstr(st, UPNP_ALL_ST) != NULL)
        return ALL_ST;
    if (strstr(st, UPNP_ROOTDEVICE_ST) != NULL)
        return ROOTDEVICE_ST;
    if (strstr(st, UPNP_MEDIASERVER_ST) != NULL)
        return MEDIASERVER_ST;
    if (strstr(st, UPNP_CONTENTDIRECTORY_ST) != NULL)
        return CONTENTDIRECTORY_ST;
    if (strstr(st, UPNP_CONNECTIONMANAGER_ST) != NULL)
        return CONNECTIONMANAGER_ST;
    return INVALID_ST;
}                               /* upnp_get_msearch_type */

/*
 * Process an M-SEARCH message replying with the right response 
 * for the requested ST
 */
static int upnp_send_msearch_reply(char *buf, char *ip_address, int port)
{
    UPNP_ST_TYPE msearch_type;
    int res = UPNP_SUCCESS;
    socket_t ssdp_socket;

    msearch_type = upnp_get_msearch_type(buf);
    if (msearch_type == INVALID_ST) {
        return UPNP_INVALID_MSEARCH_MESSAGE_ERROR;
    }

    /* 
     * Message is OK, send reply back to the client 
     */

    ssdp_socket =
        upnp_new_ssdp_client_socket(g_context.init_param.ip_address);

    logger_log(LOG_TRACE, LOG_MSG("sending M-SEARCH reply to %s:%d"),
               ip_address, port);

    if ((msearch_type == ROOTDEVICE_ST) || (msearch_type == ALL_ST)) {
        logger_log(LOG_TRACE,
                   LOG_MSG("sending M-SEARCH reply for " UPNP_ROOTDEVICE_ST));
        res += upnp_send_message(ssdp_socket, ip_address, port,
                                 UPNP_MSEARCH_REPLY_MSG(UPNP_ROOTDEVICE_ST),
                                 g_context.init_param.ip_address,
                                 g_context.init_param.port,
                                 g_context.init_param.location, 
                                 UPNP_ROOTDEVICE_XML,
                                 g_context.uuid,
                                 g_context.os_name_version);
    }
    if ((msearch_type == MEDIASERVER_ST) || (msearch_type == ALL_ST)) {
        logger_log(LOG_TRACE,
                   LOG_MSG("sending M-SEARCH reply for "
                           UPNP_MEDIASERVER_ST));
        res += upnp_send_message(ssdp_socket, ip_address, port,
                                 UPNP_MSEARCH_REPLY_MSG(UPNP_MEDIASERVER_ST),
                                 g_context.init_param.ip_address,
                                 g_context.init_param.port,
                                 g_context.init_param.location, 
                                 UPNP_ROOTDEVICE_XML,
                                 g_context.uuid,
                                 g_context.os_name_version);
    }
    if ((msearch_type == CONTENTDIRECTORY_ST) || (msearch_type == ALL_ST)) {
        logger_log(LOG_TRACE,
                   LOG_MSG("sending M-SEARCH reply for "
                           UPNP_CONTENTDIRECTORY_ST));
        res += upnp_send_message(ssdp_socket, ip_address, port,
                                 UPNP_MSEARCH_REPLY_MSG
                                 (UPNP_CONTENTDIRECTORY_ST),
                                 g_context.init_param.ip_address,
                                 g_context.init_param.port,
                                 g_context.init_param.location, 
                                 UPNP_CONTENTDIRECTORY_XML,
                                 g_context.uuid,
                                 g_context.os_name_version);
    }
    if ((msearch_type == CONNECTIONMANAGER_ST) || (msearch_type == ALL_ST)) {
        logger_log(LOG_TRACE,
                   LOG_MSG("sending M-SEARCH reply for "
                           UPNP_CONNECTIONMANAGER_ST));
        res += upnp_send_message(ssdp_socket, ip_address, port,
                                 UPNP_MSEARCH_REPLY_MSG
                                 (UPNP_CONNECTIONMANAGER_ST),
                                 g_context.init_param.ip_address,
                                 g_context.init_param.port,
                                 g_context.init_param.location, 
                                 UPNP_CONNECTIONMANAGER_XML,
                                 g_context.uuid,
                                 g_context.os_name_version);
    }

    shutdown(ssdp_socket, SD_BOTH);
    closesocket(ssdp_socket);

    return res;
}                               /* upnp_process_reply_msearch */


/****************************************************************************
 *
 * Discover thread
 *
 ****************************************************************************/

/*
 * Discover thread function
 */
#define UPNP_MAX_BUF_SIZE 512
static void *upnp_discover_thread_proc(void *arg)
{
    socket_t ssdp_socket;
    struct sockaddr_in cli_addr;
    int cli_length = sizeof(cli_addr);
    char buf[UPNP_MAX_BUF_SIZE];
    int res;

    logger_log(LOG_TRACE, LOG_MSG("starting discover thread  %s"),
               g_context.init_param.ip_address);

    ssdp_socket =
        upnp_new_ssdp_server_socket(g_context.init_param.ip_address);
    if (ssdp_socket == UPNP_SOCKET_ERROR) {
        logger_log(LOG_ERROR,
                   LOG_MSG
                   ("could not create discover thread socket, exiting thread"));
        return NULL;
    }

    /*
     * Start the loop and receive UPnP messages
     */
    while (g_context.discover_run) {
        pthread_mutex_lock(&g_context.discover_mutex);

        memset(buf, 0x0, UPNP_MAX_BUF_SIZE);

        /* receive message */
        res =
            recvfrom(ssdp_socket, buf, UPNP_MAX_BUF_SIZE - 1, 0,
                     (struct sockaddr *) &cli_addr, &cli_length);
        if (res > 0) {
            /* TODO: IP address filtering */
            logger_log(LOG_TRACE, LOG_MSG("Received message from %s:%u"),
                        inet_ntoa(cli_addr.sin_addr),
                        ntohs(cli_addr.sin_port));
            if (UPNP_MSEARCH_RECEIVED(buf)) {
                logger_log(LOG_TRACE, LOG_MSG("Received M-SEARCH from %s:%u"),
                           inet_ntoa(cli_addr.sin_addr),
                           ntohs(cli_addr.sin_port));
                if (upnp_send_msearch_reply(buf, 
                        inet_ntoa(cli_addr.sin_addr),
                        ntohs(cli_addr.sin_port)) != UPNP_SUCCESS) {
                    logger_log(LOG_ERROR,
                               LOG_MSG
                               ("Failed to send M-SEARCH reply to %s:%u"),
                               inet_ntoa(cli_addr.sin_addr),
                               ntohs(cli_addr.sin_port));
                }
            }
            else if (UPNP_NOTIFY_RECEIVED(buf)) {
                logger_log(LOG_TRACE, LOG_MSG("Received NOTIFY from %s:%u"),
                           inet_ntoa(cli_addr.sin_addr),
                           ntohs(cli_addr.sin_port));
            }
        }

        pthread_mutex_unlock(&g_context.discover_mutex);

    }                           /* while( g_discover_run ) */

    logger_log(LOG_INFO, LOG_MSG("discover thread now stopped"));

    return arg;
}                               /* upnp_discover_thread_proc */


/****************************************************************************
 *
 * Utilities
 *
 ****************************************************************************/


/*
 * Returns the OS info in the form OS name/OS version
 */
static void upnp_get_os_info(char *version_info)
{
#ifdef WIN32
    OSVERSIONINFO versioninfo;

    versioninfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&versioninfo) != 0) {
        sprintf(version_info, "MS-Windows/%d.%d.%d",
                versioninfo.dwMajorVersion,
                versioninfo.dwMinorVersion, versioninfo.dwBuildNumber);
    }
    else {
        *version_info = '\0';
    }
#else
    int ret_code;
    struct utsname sys_info;

    ret_code = uname(&sys_info);
    if (ret_code == -1) {
        *version_info = '\0';
    }
    else {
        sprintf(version_info, "%s/%s", sys_info.sysname, sys_info.release);
    }
#endif
}                           /* upnp_get_os_info */

static
int ip4_matchnet( const struct in6_addr *ip, const struct in_addr *net, const unsigned char mask )
{
	struct in_addr m;
	/* constuct a bit mask out of the net length.
	 * remoteip and ip are network byte order, it's easier
	 * to convert mask to network byte order than both
	 * to host order. It's ugly, isn't it? */
	m.s_addr = htonl(-1 - ((1 << (32 - mask)) - 1));

	return ((ip->s6_addr[3] & m.s_addr) == net->s_addr);
}

/****************************************************************************
 *
 * Init and shutdown
 *
 ****************************************************************************/


/**
 * Initialize the UPnP engine, starting the SSDP server on port 1900
 *
 * @param init_param The initialization paramenters. Need to specify the host IP address 
                     to use, in string format, for example "192.168.0.1". If ip_address field is NULL
                     the first available ip address will be selected.
                     If port field is 0, a default port will be used
                     If init_param is NULL, the first availabe ip address and port will be used.
 *
 * @return UPNP_SUCCESS if all the threads are started successfully, UPNP_INIT_ERROR otherwise. 
 */
int upnp_init(upnp_init_param *init_param)
{
     if (g_context.upnp_initialized == 0) {
         if (upnp_socket_init() != UPNP_SUCCESS) {
             logger_log(LOG_ERROR,
                        LOG_MSG("could not initialize socket engine"));
             /* This is a critical error */
             return UPNP_INIT_ERROR;
         }

         /* 
          * Initialize some context variables 
          */
         memcpy(&g_context.init_param, init_param,
                sizeof(upnp_init_param));

         upnp_get_os_info(g_context.os_name_version);

         g_context.discover_run = 1;
         g_context.alive_run = 1;

         /* DLNA Requirement [7.2.4.9]: Upon startup, UPnP devices 
          * should broadcast an ssdp:byebye before sending the initial 
          * ssdp:alive onto the local network.
          */
         logger_log( LOG_INFO, LOG_MSG("sending upnp:byebye messages") );
         upnp_send_byebye();

         /* Fire the UPnP discover thread on port 1900 */
         logger_log(LOG_INFO, LOG_MSG("starting discover thread..."));
         pthread_mutex_init(&g_context.discover_mutex, NULL);
         if (pthread_create(&g_context.discover_thread, NULL, upnp_discover_thread_proc,
                            NULL) != 0) {
             logger_log(LOG_ERROR,
                        LOG_MSG("could not start discover thread"));
             /* This is a critical error */
             return UPNP_INIT_ERROR;
         }
         logger_log(LOG_INFO, LOG_MSG("discover thread started"));

         /* Fire the alive thread to send announcements */
         logger_log(LOG_INFO, LOG_MSG("starting alive thread..."));
         pthread_mutex_init(&g_context.alive_mutex, NULL);
         if (pthread_create(&g_context.alive_thread, NULL, upnp_alive_thread_proc,
                            NULL) != 0) {
             logger_log(LOG_ERROR,
                        LOG_MSG("could not start alive thread"));
             /* This is a critical error */
             return UPNP_INIT_ERROR;
         }
         logger_log(LOG_INFO, LOG_MSG("alive thread started"));

         g_context.upnp_initialized = 1;
     }

     return UPNP_SUCCESS;
 }                           /* upnp_init */


/**
* Shuts down the UPnP engine
*/
 void upnp_shutdown()
 {
     if (g_context.upnp_initialized == 1) {

         /* Stop alive thread */
         pthread_mutex_lock(&g_context.alive_mutex);
         logger_log( LOG_INFO, LOG_MSG("shutting down UPnP engine: alive thread") );
         g_context.alive_run = 0;
         pthread_mutex_unlock(&g_context.alive_mutex);
         pthread_cancel(g_context.alive_thread);
         pthread_mutex_destroy(&g_context.alive_mutex);

         /* Stop discover thread */
         logger_log( LOG_INFO, LOG_MSG("shutting down UPnP engine: discover thread") );
         pthread_mutex_lock(&g_context.discover_mutex);
         g_context.discover_run = 0;
         pthread_mutex_unlock(&g_context.discover_mutex);
         pthread_cancel(g_context.discover_thread);
         pthread_mutex_destroy(&g_context.discover_mutex);

         /* Send byebye messages */
         logger_log( LOG_INFO, LOG_MSG("sending upnp:byebye messages") );
         upnp_send_byebye();

         /* Time's up for sockets */
         upnp_socket_cleanup();

         g_context.upnp_initialized = 0;
     }
 }                           /* upnp_shutdown */


 /**
 * @return the UPnP product name
 */
char *upnp_get_product_name()
{
   return UPNP_PRODUCT_NAME;
} /* upnp_get_product_name */

/**
 * @return the UPnP product version
 */
char *upnp_get_product_version()
{
   return UPNP_PRODUCT_VERSION;
} /* upnp_get_product_version */

/**
 * @return 0 if device is not allowed, 1 otherwise.
 */
unsigned char upnp_is_allowed_device( char *ip_address )
{
   /* FIXME: to be implemented */
   return 1;
} /* upnp_is_allowed_device */
