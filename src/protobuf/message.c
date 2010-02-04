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
#include <push/protobuf/basics.h>
#include <push/protobuf/field-map.h>
#include <push/protobuf/message.h>
#include <push/protobuf/primitives.h>


/*-----------------------------------------------------------------------
 * Dispatch callback
 */

/**
 * A callback that takes in a field tag as input, and then dispatches
 * to a reader callback for that field.  The readers are stored in a
 * “field map”, which is an expandable array of
 * push_protobuf_message_field_map_t instances.
 */

typedef struct _dispatch
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * A mapping of field numbers to the callback that reads the
     * corresponding field.
     */

    push_protobuf_field_map_t  *field_map;

    /**
     * The field callback that we should dispatch to.
     */

    push_callback_t  *dispatch_callback;

} dispatch_t;


static push_error_code_t
dispatch_activate(push_parser_t *parser,
                  push_callback_t *pcallback,
                  void *input)
{
    dispatch_t  *callback = (dispatch_t *) pcallback;
    push_protobuf_tag_t  *field_tag;
    push_protobuf_tag_number_t  field_number;
    push_callback_t  *field_callback;

    field_tag = (push_protobuf_tag_t *) input;
    PUSH_DEBUG_MSG("dispatch: Activating.  Got tag 0x%04"PRIx32"\n",
                   *field_tag);

    /*
     * Extract the field number from the tag_callback.
     */

    field_number = PUSH_PROTOBUF_GET_TAG_NUMBER(*field_tag);

    PUSH_DEBUG_MSG("dispatch: Dispatching field %"PRIu32".\n",
                   field_number);

    /*
     * Get the field callback for this field number.
     */

    field_callback =
        push_protobuf_field_map_get_field(callback->field_map,
                                          field_number);

    if (field_callback == NULL)
    {
        /*
         * TODO: If we didn't find a field callback that wants to
         * handle this field, skip it.
         */

        PUSH_DEBUG_MSG("dispatch: No field callback for field %"PRIu32".\n",
                       field_number);

        return PUSH_PARSE_ERROR;

    } else {
        push_error_code_t  activate_result;

        /*
         * Found it!  Activate that callback, and save it so that our
         * process_bytes function can pass off to it.
         */

        PUSH_DEBUG_MSG("dispatch: Callback %p matches.\n",
                       field_callback);

        /*
         * The field callback is going to need to verify the wire
         * type, so make sure to pass in the tag as input.
         */

        activate_result =
            push_callback_activate(parser, field_callback,
                                   field_tag);

        if (activate_result != PUSH_SUCCESS)
        {
            PUSH_DEBUG_MSG("dispatch: Could not activate field "
                           "callback.\n");
            return activate_result;
        }

        callback->dispatch_callback = field_callback;
        return PUSH_SUCCESS;
    }

}


static ssize_t
dispatch_process_bytes(push_parser_t *parser,
                       push_callback_t *pcallback,
                       const void *vbuf,
                       size_t bytes_available)
{
    dispatch_t  *callback = (dispatch_t *) pcallback;

    /*
     * We figured out which callback to use when we activated, so just
     * pass off to it.  Use a tail call so that its result is our
     * result.
     */

    return push_callback_tail_process_bytes
        (parser, &callback->base,
         callback->dispatch_callback,
         vbuf, bytes_available);
}


static void
dispatch_free(push_callback_t *pcallback)
{
    dispatch_t  *callback = (dispatch_t *) pcallback;

    PUSH_DEBUG_MSG("dispatch: Freeing field map...\n");
    push_protobuf_field_map_free(callback->field_map);
}


static push_callback_t *
dispatch_new(push_protobuf_field_map_t *field_map)
{
    dispatch_t  *result =
        (dispatch_t *) malloc(sizeof(dispatch_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       dispatch_activate,
                       dispatch_process_bytes,
                       dispatch_free);

    result->field_map = field_map;

    return &result->base;
}


/*-----------------------------------------------------------------------
 * Top-level message callback
 */


push_callback_t *
push_protobuf_message_new(push_protobuf_field_map_t *field_map)
{
    push_callback_t  *read_field_tag;
    push_callback_t  *dispatch;
    push_callback_t  *compose;
    push_callback_t  *fold;

    read_field_tag = push_protobuf_varint32_new();
    if (read_field_tag == NULL)
    {
        return NULL;
    }

    dispatch = dispatch_new(field_map);
    if (dispatch == NULL)
    {
        push_callback_free(read_field_tag);
        return NULL;
    }

    compose = push_compose_new(read_field_tag, dispatch);
    if (compose == NULL)
    {
        push_callback_free(read_field_tag);
        push_callback_free(dispatch);
        return NULL;
    }

    fold = push_fold_new(compose);
    if (fold == NULL)
    {
        /*
         * We only have to free the compose, since it links to the
         * other two.
         */

        push_callback_free(compose);
        return NULL;
    }

    return fold;
}
