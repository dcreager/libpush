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
#include <push/talloc.h>

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
     * The push_callback_t superclass for this callback.
     */

    push_callback_t  callback;

    /**
     * A mapping of field numbers to the callback that reads the
     * corresponding field.
     */

    push_protobuf_field_map_t  *field_map;

    /**
     * A callback that can skip unknown length-prefixed fields.
     */

    push_callback_t  *skip_length_prefixed;

} dispatch_t;


static void
dispatch_set_success(void *user_data,
                     push_success_continuation_t *success)
{
    dispatch_t  *dispatch = (dispatch_t *) user_data;

    push_protobuf_field_map_set_success(dispatch->field_map,
                                        success);

    push_continuation_call(&dispatch->skip_length_prefixed
                           ->set_success,
                           success);
}


static void
dispatch_set_incomplete(void *user_data,
                        push_incomplete_continuation_t *incomplete)
{
    dispatch_t  *dispatch = (dispatch_t *) user_data;

    push_protobuf_field_map_set_incomplete(dispatch->field_map,
                                           incomplete);

    push_continuation_call(&dispatch->skip_length_prefixed
                           ->set_incomplete,
                           incomplete);
}


static void
dispatch_set_error(void *user_data,
                   push_error_continuation_t *error)
{
    dispatch_t  *dispatch = (dispatch_t *) user_data;

    dispatch->callback.error = error;

    push_protobuf_field_map_set_error(dispatch->field_map,
                                      error);

    push_continuation_call(&dispatch->skip_length_prefixed
                           ->set_error,
                           error);
}


static void
dispatch_activate(void *user_data,
                  void *result,
                  void *buf,
                  size_t bytes_remaining)
{
    dispatch_t  *dispatch = (dispatch_t *) user_data;
    push_protobuf_tag_t  *field_tag;
    push_protobuf_tag_number_t  field_number;
    push_callback_t  *field_callback;

    field_tag = (push_protobuf_tag_t *) result;
    PUSH_DEBUG_MSG("%s: Activating.  Got tag 0x%04"PRIx32"\n",
                   push_talloc_get_name(dispatch),
                   *field_tag);

    /*
     * Extract the field number from the tag_callback.
     */

    field_number = PUSH_PROTOBUF_GET_TAG_NUMBER(*field_tag);

    PUSH_DEBUG_MSG("%s: Dispatching field %"PRIu32".\n",
                   push_talloc_get_name(dispatch),
                   field_number);

    /*
     * Get the field callback for this field number.
     */

    field_callback =
        push_protobuf_field_map_get_field(dispatch->field_map,
                                          field_number);

    if (field_callback == NULL)
    {
        push_protobuf_tag_type_t  field_type =
            PUSH_PROTOBUF_GET_TAG_TYPE(*field_tag);

        switch (field_type)
        {
          case PUSH_PROTOBUF_TAG_TYPE_LENGTH_DELIMITED:
            field_callback = dispatch->skip_length_prefixed;
            break;

          default:
            /*
             * TODO: Add skippers for the other field types.
             */

            PUSH_DEBUG_MSG("%s: No field callback for "
                           "field %"PRIu32".\n",
                           push_talloc_get_name(dispatch),
                           field_number);

            push_continuation_call(dispatch->callback.error,
                                   PUSH_PARSE_ERROR,
                                   "No callback for field");

            return;
        }
    }

    /*
     * Found it!  Activate that callback we just found.  The field
     * callback is going to need to verify the wire type, so make sure
     * to pass in the tag as input.
     */

    PUSH_DEBUG_MSG("%s: Callback %p matches.\n",
                   push_talloc_get_name(dispatch),
                   field_callback);

    push_continuation_call(&field_callback->activate,
                           field_tag,
                           buf, bytes_remaining);

    return;

}


static push_callback_t *
dispatch_new(const char *name,
             void *parent,
             push_parser_t *parser,
             push_protobuf_field_map_t *field_map)
{
    void  *context;
    dispatch_t  *dispatch = NULL;
    push_callback_t  *skip_length_prefixed = NULL;

    /*
     * If the field map is NULL, return NULL ourselves.
     */

    if (field_map == NULL)
        return NULL;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Next, allocate the dispatch callback itself.
     */

    dispatch = push_talloc(context, dispatch_t);
    if (dispatch == NULL) goto error;

    push_talloc_set_name_const(dispatch, name);

    /*
     * Then try to create the skipper callbacks.
     */

    skip_length_prefixed =
        push_protobuf_skip_length_prefixed_new
        (push_talloc_asprintf(context, "%s.skip-length-prefixed", name),
         context, parser);
    if (skip_length_prefixed == NULL) goto error;

    /*
     * Make the field map a child of the dispatch callback.
     */

    push_talloc_steal(dispatch, field_map);

    /*
     * Fill in the data items.
     */

    dispatch->field_map = field_map;
    dispatch->skip_length_prefixed = skip_length_prefixed;

    /*
     * Initialize the push_callback_t instance.
     */

    push_callback_init(&dispatch->callback, parser, dispatch,
                       dispatch_activate,
                       dispatch_set_success,
                       dispatch_set_incomplete,
                       dispatch_set_error);

    return &dispatch->callback;

  error:
    /*
     * Before returning, free any objects we created before the error.
     */

    push_talloc_free(context);
    return NULL;
}


/*-----------------------------------------------------------------------
 * Top-level message callback
 */


push_callback_t *
push_protobuf_message_new(const char *name,
                          void *parent,
                          push_parser_t *parser,
                          push_protobuf_field_map_t *field_map)
{
    void  *context;
    push_callback_t  *read_field_tag;
    push_callback_t  *dispatch;
    push_callback_t  *compose;
    push_callback_t  *fold;

    /*
     * If the field map is NULL, return NULL ourselves.
     */

    if (field_map == NULL)
        return NULL;

    /*
     * Create a memory context for the objects we're about to create.
     */

    context = push_talloc_new(parent);
    if (context == NULL) return NULL;

    /*
     * Create the callbacks.
     */

    if (name == NULL) name = "message";

    read_field_tag = push_protobuf_read_varint32_new
        (push_talloc_asprintf(context, "%s.tag", name),
         context, parser);
    dispatch = dispatch_new
        (push_talloc_asprintf(context, "%s.dispatch", name),
         context, parser, field_map);
    compose = push_compose_new
        (push_talloc_asprintf(context, "%s.compose", name),
         context, parser,
         read_field_tag, dispatch);
    fold = push_fold_new
        (push_talloc_asprintf(context, "%s.fold", name),
         context, parser, compose);

    /*
     * Because of NULL propagation, we only have to check the last
     * result to see if everything was created okay.
     */

    if (fold == NULL) goto error;
    return fold;

  error:
    /*
     * Before returning, free any objects we created before the error.
     */

    push_talloc_free(context);
    return NULL;
}
