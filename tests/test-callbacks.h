/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef TEST_CALLBACKS_H
#define TEST_CALLBACKS_H

#include <push/basics.h>


/**
 * Create a callback that parses a single uint32_t.
 */

push_callback_t *
integer_callback_new(push_parser_t *parser);


/**
 * Create a callback that parses a single uint32_t and adds it to a
 * running sum.  The previous value of the sum is taken in as an
 * input; the new value is the output.
 */

push_callback_t *
sum_callback_new(push_parser_t *parser);


#endif  /* TEST_CALLBACKS_H */
