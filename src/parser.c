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


static void
parser_success(void *user_data,
               void *result,
               const void *buf,
               size_t bytes_remaining)
{
    push_parser_t  *parser = (push_parser_t *) user_data;

    PUSH_DEBUG_MSG("parser: Parse successful with final result %p.\n",
                   result);

    PUSH_DEBUG_MSG("parser: %zu bytes remaining in current chunk.\n",
                   bytes_remaining);

    /*
     * When the last callback finishes with a successful parse, save
     * the result code and the callback's output value.
     */

    parser->result_code = PUSH_SUCCESS;
    parser->result = result;

    /*
     * Register a continue callback that will ignore any further data.
     */

    parser->cont = &parser->ignore;
}


static void
parser_incomplete(void *user_data,
                  push_continue_continuation_t *cont)
{
    push_parser_t  *parser = (push_parser_t *) user_data;

    PUSH_DEBUG_MSG("parser: Finished parsing this chunk, "
                   "parse incomplete.\n");

    /*
     * When the current callback returns an incomplete, register its
     * continue continuation, so that we can send the next chunk of
     * data to it.
     */

    parser->cont = cont;

    /*
     * Save the incomplete result code.
     */

    parser->result_code = PUSH_INCOMPLETE;
}


static void
parser_error(void *user_data,
             push_error_code_t error_code,
             const char *error_message)
{
    push_parser_t  *parser = (push_parser_t *) user_data;

    PUSH_DEBUG_MSG("parser: Parse fails with error code %d.\n",
                   error_code);

    /*
     * When the last callback finishes with an error, save the result
     * code.
     */

    parser->result_code = error_code;
}


static void
parser_ignore(void *user_data,
              const void *buf,
              size_t bytes_remaining)
{
    /*
     * Don't do anything.
     */

    PUSH_DEBUG_MSG("parser: Skipping %zu bytes after finished parse.\n",
                   bytes_remaining);
}


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
     * Initialize the continuations that we implement.
     */

    push_continuation_set(&result->success,
                          parser_success,
                          result);

    push_continuation_set(&result->incomplete,
                          parser_incomplete,
                          result);

    push_continuation_set(&result->error,
                          parser_error,
                          result);

    push_continuation_set(&result->ignore,
                          parser_ignore,
                          result);

    return result;
}


void
push_parser_free(push_parser_t *parser)
{
    /*
     * TODO: Free the parser's callback.
     */

    /*
     * Then free the parser itself.
     */

    free(parser);
}


void
push_parser_set_callback(push_parser_t *parser,
                         push_callback_t *callback)
{
    /*
     * The parser should call the callback's activation callback when
     * it starts.
     */

    parser->activate = &callback->activate;

    /*
     * There is no initial continue continuation; this will be set
     * when a parser returns an incomplete.
     */

    parser->cont = NULL;
}


push_error_code_t
push_parser_activate(push_parser_t *parser,
                     void *input)
{
    PUSH_DEBUG_MSG("parser: Activating with input pointer %p.\n",
                   input);

    /*
     * We activate the initial callback without any data.  In most
     * cases, this will cause it to return incomplete.
     */

    push_continuation_call(parser->activate, input, NULL, 0);

    /*
     * Eventually, the callback will call one of the parser's
     * continuations, which will set the result_code field.
     */

    return parser->result_code;
}


push_error_code_t
push_parser_submit_data(push_parser_t *parser,
                        const void *buf,
                        size_t bytes_available)
{
    PUSH_DEBUG_MSG("parser: Processing %zu bytes at %p.\n",
                   bytes_available, buf);

    /*
     * Pass the data into the current continue continuation.
     */

    push_continuation_call(parser->cont, buf, bytes_available);

    /*
     * Eventually, the callback will call one of the parser's
     * continuations, which will set the result_code field.
     */

    return parser->result_code;
}


push_error_code_t
push_parser_eof(push_parser_t *parser)
{
    PUSH_DEBUG_MSG("parser: EOF received.\n");

    /*
     * Pass the EOF into the current continue continuation.
     */

    push_continuation_call(parser->cont, NULL, 0);

    /*
     * Eventually, the callback will call one of the parser's
     * continuations, which will set the result_code field.
     */

    return parser->result_code;
}
