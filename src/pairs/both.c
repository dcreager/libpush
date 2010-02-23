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


/**
 * The push_callback_t subclass that defines a both callback.  For
 * now, we just implement this using the definition from Hughes's
 * paper:
 *
 *   a &&& b = arr (\a -> (a,a)) >>> (a *** b)
 */

typedef struct _dup
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The output pair that we construct.
     */

    push_pair_t  result;

} dup_t;


static void
dup_activate(void *user_data,
             void *result,
             const void *buf,
             size_t bytes_remaining)
{
    dup_t  *dup = (dup_t *) user_data;

    /*
     * Duplicate the input into a pair, and immediately succeed with
     * that result.
     */

    dup->result.first = result;
    dup->result.second = result;

    push_continuation_call(dup->callback.success,
                           &dup->result,
                           buf, bytes_remaining);

    return;
}


push_callback_t *
push_dup_new(const char *name,
             push_parser_t *parser)
{
    dup_t  *dup = (dup_t *) malloc(sizeof(dup_t));

    if (dup == NULL)
        return NULL;

    if (name == NULL)
        name = "dup";

    push_callback_init(name,
                       &dup->callback, parser, dup,
                       dup_activate,
                       NULL, NULL, NULL);

    return &dup->callback;
}


push_callback_t *
push_both_new(const char *name,
              push_parser_t *parser,
              push_callback_t *a,
              push_callback_t *b)
{
    const char  *dup_name;
    const char  *par_name;
    const char  *compose_name;

    push_callback_t  *dup;
    push_callback_t  *par;
    push_callback_t  *callback;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "both";

    dup_name = push_string_concat(name, ".dup");
    if (dup_name == NULL) return NULL;

    par_name = push_string_concat(name, ".par");
    if (par_name == NULL) return NULL;

    compose_name = push_string_concat(name, ".compose");
    if (compose_name == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    dup = push_dup_new(dup_name, parser);
    if (dup == NULL)
        return NULL;

    par = push_par_new(par_name, parser, a, b);
    if (par == NULL)
    {
        push_callback_free(dup);
        return NULL;
    }

    callback = push_compose_new(compose_name, parser, dup, par);
    if (callback == NULL)
    {
        push_callback_free(dup);
        push_callback_free(par);
        return NULL;
    }

    return callback;
}
