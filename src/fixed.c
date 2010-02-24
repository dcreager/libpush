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
#include <push/talloc.h>


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
                   push_talloc_get_name(fixed),
                   bytes_remaining, buf);

    if (bytes_remaining < fixed->size)
    {
        PUSH_DEBUG_MSG("%s: Need more than %zu bytes to read data.\n",
                       push_talloc_get_name(fixed),
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
                void *parent,
                push_parser_t *parser,
                size_t size)
{
    fixed_t  *fixed = push_talloc(parent, fixed_t);

    if (fixed == NULL)
        return NULL;

    /*
     * Fill in the data items.
     */

    fixed->size = size;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL) name = "fixed";
    push_talloc_set_name_const(fixed, name);

    push_callback_init(&fixed->callback, parser, fixed,
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
               void *parent,
               push_parser_t *parser,
               size_t size)
{
    void  *context;
    push_callback_t  *fixed;
    push_callback_t  *min_bytes;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Create the callbacks.
     */

    if (name == NULL) name = "fixed";

    fixed = inner_fixed_new
        (push_talloc_asprintf(context, "%s.inner", name),
         context, parser, size);
    min_bytes = push_min_bytes_new
        (push_talloc_asprintf(context, "%s.min-bytes", name),
         context, parser, fixed, size);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (min_bytes == NULL) goto error;

    return min_bytes;

  error:
    /*
     * Before returning, free any objects we created before the error.
     */

    push_talloc_free(context);
    return NULL;
}
