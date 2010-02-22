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


push_callback_t *
push_par_new(push_parser_t *parser,
             push_callback_t *a,
             push_callback_t *b)
{
    /*
     * For now, we just implement this using the definition from
     * Hughes's paper:
     *
     *   a *** b = first a >>> second b
     */

    push_callback_t  *first;
    push_callback_t  *second;
    push_callback_t  *callback;

    first = push_first_new(parser, a);
    if (first == NULL)
        return NULL;

    second = push_second_new(parser, b);
    if (second == NULL)
    {
        push_callback_free(first);
        return NULL;
    }

    callback = push_compose_new(parser, first, second);
    if (callback == NULL)
    {
        push_callback_free(first);
        push_callback_free(second);
        return NULL;
    }

    return callback;
}
