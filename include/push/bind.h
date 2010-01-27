/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_BIND_H
#define PUSH_BIND_H

#include <stdbool.h>

#include <push/basics.h>


/**
 * A callback that binds to subcallbacks together.  The first callback
 * is allowed to parse the data until it finishes, by returning a
 * success or error code.  (PUSH_INCOMPLETE doesn't count as finishing
 * the parse.)  Once the first callback has finished, its result is
 * used to activate the second callback, at which point it is allowed
 * to parse the remaining data.  Once the second callback finishes,
 * the push_bind_t finishes.
 */

typedef struct _push_bind
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * The first child callback in the bind.
     */

    push_callback_t  *first;

    /**
     * The second child callback in the bind.
     */

    push_callback_t  *second;

    /**
     * Whether the first or second child callback is active.
     */

    bool first_active;

} push_bind_t;


/**
 * Allocate and initialize a new callback that binds together two
 * child callbacks.
 */

push_bind_t *
push_bind_new(push_callback_t *first,
              push_callback_t *second);


#endif  /* PUSH_BIND_H */
