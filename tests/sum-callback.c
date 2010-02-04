/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <inttypes.h>
#include <stdint.h>

#include <push/basics.h>
#include <push/combinators.h>
#include <push/pairs.h>
#include <push/primitives.h>
#include <test-callbacks.h>


/*-----------------------------------------------------------------------
 * Sum callback implementation
 *
 * The sum callback is implemented using pairs: It expects to receive
 * a pair of uint32_t's.  The first one is the integer that was just
 * parsed; the second is the previous value of the sum.  The output is
 * a pair, with the first element NULL, and the second the new sum.
 *
 * The “repeated sum” callback composes an integer callback and a sum
 * callback together.  The composed callback has the following design:
 *
 *    +------------------------------------------------+
 *    |                                                |
 *    | ignore +-----+  next int  +-------+            |
 *    |   /===>| Int |===========>|       |            |
 *    |==<     +-----+            | Inner |===========>|
 *    |   \    +------+           |  Sum  |  new sum   |
 *    |    \==>| Noop |==========>|       |            |
 *    |  old   +------+  old sum  +-------+            |
 *    |  sum                                           |
 *    +------------------------------------------------+
 *
 * So it takes in a pair, but the callback expects the first element
 * to be NULL on input, and outputs a NULL there as well.  The first
 * element is only used internally in between the Int and Sum
 * callbacks.
 */


typedef struct _inner_sum_callback
{
    push_callback_t  base;
    uint32_t  *input_int;
    uint32_t  *input_sum;
    uint32_t  output_sum;
} inner_sum_callback_t;


static push_error_code_t
inner_sum_activate(push_parser_t *parser,
                   push_callback_t *pcallback,
                   void *vinput)
{
    inner_sum_callback_t  *callback =
        (inner_sum_callback_t *) pcallback;
    push_pair_t  *input = (push_pair_t *) vinput;

    callback->input_int = (uint32_t *) input->first;
    callback->input_sum = (uint32_t *) input->second;

    PUSH_DEBUG_MSG("sum: Activating callback.  "
                   "Received value %"PRIu32", sum %"PRIu32".\n",
                   *callback->input_int,
                   *callback->input_sum);
    return PUSH_SUCCESS;
}


static ssize_t
inner_sum_process_bytes(push_parser_t *parser,
                        push_callback_t *pcallback,
                        const void *buf,
                        size_t bytes_available)
{
    inner_sum_callback_t  *callback = (inner_sum_callback_t *) pcallback;

    /*
     * Add the two numbers together.  The result pointer does't change
     * if we execute this callback more than once, so it was set in
     * the constructor.
     */

    callback->output_sum = *callback->input_int + *callback->input_sum;

    PUSH_DEBUG_MSG("sum: Adding, sum is now %"PRIu32"\n",
                   callback->output_sum);

    /*
     * We don't actually parse anything, so we always succeed.
     */

    return bytes_available;
}


static push_callback_t *
inner_sum_callback_new()
{
    inner_sum_callback_t  *sum =
        (inner_sum_callback_t *) malloc(sizeof(inner_sum_callback_t));

    if (sum == NULL)
        return NULL;

    push_callback_init(&sum->base,
                       inner_sum_activate,
                       inner_sum_process_bytes,
                       NULL);

    sum->base.result = &sum->output_sum;

    return &sum->base;
}


push_callback_t *
sum_callback_new()
{
    push_callback_t  *integer;
    push_callback_t  *noop;
    push_callback_t  *both;
    push_callback_t  *inner_sum;
    push_callback_t  *compose;

    integer = integer_callback_new();
    noop = push_noop_new();
    both = push_both_new(integer, noop);
    inner_sum = inner_sum_callback_new();
    compose = push_compose_new(both, inner_sum);

    return compose;
}
