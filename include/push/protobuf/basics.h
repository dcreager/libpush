/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PROTOBUF_BASICS_H
#define PUSH_PROTOBUF_BASICS_H

#include <stdint.h>

/**
 * The tag portion of a Protocol Buffer field.  Consists of a <i>tag
 * number</i> (represented by push_protobuf_tag_number_t) and a <i>tag
 * type</i> (represented by push_protobuf_tag_type_t).
 */

typedef uint32_t  push_protobuf_tag_t;

/**
 * The field ID portion of a Protocol Buffer tag.
 */

typedef uint32_t  push_protobuf_tag_number_t;

/**
 * The field type portion of a Protocol Buffer tag.
 */

typedef enum _push_protobuf_tag_type
{
    PUSH_PROTOBUF_TAG_TYPE_VARINT = 0,
    PUSH_PROTOBUF_TAG_TYPE_FIXED64 = 1,
    PUSH_PROTOBUF_TAG_TYPE_LENGTH_DELIMITED = 2,
    PUSH_PROTOBUF_TAG_TYPE_START_GROUP = 3,
    PUSH_PROTOBUF_TAG_TYPE_END_GROUP = 4,
    PUSH_PROTOBUF_TAG_TYPE_FIXED32 = 5
} push_protobuf_tag_type_t;


/**
 * Construct a push_protobuf_tag_t from a push_protobuf_tag_number_t
 * and a push_protobuf_tag_type_t.
 */

#define PUSH_PROTOBUF_MAKE_TAG(field_number, tag_type)  \
    (((field_number) << 3) | ((tag_type) & 0x07))


/**
 * Extract the tag type from a push_protobuf_tag_t.
 */

#define PUSH_PROTOBUF_GET_TAG_TYPE(tag) ((tag) & 0x07)


/**
 * Extract the tag number from a push_protobuf_tag_t.
 */

#define PUSH_PROTOBUF_GET_TAG_NUMBER(tag) ((tag) >> 3)


/**
 * The maximum length of a varint-encoded integer.
 */

#define PUSH_PROTOBUF_MAX_VARINT_LENGTH   10


/**
 * The maximum length of a varint-encoded integer whose value fits
 * into 32 bits.
 */

#define PUSH_PROTOBUF_MAX_VARINT32_LENGTH  5


/**
 * Encode an int32_t into a varint-appropriate uint32_t using the
 * Protocol Buffer zig-zag encoding.  This ensures that numbers with
 * small magnitude have smaller serializations.
 */

#define PUSH_PROTOBUF_ZIGZAG_ENCODE32(n)        \
    (((n) << 1) ^ ((n) >> 31))


/**
 * Decode a varint uint32_t into an int32_t using the Protocol Buffer
 * zig-zag encoding.
 */

#define PUSH_PROTOBUF_ZIGZAG_DECODE32(n)        \
    (((n) >> 1) ^ -((int32_t) (n) & 1))


/**
 * Encode an int64_t into a varint-appropriate uint64_t using the
 * Protocol Buffer zig-zag encoding.  This ensures that numbers with
 * small magnitude have smaller serializations.
 */

#define PUSH_PROTOBUF_ZIGZAG_ENCODE64(n)        \
    (((n) << 1) ^ ((n) >> 63))


/**
 * Decode a varint uint64_t into an int64_t using the Protocol Buffer
 * zig-zag encoding.
 */

#define PUSH_PROTOBUF_ZIGZAG_DECODE64(n)        \
    (((n) >> 1) ^ -((int64_t) (n) & 1))


#endif  /* PUSH_PROTOBUF_BASICS_H */
