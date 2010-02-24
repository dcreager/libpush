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
#include <push/talloc.h>
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
    inner_sum_t  *inner_sum = push_talloc(parser, inner_sum_t);

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
    const char  *dup_name = NULL;
    const char  *integer_name = NULL;
    const char  *index_name = NULL;
    const char  *both_name = NULL;
    const char  *first_name = NULL;
    const char  *inner_sum_name = NULL;
    const char  *compose1_name = NULL;
    const char  *compose2_name = NULL;

    push_callback_t  *dup = NULL;
    push_callback_t  *integer = NULL;
    push_callback_t  *index = NULL;
    push_callback_t  *both = NULL;
    push_callback_t  *first = NULL;
    push_callback_t  *inner_sum = NULL;
    push_callback_t  *compose1 = NULL;
    push_callback_t  *compose2 = NULL;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "indexed-sum";

    dup_name = push_string_concat(parser, name, ".dup");
    if (dup_name == NULL) goto error;

    integer_name = push_string_concat(parser, name, ".value");
    if (integer_name == NULL) goto error;

    index_name = push_string_concat(parser, name, ".index");
    if (index_name == NULL) goto error;

    both_name = push_string_concat(parser, name, ".both");
    if (both_name == NULL) goto error;

    first_name = push_string_concat(parser, name, ".first");
    if (first_name == NULL) goto error;

    inner_sum_name = push_string_concat(parser, name, ".sum");
    if (inner_sum_name == NULL) goto error;

    compose1_name = push_string_concat(parser, name, ".compose1");
    if (compose1_name == NULL) goto error;

    compose2_name = push_string_concat(parser, name, ".compose2");
    if (compose2_name == NULL) goto error;

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

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (compose2 == NULL) goto error;

    /*
     * Make each name string be the child of its callback.
     */

    push_talloc_steal(dup, dup_name);
    push_talloc_steal(index, index_name);
    push_talloc_steal(integer, integer_name);
    push_talloc_steal(both, both_name);
    push_talloc_steal(first, first_name);
    push_talloc_steal(inner_sum, inner_sum_name);
    push_talloc_steal(compose1, compose1_name);
    push_talloc_steal(compose2, compose2_name);

    return compose2;

  error:
    if (dup_name != NULL) push_talloc_free(dup_name);
    if (dup != NULL) push_talloc_free(dup);

    if (index_name != NULL) push_talloc_free(index_name);
    if (index != NULL) push_talloc_free(index);

    if (integer_name != NULL) push_talloc_free(integer_name);
    if (integer != NULL) push_talloc_free(integer);

    if (both_name != NULL) push_talloc_free(both_name);
    if (both != NULL) push_talloc_free(both);

    if (first_name != NULL) push_talloc_free(first_name);
    if (first != NULL) push_talloc_free(first);

    if (inner_sum_name != NULL) push_talloc_free(inner_sum_name);
    if (inner_sum != NULL) push_talloc_free(inner_sum);

    if (compose1_name != NULL) push_talloc_free(compose1_name);
    if (compose1 != NULL) push_talloc_free(compose1);

    if (compose2_name != NULL) push_talloc_free(compose2_name);
    if (compose2 != NULL) push_talloc_free(compose2);

    return NULL;
}
