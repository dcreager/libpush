/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_COMBINATORS_H
#define PUSH_COMBINATORS_H

/**
 * @file
 *
 * This file defines the <i>parser combinators</i>, which wrap other
 * combinators.
 */


/**
 * Create a new callback that composes together two child callbacks.
 * The first callback is allowed to parse the data until it finishes,
 * by returning a success or error code.  (PUSH_INCOMPLETE doesn't
 * count as finishing the parse.)  Once the first callback has
 * finished, its result is used to activate the second callback, at
 * which point it is allowed to parse the remaining data.  Once the
 * second callback finishes, the push_compose_t finishes.
 *
 * This combinator is equivalent to the Haskell
 * <code>&gt;&gt;&gt;</code> arrow operator.
 */

push_callback_t *
push_compose_new(push_parser_t *parser,
                 push_callback_t *first,
                 push_callback_t *second);


/**
 * Create a new callback that calls another callback repeatedly.  The
 * input of each iteration of the wrapped callback is passed as input
 * into the next iteration.  We terminate the loop when the wrapped
 * callback generates a parse failure; whatever result we had
 * accumulated to that point is then returned as the result of the
 * fold callback.  Note, however, that the wrapped callback must
 * generate the parse error <i>immediately</i>; if it partially parses
 * the data, and then discovers the parse error in a later call, we
 * cannot backtrack the data.  This case generates a parse error for
 * the fold.
 */

push_callback_t *
push_fold_new(push_callback_t *wrapped);


/**
 * Create a new callback that wraps another callback, ensuring that a
 * certain number of bytes are available before calling the wrapped
 * callback.  The data is buffered, if needed, until the minimum is
 * met.
 */

push_callback_t *
push_min_bytes_new(push_parser_t *parser,
                   push_callback_t *wrapped,
                   size_t minimum_bytes);


/**
 * Create a new callback that wraps another callback, ensuring that no
 * more than a certain number of bytes are passed into the wrapped
 * callback.  There is no guarantee that <b>exactly</b> this many
 * bytes will be processed; if the wrapped callback returns a success
 * code earlier, the max-bytes callback succeeds, too.  Once we reach
 * the maximum number of bytes, we send an EOF to the wrapped callback
 * to give it a chance to throw a parse error, if necessary.
 */

push_callback_t *
push_max_bytes_new(push_callback_t *wrapped,
                   size_t maximum_bytes);


/**
 * Create a new callback that wraps another callback, ensuring that no
 * more than a certain number of bytes are passed into the wrapped
 * callback.  This works exactly like the max-bytes combinator, except
 * that the number of bytes is taken as an input during parsing,
 * rather than provided once when constructing the callback.  The
 * input to the callback should be a pair.  The first element should
 * be a pointer to a size_t, indicating the number of bytes to use as
 * a threshold.  The second element is the input value that should be
 * passed into the wrapped callback.
 */

push_callback_t *
push_dynamic_max_bytes_new(push_callback_t *wrapped);



#endif  /* PUSH_COMBINATORS_H */
