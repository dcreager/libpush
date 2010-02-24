/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <push/basics.h>
#include <push/combinators.h>
#include <push/pairs.h>
#include <push/talloc.h>


push_callback_t *
push_par_new(const char *name,
             void *parent,
             push_parser_t *parser,
             push_callback_t *a,
             push_callback_t *b)
{
    /*
     * For now, we just implement this using the definition from
     * Hughes's paper:
     *
     *   a *** b = first a >>> second b
     */

    void  *context;
    push_callback_t  *first;
    push_callback_t  *second;
    push_callback_t  *callback;

    /*
     * If either wrapped callback is NULL, return NULL ourselves.
     */

    if ((a == NULL) || (b == NULL))
        return NULL;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Create the callbacks.
     */

    if (name == NULL) name = "par";

    first = push_first_new
        (push_talloc_asprintf(context, "%s.first", name),
         context, parser, a);
    second = push_second_new
        (push_talloc_asprintf(context, "%s.second", name),
         context, parser, b);
    callback = push_compose_new
        (push_talloc_asprintf(context, "%s.compose", name),
         context, parser, first, second);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (callback == NULL) goto error;
    return callback;

  error:
    /*
     * Before returning, free any objects we created before the error.
     */

    push_talloc_free(context);
    return NULL;
}
