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
 * The user data struct for an HWM-string callback.
 */

typedef struct _hwm_string
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The continue continuation for this callback.
     */

    push_continue_continuation_t  cont;

    /**
     * A pointer to the HWM buffer that we'll put the string into.
     */

    hwm_buffer_t  *buf;

    /**
     * The number of bytes left to add to the HWM buffer.
     */

    size_t  bytes_left;

} hwm_string_t;


static void
hwm_string_continue(void *user_data,
                    const void *buf,
                    size_t bytes_remaining)
{
    hwm_string_t  *hwm_string = (hwm_string_t *) user_data;
    size_t  bytes_to_copy;

    /*
     * EOF is a parse error if we haven't read in all of the string
     * yet.
     */

    if (bytes_remaining == 0)
    {
        if (hwm_string->bytes_left == 0)
        {
            void  *str;

            /*
             * In most cases, we'll have already returned PUSH_SUCCESS
             * when we actually read in the data, but we'll find
             * ourselves here if we're asked to read in a 0-byte
             * string that occurs right at the end of the stream.
             */

            PUSH_DEBUG_MSG("%s: EOF found at end of string.  "
                           "Parse successful.\n",
                           hwm_string->callback.name);

            /*
             * Get a pointer to the HWM buffer's contents.
             */

            str = hwm_buffer_writable_mem(hwm_string->buf, void);
            if (str == NULL)
            {
                PUSH_DEBUG_MSG("%s: Cannot get pointer to buffer.\n",
                               hwm_string->callback.name);

                push_continuation_call(hwm_string->callback.error,
                                       PUSH_MEMORY_ERROR,
                                       "Cannot get pointer to buffer");

                return;
            }

            /*
             * Our result is a pointer to the HWM buffer's contents.
             */

            push_continuation_call(hwm_string->callback.success,
                                   str,
                                   buf, bytes_remaining);

            return;

        } else {
            PUSH_DEBUG_MSG("%s: EOF found before end of string.  "
                           "Parse fails.\n",
                           hwm_string->callback.name);

            push_continuation_call(hwm_string->callback.error,
                                   PUSH_PARSE_ERROR,
                                   "EOF found before end of string");

            return;
        }
    }

    /*
     * Make sure we don't copy more data than is available, or more
     * data than we need.
     */

    bytes_to_copy =
        (bytes_remaining < hwm_string->bytes_left)?
        bytes_remaining:
        hwm_string->bytes_left;

    PUSH_DEBUG_MSG("%s: Copying %zu bytes into buffer.\n",
                   hwm_string->callback.name,
                   bytes_to_copy);

    /*
     * Append this chunk of data to the buffer.
     */

    if (!hwm_buffer_append_mem(hwm_string->buf, buf, bytes_to_copy))
    {
        PUSH_DEBUG_MSG("%s: Copying failed.\n",
                       hwm_string->callback.name);

        push_continuation_call(hwm_string->callback.error,
                               PUSH_MEMORY_ERROR,
                               "Copying failed");

        return;
    }

    /*
     * If that's the end of the string, append a NUL terminator and
     * return a success code.
     */

    hwm_string->bytes_left -= bytes_to_copy;
    buf += bytes_to_copy;
    bytes_remaining -= bytes_to_copy;

    if (hwm_string->bytes_left == 0)
    {
        uint8_t  *str;

        PUSH_DEBUG_MSG("%s: Copying finished.  Appending "
                       "NUL terminator.\n",
                       hwm_string->callback.name);

        /*
         * Get a pointer to the HWM buffer's contents.
         */

        str = hwm_buffer_writable_mem(hwm_string->buf, uint8_t);
        if (str == NULL)
        {
            PUSH_DEBUG_MSG("%s: Cannot get pointer to buffer.\n",
                           hwm_string->callback.name);

            push_continuation_call(hwm_string->callback.error,
                                   PUSH_MEMORY_ERROR,
                                   "Cannot get pointer to buffer");

            return;
        }

        /*
         * Tack on a NUL terminator.
         */

        str[hwm_string->buf->current_size] = '\0';
        hwm_string->buf->current_size++;

        /*
         * Our result is the pointer to the buffer contents.
         */


        push_continuation_call(hwm_string->callback.success,
                               str,
                               buf, bytes_remaining);

        return;
    }

    /*
     * If there's more string to copy, return an incomplete code.
     */

    push_continuation_call(hwm_string->callback.incomplete,
                           &hwm_string->cont);
}


static void
hwm_string_activate(void *user_data,
                    void *result,
                    const void *buf,
                    size_t bytes_remaining)
{
    hwm_string_t  *hwm_string = (hwm_string_t *) user_data;
    size_t  *input_size = (size_t *) result;

    PUSH_DEBUG_MSG("%s: Activating.  Will read %zu bytes.\n",
                   hwm_string->callback.name,
                   *input_size);

    /*
     * Initialize the callback's fields.
     */

    hwm_string->bytes_left = *input_size;

    if (!hwm_buffer_clear(hwm_string->buf))
    {
        PUSH_DEBUG_MSG("%s: Cannot clear HWM buffer.\n",
                       hwm_string->callback.name);

        push_continuation_call(hwm_string->callback.error,
                               PUSH_MEMORY_ERROR,
                               "Cannot clear HWM buffer");

        return;
    }

    /*
     * Since we know in advance how big the string will need to be,
     * preallocate enough space for it.  Include an extra byte for the
     * NUL terminator.
     */

    if (hwm_buffer_ensure_size(hwm_string->buf, (*input_size) + 1))
    {
        PUSH_DEBUG_MSG("%s: Successfully allocated %zu bytes.\n",
                       hwm_string->callback.name,
                       (*input_size) + 1);
    } else {
        PUSH_DEBUG_MSG("%s: Could not allocate %zu bytes.\n",
                       hwm_string->callback.name,
                       (*input_size) + 1);

        push_continuation_call(hwm_string->callback.error,
                               PUSH_MEMORY_ERROR,
                               "Could not allocate HWM buffer");

        return;
    }


    if (bytes_remaining == 0)
    {
        /*
         * If we don't get any data when we're activated, return an
         * incomplete and wait for some data.
         */

        push_continuation_call(hwm_string->callback.incomplete,
                               &hwm_string->cont);

        return;

    } else {
        /*
         * Otherwise let the continue continuation go ahead and
         * process this chunk of data.
         */

        hwm_string_continue(user_data, buf, bytes_remaining);
        return;
    }
}


push_callback_t *
push_hwm_string_new(const char *name,
                    push_parser_t *parser,
                    hwm_buffer_t *buf)
{
    hwm_string_t  *hwm_string =
        (hwm_string_t *) malloc(sizeof(hwm_string_t));

    if (hwm_string == NULL)
        return NULL;

    /*
     * Fill in the data items.
     */

    hwm_string->buf = buf;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL)
        name = "hwm-string";

    push_callback_init(name,
                       &hwm_string->callback, parser, hwm_string,
                       hwm_string_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&hwm_string->cont,
                          hwm_string_continue,
                          hwm_string);

    return &hwm_string->callback;
}
