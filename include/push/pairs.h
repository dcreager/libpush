/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PAIRS_H
#define PUSH_PAIRS_H

#include <stdbool.h>

#include <push/basics.h>


/**
 * Stores a pair of values.
 */

typedef struct _push_pair
{
    /**
     * The first element of the pair.
     */

    void  *first;

    /**
     * The second element of the pair.
     */

    void  *second;

} push_pair_t;


/**
 * Create a new callback that takes a pair as input, and applies
 * another callback to the first element of the pair.  The second
 * element is left unchanged.
 *
 * This combinator is equivalent to the Haskell <code>first</code>
 * arrow operator.
 */

push_callback_t *
push_first_new(push_callback_t *wrapped);


/**
 * Create a new callback that takes a pair as input, and applies
 * another callback to the second element of the pair.  The first
 * element is left unchanged.
 *
 * This combinator is equivalent to the Haskell <code>second</code>
 * arrow operator.
 */

push_callback_t *
push_second_new(push_callback_t *wrapped);


/**
 * Create a new callback that takes a pair as input, and applies one
 * callback to the first element of the pair, and a different callback
 * to the second element.
 *
 * This combinator is equivalent to the Haskell <code>***</code> arrow
 * operator.
 */

push_callback_t *
push_par_new(push_callback_t *first,
             push_callback_t *second);


/**
 * Create a new callback that takes a value as input, and applies two
 * callback to the value.  The results of the two callbacks are placed
 * into a pair, which is the result of the outer callback.
 *
 * This combinator is equivalent to the Haskell
 * <code>&amp;&amp;&amp;</code> arrow operator.
 */

push_callback_t *
push_both_new(push_callback_t *first,
              push_callback_t *second);


#endif  /* PUSH_PAIRS_H */