/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdbool.h>

#include <push/basics.h>
#include <push/protobuf/basics.h>
#include <push/protobuf/field-map.h>
#include <push/protobuf/combinators.h>

bool
push_protobuf_add_submessage(push_protobuf_field_map_t *field_map,
                             push_protobuf_tag_number_t field_number,
                             push_callback_t *message_callback)
{
    push_callback_t  *varint_prefixed;

    /*
     * First, create the field callback.
     */

    varint_prefixed =
        push_protobuf_varint_prefixed_new(message_callback);

    if (varint_prefixed == NULL)
    {
        return false;
    }

    /*
     * Try to add the new field.  If we can't, free the compose before
     * returning.
     */

    if (!push_protobuf_field_map_add_field
        (field_map, field_number,
         PUSH_PROTOBUF_TAG_TYPE_LENGTH_DELIMITED,
         varint_prefixed))
    {
        push_callback_free(varint_prefixed);
        return false;
    }

    return true;
}
