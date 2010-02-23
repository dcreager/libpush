/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <push/basics.h>
#include <push/combinators.h>
#include <push/pairs.h>
#include <push/primitives.h>
#include <push/protobuf/primitives.h>


push_callback_t *
push_protobuf_varint_prefixed_new(push_parser_t *parser,
                                  push_callback_t *wrapped)
{
    push_callback_t  *dup = NULL;
    push_callback_t  *size = NULL;
    push_callback_t  *first = NULL;
    push_callback_t  *max_bytes = NULL;
    push_callback_t  *compose1 = NULL;
    push_callback_t  *compose2 = NULL;

    /*
     * First, create the callbacks.
     */

    dup = push_dup_new(parser);
    if (dup == NULL)
        goto error;

    size = push_protobuf_varint_size_new(parser);
    if (size == NULL)
        goto error;

    first = push_first_new(parser, size);
    if (first == NULL)
        goto error;

    compose1 = push_compose_new(parser, dup, first);
    if (compose1 == NULL)
        goto error;

    max_bytes = push_dynamic_max_bytes_new(parser, wrapped);
    if (max_bytes == NULL)
        goto error;

    compose2 = push_compose_new(parser, compose1, max_bytes);
    if (compose2 == NULL)
        goto error;

    return compose2;

  error:
    /*
     * Before returning the NULL error code, free everything that we
     * might've created so far.
     */

    if (dup != NULL)
        push_callback_free(dup);

    if (size != NULL)
        push_callback_free(size);

    if (first != NULL)
        push_callback_free(first);

    if (max_bytes != NULL)
        push_callback_free(max_bytes);

    if (compose1 != NULL)
        push_callback_free(compose1);

    if (compose2 != NULL)
        push_callback_free(compose2);

    return NULL;
}
