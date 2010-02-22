/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stddef.h>
#include <stdlib.h>

#include <push/basics.h>
#include <push/combinators.h>


/**
 * The user data struct for a compose callback.
 */

typedef struct _compose
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The first wrapped callback in the composition.
     */

    push_callback_t  *first;

    /**
     * The second wrapped callback in the composition.
     */

    push_callback_t  *second;

} compose_t;


static void
compose_activate(void *user_data,
                 void *result,
                 const void *buf,
                 size_t bytes_remaining)
{
    compose_t  *compose = (compose_t *) user_data;

    /*
     * The first callback should succeed by activating the second.
     * The second callback should succeed with our overall success
     * continuation.  Both callbacks should incomplete and error with
     * our overall incomplete and error continuations.
     */

    PUSH_DEBUG_MSG("compose: Activating.\n");

    compose->first->success = &compose->second->activate;
    compose->first->incomplete = compose->callback.incomplete;
    compose->first->error = compose->callback.error;

    compose->second->success = compose->callback.success;
    compose->second->incomplete = compose->callback.incomplete;
    compose->second->error = compose->callback.error;

    /*
     * Then just pass the input value off to the first callback.
     */

    push_continuation_call(&compose->first->activate,
                           result,
                           buf, bytes_remaining);

    return;
}


push_callback_t *
push_compose_new(push_parser_t *parser,
                 push_callback_t *first,
                 push_callback_t *second)
{
    compose_t  *compose = (compose_t *) malloc(sizeof(compose_t));

    if (compose == NULL)
        return NULL;

    push_callback_init(&compose->callback, parser,
                       compose_activate, compose);

    /*
     * Fill in the data items.
     */

    compose->first = first;
    compose->second = second;

    return &compose->callback;
}
