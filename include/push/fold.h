/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_FOLD_H
#define PUSH_FOLD_H

#include <stdbool.h>
#include <stdlib.h>

#include <push/basics.h>


/**
 * Allocate and initialize a new callback that calls another callback
 * repeatedly.  The input of each iteration of the wrapped callback is
 * passed as input into the next iteration.  We terminate the loop
 * when the wrapped callback generates a parse failure; whatever
 * result we had accumulated to that point is then returned as the
 * result of the fold callback.  Note, however, that the wrapped
 * callback must generate the parse error <i>immediately</i>; if it
 * partially parses the data, and then discovers the parse error in a
 * later call, we cannot backtrack the data.  This case generates a
 * parse error for the fold.
 */

push_callback_t *
push_fold_new(push_callback_t *wrapped);


#endif  /* PUSH_FOLD_H */
