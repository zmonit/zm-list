/*  =========================================================================
    zm_cache - Internal cache

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.                           
                                                                                   
    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain   
    one at http://mozilla.org/MPL/2.0/.                                            
    =========================================================================
*/

#ifndef ZM_CACHE_H_INCLUDED
#define ZM_CACHE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new zm_cache
ZM_LIST_PRIVATE zm_cache_t *
    zm_cache_new (void);

//  Destroy the zm_cache
ZM_LIST_PRIVATE void
    zm_cache_destroy (zm_cache_t **self_p);

//  Self test of this class
ZM_LIST_PRIVATE void
    zm_cache_test (bool verbose);

//  --------------------------------------------------------------------------
//  Put new or update existing  item into cache, which now owns the value

ZM_LIST_PRIVATE void
    zm_cache_put (zm_cache_t *self, char *key, zm_proto_t *msg);

//  --------------------------------------------------------------------------
//  Garbage collect the cache

ZM_LIST_PRIVATE void
    zm_cache_gc (zm_cache_t *self, char *key);

//  --------------------------------------------------------------------------
//  Get an item from cache.

ZM_LIST_PRIVATE zm_proto_t *
    zm_cache_get (zm_cache_t *self, char *key);

//  --------------------------------------------------------------------------
//  Return first entry or NULL, id filters id of returned proto. id == 0 means
//  no filter

ZM_LIST_PRIVATE zm_proto_t *
    zm_cache_first (zm_cache_t *self, int id);

//  --------------------------------------------------------------------------
//  Return next entry or NULL, id filters id of returned proto. id == 0 means
//  no filter

ZM_LIST_PRIVATE zm_proto_t *
    zm_cache_next (zm_cache_t *self, int id);

//  --------------------------------------------------------------------------
//  Return key of actual entry, see zm_cache_first/zm_cache_next

ZM_LIST_PRIVATE char *
    zm_cache_key (zm_cache_t *self);

//  @end

#ifdef __cplusplus
}
#endif

#endif
