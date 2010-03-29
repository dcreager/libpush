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
#include <push/pairs.h>
#include <push/talloc.h>


/**
 * The push_callback_t subclass that defines a first callback.
 */

typedef struct _first
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The success continuation that we have the wrapped callback use.
     * It constructs the output pair from the wrapped callback's
     * result.
     */

    push_success_continuation_t  wrapped_success;

    /**
     * The wrapped callback.
     */

    push_callback_t  *wrapped;

    /**
     * The value that we copy through the second element of the pair.
     */

    void  *second;

    /**
     * The pair object that we output as our result.
     */

    push_pair_t  result;

} first_t;


static void
first_set_incomplete(void *user_data,
                     push_incomplete_continuation_t *incomplete)
{
    first_t  *first = (first_t *) user_data;
    push_continuation_call(&first->wrapped->set_incomplete,
                           incomplete);
}


static void
first_set_error(void *user_data,
                push_error_continuation_t *error)
{
    first_t  *first = (first_t *) user_data;
    push_continuation_call(&first->wrapped->set_error,
                           error);
}


static void
first_activate(void *user_data,
               void *result,
               void *buf,
               size_t bytes_remaining)
{
    first_t  *first = (first_t *) user_data;
    push_pair_t  *input = (push_pair_t *) result;

    /*
     * Save the second element of the input pair, so that we can copy
     * it into our result later.
     */

    first->second = input->second;

    /*
     * We activate this callback by passing the first element of the
     * input pair into our wrapped callback.
     */

    PUSH_DEBUG_MSG("%s: Activating wrapped callback.\n",
                   push_talloc_get_name(first));

    push_continuation_call(&first->wrapped->activate,
                           input->first,
                           buf, bytes_remaining);

    return;
}


static void
first_wrapped_success(void *user_data,
                      void *result,
                      void *buf,
                      size_t bytes_remaining)
{
    first_t  *first = (first_t *) user_data;

    /*
     * Create the output pair from this result and our saved value.
     */

    PUSH_DEBUG_MSG("%s: Constructing output pair.\n",
                   push_talloc_get_name(first));

    first->result.first = result;
    first->result.second = first->second;

    push_continuation_call(first->callback.success,
                           &first->result,
                           buf, bytes_remaining);

    return;
}


push_callback_t *
push_first_new(const char *name,
               void *parent,
               push_parser_t *parser,
               push_callback_t *wrapped)
{
    first_t  *first;

    /*
     * If the wrapped callback is NULL, return NULL ourselves.
     */

    if (wrapped == NULL)
        return NULL;

    /*
     * Allocate the user data struct.
     */

    first = push_talloc(parent, first_t);
    if (first == NULL) return NULL;

    /*
     * Make the wrapped callback a child of the new callback.
     */

    push_talloc_steal(first, wrapped);

    /*
     * Fill in the data items.
     */

    first->wrapped = wrapped;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL) name = "first";
    push_talloc_set_name_const(first, name);

    push_callback_init(&first->callback, parser, first,
                       first_activate,
                       NULL,
                       first_set_incomplete,
                       first_set_error);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&first->wrapped_success,
                          first_wrapped_success,
                          first);

    /*
     * The wrapped callback should succeed by calling our
     * wrapped_succeed continuation, so that we can construct the
     * output pair.
     */

    push_continuation_call(&wrapped->set_success,
                           &first->wrapped_success);

    return &first->callback;
}
