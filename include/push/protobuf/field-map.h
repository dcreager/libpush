/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PROTOBUF_FIELD_MAP_H
#define PUSH_PROTOBUF_FIELD_MAP_H

#include <stdbool.h>

#include <hwm-buffer.h>

#include <push/basics.h>
#include <push/protobuf/basics.h>


/**
 * A map of field numbers to callbacks for reading that field.  Used
 * within a message callback; once it reads in a field number, it
 * dispatches to a callback that can read that field.  This map stores
 * the list of possible callbacks.
 */

typedef struct _push_protobuf_field_map  push_protobuf_field_map_t;


/**
 * Create a new field map.  The field map should be created and
 * populated before creating the message callback that will use it.
 */

push_protobuf_field_map_t *
push_protobuf_field_map_new();


/**
 * Free a field map.  This should not generally need to be called by
 * your code; if you've created a message callback that uses this
 * field map, it will free the field map when it gets freed.  If
 * you've created the field map, though, and haven't had a chance to
 * pass it off to a message callback (memory allocation error,
 * anyone?), then you probably should free it yourself.
 */

void
push_protobuf_field_map_free(push_protobuf_field_map_t *field_map);


/**
 * Add a new field to a field map.  The value callback will be called
 * whenever we read a field with the matching field number.  This
 * value callback should also store the parsed value away somewhere,
 * if needed.  We also verify that the tag type matches what we
 * expect, throwing a parse error if it doesn't.
 *
 * @return <code>false</code> if we cannot add the new field.
 */

bool
push_protobuf_field_map_add_field
    (push_protobuf_field_map_t *field_map,
     push_protobuf_tag_number_t field_number,
     push_protobuf_tag_type_t expected_tag_type,
     push_callback_t *value_callback);


/**
 * Get the field callback for the specified field.  If that field
 * isn't in the field map, return NULL.
 */

push_callback_t *
push_protobuf_field_map_get_field
    (push_protobuf_field_map_t *field_map,
     push_protobuf_tag_number_t field_number);


/**
 * Add a new submessage to a field map.
 *
 * @return <code>false</code> if we cannot add the new field.
 */

bool
push_protobuf_add_submessage(push_protobuf_field_map_t *field_map,
                             push_protobuf_tag_number_t field_number,
                             push_callback_t *message);


/**
 * Create a new callback that reads a length-prefixed Protocol Buffer
 * string into a high-water mark buffer.
 */

bool
push_protobuf_add_hwm_string(push_protobuf_field_map_t *field_map,
                             push_protobuf_tag_number_t field_number,
                             hwm_buffer_t *dest);


/**
 * Add a new <code>uint32</code> field to a field map.  When parsing,
 * the field's value will be assigned to the dest pointer.
 *
 * @return <code>false</code> if we cannot add the new field.
 */

bool
push_protobuf_assign_uint32(push_protobuf_field_map_t *field_map,
                            push_protobuf_tag_number_t field_number,
                            uint32_t *dest);


/**
 * Add a new <code>uint64</code> field to a field map.  When parsing,
 * the field's value will be assigned to the dest pointer.
 *
 * @return <code>false</code> if we cannot add the new field.
 */

bool
push_protobuf_assign_uint64(push_protobuf_field_map_t *field_map,
                            push_protobuf_tag_number_t field_number,
                            uint64_t *dest);


#endif  /* PUSH_PROTOBUF_FIELD_MAP_H */
