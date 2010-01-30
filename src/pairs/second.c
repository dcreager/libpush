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
#include <push/pairs.h>


/**
 * The push_callback_t subclass that defines a second callback.
 */

typedef struct _second
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * The wrapped callback.
     */

    push_callback_t  *wrapped;

    /**
     * A reference to our input pair.  This lets us copy out the first
     * element once the wrapped callback finishes.
     */

    push_pair_t  *input;

    /**
     * The pair object that we output as our result.
     */

    push_pair_t  result;

} second_t;


static push_error_code_t
second_activate(push_parser_t *parser,
                push_callback_t *pcallback,
                void *input)
{
    second_t  *callback = (second_t *) pcallback;

    /*
     * Save a reference to the pair, so that we can copy the first
     * element into our result later.
     */

    callback->input = (push_pair_t *) input;

    /*
     * We activate this callback by passing the second element of the
     * input pair into our wrapped callback.
     */

    PUSH_DEBUG_MSG("second: Activating wrapped callback.\n");
    return push_callback_activate(parser, callback->wrapped,
                                  callback->input->second);
}


static ssize_t
second_process_bytes(push_parser_t *parser,
                     push_callback_t *pcallback,
                     const void *vbuf,
                     size_t bytes_available)
{
    second_t  *callback = (second_t *) pcallback;
    ssize_t  result;

    /*
     * Pass off to the wrapped callback.  This isn't a tail call
     * because we don't use the wrapped callback's result as our
     * result.
     */

    result = push_callback_process_bytes(parser, callback->wrapped,
                                         vbuf, bytes_available);

    /*
     * If the wrapped callback gives us a success result, create a new
     * pair for our result.
     */

    if (result >= 0)
    {
        callback->result.first = callback->input->first;
        callback->result.second = callback->wrapped->result;
    }

    return result;
}


static void
second_free(push_callback_t *pcallback)
{
    second_t  *callback = (second_t *) pcallback;

    PUSH_DEBUG_MSG("second: Freeing wrapped callback.\n");
    push_callback_free(callback->wrapped);
}


push_callback_t *
push_second_new(push_callback_t *wrapped)
{
    second_t  *callback =
        (second_t *) malloc(sizeof(second_t));

    if (callback == NULL)
        return NULL;

    push_callback_init(&callback->base,
                       second_activate,
                       second_process_bytes,
                       second_free);

    callback->wrapped = wrapped;
    callback->base.result = &callback->result;

    return &callback->base;
}
