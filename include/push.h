/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_H
#define PUSH_H

/**
 * @file
 *
 * When using the push parser, you provide a <i>callback</i> that is
 * used to process the bytes in the input buffer.  This callback is
 * defined by the push_callback_t type.
 */

#include <stdbool.h>
#include <stdlib.h>

#include <hwm-buffer.h>

#define PUSH_DEBUG 0

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
 * Called by a push parser when bytes are available to be processed.
 * The bytes should be handled accordingly.  The number of bytes
 * presented to the callback might be less than what was requested; in
 * this case, the function should process the partial data as needed.
 *
 * The function can use the push_parser_set_callback() function to
 * change the parser's current callback; this callback will be used to
 * process the next data that becomes available.  When doing this, it
 * might be the case that some of the data that was given to
 * <i>this</i> callback should actually be passed on to the
 * <i>next</i> callback.  The function can signal this by returning a
 * non-zero value; this many bytes will be passed on to the parser's
 * next callback function.
 *
 * Note that if you don't change the parser's callback, you shouldn't
 * return a non-zero value, since that data will just be immediately
 * passed back into the same callback function.  This will most likely
 * result in an infinite loop.
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
 * @return The number of bytes successfully processed.
 */

typedef size_t
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
     * The minimum number of bytes that will be passed into the next
     * call to process_bytes.  If there aren't that many bytes
     * available, the push parser will buffer the bytes until they are
     * available.
     */

    size_t  min_bytes_requested;

    /**
     * The maximum number of bytes that will be passed into the next
     * call to process_bytes.  If this is 0, process_bytes will get
     * however bytes are available.
     */

    size_t  max_bytes_requested;

    /**
     * Called by the push parser when bytes are available for
     * processing.
     */

    push_process_bytes_func_t  *process_bytes;

    /**
     * Called by the push_callback_free() function to free any
     * additional data used by this callback.
     */

    push_callback_free_func_t  *free;

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
push_callback_new(push_process_bytes_func_t *process_bytes,
                  size_t min_bytes_requested,
                  size_t max_bytes_requested);


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
                   size_t min_bytes_requested,
                   size_t max_bytes_requested,
                   push_process_bytes_func_t *process_bytes,
                   push_callback_free_func_t *free);


/**
 * Free a callback object.
 */

void
push_callback_free(push_callback_t *callback);


/**
 * @brief A push parser.
 */

struct _push_parser
{
    /**
     * The parser's current callback.  Whenever data is available, it
     * will be passed into this callback's process_bytes function.
     *
     * @private
     */

    push_callback_t  *current_callback;

    /**
     * A buffer that stores data that has not yet been processed by
     * the callback.
     */

    hwm_buffer_t  leftover;
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
 * Change the callback that the push parser will use for the next bit
 * of data.  This can safely be called from within another callback's
 * process_bytes function.
 */

void
push_parser_set_callback(push_parser_t *parser,
                         push_callback_t *callback);


/**
 * Submit some data to the push parser for processing.
 */

bool
push_parser_submit_data(push_parser_t *parser,
                        const void *buf,
                        size_t bytes_available);


#endif  /* PUSH_H */
