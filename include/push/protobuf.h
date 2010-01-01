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

/**
 * @file
 *
 * Contains callback implementations for parsing Google Protocol
 * Buffer messages.
 */

#include <push/protobuf/varint32.h>
#include <push/protobuf/varint64.h>


#define PUSH_PROTOBUF_MAX_VARINT_LENGTH   10
#define PUSH_PROTOBUF_MAX_VARINT32_LENGTH  5


#define PUSH_PROTOBUF_WIRETYPE_VARINT            0
#define PUSH_PROTOBUF_WIRETYPE_FIXED64           1
#define PUSH_PROTOBUF_WIRETYPE_LENGTH_DELIMITED  2
#define PUSH_PROTOBUF_WIRETYPE_START_GROUP       3
#define PUSH_PROTOBUF_WIRETYPE_END_GROUP         4
#define PUSH_PROTOBUF_WIRETYPE_FIXED32           5


#endif  /* PUSH_PROTOBUF_H */
