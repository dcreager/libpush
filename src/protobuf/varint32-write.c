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
#include <string.h>

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
     * The continue continuation that handles any buffers we receive.
     */

    push_continue_continuation_t  cont;

    /**
     * The original integer.
     */

    uint32_t  *src;

    /**
     * A buffer that stores the encoded value.  This isn't always
     * filled in; if we're activated with a buffer that's large enough
     * to hold any varint32, we just encode directly into that buffer.
     * Otherwise, we encode into here, and then copy into the output
     * buffer as appropriate.
     */

    uint8_t  encoded[PUSH_PROTOBUF_MAX_VARINT32_LENGTH];

    /**
     * The length of the encoded value.
     */

    size_t  encoded_length;

    /**
     * The number of bytes that have already been copied from encoded
     * into an output buffer.
     */

    size_t  bytes_written;

} varint32_t;


/**
 * Quickly encode a varint32 integer.  You must ensure that dest has
 * at least PUSH_PROTOBUF_MAX_VARINT32_LENGTH bytes available.
 */

static size_t
varint32_fast_write(uint8_t *dest, uint32_t src)
{
    /*
     * Fast path: We have enough bytes left in the buffer to guarantee
     * that this write won't cross the end, so we can skip the checks.
     */

    dest[0] = (uint8_t) (src | 0x80);
    if (src >= (1 << 7))
    {
        dest[1] = (uint8_t) ((src >> 7) | 0x80);
        if (src >= (1 << 14))
        {
            dest[2] = (uint8_t) ((src >> 14) | 0x80);
            if (src >= (1 << 21))
            {
                dest[3] = (uint8_t) ((src >> 21) | 0x80);
                if (src >= (1 << 28))
                {
                    dest[4] = (uint8_t) (src >> 28);
                    return 5;
                } else {
                    dest[3] &= 0x7F;
                    return 4;
                }
            } else {
                dest[2] &= 0x7F;
                return 3;
            }
        } else {
            dest[1] &= 0x7F;
            return 2;
        }
    } else {
        dest[0] &= 0x7F;
        return 1;
    }
}


static void
varint32_continue(void *user_data,
                  void *buf,
                  size_t bytes_remaining)
{
    varint32_t  *varint32 = (varint32_t *) user_data;
    size_t  bytes_to_copy;

    /*
     * Hopefully we can copy over everything, but there might not be
     * enough space.
     */

    bytes_to_copy =
        varint32->encoded_length - varint32->bytes_written;

    if (bytes_remaining < bytes_to_copy)
        bytes_to_copy = bytes_remaining;

    /*
     * Copy the encoded value into the output buffer.
     */

    PUSH_DEBUG_MSG("%s: Copying %zu bytes to output buffer.\n",
                   push_talloc_get_name(varint32),
                   bytes_to_copy);

    memcpy(buf, varint32->encoded + varint32->bytes_written,
           bytes_to_copy);
    buf += bytes_to_copy;
    bytes_remaining -= bytes_to_copy;
    varint32->bytes_written += bytes_to_copy;

    /*
     * If that's everything, fire our success continuation; otherwise,
     * fire the incomplete continuation.
     */

    if (varint32->bytes_written == varint32->encoded_length)
    {
        push_continuation_call(varint32->callback.success,
                               varint32->src,
                               buf, bytes_remaining);

        return;
    } else {
        PUSH_DEBUG_MSG("%s: %zu bytes remaining.\n",
                       push_talloc_get_name(varint32),
                       varint32->encoded_length -
                       varint32->bytes_written);

        push_continuation_call(varint32->callback.incomplete,
                               &varint32->cont);

        return;
    }
}


static void
varint32_activate(void *user_data,
                  void *result,
                  void *buf,
                  size_t bytes_remaining)
{
    varint32_t  *varint32 = (varint32_t *) user_data;
    uint32_t  *value;

    PUSH_DEBUG_MSG("%s: Activating with %zu bytes.\n",
                   push_talloc_get_name(varint32),
                   bytes_remaining);

    /*
     * Our input is the integer that we should write out.
     */

    value = (uint32_t *) result;

    /*
     * If there are enough bytes in the output buffer to encode any
     * varint32, go ahead and do it.
     */

    if (bytes_remaining >= PUSH_PROTOBUF_MAX_VARINT32_LENGTH)
    {
        size_t  encoded_length;

        PUSH_DEBUG_MSG("%s: Encoding directly into output buffer.\n",
                       push_talloc_get_name(varint32));

        encoded_length = varint32_fast_write(buf, *value);
        buf += encoded_length;
        bytes_remaining -= encoded_length;

        PUSH_DEBUG_MSG("%s: Wrote %zu bytes.\n",
                       push_talloc_get_name(varint32),
                       encoded_length);

        push_continuation_call(varint32->callback.success,
                               value,
                               buf, bytes_remaining);

        return;
    }

    /*
     * Otherwise, encode into our own buffer.
     */

    PUSH_DEBUG_MSG("%s: Encoding into temporary buffer.\n",
                   push_talloc_get_name(varint32));

    varint32->src = value;
    varint32->encoded_length =
        varint32_fast_write(varint32->encoded, *value);
    varint32->bytes_written = 0;

    PUSH_DEBUG_MSG("%s: Encoded length is %zu bytes.\n",
                   push_talloc_get_name(varint32),
                   varint32->encoded_length);

    if (bytes_remaining == 0)
    {
        /*
         * If we don't get an output buffer when we're activated,
         * return an incomplete and wait for one.
         */

        push_continuation_call(varint32->callback.incomplete,
                               &varint32->cont);

        return;

    } else {
        /*
         * Otherwise let the continue continuation go ahead and
         * process this output buffer.
         */

        varint32_continue(user_data, buf, bytes_remaining);
        return;
    }
}


push_callback_t *
push_protobuf_write_varint32_new(const char *name,
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

    push_continuation_set(&varint32->cont,
                          varint32_continue,
                          varint32);

    return &varint32->callback;
}


/**
 * Parse a varint into a size_t.  We can only use the varint32 parser
 * if size_t is the same size as uint32_t.
 */

#if SIZE_MAX == UINT32_MAX

push_callback_t *
push_protobuf_write_varint_size_new(const char *name,
                                    void *parent,
                                    push_parser_t *parser)
{
    return push_protobuf_write_varint32_new(name, parent, parser);
}

#endif
