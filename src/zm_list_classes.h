/*  =========================================================================
    zm_list_classes - private header file

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.                           
                                                                                   
    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain   
    one at http://mozilla.org/MPL/2.0/.                                            
################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
    =========================================================================
*/

#ifndef ZM_LIST_CLASSES_H_INCLUDED
#define ZM_LIST_CLASSES_H_INCLUDED

//  Platform definitions, must come first
#include "platform.h"

//  External API
#include "../include/zmlist.h"

//  Extra headers

//  Opaque class structures to allow forward references
#ifndef ZM_CACHE_T_DEFINED
typedef struct _zm_cache_t zm_cache_t;
#define ZM_CACHE_T_DEFINED
#endif

//  Internal API

#include "zm_cache.h"

//  *** To avoid double-definitions, only define if building without draft ***
#ifndef ZM_LIST_BUILD_DRAFT_API

//  *** Draft method, defined for internal use only ***
//  Self test of this class.
ZM_LIST_PRIVATE void
    zm_cache_test (bool verbose);

//  Self test for private classes
ZM_LIST_PRIVATE void
    zm_list_private_selftest (bool verbose);

#endif // ZM_LIST_BUILD_DRAFT_API

#endif
