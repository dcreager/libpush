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
push_par_new(const char *name,
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

    const char  *first_name;
    const char  *second_name;
    const char  *compose_name;

    push_callback_t  *first;
    push_callback_t  *second;
    push_callback_t  *callback;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "both";

    first_name = push_string_concat(name, ".first");
    if (first_name == NULL) return NULL;

    second_name = push_string_concat(name, ".second");
    if (second_name == NULL) return NULL;

    compose_name = push_string_concat(name, ".compose");
    if (compose_name == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    first = push_first_new(first_name, parser, a);
    if (first == NULL)
        return NULL;

    second = push_second_new(second_name, parser, b);
    if (second == NULL)
    {
        push_callback_free(first);
        return NULL;
    }

    callback = push_compose_new(compose_name, parser, first, second);
    if (callback == NULL)
    {
        push_callback_free(first);
        push_callback_free(second);
        return NULL;
    }

    return callback;
}
