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
 *   - DEST_T
 *   - PROTOBUF_TYPE
 *   - TAG_TYPE
 */

#if 0
#define _PRETTY_STR(pretty) #pretty
#define PRETTY_STR _PRETTY_STR(PRETTY_NAME)

#define _STRUCT_NAME(pretty) _assign_##pretty
#define STRUCT_NAME _STRUCT_NAME(PRETTY_NAME)

#define _ASSIGN_T(pretty) assign_##pretty##_t
#define ASSIGN_T _ASSIGN_T(PRETTY_NAME)

#define _VALUE_CALLBACK_T(protobuf) push_protobuf_##protobuf##_t
#define VALUE_CALLBACK_T _VALUE_CALLBACK_T(PROTOBUF_TYPE)

#define _VALUE_CALLBACK_NEW(protobuf) push_protobuf_##protobuf##_new
#define VALUE_CALLBACK_NEW _VALUE_CALLBACK_NEW(PROTOBUF_TYPE)

#define _PUSH_PROTOBUF_ASSIGN(pretty) push_protobuf_assign_##pretty
#define PUSH_PROTOBUF_ASSIGN _PUSH_PROTOBUF_ASSIGN(PRETTY_NAME)
#endif

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
     * The callback that reads the value.
     */

    VALUE_CALLBACK_T  *value_callback;

    /**
     * The pointer to assign the value to.
     */

    DEST_T  *dest;

} ASSIGN_T;


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

    PUSH_DEBUG_MSG(PRETTY_STR": Assign value.\n");
    *(callback->dest) = callback->value_callback->value;

    /*
     * We don't actually parse anything; pass off to the next callback
     * and return.
     */

    push_parser_set_callback(parser, pcallback->next_callback);
    return bytes_available;
}


static void
assign_free(push_callback_t *pcallback)
{
    ASSIGN_T  *callback =
        (ASSIGN_T *) pcallback;

    PUSH_DEBUG_MSG(PRETTY_STR": Freeing assign callback %p...\n",
                   pcallback);
    push_callback_free(&callback->value_callback->base);
}


static ASSIGN_T *
assign_new(VALUE_CALLBACK_T *value_callback, DEST_T *dest)
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
                       0,
                       0,
                       NULL,
                       assign_process_bytes,
                       push_eof_not_allowed,
                       assign_free,
                       NULL);

    result->value_callback = value_callback;
    result->dest = dest;

    return result;
}


bool
PUSH_PROTOBUF_ASSIGN(push_protobuf_message_t *message,
                     push_protobuf_tag_number_t field_number,
                     DEST_T *dest)
{
    VALUE_CALLBACK_T  *value_callback = NULL;
    ASSIGN_T  *assign_callback = NULL;
    push_protobuf_field_t  *field_callback = NULL;

    /*
     * First, create the callbacks.
     */

    value_callback = VALUE_CALLBACK_NEW(NULL, false);
    if (value_callback == NULL)
        goto error;

    assign_callback = assign_new(NULL, dest);
    if (assign_callback == NULL)
        goto error;

    field_callback =
        push_protobuf_field_new(TAG_TYPE, &value_callback->base, false);
    if (field_callback == NULL)
        goto error;

    /*
     * Then link them up.
     */

    assign_callback->value_callback = value_callback;
    field_callback->base.next_callback = &assign_callback->base;
    assign_callback->base.next_callback =
        &message->tag_callback->base;

    /*
     * Try to add the new field.  If we can't, free the field before
     * returning.  (We don't have to explicitly free the other two
     * callbacks at this point, since the field_callback is linked to
     * them.)
     */

    if (!push_protobuf_message_add_field(message, field_number,
                                         field_callback))
    {
        push_callback_free(&field_callback->base);
        return false;
    }

    return true;

  error:
    /*
     * Before returning the NULL error code, free everything that we
     * might've created so far.
     */

    if (value_callback != NULL)
        push_callback_free(&value_callback->base);

    if (assign_callback != NULL)
        push_callback_free(&assign_callback->base);

    if (field_callback != NULL)
        push_callback_free(&field_callback->base);

    return NULL;
}
