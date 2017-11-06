/*  =========================================================================
    zm_list - zm list actor

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.                           
                                                                                   
    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain   
    one at http://mozilla.org/MPL/2.0/.                                            
    =========================================================================
*/

/*
@header
    zm_list - zm list actor
@discuss
@end
*/

#include "zm_list_classes.h"

//  Structure of our actor

struct _zm_list_t {
    zsock_t *pipe;              //  Actor command pipe
    zpoller_t *poller;          //  Socket poller
    bool terminated;            //  Did caller ask us to quit?
    bool verbose;               //  Verbose logging enabled?
    //  Declare properties
    mlm_client_t *client;       //  Malamute client
    zm_cache_t *cache;          //  Internal cache for all METRICS
    zm_proto_t *msg;            //  zmproto message
};


//  --------------------------------------------------------------------------
//  Create a new zm_list instance

static zm_list_t *
zm_list_new (zsock_t *pipe, void *args)
{
    zm_list_t *self = (zm_list_t *) zmalloc (sizeof (zm_list_t));
    assert (self);

    self->pipe = pipe;
    self->terminated = false;
    self->poller = zpoller_new (self->pipe, NULL);

    //  Initialize properties
    self->client = mlm_client_new ();
    zpoller_add (self->poller, mlm_client_msgpipe (self->client));
    self->cache = zm_cache_new ();

    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zm_list instance

static void
zm_list_destroy (zm_list_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zm_list_t *self = *self_p;

        //  Free actor properties
        mlm_client_destroy (&self->client);

        //  Free object itself
        zpoller_destroy (&self->poller);
        free (self);
        *self_p = NULL;
    }
}


//  Start this actor. Return a value greater or equal to zero if initialization
//  was successful. Otherwise -1.

static int
zm_list_start (zm_list_t *self)
{
    assert (self);

    //  TODO: Add startup actions

    return 0;
}


//  Stop this actor. Return a value greater or equal to zero if stopping 
//  was successful. Otherwise -1.

static int
zm_list_stop (zm_list_t *self)
{
    assert (self);

    //  TODO: Add shutdown actions

    return 0;
}


//  Here we handle incoming message from the node

static void
zm_list_recv_api (zm_list_t *self)
{
    //  Get the whole message of the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    if (!request)
       return;        //  Interrupted

    char *command = zmsg_popstr (request);
    if (streq (command, "START"))
        zm_list_start (self);
    else
    if (streq (command, "STOP"))
        zm_list_stop (self);
    else
    if (streq (command, "VERBOSE"))
        self->verbose = true;
    else
    if (streq (command, "$TERM"))
        //  The $TERM command is send by zactor_destroy() method
        self->terminated = true;
    else {
        zsys_error ("invalid command '%s'", command);
        assert (false);
    }
    zstr_free (&command);
    zmsg_destroy (&request);
}

void
zm_list_recv_mlm (zm_list_t *self)
{
    assert (self);

    zm_proto_recv_mlm (self->msg, self->client);

    if (streq (mlm_client_command (self->client), "MAILBOX DELIVER")) {
        char *key = zsys_sprintf ("%s@%s",
            zm_proto_type (self->msg),
            zm_proto_device (self->msg));
        
        zm_cache_gc (self->cache, key);
        zm_proto_t *get = zm_cache_get (self->cache, key);
        zstr_free (&key);

        if (get)
            zm_proto_sendto (
                    get,
                    self->client,
                    mlm_client_sender (self->client),
                    mlm_client_subject (self->client));
        else {
            zm_proto_set_id (self->msg, ZM_PROTO_ERROR);
            zm_proto_sendto (
                    self->msg,
                    self->client,
                    mlm_client_sender (self->client),
                    mlm_client_subject (self->client));
        }
    }
    else
    if (streq (mlm_client_command (self->client), "STREAM DELIVER")) {
        zm_cache_put (self->cache, (char*) mlm_client_subject (self->client), self->msg);
    }
}

//  --------------------------------------------------------------------------
//  This is the actor which runs in its own thread.

void
zm_list_actor (zsock_t *pipe, void *args)
{
    zm_list_t * self = zm_list_new (pipe, args);
    if (!self)
        return;          //  Interrupted

    //  Signal actor successfully initiated
    zsock_signal (self->pipe, 0);

    while (!self->terminated) {
        zsock_t *which = (zsock_t *) zpoller_wait (self->poller, 0);
        if (which == self->pipe)
            zm_list_recv_api (self);
        else
        if (which == mlm_client_msgpipe (self->client))
            zm_list_recv_mlm (self);
    }
    zm_list_destroy (&self);
}

//  --------------------------------------------------------------------------
//  Self test of this actor.

void
zm_list_test (bool verbose)
{
    printf (" * zm_list: ");
    //  @selftest
    //  Simple create/destroy test

    zactor_t *zm_list = zactor_new (zm_list_actor, NULL);

    zactor_destroy (&zm_list);
    //  @end

    printf ("OK\n");
}
