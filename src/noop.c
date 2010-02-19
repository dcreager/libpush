/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>

#include <push/basics.h>
#include <push/primitives.h>


/**
 * The user data struct for a noop callback.
 */

typedef struct _noop
{
    /**
     * The continuation that we'll call on a successful parse.
     */

    push_success_continuation_t  *success;

} noop_t;


static void
noop_activate(void *user_data,
              void *result,
              const void *buf,
              size_t bytes_remaining)
{
    noop_t  *noop = (noop_t *) user_data;

    PUSH_DEBUG_MSG("noop: Activating.\n");

    /*
     * Immediately succeed with the same input value.
     */

    push_continuation_call(noop->success,
                           result,
                           buf,
                           bytes_remaining);

    return;
}


push_callback_t *
push_noop_new(push_parser_t *parser)
{
    noop_t  *noop = (noop_t *) malloc(sizeof(noop_t));
    push_callback_t  *callback;

    if (noop == NULL)
        return NULL;

    callback = push_callback_new();
    if (callback == NULL)
    {
        free(noop);
        return NULL;
    }

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&callback->activate,
                          noop_activate,
                          noop);

    /*
     * By default, we call the parser's implementations of the
     * continuations that we call.
     */

    noop->success = &parser->success;

    /*
     * Set the pointers for the continuations that we call, so that
     * they can be changed by combinators, if necessary.
     */

    callback->success = &noop->success;
    callback->incomplete = NULL;
    callback->error = NULL;

    return callback;
}
