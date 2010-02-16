/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <hwm-buffer.h>

#include <push/basics.h>
#include <push/primitives.h>


/**
 * The push_callback_t subclass that defines an HWM-string callback.
 */

typedef struct _hwm_string
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * A pointer to the HWM buffer that we'll put the string into.
     */

    hwm_buffer_t  *buf;

    /**
     * The number of bytes left to add to the HWM buffer.
     */

    size_t  bytes_left;

} hwm_string_t;


static push_error_code_t
hwm_string_activate(push_parser_t *parser,
                    push_callback_t *pcallback,
                    void *input)
{
    hwm_string_t  *callback = (hwm_string_t *) pcallback;
    size_t  *input_size = (size_t *) input;

    PUSH_DEBUG_MSG("hwm-string: Activating.  Will read %zu bytes.\n",
                   *input_size);

    /*
     * Initialize the callback's fields.
     */

    callback->bytes_left = *input_size;

    if (!hwm_buffer_clear(callback->buf))
    {
        PUSH_DEBUG_MSG("hwm-string: Cannot clear HWM buffer.\n");
        return PUSH_MEMORY_ERROR;
    }

    /*
     * Since we know in advance how big the string will need to be,
     * preallocate enough space for it.  Include an extra byte for the
     * NUL terminator.
     */

    if (hwm_buffer_ensure_size(callback->buf, (*input_size) + 1))
    {
        PUSH_DEBUG_MSG("hwm-string: Successfully allocated %zu bytes.\n",
                       (*input_size) + 1);

        return PUSH_SUCCESS;

    } else {
        PUSH_DEBUG_MSG("hwm-string: Could not allocate %zu bytes.\n",
                       (*input_size) + 1);

        return PUSH_MEMORY_ERROR;
    }
}


static ssize_t
hwm_string_process_bytes(push_parser_t *parser,
                         push_callback_t *pcallback,
                         const void *vbuf,
                         size_t bytes_available)
{
    hwm_string_t  *callback = (hwm_string_t *) pcallback;
    size_t  bytes_to_copy;

    /*
     * EOF is a parse error if we haven't read in all of the string
     * yet.
     */

    if (bytes_available == 0)
    {
        if (callback->bytes_left == 0)
        {
            void  *buf;

            /*
             * In most cases, we'll have already returned PUSH_SUCCESS
             * when we actually read in the data, but we'll find
             * ourselves here if we're asked to read in a 0-byte
             * string that occurs right at the end of the stream.
             */

            PUSH_DEBUG_MSG("hwm-string: EOF found at end of string.  "
                           "Parse successful.\n");

            /*
             * Get a pointer to the HWM buffer's contents.
             */

            buf = hwm_buffer_writable_mem(callback->buf, void);
            if (buf == NULL)
            {
                PUSH_DEBUG_MSG("hwm-string: Cannot get pointer to buffer.\n");
                return PUSH_MEMORY_ERROR;
            }

            /*
             * Our result is a pointer to the HWM buffer's contents.
             */

            callback->base.result = buf;
            return PUSH_SUCCESS;

        } else {
            PUSH_DEBUG_MSG("hwm-string: EOF found before end of string.  "
                           "Parse fails.\n");
            return PUSH_PARSE_ERROR;
        }
    }

    /*
     * Make sure we don't copy more data than is available, or more
     * data than we need.
     */

    bytes_to_copy =
        (bytes_available < callback->bytes_left)?
        bytes_available:
        callback->bytes_left;

    PUSH_DEBUG_MSG("hwm-string: Copying %zu bytes into buffer.\n",
                   bytes_to_copy);

    /*
     * Append this chunk of data to the buffer.
     */

    if (!hwm_buffer_append_mem(callback->buf, vbuf, bytes_to_copy))
    {
        PUSH_DEBUG_MSG("hwm-string: Copying failed.\n");
        return PUSH_MEMORY_ERROR;
    }

    /*
     * If that's the end of the string, append a NUL terminator and
     * return a success code.
     */

    callback->bytes_left -= bytes_to_copy;
    bytes_available -= bytes_to_copy;

    if (callback->bytes_left == 0)
    {
        uint8_t  *buf;

        PUSH_DEBUG_MSG("hwm-string: Copying finished.  Appending "
                       "NUL terminator.\n");

        /*
         * Get a pointer to the HWM buffer's contents.
         */

        buf = hwm_buffer_writable_mem(callback->buf, uint8_t);
        if (buf == NULL)
        {
            PUSH_DEBUG_MSG("hwm-string: Cannot get pointer to buffer.\n");
            return PUSH_MEMORY_ERROR;
        }

        /*
         * Tack on a NUL terminator.
         */

        buf[callback->buf->current_size] = '\0';
        callback->buf->current_size++;

        /*
         * Our result is the pointer to the buffer contents.
         */

        callback->base.result = buf;
        return bytes_available;
    }

    /*
     * If there's more string to copy, return an incomplete code.
     */

    return PUSH_INCOMPLETE;
}


push_callback_t *
push_hwm_string_new(hwm_buffer_t *buf)
{
    hwm_string_t  *callback = (hwm_string_t *)
        malloc(sizeof(hwm_string_t));

    if (callback == NULL)
        return NULL;

    push_callback_init(&callback->base,
                       hwm_string_activate,
                       hwm_string_process_bytes,
                       NULL);

    callback->buf = buf;
    callback->bytes_left = 0;

    return &callback->base;
}
