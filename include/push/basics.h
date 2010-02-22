/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_BASICS_H
#define PUSH_BASICS_H

/**
 * @file
 *
 * This file defines the basic type that each parser callback must
 * implement (push_callback_t), and the high-level type that most user
 * code will use (push_parser_t).
 */

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef PUSH_DEBUG
#define PUSH_DEBUG 0
#endif

#ifndef PUSH_CONTINUATION_DEBUG
#define PUSH_CONTINUATION_DEBUG 0
#endif

#if PUSH_DEBUG

#include <stdio.h>
#define PUSH_DEBUG_MSG(...) fprintf(stderr, __VA_ARGS__)

#if PUSH_CONTINUATION_DEBUG
#define PUSH_CONTINUATION_DEBUG_MSG(...) fprintf(stderr, __VA_ARGS__)
#else
#define PUSH_CONTINUATION_DEBUG_MSG(...) /* skipping debug message */
#endif

#else
#define PUSH_DEBUG_MSG(...) /* skipping debug message */
#define PUSH_CONTINUATION_DEBUG_MSG(...) /* skipping debug message */
#endif


/**
 * Error codes that can be returned by a callback's process_bytes
 * function and the push_parser_submit_data() function.
 */

typedef enum
{
    /**
     * Indicates that all of the bytes were processed successfully.
     */

    PUSH_SUCCESS = 0,

    /**
     * Indicates that the parse has succeeded so far, but that there
     * wasn't enough input to finish parsing.
     */

    PUSH_INCOMPLETE = -1,

    /**
     * Indicates that the data was invalid in some way.
     */

    PUSH_PARSE_ERROR = -2,

    /**
     * Indicates that there was a problem allocating memory during
     * parsing.
     */

    PUSH_MEMORY_ERROR = -3,

    /**
     * A special code that is used within compound callbacks to
     * indicate that a child callback generated a parse error.  This
     * shouldn't ever be a visible result from any of the public API
     * functions.  It's used when a compound callback needs to detect
     * a child's failure, and translate that into a success or failure
     * of the outer callback.
     */

    PUSH_INNER_PARSE_ERROR = -4,

} push_error_code_t;


/*----------------------------------------------------------------------*/
/** @section continuations Continuation functions */


/**
 * Call a continuation.  Since this is implemented as a macro, and we
 * use the same field names in all of our continuation objects, we
 * don't need a separate activation function for each kind of
 * continuation.
 *
 * @param continuation A pointer to a continuation object.
 */

#if PUSH_CONTINUATION_DEBUG

#define push_continuation_call(continuation, ...)               \
    do {                                                        \
        PUSH_DEBUG_MSG("continuation: Calling continuation "    \
                       "%s[%p].\n",                             \
                       (continuation)->name,                    \
                       (continuation)->user_data);              \
        (continuation)->func((continuation)->user_data,         \
                             __VA_ARGS__);                      \
    } while(0)

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
 * <code>func</code> and <code>user_data</code> fields.
 */

#if PUSH_CONTINUATION_DEBUG

#define PUSH_DEFINE_CONTINUATION(base_name)             \
    typedef struct _push_##base_name##_continuation     \
    {                                                   \
        const char  *name;                              \
        push_##base_name##_continuation_func_t  *func;  \
        void  *user_data;                               \
    } push_##base_name##_continuation_t

#else

#define PUSH_DEFINE_CONTINUATION(base_name)             \
    typedef struct _push_##base_name##_continuation     \
    {                                                   \
        push_##base_name##_continuation_func_t  *func;  \
        void  *user_data;                               \
    } push_##base_name##_continuation_t

#endif


/**
 * A success continuation function.  Will be called by a callback when
 * it has successfully read a value.  This includes a pointer to any
 * remaining data that's currently available.
 */

typedef void
push_success_continuation_func_t(void *user_data,
                                 void *result,
                                 const void *buf,
                                 size_t bytes_remaining);

PUSH_DEFINE_CONTINUATION(success);


/**
 * A continue continuation function.  Will be called by the parser
 * when more data becomes available.  This function will be called
 * with a NULL buf and 0 bytes_remaining when the end of the stream is
 * reached.
 */

typedef void
push_continue_continuation_func_t(void *user_data,
                                  const void *buf,
                                  size_t bytes_remaining);

PUSH_DEFINE_CONTINUATION(continue);


/**
 * An incomplete continuation function.  Will be called by a callback
 * when it has exhausted the available data.  It should provide a
 * continue continuation that should be used to resume processing when
 * more data becomes available.
 */

typedef void
push_incomplete_continuation_func_t(void *user_data,
                                    push_continue_continuation_t *cont);

PUSH_DEFINE_CONTINUATION(incomplete);


/**
 * An error continuation function.  Will be called by a callback when
 * it encounters a error while parsing.  This can be a parse error —
 * indicating that the stream contains invalid data — or a fatal error
 * — indicating that some exceptional circumstance occurred that
 * prevents the parse from continuing.  (This might be a failed
 * malloc, for instance.)
 */

typedef void
push_error_continuation_func_t(void *user_data,
                               push_error_code_t error_code,
                               const char *error_message);

PUSH_DEFINE_CONTINUATION(error);


/**
 * @brief A callback object.
 *
 * This type encapsulates all of the functions that cooperate to
 * implement a parser callback.  The callback must implement an
 * “activation” function that is called to seed the callback with its
 * input value, along with an optional initial chunk of data.  Most
 * callbacks will also implement a “continue” continuation; this
 * continuation is used if the callback exhausts the current chunk of
 * data, and will be used to resume the callback when the next chunk
 * becomes available.
 *
 * Most callbacks will also have a “user data” struct, which they can
 * use to store any additional state needed during the execution of
 * the callback.  There isn't a field in push_callback_t for this
 * struct, though; instead, you put the user data pointer into the
 * activation continuation, and make sure to place it in any other
 * continuations that the callback implements.
 *
 * Finally, the callback will have other continuations provided to it;
 * these should be called when the parsing is complete.  The different
 * continuations handle the different reasons that the parsing can
 * complete: a successful parse, an error, or an “incomplete”.  (An
 * incomplete is when the callback runs out of data in the current
 * chunk before finishing its parse.)  Callbacks should include the
 * appropriate continuation objects in their user data structs, and
 * set the pointers in the push_callback_t struct to point to these
 * objects.  If the pointers are NULL, this indicates that the
 * callback will never try to call that continuation.
 */

typedef struct _push_callback
{
    /**
     * Called by the push parser when this callback is activated.
     * There may not be data available yet; a NULL buf pointer or a 0
     * bytes_remaining does <b>not</b> signify end-of-stream.
     */

    push_success_continuation_t  activate;

    /**
     * A pointer to the success continuation that the callback will
     * call when the parse succeeds.
     */

    push_success_continuation_t  **success;

    /**
     * A pointer to the incomplete continuation that the callback will
     * call when the parse succeeds.
     */

    push_incomplete_continuation_t  **incomplete;

    /**
     * A pointer to the error continuation that the callback will
     * call when the parse succeeds.
     */

    push_error_continuation_t  **error;

} push_callback_t;


/**
 * @brief Create a new push parser callback.
 *
 * @return NULL if we can't create the new callback object.
 */

push_callback_t *
push_callback_new();


/**
 * Free a callback object.
 */

void
push_callback_free(push_callback_t *callback);


/**
 * @brief A push parser.
 */

typedef struct _push_parser
{
    /**
     * The activation function of the parser's top-level callback.
     *
     * @private
     */

    push_success_continuation_t  *activate;

    /**
     * The continue continuation that should receive the next chunk of
     * data.  This will only be set when a callback reaches the end of
     * an input chunk.
     */

    push_continue_continuation_t  *cont;

    /**
     * The success/failure code of processing the most recent chunk of
     * data.  This will be set by the parser's success, incomplete,
     * and error continuations.
     */

    push_error_code_t  result_code;

    /**
     * The final result of a successful parse.  This will be set by
     * the parser's success continuation.
     */

    void  *result;

    /**
     * A success continuation that sets the final result of the parse.
     * This will be the success continuation for the “last” callback
     * in the parser.
     */

    push_success_continuation_t  success;

    /**
     * An incomplete continuation that marks which callback should
     * receive the next chunk of data that becomes available.  This
     * will be the incomplete continuation for all of the callbacks in
     * the parser.
     */

    push_incomplete_continuation_t  incomplete;

    /**
     * A error continuation that records information about the error.
     * This will be the error continuation for the most callbacks in
     * the parser — though some combinators will be able to recover
     * from certain errors in their wrapped callbacks.
     */

    push_error_continuation_t  error;

    /**
     * A continue continuation that will ignore any remaining input.
     * This will be used after the top-level callback finishes, so
     * that we can silently ignore any remaining data in the stream.
     */

    push_continue_continuation_t  ignore;

} push_parser_t;


/**
 * Create a new push parser.
 *
 * @return NULL if we can't create the new parser.
 */

push_parser_t *
push_parser_new();


/**
 * Free a push parser, including its callback.
 */

void
push_parser_free(push_parser_t *parser);


/**
 * Set the top-level callback for a parser.
 *
 * @param callback The parser's initial callback.
 */

void
push_parser_set_callback(push_parser_t *parser,
                         push_callback_t *callback);


/**
 * Activates a push parser, by activating its callback.  This function
 * doesn't prevent you from activating the callback twice, so if this
 * is important, don't do it!
 *
 * @param parser The push parser
 *
 * @param input The input parameter to the callback's activation
 *     function.
 *
 * @return The result of the callback's activation function.  This
 *     will be an error code if the activation failed for some reason.
 */

push_error_code_t
push_parser_activate(push_parser_t *parser,
                     void *input);


/**
 * Return the “result” of the parse.  The contents of this will be
 * callback-specific.  Some callbacks might not return anything, in
 * which case the value returned will be undefined.
 *
 * @param parser The push parser
 *
 * @return The result value of the parser's callback.
 */

#define push_parser_result(parser, type) ((type *) (parser)->result)


/**
 * Submit some data to the push parser for processing.  If there is an
 * error during processing, a negative error code (of type
 * push_error_code_t) will be returned.
 *
 * @param parser The push parser
 *
 * @param buf The buffer of bytes to present to the callback.  This
 *     buffer will not be modified during parsing.
 *
 * @param bytes_available The number of bytes present in buf.
 *
 * @result A push_error_code_t indicating whether parsing has
 *     succeeded or not.  If the buffer didn't provide enough data for
 *     a full parse, we'll return PUSH_INCOMPLETE.
 */

push_error_code_t
push_parser_submit_data(push_parser_t *parser,
                        const void *buf,
                        size_t bytes_available);


/**
 * Notify the push parser that there are no more bytes left to
 * process.
 *
 * @param parser The push parser
 *
 * @result A push_error_code_t indicating whether parsing has
 *     succeeded or not.
 */

push_error_code_t
push_parser_eof(push_parser_t *parser);


#endif  /* PUSH_BASICS_H */
