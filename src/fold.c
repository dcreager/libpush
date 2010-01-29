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


/**
 * The push_callback_t subclass that defines a fold callback.
 */

typedef struct _fold
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * The wrapped callback.
     */

    push_callback_t  *wrapped;

    /**
     * A flag indicating whether we're in the middle of passing data
     * to the wrapped callback.  This is true after a call to the
     * wrapped callback returns PUSH_INCOMPLETE.  Later calls cannot
     * backtrack the data that we already passed in, so if we get a
     * PUSH_PARSE_ERROR after a PUSH_INCOMPLETE, we generate a parse
     * error for the fold.
     */

    bool within_wrapped;

    /**
     * A saved copy of the most recent result.  While we're in the
     * middle of trying the next iteration, we might generate a new
     * result.  If the next iteration fails, then we need to return
     * the result that we had before trying the iteration.
     */

    void  *last_result;

} fold_t;


static push_error_code_t
fold_activate(push_parser_t *parser,
              push_callback_t *pcallback,
              void *input)
{
    fold_t  *callback = (fold_t *) pcallback;

    /*
     * We activate the fold by activating the first callback.  The
     * initial value is our first result; if the first attempt at
     * parsing the wrapped callback fails, it's what we return.
     */

    PUSH_DEBUG_MSG("fold: Saving initial input as most recent result.\n");
    callback->last_result = input;

    PUSH_DEBUG_MSG("fold: Activating wrapped callback.\n");
    return push_callback_activate(parser, callback->wrapped, input);
}


/**
 * Called to process one iteration of the wrapped callback inside of
 * fold_process_bytes.
 *
 * @return A status code that can be used for the overall fold
 * callback.  If we return a success code, this indicates that the
 * inner callback succeeded, and that we should try another iteration.
 * The special PUSH_INNER_PARSE_ERROR code indicates that the wrapped
 * callback failed in such a way that the fold succeeds.
 * PUSH_PARSE_ERROR, on the other hand, indicates that the wrapped
 * callback failed in an illegal way, and that the fold should also
 * fail as a result.
 */

static ssize_t
fold_process_one_iteration(push_parser_t *parser,
                           fold_t *callback,
                           const void *vbuf,
                           size_t bytes_available)
{
    ssize_t  result;

    /*
     * Pass the data into the wrapped callback.  We don't use a tail
     * call, since we don't use the wrapped result directly as the
     * fold result.  (We have to first try another loop iteration, and
     * only return this wrapped result if the next iteration fails.)
     */

    result =
        push_callback_process_bytes(parser, callback->wrapped,
                                    vbuf, bytes_available);

    /*
     * If the parse failed, then the overall fold succeeds — assuming
     * we hadn't already sent some into the callback previously.  Load
     * up the old result as our success result.
     */

    if (result == PUSH_PARSE_ERROR)
    {
        if (callback->within_wrapped)
        {
            /*
             * Boo, this is a parse error after an incomplete.  That's
             * not allowed!  The overall fold gets a parse error.
             */

            PUSH_DEBUG_MSG("fold: Parse error after incomplete.  "
                           "Fold results in a parse error!\n");

            return PUSH_PARSE_ERROR;
        } else {
            /*
             * This is an immediate parse error, which is okay.  The
             * fold is a success, and the result that we had before we
             * tried this iteration is the final result.
             */

            PUSH_DEBUG_MSG("fold: Parse error.  Fold succeeds with "
                           "previous result.\n");
            callback->base.result = callback->last_result;

            /*
             * Use the special “inner parse error” code to let the
             * outer loop know that the wrapped callback failed, but
             * that we should succeed.
             */

            return PUSH_INNER_PARSE_ERROR;
        }
    }

    /*
     * If the wrapped callback succeeds, we should copy its result
     * into our “last_result” pointer.  That way if the next iteration
     * fails, we can return it as our final result.
     */

    if (result >= 0)
    {
        push_error_code_t  activate_result;

        PUSH_DEBUG_MSG("fold: Wrapped callback succeeded using "
                       "%zu bytes.  Saving its result for later.\n",
                       bytes_available - result);
        callback->last_result = callback->wrapped->result;

        /*
         * We should also reactivate the wrapped callback so it's
         * ready for the next iteration.  Its input is the current
         * result.
         */

        activate_result =
            push_callback_activate(parser, callback->wrapped,
                                   callback->last_result);

        if (activate_result != PUSH_SUCCESS)
            return activate_result;
    }

    /*
     * If the callback returned INCOMPLETE, then we should remember
     * that we're in the middle of parsing the callback.
     */

    callback->within_wrapped = (result == PUSH_INCOMPLETE);

    /*
     * Any result that's not a parse error can be used as-is as our
     * result.
     */

    return result;
}


static ssize_t
fold_process_bytes(push_parser_t *parser,
                   push_callback_t *pcallback,
                   const void *vbuf,
                   size_t bytes_available)
{
    fold_t  *callback = (fold_t *) pcallback;

    /*
     * If this is EOF, pass the EOF on to the wrapped callback if
     * we're in the middle of passing it data.  Otherwise, the EOF has
     * come in between iterations of the wrapped callback, which is
     * just fine.
     */

    if (bytes_available == 0)
    {
        if (callback->within_wrapped)
        {
            ssize_t  result;

            PUSH_DEBUG_MSG("fold: Passing EOF on to wrapped callback.\n");
            result =
                push_callback_process_bytes(parser, callback->wrapped,
                                            vbuf, bytes_available);

            /*
             * If the wrapped callback says EOF is okay, then we count
             * this as a successful parse — which means that we need
             * to save the new result as our final result.
             */

            if (result >= 0)
            {
                PUSH_DEBUG_MSG("fold: Final wrapped callback successful "
                               "at EOF.  Saving result.\n");

                callback->base.result = callback->last_result =
                    callback->wrapped->result;
            }

            return result;
        } else {
            /*
             * We're not within the wrapped callback, so the EOF is
             * okay.  Make sure to save our last result as the final
             * result.
             */

            PUSH_DEBUG_MSG("fold: EOF in between iterations.  Fold is "
                           "successful.\n");
            callback->base.result = callback->last_result;

            return PUSH_SUCCESS;
        }
    }

    /*
     * Loop through this for as long as we still have data to process.
     */

    while (bytes_available > 0)
    {
        ssize_t  result;

        PUSH_DEBUG_MSG("fold: Attempting iteration with %zu bytes.%s\n",
                       bytes_available,
                       callback->within_wrapped?
                           "  (within wrapped callback)":
                           "");

        result = fold_process_one_iteration(parser, callback,
                                            vbuf, bytes_available);

        /*
         * If the wrapped callback fails, the fold succeeds.  We'll
         * have already set the result pointer in
         * fold_process_one_iteration.  Our success code should
         * indicate that all of the bytes are still available, since
         * the wrapped callback failed to process any of them.
         */

        if (result == PUSH_INNER_PARSE_ERROR)
        {
            return bytes_available;
        }

        /*
         * Any other failure code should be returned as-is.
         */

        if (result < 0)
        {
            return result;
        }

        /*
         * Otherwise we had a successful parse of the wrapped
         * callback.  Advance the buffer and try the next iteration.
         */

        vbuf += (bytes_available - result);
        bytes_available = result;
    }

    /*
     * If we fall through the loop, then the wrapped callback
     * succeeded and used up the entire buffer.  That's great, but the
     * fold hasn't succeeded yet, since we need to try another
     * iteration.
     */

    PUSH_DEBUG_MSG("fold: Wrapped callback consumed "
                   "entire buffer.  Fold is incomplete.\n");
    return PUSH_INCOMPLETE;
}


static void
fold_free(push_callback_t *pcallback)
{
    fold_t  *callback = (fold_t *) pcallback;

    PUSH_DEBUG_MSG("fold: Freeing wrapped callback.\n");
    push_callback_free(callback->wrapped);
}


push_callback_t *
push_fold_new(push_callback_t *wrapped)
{
    fold_t  *result = (fold_t *) malloc(sizeof(fold_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       fold_activate,
                       fold_process_bytes,
                       fold_free);

    result->wrapped = wrapped;
    result->within_wrapped = false;
    result->last_result = NULL;

    return &result->base;
}
