/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_SKIP_H
#define PUSH_SKIP_H

#include <stdlib.h>

#include <push/basics.h>


/**
 * A callback that skips the specified number of bytes.  The
 * callback's input should be a pointer to a size_t, indicating the
 * number of bytes to skip.
 */

typedef struct _push_skip
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

} push_skip_t;


/**
 * Allocate and initialize a new push_skip_t.
 */

push_skip_t *
push_skip_new();


#endif  /* PUSH_SKIP_H */
