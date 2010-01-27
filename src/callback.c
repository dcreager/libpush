/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>

#include <push/basics.h>

#ifndef PUSH_FREE
#define PUSH_FREE 0
#endif

#if PUSH_FREE
#define PUSH_FREE_MSG(...) PUSH_DEBUG_MSG(__VA_ARGS__)
#else
#define PUSH_FREE_MSG(...) /* skipping debug message */
#endif

void
push_callback_init(push_callback_t *callback,
                   push_activate_func_t *activate,
                   push_process_bytes_func_t *process_bytes,
                   push_callback_free_func_t *free)
{
    callback->activate = activate;
    callback->process_bytes = process_bytes;
    callback->free = free;
    callback->freeing = false;
}


push_callback_t *
push_callback_new(push_activate_func_t *activate,
                  push_process_bytes_func_t *process_bytes)
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
                       activate,
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
    {
        PUSH_FREE_MSG("callback: %p: Already started freeing.\n",
                      callback);
        return;
    }

    /*
     * If we haven't started freeing the callback yet, set the freeing
     * flag so that a possible later call will now that we're taking
     * care of it.
     */

    PUSH_FREE_MSG("callback: %p: Starting to free.\n",
                  callback);

    callback->freeing = true;

    /*
     * If the callback has a custom free function, call it first.
     */

    if (callback->free != NULL)
    {
        PUSH_FREE_MSG("callback: %p: Calling custom free function.\n",
                      callback);
        callback->free(callback);
    }

    /*
     * Finally, free the callback object itself.
     */

    PUSH_FREE_MSG("callback: %p: Freeing push_callback_t instance.\n",
                  callback);
    free(callback);

    PUSH_FREE_MSG("callback: %p: Finished freeing.\n",
                  callback);
}


push_error_code_t
push_callback_activate(push_parser_t *parser,
                       push_callback_t *callback,
                       void *input)
{
    /*
     * Call the callback's activation function, if any.  Return the
     * activation's error code as our own error code.
     */

    if (callback->activate != NULL)
    {
        return callback->activate(parser, callback, input);
    }

    /*
     * If there's no activation function, then there's no error.
     */

    return PUSH_SUCCESS;
}


ssize_t
push_callback_process_bytes(push_parser_t *parser,
                            push_callback_t *callback,
                            const void *buf,
                            size_t bytes_available)
{
    return callback->process_bytes(parser, callback,
                                   buf, bytes_available);
}
