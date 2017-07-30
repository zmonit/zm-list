/*  =========================================================================
    zm_cache - Internal cache

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.                           
                                                                                   
    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain   
    one at http://mozilla.org/MPL/2.0/.                                            
    =========================================================================
*/

/*
@header
    zm_cache - Internal cache
@discuss
@end
*/

#include "zm_list_classes.h"

//  Structure of our class

struct _zm_cache_t {
    //  class properties
    bool verbose;
    zhashx_t *cache;
};


//  --------------------------------------------------------------------------
//  Create a new zm_cache

zm_cache_t *
zm_cache_new (void)
{
    zm_cache_t *self = (zm_cache_t *) zmalloc (sizeof (zm_cache_t));
    assert (self);
    //  Initialize class properties here
    self->cache = zhashx_new ();
    zhashx_set_destructor (self->cache, (void (*)(void**))zm_proto_destroy);
    zhashx_set_duplicator (self->cache, (void* (*)(const void*))zm_proto_dup);
    zhashx_set_key_destructor (self->cache, (void (*)(void**))zstr_free);
    zhashx_set_key_duplicator (self->cache, (void* (*)(const void*))strdup);
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zm_cache

void
zm_cache_destroy (zm_cache_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zm_cache_t *self = *self_p;
        //  Free class properties here
        zhashx_destroy (&self->cache);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Destroy the zm_cache

void
zm_cache_put (zm_cache_t *self, char *key, zm_proto_t *msg) {
    zhashx_update (self->cache, key, msg);
}

static bool
zm_proto_expired (zm_proto_t *self, int64_t now) {
    assert (self);

    return ((zm_proto_time (self) + zm_proto_ttl (self)) <= now);
}

//  --------------------------------------------------------------------------
//  Garbage collect the cache

void
zm_cache_gc (zm_cache_t *self, char *key) {
    int64_t now = zclock_time ();
    zm_proto_t *msg = (zm_proto_t*) zhashx_first (self->cache);
    while (msg) {
        char *key = (char*) zhashx_cursor (self->cache);
        if (zm_proto_expired (msg, now))
            zhashx_delete (self->cache, key);
        msg = (zm_proto_t*) zhashx_next (self->cache);
    }
}

zm_proto_t *
zm_cache_get (zm_cache_t *self, char *key) {
    int64_t now = zclock_time ();
    zm_proto_t *ret = (zm_proto_t*) zhashx_lookup (self->cache, key);
    if (!ret)
        return NULL;
    if (zm_proto_expired (ret, now)) {
        zhashx_delete (self->cache, key);
        return NULL;
    }
    return ret;
}

static bool
s_is_type (zm_proto_t *msg, int type) {
    if (type == 0)
        return true;
    else
        return (zm_proto_id (msg) == type);
}

//  --------------------------------------------------------------------------
//  Return first entry or NULL, id filters id of returned proto. id == 0 means
//  no filter

zm_proto_t *
zm_cache_first (zm_cache_t *self, int id) {
    assert (self);

    zm_proto_t *ret = (zm_proto_t*) zhashx_first (self->cache);
    while (ret && !s_is_type (ret, id))
        ret = (zm_proto_t*) zhashx_next (self->cache);
    return ret;
}

//  --------------------------------------------------------------------------
//  Return next entry or NULL, id filters id of returned proto. id == 0 means
//  no filter

zm_proto_t *
zm_cache_next (zm_cache_t *self, int id) {
    assert (self);

    zm_proto_t *ret = NULL;
    do {
        ret = (zm_proto_t*) zhashx_next (self->cache);
    }
    while (ret && !s_is_type (ret, id));

    return ret;
}

//  --------------------------------------------------------------------------
//  Return key of actual entry, see zm_cache_first/zm_cache_next

char *zm_cache_key (zm_cache_t *self) {
    return (char*) zhashx_cursor (self->cache);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
zm_cache_test (bool verbose)
{
    printf (" * zm_cache: ");

    //  @selftest
    //  Simple create/destroy test

    zm_cache_t *self = zm_cache_new ();
    assert (self);

    zm_proto_t *msg = zm_proto_new ();
    zm_proto_encode_metric (msg, "DEV", zclock_time (), 10000, NULL, "current.input", "11", "A");

    //FIXME: the most common query is give me all metrics for selected device
    //       current design does not support that
    zm_cache_put (self, "current.input@DEV", msg);
    zm_proto_encode_metric (msg, "EXPIRED-DEV", zclock_time ()-100, 50, NULL, "current.input", "11", "A"); // already expired message
    zm_cache_put (self, "current.input@EXPIRED-DEV", msg);

    assert (zm_cache_get (self, "current.input@DEV"));
    assert (!zm_cache_get (self, "current.input@NODEV"));
    assert (!zm_cache_get (self, "current.input@EXPIRED-DEV"));
    
    zm_proto_encode_alert (msg, "DEV", zclock_time (), 10000, NULL, "alert_rule11", 1, "some alert"); // already expired message
    zm_cache_put (self, "alert_rule11@DEV", msg);

    assert (!zm_cache_first (self, ZM_PROTO_DEVICE));
    assert (!zm_cache_next (self, ZM_PROTO_DEVICE));

    assert (zm_cache_first (self, ZM_PROTO_METRIC));
    assert (!zm_cache_next (self, ZM_PROTO_METRIC));

    assert (zm_cache_first (self, ZM_PROTO_ALERT));
    assert (!zm_cache_next (self, ZM_PROTO_ALERT));
    
    zm_proto_destroy (&msg);
    zm_cache_destroy (&self);
    //  @end
    printf ("OK\n");
}
