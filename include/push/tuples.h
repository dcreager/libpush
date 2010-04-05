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
 * Stores a tuple of values.  Tuples do not grow after they have been
 * created.
 */

typedef struct _push_tuple
{
    /**
     * The number of elements in the tuple.
     */

    size_t  size;

    /**
     * The elements of the tuple.
     */

    void  *elements[];

} push_tuple_t;


#define PUSH_TUPLE_INIT(sz, ...) { sz, { __VA_ARGS__ } }


/**
 * Allocate a new tuple with the specified number of elements.  Each
 * element starts off NULL.
 */

push_tuple_t *
push_tuple_new(void *parent, size_t size);


/**
 * Free a tuple.
 */

void
push_tuple_free(push_tuple_t *tuple);


/**
 * Create a new callback that takes a tuple as input, and applies
 * another callback to the nth element of the pair.  The other
 * elements are left unchanged.
 */

push_callback_t *
push_nth_new(const char *name,
             void *parent,
             push_parser_t *parser,
             push_callback_t *wrapped,
             size_t n,
             size_t tuple_size);


/**
 * Create a new callback that takes a pair as input, and applies
 * another callback to the first element of the pair.  The second
 * element is left unchanged.
 *
 * This combinator is equivalent to the Haskell <code>first</code>
 * arrow operator.
 */

push_callback_t *
push_first_new(const char *name,
               void *parent,
               push_parser_t *parser,
               push_callback_t *wrapped);


/**
 * Create a new callback that takes a pair as input, and applies
 * another callback to the second element of the pair.  The first
 * element is left unchanged.
 *
 * This combinator is equivalent to the Haskell <code>second</code>
 * arrow operator.
 */

push_callback_t *
push_second_new(const char *name,
                void *parent,
                push_parser_t *parser,
                push_callback_t *wrapped);


/**
 * Create a new callback that duplicates its input.  The output will
 * be a pair, where the first and second element are both the same as
 * the input.
 */

push_callback_t *
push_dup_new(const char *name,
             void *parent,
             push_parser_t *parser,
             size_t tuple_size);


/**
 * Create a new callback that takes a tuple as input, and applies a
 * different callback to the each element of the tuple.
 *
 * When applied to a 2-tuple, this combinator is equivalent to the
 * Haskell <code>***</code> arrow operator.
 */

push_callback_t *
push_par_new(const char *name,
             void *parent,
             push_parser_t *parser,
             size_t tuple_size,
             push_callback_t **callbacks);


/**
 * Create a new callback that takes a value as input, and applies a
 * number of different callbacks to the value.  The results of the
 * callbacks are placed into a tuple, which is the result of the outer
 * callback.
 *
 * When called with two callbacks, this combinator is equivalent to
 * the Haskell <code>&amp;&amp;&amp;</code> arrow operator.
 */

push_callback_t *
push_all_new(const char *name,
             void *parent,
             push_parser_t *parser,
             size_t tuple_size,
             push_callback_t **callbacks);


#endif  /* PUSH_PAIRS_H */
