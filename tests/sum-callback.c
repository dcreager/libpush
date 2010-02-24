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
 *    +------------------------------------------------------+
 *    |                           next                       |
 *    |           ignore +-----+   int  +-------+            |
 *    |   +-----+   /===>| Int |=======>|       |            |
 *    |==>| Dup |==<     +-----+        | Inner |===========>|
 *    |   +-----+   \                   |  Sum  |  new sum   |
 *    |              \=================>|       |            |
 *    |                   old sum       +-------+            |
 *    |                                                      |
 *    +------------------------------------------------------+
 *
 * So it takes in a pair, but the callback expects the first element
 * to be NULL on input, and outputs a NULL there as well.  The first
 * element is only used internally in between the Int and Sum
 * callbacks.
 */


typedef struct _inner_sum
{
    push_callback_t  callback;
    uint32_t  result;
} inner_sum_t;


static void
inner_sum_activate(void *user_data,
                   void *result,
                   const void *buf,
                   size_t bytes_remaining)
{
    inner_sum_t  *inner_sum = (inner_sum_t *) user_data;
    push_pair_t  *input = (push_pair_t *) result;
    uint32_t  *input_int = (uint32_t *) input->first;
    uint32_t  *input_sum = (uint32_t *) input->second;

    PUSH_DEBUG_MSG("%s: Activating callback.  "
                   "Received value %"PRIu32", sum %"PRIu32".\n",
                   inner_sum->callback.name,
                   *input_int,
                   *input_sum);

    inner_sum->result = *input_int + *input_sum;

    PUSH_DEBUG_MSG("%s: Adding, sum is now %"PRIu32"\n",
                   inner_sum->callback.name,
                   inner_sum->result);

    push_continuation_call(inner_sum->callback.success,
                           &inner_sum->result,
                           buf, bytes_remaining);

    return;
}


static push_callback_t *
inner_sum_callback_new(const char *name,
                       push_parser_t *parser)
{
    inner_sum_t  *inner_sum = push_talloc(parser, inner_sum_t);

    if (inner_sum == NULL)
        return NULL;

    if (name == NULL)
        name = "sum";

    push_callback_init(name,
                       &inner_sum->callback, parser, inner_sum,
                       inner_sum_activate,
                       NULL, NULL, NULL);

    return &inner_sum->callback;
}


push_callback_t *
sum_callback_new(const char *name,
                 push_parser_t *parser)
{
    const char  *dup_name = NULL;
    const char  *integer_name = NULL;
    const char  *first_name = NULL;
    const char  *inner_sum_name = NULL;
    const char  *compose1_name = NULL;
    const char  *compose2_name = NULL;

    push_callback_t  *dup = NULL;
    push_callback_t  *integer = NULL;
    push_callback_t  *first = NULL;
    push_callback_t  *inner_sum = NULL;
    push_callback_t  *compose1 = NULL;
    push_callback_t  *compose2 = NULL;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "sum";

    dup_name = push_string_concat(parser, name, ".dup");
    if (dup_name == NULL) goto error;

    integer_name = push_string_concat(parser, name, ".value");
    if (integer_name == NULL) goto error;

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
    integer = integer_callback_new(integer_name, parser);
    first = push_first_new(first_name, parser, integer);
    inner_sum = inner_sum_callback_new(inner_sum_name, parser);
    compose1 = push_compose_new(compose1_name, parser, dup, first);
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
    push_talloc_steal(integer, integer_name);
    push_talloc_steal(first, first_name);
    push_talloc_steal(inner_sum, inner_sum_name);
    push_talloc_steal(compose1, compose1_name);
    push_talloc_steal(compose2, compose2_name);

    return compose2;

  error:
    if (dup_name != NULL) push_talloc_free(dup_name);
    if (dup != NULL) push_talloc_free(dup);

    if (integer_name != NULL) push_talloc_free(integer_name);
    if (integer != NULL) push_talloc_free(integer);

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
