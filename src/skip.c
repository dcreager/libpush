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

#include <push/basics.h>
#include <push/primitives.h>


/**
 * The user data struct for a skip callback.
 */

typedef struct _skip
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The continue continuation for this callback.
     */

    push_continue_continuation_t  cont;

    /**
     * The total number of bytes to skip.
     */

    size_t  total_to_skip;

    /**
     * The number of bytes left to skip.
     */

    size_t  left_to_skip;

} skip_t;


static void
skip_continue(void *user_data,
              const void *buf,
              size_t bytes_remaining)
{
    skip_t  *skip = (skip_t *) user_data;
    size_t  bytes_to_skip;

    /*
     * If we reach EOF, then it's a parse error, since we didn't
     * receive as many bytes as we needed to skip over.
     */

    if (bytes_remaining == 0)
    {
        PUSH_DEBUG_MSG("skip: Reached EOF still needing to skip "
                       "%zu bytes.\n",
                       skip->left_to_skip);

        push_continuation_call(skip->callback.error,
                               PUSH_PARSE_ERROR,
                               "Reached EOF before end of skip");

        return;
    }

    /*
     * Skip over the data in the current chunk.
     */

    bytes_to_skip =
        (bytes_remaining > skip->left_to_skip)?
        skip->left_to_skip:
        bytes_remaining;

    PUSH_DEBUG_MSG("skip: Skipping over %zu bytes.\n",
                   bytes_to_skip);

    buf += bytes_to_skip;
    bytes_remaining -= bytes_to_skip;
    skip->left_to_skip -= bytes_to_skip;

    /*
     * If we've skipped over everything we need to, then fire off a
     * success result.
     */

    if (skip->left_to_skip == 0)
    {
        PUSH_DEBUG_MSG("skip: Finished skipping.\n");

        push_continuation_call(skip->callback.success,
                               NULL,
                               buf, bytes_remaining);

        return;
    }

    /*
     * Otherwise, we return an incomplete result.
     */

    PUSH_DEBUG_MSG("skip: %zu bytes left to skip.\n",
                   callback->left_to_skip);

    push_continuation_call(skip->callback.incomplete,
                           &skip->cont);
}


static void
skip_activate(void *user_data,
              void *result,
              const void *buf,
              size_t bytes_remaining)
{
    skip_t  *skip = (skip_t *) user_data;
    size_t  *bytes_to_skip = (size_t *) result;

    PUSH_DEBUG_MSG("skip: Activating.  Will skip %zu bytes.\n",
                   *bytes_to_skip);

    skip->total_to_skip = *bytes_to_skip;
    skip->left_to_skip = *bytes_to_skip;

    if (bytes_remaining == 0)
    {
        /*
         * If we don't get any data when we're activated, return an
         * incomplete and wait for some data.
         */

        push_continuation_call(skip->callback.incomplete,
                               &skip->cont);

        return;

    } else {
        /*
         * Otherwise let the continue continuation go ahead and
         * process this chunk of data.
         */

        skip_continue(user_data, buf, bytes_remaining);
        return;
    }
}


push_callback_t *
push_skip_new(push_parser_t *parser)
{
    skip_t  *skip = (skip_t *) malloc(sizeof(skip_t));

    if (skip == NULL)
        return NULL;

    /*
     * Initialize the push_callback_t instance.
     */

    push_callback_init(&skip->callback, parser, skip,
                       skip_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&skip->cont,
                          skip_continue,
                          skip);

    return &skip->callback;
}
