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
#include <push/bind.h>


static push_error_code_t
bind_activate(push_parser_t *parser,
              push_callback_t *pcallback,
              void *input)
{
    push_bind_t  *callback = (push_bind_t *) pcallback;


    /*
     * We activate the bind by activating the first callback.
     */

    callback->first_active = true;
    PUSH_DEBUG_MSG("bind: Activating first callback.\n");
    return push_callback_activate(parser, callback->first, input);
}


static ssize_t
bind_process_bytes(push_parser_t *parser,
                   push_callback_t *pcallback,
                   const void *vbuf,
                   size_t bytes_available)
{
    push_bind_t  *callback = (push_bind_t *) pcallback;
    ssize_t  result;

    /*
     * If the first callback is active, pass the data in to it.
     */

    if (callback->first_active)
    {
        push_error_code_t  activate_result;

        PUSH_DEBUG_MSG("bind: Passing %zu bytes to first callback.\n",
                       bytes_available);

        result = push_callback_process_bytes(parser, callback->first,
                                             vbuf, bytes_available);

        /*
         * If we get an error code or an incomplete, return that
         * status code right away.
         */

        if (result < 0)
        {
            PUSH_DEBUG_MSG("bind: First callback didn't succeed.\n");
            return result;
        }

        /*
         * Otherwise, the first callback succeeded.  Switch over to
         * the second callback.
         */

        PUSH_DEBUG_MSG("bind: First callback succeeded.\n");
        callback->first_active = false;

        /*
         * Activate the second callback, using the result of the
         * first.
         */

        PUSH_DEBUG_MSG("bind: Activating second callback.\n");
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

        if ((bytes_available > 0) && (result == 0))
        {
            PUSH_DEBUG_MSG("bind: First callback processed "
                           "all %zu bytes.\n",
                           bytes_available);

            return PUSH_INCOMPLETE;
        } else {
            /*
             * Before falling through to the second callback, we need
             * to update the buffer pointer.
             */

            ptrdiff_t  bytes_processed = bytes_available - result;

            PUSH_DEBUG_MSG("bind: First callback left %zd bytes.\n",
                           result);

            vbuf += bytes_processed;
            bytes_available = result;
        }
    }

    /*
     * Either we're starting off this call with the second callback
     * active, or the first callback parsed some of the data, and
     * there's some left for the second callback to call.
     */

    PUSH_DEBUG_MSG("bind: Passing %zu bytes to second callback.\n",
                   bytes_available);

    result = push_callback_process_bytes(parser, callback->second,
                                         vbuf, bytes_available);

    /*
     * If the second callback succeeds, then we should use its result
     * as our result.
     */

    if (result >= 0)
    {
        PUSH_DEBUG_MSG("bind: Second callback succeeded.\n");
        callback->base.result = callback->second->result;
    } else {
        PUSH_DEBUG_MSG("bind: Second callback didn't succeed.\n");
    }

    return result;
}


push_bind_t *
push_bind_new(push_callback_t *first,
              push_callback_t *second)
{
    push_bind_t  *result = (push_bind_t *) malloc(sizeof(push_bind_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       bind_activate,
                       bind_process_bytes,
                       NULL);

    result->first = first;
    result->second = second;
    result->first_active = true;

    return result;
}
