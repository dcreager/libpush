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
    return result;
}


void
push_parser_free(push_parser_t *parser)
{
    /*
     * Free the parser's callback, and then the parser itself.
     */

    push_callback_free(parser->current_callback);
    free(parser);
}


void
push_parser_submit_data(push_parser_t *parser,
                        const void *buf,
                        size_t bytes_available)
{
    while (bytes_available > 0)
    {
        push_callback_t  *callback;

        size_t  bytes_to_send;
        size_t  bytes_left;
        size_t  bytes_processed;

        /*
         * Pass the data into the current callback.  The callback
         * might be requesting fewer bytes than are avaiable; if so,
         * only tell them about the bytes that they're requesting.
         */

        callback = parser->current_callback;

        bytes_to_send =
            (bytes_available > callback->bytes_requested)?
            callback->bytes_requested:
            bytes_available;

        bytes_left = parser->current_callback->
            process_bytes(parser, parser->current_callback,
                          buf, bytes_to_send);

        /*
         * The callback might not have processed all the data — either
         * because we didn't tell it about all of the data, or because
         * it returned a status code indicating that there's some data
         * left over.  Either way, update our bytes_available count
         * with the amount of data that was actually processed.
         */

        bytes_processed = bytes_to_send - bytes_left;
        bytes_available -= bytes_processed;
        buf += bytes_processed;
    }
}
