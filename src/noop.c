/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>

#include <push/basics.h>
#include <push/primitives.h>


/**
 * The push_callback_t subclass that defines a noop callback.
 */

typedef struct _noop
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * A copy of the input pointer.  Once the process_bytes function
     * is called, we copy this to our output.
     */

    void  *input;

} noop_t;


static push_error_code_t
noop_activate(push_parser_t *parser,
              push_callback_t *pcallback,
              void *input)
{
    noop_t  *callback = (noop_t *) pcallback;

    PUSH_DEBUG_MSG("noop: Activating.\n");
    callback->input = input;

    return PUSH_SUCCESS;
}


static ssize_t
noop_process_bytes(push_parser_t *parser,
                   push_callback_t *pcallback,
                   const void *vbuf,
                   size_t bytes_available)
{
    noop_t  *callback = (noop_t *) pcallback;

    /*
     * Copy the input pointer to the output.
     */

    callback->base.result = callback->input;

    /*
     * We don't actually parse anything, so we always succeed.
     */

    return bytes_available;
}


push_callback_t *
push_noop_new()
{
    noop_t  *callback = (noop_t *) malloc(sizeof(noop_t));

    if (callback == NULL)
        return NULL;

    push_callback_init(&callback->base,
                       noop_activate,
                       noop_process_bytes,
                       NULL);

    return &callback->base;
}
