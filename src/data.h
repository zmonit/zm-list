/*  =========================================================================
    data - state api

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.                           
                                                                                   
    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain   
    one at http://mozilla.org/MPL/2.0/.                                            
    =========================================================================
*/

#ifndef DATA_H_INCLUDED
#define DATA_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new data
ZM_DEVICE_PRIVATE data_t *
    data_new (void);

//  Destroy the data
ZM_DEVICE_PRIVATE void
    data_destroy (data_t **self_p);

//  Self test of this class
ZM_DEVICE_PRIVATE void
    data_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
