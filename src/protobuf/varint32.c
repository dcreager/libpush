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
#include <push/talloc.h>

#include <push/protobuf/basics.h>
#include <push/protobuf/primitives.h>


/**
 * The user data struct for a varint32 callback.
 */

typedef struct _varint32
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

    uint32_t  value;

} varint32_t;


static void
varint32_rest_continue(void *user_data,
                       const void *buf,
                       size_t bytes_remaining)
{
    varint32_t  *varint32 = (varint32_t *) user_data;
    const uint8_t  *ibuf = (uint8_t *) buf;

    /*
     * If we don't have any data to process, that's a parse error.
     */

    if (bytes_remaining == 0)
    {
        PUSH_DEBUG_MSG("%s: Reached EOF before end of varint.\n",
                       push_talloc_get_name(varint32));

        push_continuation_call(varint32->callback.error,
                               PUSH_PARSE_ERROR,
                               "Reached EOF before end of varint");

        return;
    }

    /*
     * This continuation gets called for the second and later chunks,
     * or during the first chunk if we can't use the fast path.
     */

    uint8_t  shift = 7 * varint32->bytes_processed;

    PUSH_DEBUG_MSG("%s: Using slow path on %zu bytes.\n",
                   push_talloc_get_name(varint32),
                   bytes_remaining);

    while (bytes_remaining > 0)
    {
        if (varint32->bytes_processed > PUSH_PROTOBUF_MAX_VARINT_LENGTH)
        {
            PUSH_DEBUG_MSG("%s: More than %u bytes in value.\n",
                           push_talloc_get_name(varint32),
                           PUSH_PROTOBUF_MAX_VARINT_LENGTH);

            push_continuation_call(varint32->callback.error,
                                   PUSH_PARSE_ERROR,
                                   "Varint is too long");

            return;
        }

        PUSH_DEBUG_MSG("%s: Reading byte %zu, shifting by %"PRIu8"\n",
                       push_talloc_get_name(varint32),
                       varint32->bytes_processed, shift);

        PUSH_DEBUG_MSG("%s:   byte = 0x%02"PRIx8"\n",
                       push_talloc_get_name(varint32),
                       *ibuf);

        varint32->value |= (*ibuf & 0x7F) << shift;
        shift += 7;

        varint32->bytes_processed++;
        buf++;
        bytes_remaining--;

        if (*ibuf < 0x80)
        {
            /*
             * This byte ends the varint.
             */

            PUSH_DEBUG_MSG("%s: Read value %"PRIu32
                           ", using %zu bytes\n",
                           push_talloc_get_name(varint32),
                           varint32->value, varint32->bytes_processed);

            push_continuation_call(varint32->callback.success,
                                   &varint32->value,
                                   buf, bytes_remaining);

            return;
        }

        ibuf++;
    }

    /*
     * We ran out of data without processing a full varint, so return
     * the incomplete code.
     */

    push_continuation_call(varint32->callback.incomplete,
                           &varint32->rest_cont);

    return;
}


static void
varint32_first_continue(void *user_data,
                        const void *buf,
                        size_t bytes_remaining)
{
    varint32_t  *varint32 = (varint32_t *) user_data;
    uint8_t  *ibuf = (uint8_t *) buf;

    /*
     * If we don't have any data to process, that's a parse error.
     */

    if (bytes_remaining == 0)
    {
        PUSH_DEBUG_MSG("%s: Reached EOF before end of varint.\n",
                       push_talloc_get_name(varint32));

        push_continuation_call(varint32->callback.error,
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
     * Special case for single-byte varints — super common, especially
     * for tag numbers.
     */

    if (*ibuf < 0x80)
    {
        PUSH_DEBUG_MSG("%s: Using super-fast path\n",
                       push_talloc_get_name(varint32));

        varint32->value = *ibuf;

        PUSH_DEBUG_MSG("%s: Read value %"PRIu32", using 1 byte\n",
                       push_talloc_get_name(varint32),
                       varint32->value);

        buf++;
        bytes_remaining--;

        push_continuation_call(varint32->callback.success,
                               &varint32->value,
                               buf, bytes_remaining);

        return;
    }

    /*
     * If there are enough bytes to read a maximum-length varint, or
     * if the last byte in the buffer would end a varint, then we can
     * use the “fast path”, and read each byte unchecked.
     */

    if ((bytes_remaining >= PUSH_PROTOBUF_MAX_VARINT_LENGTH) ||
        (ibuf[bytes_remaining-1] < 0x80))
    {
        uint32_t  result;
        const uint8_t  *ptr = (const uint8_t *) buf;
        uint8_t  b;
        int  i;

        PUSH_DEBUG_MSG("%s: Using fast path\n",
                       push_talloc_get_name(varint32));

        /*
         * Fast path: we know there are enough bytes in the buffer, so
         * we can read each byte unchecked.
         */

        b = *(ptr++); result  = (b & 0x7f)      ; if (!(b & 0x80)) goto done;
        b = *(ptr++); result |= (b & 0x7f) <<  7; if (!(b & 0x80)) goto done;
        b = *(ptr++); result |= (b & 0x7f) << 14; if (!(b & 0x80)) goto done;
        b = *(ptr++); result |= (b & 0x7f) << 21; if (!(b & 0x80)) goto done;
        b = *(ptr++); result |=  b         << 28; if (!(b & 0x80)) goto done;

        for (i = 0; i < 5; i++)
        {
            b = *(ptr++); if (!(b & 0x80)) goto done;
        }

        PUSH_DEBUG_MSG("%s: More than %u bytes in value.\n",
                       push_talloc_get_name(varint32),
                       PUSH_PROTOBUF_MAX_VARINT_LENGTH);

        push_continuation_call(varint32->callback.error,
                               PUSH_PARSE_ERROR,
                               "Varint is too long");

        return;

      done:
        PUSH_DEBUG_MSG("%s: Read value %"PRIu32", using %zd bytes\n",
                       push_talloc_get_name(varint32),
                       result, (ptr - ibuf));

        buf += (ptr - ibuf);
        bytes_remaining -= (ptr - ibuf);

        varint32->value = result;

        push_continuation_call(varint32->callback.success,
                               &varint32->value,
                               buf, bytes_remaining);

        return;
    }

    /*
     * Otherwise, we have to use the slow path.
     */

    return varint32_rest_continue(user_data, buf, bytes_remaining);
}


static void
varint32_activate(void *user_data,
                  void *result,
                  const void *buf,
                  size_t bytes_remaining)
{
    varint32_t  *varint32 = (varint32_t *) user_data;

    PUSH_DEBUG_MSG("%s: Activating with %zu bytes.\n",
                   push_talloc_get_name(varint32),
                   bytes_remaining);

    /*
     * Initialize the fields that store the current value.
     */

    varint32->bytes_processed = 0;
    varint32->value = 0;

    if (bytes_remaining == 0)
    {
        /*
         * If we don't get any data when we're activated, return an
         * incomplete and wait for some data.
         */

        push_continuation_call(varint32->callback.incomplete,
                               &varint32->first_cont);

        return;

    } else {
        /*
         * Otherwise let the continue continuation go ahead and
         * process this chunk of data.
         */

        varint32_first_continue(user_data, buf, bytes_remaining);
        return;
    }
}


push_callback_t *
push_protobuf_varint32_new(const char *name,
                           void *parent,
                           push_parser_t *parser)
{
    varint32_t  *varint32 = push_talloc(parent, varint32_t);

    if (varint32 == NULL)
        return NULL;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL) name = "varint32";
    push_talloc_set_name_const(varint32, name);

    push_callback_init(&varint32->callback, parser, varint32,
                       varint32_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&varint32->first_cont,
                          varint32_first_continue,
                          varint32);

    push_continuation_set(&varint32->rest_cont,
                          varint32_rest_continue,
                          varint32);

    return &varint32->callback;
}


/**
 * Parse a varint into a size_t.  We can only use the varint32 parser
 * if size_t is the same size as uint32_t.
 */

#if SIZE_MAX == UINT32_MAX

push_callback_t *
push_protobuf_varint_size_new(const char *name,
                              void *parent,
                              push_parser_t *parser)
{
    return push_protobuf_varint32_new(name, parent, parser);
}

#endif
