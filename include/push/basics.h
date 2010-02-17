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

#if PUSH_DEBUG
#include <stdio.h>
#define PUSH_DEBUG_MSG(...) fprintf(stderr, __VA_ARGS__)
#else
#define PUSH_DEBUG_MSG(...) /* skipping debug message */
#endif


/* forward declarations */

typedef struct _push_parser push_parser_t;
typedef struct _push_callback push_callback_t;


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


/**
 * Called by a push parser when a callback is <i>activated</i> — that
 * is, just before it is first passed in any data.  This allows the
 * callback to perform any necessary initialization before it is given
 * data to process.
 *
 * @param parser The push parser that initiated the call.
 *
 * @param callback The parser's callback object.
 *
 * @param input An optional input value.  Each type of callback uses
 *     this parameter differently (or not at all).
 *
 * @result A push_error_code_t value that indicates whether there were
 * any problems activating the callback.
 */

typedef push_error_code_t
push_activate_func_t(push_parser_t *parser,
                     push_callback_t *callback,
                     void *input);


/**
 * Called by a push parser when bytes are available to be processed.
 * The bytes should be handled accordingly.  The number of bytes
 * presented to the callback might be less than what is required; in
 * this case, the function should process the partial data as needed,
 * and leave its internal state such that it can resume parsing on the
 * next call.  The function should signal this case by returning
 * PUSH_INCOMPLETE.
 *
 * The number of bytes will be 0 only at the end of the input.  The
 * callback should return a PUSH_PARSE_ERROR if this callback has only
 * parsed a partial value to this point.
 *
 * It might be the case that the callback doesn't need all of the data
 * to parse successfully.  The function can signal this by returning a
 * non-zero value; this many bytes will be left in the parsing buffer.
 * This is useful when using the parser combinators to create compound
 * parsers, since the leftover bytes will be passed in to the next
 * parser in the chain.
 *
 * If the callback parses successfully, it can optionally return a
 * value by assigning to the result pointer in the push_callback_t
 * object.
 *
 * If there is a parse error while processing these bytes, the
 * function should return PUSH_PARSE_ERROR.  If the callback needs to
 * allocate some memory, but cannot, it should return
 * PUSH_MEMORY_ERROR.
 *
 * @param parser The push parser that initiated the call.
 *
 * @param callback The parser's callback object.
 *
 * @param buf Points at the start of the data that should be
 * processed.
 *
 * @param bytes_available The number of bytes that are available to be
 * processed.  The function should only read this many bytes from buf.
 *
 * @return The number of bytes left over after successfully processing
 * the data, or a push_error_code_t value if there is an error while
 * processing the data.
 */

typedef ssize_t
push_process_bytes_func_t(push_parser_t *parser,
                          push_callback_t *callback,
                          const void *buf,
                          size_t bytes_available);


/**
 * Called when a push_callback_t instance needs to be freed.  If there
 * are any additional fields in the callback object, they can be freed
 * as needed.  This function should <b>not</b> free the callback itself.
 *
 * @param callback The parser's callback object.
 */

typedef void
push_callback_free_func_t(push_callback_t *callback);


/**
 * @brief A callback object.
 *
 * This type encapsulates all of the functions that the push parser
 * might call, along with any additional data needed by the callback.
 * Callbacks will usually be implemented by a new type that has a
 * push_callback_t as its first element; this ensures that a pointer
 * to the more detailed type can be passed in whenever a
 * push_callback_t pointer is expected.
 */

struct _push_callback
{
    /**
     * Called by the push parser when this callback is activated.
     */

    push_activate_func_t * const  activate;

    /**
     * Called by the push parser when bytes are available for
     * processing.
     */

    push_process_bytes_func_t * const  process_bytes;

    /**
     * Called by the push_callback_free() function to free any
     * additional data used by this callback.
     */

    push_callback_free_func_t  *free;

    /**
     * The result of the callback.
     */

    void  *result;

    /**
     * A flag indicating that we've started freeing this callback.
     * Often there will be cycles of pointers amongst a group of
     * callback objects.  This flag allows each callback's free
     * function to blindly call push_callback_free() on any other
     * callbacks that its linked to — we will take care of ensuring
     * that any particular callback is only freed once, even in the
     * prescence of circular references.
     */

    bool  freeing;
};


/**
 * @brief Create a new push parser callback.
 *
 * The new callback will use the process_bytes function to process the
 * data received by the push parser.
 *
 * This function should only be used if the process_bytes function
 * doesn't need to store any additional data in the callback object.
 * In most cases, each kind of callback will define its own
 * specialized “subclass” of the push_callback_t type; for these
 * callbacks, you should call the constructor for that type, rather
 * than push_callback_new().
 *
 * @return NULL if we can't create the new callback object.
 */

push_callback_t *
push_callback_new(push_activate_func_t *activate,
                  push_process_bytes_func_t *process_bytes);


/**
 * @brief Initialize the push_callback_t portion of a specialized
 * callback type.
 *
 * This function shouldn't be called directly by users; it's used by
 * specialized callback initializers to make sure that the
 * push_callback_t “superclass” portion of the <code>struct</code> is
 * initialized consistently.
 */

void
push_callback_init(push_callback_t *callback,
                   push_activate_func_t *activate,
                   push_process_bytes_func_t *process_bytes,
                   push_callback_free_func_t *free);

/**
 * @brief Initialize the push_callback_t portion of a specialized
 * callback type.
 *
 * This function shouldn't be called directly by users; it's used by
 * specialized callback initializers to make sure that the
 * push_callback_t “superclass” portion of the <code>struct</code> is
 * initialized consistently.
 *
 * This is simply a static initializer version of push_callback_init.
 */

#define PUSH_CALLBACK_INIT(activate, process_bytes, free) \
    { (activate), (process_bytes), (free), NULL, false }


/**
 * Free a callback object.
 */

void
push_callback_free(push_callback_t *callback);


/**
 * Activates a callback.  This function doesn't prevent you from
 * activating the callback twice, so if this is important, don't do
 * it!
 *
 * @param parser The top-level parser object.
 *
 * @param callback The callback object.
 *
 * @param input The input parameter to the callback's activation
 *     function.
 *
 * @return The result of the callback's activation function.  This
 *     will be an error code if the activation failed for some reason.
 */

push_error_code_t
push_callback_activate(push_parser_t *parser,
                       push_callback_t *callback,
                       void *input);


/**
 * Calls a callback's process_bytes function.
 */

ssize_t
push_callback_process_bytes(push_parser_t *parser,
                            push_callback_t *callback,
                            const void *buf,
                            size_t bytes_available);


/**
 * “Tail-calls” a child callback on behalf of a parent callback.
 * Calls the child's process_bytes function as usual; however, if it
 * succeeds, we copy its result into the result of the parent before
 * returning.
 */

ssize_t
push_callback_tail_process_bytes(push_parser_t *parser,
                                 push_callback_t *parent,
                                 push_callback_t *child,
                                 const void *buf,
                                 size_t bytes_available);


/**
 * @brief A push parser.
 */

struct _push_parser
{
    /**
     * The parser's callback.  Whenever data is available, it will be
     * passed into this callback's process_bytes function.
     *
     * @private
     */

    push_callback_t  *callback;

    /**
     * Once we've successfully parsed a value, we need to ignore any
     * remaining data.  This flag indicates that a previous call to
     * push_parser_submit_bytes resulted in a successful parse, and
     * therefore that we should ignore any data from further calls.
     */

    bool  finished;
};


/**
 * Create a new push parser.
 *
 * @param callback The parser's initial callback.
 *
 * @return NULL if we can't create the new parser.
 */

push_parser_t *
push_parser_new(push_callback_t *callback);


/**
 * Free a push parser, including its callback.
 */

void
push_parser_free(push_parser_t *parser);


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

void *
push_parser_result(push_parser_t *parser);


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
