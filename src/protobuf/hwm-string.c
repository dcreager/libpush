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
                             void *parent,
                             push_parser_t *parser,
                             hwm_buffer_t *buf)
{
    void  *context;
    push_callback_t  *read_size = NULL;
    push_callback_t  *read = NULL;
    push_callback_t  *compose = NULL;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Create the callbacks.
     */

    if (name == NULL) name = "pb-hwm-string";

    read_size = push_protobuf_varint_size_new
        (push_talloc_asprintf(context, "%s.size", name),
         context, parser);
    read = push_hwm_string_new
        (push_talloc_asprintf(context, "%s.read", name),
         context, parser, buf);
    compose = push_compose_new
        (push_talloc_asprintf(context, "%s.compose", name),
         context, parser, read_size, read);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (compose == NULL) goto error;
    return compose;

  error:
    /*
     * Before returning, free any objects we created before the error.
     */

    push_talloc_free(context);
    return NULL;
}


bool
push_protobuf_add_hwm_string(const char *message_name,
                             const char *field_name,
                             void *parent,
                             push_parser_t *parser,
                             push_protobuf_field_map_t *field_map,
                             push_protobuf_tag_number_t field_number,
                             hwm_buffer_t *dest)
{
    void  *context;
    const char  *full_field_name;
    push_callback_t  *field_callback;

    /*
     * If the field map is NULL, return false.
     */

    if (field_map == NULL)
        return false;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Create the callbacks.
     */

    if (message_name == NULL) message_name = "message";
    if (field_name == NULL) field_name = ".hwm";

    full_field_name =
        push_talloc_asprintf(context, "%s.%s",
                             message_name, field_name);

    field_callback =
        push_protobuf_hwm_string_new
        (full_field_name,
         context, parser, dest);
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

    return true;

  error:
    /*
     * Before returning, free any objects we created before the error.
     */

    push_talloc_free(context);
    return false;
}
