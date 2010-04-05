/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdarg.h>

#include <push/basics.h>
#include <push/combinators.h>
#include <push/talloc.h>
#include <push/tuples.h>


push_callback_t *
push_par_anew(const char *name,
              void *parent,
              push_parser_t *parser,
              size_t tuple_size,
              push_callback_t **callbacks)
{
    /*
     * For now, we just implement this using the definition from
     * Hughes's paper:
     *
     *   a *** b = first a >>> second b
     *
     * When there are more than two callbacks, we add additional calls
     * to nth to the end of the composition chain.
     */

    size_t  i;
    void  *context;
    push_callback_t  *callback;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Create the callbacks.  We start with “first a”; then we
     * repeatedly compose in “second b”, “third c”, etc., until we've
     * exhausted all of the callbacks.  If any of the wrapped
     * callbacks is NULL, we return NULL ourselves.
     */

    if (name == NULL) name = "par";

    if (callbacks[0] == NULL) goto error;

    callback = push_nth_new
        (push_talloc_asprintf(context, "%s.0", name),
         context, parser, callbacks[0], 0, tuple_size);

    for (i = 1; i < tuple_size; i++)
    {
        push_callback_t  *nth;

        if (callbacks[i] == NULL) goto error;

        nth = push_nth_new
            (push_talloc_asprintf(context, "%s.%zu", name, i),
             context, parser, callbacks[i], i, tuple_size);

        callback = push_compose_new
            (push_talloc_asprintf(context, "%s.compose%zu", name, i),
             context, parser, callback, nth);
    }

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


push_callback_t *
push_par_vnew(const char *name,
              void *parent,
              push_parser_t *parser,
              size_t tuple_size,
              va_list callbacks)
{
    /*
     * For now, we just implement this using the definition from
     * Hughes's paper:
     *
     *   a *** b = first a >>> second b
     *
     * When there are more than two callbacks, we add additional calls
     * to nth to the end of the composition chain.
     */

    size_t  i;
    void  *context;
    push_callback_t  *wrapped;
    push_callback_t  *callback;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Create the callbacks.  We start with “first a”; then we
     * repeatedly compose in “second b”, “third c”, etc., until we've
     * exhausted all of the callbacks.  If any of the wrapped
     * callbacks is NULL, we return NULL ourselves.
     */

    if (name == NULL) name = "par";

    wrapped = va_arg(callbacks, push_callback_t *);
    if (wrapped == NULL) goto error;

    callback = push_nth_new
        (push_talloc_asprintf(context, "%s.0", name),
         context, parser, wrapped, 0, tuple_size);

    for (i = 1; i < tuple_size; i++)
    {
        push_callback_t  *nth;

        wrapped = va_arg(callbacks, push_callback_t *);
        if (wrapped == NULL) goto error;

        nth = push_nth_new
            (push_talloc_asprintf(context, "%s.%zu", name, i),
             context, parser, wrapped, i, tuple_size);

        callback = push_compose_new
            (push_talloc_asprintf(context, "%s.compose%zu", name, i),
             context, parser, callback, nth);
    }

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


push_callback_t *
push_par_new(const char *name,
             void *parent,
             push_parser_t *parser,
             size_t tuple_size,
             ...)
{
    va_list  callbacks;
    push_callback_t  *result;

    va_start(callbacks, tuple_size);
    result = push_par_vnew(name, parent, parser,
                           tuple_size, callbacks);
    va_end(callbacks);

    return result;
}
