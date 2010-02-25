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
#include <string.h>

#include <push/basics.h>
#include <push/talloc.h>

#if PUSH_FREE
#define PUSH_FREE_MSG(...) PUSH_DEBUG_MSG(__VA_ARGS__)
#else
#define PUSH_FREE_MSG(...) /* skipping debug message */
#endif


static void
default_set_success(void *user_data,
                    push_success_continuation_t *success)
{
    push_callback_t  *callback = (push_callback_t *) user_data;
    callback->success = success;
}


static void
default_set_incomplete(void *user_data,
                       push_incomplete_continuation_t *incomplete)
{
    push_callback_t  *callback = (push_callback_t *) user_data;
    callback->incomplete = incomplete;
}


static void
default_set_error(void *user_data,
                  push_error_continuation_t *error)
{
    push_callback_t  *callback = (push_callback_t *) user_data;
    callback->error = error;
}


void
_push_callback_init
(push_callback_t *callback,
 push_parser_t *parser,
 const char *user_data_name,
 void *user_data,
 const char *activate_name,
 push_success_func_t *activate_func,
 const char *set_success_name,
 push_set_success_func_t *set_success_func,
 const char *set_incomplete_name,
 push_set_incomplete_func_t *set_incomplete_func,
 const char *set_error_name,
 push_set_error_func_t *set_error_func)
{
    /*
     * Fill in the callback's activate continuation object.
     */

    push_continuation_set(&callback->activate,
                          activate_func,
                          user_data);

#if PUSH_CONTINUATION_DEBUG
    /*
     * We have to override the name of the continuation functions; the
     * default implementation of the push_continuation_set macro will
     * always call it “activate_func”, etc.
     */

    callback->activate.name = activate_name;
#endif


    if (set_success_func == NULL)
    {
        push_continuation_set(&callback->set_success,
                              default_set_success, callback);

#if PUSH_CONTINUATION_DEBUG
        {
            const char  *name =
                push_talloc_asprintf(callback, "%s%s",
                                     user_data_name, "_set_success");

            if (name != NULL)
                callback->set_success.name = name;
        }
#endif
    } else {
        push_continuation_set(&callback->set_success,
                              set_success_func,
                              user_data);

#if PUSH_CONTINUATION_DEBUG
        callback->set_success.name = set_success_name;
#endif
    }


    if (set_incomplete_func == NULL)
    {
        push_continuation_set(&callback->set_incomplete,
                              default_set_incomplete, callback);

#if PUSH_CONTINUATION_DEBUG
        {
            const char  *name =
                push_talloc_asprintf(callback, "%s%s",
                                     user_data_name, "_set_incomplete");

            if (name != NULL)
                callback->set_incomplete.name = name;
        }
#endif
    } else {
        push_continuation_set(&callback->set_incomplete,
                              set_incomplete_func,
                              user_data);

#if PUSH_CONTINUATION_DEBUG
        callback->set_incomplete.name = set_incomplete_name;
#endif
    }


    if (set_error_func == NULL)
    {
        push_continuation_set(&callback->set_error,
                              default_set_error, callback);

#if PUSH_CONTINUATION_DEBUG
        {
            const char  *name =
                push_talloc_asprintf(callback, "%s%s",
                                     user_data_name, "_set_error");

            if (name != NULL)
                callback->set_error.name = name;
        }
#endif
    } else {
        push_continuation_set(&callback->set_error,
                              set_error_func,
                              user_data);

#if PUSH_CONTINUATION_DEBUG
        callback->set_error.name = set_error_name;
#endif
    }

    /*
     * By default, we call the parser's implementations of the
     * continuations that we call.
     */

    push_continuation_call(&callback->set_success,
                           &parser->success);

    push_continuation_call(&callback->set_incomplete,
                           &parser->incomplete);

    push_continuation_call(&callback->set_error,
                           &parser->error);
}
