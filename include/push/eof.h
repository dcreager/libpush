/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_EOF_H
#define PUSH_EOF_H

#include <push/basics.h>


/**
 * Allocate and initialize a new callback that requires the end of the
 * stream.  If any data is present, it results in a parse error.
 */

push_callback_t *
push_eof_new();


#endif  /* PUSH_EOF_H */
