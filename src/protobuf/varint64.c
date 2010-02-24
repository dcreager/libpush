/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <push/basics.h>
#include <push/talloc.h>

#include <push/protobuf/basics.h>
#include <push/protobuf/primitives.h>


/**
 * The user data struct for a varint64 callback.
 */


typedef struct _varint64
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The continute continuation that handles the first chunk of data
     * we receive.
     */

    push_continue_continuation_t  first_cont;

    /**
     * The continute continuation that handles the second and later
     * chunks of data we receive.
     */

    push_continue_continuation_t  rest_cont;

    /**
     * The number of bytes currently added to the varint.
     */

    size_t  bytes_processed;

    /**
     * The parsed value.
     */

    uint64_t  value;

} varint64_t;


static void
varint64_rest_continue(void *user_data,
                       const void *buf,
                       size_t bytes_remaining)
{
    varint64_t  *varint64 = (varint64_t *) user_data;
    const uint8_t  *ibuf = (uint8_t *) buf;

    /*
     * If we don't have any data to process, that's a parse error.
     */

    if (bytes_remaining == 0)
    {
        PUSH_DEBUG_MSG("%s: Reached EOF before end of varint.\n",
                       varint64->callback.name);

        push_continuation_call(varint64->callback.error,
                               PUSH_PARSE_ERROR,
                               "Reached EOF before end of varint");

        return;
    }

    /*
     * This continuation gets called for the second and later chunks,
     * or during the first chunk if we can't use the fast path.
     */

    uint8_t  shift = 7 * varint64->bytes_processed;

    PUSH_DEBUG_MSG("%s: Using slow path on %zu bytes.\n",
                   varint64->callback.name,
                   bytes_remaining);

    while (bytes_remaining > 0)
    {
        if (varint64->bytes_processed > PUSH_PROTOBUF_MAX_VARINT_LENGTH)
        {
            PUSH_DEBUG_MSG("%s: More than %u bytes in value.\n",
                           varint64->callback.name,
                           PUSH_PROTOBUF_MAX_VARINT_LENGTH);

            push_continuation_call(varint64->callback.error,
                                   PUSH_PARSE_ERROR,
                                   "Varint is too long");

            return;
        }

        PUSH_DEBUG_MSG("%s: Reading byte %zu, shifting by %"PRIu8"\n",
                       varint64->callback.name,
                       varint64->bytes_processed, shift);

        PUSH_DEBUG_MSG("%s:   byte = 0x%02"PRIx8"\n",
                       varint64->callback.name,
                       *ibuf);

        varint64->value |= ((uint64_t) (*ibuf & 0x7F)) << shift;
        shift += 7;

        varint64->bytes_processed++;
        buf++;
        bytes_remaining--;

        if (*ibuf < 0x80)
        {
            /*
             * This byte ends the varint.
             */

            PUSH_DEBUG_MSG("%s: Read value %"PRIu64
                           ", using %zu bytes\n",
                           varint64->callback.name,
                           varint64->value, varint64->bytes_processed);

            push_continuation_call(varint64->callback.success,
                                   &varint64->value,
                                   buf, bytes_remaining);

            return;
        }

        ibuf++;
    }

    /*
     * We ran out of data without processing a full varint, so
     * return the incomplete code.
     */

    push_continuation_call(varint64->callback.incomplete,
                           &varint64->rest_cont);

    return;
}


static void
varint64_first_continue(void *user_data,
                        const void *buf,
                        size_t bytes_remaining)
{
    varint64_t  *varint64 = (varint64_t *) user_data;
    uint8_t  *ibuf = (uint8_t *) buf;

    /*
     * If we don't have any data to process, that's a parse error.
     */

    if (bytes_remaining == 0)
    {
        PUSH_DEBUG_MSG("%s: Reached EOF before end of varint.\n",
                       varint64->callback.name);

        push_continuation_call(varint64->callback.error,
                               PUSH_PARSE_ERROR,
                               "Reached EOF before end of varint");

        return;
    }

    /*
     * This continuation gets called for the first data chunk.  In
     * many cases, we'll be able to use a fast path to parse the value
     * super-quickly.
     */

    /*
     * If there are enough bytes to read a maximum-length varint, or
     * if the last byte in the buffer would end a varint, then we can
     * use the “fast path”, and read each byte unchecked.
     */

    if ((bytes_remaining >= PUSH_PROTOBUF_MAX_VARINT_LENGTH) ||
        (ibuf[bytes_remaining-1] < 0x80))
    {
        uint32_t  part0 = 0;
        uint32_t  part1 = 0;
        uint32_t  part2 = 0;
        uint64_t  result;
        const uint8_t  *ptr = (const uint8_t *) buf;
        uint8_t  b;

        PUSH_DEBUG_MSG("%s: Using fast path\n",
                       varint64->callback.name);

        /*
         * Fast path: we know there are enough bytes in the buffer, so
         * we can read each byte unchecked.
         */

        b = *(ptr++); part0  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
        b = *(ptr++); part0 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
        b = *(ptr++); part0 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
        b = *(ptr++); part0 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
        b = *(ptr++); part1  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
        b = *(ptr++); part1 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
        b = *(ptr++); part1 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
        b = *(ptr++); part1 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
        b = *(ptr++); part2  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
        b = *(ptr++); part2 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;

        PUSH_DEBUG_MSG("%s: More than %u bytes in value.\n",
                       varint64->callback.name,
                       PUSH_PROTOBUF_MAX_VARINT_LENGTH);

        push_continuation_call(varint64->callback.error,
                               PUSH_PARSE_ERROR,
                               "Varint is too long");

        return;

      done:
        result =
            (((uint64_t) part0)      ) |
            (((uint64_t) part1) << 28) |
            (((uint64_t) part2) << 56);

        PUSH_DEBUG_MSG("%s: Read value %"PRIu64", using %zd bytes\n",
                       varint64->callback.name,
                       result, (ptr - ibuf));

        buf += (ptr - ibuf);
        bytes_remaining -= (ptr - ibuf);

        varint64->value = result;

        push_continuation_call(varint64->callback.success,
                               &varint64->value,
                               buf, bytes_remaining);

        return;
    }

    /*
     * Otherwise, we have to use the slow path.
     */

    return varint64_rest_continue(user_data, buf, bytes_remaining);
}


static void
varint64_activate(void *user_data,
                  void *result,
                  const void *buf,
                  size_t bytes_remaining)
{
    varint64_t  *varint64 = (varint64_t *) user_data;

    PUSH_DEBUG_MSG("%s: Activating with %zu bytes.\n",
                   varint64->callback.name,
                   bytes_remaining);

    /*
     * Initialize the fields that store the current value.
     */

    varint64->bytes_processed = 0;
    varint64->value = 0;

    if (bytes_remaining == 0)
    {
        /*
         * If we don't get any data when we're activated, return an
         * incomplete and wait for some data.
         */

        push_continuation_call(varint64->callback.incomplete,
                               &varint64->first_cont);

        return;

    } else {
        /*
         * Otherwise let the continue continuation go ahead and
         * process this chunk of data.
         */

        varint64_first_continue(user_data, buf, bytes_remaining);
        return;
    }
}


push_callback_t *
push_protobuf_varint64_new(const char *name,
                           push_parser_t *parser)
{
    varint64_t  *varint64 = push_talloc(parser, varint64_t);

    if (varint64 == NULL)
        return NULL;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL)
        name = "varint64";

    push_callback_init(name,
                       &varint64->callback, parser, varint64,
                       varint64_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&varint64->first_cont,
                          varint64_first_continue,
                          varint64);

    push_continuation_set(&varint64->rest_cont,
                          varint64_rest_continue,
                          varint64);

    return &varint64->callback;
}


/**
 * Parse a varint into a size_t.  We can only use the varint64 parser
 * if size_t is the same size as uint64_t.
 */

#if SIZE_MAX == UINT64_MAX

push_callback_t *
push_protobuf_varint_size_new(const char *name,
                              push_parser_t *parser)
{
    return push_protobuf_varint64_new(name, parser);
}

#endif
