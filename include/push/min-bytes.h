/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_MIN_BYTES_H
#define PUSH_MIN_BYTES_H

#include <stdlib.h>

#include <push/basics.h>


/**
 * A callback that wraps another callback.  It will ensure that a
 * certain number of bytes are available before calling the wrapped
 * callback, buffering the data if needed.
 */

typedef struct _push_min_bytes
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
     * The minimum number of bytes to pass in to the wrapped callback.
     */

    size_t  minimum_bytes;

    /**
     * A buffer for storing data until we reach the minimum.
     */

    void  *buffer;

    /**
     * The number of bytes currently stored in the buffer.
     */

    size_t  bytes_buffered;

} push_min_bytes_t;


/**
 * Allocate and initialize a new push_min_bytes_t.
 */

push_min_bytes_t *
push_min_bytes_new(push_callback_t *wrapped,
                   size_t minimum_bytes);


#endif  /* PUSH_MIN_BYTES_H */
