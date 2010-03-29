/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_CONTINUATIONS_H
#define PUSH_CONTINUATIONS_H

/**
 * @file
 *
 * This file defines the helper macros that are used to define and
 * call <i>continuations</i>.
 */

#include <push/debug.h>


/**
 * Call a continuation.  Since this is implemented as a macro, and we
 * use the same field names in all of our continuation objects, we
 * don't need a separate activation function for each kind of
 * continuation.
 *
 * @param continuation A pointer to a continuation object.
 */

#if PUSH_CONTINUATION_DEBUG && PUSH_DEBUG

#define push_continuation_call(continuation, ...)               \
    (                                                           \
        PUSH_DEBUG_MSG("continuation: Calling continuation "    \
                       "%s[%p].\n",                             \
                       (continuation)->name,                    \
                       (continuation)->user_data),              \
        (continuation)->func((continuation)->user_data,         \
                             __VA_ARGS__)                       \
    )

#else

#define push_continuation_call(continuation, ...)   \
    ((continuation)->func((continuation)->user_data, __VA_ARGS__))

#endif


/**
 * Sets the function and user data portions of a callback.
 *
 * @param continuation A pointer to a continuation object.
 */

#if PUSH_CONTINUATION_DEBUG

#define push_continuation_set(continuation, the_func, the_user_data)    \
    do {                                                                \
        (continuation)->name = #the_func;                               \
        (continuation)->func = (the_func);                              \
        (continuation)->user_data = (the_user_data);                    \
    } while(0)

#else

#define push_continuation_set(continuation, the_func, the_user_data)    \
    do {                                                                \
        (continuation)->func = (the_func);                              \
        (continuation)->user_data = (the_user_data);                    \
    } while(0)

#endif


/**
 * Defines a continuation object.  Given the base name
 * <code>foo</code>, it constructs a type called
 * <code>push_foo_continuation_t</code>, with the appropriate
 * <code>func</code> and <code>user_data</code> fields.  It expects
 * there to already be a type called <code>push_foo_func_t</code>,
 * which the function type (not function pointer type) that represents
 * the continuation's underlying function.  The function should take
 * in a user_data pointer as its first parameter.
 */

#if PUSH_CONTINUATION_DEBUG

#define PUSH_DEFINE_CONTINUATION(base_name)             \
    typedef struct _push_##base_name##_continuation     \
    {                                                   \
        const char  *name;                              \
        push_##base_name##_func_t  *func;               \
        void  *user_data;                               \
    } push_##base_name##_continuation_t

#else

#define PUSH_DEFINE_CONTINUATION(base_name)             \
    typedef struct _push_##base_name##_continuation     \
    {                                                   \
        push_##base_name##_func_t  *func;               \
        void  *user_data;                               \
    } push_##base_name##_continuation_t

#endif


#endif  /* PUSH_CONTINUATIONS_H */
