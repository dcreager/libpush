/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_DEBUG_H
#define PUSH_DEBUG_H

#ifndef PUSH_DEBUG
#define PUSH_DEBUG 0
#endif

#ifndef PUSH_CONTINUATION_DEBUG
#define PUSH_CONTINUATION_DEBUG 0
#endif

#if PUSH_DEBUG
#include <stdio.h>
#define PUSH_DEBUG_MSG(...) fprintf(stderr, __VA_ARGS__)
#else
#define PUSH_DEBUG_MSG(...) /* skipping debug message */
#endif

#endif  /* PUSH_DEBUG_H */
