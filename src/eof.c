/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <push/basics.h>
#include <push/eof.h>


static ssize_t
eof_process_bytes(push_parser_t *parser,
                  push_callback_t *pcallback,
                  const void *vbuf,
                  size_t bytes_available)
{
    /*
     * Any data results in a parse error.
     */

    if (bytes_available > 0)
    {
        PUSH_DEBUG_MSG("eof: Expected EOF, but got %zu bytes.\n",
                       bytes_available);

        return PUSH_PARSE_ERROR;
    } else {
        PUSH_DEBUG_MSG("eof: Reached expected EOF.\n");

        return 0;
    }
}


push_callback_t *
push_eof_new()
{
    return
        push_callback_new(NULL,
                          eof_process_bytes);
}
