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

#include <stdlib.h>
#include <string.h>

/* Under Win32, define inline to include ffmpeg headers */
#ifdef WIN32
#  define inline _inline
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#include "libxml/parser.h"
#include "libxml/tree.h"

#include "logger.h"
#include "md5utils.h"
#include "xmlutils.h"

//#include "upnp-types.h"

#include "item.h"

#include "cds.h"


typedef enum
{
   CDS_OBJ_ITEM = 0,
   CDS_OBJ_FOLDER,
} CDS_OBJ_TYPE;

typedef ITEM_ID OBJECT_ID;

typedef struct cds_object cds_object;
struct cds_object {

   CDS_OBJ_TYPE type;

   /* Parent object. */
   cds_object *parent;

   /* Next and previous siblings */
   cds_object *next;
   cds_object *previous;

   union
   {
      /* type == CDS_OBJ_FOLDER */
      struct
      {
         char *name;
         OBJECT_ID id;
         int num_children;
         cds_object *first_child;
         cds_object *last_child;
      };

      /* type == CDS_OBJ_ITEM */
      item_info *item;
   };
};


/* Unique IDs for the tree roots. */
#define CDS_ROOT_TREE_ID "2673a016ad6e08603d7aea0e4fed596b"
#define CDS_MUSIC_TREE_ID "e7d5184e4366142787fa4a153bcd3c6a"
#define CDS_PHOTO_TREE_ID "9007afba8fdf31332b36c8e5afb440d1"
#define CDS_VIDEO_TREE_ID "d97685b624d6c12778e7080e76b3fb3f"

/* 
 * The root tree with the three virtual folders for audio, photo
 * and video. The hierarchy is partitioned into these folders so
 * that items of different kinds do not get mixed up.
 * More initializations are carried out during cds_init().
 */
static cds_object root_tree = { CDS_OBJ_FOLDER, NULL, NULL, NULL, {"Root", CDS_ROOT_TREE_ID, 0, NULL, NULL} };
static cds_object audio_tree = { CDS_OBJ_FOLDER, NULL, NULL, NULL, {"Music", CDS_PHOTO_TREE_ID, 0, NULL, NULL} };
static cds_object photo_tree = { CDS_OBJ_FOLDER, NULL, NULL, NULL, {"Photo", CDS_MUSIC_TREE_ID, 0, NULL, NULL} };
static cds_object video_tree = { CDS_OBJ_FOLDER, NULL, NULL, NULL, {"Video", CDS_VIDEO_TREE_ID, 0, NULL, NULL} };

/**
 * A structure with the Browse action
 * request information.
 */
typedef struct browse_request
{
   char *ObjectID;
   enum {
      BrowseMetadata,
      BrowseDirectChildren
   } BrowseFlag;
   char *Filter;
   int StartingIndex;
   int RequestedCount;
   char *SortCriteria;
} browse_request;


/*
 * State Variables definitions
 */

/* No support of search strings for the time being. */
#define CDS_SEARCH_CAPABILITIES ""

/* No support of sort strings for the time being. */
#define CDS_SORT_CAPABILITIES ""

/* Static system update ID. */
#define CDS_SYSTEM_UPDATE_ID "1"

/*---------------------------------------------------------------------------
 *
 * cds_object operations
 *
 *--------------------------------------------------------------------------*/

/*
 * Delete an object from the tree.
 * Deletion does not free memory associated with
 * cds_object being deleted. It is up to the application
 * to free the memory, as in 
 *    free( cds_del_object(obj) );
 *
 * @param obj A pointer to the cds_object to be deleted.
 * @return A pointer to the cds_object deleted.
 */
static
cds_object *cds_del_object( cds_object *obj )
{
   if( obj != NULL )
   {
      if( obj->previous != NULL )
      {
         obj->previous->next = obj->next;
      }

      if( obj->next != NULL )
      {
         obj->next->previous = obj->previous;
      }

      if( obj->parent->first_child == obj )
      {
         obj->parent->first_child = NULL;
      }
      if( obj->parent->last_child == obj )
      {
         obj->parent->last_child = obj->previous;
      }

      obj->parent->num_children -= 1;
   }

   return obj;
} /* cds_del_object */

/*
 * Count the number of item_type items underneath a certain
 * root node. 
 *
 * @param root The root node to start the search from.
 * @param item_type The item type to search for
 * @param recurse If true, function will count in the 
 *    entire tree underneath the root node.
 * @return The number of children found.
 */
static
int cds_count_children( cds_object *root, ITEM_TYPE item_type, int recurse )
{
   cds_object *child;
   int count = 0;

   child = root->first_child;
   while( child != NULL )
   {
      if( item_type == ITEM_UNDEFINED )
      {
         count++;
      }

      switch(child->type)
      {
         case CDS_OBJ_FOLDER:
            if( recurse ) 
            {
               count += cds_count_children( child, item_type, recurse );
            }
            break;

         case CDS_OBJ_ITEM:
            if( child->item->type & item_type )
            {
               count++;
            }
            break;
      }

      child = child->next;
   }

   return count;
} /* cds_count_items */

/*
 * Helper function to count the direct children of a 
 * item_type kind under a node.
 *
 * @param root The root node to start the search from.
 * @param item_type The item type to search for
 * @return The number of direct children found.
 */
static
int cds_count_direct_children( cds_object *root, ITEM_TYPE item_type )
{
   return cds_count_children( root, item_type, 0 );
} /* cds_count_direct_children */


/*---------------------------------------------------------------------------
 *
 * items & folder operations (add, find, delete, ...)
 *
 *--------------------------------------------------------------------------*/


/*
 * Find the right tree for an item.
 *
 * @param item The item object of the search
 * @return The root tree where the item falls under.
 */
static
cds_object *cds_find_item_tree( item_info *item )
{
   switch( item->type )
   {
      case ITEM_PHOTO:
         return &photo_tree;

      case ITEM_AUDIO:
         return &audio_tree;

      case ITEM_VIDEO:
      case ITEM_AUDIOVIDEO:
         return &video_tree;

      default:
         /* Not a valid item to add! */
         return NULL;
   }
} /* find_item_tree */

/*
 * Search a folder object ID via a depth-first visit of the tree. 
 * To be changed into a breadth-first maybe?
 *
 * @param id A pointer tp The cds_object ID. The cds_object to be found must
 *    be a CDS_OBJ_FOLDER or the search will not succeed.
 * @param The root node to start the search from.
 * @return The cds_object corresponding to that ID.
 */
static
cds_object *cds_find_folder_id( OBJECT_ID *id, cds_object *root )
{
   if( root == NULL )
   {
      root = &root_tree;
   }
   if( strcmp(root->id, *id) == 0 )
   {
      return root;
   }
   else
   {
      cds_object *child;

      child = root->first_child;
      while( child != NULL )
      {
         /* Just search underneath a folder. */
         if( (child->type == CDS_OBJ_FOLDER) && cds_find_folder_id(id, child) )
         {
            return child;
         }

         child = child->next;
      }
   }

   return NULL;
} /* cds_find_folder_id */

/*
 * Add an item to a tree node (parent).
 *
 * @param item The item to be added.
 * @param parent_id The parent node ID to add the item to. 
 *    Must be representing a folder object or the function 
 *    will return NULL.
 *    Reason we are using an OBJECT_ID instead of a cds_object
 *    is because folder objects are duplicated across the 
 *    virtual trees so the ID is the only property which
 *    is unique and shared while the cds_object are different.
 * @return The cds_object containing the added item.
 */
static
cds_object *cds_add_item( item_info *item, OBJECT_ID *parent_id )
{
   cds_object *new_object = NULL;
   cds_object *real_tree = NULL;
   cds_object *real_parent = NULL;

   /* No point in adding a null item! */
   if( item == NULL )
   {
      return NULL;
   }

   /* 
    * Find the right tree for the item. 
    */
   real_tree = cds_find_item_tree( item );
   if( real_tree == NULL )
   {
      return NULL;
   }

   /* 
    * Now that we got hold of the right tree
    * we need to find the real parent for the item.
    * Parents are infact duplicated across the three
    * virtual trees, so we need to look it up 
    * in the right tree - so to add a music item under
    * a folder in the music tree and not the photo tree
    * for instance.
    */
   if( strcmp(*parent_id, root_tree.id) == 0 )
   {
      /* 
       * We were asked to add an item to the root
       * folder so the real parent is the root of the
       * virtual trees in this case.
       */
      real_parent = real_tree;
   }
   else
   {
      /* Find the actual folder this item belongs to. */
      real_parent = cds_find_folder_id( parent_id, real_tree );
      if( real_parent == NULL )
      {
            /* 
             * This should NEVER happen, as the parent object 
             * should have been added automatically. Maybe the 
             * specific folder under this current tree was manually 
             * deleted? 
             */
         return NULL;
      }
   }

   /* 
    * Create the new node. 
    */
   new_object = (cds_object *)calloc( 1, sizeof(cds_object) );
   new_object->parent = real_parent;
   new_object->previous = real_parent->last_child; /* Add it to the end of the parent's children list. */
   new_object->item = item;

   /* Update pointers to first and last child. */
   if( real_parent->first_child == NULL )
   {
      real_parent->first_child = new_object;
      real_parent->last_child = real_parent->first_child;
   }
   else
   {
      real_parent->last_child->next = new_object;
      real_parent->last_child = new_object;
   }

   /* Update the number of direct children for this node. */
   real_parent->num_children += 1;

   return new_object;
} /* cds_add_item */

/*
 * Add a folder to a tree node (parent).
 * The parent node to add the folder to must be a folder object
 * or the function will return an error.
 * The folder will be added to each of the virtual trees.
 *
 * @param path The folder physical path on disk. This is used
 *    to compute the MD5 hash that will make up the folder ID.
 * @param name The logical (display) name for the folder.
 * @param parent_id The parent node ID to add the item to. 
 *    Must be representing a folder object or the function 
 *    will return NULL.
 *    Reason we are using an OBJECT_ID instead of a cds_object
 *    is because folder objects are duplicated across the 
 *    virtual trees so the ID is the only property which
 *    is unique and shared. To find the cds_object associated
 *    to the OBJECT_ID, use cds_find_folder_id().
 * @return The ID for the new added folder.
 */
static
OBJECT_ID *cds_add_folder( char *path, char *name, OBJECT_ID *parent_id )
{
   cds_object *new_folder = NULL;
   cds_object *curr_tree = NULL;
   cds_object *real_parent = NULL;
   ITEM_ID digest;

   /* No point in adding if the path/name are invalid! */
   if( (path == NULL) || (path[0] == 0) || 
       (name == NULL) || (name[0] == 0) )
   {
      return NULL;
   }

   /* 
    * Precompute the MD5 digest for the folder. 
    * Digest is derived from the folder path so we
    * are pretty sure there won't be two identical
    * IDs - we could not say the same if we based 
    * the digest on just the folder name instead.
    */
   md5_message_digest( digest, path );

   /* 
    * Create the new folder in each of the three
    * virtual trees. We still do not know if this folder
    * will contain music, photo or video so we need to
    * make sure the folder exists in each of the three
    * directories - or cds_add_item will fail!
    */
   for( curr_tree = root_tree.first_child; curr_tree != NULL; curr_tree = curr_tree->next )
   {
      if( strcmp(*parent_id, root_tree.id) == 0 )
      {
         real_parent = curr_tree;
      }
      else
      {
         real_parent = cds_find_folder_id( parent_id, curr_tree );
         if( real_parent == NULL )
         {
            /* 
             * This should NEVER happen, as the parent object 
             * should have been added automatically by this
             * function. Maybe the specific folder under this 
             * current tree was manually deleted? 
             */
            return NULL;
         }
      }

      /* 
       * Create the new folder object now and make it a child
       * of the found real parent in the current tree.
       */
      new_folder = (cds_object *)calloc( 1, sizeof(cds_object) );
      new_folder->type = CDS_OBJ_FOLDER;
      new_folder->name = name;
      strcpy( new_folder->id, digest );
      new_folder->parent = real_parent;
      new_folder->previous = real_parent->last_child; /* Add it to the end of the parent's children list. */

      /* Update pointers to first and last child. */
      if( real_parent->first_child == NULL )
      {
         real_parent->first_child = new_folder;
         real_parent->last_child = real_parent->first_child;
      }
      else
      {
         real_parent->last_child->next = new_folder;
         real_parent->last_child = new_folder;
      }

      /* Update number of children for this node. */
      real_parent->num_children += 1;
   }

   return &new_folder->id;
} /* cds_add_folder */


/*
 * Reset a tree by deleting all of the children nodes.
 *
 * @param root The root node to reset.
 */
static
void cds_reset_tree( cds_object *root )
{
   cds_object *obj = root->first_child;
   while( obj != NULL )
   {
      if( obj->type == CDS_OBJ_FOLDER )
      {
         cds_reset_tree( obj );
      }

      free( cds_del_object( obj ) );

      obj = root->first_child;
   }
} /* cds_reset_tree */




static
void cds_print_tree( cds_object *node, int indent )
{
   int i;

   for ( i = 0; i<indent; i++ ) putc( '\t', stdout );

   if( node == NULL ) return;

   if( node->type == CDS_OBJ_ITEM )
   {
      printf( "%s (%s)\n", node->item->filename, node->item->id );
   }
   else
   {
      cds_object *child;
      printf( "%s (%s)\n", node->name, node->id );

      child = node->first_child;
      while( child != NULL )
      {
         cds_print_tree(child, indent+1);
         child = child->next;
      }
   }
}



/*-------------------------------------------------------------------------
 *
 * CDS PUBLIC API
 *
 *------------------------------------------------------------------------*/


/*
 * Initialize the CDS.
 *
 * @return CDS_SUCCESS if successful, another value otherwise.
 */
int cds_init()
{
   /* Initialize the FFMpeg library. */
   av_register_all();
   logger_log( LOG_TRACE, LOG_MSG("FFMpeg initialized") );

   /* 
    * Build the tree structure with the three virtual
    * trees for audio, photo and video.
    */
   root_tree.first_child = &audio_tree;
   root_tree.last_child = &video_tree;
   root_tree.num_children = 3;

   audio_tree.parent = &root_tree;
   audio_tree.next = &photo_tree;
   
   photo_tree.parent = &root_tree;
   photo_tree.previous = &audio_tree;
   photo_tree.next = &video_tree;

   video_tree.parent = &root_tree;
   video_tree.previous = &photo_tree;

   return CDS_SUCCESS;
} /* cds_init */

/*
 * Re-initialize the CDS.
 *
 * @return CDS_SUCCESS if successful, another value otherwise.
 */
int cds_reinit()
{
   cds_reset_tree( &audio_tree );
   cds_reset_tree( &video_tree );
   cds_reset_tree( &photo_tree );

   return CDS_SUCCESS;
} /* cds_reinit */

/**
 * Returns the SCPD description of the CDS as per
 * the UPnP specifications.
 * @return A pointer to the XML description string
 *    for the CDS service. The returned pointer points
 *    to a static memory area so does not need to be
 *    freed.
 */
char *cds_get_scpd()
{
   static char *cds_scpd = 
      "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
      "<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">"
      "  <specVersion>"
      "    <major>1</major>"
      "    <minor>0</minor>"
      "  </specVersion>"
      "  <actionList>"
      "    <action>"
      "      <name>Browse</name>"
      "      <argumentList>"
      "        <argument>"
      "          <name>ObjectID</name>"
      "          <direction>in</direction>"
      "          <relatedStateVariable>A_ARG_TYPE_ObjectID</relatedStateVariable>"
      "        </argument>"
      "        <argument>"
      "          <name>BrowseFlag</name>"
      "          <direction>in</direction>"
      "          <relatedStateVariable>A_ARG_TYPE_BrowseFlag</relatedStateVariable>"
      "       </argument>"
      "        <argument>"
      "          <name>Filter</name>"
      "          <direction>in</direction>"
      "          <relatedStateVariable>A_ARG_TYPE_Filter</relatedStateVariable>"
      "        </argument>"
      "        <argument>"
      "          <name>StartingIndex</name>"
      "          <direction>in</direction>"
      "          <relatedStateVariable>A_ARG_TYPE_Index</relatedStateVariable>"
      "        </argument>"
      "        <argument>"
      "          <name>RequestedCount</name>"
      "          <direction>in</direction>"
      "          <relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>"
      "        </argument>"
      "        <argument>"
      "          <name>SortCriteria</name>"
      "          <direction>in</direction>"
      "          <relatedStateVariable>A_ARG_TYPE_SortCriteria</relatedStateVariable>" \
      "       </argument>"
      "        <argument>"
      "          <name>Result</name>"
      "          <direction>out</direction>"
      "          <relatedStateVariable>A_ARG_TYPE_Result</relatedStateVariable>"
      "        </argument>"
      "        <argument>"
      "          <name>NumberReturned</name>"
      "          <direction>out</direction>"
      "          <relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>"
      "        </argument>"
      "        <argument>"
      "          <name>TotalMatches</name>"
      "          <direction>out</direction>"
      "          <relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>"
      "        </argument>"
      "        <argument>"
      "          <name>UpdateID</name>"
      "          <direction>out</direction>"
      "          <relatedStateVariable>A_ARG_TYPE_UpdateID</relatedStateVariable>"
      "        </argument>"
      "      </argumentList>"
      "    </action>"
      "    <action>"
      "      <name>GetSystemUpdateID</name>"
      "      <argumentList>"
      "        <argument>"
      "          <name>Id</name>"
      "          <direction>out</direction>"
      "          <relatedStateVariable>SystemUpdateID</relatedStateVariable>"
      "        </argument>"
      "      </argumentList>"
      "    </action>"
      "    <action>"
      "      <name>GetSearchCapabilities</name>"
      "      <argumentList>"
      "        <argument>"
      "          <name>SearchCaps</name>"
      "          <direction>out</direction>"
      "          <relatedStateVariable>SearchCapabilities</relatedStateVariable>"
      "        </argument>"
      "      </argumentList>"
      "    </action>"
      "    <action>"
      "      <name>GetSortCapabilities</name>"
      "      <argumentList>"
      "        <argument>"
      "          <name>SortCaps</name>"
      "          <direction>out</direction>"
      "          <relatedStateVariable>SortCapabilities</relatedStateVariable>"
      "        </argument>"
      "      </argumentList>"
      "    </action>"
      "  </actionList>"
      "  <serviceStateTable>"
      "    <stateVariable sendEvents=\"no\">"
      "      <name>A_ARG_TYPE_BrowseFlag</name>"
      "      <dataType>string</dataType>"
      "      <allowedValueList>"
      "        <allowedValue>BrowseMetadata</allowedValue>"
      "        <allowedValue>BrowseDirectChildren</allowedValue>"
      "      </allowedValueList>"
      "    </stateVariable>"
      "    <stateVariable sendEvents=\"yes\">"
      "      <name>SystemUpdateID</name>"
      "      <dataType>ui4</dataType>"
      "    </stateVariable>"
      "    <stateVariable sendEvents=\"no\">"
      "      <name>A_ARG_TYPE_Count</name>"
      "      <dataType>ui4</dataType>"
      "    </stateVariable>"
      "    <stateVariable sendEvents=\"no\">"
      "      <name>A_ARG_TYPE_SortCriteria</name>"
      "      <dataType>string</dataType>"
      "    </stateVariable>"
      "    <stateVariable sendEvents=\"no\">"
      "      <name>SortCapabilities</name>"
      "      <dataType>string</dataType>"
      "    </stateVariable>"
      "    <stateVariable sendEvents=\"no\">"
      "      <name>A_ARG_TYPE_Index</name>"
      "      <dataType>ui4</dataType>"
      "    </stateVariable>"
      "    <stateVariable sendEvents=\"no\">"
      "      <name>A_ARG_TYPE_ObjectID</name>"
      "      <dataType>string</dataType>"
      "    </stateVariable>"
      "    <stateVariable sendEvents=\"no\">"
      "      <name>A_ARG_TYPE_UpdateID</name>"
      "      <dataType>ui4</dataType>"
      "    </stateVariable>"
      "    <stateVariable sendEvents=\"no\">"
      "      <name>A_ARG_TYPE_Result</name>"
      "      <dataType>string</dataType>"
      "    </stateVariable>"
      "   <stateVariable sendEvents=\"no\">"
      "      <name>SearchCapabilities</name>"
      "      <dataType>string</dataType>"
      "    </stateVariable>"
      "    <stateVariable sendEvents=\"no\">"
      "      <name>A_ARG_TYPE_Filter</name>"
      "      <dataType>string</dataType>"
      "    </stateVariable>"
      "  </serviceStateTable>"
      "</scpd>";

   return cds_scpd;
} /* cds_get_scpd */


/*-------------------------------------------------------------------------
 *
 * CDS PUBLIC API (SOAP ACTIONS)
 *
 *------------------------------------------------------------------------*/


/*
         Examples of BROWSE requests.

The first one is from the Samsung TV MediaRenderer. It is asking to browse 
the direct children of the A_F object id - which is the Audio container
root.

POST /upnp/control/ContentDirectory1 HTTP/1.0
HOST: 192.168.1.102:52235
CONTENT-LENGTH: 417
CONTENT-TYPE: text/xml;charset="utf-8"
USER-AGENT: DLNADOC/1.50
SOAPACTION: "urn:schemas-upnp-org:service:ContentDirectory:1#Browse"

<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
   <s:Body>
      <u:Browse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
         <ObjectID>A_F</ObjectID>
         <BrowseFlag>BrowseDirectChildren</BrowseFlag>
         <Filter>*</Filter>
         <StartingIndex>0</StartingIndex>
         <RequestedCount>0</RequestedCount>
         <SortCriteria></SortCriteria>
      </u:Browse>
   </s:Body>
</s:Envelope>

HTTP/1.1 200 OK
CONTENT-LENGTH: 909
CONTENT-TYPE: text/xml; charset="utf-8"
DATE: Sun, 17 May 2009 07:09:12 GMT
EXT:
SERVER: MS-Windows/XP UPnP/1.0 PROTOTYPE/1.0

<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
   <s:Body>
      <u:BrowseResponse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
         <Result>
            &lt;DIDL-Lite xmlns=&quot;urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/&quot; xmlns:dc=&quot;http://purl.org/dc/elements/1.1/&quot; xmlns:upnp=&apos;urn:schemas-upnp-org:metadata-1-0/upnp/&apos; xmlns:dlna=&quot;urn:schemas-dlna-org:metadata-1-0/&quot; xmlns:sec=&quot;http://www.sec.co.kr/&quot;&gt;
               &lt;container id=&quot;A_F_0000&quot; parentID=&quot;A_F&quot; childCount=&quot;218&quot; restricted=&quot;1&quot;&gt;
                  &lt;dc:title&gt;Canzoni Bimbi&lt;/dc:title&gt;
                  &lt;upnp:class&gt;object.container&lt;/upnp:class&gt;
               &lt;/container&gt;
            &lt;/DIDL-Lite&gt;
         </Result>
         <NumberReturned>1</NumberReturned>
         <TotalMatches>1</TotalMatches>
         <UpdateID>0</UpdateID>
      </u:BrowseResponse>
   </s:Body>
</s:Envelope>

======================================================================================

This one is a BrowseMetadata action for the object id A_F_0000_217.

POST /upnp/control/ContentDirectory1 HTTP/1.0
HOST: 192.168.1.102:52235
CONTENT-LENGTH: 420
CONTENT-TYPE: text/xml;charset="utf-8"
USER-AGENT: DLNADOC/1.50
SOAPACTION: "urn:schemas-upnp-org:service:ContentDirectory:1#Browse"

<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
   <s:Body>
      <u:Browse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
         <ObjectID>A_F_0000_217</ObjectID>
         <BrowseFlag>BrowseMetadata</BrowseFlag>
         <Filter>*</Filter>
         <StartingIndex>0</StartingIndex>
         <RequestedCount>1</RequestedCount>
         <SortCriteria></SortCriteria>
      </u:Browse>
   </s:Body>
</s:Envelope>

HTTP/1.1 200 OK
CONTENT-LENGTH: 1465
CONTENT-TYPE: text/xml; charset="utf-8"
DATE: Sun, 17 May 2009 07:09:12 GMT
EXT:
SERVER: MS-Windows/XP UPnP/1.0 PROTOTYPE/1.0

<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
   <s:Body>
      <u:BrowseResponse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
      <Result>
         &lt;DIDL-Lite xmlns=&quot;urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/&quot; xmlns:dc=&quot;http://purl.org/dc/elements/1.1/&quot; xmlns:upnp=&apos;urn:schemas-upnp-org:metadata-1-0/upnp/&apos; xmlns:dlna=&quot;urn:schemas-dlna-org:metadata-1-0/&quot; xmlns:sec=&quot;http://www.sec.co.kr/&quot;&gt;
            &lt;item id=&quot;A_F_0000_217&quot; parentID=&quot;A_F_0000&quot; restricted=&quot;1&quot;&gt;
               &lt;dc:title&gt;Zecchino d&amp;apos;oro - Un poco di zucchero&lt;/dc:title&gt;
               &lt;upnp:class&gt;object.item.audioItem&lt;/upnp:class&gt;
               &lt;upnp:album&gt;Nessun album&lt;/upnp:album&gt;
               &lt;sec:dcmInfo&gt;MOODSCORE=30322,MOODID=1,CREATIONDATE=1239615310,YEAR=2009&lt;/sec:dcmInfo&gt;
               &lt;dc:date&gt;2009-04-13&lt;/dc:date&gt;
               &lt;dc:creator&gt;Nessun cantante&lt;/dc:creator&gt;
               &lt;upnp:genre&gt;Other&lt;/upnp:genre&gt;
               &lt;res protocolInfo=&quot;http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000&quot; size=&quot;3673383&quot; duration=&quot;0:03:03&quot;&gt;http://192.168.1.102:53235/A_F_0000_217.MP3&lt;/res&gt;
            &lt;/item&gt;
         &lt;/DIDL-Lite&gt;
      </Result>
      <NumberReturned>1</NumberReturned>
      <TotalMatches>1</TotalMatches>
      <UpdateID>0</UpdateID>
      </u:BrowseResponse>
   </s:Body>
</s:Envelope>
*/

/*
 * Parse the browse request XML into a browse request
 * structure.
 * Returns either CDS_SUCCESS or the UPnP error codes
 * for the Browse Action.
 */
static
int cds_parse_browse_request( char *soap_action_body, browse_request *browse_req )
{
   xmlNode *body_node = NULL,
           *browse_node = NULL,
           *node = NULL;

   body_node = xml_get_soap_body( soap_action_body );
   if( body_node == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("Error while parsing XML message") );
      return CDS_402_ERROR;
   }

   /* Get the Browse node. */
   browse_node = xml_first_node_by_name( body_node, "Browse" );
   if( browse_node == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("Error while parsing XML message") );
      return CDS_402_ERROR;
   }

   /* Get the ObjectID value. */
   node = xml_first_node_by_name( browse_node, "ObjectID" );
   if( node == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("Error while parsing XML message") );
      return CDS_402_ERROR;
   }
   browse_req->ObjectID = xmlNodeGetContent( node );

   /* Get the BrowseFlag value. */
   node = xml_first_node_by_name( browse_node, "BrowseFlag" );
   if( node == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("Error while parsing XML message") );
      return CDS_402_ERROR;
   }
   browse_req->BrowseFlag = (strcmp(xmlNodeGetContent(node), "BrowseMetadata") == 0) ? BrowseMetadata : BrowseDirectChildren;

   /* Get the Filter value. */
   node = xml_first_node_by_name( browse_node, "Filter" );
   if( node == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("Error while parsing XML message") );
      return CDS_402_ERROR;
   }
   browse_req->Filter = xmlNodeGetContent( node );

   /* Get the StartingIndex value. */
   node = xml_first_node_by_name( browse_node, "StartingIndex" );
   if( node == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("Error while parsing XML message") );
      return CDS_402_ERROR;
   }
   browse_req->StartingIndex = atoi( xmlNodeGetContent( node ) );

   /* Get the RequestedCount value. */
   node = xml_first_node_by_name( browse_node, "RequestedCount" );
   if( node == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("Error while parsing XML message") );
      return CDS_402_ERROR;
   }
   browse_req->RequestedCount = atoi( xmlNodeGetContent( node ) );

   /* Get the SortCriteria value. */
   node = xml_first_node_by_name( browse_node, "SortCriteria" );
   if( node == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("Error while parsing XML message") );
      return CDS_402_ERROR;
   }
   browse_req->SortCriteria = xmlNodeGetContent( node );

   return CDS_SUCCESS;
} /* cds_parse_browse_request */

/*
 * Browse action - BrowseMetadata flag processing.
 *
 * @param browse_req The browse request structure.
 * @BrowseResult The browse result, including the envelope, in XML 
 *    format.
 * @return CDS_SUCCESS is successful, or the UPnP defined error codes
 *    for the Browse action: 
 *    402 (Invalid Args)
 *    501 (Action Failed)
 *    701 (No Such Object)
 *    709 (Unsupported or invalid sort criteria)
 *    720 (Cannot process the request)
 */
static
int cds_browse_metadata( browse_request *browse_req, char **BrowseResult )
{
   /* Canned response for the time being. */
   static char *canned_response = 
      "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
         "<s:Body>"
            "<u:BrowseResponse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">"
            "<Result>"
               "&lt;DIDL-Lite xmlns=&quot;urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/&quot; xmlns:dc=&quot;http://purl.org/dc/elements/1.1/&quot; xmlns:upnp=&apos;urn:schemas-upnp-org:metadata-1-0/upnp/&apos; xmlns:dlna=&quot;urn:schemas-dlna-org:metadata-1-0/&quot; xmlns:sec=&quot;http://www.sec.co.kr/&quot;&gt;"
                  "&lt;item id=&quot;A_F_0000_217&quot; parentID=&quot;A_F_0000&quot; restricted=&quot;1&quot;&gt;"
                     "&lt;dc:title&gt;Zecchino d&amp;apos;oro - Un poco di zucchero&lt;/dc:title&gt;"
                     "&lt;upnp:class&gt;object.item.audioItem&lt;/upnp:class&gt;"
                     "&lt;upnp:album&gt;Nessun album&lt;/upnp:album&gt;"
                     "&lt;sec:dcmInfo&gt;MOODSCORE=30322,MOODID=1,CREATIONDATE=1239615310,YEAR=2009&lt;/sec:dcmInfo&gt;"
                     "&lt;dc:date&gt;2009-04-13&lt;/dc:date&gt;"
                     "&lt;dc:creator&gt;Nessun cantante&lt;/dc:creator&gt;"
                     "&lt;upnp:genre&gt;Other&lt;/upnp:genre&gt;"
                     "&lt;res protocolInfo=&quot;http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000&quot; size=&quot;3673383&quot; duration=&quot;0:03:03&quot;&gt;http://192.168.1.102:53235/A_F_0000_217.MP3&lt;/res&gt;"
                  "&lt;/item&gt;"
               "&lt;/DIDL-Lite&gt;"
            "</Result>"
            "<NumberReturned>1</NumberReturned>"
            "<TotalMatches>1</TotalMatches>"
            "<UpdateID>0</UpdateID>"
            "</u:BrowseResponse>"
         "</s:Body>"
      "</s:Envelope>";

   *BrowseResult = strdup( canned_response );
   return CDS_SUCCESS;
} /* cds_browse_metadata */

/*
 * Browse action - BrowseDirectChildren flag processing.
 *
 * @param browse_req The browse request structure.
 * @BrowseResult The browse result, including the envelope, in XML 
 *    format.
 * @return CDS_SUCCESS is successful, or the UPnP defined error codes
 *    for the Browse action: 
 *    402 (Invalid Args)
 *    501 (Action Failed)
 *    701 (No Such Object)
 *    709 (Unsupported or invalid sort criteria)
 *    720 (Cannot process the request)
 */
static
int cds_browse_direct_children( browse_request *browse_req, char **BrowseResult )
{
   /* Canned response for the time being. */
   static char *canned_response = 
      "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
         "<s:Body>"
            "<u:BrowseResponse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">"
               "<Result>"
                  "&lt;DIDL-Lite xmlns=&quot;urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/&quot; xmlns:dc=&quot;http://purl.org/dc/elements/1.1/&quot; xmlns:upnp=&apos;urn:schemas-upnp-org:metadata-1-0/upnp/&apos; xmlns:dlna=&quot;urn:schemas-dlna-org:metadata-1-0/&quot; xmlns:sec=&quot;http://www.sec.co.kr/&quot;&gt;"
                     "&lt;container id=&quot;A_F_0000&quot; parentID=&quot;A_F&quot; childCount=&quot;218&quot; restricted=&quot;1&quot;&gt;"
                        "&lt;dc:title&gt;Canzoni Bimbi&lt;/dc:title&gt;"
                        "&lt;upnp:class&gt;object.container&lt;/upnp:class&gt;"
                     "&lt;/container&gt;"
                  "&lt;/DIDL-Lite&gt;"
               "</Result>"
               "<NumberReturned>1</NumberReturned>"
               "<TotalMatches>1</TotalMatches>"
               "<UpdateID>0</UpdateID>"
            "</u:BrowseResponse>"
         "</s:Body>"
      "</s:Envelope>";

   *BrowseResult = strdup( canned_response );
   return CDS_SUCCESS;
} /* cds_browse_direct_children */

/**
 * Browse Action.
 *
 * @param soap_action The Browse request body soap action, including the envelope,
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
int cds_Browse( char *soap_action_body, char **BrowseResponse )
{
   browse_request browse_req;
   int res;

   if( (res = cds_parse_browse_request( soap_action_body, &browse_req )) == CDS_SUCCESS )
   {
      switch( browse_req.BrowseFlag )
      {
         case BrowseMetadata:
            res = cds_browse_metadata( &browse_req, BrowseResponse );
            break;

         case BrowseDirectChildren:
            res = cds_browse_direct_children( &browse_req, BrowseResponse );
            break;

         default:
            res = CDS_720_ERROR;
            break;
      }
      return res;
   }
   else
   {
      /* Build the error message in the browse response. */
      return res;
   }
} /* cds_Browse */

/**
 * GetSearchCapabilities Action.
 *
 * @param GetSearchCapsResponse The GetSearchCapabilitiesResponse
 *    result body in XML format. This is a newly allocated buffer
 *    so memory must be freed after calling this function.
 * @return CDS_SUCCESS is successful, or the UPnP defined error codes
 *    for the Browse action: 
 *    402 (Invalid Args)
 *    501 (Action Failed)
 */
int cds_GetSearchCapabilities( char **GetSearchCapabilitiesResponse )
{
   static char *search_caps_response =  
       "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
       "  <s:Body>"
       "    <u:GetSearchCapabilitiesResponse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">"
       "      <SearchCaps>"CDS_SEARCH_CAPABILITIES"</SearchCaps>"
       "    </u:GetSearchCapabilitiesResponse>"
       "  </s:Body>"
       "</s:Envelope>";

   *GetSearchCapabilitiesResponse = strdup( search_caps_response );

   return CDS_SUCCESS;
} /* cds_GetSearchCapabilities */

/**
 * GetSortCapabilities Action.
 *
 * @param GetSortCapabilitiesResponse The GetSortCapabilitiesResponse
 *    result body in XML format. This is a newly allocated buffer
 *    so memory must be freed after calling this function.
 * @return CDS_SUCCESS is successful, or the UPnP defined error codes
 *    for the Browse action: 
 *    402 (Invalid Args)
 *    501 (Action Failed)
 */
int cds_GetSortCapabilities( char **GetSearchCapabilitiesResponse )
{
   static char *sort_caps_response = 
       "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
       "  <s:Body>"
       "    <u:GetSortCapabilitiesResponse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">"
       "      <SortCaps>"CDS_SORT_CAPABILITIES"</SortCaps>"
       "    </u:GetSortCapabilitiesResponse>"
       "  </s:Body>"
       "</s:Envelope>";

   *GetSearchCapabilitiesResponse = strdup( sort_caps_response );

   return CDS_SUCCESS;
} /* cds_GetSortCapabilities */

/**
 * GetSystemUpdateID Action.
 *
 * @param GetSystemUpdateIDResponse The GetSystemUpdateIDResponse
 *    result body in XML format. This is a newly allocated buffer
 *    so memory must be freed after calling this function.
 * @return CDS_SUCCESS is successful, or the UPnP defined error codes
 *    for the Browse action: 
 *    402 (Invalid Args)
 *    501 (Action Failed)
 */
int cds_GetSystemUpdateID( char **GetSystemUpdateIDResponse )
{
   static char *sys_update_id_response = 
       "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
       "  <s:Body>"
       "    <u:GetSystemUpdateIDResponse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">"
       "      <Id>"CDS_SYSTEM_UPDATE_ID"</Id>"
       "    </u:GetSystemUpdateIDResponse>"
       "  </s:Body>"
       "</s:Envelope>";            

   *GetSystemUpdateIDResponse = strdup( sys_update_id_response );

   return CDS_SUCCESS;
}

/*
POST /upnp/control/ContentDirectory1 HTTP/1.0
HOST: 192.168.1.102:52235
CONTENT-LENGTH: 316
CONTENT-TYPE: text/xml;charset="utf-8"
USER-AGENT: DLNADOC/1.50
SOAPACTION: "urn:schemas-upnp-org:service:ContentDirectory:1#X_GetObjectIDfromIndex"

<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
   <s:Body>
      <u:X_GetObjectIDfromIndex xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
         <CategoryType>22</CategoryType>
         <Index>216</Index>
      </u:X_GetObjectIDfromIndex>
   </s:Body>
</s:Envelope>

HTTP/1.1 200 OK
CONTENT-LENGTH: 316
CONTENT-TYPE: text/xml; charset="utf-8"
DATE: Sun, 17 May 2009 07:09:12 GMT
EXT:
SERVER: MS-Windows/XP UPnP/1.0 PROTOTYPE/1.0

<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
   <s:Body>
      <u:X_GetObjectIDfromIndexResponse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
         <ObjectID>A_F_0000_217</ObjectID>
      </u:X_GetObjectIDfromIndexResponse>
   </s:Body>
</s:Envelope>
*/

/**
 * X_GetObjectIDfromIndex Action. This I suppose is used by Samsung
 * MediaRenderer to map child number X to the internal ID used by
 * the Samsung MediaServer (or the Samsung HTTP Streaming Server) and
 * when browsing the contents of a folder on the TV. There is a
 * X_GetObjectIDfromIndex request for each item visible on the 
 * screen in a certain moment in time.
 * I quite do not get why this action is needed, really...
 *
 * @param request_body The request body, including the envelope,
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
int cds_X_GetObjectIDfromIndex( char *soap_action, char **X_GetObjectIDfromIndexResponse )
{
   xmlNode *body_node, *node;
   int cat_type;
   int index;

   body_node = xml_get_soap_body( soap_action );
   if( body_node == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("Error while parsing XML message") );
      return CDS_402_ERROR;
   }

   /* Get the X_GetObjectIDfromIndex node. */
   node = xml_first_node_by_name( body_node, "X_GetObjectIDfromIndex" );
   if( node == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("Error while parsing XML message") );
      return CDS_402_ERROR;
   }

   /* Get the CategoryType and Index nodes. */
   node = xml_first_node_by_name( node, "CategoryType" );
   if( node == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("Error while parsing XML message") );
      return CDS_402_ERROR;
   }
   cat_type = atoi( xmlNodeGetContent(node) );
   node = xml_next_sibling_by_name( node, "Index" );
   if( node == NULL )
   {
      logger_log( LOG_ERROR, LOG_MSG("Error while parsing XML message") );
      return CDS_402_ERROR;
   }
   index = atoi( xmlNodeGetContent(node) );

   /*
    * TBD: Figure out how to associate the category type/index to the object ID. 
    */

   return CDS_SUCCESS;
} /* cds_X_GetObjectIDfromIndex */

/**
 * CDS Action dispatcher.
 *
 * @param soap_action The soap action string (the one in the HTTP header)
 * @param soap_action_body The soap action body, including the envelope, in XML format.
 * @action_response The action response, including the envelope, in XML format.
 * @return CDS_SUCCESS if successful, or another value otherwise, depending on the
 *    UPnP error derived during the action processing.
 */
int cds_dispatch_action( char *soap_action, char *soap_action_body, char **action_response )
{
   if( strstr(soap_action, "#Browse") )
   {
      return cds_Browse( soap_action_body, action_response );
   }
   
   if( strstr(soap_action, "#GetSortCapabilities") )
   {
      return cds_GetSortCapabilities( action_response );
   }

   if( strstr(soap_action, "#GetSearchCapabilities") )
   {
      return cds_GetSearchCapabilities( action_response );
   }

   if( strstr(soap_action, "#GetSystemUpdateID") )
   {
      return cds_GetSystemUpdateID( action_response );
   }

   if( strstr(soap_action, "#X_GetObjectIDfromIndex") )
   {
      return cds_X_GetObjectIDfromIndex( soap_action_body, action_response );
   }

   /* "Cannot process the request" error */
   return CDS_720_ERROR;
} /* cds_action */


#include "yada.h"
void cds_test()
{
   item_info *ii1, *ii2, *ii3;
   OBJECT_ID *obj_id;
   char *soap_res;

   cds_init();

   if( item_getinfo( "D:\\MPEG-1.mpg", &ii1 ) == DLNA_SUCCESS )
   {
   }
   
   if( item_getinfo( "D:\\test.jpg", &ii2 ) == DLNA_SUCCESS )
   {
   }

   if( item_getinfo( "D:\\test.mp3", &ii3 ) == DLNA_SUCCESS )
   {
   }

   /* ---------------------------------------------------------------------- */
   /*  Build Content Directory
   /* ---------------------------------------------------------------------- */

   cds_add_item( ii1, &root_tree.id );
   obj_id = cds_add_folder( "D:\\", "Pearl Jam", &root_tree.id );

   cds_add_item( ii2, obj_id );
   cds_add_item( ii3, obj_id );
   obj_id = cds_add_folder( "D:\\", "Pearl Jam2", obj_id );
   cds_add_item( ii2, obj_id );
   cds_add_item( ii2, obj_id );
   cds_add_item( ii2, obj_id );
   cds_add_item( ii2, obj_id );
   cds_add_item( ii3, &root_tree.id );
   obj_id = cds_add_folder( "D:\\", "Pearl Jam3", obj_id );
   cds_add_item( ii3, obj_id );
   cds_add_item( ii3, obj_id );
   cds_add_item( ii3, obj_id );

   printf( "\n\n------------------ FS TREE ----------------------- \n\n" );
   cds_print_tree( &root_tree, 0 );
   printf( "\nAudio count = %d\nPhoto count = %d\nVideo count = %d\nTotal count = %d", 
      cds_count_children( &root_tree, ITEM_AUDIO, 1 ), 
      cds_count_children( &root_tree, ITEM_PHOTO, 1 ), 
      cds_count_children( &root_tree, ITEM_AUDIOVIDEO, 1 ),
      cds_count_children( &root_tree, ITEM_UNDEFINED, 0 ) );
   printf( "\n\n------------------ FS TREE ----------------------- \n\n" );

   /* ---------------------------------------------------------------------- */
   /*  TEST SOAP ACTIONS
   /* ---------------------------------------------------------------------- */

   printf( "\n\n------------------ Browse - BrowseMetadata ----------------------- \n\n" );

#define TEST_BROWSE_METADATA \
   "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\n"\
      "<s:Body>\n"\
         "<u:Browse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">\n"\
            "<ObjectID>A_F_0000_217</ObjectID>\n"\
            "<BrowseFlag>BrowseMetadata</BrowseFlag>\n"\
            "<Filter>*</Filter>\n"\
            "<StartingIndex>0</StartingIndex>\n"\
            "<RequestedCount>1</RequestedCount>\n"\
            "<SortCriteria></SortCriteria>\n"\
         "</u:Browse>\n"\
      "</s:Body>\n"\
   "</s:Envelope>"

   printf( "Request:\n\n%s\n\n", TEST_BROWSE_METADATA );
   cds_Browse( TEST_BROWSE_METADATA, &soap_res );
   printf( "Response:\n\n%s\n\n", soap_res );
   free( soap_res );

   printf( "\n\n------------------ Browse - BrowseDirectChildren ----------------------- \n\n" );

#define TEST_BROWSE_DIRECT_CHILDREN \
   "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"\
      "<s:Body>"\
         "<u:Browse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">"\
            "<ObjectID>A_F</ObjectID>"\
            "<BrowseFlag>BrowseDirectChildren</BrowseFlag>"\
            "<Filter>*</Filter>"\
            "<StartingIndex>0</StartingIndex>"\
            "<RequestedCount>0</RequestedCount>"\
            "<SortCriteria></SortCriteria>"\
         "</u:Browse>"\
      "</s:Body>"\
   "</s:Envelope>"\

   printf( "Request:\n\n%s\n\n", TEST_BROWSE_DIRECT_CHILDREN );
   cds_Browse( TEST_BROWSE_DIRECT_CHILDREN, &soap_res );
   printf( "Response:\n\n%s\n\n", soap_res );
   free( soap_res );

   printf( "\n\n------------------ X_GetObjectIDfromIndex ----------------------- \n\n" );

#define TEST_X_GET_OBJ_FROM_IDX  \
   "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\n"\
      "<s:Body>\n"\
         "<u:X_GetObjectIDfromIndex xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">\n"\
            "<CategoryType>22</CategoryType>\n"\
            "<Index>216</Index>\n"\
         "</u:X_GetObjectIDfromIndex>\n"\
      "</s:Body>\n"\
   "</s:Envelope>"

   printf( "Request:\n\n%s\n\n", TEST_X_GET_OBJ_FROM_IDX );
   cds_X_GetObjectIDfromIndex( TEST_X_GET_OBJ_FROM_IDX, &soap_res );
   /*
   printf( "Response:\n\n%s\n\n", soap_res );
   free( soap_res );
   */

   printf( "\n\n------------------ GetSearchCapabilities ----------------------- \n\n" );

   printf( "Request:\n\n(null)\n\n" );
   cds_GetSearchCapabilities( &soap_res );
   printf( "Response:\n\n%s\n\n", soap_res );
   free( soap_res );

   printf( "\n\n------------------ GetSortCapabilities ----------------------- \n\n" );

   printf( "Request:\n\n(null)\n\n" );
   cds_GetSortCapabilities( &soap_res );
   printf( "Response:\n\n%s\n\n", soap_res );
   free( soap_res );

   printf( "\n\n------------------ GetSystemUpdateID ----------------------- \n\n" );

   printf( "Request:\n\n(null)\n\n" );
   cds_GetSystemUpdateID( &soap_res );
   printf( "Response:\n\n%s\n\n", soap_res );
   free( soap_res );

   item_freeinfo( ii1 );
   item_freeinfo( ii2 );
   item_freeinfo( ii3 );

   printf( "\n\n" );
} /* cds_test */