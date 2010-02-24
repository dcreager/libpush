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
#include <push/primitives.h>
#include <push/talloc.h>

#include <push/protobuf/primitives.h>


push_callback_t *
push_protobuf_skip_length_prefixed_new(const char *name,
                                       push_parser_t *parser)
{
    const char  *read_size_name = NULL;
    const char  *skip_name = NULL;
    const char  *compose_name = NULL;

    push_callback_t  *read_size = NULL;
    push_callback_t  *skip = NULL;
    push_callback_t  *compose = NULL;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "pb-skip-lp";

    read_size_name = push_string_concat(parser, name, ".size");
    if (read_size_name == NULL) goto error;

    skip_name = push_string_concat(parser, name, ".skip");
    if (skip_name == NULL) goto error;

    compose_name = push_string_concat(parser, name, ".compose");
    if (compose_name == NULL) goto error;

    /*
     * Then create the callbacks.
     */

    read_size =
        push_protobuf_varint_size_new(read_size_name, parser);
    skip = push_skip_new(skip_name, parser);
    compose = push_compose_new(compose_name, parser, read_size, skip);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (compose == NULL) goto error;

    /*
     * Make each name string be the child of its callback.
     */

    push_talloc_steal(read_size, read_size_name);
    push_talloc_steal(skip, skip_name);
    push_talloc_steal(compose, compose_name);

    return compose;

  error:
    if (read_size_name != NULL) push_talloc_free(read_size_name);
    if (read_size != NULL) push_talloc_free(read_size);

    if (skip_name != NULL) push_talloc_free(skip_name);
    if (skip != NULL) push_talloc_free(skip);

    if (compose_name != NULL) push_talloc_free(compose_name);
    if (compose != NULL) push_talloc_free(compose);

    return NULL;
}
