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
    dup_t  *dup = push_talloc(parser, dup_t);

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
    const char  *dup_name = NULL;
    const char  *par_name = NULL;
    const char  *compose_name = NULL;

    push_callback_t  *dup = NULL;
    push_callback_t  *par = NULL;
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

    dup_name = push_string_concat(parser, name, ".dup");
    if (dup_name == NULL) goto error;

    par_name = push_string_concat(parser, name, ".par");
    if (par_name == NULL) goto error;

    compose_name = push_string_concat(parser, name, ".compose");
    if (compose_name == NULL) goto error;

    /*
     * Then create the callbacks.
     */

    dup = push_dup_new(dup_name, parser);
    par = push_par_new(par_name, parser, a, b);
    callback = push_compose_new(compose_name, parser, dup, par);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (callback == NULL) goto error;

    /*
     * Make each name string be the child of its callback.
     */

    push_talloc_steal(dup, dup_name);
    push_talloc_steal(par, par_name);
    push_talloc_steal(callback, compose_name);

    return callback;

  error:
    if (dup_name != NULL) push_talloc_free(dup_name);
    if (dup != NULL) push_talloc_free(dup);

    if (par_name != NULL) push_talloc_free(par_name);
    if (par != NULL) push_talloc_free(par);

    if (compose_name != NULL) push_talloc_free(compose_name);
    if (callback != NULL) push_talloc_free(callback);

    return NULL;
}
