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


/**
 * A callback that skips the specified number of bytes.  The
 * bytes_to_skip field should be filled in before passing off to this
 * callback.
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
push_skip_new(push_callback_t *next_callback,
              bool eof_allowed);


#endif  /* PUSH_SKIP_H */
