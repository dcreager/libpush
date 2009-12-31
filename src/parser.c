/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdbool.h>
#include <string.h>

#include <push.h>

push_parser_t *
push_parser_new(push_callback_t *callback)
{
    push_parser_t  *result;

    /*
     * First try to allocate the new parser instance.  If we can't,
     * return NULL.
     */

    result = (push_parser_t *) malloc(sizeof(push_parser_t));
    if (result == NULL)
    {
        return NULL;
    }

    /*
     * If it works, initialize and return the new instance.
     */

    result->current_callback = callback;
    hwm_buffer_init(&result->leftover);
    return result;
}


void
push_parser_free(push_parser_t *parser)
{
    /*
     * Free the parser's callback, and then the parser itself.
     */

    push_callback_free(parser->current_callback);
    hwm_buffer_done(&parser->leftover);
    free(parser);
}


/**
 * Send some data to the current callback's process_bytes function.
 * The caller is responsible for ensuring that <code>bytes_available >
 * callback->min_bytes_requested</code>.  We will make sure not to
 * send in more than <code>max_bytes_requested</code>.  The buf and
 * bytes_available variables will be updated based on the results of
 * the callback, in case it doesn't process all of the data.
 */

static bool
process_bytes(push_parser_t *parser,
              const void **buf,
              size_t *bytes_available)
{
    push_callback_t  *callback = parser->current_callback;

    size_t  bytes_to_send;
    size_t  bytes_left;
    size_t  bytes_processed;

    /*
     * We might have more data than the callback's maximum; if so,
     * only tell them about the bytes that they're requesting.
     */

    bytes_to_send =
        (callback->max_bytes_requested > 0)?
        callback->max_bytes_requested:
        *bytes_available;

    bytes_left = callback->process_bytes
        (parser, callback, *buf, bytes_to_send);

    /*
     * The callback might not have processed all the data — either
     * because we didn't tell it about all of the data, or because it
     * returned a status code indicating that there's some data left
     * over.  (Or both.)  Either way, update our bytes_available count
     * with the amount of data that was actually processed.
     */

    bytes_processed = bytes_to_send - bytes_left;
    *bytes_available -= bytes_processed;
    *buf += bytes_processed;

    return true;
}


/**
 * Send the contents of the leftovers buffer into the callback's
 * process_bytes function.  If there isn't enough data in the
 * leftovers buffer to meet the callback's minimum, we'll append in
 * some data from the submitted first.  Any data that isn't processed
 * by the callback will be kept in the leftovers buffer.
 */

static bool
process_leftovers(push_parser_t *parser,
                  const void **buf,
                  size_t *bytes_available)
{
    push_callback_t  *callback = parser->current_callback;
    const void  *leftover_buf;
    size_t  original_size;
    size_t  leftover_size;

    /*
     * If needed, append some data from the submitted buffer to meet
     * the minimum.
     */

    if (parser->leftover.current_size < callback->min_bytes_requested)
    {
        size_t  bytes_to_add;

        /*
         * Add just enough data from the submitted buffer to meet the
         * callback's minimum.
         */

        bytes_to_add =
            callback->min_bytes_requested -
            parser->leftover.current_size;

        PUSH_DEBUG_MSG("Adding %zu bytes from %p to leftover buffer\n",
                       bytes_to_add, *buf);

        if (!hwm_buffer_append_mem(&parser->leftover, *buf,
                                   bytes_to_add))
        {
            /*
             * Uh-oh!  TODO: Better error handling.
             */

            PUSH_DEBUG_MSG("Cannot copy bytes into leftover buffer.\n");
            return false;
        }

        *buf += bytes_to_add;
        *bytes_available -= bytes_to_add;
    }

    /*
     * Pass the leftover data into the callback.
     */

    leftover_buf = hwm_buffer_mem(&parser->leftover, void);
    original_size = leftover_size = parser->leftover.current_size;

    if (!process_bytes(parser, &leftover_buf, &leftover_size))
    {
        /*
         * Uh-oh!  TODO: Better error handling.
         */

        PUSH_DEBUG_MSG("Could not process bytes in leftover buffer.\n");
        return false;
    }

    /*
     * If we still have some leftovers, move them up to the beginning
     * of the leftover buffer.  Otherwise, clear the leftover buffer.
     */

    if (leftover_size == 0)
    {
        PUSH_DEBUG_MSG("Clearing leftover buffer.\n");
        parser->leftover.current_size = 0;
    } else {
        size_t  bytes_processed = original_size - leftover_size;
        void  *writable =
            hwm_buffer_writable_mem(&parser->leftover, void);

        PUSH_DEBUG_MSG("Keeping %zu bytes in leftover buffer.\n",
                       leftover_size);

        memmove(writable, writable + bytes_processed, leftover_size);
        parser->leftover.current_size = leftover_size;
    }

    return true;
}


bool
push_parser_submit_data(push_parser_t *parser,
                        const void *buf,
                        size_t bytes_available)
{
    /*
     * Execute the body of the while loop for as long as we have
     * enough data to meet the callback's minimum.
     */

    while ((parser->leftover.current_size + bytes_available) >=
           (parser->current_callback->min_bytes_requested))
    {
        if (hwm_buffer_is_empty(&parser->leftover))
        {
            /*
             * If there's no data in the leftover buffer, then we just
             * pass the submitted buffer into the callback.  (We know
             * this must meet the minimum, because otherwise the
             * while-loop test would've failed.)
             */

            if (!process_bytes(parser, &buf, &bytes_available))
            {
                return false;
            }

        } else {
            /*
             * If there's data in the leftover buffer, then we pass
             * that into the callback first.
             */

            if (!process_leftovers(parser, &buf, &bytes_available))
            {
                return false;
            }
        }
    }

    /*
     * If we can't send any more data into the callback, but we still
     * have some left, store it in the leftover buffer.
     */

    if (bytes_available > 0)
    {
        if (!hwm_buffer_append_mem(&parser->leftover, buf, bytes_available))
        {
            return false;
        }
    }

    return true;
}
