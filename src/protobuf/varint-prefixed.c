/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */


#include <talloc.h>

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
    const char  *dup_name = NULL;
    const char  *size_name = NULL;
    const char  *first_name = NULL;
    const char  *max_bytes_name = NULL;
    const char  *compose1_name = NULL;
    const char  *compose2_name = NULL;

    push_callback_t  *dup = NULL;
    push_callback_t  *size = NULL;
    push_callback_t  *first = NULL;
    push_callback_t  *max_bytes = NULL;
    push_callback_t  *compose1 = NULL;
    push_callback_t  *compose2 = NULL;

    /*
     * If the wrapped callback is NULL, return NULL ourselves.
     */

    if (wrapped == NULL)
        return NULL;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "varint-prefixed";

    dup_name = push_string_concat(parser, name, ".dup");
    if (dup_name == NULL) goto error;

    size_name = push_string_concat(parser, name, ".size");
    if (size_name == NULL) goto error;

    first_name = push_string_concat(parser, name, ".first");
    if (first_name == NULL) goto error;

    max_bytes_name = push_string_concat(parser, name, ".max");
    if (max_bytes_name == NULL) goto error;

    compose1_name = push_string_concat(parser, name, ".compose1");
    if (compose1_name == NULL) goto error;

    compose2_name = push_string_concat(parser, name, ".compose2");
    if (compose2_name == NULL) goto error;

    /*
     * Then create the callbacks.
     */

    dup = push_dup_new(dup_name, parser);
    size = push_protobuf_varint_size_new(size_name, parser);
    first = push_first_new(first_name, parser, size);
    compose1 =
        push_compose_new(compose1_name, parser, dup, first);
    max_bytes =
        push_dynamic_max_bytes_new(max_bytes_name, parser, wrapped);
    compose2 =
        push_compose_new(compose2_name, parser, compose1, max_bytes);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (compose2 == NULL) goto error;

    /*
     * Make each name string be the child of its callback.
     */

    talloc_steal(dup, dup_name);
    talloc_steal(size, size_name);
    talloc_steal(first, first_name);
    talloc_steal(compose1, compose1_name);
    talloc_steal(max_bytes, max_bytes_name);
    talloc_steal(compose2, compose2_name);

    return compose2;

  error:
    if (dup_name != NULL) talloc_free(dup_name);
    if (dup != NULL) talloc_free(dup);

    if (size_name != NULL) talloc_free(size_name);
    if (size != NULL) talloc_free(size);

    if (first_name != NULL) talloc_free(first_name);
    if (first != NULL) talloc_free(first);

    if (compose1_name != NULL) talloc_free(compose1_name);
    if (compose1 != NULL) talloc_free(compose1);

    if (max_bytes_name != NULL) talloc_free(max_bytes_name);
    if (max_bytes != NULL) talloc_free(max_bytes);

    if (compose2_name != NULL) talloc_free(compose2_name);
    if (compose2 != NULL) talloc_free(compose2);

    return NULL;
}
