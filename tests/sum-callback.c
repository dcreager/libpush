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
                   push_talloc_get_name(inner_sum),
                   *input_int,
                   *input_sum);

    inner_sum->result = *input_int + *input_sum;

    PUSH_DEBUG_MSG("%s: Adding, sum is now %"PRIu32"\n",
                   push_talloc_get_name(inner_sum),
                   inner_sum->result);

    push_continuation_call(inner_sum->callback.success,
                           &inner_sum->result,
                           buf, bytes_remaining);

    return;
}


static push_callback_t *
inner_sum_callback_new(const char *name,
                       void *parent,
                       push_parser_t *parser)
{
    inner_sum_t  *inner_sum = push_talloc(parent, inner_sum_t);

    if (inner_sum == NULL)
        return NULL;

    if (name == NULL)
        name = "sum";

    push_callback_init(&inner_sum->callback, parser, inner_sum,
                       inner_sum_activate,
                       NULL, NULL, NULL);

    return &inner_sum->callback;
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
         context, parser);
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
