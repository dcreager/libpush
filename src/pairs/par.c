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

    const char  *first_name = NULL;
    const char  *second_name = NULL;
    const char  *compose_name = NULL;

    push_callback_t  *first = NULL;
    push_callback_t  *second = NULL;
    push_callback_t  *callback = NULL;

    /*
     * If either wrapped callback is NULL, return NULL ourselves.
     */

    if ((a == NULL) || (b == NULL))
        return NULL;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "both";

    first_name = push_string_concat(parser, name, ".first");
    if (first_name == NULL) goto error;

    second_name = push_string_concat(parser, name, ".second");
    if (second_name == NULL) goto error;

    compose_name = push_string_concat(parser, name, ".compose");
    if (compose_name == NULL) goto error;

    /*
     * Then create the callbacks.
     */

    first = push_first_new(first_name, parser, a);
    second = push_second_new(second_name, parser, b);
    callback = push_compose_new(compose_name, parser, first, second);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (callback == NULL) goto error;

    /*
     * Make each name string be the child of its callback.
     */

    push_talloc_steal(first, first_name);
    push_talloc_steal(second, second_name);
    push_talloc_steal(callback, compose_name);

    return callback;

  error:
    if (first_name != NULL) push_talloc_free(first_name);
    if (first != NULL) push_talloc_free(first);

    if (second_name != NULL) push_talloc_free(second_name);
    if (second != NULL) push_talloc_free(second);

    if (compose_name != NULL) push_talloc_free(compose_name);
    if (callback != NULL) push_talloc_free(callback);

    return NULL;
}
