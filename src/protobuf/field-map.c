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


void
push_protobuf_field_map_set_success
(push_protobuf_field_map_t *field_map,
 push_success_continuation_t *success)
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
        push_continuation_call(&entries[i].callback->set_success,
                               success);
    }
}


void
push_protobuf_field_map_set_incomplete
(push_protobuf_field_map_t *field_map,
 push_incomplete_continuation_t *incomplete)
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
        push_continuation_call(&entries[i].callback->set_incomplete,
                               incomplete);
    }
}


void
push_protobuf_field_map_set_error
(push_protobuf_field_map_t *field_map,
 push_error_continuation_t *error)
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
        push_continuation_call(&entries[i].callback->set_error,
                               error);
    }
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
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * A pointer to the field tag we take in as input.
     */

    push_protobuf_tag_t  *actual_tag;

    /**
     * The tag type that we expect to receive.
     */

    push_protobuf_tag_type_t  expected_tag_type;

} verify_tag_t;


static void
verify_tag_activate(void *user_data,
                    void *result,
                    const void *buf,
                    size_t bytes_remaining)
{
    verify_tag_t  *verify_tag = (verify_tag_t *) user_data;

    verify_tag->actual_tag = (push_protobuf_tag_t *) result;

    PUSH_DEBUG_MSG("%s: Activating.  Got tag 0x%04"PRIx32"\n",
                   verify_tag->callback.name,
                   *verify_tag->actual_tag);

    PUSH_DEBUG_MSG("%s: Got tag type %d, expecting tag type %d.\n",
                   verify_tag->callback.name,
                   PUSH_PROTOBUF_GET_TAG_TYPE(*verify_tag->actual_tag),
                   verify_tag->expected_tag_type);

    if (PUSH_PROTOBUF_GET_TAG_TYPE(*verify_tag->actual_tag) ==
        verify_tag->expected_tag_type)
    {
        /*
         * Tag types match, so move on to parsing the value.
         */

        PUSH_DEBUG_MSG("%s: Tag types match.\n",
                       verify_tag->callback.name);

        push_continuation_call(verify_tag->callback.success,
                               NULL,
                               buf, bytes_remaining);

        return;
    } else {
        /*
         * Tag types don't match, we've got a parse error!
         */

        PUSH_DEBUG_MSG("%s: Tag types don't match.\n",
                       verify_tag->callback.name);

        push_continuation_call(verify_tag->callback.error,
                               PUSH_PARSE_ERROR,
                               "Tag types don't match");

        return;
    }

}


static push_callback_t *
verify_tag_new(const char *name,
               push_parser_t *parser,
               push_protobuf_tag_type_t expected_tag_type)
{
    verify_tag_t  *verify_tag =
        (verify_tag_t *) malloc(sizeof(verify_tag_t));

    if (verify_tag == NULL)
        return NULL;

    /*
     * Fill in the data items.
     */

    verify_tag->expected_tag_type = expected_tag_type;

    /*
     * Initialize the push_callback_t instance.
     */

    if (name == NULL)
        name = "verify_tag";

    push_callback_init(name,
                       &verify_tag->callback, parser, verify_tag,
                       verify_tag_activate,
                       NULL, NULL, NULL);

    return &verify_tag->callback;
}


/*-----------------------------------------------------------------------
 * Field callbacks
 */

static push_callback_t *
create_field_callback(const char *name,
                      push_parser_t *parser,
                      push_protobuf_tag_type_t expected_tag_type,
                      push_callback_t *value_callback)
{
    const char  *verify_tag_name;
    const char  *compose_name;

    push_callback_t  *verify_tag;
    push_callback_t  *compose;

    /*
     * First construct all of the names.
     */

    if (name == NULL)
        name = "both";

    verify_tag_name = push_string_concat(name, ".verify-tag");
    if (verify_tag_name == NULL) return NULL;

    compose_name = push_string_concat(name, ".tag-compose");
    if (compose_name == NULL) return NULL;

    /*
     * Then create the callbacks.
     */

    verify_tag = verify_tag_new(verify_tag_name,
                                parser, expected_tag_type);
    if (verify_tag == NULL)
    {
        return NULL;
    }

    compose = push_compose_new(compose_name,
                               parser, verify_tag, value_callback);
    if (compose == NULL)
    {
        push_callback_free(verify_tag);
        return NULL;
    }

    return compose;
}


bool
push_protobuf_field_map_add_field
(const char *name,
 push_parser_t *parser,
 push_protobuf_field_map_t *field_map,
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
        create_field_callback(name, parser, expected_tag_type,
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
