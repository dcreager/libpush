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
 * Indexed sum callback implementation
 *
 * The indexed sum callback is implemented like the sum callback,
 * using pairs.  The first element of the pair is itself a pair — two
 * parsed integer, the index and the value.  The second element of the
 * pair is an array of sums.
 */


typedef struct _inner_sum
{
    push_callback_t  callback;
    uint32_t  num_sums;
} inner_sum_t;


static void
inner_sum_activate(void *user_data,
                   void *result,
                   const void *buf,
                   size_t bytes_remaining)
{
    inner_sum_t  *inner_sum = (inner_sum_t *) user_data;
    push_pair_t  *input = (push_pair_t *) result;
    push_pair_t  *input_ints = (push_pair_t *) input->first;
    uint32_t  *input_index = (uint32_t *) input_ints->first;
    uint32_t  *input_int = (uint32_t *) input_ints->second;
    uint32_t  *input_sums = (uint32_t *) input->second;

    PUSH_DEBUG_MSG("sum: Activating callback.  "
                   "Received value %"PRIu32" for index "
                   "%"PRIu32".\n",
                   *input_int,
                   *input_index);

    if ((*input_index < 0) || (*input_index >= inner_sum->num_sums))
    {
        PUSH_DEBUG_MSG("sum: Index is out of range.\n");

        push_continuation_call(inner_sum->callback.error,
                               PUSH_PARSE_ERROR,
                               "Index is out of range");

        return;
    }

    PUSH_DEBUG_MSG("sum: Previous sum #%"PRIu32" was "
                   "%"PRIu32".\n",
                   *input_index,
                   input_sums[*input_index]);

    input_sums[*input_index] += *input_int;

    PUSH_DEBUG_MSG("sum: Adding, sum is now %"PRIu32"\n",
                   input_sums[*input_index]);

    push_continuation_call(inner_sum->callback.success,
                           input_sums,
                           buf, bytes_remaining);

    return;
}


static push_callback_t *
inner_sum_callback_new(push_parser_t *parser, uint32_t num_sums)
{
    inner_sum_t  *inner_sum =
        (inner_sum_t *) malloc(sizeof(inner_sum_t));

    if (inner_sum == NULL)
        return NULL;

    push_callback_init(&inner_sum->callback, parser, inner_sum,
                       inner_sum_activate,
                       NULL, NULL, NULL);

    inner_sum->num_sums = num_sums;

    return &inner_sum->callback;
}


push_callback_t *
indexed_sum_callback_new(push_parser_t *parser, uint32_t num_sums)
{
    push_callback_t  *dup;
    push_callback_t  *integer;
    push_callback_t  *index;
    push_callback_t  *both;
    push_callback_t  *first;
    push_callback_t  *inner_sum;
    push_callback_t  *compose1;
    push_callback_t  *compose2;

    dup = push_dup_new(parser);

    index = integer_callback_new(parser);
    integer = integer_callback_new(parser);
    both = push_both_new(parser, index, integer);

    first = push_first_new(parser, both);
    inner_sum = inner_sum_callback_new(parser, num_sums);

    compose1 = push_compose_new(parser, dup, first);
    compose2 = push_compose_new(parser, compose1, inner_sum);

    return compose2;
}
