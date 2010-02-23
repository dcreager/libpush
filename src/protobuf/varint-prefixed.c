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
push_protobuf_varint_prefixed_new(const char *name,
                                  push_parser_t *parser,
                                  push_callback_t *wrapped)
{
    const char  *dup_name;
    const char  *size_name;
    const char  *first_name;
    const char  *max_bytes_name;
    const char  *compose1_name;
    const char  *compose2_name;

    push_callback_t  *dup = NULL;
    push_callback_t  *size = NULL;
    push_callback_t  *first = NULL;
    push_callback_t  *max_bytes = NULL;
    push_callback_t  *compose1 = NULL;
    push_callback_t  *compose2 = NULL;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "varint-prefixed";

    dup_name = push_string_concat(name, ".dup");
    if (dup_name == NULL) return NULL;

    size_name = push_string_concat(name, ".size");
    if (size_name == NULL) return NULL;

    first_name = push_string_concat(name, ".first");
    if (first_name == NULL) return NULL;

    max_bytes_name = push_string_concat(name, ".max");
    if (max_bytes_name == NULL) return NULL;

    compose1_name = push_string_concat(name, ".compose1");
    if (compose1_name == NULL) return NULL;

    compose2_name = push_string_concat(name, ".compose2");
    if (compose2_name == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    dup = push_dup_new(dup_name, parser);
    if (dup == NULL)
        goto error;

    size = push_protobuf_varint_size_new(size_name, parser);
    if (size == NULL)
        goto error;

    first = push_first_new(first_name, parser, size);
    if (first == NULL)
        goto error;

    compose1 =
        push_compose_new(compose1_name, parser, dup, first);
    if (compose1 == NULL)
        goto error;

    max_bytes =
        push_dynamic_max_bytes_new(max_bytes_name, parser, wrapped);
    if (max_bytes == NULL)
        goto error;

    compose2 =
        push_compose_new(compose2_name, parser, compose1, max_bytes);
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
