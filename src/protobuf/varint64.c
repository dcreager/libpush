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

#include <push.h>
#include <push/protobuf.h>


static ssize_t
varint64_process_bytes(push_parser_t *parser,
                       push_callback_t *pcallback,
                       const void *vbuf,
                       size_t bytes_available)
{
    push_protobuf_varint64_t  *callback =
        (push_protobuf_varint64_t *) pcallback;

    uint8_t  *buf = (uint8_t *) vbuf;

    PUSH_DEBUG_MSG("varint64: Processing %zu bytes at %p\n",
                   bytes_available, vbuf);

    /*
     * If there are enough bytes to read a maximum-length varint, or
     * if the last byte in the buffer would end a varint, then we can
     * use the “fast path”, and read each byte unchecked.
     */

    if ((callback->bytes_processed == 0) &&
        ((bytes_available >= PUSH_PROTOBUF_MAX_VARINT_LENGTH) ||
         (buf[bytes_available-1] < 0x80)))
    {
        uint32_t  part0 = 0;
        uint32_t  part1 = 0;
        uint32_t  part2 = 0;
        uint64_t  result;
        uint8_t  *ptr = buf;
        uint8_t  b;

        PUSH_DEBUG_MSG("varint64: Using fast path\n");

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

        PUSH_DEBUG_MSG("varint64: More than %u bytes in value.\n",
                       PUSH_PROTOBUF_MAX_VARINT_LENGTH);
        return PUSH_PARSE_ERROR;

      done:
        result =
            (((uint64_t) part0)      ) |
            (((uint64_t) part1) << 28) |
            (((uint64_t) part2) << 56);

        PUSH_DEBUG_MSG("varint64: Read value %"PRIu64", using %zd bytes\n",
                       result, (ptr - buf));
        bytes_available -= (ptr - buf);

        callback->value = result;
        callback->bytes_processed = 0;
        push_parser_set_callback(parser, callback->base.next_callback);

        return bytes_available;

    } else {
        /*
         * Slow path: we don't know if there are enough bytes in the
         * buffer, so each read must be checked.
         */

        uint8_t  shift = 7 * callback->bytes_processed;

        PUSH_DEBUG_MSG("varint64: Using slow path\n");

        while (bytes_available > 0)
        {
            if (callback->bytes_processed > PUSH_PROTOBUF_MAX_VARINT_LENGTH)
            {
                PUSH_DEBUG_MSG("varint64: More than %u bytes in value.\n",
                               PUSH_PROTOBUF_MAX_VARINT_LENGTH);
                return PUSH_PARSE_ERROR;
            }

            PUSH_DEBUG_MSG("varint64: Reading byte %zu, shifting by %"PRIu8"\n",
                           callback->bytes_processed, shift);

            PUSH_DEBUG_MSG("varint64:   byte = 0x%02"PRIx8"\n", *buf);

            callback->value |= ((uint64_t) (*buf & 0x7F)) << shift;
            shift += 7;

            callback->bytes_processed++;
            bytes_available--;

            if (*buf < 0x80)
            {
                /*
                 * This byte ends the varint.
                 */

                PUSH_DEBUG_MSG("varint64: Read value %"PRIu64
                               ", using %zu bytes\n",
                               callback->value, callback->bytes_processed);

                callback->bytes_processed = 0;
                push_parser_set_callback(parser, callback->base.next_callback);
                return bytes_available;
            }

            buf++;
        }

        return bytes_available;
    }
}


static push_error_code_t
varint64_eof(push_parser_t *parser,
             push_callback_t *pcallback)
{
    push_protobuf_varint64_t  *callback =
        (push_protobuf_varint64_t *) pcallback;

    /*
     * If we've processed part of a varint, but not all of it, then
     * EOF isn't allowed.
     */

    if (callback->bytes_processed > 0)
    {
        PUSH_DEBUG_MSG("Reached EOF in middle of varint.  Parse fails.\n");
        return PUSH_PARSE_ERROR;
    }

    /*
     * Otherwise, the EOF occurred instead of a varint.  The
     * eof_allowed flag lets us know whether that's allowed.
     */

    return callback->eof_allowed?
        PUSH_SUCCESS:
        PUSH_PARSE_ERROR;
}


push_protobuf_varint64_t *
push_protobuf_varint64_new(push_callback_t *next_callback,
                           bool eof_allowed)
{
    push_protobuf_varint64_t  *result =
        (push_protobuf_varint64_t *) malloc(sizeof(push_protobuf_varint64_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       1,
                       PUSH_PROTOBUF_MAX_VARINT_LENGTH,
                       varint64_process_bytes,
                       varint64_eof,
                       NULL,
                       next_callback);

    result->bytes_processed = 0; 
    result->value = 0;
    result->eof_allowed = eof_allowed;

    return result;
}