/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PUSH_PROTOBUF_FIELD_H
#define PUSH_PROTOBUF_FIELD_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <push.h>
#include <push/protobuf/basics.h>
#include <push/protobuf/varint32.h>

/**
 * A callback for parsing a single field from a protobuf message.
 */

typedef struct _push_protobuf_field
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * The expected tag type for this field.  If the actual tag type
     * encountered doesn't match, it's a parse error.
     */

    push_protobuf_tag_type_t  expected_tag_type;

    /**
     * The callback that will be used to parse the field's tag.
     */

    push_protobuf_varint32_t  *tag_callback;

    /**
     * The callback that verifies that the tag is correct for this
     * field.
     */

    push_callback_t  *verify_tag_callback;

    /**
     * The callback that will be used to parse the field's value.
     */

    push_callback_t  *value_callback;

    /**
     * The callback that will be used to finish processing of this
     * field.
     */

    push_callback_t  *finish_callback;

} push_protobuf_field_t;


/**
 * Allocate and initialize a new push_protobuf_field_t.
 */

push_protobuf_field_t *
push_protobuf_field_new(push_protobuf_tag_type_t expected_tag_type,
                        push_callback_t *value_callback,
                        bool eof_allowed);


#endif  /* PUSH_PROTOBUF_FIELD_H */
