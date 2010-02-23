/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2010, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <hwm-buffer.h>

#include <push/basics.h>
#include <push/combinators.h>
#include <push/primitives.h>

#include <push/protobuf/basics.h>
#include <push/protobuf/field-map.h>
#include <push/protobuf/primitives.h>


push_callback_t *
push_protobuf_hwm_string_new(const char *name,
                             push_parser_t *parser,
                             hwm_buffer_t *buf)
{
    const char  *read_size_name;
    const char  *read_name;
    const char  *compose_name;

    push_callback_t  *read_size = NULL;
    push_callback_t  *read = NULL;
    push_callback_t  *compose = NULL;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "pb-hwm-string";

    read_size_name = push_string_concat(name, ".size");
    if (read_size_name == NULL) return NULL;

    read_name = push_string_concat(name, ".read");
    if (read_name == NULL) return NULL;

    compose_name = push_string_concat(name, ".compose");
    if (compose_name == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    read_size =
        push_protobuf_varint_size_new(read_size_name, parser);
    if (read_size == NULL)
        goto error;

    read = push_hwm_string_new(read_name, parser, buf);
    if (read == NULL)
        goto error;

    compose = push_compose_new(compose_name, parser, read_size, read);
    if (compose == NULL)
        goto error;

    return compose;

  error:
    /*
     * Before returning the NULL error code, free everything that we
     * might've created so far.
     */

    if (read_size != NULL)
        push_callback_free(read_size);

    if (read != NULL)
        push_callback_free(read);

    if (compose != NULL)
        push_callback_free(compose);

    return NULL;
}


bool
push_protobuf_add_hwm_string(const char *message_name,
                             const char *field_name,
                             push_parser_t *parser,
                             push_protobuf_field_map_t *field_map,
                             push_protobuf_tag_number_t field_number,
                             hwm_buffer_t *dest)
{
    const char  *full_field_name;
    push_callback_t  *field_callback;

    /*
     * First construct all of the names.
     */

    if (message_name == NULL)
        message_name = "message";

    if (field_name == NULL)
        field_name = ".hwm";

    full_field_name = push_string_concat(message_name, field_name);
    if (full_field_name == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    field_callback =
        push_protobuf_hwm_string_new(full_field_name, parser, dest);

    if (field_callback == NULL)
    {
        return false;
    }

    /*
     * Try to add the new field.  If we can't, free the callback
     * before returning.
     */

    if (!push_protobuf_field_map_add_field
        (full_field_name,
         parser, field_map, field_number,
         PUSH_PROTOBUF_TAG_TYPE_LENGTH_DELIMITED,
         field_callback))
    {
        push_callback_free(field_callback);
        return false;
    }

    return true;
}
