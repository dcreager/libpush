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
 * Allocate and initialize a new callback that wraps another callback.
 * It will ensure that a certain number of bytes are available before
 * calling the wrapped callback, buffering the data if needed.
 */

push_callback_t *
push_min_bytes_new(push_callback_t *wrapped,
                   size_t minimum_bytes);


#endif  /* PUSH_MIN_BYTES_H */
