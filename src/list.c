/*  =========================================================================
    list - zm list actor

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.                           
                                                                                   
    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain   
    one at http://mozilla.org/MPL/2.0/.                                            
    =========================================================================
*/

/*
@header
    list - zm list actor
@discuss
@end
*/

#include "zm_device_classes.h"

//  Structure of our actor

struct _list_t {
    zsock_t *pipe;              //  Actor command pipe
    bool terminated;            //  Did caller ask us to quit?
    bool verbose;               //  Verbose logging enabled?
    bool active;                //  Did caller ask us to start/stop?
    char *endpoint;             //  Malamute endpoint
    char *name;                 //  Malamute client's name
    uint64_t poll_interval;     //  Socket poller polling interval (msec)
    mlm_client_t *client;       //  Malamute client
    zpoller_t *poller;          //  Socket poller
    zlistx_t *consumers;        //  
    char *producer;             //
};


//  --------------------------------------------------------------------------
//  Create a new list instance

static list_t *
list_new (zsock_t *pipe, void *args)
{
    list_t *self = (list_t *) zmalloc (sizeof (list_t));
    assert (self);

    self->pipe = pipe;
    self->terminated = false;
    self->verbose = false;
    self->active = false;
    self->endpoint = NULL;
    self->name = NULL;
    self->poll_interval = (uint64_t) 1000;

    self->client = mlm_client_new ();
    if (!self->client) {
        zsys_error ("mlm_client_new () failed.");
        free (self);
        self = NULL;
        return NULL;
    }

    self->poller = zpoller_new (self->pipe, NULL);
    if (!self->poller) {
        zsys_error ("zpoller_new () failed");
        mlm_client_destroy (&self->client);
        free (self);
        self = NULL;
        return NULL;
    }

    zlistx_t *consumers = zlistx_new ();
    if (!consumers) {
        zsys_error ("zlistx_new () failed");
        zpoller_destroy (&self->poller);
        mlm_client_destroy (&self->client);
        free (self);
        self = NULL;
        return NULL;
    }
    zlistx_set_destructor (self->consumers, (zlistx_destructor_fn *) zstr_free);
    zlistx_set_duplicator (self->consumers, (zlistx_duplicator_fn *) strdup);

    self->producer = NULL;

    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the list instance

static void
list_destroy (list_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        list_t *self = *self_p;

        //  Free actor properties
        zstr_free (&self->endpoint);
        zstr_free (&self->name);
        zpoller_destroy (&self->poller);
        mlm_client_destroy (&self->client);
        zstr_free (&self->endpoint);
        zstr_free (&self->name);
        zlistx_destroy (&self->consumers);
        zstr_free (&self->producer);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  Start this actor. Return a value greater or equal to zero if initialization
//  was successful. Otherwise -1.

static int
list_command_start (list_t *self)
{
    assert (self);
    if (self->active)
        return 0;
    if (!self->endpoint || !self->name) {
        if (self->verbose)
            zsys_debug ("Malamute connection information not set. Use CONNECT command before START command.");
        return -1;
    }

    int rv = mlm_client_connect (self->client, self->endpoint, 5000, self->name);
    if (rv == -1) {
        zsys_error ("mlm_client_connect (endpoint = '%s', timeout = '5000', address = '%s') failed.",
                self->endpoint, self->name);
        return -1;
    }
    if (self->producer) {
        int rv = mlm_client_set_producer (self->client, self->producer);
        if (rv == -1) {
            zsys_error ("mlm_client_set_producer ('%s') failed.", self->producer);
            return -1;
        }
    }
    char *item = (char *) zlistx_first (self->consumers);
    while (item) {
        int rv = mlm_client_set_consumer (self->client, item, ".*");
        if (rv == -1) {
            zsys_error ("mlm_client_set_consumer ('%s', '.*') failed.", item);
            return -1;
        }
        item = (char *) zlistx_next (self->consumers);
    }

    self->active = true;

    return 0;
}


//  Stop this actor. Return a value greater or equal to zero if stopping 
//  was successful. Otherwise -1.

static int
list_command_stop (list_t *self)
{
    assert (self);
    self->active = false;

    return 0;
}


//  Note to self: It might be a good idea to restrict the setter commands
//                like CONNECTION/CONSUMER... to only after actor is 
//                stopped.

//  Set malamute connection information.
//  Return 0 on success, -1 on error

static int
list_command_connection (list_t *self, zmsg_t *message)
{
    assert (self);
    assert (message);

    char *endpoint = zmsg_popstr (message);
    char *name = zmsg_popstr (message);
    if (!endpoint || !name) {
        zstr_free (&self->endpoint);
        zstr_free (&self->name);
        return -1;
    }
    zstr_free (&self->endpoint);
    self->endpoint = strdup (endpoint);
    zstr_free (&self->name);
    self->name = strdup (name);

    zstr_free (&endpoint);
    zstr_free (&name);

    return 0;
}


//  Add 'stream' to list of consumed streams.
//  Return 0 on success, -1 on error

static int
list_command_consumer (list_t *self, zmsg_t *message)
{
    assert (self);
    assert (message);

    char *stream = zmsg_popstr (message);
    if (!stream)
        return -1;
    zlistx_add_end (self->consumers, (void *) stream);
    zstr_free (&stream);
    return 0;
}

//  Set publish stream.
//  Return 0 on success, -1 on error

static int
list_command_producer (list_t *self, zmsg_t *message)
{
    assert (self);
    assert (message);

    char *stream = zmsg_popstr (message);
    if (!stream)
        return -1;
    zstr_free (&self->producer);
    self->producer = strdup (stream);
    zstr_free (&stream);
    return 0;
}

//  Here we handle incoming message from the node

static void
list_recv_api (list_t *self)
{
    //  Get the whole message of the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    if (!request)
       return;        //  Interrupted

    char *command = zmsg_popstr (request);
    if (streq (command, "START"))
    {
        if (self->verbose)
            zsys_debug ("START command received.");
        int rv = list_command_start (self);
        if (rv == -1)
            zsys_error ("START command failed.");
    } 
    else
    if (streq (command, "STOP"))
    {
        if (self->verbose)
            zsys_debug ("STOP command received.");
        int rv = list_command_stop (self);
        if (rv == -1)
            zsys_error ("STOP command failed.");
    }
    else
    if (streq (command, "VERBOSE"))
    {
        zsys_debug ("VERBOSE command received.");
        self->verbose = true;
    }
    else
    if (streq (command, "$TERM"))
    {
        //  The $TERM command is send by zactor_destroy() method
        if (self->verbose)
            zsys_debug ("$TERM command received.");
        self->terminated = true;
    }
    else
    if (streq (command, "CONNECTION"))
    {
        if (self->verbose)
            zsys_debug ("CONNECTION command received.");
        int rv = list_command_connection (self, request);
        if (rv == -1)
            zsys_error ("CONNECTION command failed.");
    }
    else
    if (streq (command, "CONSUMER"))
    {
        if (self->verbose)
            zsys_debug ("CONSUMER command received.");
        int rv = list_command_consumer (self, request);
        if (rv == -1)
            zsys_error ("CONSUMER command failed.");
    }
    else
    if (streq (command, "PRODUCER"))
    {
        if (self->verbose)
            zsys_debug ("PRODUCER command received.");
        int rv = list_command_producer (self, request);
        if (rv == -1)
            zsys_error ("PRODUCER command failed.");
    }
    else {
        zsys_error ("invalid command '%s'", command);
        assert (false);
    }
    zstr_free (&command);
    zmsg_destroy (&request);
}


//  --------------------------------------------------------------------------
//  This is the actor which runs in its own thread.

void
list_actor (zsock_t *pipe, void *args)
{
    list_t * self = list_new (pipe, args);
    if (!self)
        return;          //  Interrupted

    //  Signal actor successfully initiated
    zsock_signal (self->pipe, 0);

    while (!self->terminated) {
        zsock_t *which = (zsock_t *) zpoller_wait (self->poller, 0);
        if (which == self->pipe)
            list_recv_api (self);
       //  Add other sockets when you need them.
    }
    list_destroy (&self);
}

//  --------------------------------------------------------------------------
//  Self test of this actor.

void
list_test (bool verbose)
{
    printf (" * list: ");
    //  @selftest

    //  Simple create/destroy test
    zactor_t *list = zactor_new (list_actor, NULL);
    assert (list);
    zactor_destroy (&list);
    assert (list == NULL);
    zactor_destroy (&list);

    //  

    //  @end

    printf ("OK\n");
}
