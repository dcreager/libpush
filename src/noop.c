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
 * The user data struct for a noop callback.
 */

typedef struct _noop
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

} noop_t;


static void
noop_activate(void *user_data,
              void *result,
              const void *buf,
              size_t bytes_remaining)
{
    noop_t  *noop = (noop_t *) user_data;

    PUSH_DEBUG_MSG("noop: Activating.\n");

    /*
     * Immediately succeed with the same input value.
     */

    push_continuation_call(noop->callback.success,
                           result,
                           buf,
                           bytes_remaining);

    return;
}


push_callback_t *
push_noop_new(push_parser_t *parser)
{
    noop_t  *noop = (noop_t *) malloc(sizeof(noop_t));

    if (noop == NULL)
        return NULL;

    push_callback_init(&noop->callback, parser,
                       noop_activate, noop);

    return &noop->callback;
}
