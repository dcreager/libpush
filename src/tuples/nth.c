/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <string.h>

#include <push/basics.h>
#include <push/talloc.h>
#include <push/tuples.h>


/**
 * The push_callback_t subclass that defines an nth callback.
 */

typedef struct _nth
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The success continuation that we have the wrapped callback use.
     * It constructs the output tuple from the wrapped callback's
     * result.
     */

    push_success_continuation_t  wrapped_success;

    /**
     * The wrapped callback.
     */

    push_callback_t  *wrapped;

    /**
     * The element that we apply the wrapped callback to.
     */

    size_t  n;

    /**
     * The input tuple.  We hold onto it so that we can copy over the
     * non-nth elements into the output tuple.
     */

    push_tuple_t  *input;

    /**
     * The tuple object that we output as our result.
     */

    push_tuple_t  *output;

} nth_t;


static void
nth_set_incomplete(void *user_data,
                   push_incomplete_continuation_t *incomplete)
{
    nth_t  *nth = (nth_t *) user_data;
    push_continuation_call(&nth->wrapped->set_incomplete,
                           incomplete);
}


static void
nth_set_error(void *user_data,
              push_error_continuation_t *error)
{
    nth_t  *nth = (nth_t *) user_data;
    push_continuation_call(&nth->wrapped->set_error,
                           error);
}


static void
nth_activate(void *user_data,
             void *result,
             void *buf,
             size_t bytes_remaining)
{
    nth_t  *nth = (nth_t *) user_data;

    /*
     * Save the input tuple, so that we can copy it into our result
     * later.
     */

    nth->input = (push_tuple_t *) result;

    /*
     * Verify that the input tuple has the number of elements that we
     * expect.
     */

    if (nth->input->size != nth->output->size)
    {
        PUSH_DEBUG_MSG("%s: Input has wrong size "
                       "(expected = %zu, actual = %zu).\n",
                       push_talloc_get_name(nth),
                       nth->output->size,
                       nth->input->size);

        push_continuation_call(nth->callback.error,
                               PUSH_MEMORY_ERROR,
                               "Input has wrong size");
        return;
    }

    /*
     * We activate this callback by passing the nth element of the
     * input tuple into our wrapped callback.
     */

    PUSH_DEBUG_MSG("%s: Activating wrapped callback.\n",
                   push_talloc_get_name(nth));

    push_continuation_call(&nth->wrapped->activate,
                           nth->input->elements[nth->n],
                           buf, bytes_remaining);

    return;
}


static void
nth_wrapped_success(void *user_data,
                    void *result,
                    void *buf,
                    size_t bytes_remaining)
{
    nth_t  *nth = (nth_t *) user_data;

    /*
     * Create the output tuple from this result and our saved value.
     */

    PUSH_DEBUG_MSG("%s: Constructing output tuple.\n",
                   push_talloc_get_name(nth));

    memcpy(nth->output->elements, nth->input->elements,
           nth->output->size * sizeof(void *));
    nth->output->elements[nth->n] = result;

    push_continuation_call(nth->callback.success,
                           nth->output,
                           buf, bytes_remaining);

    return;
}


push_callback_t *
push_nth_new(const char *name,
             void *parent,
             push_parser_t *parser,
             push_callback_t *wrapped,
             size_t n,
             size_t tuple_size)
{
    nth_t  *nth;

    /*
     * If the wrapped callback is NULL, return NULL ourselves.
     */

    if (wrapped == NULL)
        return NULL;

    /*
     * Allocate the user data struct.
     */

    nth = push_talloc(parent, nth_t);
    if (nth == NULL) return NULL;

    /*
     * Try to allocate the output pair.
     */

    nth->output = push_tuple_new(nth, tuple_size);
    if (nth->output == NULL)
    {
        push_talloc_free(nth);
        return NULL;
    }

    /*
     * Make the wrapped callback a child of the new callback.
     */

    push_talloc_steal(nth, wrapped);

    /*
     * Fill in the data items.
     */

    nth->wrapped = wrapped;
    nth->n = n;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL) name = "nth";
    push_talloc_set_name_const(nth, name);

    push_callback_init(&nth->callback, parser, nth,
                       nth_activate,
                       NULL,
                       nth_set_incomplete,
                       nth_set_error);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&nth->wrapped_success,
                          nth_wrapped_success,
                          nth);

    /*
     * The wrapped callback should succeed by calling our
     * wrapped_succeed continuation, so that we can construct the
     * output tuple.
     */

    push_continuation_call(&wrapped->set_success,
                           &nth->wrapped_success);

    return &nth->callback;
}


push_callback_t *
push_first_new(const char *name,
               void *parent,
               push_parser_t *parser,
               push_callback_t *wrapped)
{
    if (name == NULL) name = "first";

    return push_nth_new(name, parent, parser, wrapped, 0, 2);
}


push_callback_t *
push_second_new(const char *name,
                void *parent,
                push_parser_t *parser,
                push_callback_t *wrapped)
{
    if (name == NULL) name = "second";

    return push_nth_new(name, parent, parser, wrapped, 1, 2);
}
