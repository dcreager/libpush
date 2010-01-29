/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stddef.h>
#include <stdlib.h>

#include <push/basics.h>
#include <push/combinators.h>


/**
 * The push_callback_t subclass that defines a compose callback.
 */

typedef struct _compose
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * The first child callback in the compose.
     */

    push_callback_t  *first;

    /**
     * The second child callback in the compose.
     */

    push_callback_t  *second;

    /**
     * Whether the first or second child callback is active.
     */

    bool first_active;

} compose_t;


static push_error_code_t
compose_activate(push_parser_t *parser,
                 push_callback_t *pcallback,
                 void *input)
{
    compose_t  *callback = (compose_t *) pcallback;

    /*
     * We activate the compose by activating the first callback.
     */

    callback->first_active = true;
    PUSH_DEBUG_MSG("compose: Activating first callback.\n");
    return push_callback_activate(parser, callback->first, input);
}


static ssize_t
compose_process_bytes(push_parser_t *parser,
                      push_callback_t *pcallback,
                      const void *vbuf,
                      size_t bytes_available)
{
    compose_t  *callback = (compose_t *) pcallback;

    /*
     * If the first callback is active, pass the data in to it.
     */

    if (callback->first_active)
    {
        ssize_t  process_result;
        push_error_code_t  activate_result;

        PUSH_DEBUG_MSG("compose: Passing %zu bytes to first callback.\n",
                       bytes_available);

        process_result =
            push_callback_process_bytes(parser, callback->first,
                                        vbuf, bytes_available);

        /*
         * If we get an error code or an incomplete, return that
         * status code right away.
         */

        if (process_result < 0)
        {
            PUSH_DEBUG_MSG("compose: First callback didn't succeed.\n");
            return process_result;
        }

        /*
         * Otherwise, the first callback succeeded.  Switch over to
         * the second callback.
         */

        PUSH_DEBUG_MSG("compose: First callback succeeded.\n");
        callback->first_active = false;

        /*
         * Activate the second callback, using the result of the
         * first.
         */

        PUSH_DEBUG_MSG("compose: Activating second callback.\n");
        activate_result = push_callback_activate(parser, callback->second,
                                                 callback->first->result);

        if (activate_result != PUSH_SUCCESS)
            return activate_result;

        /*
         * If we have data left in the buffer, or if there wasn't any
         * data to begin with (indicating EOF), then we want to fall
         * through and let the second callback process it.  Otherwise
         * (if we had data to begin with, and the first callback
         * processed it all), then we return an incomplete code.
         */

        if ((bytes_available > 0) && (process_result == 0))
        {
            PUSH_DEBUG_MSG("compose: First callback processed "
                           "all %zu bytes.\n",
                           bytes_available);

            return PUSH_INCOMPLETE;
        } else {
            /*
             * Before falling through to the second callback, we need
             * to update the buffer pointer.
             */

            ptrdiff_t  bytes_processed = bytes_available - process_result;

            PUSH_DEBUG_MSG("compose: First callback left %zd bytes.\n",
                           process_result);

            vbuf += bytes_processed;
            bytes_available = process_result;
        }
    }

    /*
     * Either we're starting off this call with the second callback
     * active, or the first callback parsed some of the data, and
     * there's some left for the second callback to call.
     */

    PUSH_DEBUG_MSG("compose: Passing %zu bytes to second callback.\n",
                   bytes_available);

    return push_callback_tail_process_bytes(parser, &callback->base,
                                            callback->second,
                                            vbuf, bytes_available);
}


static void
compose_free(push_callback_t *pcallback)
{
    compose_t  *callback = (compose_t *) pcallback;

    PUSH_DEBUG_MSG("compose: Freeing first callback.\n");
    push_callback_free(callback->first);

    PUSH_DEBUG_MSG("compose: Freeing second callback.\n");
    push_callback_free(callback->second);
}


push_callback_t *
push_compose_new(push_callback_t *first,
                 push_callback_t *second)
{
    compose_t  *result =
        (compose_t *) malloc(sizeof(compose_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       compose_activate,
                       compose_process_bytes,
                       compose_free);

    result->first = first;
    result->second = second;
    result->first_active = true;

    return &result->base;
}
