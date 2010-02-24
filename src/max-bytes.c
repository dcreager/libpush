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
#include <string.h>

#include <push/basics.h>
#include <push/combinators.h>
#include <push/pairs.h>
#include <push/talloc.h>


/**
 * The user data struct for a max-bytes callback.
 */

typedef struct _max_bytes
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The continue continuation that will check the next chunk of
     * data to ensure it doesn't go over the maximum.  Will pass
     * however much data fits under the maximum to the wrapped_cont
     * continuation.
     */

    push_continue_continuation_t  cont;

    /**
     * The incomplete continuation that the wrapped callback uses if
     * there's still more data that we can send in.
     */

    push_incomplete_continuation_t  wrapped_incomplete;

    /**
     * The success continuation that the wrapped callback uses if the
     * current data chunk puts us over the maximum.
     */

    push_success_continuation_t  wrapped_success;

    /**
     * The incomplete continuation that the wrapped callback uses if
     * the current data chunk puts us over the maximum.
     */

    push_incomplete_continuation_t  wrapped_finished;

    /**
     * The wrapped callback.
     */

    push_callback_t  *wrapped;

    /**
     * The continue continuation that the wrapped callback gave us
     * most recently.
     */

    push_continue_continuation_t  *wrapped_cont;

    /**
     * The maximum number of bytes to pass in to the wrapped callback.
     */

    size_t  maximum_bytes;

    /**
     * The number of bytes that we've already sent into the wrapped
     * callback.
     */

    size_t  bytes_processed;

    /**
     * A pointer to the remainder of the data chunk for when the
     * current chunk would exceed the maximum.
     */

    const void  *leftover_buf;

    /**
     * The size of leftover_buf.
     */

    size_t  leftover_size;

} max_bytes_t;


static void
max_bytes_activate(void *user_data,
                   void *result,
                   const void *buf,
                   size_t bytes_remaining)
{
    max_bytes_t  *max_bytes = (max_bytes_t *) user_data;

    PUSH_DEBUG_MSG("%s: Activating.  Capping at %zu bytes.\n",
                   max_bytes->callback.name,
                   max_bytes->maximum_bytes);

    /*
     * If the current data chunk fits in under the maximum, go ahead
     * and send it into the wrapped callback.
     */

    if (bytes_remaining <= max_bytes->maximum_bytes)
    {
        PUSH_DEBUG_MSG("%s: Activating wrapped callback "
                       "with %zu bytes.\n",
                       max_bytes->callback.name,
                       bytes_remaining);

        /*
         * If the callback succeeds or errors, then we do the same.
         * If the callback incompletes, we need to remember the
         * continuation if we've not yet reached the maximum;
         * otherwise, we have to try to send it an EOF.
         */

        push_continuation_call(&max_bytes->wrapped->set_success,
                               max_bytes->callback.success);

        if (bytes_remaining == max_bytes->maximum_bytes)
        {
            max_bytes->leftover_buf = NULL;
            max_bytes->leftover_size = 0;

            push_continuation_call(&max_bytes->wrapped->set_incomplete,
                                   &max_bytes->wrapped_finished);
        } else {
            push_continuation_call(&max_bytes->wrapped->set_incomplete,
                                   &max_bytes->wrapped_incomplete);
        }

        push_continuation_call(&max_bytes->wrapped->set_error,
                               max_bytes->callback.error);

        /*
         * Make sure to remember how many more bytes we can send in
         * after this chunk.
         */

        max_bytes->bytes_processed = bytes_remaining;

        push_continuation_call(&max_bytes->wrapped->activate,
                               result,
                               buf, bytes_remaining);

        return;
    }

    /*
     * If the current chunk is bigger than the maximum, then we send
     * in the maximum portion into the wrapped callback.  We save a
     * pointer to the portion of the chunk that exceeds the maximum,
     * so that we can pass it on to the next callback.
     */

    max_bytes->leftover_buf =
        buf + max_bytes->maximum_bytes;
    max_bytes->leftover_size =
        bytes_remaining - max_bytes->maximum_bytes;

    /*
     * If the wrapped callback succeeds, we have to catch the success
     * and include the leftover data in the current chunk in the
     * response.  If the wrapped callback incompletes, then we have to
     * send it an EOF to see if it can succeed with the data that it's
     * already received.  (We can't send it more, since we've already
     * reached the maximum.)  If the wrapped callback errors, we
     * error, too.
     */

    push_continuation_call(&max_bytes->wrapped->set_success,
                           &max_bytes->wrapped_success);

    push_continuation_call(&max_bytes->wrapped->set_incomplete,
                           &max_bytes->wrapped_finished);

    push_continuation_call(&max_bytes->wrapped->set_error,
                           max_bytes->callback.error);

    PUSH_DEBUG_MSG("%s: Activating wrapped callback "
                   "with %zu bytes.\n",
                   max_bytes->callback.name,
                   max_bytes->maximum_bytes);

    push_continuation_call(&max_bytes->wrapped->activate,
                           result,
                           buf, max_bytes->maximum_bytes);

    return;
}


static void
dynamic_max_bytes_activate(void *user_data,
                           void *result,
                           const void *buf,
                           size_t bytes_remaining)
{
    max_bytes_t  *max_bytes = (max_bytes_t *) user_data;

    /*
     * Extract the threshold from the input pair, and pass off to the
     * regular activation function.
     */

    push_pair_t  *pair = (push_pair_t *) result;
    size_t  *maximum_bytes = (size_t *) pair->first;

    max_bytes->maximum_bytes = *maximum_bytes;

    max_bytes_activate(user_data, pair->second,
                       buf, bytes_remaining);
}


static void
max_bytes_cont(void *user_data,
               const void *buf,
               size_t bytes_remaining)
{
    max_bytes_t  *max_bytes = (max_bytes_t *) user_data;
    size_t  total_bytes =
        max_bytes->bytes_processed + bytes_remaining;
    size_t  bytes_to_send;

    PUSH_DEBUG_MSG("%s: Processing %zu bytes.\n",
                   max_bytes->callback.name,
                   bytes_remaining);

    /*
     * If the current data chunk fits in under the maximum, go ahead
     * and send it into the wrapped callback.
     */

    if (total_bytes <= max_bytes->maximum_bytes)
    {
        PUSH_DEBUG_MSG("%s: Sending %zu bytes to "
                       "wrapped callback.\n",
                       max_bytes->callback.name,
                       bytes_remaining);

        /*
         * If the callback succeeds or errors, then we do the same.
         * If the callback incompletes, we need to remember the
         * continuation if we've not yet reached the maximum;
         * otherwise, we have to try to send it an EOF.
         */

        push_continuation_call(&max_bytes->wrapped->set_success,
                               max_bytes->callback.success);

        if (total_bytes == max_bytes->maximum_bytes)
        {
            max_bytes->leftover_buf = NULL;
            max_bytes->leftover_size = 0;

            push_continuation_call(&max_bytes->wrapped->set_incomplete,
                                   &max_bytes->wrapped_finished);
        } else {
            push_continuation_call(&max_bytes->wrapped->set_incomplete,
                                   &max_bytes->wrapped_incomplete);
        }

        push_continuation_call(&max_bytes->wrapped->set_error,
                               max_bytes->callback.error);

        /*
         * Make sure to remember how many more bytes we can send in
         * after this chunk.
         */

        max_bytes->bytes_processed = total_bytes;

        push_continuation_call(max_bytes->wrapped_cont,
                               buf, bytes_remaining);

        return;
    }

    /*
     * If the current chunk is bigger than the maximum, then we send
     * in the maximum portion into the wrapped callback.  We save a
     * pointer to the portion of the chunk that exceeds the maximum,
     * so that we can pass it on to the next callback.
     */

    bytes_to_send =
        max_bytes->maximum_bytes - max_bytes->bytes_processed;

    max_bytes->leftover_buf =
        buf + bytes_to_send;
    max_bytes->leftover_size =
        bytes_remaining - bytes_to_send;

    /*
     * If the wrapped callback succeeds, we have to catch the success
     * and include the leftover data in the current chunk in the
     * response.  If the wrapped callback incompletes, then we have to
     * send it an EOF to see if it can succeed with the data that it's
     * already received.  (We can't send it more, since we've already
     * reached the maximum.)  If the wrapped callback errors, we
     * error, too.
     */

    push_continuation_call(&max_bytes->wrapped->set_success,
                           &max_bytes->wrapped_success);

    push_continuation_call(&max_bytes->wrapped->set_incomplete,
                           &max_bytes->wrapped_finished);

    push_continuation_call(&max_bytes->wrapped->set_error,
                           max_bytes->callback.error);

    PUSH_DEBUG_MSG("%s: Sending %zu bytes to "
                   "wrapped callback.\n",
                   max_bytes->callback.name,
                   bytes_to_send);

    push_continuation_call(max_bytes->wrapped_cont,
                           buf, bytes_to_send);

    return;
}


static void
max_bytes_wrapped_incomplete(void *user_data,
                             push_continue_continuation_t *cont)
{
    max_bytes_t  *max_bytes = (max_bytes_t *) user_data;

    /*
     * The wrapped callback says it needs more data, and we haven't
     * reached the maximum yet.  Register our own continue
     * continuation that ensures that we don't go over the maximum
     * when we get the next chunk.
     */

    PUSH_DEBUG_MSG("%s: Wrapped callback incomplete, maximum "
                   "not yet reached.\n",
                   max_bytes->callback.name);

    max_bytes->wrapped_cont = cont;

    push_continuation_call(max_bytes->callback.incomplete,
                           &max_bytes->cont);

    return;
}


static void
max_bytes_wrapped_success(void *user_data,
                          void *result,
                          const void *buf,
                          size_t bytes_remaining)
{
    max_bytes_t  *max_bytes = (max_bytes_t *) user_data;

    /*
     * The wrapped callback has succeeded, but the data chunk we sent
     * in put us over the maximum.  We've saved the remainder in
     * leftover_buf; we need to include this in our success message.
     *
     * The wrapped callback might not have processed the entire chunk
     * of data that we sent it, though; technically, we should
     * concatenate the buf we get from the wrapped callback with the
     * leftover_buf we saved from earlier.  When we sent the data in
     * to the callback, the two came from the same continguous region
     * of memory.  For now, we assume that the wrapped callback hasn't
     * fiddled with the memory on us, so we can just play with
     * pointers to do the concantenation.
     */

    PUSH_DEBUG_MSG("%s: Wrapped callback succeeded "
                   "using %zu bytes.\n",
                   max_bytes->callback.name,
                   (max_bytes->maximum_bytes - bytes_remaining));

    max_bytes->leftover_buf -= bytes_remaining;
    max_bytes->leftover_size += bytes_remaining;

    PUSH_DEBUG_MSG("%s: Sending %zu bytes on "
                   "to next callback.\n",
                   max_bytes->callback.name,
                   max_bytes->leftover_size);

    push_continuation_call(max_bytes->callback.success,
                           result,
                           max_bytes->leftover_buf,
                           max_bytes->leftover_size);

    return;
}


static void
max_bytes_wrapped_finished(void *user_data,
                           push_continue_continuation_t *cont)
{
    max_bytes_t  *max_bytes = (max_bytes_t *) user_data;

    /*
     * The wrapped callback says it needs more data, but we've already
     * given it the maximum.  Let's send it an EOF; if it succeeds,
     * then we can succeed — though we can't have the wrapped callback
     * call our success continuation directly, since we've got that
     * pesky leftover_buf lying around.  If the wrapped callback
     * generates an error, we generate an error, too.
     */

    PUSH_DEBUG_MSG("%s: Wrapped callback incomplete, but "
                   "we've reached maximum.  Sending EOF.\n",
                   max_bytes->callback.name);

    push_continuation_call(&max_bytes->wrapped->set_success,
                           &max_bytes->wrapped_success);

    push_continuation_call(&max_bytes->wrapped->set_incomplete,
                           &max_bytes->wrapped_finished);

    push_continuation_call(&max_bytes->wrapped->set_error,
                           max_bytes->callback.error);

    push_continuation_call(cont, NULL, 0);

    return;
}


push_callback_t *
push_max_bytes_new(const char *name,
                   push_parser_t *parser,
                   push_callback_t *wrapped,
                   size_t maximum_bytes)
{
    max_bytes_t  *max_bytes;

    /*
     * If the wrapped callback is NULL, return NULL ourselves.
     */

    if (wrapped == NULL)
        return NULL;

    /*
     * Allocate the user data struct.
     */

    max_bytes = push_talloc(parser, max_bytes_t);
    if (max_bytes == NULL)
        return NULL;

    /*
     * Make the wrapped callback a child of the new callback.
     */

    push_talloc_steal(max_bytes, wrapped);

    /*
     * Fill in the data items.
     */

    max_bytes->wrapped = wrapped;
    max_bytes->maximum_bytes = maximum_bytes;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL)
        name = "max-bytes";

    push_callback_init(name,
                       &max_bytes->callback, parser, max_bytes,
                       max_bytes_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&max_bytes->cont,
                          max_bytes_cont,
                          max_bytes);

    push_continuation_set(&max_bytes->wrapped_incomplete,
                          max_bytes_wrapped_incomplete,
                          max_bytes);

    push_continuation_set(&max_bytes->wrapped_success,
                          max_bytes_wrapped_success,
                          max_bytes);

    push_continuation_set(&max_bytes->wrapped_finished,
                          max_bytes_wrapped_finished,
                          max_bytes);

    return &max_bytes->callback;
}


push_callback_t *
push_dynamic_max_bytes_new(const char *name,
                           push_parser_t *parser,
                           push_callback_t *wrapped)
{
    max_bytes_t  *max_bytes;

    /*
     * If the wrapped callback is NULL, return NULL ourselves.
     */

    if (wrapped == NULL)
        return NULL;

    /*
     * Allocate the user data struct.
     */

    max_bytes = push_talloc(parser, max_bytes_t);
    if (max_bytes == NULL)
        return NULL;

    /*
     * Make the wrapped callback a child of the new callback.
     */

    push_talloc_steal(max_bytes, wrapped);

    /*
     * Fill in the data items.
     */

    max_bytes->wrapped = wrapped;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL)
        name = "dyn-max-bytes";

    push_callback_init(name,
                       &max_bytes->callback, parser, max_bytes,
                       dynamic_max_bytes_activate,
                       NULL, NULL, NULL);

    /*
     * Fill in the continuation objects for the continuations that we
     * implement.
     */

    push_continuation_set(&max_bytes->cont,
                          max_bytes_cont,
                          max_bytes);

    push_continuation_set(&max_bytes->wrapped_incomplete,
                          max_bytes_wrapped_incomplete,
                          max_bytes);

    push_continuation_set(&max_bytes->wrapped_success,
                          max_bytes_wrapped_success,
                          max_bytes);

    push_continuation_set(&max_bytes->wrapped_finished,
                          max_bytes_wrapped_finished,
                          max_bytes);

    return &max_bytes->callback;
}
