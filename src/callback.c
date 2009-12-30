/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <push.h>

push_callback_t *
push_callback_new(push_process_bytes_func_t *process_bytes,
                  size_t bytes_requested)
{
    push_callback_t  *result;

    /*
     * First try to allocate the new callback instance.  If we can't,
     * return NULL.
     */

    result = (push_callback_t *) malloc(sizeof(push_callback_t));
    if (result == NULL)
    {
        return NULL;
    }

    /*
     * If it works, initialize and return the new instance.
     */

    result->bytes_requested = bytes_requested;
    result->process_bytes = process_bytes;
    result->free = NULL;
    return result;
}


void
push_callback_free(push_callback_t *callback)
{
    /*
     * If the callback has a custom free function, call it first.
     * Then, free the callback instance.
     */

    if (callback->free != NULL)
        callback->free(callback);

    free(callback);
}
