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
#include <push/talloc.h>

#include <push/protobuf/basics.h>
#include <push/protobuf/field-map.h>
#include <push/protobuf/primitives.h>


push_callback_t *
push_protobuf_hwm_string_new(const char *name,
                             push_parser_t *parser,
                             hwm_buffer_t *buf)
{
    const char  *read_size_name = NULL;
    const char  *read_name = NULL;
    const char  *compose_name = NULL;

    push_callback_t  *read_size = NULL;
    push_callback_t  *read = NULL;
    push_callback_t  *compose = NULL;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "pb-hwm-string";

    read_size_name = push_string_concat(parser, name, ".size");
    if (read_size_name == NULL) goto error;

    read_name = push_string_concat(parser, name, ".read");
    if (read_name == NULL) goto error;

    compose_name = push_string_concat(parser, name, ".compose");
    if (compose_name == NULL) goto error;

    /*
     * Then create the callbacks.
     */

    read_size =
        push_protobuf_varint_size_new(read_size_name, parser);
    if (read_size == NULL) goto error;

    read = push_hwm_string_new(read_name, parser, buf);
    if (read == NULL) goto error;

    compose = push_compose_new(compose_name, parser, read_size, read);
    if (compose == NULL) goto error;

    /*
     * Make each name string be the child of its callback.
     */

    push_talloc_steal(read_size, read_size_name);
    push_talloc_steal(read, read_name);
    push_talloc_steal(compose, compose_name);

    return compose;

  error:
    if (read_size_name != NULL) push_talloc_free(read_size_name);
    if (read_size != NULL) push_talloc_free(read_size);

    if (read_name != NULL) push_talloc_free(read_name);
    if (read != NULL) push_talloc_free(read);

    if (compose_name != NULL) push_talloc_free(compose_name);
    if (compose != NULL) push_talloc_free(compose);

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
    const char  *full_field_name = NULL;
    push_callback_t  *field_callback = NULL;

    /*
     * First construct all of the names.
     */

    if (message_name == NULL)
        message_name = "message";

    if (field_name == NULL)
        field_name = ".hwm";

    full_field_name =
        push_string_concat(parser, message_name, field_name);
    if (full_field_name == NULL) goto error;

    /*
     * Then create the callbacks.
     */

    field_callback =
        push_protobuf_hwm_string_new(full_field_name, parser, dest);
    if (field_callback == NULL) goto error;

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
        goto error;
    }

    /*
     * Make each name string be the child of its callback.
     */

    push_talloc_steal(field_callback, full_field_name);

    return true;

  error:
    if (full_field_name != NULL) push_talloc_free(full_field_name);

    if (field_callback != NULL) push_talloc_free(field_callback);

    return false;
}
