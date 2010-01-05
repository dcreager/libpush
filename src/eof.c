/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <push.h>
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
        return PUSH_PARSE_ERROR;
    else
        return 0;
}


push_callback_t *
push_eof_new()
{
    return
        push_callback_new(NULL,
                          eof_process_bytes,
                          push_eof_allowed,
                          1,
                          0,
                          NULL);
}
