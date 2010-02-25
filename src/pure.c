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
#include <push/talloc.h>


/**
 * The user data struct for a pure callback.
 */

typedef struct _pure
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The function that we will call for each input received.
     */

    push_pure_continuation_t  pure;

} pure_t;


static void
pure_activate(void *user_data,
              void *input,
              const void *buf,
              size_t bytes_remaining)
{
    pure_t  *pure = (pure_t *) user_data;
    void  *output;

    /*
     * Send the input value to the pure function, and output its
     * result.
     */

    if (push_continuation_call(&pure->pure, input, &output))
    {
        push_continuation_call(pure->callback.success,
                               output,
                               buf, bytes_remaining);

        return;
    } else {
        push_continuation_call(pure->callback.error,
                               PUSH_PARSE_ERROR,
                               "Pure function failed");

        return;
    }
}


push_callback_t *
push_pure_new(const char *name,
              void *parent,
              push_parser_t *parser,
              push_pure_func_t *func,
              void *user_data)
{
    pure_t  *pure;

    /*
     * Allocate the pure callback's user data struct.
     */

    pure = push_talloc(parent, pure_t);
    if (pure == NULL) return NULL;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL) name = "pure";
    push_talloc_set_name_const(pure, name);

    push_callback_init(&pure->callback, parser, pure,
                       pure_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation that we'll use to call the wrapped
     * function.
     */

    push_continuation_set(&pure->pure, func, user_data);

    return &pure->callback;
}


push_callback_t *
push_pure_data_new(const char *name,
                   void *parent,
                   push_parser_t *parser,
                   push_pure_func_t *func,
                   void **user_data,
                   size_t user_data_size)
{
    void  *context;
    pure_t  *pure;
    void  *data;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Allocate the pure callback's user data struct.
     */

    pure = push_talloc(context, pure_t);
    if (pure == NULL) goto error;

    /*
     * Allocate a user data struct for the wrapped function, if
     * needed.
     */

    data = push_talloc_size(context, user_data_size);
    if (data == NULL) goto error;

    /*
     * Save a copy of the new user data struct for the caller, if they
     * want it.
     */

    if (user_data != NULL)
        *user_data = data;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL) name = "pure";
    push_talloc_set_name_const(pure, name);

    push_callback_init(&pure->callback, parser, pure,
                       pure_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation that we'll use to call the wrapped
     * function.
     */

    push_continuation_set(&pure->pure, func, data);

    return &pure->callback;

  error:
    push_talloc_free(context);
    return NULL;
}


push_callback_t *
push_pure_cont_new(const char *name,
                   void *parent,
                   push_parser_t *parser,
                   push_pure_continuation_t *cont)
{
    pure_t  *pure = push_talloc(parent, pure_t);

    if (pure == NULL)
        return NULL;

    if (name == NULL) name = "pure";
    push_talloc_set_name_const(pure, name);

    push_callback_init(&pure->callback, parser, pure,
                       pure_activate,
                       NULL, NULL, NULL);

    pure->pure = (*cont);

    return &pure->callback;
}
