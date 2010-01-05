/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <push.h>
#include <push/skip.h>


static push_error_code_t
skip_activate(push_parser_t *parser,
              push_callback_t *pcallback,
              push_callback_t *old_callback)
{
    push_skip_t  *callback = (push_skip_t *) pcallback;

    callback->bytes_skipped = 0;

    return PUSH_SUCCESS;
}


static ssize_t
skip_process_bytes(push_parser_t *parser,
                   push_callback_t *pcallback,
                   const void *vbuf,
                   size_t bytes_available)
{
    push_skip_t  *callback = (push_skip_t *) pcallback;
    size_t  skip_size;

    if (callback->bytes_skipped > callback->bytes_to_skip)
    {
        /*
         * Well, this shouldn't have happened...  We've skipped more
         * bytes than we were asked to.  Let's call this a parse
         * error.
         */

        return PUSH_PARSE_ERROR;
    }

    /*
     * Determine how many bytes in this chunk to skip over.
     */

    skip_size = callback->bytes_to_skip - callback->bytes_skipped;

    if (skip_size > bytes_available)
        skip_size = bytes_available;

    /*
     * Skip over the bytes.
     */

    callback->bytes_skipped += skip_size;
    bytes_available -= skip_size;

    /*
     * If we've skipped over all of the bytes, we pass off to the next
     * callback.
     */

    if (callback->bytes_skipped == callback->bytes_to_skip)
    {
        push_parser_set_callback(parser, pcallback->next_callback);
    }

    /*
     * And return...
     */

    return bytes_available;
}


static push_error_code_t
skip_eof(push_parser_t *parser,
         push_callback_t *pcallback)
{
    push_skip_t  *callback = (push_skip_t *) pcallback;

    /*
     * Only allow EOF at the beginning of the skipping; once we've
     * started skipping some of the bytes, no more EOF!
     */

    return
        (callback->bytes_skipped == 0)?
        PUSH_SUCCESS:
        PUSH_PARSE_ERROR;
}


push_skip_t *
push_skip_new(push_callback_t *next_callback,
              bool eof_allowed)
{
    push_skip_t  *result = (push_skip_t *) malloc(sizeof(push_skip_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       1,
                       0,
                       skip_activate,
                       skip_process_bytes,
                       eof_allowed?
                           skip_eof:
                           push_eof_not_allowed,
                       NULL,
                       next_callback);

    result->bytes_to_skip = 0; 
    result->bytes_skipped = 0; 

    return result;
}
