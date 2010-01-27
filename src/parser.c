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
#include <stdlib.h>
#include <unistd.h>

#include <push/basics.h>

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
     * If it works, initialize the new instance.
     */

    result->callback = callback;
    result->finished = false;

    return result;
}


void
push_parser_free(push_parser_t *parser)
{
    /*
     * Free the parser's callback, and then the parser itself.
     */

    push_callback_free(parser->callback);
    free(parser);
}


push_error_code_t
push_parser_activate(push_parser_t *parser,
                     void *input)
{
    PUSH_DEBUG_MSG("parser: Activating with input pointer %p.\n",
                   input);
    parser->finished = false;
    return push_callback_activate(parser, parser->callback, input);
}


push_error_code_t
push_parser_submit_data(push_parser_t *parser,
                        const void *buf,
                        size_t bytes_available)
{
    push_callback_t  *callback = parser->callback;
    ssize_t  result;

    PUSH_DEBUG_MSG("parser: Processing %zu bytes at %p.\n",
                   bytes_available, buf);

    /*
     * If we've already finished parsing, ignore this data and return.
     */

    if (parser->finished)
    {
        PUSH_DEBUG_MSG("parser: Parse already successful; "
                       "skipping rest of input.\n");
        return PUSH_SUCCESS;
    }

    /*
     * Otherwise, pass the data into the parser's callback.
     */

    result = push_callback_process_bytes(parser, callback,
                                         buf, bytes_available);

    /*
     * If we receive an error code (including PUSH_INCOMPLETE), return
     * it.
     */

    if (result < 0)
        return result;

    /*
     * Otherwise, we have a successful parse, possibly with some
     * leftover data.  Ignore the leftover data, remember that we've
     * finished parsing, and return the success code.
     */

    parser->finished = true;
    return PUSH_SUCCESS;
}


push_error_code_t
push_parser_eof(push_parser_t *parser)
{
    PUSH_DEBUG_MSG("parser: EOF received.\n");

    /*
     * If we already parsed successfully with previous calls, then the
     * EOF is valid, since we're supposed to ignore any later data.
     */

    if (parser->finished)
    {
        PUSH_DEBUG_MSG("parser: Parse already successful; "
                       "ignoring EOF.\n");
        return PUSH_SUCCESS;
    }

    /*
     * Otherwise, tell the callback about the EOF, so it can tell us
     * whether there's a parse error..
     */

    return parser->callback->process_bytes(parser, parser->callback,
                                           NULL, 0);
}
