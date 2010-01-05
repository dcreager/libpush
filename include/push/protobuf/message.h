/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PROTOBUF_MESSAGE_H
#define PUSH_PROTOBUF_MESSAGE_H

#include <stdbool.h>

#include <hwm-buffer.h>

#include <push.h>
#include <push/protobuf/basics.h>
#include <push/protobuf/field.h>
#include <push/protobuf/varint32.h>


/**
 * A mapping of a field number to the callback that can read that
 * field.
 */

typedef struct _push_protobuf_message_field_map
{
    /**
     * The field number.
     */

    push_protobuf_tag_number_t  field_number;

    /**
     * The callback that can read this field.  The callback won't need
     * to parse the field's tag — the message callback will have taken
     * care of that already.
     */

    push_protobuf_field_t  *callback;

} push_protobuf_message_field_map_t;


/**
 * A callback for parsing a protobuf message with an arbitrary set of
 * fields.
 */

typedef struct _push_protobuf_message
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * A mapping of field numbers to the callback that reads the
     * corresponding field.  Stored as an expandable array of
     * push_protobuf_message_field_map_t instances.
     */

    hwm_buffer_t  field_map;

    /**
     * The callback that parses the next field's tag.
     */

    push_protobuf_varint32_t  *tag_callback;

    /**
     * The callback that dispatches to the right field callback for
     * the current field.
     */

    push_callback_t  *dispatch_callback;

} push_protobuf_message_t;


/**
 * Allocate and initialize a new push_protobuf_message_t.  The new
 * instance will not have any field callbacks in it; you'll need to
 * use the other functions in this header to add field callbacks
 * before the message callback is useful.
 */

push_protobuf_message_t *
push_protobuf_message_new();


/**
 * Add a new field callback to the message.  The field callback will
 * be called whenever we read a field with the matching field number.
 *
 * @return <code>false</code> if we cannot add the new field.
 */

bool
push_protobuf_message_add_field(push_protobuf_message_t *message,
                                push_protobuf_tag_number_t field_number,
                                push_protobuf_field_t *field);


/**
 * Add a new <code>uint32</code> field to the message callback.  When
 * parsing, the field's value will be assigned to the dest pointer.
 *
 * @return <code>false</code> if we cannot add the new field.
 */

bool
push_protobuf_assign_uint32(push_protobuf_message_t *message,
                            push_protobuf_tag_number_t field_number,
                            uint32_t *dest);


/**
 * Add a new <code>uint64</code> field to the message callback.  When
 * parsing, the field's value will be assigned to the dest pointer.
 *
 * @return <code>false</code> if we cannot add the new field.
 */

bool
push_protobuf_assign_uint64(push_protobuf_message_t *message,
                            push_protobuf_tag_number_t field_number,
                            uint64_t *dest);


#endif  /* PUSH_PROTOBUF_MESSAGE_H */
