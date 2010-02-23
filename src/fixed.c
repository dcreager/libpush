/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <push/basics.h>
#include <push/combinators.h>
#include <push/primitives.h>


/**
 * The user data struct for a fixed callback.
 */

typedef struct _fixed
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The continue continuation that will resume the fixed parser.
     */

    push_continue_continuation_t  cont;

    /**
     * The size of the buffer.
     */

    size_t  size;

} fixed_t;


static void
fixed_continue(void *user_data,
               const void *buf,
               size_t bytes_remaining)
{
    fixed_t  *fixed = (fixed_t *) user_data;

    PUSH_DEBUG_MSG("%s: Processing %zu bytes at %p.\n",
                   fixed->callback.name,
                   bytes_remaining, buf);

    if (bytes_remaining < fixed->size)
    {
        PUSH_DEBUG_MSG("%s: Need more than %zu bytes to read data.\n",
                       fixed->callback.name,
                       bytes_remaining);

        push_continuation_call(fixed->callback.error,
                               PUSH_PARSE_ERROR,
                               "Need more bytes to read data");

        return;
    } else {
        void  *value = (void *) buf;

        buf += fixed->size;
        bytes_remaining -= fixed->size;

        push_continuation_call(fixed->callback.success,
                               value,
                               buf, bytes_remaining);

        return;
    }
}


static void
fixed_activate(void *user_data,
                 void *result,
                 const void *buf,
                 size_t bytes_remaining)
{
    fixed_t  *fixed = (fixed_t *) user_data;

    if (bytes_remaining == 0)
    {
        /*
         * If we don't get any data when we're activated, return an
         * incomplete and wait for some data.
         */

        push_continuation_call(fixed->callback.incomplete,
                               &fixed->cont);

        return;

    } else {
        /*
         * Otherwise let the continue continuation go ahead and
         * process this chunk of data.
         */

        fixed_continue(user_data, buf, bytes_remaining);
        return;
    }
}


static push_callback_t *
inner_fixed_new(const char *name,
                push_parser_t *parser,
                size_t size)
{
    fixed_t  *fixed = (fixed_t *) malloc(sizeof(fixed_t));

    if (fixed == NULL)
        return NULL;

    /*
     * Fill in the data items.
     */

    fixed->size = size;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL)
        name = "fixed";

    push_callback_init(name,
                       &fixed->callback, parser, fixed,
                       fixed_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&fixed->cont,
                          fixed_continue,
                          fixed);

    return &fixed->callback;
}


push_callback_t *
push_fixed_new(const char *name,
               push_parser_t *parser,
               size_t size)
{
    const char  *fixed_name;
    const char  *min_bytes_name;

    push_callback_t  *fixed = NULL;
    push_callback_t  *min_bytes = NULL;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "fixed";

    fixed_name = push_string_concat(name, ".inner");
    if (fixed_name == NULL) return NULL;

    min_bytes_name = push_string_concat(name, ".min-bytes");
    if (min_bytes_name == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    fixed = inner_fixed_new(fixed_name, parser, size);
    if (fixed == NULL)
        return NULL;

    min_bytes = push_min_bytes_new(min_bytes_name,
                                   parser, fixed, size);
    if (min_bytes == NULL)
    {
        push_callback_free(fixed);
        return NULL;
    }

    return min_bytes;
}
