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
 * A callback that calls another callback repeatedly.  The input of
 * each iteration of the wrapped callback is passed as input into the
 * next iteration.  We terminate the loop when the wrapped callback
 * generates a parse failure; whatever result we had accumulated to
 * that point is then returned as the result of the fold callback.
 * Note, however, that the wrapped callback must generate the parse
 * error <i>immediately</i>; if it partially parses the data, and then
 * discovers the parse error in a later call, we cannot backtrack the
 * data.  This case generates a parse error for the fold.
 */

typedef struct _push_fold
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
     * A flag indicating whether we're in the middle of passing data
     * to the wrapped callback.  This is true after a call to the
     * wrapped callback returns PUSH_INCOMPLETE.  Later calls cannot
     * backtrack the data that we already passed in, so if we get a
     * PUSH_PARSE_ERROR after a PUSH_INCOMPLETE, we generate a parse
     * error for the fold.
     */

    bool within_wrapped;

    /**
     * A saved copy of the most recent result.  While we're in the
     * middle of trying the next iteration, we might generate a new
     * result.  If the next iteration fails, then we need to return
     * the result that we had before trying the iteration.
     */

    void  *last_result;

} push_fold_t;


/**
 * Allocate and initialize a new push_fold_t.
 */

push_fold_t *
push_fold_new(push_callback_t *wrapped);


#endif  /* PUSH_FOLD_H */
