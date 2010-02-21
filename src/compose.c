/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stddef.h>
#include <stdlib.h>

#include <push/basics.h>
#include <push/combinators.h>


push_callback_t *
push_compose_new(push_parser_t *parser,
                 push_callback_t *first,
                 push_callback_t *second)
{
    push_callback_t  *callback;

    callback = push_callback_new();
    if (callback == NULL)
        return NULL;

    /*
     * The compose is activated by activating the first wrapped
     * callback.
     */

    callback->activate = first->activate;

    /*
     * The first wrapped callback should succeed by activating the
     * second callback; it should fail and incomplete as it would have
     * if it weren't wrapped in a compose.
     */

    if (first->success != NULL)
        *first->success = &second->activate;

    /*
     * The compose succeeds when the second wrapped callback succeeds.
     * It cannot incomplete or fail on its own — only its wrapped
     * callbacks can.
     */

    callback->success = second->success;
    callback->incomplete = NULL;
    callback->error = NULL;

    return callback;
}
