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

typedef struct _both
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * The input pair that we pass in to the wrapped callback.
     */

    push_pair_t  input;

    /**
     * The wrapped (a *** b) callback.
     */

    push_callback_t  *wrapped;

} both_t;


static push_error_code_t
both_activate(push_parser_t *parser,
              push_callback_t *pcallback,
              void *input)
{
    both_t  *callback = (both_t *) pcallback;

    /*
     * Duplicate the input into a pair, and pass that into the wrapped
     * (a *** b) callback.
     */

    callback->input.first = input;
    callback->input.second = input;

    return push_callback_activate(parser, callback->wrapped,
                                  &callback->input);
}


static ssize_t
both_process_bytes(push_parser_t *parser,
                   push_callback_t *pcallback,
                   const void *vbuf,
                   size_t bytes_available)
{
    both_t  *callback = (both_t *) pcallback;

    /*
     * Delegate to the wrapped callback for our behavior.
     */

    return push_callback_tail_process_bytes
        (parser, &callback->base,
         callback->wrapped,
         vbuf, bytes_available);
}


static void
both_free(push_callback_t *pcallback)
{
    both_t  *callback = (both_t *) pcallback;

    PUSH_DEBUG_MSG("both: Freeing wrapped callback.\n");
    push_callback_free(callback->wrapped);
}


push_callback_t *
push_both_new(push_callback_t *a,
              push_callback_t *b)
{
    push_callback_t  *wrapped;
    both_t  *callback;

    wrapped = push_par_new(a, b);
    if (wrapped == NULL)
        return NULL;

    callback = (both_t *) malloc(sizeof(both_t));
    if (callback == NULL)
    {
        push_callback_free(wrapped);
        return NULL;
    }

    push_callback_init(&callback->base,
                       both_activate,
                       both_process_bytes,
                       both_free);

    callback->wrapped = wrapped;

    return &callback->base;
}
