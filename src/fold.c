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
 * The user data struct for a fold callback.
 */

typedef struct _fold
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * An incomplete continuation that “remembers” when the wrapped
     * callback generates an incomplete.  This is used to select
     * between our two error continuations — the first turns wrapped
     * parse errors into fold successes, the second keeps them as
     * parse errors.
     */

    push_incomplete_continuation_t  remember_incomplete;

    /**
     * A continue continuation that is called after empty initial data
     * chunks.  These don't count towards the “no parse errors after
     * incompletes” rule, but we have to keep the first_chunk field up
     * to date in this case.
     */

    push_continue_continuation_t  continue_after_empty;

    /**
     * An error continuation that catches a wrapped parse error, and
     * turns it into a fold success.  This is only the right behavior
     * if the wrapped callback hasn't returned an incomplete yet,
     * since it's required to yield a parse error <i>immediately</i>.
     */

    push_error_continuation_t  initial_error;

    /**
     * An error continuation that passes on wrapped parse error as
     * parse errors of the overall fold.  This is the right behavior
     * if the wrapped callback has returned an incomplete during this
     * iteration.
     */

    push_error_continuation_t  later_error;

    /**
     * The wrapped callback.
     */

    push_callback_t  *wrapped;

    /**
     * The wrapped callback's continue continuation.  This is used to
     * actually process the data during our own continue_after_empty
     * continuation.
     */

    push_continue_continuation_t  *wrapped_cont;

    /**
     * A saved copy of the most recent result.  While we're in the
     * middle of trying the next iteration, we might generate a new
     * result.  If the next iteration fails, then we need to return
     * the result that we had before trying the iteration.
     */

    void  *last_result;

    /**
     * A pointer to the first data chunk of each iteration.  If the
     * wrapped callback fails during this chunk, then the overall fold
     * succeeds, and we have to pass this initial chunk on to our
     * success continuation.
     */

    const void  *first_chunk;

    /**
     * The length of first_chunk.
     */

    size_t  first_size;

} fold_t;


static void
fold_activate(void *user_data,
              void *result,
              const void *buf,
              size_t bytes_remaining)
{
    fold_t  *fold = (fold_t *) user_data;

    /*
     * We activate each iteration of the fold by saving the result,
     * and then activating the wrapped callback.
     */


    PUSH_DEBUG_MSG("%s: Saving %p as most recent result.\n",
                   fold->callback.name,
                   result);
    fold->last_result = result;

    /*
     * If the wrapped callback succeeds, it should reactivate the fold
     * to start the next iteration.  If it fails in this initial
     * chunk, then the overall fold succeeds.  If it incompletes, then
     * we need to remember this so that we can ensure that it doesn't
     * generate a parse error after that.
     */

    push_continuation_call(&fold->wrapped->set_success,
                           &fold->callback.activate);

    push_continuation_call(&fold->wrapped->set_incomplete,
                           &fold->remember_incomplete);

    push_continuation_call(&fold->wrapped->set_error,
                           &fold->initial_error);

    /*
     * Before calling the wrapped callback, save this initial chunk of
     * data, in case we need to pass it on during a successful result.
     */

    fold->first_chunk = buf;
    fold->first_size = bytes_remaining;

    PUSH_DEBUG_MSG("%s: Activating wrapped callback "
                   "with %zu bytes.\n",
                   fold->callback.name,
                   bytes_remaining);

    push_continuation_call(&fold->wrapped->activate,
                           result,
                           buf, bytes_remaining);

    return;
}


static void
fold_remember_incomplete(void *user_data,
                         push_continue_continuation_t *cont)
{
    fold_t  *fold = (fold_t *) user_data;

    /*
     * If we reach this continuation, then the wrapped callback
     * generated an incomplete.  Assuming that the first chunk of data
     * isn't empty, then the wrapped callback is no longer allowed to
     * generate a parse error.  If it does, then the fold generates a
     * parse error, as well.
     */

    if (fold->first_size > 0)
    {
        PUSH_DEBUG_MSG("%s: Wrapped callback is incomplete.  "
                       "It can no longer generate a parse error.\n",
                       fold->callback.name);

        /*
         * The wrapped callback can now pass on its incompletes
         * without us having to spy on them, since the damage is done.
         * If it generates an error, we have to use a different error
         * continuation than before, since we can't recover the parse
         * error as a fold success.
         */

        push_continuation_call(&fold->wrapped->set_incomplete,
                               fold->callback.incomplete);

        push_continuation_call(&fold->wrapped->set_error,
                               &fold->later_error);

        /*
         * Pass on the incomplete.
         */

        push_continuation_call(fold->callback.incomplete, cont);

        return;
    }

    /*
     * If the first chunk of data was empty (which can happen when the
     * callback is first activated), then it doesn't count towards the
     * “no parse errors after incompletes” rule.  We have to register
     * our own continue continuation, though, since we have to keep
     * that first_chunk field up to date.
     */

    PUSH_DEBUG_MSG("%s: Wrapped callback is incomplete, "
                   "but first data chunk is empty.\n",
                   fold->callback.name);

    fold->wrapped_cont = cont;

    push_continuation_call(fold->callback.incomplete,
                           &fold->continue_after_empty);

    return;
}


static void
fold_continue_after_empty(void *user_data,
                          const void *buf,
                          size_t bytes_remaining)
{
    fold_t  *fold = (fold_t *) user_data;

    /*
     * If we reach this continuation, we've only gotten empty data
     * chunks so far, so they didn't count towards the “no parse
     * errors after incompletes” rule.  If we don't have any data,
     * then this is an EOF in between iterations, which is allowed.
     */

    if (bytes_remaining == 0)
    {
        PUSH_DEBUG_MSG("%s: EOF in between iterations.  Fold is "
                       "successful.\n",
                       fold->callback.name);

        push_continuation_call(fold->callback.success,
                               fold->last_result,
                               NULL, 0);

        return;
    }

    /*
     * If we do have data, we want to pass this data on to the wrapped
     * callback, but we need to keep the first_chunk field up to date
     * before we do so.
     */

    PUSH_DEBUG_MSG("%s: Continuing wrapped callback with "
                   "%zu bytes.\n",
                   fold->callback.name,
                   bytes_remaining);

    fold->first_chunk = buf;
    fold->first_size = bytes_remaining;

    push_continuation_call(fold->wrapped_cont, buf, bytes_remaining);

    return;
}


static void
fold_initial_error(void *user_data,
                   push_error_code_t error_code,
                   const char *error_message)
{
    fold_t  *fold = (fold_t *) user_data;

    /*
     * If we reach this continuation, then the wrapped callback
     * generated an error in its initial data chunk.  If this is a
     * parse error, then the fold succeeds, and we use the last
     * successful result as the fold's result.
     */

    if (error_code == PUSH_PARSE_ERROR)
    {
        PUSH_DEBUG_MSG("%s: Parse error.  Fold succeeds with "
                       "previous result.\n",
                       fold->callback.name);

        push_continuation_call(fold->callback.success,
                               fold->last_result,
                               fold->first_chunk,
                               fold->first_size);

        return;
    }

    /*
     * We pass on any other error as-is.
     */

    push_continuation_call(fold->callback.error,
                           error_code, error_message);

    return;
}


static void
fold_later_error(void *user_data,
                 push_error_code_t error_code,
                 const char *error_message)
{
    fold_t  *fold = (fold_t *) user_data;

    /*
     * If we reach this continuation, then the wrapped callback
     * generated an error are having returned an incomplete.  If this
     * is a parse error, then the fold generates a parse error, too.
     */

    if (error_code == PUSH_PARSE_ERROR)
    {
        PUSH_DEBUG_MSG("%s: Parse error after incomplete.  "
                       "Fold results in a parse error!\n",
                       fold->callback.name);

        push_continuation_call(fold->callback.error,
                               PUSH_PARSE_ERROR,
                               "Parse error in fold after incomplete");

        return;
    }

    /*
     * We pass on any other error as-is.
     */

    push_continuation_call(fold->callback.error,
                           error_code, error_message);

    return;
}


push_callback_t *
push_fold_new(const char *name,
              push_parser_t *parser,
              push_callback_t *wrapped)
{
    fold_t  *fold = (fold_t *) malloc(sizeof(fold_t));

    if (fold == NULL)
        return NULL;

    /*
     * Fill in the data items.
     */

    fold->wrapped = wrapped;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL)
        name = "fold";

    push_callback_init(name,
                       &fold->callback, parser, fold,
                       fold_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&fold->remember_incomplete,
                          fold_remember_incomplete,
                          fold);

    push_continuation_set(&fold->continue_after_empty,
                          fold_continue_after_empty,
                          fold);

    push_continuation_set(&fold->initial_error,
                          fold_initial_error,
                          fold);

    push_continuation_set(&fold->later_error,
                          fold_later_error,
                          fold);

    return &fold->callback;
}
