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

#define PRETTY_STR            "uint32"
#define STRUCT_NAME           _assign_uint32
#define ASSIGN_T              assign_uint32_t
#define PARSED_T              uint32_t
#define PRIuPARSED            PRIu32
#define DEST_T                uint32_t
#define VALUE_CALLBACK_T      push_protobuf_varint32_t
#define VALUE_CALLBACK_NEW    push_protobuf_varint32_new
#define PUSH_PROTOBUF_ASSIGN  push_protobuf_assign_uint32
#define TAG_TYPE              PUSH_PROTOBUF_TAG_TYPE_VARINT

#include "assign-template.c"
