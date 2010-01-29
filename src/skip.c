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
 * The push_callback_t subclass that defines a skip callback.
 */

typedef struct _skip
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * The number of bytes to skip.
     */

    size_t  bytes_to_skip;

    /**
     * The number of bytes skipped so far.
     */

    size_t  bytes_skipped;

} skip_t;


static push_error_code_t
skip_activate(push_parser_t *parser,
              push_callback_t *pcallback,
              void *input)
{
    skip_t  *callback = (skip_t *) pcallback;
    size_t  *bytes_to_skip = (size_t *) input;

    PUSH_DEBUG_MSG("skip: Activating.  Will skip %zu bytes.\n",
                   *bytes_to_skip);

    callback->bytes_to_skip = *bytes_to_skip;
    callback->bytes_skipped = 0;

    return PUSH_SUCCESS;
}


static ssize_t
skip_process_bytes(push_parser_t *parser,
                   push_callback_t *pcallback,
                   const void *vbuf,
                   size_t bytes_available)
{
    skip_t  *callback = (skip_t *) pcallback;
    size_t  skip_size;

    if (callback->bytes_skipped > callback->bytes_to_skip)
    {
        /*
         * Well, this shouldn't have happened...  We've skipped more
         * bytes than we were asked to.  Let's call this a parse
         * error.
         */

        PUSH_DEBUG_MSG("skip: Fatal error, we somehow skipped over "
                       "too many bytes!\n");

        return PUSH_PARSE_ERROR;
    }

    /*
     * If we reach EOF, then it's a parse error, since we didn't
     * receive as many bytes as we needed to skip over.
     */

    if (bytes_available == 0)
    {
        PUSH_DEBUG_MSG("skip: Reached EOF still needing to skip "
                       "%zu bytes.\n",
                       callback->bytes_to_skip - callback->bytes_skipped);

        return PUSH_PARSE_ERROR;
    }

    /*
     * Determine how many bytes in this chunk to skip over.
     */

    skip_size = callback->bytes_to_skip - callback->bytes_skipped;

    if (skip_size > bytes_available)
        skip_size = bytes_available;

    PUSH_DEBUG_MSG("skip: Skipping over %zu bytes.\n", skip_size);

    /*
     * Skip over the bytes.
     */

    callback->bytes_skipped += skip_size;
    bytes_available -= skip_size;

    /*
     * If we haven't skipped over all of the bytes yet, then we need
     * to return the “incomplete” code.
     */

    if (callback->bytes_skipped < callback->bytes_to_skip)
    {
        PUSH_DEBUG_MSG("skip: %zu bytes left to skip.\n",
                       callback->bytes_to_skip - callback->bytes_skipped);
        return PUSH_INCOMPLETE;
    }

    /*
     * Otherwise return a success code, indicating how many bytes are
     * left in this chunk.
     */

    PUSH_DEBUG_MSG("skip: Finished skipping; %zu bytes left.\n",
                   bytes_available);

    return bytes_available;
}


push_callback_t *
push_skip_new()
{
    skip_t  *result = (skip_t *) malloc(sizeof(skip_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       skip_activate,
                       skip_process_bytes,
                       NULL);

    result->bytes_to_skip = 0; 
    result->bytes_skipped = 0; 

    return &result->base;
}
