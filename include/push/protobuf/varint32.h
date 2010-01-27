/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PROTOBUF_VARINT32_H
#define PUSH_PROTOBUF_VARINT32_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <push.h>

/**
 * A callback for parsing a varint-encoded integer, which we don't
 * expect to be more than 32 bits.  The result pointer will point at
 * the parsed value, stored as a uint32_t.
 */

typedef struct _push_protobuf_varint32
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * The number of bytes currently added to the varint.
     */

    size_t  bytes_processed;

    /**
     * The parsed value.
     */

    uint32_t  value;

} push_protobuf_varint32_t;


/**
 * Allocate and initialize a new push_protobuf_varint32_t.
 */

push_protobuf_varint32_t *
push_protobuf_varint32_new();


#endif  /* PUSH_PROTOBUF_VARINT32_H */

