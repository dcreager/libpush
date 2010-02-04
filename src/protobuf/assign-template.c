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

#include <push.h>
#include <push/protobuf.h>


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
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * The input value.
     */

    PARSED_T  *input;

    /**
     * The pointer to assign the value to.
     */

    DEST_T  *dest;

} ASSIGN_T;


static push_error_code_t
assign_activate(push_parser_t *parser,
                push_callback_t *pcallback,
                void *input)
{
    ASSIGN_T  *callback = (ASSIGN_T *) pcallback;

    callback->input = (PARSED_T *) input;
    PUSH_DEBUG_MSG(PRETTY_STR": Activating.  Got value "
                   "%"PRIuPARSED".\n",
                   *callback->input);

    return PUSH_SUCCESS;
}

static ssize_t
assign_process_bytes(push_parser_t *parser,
                     push_callback_t *pcallback,
                     const void *vbuf,
                     size_t bytes_available)
{
    ASSIGN_T  *callback = (ASSIGN_T *) pcallback;

    /*
     * Copy the value from the callback into the destination.
     */

    PUSH_DEBUG_MSG(PRETTY_STR": Assigning value to %p.\n",
                   callback->dest);
    *callback->dest = *callback->input;

    /*
     * We don't actually parse anything, so return success.
     */

    callback->base.result = callback->input;
    return bytes_available;
}


static push_callback_t *
assign_new(DEST_T *dest)
{
    ASSIGN_T  *result =
        (ASSIGN_T *) malloc(sizeof(ASSIGN_T));

    if (result == NULL)
        return NULL;

    /*
     * EOF is not allowed in place of assign — that's in between
     * the tag and the value, which is a parse error.
     */

    push_callback_init(&result->base,
                       assign_activate,
                       assign_process_bytes,
                       NULL);

    result->dest = dest;

    return &result->base;
}


bool
PUSH_PROTOBUF_ASSIGN(push_protobuf_field_map_t *field_map,
                     push_protobuf_tag_number_t field_number,
                     DEST_T *dest)
{
    push_callback_t  *value_callback = NULL;
    push_callback_t  *assign_callback = NULL;
    push_callback_t  *field_callback = NULL;

    /*
     * First, create the callbacks.
     */

    value_callback = VALUE_CALLBACK_NEW();
    if (value_callback == NULL)
        goto error;

    assign_callback = assign_new(dest);
    if (assign_callback == NULL)
        goto error;

    field_callback = push_compose_new(value_callback,
                                      assign_callback);
    if (field_callback == NULL)
        goto error;

    /*
     * Try to add the new field.  If we can't, free the field before
     * returning.  (We don't have to explicitly free the other two
     * callbacks at this point, since the field_callback is linked to
     * them.)
     */

    if (!push_protobuf_field_map_add_field(field_map, field_number,
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
