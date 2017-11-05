/*  =========================================================================
    zm_list - zm list actor

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.                           
                                                                                   
    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain   
    one at http://mozilla.org/MPL/2.0/.                                            
    =========================================================================
*/

#ifndef ZM_LIST_H_INCLUDED
#define ZM_LIST_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


//  @interface
//  Create new zm_list actor instance.
//  @TODO: Describe the purpose of this actor!
//
//      zactor_t *zm_list = zactor_new (zm_list, NULL);
//
//  Destroy zm_list instance.
//
//      zactor_destroy (&zm_list);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (zm_list, "VERBOSE");
//
//  Start zm_list actor.
//
//      zstr_sendx (zm_list, "START", NULL);
//
//  Stop zm_list actor.
//
//      zstr_sendx (zm_list, "STOP", NULL);
//
//  This is the zm_list constructor as a zactor_fn;
ZM_LIST_EXPORT void
    zm_list_actor (zsock_t *pipe, void *args);

//  Self test of this actor
ZM_LIST_EXPORT void
    zm_list_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
