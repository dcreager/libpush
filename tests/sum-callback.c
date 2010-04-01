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
#include <push/pure.h>
#include <push/primitives.h>
#include <push/tuples.h>
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


static bool
inner_sum_func(uint32_t *result, push_tuple_t *input, uint32_t **output)
{
    uint32_t  *input_int = (uint32_t *) input->elements[0];
    uint32_t  *input_sum = (uint32_t *) input->elements[1];

    PUSH_DEBUG_MSG("inner-sum: Activating callback.  "
                   "Received value %"PRIu32", sum %"PRIu32".\n",
                   *input_int,
                   *input_sum);

    *result = *input_int + *input_sum;

    PUSH_DEBUG_MSG("inner-sum: Adding, sum is now %"PRIu32"\n",
                   *result);

    *output = result;
    return true;
}


push_define_pure_data_callback(inner_sum_new, inner_sum_func,
                               "inner-sum",
                               push_tuple_t, uint32_t, uint32_t);


static push_callback_t *
inner_sum_callback_new(const char *name,
                       void *parent,
                       push_parser_t *parser)
{
    return inner_sum_new(name, parent, parser, NULL);
}


push_callback_t *
sum_callback_new(const char *name,
                 void *parent,
                 push_parser_t *parser)
{
    void  *context;
    push_callback_t  *dup;
    push_callback_t  *integer;
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
     * Create the callbacks.
     */

    if (name == NULL) name = "sum";

    dup = push_dup_new
        (push_talloc_asprintf(context, "%s.dup", name),
         context, parser, 2);
    integer = integer_callback_new
        (push_talloc_asprintf(context, "%s.integer", name),
         context, parser);
    first = push_first_new
        (push_talloc_asprintf(context, "%s.first", name),
         context, parser, integer);
    inner_sum = inner_sum_callback_new
        (push_talloc_asprintf(context, "%s.inner", name),
         context, parser);
    compose1 = push_compose_new
        (push_talloc_asprintf(context, "%s.compose1", name),
         context, parser, dup, first);
    compose2 = push_compose_new
        (push_talloc_asprintf(context, "%s.compose2", name),
         context, parser, compose1, inner_sum);

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
