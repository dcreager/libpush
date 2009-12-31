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

void
push_callback_init(push_callback_t *callback,
                   size_t min_bytes_requested,
                   size_t max_bytes_requested,
                   push_process_bytes_func_t *process_bytes,
                   push_callback_free_func_t *free)
{
    callback->min_bytes_requested = min_bytes_requested;
    callback->max_bytes_requested = max_bytes_requested;
    callback->process_bytes = process_bytes;
    callback->free = free;
    callback->freeing = false;
}


push_callback_t *
push_callback_new(push_process_bytes_func_t *process_bytes,
                  size_t min_bytes_requested,
                  size_t max_bytes_requested)
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

    push_callback_init(result,
                       min_bytes_requested,
                       max_bytes_requested,
                       process_bytes,
                       NULL);
    return result;
}


void
push_callback_free(push_callback_t *callback)
{
    /*
     * If we've already started freeing this callback, then we've
     * encountered a cycle of callback references.  We don't need to
     * free the callback, since some other function higher up on the
     * call stack is taking care of it.
     */

    if (callback->freeing)
        return;

    /*
     * If we haven't started freeing the callback yet, set the freeing
     * flag so that a possible later call will now that we're taking
     * care of it.
     */

    callback->freeing = true;

    /*
     * If the callback has a custom free function, call it first.
     * Then, free the callback instance.
     */

    if (callback->free != NULL)
        callback->free(callback);

    free(callback);
}
