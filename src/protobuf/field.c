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

#include <push.h>
#include <push/protobuf.h>


/*-----------------------------------------------------------------------
 * Verify tag callback
 */

/**
 * The callback class for the field's verify_tag_callback.
 */

typedef struct _verify_tag
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * A link back to the field callback.
     */

    push_protobuf_field_t  *field_callback;
} verify_tag_t;


static ssize_t
verify_tag_process_bytes(push_parser_t *parser,
                         push_callback_t *pcallback,
                         const void *vbuf,
                         size_t bytes_available)
{
    verify_tag_t  *callback =
        (verify_tag_t *) pcallback;

    push_protobuf_tag_t  actual_tag;
    push_protobuf_tag_type_t  expected_tag_type;

    PUSH_DEBUG_MSG("field: Verifying tag type.\n");

    actual_tag = callback->field_callback->tag_callback->value;
    expected_tag_type = callback->field_callback->expected_tag_type;

    PUSH_DEBUG_MSG("field:   Got tag type %d, expecting tag type %d.\n",
                   PUSH_PROTOBUF_GET_TAG_TYPE(actual_tag),
                   expected_tag_type);

    if (PUSH_PROTOBUF_GET_TAG_TYPE(actual_tag) == expected_tag_type)
    {
        /*
         * Tag types match, so move on to parsing the value.
         */

        PUSH_DEBUG_MSG("field:   Tag types match, switching to "
                       "value callback (%p).\n",
                       callback->field_callback->value_callback);

        push_parser_set_callback(parser,
                                 callback->field_callback->value_callback);
        return bytes_available;
    } else {
        /*
         * Tag types don't match, we've got a parse error!
         */

        return PUSH_PARSE_ERROR;
    }
}


static void
verify_tag_free(push_callback_t *pcallback)
{
    verify_tag_t  *callback =
        (verify_tag_t *) pcallback;

    PUSH_DEBUG_MSG("field: Freeing verify_tag callback %p...\n", pcallback);
    push_callback_free(&callback->field_callback->base);
}


static verify_tag_t *
verify_tag_new(push_protobuf_field_t *field_callback)
{
    verify_tag_t  *result =
        (verify_tag_t *) malloc(sizeof(verify_tag_t));

    if (result == NULL)
        return NULL;

    /*
     * EOF is not allowed in place of verify_tag — that's in between
     * the tag and the value, which is a parse error.
     */

    push_callback_init(&result->base,
                       0,
                       0,
                       verify_tag_process_bytes,
                       push_eof_not_allowed,
                       verify_tag_free,
                       NULL);

    result->field_callback = field_callback;

    return result;
}


/*-----------------------------------------------------------------------
 * “Finish” tag callback
 */

/**
 * This callback finishes the processing of a field.  There's not
 * actually anything interesting to do, we just have to pass control
 * to the “next_callback”.  We can't link the value_callback to
 * next_callback directly, since next_callback might not be set when
 * the constructor is called.
 */

typedef struct _finish
{
    /**
     * The callback's “superclass” instance.
     */

    push_callback_t  base;

    /**
     * A link back to the field callback.
     */

    push_protobuf_field_t  *field_callback;
} finish_t;


static ssize_t
finish_process_bytes(push_parser_t *parser,
                     push_callback_t *pcallback,
                     const void *vbuf,
                     size_t bytes_available)
{
    finish_t  *callback =
        (finish_t *) pcallback;

    /*
     * The finish doesn't actually do anything, we just pass off
     * to the field's next_callback.
     */

    PUSH_DEBUG_MSG("field: Finishing; switching to next callback.\n");

    push_parser_set_callback(parser,
                             callback->field_callback->
                             base.next_callback);
    return bytes_available;
}


static void
finish_free(push_callback_t *pcallback)
{
    finish_t  *callback =
        (finish_t *) pcallback;

    PUSH_DEBUG_MSG("field: Freeing finish callback %p...\n", pcallback);
    push_callback_free(&callback->field_callback->base);
}


static finish_t *
finish_new(push_protobuf_field_t *field_callback)
{
    finish_t  *result =
        (finish_t *) malloc(sizeof(finish_t));

    if (result == NULL)
        return NULL;

    /*
     * EOF isn't allowed in place of finish.
     */

    push_callback_init(&result->base,
                       0,
                       0,
                       finish_process_bytes,
                       push_eof_not_allowed,
                       finish_free,
                       NULL);

    result->field_callback = field_callback;

    return result;
}


/*-----------------------------------------------------------------------
 * Top-level field callback
 */

static ssize_t
field_process_bytes(push_parser_t *parser,
                    push_callback_t *pcallback,
                    const void *vbuf,
                    size_t bytes_available)
{
    push_protobuf_field_t  *callback =
        (push_protobuf_field_t *) pcallback;

    /*
     * The field_callback doesn't actually do anything, we just thread
     * through our child callbacks.  Pass off right away into the
     * tag_callback.
     */

    PUSH_DEBUG_MSG("field: Switching to tag callback.\n");

    push_parser_set_callback(parser, &callback->tag_callback->base);
    return bytes_available;
}


static void
field_free(push_callback_t *pcallback)
{
    push_protobuf_field_t  *callback =
        (push_protobuf_field_t *) pcallback;

    PUSH_DEBUG_MSG("field: Freeing field callback %p...\n", pcallback);
    push_callback_free(&callback->tag_callback->base);
    push_callback_free(callback->verify_tag_callback);
    push_callback_free(callback->value_callback);
    push_callback_free(callback->finish_callback);
}


push_protobuf_field_t *
push_protobuf_field_new(push_protobuf_tag_type_t expected_tag_type,
                        push_callback_t *value_callback,
                        push_callback_t *next_callback,
                        bool eof_allowed)
{
    push_protobuf_field_t  *result =
        (push_protobuf_field_t *) malloc(sizeof(push_protobuf_field_t));

    if (result == NULL)
    {
        PUSH_DEBUG_MSG("Couldn't allocate field callback.\n");
        goto error;
    }

    push_callback_init(&result->base,
                       0,
                       0,
                       field_process_bytes,
                       eof_allowed?
                           push_eof_allowed:
                           push_eof_not_allowed,
                       field_free,
                       NULL);

    result->expected_tag_type = expected_tag_type;

    /*
     * Try to create that callback for reading the field's tag.
     */

    result->tag_callback =
        push_protobuf_varint32_new(NULL, eof_allowed);
    if (result->tag_callback == NULL)
    {
        PUSH_DEBUG_MSG("Couldn't allocate tag callback.\n");
        goto error;
    }

    /*
     * ...and then the callback for verifying the tag.
     */

    {
        verify_tag_t  *verify_tag_callback;

        verify_tag_callback = verify_tag_new(result);
        if (verify_tag_callback == NULL)
        {
            PUSH_DEBUG_MSG("Couldn't allocate tag verification callback.\n");
            goto error;
        }

        result->verify_tag_callback = &verify_tag_callback->base;
    }

    /*
     * Change the value callback's EOF function, since we don't want
     * to allow an EOF in between the tag and the value.
     */

    result->value_callback = value_callback;
    value_callback->eof = push_eof_not_allowed;

    /*
     * Create the finish callback.
     */

    {
        finish_t  *finish_callback;

        finish_callback = finish_new(result);
        if (finish_callback == NULL)
        {
            PUSH_DEBUG_MSG("Couldn't allocate finish callback.\n");
            goto error;
        }

        result->finish_callback = &finish_callback->base;
    }

    /*
     * Finally, link all of the callbacks together.
     */

    result->tag_callback->base.next_callback =
        result->verify_tag_callback;

    result->verify_tag_callback->next_callback =
        value_callback;

    value_callback->next_callback =
        result->finish_callback;

    return result;

  error:
    /*
     * Before returning the NULL error code, free everything that we
     * might've created so far.
     */

    if (result != NULL)
    {
        push_callback_free(&result->base);
        free(result);
    }

    return NULL;
}
