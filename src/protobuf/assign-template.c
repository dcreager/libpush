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
#include <push/protobuf/basics.h>
#include <push/protobuf/field-map.h>
#include <push/protobuf/primitives.h>


/**
 * @file
 *
 * This file isn't compiled directly.  Instead, the other assign-*
 * files in this directory set some macros and then include this file.
 * This ensures that all of the assign callbacks are implemented in a
 * consistent fashion.
 *
 * Expected macros:
 *
 *   - PRETTY_NAME
 *   - PARSED_T
 *   - PRIuPARSED
 *   - DEST_T
 *   - PROTOBUF_TYPE
 *   - TAG_TYPE
 */

/*-----------------------------------------------------------------------
 * Assign callback
 */

typedef struct STRUCT_NAME
{
    /**
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * The input value.
     */

    PARSED_T  *input;

    /**
     * The pointer to assign the value to.
     */

    DEST_T  *dest;

} ASSIGN_T;


static void
assign_activate(void *user_data,
                void *result,
                const void *buf,
                size_t bytes_remaining)
{
    ASSIGN_T  *assign = (ASSIGN_T *) user_data;

    assign->input = (PARSED_T *) result;
    PUSH_DEBUG_MSG("%s: Activating.  Got value "
                   "%"PRIuPARSED".\n",
                   assign->callback.name,
                   *assign->input);

    /*
     * Copy the value from the callback into the destination.
     */

    PUSH_DEBUG_MSG("%s: Assigning value to %p.\n",
                   assign->callback.name,
                   assign->dest);
    *assign->dest = *assign->input;

    push_continuation_call(assign->callback.success,
                           assign->input,
                           buf, bytes_remaining);

    return;
}


static push_callback_t *
assign_new(const char *name,
           push_parser_t *parser, DEST_T *dest)
{
    ASSIGN_T  *assign =
        (ASSIGN_T *) malloc(sizeof(ASSIGN_T));

    if (assign == NULL)
        return NULL;

    /*
     * Fill in the data items.
     */

    assign->dest = dest;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL)
        name = PRETTY_STR;

    push_callback_init(name,
                       &assign->callback, parser, assign,
                       assign_activate,
                       NULL, NULL, NULL);

    return &assign->callback;
}


bool
PUSH_PROTOBUF_ASSIGN(const char *message_name,
                     const char *field_name,
                     push_parser_t *parser,
                     push_protobuf_field_map_t *field_map,
                     push_protobuf_tag_number_t field_number,
                     DEST_T *dest)
{
    const char  *full_field_name;
    const char  *value_name;
    const char  *assign_name;
    const char  *compose_name;

    push_callback_t  *value_callback = NULL;
    push_callback_t  *assign_callback = NULL;
    push_callback_t  *field_callback = NULL;

    /*
     * First construct all of the names.
     */

    if (message_name == NULL)
        message_name = "message";

    if (field_name == NULL)
        field_name = ".assign";

    full_field_name = push_string_concat(message_name, field_name);
    if (full_field_name == NULL) return NULL;

    value_name = push_string_concat(full_field_name, "." VALUE_STR);
    if (value_name == NULL) return NULL;

    assign_name = push_string_concat(full_field_name, "." PRETTY_STR);
    if (assign_name == NULL) return NULL;

    compose_name = push_string_concat(full_field_name, ".compose");
    if (compose_name == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    value_callback = VALUE_CALLBACK_NEW(value_name, parser);
    if (value_callback == NULL)
        goto error;

    assign_callback = assign_new(assign_name, parser, dest);
    if (assign_callback == NULL)
        goto error;

    field_callback = push_compose_new(compose_name,
                                      parser, value_callback,
                                      assign_callback);
    if (field_callback == NULL)
        goto error;

    /*
     * Try to add the new field.  If we can't, free the field before
     * returning.  (We don't have to explicitly free the other two
     * callbacks at this point, since the field_callback is linked to
     * them.)
     */

    if (!push_protobuf_field_map_add_field(full_field_name,
                                           parser,
                                           field_map, field_number,
                                           TAG_TYPE, field_callback))
    {
        push_callback_free(field_callback);
        return false;
    }

    return true;

  error:
    /*
     * Before returning the NULL error code, free everything that we
     * might've created so far.
     */

    if (value_callback != NULL)
        push_callback_free(value_callback);

    if (assign_callback != NULL)
        push_callback_free(assign_callback);

    if (field_callback != NULL)
        push_callback_free(field_callback);

    return NULL;
}
