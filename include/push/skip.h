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
 * Allocate and initialize a new callback that skips the specified
 * number of bytes.  The callback's input should be a pointer to a
 * size_t, indicating the number of bytes to skip.
 */

push_callback_t *
push_skip_new();


#endif  /* PUSH_SKIP_H */
