/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PROTOBUF_PRIMITIVES_H
#define PUSH_PROTOBUF_PRIMITIVES_H

#include <hwm-buffer.h>

#include <push/basics.h>


/**
 * Create a new callback that reads a length-prefixed Protocol Buffer
 * string into a high-water mark buffer.
 */

push_callback_t *
push_protobuf_hwm_string_new(const char *name,
                             void *parent,
                             push_parser_t *parser,
                             hwm_buffer_t *buf);


/**
 * Create a new callback that skips over a length-prefixed Protocol
 * Buffers field.
 */

push_callback_t *
push_protobuf_skip_length_prefixed_new(const char *name,
                                       void *parent,
                                       push_parser_t *parser);


/**
 * Create a new callback for parsing a varint-encoded integer, which
 * we don't expect to be more than 32 bits.  The result pointer will
 * point at the parsed value, stored as a uint32_t.
 */

push_callback_t *
push_protobuf_varint32_new(const char *name,
                           void *parent,
                           push_parser_t *parser);


/**
 * Create a new callback for parsing a varint-encoded integer, which
 * we don't expect to be more than 64 bits.  The result pointer will
 * point at the parsed value, stored as a uint64_t.
 */

push_callback_t *
push_protobuf_varint64_new(const char *name,
                           void *parent,
                           push_parser_t *parser);


/**
 * Create a new callback for parsing a varint-encoded integer, which
 * we will use as a size.  The result pointer will point at the parsed
 * value, stored as a size_t.
 */

push_callback_t *
push_protobuf_varint_size_new(const char *name,
                              void *parent,
                              push_parser_t *parser);


#endif  /* PUSH_PROTOBUF_PRIMITIVES_H */
