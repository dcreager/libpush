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
    PUSH_DEBUG_MSG(PRETTY_STR": Activating.  Got value "
                   "%"PRIuPARSED".\n",
                   *assign->input);

    /*
     * Copy the value from the callback into the destination.
     */

    PUSH_DEBUG_MSG(PRETTY_STR": Assigning value to %p.\n",
                   assign->dest);
    *assign->dest = *assign->input;

    push_continuation_call(assign->callback.success,
                           assign->input,
                           buf, bytes_remaining);

    return;
}


static push_callback_t *
assign_new(push_parser_t *parser, DEST_T *dest)
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

    push_callback_init(&assign->callback, parser, assign,
                       assign_activate,
                       NULL, NULL, NULL);

    return &assign->callback;
}


bool
PUSH_PROTOBUF_ASSIGN(push_parser_t *parser,
                     push_protobuf_field_map_t *field_map,
                     push_protobuf_tag_number_t field_number,
                     DEST_T *dest)
{
    push_callback_t  *value_callback = NULL;
    push_callback_t  *assign_callback = NULL;
    push_callback_t  *field_callback = NULL;

    /*
     * First, create the callbacks.
     */

    value_callback = VALUE_CALLBACK_NEW(parser);
    if (value_callback == NULL)
        goto error;

    assign_callback = assign_new(parser, dest);
    if (assign_callback == NULL)
        goto error;

    field_callback = push_compose_new(parser, value_callback,
                                      assign_callback);
    if (field_callback == NULL)
        goto error;

    /*
     * Try to add the new field.  If we can't, free the field before
     * returning.  (We don't have to explicitly free the other two
     * callbacks at this point, since the field_callback is linked to
     * them.)
     */

    if (!push_protobuf_field_map_add_field(parser,
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
