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


/**
 * The user data struct a min-bytes callback.
 */

typedef struct _min_bytes
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The continue continuation that will resume the min-bytes
     * parser for the first chunk of data.
     */

    push_continue_continuation_t  first_cont;

    /**
     * The continue continuation that will resume the min-bytes
     * parser for all other chunks of data.
     */

    push_continue_continuation_t  rest_cont;

    /**
     * A success continuation for when we have to send data into the
     * wrapped callback in two pieces.  This happen when a chunk gives
     * us more than enough to meet the minimum, and we've buffered
     * part of the minimum.  In that case, we send in the buffered
     * data first (with exactly enough to meet the minimum), and the
     * remainder of the chunk second.
     */

    push_success_continuation_t  leftover_success;


    /**
     * An incomplete continuation for when we have to send data into
     * the wrapped callback in two pieces.  See leftover_success.
     */

    push_incomplete_continuation_t  leftover_incomplete;

    /**
     * A pointer to the remainder of the data chunk for when we have
     * to send data into the wrapped callback in two pieced.  See
     * leftover_success.
     */

    const void  *leftover_buf;

    /**
     * The size of leftover_buf.
     */

    size_t  leftover_size;

    /**
     * The input that we'll send to the wrapped callback.
     */

    void  *input;

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


static void
min_bytes_first_continue(void *user_data,
                         const void *buf,
                         size_t bytes_remaining)
{
    min_bytes_t  *min_bytes = (min_bytes_t *) user_data;

    /*
     * This continuation is called with the first chunk of data that
     * we receive.
     */

    PUSH_DEBUG_MSG("min-bytes: Processing %zu bytes.\n",
                   bytes_remaining);

    /*
     * If this chunk of data is large enough to satisy the minimum,
     * just go ahead and send it to the wrapped callback.
     */

    if (bytes_remaining >= min_bytes->minimum_bytes)
    {
        PUSH_DEBUG_MSG("min-bytes: First chunk of data "
                       "is large enough.\n");

        /*
         * We don't have anything further to do, so the wrapped
         * callback should use our final continuations.
         */

        min_bytes->wrapped->success = min_bytes->callback.success;
        min_bytes->wrapped->incomplete = min_bytes->callback.incomplete;
        min_bytes->wrapped->error = min_bytes->callback.error;

        push_continuation_call(&min_bytes->wrapped->activate,
                               min_bytes->input,
                               buf, bytes_remaining);

        return;
    }

    /*
     * Otherwise, if this is EOF, then we haven't reached the minimum,
     * which is a parse error.
     */

    if (bytes_remaining == 0)
    {
        PUSH_DEBUG_MSG("min-bytes: Reached EOF without meeting "
                       "minimum.\n");

        push_continuation_call(min_bytes->callback.error,
                               PUSH_PARSE_ERROR,
                               "Reached EOF without meeting minimum");

        return;
    }

    /*
     * Otherwise, add this chunk of data to the buffer and return
     * incomplete.
     */

    PUSH_DEBUG_MSG("min-bytes: Haven't met minimum, currently "
                   "have %zu bytes total.\n",
                   bytes_remaining);

    memcpy(min_bytes->buffer, buf, bytes_remaining);
    min_bytes->bytes_buffered = bytes_remaining;

    push_continuation_call(min_bytes->callback.incomplete,
                           &min_bytes->rest_cont);

    return;
}


static void
min_bytes_activate(void *user_data,
                   void *result,
                   const void *buf,
                   size_t bytes_remaining)
{
    min_bytes_t  *min_bytes = (min_bytes_t *) user_data;

    PUSH_DEBUG_MSG("min-bytes: Activating.\n");
    min_bytes->input = result;

    PUSH_DEBUG_MSG("min-bytes: Clearing buffer.\n");
    min_bytes->bytes_buffered = 0;

    if (bytes_remaining == 0)
    {
        /*
         * If we don't get any data when we're activated, return an
         * incomplete and wait for some data.
         */

        push_continuation_call(min_bytes->callback.incomplete,
                               &min_bytes->first_cont);

        return;

    } else {
        /*
         * Otherwise let the continue continuation go ahead and
         * process this chunk of data.
         */

        min_bytes_first_continue(user_data, buf, bytes_remaining);
        return;
    }
}


static void
min_bytes_rest_continue(void *user_data,
                        const void *buf,
                        size_t bytes_remaining)
{
    min_bytes_t  *min_bytes = (min_bytes_t *) user_data;
    size_t  total_available =
        min_bytes->bytes_buffered + bytes_remaining;

    /*
     * This continuation is called with the second and later chunks of
     * data that we receive.
     */

    PUSH_DEBUG_MSG("min-bytes: Processing %zu bytes.\n",
                   bytes_remaining);

    /*
     * If this chunk of data gives us enough to satisy the minimum,
     * send it to the wrapped callback.
     */

    if (total_available >= min_bytes->minimum_bytes)
    {
        size_t  bytes_to_copy;

        /*
         * Between the current buffer and the newly received chunk,
         * we've got enough for the wrapped callback.  Copy just
         * enough into the buffer to meet the minimum, then send in
         * the buffered data.
         */

        bytes_to_copy =
            min_bytes->minimum_bytes - min_bytes->bytes_buffered;

        PUSH_DEBUG_MSG("min-bytes: Copying %zu bytes to meet minimum.\n",
                       bytes_to_copy);

        memcpy(min_bytes->buffer + min_bytes->bytes_buffered,
               buf, bytes_to_copy);
        min_bytes->bytes_buffered += bytes_remaining;

        buf += bytes_to_copy;
        bytes_remaining -= bytes_to_copy;

        /*
         * If this chunk gives us exactly enough to meet the minimum,
         * with nothing left over, then we can transfer control over
         * to the wrapped callback completely.
         */

        if (bytes_remaining == 0)
        {
            min_bytes->wrapped->success =
                min_bytes->callback.success;
            min_bytes->wrapped->incomplete =
                min_bytes->callback.incomplete;
            min_bytes->wrapped->error =
                min_bytes->callback.error;

        } else {
            /*
             * Otherwise, once the wrapped callback processes the
             * buffered data, we have to handle the rest of the chunk
             * we just received.
             */

            min_bytes->leftover_buf = buf;
            min_bytes->leftover_size = bytes_remaining;

            min_bytes->wrapped->success =
                &min_bytes->leftover_success;
            min_bytes->wrapped->incomplete =
                &min_bytes->leftover_incomplete;
            min_bytes->wrapped->error =
                min_bytes->callback.error;
        }

        push_continuation_call(&min_bytes->wrapped->activate,
                               min_bytes->input,
                               min_bytes->buffer,
                               min_bytes->minimum_bytes);

        return;
    }

    /*
     * Otherwise, if this is EOF, then we haven't reached the minimum,
     * which is a parse error.
     */

    if (bytes_remaining == 0)
    {
        PUSH_DEBUG_MSG("min-bytes: Reached EOF without meeting "
                       "minimum.\n");

        push_continuation_call(min_bytes->callback.error,
                               PUSH_PARSE_ERROR,
                               "Reached EOF without meeting minimum");

        return;
    }

    /*
     * Otherwise, add this chunk of data to the buffer and return
     * incomplete.
     */

    PUSH_DEBUG_MSG("min-bytes: Haven't met minimum, currently "
                   "have %zu bytes total.\n",
                   bytes_remaining);

    memcpy(min_bytes->buffer + min_bytes->bytes_buffered,
           buf, bytes_remaining);
    min_bytes->bytes_buffered = bytes_remaining;

    push_continuation_call(min_bytes->callback.incomplete,
                           &min_bytes->rest_cont);

    return;
}


static void
min_bytes_leftover_success(void *user_data,
                           void *result,
                           const void *buf,
                           size_t bytes_remaining)
{
    min_bytes_t  *min_bytes = (min_bytes_t *) user_data;

    /*
     * If we reach this continuation, then we sent in exactly enough
     * data to meet the minimum, and this was enough for the wrapped
     * callback to succeed.  There's data left over in the current
     * chunk, which we pass back in our overall success result.
     */

    PUSH_DEBUG_MSG("min-bytes: Wrapped callback succeeded with "
                   "%zu bytes left in chunk.\n",
                   min_bytes->leftover_size);

    /*
     * We assume that the wrapped callback will always process at
     * least the minimum number of bytes.  If it didn't, throw a parse
     * error.
     */

    if (bytes_remaining != 0)
    {
        PUSH_DEBUG_MSG("min-bytes: Wrapped callback didn't process "
                       "all %zu bytes.\n",
                       min_bytes->minimum_bytes);

        push_continuation_call(min_bytes->callback.error,
                               PUSH_PARSE_ERROR,
                               "Wrapped callback didn't process "
                               "full minimum.");

        return;
    }

    push_continuation_call(min_bytes->callback.success,
                           result,
                           min_bytes->leftover_buf,
                           min_bytes->leftover_size);

    return;
}


static void
min_bytes_leftover_incomplete(void *user_data,
                              push_continue_continuation_t *cont)
{
    min_bytes_t  *min_bytes = (min_bytes_t *) user_data;

    /*
     * If we reach this continuation, then we sent in exactly enough
     * data to meet the minimum, but the wrapped callback needs more.
     * There's data left over in the current chunk, so we pass that in
     * to the wrapped callback.
     */

    PUSH_DEBUG_MSG("min-bytes: Sending remaining %zu bytes in chunk "
                   "into wrapped callback.\n",
                   min_bytes->leftover_size);

    /*
     * We don't have anything further to do, so the wrapped
     * callback should use our final continuations.
     */

    min_bytes->wrapped->success = min_bytes->callback.success;
    min_bytes->wrapped->incomplete = min_bytes->callback.incomplete;
    min_bytes->wrapped->error = min_bytes->callback.error;

    push_continuation_call(cont,
                           min_bytes->leftover_buf,
                           min_bytes->leftover_size);

    return;
}


push_callback_t *
push_min_bytes_new(push_parser_t *parser,
                   push_callback_t *wrapped,
                   size_t minimum_bytes)
{
    min_bytes_t  *min_bytes =
        (min_bytes_t *) malloc(sizeof(min_bytes_t));

    if (min_bytes == NULL)
        return NULL;

    /*
     * Try to allocate the internal buffer.
     */

    min_bytes->buffer = malloc(minimum_bytes);
    if (min_bytes->buffer == NULL)
    {
        free(min_bytes);
        return NULL;
    }

    /*
     * Initialize the callback superclass.
     */

    push_callback_init(&min_bytes->callback, parser,
                       min_bytes_activate, min_bytes);

    /*
     * Fill in the data items.
     */

    min_bytes->wrapped = wrapped;
    min_bytes->minimum_bytes = minimum_bytes;

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&min_bytes->first_cont,
                          min_bytes_first_continue,
                          min_bytes);

    push_continuation_set(&min_bytes->rest_cont,
                          min_bytes_rest_continue,
                          min_bytes);

    push_continuation_set(&min_bytes->leftover_success,
                          min_bytes_leftover_success,
                          min_bytes);

    push_continuation_set(&min_bytes->leftover_incomplete,
                          min_bytes_leftover_incomplete,
                          min_bytes);

    return &min_bytes->callback;
}
