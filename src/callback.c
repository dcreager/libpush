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

#ifndef PUSH_FREE
#define PUSH_FREE 0
#endif

#if PUSH_FREE
#define PUSH_FREE_MSG(...) PUSH_DEBUG_MSG(__VA_ARGS__)
#else
#define PUSH_FREE_MSG(...) /* skipping debug message */
#endif


void
_push_callback_init(push_callback_t *callback,
                    push_parser_t *parser,
                    const char *activate_name,
                    push_success_continuation_func_t *activate_func,
                    void *activate_user_data)
{
    /*
     * Fill in the callback's activate continuation object.
     */

    push_continuation_set(&callback->activate,
                          activate_func,
                          activate_user_data);

#if PUSH_CONTINUATION_DEBUG
    /*
     * We have to override the name of the continuation function; the
     * default implementation of the push_continuation_set macro will
     * always call it “activate_func”.
     */

    callback->activate.name = activate_name;
#endif

    /*
     * By default, we call the parser's implementations of the
     * continuations that we call.
     */

    callback->success = &parser->success;
    callback->incomplete = &parser->incomplete;
    callback->error = &parser->error;
}


void
push_callback_free(push_callback_t *callback)
{
    PUSH_FREE_MSG("callback: %p: Freeing push_callback_t instance.\n",
                  callback);
    free(callback);
}
