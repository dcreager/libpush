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
 * The push_callback_t subclass that defines a second callback.
 */

typedef struct _second
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
     * The value that we copy through the first element of the pair.
     */

    void  *first;

    /**
     * The pair object that we output as our result.
     */

    push_pair_t  result;

} second_t;


static void
second_set_incomplete(void *user_data,
                      push_incomplete_continuation_t *incomplete)
{
    second_t  *second = (second_t *) user_data;
    push_continuation_call(&second->wrapped->set_incomplete,
                           incomplete);
}


static void
second_set_error(void *user_data,
                 push_error_continuation_t *error)
{
    second_t  *second = (second_t *) user_data;
    push_continuation_call(&second->wrapped->set_error,
                           error);
}


static void
second_activate(void *user_data,
                void *result,
                void *buf,
                size_t bytes_remaining)
{
    second_t  *second = (second_t *) user_data;
    push_pair_t  *input = (push_pair_t *) result;

    /*
     * Save the first element of the input pair, so that we can copy
     * it into our result later.
     */

    second->first = input->first;

    /*
     * We activate this callback by passing the second element of the
     * input pair into our wrapped callback.
     */

    PUSH_DEBUG_MSG("%s: Activating wrapped callback.\n",
                   push_talloc_get_name(second));

    push_continuation_call(&second->wrapped->activate,
                           input->second,
                           buf, bytes_remaining);

    return;
}


static void
second_wrapped_success(void *user_data,
                       void *result,
                       void *buf,
                       size_t bytes_remaining)
{
    second_t  *second = (second_t *) user_data;

    /*
     * Create the output pair from this result and our saved value.
     */

    PUSH_DEBUG_MSG("%s: Constructing output pair.\n",
                   push_talloc_get_name(second));

    second->result.first = second->first;
    second->result.second = result;

    push_continuation_call(second->callback.success,
                           &second->result,
                           buf, bytes_remaining);

    return;
}


push_callback_t *
push_second_new(const char *name,
                void *parent,
                push_parser_t *parser,
                push_callback_t *wrapped)
{
    second_t  *second;

    /*
     * If the wrapped callback is NULL, return NULL ourselves.
     */

    if (wrapped == NULL)
        return NULL;

    /*
     * Allocate the user data struct.
     */

    second = push_talloc(parent, second_t);
    if (second == NULL) return NULL;

    /*
     * Make the wrapped callback a child of the new callback.
     */

    push_talloc_steal(second, wrapped);

    /*
     * Fill in the data items.
     */

    second->wrapped = wrapped;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL) name = "second";
    push_talloc_set_name_const(second, name);

    push_callback_init(&second->callback, parser, second,
                       second_activate,
                       NULL,
                       second_set_incomplete,
                       second_set_error);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&second->wrapped_success,
                          second_wrapped_success,
                          second);

    /*
     * The wrapped callback should succeed by calling our
     * wrapped_succeed continuation, so that we can construct the
     * output pair.
     */

    push_continuation_call(&wrapped->set_success,
                           &second->wrapped_success);

    return &second->callback;
}
