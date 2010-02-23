/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <inttypes.h>

#define VALUE_STR             "varint64"
#define PRETTY_STR            "uint64"
#define STRUCT_NAME           _assign_uint64
#define ASSIGN_T              assign_uint64_t
#define PARSED_T              uint64_t
#define PRIuPARSED            PRIu64
#define DEST_T                uint64_t
#define VALUE_CALLBACK_T      push_protobuf_varint64_t
#define VALUE_CALLBACK_NEW    push_protobuf_varint64_new
#define PUSH_PROTOBUF_ASSIGN  push_protobuf_assign_uint64
#define TAG_TYPE              PUSH_PROTOBUF_TAG_TYPE_VARINT

#include "assign-template.c"
