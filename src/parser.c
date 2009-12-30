/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <push.h>

push_parser_t *
push_parser_new(push_callback_t *callback)
{
    push_parser_t  *result;

    /*
     * First try to allocate the new parser instance.  If we can't,
     * return NULL.
     */

    result = (push_parser_t *) malloc(sizeof(push_parser_t));
    if (result == NULL)
    {
        return NULL;
    }

    /*
     * If it works, initialize and return the new instance.
     */

    result->current_callback = callback;
    return result;
}


void
push_parser_free(push_parser_t *parser)
{
    /*
     * Free the parser's callback, and then the parser itself.
     */

    push_callback_free(parser->current_callback);
    free(parser);
}
