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
#include <push/primitives.h>


/**
 * The push_callback_t subclass that defines a eof callback.
 */

typedef struct _eof
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

} eof_t;


static push_error_code_t
eof_activate(push_parser_t *parser,
             push_callback_t *pcallback,
             void *input)
{
    eof_t  *callback = (eof_t *) pcallback;

    PUSH_DEBUG_MSG("eof: Activating.\n");
    callback->input = input;

    return PUSH_SUCCESS;
}


static ssize_t
eof_process_bytes(push_parser_t *parser,
                  push_callback_t *pcallback,
                  const void *vbuf,
                  size_t bytes_available)
{
    eof_t  *callback = (eof_t *) pcallback;

    /*
     * Any data results in a parse error.
     */

    if (bytes_available > 0)
    {
        PUSH_DEBUG_MSG("eof: Expected EOF, but got %zu bytes.\n",
                       bytes_available);

        return PUSH_PARSE_ERROR;
    } else {
        PUSH_DEBUG_MSG("eof: Reached expected EOF.\n");

        /*
         * Copy the input pointer to the output on success.
         */

        callback->base.result = callback->input;

        return 0;
    }
}


push_callback_t *
push_eof_new()
{
    eof_t  *callback = (eof_t *) malloc(sizeof(eof_t));

    if (callback == NULL)
        return NULL;

    push_callback_init(&callback->base,
                       eof_activate,
                       eof_process_bytes,
                       NULL);

    return &callback->base;
}
