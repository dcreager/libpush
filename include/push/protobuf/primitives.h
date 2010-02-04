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

#include <push/basics.h>


/**
 * Create a new callback for parsing a varint-encoded integer, which
 * we don't expect to be more than 32 bits.  The result pointer will
 * point at the parsed value, stored as a uint32_t.
 */

push_callback_t *
push_protobuf_varint32_new();


/**
 * Create a new callback for parsing a varint-encoded integer, which
 * we don't expect to be more than 64 bits.  The result pointer will
 * point at the parsed value, stored as a uint64_t.
 */

push_callback_t *
push_protobuf_varint64_new();


#endif  /* PUSH_PROTOBUF_PRIMITIVES_H */

