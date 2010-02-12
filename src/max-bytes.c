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
#include <string.h>

#include <push/basics.h>
#include <push/combinators.h>
#include <push/pairs.h>


/**
 * The push_callback_t subclass that defines a max-bytes callback.
 */

typedef struct _max_bytes
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * The wrapped callback.
     */

    push_callback_t  *wrapped;

    /**
     * The maximum number of bytes to pass in to the wrapped callback.
     */

    size_t  maximum_bytes;

    /**
     * The number of bytes that still need to be passed into the
     * wrapped callback.
     */

    size_t  bytes_remaining;

} max_bytes_t;


static push_error_code_t
max_bytes_activate(push_parser_t *parser,
                   push_callback_t *pcallback,
                   void *input)
{
    max_bytes_t  *callback = (max_bytes_t *) pcallback;

    PUSH_DEBUG_MSG("max-bytes: Activating.  Capping at %zu bytes.\n",
                   callback->maximum_bytes);
    callback->bytes_remaining = callback->maximum_bytes;

    PUSH_DEBUG_MSG("max-bytes: Activating wrapped callback.\n");
    return push_callback_activate(parser, callback->wrapped, input);
}


static push_error_code_t
dynamic_max_bytes_activate(push_parser_t *parser,
                           push_callback_t *pcallback,
                           void *input)
{
    max_bytes_t  *callback = (max_bytes_t *) pcallback;

    /*
     * Extract the threshold from the input pair.
     */

    push_pair_t  *pair = (push_pair_t *) input;
    size_t  *maximum_bytes = (size_t *) pair->first;

    callback->maximum_bytes = *maximum_bytes;

    PUSH_DEBUG_MSG("max-bytes: [Dynamic] Activating.  "
                   "Capping at %zu bytes.\n",
                   callback->maximum_bytes);
    callback->bytes_remaining = callback->maximum_bytes;

    PUSH_DEBUG_MSG("max-bytes: Activating wrapped callback.\n");
    return push_callback_activate(parser, callback->wrapped,
                                  pair->second);
}


static ssize_t
max_bytes_process_bytes(push_parser_t *parser,
                        push_callback_t *pcallback,
                        const void *vbuf,
                        size_t bytes_available)
{
    max_bytes_t  *callback = (max_bytes_t *) pcallback;
    ssize_t  result;

    PUSH_DEBUG_MSG("max-bytes: Processing %zu bytes.\n",
                   bytes_available);

    /*
     * If we can still send more bytes into the callback, and there
     * are some available in the buffer, then try to send them in.
     */

    if ((callback->bytes_remaining > 0) && (bytes_available > 0))
    {
        size_t  bytes_to_send;
        ssize_t  result;

        /*
         * Don't send in too many bytes...
         */

        bytes_to_send =
            (callback->bytes_remaining < bytes_available)?
            callback->bytes_remaining:
            bytes_available;

        /*
         * We use a tail call so that the wrapped callback's result is
         * our result on success.
         */

        PUSH_DEBUG_MSG("max-bytes: Calling wrapped callback with "
                       "%zu bytes.\n",
                       bytes_to_send);

        result = push_callback_tail_process_bytes
            (parser, &callback->base, callback->wrapped,
             vbuf, bytes_to_send);

        /*
         * If the wrapped callback throws an error (not including
         * incomplete), return the error.
         */

        if ((result < 0) && (result != PUSH_INCOMPLETE))
        {
            PUSH_DEBUG_MSG("max-bytes: Wrapped callback failed.\n");
            return result;
        }

        /*
         * If the wrapped callback succeeds, we go ahead and succeed,
         * too.
         */

        if (result >= 0)
        {
            size_t  bytes_processed;

            bytes_processed = bytes_to_send - result;
            vbuf += bytes_processed;
            bytes_available -= bytes_processed;
            callback->bytes_remaining -= bytes_processed;

            PUSH_DEBUG_MSG("max-bytes: Wrapped callback succeeded "
                           "using %zu bytes.\n",
                           (callback->maximum_bytes -
                            callback->bytes_remaining));

            /*
             * Don't return result directly, since there might be data
             * in the buffer that we didn't send into the wrapped
             * callback.  We need to include this extra data in our
             * result.
             */

            return bytes_available;
        }

        PUSH_DEBUG_MSG("max-bytes: Wrapped callback incomplete "
                       "after using %zu bytes.\n",
                       bytes_to_send);

        vbuf += bytes_to_send;
        bytes_available -= bytes_to_send;
        callback->bytes_remaining -= bytes_to_send;

        if (callback->bytes_remaining > 0)
        {
            /*
             * If the wrapped callback indicated that it's not done
             * yet, and we haven't reached the threshold yet, return
             * incomplete ourselves.
             */

            return PUSH_INCOMPLETE;
        }
    }

    /*
     * If we fall through to here, then we're in one of three
     * situations: (1) we received some data but had already sent in
     * the maximum in a previous process_bytes call; (2) we received
     * some data and just sent in the maximum now; or (3) we reached
     * EOF.  In all three cases, we send an EOF to the wrapped
     * callback to give it a final chance to throw a parse error.
     */

    PUSH_DEBUG_MSG("max-bytes: Sending EOF to wrapped callback.\n");

    result = push_callback_tail_process_bytes
        (parser, &callback->base, callback->wrapped,
         NULL, 0);

    /*
     * If we get a success code, we return bytes_available, since the
     * EOF we're sending to the wrapped callback might not be our EOF.
     * (If it is, we'll correctly return a 0; if not, then the EOF
     * success will let the next callback in the chain continue
     * processing.)
     */

    if (result >= 0)
    {
        PUSH_DEBUG_MSG("max-bytes: Wrapped callback succeeds at EOF.  "
                       "%zu bytes remaining.\n",
                       bytes_available);
        return bytes_available;
    }

    return result;
}


static void
max_bytes_free(push_callback_t *pcallback)
{
    max_bytes_t  *callback = (max_bytes_t *) pcallback;

    PUSH_DEBUG_MSG("max-bytes: Freeing wrapped callback.\n");
    push_callback_free(callback->wrapped);
}


push_callback_t *
push_max_bytes_new(push_callback_t *wrapped,
                   size_t maximum_bytes)
{
    max_bytes_t  *callback =
        (max_bytes_t *) malloc(sizeof(max_bytes_t));

    if (callback == NULL)
        return NULL;

    push_callback_init(&callback->base,
                       max_bytes_activate,
                       max_bytes_process_bytes,
                       max_bytes_free);

    callback->wrapped = wrapped;
    callback->maximum_bytes = maximum_bytes;
    callback->bytes_remaining = maximum_bytes;

    return &callback->base;
}


push_callback_t *
push_dynamic_max_bytes_new(push_callback_t *wrapped)
{
    max_bytes_t  *callback =
        (max_bytes_t *) malloc(sizeof(max_bytes_t));

    if (callback == NULL)
        return NULL;

    push_callback_init(&callback->base,
                       dynamic_max_bytes_activate,
                       max_bytes_process_bytes,
                       max_bytes_free);

    callback->wrapped = wrapped;
    callback->maximum_bytes = 0;
    callback->bytes_remaining = 0;

    return &callback->base;
}