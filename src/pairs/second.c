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


/**
 * The push_callback_t subclass that defines a second callback.
 */

typedef struct _second
{
    /**
     * The continuation that we'll call on a successful parse.
     */

    push_success_continuation_t  *success;

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
second_activate(void *user_data,
                void *result,
                const void *buf,
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

    PUSH_DEBUG_MSG("second: Activating wrapped callback.\n");

    push_continuation_call(&second->wrapped->activate,
                           input->second,
                           buf, bytes_remaining);

    return;
}


static void
second_wrapped_success(void *user_data,
                       void *result,
                       const void *buf,
                       size_t bytes_remaining)
{
    second_t  *second = (second_t *) user_data;

    /*
     * Create the output pair from this result and our saved value.
     */

    second->result.first = second->first;
    second->result.second = result;

    push_continuation_call(second->success,
                           &second->result,
                           buf, bytes_remaining);

    return;
}


push_callback_t *
push_second_new(push_parser_t *parser,
                push_callback_t *wrapped)
{
    second_t  *second = (second_t *) malloc(sizeof(second_t));
    push_callback_t  *callback;

    if (second == NULL)
        return NULL;

    callback = push_callback_new();
    if (callback == NULL)
    {
        free(second);
        return NULL;
    }

    /*
     * Fill in the data items.
     */

    second->wrapped = wrapped;

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&callback->activate,
                          second_activate,
                          second);

    push_continuation_set(&second->wrapped_success,
                          second_wrapped_success,
                          second);

    /*
     * The wrapped callback should succeed by calling our success
     * continuation, which will construct the correct output pair.
     */

    if (wrapped->success != NULL)
        *wrapped->success = &second->wrapped_success;

    /*
     * By default, we call the parser's implementations of the
     * continuations that we call.
     */

    second->success = &parser->success;

    /*
     * Set the pointers for the continuations that we call, so that
     * they can be changed by combinators, if necessary.  We cannot
     * incomplete or fail on our own — only our wrapped callback can.
     */

    callback->success = &second->success;
    callback->incomplete = NULL;
    callback->error = NULL;

    return callback;
}
