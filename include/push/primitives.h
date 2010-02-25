/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PRIMITIVES_H
#define PUSH_PRIMITIVES_H

/**
 * @file
 *
 * This file defines the built-in primitive parser callbacks.  These
 * callbacks operator on their own, and do not wrap any other
 * callbacks.
 */

#include <hwm-buffer.h>

#include <push/basics.h>


/**
 * Create a new callback that requires the end of the stream.  If any
 * data is present, it results in a parse error.
 */

push_callback_t *
push_eof_new(const char *name,
             void *parent,
             push_parser_t *parser);


/**
 * Create a new callback that reads in a fixed amount of data into a
 * buffer.  This can be used to read constant-sized data structures,
 * for instance.  The result pointer is allowed to point directly into
 * the data chunk, so the caller should copy this into a separate
 * buffer if the data needs to be available across multiple data
 * chunks.
 */

push_callback_t *
push_fixed_new(const char *name,
               void *parent,
               push_parser_t *parser,
               size_t size);


/**
 * Create a new callback that reads a string into a high-water mark
 * buffer.  This callback doesn't do anything to determine the length
 * of the string; instead, it takes in a pointer to a size_t as input,
 * and uses that as the length of the string.  The callback's result
 * will be a pointer to the data in the HWM buffer — not a pointer to
 * the hwm_buffer_t object itself.  We will ensure that there is a NUL
 * pointer at the end of the string.
 */

push_callback_t *
push_hwm_string_new(const char *name,
                    void *parent,
                    push_parser_t *parser,
                    hwm_buffer_t *buf);


/**
 * Create a new callback that does nothing.  It parses no data, and
 * copies its input to its output.
 */

push_callback_t *
push_noop_new(const char *name,
              void *parent,
              push_parser_t *parser);


/**
 * A function that can be wrapped into a callback using push_pure_new
 * or push_pure_cont_new.
 */

typedef bool
push_pure_func_t(void *user_data,
                 void *input,
                 void **output);

PUSH_DEFINE_CONTINUATION(pure);


/**
 * Create a new callback that applies the given function to its input,
 * and immediately succeeds with the function's result as the
 * callback's output.  You provide a pointer to the function, and a
 * user data struct that will be passed in to the function along with
 * the input value.
 */

push_callback_t *
push_pure_new(const char *name,
              void *parent,
              push_parser_t *parser,
              push_pure_func_t *func,
              void *user_data);


/**
 * Create a new callback that applies the given function to its input,
 * and immediately succeeds with the function's result as the
 * callback's output.  You provide a pointer to the function.  This
 * version allocates a user data struct for you; you pass in the size.
 * A pointer to the new user data struct is placed in *user_data, if
 * user_data isn't NULL.
 */

push_callback_t *
push_pure_data_new(const char *name,
                   void *parent,
                   push_parser_t *parser,
                   push_pure_func_t *func,
                   void **user_data,
                   size_t user_data_size);


/**
 * Create a new callback that applies the given function to its input,
 * and immediately succeeds with the function's result as the
 * callback's output.  You provide a pointer to a continuation object,
 * which encapsulates the function pointer and a user data struct.
 */

push_callback_t *
push_pure_cont_new(const char *name,
                   void *parent,
                   push_parser_t *parser,
                   push_pure_continuation_t *cont);


/**
 * Create a new callback that skips the specified number of bytes.
 * The callback's input should be a pointer to a size_t, indicating
 * the number of bytes to skip.
 */

push_callback_t *
push_skip_new(const char *name,
              void *parent,
              push_parser_t *parser);


#endif  /* PUSH_PRIMITIVES_H */
