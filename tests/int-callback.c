/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <inttypes.h>
#include <stdint.h>

#include <push/basics.h>
#include <push/combinators.h>
#include <test-callbacks.h>


typedef struct _integer
{
    /**
     * The continuation that we'll call on a successful parse.
     */

    push_success_continuation_t  *success;

    /**
     * The continuation that we'll call if we run out of data in the
     * current chunk.
     */

    push_incomplete_continuation_t  *incomplete;

    /**
     * The continuation that we'll call if we encounter an error.
     */

    push_error_continuation_t  *error;

    /**
     * The continue continuation that will resume the EOF parser.
     */

    push_continue_continuation_t  cont;

} integer_t;


static void
integer_continue(void *user_data,
                 const void *buf,
                 size_t bytes_remaining)
{
    integer_t  *integer = (integer_t *) user_data;

    PUSH_DEBUG_MSG("integer: Processing %zu bytes at %p.\n",
                   bytes_remaining, buf);

    if (bytes_remaining < sizeof(uint32_t))
    {
        push_continuation_call(integer->error,
                               PUSH_PARSE_ERROR,
                               "Need more bytes to parse an integer");

        return;
    } else {
        uint32_t  next_integer = *((const uint32_t *) buf);
        buf += sizeof(uint32_t);
        bytes_remaining -= sizeof(uint32_t);

        PUSH_DEBUG_MSG("integer: Got %"PRIu32".\n", next_integer);

        push_continuation_call(integer->success,
                               &next_integer,
                               buf,
                               bytes_remaining);

        return;
    }
}


static void
integer_activate(void *user_data,
                 void *result,
                 const void *buf,
                 size_t bytes_remaining)
{
    integer_t  *integer = (integer_t *) user_data;

    if (bytes_remaining == 0)
    {
        /*
         * If we don't get any data when we're activated, return an
         * incomplete and wait for some data.
         */

        push_continuation_call(integer->incomplete,
                               &integer->cont);

        return;

    } else {
        /*
         * Otherwise let the continue continuation go ahead and
         * process this chunk of data.
         */

        integer_continue(user_data, buf, bytes_remaining);
        return;
    }
}


push_callback_t *
integer_callback_new(push_parser_t *parser)
{
    integer_t  *integer = (integer_t *) malloc(sizeof(integer_t));
    push_callback_t  *callback;

    if (integer == NULL)
        return NULL;

    callback = push_callback_new();
    if (callback == NULL)
    {
        free(integer);
        return NULL;
    }

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&callback->activate,
                          integer_activate,
                          integer);

    push_continuation_set(&integer->cont,
                          integer_continue,
                          integer);

    /*
     * By default, we call the parser's implementations of the
     * continuations that we call.
     */

    integer->success = &parser->success;
    integer->incomplete = &parser->incomplete;
    integer->error = &parser->error;

    /*
     * Set the pointers for the continuations that we call, so that
     * they can be changed by combinators, if necessary.
     */

    callback->success = &integer->success;
    callback->incomplete = &integer->incomplete;
    callback->error = &integer->error;

    return callback;
}

