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


/*-----------------------------------------------------------------------
 * Dispatch callback
 */

/**
 * The callback class for the message's dispatch_callback.
 */

typedef struct _dispatch
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * A link back to the message callback.
     */

    push_protobuf_message_t  *message_callback;
} dispatch_t;


static ssize_t
dispatch_process_bytes(push_parser_t *parser,
                       push_callback_t *pcallback,
                       const void *vbuf,
                       size_t bytes_available)
{
    dispatch_t  *callback =
        (dispatch_t *) pcallback;

    push_protobuf_tag_t  field_tag;
    push_protobuf_tag_number_t  field_number;

    /*
     * Extract the field number from the tag_callback.
     */

    field_tag = callback->message_callback->tag_callback->value;
    field_number = PUSH_PROTOBUF_GET_TAG_NUMBER(field_tag);

    PUSH_DEBUG_MSG("message: Dispatching field %"PRIu32".\n",
                   field_number);

    /*
     * Loop through all of our field callbacks.  If we find one whose
     * field number matches, pass of to that callback.
     */

    {
        const push_protobuf_message_field_map_t  *field_map;
        unsigned int  i;

        field_map =
            hwm_buffer_mem(&callback->message_callback->field_map,
                           push_protobuf_message_field_map_t);

        for (i = 0;
             i < hwm_buffer_current_list_size
                 (&callback->message_callback->field_map,
                  push_protobuf_message_field_map_t);
             i++)
        {
            if (field_map[i].field_number == field_number)
            {
                /*
                 * Found one!  Switch over to that callback.
                 */

                push_protobuf_field_t  *field_callback =
                    field_map[i].callback;

                PUSH_DEBUG_MSG("message: Callback %p matches.\n",
                               field_callback);

                /*
                 * The field callback is going to need to verify the
                 * wire type, so make sure to tell it what tag we got.
                 */

                field_callback->actual_tag = field_tag;

                push_parser_set_callback(parser, &field_callback->base);
                return bytes_available;
            }
        }
    }

    /*
     * TODO: If we didn't find a field callback that wants to handle this
     * field, skip it.
     */

    PUSH_DEBUG_MSG("message: No field callback for field %"PRIu32".\n",
                   field_number);

    return PUSH_PARSE_ERROR;
}


static void
dispatch_free(push_callback_t *pcallback)
{
    dispatch_t  *callback =
        (dispatch_t *) pcallback;

    PUSH_DEBUG_MSG("message: Freeing dispatch callback %p...\n", pcallback);
    push_callback_free(&callback->message_callback->base);
}


static dispatch_t *
dispatch_new(push_protobuf_message_t *message_callback)
{
    dispatch_t  *result =
        (dispatch_t *) malloc(sizeof(dispatch_t));

    if (result == NULL)
        return NULL;

    /*
     * EOF is not allowed in place of dispatch — that's in between
     * the tag and the value, which is a parse error.
     */

    push_callback_init(&result->base,
                       0,
                       0,
                       NULL,
                       dispatch_process_bytes,
                       push_eof_not_allowed,
                       dispatch_free,
                       NULL);

    result->message_callback = message_callback;

    return result;
}


/*-----------------------------------------------------------------------
 * Top-level message callback
 */

static ssize_t
message_process_bytes(push_parser_t *parser,
                      push_callback_t *pcallback,
                      const void *vbuf,
                      size_t bytes_available)
{
    push_protobuf_message_t  *callback =
        (push_protobuf_message_t *) pcallback;

    /*
     * The message_callback doesn't actually do anything, we just
     * thread through our child callbacks.  Pass off right away into
     * the tag_callback.
     */

    PUSH_DEBUG_MSG("message: Switching to tag callback.\n");

    push_parser_set_callback(parser, &callback->tag_callback->base);
    return bytes_available;
}


static void
message_free(push_callback_t *pcallback)
{
    push_protobuf_message_t  *callback =
        (push_protobuf_message_t *) pcallback;

    PUSH_DEBUG_MSG("message: Freeing message callback %p...\n", pcallback);
    push_callback_free(&callback->tag_callback->base);
    push_callback_free(callback->dispatch_callback);

    /*
     * Free each of the field callbacks.
     */

    {
        push_protobuf_message_field_map_t  *field_map;
        unsigned int  i;

        field_map =
            hwm_buffer_writable_mem(&callback->field_map,
                                    push_protobuf_message_field_map_t);

        for (i = 0;
             i < hwm_buffer_current_list_size
                 (&callback->field_map,
                  push_protobuf_message_field_map_t);
             i++)
        {
            if (field_map[i].callback != NULL)
            {
                push_callback_t  *field_callback =
                    &field_map[i].callback->base;

                push_callback_free(field_callback);
            }
        }
    }
}


push_protobuf_message_t *
push_protobuf_message_new()
{
    push_protobuf_message_t  *result =
        (push_protobuf_message_t *) malloc(sizeof(push_protobuf_message_t));

    if (result == NULL)
    {
        PUSH_DEBUG_MSG("Couldn't allocate message callback.\n");
        goto error;
    }

    push_callback_init(&result->base,
                       0,
                       0,
                       NULL,
                       message_process_bytes,
                       push_eof_allowed,
                       message_free,
                       NULL);

    hwm_buffer_init(&result->field_map);

    /*
     * Try to create that callback for reading the message's tag.
     */

    result->tag_callback =
        push_protobuf_varint32_new(NULL, true);
    if (result->tag_callback == NULL)
    {
        PUSH_DEBUG_MSG("Couldn't allocate tag callback.\n");
        goto error;
    }

    /*
     * ...and then the callback for dispatching the field.
     */

    {
        dispatch_t  *dispatch_callback;

        dispatch_callback = dispatch_new(result);
        if (dispatch_callback == NULL)
        {
            PUSH_DEBUG_MSG("Couldn't allocate dispatch callback.\n");
            goto error;
        }

        result->dispatch_callback = &dispatch_callback->base;
    }

    /*
     * Finally, link all of the callbacks together.
     */

    result->tag_callback->base.next_callback =
        result->dispatch_callback;

    return result;

  error:
    /*
     * Before returning the NULL error code, free everything that we
     * might've created so far.
     */

    if (result != NULL)
        push_callback_free(&result->base);

    return NULL;
}


bool
push_protobuf_message_add_field(push_protobuf_message_t *message,
                                push_protobuf_tag_number_t field_number,
                                push_protobuf_field_t *field)
{
    push_protobuf_message_field_map_t  *new_field_map;

    /*
     * Try to allocate a new element in the list of field callbacks.
     * If we can't, return an error code.
     */

    new_field_map =
        hwm_buffer_append_list_elem(&message->field_map,
                                    push_protobuf_message_field_map_t);

    if (new_field_map == NULL)
        return false;

    /*
     * If the allocation worked, save the callback.
     */

    new_field_map->field_number = field_number;
    new_field_map->callback = field;

    return true;
}
