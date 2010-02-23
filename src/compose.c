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
compose_set_success(void *user_data,
                    push_success_continuation_t *success)
{
    compose_t  *compose = (compose_t *) user_data;

    /*
     * The second wrapped callback should succeed using this new
     * continuation.  The first callback still succeeds by activating
     * the second.
     */

    push_continuation_call(&compose->second->set_success,
                           success);
}


static void
compose_set_incomplete(void *user_data,
                       push_incomplete_continuation_t *incomplete)
{
    compose_t  *compose = (compose_t *) user_data;

    /*
     * Both wrapped callbacks should use this new incomplete
     * continuation.
     */

    push_continuation_call(&compose->first->set_incomplete,
                           incomplete);
    push_continuation_call(&compose->second->set_incomplete,
                           incomplete);
}


static void
compose_set_error(void *user_data,
                  push_error_continuation_t *error)
{
    compose_t  *compose = (compose_t *) user_data;

    /*
     * Both wrapped callbacks should use this new error continuation.
     */

    push_continuation_call(&compose->first->set_error,
                           error);
    push_continuation_call(&compose->second->set_error,
                           error);
}


push_callback_t *
push_compose_new(const char *name,
                 push_parser_t *parser,
                 push_callback_t *first,
                 push_callback_t *second)
{
    compose_t  *compose = (compose_t *) malloc(sizeof(compose_t));

    if (compose == NULL)
        return NULL;

    /*
     * Fill in the data items.
     */

    compose->first = first;
    compose->second = second;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL)
        name = "compose";

    push_callback_init(name,
                       &compose->callback, parser, compose,
                       NULL,
                       compose_set_success,
                       compose_set_incomplete,
                       compose_set_error);

    /*
     * The compose should activate by activating the first wrapped
     * callback.
     */

    compose->callback.activate = compose->first->activate;

    /*
     * The first callback should succeed by activating the second.
     */

    push_continuation_call(&first->set_success,
                           &second->activate);

    return &compose->callback;
}
