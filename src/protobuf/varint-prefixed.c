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
push_protobuf_varint_prefixed_new(push_callback_t *wrapped)
{
    push_callback_t  *size_callback = NULL;
    push_callback_t  *noop_callback = NULL;
    push_callback_t  *both_callback = NULL;
    push_callback_t  *max_bytes_callback = NULL;
    push_callback_t  *compose_callback = NULL;

    /*
     * First, create the callbacks.
     */

    size_callback = push_protobuf_varint_size_new();
    if (size_callback == NULL)
        goto error;

    noop_callback = push_noop_new();
    if (noop_callback == NULL)
        goto error;

    both_callback = push_both_new(size_callback, noop_callback);
    if (both_callback == NULL)
        goto error;

    max_bytes_callback = push_dynamic_max_bytes_new(wrapped);
    if (max_bytes_callback == NULL)
        goto error;

    compose_callback = push_compose_new(both_callback, max_bytes_callback);
    if (compose_callback == NULL)
        goto error;

    return compose_callback;

  error:
    /*
     * Before returning the NULL error code, free everything that we
     * might've created so far.
     */

    if (size_callback != NULL)
        push_callback_free(size_callback);

    if (noop_callback != NULL)
        push_callback_free(noop_callback);

    if (both_callback != NULL)
        push_callback_free(both_callback);

    if (max_bytes_callback != NULL)
        push_callback_free(max_bytes_callback);

    if (compose_callback != NULL)
        push_callback_free(compose_callback);

    return NULL;
}
