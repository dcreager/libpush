/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PURE_H
#define PUSH_PURE_H

#include <push/basics.h>
#include <push/talloc.h>

/**
 * @file
 *
 * This file defines a macro for creating parser callbacks from pure C
 * functions.
 */


#define push_define_pure_data_callback(new_func, pure_func,     \
                                       default_name,            \
                                       input_t,                 \
                                       output_t,                \
                                       user_data_t)             \
    typedef struct _##new_func                                  \
    {                                                           \
        push_callback_t  callback;                              \
        user_data_t  user_data;                                 \
    } new_func##_t;                                             \
                                                                \
    static void                                                 \
    new_func##_activate(void *ud, void *vinput,                 \
                        void *buf,                              \
                        size_t bytes_remaining)                 \
    {                                                           \
        new_func##_t  *pure = (new_func##_t *) ud;              \
        input_t  *input = (input_t *) vinput;                   \
        output_t  *output;                                      \
                                                                \
        if (pure_func(&pure->user_data, input, &output))        \
        {                                                       \
            push_continuation_call(pure->callback.success,      \
                                   output,                      \
                                   buf, bytes_remaining);       \
            return;                                             \
        } else {                                                \
            push_continuation_call(pure->callback.error,        \
                                   PUSH_PARSE_ERROR,            \
                                   "Pure function failed");     \
            return;                                             \
        }                                                       \
    }                                                           \
                                                                \
    static push_callback_t *                                    \
    new_func(const char *name,                                  \
             void *parent,                                      \
             push_parser_t *parser,                             \
             user_data_t **user_data)                           \
    {                                                           \
        new_func##_t  *pure;                                    \
                                                                \
        pure = push_talloc(parent, new_func##_t);               \
        if (pure == NULL) return NULL;                          \
                                                                \
        if (user_data != NULL)                                  \
            *user_data = &pure->user_data;                      \
                                                                \
        if (name == NULL) name = default_name;                  \
        push_talloc_set_name_const(pure, name);                 \
                                                                \
        push_callback_init(&pure->callback, parser, pure,       \
                           new_func##_activate,                 \
                           NULL, NULL, NULL);                   \
                                                                \
        return &pure->callback;                                 \
    }


#define push_define_pure_callback(new_func, pure_func,          \
                                  default_name,                 \
                                  input_t,                      \
                                  output_t,                     \
                                  user_data_t)                  \
    typedef struct _##new_func                                  \
    {                                                           \
        push_callback_t  callback;                              \
        user_data_t  *user_data;                                \
    } new_func##_t;                                             \
                                                                \
    static void                                                 \
    new_func##_activate(void *ud, void *vinput,                 \
                        void *buf,                              \
                        size_t bytes_remaining)                 \
    {                                                           \
        new_func##_t  *pure = (new_func##_t *) ud;              \
        input_t  *input = (input_t *) vinput;                   \
        output_t  *output;                                      \
                                                                \
        if (pure_func(pure->user_data, input, &output))         \
        {                                                       \
            push_continuation_call(pure->callback.success,      \
                                   output,                      \
                                   buf, bytes_remaining);       \
            return;                                             \
        } else {                                                \
            push_continuation_call(pure->callback.error,        \
                                   PUSH_PARSE_ERROR,            \
                                   "Pure function failed");     \
            return;                                             \
        }                                                       \
    }                                                           \
                                                                \
    static push_callback_t *                                    \
    new_func(const char *name,                                  \
             void *parent,                                      \
             push_parser_t *parser,                             \
             user_data_t *user_data)                            \
    {                                                           \
        new_func##_t  *pure;                                    \
                                                                \
        pure = push_talloc(parent, new_func##_t);               \
        if (pure == NULL) return NULL;                          \
                                                                \
        if (name == NULL) name = default_name;                  \
        push_talloc_set_name_const(pure, name);                 \
                                                                \
        push_callback_init(&pure->callback, parser, pure,       \
                           new_func##_activate,                 \
                           NULL, NULL, NULL);                   \
                                                                \
        pure->user_data = user_data;                            \
                                                                \
        return &pure->callback;                                 \
    }


#endif  /* PUSH_PURE_H */
