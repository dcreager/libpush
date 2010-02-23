/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdint.h>

#include <push/basics.h>
#include <push/primitives.h>
#include <test-callbacks.h>


push_callback_t *
integer_callback_new(const char *name,
                     push_parser_t *parser)
{
    if (name == NULL)
        name = "integer";

    return push_fixed_new(name, parser, sizeof(uint32_t));
}
