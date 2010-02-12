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
#include <push/protobuf/primitives.h>


push_callback_t *
push_protobuf_skip_length_prefixed_new()
{
    push_callback_t  *read_size = NULL;
    push_callback_t  *skip = NULL;
    push_callback_t  *compose = NULL;

    /*
     * First, create the callbacks.
     */

    read_size = push_protobuf_varint_size_new();
    if (read_size == NULL)
        goto error;

    skip = push_skip_new();
    if (skip == NULL)
        goto error;

    compose = push_compose_new(read_size, skip);
    if (compose == NULL)
        goto error;

    return compose;

  error:
    /*
     * Before returning the NULL error code, free everything that we
     * might've created so far.
     */

    if (read_size != NULL)
        push_callback_free(read_size);

    if (skip != NULL)
        push_callback_free(skip);

    if (compose != NULL)
        push_callback_free(compose);

    return NULL;
}
