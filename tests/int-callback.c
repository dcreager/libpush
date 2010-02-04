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


typedef struct _integer_callback
{
    push_callback_t  base;
    uint32_t  integer;
} integer_callback_t;


static ssize_t
integer_process_bytes(push_parser_t *parser,
                      push_callback_t *pcallback,
                      const void *buf,
                      size_t bytes_available)
{
    integer_callback_t  *callback = (integer_callback_t *) pcallback;

    PUSH_DEBUG_MSG("integer: Processing %zu bytes at %p.\n",
                   bytes_available, buf);

    if (bytes_available < sizeof(uint32_t))
    {
        return PUSH_PARSE_ERROR;
    } else {
        const uint32_t  *next_integer = (const uint32_t *) buf;
        buf += sizeof(uint32_t);
        bytes_available -= sizeof(uint32_t);

        PUSH_DEBUG_MSG("integer: Got %"PRIu32".\n", *next_integer);

        callback->integer = *next_integer;

        return bytes_available;
    }
}


push_callback_t *
integer_callback_new()
{
    integer_callback_t  *ints =
        (integer_callback_t *) malloc(sizeof(integer_callback_t));
    push_callback_t  *result;

    if (ints == NULL)
        return NULL;

    push_callback_init(&ints->base,
                       NULL,
                       integer_process_bytes,
                       NULL);

    ints->integer = 0;
    ints->base.result = &ints->integer;

    result = push_min_bytes_new(&ints->base, sizeof(uint32_t));
    if (result == NULL)
    {
        push_callback_free(&ints->base);
        return NULL;
    }

    return result;
}

