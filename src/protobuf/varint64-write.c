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
 * The user data struct for a varint64 callback.
 */

typedef struct _varint64
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

    uint64_t  *src;

    /**
     * A buffer that stores the encoded value.  This isn't always
     * filled in; if we're activated with a buffer that's large enough
     * to hold any varint64, we just encode directly into that buffer.
     * Otherwise, we encode into here, and then copy into the output
     * buffer as appropriate.
     */

    uint8_t  encoded[PUSH_PROTOBUF_MAX_VARINT_LENGTH];

    /**
     * The length of the encoded value.
     */

    size_t  encoded_length;

    /**
     * The number of bytes that have already been copied from encoded
     * into an output buffer.
     */

    size_t  bytes_written;

} varint64_t;


/**
 * Quickly encode a varint64 integer.  You must ensure that dest has
 * at least PUSH_PROTOBUF_MAX_VARINT_LENGTH bytes available.
 */

static size_t
varint64_fast_write(uint8_t *dest, uint64_t src)
{
    /*
     * Fast path: We have enough bytes left in the buffer to guarantee
     * that this write won't cross the end, so we can skip the checks.
     */

    /*
     * Splitting into 32-bit pieces gives better performance on 32-bit
     * processors.
     */

    uint32_t  part0 = (uint32_t) (src      );
    uint32_t  part1 = (uint32_t) (src >> 28);
    uint32_t  part2 = (uint32_t) (src >> 56);

    size_t  size;

    /*
     * Here we can't really optimize for small numbers, since the
     * value is split into three parts.  Cheking for numbers < 128,
     * for instance, would require three comparisons, since you'd have
     * to make sure part1 and part2 are zero.  However, if the caller
     * is using 64-bit integers, it is likely that they expect the
     * numbers to often be very large, so we probably don't want to
     * optimize for small numbers anyway.  Thus, we end up with a
     * hardcoded binary search tree...
     */

    if (part2 == 0)
    {
        if (part1 == 0)
        {
            if (part0 < (1 << 14))
            {
                if (part0 < (1 << 7))
                {
                    size = 1; goto size1;
                } else {
                    size = 2; goto size2;
                }
            } else {
                if (part0 < (1 << 21))
                {
                    size = 3; goto size3;
                } else {
                    size = 4; goto size4;
                }
            }
        } else {
            if (part1 < (1 << 14))
            {
                if (part1 < (1 << 7))
                {
                    size = 5; goto size5;
                } else {
                    size = 6; goto size6;
                }
            } else {
                if (part1 < (1 << 21))
                {
                    size = 7; goto size7;
                } else {
                    size = 8; goto size8;
                }
            }
        }
    } else {
        if (part2 < (1 << 7))
        {
            size = 9; goto size9;
        } else {
            size = 10; goto size10;
        }
    }

  size10: dest[9] = (uint8_t) ((part2 >>  7) | 0x80);
  size9 : dest[8] = (uint8_t) ((part2      ) | 0x80);
  size8 : dest[7] = (uint8_t) ((part1 >> 21) | 0x80);
  size7 : dest[6] = (uint8_t) ((part1 >> 14) | 0x80);
  size6 : dest[5] = (uint8_t) ((part1 >>  7) | 0x80);
  size5 : dest[4] = (uint8_t) ((part1      ) | 0x80);
  size4 : dest[3] = (uint8_t) ((part0 >> 21) | 0x80);
  size3 : dest[2] = (uint8_t) ((part0 >> 14) | 0x80);
  size2 : dest[1] = (uint8_t) ((part0 >>  7) | 0x80);
  size1 : dest[0] = (uint8_t) ((part0      ) | 0x80);

    dest[size-1] &= 0x7F;
    return size;
}


static void
varint64_continue(void *user_data,
                  void *buf,
                  size_t bytes_remaining)
{
    varint64_t  *varint64 = (varint64_t *) user_data;
    size_t  bytes_to_copy;

    /*
     * Hopefully we can copy over everything, but there might not be
     * enough space.
     */

    bytes_to_copy =
        varint64->encoded_length - varint64->bytes_written;

    if (bytes_remaining < bytes_to_copy)
        bytes_to_copy = bytes_remaining;

    /*
     * Copy the encoded value into the output buffer.
     */

    PUSH_DEBUG_MSG("%s: Copying %zu bytes to output buffer.\n",
                   push_talloc_get_name(varint64),
                   bytes_to_copy);

    memcpy(buf, varint64->encoded + varint64->bytes_written,
           bytes_to_copy);
    buf += bytes_to_copy;
    bytes_remaining -= bytes_to_copy;
    varint64->bytes_written += bytes_to_copy;

    /*
     * If that's everything, fire our success continuation; otherwise,
     * fire the incomplete continuation.
     */

    if (varint64->bytes_written == varint64->encoded_length)
    {
        push_continuation_call(varint64->callback.success,
                               varint64->src,
                               buf, bytes_remaining);

        return;
    } else {
        PUSH_DEBUG_MSG("%s: %zu bytes remaining.\n",
                       push_talloc_get_name(varint64),
                       varint64->encoded_length -
                       varint64->bytes_written);

        push_continuation_call(varint64->callback.incomplete,
                               &varint64->cont);

        return;
    }
}


static void
varint64_activate(void *user_data,
                  void *result,
                  void *buf,
                  size_t bytes_remaining)
{
    varint64_t  *varint64 = (varint64_t *) user_data;
    uint64_t  *value;

    PUSH_DEBUG_MSG("%s: Activating with %zu bytes.\n",
                   push_talloc_get_name(varint64),
                   bytes_remaining);

    /*
     * Our input is the integer that we should write out.
     */

    value = (uint64_t *) result;

    /*
     * If there are enough bytes in the output buffer to encode any
     * varint64, go ahead and do it.
     */

    if (bytes_remaining >= PUSH_PROTOBUF_MAX_VARINT_LENGTH)
    {
        size_t  encoded_length;

        PUSH_DEBUG_MSG("%s: Encoding directly into output buffer.\n",
                       push_talloc_get_name(varint64));

        encoded_length = varint64_fast_write(buf, *value);
        buf += encoded_length;
        bytes_remaining -= encoded_length;

        PUSH_DEBUG_MSG("%s: Wrote %zu bytes.\n",
                       push_talloc_get_name(varint64),
                       encoded_length);

        push_continuation_call(varint64->callback.success,
                               value,
                               buf, bytes_remaining);

        return;
    }

    /*
     * Otherwise, encode into our own buffer.
     */

    PUSH_DEBUG_MSG("%s: Encoding into temporary buffer.\n",
                   push_talloc_get_name(varint64));

    varint64->src = value;
    varint64->encoded_length =
        varint64_fast_write(varint64->encoded, *value);
    varint64->bytes_written = 0;

    PUSH_DEBUG_MSG("%s: Encoded length is %zu bytes.\n",
                   push_talloc_get_name(varint64),
                   varint64->encoded_length);

    if (bytes_remaining == 0)
    {
        /*
         * If we don't get an output buffer when we're activated,
         * return an incomplete and wait for one.
         */

        push_continuation_call(varint64->callback.incomplete,
                               &varint64->cont);

        return;

    } else {
        /*
         * Otherwise let the continue continuation go ahead and
         * process this output buffer.
         */

        varint64_continue(user_data, buf, bytes_remaining);
        return;
    }
}


push_callback_t *
push_protobuf_write_varint64_new(const char *name,
                                 void *parent,
                                 push_parser_t *parser)
{
    varint64_t  *varint64 = push_talloc(parent, varint64_t);

    if (varint64 == NULL)
        return NULL;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL) name = "varint64";
    push_talloc_set_name_const(varint64, name);

    push_callback_init(&varint64->callback, parser, varint64,
                       varint64_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&varint64->cont,
                          varint64_continue,
                          varint64);

    return &varint64->callback;
}


/**
 * Parse a varint into a size_t.  We can only use the varint64 parser
 * if size_t is the same size as uint32_t.
 */

#if SIZE_MAX == UINT64_MAX

push_callback_t *
push_protobuf_write_varint_size_new(const char *name,
                                    void *parent,
                                    push_parser_t *parser)
{
    return push_protobuf_write_varint64_new(name, parent, parser);
}

#endif
