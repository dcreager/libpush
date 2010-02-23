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
push_protobuf_add_submessage(const char *message_name,
                             const char *field_name,
                             push_parser_t *parser,
                             push_protobuf_field_map_t *field_map,
                             push_protobuf_tag_number_t field_number,
                             push_callback_t *message_callback)
{
    const char  *full_field_name;
    const char  *prefixed_name;

    push_callback_t  *varint_prefixed;

    /*
     * First construct all of the names.
     */

    if (message_name == NULL)
        message_name = "message";

    if (field_name == NULL)
        field_name = ".submessage";

    full_field_name = push_string_concat(message_name, field_name);
    if (full_field_name == NULL) return NULL;

    prefixed_name = push_string_concat(full_field_name, ".limit");
    if (prefixed_name == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    varint_prefixed =
        push_protobuf_varint_prefixed_new(prefixed_name,
                                          parser, message_callback);

    if (varint_prefixed == NULL)
    {
        return false;
    }

    /*
     * Try to add the new field.  If we can't, free the compose before
     * returning.
     */

    if (!push_protobuf_field_map_add_field
        (full_field_name, parser, field_map, field_number,
         PUSH_PROTOBUF_TAG_TYPE_LENGTH_DELIMITED,
         varint_prefixed))
    {
        push_callback_free(varint_prefixed);
        return false;
    }

    return true;
}
