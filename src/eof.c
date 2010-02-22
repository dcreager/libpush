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
 * The user data struct for an EOF callback.
 */

typedef struct _eof
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The continue continuation that will resume the EOF parser.
     */

    push_continue_continuation_t  cont;

    /**
     * A copy of the input pointer.  If our parse succeeds, we use it
     * as our output, as well.
     */

    void  *input;

} eof_t;


static void
eof_activate(void *user_data,
             void *result,
             const void *buf,
             size_t bytes_remaining)
{
    eof_t  *eof = (eof_t *) user_data;

    PUSH_DEBUG_MSG("eof: Activating.\n");
    eof->input = result;

    /*
     * Any data results in a parse error.
     */

    if (bytes_remaining > 0)
    {
        PUSH_DEBUG_MSG("eof: Expected EOF, but got %zu bytes.\n",
                       bytes_remaining);

        push_continuation_call(eof->callback.error,
                               PUSH_PARSE_ERROR,
                               "Expected EOF, but got data");

        return;
    }

    /*
     * Otherwise, we need to register a continue continuation.
     */

    push_continuation_call(eof->callback.incomplete,
                           &eof->cont);
}


static void
eof_continue(void *user_data,
             const void *buf,
             size_t bytes_remaining)
{
    eof_t  *eof = (eof_t *) user_data;

    /*
     * Any data results in a parse error.
     */

    if (bytes_remaining > 0)
    {
        PUSH_DEBUG_MSG("eof: Expected EOF, but got %zu bytes.\n",
                       bytes_remaining);

        push_continuation_call(eof->callback.error,
                               PUSH_PARSE_ERROR,
                               "Expected EOF, but got data");

        return;
    } else {
        PUSH_DEBUG_MSG("eof: Reached expected EOF.\n");

        push_continuation_call(eof->callback.success,
                               eof->input,
                               buf, bytes_remaining);

        return;
    }
}


push_callback_t *
push_eof_new(push_parser_t *parser)
{
    eof_t  *eof = (eof_t *) malloc(sizeof(eof_t));

    if (eof == NULL)
        return NULL;

    push_callback_init(&eof->callback, parser,
                       eof_activate, eof);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&eof->cont,
                          eof_continue,
                          eof);

    return &eof->callback;
}
