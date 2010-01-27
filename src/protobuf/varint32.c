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
#include <stdbool.h>
#include <stdint.h>

#include <push.h>
#include <push/protobuf.h>


static push_error_code_t
varint32_activate(push_parser_t *parser,
                  push_callback_t *pcallback,
                  void *input)
{
    push_protobuf_varint32_t  *callback =
        (push_protobuf_varint32_t *) pcallback;

    /*
     * Initialize the fields that store the current value.
     */

    callback->bytes_processed = 0;
    callback->value = 0;

    return PUSH_SUCCESS;
}


static ssize_t
varint32_process_bytes(push_parser_t *parser,
                       push_callback_t *pcallback,
                       const void *vbuf,
                       size_t bytes_available)
{
    push_protobuf_varint32_t  *callback =
        (push_protobuf_varint32_t *) pcallback;

    uint8_t  *buf = (uint8_t *) vbuf;

    PUSH_DEBUG_MSG("varint32: Processing %zu bytes at %p\n",
                   bytes_available, vbuf);

    /*
     * If we don't have any data to process, that's a parse error.
     */

    if (bytes_available == 0)
        return PUSH_PARSE_ERROR;

    /*
     * In all cases below, once we've parsed the varint, we store it
     * into the callback's value field.  The callback constructor will
     * already have pointed the result pointer at this field, so the
     * value will be correctly passed into any downstream callbacks.
     */

    /*
     * Special case for single-byte varints — super common, especially
     * for tag numbers.
     */

    if ((callback->bytes_processed == 0) &&
        (*buf < 0x80))
    {
        PUSH_DEBUG_MSG("varint32: Using super-fast path\n");

        callback->value = *buf;

        PUSH_DEBUG_MSG("varint32: Read value %"PRIu32", using 1 byte\n",
                       callback->value);

        return bytes_available - 1;
    }

    /*
     * If there are enough bytes to read a maximum-length varint, or
     * if the last byte in the buffer would end a varint, then we can
     * use the “fast path”, and read each byte unchecked.
     */

    if ((callback->bytes_processed == 0) &&
        ((bytes_available >= PUSH_PROTOBUF_MAX_VARINT_LENGTH) ||
         (buf[bytes_available-1] < 0x80)))
    {
        uint32_t  result;
        uint8_t  *ptr = buf;
        uint8_t  b;
        int  i;

        PUSH_DEBUG_MSG("varint32: Using fast path\n");

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

        PUSH_DEBUG_MSG("varint32: More than %u bytes in value.\n",
                       PUSH_PROTOBUF_MAX_VARINT_LENGTH);
        return PUSH_PARSE_ERROR;

      done:
        PUSH_DEBUG_MSG("varint32: Read value %"PRIu32", using %zd bytes\n",
                       result, (ptr - buf));
        bytes_available -= (ptr - buf);

        callback->value = result;

        return bytes_available;

    } else {
        /*
         * Slow path: we don't know if there are enough bytes in the
         * buffer, so each read must be checked.
         */

        uint8_t  shift = 7 * callback->bytes_processed;

        PUSH_DEBUG_MSG("varint32: Using slow path\n");

        while (bytes_available > 0)
        {
            if (callback->bytes_processed > PUSH_PROTOBUF_MAX_VARINT_LENGTH)
            {
                PUSH_DEBUG_MSG("varint32: More than %u bytes in value.\n",
                               PUSH_PROTOBUF_MAX_VARINT_LENGTH);
                return PUSH_PARSE_ERROR;
            }

            PUSH_DEBUG_MSG("varint32: Reading byte %zu, shifting by %"PRIu8"\n",
                           callback->bytes_processed, shift);

            PUSH_DEBUG_MSG("varint32:   byte = 0x%02"PRIx8"\n", *buf);

            callback->value |= (*buf & 0x7F) << shift;
            shift += 7;

            callback->bytes_processed++;
            bytes_available--;

            if (*buf < 0x80)
            {
                /*
                 * This byte ends the varint.
                 */

                PUSH_DEBUG_MSG("varint32: Read value %"PRIu32
                               ", using %zu bytes\n",
                               callback->value, callback->bytes_processed);

                return bytes_available;
            }

            buf++;
        }

        /*
         * We ran out of data without processing a full varint, so
         * return the incomplete code.
         */

        return PUSH_INCOMPLETE;
    }
}


push_protobuf_varint32_t *
push_protobuf_varint32_new()
{
    push_protobuf_varint32_t  *result =
        (push_protobuf_varint32_t *) malloc(sizeof(push_protobuf_varint32_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       varint32_activate,
                       varint32_process_bytes,
                       NULL);

    result->bytes_processed = 0; 
    result->value = 0;
    result->base.result = &result->value;

    return result;
}
