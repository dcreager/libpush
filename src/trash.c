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
#include <push/trash.h>


static ssize_t
trash_process_bytes(push_parser_t *parser,
                    push_callback_t *pcallback,
                    const void *vbuf,
                    size_t bytes_available)
{
    /*
     * We always “process” all of the input.
     */

    return 0;
}


push_callback_t *
push_trash_new()
{
    return
        push_callback_new(trash_process_bytes,
                          push_eof_allowed,
                          1,
                          0,
                          NULL);
}
