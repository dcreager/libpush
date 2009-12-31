/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PROTOBUF_H
#define PUSH_PROTOBUF_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

/**
 * @file
 *
 * Contains callback implementations for parsing Google Protocol
 * Buffer messages.
 */


#define PUSH_PROTOBUF_MAX_VARINT_LENGTH   10
#define PUSH_PROTOBUF_MAX_VARINT32_LENGTH  5


#define PUSH_PROTOBUF_WIRETYPE_VARINT            0
#define PUSH_PROTOBUF_WIRETYPE_FIXED64           1
#define PUSH_PROTOBUF_WIRETYPE_LENGTH_DELIMITED  2
#define PUSH_PROTOBUF_WIRETYPE_START_GROUP       3
#define PUSH_PROTOBUF_WIRETYPE_END_GROUP         4
#define PUSH_PROTOBUF_WIRETYPE_FIXED32           5


/**
 * A callback for parsing a varint-encoded integer, which we don't
 * expect to be more than 32 bits.  Once the integer has been parsed,
 * we pass off to next_callback.
 */

typedef struct _push_protobuf_varint32
{
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

    /**
     * The callback to pass off to once we've read the varint.
     */

    push_callback_t  *next_callback;
} push_protobuf_varint32_t;


/**
 * Allocate and initialize a new push_protobuf_varint32_t.
 */

push_protobuf_varint32_t *
push_protobuf_varint32_new(push_callback_t *next_callback,
                           bool eof_allowed);


#endif  /* PUSH_PROTOBUF_H */
