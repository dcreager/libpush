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
 * The push_callback_t subclass that defines a first callback.
 */

typedef struct _first
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
     * The value that we copy through the second element of the pair.
     */

    void  *second;

    /**
     * The pair object that we output as our result.
     */

    push_pair_t  result;

} first_t;


static void
first_activate(void *user_data,
               void *result,
               const void *buf,
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

    PUSH_DEBUG_MSG("first: Activating wrapped callback.\n");

    push_continuation_call(&first->wrapped->activate,
                           input->first,
                           buf, bytes_remaining);

    return;
}


static void
first_wrapped_success(void *user_data,
                      void *result,
                      const void *buf,
                      size_t bytes_remaining)
{
    first_t  *first = (first_t *) user_data;

    /*
     * Create the output pair from this result and our saved value.
     */

    first->result.first = result;
    first->result.second = first->second;

    push_continuation_call(first->success,
                           &first->result,
                           buf, bytes_remaining);

    return;
}


push_callback_t *
push_first_new(push_parser_t *parser,
               push_callback_t *wrapped)
{
    first_t  *first = (first_t *) malloc(sizeof(first_t));
    push_callback_t  *callback;

    if (first == NULL)
        return NULL;

    callback = push_callback_new();
    if (callback == NULL)
    {
        free(first);
        return NULL;
    }

    /*
     * Fill in the data items.
     */

    first->wrapped = wrapped;

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&callback->activate,
                          first_activate,
                          first);

    push_continuation_set(&first->wrapped_success,
                          first_wrapped_success,
                          first);

    /*
     * The wrapped callback should succeed by calling our success
     * continuation, which will construct the correct output pair.
     */

    if (wrapped->success != NULL)
        *wrapped->success = &first->wrapped_success;

    /*
     * By default, we call the parser's implementations of the
     * continuations that we call.
     */

    first->success = &parser->success;

    /*
     * Set the pointers for the continuations that we call, so that
     * they can be changed by combinators, if necessary.  We cannot
     * incomplete or fail on our own — only our wrapped callback can.
     */

    callback->success = &first->success;
    callback->incomplete = NULL;
    callback->error = NULL;

    return callback;
}
