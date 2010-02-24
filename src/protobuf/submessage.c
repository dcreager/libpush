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
#include <push/talloc.h>

#include <push/protobuf/basics.h>
#include <push/protobuf/field-map.h>
#include <push/protobuf/combinators.h>

bool
push_protobuf_add_submessage(const char *message_name,
                             const char *field_name,
                             void *parent,
                             push_parser_t *parser,
                             push_protobuf_field_map_t *field_map,
                             push_protobuf_tag_number_t field_number,
                             push_callback_t *message_callback)
{
    void  *context;
    const char  *full_field_name;
    push_callback_t  *varint_prefixed = NULL;

    /*
     * If the field map or message callback is NULL, return false.
     */

    if ((field_map == NULL) || (message_callback == NULL))
        return NULL;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Create the callbacks.
     */

    if (message_name == NULL) message_name = "message";
    if (field_name == NULL) field_name = ".submessage";

    full_field_name =
        push_talloc_asprintf(context, "%s.%s",
                             message_name, field_name);

    varint_prefixed = push_protobuf_varint_prefixed_new
        (push_talloc_asprintf(context, "%s.dup",
                              full_field_name),
         context, parser, message_callback);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (varint_prefixed == NULL) goto error;

    /*
     * Try to add the new field.  If we can't, free the compose before
     * returning.
     */

    if (!push_protobuf_field_map_add_field
        (full_field_name, parser, field_map, field_number,
         PUSH_PROTOBUF_TAG_TYPE_LENGTH_DELIMITED,
         varint_prefixed))
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
