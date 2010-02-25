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
#include <stdbool.h>
#include <stdint.h>

#include <push/basics.h>
#include <push/combinators.h>
#include <push/pure.h>
#include <push/primitives.h>
#include <push/talloc.h>

#include <push/protobuf/basics.h>
#include <push/protobuf/field-map.h>
#include <push/protobuf/primitives.h>



static bool
assign_uint32(uint32_t *dest, uint32_t *input, uint32_t **output)
{
    *dest = *input;
    *output = dest;
    return true;
}

push_define_pure_callback(assign_uint32_new, assign_uint32, "assign",
                          uint32_t, uint32_t, uint32_t);



static bool
assign_uint64(uint64_t *dest, uint64_t *input, uint64_t **output)
{
    *dest = *input;
    *output = dest;
    return true;
}

push_define_pure_callback(assign_uint64_new, assign_uint64, "assign",
                          uint64_t, uint64_t, uint64_t);



static bool
assign_int32(int32_t *dest, uint32_t *input, int32_t **output)
{
    *dest = *input;
    *output = dest;
    return true;
}

push_define_pure_callback(assign_int32_new, assign_int32, "assign",
                          uint32_t, int32_t, int32_t);



static bool
assign_int64(int64_t *dest, uint64_t *input, int64_t **output)
{
    *dest = *input;
    *output = dest;
    return true;
}

push_define_pure_callback(assign_int64_new, assign_int64, "assign",
                          uint64_t, int64_t, int64_t);



static bool
assign_sint32(int32_t *dest, uint32_t *input, int32_t **output)
{
    *dest = PUSH_PROTOBUF_ZIGZAG_DECODE32(*input);
    *output = dest;
    return true;
}

push_define_pure_callback(assign_sint32_new, assign_sint32, "assign",
                          uint32_t, int32_t, int32_t);



static bool
assign_sint64(int64_t *dest, uint64_t *input, int64_t **output)
{
    *dest = PUSH_PROTOBUF_ZIGZAG_DECODE64(*input);
    *output = dest;
    return true;
}

push_define_pure_callback(assign_sint64_new, assign_sint64, "assign",
                          uint64_t, int64_t, int64_t);



#define ADD_FIELD(ASSIGN, VALUE_STR, VALUE_CALLBACK_NEW,                \
                  DEST_STR, DEST_T, ASSIGN_NEW,                         \
                  TAG_TYPE)                                             \
bool                                                                    \
ASSIGN(const char *message_name,                                        \
       const char *field_name,                                          \
       void *parent,                                                    \
       push_parser_t *parser,                                           \
       push_protobuf_field_map_t *field_map,                            \
       push_protobuf_tag_number_t field_number,                         \
       DEST_T *dest)                                                    \
{                                                                       \
    void  *context;                                                     \
    const char  *full_field_name;                                       \
    push_callback_t  *value;                                            \
    push_callback_t  *assign;                                           \
    push_callback_t  *field;                                            \
                                                                        \
    /*                                                                  \
     * If the field map is NULL, return false.                          \
     */                                                                 \
                                                                        \
    if (field_map == NULL)                                              \
        return false;                                                   \
                                                                        \
    /*                                                                  \
     * Create a memory context for the objects we're about to create.   \
     */                                                                 \
                                                                        \
    context = push_talloc_new(parent);                                  \
    if (context == NULL) return NULL;                                   \
                                                                        \
    /*                                                                  \
     * Create the callbacks.                                            \
     */                                                                 \
                                                                        \
    if (message_name == NULL) message_name = "message";                 \
    if (field_name == NULL) field_name = ".assign";                     \
                                                                        \
    full_field_name =                                                   \
        push_talloc_asprintf(context, "%s.%s",                          \
                             message_name, field_name);                 \
                                                                        \
    value = VALUE_CALLBACK_NEW                                          \
        (push_talloc_asprintf(context, "%s." VALUE_STR,                 \
                              full_field_name),                         \
         context, parser);                                              \
    assign = ASSIGN_NEW                                                 \
        (push_talloc_asprintf(context, "%s." DEST_STR,                  \
                              full_field_name),                         \
         context, parser,                                               \
         dest);                                                         \
    field = push_compose_new                                            \
        (push_talloc_asprintf(context, "%s.compose",                    \
                              full_field_name),                         \
         context, parser,                                               \
         value, assign);                                                \
                                                                        \
    /*                                                                  \
     * Because of NULL propagation, we only have to check the last      \
     * result to see if everything was created okay.                    \
     */                                                                 \
                                                                        \
    if (field == NULL) goto error;                                      \
                                                                        \
    /*                                                                  \
     * Try to add the new field.  If we can't, free the field before    \
     * returning.                                                       \
     */                                                                 \
                                                                        \
    if (!push_protobuf_field_map_add_field                              \
        (full_field_name, parser,                                       \
         field_map, field_number, TAG_TYPE, field))                     \
    {                                                                   \
        goto error;                                                     \
    }                                                                   \
                                                                        \
    return true;                                                        \
                                                                        \
  error:                                                                \
    /*                                                                  \
     * Before returning, free any objects we created before the error.  \
     */                                                                 \
                                                                        \
    push_talloc_free(context);                                          \
    return false;                                                       \
}


ADD_FIELD(push_protobuf_assign_uint32,
          "varint32", push_protobuf_varint32_new,
          "uint32", uint32_t, assign_uint32_new,
          PUSH_PROTOBUF_TAG_TYPE_VARINT);


ADD_FIELD(push_protobuf_assign_uint64,
          "varint64", push_protobuf_varint64_new,
          "uint64", uint64_t, assign_uint64_new,
          PUSH_PROTOBUF_TAG_TYPE_VARINT);


ADD_FIELD(push_protobuf_assign_int32,
          "varint32", push_protobuf_varint32_new,
          "int32", int32_t, assign_int32_new,
          PUSH_PROTOBUF_TAG_TYPE_VARINT);


ADD_FIELD(push_protobuf_assign_int64,
          "varint64", push_protobuf_varint64_new,
          "int64", int64_t, assign_int64_new,
          PUSH_PROTOBUF_TAG_TYPE_VARINT);


ADD_FIELD(push_protobuf_assign_sint32,
          "varint32", push_protobuf_varint32_new,
          "int32", int32_t, assign_sint32_new,
          PUSH_PROTOBUF_TAG_TYPE_VARINT);


ADD_FIELD(push_protobuf_assign_sint64,
          "varint64", push_protobuf_varint64_new,
          "int64", int64_t, assign_sint64_new,
          PUSH_PROTOBUF_TAG_TYPE_VARINT);
