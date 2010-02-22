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
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The continue continuation that will resume the EOF parser.
     */

    push_continue_continuation_t  cont;

    /**
     * The parsed integer.
     */

    uint32_t  parsed_value;

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
        push_continuation_call(integer->callback.error,
                               PUSH_PARSE_ERROR,
                               "Need more bytes to parse an integer");

        return;
    } else {
        const uint32_t  *next_integer = (const uint32_t *) buf;
        buf += sizeof(uint32_t);
        bytes_remaining -= sizeof(uint32_t);

        integer->parsed_value = *next_integer;

        PUSH_DEBUG_MSG("integer: Got %"PRIu32".\n", integer->parsed_value);

        push_continuation_call(integer->callback.success,
                               &integer->parsed_value,
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

        push_continuation_call(integer->callback.incomplete,
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


static push_callback_t *
inner_integer_callback_new(push_parser_t *parser)
{
    integer_t  *integer = (integer_t *) malloc(sizeof(integer_t));

    if (integer == NULL)
        return NULL;

    push_callback_init(&integer->callback, parser, integer,
                       integer_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&integer->cont,
                          integer_continue,
                          integer);

    return &integer->callback;
}


push_callback_t *
integer_callback_new(push_parser_t *parser)
{
    push_callback_t  *integer = NULL;
    push_callback_t  *min_bytes = NULL;

    integer = inner_integer_callback_new(parser);
    if (integer == NULL)
        return NULL;

    min_bytes = push_min_bytes_new(parser, integer, sizeof(uint32_t));
    if (min_bytes == NULL)
    {
        push_callback_free(integer);
        return NULL;
    }

    return min_bytes;
}

