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
#include <push/min-bytes.h>


/**
 * The push_callback_t subclass that defines a min-bytes callback.
 */

typedef struct _min_bytes
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
     * The minimum number of bytes to pass in to the wrapped callback.
     */

    size_t  minimum_bytes;

    /**
     * A buffer for storing data until we reach the minimum.
     */

    void  *buffer;

    /**
     * The number of bytes currently stored in the buffer.
     */

    size_t  bytes_buffered;

} min_bytes_t;


static push_error_code_t
min_bytes_activate(push_parser_t *parser,
                   push_callback_t *pcallback,
                   void *input)
{
    min_bytes_t  *callback = (min_bytes_t *) pcallback;

    PUSH_DEBUG_MSG("min-bytes: Activating.\n");

    PUSH_DEBUG_MSG("min-bytes: Clearing buffer.\n");
    callback->bytes_buffered = 0;

    PUSH_DEBUG_MSG("min-bytes: Activating wrapped callback.\n");
    return push_callback_activate(parser, callback->wrapped, input);
}


static ssize_t
min_bytes_process_bytes(push_parser_t *parser,
                        push_callback_t *pcallback,
                        const void *vbuf,
                        size_t bytes_available)
{
    min_bytes_t  *callback = (min_bytes_t *) pcallback;

    PUSH_DEBUG_MSG("min-bytes: Processing %zu bytes.\n",
                   bytes_available);

    if (callback->bytes_buffered < callback->minimum_bytes)
    {
        size_t  total_available;
        size_t  bytes_to_copy;
        ssize_t  result;

        /*
         * If we reach EOF without having met the minimum number of
         * bytes, we have a parse error.
         */

        if (bytes_available == 0)
        {
            PUSH_DEBUG_MSG("min-bytes: Reached EOF without meeting "
                           "minimum.\n");
            return PUSH_PARSE_ERROR;
        }

        /*
         * We haven't yet reached the minimum, but this chunk of data
         * might do it.  The total number of bytes we have available
         * is the sum of what we've already buffered and what we just
         * received.
         */

        total_available = callback->bytes_buffered + bytes_available;

        /*
         * If this chunk still isn't enough, copy it into the buffer
         * and return an incomplete code.
         */

        if (total_available < callback->minimum_bytes)
        {
            PUSH_DEBUG_MSG("min-bytes: Haven't met minimum, currently "
                           "have %zu bytes total.\n",
                           callback->bytes_buffered + bytes_available);

            memcpy(callback->buffer + callback->bytes_buffered,
                   vbuf, bytes_available);
            callback->bytes_buffered += bytes_available;

            return PUSH_INCOMPLETE;
        }

        /*
         * We've got enough data.  If there's nothing in the buffer,
         * then the first chunk of data is big enough, and we can just
         * send it into the wrapped callback directly.
         */

        if (callback->bytes_buffered == 0)
        {
            /*
             * Before passing off to the wrapped callback, claim that
             * we've “buffered” the minimum number of bytes.  (Even
             * though we've taken a shortcut and sent them into the
             * callback directly, rather than actually putting them
             * into the buffer.)  This will ensure that in any
             * subsequent calls, we skip all this buffering logic and
             * just go straight to the wrapped callback.
             */

            PUSH_DEBUG_MSG("min-bytes: First chunk meets the minimum. "
                           "Passing directly to wrapped callback.\n");

            callback->bytes_buffered = callback->minimum_bytes;
            return push_callback_tail_process_bytes
                (parser, &callback->base, callback->wrapped,
                 vbuf, bytes_available);
        }

        /*
         * Between the current buffer and the newly received chunk,
         * we've got enough for the wrapped callback.  Copy just
         * enough into the buffer to meet the minimum, then send in the
         * buffered data.
         */

        bytes_to_copy =
            callback->minimum_bytes - callback->bytes_buffered;

        PUSH_DEBUG_MSG("min-bytes: Copying %zu bytes to meet minimum.\n",
                       bytes_to_copy);

        memcpy(callback->buffer + callback->bytes_buffered,
               vbuf, bytes_to_copy);
        callback->bytes_buffered += bytes_available;

        result = push_callback_tail_process_bytes
            (parser, &callback->base, callback->wrapped,
             callback->buffer, callback->minimum_bytes);

        /*
         * If the wrapped callback succeeds, but doesn't use the
         * entire buffer, then return a parse error, since we have no
         * way to pass the unused bytes back up to any other
         * callbacks.
         */

        if (result > 0)
        {
            PUSH_DEBUG_MSG("min-bytes: Fatal -- wrapped callback "
                           "did not use all %zu bytes.\n",
                           callback->minimum_bytes);
            return PUSH_PARSE_ERROR;
        }

        /*
         * If the wrapped callback succeeds, return a success code —
         * but first, make sure to add back in the bytes that we
         * didn't pass into the callback.
         */

        if (result == 0)
        {
            PUSH_DEBUG_MSG("min-bytes: Wrapped callback succeeded using "
                           "%zu bytes.  %zu bytes unused.\n",
                           callback->minimum_bytes,
                           bytes_available - bytes_to_copy);
            return (bytes_available - bytes_to_copy);
        }

        /*
         * If the wrapped callback fails (without being incomplete),
         * go ahead and return that.
         */

        if (result != PUSH_INCOMPLETE)
        {
            return result;
        }

        /*
         * Otherwise, we need to pass the rest of the newly received
         * chunk (if any) back into the callback.
         */

        vbuf += bytes_to_copy;
        bytes_available -= bytes_to_copy;

        if (bytes_available == 0)
            return PUSH_INCOMPLETE;
    }

    /*
     * If we reach here, then we've either (a) already met the minimum
     * with previous chunks of data, or (b) we met the minimum with
     * the first part of this chunk, and have some data left over.
     * Either way, we need to pass the data into the wrapped callback.
     */

    return push_callback_tail_process_bytes
        (parser, &callback->base, callback->wrapped,
         vbuf, bytes_available);
}


static void
min_bytes_free(push_callback_t *pcallback)
{
    min_bytes_t  *callback = (min_bytes_t *) pcallback;

    PUSH_DEBUG_MSG("min-bytes: Freeing wrapped callback.\n");
    push_callback_free(callback->wrapped);
}


push_callback_t *
push_min_bytes_new(push_callback_t *wrapped,
                   size_t minimum_bytes)
{
    min_bytes_t  *result =
        (min_bytes_t *) malloc(sizeof(min_bytes_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       min_bytes_activate,
                       min_bytes_process_bytes,
                       min_bytes_free);

    result->buffer = malloc(minimum_bytes);
    if (result->buffer == NULL)
    {
        free(result);
        return NULL;
    }

    result->wrapped = wrapped;
    result->minimum_bytes = minimum_bytes;
    result->bytes_buffered = 0;

    return &result->base;
}
