/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_COMPOSE_H
#define PUSH_COMPOSE_H

#include <stdbool.h>

#include <push/basics.h>


/**
 * A callback that composes two subcallbacks together.  The first
 * callback is allowed to parse the data until it finishes, by
 * returning a success or error code.  (PUSH_INCOMPLETE doesn't count
 * as finishing the parse.)  Once the first callback has finished, its
 * result is used to activate the second callback, at which point it
 * is allowed to parse the remaining data.  Once the second callback
 * finishes, the push_compose_t finishes.
 *
 * This combinator is equivalent to the Haskell
 * <code>&gt;&gt;&gt;</code> arrow operator.
 */

typedef struct _push_compose
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * The first child callback in the compose.
     */

    push_callback_t  *first;

    /**
     * The second child callback in the compose.
     */

    push_callback_t  *second;

    /**
     * Whether the first or second child callback is active.
     */

    bool first_active;

} push_compose_t;


/**
 * Allocate and initialize a new callback that composes together two
 * child callbacks.
 */

push_compose_t *
push_compose_new(push_callback_t *first,
                 push_callback_t *second);


#endif  /* PUSH_COMPOSE_H */
