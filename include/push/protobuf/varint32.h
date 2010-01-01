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
 * expect to be more than 32 bits.  Once the integer has been parsed,
 * we pass off to next_callback.
 */

typedef struct _push_protobuf_varint32
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * The number of bytes currently added to the varint.  After we've
     * parsed an integer, we reset the count 0, so that we're ready to
     * process another one later.
     */

    size_t  bytes_processed;

    /**
     * The parsed value.
     */

    uint32_t  value;

    /**
     * Whether EOF is allowed in place of this varint.
     */

    bool  eof_allowed;

} push_protobuf_varint32_t;


/**
 * Allocate and initialize a new push_protobuf_varint32_t.
 */

push_protobuf_varint32_t *
push_protobuf_varint32_new(push_callback_t *next_callback,
                           bool eof_allowed);


#endif  /* PUSH_PROTOBUF_VARINT32_H */

