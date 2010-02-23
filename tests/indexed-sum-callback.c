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

    PUSH_DEBUG_MSG("%s: Activating callback.  "
                   "Received value %"PRIu32" for index "
                   "%"PRIu32".\n",
                   inner_sum->callback.name,
                   *input_int,
                   *input_index);

    if ((*input_index < 0) || (*input_index >= inner_sum->num_sums))
    {
        PUSH_DEBUG_MSG("%s: Index is out of range.\n",
                       inner_sum->callback.name);

        push_continuation_call(inner_sum->callback.error,
                               PUSH_PARSE_ERROR,
                               "Index is out of range");

        return;
    }

    PUSH_DEBUG_MSG("%s: Previous sum #%"PRIu32" was "
                   "%"PRIu32".\n",
                   inner_sum->callback.name,
                   *input_index,
                   input_sums[*input_index]);

    input_sums[*input_index] += *input_int;

    PUSH_DEBUG_MSG("%s: Adding, sum is now %"PRIu32"\n",
                   inner_sum->callback.name,
                   input_sums[*input_index]);

    push_continuation_call(inner_sum->callback.success,
                           input_sums,
                           buf, bytes_remaining);

    return;
}


static push_callback_t *
inner_sum_callback_new(const char *name,
                       push_parser_t *parser, uint32_t num_sums)
{
    inner_sum_t  *inner_sum =
        (inner_sum_t *) malloc(sizeof(inner_sum_t));

    if (inner_sum == NULL)
        return NULL;

    if (name == NULL)
        name = "inner-sum";

    push_callback_init(name,
                       &inner_sum->callback, parser, inner_sum,
                       inner_sum_activate,
                       NULL, NULL, NULL);

    inner_sum->num_sums = num_sums;

    return &inner_sum->callback;
}


push_callback_t *
indexed_sum_callback_new(const char *name,
                         push_parser_t *parser,
                         uint32_t num_sums)
{
    const char  *dup_name;
    const char  *integer_name;
    const char  *index_name;
    const char  *both_name;
    const char  *first_name;
    const char  *inner_sum_name;
    const char  *compose1_name;
    const char  *compose2_name;

    push_callback_t  *dup;
    push_callback_t  *integer;
    push_callback_t  *index;
    push_callback_t  *both;
    push_callback_t  *first;
    push_callback_t  *inner_sum;
    push_callback_t  *compose1;
    push_callback_t  *compose2;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "indexed-sum";

    dup_name = push_string_concat(name, ".dup");
    if (dup_name == NULL) return NULL;

    integer_name = push_string_concat(name, ".value");
    if (integer_name == NULL) return NULL;

    index_name = push_string_concat(name, ".index");
    if (index_name == NULL) return NULL;

    both_name = push_string_concat(name, ".both");
    if (both_name == NULL) return NULL;

    first_name = push_string_concat(name, ".first");
    if (first_name == NULL) return NULL;

    inner_sum_name = push_string_concat(name, ".sum");
    if (inner_sum_name == NULL) return NULL;

    compose1_name = push_string_concat(name, ".compose1");
    if (compose1_name == NULL) return NULL;

    compose2_name = push_string_concat(name, ".compose2");
    if (compose2_name == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    dup = push_dup_new(dup_name, parser);

    index = integer_callback_new(index_name, parser);
    integer = integer_callback_new(integer_name, parser);
    both = push_both_new(both_name, parser, index, integer);

    first = push_first_new(first_name, parser, both);
    inner_sum = inner_sum_callback_new(inner_sum_name,
                                       parser, num_sums);

    compose1 = push_compose_new(compose1_name,
                                parser, dup, first);
    compose2 = push_compose_new(compose2_name,
                                parser, compose1, inner_sum);

    return compose2;
}
