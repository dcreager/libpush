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

#ifndef PUSH_FREE
#define PUSH_FREE 0
#endif

#if PUSH_FREE
#define PUSH_FREE_MSG(...) PUSH_DEBUG_MSG(__VA_ARGS__)
#else
#define PUSH_FREE_MSG(...) /* skipping debug message */
#endif


push_callback_t *
push_callback_new()
{
    return (push_callback_t *) malloc(sizeof(push_callback_t));
}


void
push_callback_free(push_callback_t *callback)
{
    PUSH_FREE_MSG("callback: %p: Freeing push_callback_t instance.\n",
                  callback);
    free(callback);
}
