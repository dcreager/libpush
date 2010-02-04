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

#include <hwm-buffer.h>

#include <push/basics.h>
#include <push/combinators.h>
#include <push/protobuf/basics.h>
#include <push/protobuf/field-map.h>


/**
 * A mapping of a field number to the callback that can read that
 * field.
 */

typedef struct _field_map_entry
{
    /**
     * The field number.
     */

    push_protobuf_tag_number_t  field_number;

    /**
     * The callback that can read this field.  The callback won't need
     * to parse the field's tag — the message callback will have taken
     * care of that already.
     */

    push_callback_t  *callback;

} field_map_entry_t;


struct _push_protobuf_field_map
{
    /**
     * A mapping of field numbers to the callback that reads the
     * corresponding field.  Stored as an expandable array of
     * field_map_entry_t instances.
     */

    hwm_buffer_t  entries;
};


push_protobuf_field_map_t *
push_protobuf_field_map_new()
{
    push_protobuf_field_map_t  *field_map =
        (push_protobuf_field_map_t *)
        malloc(sizeof(push_protobuf_field_map_t));

    if (field_map == NULL)
        return NULL;

    hwm_buffer_init(&field_map->entries);
    return field_map;
}


void
push_protobuf_field_map_free(push_protobuf_field_map_t *field_map)
{
    field_map_entry_t  *entries;
    unsigned int  i;

    entries =
        hwm_buffer_writable_mem(&field_map->entries,
                                field_map_entry_t);

    for (i = 0;
         i < hwm_buffer_current_list_size
             (&field_map->entries, field_map_entry_t);
         i++)
    {
        if (entries[i].callback != NULL)
        {
            PUSH_DEBUG_MSG("message: Freeing callback for "
                           "field %"PRIu32"...\n",
                           entries[i].field_number);
            push_callback_free(entries[i].callback);
        }
    }

    free(field_map);
}


push_callback_t *
push_protobuf_field_map_get_field
    (push_protobuf_field_map_t *field_map,
     push_protobuf_tag_number_t field_number)
{
    const field_map_entry_t  *entries;
    unsigned int  i;

    entries =
        hwm_buffer_mem(&field_map->entries, field_map_entry_t);

    for (i = 0;
         i < hwm_buffer_current_list_size
             (&field_map->entries, field_map_entry_t);
         i++)
    {
        if (entries[i].field_number == field_number)
        {
            /*
             * Found one!  Return the callback.
             */

            return entries[i].callback;
        }
    }

    /*
     * If we fall through the loop, there wasn't a matching callack.
     */

    return NULL;
}


/*-----------------------------------------------------------------------
 * Verify tag callback
 */

/**
 * A callback that verifies that a field tag has the expected type.
 * The field tag should be passed in as input as a uint32_t.  If it
 * matches the expected field type, we succeed; otherwise, we throw a
 * parse error.
 */

typedef struct _verify_tag
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * A pointer to the field tag we take in as input.
     */

    push_protobuf_tag_t  *actual_tag;

    /**
     * A link back to the field callback.
     */

    push_protobuf_tag_type_t  expected_tag_type;

} verify_tag_t;


static push_error_code_t
verify_tag_activate(push_parser_t *parser,
                    push_callback_t *pcallback,
                    void *input)
{
    verify_tag_t  *callback = (verify_tag_t *) pcallback;

    callback->actual_tag = (push_protobuf_tag_t *) input;
    PUSH_DEBUG_MSG("verify-tag: Activating.  Got tag 0x%04"PRIx32"\n",
                   *callback->actual_tag);

    return PUSH_SUCCESS;
}


static ssize_t
verify_tag_process_bytes(push_parser_t *parser,
                         push_callback_t *pcallback,
                         const void *vbuf,
                         size_t bytes_available)
{
    verify_tag_t  *callback = (verify_tag_t *) pcallback;

    PUSH_DEBUG_MSG("verify-tag: Got tag type %d, expecting tag type %d.\n",
                   PUSH_PROTOBUF_GET_TAG_TYPE(*callback->actual_tag),
                   callback->expected_tag_type);

    if (PUSH_PROTOBUF_GET_TAG_TYPE(*callback->actual_tag) ==
        callback->expected_tag_type)
    {
        /*
         * Tag types match, so move on to parsing the value.
         */

        PUSH_DEBUG_MSG("verify-tag: Tag types match.\n");

        return bytes_available;
    } else {
        /*
         * Tag types don't match, we've got a parse error!
         */

        PUSH_DEBUG_MSG("verify-tag: Tag types don't match.\n");

        return PUSH_PARSE_ERROR;
    }
}


static push_callback_t *
verify_tag_new(push_protobuf_tag_type_t expected_tag_type)
{
    verify_tag_t  *result =
        (verify_tag_t *) malloc(sizeof(verify_tag_t));

    if (result == NULL)
        return NULL;

    push_callback_init(&result->base,
                       verify_tag_activate,
                       verify_tag_process_bytes,
                       NULL);

    result->expected_tag_type = expected_tag_type;
    result->base.result = NULL;

    return &result->base;
}


/*-----------------------------------------------------------------------
 * Field callbacks
 */

static push_callback_t *
create_field_callback(push_protobuf_tag_type_t expected_tag_type,
                      push_callback_t *value_callback)
{
    push_callback_t  *verify_tag;
    push_callback_t  *compose;

    verify_tag = verify_tag_new(expected_tag_type);
    if (verify_tag == NULL)
    {
        return NULL;
    }

    compose = push_compose_new(verify_tag, value_callback);
    if (compose == NULL)
    {
        push_callback_free(verify_tag);
        return NULL;
    }

    return compose;
}


bool
push_protobuf_field_map_add_field
    (push_protobuf_field_map_t *field_map,
     push_protobuf_tag_number_t field_number,
     push_protobuf_tag_type_t expected_tag_type,
     push_callback_t *value_callback)
{
    field_map_entry_t  *new_entry;
    push_callback_t  *field_callback;

    /*
     * First, try to create a field callback for this field.
     */

    field_callback =
        create_field_callback(expected_tag_type,
                              value_callback);

    if (field_callback == NULL)
        return false;

    /*
     * Then try to allocate a new element in the list of field
     * callbacks.  If we can't, return an error code.
     */

    new_entry =
        hwm_buffer_append_list_elem(&field_map->entries,
                                    field_map_entry_t);

    if (new_entry == NULL)
        return false;

    /*
     * If the allocation worked, stash the number and callback into
     * the new entry.
     */

    new_entry->field_number = field_number;
    new_entry->callback = field_callback;

    return true;
}
