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
             void *parent,
             push_parser_t *parser)
{
    dup_t  *dup = push_talloc(parent, dup_t);

    if (dup == NULL)
        return NULL;

    if (name == NULL) name = "dup";
    push_talloc_set_name_const(dup, name);

    push_callback_init(&dup->callback, parser, dup,
                       dup_activate,
                       NULL, NULL, NULL);

    return &dup->callback;
}


push_callback_t *
push_both_new(const char *name,
              void *parent,
              push_parser_t *parser,
              push_callback_t *a,
              push_callback_t *b)
{
    void  *context;
    push_callback_t  *dup;
    push_callback_t  *par;
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

    if (name == NULL) name = "both";

    dup = push_dup_new
        (push_talloc_asprintf(context, "%s.dup", name),
         context, parser);
    par = push_par_new
        (push_talloc_asprintf(context, "%s.par", name),
         context, parser, a, b);
    callback = push_compose_new
        (push_talloc_asprintf(context, "%s.compose", name),
         context, parser, dup, par);

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
