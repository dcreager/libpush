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
    uint32_t  num_sums;
} inner_sum_t;


static bool
inner_sum_func(void *user_data, void *vinput, void **output)
{
    inner_sum_t  *inner_sum = (inner_sum_t *) user_data;
    push_pair_t  *input = (push_pair_t *) vinput;
    push_pair_t  *input_ints = (push_pair_t *) input->first;
    uint32_t  *input_index = (uint32_t *) input_ints->first;
    uint32_t  *input_int = (uint32_t *) input_ints->second;
    uint32_t  *input_sums = (uint32_t *) input->second;

    PUSH_DEBUG_MSG("inner-sum: Activating callback.  "
                   "Received value %"PRIu32" for index "
                   "%"PRIu32".\n",
                   *input_int,
                   *input_index);

    if ((*input_index < 0) || (*input_index >= inner_sum->num_sums))
    {
        PUSH_DEBUG_MSG("inner-sum: Index is out of range.\n");
        return false;
    }

    PUSH_DEBUG_MSG("inner-sum: Previous sum #%"PRIu32" was "
                   "%"PRIu32".\n",
                   *input_index,
                   input_sums[*input_index]);

    input_sums[*input_index] += *input_int;

    PUSH_DEBUG_MSG("inner-sum: Adding, sum is now %"PRIu32"\n",
                   input_sums[*input_index]);

    *output = input_sums;
    return true;
}


static push_callback_t *
inner_sum_callback_new(const char *name,
                       void *parent,
                       push_parser_t *parser,
                       uint32_t num_sums)
{
    push_callback_t  *inner_sum;
    void  *vuser_data;
    inner_sum_t  *user_data;

    if (name == NULL) name = "inner-sum";

    inner_sum = push_pure_data_new
        (name, parent, parser,
         inner_sum_func, &vuser_data,
         sizeof(inner_sum_t));

    if (inner_sum == NULL) return NULL;

    user_data = (inner_sum_t *) vuser_data;
    user_data->num_sums = num_sums;
    return inner_sum;
}


push_callback_t *
indexed_sum_callback_new(const char *name,
                         void *parent,
                         push_parser_t *parser,
                         uint32_t num_sums)
{
    void  *context;
    push_callback_t  *dup;
    push_callback_t  *integer;
    push_callback_t  *index;
    push_callback_t  *both;
    push_callback_t  *first;
    push_callback_t  *inner_sum;
    push_callback_t  *compose1;
    push_callback_t  *compose2;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    if (name == NULL) name = "indexed-sum";

    dup = push_dup_new
        (push_talloc_asprintf(context, "%s.dup", name),
         context, parser);
    index = integer_callback_new
        (push_talloc_asprintf(context, "%s.index", name),
         context, parser);
    integer = integer_callback_new
        (push_talloc_asprintf(context, "%s.integer", name),
         context, parser);
    both = push_both_new
        (push_talloc_asprintf(context, "%s.both", name),
         context, parser, index, integer);
    first = push_first_new
        (push_talloc_asprintf(context, "%s.first", name),
         context, parser, both);
    inner_sum = inner_sum_callback_new
        (push_talloc_asprintf(context, "%s.inner", name),
         context,
         parser, num_sums);
    compose1 = push_compose_new
        (push_talloc_asprintf(context, "%s.compose1", name),
         context,
         parser, dup, first);
    compose2 = push_compose_new
        (push_talloc_asprintf(context, "%s.compose2", name),
         context,
         parser, compose1, inner_sum);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (compose2 == NULL) goto error;
    return compose2;

  error:
    /*
     * Before returning, free any objects we created before the error.
     */

    push_talloc_free(context);
    return NULL;
}
